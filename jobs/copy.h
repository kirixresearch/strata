/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-03-16
 *
 */


#ifndef H_JOBS_COPY_H
#define H_JOBS_COPY_H


namespace jobs
{


class CopyJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.CopyJob")
    XCM_BEGIN_INTERFACE_MAP(CopyJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    CopyJob();
    virtual ~CopyJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

