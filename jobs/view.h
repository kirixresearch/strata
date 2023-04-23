/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-03-16
 *
 */


#ifndef H_JOBS_VIEW_H
#define H_JOBS_VIEW_H


namespace jobs
{


class ViewJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.ViewJob")
    XCM_BEGIN_INTERFACE_MAP(ViewJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ViewJob();
    virtual ~ViewJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

