/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-03-26
 *
 */


#ifndef H_JOBS_EXECUTE_H
#define H_JOBS_EXECUTE_H


namespace jobs
{


class ExecuteJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.ExecuteJob")
    XCM_BEGIN_INTERFACE_MAP(ExecuteJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ExecuteJob();
    virtual ~ExecuteJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

