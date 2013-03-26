/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; David Z. Williams
 * Created:  2013-01-15
 *
 */


#include "jobspch.h"
#include "divide.h"


namespace jobs
{


// DivideJob implementation

DivideJob::DivideJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
    m_config["metadata"]["type"] = L"application/vnd.kx.divide-job";
    m_config["metadata"]["version"] = 1;
}

DivideJob::~DivideJob()
{
}

bool DivideJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.divide-job",
            "version" : 1,
            "description" : ""
        },
        "params":
        {
            "input" : <path>,
            "output" : <path>,      // prefix used for output path
            "row_count" : <integer>
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

    if (!params.childExists("row_count"))
        return false;

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

    return true;
}

int DivideJob::runJob()
{
    if (!isInputValid())
    {
        // TODO: correlate input validation with job error information
    
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }


    kl::JsonNode params = m_config["params"];

    // get the input
    std::wstring input_path = params["input"].getString();
    tango::ISetPtr input_set = m_db->openSet(input_path);

    // get the output prefix and row count
    std::wstring output_prefix = params["output"].getString();
    size_t output_row_count = params["row_count"].getInteger();

    if (input_set.isNull())
    {
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;    
    }

    size_t max_row_count = input_set->getRowCount();
    if (output_row_count <= 0 || output_row_count >= max_row_count)
        output_row_count = max_row_count;

    size_t output_table_count = (max_row_count/output_row_count) + (max_row_count%output_row_count > 0 ? 1 : 0);
    size_t rows_left = max_row_count;


    m_job_info->setMaxCount(max_row_count);


    tango::IJobPtr tango_job;
    tango::IStructurePtr structure;
    structure = input_set->getStructure();

    tango::ISetPtr dest_set;
    tango::IIteratorPtr iter;
    
    iter = input_set->createIterator(L"", L"", NULL);
    iter->goFirst();


    std::vector< std::pair<tango::ISetPtr, std::wstring> > to_store;

    int i = 0;
    while (rows_left > 0)
    {
        std::wstring suffix, output_path;

        i++;

        if (output_table_count < 10)
            suffix = kl::stdswprintf(L"%01d", i+1);
        else if (output_table_count >= 10 && output_table_count < 100)
            suffix = kl::stdswprintf(L"%02d", i+1);
        else
            suffix = kl::stdswprintf(L"%03d", i+1);

        output_path = output_prefix + suffix;

        if (rows_left >= output_row_count)
            rows_left -= output_row_count;
                else
            rows_left = 0;

        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        tango::CopyInfo info;
        info.iter_input = iter;
        info.output = output_path;
        info.max_rows = (rows_left >= output_row_count) ? output_row_count : rows_left;
        m_db->copyData(&info, tango_job);

        if (isCancelling())
            break;
    }

    return 0;
}

void DivideJob::runPostJob()
{
}


};  // namespace jobs

