/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#include "appmain.h"
#include <xcm/xcm.h>
#include <kl/thread.h>
#include "jobqueue.h"
#include "jobqueue_private.h"



const int ID_JobFinishedNotify = 59984;



class JobThread : public kl::Thread
{
public:

    JobQueue* m_job_queue;
    jobs::IJobPtr m_job;
    jobs::IJobInfoPtr m_job_info;
    int m_job_id;
    bool* m_started_flag;


    JobThread(JobQueue* queue, jobs::IJobPtr job, bool* started_flag) : kl::Thread()
    {
        m_job = job;
        m_job_info = job->getJobInfo();
        m_job_queue = queue;
        m_job_queue->ref();
        m_job_id = 0;
        m_started_flag = started_flag;
    }

    ~JobThread()
    {
        m_job_queue->decrementActiveJobs();
        m_job_queue->unref();
    }

    unsigned int entry()
    {
        m_job_queue->incrementActiveJobs();
        
        #ifdef _MSC_VER
        static int counter = 0;
        counter += 100;
        // ensures that the rand() is truly random in this thread
        int seed = (int)time(NULL);
        seed += (int)clock();
        seed += counter;
        srand(seed);
        #endif
    

        m_job_info->setState(jobStateRunning);
        m_job_info->setStartTime(time(NULL));

        if (m_started_flag)
            *m_started_flag = true;
        
        int res = m_job->runJob();

        m_job_info->setFinishTime(time(NULL));

        if (m_job_info->getState() == jobStateRunning)
        {
            m_job_info->setState(jobStateFinished);
        }

        if (m_job_info->getState() == jobStateCancelling)
        {
            m_job_info->setState(jobStateCancelled);
        }

        return 0;
    }

    void exit()
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_JobFinishedNotify);
        event.SetInt(m_job_id);
        ::wxPostEvent(m_job_queue, event);
    }
};


IJobQueuePtr createJobQueueObject()
{
    return static_cast<IJobQueue*>(new JobQueue);
}


BEGIN_EVENT_TABLE(JobQueue, wxEvtHandler)
    EVT_MENU(ID_JobFinishedNotify, JobQueue::onJobFinished)
END_EVENT_TABLE()



JobQueue::JobQueue()
{
    m_job_id_counter = 0;
    m_active_jobs = 0;
}

JobQueue::~JobQueue()
{
}

int JobQueue::addJob(jobs::IJobPtr job, int initial_state)
{
    int new_job_id;
    jobs::IJobInfoPtr job_info;
    
    wxASSERT_MSG(job.p, wxT("Job is null!"));
    
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        
        job_info = job->getJobInfo();
        
        m_job_id_counter++;
        new_job_id = m_job_id_counter;

        job->setJobId(new_job_id);
        job_info->setJobId(new_job_id);
        
        switch (initial_state)
        {
            default:
            case jobStateRunning:
            case jobStatePaused:
                job_info->setState(jobStatePaused);
                break;
            
            case jobStateQueued:
                job_info->setState(jobStateQueued);
                break;
        } 
        
        m_job_info.push_back(job_info);
        m_jobs.push_back(job);
    }
    

    if (initial_state == jobStateRunning)
    {
        startJob(job);
    }

    sigJobAdded().fire(job_info);
    sigQueueChanged().fire();

    return new_job_id;
}

int JobQueue::addJobInfo(jobs::IJobInfoPtr job_info, int initial_state)
{
    int new_job_id;
    
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        
        m_job_id_counter++;
        new_job_id = m_job_id_counter;

        job_info->setJobId(new_job_id);
        job_info->sigStateChanged().connect(this, &JobQueue::onJobInfoEntryStateChanged);
        job_info->setState(initial_state);
        m_job_info.push_back(job_info);
    }
    
    sigJobAdded().fire(job_info);
    sigQueueChanged().fire();

    return new_job_id;
}

jobs::IJobPtr JobQueue::lookupJob(int job_id)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    for (std::vector<jobs::IJobPtr>::iterator it = m_jobs.begin();
            it != m_jobs.end(); ++it)
    {
        if (it->p->getJobId() == job_id)
            return *it;
    }

    return xcm::null;
}

bool JobQueue::startJob(int job_id)
{
    jobs::IJobPtr job = lookupJob(job_id);
    if (!job)
        return false;

    return startJob(job);
}

jobs::IJobInfoPtr JobQueue::getJobInfo(int job_id)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::vector<jobs::IJobInfoPtr>::iterator it;
    for (it = m_job_info.begin(); it != m_job_info.end(); ++it)
    {
        if (it->p->getJobId() == job_id)
            return *it;
    }

    return xcm::null;
}

jobs::IJobInfoEnumPtr JobQueue::getJobInfoEnum(int job_state_mask)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<jobs::IJobInfoPtr>* vec = new xcm::IVectorImpl<jobs::IJobInfoPtr>;

    std::vector<jobs::IJobInfoPtr>::iterator it;
    for (it = m_job_info.begin(); it != m_job_info.end(); ++it)
    {
        if (job_state_mask & it->p->getState())
            vec->append(*it);
    }

    return vec;
}

bool JobQueue::getJobsActive()
{    
    // note I left the mutex out of here on purpose;
    // avoiding the mutex lock can save some time here
    return (m_active_jobs > 0) ? true : false;
}

bool JobQueue::startJob(jobs::IJobPtr& job)
{
    bool started_flag = false;
    
    // start the job in a thread
    JobThread* job_thread = new JobThread(this, job, &started_flag);
    job_thread->m_job = job;
    job_thread->m_job_id = job->getJobId();


    job->getJobInfo()->setState(jobStateRunning);
    if (job_thread->create() != 0)
        return false;

    while (!started_flag)
        kl::Thread::sleep(10);
        
    return true;
}

void JobQueue::incrementActiveJobs()
{
    m_active_jobs_mutex.lock();
    m_active_jobs++;
    m_active_jobs_mutex.unlock();
}

void JobQueue::decrementActiveJobs()
{
    m_active_jobs_mutex.lock();
    m_active_jobs--;
    m_active_jobs_mutex.unlock();
}

void JobQueue::onJobFinished(wxCommandEvent& event)
{
    std::vector<jobs::IJobPtr>::iterator it;
    int job_id = event.GetInt();
    jobs::IJobPtr job;
    
    m_obj_mutex.lock();
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        if (it->p->getJobId() == job_id)
        {
            job = it->p;
            
            // remove from the active job list
            m_jobs.erase(it);

            break;
        }
    }
    m_obj_mutex.unlock();
    

    if (job)
    {
        // run post job
        job->runPostJob();

        // fire the job's finished signal
        job->sigJobFinished().fire(job);
        
        job.clear();
    }

    sigQueueChanged().fire();
}

void JobQueue::onJobInfoEntryStateChanged(jobs::IJobInfoPtr job_info)
{
    int state = job_info->getState();
    if (state == jobStateRunning)
    {
        incrementActiveJobs();
    }
     else if (state == jobStateFailed ||
              state == jobStateCancelled ||
              state == jobStateFinished)
    {
        decrementActiveJobs();
    }
}

