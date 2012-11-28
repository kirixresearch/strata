/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
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
    m_max_count = 0.0;
}

AppendJob::~AppendJob()
{
}


/*
void AppendJob::addAppendSet(tango::ISetPtr append_set)
{
    if (append_set->getSetFlags() & tango::sfFastRowCount)
    {
        m_max_count += (tango::tango_int64_t)append_set->getRowCount();
        m_job_info->setMaxCount(m_max_count);
    }

    m_append_sets.push_back(append_set);
}
*/


int AppendJob::runJob()
{
    tango::IJobPtr tango_job;

    tango::IIteratorPtr source_iter;
    tango::ISetPtr target_set;
    std::vector<tango::ISetPtr>::iterator it;


    // get input data
    std::vector<tango::ISetPtr> sets;
    kl::JsonNode input_arr = m_config["input"];
    for (size_t i = 0; i < input_arr.getChildCount(); ++i)
    {
        if (!input_arr[i].childExists("path"))
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrInvalidParameter, L"");
            return 0;
        }

        tango::ISetPtr set = m_db->openSet(input_arr[i]["path"]);
        if (set.isNull())
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrInvalidParameter, L"");
            return 0;
        }
    }

    if (sets.size() == 0)
    {
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }


    // output target
    std::wstring output_path = m_config["output"];
    if (m_db->getFileExist(output_path))
    {
        // append to existing table
        target_set = m_db->openSet(output_path);
        if (target_set.isNull())
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
        std::vector<tango::ISetPtr> set_ptrs;

        int col_idx = 0;
        int input_col_count = 0;
        int output_col_count = 0;

        for (it = sets.begin(); it != sets.end(); ++it)
        {
            // merging tables -- merge their fields into one amalgamated
            // output structure (NOTE: we don't do this for append because
            // doing an append implies that the result set should have the
            // same structure as the set we're appending to)

            input_structure = (*it)->getStructure();
                
            if (!output_structure)
            {
                output_structure = (*it)->getStructure();
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


        target_set = m_db->createSet(output_path, output_structure, NULL);
        if (target_set.isNull())
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrWriteError, L"");
            return 0;
        }
    }





    for (it = sets.begin(); it != sets.end(); ++it)
    {
        tango_job = m_db->createJob();
        setTangoJob(tango_job, false);

        source_iter = (*it)->createIterator(L"", L"", NULL);
        if (!source_iter)
            continue;

        source_iter->goFirst();
        target_set->insert(source_iter, L"", 0, tango_job);

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrInsufficientDiskSpace, L"");
            break;
        }

        if (tango_job->getCancelled())
            break;
    }


    return 0;
}

void AppendJob::runPostJob()
{
}



};  // namespace jobs
