/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-01-15
 *
 */


#ifndef H_JOBS_DIVIDE_H
#define H_JOBS_DIVIDE_H


namespace jobs
{


class DivideJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.DivideJob")
    XCM_BEGIN_INTERFACE_MAP(DivideJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    DivideJob();
    virtual ~DivideJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


} // namespace jobs


#endif

