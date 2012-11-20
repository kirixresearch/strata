/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#ifndef __APP_JOBQUEUE_PRIVATE_H
#define __APP_JOBQUEUE_PRIVATE_H





/*
    progress format string parameters:

    $c = current count
    $C = current count / 1024
    $m = maximum count
    $M = maximum count / 1024
    $p0 = percentage (0 decimal places)
    $p1 = percentage (1 decimal place)
    $p2 = percentage (2 decimal places)
*/

class JobInfo : public IJobInfo
{
    XCM_CLASS_NAME("cfw.JobInfo")
    XCM_BEGIN_INTERFACE_MAP(JobInfo)
        XCM_INTERFACE_ENTRY(IJobInfo)
    XCM_END_INTERFACE_MAP()

    XCM_IMPLEMENT_SIGNAL1(sigOnGetCurrentCount, double*)
    XCM_IMPLEMENT_SIGNAL1(sigOnGetMaxCount, double*)
    XCM_IMPLEMENT_SIGNAL1(sigOnGetPercentage, double*)
    XCM_IMPLEMENT_SIGNAL1(sigStateChanged, IJobInfoPtr)

public:

    JobInfo();

    void setJobId(int job_id);
    int getJobId();

    void setInfoMask(int mask);
    int getInfoMask();

    void setId(int id);
    int getId();

    void setCancelAllowed(bool new_val);
    bool getCancelAllowed();

    void setState(int new_val);
    int getState();

    void setVisible(bool new_val);
    bool getVisible();

    void setTitle(const std::wstring& new_val);
    std::wstring getTitle();

    // see note above on how to create a format string
    void setProgressStringFormat(const std::wstring& no_max_count_format,
                                 const std::wstring& max_count_format);
    void setProgressString(const std::wstring& new_val);
    std::wstring getProgressString();

    void setError(int error_code, const std::wstring& error_string);
    int getErrorCode();
    std::wstring getErrorString();
    
    void setStartTime(time_t new_val);
    time_t getStartTime();

    void setFinishTime(time_t new_val);
    time_t getFinishTime();

    void incrementCurrentCount(double amount);
    void setCurrentCount(double new_val);
    double getCurrentCount();

    void setMaxCount(double new_val);
    double getMaxCount();

    double getPercentage();

private:

    xcm::mutex m_obj_mutex;
    time_t m_start_time;
    int m_job_id;
    int m_job_state;
    bool m_visible;
    std::wstring m_title;
    std::wstring m_progress_string;
    std::wstring m_progress_string_format_nomax;
    std::wstring m_progress_string_format_max;
    std::wstring m_error_string;
    int m_error_code;
    time_t m_finish_time;
    time_t m_current_time;
    double m_current_count;
    double m_max_count;
    bool m_cancel_allowed;
    int m_info_mask;
};



class JobQueue : public wxEvtHandler,
                 public IJobQueue,
                 public xcm::signal_sink
{
    friend class JobThread;
    
    XCM_CLASS_NAME("cfw.JobQueue")
    XCM_BEGIN_INTERFACE_MAP(JobQueue)
        XCM_INTERFACE_ENTRY(IJobQueue)
    XCM_END_INTERFACE_MAP()

    XCM_IMPLEMENT_SIGNAL0(sigQueueChanged);
    XCM_IMPLEMENT_SIGNAL1(sigJobAdded, IJobInfoPtr);

public:
    
    JobQueue();
    ~JobQueue();

    int addJob(IJobPtr job, int initial_state);
    int addJobInfo(IJobInfoPtr job, int initial_state);

    IJobPtr lookupJob(int job_id);
    bool startJob(int job_id);

    IJobInfoPtr getJobInfo(int job_id);
    IJobInfoEnumPtr getJobInfoEnum(int job_state_mask);
    bool getJobsActive();

private:

    bool startJob(IJobPtr& job);
    void incrementActiveJobs();
    void decrementActiveJobs();

    void onJobFinished(wxCommandEvent& evt);
    void onJobInfoEntryStateChanged(IJobInfoPtr job_info);
    
private:

    xcm::mutex m_obj_mutex;
    std::vector<IJobPtr> m_jobs;
    std::vector<IJobInfoPtr> m_job_info;
    int m_job_id_counter;
    
    xcm::mutex m_active_jobs_mutex;
    int m_active_jobs;

    DECLARE_EVENT_TABLE()
};




#endif

