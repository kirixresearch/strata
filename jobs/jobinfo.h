/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-27
 *
 */


#ifndef __JOBS_JOBINFO_H
#define __JOBS_JOBINFO_H


namespace jobs
{


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
    XCM_CLASS_NAME("jobs.JobInfo")
    XCM_BEGIN_INTERFACE_MAP(JobInfo)
        XCM_INTERFACE_ENTRY(IJobInfo)
    XCM_END_INTERFACE_MAP()

    XCM_IMPLEMENT_SIGNAL1(sigStateChanged, IJobInfoPtr)

public:

    JobInfo();
    virtual ~JobInfo();

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
    void getProgressStringFormat(std::wstring* no_max_count_format,
                                 std::wstring* max_count_format);

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

protected:

    kl::mutex m_obj_mutex;
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


};


#endif

