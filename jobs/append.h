/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-26
 *
 */


#ifndef H_JOBS_APPEND_H
#define H_JOBS_APPEND_H


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

    bool isInputValid();
    int runJob();
    void runPostJob();
};


} // namespace jobs


#endif

