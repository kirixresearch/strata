/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-18
 *
 */


#include "jobspch.h"
#include "update.h"


namespace jobs
{


// UpdateJob implementation

UpdateJob::UpdateJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
}

UpdateJob::~UpdateJob()
{
}

bool UpdateJob::isInputValid()
{
/*
    // example format:
    {
        "metadata":
        {
            "type" : "application/vnd.kx.update-job",
            "version" : 1,
            "description" : ""
        },
        "input" : <path>
    }
*/
    if (m_config.isNull())
        return false;

    // TODO: check job type and version

    if (!m_config.childExists("input"))
        return false;

    if (!m_config.childExists("actions"))
        return false;

    kl::JsonNode actions_node = m_config.getChild("actions");
    if (!actions_node.isArray())
        return false;

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

    return true;
}

int UpdateJob::runJob()
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

        // TODO: need to decide how to handle error strings; these need to 
        // be translated, so shouldn't be in this class
        //m_job_info->setProgressString(towstr(_("Update failed: The table may be in use by another user.")));
    }

    return 0;
}

void UpdateJob::runPostJob()
{
}


};  // namespace jobs

