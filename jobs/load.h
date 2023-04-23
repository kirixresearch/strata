/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-04-09
 *
 */


#ifndef H_JOBS_BULKCOPY_H
#define H_JOBS_BULKCOPY_H


namespace jobs
{


class LoadJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.LoadJob")
    XCM_BEGIN_INTERFACE_MAP(LoadJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    LoadJob();
    virtual ~LoadJob();

    bool isInputValid();
    int runJob();
    void runPostJob();

};


}; // namespace jobs


#endif

