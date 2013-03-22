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
#include "jsonconfig.h"
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

    // store the job information in a json node;
    // TODO: decide if we want to use these particular variables/format or
    // something else
    kl::JsonNode node;
    kl::JsonNode metadata_node = node["metadata"];
    metadata_node["type"] = L"application/vnd.kx.jobscheduler";
    metadata_node["version"] = 1;
    metadata_node["description"] = L"";

    kl::JsonNode jobs_node = node["jobs"];

    std::vector<JobSchedulerEntry>::iterator it_jobs, it_jobs_end;
    it_jobs_end = m_jobs.end();

    for (it_jobs = m_jobs.begin(); it_jobs != it_jobs_end; ++it_jobs)
    {
        kl::JsonNode jobs_child_node = jobs_node.appendElement();

        jobs_child_node["name"].setString(towstr(it_jobs->name));
        jobs_child_node["schedule"].setInteger(it_jobs->schedule);
        jobs_child_node["active"].setBoolean(it_jobs->active);
        jobs_child_node["start_time"].setInteger(it_jobs->start_time);
        jobs_child_node["finish_time"].setInteger(it_jobs->finish_time);
        jobs_child_node["finish_active"].setBoolean(it_jobs->finish_active);
        jobs_child_node["daily_repeat_active"].setBoolean(it_jobs->daily_repeat_active);
        jobs_child_node["daily_repeat_interval"].setInteger(it_jobs->daily_repeat_interval);
        jobs_child_node["weekly_days_of_week"].setInteger(it_jobs->weekly_days);
        jobs_child_node["monthly_day_number"].setInteger(it_jobs->monthly_daynumber);
        jobs_child_node["commands"].setArray();

        kl::JsonNode commands_node = jobs_child_node["commands"];

        std::vector<wxString>::iterator it_commands, it_commands_end;
        it_commands_end = it_jobs->commands.end();

        for (it_commands = it_jobs->commands.begin(); it_commands != it_commands_end; ++it_commands)
        {
            kl::JsonNode commands_child_node = commands_node.appendElement();
            commands_child_node = towstr(*it_commands);
        }
    }

    // save the job
    wxString path = wxString::Format(wxT("/.appdata/%s/panels/jobscheduler"),
                              towx(db->getActiveUid()).c_str());

    if (!JsonConfig::saveToDb(node, g_app->getDatabase(), towstr(path), L"application/vnd.kx.jobscheduler"))
        return false;

    // we may have just added a job that is about to run
    checkForJobs();

    return true;
}

bool JobScheduler::load()
{
    XCM_AUTO_LOCK(m_obj_mutex);


    tango::IDatabasePtr db = g_app->getDatabase();
    if (!db)
        return false;

    // if the old jobs location exists, delete it
    wxString old_location = wxString::Format(wxT("/.appdata/%s/dcfe/jobscheduler"),
                                      towx(db->getActiveUid()).c_str());

    if (db->getFileExist(towstr(old_location)))
        db->deleteFile(towstr(old_location));

    wxString path = wxString::Format(wxT("/.appdata/%s/panels/jobscheduler"),
                              towx(db->getActiveUid()).c_str());

    // open the new location
    kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), towstr(path));
    if (!node.isOk())
        return false;

    // if we don't have the correct version, we're done
    if (!isValidFileVersion(node, L"application/vnd.kx.jobscheduler", 1))
        return false;

    // load the job info
    m_jobs.clear();
    std::vector<JobSchedulerEntry> jobs;

    kl::JsonNode jobs_node = node["jobs"];
    if (!jobs_node.isOk())
        return false;

    std::vector<kl::JsonNode> jobs_node_children = jobs_node.getChildren();
    std::vector<kl::JsonNode>::iterator it_jobs, it_jobs_end;
    it_jobs_end = jobs_node_children.end();
    
    for (it_jobs = jobs_node_children.begin(); it_jobs != it_jobs_end; ++it_jobs)
    {
        kl::JsonNode jobs_child_node = *it_jobs;
        if (!jobs_child_node.isOk())
            continue;

        JobSchedulerEntry job;

        job.name = jobs_child_node["name"].getString();
        job.schedule = (JobSchedulerEntry::Schedule)jobs_child_node["schedule"].getInteger();
        job.active = jobs_child_node["active"].getInteger() != 0 ? true : false;
        job.start_time = jobs_child_node["start_time"].getInteger();
        job.finish_time = jobs_child_node["finish_time"].getInteger();
        job.finish_active = jobs_child_node["finish_active"].getInteger() != 0 ? true : false;
        job.daily_repeat_active = jobs_child_node["daily_repeat_active"].getInteger() != 0 ? true : false;
        job.daily_repeat_interval = jobs_child_node["daily_repeat_interval"].getInteger();
        job.weekly_days = jobs_child_node["weekly_days_of_week"].getInteger();
        job.monthly_daynumber = jobs_child_node["monthly_day_number"].getInteger();

        std::vector<kl::JsonNode> commands_node_children = jobs_child_node["commands"].getChildren();
        std::vector<kl::JsonNode>::iterator it_commands, it_commands_end;
        it_commands_end = commands_node_children.end();

        for (it_commands = commands_node_children.begin(); it_commands != it_commands_end; ++it_commands)
        {
            kl::JsonNode command_child_node = *it_commands;
            job.commands.push_back(towx(command_child_node.getString()));
        }

        calcNextRun(job);
        jobs.push_back(job);
    } 

    m_jobs = jobs;
    return true;
}

