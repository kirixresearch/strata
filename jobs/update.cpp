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
    m_config["metadata"]["type"] = L"application/vnd.kx.update-job";
    m_config["metadata"]["version"] = 1;
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
        "params":
        {
            "input" : <path>,
            "where": "",
            "set" : [
                {
                    "column" : <string>,
                    "expression"
                },
                ...
            ]
        }
    }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

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


    kl::JsonNode params = m_config["params"];

    // get the input parameters
    std::wstring input_path = params["input"].getString();

    std::wstring where_param;
    if (params.childExists("where"))
        where_param = params["where"].getString();

    std::vector<kl::JsonNode> set_nodes;
    set_nodes = params["set"].getChildren();


    // build the update SQL
    std::wstring update_sql = L"";
    update_sql += L"UPDATE ";
    update_sql += tango::quoteIdentifier(m_db, input_path);
    update_sql += L" SET ";

    std::vector<kl::JsonNode>::iterator it, it_end;
    it_end = set_nodes.end();

    bool first = true;
    for (it = set_nodes.begin(); it != it_end; ++it)
    {
        if (!first)
            update_sql += L",";
        first = false;

        std::wstring column, expression;
        if (!it->childExists("column") || !it->childExists("expression"))
            continue;

        column = it->getChild("column").getString();
        expression = it->getChild("expression").getString();

        update_sql += tango::quoteIdentifier(m_db, column);
        update_sql += L" = ";
        update_sql += expression;
    }

    if (where_param.length() > 0)
        update_sql += (L" WHERE " + where_param);


    tango::IJobPtr tango_job = m_db->createJob();
    setTangoJob(tango_job);

    xcm::IObjectPtr result;
    m_db->execute(update_sql, tango::sqlPassThrough, result, tango_job);


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
        //m_job_info->setProgressString(towstr(_("Update failed")));
    }

    return 0;
}

void UpdateJob::runPostJob()
{
}


};  // namespace jobs

