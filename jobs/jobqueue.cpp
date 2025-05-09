/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2002-01-30
 *
 */


#include "jobspch.h"
#include <kl/xcm.h>
#include <kl/thread.h>
#include "jobqueue.h"


namespace jobs
{


class JobThread : public kl::thread
{

public:

    JobQueue* m_job_queue;
    IJobPtr m_job;
    IJobInfoPtr m_job_info;
    int m_job_id;
    bool* m_started_flag;

    JobThread(JobQueue* queue, IJobPtr job, bool* started_flag) : kl::thread()
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
        m_job_queue->onJobFinished(m_job_id);
    }
};


JobQueue::JobQueue()
{
    m_job_id_counter = 0;
    m_active_jobs = 0;
}

JobQueue::~JobQueue()
{
}

int JobQueue::addJob(IJobPtr job, int initial_state)
{
    int new_job_id;
    IJobInfoPtr job_info;

    {
        KL_AUTO_LOCK(m_obj_mutex);
        
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

int JobQueue::addJobInfo(IJobInfoPtr job_info, int initial_state)
{
    int new_job_id;
    
    {
        KL_AUTO_LOCK(m_obj_mutex);
        
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

IJobPtr JobQueue::lookupJob(int job_id)
{
    KL_AUTO_LOCK(m_obj_mutex);

    for (std::vector<IJobPtr>::iterator it = m_jobs.begin();
            it != m_jobs.end(); ++it)
    {
        if (it->p->getJobId() == job_id)
            return *it;
    }

    return xcm::null;
}

bool JobQueue::startJob(int job_id)
{
    IJobPtr job = lookupJob(job_id);
    if (!job)
        return false;

    return startJob(job);
}

IJobInfoPtr JobQueue::getJobInfo(int job_id)
{
    KL_AUTO_LOCK(m_obj_mutex);

    std::vector<IJobInfoPtr>::iterator it;
    for (it = m_job_info.begin(); it != m_job_info.end(); ++it)
    {
        if (it->p->getJobId() == job_id)
            return *it;
    }

    return xcm::null;
}

IJobInfoEnumPtr JobQueue::getJobInfoEnum(int job_state_mask)
{
    KL_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<IJobInfoPtr>* vec = new xcm::IVectorImpl<IJobInfoPtr>;

    std::vector<IJobInfoPtr>::iterator it;
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

void JobQueue::onJobFinished(int job_id)
{
    IJobPtr job;
    job = removeJob(job_id);    

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

IJobPtr JobQueue::removeJob(int job_id)
{
    std::vector<IJobPtr>::iterator it;
    IJobPtr job;

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

    return job;
}

bool JobQueue::startJob(IJobPtr& job)
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
        kl::thread::sleep(10);
        
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

/*

// TODO: get rid of wxCommandEvent dependency
//wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_JobFinishedNotify);
//event.SetInt(m_job_id);
//::wxPostEvent(m_job_queue, event);

void JobQueue::onJobFinished(wxCommandEvent& event)
{
    std::vector<IJobPtr>::iterator it;
    int job_id = event.GetInt();
    IJobPtr job;
    
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
*/

void JobQueue::onJobInfoEntryStateChanged(IJobInfoPtr job_info)
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


};

