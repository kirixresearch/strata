/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-04-09
 *
 */


#include "jobspch.h"
#include "load.h"


namespace jobs
{

// example:

/*
{
    "metadata" : {
        "type" : "application/vnd.kx.load-job",
        "version" : 1,
        "description" : ""
    },
    "params": {

        "objects": [

            {
                "source_connection": "",
                "source_path": "",

                "destination_connection": "",
                "destination_path": ""
            }
        ]


    }
}
*/


// LoadJob implementation

LoadJob::LoadJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.load-job";
    m_config["metadata"]["version"] = 1;
}

LoadJob::~LoadJob()
{
}

bool LoadJob::isInputValid()
{
    // TODO: fill out

    return true;
}

int LoadJob::runJob()
{
    // make sure we have a valid input
    if (!isInputValid())
    {
        // TODO: correlate input validation with job error information
    
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }

    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
    if (dbmgr.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    std::map<std::wstring, xd::IDatabasePtr> connection_pool;

    kl::JsonNode objects_node = params_node["objects"];
    size_t i, cnt = objects_node.getChildCount();
    for (i = 0; i < cnt; ++i)
    {
        kl::JsonNode object = objects_node[i];


        std::wstring source_connection = object["source_connection"];
        std::wstring destination_connection = object["destination_connection"];

        std::wstring source_path = object["source_path"];
        std::wstring destination_path = object["destination_path"];


        xd::IDatabasePtr source_db;
        xd::IDatabasePtr destination_db;

        source_db = connection_pool[source_connection];
        if (source_db.isNull())
        {
            source_db = dbmgr->open(source_connection);
            connection_pool[source_connection] = source_db;
        }

        destination_db = connection_pool[destination_connection];
        if (destination_db.isNull())
        {
            destination_db = dbmgr->open(destination_connection);
            connection_pool[destination_connection] = destination_db;
        }


        if (source_db.isNull() || destination_db.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }


        xd::IFileInfoPtr finfo = source_db->getFileInfo(source_path);
        if (finfo.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }


        if (finfo->getType() == xd::filetypeStream)
        {
            xd::IStreamPtr instream = source_db->openStream(source_path);
            if (instream.isNull())
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            if (!destination_db->createStream(destination_path, finfo->getMimeType()))
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            xd::IStreamPtr outstream = destination_db->openStream(destination_path);
            if (outstream.isNull())
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
                    if (!outstream->write(buf, read, &written))
                        error = true;
                    if (!error && written != read)
                        error = true;
                    if (error)
                    {
                        // write error / disk space
                        delete[] buf;
                        instream.clear();
                        outstream.clear();
                        destination_db->deleteFile(destination_path);
                        m_job_info->setState(jobStateFailed);
                        return 0;
                    }
                }
            }

            delete[] buf;
            return 0;
        }




        xd::QueryParams qp;
        qp.from = source_path;
        qp.format.determine_structure = true; // for csvs where we don't know the structure, perform a full scan to get correct metrics

        if (object.childExists("source_format"))
        {
            kl::JsonNode format = object["source_format"];

            qp.format.format = xd::formatDelimitedText;
            qp.format.delimiters = format.getChild("delimiter").getString();
            qp.format.text_qualifiers = format.getChild("text_qualifier").getString();
            qp.format.first_row_column_names = format.getChild("header_row").getBoolean();
        }

        xd::IIteratorPtr source_iter = source_db->query(qp);

        if (source_iter.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        source_iter->goFirst();


        // create the destination table

        xd::FormatDefinition destination_format;

        if (object.childExists("destination_format"))
        {
            kl::JsonNode format = object["destination_format"];

            destination_format.format = xd::formatDelimitedText;
            if (kl::icontains(destination_path, L".icsv"))
                destination_format.format = xd::formatTypedDelimitedText;

            destination_format.delimiters = format.getChild("delimiter").getString();
            destination_format.text_qualifiers = format.getChild("text_qualifier").getString();
            destination_format.first_row_column_names = format.getChild("header_row").getBoolean();
        }


        
        // if 'append' is not set or is false, drop the destination table
        if (object.childExists("overwrite") && object.getChild("overwrite").getBoolean())
        {
            destination_db->deleteFile(destination_path);

            xd::IStructurePtr structure = source_iter->getStructure();
            for (int i = 0, colcount = structure->getColumnCount(); i < colcount; ++i)
                destination_format.columns.push_back(structure->getColumnInfoByIdx(i));

            if (!destination_db->createTable(destination_path, destination_format))
            {
                // could not create output file
                m_job_info->setState(jobStateFailed);
                return 0;
            }
        }
         else
        {
            if (!destination_db->getFileExist(destination_path))
            {
                xd::IStructurePtr structure = source_iter->getStructure();
                for (int i = 0, colcount = structure->getColumnCount(); i < colcount; ++i)
                    destination_format.columns.push_back(structure->getColumnInfoByIdx(i));

                if (!destination_db->createTable(destination_path, destination_format))
                {
                    // could not create output file
                    m_job_info->setState(jobStateFailed);
                    return 0;
                }
            }
        }


        // right now there is no transformation happening -- just copy the whole table

        xd::CopyParams info;
        info.iter_input = source_iter;
        info.output = destination_path;
        info.append = true;
        
        // TODO: add copy loop here
        xd::IJobPtr xd_job = destination_db->createJob();
        setXdJob(xd_job);

        destination_db->copyData(&info, xd_job);

        
        if (xd_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (xd_job->getStatus() == xd::jobFailed)
        {
            m_job_info->setState(jobStateFailed);

            // TODO: need to decide how to handle error strings; these need to 
            // be translated, so shouldn't be in this class
            //m_job_info->setProgressString(towstr(_("Modify failed: The table may be in use by another user.")));
        }
    }


    return 0;
}

void LoadJob::runPostJob()
{
}


};  // namespace jobs

