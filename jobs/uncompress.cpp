/*!
 *
 * Copyright (c) 2015, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2015-03-25
 *
 */


#include "jobspch.h"
#include "uncompress.h"


namespace jobs
{


// UncompressJob implementation

UncompressJob::UncompressJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.uncompress-job";
    m_config["metadata"]["version"] = 1;
}

UncompressJob::~UncompressJob()
{
}

bool UncompressJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.uncompress-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "output" : <path>,
            "format" : "zip"
            "stream" : ""
        }
    }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

    return true;
}

int UncompressJob::runJob()
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


    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    // get the index node children
    std::wstring input = params_node["input"];
    std::wstring output = params_node["output"];
    std::wstring stream = params_node["stream"];


    return 0;
}

void UncompressJob::runPostJob()
{
}


};  // namespace jobs

