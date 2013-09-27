/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-03-16
 *
 */


#include "jobspch.h"
#include "copy.h"
#include "util.h"
#include <kl/hex.h>


namespace jobs
{


// CopyJob implementation

CopyJob::CopyJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.copy-job";
    m_config["metadata"]["version"] = 1;
}

CopyJob::~CopyJob()
{
}

bool CopyJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.copy-job",
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

int CopyJob::runJob()
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


    xd::IJobPtr tango_job = m_db->createJob();
    setXdJob(tango_job);

    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    // get the input
    xd::CopyParams info;
    info.input = params_node["input"].getString();
    info.output = params_node["output"].getString();

    if (params_node.childExists("input_iterator"))
    {
        info.iter_input = (xd::IIterator*)(unsigned long)(kl::hexToUint64(params_node["input_iterator"].getString()));
        info.input = L"";
    }


    if (params_node.childExists("order"))
        info.order = params_node["order"].getString();
    if (params_node.childExists("where"))
        info.where = params_node["where"].getString();

    m_db->copyData(&info, tango_job.p);

    return 0;
}

void CopyJob::runPostJob()
{
}


};  // namespace jobs

