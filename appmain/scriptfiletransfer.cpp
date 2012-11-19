/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2008-02-15
 *
 */


#include "appmain.h"
#include "scripthost.h"
#include "scriptfiletransfer.h"
#include "curlutil.h"
#include <curl/curl.h>
#include <kl/file.h>


// -- FileTransferThread class --

class FileTransferThread : public kl::Thread
{
public:

    FileTransfer* m_request;
    int m_method;

    FileTransferThread(FileTransfer* request) : kl::Thread()
    {
        m_request = request;
        m_request->baseRef();
    }

    ~FileTransferThread()
    {
        m_request->baseUnref();
    }

    unsigned int entry()
    {
        m_request->doAction();
        
        // set busy to false
        m_request->m_state_mutex.lock();
        m_request->m_busy = false;
        m_request->m_state_mutex.unlock();

        // fire 'finished' signal
        m_request->fireFinishedEvent();
        
        return 0;
    }
};



// (CLASS) FileTransfer
// Category: Network
// Description: A class for communicating via FTP and SFTP.
// Remarks: FileTransfer allows the user to communicate with SFTP and FTP.  Both the normal
//     FTP and the secure SFTP protocols are supported.

// (EVENT) FileTransfer.finished
// Description: Fired when asynchronous file transfers are finished

FileTransfer::FileTransfer()
{
    m_curl = 0;
    m_async = false;
    m_upload = false;
    m_cancelled = false;
    m_total_bytes = 0.0;
    m_proxy = "";
    m_proxy_port = 0;
}

FileTransfer::~FileTransfer()
{
    if (m_curl)
        curlDestroyHandle(m_curl);
}

// (CONSTRUCTOR) FileTransfer.constructor
// Description: Creates a new FileTransfer object.
//
// Syntax: FileTransfer()
//
// Remarks: Creates a new FileTransfer object.

void FileTransfer::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    initComponent(env);
    
    // get ftp proxy info from the registry
    m_proxy = tostr(getAppPrefsString(wxT("internet.proxy.ftp")));
    m_proxy_port = g_app->getAppPreferences()->getLong(wxT("internet.proxy.ftp_port"), 21);
    
    getMember(L"finished")->setObject(Event::createObject(env));
}

class FileTransferInfo
{
public:

    FileTransferInfo()
    {
        file = 0;
        request = NULL;
    }
    
    FileTransfer* request;
    xf_file_t file;
    std::wstring filename;
};

static int transfer_null_writer_func(void* buffer, size_t size, size_t nmemb, void* _info)
{
    #ifdef _DEBUG
    // let you see the contents in debug
    char* buf = (char*)buffer;
    #endif
    
    return nmemb;
}

static int transfer_writer_func(void* buffer, size_t size, size_t nmemb, void* _info)
{
    FileTransferInfo* info = (FileTransferInfo*)_info;
    
    {
        XCM_AUTO_LOCK(info->request->m_state_mutex);
        if (info->request->m_cancelled)
            return -1;
    }
        
    if (!info->file)
    {
        info->file = xf_open(info->filename, xfCreate, xfWrite, xfShareNone);
        if (!info->file)
            return -1;
    }
    
    
    info->request->m_state_mutex.lock();
    info->request->m_total_bytes += (size*nmemb);
    info->request->m_state_mutex.unlock();


    return xf_write(info->file, buffer, size, nmemb);
}


size_t transfer_reader_func(void* buffer, size_t size, size_t nmemb, void* _info)
{
    FileTransferInfo* info = (FileTransferInfo*)_info;
    
    {
        XCM_AUTO_LOCK(info->request->m_state_mutex);
        if (info->request->m_cancelled)
            return -1;
    }

    if (!info->file)
    {
        info->file = xf_open(info->filename, xfOpen, xfRead, xfShareReadWrite);
        if (!info->file)
            return -1;
    }
    
    info->request->m_state_mutex.lock();
    info->request->m_total_bytes += (size*nmemb);
    info->request->m_state_mutex.unlock();

    return xf_read(info->file, buffer, size, nmemb);
}

static int curl_debug_func(CURL* curl, curl_infotype type, char* msg, size_t size, void*)
{
    if (type != CURLINFO_TEXT)
        return 0;
        
    char message[1024];
    memcpy(message, msg, size);
    message[size] = 0;
    wxLogDebug(wxString::From8BitData(message));
    return 0;
}


// (METHOD) FileTransfer.setAsync
// Description: Turns asynchronous mode on or off
//
// Syntax: function FileTransfer.setAsync(value : Boolean)
//
// Remarks: Turns asynchronous mode on or off.  If asynchronous mode is on,
//     requests will return immediately and the processing will be done in
//     the background.  Upon completion of the request, the |finished| event
//     is fired.
//
// Param(value): Specifying |true| turns on asynchronous mode and |false|
//     turns it off


void FileTransfer::setAsync(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
        m_async = env->getParam(0)->getBoolean();
         else
        m_async = true;
}


void FileTransfer::doAction()
{
    CURLcode res;
    FileTransferInfo info;
    info.request = this;
    info.filename = m_filename;
    m_cancelled = false;
    m_total_bytes = 0.0;
    
    m_curl = curlCreateHandle();
    
    // set the curl proxy info
    res = curl_easy_setopt(m_curl, CURLOPT_PROXY, m_proxy.c_str());
    res = curl_easy_setopt(m_curl, CURLOPT_PROXYPORT, m_proxy_port);
    
    res = curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
    if (m_upload)
    {
        curl_easy_setopt(m_curl, CURLOPT_UPLOAD, TRUE);
        curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, transfer_reader_func);
        curl_easy_setopt(m_curl, CURLOPT_READDATA, &info);
    }
     else
    {
        curl_easy_setopt(m_curl, CURLOPT_UPLOAD, FALSE);
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, transfer_writer_func);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &info);
    }
    
    res = curl_easy_perform(m_curl);
    if (res != CURLE_OK)
        return;
    
    if (info.file)
        xf_close(info.file);
}

// (METHOD) FileTransfer.download
// Description: Downloads a file from an FTP or SFTP server
//
// Syntax: function FileTransfer.download(url : String,
//                                        destination_filename : String) : Boolean
//
// Remarks: Calling this method downloads a file from an FTP or an
//     SFTP server onto the local computer.  The url specified in
//     the |url| parameter takes the following format: 
//     sftp://username:password@servername.domain.com/path/to/filename.ext --
//     If you wish to use the ftp protocol, change the protocol identifier before
//     the first colon in the url string.
//
// Param(url): The remote location to download
// Param(destination_filename): The filename to save, with full path name.
// Returns: Returns true upon successful completion, otherwise false if
//     an error occurred.

void FileTransfer::download(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 2)
        return;

    m_url = kl::tostring(env->getParam(0)->getString());
    m_filename = env->getParam(1)->getString();
    m_upload = false;
    
    if (m_async)
    {
        m_state_mutex.lock();
        m_busy = true;
        m_state_mutex.unlock();
            
        FileTransferThread* thread = new FileTransferThread(this);
        if (thread->create() != 0)
            return;

        retval->setBoolean(true);
    }
     else
    {
        doAction();
    }
    
    retval->setBoolean(true);
}


// (METHOD) FileTransfer.upload
// Description: Uploads a file to an FTP or SFTP server
//
// Syntax: function FileTransfer.upload(source_filename : String,
//                                      destination_url : String) : Boolean
//
// Remarks: Calling this method uploads a file from the local computer to
//     an FTP or an SFTP server.  The destination url specified in
//     the |destination_url| parameter takes the following format: 
//     sftp://username:password@servername.domain.com/path/to/filename.ext --
//     If you wish to use the ftp protocol, change the protocol identifier before
//     the first colon in the url string.
//
// Param(source_filename): The filename to upload
// Param(destination_url): The destination location of the upload
// Returns: Returns true upon successful completion, otherwise false if
//     an error occurred.


void FileTransfer::upload(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 2)
        return;

    m_filename = env->getParam(0)->getString();
    m_url = kl::tostring(env->getParam(1)->getString());
    m_upload = true;

    if (m_async)
    {
        m_state_mutex.lock();
        m_busy = true;
        m_state_mutex.unlock();
            
        FileTransferThread* thread = new FileTransferThread(this);
        if (thread->create() != 0)
            return;

        retval->setBoolean(true);
    }
     else
    {
        doAction();
    }
    
    retval->setBoolean(true);
}

// (METHOD) FileTransfer.isDone
// Description: Returns a value indicating whether the request is done
//
// Syntax: function FileTransfer.isDone() : Boolean
//
// Remarks: Returns a boolean value indicating whether the most recent
//     file transfer operation is finished.
//
// Returns: True if the file transfer is finished, false if it is still running

void FileTransfer::isDone(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(isBusyInternal() ? false : true);
}


std::string getUrlPartBeforeFilename(const std::string& url)
{
    size_t start_pos = 0;
    size_t colon = url.find_first_of("://");
    if (colon != url.npos)
    {
        start_pos = colon+3;
    }
    
    size_t slash = url.find_first_of('/',start_pos);
    if (slash == url.npos)
        return url;
        
    return url.substr(0, slash);
}


// (METHOD) FileTransfer.rename
// Description: Renames a filename on a remote FTP or SFTP server
//
// Syntax: function FileTransfer.rename(source_url : String,
//                                      new_name : String) : Boolean
//
// Remarks: Calling this method renames a file on a remote FTP or SFTP server
//     The source url specified in |source_url| parameter takes the following format: 
//     sftp://username:password@servername.domain.com/path/to/filename.ext --
//     If you wish to use the ftp protocol, change the protocol identifier before
//     the first colon in the url string.  The filename specified in the |new_name|
//     parameter indicates the new name that the file will take.  It should be specified
//     as a simple filename, without a qualifying path name.
//
// Param(source_url): The file to rename on the remote server
// Param(new_name): The new name the file should take
// Returns: Returns true upon successful completion, otherwise false if
//     an error occurred.


void FileTransfer::rename(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 2)
        return;
    
    if (!m_curl)
    {
        // no active url
        return;
    }
    
    CURLcode res;
    struct curl_slist* command_list = NULL;
    

    std::string url = getUrlPartBeforeFilename(m_url);
        

    if (url.npos != url.find("sftp://"))
    {
        // SFTP rename
        std::string command = "RENAME ";
        command += kl::tostring(env->getParam(0)->getString());
        command += " ";
        command += kl::tostring(env->getParam(1)->getString());
        
        command_list = curl_slist_append(command_list, command.c_str());
    }
     else
    {
        // FTP rename
        std::string command1 = "RNFR " + kl::tostring(env->getParam(0)->getString());
        std::string command2 = "RNTO " + kl::tostring(env->getParam(1)->getString());
        
        command_list = curl_slist_append(command_list, command1.c_str());
        command_list = curl_slist_append(command_list, command2.c_str());
    }
    
    res = curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, transfer_null_writer_func);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, 0);

    curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, curl_debug_func);

    curl_easy_setopt(m_curl, CURLOPT_POSTQUOTE, command_list);
    res = curl_easy_perform(m_curl);
    
    curl_slist_free_all(command_list);
     
    if (res != CURLE_OK)
        return;
        
    retval->setBoolean(true);
}

// (METHOD) FileTransfer.getBytesTransferred
// Description: Returns the total number of bytes received or sent.
//
// Syntax: function FileTransfer.getBytesTransferred() :  Number
//
// Remarks: A call to this method returns the total number of bytes
//     received or sent.  This function may also be called during
//     asynchronous transfers.
//
// Returns: Total number of transferred bytes

void FileTransfer::getBytesTransferred(kscript::ExprEnv* env, kscript::Value* retval)
{
    XCM_AUTO_LOCK(m_state_mutex);
    retval->setDouble(m_total_bytes);
}


// (METHOD) FileTransfer.cancel
// Description: Cancels an asynchronous transfer
//
// Syntax: function FileTransfer.cancel()
//
// Remarks: Calling this method causes an asynchronous transfer to abort.

void FileTransfer::cancel(kscript::ExprEnv* env, kscript::Value* retval)
{
    XCM_AUTO_LOCK(m_state_mutex);
    m_cancelled = true;
}



bool FileTransfer::isBusyInternal()
{
    bool result;
    m_state_mutex.lock();
    result = m_busy;
    m_state_mutex.unlock();
    return result;
}


void FileTransfer::fireFinishedEvent()
{
    invokeJsEvent(L"finished");
}
