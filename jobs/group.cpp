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

GroupJob::GroupJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
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

    // TODO: check job type and version


    kl::JsonNode params = m_config["params"];
    if (params.isUndefined())
        return false;

    if (!params.childExists("input"))
        return false;

    if (!params.childExists("output"))
        return false;

    if (!params.childExists("group"))
        return false;

    if (!params.childExists("columns"))
        return false;

    kl::JsonNode group_node = params.getChild("group");
    if (!group_node.isArray())
        return false;

    kl::JsonNode columns_node = params.getChild("columns");
    if (!columns_node.isArray())
        return false;

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


    kl::JsonNode params = m_config["params"];

    // get the input parameters
    std::wstring input_path = params["input"].getString();
    std::wstring output_path = params["output"].getString();
    std::vector<kl::JsonNode> group_nodes = params["group"].getChildren();
    std::vector<kl::JsonNode> column_nodes = params["columns"].getChildren();

    std::wstring where_params;
    if (params.childExists("where"))
        where_params = params["where"].getString();

    std::wstring having_params;
    if (params.childExists("having"))
        having_params = params["having"].getString();

    bool unique_records = false;
    if (params.childExists("unique"))
        unique_records = params["unique"].getBoolean();

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
    //tango::requoteAllIdentifiers(m_db, group_values);
    //tango::requoteAllIdentifiers(m_db, column_values);

    std::wstring group_params, column_params;
    jobs::vectorToDelimitedString(group_values, group_params);
    jobs::vectorToDelimitedString(column_values, column_params);


    tango::IJobPtr tango_job;
    tango::ISetPtr output_set;
    tango::ISetPtr input_set = m_db->openSet(input_path);

    if (!unique_records)
    {
        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        output_set = m_db->runGroupQuery(input_set,
                                         group_params,
                                         column_params,
                                         where_params,
                                         having_params,
                                         tango_job.p);

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

        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        tango::ISetPtr intermediate_output_set1;
        intermediate_output_set1 = m_db->runGroupQuery(input_set,
                                                       group_params,
                                                       L"[DETAIL]",
                                                       where_params,
                                                       having_params,
                                                       tango_job.p);

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
            //m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));

            return 0;
        }

        if (output_set.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }


        std::wstring sql = L"SELECT DISTINCT * FROM ";
        sql += intermediate_output_set1->getObjectPath();

        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        xcm::IObjectPtr result;
        m_db->execute(sql, 0, result, tango_job.p);

        tango::IIteratorPtr iter = result;

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (iter.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        tango::ISetPtr intermediate_output_set2 = iter->getSet();
        if (intermediate_output_set2.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }



        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        output_set = m_db->runGroupQuery(intermediate_output_set2,
                                         group_params,
                                         column_params,
                                         where_params,
                                         having_params,
                                         tango_job.p);

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
            //m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));

            return 0;
        }
    }


    if (output_path.length() > 0)
        m_db->storeObject(output_set, output_path);


    return 0;
}

void GroupJob::runPostJob()
{
}


};  // namespace jobs

