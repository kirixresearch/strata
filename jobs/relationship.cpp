/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-02-19
 *
 */


#include "jobspch.h"
#include "relationship.h"
#include "util.h"


namespace jobs
{


// RelationshipJob implementation

RelationshipJob::RelationshipJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.relationship-job";
    m_config["metadata"]["version"] = 1;
}

RelationshipJob::~RelationshipJob()
{
}

bool RelationshipJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.relationship-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "relationships" : [
                {
                    "name" : <string>,              // tag name of the relationship
                    "left_path" : <path>,           // path of the left table in the relationship
                    "left_expression" : <string>,   // fields in the left table that form the relationship
                    "right_path" : <path>,          // path of the right table in the relationship
                    "right_expression" : <string>   // fields in the right table that form the relationship
                },
                ...
            ]
        }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

    return true;
}

int RelationshipJob::runJob()
{
    // make sure we have a valid input
    if (!isInputValid())
    {
        // TODO: correlate input validation with job error information
    
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }

    xd::IRelationSchemaPtr rels = m_db;

    // make sure the database is valid
    if (m_db.isNull() || rels.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }    


    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    std::vector<kl::JsonNode>::iterator it, it_end;
    std::vector<kl::JsonNode> relationships = params_node["relationships"].getChildren();


    // get the relationships
    it_end = relationships.end();
    for (it = relationships.begin(); it != it_end; ++it)
    {
        std::wstring name = it->getChild("name").getString();
        std::wstring left_path = it->getChild("left_path").getString();
        std::wstring left_expr = it->getChild("left_expression").getString();
        std::wstring right_path = it->getChild("right_path").getString();
        std::wstring right_expr = it->getChild("right_expression").getString();


        // get the indexes
        xd::IndexInfoEnum right_set_indexes = m_db->getIndexEnum(right_path);

        // if we're on an external database, then move on
        if (jobs::getMountRoot(m_db, right_path).length() != 0)
            continue;

        // if the index already exists, then move on
        xd::IndexInfo idx = jobs::lookupIndex(right_set_indexes, right_expr, false);
        if (idx.isOk())
            continue;


        // quote identifiers
        std::wstring q_name = xd::quoteIdentifier(m_db, name);
        std::wstring q_input = xd::quoteIdentifier(m_db, right_path);

        std:: wstring sql;
        sql += L"CREATE INDEX ";
        sql += q_name;
        sql += L" ON ";
        sql += q_input;
        sql += L" (";
        sql += right_expr;
        sql += L"); ";

        // create the indexes
        xd::IJobPtr job = m_db->createJob();
        setXdJob(job);
        xcm::IObjectPtr null_result;
        m_db->execute(sql, 0, null_result, job);
    }


    // once the indexes are created, add the relationships
    it_end = relationships.end();
    for (it = relationships.begin(); it != it_end; ++it)
    {
        std::wstring name = it->getChild("name").getString();
        std::wstring left_path = it->getChild("left_path").getString();
        std::wstring left_expr = it->getChild("left_expression").getString();
        std::wstring right_path = it->getChild("right_path").getString();
        std::wstring right_expr = it->getChild("right_expression").getString();

        rels->createRelation(name, left_path, right_path, left_expr, right_expr);
    }

    return 0;
}

void RelationshipJob::runPostJob()
{
}


};  // namespace jobs

