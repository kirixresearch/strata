/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "xdfs.h"
#include "database.h"
#include "xlsxset.h"
#include "xlsxiterator.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/util.h"


const std::string empty_string = "";
const std::wstring empty_wstring = L"";



XlsxIterator::XlsxIterator(FsDatabase* database)
{
    m_database = database;
    m_database->ref();

    m_current_row = 1;
    m_bof = false;
    m_eof = false;
    m_set = NULL;
    m_file = NULL;
}

XlsxIterator::~XlsxIterator()
{
    std::vector<XlsxDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete (*it);
    }

    if (m_set)
        m_set->unref();

    m_database->unref();
}

bool XlsxIterator::init(XlsxSet* set)
{
    m_set = set;
    m_set->ref();

    m_file = &m_set->m_file;

    m_current_row = 1;
    refreshStructure();

    return true;
}

void XlsxIterator::setTable(const std::wstring& tbl)
{
}

std::wstring XlsxIterator::getTable()
{
    if (!m_set)
        return L"";
    return m_set->getObjectPath();
}

xd::rowpos_t XlsxIterator::getRowCount()
{
    return m_file->getRowCount();
}

xd::IDatabasePtr XlsxIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr XlsxIterator::clone()
{
    XlsxIterator* iter = new XlsxIterator(m_database);
    
    if (!iter->init(m_set))
    {
        return xcm::null;
    }
    
    iter->goRow(m_current_row);
    
    return static_cast<xd::IIterator*>(iter);
}

unsigned int XlsxIterator::getIteratorFlags()
{
    return (xd::ifFastRowCount | xd::ifFastSkip);
}

void XlsxIterator::skip(int delta)
{
    m_current_row += delta;

    if (m_current_row <= 0)
    {
        m_current_row = 1;
        m_bof = true;
        return;
    }

    if ((unsigned int)m_current_row > m_file->getRowCount())
    {
        m_current_row = m_file->getRowCount();
        m_eof = true;
        return;
    }

    goRow(m_current_row);
}

void XlsxIterator::goFirst()
{
    m_bof = false;
    m_eof = false;
    m_current_row = 1;
    goRow(m_current_row);
}

void XlsxIterator::goLast()
{
    m_bof = false;
    m_eof = false;
    m_current_row = m_file->getRowCount();
    goRow(m_current_row);
}

xd::rowid_t XlsxIterator::getRowId()
{
    return m_current_row;
}

bool XlsxIterator::bof()
{
    return m_bof;
}

bool XlsxIterator::eof()
{
    return m_eof;
}

bool XlsxIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool XlsxIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool XlsxIterator::setPos(double pct)
{
    return false;
}

double XlsxIterator::getPos()
{
    return 0.0;
}

void XlsxIterator::goRow(const xd::rowid_t& rowid)
{
    m_current_row = (unsigned int)rowid;
    m_file->goRow(m_current_row);
}

xd::Structure XlsxIterator::getStructure()
{
    xd::Structure s;

    std::vector<XlsxDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        xd::ColumnInfo col;

        col.name = (*it)->name;
        col.type = (*it)->type;
        col.width = (*it)->width;
        col.scale = (*it)->scale;
        col.column_ordinal = (*it)->ordinal;
        col.expression = (*it)->expr_text;
        
        if ((*it)->xlsx_type == 'Y')    // currency
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


static std::wstring getSpreadsheetColumnName(int idx)
{
    std::wstring res;
    int n;

    while (true)
    {
        n = idx % 26;
        res.insert(res.begin(), ('A' + n));
        idx -= n;
        idx /= 26;
        if (idx == 0)
            break;
    }

    return res;
}


bool XlsxIterator::refreshStructure()
{
    // clear out any existing structure
    std::vector<XlsxDataAccessInfo*>::iterator field_it;
    for (field_it = m_fields.begin();
         field_it != m_fields.end(); ++field_it)
    {
        delete (*field_it);
    }
    m_fields.clear();


    size_t i, col_count = m_file->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        XlsxDataAccessInfo* dai = new XlsxDataAccessInfo;
        dai->name = getSpreadsheetColumnName((int)i);
        dai->type = xd::typeWideCharacter;
        dai->width = 255;
        dai->scale = 0;
        dai->ordinal = (int)i;
        dai->expr_text = L"";
        m_fields.push_back(dai);
        
        // parse any expression, if necessary
        if (dai->expr_text.length() > 0)
            dai->expr = parse(dai->expr_text);
    }

    return true;
}

bool XlsxIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<XlsxDataAccessInfo*>::iterator it2;
    
    // -- handle delete --
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
                XlsxDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // -- handle modify --
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionModify)
            continue;

        for (it2 = m_fields.begin();  it2 != m_fields.end(); ++it2)
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

    // -- handle create --
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionCreate)
            continue;

        if (it->params.expression.length() > 0)
        {
            XlsxDataAccessInfo* dai = new XlsxDataAccessInfo;
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

    // -- handle insert --
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
            XlsxDataAccessInfo* dai = new XlsxDataAccessInfo;
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


xd::objhandle_t XlsxIterator::getHandle(const std::wstring& expr)
{
    std::vector<XlsxDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        XlsxDataAccessInfo* dai = new XlsxDataAccessInfo;
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

    XlsxDataAccessInfo* dai = new XlsxDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool XlsxIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<XlsxDataAccessInfo*>::iterator it;
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

xd::ColumnInfo XlsxIterator::getInfo(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::ColumnInfo();

    xd::ColumnInfo colinfo;
    colinfo.name = dai->name;
    colinfo.type = dai->type;
    colinfo.width = dai->width;
    colinfo.scale = dai->scale;
    colinfo.expression = dai->expr_text;
    
    if (dai->type == xd::typeDate ||
        dai->type == xd::typeInteger)
    {
        colinfo.width = 4;
    }
     else if (dai->type == xd::typeDateTime ||
              dai->type == xd::typeDouble)
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

int XlsxIterator::getType(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;

    return dai->type;
}

int XlsxIterator::getRawWidth(xd::objhandle_t data_handle)
{
    return 0;
}

const unsigned char* XlsxIterator::getRawPtr(xd::objhandle_t data_handle)
{
    return NULL;
}


const std::string& XlsxIterator::getString(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
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

    dai->str_result = kl::tostring(m_file->getString(dai->ordinal));
    return dai->str_result;
}

const std::wstring& XlsxIterator::getWideString(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
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

        dai->wstr_result = m_file->getString(dai->ordinal);
        return dai->wstr_result;
    }

    return empty_wstring;
}

xd::datetime_t XlsxIterator::getDateTime(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
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

    XlsxDateTime xbase_date = m_file->getDateTime(dai->ordinal);
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

double XlsxIterator::getDouble(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
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

    return m_file->getDouble(dai->ordinal);
}

int XlsxIterator::getInteger(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
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

    return m_file->getInteger(dai->ordinal);
}

bool XlsxIterator::getBoolean(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
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

    return m_file->getBoolean(dai->ordinal);
}

bool XlsxIterator::isNull(xd::objhandle_t data_handle)
{
    XlsxDataAccessInfo* dai = (XlsxDataAccessInfo*)data_handle;
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
            
    return m_file->isNull(dai->ordinal);
}





