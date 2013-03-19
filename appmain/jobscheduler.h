/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-09-29
 *
 */


#ifndef __APP_JOBSCHEDULER_H
#define __APP_JOBSCHEDULER_H


class JobSchedulerEntry
{
friend class JobScheduler;

public:

    enum Schedule
    {
        Once = 0,
        Daily,
        Weekly,
        Monthly
    };

    enum Days
    {
        InvalidDay = 0x00,
        Mon        = 0x01,
        Tue        = 0x02,
        Wed        = 0x04,
        Thu        = 0x08,
        Fri        = 0x10,
        Sat        = 0x20,
        Sun        = 0x40,
        Weekdays   = Mon|Tue|Wed|Thu|Fri,
        Weekend    = Sat|Sun,
        AllDays    = Mon|Tue|Wed|Thu|Fri|Sat|Sun,
    };

public:

    JobSchedulerEntry()
    {
        name = wxEmptyString;
        schedule = JobSchedulerEntry::Once;
        active = true;
        
        start_time = 0;
        finish_time = 0;
        finish_active = false;
        
        daily_repeat_active = false;
        daily_repeat_interval = 30;
        weekly_days = JobSchedulerEntry::Mon;
        monthly_daynumber = 1;
        
        next_run = 0;
    }

public:

    wxString name;
    Schedule schedule;          // daily, weekly, etc.
    bool active;                // only active jobs will run
    
    time_t start_time;
    time_t finish_time;
    bool finish_active;
    
    bool daily_repeat_active;
    int daily_repeat_interval;  // in minutes
    int weekly_days;            // day bitmask
    int monthly_daynumber;      // 1-31
    
    std::vector<wxString> commands;

private:

    time_t next_run;
};



class JobScheduler : wxEvtHandler
{
public:

    JobScheduler();
    ~JobScheduler();

    bool save();
    bool load();
    
    void setInterval(int seconds);
    void clearJobs();
    void addJob(const JobSchedulerEntry& entry);
    std::vector<JobSchedulerEntry> getJobs();

private:

    void checkForJobs();
    void runJob(JobSchedulerEntry& e);
    void calcNextRun(JobSchedulerEntry& e);
    void updateJobVersion(JobSchedulerEntry& e, int old_interval);

    void onTimer(wxTimerEvent& event);

private:

    std::vector<JobSchedulerEntry> m_jobs;
    wxTimer m_fast_timer;
    wxTimer m_timer;

    xcm::mutex m_obj_mutex;

    DECLARE_EVENT_TABLE()
};





#endif

