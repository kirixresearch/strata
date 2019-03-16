/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#include <xd/xd.h>
#include "database.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/localrowcache.h"
#include "../xdcommon/util.h"
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/md5.h>
#include <kl/math.h>

#define XLNT_STATIC
#include <xlnt/xlnt.hpp>


const int row_array_size = 1000;

const std::string empty_string = "";
const std::wstring empty_wstring = L"";


ExcelIterator::ExcelIterator(ExcelDatabase* database)
{
    m_database = database;
    m_database->ref();

    m_cur_block = 0;
    m_row_pos = 0;
    m_data = NULL;
    m_data_len = 0;
    m_row_number = 0;
    m_row_count = 0;
    m_row = NULL;
    m_eof = false;

    m_ws = new xlnt::worksheet();
}

ExcelIterator::~ExcelIterator()
{
    delete m_ws;

    if (m_database)
        m_database->unref();
}

bool ExcelIterator::init(const std::wstring& path)
{
    m_path = path;
    if (m_path.substr(0,1) == L"/")
        m_path = m_path.substr(1);

    try
    {
        *m_ws = m_database->m_wb->sheet_by_title((const char*)kl::toUtf8(m_path));
    }
    catch(...)
    {
        return false;
    }


    m_row_count = m_ws->highest_row_or_props();

    refreshStructure();

    return true;
}


void ExcelIterator::setTable(const std::wstring& tbl)
{
    m_path = tbl;
}


std::wstring ExcelIterator::getTable()
{
    return m_path;
}

xd::rowpos_t ExcelIterator::getRowCount()
{
    return m_row_count;
}

xd::IDatabasePtr ExcelIterator::getDatabase()
{
    return static_cast<xd::IDatabase*>(m_database);
}

xd::IIteratorPtr ExcelIterator::clone()
{
    ExcelIterator* iter = new ExcelIterator(m_database);
    if (!iter->init(m_path))
    {
        delete iter;
        return xcm::null;
    }


    // ...

    return static_cast<xd::IIterator*>(iter);
}


void ExcelIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
}
    
    
unsigned int ExcelIterator::getIteratorFlags()
{
    return xd::ifFastRowCount;
}

void ExcelIterator::skip(int delta)
{
    m_row_number += delta;
}


void ExcelIterator::goFirst()
{
    m_row_number = 1;
}

void ExcelIterator::goLast()
{
}

xd::rowid_t ExcelIterator::getRowId()
{
    return m_row_pos;
}

bool ExcelIterator::bof()
{
    return m_row_number < 1 ? true : false;
}

bool ExcelIterator::eof()
{
    return m_row_number > m_row_count ? true : false;
}

bool ExcelIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool ExcelIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool ExcelIterator::setPos(double pct)
{
    return false;
}

void ExcelIterator::goRow(const xd::rowid_t& rowid)
{
}

double ExcelIterator::getPos()
{
    return (double)(long long)m_row_pos;
}

xd::Structure ExcelIterator::getStructure()
{
    return m_structure;
}

bool ExcelIterator::refreshStructure()
{
    m_structure.clear();

    xlnt::column_t coln = m_ws->highest_column();
    size_t i;

    for (i = 1; i <= coln.index; ++i)
    {
        xd::ColumnInfo colinfo;
        colinfo.name = kl::towstring(xlnt::column_t::column_string_from_index(i));
        colinfo.type = xd::typeWideCharacter;
        colinfo.width = 255;
        colinfo.scale = 0;
        colinfo.calculated = false;
        colinfo.expression = L"";

        m_structure.push_back(colinfo);
    }


    size_t col_count = m_structure.getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& colinfo = m_structure.getColumnInfoByIdx(i);

        ExcelDataAccessInfo* field = new ExcelDataAccessInfo;
        field->name = colinfo.name;
        field->type = colinfo.type;
        field->width = colinfo.width;
        field->scale = colinfo.scale;
        field->ordinal = (int)i;
        field->col_number = i+1;

        m_fields.push_back(field);
    }


    return true;
}

bool ExcelIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    return false;
}



xd::objhandle_t ExcelIterator::getHandle(const std::wstring& expr)
{
    std::vector<ExcelDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, expr))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        ExcelDataAccessInfo* dai = new ExcelDataAccessInfo;
        dai->expr = NULL;
        dai->expr_text = expr;
        dai->type = xd::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<xd::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }
        
        m_exprs.push_back(dai);
        return (xd::objhandle_t)dai;
    }
    
    
    kscript::ExprParser* parser = parse(expr);
    if (!parser)
    {
        return (xd::objhandle_t)0;
    }

    ExcelDataAccessInfo* dai = new ExcelDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool ExcelIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<ExcelDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

xd::ColumnInfo ExcelIterator::getInfo(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::ColumnInfo();

    return m_structure.getColumnInfo(dai->name);
}

int ExcelIterator::getType(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;
    
    return dai->type;
}

int ExcelIterator::getRawWidth(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* ExcelIterator::getRawPtr(xd::objhandle_t data_handle)
{


    return NULL;
}


const std::string& ExcelIterator::getString(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_string;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->str_result = kl::tostring(dai->expr_result.getString());
        return dai->str_result;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return empty_string;
    }

    if (eof())
    {
        dai->str_result = "";
        return dai->str_result;
    }

    dai->str_result = m_ws->cell(dai->col_number, m_row_number).to_string();
    return dai->str_result;
}

const std::wstring& ExcelIterator::getWideString(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->wstr_result = dai->expr_result.getString();
        return dai->wstr_result;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return empty_wstring;
    }


    if (eof())
    {
        dai->wstr_result = L"";
        return dai->wstr_result;
    }

    dai->wstr_result = kl::fromUtf8((const char*)m_ws->cell(dai->col_number, m_row_number).to_string().c_str());

    return dai->wstr_result;
}

xd::datetime_t ExcelIterator::getDateTime(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        kscript::ExprDateTime edt = dai->expr_result.getDateTime();

        xd::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        if (dai->type == xd::typeDateTime)
            dt |= edt.time;

        return dt;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    if (eof())
        return 0;

    /*
    if (dai->type == xd::typeDate)
    {
        xd::datetime_t dt;
        dt = *(unsigned int*)(m_row+dai->offset);
        dt <<= 32;
        return dt;
    }
     else if (dai->type == xd::typeDateTime)
    {
        return *(xd::datetime_t*)(m_row+dai->offset);
    }
     else
    {
        return 0;
    }

    return (xd::datetime_t)(m_row+dai->offset);
    */

    return 0;
}

static double decstr2dbl(const char* c, int width, int scale)
{
    double res = 0;
    double d = kl::pow10(width-scale-1);
    bool neg = false;
    while (width)
    {
        if (*c == '-')
            neg = true;

        if (*c >= '0' && *c <= '9')
        {
            res += d * (*c - '0');
        }

        d /= 10;
        c++;
        width--;
    }

    if (neg)
    {
        res *= -1.0;
    }

    return res;
}


double ExcelIterator::getDouble(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0.0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getDouble();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0.0;
    }

    if (eof())
        return 0.0;

    /*
    const unsigned char* col_data = m_row+dai->offset;

    switch (dai->type)
    {
        case xd::typeDouble:
            return *(double*)col_data;    
        case xd::typeInteger:
            return *(int*)col_data;
        case xd::typeNumeric:
            return kl::dblround(decstr2dbl((char*)col_data, dai->width, dai->scale),
                                dai->scale);
        default:
            return 0.0;
    } */

    return 0.0;
}

int ExcelIterator::getInteger(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getInteger();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    if (eof())
        return 0;

    /*
    const unsigned char* col_data = m_row+dai->offset;

    switch (dai->type)
    {
        case xd::typeDouble:
            return (int)(*(double*)col_data);    
        case xd::typeInteger:
            return *(int*)col_data;
        case xd::typeNumeric:
            return (int)(kl::dblround(decstr2dbl((char*)col_data, dai->width, dai->scale),
                                      dai->scale));
        default:
            return 0;
    }
    */

    return 0;
}

bool ExcelIterator::getBoolean(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getBoolean();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return false;
    }

    if (eof())
        return false;

    /*
    const unsigned char* col_data = m_row+dai->offset;

    return (*col_data == 'T' ? true : false);
    */

    return false;
}

bool ExcelIterator::isNull(xd::objhandle_t data_handle)
{
    ExcelDataAccessInfo* dai = (ExcelDataAccessInfo*)data_handle;
    if (dai == NULL)
        return true;

    if (dai->expr)
        return false;

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return true;
    }

    if (eof())
        return true;

    return false;
}
