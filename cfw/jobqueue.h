/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#ifndef __CFW_JOBQUEUE_H
#define __CFW_JOBQUEUE_H


namespace cfw
{


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

    virtual void runPreJob() = 0;
    virtual int runJob() = 0;
    virtual void runPostJob() = 0;
    
    // runNowAndBlock() calls runPreJob(), runJob(),
    // and runPostJob() immediately
    virtual void runNowAndBlock() = 0;

    XCM_DECLARE_SIGNAL1(sigJobFinished, IJobPtr)
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


xcm_interface IJobQueue : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IJobQueue")

public:

    virtual int addJob(IJobPtr job, int initial_state) = 0;
    virtual int addJobInfo(IJobInfoPtr job, int initial_state) = 0;
    virtual IJobPtr lookupJob(int job_id) = 0;
    virtual bool startJob(int job_id) = 0;

    virtual IJobInfoPtr getJobInfo(int job_id) = 0;
    virtual IJobInfoEnumPtr getJobInfoEnum(int job_state_mask) = 0;
    virtual bool getJobsActive() = 0;

    XCM_DECLARE_SIGNAL0(sigQueueChanged);
    XCM_DECLARE_SIGNAL1(sigJobAdded, cfw::IJobInfoPtr);
};



class JobBase : public cfw::IJob
{

XCM_CLASS_NAME("cfw.JobBase")
XCM_BEGIN_INTERFACE_MAP(JobBase)
    XCM_INTERFACE_ENTRY(cfw::IJob)
XCM_END_INTERFACE_MAP()

XCM_IMPLEMENT_SIGNAL1(sigJobFinished, IJobPtr)

public:

    JobBase()
    {
        m_jobbase_jobid = 0;
        m_extra_long = 0;
        m_sp_job_info.create_instance("cfw.JobInfo");
        m_job_info = m_sp_job_info.p;
        m_cancelling = false;

        std::wstring str1 = _("$c records processed");
        std::wstring str2 = _("$c of $m records processed ($p1%)");
        getJobInfo()->setProgressStringFormat(str1, str2);
    }

    int getJobId()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return m_jobbase_jobid;
    }

    void setJobId(int job_id)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_jobbase_jobid = job_id;
    }

    IJobInfoPtr getJobInfo()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return m_job_info;
    }

    void setJobInfo(IJobInfoPtr new_val)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_job_info = new_val;
    }

    long getExtraLong()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return m_extra_long;
    }

    void setExtraLong(long value)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_extra_long = value;
    }

    void setExtraString(const std::wstring& s)
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        m_extra_string = s;
    }

    std::wstring getExtraString()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);
        return m_extra_string;
    }

    bool cancel()
    {
        XCM_AUTO_LOCK(m_jobbase_mutex);

        if (!m_job_info->getCancelAllowed())
            return false;

        m_job_info->setState(jobStateCancelling);
        m_cancelling = true;

        return true;
    }

    bool isCancelling()
    {
        return m_cancelling ? true : false;
    }

    void runPreJob()
    {
    }

    void runPostJob()
    {
    }
    
    void runNowAndBlock()
    {
        runPreJob();
        runJob();
        runPostJob();
    }


protected:
    cfw::IJobInfoPtr m_sp_job_info;
    cfw::IJobInfo* m_job_info;
    std::wstring m_extra_string;
    long m_extra_long;

private:
    int m_jobbase_jobid;
    xcm::mutex m_jobbase_mutex;
    bool m_cancelling;
};




};  // namespace cfw


#endif

