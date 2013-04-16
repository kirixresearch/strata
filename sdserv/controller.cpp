/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */

#include "sdserv.h"
#include "controller.h"
#include "request.h"



Controller::Controller()
{
}

Controller::~Controller()
{
    removeAllServerSessionObjects();
}

bool Controller::onRequest(RequestInfo& req)
{
    std::wstring uri = req.getURI();
    uri = kl::beforeFirst(uri, '?');
    if (uri.length() > 0 && uri[uri.length()-1] == '/')
       uri = uri.substr(0, uri.length()-1);
 
 
    // debugging code

    clock_t start = clock(), end;
    static time_t last_time = 0;
    time_t t = time(NULL);
    
    if (t-last_time >= 2) // if more than two seconds have passed, add a blank line
        printf("\n");
    last_time = t;
    
    std::wstring str;
    if (req.getValueExists(L"path"))
        str = req.getValue(L"path");
    else if (req.getValueExists(L"handle"))
        str = req.getValue(L"handle");
        
    if (str.length() > 43)
    {
        int ending_start = (int)str.length();
        ending_start -= 30;
        if (ending_start < 10)
            ending_start = 10;
        str = str.substr(0, 10) + L"..." + str.substr(ending_start,30);
    }
    
    std::wstring apimethod = req.getValue(L"m");
    

    struct tm tm;
    localtime_r(&t, &tm);
    char timestamp[255];
    strftime(timestamp, 255, "%H:%M:%S", &tm);
    printf("%s %-13ls %-44ls", timestamp, apimethod.c_str(), str.c_str());
    // end debugging code
 

    if (apimethod.empty())
    {
        // no api method invoked, invoker wants data
        apiFetchRows(req);
        return true;
    }
 

    //     if (apimethod == L"login")            apiLogin(req);
    //else if (apimethod == L"selectdb")         apiSelectDb(req);
         if (apimethod == L"folderinfo")       apiFolderInfo(req);
    else if (apimethod == L"fileinfo")         apiFileInfo(req);
    else if (apimethod == L"createstream")     apiCreateStream(req);
    else if (apimethod == L"createtable")      apiCreateTable(req);
    else if (apimethod == L"createfolder")     apiCreateFolder(req);
    else if (apimethod == L"movefile")         apiMoveFile(req);
    else if (apimethod == L"renamefile")       apiRenameFile(req);
    else if (apimethod == L"deletefile")       apiDeleteFile(req);
    else if (apimethod == L"openstream")       apiOpenStream(req);
    else if (apimethod == L"readstream")       apiReadStream(req);
    else if (apimethod == L"writestream")      apiWriteStream(req);
    else if (apimethod == L"query")            apiQuery(req);
    else if (apimethod == L"groupquery")       apiGroupQuery(req);
    else if (apimethod == L"describetable")    apiDescribeTable(req);
    else if (apimethod == L"fetchrows")        apiFetchRows(req);
    else if (apimethod == L"insertrows")       apiInsertRows(req);
    else if (apimethod == L"clone")            apiClone(req);
    else if (apimethod == L"close")            apiClose(req);
    else if (apimethod == L"alter")            apiAlter(req);
    else if (apimethod == L"refresh")          apiRefresh(req);
    else if (apimethod == L"startbulkinsert")  apiStartBulkInsert(req);
    else if (apimethod == L"finishbulkinsert") apiFinishBulkInsert(req);
    else if (apimethod == L"bulkinsert")       apiBulkInsert(req);
    else return false;

    end = clock();
    printf("%5d %4dms\n", req.getContentLength(), (end-start));
    
    return true;
}

ServerSessionObject* Controller::getServerSessionObject(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_session_object_mutex);

    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return NULL;

    return it->second;
}

void Controller::addServerSessionObject(const std::wstring& name, ServerSessionObject* obj)
{
    XCM_AUTO_LOCK(m_session_object_mutex);

    m_session_objects[name] = obj;
}

void Controller::removeServerSessionObject(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_session_object_mutex);

    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return;
        
    m_session_objects.erase(it);
}

void Controller::removeAllServerSessionObjects()
{
    XCM_AUTO_LOCK(m_session_object_mutex);

    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();
    
    for (it = m_session_objects.begin(); it != it_end; ++it)
    {
        // free the session objects
        delete it->second;
    }
    
    m_session_objects.clear();
}


std::wstring Controller::createHandle() const
{
    char buf[255];
    snprintf(buf, 255, "%d.%d.%d", (int)time(NULL), (int)clock(), rand());
    return kl::towstring(kl::md5str(buf));
}

static void JsonNodeToColumn(kl::JsonNode& column, tango::IColumnInfoPtr col)
{
    if (column.childExists("name"))
        col->setName(column["name"]);
    
    if (column.childExists("type"))
    {
        std::wstring type = column["type"];
        int ntype = tango::stringToDbtype(type);
        col->setType(ntype);
    }
    
    if (column.childExists("width"))
        col->setWidth(column["width"].getInteger());

    if (column.childExists("scale")) 
        col->setScale(column["scale"].getInteger());
        
    if (column.childExists("expression")) 
        col->setExpression(column["expression"]);
        
    if (column.childExists("calculated")) 
        col->setCalculated(column["calculated"].getBoolean());
    
    
    //col->setColumnOrdinal(i); // add this later if necessary
}



void Controller::returnApiError(RequestInfo& req, const char* msg, const char* code)
{
    kl::JsonNode response;
    response["success"].setBoolean(false);
    response["error_code"] = code;
    response["msg"] = msg;
    req.write(response.toString());
}

tango::IDatabasePtr Controller::getSessionDatabase(RequestInfo& req)
{
    std::wstring base_path = L"/";

    std::map< std::wstring , tango::IDatabasePtr >::iterator it;
    
    {
        XCM_AUTO_LOCK(m_databases_object_mutex);
        it = m_databases.find(base_path);
        if (it != m_databases.end())
            return it->second;
    }

    tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();
    if (dbmgr.isNull())
        return xcm::null;


    std::wstring cstr = g_server.getDatabaseConnectionString(L"/");
    if (cstr.length() == 0)
        return xcm::null;


    tango::IDatabasePtr db = dbmgr->open(cstr);

    if (db.isNull())
        return xcm::null;

    {
        XCM_AUTO_LOCK(m_databases_object_mutex);
        m_databases[base_path] = db;
        return db;
    }
}



void Controller::apiFolderInfo(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;

    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    

    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    kl::JsonNode items = response["items"];

    tango::IFileInfoEnumPtr folder_info = db->getFolderInfo(path);
    if (folder_info.isOk())
    {
        size_t i, cnt = folder_info->size();
        for (i = 0; i < cnt; ++i)
        {
            kl::JsonNode item = items.appendElement();

            tango::IFileInfoPtr finfo = folder_info->getItem(i);
            item["name"] = finfo->getName();

            switch (finfo->getType())
            {
                case tango::filetypeFolder: item["type"] = "folder"; break;
                case tango::filetypeNode: item["type"] = "node";     break;
                case tango::filetypeSet: item["type"] = "table";     break;
                case tango::filetypeStream: item["type"] = "stream"; break;
                default: continue;
            }
            
            switch (finfo->getFormat())
            {
                case tango::formatNative:          item["format"] = "native";            break;
                case tango::formatXbase:           item["format"] = "xbase";             break;
                case tango::formatDelimitedText:   item["format"] = "delimited_text";    break;
                case tango::formatFixedLengthText: item["format"] = "fixedlength_text";  break;
                case tango::formatText:            item["format"] = "fixed_length_text"; break;
                default: item["type"] = "unknown"; break;
            }

            item["mime_type"] = finfo->getMimeType();
            item["is_mount"].setBoolean(finfo->isMount());
            item["primary_key"] = finfo->getPrimaryKey();
            item["size"] = (double)finfo->getSize();
        }
    }
     else
    {
        returnApiError(req, "Path does not exist");
        return;
    }

    req.write(response.toString());
}



void Controller::apiFileInfo(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    

    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    kl::JsonNode file_info = response["file_info"];

    tango::IFileInfoPtr finfo = db->getFileInfo(path);
    if (finfo.isOk())
    {
        file_info["name"] = finfo->getName();
        
        switch (finfo->getType())
        {
            case tango::filetypeFolder: file_info["type"] = "folder"; break;
            case tango::filetypeNode: file_info["type"] = "node";     break;
            case tango::filetypeSet: file_info["type"] = "table";     break;
            case tango::filetypeStream: file_info["type"] = "stream"; break;
            default: file_info["type"] = "unknown"; break;
        }

        switch (finfo->getFormat())
        {
            case tango::formatNative:          file_info["format"] = "native";            break;
            case tango::formatXbase:           file_info["format"] = "xbase";             break;
            case tango::formatDelimitedText:   file_info["format"] = "delimited_text";    break;
            case tango::formatFixedLengthText: file_info["format"] = "fixedlength_text";  break;
            case tango::formatText:            file_info["format"] = "fixed_length_text"; break;
            default: file_info["type"] = "unknown"; break;
        }

        file_info["mime_type"] = finfo->getMimeType();
        file_info["is_mount"].setBoolean(finfo->isMount());
        file_info["primary_key"] = finfo->getPrimaryKey();
        file_info["size"] = (double)finfo->getSize();
        file_info["object_id"] = finfo->getObjectId();

        if (finfo->getType() == tango::filetypeSet)
        {
            if (finfo->getFlags() & tango::sfFastRowCount)
            {
                file_info["row_count"] = (double)finfo->getRowCount();
                file_info["fast_row_count"].setBoolean(true);
            }
        }
    }
     else
    {
        returnApiError(req, "Path does not exist");
        return;
    }

    req.write(response.toString());
}


void Controller::apiCreateStream(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    std::wstring mime_type = req.getValue(L"mime_type");
    
    if (!db->createStream(path, mime_type))
    {
        returnApiError(req, "Cannot create object");
        return;
    }
    
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);

    req.write(response.toString());
}

void Controller::apiCreateTable(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    if (!req.getValueExists(L"columns"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    std::wstring s_columns = req.getValue(L"columns");
    
    kl::JsonNode columns;
    columns.fromString(s_columns);
    
    tango::IStructurePtr structure = db->createStructure();

    int i, cnt = columns.getChildCount();
    for (i = 0; i < cnt; ++i)
    {
        tango::IColumnInfoPtr col = structure->createColumn();
        
        kl::JsonNode column = columns[i];
        std::wstring type = column["type"];
        int ntype = tango::stringToDbtype(type);

        col->setName(column["name"]);
        col->setType(ntype);
        col->setWidth(column["width"].getInteger());
        col->setScale(column["scale"].getInteger());
        col->setColumnOrdinal(i);
        col->setExpression(column["expression"]);
        col->setCalculated(column["expression"].getString().length() > 0 ? true : false);
    }

    if (!db->createTable(path, structure, NULL))
    {
        returnApiError(req, "Cannot create table");
        return;
    }

    
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["path"] = path;
    
    req.write(response.toString());
}


void Controller::apiCreateFolder(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    
    if (db->createFolder(path))
    {
        // return success to caller
        kl::JsonNode response;
        response["success"].setBoolean(true);
        req.write(response.toString());
    }
     else
    {
        returnApiError(req, "Could not create folder");
    }
}



void Controller::apiMoveFile(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;

    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    if (!req.getValueExists(L"destination"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    std::wstring destination = req.getValue(L"destination");

    if (db->moveFile(path,destination))
    {
        // return success to caller
        kl::JsonNode response;
        response["success"].setBoolean(true);
        req.write(response.toString());
    }
     else
    {
        returnApiError(req, "Could not move file");
    }
}



void Controller::apiRenameFile(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    if (!req.getValueExists(L"new_name"))
    {
        returnApiError(req, "Missing new_name parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    std::wstring new_name = req.getValue(L"new_name");

    if (db->renameFile(path, new_name))
    {
        // return success to caller
        kl::JsonNode response;
        response["success"].setBoolean(true);
        req.write(response.toString());
    }
     else
    {
        returnApiError(req, "Could not rename file");
    }
}


void Controller::apiDeleteFile(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    
    if (db->deleteFile(path))
    {
        // return success to caller
        kl::JsonNode response;
        response["success"].setBoolean(true);
        req.write(response.toString());
    }
     else
    {
        returnApiError(req, "Could not delete file");
    }
}


void Controller::apiOpenStream(RequestInfo& req)
{
/*
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    
    tango::IStreamPtr stream = db->openStream(path);
    if (stream.isNull())
    {
        returnApiError(req, "Cannot open object");
        return;
    }
    
    // add object to session
    std::wstring handle = createHandle();
    session->streams[handle] = stream;
        
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    
    req.write(response.toString());
*/
}


void Controller::apiReadStream(RequestInfo& req)
{
/*
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
    
    if (!req.getValueExists(L"handle"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
   
    if (!req.getValueExists(L"read_size"))
    {
        returnApiError(req, "Missing read_size parameter");
        return;
    }
    
    std::wstring handle = req.getValue(L"handle");
    std::wstring read_size = req.getValue(L"read_size");
    
    std::map<std::wstring, tango::IStreamPtr>::iterator it;
    it = session->streams.find(handle);
    if (it == session->streams.end())
    {
        returnApiError(req, "Invalid handle");
        return;
    }
    
    tango::IStreamPtr stream = it->second;


    unsigned long read_size_n = (unsigned long)kl::wtoi(read_size);
    char* raw_buf = new char[read_size_n];
    unsigned long raw_buf_len = 0;
    bool res = stream->read(raw_buf, read_size_n, &raw_buf_len);
    
    if (!res || raw_buf_len == 0)
    {
        delete[] raw_buf;
        returnApiError(req, "Read error/eof");
        return;
    }
    
    char* base64_buf = new char[(raw_buf_len+2)*4];
    
    kl::base64_encodestate state;
    kl::base64_init_encodestate(&state);
    int l1 = kl::base64_encode_block(raw_buf, raw_buf_len, base64_buf, &state);
    int l2 = kl::base64_encode_blockend(base64_buf+l1, &state);
    int base64_len = l1+l2;
    if (base64_len > 0 && base64_buf[base64_len-1] == '\n')
        base64_len--;
    base64_buf[base64_len] = 0;


    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["data"] = base64_buf;
    
    delete[] base64_buf;
    
    req.write(response.toString());
*/
}


void Controller::apiWriteStream(RequestInfo& req)
{
/*
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
    
    if (!req.getValueExists(L"handle"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
   
    if (!req.getValueExists(L"write_size"))
    {
        returnApiError(req, "Missing read_size parameter");
        return;
    }
    
    std::wstring handle = req.getValue(L"handle");
    std::wstring s_write_size = req.getValue(L"write_size");
    std::wstring data = req.getValue(L"data");
    
    int write_size = kl::wtoi(s_write_size);
    
    std::map<std::wstring, tango::IStreamPtr>::iterator it;
    it = session->streams.find(handle);
    if (it == session->streams.end())
    {
        returnApiError(req, "Invalid handle");
        return;
    }
    
    tango::IStreamPtr stream = it->second;


    std::string content = kl::tostring(data);
    char* buf = new char[content.length()+1];
    kl::base64_decodestate state;
    kl::base64_init_decodestate(&state);
    int len = kl::base64_decode_block(content.c_str(), content.length(), buf, &state);
    buf[len] = 0;

    if (write_size > len)
        write_size = len;
    
    unsigned long written = 0;
    
    if (!stream->write(buf, write_size, &written))
    {
        delete[] buf;
        returnApiError(req, "Write error");
        return;
    }
    
    delete[] buf;
    
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["written"] = (int)written;
    

    req.write(response.toString());
    */
}




void Controller::apiQuery(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
        
    if (req.getValue(L"mode") == L"createiterator")
    {
        tango::IIteratorPtr iter = db->createIterator(req.getValue(L"path"), req.getValue(L"columns"), req.getValue(L"order"), NULL);
        if (!iter.isOk())
        {
            returnApiError(req, "Could not create iterator");
            return;
        }
        

        // add object to session
        std::wstring handle = createHandle();
        SessionQueryResult* q = new SessionQueryResult;
        q->iter = iter;
        q->rowpos = 1;
        addServerSessionObject(handle, q);
        
        // return success to caller
        kl::JsonNode response;
        response["success"].setBoolean(true);
        response["handle"] = handle;
        
        if (iter.isOk() && (iter->getIteratorFlags() & tango::ifFastRowCount))
        {
            response["row_count"] = (double)iter->getRowCount();
        }

        req.write(response.toString());
    }
     else if (req.getValue(L"mode") == L"sql")
    {
        std::wstring sql = req.getValue(L"sql");
        if (sql.length() == 0)
        {
            returnApiError(req, "Invalid or missing sql parameter");
            return;
        }
        
        xcm::IObjectPtr obj;
        db->execute(sql, 0, obj, NULL);

        tango::IIteratorPtr iter = obj;
        if (iter.isNull())
        {
            returnApiError(req, "SQL syntax error");
            return;
        }
          
        // add object to session
        std::wstring handle = createHandle();
        SessionQueryResult* q = new SessionQueryResult;
        q->iter = iter;
        q->rowpos = 1;
        addServerSessionObject(handle, q);
        
        // return success to caller
        kl::JsonNode response;
        response["success"].setBoolean(true);
        response["handle"] = handle;
        
        if (iter->getIteratorFlags() & tango::ifFastRowCount)
        {
            response["row_count"] = (double)iter->getRowCount();
        }
        
        req.write(response.toString());
    }
     else
    {
        returnApiError(req, "Invalid query mode");
    }
}



void Controller::apiGroupQuery(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;

    std::wstring path = req.getValue(L"path");
    std::wstring output = req.getValue(L"output");
    std::wstring group = req.getValue(L"group");
    std::wstring columns = req.getValue(L"columns");
    std::wstring wherep = req.getValue(L"where");
    std::wstring having = req.getValue(L"having");

    tango::GroupQueryInfo info;
    info.input = path;
    info.output = output;
    info.columns = columns;
    info.group = group;
    info.where = wherep;
    info.having = having;
    
    bool b = db->groupQuery(&info, NULL);
    if (!b)
    {
        returnApiError(req, "Grouping operation failed");
        return;
    }
    
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["path"] = info.output;
    
    req.write(response.toString());
}


void Controller::apiDescribeTable(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    

    kl::JsonNode response;
    
    
    tango::IStructurePtr structure;
        
    std::wstring handle = req.getValue(L"handle");
    std::wstring path = req.getValue(L"path");
    
    if (path.length() > 0)
    {
        structure = db->describeTable(path);
    }
     else if (handle.length() > 0)
    {
        SessionQueryResult* qr = (SessionQueryResult*)getServerSessionObject(handle);
        if (qr && qr->iter.isOk())
        {
            structure = qr->iter->getStructure();
        }
    }
     else
    {
        returnApiError(req, "Missing parameter");
        return;
    }
    
        
    if (structure.isNull())
    {
        returnApiError(req, "Invalid handle");
        return;
    }



    response["success"].setBoolean(true);
    int idx, count = structure->getColumnCount();
    
    // set the items
    kl::JsonNode columns = response["columns"];
    for (idx = 0; idx < count; ++idx)
    {
        kl::JsonNode item = columns.appendElement();
        
        tango::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);
        item["name"] = info->getName();
        item["type"] = tango::dbtypeToString(info->getType());
        item["width"].setInteger(info->getWidth());
        item["scale"].setInteger(info->getScale());
        item["expression"] = info->getExpression();
    }

    req.write(response.toString());
}

static void quotedAppend(std::wstring& str, const std::wstring& cell)
{
    str += '"';

    const wchar_t* ch = cell.c_str();
    while (*ch)
    {
        if (*ch == '"' || *ch == '\\')
            str += L'\\';
        str += *ch;
        ch++;
    }

    str += '"';
}

void Controller::apiFetchRows(RequestInfo& req)
{
    std::wstring handle = req.getValue(L"handle");
    int start = kl::wtoi(req.getValue(L"start", L"-1"));
    int limit = kl::wtoi(req.getValue(L"limit", L"-1"));
    SessionQueryResult* qr = NULL;

    if (handle.empty())
    {
        tango::IDatabasePtr db = getSessionDatabase(req);
        if (db.isNull())
            return;

        tango::IIteratorPtr iter = db->createIterator(req.getURI(), L"", L"", NULL);
        if (!iter.isOk())
        {
            returnApiError(req, "Could not create iterator");
            return;
        }
        

        // add object to session
        handle = createHandle();
        qr = new SessionQueryResult;
        qr->iter = iter;
        qr->rowpos = 1;
        addServerSessionObject(handle, qr);
    }
     else
    {
        // add object to session
        qr = (SessionQueryResult*)getServerSessionObject(handle);
        if (!qr)
        {
            returnApiError(req, "Invalid handle");
            return;
        }
    }
    
    tango::IIterator* iter = qr->iter.p;
    
    if (qr->columns.size() == 0)
    {
        // init handles;
        tango::IStructurePtr s = iter->getStructure();
        for (int i = 0; i < s->getColumnCount(); ++i)
        {
            tango::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
            
            SessionQueryResultColumn qrc;
            qrc.handle = iter->getHandle(colinfo->getName());
            qrc.type = colinfo->getType();
            qrc.width = colinfo->getWidth();
            qrc.scale = colinfo->getScale();
            
            qr->columns.push_back(qrc);
        }
    }
    
    std::wstring str;
    str.reserve((limit>0?limit:100)*180);
    
    if (start != -1)
    {
        if (start == 1)
        {
            iter->goFirst();
            qr->rowpos = 1;
        }
         else
        {
            long long newpos = start;
            newpos -= ((long long)qr->rowpos);
            if (newpos != 0)
            {
                iter->skip((int)newpos);
                qr->rowpos = (tango::rowpos_t)start;
            }
        }
    }

    str = L"{ \"success\": true, \"handle\": \"" + handle + L"\", \"rows\": [ ";
    std::wstring cell;
    
    int row = 0, col, rowcnt = 0;
    for (row = 0; (limit > 0 ? row < limit : true); ++row)
    {
        if (iter->eof())
            break;
        
        if (rowcnt > 0)
            str += L"],[";
             else
            str += L"[";
        
        rowcnt++;
        
        for (col = 0; col < (int)qr->columns.size(); ++col)
        {
            if (col > 0)
                str += L",";
            
            switch (qr->columns[col].type)
            {
                default:
                    cell = iter->getWideString(qr->columns[col].handle);
                    break;
                
                case tango::typeInteger:
                    cell = kl::itowstring(iter->getInteger(qr->columns[col].handle));
                    break;
                
                case tango::typeBoolean:
                    cell = ((iter->getBoolean(qr->columns[col].handle)) ? L"true" : L"false");
                    break;
                
                case tango::typeNumeric:
                case tango::typeDouble:
                {
                    wchar_t buf[255];
                    swprintf(buf, 255, L"%.*f", qr->columns[col].scale, iter->getDouble(qr->columns[col].handle));
                    cell = buf;
                }
                break;
                
                case tango::typeDate:
                {
                    wchar_t buf[64];
                    buf[0] = 0;
                    tango::DateTime dt = iter->getDateTime(qr->columns[col].handle);
                    if (!dt.isNull())
                        swprintf(buf, 64, L"%04d-%02d-%02d", dt.getYear(), dt.getMonth(), dt.getDay());
                    cell = buf;
                }
                break;
                
                case tango::typeDateTime:
                {
                    wchar_t buf[64];
                    buf[0] = 0;
                    tango::DateTime dt = iter->getDateTime(qr->columns[col].handle);
                    if (!dt.isNull())
                        swprintf(buf, 64, L"%04d-%02d-%02d %02d:%02d:%02d", dt.getYear(), dt.getMonth(), dt.getDay(), dt.getHour(), dt.getMinute(), dt.getSecond());
                    cell = buf;
                }
                break;
            }
            
			quotedAppend(str, cell);

        }
        
        iter->skip(1);
        qr->rowpos++;
    }
    
    if (rowcnt == 0)
        str += L"] }";
         else
        str += L"] ] }";
    
    req.write(kl::tostring(str));
}


void Controller::apiInsertRows(RequestInfo& req)
{
/*
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;

    std::wstring path = req.getValue(L"path");
    if (path.length() == 0)
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring source_handle = req.getValue(L"source_handle");
    if (source_handle.length() == 0)
    {
        returnApiError(req, "Missing source_handle parameter");
        return;
    }
    
    std::map<std::wstring, SessionQueryResult>::iterator it;
    it = session->iters.find(source_handle);
    if (it == session->iters.end())
    {
        returnApiError(req, "Invalid source_handle parameter");
        return;
    }


    it->second.iter->goFirst();

    tango::CopyInfo info;
    info.iter_input = it->second.iter;
    info.append = true;
    info.where = req.getValue(L"where");
    if (req.getValueExists(L"max_rows"))
        info.max_rows = kl::wtoi(req.getValue(L"max_rows"));

    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    //response["row_count"].setInteger(row_count);
    
    req.write(response.toString());
    */
}



void Controller::apiClone(RequestInfo& req)
{
/*
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;

    std::wstring handle = req.getValue(L"handle");
    if (handle.length() == 0)
    {
        returnApiError(req, "Missing handle parameter");
        return;
    }
    
    std::map<std::wstring, SessionQueryResult>::iterator it;
    it = session->iters.find(handle);
    if (it == session->iters.end())
    {
        returnApiError(req, "Invalid handle parameter");
        return;
    }
    
    tango::IIteratorPtr iter = it->second.iter->clone();
    if (iter.isNull())
    {
        returnApiError(req, "Clone not supported");
        return;
    }


    std::wstring new_handle = createHandle();
    session->iters[new_handle].iter = iter;
    session->iters[new_handle].rowpos = it->second.rowpos;
    
    
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = new_handle;
    
    req.write(response.toString());
*/
}

void Controller::apiClose(RequestInfo& req)
{
    std::wstring handle = req.getValue(L"handle");
    if (handle.length() == 0)
    {
        returnApiError(req, "Missing handle parameter");
        return;
    }
     
    // add object to session
    SessionQueryResult* qr = NULL;

    {
        XCM_AUTO_LOCK(m_session_object_mutex);

        qr = (SessionQueryResult*)getServerSessionObject(handle);
        if (!qr)
        {
            returnApiError(req, "Invalid handle");
            return;
        }

        removeServerSessionObject(handle);
    }
    
    std::vector<SessionQueryResultColumn>::iterator cit;
    for (cit = qr->columns.begin(); cit != qr->columns.end(); ++cit)
        qr->iter->releaseHandle(cit->handle);

    delete qr;

    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    
    req.write(response.toString());
}



void Controller::apiAlter(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    if (!req.getValueExists(L"actions"))
    {
        returnApiError(req, "Missing actions parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    std::wstring s_actions = req.getValue(L"actions");
    
    
    tango::IStructurePtr structure = db->describeTable(path);
    if (structure.isNull())
    {
        returnApiError(req, "Could not access table");
        return;
    }
    

    
    
    
    kl::JsonNode actions;
    actions.fromString(s_actions);
    
    size_t i, cnt = actions.getChildCount();
    for (i = 0; i < cnt; ++i)
    {
        kl::JsonNode action = actions[i];
        
        if (action["action"].getString() == L"create")
        {
            tango::IColumnInfoPtr colinfo = structure->createColumn();

            kl::JsonNode params = action["params"];
            JsonNodeToColumn(params, colinfo);
        }
         else if (action["action"].getString() == L"insert")
        {
            tango::IColumnInfoPtr colinfo = structure->insertColumn(action["position"].getInteger());
            if (colinfo.isNull())
            {
                returnApiError(req, "Invalid insert position");
                return;
            }
            
            kl::JsonNode params = action["params"];
            JsonNodeToColumn(params, colinfo);
        }
         else if (action["action"].getString() == L"modify")
        {
            tango::IColumnInfoPtr colinfo = structure->modifyColumn(action["target_column"]);
            if (colinfo.isNull())
            {
                returnApiError(req, "Invalid target column for modify operation");
                return;
            }
            
            kl::JsonNode params = action["params"];
            JsonNodeToColumn(params, colinfo);
        }
         else if (action["action"].getString() == L"delete")
        {
            if (!structure->deleteColumn(action["target_column"]))
            {
                returnApiError(req, "Invalid target column for modify operation");
                return;
            }
        }

    }
    
    
    bool res = db->modifyStructure(path, structure, NULL);
    
    // return success/failure to caller
    kl::JsonNode response;
    response["success"].setBoolean(res);
    
    req.write(response.toString());
}




void Controller::apiRefresh(RequestInfo& req)
{
/*
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;

    std::wstring handle = req.getValue(L"handle");
    if (handle.length() == 0)
    {
        returnApiError(req, "Missing handle parameter");
        return;
    }
    

    std::map<std::wstring, SessionQueryResult>::iterator it;
    it = session->iters.find(handle);
    if (it == session->iters.end())
    {
        returnApiError(req, "Invalid handle parameter");
        return;
    }

    it->second.iter->refreshStructure();
    it->second.columns.clear();
    
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);

    req.write(response.toString());
*/
}




static tango::datetime_t parseDateTime(const std::wstring& wstr)
{
    char buf[32];
    int parts[6] = { 0,0,0,0,0,0 };
    size_t len = wstr.length();
    if (len > 30)
        return 0;
        
    std::string str = kl::tostring(wstr);
    strcpy(buf, str.c_str());
    
    size_t i = 0;
    char* start = buf;
    size_t partcnt = 0;
    bool last = false;
    for (i = 0; i <= sizeof(buf); ++i)
    {
        if (buf[i] == '/' || buf[i] == '-' || buf[i] == ':' || buf[i] == '.' || buf[i] == ' ' || buf[i] == 0)
        {
            if (buf[i] == 0)
                last = true;
            buf[i] = 0;
            parts[partcnt++] = atoi(start);
            start = buf+i+1;
            if (partcnt == 6 || last)
                break;
        }
    }

    if (partcnt < 3)
    {
        return 0;
    }
     else if (partcnt == 6)
    {
        tango::DateTime dt(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5]);
        return dt.getDateTime();
    }
     else if (partcnt >= 3)
    {
        tango::DateTime dt(parts[0], parts[1], parts[2]);
        return dt.getDateTime();
    }

    return 0;
}


    
void Controller::apiStartBulkInsert(RequestInfo& req)
{
/*
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
    
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    if (!req.getValueExists(L"columns"))
    {
        returnApiError(req, "Missing columns parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    
    tango::IRowInserterPtr inserter = db->bulkInsert(path);
    tango::IStructurePtr structure = db->describeTable(path);
        
    if (inserter.isNull() || structure.isNull())
    {
        returnApiError(req, "Cannot open table for writing");
        return;
    }
    
    if (!inserter->startInsert(req.getValue(L"columns")))
    {
        returnApiError(req, "Cannot not initialize inserter");
        return;
    }
    

    std::wstring handle = createHandle();
    SessionRowInserter& ri = session->inserters[handle];
    ri.inserter = inserter;
    
    std::vector<std::wstring> cols;
    kl::parseDelimitedList(req.getValue(L"columns"), cols, ',');
    std::vector<std::wstring>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        SessionRowInserterColumn ric;
        ric.handle = inserter->getHandle(*it);
        if (!ric.handle)
        {
            returnApiError(req, "Cannot not initialize inserter");
            return;
        }
        
        tango::IColumnInfoPtr colinfo = structure->getColumnInfo(*it);
        if (colinfo.isNull())
        {
            returnApiError(req, "Cannot not initialize inserter");
            return;
        }
        
        ric.type = colinfo->getType();
        
        ri.columns.push_back(ric);
    }

    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    
    req.write(response.toString());
*/
}


void Controller::apiBulkInsert(RequestInfo& req)
{
/*
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
    
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    

    std::wstring handle = req.getValue(L"handle");

    std::map<std::wstring, SessionRowInserter>::iterator it;
    it = session->inserters.find(handle);
    if (it == session->inserters.end())
    {
        returnApiError(req, "Invalid handle");
        return;
    }
    
    SessionRowInserter& ri = it->second;
    
    std::wstring s_rows = req.getValue(L"rows");
    kl::JsonNode rows;
    rows.fromString(s_rows);
    
    size_t rown, row_cnt = rows.getChildCount();
    size_t coln, col_cnt = ri.columns.size();
    for (rown = 0; rown < row_cnt; ++rown)
    {
        kl::JsonNode row = rows[rown];
        if (row.getChildCount() != col_cnt)
        {
            returnApiError(req, "Column count mismatch");
            return;
        }
        
        for (coln = 0; coln < col_cnt; ++coln)
        {
            kl::JsonNode col = row[coln];
            
            if (col.isNull())
            {
                ri.inserter->putNull(ri.columns[coln].handle);
                continue;
            }
            
            switch (ri.columns[coln].type)
            {
                default:
                case tango::typeUndefined:     break;
                case tango::typeInvalid:       break;
                case tango::typeCharacter:     ri.inserter->putWideString(ri.columns[coln].handle, col.getString()); break; 
                case tango::typeWideCharacter: ri.inserter->putWideString(ri.columns[coln].handle, col.getString()); break;
                case tango::typeNumeric:       ri.inserter->putDouble(ri.columns[coln].handle, kl::nolocale_wtof(col.getString())); break;
                case tango::typeDouble:        ri.inserter->putDouble(ri.columns[coln].handle, kl::nolocale_wtof(col.getString())); break;      break;
                case tango::typeInteger:       ri.inserter->putInteger(ri.columns[coln].handle, kl::wtoi(col.getString())); break;
                case tango::typeDate:          ri.inserter->putDateTime(ri.columns[coln].handle, parseDateTime(col.getString())); break;
                case tango::typeDateTime:      ri.inserter->putDateTime(ri.columns[coln].handle, parseDateTime(col.getString())); break;
                case tango::typeBoolean:       ri.inserter->putBoolean(ri.columns[coln].handle, col.getBoolean()); break;
                case tango::typeBinary:        break;
            }
        }
        
        ri.inserter->insertRow();
    }
    
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    
    req.write(response.toString());
*/
}

void Controller::apiFinishBulkInsert(RequestInfo& req)
{
/*
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
    
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    

    std::wstring handle = req.getValue(L"handle");

    std::map<std::wstring, SessionRowInserter>::iterator it;
    it = session->inserters.find(handle);
    if (it == session->inserters.end())
    {
        returnApiError(req, "Invalid handle");
        return;
    }
            
    it->second.inserter->finishInsert();
    session->inserters.erase(it);
    
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);

    req.write(response.toString());
*/
}
