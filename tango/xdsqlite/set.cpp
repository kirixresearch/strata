/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#include "sqlite3.h"


#include <kl/klib.h>
#include <kl/md5.h>
#include <kl/utf8.h>
#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/util.h"


SlSet::SlSet()
{
    m_db = NULL;
}

SlSet::~SlSet()
{
}

bool SlSet::init()
{
    return true;
}





// -- tango::IStorable implementation --


void SlSet::setObjectPath(const std::wstring& path)
{
    m_path = path;
}

std::wstring SlSet::getObjectPath()
{
    if (m_path.empty())
        return m_tablename;
        
    return m_path;
}

bool SlSet::isTemporary()
{
    std::wstring path = getObjectPath();
    if (path.find(L"/.temp") != -1)
        return true;
    return false;
}

// -- tango::ISet interface implementation --

unsigned int SlSet::getSetFlags()
{
    //return tango::sfFastRowCount;
    return 0;
}

std::wstring SlSet::getSetId()
{
    if (m_set_id.length() == 0)
    {
        std::wstring id = L"xdsqlite:";
        id += m_tablename;
                
        m_set_id = kl::md5str(id);
    }
    
    return m_set_id;
}




tango::IStructurePtr SlSet::getStructure()
{
    if (m_structure.isOk())
        return m_structure->clone();
    
    m_structure = m_dbint->getStructureFromPath(m_tablename);
    if (!m_structure)
        return xcm::null;
               
    return m_structure->clone();
}

bool SlSet::modifyStructure(tango::IStructure* struct_config,
                              tango::IJob* job)
{
    XCM_AUTO_LOCK(m_object_mutex);
    return true;
}

tango::IRowInserterPtr SlSet::getRowInserter()
{
    SlRowInserter* inserter = new SlRowInserter(this);
    return static_cast<tango::IRowInserter*>(inserter);
}

tango::IRowDeleterPtr SlSet::getRowDeleter()
{
    return xcm::null;
}

int SlSet::insert(tango::IIteratorPtr source_iter,
                  const std::wstring& where_condition,
                  int max_rows,
                  tango::IJob* job)
{
    return xdcmnInsert(source_iter, this, where_condition, max_rows, job);
}

int SlSet::update(tango::ISetPtr constraint,
                      const std::wstring& params,
                      tango::IJob* job)
{
    return 0;
}

int SlSet::remove(tango::ISetPtr constraint, tango::IJob* job)
{
    return 0;
}

tango::IIndexInfoEnumPtr SlSet::getIndexEnum()
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* indexes;
    indexes = new xcm::IVectorImpl<tango::IIndexInfoPtr>;
    return indexes;
}

tango::IIndexInfoPtr SlSet::createIndex(const std::wstring& tag,
                                            const std::wstring& expr,
                                            tango::IJob* job)
{
    return xcm::null;
}

tango::IIndexInfoPtr SlSet::lookupIndex(const std::wstring& expr,
                                            bool exact_column_order)
{
    return xcm::null;
}

bool SlSet::deleteIndex(const std::wstring& name)
{
    return false;
}


bool SlSet::renameIndex(const std::wstring& name,
                            const std::wstring& new_name)
{
    return false;
}


tango::IRelationEnumPtr SlSet::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IRelationPtr SlSet::createRelation(const std::wstring& tag,
                                          const std::wstring& right_set_path,
                                          const std::wstring& left_expr,
                                          const std::wstring& right_expr)
{
    return xcm::null;
}

tango::IRelationPtr SlSet::getRelation(const std::wstring& rel_tag)
{
    return xcm::null;
}

int SlSet::getRelationCount()
{
    return xcm::null;
}

bool SlSet::deleteRelation(const std::wstring& tag)
{
    return false;
}

bool SlSet::deleteAllRelations()
{
    return false;
}

tango::IIteratorPtr SlSet::createIterator(const std::wstring& columns,
                                          const std::wstring& expr,
                                          tango::IJob* job)
{
    // -- create an iterator based on our select statement --
    SlIterator* iter = new SlIterator;
    iter->m_db = m_db;
    iter->m_dbint = m_dbint;
    iter->m_database = m_database;
    iter->m_ordinal = m_ordinal;
    iter->m_set = static_cast<tango::ISet*>(this);
    

    std::wstring sql;
    sql = L"SELECT ";
    if (columns.empty())
    {
        sql += L"*";
    }
     else
    {
        sql += columns;
    }
    sql += L" FROM ";
    sql += m_tablename;
    if (expr.length() > 0)
    {
        sql += L" ORDER BY ";
        sql += expr;
    }

    if (!iter->init(sql))
    {
        delete iter;
        return xcm::null;
    }


    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t SlSet::getRowCount()
{
    return m_row_count;
}




// -- SlRowInserter class implementation --

SlRowInserter::SlRowInserter(SlSet* set)
{
    m_inserting = false;
    m_stmt = NULL;
    m_set = set;
    m_set->ref();
}

SlRowInserter::~SlRowInserter()
{
    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }


    std::vector<SlRowInserterData>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete[] it->str_data;
    }

    m_set->unref();
}

tango::objhandle_t SlRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<SlRowInserterData>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (0 == wcscasecmp(column_name.c_str(),
                            it->name.c_str()))
        {
            return (tango::objhandle_t)(&(*it));
        }
    }

    return 0;
}

tango::IColumnInfoPtr SlRowInserter::getInfo(tango::objhandle_t column_handle)
{
    SlRowInserterData* r = (SlRowInserterData*)column_handle;
    if (!r)
    {
        return xcm::null;
    }

    return r->colinfo;
}

bool SlRowInserter::startInsert(const std::wstring& col_list)
{
    std::vector<std::wstring> cols;

    tango::IStructurePtr set_structure = m_set->getStructure();

    int i, col_count;

    if (col_list.empty() || col_list == L"*")
    {
        // -- inserting every field --
        tango::IStructurePtr s = m_set->getStructure();
        if (s.isNull())
            return false;
        col_count = s->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
            cols.push_back(colinfo->getName());
        }
    }
     else
    {
        kl::parseDelimitedList(col_list, cols, L',', true);
    }


    col_count = cols.size();

    // -- make sure all of the columns are real --

    for (i = 0; i < col_count; ++i)
    {
        if (!set_structure->getColumnExist(cols[i]))
        {
            return false;
        }
    }


    // -- create the SQL insert statement --
    std::wstring sql;
    sql = L"INSERT INTO ";
    sql += m_set->m_tablename;
    sql += L" (";

    for (i = 0; i < col_count; ++i)
    {
        sql += cols[i];
        if (i+1 < col_count)
            sql += L","; 
    }

    sql += L") VALUES (";

    for (i = 0; i < col_count; ++i)
    {
        sql += L"?";
        if (i+1 < col_count)
            sql += L","; 
    }
    
    sql += L");";





    // -- begin a transaction --
    sqlite3_exec(m_set->m_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);





    std::string ascsql;
    ascsql = kl::tostring(sql);


    if (SQLITE_OK != sqlite3_prepare(m_set->m_db, 
                                     ascsql.c_str(),  // stmt
                                     ascsql.length(),
                                     &m_stmt,
                                     0))
    {
        return false;
    }


    for (i = 0; i < col_count; ++i)
    {
        SlRowInserterData data;

        data.colinfo = set_structure->getColumnInfo(cols[i]);
        data.type = data.colinfo->getType();
        data.length = data.colinfo->getWidth();
        data.idx = i+1;
        data.name = cols[i];
        
        data.buf_len = (data.length * 5) + 1;
        if (data.type == tango::typeDate || 
            data.type == tango::typeDateTime)
        {
            data.buf_len = 30;
        }
        
        data.str_data = new char[data.buf_len];

        m_fields.push_back(data);
    }

    return true;
}

void SlRowInserter::finishInsert()
{
    m_inserting = false;

    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    // -- commit this transaction --
    sqlite3_exec(m_set->m_db, "COMMIT;", NULL, NULL, NULL);

}

bool SlRowInserter::insertRow()
{
    int rc;

    while (1)
    {
        rc = sqlite3_step(m_stmt);
        if (rc == SQLITE_BUSY)
        {
            continue;
        }
         else if (rc == SQLITE_ERROR)
        {
            sqlite3_reset(m_stmt);
            return false;
        }
         else
        {
            break;
        }
    }

    sqlite3_reset(m_stmt);
    return true;
}

bool SlRowInserter::flush()
{
    return false;
}

bool SlRowInserter::putRawPtr(tango::objhandle_t column_handle,
                              const unsigned char* value,
                              int length)
{
    return false;
}

bool SlRowInserter::putString(tango::objhandle_t column_handle,
                              const std::string& value)
{
    SlRowInserterData* r = (SlRowInserterData*)column_handle;
    if (!r)
        return false;

    strncpy(r->str_data, value.c_str(), r->length);
    r->str_data[r->length] = 0;
    
    if (SQLITE_OK != sqlite3_bind_text(m_stmt,
                                       r->idx,
                                       r->str_data,
                                       value.length(),
                                       SQLITE_STATIC
                                       ))
    {
        return false;
    }

    return true;
}

bool SlRowInserter::putWideString(tango::objhandle_t column_handle,
                                  const std::wstring& value)
{
    SlRowInserterData* r = (SlRowInserterData*)column_handle;
    if (!r)
        return false;

    size_t output_buf_size = 0;
    kl::utf8_wtoutf8(r->str_data, r->buf_len, value.c_str(), value.length(), &output_buf_size);
        
    if (SQLITE_OK != sqlite3_bind_text(m_stmt,
                                       r->idx,
                                       r->str_data,
                                       output_buf_size,
                                       SQLITE_STATIC
                                       ))
    {
        return false;
    }

    return true;
}

bool SlRowInserter::putDouble(tango::objhandle_t column_handle,
                              double value)
{
    SlRowInserterData* r = (SlRowInserterData*)column_handle;
    if (!r)
        return false;
        
    if (SQLITE_OK != sqlite3_bind_double(m_stmt,
                                       r->idx,
                                       value))
    {
        return false;
    }

    return true;
}

bool SlRowInserter::putInteger(tango::objhandle_t column_handle,
                               int value)
{
    SlRowInserterData* r = (SlRowInserterData*)column_handle;
    if (!r)
        return false;
        
    if (SQLITE_OK != sqlite3_bind_int(m_stmt,
                                      r->idx,
                                      value))
    {
        return false;
    }

    return true;
}

bool SlRowInserter::putBoolean(tango::objhandle_t column_handle,
                               bool value)
{
    SlRowInserterData* r = (SlRowInserterData*)column_handle;
    if (!r)
        return false;
    r->str_data[0] = value ? '0':'1';
    r->str_data[1] = 0;
    
    if (SQLITE_OK != sqlite3_bind_text(m_stmt,
                                   r->idx,
                                   r->str_data,
                                   -1,
                                   SQLITE_STATIC
                                   ))
    {
        return false;
    }
    
    return true;
}

bool SlRowInserter::putDateTime(tango::objhandle_t column_handle,
                                tango::datetime_t datetime)
{
    SlRowInserterData* r = (SlRowInserterData*)column_handle;
    if (!r)
        return false;

    int y = 0, m = 0, d = 0,
        hh = 0, mm = 0, ss = 0;

    if (datetime > 0)
    {
        julianToDate(datetime >> 32, &y, &m, &d);

        tango::datetime_t time_stamp = (datetime >> 32);
        if (time_stamp)
        {
            hh = (int)(time_stamp / 3600000);
            time_stamp -= (hh * 3600000);
            mm = (int)(time_stamp / 60000);
            time_stamp -= (mm * 60000);
            ss = (int)(time_stamp / 1000);
            time_stamp -= (ss * 1000);
        }
    }



    if (r->type == tango::typeDate)
    {
        sprintf(r->str_data,
                "%04d-%02d-%02d",
                y, m, d);
    }
     else
    {
        sprintf(r->str_data,
                "%04d-%02d-%02d %02d:%02d:%02d",
                y, m, d, hh, mm, ss);
    }
    
    if (SQLITE_OK != sqlite3_bind_text(m_stmt,
                                       r->idx,
                                       r->str_data,
                                       -1,
                                       SQLITE_STATIC
                                       ))
    {
        return false;
    }

    return true;
}

bool SlRowInserter::putNull(tango::objhandle_t column_handle)
{
    SlRowInserterData* r = (SlRowInserterData*)column_handle;
    if (!r)
        return false;
        
    if (SQLITE_OK != sqlite3_bind_null(m_stmt, r->idx))
    {
        return false;
    }

    return true;
}

