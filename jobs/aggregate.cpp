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

AggregateJob::AggregateJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.aggregate-job";
    m_config["metadata"]["version"] = 1;

    m_agg_jobinfo = new AggregateJobInfo;
    m_agg_jobinfo->ref();

    setJobInfo(static_cast<IJobInfo*>(m_agg_jobinfo));
}

AggregateJob::~AggregateJob()
{
    m_agg_jobinfo->unref();
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

    // TODO: add validation

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
    kl::JsonNode params_node;
    params_node.fromString(getParameters());

    kl::JsonNode jobs_node = params_node["jobs"];

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

        
        // if progress string format information is passed (see jobs.cpp),
        // then set the job info parameters accordingly
        std::wstring progress_string_format_nomax;
        std::wstring progress_string_format_max;
        if (job_node.childExists("__progress_string_format_nomax"))
            progress_string_format_nomax = job_node["__progress_string_format_nomax"];
        if (job_node.childExists("__progress_string_format_max"))
            progress_string_format_max = job_node["__progress_string_format_max"];
        job->getJobInfo()->setProgressStringFormat(progress_string_format_nomax, progress_string_format_max);


        job->setDatabase(m_db);
        job->setParameters(params.toString());

        jobs.push_back(job);
    }


    std::vector<jobs::IJobPtr>::iterator jit;
    for (jit = jobs.begin(); jit != jobs.end(); ++jit)
    {
        setCurrentJob(*jit);
        m_agg_jobinfo->setCurrentJobInfo((*jit)->getJobInfo());

        (*jit)->runJob();
        (*jit)->runPostJob();
    }

    setCurrentJob(xcm::null);


    return 0;
}

void AggregateJob::runPostJob()
{
}


};  // namespace jobs

