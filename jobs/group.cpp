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
#include "util.h"


namespace jobs
{


// GroupJob implementation

GroupJob::GroupJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.group-job";
    m_config["metadata"]["version"] = 1;
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
            "type" : "application/vnd.kx.group-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "output" : <path>,
            "group" : <array>,
            "columns" : <array>,
            "where" : <string>,      // not required; default = ""
            "having" : <string>,     // not required; default = ""
            "unique" : <boolean>     // not required; default = false
        }
    }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

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


    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    // get the input parameters
    std::wstring input_path = params_node["input"].getString();
    std::wstring output_path = params_node["output"].getString();
    std::vector<kl::JsonNode> group_nodes = params_node["group"].getChildren();
    std::vector<kl::JsonNode> column_nodes = params_node["columns"].getChildren();

    std::wstring where_params;
    if (params_node.childExists("where"))
        where_params = params_node["where"].getString();

    std::wstring having_params;
    if (params_node.childExists("having"))
        having_params = params_node["having"].getString();

    bool unique_records = false;
    if (params_node.childExists("unique"))
        unique_records = params_node["unique"].getBoolean();

    std::vector<std::wstring> group_values, column_values;
    std::vector<kl::JsonNode>::iterator it_node;

    group_values.reserve(group_nodes.size());
    for (it_node = group_nodes.begin(); it_node != group_nodes.end(); ++it_node)
        group_values.push_back(it_node->getString());

    column_values.reserve(column_nodes.size());
    for (it_node = column_nodes.begin(); it_node != column_nodes.end(); ++it_node)
        column_values.push_back(it_node->getString());

    // TODO: anyway to make sure format is properly quoted?
    // following is a possibility, except that column values
    // are of the form <output_name>=<group_func<input>, so
    // requoting doesn't work:
    //xd::requoteAllIdentifiers(m_db, group_values);
    //xd::requoteAllIdentifiers(m_db, column_values);

    std::wstring group_params, column_params;
    jobs::vectorToDelimitedString(group_values, group_params);
    jobs::vectorToDelimitedString(column_values, column_params);


    xd::IJobPtr xd_job;

    if (!unique_records)
    {
        xd_job = m_db->createJob();
        setXdJob(xd_job);

        xd::GroupQueryParams info;
        info.input = input_path;
        info.output = output_path;
        info.group = group_params;
        info.columns = column_params;
        info.where = where_params;
        info.having = having_params;

        bool res = m_db->groupQuery(&info, xd_job.p);

        if (!res || xd_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (xd_job->getStatus() == xd::jobFailed)
        {
            m_job_info->setState(jobStateFailed);

            // TODO: need to decide how to handle error strings; these need to 
            // be translated, so shouldn't be in this class
            //m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));

            return 0;
        }
    }
     else
    {
        // this whole section is specialized code to optimize the option
        // for unique records:  rather than selecting distinct first, then
        // doing a group operation, this first groups with the related
        // "where" and "having", then removes duplicates, then regroups
        // it's optimized for instances where "where" and "having" are
        // specified and the expected output size is small

        xd_job = m_db->createJob();
        setXdJob(xd_job);

        xd::GroupQueryParams info1;
        info1.input = input_path;
        info1.output = L"xtmp_" + kl::getUniqueString();
        info1.group = group_params;
        info1.columns = L"[DETAIL]";
        info1.where = where_params;
        info1.having = having_params;
        bool res1 = m_db->groupQuery(&info1, xd_job.p);
        m_to_delete.push_back(info1.output);

        if (xd_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (!res1 || xd_job->getStatus() == xd::jobFailed)
        {
            m_job_info->setState(jobStateFailed);

            // TODO: need to decide how to handle error strings; these need to 
            // be translated, so shouldn't be in this class
            //m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));

            return 0;
        }



        std::wstring output2 = L"xtmp_" + kl::getUniqueString();
        std::wstring sql = L"SELECT DISTINCT * INTO " + output2 + L" FROM " + info1.output;
        m_to_delete.push_back(output2);

        xd_job = m_db->createJob();
        setXdJob(xd_job);

        xcm::IObjectPtr result;
        m_db->execute(sql, 0, result, xd_job.p);

        xd::IIteratorPtr iter = result;

        if (xd_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (m_db->getFileExist(output2))
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }




        xd_job = m_db->createJob();
        setXdJob(xd_job);

        xd::GroupQueryParams info3;
        info3.input = output2;
        info3.output = output_path;
        info3.group = group_params;
        info3.columns = column_params;
        info3.where = where_params;
        info3.having = having_params;
        m_to_delete.push_back(info1.output);

        bool res3 =  m_db->groupQuery(&info1, xd_job.p);

        if (xd_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (!res3 || xd_job->getStatus() == xd::jobFailed)
        {
            m_job_info->setState(jobStateFailed);

            // TODO: need to decide how to handle error strings; these need to 
            // be translated, so shouldn't be in this class
            //m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));

            return 0;
        }
    }


    return 0;
}

void GroupJob::runPostJob()
{
/*
    if (m_db)
    {
        std::vector<std::wstring>::iterator it;
        for (it = m_to_delete.begin(); it != m_to_delete.end(); ++it)
            m_db->deleteFile(*it);
    }
    */
}


};  // namespace jobs

