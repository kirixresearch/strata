/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-05-22
 *
 */


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
#include "drizzle_client.h"
#include "database.h"
#include "set.h"
#include "iterator.h"


// -- DrizzleSet class implementation --

DrizzleSet::DrizzleSet()
{
}

DrizzleSet::~DrizzleSet()
{
}

bool DrizzleSet::init()
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



void DrizzleSet::setObjectPath(const std::wstring& path)
{
    m_ofs_path = path;
    return;
}

std::wstring DrizzleSet::getObjectPath()
{
    if (!m_ofs_path.empty())
        return m_ofs_path;
        
    return m_tablename;
}

bool DrizzleSet::isTemporary()
{
    return false;
}



unsigned int DrizzleSet::getSetFlags()
{
    return 0;
    //return tango::sfFastRowCount;
}

std::wstring DrizzleSet::getSetId()
{
    IDrizzleDatabasePtr mydb = m_database;
    
    std::wstring id = L"xddrizzle:";
    id += mydb->getServer();
    id += L":";
    id += mydb->getDatabase();
    id += L":";
    id += m_tablename;
    
    return kl::md5str(id);
}

tango::IStructurePtr DrizzleSet::getStructure()
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

    wchar_t query[512];
    swprintf(query, 512, L"SELECT * FROM %ls WHERE 1=0", tablename.c_str());


    IDrizzleDatabasePtr mydb = m_database;
    if (mydb.isNull())
        return xcm::null;
    drizzle_con_st* con = mydb->open();
    if (!con)
        return xcm::null;
    
    std::string asc_query = kl::tostring(query);
    
    
    drizzle_return_t ret;
    drizzle_result_st* result;
    result = drizzle_query_str(con, NULL, asc_query.c_str(), &ret);
    if (ret != DRIZZLE_RETURN_OK || result == NULL)
    {
        mydb->setError(tango::errorGeneral, kl::towstring(drizzle_error(m_drizzle)));
        drizzle_con_free(con);
        return xcm::null;
    }
    
    
    ret = drizzle_result_buffer(result);


    // create new tango::IStructure
    Structure* s = new Structure;

    int i = 0;
    drizzle_column_st* col;
    
    drizzle_column_seek(result, 0);
    while ((col = drizzle_column_next(result)))
    {
        int drizzle_type = drizzle_column_type(col);
        int tango_type = drizzle2tangoType(drizzle_type);

        std::wstring wcol_name = kl::towstring(drizzle_column_name(col));

        tango::IColumnInfoPtr colinfo = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        colinfo->setName(wcol_name);
        colinfo->setType(tango_type);
        colinfo->setWidth(drizzle_column_size(col));
        colinfo->setScale(tango_type == tango::typeDouble ? 4 : drizzle_column_decimals(col));
        colinfo->setColumnOrdinal(i);
        
        // limit blob/text fields to 4096 characters (for now) --
        // this seems to be sensible behavior because copies of
        // the table will not clog of the database space-wise
        if ((drizzle_type == DRIZZLE_COLUMN_TYPE_BLOB || drizzle_type == DRIZZLE_COLUMN_TYPE_LONG_BLOB) && colinfo->getWidth() > 4096)
            colinfo->setWidth(4096);

        
        s->addColumn(colinfo);
        
        i++;
    }

    drizzle_result_free(result);
    drizzle_con_free(con);
    
    tango::IStructurePtr structure = static_cast<tango::IStructure*>(s);
    appendCalcFields(structure);
    return structure;
}

bool DrizzleSet::modifyStructure(tango::IStructure* struct_config,
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
                command += createDrizzleFieldString(fieldname,
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

tango::IRowInserterPtr DrizzleSet::getRowInserter()
{
    DrizzleRowInserter* inserter = new DrizzleRowInserter(this);
    return static_cast<tango::IRowInserter*>(inserter);
}

tango::IRowDeleterPtr DrizzleSet::getRowDeleter()
{
    return xcm::null;
}

int DrizzleSet::insert(tango::IIteratorPtr source_iter,
                     const std::wstring& where_condition,
                     int max_rows,
                     tango::IJob* job)
{
    return xdcmnInsert(source_iter, this, where_condition, max_rows, job);
}



tango::IIteratorPtr DrizzleSet::createIterator(const std::wstring& columns,
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

    DrizzleIterator* iter = new DrizzleIterator;
    iter->m_database = m_database;
    iter->m_set = this;
    iter->init(query);
    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t DrizzleSet::getRowCount()
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

    IDrizzleDatabasePtr mydb = m_database;
    if (mydb.isNull())
        return 0;
    drizzle_con_st* con = mydb->open();
    if (!con)
        return 0;

    std::string asc_query = kl::tostring(query);
    
    drizzle_return_t ret;
    drizzle_result_st* result = drizzle_query_str(con, NULL, asc_query.c_str(), &ret);
    if (ret != DRIZZLE_RETURN_OK || result == NULL)
    {
        mydb->setError(tango::errorGeneral, kl::towstring(drizzle_error(m_drizzle)));
        drizzle_con_free(con);
        return false;
    }

    char** row = (char**)drizzle_row_next(result);
    tango::rowpos_t row_count = atoi(row[0]);
    drizzle_result_free(result);
    drizzle_con_free(con);
    
    return row_count;
}



// -- DrizzleRowInserter class implementation --

DrizzleRowInserter::DrizzleRowInserter(DrizzleSet* set)
{
    m_drizzle = set->m_drizzle;
    m_con = NULL;
    
    m_set = set;
    m_set->ref();
    
    m_inserting = false;

    m_asc_insert_stmt.reserve(65535);
    m_insert_stmt.reserve(65535);
    
    tango::IAttributesPtr attr = m_set->m_database->getAttributes();
    m_quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    m_quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);
}


DrizzleRowInserter::~DrizzleRowInserter()
{
    if (m_con)
    {
        drizzle_con_free(m_con);
    }
    
    m_set->unref();
}

tango::objhandle_t DrizzleRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<DrizzleInsertData>::iterator it;

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!wcscasecmp(it->m_col_name.c_str(), column_name.c_str()))
            return (tango::objhandle_t)&(*it);
    }

    return 0;
}

tango::IColumnInfoPtr DrizzleRowInserter::getInfo(tango::objhandle_t column_handle)
{
    DrizzleInsertData* data = (DrizzleInsertData*)column_handle;

    if (!data)
    {
        return xcm::null;
    }

    tango::IStructurePtr structure = m_set->getStructure();
    return structure->getColumnInfo(data->m_col_name);
}


bool DrizzleRowInserter::putRawPtr(tango::objhandle_t column_handle,
                                   const unsigned char* value,
                                   int length)
{
/*
    DrizzleInsertData* data = (DrizzleInsertData*)column_handle;

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

bool DrizzleRowInserter::putString(tango::objhandle_t column_handle,
                                 const std::string& value)
{
    DrizzleInsertData* data = (DrizzleInsertData*)column_handle;

    if (!data)
        return false;

    doubleQuoteCopy(data->m_text, value);
    data->m_specified = true;
    return true;
}

bool DrizzleRowInserter::putWideString(tango::objhandle_t column_handle,
                                     const std::wstring& value)
{
    return putString(column_handle, kl::tostring(value));
}

bool DrizzleRowInserter::putDouble(tango::objhandle_t column_handle, double value)
{
    DrizzleInsertData* data = (DrizzleInsertData*)column_handle;

    wchar_t buf[64];
    swprintf(buf, 64, L"'%.*f'", data->m_tango_scale, value);

    data->m_text = buf;
    data->m_specified = true;
    
    return true;
}

bool DrizzleRowInserter::putInteger(tango::objhandle_t column_handle, int value)
{
    DrizzleInsertData* data = (DrizzleInsertData*)column_handle;
    
    if (!data)
        return false;

    wchar_t buf[64];
    swprintf(buf, 64, L"'%d'", value);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool DrizzleRowInserter::putBoolean(tango::objhandle_t column_handle, bool value)
{
    DrizzleInsertData* data = (DrizzleInsertData*)column_handle;

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

bool DrizzleRowInserter::putDateTime(tango::objhandle_t column_handle,
                                   tango::datetime_t datetime)
{
    DrizzleInsertData* data = (DrizzleInsertData*)column_handle;

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


bool DrizzleRowInserter::putNull(tango::objhandle_t column_handle)
{
    DrizzleInsertData* data = (DrizzleInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    data->m_text = L"NULL";
    data->m_specified = true;

    return true;
}

bool DrizzleRowInserter::startInsert(const std::wstring& col_list)
{
    IDrizzleDatabasePtr mydb = m_set->m_database;
    
    if (mydb.isNull())
        return false;
        
    m_con = mydb->open();
    if (m_con == NULL)
        return false;
    
    tango::IStructurePtr s = m_set->getStructure();

    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;
    std::wstring field_list;

    kl::parseDelimitedList(col_list, columns, L',');


    if (!wcscmp(col_list.c_str(), L"*"))
    {
        columns.clear();

        int i, col_count = s->getColumnCount();

        for (i = 0; i < col_count; ++i)
            columns.push_back(s->getColumnName(i));
    }

    m_insert_data.clear();

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        tango::IColumnInfoPtr col_info = s->getColumnInfo(*it);

        if (col_info.isNull())
            return false;

        field_list += m_quote_openchar;
        field_list += col_info->getName();
        field_list += m_quote_closechar;

        if (it+1 != columns.end())
            field_list += L",";

        DrizzleInsertData d;
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

bool DrizzleRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    // make the insert statement
    std::vector<DrizzleInsertData>::iterator it;
    std::vector<DrizzleInsertData>::iterator begin_it = m_insert_data.begin();
    std::vector<DrizzleInsertData>::iterator end_it = m_insert_data.end();

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
    
    drizzle_return_t ret;
    drizzle_result_st* result;
    result = drizzle_query_str(m_con, NULL, m_asc_insert_stmt.c_str(), &ret);
    
    if (ret != DRIZZLE_RETURN_OK)
    {
        const char* err = drizzle_error(m_drizzle);
        
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


bool DrizzleRowInserter::flush()
{
    return true;
}

void DrizzleRowInserter::finishInsert()
{
    m_inserting = false;
}


