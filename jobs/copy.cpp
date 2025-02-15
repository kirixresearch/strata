/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-03-16
 *
 */


#include "jobspch.h"
#include "copy.h"
#include "util.h"
#include <kl/hex.h>


namespace jobs
{


// CopyJob implementation

CopyJob::CopyJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.copy-job";
    m_config["metadata"]["version"] = 1;
}

CopyJob::~CopyJob()
{
}

bool CopyJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.copy-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "output" : <path>,
            "columns" : [],          // if omitted, all columns are copied
            "where" : <expr>,
            "order" : <expr>
        }
    }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

    return true;
}

int CopyJob::runJob()
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

    xd::IJobPtr xd_job = m_db->createJob();
    setXdJob(xd_job);

    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    // get the input
    xd::CopyParams info;
    info.input = params_node["input"].getString();
    info.output = params_node["output"].getString();

    if (params_node.childExists("input_iterator"))
    {
        info.iter_input = (xd::IIterator*)(uintptr_t)(kl::hexToUint64(params_node["input_iterator"].getString()));
        info.input = L"";
    }

    /*
    if (params_node.childExists(L"input_format") && params_node["input_format"].isString())
    {
        xd::QueryParams qp;
        qp.from = info.input;
        if (xd::Util::loadDefinitionFromString(params_node["input_format"], &qp.format))
        {
            info.iter_input = m_db->query(qp);
            if (info.iter_input.isNull())
                return 0;
        }
    }
    */

    if (params_node.childExists("columns"))
    {
        std::vector<std::wstring> vec;

        kl::JsonNode columns_node = params_node.getChild("columns");
        if (columns_node.isArray())
        {
            std::vector<kl::JsonNode> children = columns_node.getChildren();
            for (auto& col : children)
            {
                vec.push_back(col.getString());
            }
        }
        else
        {
            kl::parseDelimitedList(params_node["columns"].getString(), vec, L',', true);

            for (auto& colname : vec)
            {
                xd::dequoteIdentifier(colname, '[', ']');
                xd::dequoteIdentifier(colname, '"', '"');
            }
        }

        for (auto& colname : vec)
        {
            info.addCopyColumn(colname, colname);
        }
    }

    if (params_node.childExists("order"))
    {
        info.order = params_node["order"].getString();
    }

    if (params_node.childExists("where"))
    {
        info.where = params_node["where"].getString();
    }

    m_db->copyData(&info, xd_job.p);

    return 0;
}

void CopyJob::runPostJob()
{
}


};  // namespace jobs

