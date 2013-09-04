/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2002-01-30
 *
 */


#include "appmain.h"
#include "jobqueue.h"


const int ID_JobFinishedNotify = 59984;


BEGIN_EVENT_TABLE(JobQueue, wxEvtHandler)
    EVT_MENU(ID_JobFinishedNotify, JobQueue::onJobFinished)
END_EVENT_TABLE()


void JobQueue::onJobFinished(wxCommandEvent& event)
{
    int job_id = event.GetInt();

    jobs::IJobPtr job;
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

void JobQueue::onJobFinished(int job_id)
{
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_JobFinishedNotify);
    event.SetInt(job_id);
    ::wxPostEvent(this, event);
}

jobs::IJobQueuePtr createJobQueueObject()
{
    return static_cast<jobs::IJobQueue*>(new JobQueue);
}

