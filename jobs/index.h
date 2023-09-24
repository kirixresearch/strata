/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-01-17
 *
 */


#ifndef H_JOBS_INDEX_H
#define H_JOBS_INDEX_H


namespace jobs
{


class IndexJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.IndexJob")
    XCM_BEGIN_INTERFACE_MAP(IndexJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    IndexJob();
    virtual ~IndexJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


} // namespace jobs


#endif

