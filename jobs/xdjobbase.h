/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-26
 *
 */


#ifndef H_JOBS_XDJOBBASE_H
#define H_JOBS_XDJOBBASE_H


#include "jobinfo.h"


namespace jobs
{




class XdJobInfo : public JobInfo
{
public:

    XdJobInfo() : JobInfo()
    {
        pct = 0.0;
    }

    bool getCancelAllowed()
    {
        if (xdjob.isOk() && !xdjob->getCanCancel())
            return false;
        return JobInfo::getCancelAllowed();
    }

    void setXdJob(xd::IJobPtr _xdjob)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        if (xdjob.isOk())
        {
            // now that the job is going away, record values
            m_current_count = (double)xdjob->getCurrentCount();
            m_max_count = (double)xdjob->getMaxCount();
            pct = xdjob->getPercentage();
        }

        xdjob = _xdjob;

        if (xdjob.isOk())
        {
            // now that the job is going away, record values
            m_current_count = (double)xdjob->getCurrentCount();
            m_max_count = (double)xdjob->getMaxCount();
            pct = xdjob->getPercentage();
        }
    }

    double getCurrentCount()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (xdjob.isOk())
            m_current_count = (double)xdjob->getCurrentCount();
        return m_current_count;
    }

    double getMaxCount()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (xdjob.isOk())
            m_max_count = (double)xdjob->getMaxCount();
        return m_max_count;
    }

    double getPercentage()
    {
        KL_AUTO_LOCK(m_obj_mutex);
        if (xdjob.isOk())
            pct = xdjob->getPercentage();
        return pct;
    }

public:

    xd::IJobPtr xdjob;
    double pct;
};


class XdJobBase : public JobBase,
                  public xcm::signal_sink
{

public:

    XdJobBase() : JobBase()
    {
        sigJobFinished().connect(this, &XdJobBase::onJobFinished);

        m_xd_jobinfo = new XdJobInfo;
        m_xd_jobinfo->ref();

        setJobInfo(static_cast<IJobInfo*>(m_xd_jobinfo));
    }

    virtual ~XdJobBase()
    {
        m_xd_jobinfo->unref();
    }

    void setDatabase(xd::IDatabase* db)
    {
        m_db = db;
    }

    bool cancel()
    {
        if (!m_xd_jobinfo->getCancelAllowed())
            return false;

        if (m_xd_jobinfo->xdjob.isOk())
            m_xd_jobinfo->xdjob->cancel();

        return JobBase::cancel();
    }

protected:

    void setXdJob(xd::IJobPtr xdjob)
    {
        m_xd_jobinfo->setXdJob(xdjob);
    }

    XdJobInfo* getRawXdJobInfo()
    {
        return m_xd_jobinfo;
    }

    void onJobFinished(IJobPtr job)
    {
        m_xd_jobinfo->setXdJob(xcm::null);
    }

protected:

    xd::IDatabasePtr m_db;

private:

    XdJobInfo* m_xd_jobinfo;
};


} // namespace jobs


#endif

