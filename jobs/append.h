/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-26
 *
 */


#ifndef __JOBS_APPEND_H
#define __JOBS_APPEND_H

namespace jobs
{


xcm_interface IAppendJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IAppendJob")

public:

    virtual tango::ISetPtr getTargetSet() = 0;
};

XCM_DECLARE_SMARTPTR(IAppendJob)


class AppendJob : public XdJobBase,
                  public IAppendJob
{

    XCM_CLASS_NAME("appmain.AppendJob")
    XCM_BEGIN_INTERFACE_MAP(AppendJob)
        XCM_INTERFACE_ENTRY(IAppendJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    AppendJob();
    virtual ~AppendJob();

    void setTargetSet(tango::ISetPtr target_set);
    void addAppendSet(tango::ISetPtr append_set);

    tango::ISetPtr getTargetSet();

    int runJob();
    void runPostJob();

private:

    tango::ISetPtr m_target_set;
    std::vector<tango::ISetPtr> m_append_sets;
    double m_max_count;
};



}; // namespace jobs

#endif

