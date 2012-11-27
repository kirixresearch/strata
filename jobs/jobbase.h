/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-26
 *
 */


#ifndef __JOBS_JOBBASE_H
#define __JOBS_JOBBASE_H

#include <map>

namespace jobs
{

class JobBase : public IJob
{

XCM_CLASS_NAME("cfw.JobBase")
XCM_BEGIN_INTERFACE_MAP(JobBase)
    XCM_INTERFACE_ENTRY(IJob)
XCM_END_INTERFACE_MAP()

XCM_IMPLEMENT_SIGNAL1(sigJobFinished, IJobPtr)

public:

    JobBase()
    {
        m_jobbase_jobid = 0;
        m_extra_long = 0;
        m_sp_job_info = createJobInfoObject();
        m_job_info = m_sp_job_info.p;
        m_cancelling = false;

        std::wstring str1 = L"$c records processed";
        std::wstring str2 = L"$c of $m records processed ($p1%)";
        getJobInfo()->setProgressStringFormat(str1, str2);
    }

    int getJobId()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return m_jobbase_jobid;
    }

    void setJobId(int job_id)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_jobbase_jobid = job_id;
    }

    IJobInfoPtr getJobInfo()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return m_job_info;
    }

    void setJobInfo(IJobInfoPtr new_val)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_job_info = new_val;
    }

    long getExtraLong()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return m_extra_long;
    }

    void setExtraLong(long value)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_extra_long = value;
    }

    void setExtraString(const std::wstring& s)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_extra_string = s;
    }

    std::wstring getExtraString()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return m_extra_string;
    }

    void setParameter(const std::wstring& param, const std::wstring& value)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        
        m_config[param] = value;
    }

    void setInstructions(const std::wstring& json)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        m_config.fromString(json);
    }

    std::wstring getParameter(const std::wstring& param)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        
        if (!m_config.childExists(param))
            return L"";
             else
            m_config.getChild(param).getString();
    }

    bool cancel()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        if (!m_job_info->getCancelAllowed())
            return false;

        m_job_info->setState(jobStateCancelling);
        m_cancelling = true;

        return true;
    }

    bool isCancelling()
    {
        return m_cancelling ? true : false;
    }

    void runPostJob()
    {
    }
    
protected:

    IJobInfoPtr m_sp_job_info;
    IJobInfo* m_job_info;
    std::wstring m_extra_string;
    long m_extra_long;

private:

    int m_jobbase_jobid;
    xcm::mutex m_jobbase_mutex;
    bool m_cancelling;

    kl::JsonNode m_config;
};

}; // namespace jobs

#endif
