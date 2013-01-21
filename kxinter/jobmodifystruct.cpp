/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-08-28
 *
 */


#include "kxinter.h"
#include "jobmodifystruct.h"


// -- ModifyStructJob class implementation --

ModifyStructJob::ModifyStructJob()
{
    m_job_info->setTitle(_("Modify Structure"));
}

ModifyStructJob::~ModifyStructJob()
{
}

tango::ISetPtr ModifyStructJob::getActionSet()
{
    return m_action_set;
}

void ModifyStructJob::addToRename(wxString old_name, wxString new_name)
{
    m_to_rename.push_back(std::pair<wxString, wxString>(old_name, new_name));
}

void ModifyStructJob::addToInsert(wxString field_name)
{
    m_to_insert.push_back(field_name);
}

void ModifyStructJob::addToDelete(wxString field_name)
{
    m_to_delete.push_back(field_name);
}

void ModifyStructJob::getToRename(std::vector<std::pair<wxString,wxString> >& vec)
{
    vec = m_to_rename;
}

void ModifyStructJob::getToInsert(std::vector<wxString>& vec)
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
    // -- set the title --
    tango::IStorablePtr sp = m_action_set;

    if (sp)
    {
        if (!sp->isTemporary())
        {
            wxString op = towx(sp->getObjectPath());
            op = op.AfterLast('/');
            wxString path = cfw::makeProper(op);
            m_job_info->setTitle(wxString::Format(_("Modify Structure (%s)"), path.c_str()));
        }
    }

    sp.clear();

    m_action_set->modifyStructure(m_struct_config, NULL);

    return 0;
}

void ModifyStructJob::runPostJob()
{
    m_action_set.clear();
    m_struct_config.clear();
}




