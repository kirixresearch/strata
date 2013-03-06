/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
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
#include "kl/url.h"
#include "kl/md5.h"
#include "../xdcommon/dbfuncs.h"


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
    m_set_id = L"";
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
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    kl::JsonNode file_info = response["file_info"];
    if (file_info.isUndefined() || file_info["type"].getString() != L"table")
        return false;

    kl::JsonNode fast_row_count = file_info["fast_row_count"];
    if (fast_row_count.isOk())
    {
        if (fast_row_count.getBoolean())
        {
            kl::JsonNode row_count = file_info["row_count"];
            if (row_count.isOk())
            {
                m_set_flags |= tango::sfFastRowCount;
                m_known_row_count = row_count.getInteger();
            }
        }
    }



    m_set_id = file_info["object_id"].getString();

    m_path = path;

    return true;
}

void ClientSet::setObjectPath(const std::wstring& path)
{
    m_object_path = path;

    m_set_id = kl::md5str(m_database->m_host + L"/" + m_database->m_database + L"/" + m_set_id);
}

std::wstring ClientSet::getObjectPath()
{
    if (!m_object_path.empty())
        return m_object_path;

    return m_path;
}

bool ClientSet::isTemporary()
{
    if (m_path.find(L"/.temp/") != m_path.npos)
        return true;
         else
        return false;
}

unsigned int ClientSet::getSetFlags()
{
    return m_set_flags;
}

std::wstring ClientSet::getSetId()
{
    if (m_set_id.length() == 0)
        m_set_id = kl::md5str(m_database->m_host + L":" + m_path);

    return m_set_id;
}

tango::IStructurePtr ClientSet::getStructure()
{
    tango::IStructurePtr structure = m_database->describeTable(m_path);
    if (structure.isNull())
        return xcm::null;
    return structure->clone();
}

bool ClientSet::modifyStructure(tango::IStructure* struct_config, tango::IJob* job)
{
    if (!struct_config)
        return false;

    tango::IStructurePtr orig_structure = getStructure();
    if (orig_structure.isNull())
        return false;

    m_database->clearDescribeTableCache(m_path);


    tango::IStructurePtr structure = struct_config;
    IStructureInternalPtr struct_internal = structure;
    if (struct_internal.isNull())
        return false;


    kl::JsonNode json_actions;
    json_actions.setArray();

    std::vector<StructureAction>& actions = struct_internal->getStructureActions();
    std::vector<StructureAction>::iterator it;
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        kl::JsonNode json_action = json_actions.appendElement();
        
        switch (it->m_action)
        {
            case StructureAction::actionCreate:  json_action[L"action"] = L"create"; break;
            case StructureAction::actionModify:  json_action[L"action"] = L"modify"; break;
            case StructureAction::actionDelete:  json_action[L"action"] = L"delete"; break;
            case StructureAction::actionMove:    json_action[L"action"] = L"move";   break;
            case StructureAction::actionInsert:  json_action[L"action"] = L"insert"; break;
        }

        if (it->m_action == StructureAction::actionModify ||
            it->m_action == StructureAction::actionDelete ||
            it->m_action == StructureAction::actionMove ||
            it->m_action == StructureAction::actionInsert)
        {
            json_action[L"target_column"] = it->m_colname;
        }

        if (it->m_action == StructureAction::actionInsert)
        {
            json_action[L"position"] = it->m_pos;
        }


        if (it->m_params.isOk())
        {
            tango::IColumnInfoPtr orig_colinfo = orig_structure->getColumnInfo(it->m_colname);

            if (it->m_params->getName().length() > 0)
                json_action["params"]["name"] = it->m_params->getName();

            if (it->m_params->getType() != -1)
                json_action["params"]["type"] = tango::dbtypeToString(it->m_params->getType());

            if (it->m_params->getWidth() != -1)
                json_action["params"]["width"].setInteger(it->m_params->getWidth());

            if (it->m_params->getScale() != -1)
                json_action["params"]["scale"].setInteger(it->m_params->getScale());   

            if (it->m_params->getExpression().length() > 0)
                json_action["params"]["expression"] = it->m_params->getExpression();

            if (orig_colinfo.isNull() || it->m_params->getCalculated() != orig_colinfo->getCalculated())
                json_action["params"]["calculated"].setBoolean(it->m_params->getCalculated());
        }
    }



    ServerCallParams params;
    params.setParam(L"path", m_path);
    params.setParam(L"actions", json_actions.toString());
    
    std::wstring sres = m_database->serverCall(L"/api/alter", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

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

    IClientIteratorPtr client_iter = source_iter;
    if (client_iter.isOk() && client_iter->getClientDatabase() == m_database)
    {
        // perform the copy on the server

        ServerCallParams params;
        params.setParam(L"path", m_path);
        params.setParam(L"source_handle", client_iter->getHandle());
        params.setParam(L"where", where_condition);
        params.setParam(L"max_rows", kl::itowstring(max_rows));
        std::wstring sres = m_database->serverCall(L"/api/insertrows", &params);
        kl::JsonNode response;
        response.fromString(sres);

        if (!response["success"].getBoolean())
            return 0;

        return response["row_count"].getInteger();
    }
     else
    {
        return xdcmnInsert(source_iter, this, where_condition, max_rows, job);
    }
}


tango::IIteratorPtr ClientSet::createIterator(const std::wstring& columns,
                                              const std::wstring& order,
                                              tango::IJob* job)
{
    ServerCallParams params;
    params.setParam(L"mode", L"createiterator");
    params.setParam(L"path", m_path);
    params.setParam(L"columns", columns);
    params.setParam(L"order", order);

    std::wstring sres = m_database->serverCall(L"/api/query", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
    {
        return xcm::null;
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
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return 0;
    
    kl::JsonNode file_info = response["file_info"];
    if (file_info.isUndefined())
        return 0;

    kl::JsonNode row_count = file_info["row_count"];
    if (row_count.isUndefined())
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
    kl::JsonNode response;
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
    if (m_buffer_row_count > 0)
    {
        flush();
    }

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
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    m_rows = L"[";
    m_buffer_row_count = 0;

    return true;
}

