/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-06-10
 *
 */


#ifndef __APP_JOBMODIFYSTRUCT_H
#define __APP_JOBMODIFYSTRUCT_H


#include "tangojobbase.h"


xcm_interface IModifyStructJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IModifyStructJob")

public:

    virtual tango::ISetPtr getActionSet() = 0;
    virtual void getToRename(std::vector<std::pair<wxString,wxString> >& vec) = 0;
    virtual void getToInsert(std::vector<std::pair<wxString,int> >& vec) = 0;
    virtual void getToDelete(std::vector<wxString>& vec) = 0;
};

XCM_DECLARE_SMARTPTR(IModifyStructJob)



// -- ModifyStructJob --

class ModifyStructJob : public TangoJobBase,
                        public IModifyStructJob
{
    XCM_CLASS_NAME("appmain.ModifyStructJob")
    XCM_BEGIN_INTERFACE_MAP(ModifyStructJob)
        XCM_INTERFACE_ENTRY(IModifyStructJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ModifyStructJob();
    virtual ~ModifyStructJob();

    tango::ISetPtr getActionSet();

    void setInstructions(tango::ISetPtr action_set,
                         tango::IStructurePtr struct_config);

    void addToRename(const wxString& old_name, const wxString& new_name);
    void addToInsert(const wxString& field_name, int pos);
    void addToDelete(const wxString& field_name);
    
    void getToRename(std::vector<std::pair<wxString,wxString> >& vec);
    void getToInsert(std::vector<std::pair<wxString,int> >& vec);
    void getToDelete(std::vector<wxString>& vec);

    int runJob();
    void runPostJob();

private:

    tango::ISetPtr m_action_set;
    tango::IStructurePtr m_struct_config;

    std::vector<std::pair<wxString, wxString> > m_to_rename;
    std::vector<std::pair<wxString, int> > m_to_insert;
    std::vector<std::pair<wxString, int> > m_to_move;
    std::vector<wxString> m_to_delete;
};


#endif

