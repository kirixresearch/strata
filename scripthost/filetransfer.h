/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2008-02-15
 *
 */


#ifndef H_SCRIPTHOST_SCRIPTFILETRANSFER_H
#define H_SCRIPTHOST_SCRIPTFILETRANSFER_H



namespace scripthost
{


class FileTransfer : public ScriptHostBase
{    
    BEGIN_KSCRIPT_CLASS("FileTransfer", FileTransfer)     
        KSCRIPT_METHOD("constructor", FileTransfer::constructor)
        KSCRIPT_METHOD("setAsync", FileTransfer::setAsync)
        KSCRIPT_METHOD("download", FileTransfer::download)
        KSCRIPT_METHOD("upload", FileTransfer::upload)
        KSCRIPT_METHOD("rename", FileTransfer::rename)
        KSCRIPT_METHOD("isDone", FileTransfer::isDone)
        KSCRIPT_METHOD("cancel", FileTransfer::cancel)
        KSCRIPT_METHOD("getBytesTransferred", FileTransfer::getBytesTransferred)
    END_KSCRIPT_CLASS()

public:

    FileTransfer();
    ~FileTransfer();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setAsync(kscript::ExprEnv* env, kscript::Value* retval);
    void download(kscript::ExprEnv* env, kscript::Value* retval);
    void upload(kscript::ExprEnv* env, kscript::Value* retval);
    void rename(kscript::ExprEnv* env, kscript::Value* retval);
    void isDone(kscript::ExprEnv* env, kscript::Value* retval);
    void cancel(kscript::ExprEnv* env, kscript::Value* retval);
    void getBytesTransferred(kscript::ExprEnv* env, kscript::Value* retval);
    
public:

    bool isBusyInternal();
    void doAction();
    void fireFinishedEvent();

public:

    void* m_curl;
    std::string m_url;
    std::wstring m_filename;
    std::string m_proxy;
    long m_proxy_port;
    bool m_async;
    bool m_upload;
    
    kl::mutex m_state_mutex;
    bool m_busy;
    double m_total_bytes;
    bool m_cancelled;
};


}; // namespace scripthost


#endif

