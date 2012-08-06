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
    
         if (uri == L"/api/login")         apiLogin(req);
    else if (uri == L"/api/selectdb")      apiSelectDb(req);
    else if (uri == L"/api/folderinfo")    apiFolderInfo(req);
    else if (uri == L"/api/fileinfo")      apiFileInfo(req);
    else if (uri == L"/api/createstream")  apiCreateStream(req);
    else if (uri == L"/api/openstream")    apiOpenStream(req);
    else if (uri == L"/api/readstream")    apiReadStream(req);
    else if (uri == L"/api/writestream")   apiWriteStream(req);
    else if (uri == L"/api/query")         apiQuery(req);
    else if (uri == L"/api/describetable") apiDescribeTable(req);
    else if (uri == L"/api/fetchrows")     apiFetchRows(req);
    else return false;

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

    session->db = dbmgr->open(L"xdprovider=xdnative;database=C:\\Users\\bwilliams\\Documents\\Gold Prairie Projects\\Default Project;user id=admin;password=;");

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
        
        // return success to caller
        JsonNode response;
        response["success"].setBoolean(true);
        response["handle"] = handle;
        
        req.write(response.toString());
    }
     else
    {
        returnApiError(req, "Invalid query mode");
    }
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
        std::map<std::wstring, QueryResult>::iterator it;
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
    

    std::map<std::wstring, QueryResult>::iterator it;
    it = session->iters.find(handle);
    if (it == session->iters.end())
    {
        returnApiError(req, "Invalid handle");
        return;
    }
    
    QueryResult& qr = it->second;
    tango::IIterator* iter = it->second.iter.p;
    
    if (qr.handles.size() == 0)
    {
        // init handles;
        tango::IStructurePtr s = iter->getStructure();
        for (int i = 0; i < s->getColumnCount(); ++i)
        {
            qr.handles.push_back(iter->getHandle(s->getColumnName(i)));
        }
    }
    
    std::wstring str;
    str.reserve(limit*100);
    
    if (start == 1)
        iter->goFirst();
    
    str = L"{ \"success\": true, \"rows\": [ ";
    
    int row = 0, col;
    for (row = 0; row < limit; ++row)
    {
        if (row != 0)
            str += L"],[";
             else
            str += L"[";
        
        for (col = 0; col < (int)qr.handles.size(); ++col)
        {
            if (col > 0)
                str += L",";
            str += L"\"" + iter->getWideString(qr.handles[col]) + L"\"";
        }
        
        iter->skip(1);
    }
    
    str += L"] ] }";
    
    req.write(kl::tostring(str));
}

