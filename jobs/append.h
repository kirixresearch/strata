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


class AppendJob : public XdJobBase
{

    XCM_CLASS_NAME("jobs.AppendJob")
    XCM_BEGIN_INTERFACE_MAP(AppendJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    AppendJob();
    virtual ~AppendJob();

    int runJob();
    void runPostJob();

private:

    double m_max_count;
};



}; // namespace jobs

#endif

