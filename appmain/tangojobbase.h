/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-06-14
 *
 */


#ifndef __APP_TANGOJOBBASE_H
#define __APP_TANGOJOBBASE_H


class TangoJobBase : public JobBase,
                     public xcm::signal_sink
{
public:

    enum
    {
        useTangoCurrentCount = 0x01,
        useTangoMaxCount = 0x02,
        useTangoPercentage = 0x04,
        useTangoAll = 0xffffffff
    };

public:

    TangoJobBase(int usage = useTangoAll) : JobBase()
    {
        sigJobFinished().connect(this, &TangoJobBase::onJobFinished);

        m_base_count = 0.0;
        m_usage = usage;
    }

    bool cancel()
    {
        if (!m_tango_job)
        {
            return JobBase::cancel();
        }

        if (!m_job_info->getCancelAllowed())
            return false;

        m_tango_job->cancel();

        return JobBase::cancel();
    }

protected:

    void setTangoJob(tango::IJobPtr new_val, bool reset = true)
    {
        if (new_val.isOk())
        {
            if (m_usage & useTangoCurrentCount)
            {
                m_job_info->sigOnGetCurrentCount().connect(this, &TangoJobBase::onGetCurrentCount);
            }

            if (m_usage & useTangoMaxCount)
            {
                m_job_info->sigOnGetMaxCount().connect(this, &TangoJobBase::onGetMaxCount);
            }

            if (m_usage & useTangoPercentage)
            {
                m_job_info->sigOnGetPercentage().connect(this, &TangoJobBase::onGetPercentage);
            }
        }
         else
        {
            m_job_info->sigOnGetCurrentCount().disconnect();
            m_job_info->sigOnGetMaxCount().disconnect();
            m_job_info->sigOnGetPercentage().disconnect();
        }

        if (reset)
        {
            m_base_count = 0.0;
        }
         else
        {
            if (!m_tango_job)
                m_base_count = 0.0;
                 else
                onGetCurrentCount(&m_base_count);
        }

        m_tango_job = new_val;
    }

private:

    void onGetCurrentCount(double* result)
    {
        if (m_tango_job.p)
        {
            *result = (tango::tango_int64_t)m_tango_job.p->getCurrentCount() + m_base_count;
        }
         else
        {
            *result = m_job_info->getCurrentCount();
        }
    }

    void onGetMaxCount(double* result)
    {
        if (m_tango_job.p)
        {
            *result = (tango::tango_int64_t)m_tango_job.p->getMaxCount();
        }
         else
        {
            *result = m_job_info->getMaxCount();
        }
    }

    void onGetPercentage(double* result)
    {
        if (m_tango_job.p)
        {
            *result = m_tango_job.p->getPercentage();
        }
         else
        {
            *result = m_job_info->getPercentage();
        }
    }

    void onJobFinished(IJobPtr job)
    {
        m_job_info->sigOnGetCurrentCount().disconnect();
        m_job_info->sigOnGetMaxCount().disconnect();
        m_job_info->sigOnGetPercentage().disconnect();
        
        if (m_tango_job.p)
        {
            m_job_info->setCurrentCount((tango::tango_int64_t)m_tango_job.p->getCurrentCount());
            m_job_info->setMaxCount((tango::tango_int64_t)m_tango_job.p->getMaxCount());
        }

        // -- free our tango job --
        m_tango_job.clear();
    }

private:

    tango::IJobPtr m_tango_job;
    double m_base_count;
    unsigned int m_usage;
};



#endif

