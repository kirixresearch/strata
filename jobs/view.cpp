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
#include "view.h"
#include "util.h"
#include <kl/hex.h>


namespace jobs
{


// ViewJob implementation

ViewJob::ViewJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.view-job";
    m_config["metadata"]["version"] = 1;
}

ViewJob::~ViewJob()
{
}

bool ViewJob::isInputValid()
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

int ViewJob::runJob()
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

    std::wstring input = params_node["input"].getString();
    std::wstring output = params_node["output"].getString();
    kl::JsonNode columns_node = params_node["columns"];

    xd::FormatDefinition fd;
    fd.data_path = input;
    int i, cnt;


    // first, read the base format definition and add any calc fields to the new definition
    xd::FormatDefinition base_fd;
    if (m_db->loadDefinition(input, &base_fd))
    {
        // use the base path of the base definition
        fd.data_path = base_fd.data_path;

        std::vector<xd::ColumnInfo>::iterator it;
        for (it = base_fd.columns.begin(); it != base_fd.columns.end(); ++it)
        {
            if (it->calculated)
                fd.createColumn(*it);
        }
    }

    cnt = columns_node.getChildCount();
    for (i = 0; i < cnt; ++i)
    {
        kl::JsonNode column_node = columns_node[i];
        
        xd::ColumnInfo colinfo;
        colinfo.name = column_node["name"];
        colinfo.type = xd::stringToDbtype(column_node["type"]);
        colinfo.width = column_node["width"].getInteger();
        colinfo.scale = column_node["scale"].getInteger();
        colinfo.expression = column_node["expression"];

        int colidx = fd.columns.getColumnIdx(colinfo.name);
        if (colidx != -1)
        {
            // column already exists from base definition; overwrite it
            fd.columns.columns[colidx] = colinfo;
            fd.columns.m_map.clear();
        }

        fd.createColumn(colinfo);
    }


    m_db->saveDefinition(output, fd);

    return 0;
}

void ViewJob::runPostJob()
{
}


};  // namespace jobs

