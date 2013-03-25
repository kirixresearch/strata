/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-03-25
 *
 */


#include "jobspch.h"
#include "query.h"


namespace jobs
{


// QueryJob implementation

QueryJob::QueryJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.query-job";
    m_config["metadata"]["version"] = 1;
}

QueryJob::~QueryJob()
{
}

bool QueryJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.query-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "output" : <path>
        }
    }
*/
    if (m_config.isUndefined())
        return false;

    // TODO: add validation

    return true;
}

int QueryJob::runJob()
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


    kl::JsonNode params = m_config["params"];


    return 0;
}

void QueryJob::runPostJob()
{
}


};  // namespace jobs

