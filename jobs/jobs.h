/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-27
 *
 */

#ifndef __JOBS_JOBS_H
#define __JOBS_JOBS_H


namespace jobs
{


enum
{
    jobserrNone = 0,
    jobserrFailed = 1,
    jobserrInsufficientDiskSpace = 2
};


xcm_interface IJob;
xcm_interface IJobStat;
xcm_interface IJobInfo;
xcm_interface IJobQueue;


XCM_DECLARE_SMARTPTR(IJob)
XCM_DECLARE_SMARTPTR(IJobStat)
XCM_DECLARE_SMARTPTR(IJobInfo)
XCM_DECLARE_SMARTPTR(IJobQueue)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IJobInfoPtr>, IJobInfoEnumPtr)




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


xcm_interface IJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IJob")

public:

    virtual int getJobId() = 0;
    virtual void setJobId(int job_id) = 0;

    virtual IJobInfoPtr getJobInfo() = 0;
    virtual void setJobInfo(IJobInfoPtr new_val) = 0;

    virtual long getExtraLong() = 0;
    virtual void setExtraLong(long value) = 0;
    
    virtual void setExtraString(const std::wstring& s) = 0;
    virtual std::wstring getExtraString() = 0;

    virtual bool cancel() = 0;

    virtual int runJob() = 0;
    virtual void runPostJob() = 0;
    
    XCM_DECLARE_SIGNAL1(sigJobFinished, IJobPtr)

    virtual void setParameter(const std::wstring& param, const std::wstring& value) = 0;
    virtual void setInstructions(const std::wstring& json) = 0;
};


xcm_interface IJobInfo : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IJobInfo")

public:

    virtual void setJobId(int job_id) = 0;
    virtual int getJobId() = 0;

    virtual void setInfoMask(int mask) = 0;
    virtual int getInfoMask() = 0;

    virtual void setCancelAllowed(bool new_val) = 0;
    virtual bool getCancelAllowed() = 0;

    virtual void setState(int new_val) = 0;
    virtual int getState() = 0;

    virtual void setVisible(bool new_val) = 0;
    virtual bool getVisible() = 0;
    
    virtual void setProgressStringFormat(const std::wstring& no_max_count_format,
                                         const std::wstring& max_count_format) = 0;
    virtual void setProgressString(const std::wstring& new_val) = 0;
    virtual std::wstring getProgressString() = 0;
    
    virtual void setError(int error_code, const std::wstring& error_string) = 0;
    virtual int getErrorCode() = 0;
    virtual std::wstring getErrorString() = 0;
    
    virtual void setTitle(const std::wstring& new_val) = 0;
    virtual std::wstring getTitle() = 0;

    virtual void setStartTime(time_t new_val) = 0;
    virtual time_t getStartTime() = 0;

    virtual void setFinishTime(time_t new_val) = 0;
    virtual time_t getFinishTime() = 0;

    virtual void incrementCurrentCount(double amount) = 0;
    virtual void setCurrentCount(double new_val) = 0;
    virtual double getCurrentCount() = 0;

    virtual void setMaxCount(double new_val) = 0;
    virtual double getMaxCount() = 0;

    virtual double getPercentage() = 0;

    XCM_DECLARE_SIGNAL1(sigOnGetCurrentCount, double*)
    XCM_DECLARE_SIGNAL1(sigOnGetMaxCount, double*)
    XCM_DECLARE_SIGNAL1(sigOnGetPercentage, double*)
    XCM_DECLARE_SIGNAL1(sigStateChanged, IJobInfoPtr)
};





IJobPtr createJob(const std::wstring job_class);
IJobInfoPtr createJobInfoObject();

};



#endif
