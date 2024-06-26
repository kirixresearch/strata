/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-05-17
 *
 */


#include <ctime>
#include <xd/xd.h>
#include "jobinfo.h"


JobInfo::JobInfo()
{
    m_jobid = 0;
    m_start_time = 0;
    m_finish_time = 0;
    m_description = L"";
    m_progress_str = L"";
    m_status = xd::jobStopped;
    m_current_count = 0;
    m_max_count = 0;
    m_cancelled = false;
    m_can_cancel = true;
    m_base_pct = 0;
    m_phase_count = 1;
    m_phase_counter = 0;
    m_phase_pct = 100;
    m_percentage_set = false;
    m_percentage = 0.0;
}

JobInfo::~JobInfo()
{
}

void JobInfo::setJobId(xd::jobid_t new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_jobid = new_val;
}

void JobInfo::setCanCancel(bool new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_can_cancel = new_val;
}

void JobInfo::setStartTime(time_t new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);
    
    m_start_time = new_val;
}

void JobInfo::setFinishTime(time_t new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_finish_time = new_val;
}

void JobInfo::setDescription(const std::wstring& new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_description = new_val;
}

void JobInfo::setProgressString(const std::wstring& new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_progress_str = new_val;
}

void JobInfo::setStatus(int new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_status = new_val;
}


void JobInfo::setCurrentCount(xd::rowpos_t new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_current_count = new_val;
}

void JobInfo::setMaxCount(xd::rowpos_t new_val)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_max_count = new_val;
}

void JobInfo::setPercentage(double percentage)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_percentage_set = true;
    m_percentage = percentage;
}

void JobInfo::setPhases(int phase_count, int* phase_pcts)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_phase_count = phase_count;

    m_phase_pcts.clear();
    int i;
    for (i = 0; i < phase_count; ++i)
    {
        m_phase_pcts.push_back(*(phase_pcts+i));
    }
}

void JobInfo::startPhase(xd::IJobPtr embedded_job_info)
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_current_count = 0;

    if (m_start_time == 0)
    {
        m_start_time = time(NULL);
    }
    m_status = xd::jobRunning;
    m_phase_job = embedded_job_info;

    if (m_phase_counter == 0)
    {
        m_base_pct = 0;
    }
     else
    {
        m_base_pct += m_phase_pct;
    }

    if (m_phase_counter < 0 || (size_t)m_phase_counter >= m_phase_pcts.size())
    {
        m_phase_pct = 100;
        m_base_pct = 0;
    }
     else
    {
        m_phase_pct = m_phase_pcts[m_phase_counter];
    }

    if (m_base_pct >= 100)
    {
        m_base_pct = 0;
    }

    m_phase_counter++;
}

void JobInfo::setJobFinished()
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_finish_time = time(NULL);
    m_status = xd::jobFinished;
}

void JobInfo::setError(int error_code, const std::wstring& error_string)
{
    KL_AUTO_LOCK(m_obj_mutex);
    
    m_error_code = error_code;
    m_error_string = error_string;
}

xd::jobid_t JobInfo::getJobId()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_jobid;
}

bool JobInfo::getCanCancel()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_can_cancel;
}

bool JobInfo::getCancelled()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_cancelled;
}

time_t JobInfo::getStartTime()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_start_time;
}

time_t JobInfo::getFinishTime()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_finish_time;
}

std::wstring JobInfo::getDescription()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_description;
}

std::wstring JobInfo::getProgressString()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_progress_str;
}

int JobInfo::getStatus()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_status;
}

xd::rowpos_t JobInfo::getCurrentCount()
{
    KL_AUTO_LOCK(m_obj_mutex);

    if (m_phase_job.p)
    {
        return m_phase_job.p->getCurrentCount();
    }

    return m_current_count;
}

xd::rowpos_t JobInfo::getMaxCount()
{
    KL_AUTO_LOCK(m_obj_mutex);

    //if (m_phase_job.p)
    //{
    //    return m_phase_job.p->getMaxCount();
    //}

    return m_max_count;
}


int JobInfo::getPhaseCount()
{
    KL_AUTO_LOCK(m_obj_mutex);

    return m_phase_count;
}

int JobInfo::getCurrentPhase()
{
    KL_AUTO_LOCK(m_obj_mutex);

    if (m_phase_counter == 0)
        return 0;

    return m_phase_counter-1;
}

double JobInfo::getPercentage()
{
    KL_AUTO_LOCK(m_obj_mutex);

    if (m_phase_job.p)
    {
        double pct = m_phase_job.p->getPercentage();
        return m_base_pct+((pct*m_phase_pct)/100);
    }

    if (m_percentage_set)
        return m_percentage;
    
    if (m_max_count == 0)
        return 0.0;

    double m = (double)(long long)getMaxCount();
    double c = (double)(long long)getCurrentCount();

    return m_base_pct+((c*m_phase_pct)/m);
}

int JobInfo::getErrorCode()
{
    KL_AUTO_LOCK(m_obj_mutex);
    
    return m_error_code;
}

std::wstring JobInfo::getErrorString()
{
    KL_AUTO_LOCK(m_obj_mutex);
    
    return m_error_string;
}

bool JobInfo::cancel()
{
    KL_AUTO_LOCK(m_obj_mutex);

    if (!m_can_cancel)
    {
        return false;
    }

    if (m_phase_job.p)
    {
        if (m_phase_job.p->cancel())
        {
            m_status = xd::jobCancelled;
            m_cancelled = true;
            return true;
        }
         else
        {
            return false;
        }
    }

    m_status = xd::jobCancelled;
    m_cancelled = true;
    return true;
}






