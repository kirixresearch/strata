/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2002-01-30
 *
 */


#ifndef __APP_JOBQUEUE_H
#define __APP_JOBQUEUE_H


#include "../jobs/jobqueue.h"


class JobQueue : public jobs::JobQueue,
                 public wxEvtHandler
{
protected:

    void onJobFinished(wxCommandEvent& event);

    // function called from within thread; implementation
    // overridden to post event in main thread
    void onJobFinished(int job_id);
};


jobs::IJobQueuePtr createJobQueueObject();


#endif

