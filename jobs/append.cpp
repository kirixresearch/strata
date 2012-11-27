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

/*
    std::vector<tango::ISetPtr> sets;
    kl::JsonNode input_arr = m_config['input'];
    for (size_t i = 0; i < input_arr.getChildCount(); ++i)
    {
        tango::ISetPtr set = m_db->openSet(
    }

    std::vector<tango::ISetPtr>::iterator it;
    for (it = m_append_sets.begin(); it != m_append_sets.end(); ++it)
    {
        tango_job = m_db->createJob();
        setTangoJob(tango_job, false);

        source_iter = (*it)->createIterator(L"", L"", NULL);
        if (!source_iter)
            continue;

        source_iter->goFirst();
        m_target_set->insert(source_iter, L"", 0, tango_job);

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setError(jobserrInsufficientDiskSpace, L"");
            break;
        }

        if (tango_job->getCancelled())
            break;
    }
        */

    return 0;
}

void AppendJob::runPostJob()
{
}



};  // namespace jobs
