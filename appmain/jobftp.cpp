/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#include "appmain.h"
#include "jobftp.h"
#include <wx/protocol/ftp.h>


FtpJob::FtpJob() : cfw::JobBase()
{
    m_job_info->setTitle(_("FTP"));
}

FtpJob::~FtpJob()
{
    // -- clear out user name and password --
    m_user =  wxT("            ");
    m_password = wxT("            ");
}

void FtpJob::setHostInfo(const wxString& host, const wxString& user, const wxString& password)
{
    m_host = host;
    m_user = user;
    m_password = password;
}

void FtpJob::addDownloadFile(const wxString& remote_path, const wxString& local_path, bool binary)
{
    FtpFile f;
    f.remote_path = remote_path;
    f.local_path = local_path;
    f.type = FtpFile::typeDownload;
    f.binary = binary;

    m_files.push_back(f);
}

void FtpJob::addUploadFile(const wxString& local_path, const wxString& remote_path, bool binary)
{
    FtpFile f;
    f.remote_path = remote_path;
    f.local_path = local_path;
    f.type = FtpFile::typeUpload;
    f.binary = binary;

    m_files.push_back(f);
}


int FtpJob::runJob()
{
    wxFTP ftp;

    if (!m_user.IsEmpty())
    {
        ftp.SetUser(m_user);
        ftp.SetPassword(m_password);
    }


    m_job_info->setProgressString(_("Connecting..."));


    wxIPV4address addr;
    addr.Hostname(m_host);
    addr.Service(wxT("ftp"));

    ftp.SetTimeout(20);

    if (!ftp.Connect(addr))
    {
        m_job_info->setProgressString(_("Connect failed."));
        m_job_info->setState(cfw::jobStateFailed);
        return 0;

    /*
        // -- wait for connection to open --
        while (!ftp.WaitForConnection(1, 0))
        {
            if (isCancelling())
            {
                m_job_info->setProgressString(wxEmptyString);
                return 0;
            }
        }

        if (!client->IsConnected())
        {
            m_job_info->setProgressString(_("Connect failed."));
            m_job_info->setState(cfw::jobStateFailed);
            return 0;
        }
    */

    }

    m_job_info->setProgressString(wxEmptyString);



    std::vector<FtpFile>::iterator it;

    for (it = m_files.begin(); it != m_files.end(); ++it)
    {
        wxString remote_dir = it->remote_path.BeforeLast(wxT('/'));
        wxString remote_file = it->remote_path.AfterLast(wxT('/'));
        
        if (remote_dir == it->remote_path)
        {
            remote_dir = wxT("/");
        }

        ftp.ChDir(remote_dir);

        if (it->binary)
        {
            ftp.SetBinary();
        }
         else
        {
            ftp.SetAscii();
        }


        if (it->type == FtpFile::typeDownload)
        {
            wxInputStream *in = ftp.GetInputStream(remote_file);
            xf_file_t outf;
            
           
            if (!in)
            {
                m_job_info->setState(cfw::jobStateFailed);
                return 0;
            }


            outf = xf_open(towstr(it->local_path),
                           xfCreate, xfReadWrite, xfShareNone);

            if (!outf)
            {
                m_job_info->setState(cfw::jobStateFailed);
                return 0;
            }


            size_t stream_size = in->GetSize();

            if (stream_size == ~(size_t)0)
            {
                m_job_info->setMaxCount(0);
            }
             else
            {
                m_job_info->setMaxCount(stream_size);
            }
            

            char* buf = new char[65536];
            size_t read_bytes;
            
            while (1)
            {
                read_bytes = in->Read(buf, 65536).LastRead();

                if (read_bytes > 0)
                {
                    if (xf_write(outf, buf, 1, read_bytes) != read_bytes)
                        break;
                }

                m_job_info->incrementCurrentCount(read_bytes);

                if (read_bytes != 65536)
                    break;
            }


            delete[] buf;
            delete in;

            xf_close(outf);
        }
         else if (it->type == FtpFile::typeUpload)
        {
            m_job_info->setMaxCount(xf_get_file_size(towstr(it->local_path)));

            xf_file_t inf = xf_open(towstr(it->local_path),
                                    xfOpen, xfRead, xfShareRead);
            if (!inf)
            {
                m_job_info->setState(cfw::jobStateFailed);
                return 0;
            }

            wxOutputStream *out = ftp.GetOutputStream(remote_file);
            if (!out)
            {
                xf_close(inf);

                m_job_info->setState(cfw::jobStateFailed);
                return 0;
            }

            
            char* buf = new char[65536];
            size_t read_bytes;

            while (1)
            {
                read_bytes = xf_read(inf, buf, 1, 65536);
                
                if (read_bytes > 0)
                {
                    out->Write(buf, read_bytes);
                }

                m_job_info->incrementCurrentCount(read_bytes);

                if (read_bytes != 65536)
                    break;
            }

            delete out;
            delete[] buf;

            xf_close(inf);
        }
    }

    return 0;
}

