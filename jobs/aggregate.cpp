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
    if (m_config.isNull())
        return false;

    // TODO: check job type and version

    kl::JsonNode params = m_config["params"];
    if (params.isNull())
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

    // make sure the database is valid
    if (m_db.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }    

    // get the jobs
    kl::JsonNode params = m_config["params"];
    std::vector<kl::JsonNode> action_nodes = params.getChildren();




    tango::IJobPtr tango_job = m_db->createJob();
    setTangoJob(tango_job);



    if (tango_job->getCancelled())
    {
        m_job_info->setState(jobStateCancelling);
        return 0;
    }

    if (tango_job->getStatus() == tango::jobFailed)
    {
        m_job_info->setState(jobStateFailed);

        // TODO: error string
    }

    return 0;
}

void AggregateJob::runPostJob()
{
}


};  // namespace jobs

