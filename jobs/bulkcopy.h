/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-04-09
 *
 */


#ifndef __JOBS_TRANSFORM_H
#define __JOBS_TRANSFORM_H


namespace jobs
{


class TransformJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.TransformJob")
    XCM_BEGIN_INTERFACE_MAP(TransformJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    TransformJob();
    virtual ~TransformJob();

    bool isInputValid();
    int runJob();
    void runPostJob();

};


}; // namespace jobs


#endif

