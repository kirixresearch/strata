/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-09-29
 *
 */


#include "appmain.h"
#include "jobscheduler.h"
#include "appcontroller.h"
#include <wx/datectrl.h>
#include <kl/thread.h>


const int SECONDS_IN_DAY = 86400;


// -- JobSchedulerThread class --

class JobSchedulerThread : public kl::Thread,
                           public xcm::signal_sink
{
public:

    JobSchedulerThread(const JobSchedulerEntry& entry) : kl::Thread()
    {
        m_entry = entry;

        m_ready = true;
        m_exit = false;
    }

    ~JobSchedulerThread()
    {
    }

    unsigned int entry()
    {
        std::vector<wxString>::iterator it;

        for (it = m_entry.commands.begin();
             it != m_entry.commands.end(); ++it)
        {
            while (1)
            {
                kl::Thread::sleep(1000);
                
                m_obj_mutex.lock();
                if (m_exit)
                {
                    m_obj_mutex.unlock();
                    return 0;
                }

                if (m_ready)
                {
                    m_ready = false;
                    m_obj_mutex.unlock();
                    break;
                }
                m_obj_mutex.unlock();
            }

            IJobPtr job = g_app->getAppController()->execute(*it);
            if (job.isNull())
                return 0;

            job->sigJobFinished().connect(this, &JobSchedulerThread::onJobFinished);
        }

        return 0;
    }

    void onJobFinished(IJobPtr job)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        if (job->getJobInfo()->getState() == jobStateCancelled)
        {
            m_exit = true;
        }

        m_ready = true;
    }

private:

    xcm::mutex m_obj_mutex;
    JobSchedulerEntry m_entry;
    bool m_ready;               // thread is ready for the job's next command
    bool m_exit;                // error encountered during last command (thread should exit)
};








enum
{
    ID_Timer = 22114,
    ID_FastTimer
};


BEGIN_EVENT_TABLE(JobScheduler, wxEvtHandler)
    EVT_TIMER(ID_Timer, JobScheduler::onTimer)
    EVT_TIMER(ID_FastTimer, JobScheduler::onTimer)
END_EVENT_TABLE()


JobScheduler::JobScheduler()
{
    m_timer.SetOwner(this, ID_Timer);
    m_fast_timer.SetOwner(this, ID_FastTimer);
}

JobScheduler::~JobScheduler()
{
}

void JobScheduler::setInterval(int seconds)
{
    m_timer.Start(seconds*1000);
}

void JobScheduler::onTimer(wxTimerEvent& event)
{
    checkForJobs();
}

void JobScheduler::clearJobs()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_jobs.clear();
}

inline void getWeeklyRepeatDay(JobSchedulerEntry& e)
{
    // this function makes sure that when we convert an "intervaled" job to
    // a weekly repeating job that we get the right day to repeat the job
    
    wxDateTime dt = wxDateTime(e.start_time);
    wxDateTime::WeekDay weekday = dt.GetWeekDay();
    switch (weekday)
    {
        default:
        case wxDateTime::Mon:  e.weekly_days = JobSchedulerEntry::Mon;  break;
        case wxDateTime::Tue:  e.weekly_days = JobSchedulerEntry::Tue;  break;
        case wxDateTime::Wed:  e.weekly_days = JobSchedulerEntry::Wed;  break;
        case wxDateTime::Thu:  e.weekly_days = JobSchedulerEntry::Thu;  break;
        case wxDateTime::Fri:  e.weekly_days = JobSchedulerEntry::Fri;  break;
        case wxDateTime::Sat:  e.weekly_days = JobSchedulerEntry::Sat;  break;
        case wxDateTime::Sun:  e.weekly_days = JobSchedulerEntry::Sun;  break;
    }
}

inline void getMonthlyRepeatDayNumber(JobSchedulerEntry& e)
{
    // this function makes sure that when we convert an "intervaled" job to
    // a weekly repeating job that we get the right day to repeat the job
    
    wxDateTime dt = wxDateTime(e.start_time);
    e.monthly_daynumber = dt.GetDay();
}

void JobScheduler::addJob(const JobSchedulerEntry& _e)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    JobSchedulerEntry e = _e;
    calcNextRun(e);
    m_jobs.push_back(e);
}

void JobScheduler::runJob(JobSchedulerEntry& e)
{
    JobSchedulerThread* job_thread = new JobSchedulerThread(e);
    job_thread->create();
}

void JobScheduler::calcNextRun(JobSchedulerEntry& job)
{
    // this function will find the next run time, which is the
    // first time in the future which corresponds to the start
    // time plus any specified interval
    
    time_t now;
    time(&now);
    
    // the job has already been scheduled and is set to run
    if (job.next_run > now)
        return;

    // the job was scheduled to run one and it's run time has past
    if (job.start_time < now && job.schedule == JobSchedulerEntry::Once)
        return;
        
    // ... otherwise, recalculate the job's next run time based
    //     on its starting time and parameters
    
    job.next_run = job.start_time;
    
    switch (job.schedule)
    {
        case JobSchedulerEntry::Once:
            return;
            
        case JobSchedulerEntry::Daily:
        {
            if (job.daily_repeat_active)
            {
                // add the daily repeat interval (in minutes) until we find
                // a time that is in the future from right now
                while (job.next_run <= now)
                    job.next_run += (job.daily_repeat_interval*60);
            }
             else
            {
                // add the number of seconds in a day until we find
                // a time that is in the future from right now
                while (job.next_run <= now)
                    job.next_run += SECONDS_IN_DAY;
            }
            
            return;
        }
            
        case JobSchedulerEntry::Weekly:
        {
            // add the number of seconds in a day until we find a time
            // that is on the first specified day of the week in the future
            
            wxDateTime dt(job.next_run);
            wxDateTime::WeekDay weekday = dt.GetWeekDay();
            
            while (1)
            {
                if (job.next_run >= now)
                {
                    if (job.weekly_days == JobSchedulerEntry::InvalidDay)
                    {
                        // no days were specified for the job, bail out
                        job.next_run = 0;
                        break;
                    }
                     else if (weekday == wxDateTime::Mon && (job.weekly_days & JobSchedulerEntry::Mon))
                        break;
                     else if (weekday == wxDateTime::Tue && (job.weekly_days & JobSchedulerEntry::Tue))
                        break;
                     else if (weekday == wxDateTime::Wed && (job.weekly_days & JobSchedulerEntry::Wed))
                        break;
                     else if (weekday == wxDateTime::Thu && (job.weekly_days & JobSchedulerEntry::Thu))
                        break;
                     else if (weekday == wxDateTime::Fri && (job.weekly_days & JobSchedulerEntry::Fri))
                        break;
                     else if (weekday == wxDateTime::Sat && (job.weekly_days & JobSchedulerEntry::Sat))
                        break;
                     else if (weekday == wxDateTime::Sun && (job.weekly_days & JobSchedulerEntry::Sun))
                        break;
                }
        
                // increment the job's next run time until its
                // next run time is on the given day of the week
                job.next_run += SECONDS_IN_DAY;
                dt.Set(job.next_run);
                weekday = dt.GetWeekDay();
            }
            
            return;
        }
            
        case JobSchedulerEntry::Monthly:
        {
            // calculate the next run day based on the
            // job's monthly day number
            
            wxDateTime now(wxDateTime::Now());
            wxDateTime next_run(job.next_run);
            next_run.SetYear(now.GetYear());
            next_run.SetMonth(now.GetMonth());


            // days of the month such as 30, etc. can cause trouble because
            // of months like February that don't have that many days, so
            // we'll do a check to see if that is the case, and if it is,
            // we'll set the job to run on the last day of that month
            
            int days_in_month = wxDateTime::GetNumberOfDays(now.GetMonth(),
                                                            now.GetYear());
            if (job.monthly_daynumber > days_in_month)
            {
                next_run.SetDay(days_in_month);
            }
             else
            {
                next_run.SetDay(job.monthly_daynumber);
            }
            
            // the next run is set to happen before the current time,
            // so we need to move it to the next month
            if (next_run.IsEarlierThan(now))
            {
                wxDateSpan one_month(0,1,0,0);
                next_run.Add(one_month);
            }
            
            // set the job's next run time
            job.next_run = next_run.GetTicks();
            return;
        }
    }
}

void JobScheduler::checkForJobs()
{
    time_t cur_time = time(NULL);

    bool fast_timer = false;
    int slow_interval = m_timer.GetInterval()/1000;
    
    std::vector<JobSchedulerEntry>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        // make sure we keep inactive jobs' next run time updated
        if (!it->active)
        {
            calcNextRun(*it);
            continue;
        }
        
        // the job has reached its endpoint
        if (it->finish_active && it->finish_time < cur_time)
        {
            calcNextRun(*it);
            continue;
        }
        
        // the job has expired (its next run time is before the current time)
        if (it->next_run < cur_time)
        {
            calcNextRun(*it);
            continue;
        }
                    
        // we're close to a pending job so speed up the timer
        if (cur_time+slow_interval >= it->next_run)
            fast_timer = true;
        
        // we've hit the job's next run time so run it
        if (cur_time >= it->next_run)
            runJob(*it);
        
        // always calculate the next run time for the job
        calcNextRun(*it);
    }
    
    if (fast_timer)
    {
        if (!m_fast_timer.IsRunning())
            m_fast_timer.Start(1000);
    }
     else
    {
        m_fast_timer.Stop();
    }
}

std::vector<JobSchedulerEntry> JobScheduler::getJobs()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    return m_jobs;
}

bool JobScheduler::save()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    tango::IDatabasePtr db = g_app->getDatabase();
    if (!db)
        return false;


    // -- save jobs --
    wxString path;
    path = wxString::Format(wxT("/.appdata/%s/dcfe/jobscheduler"),
                            towx(g_app->getDatabase()->getActiveUid()).c_str());

    tango::INodeValuePtr jobscheduler_file = db->createNodeFile(towstr(path));
    if (!jobscheduler_file)
        return false;

    tango::INodeValuePtr version_node = jobscheduler_file->createChild(L"version");
    version_node->setInteger(2);

    tango::INodeValuePtr jobs_node = jobscheduler_file->createChild(L"jobs");
    int counter = 0;

    std::vector<JobSchedulerEntry>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        wchar_t buf[255];
        swprintf(buf, 255, L"job_%03d", counter++);

        tango::INodeValuePtr job_node = jobs_node->createChild(buf);

        tango::INodeValuePtr name_node = job_node->createChild(L"name");
        name_node->setString(towstr(it->name));

        tango::INodeValuePtr schedule_node = job_node->createChild(L"schedule");
        schedule_node->setInteger(it->schedule);

        tango::INodeValuePtr active_node = job_node->createChild(L"active");
        active_node->setBoolean(it->active);

        tango::INodeValuePtr starttime_node = job_node->createChild(L"start_time");
        starttime_node->setInteger(it->start_time);

        tango::INodeValuePtr finishtime_node = job_node->createChild(L"finish_time");
        finishtime_node->setInteger(it->finish_time);
        
        tango::INodeValuePtr finishactive_node = job_node->createChild(L"finish_active");
        finishactive_node->setBoolean(it->finish_active);

        tango::INodeValuePtr dailyrepeat_node = job_node->createChild(L"daily_repeat_active");
        dailyrepeat_node->setBoolean(it->daily_repeat_active);

        tango::INodeValuePtr dailyinterval_node = job_node->createChild(L"daily_repeat_interval");
        dailyinterval_node->setInteger(it->daily_repeat_interval);

        tango::INodeValuePtr weeklydays_node = job_node->createChild(L"weekly_days_of_week");
        weeklydays_node->setInteger(it->weekly_days);

        tango::INodeValuePtr monthlyday_node = job_node->createChild(L"monthly_day_number");
        monthlyday_node->setInteger(it->monthly_daynumber);

        tango::INodeValuePtr commands_node = job_node->createChild(L"commands");

        int cc = 0;
        std::vector<wxString>::iterator cit;
        for (cit = it->commands.begin(); cit != it->commands.end(); ++cit)
        {
            wchar_t buf[255];
            swprintf(buf, 255, L"command_%03d", cc++);

            tango::INodeValuePtr command_node = commands_node->createChild(buf);
            command_node->setString(towstr(*cit));
        }
    }

    // we may have just added a job that is about to run
    checkForJobs();
    
    return true;
}

bool JobScheduler::load()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_jobs.clear();

    tango::IDatabasePtr db = g_app->getDatabase();
    if (!db)
        return false;


    std::vector<JobSchedulerEntry> jobs;

    // -- load jobs --
    wxString path;
    path = wxString::Format(wxT("/.appdata/%s/dcfe/jobscheduler"),
                            towx(g_app->getDatabase()->getActiveUid()).c_str());

    tango::INodeValuePtr jobscheduler_file = db->openNodeFile(towstr(path));
    if (!jobscheduler_file)
        return false;

    tango::INodeValuePtr version_node = jobscheduler_file->getChild(L"version", false);
    if (!version_node)
        return false;

    // old job format no longer supported
    int version = version_node->getInteger();
    if (version < 2)
        return false;

    tango::INodeValuePtr jobs_node = jobscheduler_file->getChild(L"jobs", false);
    int counter = 0;

    int i, child_count = jobs_node->getChildCount();
        
    for (i = 0; i < child_count; ++i)
    {
        JobSchedulerEntry job;

        tango::INodeValuePtr job_node = jobs_node->getChildByIdx(i);

        tango::INodeValuePtr name_node = job_node->getChild(L"name", false);
        if (!name_node)
            return false;
        job.name = towx(name_node->getString());

        tango::INodeValuePtr schedule_node = job_node->getChild(L"schedule", false);
        if (!schedule_node)
            return false;
        job.schedule = (JobSchedulerEntry::Schedule)schedule_node->getInteger();

        tango::INodeValuePtr active_node = job_node->getChild(L"active", false);
        if (!active_node)
            return false;
        job.active = active_node->getBoolean();

        tango::INodeValuePtr starttime_node = job_node->getChild(L"start_time", false);
        if (!starttime_node)
            return false;
        job.start_time = starttime_node->getInteger();

        tango::INodeValuePtr finishtime_node = job_node->getChild(L"finish_time", false);
        if (!finishtime_node)
            return false;
        job.finish_time = finishtime_node->getInteger();
        
        tango::INodeValuePtr finishactive_node = job_node->getChild(L"finish_active", false);
        if (!finishactive_node)
            return false;
        job.finish_active = finishactive_node->getBoolean();

        tango::INodeValuePtr dailyrepeat_node = job_node->getChild(L"daily_repeat_active", false);
        if (!dailyrepeat_node)
            return false;
        job.daily_repeat_active = dailyrepeat_node->getBoolean();

        tango::INodeValuePtr dailyinterval_node = job_node->getChild(L"daily_repeat_interval", false);
        if (!dailyinterval_node)
            return false;
        job.daily_repeat_interval = dailyinterval_node->getInteger();

        tango::INodeValuePtr weeklydays_node = job_node->getChild(L"weekly_days_of_week", false);
        if (!weeklydays_node)
            return false;
        job.weekly_days = weeklydays_node->getInteger();

        tango::INodeValuePtr monthlyday_node = job_node->getChild(L"monthly_day_number", false);
        if (!monthlyday_node)
            return false;
        job.monthly_daynumber = monthlyday_node->getInteger();

        tango::INodeValuePtr commands_node = job_node->getChild(L"commands", false);
        if (!commands_node)
            return false;

        int command_count = commands_node->getChildCount();
        int ci;
        for (ci = 0; ci < command_count; ++ci)
        {
            tango::INodeValuePtr command_node = commands_node->getChildByIdx(ci);
            job.commands.push_back(towx(command_node->getString()));
        }

        calcNextRun(job);
        jobs.push_back(job);
    }

    m_jobs = jobs;
    return true;
}

void JobScheduler::updateJobVersion(JobSchedulerEntry& e, int old_interval)
{
    e.active = true;
    e.finish_time = e.start_time;
    e.finish_active = false;
    e.weekly_days = JobSchedulerEntry::Mon;
    e.monthly_daynumber = 1;
    
    if (old_interval == 0)
    {
        // no interval means it is a one-shot job
        e.schedule = JobSchedulerEntry::Once;
    } 
     else if (old_interval > 0 && old_interval <= 3600)
    {
        // these intervals span between a one-minute to a one-hour repeat
        e.schedule = JobSchedulerEntry::Daily;
        e.daily_repeat_active = true;
        e.daily_repeat_interval = (old_interval/60);
    }
     else if (old_interval == SECONDS_IN_DAY)
    {
        // this interval represents one day (so, repeat every day)
        e.schedule = JobSchedulerEntry::Weekly;
        e.weekly_days = JobSchedulerEntry::AllDays;
    }
     else if (old_interval == 604800)
    {
        // this interval represents one week (so, repeat once a week)
        e.schedule = JobSchedulerEntry::Weekly;
        getWeeklyRepeatDay(e);
    }
     else if (old_interval == 2419200)
    {
        // this interval represents 28 days (so repeat once a month)
        e.schedule = JobSchedulerEntry::Monthly;
        getMonthlyRepeatDayNumber(e);
    }
}




