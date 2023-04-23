/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-03-25
 *
 */


#ifndef H_JOBS_QUERY_H
#define H_JOBS_QUERY_H


namespace jobs
{


class QueryJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.QueryJob")
    XCM_BEGIN_INTERFACE_MAP(QueryJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    QueryJob();
    virtual ~QueryJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

