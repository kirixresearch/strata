/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */

#include "sdserv.h"
#include "controller.h"
#include "request.h"
#include "jsonconfig.h"


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
    
    if (uri == L"/api/login")
        printf("\n-- New connection ---------------------------------------------------\n\n");
    
    std::wstring str;
    if (req.getValueExists(L"path"))
        str = req.getValue(L"path");
    else if (req.getValueExists(L"handle"))
        str = req.getValue(L"handle");
    str = str.substr(0,44);
    
    struct tm tm;
    localtime_r(&t, &tm);
    char timestamp[255];
    strftime(timestamp, 255, "%H:%M:%S", &tm);
    printf("%s %-13ls %-44ls", timestamp, uri.substr(5).c_str(), str.c_str());
    // end debugging code
 
 
    
         if (uri == L"/api/login")            apiLogin(req);
    else if (uri == L"/api/selectdb")         apiSelectDb(req);
    else if (uri == L"/api/folderinfo")       apiFolderInfo(req);
    else if (uri == L"/api/fileinfo")         apiFileInfo(req);
    else if (uri == L"/api/readnodefile")     apiReadNodeFile(req);
    else if (uri == L"/api/writenodefile")    apiWriteNodeFile(req);
    else if (uri == L"/api/createstream")     apiCreateStream(req);
    else if (uri == L"/api/createtable")      apiCreateTable(req);
    else if (uri == L"/api/createfolder")     apiCreateFolder(req);
    else if (uri == L"/api/movefile")         apiMoveFile(req);
    else if (uri == L"/api/renamefile")       apiRenameFile(req);
    else if (uri == L"/api/deletefile")       apiDeleteFile(req);
    else if (uri == L"/api/openstream")       apiOpenStream(req);
    else if (uri == L"/api/readstream")       apiReadStream(req);
    else if (uri == L"/api/writestream")      apiWriteStream(req);
    else if (uri == L"/api/query")            apiQuery(req);
    else if (uri == L"/api/groupquery")       apiGroupQuery(req);
    else if (uri == L"/api/describetable")    apiDescribeTable(req);
    else if (uri == L"/api/fetchrows")        apiFetchRows(req);
    else if (uri == L"/api/startbulkinsert")  apiStartBulkInsert(req);
    else if (uri == L"/api/finishbulkinsert") apiFinishBulkInsert(req);
    else if (uri == L"/api/bulkinsert")       apiBulkInsert(req);
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






void Controller::returnApiError(RequestInfo& req, const char* msg, const char* code)
{
    JsonNode response;
    response["success"].setBoolean(false);
    response["error_code"] = code;
    response["msg"] = msg;
    req.write(response.toString());
}

tango::IDatabasePtr Controller::getSessionDatabase(RequestInfo& req)
{
    std::wstring sid = req.getValue(L"sid");
    SdservSession* session = (SdservSession*)getServerSessionObject(sid);
    if (!session)
    {
        returnApiError(req, "Invalid session id");
        return xcm::null;
    }

    if (session->db.isNull())
    {
        returnApiError(req, "No database selected");
        return xcm::null;
    }

    return session->db;
}

SdservSession* Controller::getSdservSession(RequestInfo& req)
{
    std::wstring sid = req.getValue(L"sid");
    SdservSession* session = (SdservSession*)getServerSessionObject(sid);
    if (!session)
    {
        returnApiError(req, "Invalid session id");
        return NULL;
    }

    return session;
}


void Controller::apiLogin(RequestInfo& req)
{
    std::wstring session_id = createHandle();
    
    // create a new session
    SdservSession* session = new SdservSession;
    addServerSessionObject(session_id, session);

    // return success and session information to caller
    JsonNode response;
    response["success"].setBoolean(true);
    response["session_id"] = session_id;
    req.write(response.toString());
}

void Controller::apiSelectDb(RequestInfo& req)
{
    std::wstring sid = req.getValue(L"sid");
    SdservSession* session = (SdservSession*)getServerSessionObject(sid);
    if (!session)
    {
        returnApiError(req, "Invalid session id");
        return;
    }

    tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();
    if (dbmgr.isNull())
    {
        returnApiError(req, "Missing dbmgr component");
        return;
    }

    std::wstring cstr = g_server.getDatabaseConnectionString(L"default");
    if (cstr.length() == 0)
    {
        returnApiError(req, "Database not found");
        return;
    }
    
    
    session->db = dbmgr->open(cstr);

    if (session->db)
    {
        // return success to caller
        JsonNode response;
        response["success"].setBoolean(true);
        req.write(response.toString());
    } 
     else
    {
        returnApiError(req, "Database could not be opened");
        return;
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
    JsonNode response;
    response["success"].setBoolean(true);
    JsonNode items = response["items"];

    tango::IFileInfoEnumPtr folder_info = db->getFolderInfo(path);
    if (folder_info.isOk())
    {
        size_t i, cnt = folder_info->size();
        for (i = 0; i < cnt; ++i)
        {
            JsonNode item = items.appendElement();

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
    JsonNode response;
    response["success"].setBoolean(true);
    JsonNode file_info = response["file_info"];

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
        
        if (finfo->getType() == tango::filetypeSet)
        {
            tango::ISetPtr set = db->openSet(path);
            if (set.isOk() && set->getSetFlags() & tango::sfFastRowCount)
            {
                file_info["row_count"] = (double)set->getRowCount();
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
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    std::wstring mime_type = req.getValue(L"mime_type");
    
    tango::IStreamPtr stream = db->createStream(path, mime_type);
    if (stream.isNull())
    {
        returnApiError(req, "Cannot open object");
        return;
    }
    
    // add object to session
    std::wstring handle = createHandle();
    session->streams[handle] = stream;
        
    // return success to caller
    JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    
    req.write(response.toString());
}

void Controller::apiCreateTable(RequestInfo& req)
{
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
    
    if (!req.getValueExists(L"columns"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    std::wstring s_columns = req.getValue(L"columns");
    
    JsonNode columns;
    columns.fromString(s_columns);
    
    
    
    tango::IStructurePtr structure = db->createStructure();

    int i, cnt = columns.getCount();
    for (i = 0; i < cnt; ++i)
    {
        tango::IColumnInfoPtr col = structure->createColumn();
        
        JsonNode column = columns[i];
        std::wstring type = column["type"];
        int ntype;

             if (type == L"undefined")     ntype = tango::typeUndefined;
        else if (type == L"invalid")       ntype = tango::typeInvalid;
        else if (type == L"character")     ntype = tango::typeCharacter;
        else if (type == L"widecharacter") ntype = tango::typeWideCharacter;
        else if (type == L"numeric")       ntype = tango::typeNumeric;
        else if (type == L"double")        ntype = tango::typeDouble;
        else if (type == L"integer")       ntype = tango::typeInteger;
        else if (type == L"date")          ntype = tango::typeDate;
        else if (type == L"datetime")      ntype = tango::typeDateTime;
        else if (type == L"boolean")       ntype = tango::typeBoolean;
        else if (type == L"binary")        ntype = tango::typeBinary;
        else ntype = tango::typeUndefined;

        col->setName(column["name"]);
        col->setType(ntype);
        col->setWidth(column["width"].getInteger());
        col->setScale(column["scale"].getInteger());
        col->setColumnOrdinal(i);
        col->setExpression(column["expression"]);
        col->setCalculated(column["expression"].getString().length() > 0 ? true : false);
    }


    
    tango::ISetPtr set = db->createSet(path, structure, NULL);
    if (set.isNull())
    {
        returnApiError(req, "Cannot create table");
        return;
    }

    // return success to caller
    JsonNode response;
    response["success"].setBoolean(true);
    
    req.write(response.toString());
}


void Controller::apiCreateFolder(RequestInfo& req)
{
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
    
    if (db->createFolder(path))
    {
        // return success to caller
        JsonNode response;
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
    
    SdservSession* session = getSdservSession(req);
    if (!session)
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
        JsonNode response;
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
    
    SdservSession* session = getSdservSession(req);
    if (!session)
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
        JsonNode response;
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
    
    SdservSession* session = getSdservSession(req);
    if (!session)
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
        JsonNode response;
        response["success"].setBoolean(true);
        req.write(response.toString());
    }
     else
    {
        returnApiError(req, "Could not delete file");
    }
}


static void tangoNodeToJsonNode(tango::INodeValuePtr nv, JsonNode& jn)
{
    size_t i, cnt;
    cnt = nv->getChildCount();
    if (cnt > 0)
    {
        for (i = 0; i < cnt; ++i)
        {
            JsonNode child = jn[nv->getChildName(i)];
            tangoNodeToJsonNode(nv->getChildByIdx(i), child);
        }
    }
     else
    {
        jn.setString(nv->getString());
    }
}




static void jsonNodeToTangoNode(JsonNode& jn, tango::INodeValuePtr nv)
{
    std::vector<std::wstring> keys = jn.getChildKeys();

    if (keys.size() > 0)
    {
        std::vector<std::wstring>::iterator it;

        for (it = keys.begin(); it < keys.end(); ++it)
        {
            tango::INodeValuePtr child = nv->createChild(*it);
            JsonNode jchild = jn[*it];
            
            jsonNodeToTangoNode(jchild, child);
        }
    }
     else
    {
        nv->setString(jn.getString());
    }
}


void Controller::apiReadNodeFile(RequestInfo& req)
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
    
    tango::INodeValuePtr nv = db->openNodeFile(path);
    if (nv.isNull())
    {
        returnApiError(req, "Cannot open node file");
        return;
    }
    

 
    // return success to caller
    JsonNode response;
    response["success"].setBoolean(true);
    tangoNodeToJsonNode(nv, response["data"]);

    
    req.write(response.toString());
}

void Controller::apiWriteNodeFile(RequestInfo& req)
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


    JsonNode node;
    if (!node.fromString(req.getValue(L"data")))
    {
        returnApiError(req, "Malformed data");
        return;
    }
    

    db->deleteFile(path);
    
    tango::INodeValuePtr root = db->createNodeFile(path);
    if (root.isNull())
    {
        returnApiError(req, "Could not create node file");
        return;
    }


    jsonNodeToTangoNode(node, root);
    

    // return success to caller
    JsonNode response;
    response["success"].setBoolean(true);
    req.write(response.toString());
}


void Controller::apiOpenStream(RequestInfo& req)
{
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
    JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    
    req.write(response.toString());
}


void Controller::apiReadStream(RequestInfo& req)
{
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
    JsonNode response;
    response["success"].setBoolean(true);
    response["data"] = base64_buf;
    
    delete[] base64_buf;
    
    req.write(response.toString());
}


void Controller::apiWriteStream(RequestInfo& req)
{
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
    JsonNode response;
    response["success"].setBoolean(true);
    response["written"] = (int)written;
    

    req.write(response.toString());
}




void Controller::apiQuery(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
        
    if (req.getValue(L"mode") == L"createiterator")
    {
        tango::ISetPtr set = db->openSet(req.getValue(L"path"));
        if (!set.isOk())
        {
            returnApiError(req, "Invalid path");
            return;
        }
        
        tango::IIteratorPtr iter = set->createIterator(req.getValue(L"columns"), req.getValue(L"expr"), NULL);
        if (!set.isOk())
        {
            returnApiError(req, "Could not create iterator");
            return;
        }
        
        // add object to session
        std::wstring handle = createHandle();
        session->iters[handle].iter = iter;
        session->iters[handle].rowpos = 1;
        
        // return success to caller
        JsonNode response;
        response["success"].setBoolean(true);
        response["handle"] = handle;
        
        if (set.isOk() && (set->getSetFlags() & tango::sfFastRowCount))
        {
            response["row_count"] = (double)set->getRowCount();
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
          
        std::wstring handle = createHandle();
        session->iters[handle].iter = iter;
        session->iters[handle].rowpos = 1;
        
        // return success to caller
        JsonNode response;
        response["success"].setBoolean(true);
        response["handle"] = handle;
        
        
        tango::ISetPtr set = iter->getSet();
        if (set.isOk() && (set->getSetFlags() & tango::sfFastRowCount))
        {
            response["row_count"] = (double)set->getRowCount();
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
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;

    std::wstring path = req.getValue(L"path");
    std::wstring group = req.getValue(L"group");
    std::wstring output = req.getValue(L"output");
    std::wstring wherep = req.getValue(L"where");
    std::wstring having = req.getValue(L"having");

    tango::ISetPtr input_set = db->openSet(path);
    if (input_set.isNull())
    {
        returnApiError(req, "Invalid input path.");
    }
    
    
    tango::ISetPtr set = db->runGroupQuery(input_set, group, output, wherep, having, NULL);
    
    std::wstring handle = createHandle();
    
    set->storeObject(handle);
    
    // return success to caller
    JsonNode response;
    response["success"].setBoolean(true);
    response["path"] = handle;
    
    req.write(response.toString());
}


void Controller::apiDescribeTable(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;


    JsonNode response;
    
    
    tango::IStructurePtr structure;
        
    std::wstring handle = req.getValue(L"handle");
    std::wstring path = req.getValue(L"path");
    
    if (path.length() > 0)
    {
        tango::ISetPtr set = db->openSet(path);
        if (set.isOk())
            structure = set->getStructure();
    }
     else if (handle.length() > 0)
    {
        std::map<std::wstring, SessionQueryResult>::iterator it;
        it = session->iters.find(handle);
        if (it != session->iters.end())
            structure = it->second.iter->getStructure();
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
    JsonNode columns = response["columns"];
    for (idx = 0; idx < count; ++idx)
    {
        JsonNode item = columns.appendElement();
        
        tango::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);
        item["name"] = info->getName();
        
        switch (info->getType())
        {
            default:
            case tango::typeUndefined:     item["type"] = "undefined";      break;
            case tango::typeInvalid:       item["type"] = "invalid";        break;
            case tango::typeCharacter:     item["type"] = "character";      break; 
            case tango::typeWideCharacter: item["type"] = "widecharacter";  break;
            case tango::typeNumeric:       item["type"] = "numeric";        break;
            case tango::typeDouble:        item["type"] = "double";         break;
            case tango::typeInteger:       item["type"] = "integer";        break;
            case tango::typeDate:          item["type"] = "date";           break;
            case tango::typeDateTime:      item["type"] = "datetime";       break;
            case tango::typeBoolean:       item["type"] = "boolean";        break;
            case tango::typeBinary:        item["type"] = "binary";         break;
        }
        
        item["width"].setInteger(info->getWidth());
        item["scale"].setInteger(info->getScale());    
        item["expression"] = info->getExpression();    
    }

    req.write(response.toString());
}

void Controller::apiFetchRows(RequestInfo& req)
{
    SdservSession* session = getSdservSession(req);
    if (!session)
        return;
        
    if (!req.getValueExists(L"start") || !req.getValueExists(L"limit"))
    {
        returnApiError(req, "Missing parameter");
        return;
    }
    
    std::wstring handle = req.getValue(L"handle");
    int start = kl::wtoi(req.getValue(L"start"));
    int limit = kl::wtoi(req.getValue(L"limit"));
    

    std::map<std::wstring, SessionQueryResult>::iterator it;
    it = session->iters.find(handle);
    if (it == session->iters.end())
    {
        returnApiError(req, "Invalid handle");
        return;
    }
    
    SessionQueryResult& qr = it->second;
    tango::IIterator* iter = it->second.iter.p;
    
    if (qr.columns.size() == 0)
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
            
            qr.columns.push_back(qrc);
        }
    }
    
    std::wstring str;
    str.reserve(limit*100);
    
    if (start == 1)
    {
        iter->goFirst();
        qr.rowpos = 1;
    }
     else
    {
        tango::tango_int64_t newpos = start;
        newpos -= ((tango::tango_int64_t)qr.rowpos);
        if (newpos != 0)
        {
            iter->skip((int)newpos);
            qr.rowpos = (tango::rowpos_t)start;
        }
    }
    
    str = L"{ \"success\": true, \"rows\": [ ";
    std::wstring cell;
    
    int row = 0, col;
    for (row = 0; row < limit; ++row)
    {
        if (row != 0)
            str += L"],[";
             else
            str += L"[";
        
        for (col = 0; col < (int)qr.columns.size(); ++col)
        {
            if (col > 0)
                str += L",";
            
            switch (qr.columns[col].type)
            {
                default:
                    cell = iter->getWideString(qr.columns[col].handle);
                    break;
                    
                case tango::typeDate:
                {
                    wchar_t buf[64];
                    buf[0] = 0;
                    tango::DateTime dt = iter->getDateTime(qr.columns[col].handle);
                    if (!dt.isNull())
                        swprintf(buf, 64, L"%04d-%02d-%02d", dt.getYear(), dt.getMonth(), dt.getDay());
                    cell = buf;
                }
                break;
                
                case tango::typeDateTime:
                {
                    wchar_t buf[64];
                    buf[0] = 0;
                    tango::DateTime dt = iter->getDateTime(qr.columns[col].handle);
                    if (!dt.isNull())
                        swprintf(buf, 64, L"%04d-%02d-%02d %02d:%02d:%02d", dt.getYear(), dt.getMonth(), dt.getDay(), dt.getHour(), dt.getMinute(), dt.getSecond());
                    cell = buf;
                }
                break;
            }
            
            kl::replaceStr(cell, L"\"", L"\\\"");
            str += L"\"" + cell + L"\"";
        }
        
        iter->skip(1);
        qr.rowpos++;
        
        if (iter->eof())
            break;
    }
    
    str += L"] ] }";
    
    req.write(kl::tostring(str));
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
    
    tango::ISetPtr set = db->openSet(path);
    tango::IRowInserterPtr inserter;
    tango::IStructurePtr structure;
    
    if (set.isOk())
    {
        inserter = set->getRowInserter();
        structure = set->getStructure();
    }
        
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
    JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    
    req.write(response.toString());
}


void Controller::apiBulkInsert(RequestInfo& req)
{
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
    JsonNode rows;
    rows.fromString(s_rows);
    
    size_t rown, row_cnt = rows.getCount();
    size_t coln, col_cnt = ri.columns.size();
    for (rown = 0; rown < row_cnt; ++rown)
    {
        JsonNode row = rows[rown];
        if (row.getCount() != col_cnt)
        {
            returnApiError(req, "Column count mismatch");
            return;
        }
        
        for (coln = 0; coln < col_cnt; ++coln)
        {
            JsonNode col = row[coln];
            
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
    JsonNode response;
    response["success"].setBoolean(true);
    response["handle"] = handle;
    
    req.write(response.toString());
}

void Controller::apiFinishBulkInsert(RequestInfo& req)
{
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
    JsonNode response;
    response["success"].setBoolean(true);

    req.write(response.toString());
}
