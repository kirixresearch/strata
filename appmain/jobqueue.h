/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#ifndef __APP_JOBQUEUE_H
#define __APP_JOBQUEUE_H


xcm_interface IJobQueue;


XCM_DECLARE_SMARTPTR(IJobQueue)


enum
{
    jobStateNone = 0x0000,
    jobStatePaused = 0x0001,
    jobStateQueued = 0x0002,
    jobStateRunning = 0x0004,
    jobStateFinished = 0x0008,
    jobStateCancelling = 0x0010,
    jobStateCancelled = 0x0020,
    jobStateFailed = 0x0040,
    jobStateAll = 0xffff
};

enum
{
    jobMaskTitle = 0x01,
    jobMaskStartTime = 0x02,
    jobMaskFinishTime = 0x04,
    jobMaskCurrentCount = 0x08,
    jobMaskMaxCount = 0x10,
    jobMaskPercentage = 0x20,
    jobMaskProgressString = 0x40,
    jobMaskProgressBar = 0x80
};

xcm_interface IJobQueue : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IJobQueue")

public:

    virtual int addJob(jobs::IJobPtr job, int initial_state) = 0;
    virtual int addJobInfo(jobs::IJobInfoPtr job, int initial_state) = 0;
    virtual jobs::IJobPtr lookupJob(int job_id) = 0;
    virtual bool startJob(int job_id) = 0;

    virtual jobs::IJobInfoPtr getJobInfo(int job_id) = 0;
    virtual jobs::IJobInfoEnumPtr getJobInfoEnum(int job_state_mask) = 0;
    virtual bool getJobsActive() = 0;

    XCM_DECLARE_SIGNAL0(sigQueueChanged);
    XCM_DECLARE_SIGNAL1(sigJobAdded, jobs::IJobInfoPtr);
};


IJobQueuePtr createJobQueueObject();

#endif

