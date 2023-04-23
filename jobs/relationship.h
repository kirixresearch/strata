/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-19
 *
 */


#ifndef H_JOBS_RELATIONSHIP_H
#define H_JOBS_RELATIONSHIP_H


namespace jobs
{


class RelationshipJob : public XdJobBase
{
    XCM_CLASS_NAME("jobs.RelationshipJob")
    XCM_BEGIN_INTERFACE_MAP(RelationshipJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    RelationshipJob();
    virtual ~RelationshipJob();

    bool isInputValid();
    int runJob();
    void runPostJob();
};


}; // namespace jobs


#endif

