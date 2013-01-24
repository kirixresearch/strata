/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-01-24
 *
 */


#ifndef __JOBS_GROUP_H
#define __JOBS_GROUP_H


namespace jobs
{


class GroupJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.GroupJob")
    XCM_BEGIN_INTERFACE_MAP(GroupJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    GroupJob();
    virtual ~GroupJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

