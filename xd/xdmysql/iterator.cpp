/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-04-16
 *
 */


#ifdef WIN32
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#endif

#include <kl/xcm.h>
#include <kl/string.h>
#include <kl/portable.h>

#include "mysql.h"
#include <xd/xd.h>
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "database.h"
#include "iterator.h"


static std::string empty_string = "";
static std::wstring empty_wstring = L"";


MysqlIterator::MysqlIterator(MysqlDatabase* database)
{
    m_database = database;
    m_database->ref();

    m_data = NULL;
    m_res = NULL;
    m_row = NULL;
    m_row_pos = 0;
    m_cache_active = false;
    m_cache_row_ptrs = NULL;
}

MysqlIterator::~MysqlIterator()
{
    if (m_res)
    {
        mysql_free_result(m_res);
    }
    
    if (m_data)
    {
        mysql_close(m_data);
    }

    // clean up field vector and expression vector

    std::vector<MysqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
        delete (*it);
    
    delete[] m_cache_row_ptrs;

    m_database->unref();
}

bool MysqlIterator::init(const std::wstring& query)
{
    if (m_data == NULL)
    {
        m_data = m_database->open();
        if (m_data == NULL)
            return false;
    

        std::string asc_query = kl::tostring(query);
        int error = mysql_query(m_data, asc_query.c_str());

        if (error)
            return false;
    }
    
    m_res = mysql_use_result(m_data);
    if (!m_res)
    {
        // query executed properly, but was not a SELECT query
        return false;
    }
    
    m_row_arr_size = mysql_num_fields(m_res);
    
    
    int i = 0;
    MYSQL_FIELD* colinfo;

    while ((colinfo = mysql_fetch_field(m_res)))
    {
        int type = mysql2xdType(colinfo->type);

        std::wstring wcol_name = kl::towstring(colinfo->name);

        MysqlDataAccessInfo* dai = new MysqlDataAccessInfo;
        dai->name = wcol_name;
        dai->type = type;
        dai->mysql_type = colinfo->type;
        dai->width = colinfo->length;
        dai->scale = type == xd::typeDouble ? 4 : colinfo->decimals;
        dai->ordinal = i;
        
        // limit blob/text fields to 4096 characters (for now) --
        // this seems to be sensible behavior because copies of
        // the table will not clog of the database space-wise
        if (colinfo->type == FIELD_TYPE_BLOB && dai->width > 4096)
            dai->width = 4096;
        
        m_fields.push_back(dai);

        i++;
    }

    // initialize place for the cache row results
    m_cache_row_ptrs = new char*[m_row_arr_size];
    
    // position cursor at the beginning of the table
    m_row = mysql_fetch_row(m_res);
    m_lengths = mysql_fetch_lengths(m_res);
    
    refreshStructure();
    
    return true;
}


void MysqlIterator::setTable(const std::wstring& table)
{
    m_path = table;
}

std::wstring MysqlIterator::getTable()
{
    return m_path;
}

xd::rowpos_t MysqlIterator::getRowCount()
{
    if (m_table.length() > 0)
    {
        std::wstring query;
        query += L"SELECT COUNT(*) FROM ";
        query += L"`";
        query += m_table;
        query += L"`";

        MYSQL* db = m_database->open();
        if (!db)
            return 0;

        std::string asc_query = kl::tostring(query);
        int error = mysql_query(db, asc_query.c_str());

        if (!error)
        {
            MYSQL_RES* res = mysql_use_result(db);
            MYSQL_ROW row = mysql_fetch_row(res);
            xd::rowpos_t row_count = atoi(row[0]);
            mysql_free_result(res);
            mysql_close(db);
            return row_count;
        }
         else
        {
            mysql_close(db);
            return 0;
        }
    }

    return 0;
}

// -- IIterator interface implementation --

xd::IDatabasePtr MysqlIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr MysqlIterator::clone()
{
    return xcm::null;
}

void MysqlIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & xd::ifReverseRowCache) != 0) ? true : false;
}

unsigned int MysqlIterator::getIteratorFlags()
{
    unsigned int flags = 0;


    // if we have a unidirectional iterator, but the back-scroll
    // cache is on, then we still can scroll back
    if (!m_cache_active)
    {
        flags |= xd::ifForwardOnly;
    }
    

    if (m_table.length() > 0)
    {
        flags |= xd::ifFastRowCount;
    }


    return flags;
}



void MysqlIterator::saveRowToCache()
{
    if (!m_row)
    {
        // no row to save
        return;
    }
    
    if (m_row_pos < m_cache.getRowCount())
    {
        // row already saved
        return;
    }



    m_cache.createRow();
    
    for (int i = 0; i < m_row_arr_size; ++i)
    {
        if (m_row[i])
        {
            m_cache.appendColumnData((unsigned char*)m_row[i], m_lengths[i] + 1);
        }
         else
        {
            m_cache.appendNullColumn();
            continue;
        }
    }
    
    m_cache.finishRow();
}



void MysqlIterator::readRowFromCache(xd::rowpos_t row)
{
    m_cache.goRow((xd::rowpos_t)row);
    m_cache.getRow(m_cache_row);

    int col;
    unsigned char* data;
    unsigned int data_size;
    bool is_null;
    
    // read cached data into the dai values
    for (col = 0; col < m_row_arr_size; ++col)
    {
        data = m_cache_row.getColumnData(col, &data_size, &is_null);
        
        if (is_null)
            m_cache_row_ptrs[col] = NULL;
             else
            m_cache_row_ptrs[col] = (char*)data;
    }      
}

void MysqlIterator::skipWithCache(int delta)
{
    if (delta == 0)
        return;
    
    if (!m_cache.isOk())
    {
        if (!m_cache.init())
        {
            // init failed, deactivate cache
            m_cache_active = false;
        }
    }
    
    

    // save row to cache if necessary
    saveRowToCache();
    
        
    long long desired_row = ((long long)m_row_pos) + delta;
    if (desired_row < 0)
        desired_row = 0;
    
    if ((xd::rowpos_t)desired_row < m_cache.getRowCount())
    {
        readRowFromCache(desired_row);
        
        m_row = m_cache_row_ptrs;
        m_row_pos = desired_row;
    }
     else
    {        
        int i;

        // fetch the row from the database, adding new rows
        // to the cache along the way
        
        for (i = 0; i < delta; ++i)
        {
            saveRowToCache();
            
            m_row = mysql_fetch_row(m_res);
            m_row_pos++;
            
            if (!m_row)
            {
                m_lengths = NULL;
                break;
            }
            
            m_lengths = mysql_fetch_lengths(m_res);
        }
             
    }
}

void MysqlIterator::skip(int delta)
{

    if (m_cache_active)
    {
        skipWithCache(delta);
        return;
    }


/*
    if (delta < 0)
    {
        delta = -delta;
        if (delta > m_row_pos)
            m_row_pos = 0;
             else
            m_row_pos -= delta;
    }
     else
    {
        m_row_pos += delta;
    }
    
    mysql_data_seek(m_res, m_row_pos);
    m_row = mysql_fetch_row(m_res);
    m_lengths = mysql_fetch_lengths(m_res);
*/


    if (delta == 0)
    {
        return;
    }

    if (delta < 0)
    {
        // backwards scrolling not allowed
        m_row = NULL;
        return;
    }

    int i;
    for (i = 0; i < delta; ++i)
    {
        m_row = mysql_fetch_row(m_res);
    }
    
    m_lengths = mysql_fetch_lengths(m_res);
}

void MysqlIterator::goFirst()
{
    if (m_cache_active)
    {
        skipWithCache(-((int)m_row_pos));
        return;
    }
    
    //m_row_pos = 0;
    //m_row = mysql_fetch_row(m_res);
    //m_lengths = mysql_fetch_lengths(m_res);
}

void MysqlIterator::goLast()
{

}

double MysqlIterator::getPos()
{
    return 0.0;
}

xd::rowid_t MysqlIterator::getRowId()
{
    return m_row_pos;
}

bool MysqlIterator::bof()
{
    return false;
}

bool MysqlIterator::eof()
{
    return m_row ? false : true;
}

bool MysqlIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool MysqlIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool MysqlIterator::setPos(double pct)
{
    return false;
}

void MysqlIterator::goRow(const xd::rowid_t& rowid)
{
}

xd::Structure MysqlIterator::getStructure()
{
    xd::Structure s;

    std::vector<MysqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        xd::ColumnInfo col;

        col.name = (*it)->name;
        col.type = (*it)->type;
        col.width = (*it)->width;
        col.scale = (*it)->scale;
        col.column_ordinal = (*it)->ordinal;
        col.expression = (*it)->expr_text;
        col.calculated = (*it)->isCalculated();

        s.createColumn(col);
    }
    
    return s;
}

bool MysqlIterator::refreshStructure()
{
/*
    xd::Structure set_structure = m_set->getStructure();
    if (set_structure.isNull())
        return;
        
    // find changed/deleted calc fields
    int i, col_count;
    for (i = 0; i < (int)m_fields.size(); ++i)
    {
        if (!m_fields[i]->isCalculated())
            continue;

        delete m_fields[i]->expr;
        m_fields[i]->expr = NULL;
        
        const xd::ColumnInfo& col = set_structure->getColumnInfo(m_fields[i]->name);
        if (col.isNull())
        {
            m_fields.erase(m_fields.begin() + i);
            i--;
            continue;
        }
                       
        m_fields[i]->type = col.type;
        m_fields[i]->width = col.width;
        m_fields[i]->scale = col.scale;
        m_fields[i]->expr_text = col.expression;
    }
    
    // find new calc fields
    
    std::vector<MysqlDataAccessInfo*>::iterator it;
    col_count = set_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& col = set_structure->getColumnInfoByIdx(i);
        if (!col.calculated)
            continue;
            
        bool found = false;
        
        for (it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (!(*it)->isCalculated())
                continue;

            if (0 == wcscasecmp((*it)->name.c_str(), col.name.c_str()))
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // add new calc field
            MysqlDataAccessInfo* dai = new MysqlDataAccessInfo;
            dai->name = col.name;
            dai->type = col.type;
            dai->width = col.width;
            dai->scale = col.scale;
            dai->ordinal = m_fields.size();
            dai->expr_text = col.expression;
            dai->expr = NULL;
                
            m_fields.push_back(dai);
        }
    }
    
    // parse all expressions

    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        (*it)->expr = parse((*it)->expr_text);
    }
    */

    return true;
}

bool MysqlIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<MysqlDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                MysqlDataAccessInfo* dai = *(it2);
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
            MysqlDataAccessInfo* dai = new MysqlDataAccessInfo;
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
        if (insert_idx < 0 || insert_idx >= (int)m_fields.size())
            continue;
        
        if (it->params.expression.length() > 0)
        {
            MysqlDataAccessInfo* dai = new MysqlDataAccessInfo;
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


xd::objhandle_t MysqlIterator::getHandle(const std::wstring& expr)
{
    std::vector<MysqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (xd::objhandle_t)(*it);
    }


    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        MysqlDataAccessInfo* dai = new MysqlDataAccessInfo;
        dai->expr = NULL;
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


    MysqlDataAccessInfo* dai = new MysqlDataAccessInfo;
    dai->expr = parser;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool MysqlIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<MysqlDataAccessInfo*>::iterator it;
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

xd::ColumnInfo MysqlIterator::getInfo(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xd::ColumnInfo();
    }

    xd::ColumnInfo colinfo;
    colinfo.name = dai->name;
    colinfo.type = dai->type;
    colinfo.width = dai->width;
    colinfo.scale = dai->scale;
    colinfo.expression = dai->expr_text;
    colinfo.calculated = dai->isCalculated();

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

    return colinfo;
}

int MysqlIterator::getType(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;

    return dai->type;
}

int MysqlIterator::getRawWidth(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* MysqlIterator::getRawPtr(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
    if (dai == NULL)
        return NULL;
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return NULL;
}

const std::string& MysqlIterator::getString(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
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

    dai->str_result = (m_row == NULL || m_row[dai->ordinal] == NULL) ?
                            "" : m_row[dai->ordinal];
    return dai->str_result;
}

const std::wstring& MysqlIterator::getWideString(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
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
            dai->str_result = kl::tostring(dai->expr_result.getString());
            return dai->wstr_result;
        }

        dai->wstr_result = (m_row == NULL || m_row[dai->ordinal] == NULL) ?
                                 L"" : kl::towstring(m_row[dai->ordinal]);
        return dai->wstr_result;
    }

    return empty_wstring;
}

xd::datetime_t MysqlIterator::getDateTime(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
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


    int y, m, d, h, mm, s;
    
    // handle nulls
    if (m_row == NULL || m_row[dai->ordinal] == NULL)
        return 0;

    char buf[64];
    char* c = m_row[dai->ordinal];


    if (dai->mysql_type == FIELD_TYPE_TIME)
    {
        memcpy(buf, c, 2);
        buf[2] = 0;
        h = atoi(buf);

        memcpy(buf, c+3, 2);
        buf[2] = 0;
        mm = atoi(buf);

        memcpy(buf, c+6, 2);
        buf[2] = 0;
        s = atoi(buf);

        xd::DateTime dt;
        dt.setYear(0);
        dt.setMonth(0);
        dt.setDay(0);
        dt.setHour(h);
        dt.setMinute(mm);
        dt.setSecond(s);
        return dt.getDateTime();
    }

    if (dai->mysql_type == FIELD_TYPE_TIMESTAMP)
    {
        memcpy(buf, c, 4);
        buf[4] = 0;
        y = atoi(buf);

        memcpy(buf, c+5, 2);
        buf[2] = 0;
        m = atoi(buf);

        memcpy(buf, c+8, 2);
        buf[2] = 0;
        d = atoi(buf);

        memcpy(buf, c+11, 2);
        buf[2] = 0;
        h = atoi(buf);

        memcpy(buf, c+14, 2);
        buf[2] = 0;
        mm = atoi(buf);

        memcpy(buf, c+17, 2);
        buf[2] = 0;
        s = atoi(buf);
        
        if (y == 0 && m == 0 && d == 0)
            return 0;
            
        xd::DateTime dt;
        dt.setYear(y);
        dt.setMonth(m);
        dt.setDay(d);
        dt.setHour(h);
        dt.setMinute(mm);
        dt.setSecond(s);
        return dt.getDateTime();
    }

    if (dai->mysql_type == FIELD_TYPE_DATETIME)
    {
        memcpy(buf, c, 4);
        buf[4] = 0;
        y = atoi(buf);

        memcpy(buf, c+5, 2);
        buf[2] = 0;
        m = atoi(buf);

        memcpy(buf, c+8, 2);
        buf[2] = 0;
        d = atoi(buf);

        memcpy(buf, c+11, 2);
        buf[2] = 0;
        h = atoi(buf);

        memcpy(buf, c+14, 2);
        buf[2] = 0;
        mm = atoi(buf);

        memcpy(buf, c+17, 2);
        buf[2] = 0;
        s = atoi(buf);

        if (y == 0 && m == 0 && d == 0)
            return 0;
            
        xd::DateTime dt;
        dt.setYear(y);
        dt.setMonth(m);
        dt.setDay(d);
        dt.setHour(h);
        dt.setMinute(mm);
        dt.setSecond(s);
        return dt.getDateTime();
    }

    if (dai->mysql_type == FIELD_TYPE_DATE)
    {
        memcpy(buf, c, 4);
        buf[4] = 0;
        y = atoi(buf);

        memcpy(buf, c+5, 2);
        buf[2] = 0;
        m = atoi(buf);

        memcpy(buf, c+8, 2);
        buf[2] = 0;
        d = atoi(buf);

        if (y == 0 && m == 0 && d == 0)
            return 0;

        xd::datetime_t dt;
        dt = dateToJulian(y, m, d);
        dt <<= 32;
        return dt;
    }

    return 0;
}

double MysqlIterator::getDouble(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0.0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getDouble();
    }

    return (m_row == NULL || m_row[dai->ordinal] == NULL || strlen(m_row[dai->ordinal]) == 0) ?
                    0.0 : atof(m_row[dai->ordinal]);
}

int MysqlIterator::getInteger(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getInteger();
    }

    return (m_row == NULL || m_row[dai->ordinal] == NULL || strlen(m_row[dai->ordinal]) == 0) ?
                    0 : atoi(m_row[dai->ordinal]);
}

bool MysqlIterator::getBoolean(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getBoolean();
    }

    return (m_row == NULL || m_row[dai->ordinal] == NULL || strlen(m_row[dai->ordinal]) == 0) ?
                      false : (*m_row[dai->ordinal] ? true : false);
}

bool MysqlIterator::isNull(xd::objhandle_t data_handle)
{
    MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    return (m_row == NULL || m_row[dai->ordinal] == NULL) ? true : false;
}



// xd::ICacheRowUpdate::updateCacheRow()

bool MysqlIterator::updateCacheRow(xd::rowid_t rowid,
                                   xd::ColumnUpdateInfo* info,
                                   size_t info_size)
{
    saveRowToCache();
    
    size_t i;
    for (i = 0; i < info_size; ++i, ++info)
    {
        MysqlDataAccessInfo* dai = (MysqlDataAccessInfo*)info->handle;
        int column = dai->ordinal;
        
        if (info->null)
            m_cache.updateValue(m_row_pos, column, NULL, 0);
        

        switch (dai->type)
        {
            case xd::typeCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->str_val.c_str(),
                                    info->str_val.length()+1);
                break;

            case xd::typeWideCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)kl::tostring(info->wstr_val).c_str(),
                                    info->wstr_val.length()+1);
                break;

            case xd::typeNumeric:
            case xd::typeDouble:
            {
                char buf[128];
                sprintf(buf, "%.*f", dai->scale, info->dbl_val);
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)buf,
                                    strlen(buf)+1);
            }
            break;

            case xd::typeInteger:
            {
                char buf[128];
                sprintf(buf, "%d", info->int_val);
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)buf,
                                    strlen(buf)+1);
            }
            break;

            case xd::typeDate:
            {
                xd::DateTime dt;
                dt.setDateTime(info->date_val);
                
                char buf[128];
                sprintf(buf, "%04d-%02d-%02d", 
                        dt.getYear(), dt.getMonth(), dt.getDay());

                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)buf,
                                    strlen(buf)+1);
                break;
            }
            break;
          
            case xd::typeDateTime:
            {
                xd::DateTime dt;
                dt.setDateTime(info->date_val);
                
                char buf[128];
                sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", 
                        dt.getYear(), dt.getMonth(), dt.getDay(),
                        dt.getHour(), dt.getMinute(), dt.getSecond());

                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)buf,
                                    strlen(buf)+1);
                break;
            }
            break;

            case xd::typeBoolean:
            {
                int i = info->bool_val ? 1 : 0;
                
                char buf[128];
                sprintf(buf, "%d", i);
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)buf,
                                    strlen(buf)+1);
            }
            break;
        }
    }


    readRowFromCache(m_row_pos);


    return true;
}

