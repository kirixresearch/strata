/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#include <wx/wx.h>
#include <xcm/xcm.h>
#include <kl/math.h>
#include "jobqueue.h"
#include "jobqueue_private.h"
#include "util.h"


namespace cfw
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
    m_progress_string_format_nomax = wxEmptyString;
    m_progress_string_format_max = wxEmptyString;
    m_error_string = wxEmptyString;
    m_error_code = 0;
    m_info_mask = jobMaskTitle | jobMaskStartTime |
                  jobMaskFinishTime | jobMaskCurrentCount |
                  jobMaskMaxCount | jobMaskProgressString |
                  jobMaskPercentage | jobMaskProgressBar;
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
        sigStateChanged().fire(static_cast<cfw::IJobInfo*>(this));
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

void JobInfo::setTitle(const wxString& title)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_title = title;
}

wxString JobInfo::getTitle()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_title;
}

void JobInfo::setProgressString(const wxString& new_val)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_progress_string = new_val;
}

// NOTE: in order for getProgressString() to return an empty string,
//       both the progress string and the format string must be empty
wxString JobInfo::getProgressString()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    
    // if a progress string has explicitly been specified, show it
    if (m_progress_string.Length() > 0)
        return m_progress_string;
    
    wxString format_str;
    if (kl::dblcompare(getMaxCount(), 0.0) > 0)
        format_str = m_progress_string_format_max;
         else
        format_str = m_progress_string_format_nomax;
    
    // if a format string has been set, translate the format string and show it
    if (format_str.Length() > 0)
    {
        wxString str, repl;
        wxString progress_str = format_str;

        if (progress_str.Contains(wxT("$c")))
        {
            str  = wxT("$c");
            repl = cfw::dbl2fstr(ceil(getCurrentCount()));
            progress_str.Replace(str.c_str(), repl.c_str());
        }
        
        if (progress_str.Contains(wxT("$C")))
        {
            str  = wxT("$C");
            repl = cfw::dbl2fstr(ceil(getCurrentCount()/1024));
            progress_str.Replace(str.c_str(), repl.c_str());
        }
        
        if (progress_str.Contains(wxT("$m")))
        {
            str  = wxT("$m");
            repl = cfw::dbl2fstr(ceil(getMaxCount()));
            progress_str.Replace(str.c_str(), repl.c_str());
        }
        
        if (progress_str.Contains(wxT("$M")))
        {
            str  = wxT("$M");
            repl = cfw::dbl2fstr(ceil(getMaxCount()/1024));
            progress_str.Replace(str.c_str(), repl.c_str());
        }
        
        if (progress_str.Contains(wxT("$p0")))
        {
            str  = wxT("$p0");
            repl = cfw::dbl2fstr(getPercentage(), 0);
            progress_str.Replace(str.c_str(), repl.c_str());
        }
        
        if (progress_str.Contains(wxT("$p1")))
        {
            str  = wxT("$p1");
            repl = cfw::dbl2fstr(getPercentage(), 1);
            progress_str.Replace(str.c_str(), repl.c_str());
        }
        
        if (progress_str.Contains(wxT("$p2")))
        {
            str  = wxT("$p2");
            repl = cfw::dbl2fstr(getPercentage(), 2);
            progress_str.Replace(str.c_str(), repl.c_str());
        }
        
        return progress_str;
    }
    
    return wxEmptyString;
}

void JobInfo::setProgressStringFormat(const wxString& no_max_count_format,
                                      const wxString& max_count_format)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_progress_string_format_nomax = no_max_count_format;
    m_progress_string_format_max = max_count_format;
}

void JobInfo::setError(int error_code, const wxString& error_string)
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

wxString JobInfo::getErrorString()
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

    if (sigOnGetCurrentCount().isActive())
    {
        sigOnGetCurrentCount().fire(&m_current_count);
    }

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

    if (sigOnGetMaxCount().isActive())
    {
        sigOnGetMaxCount().fire(&m_max_count);
    }

    return m_max_count;
}

double JobInfo::getPercentage()
{
    if (sigOnGetPercentage().isActive())
    {
        double d;
        sigOnGetPercentage().fire(&d);
        return d;
    }

    double max_rows = getMaxCount();
    if (max_rows == 0.0)
        return 0.0;
    double cur_count = getCurrentCount();
    if (cur_count == 0.0)
        return 0.0;

    return (cur_count*100)/max_rows;
}


};

