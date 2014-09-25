/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */

#include "sdservlib.h"
#include "controller.h"
#include "http.h"
#include <ctime>
#include <kl/portable.h>
#include <kl/regex.h>
#include <kl/string.h>
#include <kl/thread.h>
#include <kl/md5.h>
#include <kl/base64.h>
#include <kl/file.h>
#include <kl/json.h>
#include <kl/url.h>



Controller::Controller(Sdserv* sdserv)
{
    m_sdserv = sdserv;

    // don't allow job id of zero
    m_job_info_vec.push_back(xcm::null);
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

    std::wstring method = req.getGetValue(L"m");
    
    invokeApi(uri, method, req);
    return true;
}


void Controller::invokeApi(const std::wstring& uri, const std::wstring& method, RequestInfo& req)
{
    // debugging code

    clock_t start = clock(), end;
    static time_t last_time = 0;
    time_t t = time(NULL);
    
    if (t-last_time >= 2) // if more than two seconds have passed, add a blank line
        printf("\n");
    last_time = t;
    

    std::wstring str = uri;

    /*
    if (req.getValueExists(L"path"))
        str = req.getValue(L"path");
    else if (req.getValueExists(L"handle"))
        str = req.getValue(L"handle");
    else str = uri;
    */

    if (str.length() > 43)
    {
        int ending_start = (int)str.length();
        ending_start -= 30;
        if (ending_start < 10)
            ending_start = 10;
        str = str.substr(0, 10) + L"..." + str.substr(ending_start,30);

    }
    struct tm tm;
    localtime_r(&t, &tm);
    char timestamp[255];
    strftime(timestamp, 255, "%H:%M:%S", &tm);
    printf("%s %-13ls %-44ls", timestamp, method.c_str(), str.c_str());
    // end debugging code
 

    bool handled = false;
    m_sdserv->sigRequest.fire(method, req, &handled);
    if (handled)
    {
        end = clock();
        printf("%5d %4dms\n", req.getContentLength(), (end-start));
        return;
    }

    req.setContentType("application/json");

    //     if (method == L"login")                 apiLogin(req);
    //else if (method == L"selectdb")              apiSelectDb(req);
         if (method == L"folderinfo")            apiFolderInfo(req);
    else if (method == L"fileinfo")              apiFileInfo(req);
    else if (method == L"read" || method == L"") apiRead(req);
    else if (method == L"createstream")          apiCreateStream(req);
    else if (method == L"createtable")           apiCreateTable(req);
    else if (method == L"createfolder")          apiCreateFolder(req);
    else if (method == L"movefile")              apiMoveFile(req);
    else if (method == L"renamefile")            apiRenameFile(req);
    else if (method == L"deletefile")            apiDeleteFile(req);
    else if (method == L"copydata")              apiCopyData(req);
    else if (method == L"openstream")            apiOpenStream(req);
    else if (method == L"readstream")            apiReadStream(req);
    else if (method == L"writestream")           apiWriteStream(req);
    else if (method == L"query")                 apiQuery(req);
    else if (method == L"groupquery")            apiGroupQuery(req);
    else if (method == L"describetable")         apiDescribeTable(req);

    else if (method == L"insertrows")            apiInsertRows(req);
    else if (method == L"clone")                 apiClone(req);
    else if (method == L"close")                 apiClose(req);
    else if (method == L"alter")                 apiAlter(req);
    else if (method == L"load")                  apiLoad(req);
    else if (method == L"importupload")          apiImportUpload(req);
    else if (method == L"importload")            apiImportLoad(req);
    else if (method == L"runjob")                apiRunJob(req);
    else if (method == L"jobinfo")               apiJobInfo(req);
    else if (method == L"initdb")                apiInitDb(req);

    end = clock();
    printf("%5d %4dms\n", req.getContentLength(), (end-start));

}


ServerSessionObject* Controller::getServerSessionObject(const std::wstring& name, const char* type_check)
{
    KL_AUTO_LOCK(m_session_object_mutex);

    if (name.length() == 0)
        return NULL;

    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return NULL;

    if (type_check)
    {
        if (0 != strcmp(type_check, it->second->getType()))
            return NULL;
    }

    return it->second;
}

void Controller::addServerSessionObject(const std::wstring& name, ServerSessionObject* obj)
{
    KL_AUTO_LOCK(m_session_object_mutex);

    m_session_objects[name] = obj;
}

void Controller::removeServerSessionObject(const std::wstring& name)
{
    KL_AUTO_LOCK(m_session_object_mutex);

    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return;
        
    m_session_objects.erase(it);
}

void Controller::removeAllServerSessionObjects()
{
    KL_AUTO_LOCK(m_session_object_mutex);

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

static void JsonNodeToColumn(kl::JsonNode& column, xd::IColumnInfoPtr col)
{
    if (column.childExists("name"))
        col->setName(column["name"]);
    
    if (column.childExists("type"))
    {
        std::wstring type = column["type"];
        int ntype = xd::stringToDbtype(type);
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

xd::IDatabasePtr Controller::getSessionDatabase(RequestInfo& req)
{
    KL_AUTO_LOCK(m_databases_object_mutex);
    
    if (m_database.isOk())
        return m_database;

    if (m_sdserv->m_database.isOk())
    {
        m_database = m_sdserv->m_database;
        return m_database;
    }

    return xcm::null;
}



void Controller::apiInitDb(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);

    if (db.isNull())
    {
        xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
        if (dbmgr.isOk())
        {
            dbmgr->createDatabase(m_connection_string);
            db = dbmgr->open(m_connection_string);
            
            if (db.isOk())
            {
                KL_AUTO_LOCK(m_databases_object_mutex);
                m_sdserv->m_database = db;
                m_database = db;
            }

            printf("creating database: %ls\n", m_connection_string.c_str());
            if (db.isNull())
                printf("...but couldn't open database right away...\n");
        }
    }

    // return success/failure to caller
    kl::JsonNode response;
    response["success"].setBoolean(db.isOk());

    req.write(response.toString());
}



void Controller::apiFolderInfo(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }


    std::wstring path = req.getURI();
    
    // return success to caller
    kl::JsonNode response;

    xd::IFileInfoEnumPtr folder_info = db->getFolderInfo(path);
    if (folder_info.isOk())
    {
        response["success"].setBoolean(true);
        response["items"].setArray();

        kl::JsonNode items = response["items"];


        size_t i, cnt = folder_info->size();
        for (i = 0; i < cnt; ++i)
        {
            kl::JsonNode item = items.appendElement();

            xd::IFileInfoPtr finfo = folder_info->getItem(i);
            item["name"] = finfo->getName();

            switch (finfo->getType())
            {
                case xd::filetypeFolder: item["type"] = "folder"; break;
                case xd::filetypeNode: item["type"] = "node";     break;
                case xd::filetypeTable: item["type"] = "table";     break;
                case xd::filetypeStream: item["type"] = "stream"; break;
                default: continue;
            }
            
            switch (finfo->getFormat())
            {
                case xd::formatDefault:          item["format"] = "native";            break;
                case xd::formatXbase:           item["format"] = "xbase";             break;
                case xd::formatDelimitedText:   item["format"] = "delimited_text";    break;
                case xd::formatFixedLengthText: item["format"] = "fixedlength_text";  break;
                case xd::formatText:            item["format"] = "fixed_length_text"; break;
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
        response["success"].setBoolean(false);

        returnApiError(req, "Path does not exist");
        return;
    }

    req.write(response.toString());
}



void Controller::apiFileInfo(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }
    
    std::wstring path = req.getURI();

    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    kl::JsonNode file_info = response["file_info"];

    xd::IFileInfoPtr finfo = db->getFileInfo(path);
    if (finfo.isOk())
    {
        file_info["name"] = finfo->getName();
        
        switch (finfo->getType())
        {
            case xd::filetypeFolder: file_info["type"] = "folder"; break;
            case xd::filetypeNode: file_info["type"] = "node";     break;
            case xd::filetypeTable: file_info["type"] = "table";     break;
            case xd::filetypeStream: file_info["type"] = "stream"; break;
            default: file_info["type"] = "unknown"; break;
        }

        switch (finfo->getFormat())
        {
            case xd::formatDefault:         file_info["format"] = "default";            break;
            case xd::formatXbase:           file_info["format"] = "xbase";             break;
            case xd::formatDelimitedText:   file_info["format"] = "delimited_text";    break;
            case xd::formatFixedLengthText: file_info["format"] = "fixedlength_text";  break;
            case xd::formatText:            file_info["format"] = "fixed_length_text"; break;
            default: file_info["type"] = "unknown"; break;
        }

        file_info["mime_type"] = finfo->getMimeType();
        file_info["is_mount"].setBoolean(finfo->isMount());
        file_info["primary_key"] = finfo->getPrimaryKey();
        file_info["size"] = (double)finfo->getSize();
        file_info["object_id"] = finfo->getObjectId();
        file_info["url"] = finfo->getUrl();

        if (finfo->getType() == xd::filetypeTable)
        {
            if (finfo->getFlags() & xd::sfFastRowCount)
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


void Controller::apiCreateTable(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    
    if (!req.getValueExists(L"columns"))
    {
        returnApiError(req, "Missing columns parameter");
        return;
    }
    
    std::wstring path = req.getURI();
    std::wstring s_columns = req.getValue(L"columns");
    
    kl::JsonNode columns;
    columns.fromString(s_columns);
    
    xd::IStructurePtr structure = db->createStructure();

    int i, cnt = columns.getChildCount();
    for (i = 0; i < cnt; ++i)
    {
        xd::IColumnInfoPtr col = structure->createColumn();
        
        kl::JsonNode column = columns[i];
        std::wstring type = column["type"];
        int ntype = xd::stringToDbtype(type);

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
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }
    
    std::wstring path = req.getURI();
    
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
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }


    if (!req.getValueExists(L"destination"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getURI();
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
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    
    if (!req.getValueExists(L"new_name"))
    {
        returnApiError(req, "Missing new_name parameter");
        return;
    }
    
    std::wstring path = req.getURI();
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
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }


    std::wstring path = req.getURI();
    
    if (path == L"" || path == L"/")
    {
        returnApiError(req, "Could not delete root");
        return;
    }

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

void Controller::apiCopyData(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    
    xd::CopyParams info;
    info.input = req.getValue(L"input");
    info.output = req.getValue(L"output");
    info.where = req.getValue(L"where");
    info.order = req.getValue(L"order");
    info.limit = kl::wtoi(req.getValue(L"limit"));

    bool res = db->copyData(&info, NULL);

    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(res);
    req.write(response.toString());
}



void Controller::apiCreateStream(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }


    
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


void Controller::apiOpenStream(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    
    std::wstring path = req.getURI();
    
    xd::IStreamPtr stream = db->openStream(path);
    if (stream.isNull())
    {
        returnApiError(req, "Cannot open object");
        return;
    }
    
    // add object to session
    std::wstring handle = createHandle();
    SessionStream* so = new SessionStream;
    so->stream = stream;
    addServerSessionObject(handle, so);
        
    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    
    req.write(response.toString());
}


void Controller::apiReadStream(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

        
    if (!req.getValueExists(L"handle"))
    {
        returnApiError(req, "Missing handle parameter");
        return;
    }
   
    if (!req.getValueExists(L"read_size"))
    {
        returnApiError(req, "Missing read_size parameter");
        return;
    }
    
    std::wstring handle = req.getValue(L"handle");
    std::wstring read_size = req.getValue(L"read_size");
    
    SessionStream* so = (SessionStream*)getServerSessionObject(handle, "SessionStream");
    if (!so)
    {
        returnApiError(req, "Invalid handle");
        return;
    }


    xd::IStreamPtr stream = so->stream;


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
}


void Controller::apiWriteStream(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    
    if (!req.getValueExists(L"handle"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
   
    if (!req.getValueExists(L"write_size"))
    {
        returnApiError(req, "Missing write_size parameter");
        return;
    }
    
    std::wstring handle = req.getValue(L"handle");
    std::wstring s_write_size = req.getValue(L"write_size");
    std::wstring format = req.getValue(L"format");
    int write_size = kl::wtoi(s_write_size);

    if (write_size < 0)
    {
        returnApiError(req, "Invalid write_size parameter");
        return;
    }

    SessionStream* so = (SessionStream*)getServerSessionObject(handle, "SessionStream");
    if (!so)
    {
        returnApiError(req, "Invalid handle");
        return;
    }

    xd::IStreamPtr stream = so->stream;



    std::string content = kl::tostring(req.getValue(L"data"));
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
}




void Controller::apiQuery(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    
        
    if (req.getValue(L"mode") == L"createiterator")
    {
        xd::IIteratorPtr iter = db->query(req.getValue(L"path"),
                                             req.getValue(L"columns"),
                                             req.getValue(L"where"),
                                             req.getValue(L"order"),
                                             NULL);
        if (!iter.isOk())
        {
            returnApiError(req, "Could not create iterator");
            return;
        }
        

        // add object to session
        std::wstring handle = createHandle();
        SessionQueryResult* so = new SessionQueryResult;
        so->iter = iter;
        so->rowpos = 0;
        addServerSessionObject(handle, so);
        
        // return success to caller
        kl::JsonNode response;
        response["success"].setBoolean(true);
        response["handle"] = handle;
        
        if (iter.isOk() && (iter->getIteratorFlags() & xd::ifFastRowCount))
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
        
        
        // TODO: reimplement this

        // translate any urls in the query to table paths
        //std::wstring host = kl::beforeLast(req.getHost(), L':');
        //klregex::wregex r(L"(https|sdservs|http|sdserv)://" + host + L":?[0-9]*/");
        //r.replace(sql, L"/");
        

        xcm::IObjectPtr obj;
        db->execute(sql, 0, obj, NULL);

        xd::IIteratorPtr iter = obj;
        if (iter.isNull())
        {
            returnApiError(req, "SQL syntax error");
            return;
        }
          
        // add object to session
        std::wstring handle = createHandle();
        SessionQueryResult* so = new SessionQueryResult;
        so->iter = iter;
        so->rowpos = 0;
        addServerSessionObject(handle, so);
        
        // return success to caller
        kl::JsonNode response;
        response["success"].setBoolean(true);
        response["handle"] = handle;
        
        if (iter->getIteratorFlags() & xd::ifFastRowCount)
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
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }


    std::wstring path = req.getValue(L"path");
    std::wstring output = req.getValue(L"output");
    std::wstring group = req.getValue(L"group");
    std::wstring columns = req.getValue(L"columns");
    std::wstring wherep = req.getValue(L"where");
    std::wstring having = req.getValue(L"having");

    xd::GroupQueryParams info;
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
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    

    kl::JsonNode response;
    
    
    xd::IStructurePtr structure;
        
    std::wstring handle = req.getValue(L"handle");
    std::wstring path = req.getURI();
    

    if (handle.length() > 0)
    {
        SessionQueryResult* so = (SessionQueryResult*)getServerSessionObject(handle, "SessionQueryResult");
        if (so && so->iter.isOk())
        {
            structure = so->iter->getStructure();
        }
    }
     else
    {
        structure = db->describeTable(path);
    }

        
    if (structure.isNull())
    {
        returnApiError(req, "Invalid resource");
        return;
    }



    response["success"].setBoolean(true);
    int idx, count = structure->getColumnCount();
    
    // set the items
    kl::JsonNode columns = response["columns"];
    for (idx = 0; idx < count; ++idx)
    {
        kl::JsonNode item = columns.appendElement();
        
        xd::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);
        item["name"] = info->getName();
        item["type"] = xd::dbtypeToString(info->getType());
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
    unsigned int i;

    while (*ch)
    {
        if (*ch == '"' || *ch == '\\')
            str += L'\\';

        i = *ch;
        if (i > 65535)
        {
            // omit high unicode
        }
         else if (i > 127)
        {
            str += kl::stdswprintf(L"\\u%04x", i);
        }
         else
        {
            str += *ch;
        }

        ch++;
    }

    str += '"';
}

void Controller::apiRead(RequestInfo& req)
{
    std::wstring handle = req.getValue(L"handle");
    std::vector<SessionQueryResultColumn>* evalvec = NULL;
    int start = kl::wtoi(req.getValue(L"start", L"-1"));
    int limit = kl::wtoi(req.getValue(L"limit", L"-1"));
    bool metadata = (req.getValue(L"metadata") == L"true" ? true : false);
    bool create_handle = (handle == L"create" ? true : false);
    bool use_handle = ((!create_handle && !handle.empty()) ? true : false);
    SessionQueryResult* so = NULL;

    if (limit < 0)
        limit = 0;

    if (handle.empty() || create_handle)
    {
        xd::IDatabasePtr db = getSessionDatabase(req);
        if (db.isNull())
        {
            returnApiError(req, "Could not connect to database");
            return;
        }

        xd::IFileInfoPtr finfo = db->getFileInfo(req.getURI());
        if (finfo.isNull())
        {
            req.sendNotFoundError();
            return;
        }

        if (finfo->getType() == xd::filetypeFolder)
        {
            req.setValue(L"path", req.getURI());
            apiFolderInfo(req);
            return;
        }
         else if (finfo->getType() == xd::filetypeStream)
        {
            xd::IStreamPtr stream = db->openStream(req.getURI());
            if (stream.isNull())
            {
                req.sendNotFoundError();
                return;
            }

            req.setContentType(kl::tostring(finfo->getMimeType()).c_str());
            req.setContentLength(-1);

            char buf[4096];
            unsigned long len;
            while (stream->read(buf, 4096, &len))
                req.writePiece(buf, len);
            return;
        }


        xd::QueryParams qp;
        qp.from = req.getURI();
        qp.columns = req.getValue(L"columns");
        qp.where = req.getValue(L"where");
        qp.order = req.getValue(L"order");

        xd::IIteratorPtr iter = db->query(qp);
        if (!iter.isOk())
        {
            req.sendNotFoundError();
            return;
        }
        

        // add object to session
        if (create_handle)
            handle = createHandle();
        so = new SessionQueryResult;
        so->iter = iter;
        so->rowpos = 0;
        if (iter->getIteratorFlags() & xd::ifFastRowCount)
            so->rowcount = iter->getRowCount();
             else
            so->rowcount = -1;
    }
     else
    {
        // add object to session
        so = (SessionQueryResult*)getServerSessionObject(handle, "SessionQueryResult");
        if (!so)
        {
            returnApiError(req, "Invalid handle");
            return;
        }
    }
    

    so->mutex.lock();



    xd::IIterator* iter = so->iter.p;
    
    if (so->columns.size() == 0)
    {
        // init handles;
        xd::IStructurePtr s = iter->getStructure();
        for (int i = 0; i < s->getColumnCount(); ++i)
        {
            xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
            
            SessionQueryResultColumn qrc;
            qrc.name = colinfo->getName();
            qrc.handle = iter->getHandle(qrc.name);
            qrc.type = colinfo->getType();
            qrc.width = colinfo->getWidth();
            qrc.scale = colinfo->getScale();
            
            so->columns.push_back(qrc);
        }
    }
    
    std::wstring str;
    str.reserve((limit>0?limit:100)*180);
    

    if (start == -1)
    {
        iter->goFirst();
        so->rowpos = 0;
    }
     else
    {
        if (start == 0)
        {
            iter->goFirst();
            so->rowpos = 0;
        }
         else
        {
            long long newpos = start;
            newpos -= ((long long)so->rowpos);
            if (newpos != 0)
            {
                iter->skip((int)newpos);
                so->rowpos = (xd::rowpos_t)start;
            }
        }
    }

    str = L"";

    if (handle.empty())
        str += L"{\"success\": true,";
         else
        str += L"{\"success\": true,\"handle\": \"" + handle + L"\",";

    if (so->rowcount != -1)
        str += L"\"total_count\": \"" +  kl::itowstring((int)so->rowcount) + L"\", ";

    if (metadata)
    {
        xd::IStructurePtr structure = so->iter->getStructure();
        if (structure.isOk())
        {
            int idx, count = structure->getColumnCount();
    
            // set the items
            str += L"\"columns\":[";

            for (idx = 0; idx < count; ++idx)
            {  
                xd::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);

                if (idx > 0)
                    str += L",";

                str += L"{\"name\":";
                quotedAppend(str, info->getName());
                str += L",\"type\":\"" +  xd::dbtypeToString(info->getType()) + L"\"";
                str += L",\"width\":" + kl::itowstring(info->getWidth());
                str += L",\"scale\":" + kl::itowstring(info->getScale());
                str += L",\"expression\":";
                quotedAppend(str, info->getExpression());
                str += L"}";
            }

            str += L"],";
        }
    }



    std::vector<SessionQueryResultColumn>* columns = &so->columns;
    SessionQueryResultColumn* column;

    if (req.getValueExists(L"eval"))
    {
        SessionQueryResultColumn col;
        col.handle = so->iter->getHandle(req.getValue(L"eval"));
        col.name = L"expr";
        if (!col.handle)
        {
            so->mutex.unlock();
            returnApiError(req, "Invalid expression");
            return;
        }
        col.type = so->iter->getType(col.handle);

        evalvec = new std::vector<SessionQueryResultColumn>;
        evalvec->push_back(col);
        columns = evalvec;
    }


    str += L"\"rows\":[";
    
    int row = 0, col, rowcnt = 0;
    for (row = 0; (limit >= 0 ? row < limit : true); ++row)
    {
        if (iter->eof())
            break;
        
        if (rowcnt > 0)
            str += L"},{";
             else
            str += L"{";
        
        rowcnt++;
        
        for (col = 0; col < (int)columns->size(); ++col)
        {
            column = &(*columns)[col];

            if (col > 0)
                str += L",";
            
            quotedAppend(str, column->name);
            str += L":";

            switch (column->type)
            {
                default:
                    quotedAppend(str, iter->getWideString(column->handle));
                    break;
                
                case xd::typeInteger:
                    str += kl::itowstring(iter->getInteger(column->handle));
                    break;
                
                case xd::typeBoolean:
                    quotedAppend(str, iter->getBoolean(column->handle) ? L"true" : L"false");
                    break;
                
                case xd::typeNumeric:
                case xd::typeDouble:
                {
                    wchar_t buf[64];
                    swprintf(buf, 64, L"%.*f", column->scale, iter->getDouble(column->handle));
                    str += buf;
                }
                break;
                
                case xd::typeDate:
                {
                    wchar_t buf[16];
                    buf[0] = 0;
                    xd::DateTime dt = iter->getDateTime(column->handle);
                    if (!dt.isNull())
                        swprintf(buf, 16, L"%04d-%02d-%02d", dt.getYear(), dt.getMonth(), dt.getDay());
                    quotedAppend(str, buf);
                }
                break;
                
                case xd::typeDateTime:
                {
                    wchar_t buf[32];
                    buf[0] = 0;
                    xd::DateTime dt = iter->getDateTime(column->handle);
                    if (!dt.isNull())
                        swprintf(buf, 32, L"%04d-%02d-%02d %02d:%02d:%02d", dt.getYear(), dt.getMonth(), dt.getDay(), dt.getHour(), dt.getMinute(), dt.getSecond());
                    quotedAppend(str, buf);
                }
                break;
            }
        }
        
        iter->skip(1);
        so->rowpos++;
    }
    
    if (rowcnt == 0)
        str += L"]}";
         else
        str += L"}]}";
    
    if (evalvec)
    {
        so->iter->releaseHandle((*evalvec)[0].handle);
        delete evalvec;
    }

    so->mutex.unlock();


    if (create_handle)
    {
        addServerSessionObject(handle, so);
    }
     else
    {
        if (!use_handle)
            delete so;
    }

    req.write(kl::tostring(str));
}




static xd::datetime_t parseDateTime(const std::wstring& wstr)
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
        xd::DateTime dt(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5]);
        return dt.getDateTime();
    }
     else if (partcnt >= 3)
    {
        xd::DateTime dt(parts[0], parts[1], parts[2]);
        return dt.getDateTime();
    }

    return 0;
}


void Controller::apiInsertRows(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }


    std::wstring columns_param, handle;

    if (req.getValueExists(L"columns"))
        columns_param = req.getValue(L"columns");

    if (req.getValueExists(L"handle"))
        handle = req.getValue(L"handle");

    xd::IRowInserterPtr sp_inserter;
    xd::IRowInserter* inserter = NULL;

    std::vector<SessionRowInserterColumn> columns;
    std::vector<SessionRowInserterColumn>* p_columns = &columns;


    if (handle == L"" || handle == L"create")
    {
        xd::IStructurePtr structure = db->describeTable(req.getURI());
        if (structure.isNull())
        {
            returnApiError(req, "Invalid data resource");
            return;
        }


        sp_inserter = db->bulkInsert(req.getURI());
        inserter = sp_inserter.p;


        if (!inserter->startInsert(columns_param))
        {
            returnApiError(req, "Insert operation not allowed");
            return;
        }


        std::vector<SessionRowInserterColumn> columns;

        std::vector<std::wstring> colvec;
        kl::parseDelimitedList(columns_param, colvec, ',');
        std::vector<std::wstring>::iterator it;
        for (it = colvec.begin(); it != colvec.end(); ++it)
        {
            SessionRowInserterColumn ric;
            ric.handle = inserter->getHandle(*it);
            if (!ric.handle)
            {
                returnApiError(req, kl::tostring(L"Cannot initialize inserter (invalid column handle) " + *it).c_str());
                return;
            }
        
            xd::IColumnInfoPtr colinfo = structure->getColumnInfo(*it);
            if (colinfo.isNull())
            {
                returnApiError(req, kl::tostring(L"Cannot initialize inserter (invalid column info) " + *it).c_str());
                return;
            }
        
            ric.type = colinfo->getType();
        
            columns.push_back(ric);
        }



        // add object to session
        handle = createHandle();
        SessionRowInserter* so = new SessionRowInserter;
        so->inserter = sp_inserter;
        so->columns = columns;
        addServerSessionObject(handle, so);
    }
     else
    {
        SessionRowInserter* so = (SessionRowInserter*)getServerSessionObject(handle, "SessionRowInserter");
        if (!so)
        {
            returnApiError(req, "Invalid handle");
            return;
        }

        sp_inserter = so->inserter;
        inserter = sp_inserter.p;
        p_columns = &(so->columns);
    }






    if (req.getValueExists(L"rows"))
    {
        std::wstring s_rows = req.getValue(L"rows");
        kl::JsonNode rows;
        rows.fromString(s_rows);
    
        size_t rown, row_cnt = rows.getChildCount();
        size_t coln, col_cnt = p_columns->size();
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
                    inserter->putNull((*p_columns)[coln].handle);
                    continue;
                }
            
                switch ((*p_columns)[coln].type)
                {
                    default:
                    case xd::typeUndefined:     break;
                    case xd::typeInvalid:       break;
                    case xd::typeCharacter:     inserter->putWideString((*p_columns)[coln].handle, col.getString()); break; 
                    case xd::typeWideCharacter: inserter->putWideString((*p_columns)[coln].handle, col.getString()); break;
                    case xd::typeNumeric:       inserter->putDouble((*p_columns)[coln].handle, kl::nolocale_wtof(col.getString())); break;
                    case xd::typeDouble:        inserter->putDouble((*p_columns)[coln].handle, kl::nolocale_wtof(col.getString())); break;      break;
                    case xd::typeInteger:       inserter->putInteger((*p_columns)[coln].handle, kl::wtoi(col.getString())); break;
                    case xd::typeDate:          inserter->putDateTime((*p_columns)[coln].handle, parseDateTime(col.getString())); break;
                    case xd::typeDateTime:      inserter->putDateTime((*p_columns)[coln].handle, parseDateTime(col.getString())); break;
                    case xd::typeBoolean:       inserter->putBoolean((*p_columns)[coln].handle, col.getBoolean()); break;
                    case xd::typeBinary:        break;
                }
            }
        
            inserter->insertRow();
        }
    }


    if (handle.empty())
        inserter->finishInsert();


    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    req.write(response.toString());
}



void Controller::apiClone(RequestInfo& req)
{
/*
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    
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
    
    xd::IIteratorPtr iter = it->second.iter->clone();
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
    SessionQueryResult* so = NULL;

    {
        KL_AUTO_LOCK(m_session_object_mutex);

        so = (SessionQueryResult*)getServerSessionObject(handle);
        if (!so)
        {
            returnApiError(req, "Invalid handle");
            return;
        }

        removeServerSessionObject(handle);
    }
    
    if (so->isType("SessionQueryResultColumn"))
    {
        std::vector<SessionQueryResultColumn>::iterator cit;
        for (cit = so->columns.begin(); cit != so->columns.end(); ++cit)
            so->iter->releaseHandle(cit->handle);
    }

    if (so->isType("SessionRowInserter"))
    {
        SessionRowInserter* sri = (SessionRowInserter*)so;
        sri->inserter->finishInsert();
    }


    delete so;

    // return success to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    
    req.write(response.toString());
}



void Controller::apiAlter(RequestInfo& req)
{
    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Could not connect to database");
        return;
    }

    
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
    
    
    xd::IStructurePtr structure = db->describeTable(path);
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
            xd::IColumnInfoPtr colinfo = structure->createColumn();

            kl::JsonNode params = action["params"];
            JsonNodeToColumn(params, colinfo);
        }
         else if (action["action"].getString() == L"insert")
        {
            xd::IColumnInfoPtr colinfo = structure->insertColumn(action["position"].getInteger());
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
            xd::IColumnInfoPtr colinfo = structure->modifyColumn(action["target_column"]);
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



void Controller::apiLoad(RequestInfo& req)
{
    std::wstring target_path = req.getURI();


    RequestFileInfo fileinfo = req.getPostFileInfo(L"file");
    if (!fileinfo.isOk())
    {
        returnApiError(req, "Missing file parameter");
        return;
    }


    std::wstring ext = kl::afterLast(fileinfo.post_filename, '.');


    std::wstring data_temp = xf_get_temp_filename(L"load", ext);
    if (!xf_move(fileinfo.temp_filename, data_temp))
    {
        returnApiError(req, "Could not access uploaded file");
        return;
    }



    jobs::IJobPtr job = jobs::createJob(L"application/vnd.kx.load-job");


    std::wstring source_connection = L"Xdprovider=xdfs";
    std::wstring destination_connection = m_connection_string;

    // configure the job parameters
    kl::JsonNode params;

    params["objects"].setArray();
    kl::JsonNode objects = params["objects"];


    // add our table to the import object
    kl::JsonNode object = objects.appendElement();

    object["source_connection"] = source_connection;
    object["destination_connection"] = destination_connection;

    object["source_path"] = data_temp;
    object["destination_path"] = target_path;


    job->setParameters(params.toString());
    job->runJob();
    job->runPostJob();

    // return success/failure to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    
    req.write(response.toString());
}





class ImportUploadPostValue : public PostValueBase
{
public:

    ImportUploadPostValue(xd::IDatabasePtr db, RequestInfo& req) : m_database(db), m_req(req)
    {
    }

    void start()
    {
    }

    bool append(const unsigned char* buf, size_t len)
    {
        if (m_stream.isNull())
        {
            m_target_path = m_req.getValue(L"target_path");
            if (m_target_path.empty())
			{
				m_target_path = kl::getUniqueString();
				m_req.setValue(L"target_path", m_target_path);
			}
            if (m_database.isNull())
                return false;
            std::wstring mime_type = xf_get_mimetype_from_extension(this->getFilename());
            m_database->deleteFile(m_target_path);
            if (!m_database->createStream(m_target_path, mime_type))
                return false;
            m_stream = m_database->openStream(m_target_path);
            if (m_stream.isNull())
                return false;
        }

        unsigned long written = 0;
        if (!m_stream->write(buf, len, &written))
            return false;

        return (written == len);
    }

    virtual void cancelled()
    {
        m_stream.clear();
        m_database->deleteFile(m_target_path);
        m_database.clear();
    }

    virtual void finish()
    {
        m_stream.clear();
        m_database.clear();
    }

private:

    xd::IDatabasePtr m_database;
    xd::IStreamPtr m_stream;
    RequestInfo& m_req;
    std::wstring m_target_path;
};


class ImportUploadPostHook : public PostHookBase
{
public:

    ImportUploadPostHook(Controller* controller, RequestInfo& req) : m_controller(controller), m_req(req)
    {
    }
    
    PostValueBase* onPostValue(const std::wstring& key, const std::wstring& filename)
    {
        if (filename.length() > 0)
            return new ImportUploadPostValue(m_controller->getSessionDatabase(m_req), m_req);

        return NULL;
    }

private:

    Controller* m_controller;
    RequestInfo& m_req;
};



void Controller::apiImportUpload(RequestInfo& req)
{
    req.setPostHook(new ImportUploadPostHook(this, req));
    req.readPost();

    if (req.getError())
    {
        returnApiError(req, "Request cancelled");
        return;
    }

    std::wstring handle = req.getValue(L"target_path");
    if (handle.empty())
    {
        returnApiError(req, "Missing target_path parameter");
        return;
    }

    // return success/failure to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["handle"].setString(handle);
    req.write(response.toString());
}












class JobThread : public kl::thread
{
public:

    JobThread() : kl::thread()
    {
    }

    virtual ~JobThread()
    {
        std::vector<std::wstring>::iterator it;
        for (it = m_to_delete.begin(); it != m_to_delete.end(); ++it)
        {
            xf_remove(*it);
        }
    }

    jobs::IJobPtr addJob(const std::wstring& job_type)
    {
        jobs::IJobPtr job = jobs::createJob(job_type);
        if (job.isOk())
            m_jobs.push_back(job);
        return job;
    }

    void addFileToDelete(const std::wstring& filename)
    {
        m_to_delete.push_back(filename);
    }

    jobs::IJobInfoPtr getJobInfo()
    {
        if (m_jobs.size() == 1)
        {
            return m_jobs[0]->getJobInfo();
        }
         else
        {
            if (m_agg_jobinfo.isNull())
                m_agg_jobinfo = jobs::createAggregateJobInfoObject();
            m_agg_jobinfo->setCurrentJobIndex(0);
            m_agg_jobinfo->setJobCount(m_jobs.size());
            return m_agg_jobinfo;
        }
    }

    unsigned int entry()
    {
        std::vector<jobs::IJobPtr>::iterator it;

        size_t job_idx = 0;

        for (it = m_jobs.begin(); it != m_jobs.end(); ++it, ++job_idx)
        {
			jobs::IJobInfoPtr jobinfo = (*it)->getJobInfo();

            if (m_agg_jobinfo.isOk())
            {
                m_agg_jobinfo->setCurrentJobIndex(job_idx);
                m_agg_jobinfo->setJobCount(m_jobs.size());
				m_agg_jobinfo->setCurrentJobInfo(jobinfo);
            }

            (*it)->runJob();
            (*it)->runPostJob();
            jobinfo->setState(jobs::jobStateFinished);
        }


		if (m_agg_jobinfo.isOk())
        {
			jobs::IJobInfoPtr j = m_agg_jobinfo;
            j->setState(jobs::jobStateFinished);
			j->setFinishTime(time(NULL));
        }

        return 0;
    }

public:

    std::vector<jobs::IJobPtr> m_jobs;
    jobs::IAggregateJobInfoPtr m_agg_jobinfo;
    std::vector<std::wstring> m_to_delete;
};


void Controller::apiImportLoad(RequestInfo& req)
{
    if (!req.getValueExists(L"handle"))
    {
        returnApiError(req, "Missing handle parameter");
        return;
    }

    if (!req.getValueExists(L"target_path"))
    {
        returnApiError(req, "Missing target_path parameter");
        return;
    }

    std::wstring handle = req.getValue(L"handle");
    std::wstring target_path = req.getValue(L"target_path");
    std::wstring target_format = req.getValue(L"target_format");


    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Target database cannot be reached");
        return;
    }


    xd::IFileInfoPtr finfo = db->getFileInfo(handle);
    if (finfo.isNull() || finfo->getType() != xd::filetypeStream)
    {
        returnApiError(req, "Invalid handle");
        return;
    }

    std::wstring mime_type = finfo->getMimeType();
    std::wstring extension = xf_get_extension_from_mimetype(mime_type);
    if (extension == L"")
        extension = L"bin";
    std::wstring datafile = xf_get_temp_filename(L"impload", extension);



    JobThread* job_thread = new JobThread;
    jobs::IJobPtr job;




    // add a job to export the stream
    job = job_thread->addJob(L"application/vnd.kx.load-job");
    if (job.isNull())
    {
        delete job_thread;
        returnApiError(req, "Invalid job type (2)");
        return;
    }

    // configure the job parameters
    {
        kl::JsonNode params;
        params["objects"].setArray();
        kl::JsonNode objects = params["objects"];

        // add our table to the import object
        kl::JsonNode object = objects.appendElement();
        object["source_connection"] = m_connection_string;
        object["destination_connection"] = L"Xdprovider=xdfs";
        object["source_path"] = handle;
        object["destination_path"] = kl::filenameToUrl(datafile);
        object["overwrite"] = true;

        job->setParameters(params.toString());
        job->setDatabase(db);
    }



    // add an import job
    job = job_thread->addJob(L"application/vnd.kx.load-job");
    if (job.isNull())
    {
        delete job_thread;
        returnApiError(req, "Invalid job type (2)");
        return;
    }

    // configure the job parameters
    {
        kl::JsonNode params;
        params["objects"].setArray();
        kl::JsonNode objects = params["objects"];

        // add our table to the import object
        kl::JsonNode object = objects.appendElement();
        object["source_connection"] = L"Xdprovider=xdfs";
        object["destination_connection"] = m_connection_string;
        object["source_path"] = datafile;
        object["destination_path"] = target_path;
        object["overwrite"] = true;

        job->setParameters(params.toString());
        job->setDatabase(db);
    }

    // register job info and start the job
    m_job_info_mutex.lock();
    int job_id = (int)m_job_info_vec.size();
    m_job_info_vec.push_back(job_thread->getJobInfo());
    m_job_info_mutex.unlock();

    job_thread->addFileToDelete(datafile);
    job_thread->create();

    // return success/failure to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["job_id"].setInteger(job_id);
    
    req.write(response.toString());
}









void Controller::apiRunJob(RequestInfo& req)
{
    std::wstring json = req.getValue(L"parameters");
    kl::JsonNode root;
    if (json.empty() || !root.fromString(json))
    {
        returnApiError(req, "Could not parse parameters");
        return;
    }

    std::wstring job_type = root["metadata"]["type"];
    if (!(job_type == L"application/vnd.kx.group-job"))
    {
        returnApiError(req, "Invalid job type");
        return;        
    }

    xd::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
    {
        returnApiError(req, "Database not available");
        return;
    }


    JobThread* job_thread = new JobThread;
    jobs::IJobPtr job = job_thread->addJob(job_type);
    if (job.isNull())
    {
        delete job_thread;
        returnApiError(req, "Invalid job type (2)");
        return;
    }

    std::wstring json_params = root["params"].toString();

    job->setParameters(json_params);
    job->setDatabase(db);
    jobs::IJobInfoPtr job_info = job_thread->getJobInfo();

    m_job_info_mutex.lock();
    int job_id = (int)m_job_info_vec.size();
    m_job_info_vec.push_back(job_info);
    m_job_info_mutex.unlock();

    job_thread->create();

    // return success/failure to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["job_id"].setInteger(job_id);
    
    req.write(response.toString());
}


void Controller::apiJobInfo(RequestInfo& req)
{
    if (!req.getValueExists(L"job_id"))
    {
        returnApiError(req, "Missing job_id parameter");
        return;
    }

    size_t idx = (size_t)kl::wtoi(req.getValue(L"job_id"));


    jobs::IJobInfoPtr job_info;

    m_job_info_mutex.lock();
    if (idx < m_job_info_vec.size())
        job_info = m_job_info_vec[idx];
    m_job_info_mutex.unlock();

    if (job_info.isNull())
    {
        kl::JsonNode response;
        response["success"].setBoolean(false);
        response["msg"].setString(L"Invalid job id");    
        req.write(response.toString());
        return;
    }

    std::wstring status_string = L"running";
    if (job_info->getState() == jobs::jobStateFinished)
        status_string = L"finished";

    // return success/failure to caller
    kl::JsonNode response;
    response["success"].setBoolean(true);
    response["pct"].setInteger((int)job_info->getPercentage());
    response["current_count"].setInteger((int)job_info->getCurrentCount());
    response["status"] = status_string;
    
    req.write(response.toString());
}

