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




class AggregateJobInfo : public JobInfo, public IAggregateJobInfo
{
public:

    XCM_CLASS_NAME("jobs.AggregateJobInfo")
    XCM_BEGIN_INTERFACE_MAP(AggregateJobInfo)
        XCM_INTERFACE_ENTRY(IJobInfo)
        XCM_INTERFACE_ENTRY(IAggregateJobInfo)
    XCM_END_INTERFACE_MAP()

    AggregateJobInfo() : JobInfo()
    {
        m_job_count = 1;
        m_job_idx = 0;
    }

    void setJobCount(size_t job_count)
    {
        KL_AUTO_LOCK(m_obj_mutex);
        m_job_count = (int)job_count;
    }

    void setCurrentJobIndex(size_t job_idx)
    {
        KL_AUTO_LOCK(m_obj_mutex);
        m_job_idx = (int)job_idx;
    }

    void setCurrentJobInfo(IJobInfoPtr j)
    {
        KL_AUTO_LOCK(m_obj_mutex);
        curjob = j;
    }

    std::wstring getProgressString()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return L"";
        return curjob->getProgressString();
    }

    int getErrorCode()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return xd::errorNone;
        return curjob->getErrorCode();
    }

    std::wstring getErrorString()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return L"";
        return curjob->getErrorString();
    }

    double getCurrentCount()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return 0.0;
        return curjob->getCurrentCount();
    }

    double getMaxCount()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return 0.0;
        return curjob->getMaxCount();
    }

    double getPercentage()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (curjob.isNull()) return 0.0;
        double pct = curjob->getPercentage();
        if (m_job_count == 1)
            return pct;
        return (((double)(m_job_idx*100)) / ((double)m_job_count)) + (pct/((double)m_job_count));
    }

public:

    IJobInfoPtr curjob;
    int m_job_count;
    int m_job_idx;
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
        KL_AUTO_LOCK(m_jobbase_mutex);
        m_current_job = job;
    }

    bool cancel()
    {
        KL_AUTO_LOCK(m_jobbase_mutex);

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

