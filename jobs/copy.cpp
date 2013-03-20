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


namespace jobs
{


// CopyJob implementation

CopyJob::CopyJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
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


    kl::JsonNode params = m_config["params"];

    // get the input parameters

    tango::CopyInfo info;
    info.input = params["input"].getString();
    info.output = params["output"].getString();

    if (params.childExists("order"))
        info.order = params["order"].getString();
    if (params.childExists("where"))
        info.where = params["where"].getString();

    m_db->copyData(&info, NULL);

    return 0;
}

void CopyJob::runPostJob()
{
}


};  // namespace jobs

