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
        if (curjob.isNull()) return tango::errorNone;
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

    IJobInfoPtr getJobInfo()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return static_cast<IJobInfo*>(m_agg_jobinfo);
    }

private:

    AggregateJobInfo* m_agg_jobinfo;
};


}; // namespace jobs


#endif

