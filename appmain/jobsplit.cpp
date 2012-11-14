/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-10-18
 *
 */


#include "appmain.h"
#include "jobsplit.h"
#include "appcontroller.h"
#include <ctime>


// -- SplitJob class implementation --

SplitJob::SplitJob()
{
    m_si = NULL;
    m_job_info->setTitle(towstr(_("Divide")));
}

SplitJob::~SplitJob()
{

}

void SplitJob::setInstructions(SplitInfo* si)
{
    m_si = si;
}

int SplitJob::runJob()
{
    if (!m_si)
    {
        return 0;
    }

    int max_row_count = m_si->source_set->getRowCount();
    int rows_left = max_row_count;
    m_job_info->setMaxCount(max_row_count);

    tango::IJobPtr tango_job;
    tango::IStructurePtr structure;
    structure = m_si->source_set->getStructure();

    tango::IDatabasePtr database = g_app->getDatabase();
    tango::ISetPtr dest_set;
    tango::IIteratorPtr iter;
    
    iter = m_si->source_set->createIterator(L"", L"", NULL);
    iter->goFirst();


    std::vector< std::pair<tango::ISetPtr, wxString> > to_store;

    int i = 0;
    while (rows_left > 0)
    {
        tango_job = g_app->getDatabase()->createJob();
        setTangoJob(tango_job);

        dest_set = database->createSet(L"", structure, NULL);

        if (rows_left >= m_si->row_count)
        {
            dest_set->insert(iter, L"", m_si->row_count, tango_job);
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
            wxString output_path = m_si->prefix;
            
            if (m_si->table_count < 10)
                output_path += wxString::Format(wxT("_%01d"), i+1);
             else if (m_si->table_count >= 10 && m_si->table_count < 100)
                output_path += wxString::Format(wxT("_%02d"), i+1);
             else
                output_path += wxString::Format(wxT("_%03d"), i+1);

            to_store.push_back(std::pair<tango::ISetPtr, wxString>(dest_set, output_path));

            rows_left -= m_si->row_count;
        }
        
        i++;
    }


    if (!isCancelling())
    {
        std::vector< std::pair<tango::ISetPtr, wxString> >::iterator it;
        for (it = to_store.begin(); it != to_store.end(); ++it)
        {
            database->storeObject(it->first, towstr(it->second));
        }
    }


    m_si->source_set.clear();

    return 0;
}

void SplitJob::runPostJob()
{
    g_app->getAppController()->refreshDbDoc();
    
    // this SplitInfo object is created in the SplitPanel,
    // but needs to be deleted after the job is completed
    delete m_si;
}


