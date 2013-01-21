/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-08-28
 *
 */


#ifndef __KXINTER_JOBMODIFYSTRUCT_H
#define __KXINTER_JOBMODIFYSTRUCT_H


xcm_interface IModifyStructJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("kxinter.IModifyStructJob")

public:

    virtual tango::ISetPtr getActionSet() = 0;
    virtual void getToRename(std::vector<std::pair<wxString,wxString> >& vec) = 0;
    virtual void getToInsert(std::vector<wxString>& vec) = 0;
    virtual void getToDelete(std::vector<wxString>& vec) = 0;
};

XCM_DECLARE_SMARTPTR(IModifyStructJob)




// -- ModifyStructJob --

class ModifyStructJob : public cfw::JobBase,
                        public IModifyStructJob
{
    XCM_CLASS_NAME("kxinter.ModifyStructJob")
    XCM_BEGIN_INTERFACE_MAP(ModifyStructJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ModifyStructJob();
    virtual ~ModifyStructJob();

    tango::ISetPtr getActionSet();

    void setInstructions(tango::ISetPtr action_set,
                         tango::IStructurePtr struct_config);

    void addToRename(wxString old_name, wxString new_name);
    void addToInsert(wxString field_name);
    void addToDelete(wxString field_name);
    void getToRename(std::vector<std::pair<wxString,wxString> >& vec);
    void getToInsert(std::vector<wxString>& vec);
    void getToDelete(std::vector<wxString>& vec);

    int runJob();
    void runPostJob();

private:

    tango::ISetPtr m_action_set;
    tango::IStructurePtr m_struct_config;
    tango::IJobPtr m_tango_job;

    std::vector<std::pair<wxString, wxString> > m_to_rename;
    std::vector<wxString> m_to_insert;
    std::vector<wxString> m_to_delete;
};


#endif

