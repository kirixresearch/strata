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
    XCM_IMPLEMENT_SIGNAL1(sigJobAdded, jobs::IJobInfoPtr);

public:
    
    JobQueue();
    ~JobQueue();

    int addJob(jobs::IJobPtr job, int initial_state);
    int addJobInfo(jobs::IJobInfoPtr job, int initial_state);

    jobs::IJobPtr lookupJob(int job_id);
    bool startJob(int job_id);

    jobs::IJobInfoPtr getJobInfo(int job_id);
    jobs::IJobInfoEnumPtr getJobInfoEnum(int job_state_mask);
    bool getJobsActive();

private:

    bool startJob(jobs::IJobPtr& job);
    void incrementActiveJobs();
    void decrementActiveJobs();

    // TODO: get rid of wxCommandEvent dependency
    //void onJobFinished(wxCommandEvent& evt);
    void onJobInfoEntryStateChanged(jobs::IJobInfoPtr job_info);

private:

    xcm::mutex m_obj_mutex;
    std::vector<jobs::IJobPtr> m_jobs;
    std::vector<jobs::IJobInfoPtr> m_job_info;
    int m_job_id_counter;
    
    xcm::mutex m_active_jobs_mutex;
    int m_active_jobs;

};


}; // namespace jobs


#endif

