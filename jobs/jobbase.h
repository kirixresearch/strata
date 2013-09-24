/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
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

XCM_CLASS_NAME("jobs.JobBase")
XCM_BEGIN_INTERFACE_MAP(JobBase)
    XCM_INTERFACE_ENTRY(IJob)
XCM_END_INTERFACE_MAP()

XCM_IMPLEMENT_SIGNAL1(sigJobFinished, IJobPtr)

public:

    JobBase()
    {
        m_jobbase_jobid = 0;
        m_sp_job_info = createJobInfoObject();
        m_job_info = m_sp_job_info.p;
        m_cancelling = false;

        std::wstring str1 = L"$c records processed";
        std::wstring str2 = L"$c of $m records processed ($p1%)";
        getJobInfo()->setProgressStringFormat(str1, str2);


        // set the basic configuration:
        /*
            {
                "metadata":
                {
                    "type" : "",
                    "version" : 1,
                    "description" : ""
                },
                "params":
                {
                }
            }
        */


        m_config.setObject();

        kl::JsonNode metadata = m_config["metadata"];
        metadata.setObject();
        metadata["type"].setString(L"");
        metadata["version"].setString(L"");
        metadata["description"].setString(L"");

        kl::JsonNode params = m_config["params"];
        params.setObject();
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

    void setParameters(const std::wstring& json)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        m_config["params"].fromString(json);
    }

    std::wstring getParameters()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        std::wstring res = m_config["params"].toString();
        return res;
    }

    std::wstring toJson()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        std::wstring res = m_config.toString();
        return res;
    }

    void setExtraValue(const std::wstring& key, const std::wstring& value)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        m_extra[key] = value;
    }

    std::wstring getExtraValue(const std::wstring& key)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        return m_extra[key];
    }

    void setResultObject(xcm::IObjectPtr result)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        m_result = result;
    }

    xcm::IObjectPtr getResultObject()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        return m_result;
    }

    virtual void setRefObject(xcm::IObjectPtr ref_object)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        m_ref_object = ref_object;
    }

    void setDatabase(tango::IDatabase* db)
    {
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

    xcm::mutex m_jobbase_mutex;

    IJobInfoPtr m_sp_job_info;
    IJobInfo* m_job_info;
    kl::JsonNode m_config;
    xcm::IObjectPtr m_result;
    std::map<std::wstring, std::wstring> m_extra;
    xcm::IObjectPtr m_ref_object;

private:

    int m_jobbase_jobid;
    bool m_cancelling;
};


}; // namespace jobs


#endif

