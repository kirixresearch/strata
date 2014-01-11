/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2002-01-30
 *
 */


#ifndef __JOBS_JOBQUEUE_H
#define __JOBS_JOBQUEUE_H


namespace jobs
{


class JobQueue : public IJobQueue,
                 public xcm::signal_sink
{

    friend class JobThread;
    
    XCM_CLASS_NAME("jobs.JobQueue")
    XCM_BEGIN_INTERFACE_MAP(JobQueue)
        XCM_INTERFACE_ENTRY(IJobQueue)
    XCM_END_INTERFACE_MAP()

    XCM_IMPLEMENT_SIGNAL0(sigQueueChanged);
    XCM_IMPLEMENT_SIGNAL1(sigJobAdded, IJobInfoPtr);

public:
    
    JobQueue();
    ~JobQueue();

    int addJob(IJobPtr job, int initial_state);
    int addJobInfo(IJobInfoPtr job, int initial_state);

    IJobPtr lookupJob(int job_id);
    bool startJob(int job_id);

    IJobInfoPtr getJobInfo(int job_id);
    IJobInfoEnumPtr getJobInfoEnum(int job_state_mask);
    bool getJobsActive();

protected:

    // called from within thread; derived implementations
    // need to be thread safe
    virtual void onJobFinished(int job_id);
    IJobPtr removeJob(int job_id);

private:

    bool startJob(IJobPtr& job);
    void incrementActiveJobs();
    void decrementActiveJobs();
    void onJobInfoEntryStateChanged(IJobInfoPtr job_info);

private:

    kl::mutex m_obj_mutex;
    std::vector<IJobPtr> m_jobs;
    std::vector<IJobInfoPtr> m_job_info;
    int m_job_id_counter;
    
    kl::mutex m_active_jobs_mutex;
    int m_active_jobs;

};


}; // namespace jobs


#endif

