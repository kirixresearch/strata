/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-25
 *
 */


#include "jobspch.h"
#include "aggregate.h"


namespace jobs
{


// AggregateJob implementation

AggregateJob::AggregateJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.aggregate-job";
    m_config["metadata"]["version"] = 1;
}

AggregateJob::~AggregateJob()
{
}

bool AggregateJob::isInputValid()
{
/*
    // example format:
    {
        "metadata":
        {
            "type" : "application/vnd.kx.aggregate-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            jobs : [
            ]
        }
    }
*/
    if (m_config.isUndefined())
        return false;

    // TODO: check job type and version

    kl::JsonNode params = m_config["params"];
    if (params.isUndefined())
        return false;

    if (!params.childExists("jobs"))
        return false;

    kl::JsonNode actions_node = params.getChild("jobs");
    if (!actions_node.isArray())
        return false;

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

    return true;
}

int AggregateJob::runJob()
{
    // make sure we have a valid input
    if (!isInputValid())
    {
        // TODO: correlate input validation with job error information
    
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }

    // get the jobs
    kl::JsonNode params = m_config["params"];

    kl::JsonNode jobs_node = params["jobs"];

    // create job objects from each parameter json node
    std::vector<jobs::IJobPtr> jobs;

    size_t i, cnt = jobs_node.getChildCount();
    for (i = 0; i < cnt; ++i)
    {
        kl::JsonNode job_node = jobs_node[i];

        kl::JsonNode metadata = job_node["metadata"];
        if (!metadata.isOk())
            continue;

        kl::JsonNode params = job_node["params"];
        if (!params.isOk())
            continue;

        jobs::IJobPtr job = createJob(metadata["type"]);
        if (job.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        job->setDatabase(m_db);
        job->setParameters(params.toString());

        jobs.push_back(job);
    }


    std::vector<jobs::IJobPtr>::iterator jit;
    for (jit = jobs.begin(); jit != jobs.end(); ++jit)
    {
        (*jit)->runJob();
        (*jit)->runPostJob();
    }


    return 0;
}

void AggregateJob::runPostJob()
{
}


};  // namespace jobs

