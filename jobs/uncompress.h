/*!
 *
 * Copyright (c) 2015, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2015-03-25
 *
 */


#ifndef __JOBS_UNCOMPRESS_H
#define __JOBS_UNCOMPRESS_H


namespace jobs
{


class UncompressJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.UncompressJob")
    XCM_BEGIN_INTERFACE_MAP(UncompressJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    UncompressJob();
    virtual ~UncompressJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

