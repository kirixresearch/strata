/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#ifndef __APP_JOBQUEUE_PRIVATE_H
#define __APP_JOBQUEUE_PRIVATE_H





class JobQueue : public wxEvtHandler,
                 public IJobQueue,
                 public xcm::signal_sink
{
    friend class JobThread;
    
    XCM_CLASS_NAME("cfw.JobQueue")
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

private:

    bool startJob(IJobPtr& job);
    void incrementActiveJobs();
    void decrementActiveJobs();

    void onJobFinished(wxCommandEvent& evt);
    void onJobInfoEntryStateChanged(IJobInfoPtr job_info);
    
private:

    xcm::mutex m_obj_mutex;
    std::vector<IJobPtr> m_jobs;
    std::vector<IJobInfoPtr> m_job_info;
    int m_job_id_counter;
    
    xcm::mutex m_active_jobs_mutex;
    int m_active_jobs;

    DECLARE_EVENT_TABLE()
};




#endif

