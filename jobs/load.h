/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-04-09
 *
 */


#ifndef __JOBS_BULKCOPY_H
#define __JOBS_BULKCOPY_H


namespace jobs
{


class BulkCopyJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.BulkCopyJob")
    XCM_BEGIN_INTERFACE_MAP(BulkCopyJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    BulkCopyJob();
    virtual ~BulkCopyJob();

    bool isInputValid();
    int runJob();
    void runPostJob();

};


}; // namespace jobs


#endif

