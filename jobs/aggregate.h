/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-25
 *
 */


#ifndef __JOBS_AGGREGATE_H
#define __JOBS_AGGREGATE_H

#include "jobinfo.h"

namespace jobs
{




class AggregateJobInfo : public JobInfo
{
public:

    AggregateJobInfo() : JobInfo()
    {
    }

    void setCurrentJobInfo(IJobInfoPtr j)
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        curjob = j;
    }

    std::wstring getProgressString()
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return L"";
        return curjob->getProgressString();
    }

    int getErrorCode()
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return xd::errorNone;
        return curjob->getErrorCode();
    }

    std::wstring getErrorString()
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return L"";
        return curjob->getErrorString();
    }

    double getCurrentCount()
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return 0.0;
        return curjob->getCurrentCount();
    }

    double getMaxCount()
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return 0.0;
        return curjob->getMaxCount();
    }

    double getPercentage()
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return 0.0;
        return curjob->getPercentage();
    }

public:

    IJobInfoPtr curjob;
};


class AggregateJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.AggregateJob")
    XCM_BEGIN_INTERFACE_MAP(AggregateJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    AggregateJob();
    virtual ~AggregateJob();

    bool isInputValid();
    int runJob();
    void runPostJob();

    void setCurrentJob(IJobPtr job)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_current_job = job;
    }

    bool cancel()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        if (!m_job_info->getCancelAllowed())
            return false;
        
        if (m_current_job.isOk() && !m_current_job->getJobInfo()->getCancelAllowed())
            return false;

        m_job_info->setState(jobStateCancelling);
        m_cancelling = true;

        if (m_current_job.isOk())
            return m_current_job->cancel();

        return true;
    }

private:

    AggregateJobInfo* m_agg_jobinfo;
    IJobPtr m_current_job;
};


}; // namespace jobs


#endif

