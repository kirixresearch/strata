/*!
 *
 * Copyright (c) 2014, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2014-05-22
 *
 */


#include "xdfs.h"
#include "database.h"
#include "xbaseset.h"
#include "xbaseiterator.h"
#include "../xdcommon/util.h"
#include <kl/portable.h>


const std::string empty_string = "";
const std::wstring empty_wstring = L"";



XbaseIterator::XbaseIterator(FsDatabase* database)
{
    m_database = database;
    m_database->ref();

    m_current_row = 1;
    m_bof = false;
    m_eof = false;
    m_set = NULL;
}

XbaseIterator::~XbaseIterator()
{
    std::vector<XbaseDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete (*it);
    }

    if (m_file.isOpen())
    {
        m_file.close();
    }

    if (m_set)
        m_set->unref();

    m_database->unref();
}

bool XbaseIterator::init(XbaseSet* set, const std::wstring& filename)
{
    if (!m_file.open(filename))
        return false;

    m_set = set;
    m_set->ref();

    m_current_row = 1;

    refreshStructure();

    return true;
}

void XbaseIterator::setTable(const std::wstring& tbl)
{
}

std::wstring XbaseIterator::getTable()
{
    if (!m_set)
        return L"";
    return m_set->getObjectPath();
}

xd::rowpos_t XbaseIterator::getRowCount()
{
    return m_file.getRowCount();
}

xd::IDatabasePtr XbaseIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr XbaseIterator::clone()
{
    XbaseIterator* iter = new XbaseIterator(m_database);
    
    if (!iter->init(m_set, m_set->m_filename))
        return xcm::null;

    iter->goRow(m_current_row);
    
    return static_cast<xd::IIterator*>(iter);
}

unsigned int XbaseIterator::getIteratorFlags()
{
    return (xd::ifFastRowCount | xd::ifFastSkip);
}

void XbaseIterator::skip(int delta)
{
    m_current_row += delta;

    if (m_current_row <= 0)
    {
        m_current_row = 1;
        m_bof = true;
        return;
    }

    if ((unsigned int)m_current_row > m_file.getRowCount())
    {
        m_current_row = m_file.getRowCount();
        m_eof = true;
        return;
    }

    goRow(m_current_row);
}

void XbaseIterator::goFirst()
{
    m_bof = false;
    m_eof = false;
    m_current_row = 1;
    goRow(m_current_row);
}

void XbaseIterator::goLast()
{
    m_bof = false;
    m_eof = false;
    m_current_row = m_file.getRowCount();
    goRow(m_current_row);
}

xd::rowid_t XbaseIterator::getRowId()
{
    return m_current_row;
}

bool XbaseIterator::bof()
{
    return m_bof;
}

bool XbaseIterator::eof()
{
    return m_eof;
}

bool XbaseIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool XbaseIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool XbaseIterator::setPos(double pct)
{
    return false;
}

double XbaseIterator::getPos()
{
    return 0.0;
}

void XbaseIterator::goRow(const xd::rowid_t& rowid)
{
    m_current_row = (unsigned int)rowid;
    m_file.goRow(m_current_row);
}

xd::Structure XbaseIterator::getStructure()
{
    xd::Structure s;

    std::vector<XbaseDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        xd::ColumnInfo col;

        col.name = (*it)->name;
        col.type = (*it)->type;
        col.width = (*it)->width;
        col.scale = (*it)->scale;
        col.column_ordinal = (*it)->ordinal;
        col.expression = (*it)->expr_text;
        
        if ((*it)->xbase_type == 'Y')    // currency
            col.width = 18;
        if (col.type == xd::typeDouble)
            col.width = 8;
        if (col.type == xd::typeNumeric && col.width > xd::max_numeric_width)
            col.width = xd::max_numeric_width;
        if (col.expression.length() > 0)
            col.calculated = true;

        s.createColumn(col);
    }

    return s;
}

bool XbaseIterator::refreshStructure()
{
    // clear out any existing structure
    std::vector<XbaseDataAccessInfo*>::iterator field_it;
    for (field_it = m_fields.begin();
         field_it != m_fields.end(); ++field_it)
    {
        delete (*field_it);
    }
    m_fields.clear();


    xd::Structure s = m_set->getStructure();

    size_t i, col_count = s.getColumnCount();

    // get structure from table
    std::vector<XbaseField> fields = m_file.getFields();
    
    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& colinfo = s.getColumnInfoByIdx(i);
        
        XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
        dai->xbase_type = xd2xbaseType(colinfo.type);
        dai->name = colinfo.name;
        dai->type = colinfo.type;
        dai->width = colinfo.width;
        dai->scale = colinfo.scale;
        dai->ordinal = colinfo.column_ordinal;
        dai->expr_text = colinfo.expression;
        m_fields.push_back(dai);
        
        // look up the precise source type
        std::vector<XbaseField>::iterator it;
        for (it = fields.begin(); it != fields.end(); ++it)
        {
            std::wstring xbase_name = kl::towstring(it->name);
            if (kl::iequals(xbase_name, dai->name))
            {
                dai->xbase_type = it->type;
                break;
            }
        }
        
        // parse any expression, if necessary
        if (dai->expr_text.length() > 0)
            dai->expr = parse(dai->expr_text);
    }

    return true;
}

bool XbaseIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<XbaseDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                XbaseDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionModify)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                if (it->params.mask & xd::ColumnInfo::maskName)
                {
                    std::wstring new_name = it->params.name;
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->params.mask & xd::ColumnInfo::maskType)
                {
                    (*it2)->type = it->params.type;
                }

                if (it->params.mask & xd::ColumnInfo::maskWidth)
                {
                    (*it2)->width = it->params.width;
                }

                if (it->params.mask & xd::ColumnInfo::maskScale)
                {
                    (*it2)->scale = it->params.scale;
                }

                if (it->params.mask & xd::ColumnInfo::maskExpression)
                {
                     if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->params.expression;
                    (*it2)->expr = parse(it->params.expression);
                }
            }
        }
    }

    // handle create
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionCreate)
            continue;

        if (it->params.expression.length() > 0)
        {
            XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
            dai->name = it->params.name;
            dai->type = it->params.type;
            dai->width = it->params.width;
            dai->scale = it->params.scale;
            dai->ordinal = m_fields.size();
            dai->expr_text = it->params.expression;
            dai->expr = parse(it->params.expression);
            m_fields.push_back(dai);
        }
    }

    // handle insert
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->params.column_ordinal;
        if (insert_idx < 0 || (size_t)insert_idx >= m_fields.size())
            continue;
        
        if (it->params.expression.length() > 0)
        {
            XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
            dai->name = it->params.name;
            dai->type = it->params.type;
            dai->width = it->params.width;
            dai->scale = it->params.scale;
            dai->ordinal = m_fields.size();
            dai->expr_text = it->params.expression;
            dai->expr = parse(it->params.expression);
            m_fields.insert(m_fields.begin()+insert_idx, dai);
        }
    }
    
    return true;
}


xd::objhandle_t XbaseIterator::getHandle(const std::wstring& expr)
{
    std::vector<XbaseDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, expr))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
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

    XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool XbaseIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<XbaseDataAccessInfo*>::iterator it;
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

xd::ColumnInfo XbaseIterator::getInfo(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::ColumnInfo();

    xd::ColumnInfo colinfo;
    colinfo.name = dai->name;
    colinfo.type = dai->type;
    colinfo.width = dai->width;
    colinfo.scale = dai->scale;
    colinfo.expression = dai->expr_text;
    
    if (dai->type == xd::typeDate || dai->type == xd::typeInteger)
    {
        colinfo.width = 4;
    }
     else if (dai->type == xd::typeDateTime || dai->type == xd::typeDouble)
    {
        colinfo.width = 8;
    }
     else if (dai->type == xd::typeBoolean)
    {
        colinfo.width = 1;
    }
     else
    {
        colinfo.width = dai->width;
    }
    
    if (dai->expr_text.length() > 0)
        colinfo.calculated = true;

    return colinfo;
}

int XbaseIterator::getType(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;

    return dai->type;
}

int XbaseIterator::getRawWidth(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* XbaseIterator::getRawPtr(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0;

    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return m_file.getRaw(dai->ordinal);
}

const std::string& XbaseIterator::getString(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
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

    dai->str_result = m_file.getString(dai->ordinal);
    return dai->str_result;
}

const std::wstring& XbaseIterator::getWideString(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

    if (dai->type == xd::typeCharacter)
    {
        dai->wstr_result = kl::towstring(getString(data_handle));
        return dai->wstr_result;
    }
     else if (dai->type == xd::typeWideCharacter)
    {
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

        dai->wstr_result = kl::towstring(m_file.getString(dai->ordinal));
        return dai->wstr_result;
    }

    return empty_wstring;
}

xd::datetime_t XbaseIterator::getDateTime(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        xd::DateTime dt;
        return dt;
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

    XbaseDate xbase_date = m_file.getDateTime(dai->ordinal);
    if (xbase_date.isNull())
        return 0;
    
    xd::DateTime dt(xbase_date.year,
                       xbase_date.month,
                       xbase_date.day,
                       xbase_date.hour,
                       xbase_date.minute,
                       xbase_date.second,
                       xbase_date.millisecond);
    return dt;
}

double XbaseIterator::getDouble(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
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

    return m_file.getDouble(dai->ordinal);
}

int XbaseIterator::getInteger(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
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

    return m_file.getInteger(dai->ordinal);
}

bool XbaseIterator::getBoolean(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
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

    return m_file.getBoolean(dai->ordinal);
}

bool XbaseIterator::isNull(xd::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.isNull();
    }
    
    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return true;
    }
            
    return m_file.isNull(dai->ordinal);
}





