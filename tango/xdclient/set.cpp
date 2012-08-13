/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */



#include "xdclient.h"
#include "set.h"
#include "iterator.h"
#include "request.h"
#include "jsonutil.h"
#include "kl/url.h"
#include "kl/md5.h"


static std::wstring buildOrderParams(const std::wstring& expr)
{

    return kl::url_encodeURIComponent(expr);
/*
    // TODO: do some type of expression translation to
    // a suitable URL syntax

    std::wstring result;

    std::wstring::const_iterator it, it_end;
    it_end = expr.end();
    
    for (it = expr.begin(); it != it_end; ++it)
    {
        result.push_back(*it);
    }

    return result;
*/
}


ClientSet::ClientSet(ClientDatabase* database)
{
    m_database = database;
    m_database->ref();
    m_set_flags = 0;
    m_known_row_count = (tango::rowpos_t)-1;
}

ClientSet::~ClientSet()
{
    m_database->unref();
}

bool ClientSet::init(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = m_database->serverCall(L"/api/fileinfo", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    JsonNode file_info = response["file_info"];
    if (file_info.isNull() || file_info["type"].getString() != L"table")
        return false;

    JsonNode fast_row_count = file_info["row_count"];
    if (fast_row_count.isOk())
    {
        if (fast_row_count.getBoolean())
            m_set_flags |= tango::sfFastRowCount;
    }

    m_path = path;
    m_tablename = path;

    return true;
}

void ClientSet::setObjectPath(const std::wstring& path)
{
    m_object_path = path;
}

std::wstring ClientSet::getObjectPath()
{
    if (!m_object_path.empty())
        return m_object_path;

    return m_tablename;
}

bool ClientSet::isTemporary()
{
    return false;
}

bool ClientSet::storeObject(const std::wstring& path)
{
    return false;
}

unsigned int ClientSet::getSetFlags()
{
    return m_set_flags;
}

std::wstring ClientSet::getSetId()
{
    return kl::md5str(m_database->m_host + L":" + m_tablename);
}

tango::IStructurePtr ClientSet::getStructure()
{
    if (m_structure.isOk())
        return m_structure;

    ServerCallParams params;
    params.setParam(L"path", m_path);
    std::wstring sres = m_database->serverCall(L"/api/describetable", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    m_structure = m_database->jsonToStructure(response);

    return m_structure->clone();
}

bool ClientSet::modifyStructure(tango::IStructure* struct_config, tango::IJob* job)
{
    bool done_flag = false;
    CommonBaseSet::modifyStructure(struct_config, &done_flag);
    return true;
}

tango::IRowInserterPtr ClientSet::getRowInserter()
{
    return static_cast<tango::IRowInserter*>(new ClientRowInserter(m_database, this, m_path));
}

tango::IRowDeleterPtr ClientSet::getRowDeleter()
{
    return xcm::null;
}

int ClientSet::insert(tango::IIteratorPtr source_iter,
                      const std::wstring& where_condition,
                      int max_rows,
                      tango::IJob* job)
{
    return 0;
}

tango::IIndexInfoEnumPtr ClientSet::getIndexEnum()
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* indexes;
    indexes = new xcm::IVectorImpl<tango::IIndexInfoPtr>;
    return indexes;
}

tango::IIndexInfoPtr ClientSet::createIndex(const std::wstring& tag,
                                            const std::wstring& expr,
                                            tango::IJob* job)
{
    return xcm::null;
}
                                 
tango::IIndexInfoPtr ClientSet::lookupIndex(const std::wstring& expr,
                                            bool exact_column_order)
{
    return xcm::null;
}
                                 
bool ClientSet::deleteIndex(const std::wstring& name)
{
    return false;
}

bool ClientSet::renameIndex(const std::wstring& name,
                            const std::wstring& new_name)
{
    return false;
}

tango::IRelationEnumPtr ClientSet::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IRelationPtr ClientSet::createRelation(const std::wstring& tag,
                                              const std::wstring& right_set_path,
                                              const std::wstring& left_expr,
                                              const std::wstring& right_expr)
{
    return xcm::null;
}

tango::IRelationPtr ClientSet::getRelation(const std::wstring& tag)
{
    return xcm::null;
}

int ClientSet::getRelationCount()
{
    return 0;
}

bool ClientSet::deleteRelation(const std::wstring& tag)
{
    return false;
}

bool ClientSet::deleteAllRelations()
{
    return false;
}

tango::IIteratorPtr ClientSet::createIterator(const std::wstring& columns,
                                              const std::wstring& expr,
                                              tango::IJob* job)
{
    ServerCallParams params;
    params.setParam(L"mode", L"createiterator");
    params.setParam(L"path", m_path);
    params.setParam(L"columns", columns);
    params.setParam(L"expr", expr);

    std::wstring sres = m_database->serverCall(L"/api/query", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
    {
        return xcm::null;
    }

    if (response["row_count"].isOk())
    {
        m_known_row_count = (tango::rowpos_t)response["row_count"].getDouble();
    }

    // initialize the iterator
    ClientIterator* iter = new ClientIterator(m_database, this);
    if (!iter->init(response["handle"], L""))
    {
        delete iter;
        return xcm::null;
    }
    
    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t ClientSet::getRowCount()
{
    if (m_known_row_count != (tango::rowpos_t)-1)
    {
        return m_known_row_count;
    }


    ServerCallParams params;
    params.setParam(L"path", m_path);
    std::wstring sres = m_database->serverCall(L"/api/fileinfo", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return 0;
    
    JsonNode file_info = response["file_info"];
    if (file_info.isNull())
        return 0;

    JsonNode row_count = file_info["row_count"];
    if (row_count.isNull())
        return 0;

    return (tango::rowpos_t)row_count.getDouble();
}




ClientRowInserter::ClientRowInserter(ClientDatabase* database, ClientSet* set, const std::wstring& path)
{
    m_database = database;
    m_database->ref();

    m_structure = set->getStructure();

    m_path = path;
    m_inserting = false;
    m_rows = L"";
    m_buffer_row_count = 0;
}

ClientRowInserter::~ClientRowInserter()
{
    m_database->unref();
}

tango::objhandle_t ClientRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<ClientInsertData>::iterator it;

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!wcscasecmp(it->m_col_name.c_str(), column_name.c_str()))
            return (tango::objhandle_t)&(*it);
    }

    return 0;
}

tango::IColumnInfoPtr ClientRowInserter::getInfo(tango::objhandle_t column_handle)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
        return xcm::null;

    return m_structure->getColumnInfo(data->m_col_name);
}



bool ClientRowInserter::putRawPtr(tango::objhandle_t column_handle,
                                   const unsigned char* value,
                                   int length)
{
    return true;
}

static void escapedQuoteCopy(std::wstring& output, const std::string& input)
{
    size_t i, input_length = input.length();

    while (input_length > 0 && isspace((unsigned char)input[input_length-1]))
        input_length--;

    output.reserve((input_length*2)+5);

    output = L"\"";

    for (i = 0; i < input_length; ++i)
    {
        if (input[i] == '\\' || input[i] == '"')
            output += L'\\';

        output += input[i];
    }
    
    output += L'"';
}

bool ClientRowInserter::putString(tango::objhandle_t column_handle,
                                  const std::string& value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
        return false;

    escapedQuoteCopy(data->m_text, value);
    data->m_specified = true;
    return true;
}

bool ClientRowInserter::putWideString(tango::objhandle_t column_handle,
                                     const std::wstring& value)
{
    return putString(column_handle, kl::tostring(value));
}

bool ClientRowInserter::putDouble(tango::objhandle_t column_handle, double value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    wchar_t buf[64];
    swprintf(buf, 64, L"\"%.*f\"", data->m_tango_scale, value);

    data->m_text = buf;
    data->m_specified = true;
    
    return true;
}

bool ClientRowInserter::putInteger(tango::objhandle_t column_handle, int value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;
    
    if (!data)
        return false;

    wchar_t buf[64];
    swprintf(buf, 64, L"\"%d\"", value);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool ClientRowInserter::putBoolean(tango::objhandle_t column_handle, bool value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    wchar_t buf[8];

    if (value)
        swprintf(buf, 8, L"\"%d\"", 1);
          else
        swprintf(buf, 8, L"\"%d\"", 0);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool ClientRowInserter::putDateTime(tango::objhandle_t column_handle,
                                   tango::datetime_t datetime)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    tango::DateTime dt(datetime);

    if (dt.isNull())
    {
        data->m_text = L"null";
        data->m_specified = true;
    }
     else
    {
        wchar_t buf[64];
        swprintf(buf, 64, L"\"%04d/%02d/%02d %02d:%02d:%02d\"", 
                      dt.getYear(), dt.getMonth(), dt.getDay(), 
                      dt.getHour(), dt.getMinute(), dt.getSecond());

        data->m_text = buf;
        data->m_specified = true;
    }

    return true;
}


bool ClientRowInserter::putNull(tango::objhandle_t column_handle)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    data->m_text = L"null";
    data->m_specified = true;

    return true;
}




bool ClientRowInserter::startInsert(const std::wstring& col_list)
{
    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;
    std::wstring field_list;

    kl::parseDelimitedList(col_list, columns, L',');

    if (!wcscmp(col_list.c_str(), L"*"))
    {
        columns.clear();

        int i, col_count = m_structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
            columns.push_back(m_structure->getColumnName(i));
    }


    std::wstring scols;
    size_t c, cn = columns.size();
    for (c = 0; c < cn; ++c)
    {
        if (c > 0) scols += L",";
        scols += columns[c];
    }


    ServerCallParams params;
    params.setParam(L"path", m_path);
    params.setParam(L"columns", scols);
    std::wstring sres = m_database->serverCall(L"/api/startbulkinsert", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    m_handle = response["handle"];




    m_insert_data.clear();

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        tango::IColumnInfoPtr col_info = m_structure->getColumnInfo(*it);

        if (col_info.isNull())
            return false;

        field_list += col_info->getName();

        if (it+1 != columns.end())
            field_list += L",";

        ClientInsertData d;
        d.m_col_name = col_info->getName();
        d.m_tango_type = col_info->getType();
        d.m_tango_width = col_info->getWidth();
        d.m_tango_scale = col_info->getScale();
        d.m_text = L"null";
        d.m_specified = false;

        m_insert_data.push_back(d);
    }


    m_inserting = true;
    m_rows.reserve(16384);
    m_rows = L"[";
    m_buffer_row_count = 0;

    return true;
}

bool ClientRowInserter::insertRow()
{
    // make the insert statement
    std::vector<ClientInsertData>::iterator it;
    std::vector<ClientInsertData>::iterator begin_it = m_insert_data.begin();
    std::vector<ClientInsertData>::iterator end_it = m_insert_data.end();

    if (m_buffer_row_count == 0)
        m_rows += L"[";
         else
        m_rows += L",[";


    for (it = begin_it; it != end_it; ++it)
    {
        if (it != begin_it)
            m_rows += L",";
        m_rows += it->m_text;
    }

    m_rows += L"]";
    m_buffer_row_count++;



    if (m_buffer_row_count == 100)
    {
        if (!flush())
            return false;
    }

    // clear out values for the next row
    for (it = begin_it; it != end_it; ++it)
    {
        it->m_text = L"null";
        it->m_specified = false;
    }

 

    return true;
}

void ClientRowInserter::finishInsert()
{
    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(L"/api/finishbulkinsert", &params);
}

bool ClientRowInserter::flush()
{
    m_rows += L"]";

    ServerCallParams params;
    params.setParam(L"rows", m_rows);
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(L"/api/bulkinsert", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    m_rows = L"[";
    m_buffer_row_count = 0;

    return true;
}

