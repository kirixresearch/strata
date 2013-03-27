/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-03-26
 *
 */


#include "jobspch.h"
#include "execute.h"


namespace jobs
{


// ExecuteJob implementation

ExecuteJob::ExecuteJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.execute-job";
    m_config["metadata"]["version"] = 1;
}

ExecuteJob::~ExecuteJob()
{
}

bool ExecuteJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.execute-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "command" : <path>
        }
    }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

    return true;
}

int ExecuteJob::runJob()
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

    kl::JsonNode command_node = params_node["command"];
    std::wstring sql = command_node.getString();


    tango::IJobPtr tango_job = m_db->createJob();
    setTangoJob(tango_job);


    unsigned int flags = tango::sqlPassThrough;
    if (getExtraValue(L"tango.sqlAlwaysCopy") == L"true")
        flags = tango::sqlAlwaysCopy;

    xcm::IObjectPtr result;
    m_db->execute(sql, flags, result, tango_job);
    setResultObject(result);

    if (tango_job->getCancelled())
    {
        m_job_info->setState(jobStateCancelling);
        return 0;
    }


    return 0;
}

void ExecuteJob::runPostJob()
{
}


};  // namespace jobs

