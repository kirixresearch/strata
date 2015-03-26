/*!
 *
 * Copyright (c) 2015, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2015-03-25
 *
 */


#include "jobspch.h"
#include "uncompress.h"
#include "zip.h"
#include <kl/utf8.h>
#include <kl/file.h>


namespace jobs
{


// UncompressJob implementation

UncompressJob::UncompressJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.uncompress-job";
    m_config["metadata"]["version"] = 1;
}

UncompressJob::~UncompressJob()
{
}

bool UncompressJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.uncompress-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "output" : <path>,
            "format" : "zip"
            "stream" : ""
        }
    }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

    return true;
}

int UncompressJob::runJob()
{
    // make sure we have a valid input
    if (!isInputValid())
    {
        // TODO: correlate input validation with job error information
    
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }

    // make sure the database is valid
    if (m_db.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }    


    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    // get the index node children
    std::wstring input = params_node["input"];
    std::wstring output = params_node["output"];
    std::wstring stream = params_node["stream"];
    std::wstring format = params_node["format"];


    if (format == L"zip")
    {
        // first copy the input stream to a temporary file

        xd::IStreamPtr instream = m_db->openStream(input);
        if (instream.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        std::wstring path = xf_get_temp_filename(L"", L"zip");
        xf_file_t f = xf_open(path, xfCreate, xfReadWrite, xfShareNone);
        if (!f)
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        unsigned char* buf = new unsigned char[65536];

        unsigned long read, written;
        bool done = false;
        bool error = false;
        while (!done)
        {
            if (!instream->read(buf, 65536, &read))
                break;
            if (read != 65536)
                done = true;
            if (read > 0)
            {
                written = xf_write(f, buf, 1, read);
                if (written != read)
                    error = true;
                if (error)
                {
                    // write error / disk space
                    delete[] buf;
                    instream.clear();
                    xf_close(f);
                    xf_remove(path);
                    m_job_info->setState(jobStateFailed);
                    return 0;
                }
            }
        }

        delete[] buf;
        xf_close(f);



        // extract file from zip


        struct zip* zip = zip_open(kl::toUtf8(path), 0, NULL);
        if (!zip)
        {
            xf_remove(path);
            m_job_info->setState(jobStateFailed);
            return 0;
        }



        if (!m_db->createStream(output, xf_get_mimetype_from_extension(stream)))
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        xd::IStreamPtr outstream = m_db->openStream(output);
        if (outstream.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        struct zip_file* sf = zip_fopen(zip, kl::tostring(stream).c_str(), 0);
        if (!sf)
        {
            outstream.clear();
            m_db->deleteFile(output);
            m_job_info->setState(jobStateFailed);
            return 0;
        }




        buf = new unsigned char[65536];
        int bytes_read;
        unsigned long bytes_written;
        while (1)
        {
            bytes_read = (int)zip_fread(sf, buf, 65536);
            if (bytes_read < 0)
            {
                delete[] buf;
                zip_fclose(sf);
                zip_close(zip);
                xf_remove(path);
                return false;
            }

            if (bytes_read > 0)
            {
                bytes_written = 0;
                outstream->write(buf, bytes_read, &bytes_written);
                if (bytes_written != (unsigned long)bytes_read)
                {
                    // write error
                    outstream.clear();
                    m_db->deleteFile(output);

                    delete[] buf;
                    zip_fclose(sf);
                    zip_close(zip);
                    xf_remove(path);
                    return false;
                }
            }

            if (bytes_read != 65536)
            {
                break;
            }

        }
        delete[] buf;


        zip_fclose(sf);
        zip_close(zip);
        xf_remove(path);

    }

    return 0;
}

void UncompressJob::runPostJob()
{
}


};  // namespace jobs

