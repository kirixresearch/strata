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



JobStat::JobStat()
{
    m_id = 0;
    m_index = 0;
    m_caption = wxEmptyString;
    m_value = wxEmptyString;
}


void JobStat::setIndex(int new_val)
{
    m_index = new_val;
}

int JobStat::getIndex()
{
    return m_index;
}

void JobStat::setId(int id)
{
    m_id = id;
}

int JobStat::getId()
{
    return m_id;
}

void JobStat::setCaption(const wxString& new_val)
{
    m_caption = new_val;
}

wxString JobStat::getCaption()
{
    return m_caption;
}

void JobStat::setValue(const wxString& new_val)
{
    m_value = new_val;
}

wxString JobStat::getValue()
{
    return m_value;
}




class JobCurrentCountStat : public JobStat
{
    XCM_CLASS_NAME("cfw.JobCurrentCountStat")
    XCM_BEGIN_INTERFACE_MAP(JobStat)
        XCM_INTERFACE_CHAIN(JobStat)
    XCM_END_INTERFACE_MAP()

public:

    JobCurrentCountStat(JobInfo* info)
    {
        m_job_info = info;
        setCaption(_("Records Processed:"));
        setId(jobStatCurrentCount);
    }

    wxString getValue()
    {
        return dbl2fstr(m_job_info->getCurrentCount(), 0);
    }

private:
    
    JobInfo* m_job_info;
};


class JobMaxCountStat : public JobStat
{
    XCM_CLASS_NAME("cfw.JobMaxCountStat")
    XCM_BEGIN_INTERFACE_MAP(JobStat)
        XCM_INTERFACE_CHAIN(JobStat)
    XCM_END_INTERFACE_MAP()

public:

    JobMaxCountStat(JobInfo* info)
    {
        m_job_info = info;
        setCaption(_("Total Records:"));
        setId(jobStatMaxCount);
    }

    wxString getValue()
    {
        return dbl2fstr(m_job_info->getMaxCount(), 0);
    }

private:
    
    JobInfo* m_job_info;
};


class JobPercentageStat : public JobStat
{
    XCM_CLASS_NAME("cfw.JobPercentageStat")
    XCM_BEGIN_INTERFACE_MAP(JobStat)
        XCM_INTERFACE_CHAIN(JobStat)
    XCM_END_INTERFACE_MAP()

public:

    JobPercentageStat(JobInfo* info)
    {
        m_job_info = info;
        setCaption(_("Percentage:"));
        setId(jobStatPercentage);
    }

    wxString getValue()
    {
        return dbl2fstr(m_job_info->getPercentage(), 1) + wxT("%");
    }

private:
    
    JobInfo* m_job_info;
};


class JobElapsedTimeStat : public JobStat
{
    XCM_CLASS_NAME("cfw.JobElapsedTimeStat")
    XCM_BEGIN_INTERFACE_MAP(JobStat)
        XCM_INTERFACE_CHAIN(JobStat)
    XCM_END_INTERFACE_MAP()

public:

    JobElapsedTimeStat(JobInfo* info)
    {
        m_job_info = info;
        setCaption(_("Elapsed Time:"));
        setId(jobStatElapsedTime);
    }

    wxString getValue()
    {
        int job_state = m_job_info->getState();
        if (job_state == cfw::jobStatePaused)
            return _("Paused");
             else if (job_state == cfw::jobStateQueued)
            return _("Queued");

        // calcuate the duration of the job
        time_t start_time = m_job_info->getStartTime();
        time_t finish_time = m_job_info->getFinishTime();
        time_t duration, current_time = time(NULL);
        
        if (job_state == cfw::jobStateCancelled ||
            job_state == cfw::jobStateFinished ||
            job_state == cfw::jobStateFailed)
        {
            time_t finish_time = m_job_info->getFinishTime();
            duration = finish_time - start_time;
        }
         else if (job_state == cfw::jobStateQueued)
        {
            duration = 0;
        }
         else
        {
            duration = current_time - start_time;
        }
        
        // create the elapsed time string
        int hours = (duration/3600);
        duration -= (hours*3600);
        int minutes = (duration/60);
        int seconds = duration%60;
        
        // make sure there is at least a 1 second duration
        if (hours == 0 && minutes == 0 && seconds == 0)
            seconds = 1;
            
        wxString time_str = wxString::Format(_("%02d:%02d:%02d elapsed"),
                                             hours, minutes, seconds);
        
        // create the finish date and time string
        wxString finish_time_str, finish_date_str;
        if (finish_time > 0)
        {
            wxDateTime now = wxDateTime(finish_time);
            finish_date_str = now.Format(wxT("%A, %B %d, %Y"));
            finish_time_str = now.Format(wxT("%X"));
        }
        
        switch (job_state)
        {
            case cfw::jobStateRunning:
                return wxString::Format(_("Running (%s)"), time_str.c_str());

            case cfw::jobStateFinished:
                return wxString::Format(_("Finished on %s at %s (%s)"),
                                        finish_date_str.c_str(),
                                        finish_time_str.c_str(),
                                        time_str.c_str());

            case cfw::jobStateCancelling:
                return wxString::Format(_("Cancelling (%s)"), time_str.c_str());

            case cfw::jobStateCancelled:
                return wxString::Format(_("Cancelled on %s at %s (%s)"),
                                        finish_date_str.c_str(),
                                        finish_time_str.c_str(),
                                        time_str.c_str());
            case cfw::jobStateFailed:
                return wxString::Format(_("Failed on %s at %s (%s)"),
                                        finish_date_str.c_str(),
                                        finish_time_str.c_str(),
                                        time_str.c_str());
        }
        
        return dbl2fstr(m_job_info->getPercentage(), 1) + wxT("%");
    }

private:
    
    JobInfo* m_job_info;
};




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

    m_stats.push_back(static_cast<IJobStat*>(new JobCurrentCountStat(this)));
    m_stats.push_back(static_cast<IJobStat*>(new JobMaxCountStat(this)));
    m_stats.push_back(static_cast<IJobStat*>(new JobPercentageStat(this)));
    m_stats.push_back(static_cast<IJobStat*>(new JobElapsedTimeStat(this)));
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

IJobStatEnumPtr JobInfo::getStatInfo()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<IJobStatPtr>* vec = new xcm::IVectorImpl<IJobStatPtr>;

    std::vector<IJobStatPtr>::iterator it;
    for (it = m_stats.begin(); it != m_stats.end(); ++it)
    {
        vec->append(*it);
    }

    return vec;
}

int JobInfo::getStatCount()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_stats.size();
}

IJobStatPtr JobInfo::getStatById(int id)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::vector<IJobStatPtr>::iterator it;
    for (it = m_stats.begin(); it != m_stats.end(); ++it)
    {
        if ((*it)->getId() == id)
            return (*it);
    }

    return xcm::null;
}

IJobStatPtr JobInfo::insertStat(int position)
{
    XCM_AUTO_LOCK(m_obj_mutex);
    
    IJobStatPtr stat = static_cast<IJobStat*>(new JobStat);

    if (position >= 0)
    {
        m_stats.insert(m_stats.begin() + position, stat);
    }
     else
    {
        m_stats.push_back(stat);
    }

    return stat;
}

bool JobInfo::deleteStat(unsigned int position)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    if (position >= m_stats.size())
        return false;

    m_stats.erase(m_stats.begin() + position);

    return true;
}

bool JobInfo::deleteStatById(int id)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::vector<IJobStatPtr>::iterator it;
    for (it = m_stats.begin(); it != m_stats.end(); ++it)
    {
        if ((*it)->getId() == id)
        {
            m_stats.erase(it);
            return true;
        }
    }

    return false;
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

void JobInfo::setDescription(const wxString& desc)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_description = desc;
}

wxString JobInfo::getDescription()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_description;
}

void JobInfo::setBitmap(const wxBitmap& bitmap)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_bitmap = bitmap;
}

wxBitmap JobInfo::getBitmap()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_bitmap;
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

double JobInfo::getProgressBarPos()
{
    return getPercentage();
}


};

