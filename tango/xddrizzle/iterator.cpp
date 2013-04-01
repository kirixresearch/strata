/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-04-16
 *
 */


#include <xcm/xcm.h>
#include <kl/string.h>
#include <kl/portable.h>
#include "tango.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/sqlcommon.h"
#include "drizzle_client.h"
#include "database.h"
#include "set.h"
#include "iterator.h"


const std::string empty_string = "";
const std::wstring empty_wstring = L"";


DrizzleIterator::DrizzleIterator()
{
    m_con = NULL;
    m_result = NULL;
    m_row = NULL;
    m_row_pos = 0;
    m_cache_active = false;
    m_cache_row_ptrs = NULL;
}

DrizzleIterator::~DrizzleIterator()
{
    m_reader.uninit();
    
    if (m_result)
        drizzle_result_free(m_result);
    
    if (m_con)
        drizzle_con_free(m_con);

    // clean up field vector and expression vector

    std::vector<DrizzleDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
        delete (*it);
    
    delete[] m_cache_row_ptrs;
}

bool DrizzleIterator::init(const std::wstring& query)
{
    IDrizzleDatabasePtr mydb = m_database;
    
    drizzle_return_t ret;
    
    if (m_con == NULL)
    {
        m_con = mydb->open();
        if (m_con == NULL)
            return false;

        std::string asc_command = kl::tostring(query);

        m_result = drizzle_query_str(m_con, NULL, asc_command.c_str(), &ret);
        if (ret != DRIZZLE_RETURN_OK || m_result == NULL)
        {
            mydb->setError(tango::errorGeneral, kl::towstring(drizzle_error(m_drizzle)));
            drizzle_con_free(m_con);
            m_con = NULL;
            return false;
        }
        
        //ret = drizzle_result_buffer(m_result);
        
        
        // make sure we have retrieved column info
        if (!(m_result->options & DRIZZLE_RESULT_BUFFER_COLUMN))
        {
            if (drizzle_column_buffer(m_result) != DRIZZLE_RETURN_OK)
            {
                drizzle_result_free(m_result);
                drizzle_con_free(m_con);
                m_result = NULL;
                m_con = NULL;
            }
        }
        
        // set up the unbuffered reader
        m_reader.init(m_result);
    }
    
    
    m_row_arr_size = drizzle_result_column_count(m_result);
    
    if (m_row_arr_size == 0)
    {
        // query executed properly, but was not a SELECT query
        drizzle_result_free(m_result);
        drizzle_con_free(m_con);
        m_con = NULL;
        m_result = NULL;
        return false;
    }
    

    int i = 0;
    drizzle_column_st* col;
    
    drizzle_column_seek(m_result, 0);
    while ((col = drizzle_column_next(m_result)))
    {
        int drizzle_type = drizzle_column_type(col);
        int tango_type = drizzle2tangoType(drizzle_type);

        std::wstring wcol_name = kl::towstring(drizzle_column_name(col));

        DrizzleDataAccessInfo* dai = new DrizzleDataAccessInfo;
        dai->name = wcol_name;
        dai->type = tango_type;
        dai->drizzle_type = drizzle_type;
        dai->width = drizzle_column_size(col);
        dai->scale = (tango_type == tango::typeDouble ? 4 : drizzle_column_decimals(col));
        dai->ordinal = i;
        
        // limit blob/text fields to 4096 characters (for now) --
        // this seems to be sensible behavior because copies of
        // the table will not clog of the database space-wise
        if ((drizzle_type == DRIZZLE_COLUMN_TYPE_BLOB || drizzle_type == DRIZZLE_COLUMN_TYPE_LONG_BLOB) && dai->width > 4096)
            dai->width = 4096;
        
        m_fields.push_back(dai);

        i++;
    }

    // initialize place for the cache row results
    m_cache_row_ptrs = new char*[m_row_arr_size];
    
    // position cursor at the beginning of the table
    m_reader.next();
    m_row = m_reader.getRow();
    m_lengths = m_reader.getFieldLengths();

    // if m_set is null, create a placeholder set
    if (m_set.isNull())
    {
        // create set and initialize variables
        DrizzleSet* set = new DrizzleSet;
        set->m_database = m_database;
        set->m_drizzle = m_drizzle;
        set->m_tablename = getTableNameFromSql(query);
        
        // initialize Odbc connection for this set
        if (!set->init())
            return false;

        m_set = static_cast<tango::ISet*>(set);
    }
    
    
    refreshStructure();
    
    return true;
}


tango::ISetPtr DrizzleIterator::getSet()
{
    return m_set;
}

tango::rowpos_t DrizzleIterator::getRowCount()
{
    return 0;
}

tango::IDatabasePtr DrizzleIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr DrizzleIterator::clone()
{
    return xcm::null;
}

void DrizzleIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & tango::ifReverseRowCache) != 0) ? true : false;
}

unsigned int DrizzleIterator::getIteratorFlags()
{
    // if we have a unidirectional iterator, but the back-scroll
    // cache is on, then we still can scroll back
    if (m_cache_active)
    {
        return 0;
    }
    
    // forward only
    return tango::ifForwardOnly;
}



void DrizzleIterator::saveRowToCache()
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



void DrizzleIterator::readRowFromCache(tango::rowpos_t row)
{
    m_cache.goRow((tango::rowpos_t)row);
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

void DrizzleIterator::skipWithCache(int delta)
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
    
    if ((tango::rowpos_t)desired_row < m_cache.getRowCount())
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
            
            m_reader.next();
            m_row = m_reader.getRow();
            m_row_pos++;
            
            if (!m_row)
            {
                m_lengths = NULL;
                break;
            }
            
            m_lengths = m_reader.getFieldLengths();
        }
             
    }
}

void DrizzleIterator::skip(int delta)
{
    if (m_cache_active)
    {
        skipWithCache(delta);
        return;
    }

    if (delta == 0)
        return;

    if (delta < 0)
    {
        // backwards scrolling not allowed
        m_row = NULL;
        return;
    }

    int i;
    for (i = 0; i < delta; ++i)
    {
        m_reader.next();
        m_row = m_reader.getRow();
    }
    
    m_lengths = m_reader.getFieldLengths();
}

void DrizzleIterator::goFirst()
{
    if (m_cache_active)
    {
        skipWithCache(-((int)m_row_pos));
        return;
    }
}

void DrizzleIterator::goLast()
{

}

double DrizzleIterator::getPos()
{
    return 0.0;
}

tango::rowid_t DrizzleIterator::getRowId()
{
    return m_row_pos;
}

bool DrizzleIterator::bof()
{
    return false;
}

bool DrizzleIterator::eof()
{
    return m_row ? false : true;
}

bool DrizzleIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool DrizzleIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool DrizzleIterator::setPos(double pct)
{
    return false;
}

void DrizzleIterator::goRow(const tango::rowid_t& rowid)
{
}

tango::IStructurePtr DrizzleIterator::getStructure()
{
    Structure* s = new Structure;

    std::vector<DrizzleDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        col->setName((*it)->name);
        col->setType((*it)->type);
        col->setWidth((*it)->width);
        col->setScale((*it)->scale);
        col->setColumnOrdinal((*it)->ordinal);
        col->setExpression((*it)->expr_text);
        col->setCalculated((*it)->isCalculated());
        s->addColumn(col);
    }
    
    return static_cast<tango::IStructure*>(s);
}

void DrizzleIterator::refreshStructure()
{
    if (m_set.isNull())
        return;

    tango::IStructurePtr set_structure = m_set->getStructure();
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
        
        tango::IColumnInfoPtr col = set_structure->getColumnInfo(m_fields[i]->name);
        if (col.isNull())
        {
            m_fields.erase(m_fields.begin() + i);
            i--;
            continue;
        }
                       
        m_fields[i]->type = col->getType();
        m_fields[i]->width = col->getWidth();
        m_fields[i]->scale = col->getScale();
        m_fields[i]->expr_text = col->getExpression();
    }
    
    // find new calc fields
    
    std::vector<DrizzleDataAccessInfo*>::iterator it;
    col_count = set_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr col;
        
        col = set_structure->getColumnInfoByIdx(i);
        if (!col->getCalculated())
            continue;
            
        bool found = false;
        
        for (it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (!(*it)->isCalculated())
                continue;

            if (0 == wcscasecmp((*it)->name.c_str(), col->getName().c_str()))
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // add new calc field
            DrizzleDataAccessInfo* dai = new DrizzleDataAccessInfo;
            dai->name = col->getName();
            dai->type = col->getType();
            dai->width = col->getWidth();
            dai->scale = col->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = col->getExpression();
            dai->expr = NULL;
                
            m_fields.push_back(dai);
        }
    }
    
    // parse all expressions

    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        (*it)->expr = parse((*it)->expr_text);
    }
}

bool DrizzleIterator::modifyStructure(tango::IStructure* struct_config, tango::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<DrizzleDataAccessInfo*>::iterator it2;
    
    // handle delete
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
                DrizzleDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
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

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            DrizzleDataAccessInfo* dai = new DrizzleDataAccessInfo;
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

    // handle insert
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->m_pos;
        if (insert_idx < 0 || insert_idx >= (int)m_fields.size())
            continue;
        
        if (it->m_params->getExpression().length() > 0)
        {
            DrizzleDataAccessInfo* dai = new DrizzleDataAccessInfo;
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


tango::objhandle_t DrizzleIterator::getHandle(const std::wstring& expr)
{
    std::vector<DrizzleDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }


    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        DrizzleDataAccessInfo* dai = new DrizzleDataAccessInfo;
        dai->expr = NULL;
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


    DrizzleDataAccessInfo* dai = new DrizzleDataAccessInfo;
    dai->expr = parser;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool DrizzleIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<DrizzleDataAccessInfo*>::iterator it;
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

tango::IColumnInfoPtr DrizzleIterator::getInfo(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
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
    colinfo->setCalculated(dai->isCalculated());

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

    return static_cast<tango::IColumnInfo*>(colinfo);
}

int DrizzleIterator::getType(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0;

    return dai->type;
}

int DrizzleIterator::getRawWidth(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* DrizzleIterator::getRawPtr(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
    if (dai == NULL)
        return NULL;
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return NULL;
}

const std::string& DrizzleIterator::getString(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
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

const std::wstring& DrizzleIterator::getWideString(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
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
            dai->str_result = kl::tostring(dai->expr_result.getString());
            return dai->wstr_result;
        }

        dai->wstr_result = (m_row == NULL || m_row[dai->ordinal] == NULL) ?
                                 L"" : kl::towstring(m_row[dai->ordinal]);
        return dai->wstr_result;
    }

    return empty_wstring;
}

tango::datetime_t DrizzleIterator::getDateTime(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
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


    int y, m, d, h, mm, s;
    
    // handle nulls
    if (m_row == NULL || m_row[dai->ordinal] == NULL)
        return 0;

    char buf[64];
    char* c = m_row[dai->ordinal];


    if (dai->drizzle_type == DRIZZLE_COLUMN_TYPE_TIME)
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

        tango::DateTime dt;
        dt.setYear(0);
        dt.setMonth(0);
        dt.setDay(0);
        dt.setHour(h);
        dt.setMinute(mm);
        dt.setSecond(s);
        return dt.getDateTime();
    }

    if (dai->drizzle_type == DRIZZLE_COLUMN_TYPE_TIMESTAMP)
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
            
        tango::DateTime dt;
        dt.setYear(y);
        dt.setMonth(m);
        dt.setDay(d);
        dt.setHour(h);
        dt.setMinute(mm);
        dt.setSecond(s);
        return dt.getDateTime();
    }

    if (dai->drizzle_type == DRIZZLE_COLUMN_TYPE_DATETIME)
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
            
        tango::DateTime dt;
        dt.setYear(y);
        dt.setMonth(m);
        dt.setDay(d);
        dt.setHour(h);
        dt.setMinute(mm);
        dt.setSecond(s);
        return dt.getDateTime();
    }

    if (dai->drizzle_type == DRIZZLE_COLUMN_TYPE_DATE)
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

        tango::datetime_t dt;
        dt = dateToJulian(y, m, d);
        dt <<= 32;
        return dt;
    }

    return 0;
}

double DrizzleIterator::getDouble(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
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
                    0.0 : kl::nolocale_atof(m_row[dai->ordinal]);
}

int DrizzleIterator::getInteger(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
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

bool DrizzleIterator::getBoolean(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
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

bool DrizzleIterator::isNull(tango::objhandle_t data_handle)
{
    DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    return (m_row == NULL || m_row[dai->ordinal] == NULL) ? true : false;
}



// tango::ICacheRowUpdate::updateCacheRow()

bool DrizzleIterator::updateCacheRow(tango::rowid_t rowid,
                                   tango::ColumnUpdateInfo* info,
                                   size_t info_size)
{
    saveRowToCache();
    
    size_t i;
    for (i = 0; i < info_size; ++i, ++info)
    {
        DrizzleDataAccessInfo* dai = (DrizzleDataAccessInfo*)info->handle;
        int column = dai->ordinal;
        
        if (info->null)
            m_cache.updateValue(m_row_pos, column, NULL, 0);
        

        switch (dai->type)
        {
            case tango::typeCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->str_val.c_str(),
                                    info->str_val.length()+1);
                break;

            case tango::typeWideCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)kl::tostring(info->wstr_val).c_str(),
                                    info->wstr_val.length()+1);
                break;

            case tango::typeNumeric:
            case tango::typeDouble:
            {
                char buf[128];
                sprintf(buf, "%.*f", dai->scale, info->dbl_val);
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)buf,
                                    strlen(buf)+1);
            }
            break;

            case tango::typeInteger:
            {
                char buf[128];
                sprintf(buf, "%d", info->int_val);
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)buf,
                                    strlen(buf)+1);
            }
            break;

            case tango::typeDate:
            {
                tango::DateTime dt;
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
          
            case tango::typeDateTime:
            {
                tango::DateTime dt;
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

            case tango::typeBoolean:
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

