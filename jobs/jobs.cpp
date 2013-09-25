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
#include "jobqueue.h"
#include "aggregate.h"
#include "alter.h"
#include "append.h"
#include "load.h"
#include "copy.h"
#include "delete.h"
#include "divide.h"
#include "execute.h"
#include "index.h"
#include "group.h"
#include "query.h"
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
    if (job_class == L"application/vnd.kx.copy-job")
        return static_cast<IJob*>(new CopyJob);
    if (job_class == L"application/vnd.kx.delete-job")
        return static_cast<IJob*>(new DeleteJob);
    if (job_class == L"application/vnd.kx.divide-job")
        return static_cast<IJob*>(new DivideJob);
    if (job_class == L"application/vnd.kx.execute-job")
        return static_cast<IJob*>(new ExecuteJob);
    if (job_class == L"application/vnd.kx.index-job")
        return static_cast<IJob*>(new IndexJob);
    if (job_class == L"application/vnd.kx.group-job")
        return static_cast<IJob*>(new GroupJob);
    if (job_class == L"application/vnd.kx.query-job")
        return static_cast<IJob*>(new QueryJob);
    if (job_class == L"application/vnd.kx.relationship-job")
        return static_cast<IJob*>(new RelationshipJob);
    if (job_class == L"application/vnd.kx.summarize-job")
        return static_cast<IJob*>(new SummarizeJob);
    if (job_class == L"application/vnd.kx.load-job")
        return static_cast<IJob*>(new LoadJob);
    if (job_class == L"application/vnd.kx.update-job")
        return static_cast<IJob*>(new UpdateJob);

    return xcm::null;
}


IJobPtr createAggregateJob(std::vector<IJobPtr>& jobs)
{
    IJobPtr job = createJob(L"application/vnd.kx.aggregate-job");

    kl::JsonNode agg_params;
    agg_params.fromString(job->getParameters());

    kl::JsonNode jobs_node = agg_params["jobs"];
    jobs_node.setArray();

    std::vector<IJobPtr>::iterator it;
    for (it = jobs.begin(); it != jobs.end(); ++it)
    {
        kl::JsonNode job_entry = jobs_node.appendElement();
        job_entry.fromString((*it)->toJson());


        std::wstring progress_string_format_nomax;
        std::wstring progress_string_format_max;

        (*it)->getJobInfo()->getProgressStringFormat(&progress_string_format_nomax, &progress_string_format_max);

        job_entry["__progress_string_format_nomax"] = progress_string_format_nomax;
        job_entry["__progress_string_format_max"] = progress_string_format_max;
    }

    job->setParameters(agg_params.toString());
    return job;
}


IJobInfoPtr createJobInfoObject()
{
    return static_cast<IJobInfo*>(new JobInfo);
}

IJobQueuePtr createJobQueueObject()
{
    return static_cast<jobs::IJobQueue*>(new JobQueue);
}


};

