/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-18
 *
 */


#ifndef __JOBS_DELETE_H
#define __JOBS_DELETE_H


namespace jobs
{


class DeleteJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.DeleteJob")
    XCM_BEGIN_INTERFACE_MAP(DeleteJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    DeleteJob();
    virtual ~DeleteJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

