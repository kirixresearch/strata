/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-12
 *
 */


#ifndef H_JOBS_ALTER_H
#define H_JOBS_ALTER_H


namespace jobs
{


class AlterJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.AlterJob")
    XCM_BEGIN_INTERFACE_MAP(AlterJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    AlterJob();
    virtual ~AlterJob();

    bool isInputValid();
    int runJob();
    void runPostJob();

private:

    kl::JsonNode getJobSchema();
};


}; // namespace jobs


#endif

