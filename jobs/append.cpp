/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-26
 *
 */


#include "jobspch.h"
#include "append.h"


namespace jobs
{


// AppendJob implementation

AppendJob::AppendJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.append-job";
    m_config["metadata"]["version"] = 1;
}

AppendJob::~AppendJob()
{
}

bool AppendJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.append-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : [<path1>, <path2>, ...],
            "output" : <path>,      // optional;
            "mode" : "overwrite"    // optional; if used, either "append" or "overwrite"; if undefined, uses "append"
        }
    }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

    return true;
}

int AppendJob::runJob()
{
    if (!isInputValid())
    {
        // TODO: correlate input validation with job error information
    
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }

    tango::IJobPtr tango_job;
    tango::IIteratorPtr source_iter;

    long long max_count = 0;
    bool valid_max_count = true;


    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    // get input data
    std::vector<std::wstring> tables;
    std::vector<std::wstring>::iterator it;

    kl::JsonNode input_arr = params_node["input"];

    for (size_t i = 0; i < input_arr.getChildCount(); ++i)
    {
        std::wstring path = input_arr[i].getString();

        tango::IFileInfoPtr finfo = m_db->getFileInfo(path);
        if (finfo.isNull())
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrInvalidParameter, L"");
            return 0;
        }

        if (finfo->getFlags() & tango::sfFastRowCount)
            max_count += finfo->getRowCount();
             else
            valid_max_count = false;  // can't get a reliable total number of records

        tables.push_back(path);
    }

    if (tables.size() == 0)
    {
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }

    if (valid_max_count)
        m_job_info->setMaxCount((double)max_count);

    // output target
    std::wstring output_path = params_node["output"];

    if (params_node["mode"].getString() == L"overwrite")
    {
        if (m_db->getFileExist(output_path))
            m_db->deleteFile(output_path);

        if (m_db->getFileExist(output_path))
        {
            // delete failed
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrWriteError, L"");
            return 0;
        }
    }


    if (m_db->getFileExist(output_path))
    {
        // append to existing table
        tango::IFileInfoPtr finfo = m_db->getFileInfo(output_path);
        if (finfo.isNull() || finfo->getType() != tango::filetypeTable)
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrWriteError, L"");
            return 0;
        }
    }
     else
    {
        // output target does not yet exist;  create an output table with a merged structure
        tango::IStructurePtr input_structure = xcm::null;
        tango::IStructurePtr output_structure = xcm::null;
        tango::IColumnInfoPtr input_colinfo = xcm::null;
        tango::IColumnInfoPtr output_colinfo = xcm::null;

        int col_idx = 0;
        int input_col_count = 0;
        int output_col_count = 0;

        for (it = tables.begin(); it != tables.end(); ++it)
        {
            // merging tables -- merge their fields into one amalgamated
            // output structure (NOTE: we don't do this for append because
            // doing an append implies that the result set should have the
            // same structure as the set we're appending to)

            input_structure = m_db->describeTable(*it);
            if (input_structure.isNull())
            {
                m_job_info->setState(jobStateFailed);
                m_job_info->setError(jobserrWriteError, L"");
                return 0;
            }
            
            if (!output_structure)
            {
                output_structure = input_structure->clone();
            }

            input_col_count = input_structure->getColumnCount();
            output_col_count = output_structure->getColumnCount();

            if (output_col_count < input_col_count)
            {
                input_col_count = output_col_count;
            }

            for (col_idx = 0; col_idx < input_col_count; ++col_idx)
            {
                input_colinfo = input_structure->getColumnInfoByIdx(col_idx);
                output_colinfo = output_structure->getColumnInfoByIdx(col_idx);

                if (input_colinfo->getWidth() > output_colinfo->getWidth())
                    output_colinfo->setWidth(input_colinfo->getWidth());
            }
        }

        if (!m_db->createTable(output_path, output_structure, NULL))
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrWriteError, L"");
            return 0;
        }
    }


    for (it = tables.begin(); it != tables.end(); ++it)
    {
        tango_job = m_db->createJob();
        setTangoJob(tango_job, false);

        tango::CopyParams info;
        info.input = *it;
        info.output = output_path;
        info.append = true;
        m_db->copyData(&info, tango_job.p);

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrInsufficientDiskSpace, L"");
            break;
        }

        if (tango_job->getCancelled())
        {
            if (params_node["mode"].getString() == L"overwrite")
                m_db->deleteFile(output_path);

            break;
        }
    }


    return 0;
}

void AppendJob::runPostJob()
{
}


};  // namespace jobs

