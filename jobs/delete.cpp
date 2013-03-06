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
#include "delete.h"


namespace jobs
{


// DeleteJob implementation

DeleteJob::DeleteJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.delete-job";
    m_config["metadata"]["version"] = 1;
}

DeleteJob::~DeleteJob()
{
}

bool DeleteJob::isInputValid()
{
/*
    // example format:
    {
        "metadata":
        {
            "type" : "application/vnd.kx.delete-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "where": ""
        }
    }
*/
    if (m_config.isUndefined())
        return false;

    // TODO: check job type and version

    kl::JsonNode params = m_config["params"];
    if (params.isUndefined())
        return false;

    if (!params.childExists("input"))
        return false;

    if (!params.childExists("where"))
        return false;

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

    return true;
}

int DeleteJob::runJob()
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
    std::wstring input_path = params["input"].getString();
    std::wstring where_param = params["where"].getString();


    // build the delete SQL
    std::wstring delete_sql = L"";
    delete_sql += L"DELETE FROM ";
    delete_sql += tango::quoteIdentifier(m_db, input_path);
    delete_sql += L" WHERE ";
    delete_sql += where_param;


    tango::IJobPtr tango_job = m_db->createJob();
    setTangoJob(tango_job);

    xcm::IObjectPtr result;
    m_db->execute(delete_sql, tango::sqlPassThrough, result, tango_job);


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
        //m_job_info->setProgressString(towstr(_("Delete failed")));
    }

    return 0;
}

void DeleteJob::runPostJob()
{
}


};  // namespace jobs

