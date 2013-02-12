/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-02-13
 *
 */


#include "jobspch.h"
#include "alter.h"


namespace jobs
{


// AlterJob implementation

AlterJob::AlterJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
}

AlterJob::~AlterJob()
{
}

bool AlterJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.alter-job",
            "version" : 1,
            "description" : ""
        },
        "input" : <path>,
        "output" : <path>,
        "actions" : []
    }
*/
    if (m_config.isNull())
        return false;

    // TODO: check job type and version

    if (!m_config.childExists("input"))
        return false;

    if (!m_config.childExists("output"))
        return false;

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

    return true;
}

int AlterJob::runJob()
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

    // get the input parameters
    std::wstring input_path = m_config["input"].getString();
    std::wstring output_path = m_config["output"].getString();

    return 0;
}

void AlterJob::runPostJob()
{
}


};  // namespace jobs

