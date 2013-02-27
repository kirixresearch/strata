/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-02-12
 *
 */


#include "jobspch.h"
#include "alter.h"


namespace jobs
{


// AlterJob implementation

AlterJob::AlterJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.alter-job";
    m_config["metadata"]["version"] = 1;
}

AlterJob::~AlterJob()
{
}

bool AlterJob::isInputValid()
{
/*
    // example format:
    {
        "metadata":
        {
            "type" : "application/vnd.kx.alter-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "actions" : [
                { 
                    "action" : "add" | "drop" | "modify",
                    "name" : <string>,
                    "params" : {
                        "name" : <string>,
                        "type" : "character" | "widecharacter" | "binary" | "numeric" | "double" | "integer" | "date" | "datetime" | "boolean",
                        "width": <integer>,
                        "scale": <integer>,
                        "expression": <string> | null,  // note: null expression turns off expressions
                        "position": <integer>
                },
                ...
            ]
        }
    }
*/
    if (m_config.isNull())
        return false;

    // TODO: check job type and version

    kl::JsonNode params = m_config["params"];
    if (params.isNull())
        return false;

    if (!params.childExists("input"))
        return false;

    if (!params.childExists("actions"))
        return false;

    kl::JsonNode actions_node = params.getChild("actions");
    if (!actions_node.isArray())
        return false;

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

    return true;
}

int AlterJob::runJob()
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

    // get the input parameters
    kl::JsonNode params = m_config["params"];
    std::wstring input_path = params["input"].getString();
    std::vector<kl::JsonNode> action_nodes = params["actions"].getChildren();

    tango::ISetPtr input_set = m_db->openSet(input_path);
    if (input_set.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }


    // build the structure configuration from the action list
    tango::IStructurePtr structure = input_set->getStructure();
    tango::IColumnInfoPtr col;

    std::vector<kl::JsonNode>::iterator it, it_end;
    it_end = action_nodes.end();

    for (it = action_nodes.begin(); it != it_end; ++it)
    {
        std::wstring action;
        std::wstring column;
        kl::JsonNode params;

        std::wstring name;
        std::wstring expression;
        unsigned int type = 0;
        unsigned int width = 0;
        unsigned int scale = 0;
        unsigned int position = 0;

        bool name_exists = false;
        bool type_exists = false;
        bool width_exists = false;
        bool scale_exists = false;
        bool position_exists = false;
        bool expression_exists = false;
        bool expression_isexplicitnull = false;


        // get the action and the column
        if (it->childExists("action"))
            action = it->getChild("action").getString();
        if (it->childExists("column"))
            column = it->getChild("column").getString();

        // get the parameters and overide any defaults
        if (it->childExists("params"))
        {
            params = it->getChild("params");

            if (params.childExists("name"))
            {
                name = params.getChild("name").getString();
                name_exists = true;
            }
            if (params.childExists("type"))
            {
                type = tango::stringToDbtype(params.getChild("type").getString());
                type_exists = true;
            }
            if (params.childExists("width"))
            {
                width = params.getChild("width").getInteger();
                width_exists = true;
            }
            if (params.childExists("scale"))
            {
                scale = params.getChild("scale").getInteger();
                scale_exists = true;
            }
            if (params.childExists("expression"))
            {
                expression = params.getChild("expression");
                expression_exists = true;

                if (params.getChild("expression").isNull())
                    expression_isexplicitnull = true;
            }
            if (params.childExists("position"))
            {
                position = params.getChild("position").getInteger();
                position_exists = true;
            }
        }


        // drop action
        if (action == L"drop")
        {
            structure->deleteColumn(column);
            continue;
        }

        if (action == L"add")
        {
            tango::IColumnInfoPtr col = structure->createColumn();
            col->setName(name);
            col->setType(type);
            col->setWidth(width);
            col->setScale(scale);
            col->setCalculated(false);

            if (expression_exists && !expression_isexplicitnull)
            {
                col->setCalculated(true);
                col->setExpression(expression);
            }

            if (position_exists)
                col->setColumnOrdinal(position);

            continue;
        }

        if (action == L"modify")
        {
            tango::IColumnInfoPtr col = structure->modifyColumn(column);

            if (name_exists)
                col->setName(name);
            if (type_exists)
                col->setType(type);
            if (width_exists)
                col->setWidth(width);
            if (scale_exists)
                col->setScale(scale);
            if (position_exists)
                col->setColumnOrdinal(position);
            if (expression_exists)
            {
                if (expression_isexplicitnull)
                {
                    col->setCalculated(false);
                }
                 else
                {
                    col->setCalculated(true);
                    col->setExpression(expression);
                }
            }

            continue;
        }
    }


    tango::IJobPtr tango_job = m_db->createJob();
    setTangoJob(tango_job);

    bool res = input_set->modifyStructure(structure, tango_job);

    if (tango_job->getCancelled())
    {
        m_job_info->setState(jobStateCancelling);
        return 0;
    }

    if (tango_job->getStatus() == tango::jobFailed || !res)
    {
        m_job_info->setState(jobStateFailed);

        // TODO: need to decide how to handle error strings; these need to 
        // be translated, so shouldn't be in this class
        //m_job_info->setProgressString(towstr(_("Modify failed: The table may be in use by another user.")));
    }

    return 0;
}

void AlterJob::runPostJob()
{
}


};  // namespace jobs

