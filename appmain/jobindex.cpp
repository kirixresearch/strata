/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-03-16
 *
 */


#include "appmain.h"
#include "jobindex.h"


IndexJob::IndexJob()
{
    m_job_info->setTitle(_("Creating Index"));
}

IndexJob::~IndexJob()
{
}


void IndexJob::addInstruction(tango::ISetPtr set,
                              const wxString& tag,
                              const wxString& expr)
{
    IndexJobInstruction i;

    i.set = set;
    i.tag = tag;
    i.expr = expr;

    m_instructions.push_back(i);
}

std::vector<IndexJobInstruction> IndexJob::getInstructions()
{
    return m_instructions;
}


int IndexJob::runJob()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        m_job_info->setState(cfw::jobStateFailed);
        return 0;
    }
    
    std::vector<IndexJobInstruction>::iterator it;
    
    for (it = m_instructions.begin(); it != m_instructions.end(); ++it)
    {
        tango::IJobPtr job = db->createJob();
        setTangoJob(job);
        
        wxString path = towx(it->set->getObjectPath());
        
        std::wstring sql = L"CREATE INDEX [";
        sql += towstr(dequoteIdentifier(it->tag));
        sql += L"] ON [";
        sql += towstr(dequoteIdentifier(path));
        sql += L"](";
        sql += towstr(it->expr);
        
        xcm::IObjectPtr null_result;
        db->execute(sql, 0, null_result, job);
    }

    return 0;
}



