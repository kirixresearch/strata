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
            "type" : "application/vnd.kx.divide-data",
            "version" : 1,
            "description" : ""
        },
        "input" : <path>,
        "output" : <path>,      // prefix used for output path
        "row_count" : <integer>
    }
*/

    if (m_config.isNull())
        return false;

    // TODO: check job type and version

    if (!m_config.childExists("input"))
        return false;

    if (!m_config.childExists("output"))
        return false;

    if (!m_config.childExists("row_count"))
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

    // get the input
    std::wstring input_path = m_config["input"].getString();
    tango::ISetPtr input_set = m_db->openSet(input_path);

    // get the output prefix and row count
    std::wstring output_prefix = m_config["output"].getString();
    size_t output_row_count = m_config["row_count"].getInteger();

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
        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        dest_set = m_db->createSet(L"", structure, NULL);

        if (rows_left >= output_row_count)
        {
            dest_set->insert(iter, L"", output_row_count, tango_job);
        }
         else
        {
            dest_set->insert(iter, L"", rows_left, tango_job);
        }

        if (isCancelling())
        {
            break;
        }
         else
        {
            wchar_t buf[10];
            if (output_table_count < 10)
                swprintf(buf, L"%01d", i+1);
            else if (output_table_count >= 10 && output_table_count < 100)
                swprintf(buf, L"%02d", i+1);
            else
                swprintf(buf, L"%03d", i+1);

            std::wstring output_suffix(buf);
            std::wstring output_path = output_prefix + output_suffix;
            to_store.push_back(std::pair<tango::ISetPtr, std::wstring>(dest_set, output_path));

            if (rows_left >= output_row_count)
                rows_left -= output_row_count;
                 else
                rows_left = 0;
        }

        i++;
    }

    if (!isCancelling())
    {
        std::vector< std::pair<tango::ISetPtr, std::wstring> >::iterator it;
        for (it = to_store.begin(); it != to_store.end(); ++it)
        {
            m_db->storeObject(it->first, it->second);
        }
    }

    return 0;
}

void DivideJob::runPostJob()
{
}


};  // namespace jobs
