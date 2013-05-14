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

        objects: [

            {
                source_connection: "",
                source_path: "",

                destination_connection: "",
                destination_path: ""
            }
        ]


    }
}
*/


// LoadJob implementation

LoadJob::LoadJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
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


    tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();
    if (dbmgr.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    std::map<std::wstring, tango::IDatabasePtr> connection_pool;

    kl::JsonNode objects_node = params_node["objects"];
    size_t i, cnt = objects_node.getChildCount();
    for (i = 0; i < cnt; ++i)
    {
        kl::JsonNode object = objects_node[i];


        std::wstring source_connection = object["source_connection"];
        std::wstring destination_connection = object["destination_connection"];

        std::wstring source_path = object["source_path"];
        std::wstring destination_path = object["destination_path"];


        tango::IDatabasePtr source_db;
        tango::IDatabasePtr destination_db;

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


        tango::QueryParams qp;
        qp.from = source_path;

        if (object.childExists("source_format"))
        {
            kl::JsonNode format = object["source_format"];

            qp.format.format = tango::formatDelimitedText;
            qp.format.delimiters = format.getChild("delimiter").getString();
            qp.format.text_qualifiers = format.getChild("text_qualifier").getString();
            qp.format.first_row_column_names = format.getChild("header_row").getBoolean();
        }

        tango::IIteratorPtr source_iter = source_db->query(qp);

        if (source_iter.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        source_iter->goFirst();


        // create the destination table

        tango::FormatInfo* p_destination_format = NULL;
        tango::FormatInfo destination_format;

        if (object.childExists("destination_format"))
        {
            kl::JsonNode format = object["destination_format"];

            destination_format.format = tango::formatDelimitedText;
            destination_format.delimiters = format.getChild("delimiter").getString();
            destination_format.text_qualifiers = format.getChild("text_qualifier").getString();
            destination_format.first_row_column_names = format.getChild("header_row").getBoolean();
            p_destination_format = &destination_format;
        }


        
        // if 'append' is not set or is false, drop the destination table
        if (object.childExists("overwrite") && object.getChild("overwrite").getBoolean())
        {
            destination_db->deleteFile(destination_path);

            tango::IStructurePtr structure = source_iter->getStructure();
            if (!destination_db->createTable(destination_path, structure, p_destination_format))
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
                tango::IStructurePtr structure = source_iter->getStructure();
                if (!destination_db->createTable(destination_path, structure, p_destination_format))
                {
                    // could not create output file
                    m_job_info->setState(jobStateFailed);
                    return 0;
                }
            }
        }


        // right now there is no transformation happening -- just copy the whole table

        tango::CopyParams info;
        info.iter_input = source_iter;
        info.output = destination_path;
        info.append = true;
        
        // TODO: add copy loop here
        tango::IJobPtr tango_job = destination_db->createJob();
        setTangoJob(tango_job);

        destination_db->copyData(&info, tango_job);

        
        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (tango_job->getStatus() == tango::jobFailed)
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

