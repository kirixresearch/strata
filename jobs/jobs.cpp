/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-27
 *
 */


#include "jobspch.h"
#include "jobinfo.h"
#include "aggregate.h"
#include "alter.h"
#include "append.h"
#include "delete.h"
#include "divide.h"
#include "index.h"
#include "group.h"
#include "relationship.h"
#include "summarize.h"
#include "update.h"


namespace jobs
{


IJobPtr createJob(const std::wstring job_class)
{
    if (job_class == L"application/vnd.kx.aggregate-job")
        return static_cast<IJob*>(new AggregateJob);
    if (job_class == L"application/vnd.kx.alter-job")
        return static_cast<IJob*>(new AlterJob);
    if (job_class == L"application/vnd.kx.append-job")
        return static_cast<IJob*>(new AppendJob);
    if (job_class == L"application/vnd.kx.delete-job")
        return static_cast<IJob*>(new DeleteJob);
    if (job_class == L"application/vnd.kx.divide-job")
        return static_cast<IJob*>(new DivideJob);
    if (job_class == L"application/vnd.kx.index-job")
        return static_cast<IJob*>(new IndexJob);
    if (job_class == L"application/vnd.kx.group-job")
        return static_cast<IJob*>(new GroupJob);
    if (job_class == L"application/vnd.kx.relationship-job")
        return static_cast<IJob*>(new RelationshipJob);
    if (job_class == L"application/vnd.kx.summarize-job")
        return static_cast<IJob*>(new SummarizeJob);
    if (job_class == L"application/vnd.kx.update-job")
        return static_cast<IJob*>(new UpdateJob);

    return xcm::null;        
}


IJobInfoPtr createJobInfoObject()
{
    return static_cast<IJobInfo*>(new JobInfo);
}


};

