/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-30
 *
 */


#ifndef __APP_JOBGROUP_H
#define __APP_JOBGROUP_H


#include "tangojobbase.h"



xcm_interface IGroupJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IGroupJob")

public:

    virtual tango::ISetPtr getResultSet() = 0;
    virtual wxString getOutputPath() = 0;
};

XCM_DECLARE_SMARTPTR(IGroupJob)


class GroupJob : public TangoJobBase,
                 public IGroupJob
{
    XCM_CLASS_NAME("appmain.GroupJob")
    XCM_BEGIN_INTERFACE_MAP(GroupJob)
        XCM_INTERFACE_ENTRY(IGroupJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    GroupJob();
    virtual ~GroupJob();

    void setInstructions(tango::ISetPtr source_set,
                         const wxString& group,
                         const wxString& columns,
                         const wxString& where,
                         const wxString& having);

    void setUniqueRecordsOnly(bool val);

    void setOutputPath(const wxString& output_path);
    wxString getOutputPath();

    tango::ISetPtr getResultSet();
    tango::IIteratorPtr getResultIterator();

    int runJob();
    void runPostJob();

private:

    xcm::mutex m_obj_mutex;

    // -- parameters --
    tango::ISetPtr m_source_set;
    wxString m_group;
    wxString m_columns;
    wxString m_where;
    wxString m_having;
    wxString m_output_path;
    bool m_unique_records_only;

    // -- results --
    tango::ISetPtr m_result_set;
    tango::IIteratorPtr m_result_iter;
};







#endif

