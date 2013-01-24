/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-01-24
 *
 */


#include "jobspch.h"
#include "group.h"


namespace jobs
{


// GroupJob implementation

GroupJob::GroupJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
}

GroupJob::~GroupJob()
{
}

bool GroupJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.group-data",
            "version" : 1,
            "description" : ""
        }
    }
*/
    if (m_config.isNull())
        return false;

    // TODO: check job type and version

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

    return true;
}

int GroupJob::runJob()
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

    return 0;
}

void GroupJob::runPostJob()
{
}


};  // namespace jobs

