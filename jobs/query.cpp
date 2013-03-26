/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-03-25
 *
 */


#include "jobspch.h"
#include "query.h"


namespace jobs
{


// QueryJob implementation

QueryJob::QueryJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.query-job";
    m_config["metadata"]["version"] = 1;
}

QueryJob::~QueryJob()
{
}

bool QueryJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.query-job",
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

int QueryJob::runJob()
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

    kl::JsonNode input_node = params_node["input"];
    kl::JsonNode where_node = params_node["where"];
    kl::JsonNode order_node = params_node["order"];


    std::wstring input = input_node.getString();
    std::wstring q_input = tango::quoteIdentifier(m_db, input_node.getString());

    std::wstring where_clause = where_node.getString();
    std::wstring order_clause;

    std::vector<kl::JsonNode> order_children_node = order_node.getChildren();
    std::vector<kl::JsonNode>::iterator it, it_end;
    it_end = order_children_node.end();

    bool first = true;
    for (it = order_children_node.begin(); it != it_end; ++it)
    {
        if (!first)
            order_clause += L",";

        // TODO: make sure fieldnames are quoted (dequote/requote) before
        // adding them to the list
        order_clause += it->getString();
    }


    std::wstring sql;
    sql += L"SELECT * FROM ";
    sql += q_input;
    if (where_clause.length() > 0)
    {
        sql += L" WHERE ";
        sql += where_clause;
    }
    if (order_clause.length() > 0)
    {
        sql += L" ORDER BY ";
        sql += order_clause;
    }


    tango::IJobPtr tango_job = m_db->createJob();
    setTangoJob(tango_job);

    xcm::IObjectPtr result;
    m_db->execute(sql, tango::sqlPassThrough, result, tango_job);
    setResultObject(result);

    if (tango_job->getCancelled())
    {
        m_job_info->setState(jobStateCancelling);
        return 0;
    }


    return 0;
}

void QueryJob::runPostJob()
{
}


};  // namespace jobs

