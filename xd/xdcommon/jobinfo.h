/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-05-17
 *
 */


#ifndef __XDCOMMON_JOBINFO_H
#define __XDCOMMON_JOBINFO_H


xcm_interface IJobInternal : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IJobInternal")

public:

    virtual void setJobId(xd::jobid_t new_val) = 0;
    virtual void setCanCancel(bool new_val) = 0;

    virtual void setStartTime(time_t new_val) = 0;
    virtual void setFinishTime(time_t new_val) = 0;

    virtual void setDescription(const std::wstring& new_val) = 0;
    virtual void setProgressString(const std::wstring& new_val) = 0;
    virtual void setStatus(int new_val) = 0;

    virtual void setCurrentCount(xd::rowpos_t new_val) = 0;
    virtual void setMaxCount(xd::rowpos_t new_val) = 0;
    virtual void setPercentage(double percentage) = 0;

    virtual void setPhases(int new_val, int* phase_pcts = NULL) = 0;
    virtual void startPhase(xd::IJobPtr embedded_job_info = xcm::null) = 0;
    virtual void setJobFinished() = 0;
    virtual void setError(int error_code, const std::wstring& error_string) = 0;
    
    virtual bool getCancelled() = 0;
    virtual xd::rowpos_t getMaxCount() = 0;
};

XCM_DECLARE_SMARTPTR(IJobInternal)




class JobInfo : public xd::IJob,
                public IJobInternal
{
    XCM_CLASS_NAME("xd.Job")
    XCM_BEGIN_INTERFACE_MAP(JobInfo)
        XCM_INTERFACE_ENTRY(xd::IJob)
        XCM_INTERFACE_ENTRY(IJobInternal)
    XCM_END_INTERFACE_MAP()

public:

    JobInfo();
    virtual ~JobInfo();

    // IJobInfoInternal
    void setJobId(xd::jobid_t new_val);
    void setCanCancel(bool new_val);

    void setStartTime(time_t new_val);
    void setFinishTime(time_t new_val);

    void setDescription(const std::wstring& new_val);
    void setProgressString(const std::wstring& new_val);
    void setStatus(int new_val);

    void setCurrentCount(xd::rowpos_t new_val);
    void setMaxCount(xd::rowpos_t new_val);
    void setPercentage(double pct);

    void setPhases(int new_val, int* phase_pcts);
    void startPhase(xd::IJobPtr embedded_job_info);
    void setJobFinished();
    void setError(int error_code, const std::wstring& error_string);

    // IJobInfo
    xd::jobid_t getJobId();
    bool getCanCancel();
    bool getCancelled();
    bool cancel();

    time_t getStartTime();
    time_t getFinishTime();

    std::wstring getDescription();
    std::wstring getProgressString();
    int getStatus();

    xd::rowpos_t getCurrentCount();
    xd::rowpos_t getMaxCount();
    double getPercentage();

    int getPhaseCount();
    int getCurrentPhase();

    int getErrorCode();
    std::wstring getErrorString();

protected:

    kl::mutex m_obj_mutex;

    xd::jobid_t m_jobid;
    bool m_can_cancel;
    bool m_cancelled;

    bool m_percentage_set;
    double m_percentage;

    time_t m_start_time;
    time_t m_finish_time;
    std::wstring m_description;
    std::wstring m_progress_str;
    int m_status;
    xd::rowpos_t m_current_count;
    xd::rowpos_t m_max_count;

    int m_error_code;
    std::wstring m_error_string;

    int m_base_pct;
    int m_phase_count;
    int m_phase_counter;
    int m_phase_pct;
    std::vector<int> m_phase_pcts;
    xd::IJobPtr m_phase_job;
};




#endif

