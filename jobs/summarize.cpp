/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-02-04
 *
 */


#include "kl/portable.h"
#include "jobspch.h"
#include "group.h"
#include "summarize.h"


namespace jobs
{


// SummarizeJob implementation

SummarizeJob::SummarizeJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
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
            "type" : "application/vnd.kx.summarize-data",
            "version" : 1,
            "description" : ""
        },
        "input" : <path>,
        "output" : <path>,
        "columns" : <string>,    // comma-delimited list of fieldnames to summarize
        "where" : <string>,      // not required; default = ""
    }
*/
    if (m_config.isNull())
        return false;

    // TODO: check job type and version

    if (!m_config.childExists("input"))
        return false;

    if (!m_config.childExists("output"))
        return false;

    if (!m_config.childExists("columns"))
        return false;

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

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


    // STEP 1: build the group job parameters from the summarize parameters

    std::wstring input_path = m_config["input"].getString();
    std::wstring input_columns = m_config["columns"].getString();

    tango::ISetPtr input_set = m_db->openSet(input_path);
    if (input_set.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    tango::IStructurePtr input_structure = input_set->getStructure();
    if (input_structure.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    std::vector<std::wstring> summary_columns;
    kl::parseDelimitedList(input_columns, summary_columns, ',');

    std::vector<std::wstring>::iterator it, it_end;
    it_end = summary_columns.end();


    std::wstring group_funcs;
    int output_max_scale = 0;   // used to format the numeric summary output

    tango::IColumnInfoPtr input_colinfo;
    bool last;
    for (it = summary_columns.begin(); it != summary_columns.end(); ++it)
    {
        last = false;
        if ((it+1) == summary_columns.end())
            last = true;

        input_colinfo = input_structure->getColumnInfo(*it);

        if (input_colinfo.isNull())
            continue;

        wchar_t outcol[256];

        if (input_colinfo->getType() == tango::typeCharacter ||
            input_colinfo->getType() == tango::typeWideCharacter)
        {
            swprintf(outcol, 256, L"%s_0result0_minlength=min(length([%s])),", it->c_str(), it->c_str());
            group_funcs += outcol;

            swprintf(outcol, 256, L"%s_0result0_maxlength=max(length([%s])),", it->c_str(), it->c_str());
            group_funcs += outcol;        
        }

        if (input_colinfo->getType() == tango::typeInteger ||
            input_colinfo->getType() == tango::typeDouble ||
            input_colinfo->getType() == tango::typeNumeric)
        {
            int scale = input_colinfo->getScale();
            if (scale > output_max_scale)
                output_max_scale = scale;
            
            swprintf(outcol, 256, L"%s_0result0_sum=sum([%s]),", it->c_str(), it->c_str());
            group_funcs += outcol;

            swprintf(outcol, 256, L"%s_0result0_avg=avg([%s]),", it->c_str(), it->c_str());
            group_funcs += outcol;
        }

        swprintf(outcol, 256, L"%s_0result0_min=min([%s]),", it->c_str(), it->c_str());
        group_funcs += outcol;

        swprintf(outcol, 256, L"%s_0result0_max=max([%s]),", it->c_str(), it->c_str());
        group_funcs += outcol;

        swprintf(outcol, 256, L"%s_0result0_empty=count(empty([%s])),", it->c_str(), it->c_str());
        group_funcs += outcol;
    }

    group_funcs += L"total_count=count()";


    kl::JsonNode group_params;
    group_params["input"].setString(m_config["input"].getString());
    group_params["output"].setString(tango::getTemporaryPath());    // temporary table to hold intermediate result
    group_params["group"].setString(L"");
    group_params["columns"].setString(group_funcs);

    std::wstring where_params;
    if (m_config.childExists("where"))
        group_params["where"].setString(m_config["where"].getString());



    // STEP 2: create a group job and pass the summarize parameters

    IJobPtr group_job = static_cast<IJob*>(new GroupJob);
    group_job->setDatabase(m_db);
    group_job->setParameters(group_params.toString());
    int result = group_job->runJob();

    IJobInfoPtr job_info = group_job->getJobInfo();
    if (job_info->getState() == jobStateFailed)
    {
        m_job_info->setState(jobStateFailed);
        // TODO: error code?
        return 0;
    }



    // STEP 3: pivot the output

    std::wstring group_output_path = group_params["output"];
    tango::ISetPtr group_output_set = m_db->openSet(group_output_path);

    if (group_output_set.isNull())
    {
        m_job_info->setState(jobStateFailed);
        // TODO: error code?
        return 0;
    }

    tango::ISetPtr group_results = group_output_set;
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

    tango::ISetPtr output_set = m_db->createSet(L"",
                                                output_structure,
                                                NULL);
    if (output_set.isNull())
    {
        m_job_info->setState(jobStateFailed);
        // TODO: error code?
        return 0;
    }

    tango::IRowInserterPtr output_inserter = output_set->getRowInserter();
    if (output_inserter.isNull())
    {
        m_job_info->setState(jobStateFailed);
        // TODO: error code?
        return 0;
    }

    output_inserter->startInsert(L"*");

    i = 0;
    while (fields[i])
    {
        field_handles[i] = output_inserter->getHandle(fields[i]);
        i++;
    }


    tango::IStructurePtr group_result_structure = group_output_set->getStructure();
    tango::IIteratorPtr group_result_iter = group_output_set->createIterator(L"", L"", NULL);
    
    if (group_result_iter.isNull())
    {
        m_job_info->setState(jobStateFailed);
        // TODO: error code?
        return 0;
    }

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


    // delete the temporary group table
    m_db->deleteFile(group_output_path);

    // store the summary result table
    std::wstring output_path = m_config["output"].getString();
    if (output_path.length() > 0)
        m_db->storeObject(output_set, output_path);


    return 0;
}

void SummarizeJob::runPostJob()
{
}


};  // namespace jobs

