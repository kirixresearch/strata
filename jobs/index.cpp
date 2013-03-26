/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-01-17
 *
 */


#include "jobspch.h"
#include "index.h"


namespace jobs
{


// IndexJob implementation

IndexJob::IndexJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.index-job";
    m_config["metadata"]["version"] = 1;
}

IndexJob::~IndexJob()
{
}

bool IndexJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.index-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "indexes" : [
                {
                    "input" : <path>,
                    "name" : <string>,
                    "expression" : <string>
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

int IndexJob::runJob()
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

    // get the index node children
    std::vector<kl::JsonNode> children = params["indexes"].getChildren();
    std::vector<kl::JsonNode>::iterator it, it_end;
    it_end = children.end();


    // create indexes for each of the objects in the index array
    for (it = children.begin(); it != it_end; ++it)
    {
        std::wstring input;
        std::wstring name;
        std::wstring expression;
    
        kl::JsonNode child = *it;
        if (child.childExists("input"))
            input = child["input"];
        if (child.childExists("name"))
            name = child["name"];
        if (child.childExists("expression"))
            expression = child["expression"];                        

        if (input.length() == 0 || name.length() == 0 || expression.length() == 0)
            continue;


        tango::IJobPtr job = m_db->createJob();
        setTangoJob(job);

        // quote identifiers
        std::wstring q_name = tango::quoteIdentifier(m_db, name);
        std::wstring q_input = tango::quoteIdentifier(m_db, input);

        std::wstring sql;
        sql += L"CREATE INDEX ";
        sql += q_name;
        sql += L" ON ";
        sql += q_input;
        sql += L" (";
        sql += expression;
        sql += L");";
        
        xcm::IObjectPtr null_result;
        m_db->execute(sql, 0, null_result, job);
    }

    return 0;
}

void IndexJob::runPostJob()
{
}


};  // namespace jobs

