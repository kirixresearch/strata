/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "database.h"
#include "xbaseset.h"
#include "xbaseiterator.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/util.h"


const std::string empty_string = "";
const std::wstring empty_wstring = L"";



XbaseIterator::XbaseIterator()
{
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
        m_file.closeFile();
    }

    if (m_set)
        m_set->unref();
}

bool XbaseIterator::init(tango::IDatabasePtr db,
                         XbaseSet* set,
                         const std::wstring& filename)
{
    if (!m_file.openFile(filename))
        return false;

    m_database = db;
    m_set = set;
    m_set->ref();

    m_current_row = 1;

    refreshStructure();

    return true;
}

tango::ISetPtr XbaseIterator::getSet()
{
    return m_set;
}

tango::rowpos_t XbaseIterator::getRowCount()
{
    return 0;
}

tango::IDatabasePtr XbaseIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr XbaseIterator::clone()
{
    XbaseIterator* iter = new XbaseIterator;
    
    if (!iter->init(m_database, m_set, m_file.getFilename()))
    {
        return xcm::null;
    }
    
    iter->goRow(m_current_row);
    
    return static_cast<tango::IIterator*>(iter);
}

unsigned int XbaseIterator::getIteratorFlags()
{
    return 0;
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

tango::rowid_t XbaseIterator::getRowId()
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

void XbaseIterator::goRow(const tango::rowid_t& rowid)
{
    m_current_row = (unsigned int)rowid;
    m_file.goRow(m_current_row);
}

tango::IStructurePtr XbaseIterator::getStructure()
{
    tango::IStructurePtr s = static_cast<tango::IStructure*>(new Structure);
    IStructureInternalPtr struct_int = s;
    
    std::vector<XbaseDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        struct_int->addColumn(col);
        
        col->setName((*it)->name);
        col->setType((*it)->type);
        col->setWidth((*it)->width);
        col->setScale((*it)->scale);
        col->setColumnOrdinal((*it)->ordinal);
        col->setExpression((*it)->expr_text);
        
        if ((*it)->xbase_type == 'Y')    // currency
            col->setWidth(18);
        if (col->getType() == tango::typeDouble)
            col->setWidth(8);
        if (col->getType() == tango::typeNumeric &&
            col->getWidth() > tango::max_numeric_width)
        {
            col->setWidth(tango::max_numeric_width);
        }
        if (col->getExpression().length() > 0)
            col->setCalculated(true);
    }

    return s;
}

void XbaseIterator::refreshStructure()
{
    // clear out any existing structure
    std::vector<XbaseDataAccessInfo*>::iterator field_it;
    for (field_it = m_fields.begin();
         field_it != m_fields.end(); ++field_it)
    {
        delete (*field_it);
    }
    m_fields.clear();


    tango::IStructurePtr s = m_set->getStructure();
    int col_count = s->getColumnCount();
    int i;

    // get structure from xbase file
    std::vector<XbaseField> fields = m_file.getFields();
    
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        
        XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
        dai->xbase_type = tango2xbaseType(colinfo->getType());
        dai->name = colinfo->getName();
        dai->type = colinfo->getType();
        dai->width = colinfo->getWidth();
        dai->scale = colinfo->getScale();
        dai->ordinal = colinfo->getColumnOrdinal();
        dai->expr_text = colinfo->getExpression();
        m_fields.push_back(dai);
        
        // -- look up the precise source type --
        std::vector<XbaseField>::iterator it;
        for (it = fields.begin(); it != fields.end(); ++it)
        {
            std::wstring xbase_name = kl::towstring(it->name);
            if (0 == wcscasecmp(xbase_name.c_str(), dai->name.c_str()))
            {
                dai->xbase_type = it->type;
                break;
            }
        }
        
        // -- parse any expression, if necessary --
        if (dai->expr_text.length() > 0)
        {
            dai->expr = parse(dai->expr_text);
        }
    }
}

bool XbaseIterator::modifyStructure(tango::IStructure* struct_config,
                                    tango::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<XbaseDataAccessInfo*>::iterator it2;
    
    // -- handle delete --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                XbaseDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // -- handle modify --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                if (it->m_params->getName().length() > 0)
                {
                    std::wstring new_name = it->m_params->getName();
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->m_params->getType() != -1)
                {
                    (*it2)->type = it->m_params->getType();
                }

                if (it->m_params->getWidth() != -1)
                {
                    (*it2)->width = it->m_params->getWidth();
                }

                if (it->m_params->getScale() != -1)
                {
                    (*it2)->scale = it->m_params->getScale();
                }

                if (it->m_params->getExpression().length() > 0)
                {
                     if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->m_params->getExpression();
                    (*it2)->expr = parse(it->m_params->getExpression());
                }
            }
        }
    }

    // -- handle create --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->type = it->m_params->getType();
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            m_fields.push_back(dai);
        }
    }

    // -- handle insert --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->m_pos;
        if (insert_idx < 0 || (size_t)insert_idx >= m_fields.size())
            continue;
        
        if (it->m_params->getExpression().length() > 0)
        {
            XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->type = it->m_params->getType();
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            m_fields.insert(m_fields.begin()+insert_idx, dai);
        }
    }
    
    return true;
}


tango::objhandle_t XbaseIterator::getHandle(const std::wstring& expr)
{
    std::vector<XbaseDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
        dai->expr = NULL;
        dai->expr_text = expr;
        dai->type = tango::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<tango::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }
        
        m_exprs.push_back(dai);
        return (tango::objhandle_t)dai;
    }


    kscript::ExprParser* parser = parse(expr);
    if (!parser)
    {
        return (tango::objhandle_t)0;
    }

    XbaseDataAccessInfo* dai = new XbaseDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool XbaseIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<XbaseDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

tango::IColumnInfoPtr XbaseIterator::getInfo(tango::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    ColumnInfo* colinfo = new ColumnInfo;
    colinfo->setName(dai->name);
    colinfo->setType(dai->type);
    colinfo->setWidth(dai->width);
    colinfo->setScale(dai->scale);
    colinfo->setExpression(dai->expr_text);
    
    if (dai->type == tango::typeDate ||
        dai->type == tango::typeInteger)
    {
        colinfo->setWidth(4);
    }
     else if (dai->type == tango::typeDateTime ||
              dai->type == tango::typeDouble)
    {
        colinfo->setWidth(8);
    }
     else if (dai->type == tango::typeBoolean)
    {
        colinfo->setWidth(1);
    }
     else
    {
        colinfo->setWidth(dai->width);
    }
    
    if (dai->expr_text.length() > 0)
        colinfo->setCalculated(true);

    return static_cast<tango::IColumnInfo*>(colinfo);
}

int XbaseIterator::getType(tango::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    return dai->type;
}

int XbaseIterator::getRawWidth(tango::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* XbaseIterator::getRawPtr(tango::objhandle_t data_handle)
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

const std::string& XbaseIterator::getString(tango::objhandle_t data_handle)
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

const std::wstring& XbaseIterator::getWideString(tango::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

    if (dai->type == tango::typeCharacter)
    {
        dai->wstr_result = kl::towstring(getString(data_handle));
        return dai->wstr_result;
    }
     else if (dai->type == tango::typeWideCharacter)
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

tango::datetime_t XbaseIterator::getDateTime(tango::objhandle_t data_handle)
{
    XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        tango::DateTime dt;
        return dt;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        kscript::ExprDateTime edt = dai->expr_result.getDateTime();

        tango::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        
        if (dai->type == tango::typeDateTime)
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
    
    tango::DateTime dt(xbase_date.year,
                       xbase_date.month,
                       xbase_date.day,
                       xbase_date.hour,
                       xbase_date.minute,
                       xbase_date.second,
                       xbase_date.millisecond);
    return dt;
}

double XbaseIterator::getDouble(tango::objhandle_t data_handle)
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

int XbaseIterator::getInteger(tango::objhandle_t data_handle)
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

bool XbaseIterator::getBoolean(tango::objhandle_t data_handle)
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

bool XbaseIterator::isNull(tango::objhandle_t data_handle)
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





