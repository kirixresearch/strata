/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-02-04
 *
 */

#include "jobspch.h"
#include "kl/portable.h"
#include "group.h"
#include "summarize.h"
#include <set>


namespace jobs
{


// SummarizeJob implementation

SummarizeJob::SummarizeJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.summarize-job";
    m_config["metadata"]["version"] = 1;
}

SummarizeJob::~SummarizeJob()
{
}

bool SummarizeJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.summarize-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "output" : <path>,
            "columns" : <array>,     // array of fieldnames to summarize
            "where" : <string>,      // not required; default = ""
        }
    }
*/

    if (m_config.isUndefined())
        return false;

    // TODO: add validation

    return true;
}

int SummarizeJob::runJob()
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


    // STEP 1: build the group job parameters from the summarize parameters

    std::wstring input_path = params_node["input"].getString();
    std::wstring output_path = params_node["output"].getString();

    std::vector<kl::JsonNode> input_columns = params_node["columns"].getChildren();
    std::vector<kl::JsonNode>::iterator it_node, it_node_end;
    it_node_end = input_columns.end();
    
    std::vector<std::wstring> summary_columns;
    summary_columns.reserve(input_columns.size());

    for (it_node = input_columns.begin(); it_node != it_node_end; ++it_node)
    {
        // make sure identifer is quoted
        std::wstring col = it_node->getString();
        tango::dequoteIdentifier(m_db, col);
        tango::quoteIdentifier(m_db, col);

        summary_columns.push_back(col);
    }

    tango::IStructurePtr input_structure = m_db->describeTable(input_path);
    if (input_structure.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    std::set<std::wstring> summary_columns_unique_list;
    std::vector<std::wstring>::iterator it, it_end;
    it_end = summary_columns.end();

    std::wstring column_param;
    int output_max_scale = 0;   // used to format the numeric summary output

    tango::IColumnInfoPtr input_colinfo;
    for (it = summary_columns.begin(); it != it_end; ++it)
    {
        // see if we're already gathering information about the column
        // in question; if so, move on
        if (summary_columns_unique_list.find(*it) != summary_columns_unique_list.end())
            continue;

        summary_columns_unique_list.insert(*it);
        input_colinfo = input_structure->getColumnInfo(*it);

        if (input_colinfo.isNull())
            continue;

        wchar_t outcol[256];

        if (input_colinfo->getType() == tango::typeCharacter ||
            input_colinfo->getType() == tango::typeWideCharacter)
        {
            swprintf(outcol, 256, L"%s_0result0_minlength=min(length([%s])),", it->c_str(), it->c_str());
            column_param += outcol;

            swprintf(outcol, 256, L"%s_0result0_maxlength=max(length([%s])),", it->c_str(), it->c_str());
            column_param += outcol;        
        }

        if (input_colinfo->getType() == tango::typeInteger ||
            input_colinfo->getType() == tango::typeDouble ||
            input_colinfo->getType() == tango::typeNumeric)
        {
            int scale = input_colinfo->getScale();
            if (scale > output_max_scale)
                output_max_scale = scale;
            
            swprintf(outcol, 256, L"%s_0result0_sum=sum([%s]),", it->c_str(), it->c_str());
            column_param += outcol;

            swprintf(outcol, 256, L"%s_0result0_avg=avg([%s]),", it->c_str(), it->c_str());
            column_param += outcol;
        }

        swprintf(outcol, 256, L"%s_0result0_min=min([%s]),", it->c_str(), it->c_str());
        column_param += outcol;

        swprintf(outcol, 256, L"%s_0result0_max=max([%s]),", it->c_str(), it->c_str());
        column_param += outcol;

        swprintf(outcol, 256, L"%s_0result0_empty=count(empty([%s])),", it->c_str(), it->c_str());
        column_param += outcol;
    }

    column_param += L"total_count=count()";


    // STEP 2: create a group job and pass the summarize parameters

    std::wstring group_output_set_path = L"xtmp_" + kl::getUniqueString();

    tango::IJobPtr tango_job;
    tango_job = m_db->createJob();
    setTangoJob(tango_job);

    std::wstring where_param;
    if (params_node.childExists("where"))
        where_param = params_node["where"].getString();

    tango::GroupQueryInfo info;
    info.input = input_path;
    info.output = group_output_set_path;
    info.columns = column_param,
    info.where = where_param;

    bool res = m_db->groupQuery(&info, tango_job.p);

    m_to_delete.push_back(info.output);

    if (!res || tango_job->getCancelled())
    {
        m_job_info->setState(jobStateCancelling);
        return 0;
    }

    if (tango_job->getStatus() == tango::jobFailed)
    {
        m_job_info->setState(jobStateFailed);
        // TODO: error code?
        return 0;
    }


    // STEP 3: pivot the output

    tango::IStructurePtr output_structure = m_db->createStructure();
    tango::IColumnInfoPtr output_colinfo;

    const wchar_t* fields[] = { L"Field",
                                L"Minimum",
                                L"Maximum",
                                L"Sum_Amount",
                                L"Average",
                                L"Min_Length",
                                L"Max_Length",
                                L"Empty_Count",
                                L"Total_Count",
                                0 };

    tango::objhandle_t field_handles[255];

    int i;
    
    i = 0;
    while (fields[i])
    {
        output_colinfo = output_structure->createColumn();
        output_colinfo->setName(fields[i]);
        output_colinfo->setType(tango::typeWideCharacter);
        output_colinfo->setWidth(255);
        output_colinfo->setScale(0);

        if (0 == wcscasecmp(fields[i], L"Sum_Amount") ||
            0 == wcscasecmp(fields[i], L"Average"))
        {
            output_colinfo->setType(tango::typeNumeric);
            output_colinfo->setWidth(15);
            output_colinfo->setScale(output_max_scale);
        }
        
        if (0 == wcscasecmp(fields[i], L"Min_Length") ||
            0 == wcscasecmp(fields[i], L"Max_Length") ||
            0 == wcscasecmp(fields[i], L"Total_Count") ||
            0 == wcscasecmp(fields[i], L"Empty_Count"))
        {
            output_colinfo->setType(tango::typeNumeric);
            output_colinfo->setWidth(12);
        }

        ++i;
    }

    if (!m_db->createTable(output_path, output_structure, NULL))
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    tango::IRowInserterPtr output_inserter = m_db->bulkInsert(output_path);
    if (output_inserter.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    output_inserter->startInsert(L"*");

    i = 0;
    while (fields[i])
    {
        field_handles[i] = output_inserter->getHandle(fields[i]);
        i++;
    }


    tango::IIteratorPtr group_result_iter = m_db->createIterator(group_output_set_path, L"", L"", L"", NULL);
    if (group_result_iter.isNull())
    {
        m_job_info->setState(jobStateFailed);
        // TODO: error code?
        return 0;
    }

    tango::IStructurePtr group_result_structure = group_result_iter->getStructure();

    group_result_iter->goFirst();
    if (group_result_iter->eof())
    {
        m_job_info->setState(jobStateFailed);
        // TODO: error code?
        return 0;
    }

    std::wstring last_field_name = L"";
    std::wstring field_name;
    std::wstring col_name;
    

    tango::objhandle_t total_count_handle = group_result_iter->getHandle(L"total_count");
    double total_count = group_result_iter->getDouble(total_count_handle);
    group_result_iter->releaseHandle(total_count_handle);


    int result_field_count = group_result_structure->getColumnCount();
    for (i = 0; i < result_field_count; ++i)
    {
        output_colinfo = group_result_structure->getColumnInfoByIdx(i);

        col_name = output_colinfo->getName();
        kl::makeUpper(col_name);

        size_t idx = col_name.find(L"_0RESULT0_");
        if (idx == std::wstring::npos)
            continue;

        std::wstring type = col_name.substr(idx+10);
        kl::makeUpper(type);

        field_name = col_name.substr(0,idx);
        tango::objhandle_t h = 0;

        if (type == L"MIN")
            h = field_handles[1];
         else if (type == L"MAX")
            h = field_handles[2];
         else if (type == L"SUM")
            h = field_handles[3];
         else if (type == L"AVG")
            h = field_handles[4];
         else if (type == L"MINLENGTH")
            h = field_handles[5];
         else if (type == L"MAXLENGTH")
            h = field_handles[6];
         else if (type == L"EMPTY")
            h = field_handles[7];

        if (!h)
            continue;


        if (last_field_name != field_name)
        {
            if (i > 0)
            {
                output_inserter->insertRow();
            }
            
            last_field_name = field_name;
            std::wstring upper_field_name = field_name;
            kl::makeUpper(upper_field_name);

            output_inserter->putWideString(field_handles[0],
                                           upper_field_name);

            // total count
            output_inserter->putDouble(field_handles[8], total_count);
        }


        tango::objhandle_t result_col_handle = group_result_iter->getHandle(col_name);
        tango::IColumnInfoPtr result_col_info = group_result_iter->getInfo(result_col_handle);
    
        // empty count
        if (h == field_handles[7])
        {
            output_inserter->putDouble(h, group_result_iter->getDouble(result_col_handle));
            continue;
        }

        switch (result_col_info->getType())
        {
            case tango::typeWideCharacter:
            case tango::typeCharacter:
                output_inserter->putWideString(h, group_result_iter->getWideString(result_col_handle));
                break;

            case tango::typeDate:
            {
                tango::DateTime dt = group_result_iter->getDateTime(result_col_handle);
                std::wstring s;

                if (!dt.isNull())
                {
                    wchar_t buf[64];
                    swprintf(buf, 64, L"%04d-%02d-%02d", dt.getYear(), dt.getMonth(), dt.getDay());
                    s = buf;
                }

                output_inserter->putWideString(h, s);
            }
            break;

            case tango::typeDateTime:
            {
                tango::DateTime dt = group_result_iter->getDateTime(result_col_handle);
                std::wstring s;

                if (!dt.isNull())
                {
                    wchar_t buf[64];
                    swprintf(buf, 64, L"%04d-%02d-%02d %02d:%02d:%02d", dt.getYear(), dt.getMonth(), dt.getDay(), dt.getHour(), dt.getMinute(), dt.getSecond());
                    s = buf;
                }

                output_inserter->putWideString(h, s);
            }
            break;


            case tango::typeDouble:
            case tango::typeNumeric:
            case tango::typeInteger:
            {
                if (type == L"SUM" || type == L"AVG")
                {
                    output_inserter->putDouble(h, group_result_iter->getDouble(result_col_handle));
                }
                 else if (type == L"MINLENGTH" || type == L"MAXLENGTH")
                {
                    output_inserter->putInteger(h, group_result_iter->getInteger(result_col_handle));
                }
                 else
                {
                    wchar_t buf[255];
                    swprintf(buf, 255, L"%.*f", result_col_info->getScale(), group_result_iter->getDouble(result_col_handle));
                    output_inserter->putWideString(h, buf);
                }
            }
            break;

            case tango::typeBoolean:
            {
                output_inserter->putWideString(h, group_result_iter->getBoolean(result_col_handle) ? L"TRUE" : L"FALSE");
            }
            break;
        }
    }
    
    if (i > 0)
        output_inserter->insertRow();

    output_inserter->finishInsert();


    return 0;
}

void SummarizeJob::runPostJob()
{
    if (m_db)
    {
        std::vector<std::wstring>::iterator it;
        for (it = m_to_delete.begin(); it != m_to_delete.end(); ++it)
            m_db->deleteFile(*it);
    }
}


};  // namespace jobs

