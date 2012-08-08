/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-05-22
 *
 */


#ifdef WIN32
#include <windows.h>
#endif


#include "mysql.h"
#include <xcm/xcm.h>
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/md5.h>
#include "tango.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/util.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/extfileinfo.h"
#include "database.h"
#include "set.h"
#include "iterator.h"


// -- MySqlSet class implementation --

MySqlSet::MySqlSet()
{
}

MySqlSet::~MySqlSet()
{
}

bool MySqlSet::init()
{    
    // set the set info filename
    if (m_tablename.length() > 0)
    {
        tango::IAttributesPtr attr = m_database->getAttributes();
        std::wstring definition_path = attr->getStringAttribute(tango::dbattrDefinitionDirectory);

        setConfigFilePath(ExtFileInfo::getConfigFilenameFromSetId(definition_path, getSetId()));
    }
    
    return true;
}



void MySqlSet::setObjectPath(const std::wstring& path)
{
    m_ofs_path = path;
    return;
}

std::wstring MySqlSet::getObjectPath()
{
    if (!m_ofs_path.empty())
        return m_ofs_path;
        
    return m_tablename;
}

bool MySqlSet::isTemporary()
{
    return false;
}

bool MySqlSet::storeObject(const std::wstring& ofs_path)
{
    return false;
}

    

unsigned int MySqlSet::getSetFlags()
{
    return 0;
    //return tango::sfFastRowCount;
}

std::wstring MySqlSet::getSetId()
{
    IMySqlDatabasePtr mydb = m_database;
    
    std::wstring id = L"xdmysql:";
    id += mydb->getServer();
    id += L":";
    id += mydb->getDatabase();
    id += L":";
    id += m_tablename;
    
    return kl::md5str(id);
}

tango::IStructurePtr MySqlSet::getStructure()
{
    // get the quote characters
    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    // create select statement
    std::wstring tablename = L"";
    tablename += quote_openchar;
    tablename += m_tablename;
    tablename += quote_closechar;

    wchar_t query[1024];
    swprintf(query, 1024, L"SELECT * FROM %ls WHERE 1=0", tablename.c_str());


    IMySqlDatabasePtr mydb = m_database;
    if (mydb.isNull())
        return xcm::null;
    MYSQL* db = mydb->open();
    if (!db)
        return xcm::null;
    
    std::string asc_query = kl::tostring(query);
    int error = mysql_query(db, asc_query.c_str());
    
    // create new tango::IStructure
    Structure* s = new Structure;

    if (!error)
    {
        MYSQL_RES* res = mysql_use_result(db);

        int i = 0;
        MYSQL_FIELD* colinfo;
        while((colinfo = mysql_fetch_field(res)))
        {
            int type = mysql2tangoType(colinfo->type);

            std::wstring wcol_name = kl::towstring(colinfo->name);

            tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
            col->setName(wcol_name);
            col->setType(type);
            col->setWidth(colinfo->length);
            col->setScale(type == tango::typeDouble ? 4 : colinfo->decimals);
            col->setColumnOrdinal(i);
            
            // limit blob/text fields to 4096 characters (for now) --
            // this seems to be sensible behavior because copies of
            // the table will not clog of the database space-wise
            if (colinfo->type == FIELD_TYPE_BLOB && colinfo->length > 4096)
                col->setWidth(4096);
  
            
            s->addColumn(col);
            
            i++;
        }

        mysql_free_result(res);
    }

    mysql_close(db);
    
    tango::IStructurePtr ret = static_cast<tango::IStructure*>(s);
    appendCalcFields(ret);
    return ret;
}

bool MySqlSet::modifyStructure(tango::IStructure* struct_config,
                               tango::IJob* job)
{
    XCM_AUTO_LOCK(m_object_mutex);

    bool done_flag = false;
    if (!CommonBaseSet::modifyStructure(struct_config, &done_flag))
        return false;
    if (done_flag)
        return true;
    
    unsigned int processed_action_count = 0;

    tango::IStructurePtr current_struct = getStructure();
    IStructureInternalPtr s = struct_config;
    std::vector<StructureAction>& actions = s->getStructureActions();
    std::vector<StructureAction>::iterator it;

    std::wstring command;
    command.reserve(1024);

    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionDelete)
        {
            command = L"ALTER TABLE ";
            command += quote_openchar;
            command += m_tablename;
            command += quote_closechar;
            command += L" DROP COLUMN ";
            command += quote_openchar;
            command += it->m_colname;
            command += quote_closechar;

            xcm::IObjectPtr result_obj;
            m_database->execute(command, 0, result_obj, NULL);
            command = L"";
        }
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionModify)
        {
        }
    }

    int i;
    int col_count = current_struct->getColumnCount();
    tango::IColumnInfoPtr col_info;
    bool found = false;

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionCreate)
        {
            for (i = 0; i < col_count; ++i)
            {
                col_info = current_struct->getColumnInfoByIdx(i);

                if (!wcscasecmp(col_info->getName().c_str(), it->m_params->getName().c_str()))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::wstring fieldname = L"";
                fieldname += quote_openchar;
                fieldname += it->m_params->getName();
                fieldname += quote_closechar;
            
                command = L"ALTER TABLE ";
                command += quote_openchar;
                command += m_tablename;
                command += quote_closechar;
                command += L" ADD ";
                command += createMySqlFieldString(fieldname,
                                                  it->m_params->getType(),
                                                  it->m_params->getWidth(),
                                                  it->m_params->getScale(), false);

                xcm::IObjectPtr result_obj;
                m_database->execute(command, 0, result_obj, NULL);
                command = L"";
            }

            found = false;
        }
    }

    return true;
}

tango::IRowInserterPtr MySqlSet::getRowInserter()
{
    MySqlRowInserter* inserter = new MySqlRowInserter(this);
    return static_cast<tango::IRowInserter*>(inserter);
}

tango::IRowDeleterPtr MySqlSet::getRowDeleter()
{
    return xcm::null;
}

int MySqlSet::insert(tango::IIteratorPtr source_iter,
                     const std::wstring& where_condition,
                     int max_rows,
                     tango::IJob* job)
{
    return xdcmnInsert(source_iter, this, where_condition, max_rows, job);
}

tango::IIndexInfoEnumPtr MySqlSet::getIndexEnum()
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* indexes;
    indexes = new xcm::IVectorImpl<tango::IIndexInfoPtr>;
    return indexes;
}

tango::IIndexInfoPtr MySqlSet::createIndex(const std::wstring& tag,
                                           const std::wstring& expr,
                                           tango::IJob* job)
{
    return xcm::null;
}

tango::IIndexInfoPtr MySqlSet::lookupIndex(const std::wstring& expr,
                                           bool exact_column_order)
{
    return xcm::null;
}

bool MySqlSet::deleteIndex(const std::wstring& name)
{
    return false;
}

bool MySqlSet::renameIndex(const std::wstring& name,
                           const std::wstring& new_name)
{
    return false;
}

                     
tango::IRelationEnumPtr MySqlSet::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IRelationPtr MySqlSet::createRelation(const std::wstring& tag,
                                             const std::wstring& right_set_path,
                                             const std::wstring& left_expr,
                                             const std::wstring& right_expr)
{
    return xcm::null;
}

tango::IRelationPtr MySqlSet::getRelation(const std::wstring& tag)
{
    return xcm::null;
}

int MySqlSet::getRelationCount()
{
    return xcm::null;
}

bool MySqlSet::deleteRelation(const std::wstring& tag)
{
    return false;
}

bool MySqlSet::deleteAllRelations()
{
    return false;
}

tango::IIteratorPtr MySqlSet::createIterator(const std::wstring& columns,
                                             const std::wstring& expr,
                                             tango::IJob* job)
{
    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    std::wstring query;
    query = L"SELECT * FROM ";
    query += quote_openchar;
    query += m_tablename;
    query += quote_closechar;

    if (expr.length() > 0)
    {
        query += L" ORDER BY ";
        query += expr;
    }

    MySqlIterator* iter = new MySqlIterator;
    iter->m_database = m_database;
    iter->m_set = this;
    iter->init(query);
    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t MySqlSet::getRowCount()
{
    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    // -- create select statement --
    std::wstring query;
    query += L"SELECT COUNT(*) FROM ";
    query += quote_openchar;
    query += m_tablename;
    query += quote_closechar;

    IMySqlDatabasePtr mydb = m_database;
    if (mydb.isNull())
        return 0;
    MYSQL* db = mydb->open();
    if (!db)
        return 0;

    std::string asc_query = kl::tostring(query);
    int error = mysql_query(db, asc_query.c_str());

    if (!error)
    {
        MYSQL_RES* res = mysql_use_result(db);
        MYSQL_ROW row = mysql_fetch_row(res);
        tango::rowpos_t row_count = atoi(row[0]);
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



// -- MySqlRowInserter class implementation --

MySqlRowInserter::MySqlRowInserter(MySqlSet* set)
{
    m_mysql = NULL;
    
    m_set = set;
    m_set->ref();
    
    m_inserting = false;

    m_asc_insert_stmt.reserve(65535);
    m_insert_stmt.reserve(65535);
    
    tango::IAttributesPtr attr = m_set->m_database->getAttributes();
    m_quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    m_quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);
}


MySqlRowInserter::~MySqlRowInserter()
{
    if (m_mysql)
    {
        mysql_close(m_mysql);
        m_mysql = NULL;
    }

    m_set->unref();
}

tango::objhandle_t MySqlRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<MySqlInsertData>::iterator it;

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!wcscasecmp(it->m_col_name.c_str(), column_name.c_str()))
            return (tango::objhandle_t)&(*it);
    }

    return 0;
}

tango::IColumnInfoPtr MySqlRowInserter::getInfo(tango::objhandle_t column_handle)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return xcm::null;
    }

    tango::IStructurePtr structure = m_set->getStructure();
    return structure->getColumnInfo(data->m_col_name);
}


bool MySqlRowInserter::putRawPtr(tango::objhandle_t column_handle,
                                 const unsigned char* value,
                                 int length)
{
/*
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    switch (data->m_tango_type)
    {
        case tango::typeCharacter:
            doubleQuoteCopy(data->m_text, (const char*)value, length);
            return true;
        // FIXME: other tango types should be filled out here
        default:
            return false;
    }
*/

    return true;
}



static void doubleQuoteCopy(std::wstring& output, const std::string& input)
{
    int i;
    int input_length = input.length();

    while (input_length > 0 && isspace((unsigned char)input[input_length-1]))
    {
        input_length--;
    }

    output.reserve((input_length*2)+5);

    output = L"'";

    for (i = 0; i < input_length; ++i)
    {
        output += input[i];
        
        if (input[i] == '\\')
        {
            output += L'\\';
        }
         else if (input[i] == '\'')
        {
            output += L'\'';
        }
    }
    
    output += L'\'';
}

bool MySqlRowInserter::putString(tango::objhandle_t column_handle,
                                 const std::string& value)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    doubleQuoteCopy(data->m_text, value);
    data->m_specified = true;
    return true;
}

bool MySqlRowInserter::putWideString(tango::objhandle_t column_handle,
                                     const std::wstring& value)
{
    return putString(column_handle, kl::tostring(value));
}

bool MySqlRowInserter::putDouble(tango::objhandle_t column_handle, double value)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    wchar_t buf[64];
    swprintf(buf, 64, L"'%.*f'", data->m_tango_scale, value);

    data->m_text = buf;
    data->m_specified = true;
    
    return true;
}

bool MySqlRowInserter::putInteger(tango::objhandle_t column_handle, int value)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;
    
    if (!data)
        return false;

    wchar_t buf[64];
    swprintf(buf, 64, L"'%d'", value);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool MySqlRowInserter::putBoolean(tango::objhandle_t column_handle, bool value)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    wchar_t buf[8];

    if (value)
        swprintf(buf, 8, L"'%d'", 1);
          else
        swprintf(buf, 8, L"'%d'", 0);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool MySqlRowInserter::putDateTime(tango::objhandle_t column_handle,
                                   tango::datetime_t datetime)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    tango::DateTime dt(datetime);

    if (dt.isNull())
    {
        data->m_text = L"NULL";
        data->m_specified = true;
    }
     else
    {
        wchar_t buf[64];
        swprintf(buf, 64, L"'%d/%d/%d %02d:%02d:%02d'", 
                      dt.getYear(), dt.getMonth(), dt.getDay(), 
                      dt.getHour(), dt.getMinute(), dt.getSecond());

        data->m_text = buf;
        data->m_specified = true;
    }

    return true;
}


bool MySqlRowInserter::putNull(tango::objhandle_t column_handle)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    data->m_text = L"NULL";
    data->m_specified = true;

    return true;
}

bool MySqlRowInserter::startInsert(const std::wstring& col_list)
{
    IMySqlDatabasePtr mydb = m_set->m_database;
    
    if (mydb.isNull())
        return false;
        
    m_mysql = mydb->open();
    if (m_mysql == NULL)
        return false;
    
    tango::IStructurePtr s = m_set->getStructure();

    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;
    std::wstring field_list;

    kl::parseDelimitedList(col_list, columns, L',');


    if (!wcscmp(col_list.c_str(), L"*"))
    {
        columns.clear();

        int col_count = s->getColumnCount();
        int i;

        for (i = 0; i < col_count; ++i)
        {
            columns.push_back(s->getColumnName(i));
        }
    }

    m_insert_data.clear();

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        tango::IColumnInfoPtr col_info = s->getColumnInfo(*it);

        if (col_info.isNull())
        {
            return false;
        }

        field_list += m_quote_openchar;
        field_list += col_info->getName();
        field_list += m_quote_closechar;

        if (it+1 != columns.end())
        {
            field_list += L",";
        }

        MySqlInsertData d;
        d.m_col_name = col_info->getName();
        d.m_tango_type = col_info->getType();
        d.m_tango_width = col_info->getWidth();
        d.m_tango_scale = col_info->getScale();
        d.m_text = L"NULL";
        d.m_specified = false;

        m_insert_data.push_back(d);
    }

    m_insert_stub = L"INSERT INTO ";
    m_insert_stub += m_quote_openchar;
    m_insert_stub += m_set->m_tablename;
    m_insert_stub += m_quote_closechar;
    m_insert_stub += L" (";
    m_insert_stub += field_list;
    m_insert_stub += L") VALUES (";

    m_inserting = true;

    return true;
}

bool MySqlRowInserter::insertRow()
{
    if (!m_inserting)
    {
        return false;
    }

    // make the insert statement
    std::vector<MySqlInsertData>::iterator it;
    std::vector<MySqlInsertData>::iterator begin_it = m_insert_data.begin();
    std::vector<MySqlInsertData>::iterator end_it = m_insert_data.end();

    m_insert_stmt = L"";


    size_t fields_specified = 0;
    
    for (it = begin_it; it != end_it; ++it)
    {
        if (it->m_specified)
        {
            if (fields_specified > 0)
                m_insert_stmt += L',';

            m_insert_stmt += it->m_text;
            fields_specified++;
        }
    }

    m_insert_stmt += L')';


    if (fields_specified == m_insert_data.size())
    {
        m_insert_stmt.insert(0, m_insert_stub);
    }
     else
    {
        std::wstring insert_stub = L"INSERT INTO ";
        insert_stub += m_quote_openchar;
        insert_stub += m_set->m_tablename;
        insert_stub += m_quote_closechar;
        insert_stub += L" (";
        for (it = begin_it; it != end_it; ++it)
        {
            if (it->m_specified)
            {
                if (fields_specified > 0)
                    insert_stub += L',';
                insert_stub += m_quote_openchar;
                insert_stub += it->m_col_name;
                insert_stub += m_quote_closechar;
                fields_specified++;
            }
        }
        insert_stub += L") VALUES (";
        m_insert_stmt.insert(0, insert_stub);
    }



    // execute the insert statement
    m_asc_insert_stmt = kl::tostring(m_insert_stmt);
    int error = mysql_real_query(m_mysql,
                                 m_asc_insert_stmt.c_str(),
                                 m_asc_insert_stmt.length());
    if (error != 0)
    {
        const char* err = mysql_error(m_mysql);
        
        return false;
    }


    // clear out values for the next row
    for (it = begin_it; it != end_it; ++it)
    {
        it->m_text = L"NULL";
        it->m_specified = false;
    }

    return true;
}


bool MySqlRowInserter::flush()
{
    return true;
}

void MySqlRowInserter::finishInsert()
{
    m_inserting = false;
}


