/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-18
 *
 */


#ifndef H_JOBS_UPDATE_H
#define H_JOBS_UPDATE_H


namespace jobs
{


class UpdateJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.UpdateJob")
    XCM_BEGIN_INTERFACE_MAP(UpdateJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    UpdateJob();
    virtual ~UpdateJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


} // namespace jobs


#endif

