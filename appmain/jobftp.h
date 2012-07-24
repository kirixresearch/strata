/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#ifndef __APP_JOBFTP_H
#define __APP_JOBFTP_H


class FtpFile
{
public:

    enum
    {
        typeUpload = 0,
        typeDownload = 1
    };

    int type;
    wxString remote_path;
    wxString local_path;
    bool binary;
};


class FtpJob : public cfw::JobBase
{
    XCM_CLASS_NAME("appmain.FtpJob")
    XCM_BEGIN_INTERFACE_MAP(FtpJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    FtpJob();
    virtual ~FtpJob();

    void setHostInfo(const wxString& host, const wxString& user, const wxString& password);
    void addDownloadFile(const wxString& remote_path, const wxString& local_path, bool binary);
    void addUploadFile(const wxString& local_path, const wxString& remote_path, bool binary);

    int runJob();

private:

    wxString m_host;
    wxString m_user;
    wxString m_password;

    std::vector<FtpFile> m_files;

};




#endif
