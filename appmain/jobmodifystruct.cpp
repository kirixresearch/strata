/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-06-10
 *
 */


#include "appmain.h"
#include "jobmodifystruct.h"

// -- ModifyStructJob class implementation --

ModifyStructJob::ModifyStructJob()
{
    m_job_info->setTitle(towstr(_("Modify Structure")));
}

ModifyStructJob::~ModifyStructJob()
{
}

tango::ISetPtr ModifyStructJob::getActionSet()
{
    return m_action_set;
}

void ModifyStructJob::addToRename(const wxString& old_name,
                                  const wxString& new_name)
{
    m_to_rename.push_back(std::pair<wxString, wxString>(old_name, new_name));
}

void ModifyStructJob::addToInsert(const wxString& field_name, int pos)
{
    m_to_insert.push_back(std::pair<wxString, int>(field_name, pos));
}

void ModifyStructJob::addToDelete(const wxString& field_name)
{
    m_to_delete.push_back(field_name);
}



void ModifyStructJob::getToRename(std::vector<std::pair<wxString,wxString> >& vec)
{
    vec = m_to_rename;
}

void ModifyStructJob::getToInsert(std::vector<std::pair<wxString,int> >& vec)
{
    vec = m_to_insert;
}

void ModifyStructJob::getToDelete(std::vector<wxString>& vec)
{
    vec = m_to_delete;
}


void ModifyStructJob::setInstructions(tango::ISetPtr action_set,
                                      tango::IStructurePtr struct_config)
{
    m_action_set = action_set;
    m_struct_config = struct_config;
}

int ModifyStructJob::runJob()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }
    
    tango::IJobPtr tango_job = db->createJob();
    setTangoJob(tango_job);

    // -- set the title --

    if (m_action_set.isNull())
        return 0;
        
    bool res = m_action_set->modifyStructure(m_struct_config, tango_job);

    if (tango_job->getStatus() == tango::jobCancelled)
        return 0;

    if (tango_job->getStatus() == tango::jobFailed || !res)
    {
        m_job_info->setProgressString(towstr(_("Modify failed: The table may be in use by another user.")));
        m_job_info->setState(jobStateFailed);
    }

    return 0;
}

void ModifyStructJob::runPostJob()
{
    if (getJobInfo()->getState() != jobStateFinished)
        return;
    
    FrameworkEvent* evt = new FrameworkEvent(wxT("appmain.tableStructureModified"));
    evt->s_param = towx(m_action_set->getObjectPath());
    g_app->getMainFrame()->postEvent(evt);
}




