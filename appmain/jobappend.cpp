/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-09-28
 *
 */


#include "appmain.h"
#include "jobappend.h"


// -- AppendJob implementation --

AppendJob::AppendJob() : TangoJobBase(TangoJobBase::useTangoCurrentCount)
{
    m_job_info->setTitle(_("Append Records"));
    m_max_count = 0.0;
}

AppendJob::~AppendJob()
{
}


void AppendJob::setTargetSet(tango::ISetPtr target_set)
{
    m_target_set = target_set;
}


void AppendJob::addAppendSet(tango::ISetPtr append_set)
{
    if (append_set->getSetFlags() & tango::sfFastRowCount)
    {
        m_max_count += (tango::tango_int64_t)append_set->getRowCount();
        m_job_info->setMaxCount(m_max_count);
    }

    m_append_sets.push_back(append_set);
}


tango::ISetPtr AppendJob::getTargetSet()
{
    return m_target_set;
}

int AppendJob::runJob()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        m_job_info->setState(cfw::jobStateFailed);
        return 0;
    }
    
    tango::IJobPtr tango_job;

    tango::IIteratorPtr source_iter;

    std::vector<tango::ISetPtr>::iterator it;
    for (it = m_append_sets.begin(); it != m_append_sets.end(); ++it)
    {
        tango_job = db->createJob();
        setTangoJob(tango_job, false);

        source_iter = (*it)->createIterator(L"", L"", NULL);
        if (!source_iter)
            continue;

        source_iter->goFirst();
        m_target_set->insert(source_iter, L"", 0, tango_job);

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(cfw::jobStateFailed);
            m_job_info->setProgressString(_("ERROR: Insufficient disk space"));
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

