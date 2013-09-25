/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#include "jobspch.h"
#include "jobinfo.h"


namespace jobs
{


JobInfo::JobInfo()
{
    m_job_id = 0;
    m_cancel_allowed = true;
    m_job_state = jobStatePaused;
    m_visible = true;
    m_start_time = 0;
    m_finish_time = 0;
    m_current_count = 0.0;
    m_max_count = 0.0;
    m_progress_string_format_nomax = L"";
    m_progress_string_format_max = L"";
    m_error_string = L"";
    m_error_code = 0;
    m_info_mask = jobMaskTitle | jobMaskStartTime |
                  jobMaskFinishTime | jobMaskCurrentCount |
                  jobMaskMaxCount | jobMaskProgressString |
                  jobMaskPercentage | jobMaskProgressBar;
}

JobInfo::~JobInfo()
{
}

void JobInfo::setJobId(int job_id)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_job_id = job_id;
}

int JobInfo::getJobId()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_job_id;
}

void JobInfo::setInfoMask(int mask)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_info_mask = mask;
}

int JobInfo::getInfoMask()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_info_mask;
}

void JobInfo::setCancelAllowed(bool new_val)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_cancel_allowed = new_val;
}

bool JobInfo::getCancelAllowed()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_cancel_allowed;
}

int JobInfo::getState()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_job_state;
}

void JobInfo::setState(int new_state)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    if (m_job_state != new_state)
    {
        m_job_state = new_state;
        sigStateChanged().fire(static_cast<IJobInfo*>(this));
    }
}

void JobInfo::setVisible(bool visible)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_visible = visible;
}

bool JobInfo::getVisible()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_visible;
}

void JobInfo::setTitle(const std::wstring& title)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_title = title;
}

std::wstring JobInfo::getTitle()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_title;
}

void JobInfo::setProgressString(const std::wstring& new_val)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_progress_string = new_val;
}

// NOTE: in order for getProgressString() to return an empty string,
//       both the progress string and the format string must be empty
std::wstring JobInfo::getProgressString()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    
    // if a progress string has explicitly been specified, show it
    if (m_progress_string.size() > 0)
        return m_progress_string;
    
    bool maxcount_shown;

    std::wstring* format_str;
    if (kl::dblcompare(getMaxCount(), 0.0) > 0)
    {
        maxcount_shown = true;
        format_str = &m_progress_string_format_max;
    }
     else
    {
        maxcount_shown = false;
        format_str = &m_progress_string_format_nomax;
    }
    
    // if a format string has been set, translate the format string and show it
    if (format_str->size() > 0)
    {
        std::wstring str, repl;
        std::wstring progress_str = *format_str;

        if (progress_str.find(L"$c") != progress_str.npos)
        {
            double cur_count = getCurrentCount();
            if (!maxcount_shown && kl::dblcompare(cur_count, 0.0) == 0)
            {
                // some jobs don't actually incremenet 'current count' past zero;
                // return empty string until current count is greater than zero
                return L"";
            }

            str  = L"$c";
            repl = kl::formattedNumber(ceil(cur_count));
            kl::replaceStr(progress_str, str.c_str(), repl.c_str());
        }
        
        if (progress_str.find(L"$C") != progress_str.npos)
        {
            str  = L"$C";
            repl = kl::formattedNumber(ceil(getCurrentCount()/1024));
            kl::replaceStr(progress_str, str.c_str(), repl.c_str());
        }
        
        if (progress_str.find(L"$m") != progress_str.npos)
        {
            str  = L"$m";
            repl = kl::formattedNumber(ceil(getMaxCount()));
            kl::replaceStr(progress_str, str.c_str(), repl.c_str());
        }
        
        if (progress_str.find(L"$M") != progress_str.npos)
        {
            str  = L"$M";
            repl = kl::formattedNumber(ceil(getMaxCount()/1024));
            kl::replaceStr(progress_str, str.c_str(), repl.c_str());
        }
        
        if (progress_str.find(L"$p0") != progress_str.npos)
        {
            str  = L"$p0";
            repl = kl::formattedNumber(getPercentage(), 0);
            kl::replaceStr(progress_str, str.c_str(), repl.c_str());
        }
        
        if (progress_str.find(L"$p1") != progress_str.npos)
        {
            str  = L"$p1";
            repl = kl::formattedNumber(getPercentage(), 1);
            kl::replaceStr(progress_str, str.c_str(), repl.c_str());
        }
        
        if (progress_str.find(L"$p2") != progress_str.npos)
        {
            str  = L"$p2";
            repl = kl::formattedNumber(getPercentage(), 2);
            kl::replaceStr(progress_str, str.c_str(), repl.c_str());
        }
        
        return progress_str;
    }
    
    return L"";
}

void JobInfo::setProgressStringFormat(const std::wstring& no_max_count_format,
                                      const std::wstring& max_count_format)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_progress_string_format_nomax = no_max_count_format;
    m_progress_string_format_max = max_count_format;
}

void JobInfo::setError(int error_code, const std::wstring& error_string)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_error_code = error_code;
    m_error_string = error_string;
}

int JobInfo::getErrorCode()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_error_code;
}

std::wstring JobInfo::getErrorString()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    
    return m_error_string;
}

void JobInfo::setStartTime(time_t new_val)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_start_time = new_val;
}

time_t JobInfo::getStartTime()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_start_time;
}

void JobInfo::setFinishTime(time_t new_val)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_finish_time = new_val;
}

time_t JobInfo::getFinishTime()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_finish_time;
}

void JobInfo::incrementCurrentCount(double amount)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_current_count += amount;
}


void JobInfo::setCurrentCount(double new_val)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_current_count = new_val;
}

double JobInfo::getCurrentCount()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_current_count;
}

void JobInfo::setMaxCount(double new_val)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_max_count = new_val;
}

double JobInfo::getMaxCount()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_max_count;
}

double JobInfo::getPercentage()
{
    double max_rows = getMaxCount();
    if (max_rows == 0.0)
        return 0.0;
    double cur_count = getCurrentCount();
    if (cur_count == 0.0)
        return 0.0;

    return (cur_count*100)/max_rows;
}


};

