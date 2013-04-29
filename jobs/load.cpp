/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-04-09
 *
 */


#include "jobspch.h"
#include "bulkcopy.h"


namespace jobs
{

// example:
/*
{
    "metadata" : {
        "type" : "application/vnd.kx.bulkcopy-job",
        "version" : 1,
        "description" : ""
    },
    "params": {
    }
}
*/


// BulkCopyJob implementation

BulkCopyJob::BulkCopyJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.bulkcopy-job";
    m_config["metadata"]["version"] = 1;
}

BulkCopyJob::~BulkCopyJob()
{
}

bool BulkCopyJob::isInputValid()
{
    // TODO: fill out

    return true;
}

int BulkCopyJob::runJob()
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



    // TODO: add copy loop here
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
        //m_job_info->setProgressString(towstr(_("Modify failed: The table may be in use by another user.")));
    }

    return 0;
}

void BulkCopyJob::runPostJob()
{
}


};  // namespace jobs

