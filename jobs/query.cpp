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
    kl::JsonNode output_node = params_node["output"];
    kl::JsonNode where_node = params_node["where"];
    kl::JsonNode order_node = params_node["order"];
    kl::JsonNode distinct_node = params_node["distinct"];

    std::wstring input_str = input_node.getString();
    
    // if there is only one slash at the beginning, remove it
    // for compatibility with databases which don't have
    // hierarchical/folder namespaces
    if (kl::stringFrequency(input_str, '/') == 1 && input_str[0] == '/')
        input_str.erase(0, 1);

    std::wstring q_input_str = tango::quoteIdentifierIfNecessary(m_db, input_str);

    std::wstring output_str = output_node.getString();
    std::wstring q_output_str = tango::quoteIdentifierIfNecessary(m_db, output_node.getString());

    std::wstring where_str = where_node.getString();
    std::wstring order_str;

    std::vector<kl::JsonNode> order_children_node = order_node.getChildren();
    std::vector<kl::JsonNode>::iterator it, it_end;
    it_end = order_children_node.end();

    bool first = true;
    for (it = order_children_node.begin(); it != it_end; ++it)
    {
        if (!first)
            order_str += L",";

        // TODO: make sure fieldnames are quoted (dequote/quote) before
        // adding them to the list (remove any DESC qualifier, dequote/quote 
        // the field, and re-add the qualifier)
        order_str += it->getString();
    }

    bool distinct = false;
    if (distinct_node.isOk())
        distinct = distinct_node.getBoolean();

    std::wstring sql;
    sql += L" SELECT ";

    if (distinct)
    {
        sql += L" DISTINCT ";
    }

    sql += L" * ";

    if (output_str.length() > 0)
    {
        sql += L" INTO ";
        sql += q_output_str;
    }

    sql += L" FROM ";
    sql += q_input_str;

    if (where_str.length() > 0)
    {
        sql += L" WHERE ";
        sql += where_str;
    }

    if (order_str.length() > 0)
    {
        sql += L" ORDER BY ";
        sql += order_str;
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

