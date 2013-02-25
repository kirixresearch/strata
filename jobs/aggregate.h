/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-25
 *
 */


#ifndef __JOBS_AGGREGATE_H
#define __JOBS_AGGREGATE_H


namespace jobs
{


class AggregateJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.AggregateJob")
    XCM_BEGIN_INTERFACE_MAP(AggregateJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    AggregateJob();
    virtual ~AggregateJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

