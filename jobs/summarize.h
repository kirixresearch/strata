/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-04
 *
 */


#ifndef __JOBS_SUMMARIZE_H
#define __JOBS_SUMMARIZE_H


namespace jobs
{


class SummarizeJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.SummaizeJob")
    XCM_BEGIN_INTERFACE_MAP(SummarizeJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    SummarizeJob();
    virtual ~SummarizeJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

