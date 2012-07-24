/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-08-22
 *
 */


#ifndef __APP_JOBCOPY_H
#define __APP_JOBCOPY_H


#include "tangojobbase.h"


class CopyInstruction
{
public:

    enum
    {
        modeQuery = 0,
        modeIterator = 1,
        modeFilterSort = 2
    };

    int m_mode;

    // -- query mode parameters --
    tango::IDatabasePtr m_source_db;
    wxString m_query;

    // -- iterator mode parameters --
    tango::IIteratorPtr m_source_iter;
    wxString m_condition;
    wxString m_columns;
    
    // -- sort/filter mode parameters --
    wxString m_order;       // (used along with m_condition above)
    tango::ISetPtr m_source_set;

    // -- output parameters --
    tango::IDatabasePtr m_dest_db;
    tango::ISetPtr m_result_set;
    wxString m_dest_path;
};


xcm_interface ICopyJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ICopyJob")

public:

    virtual size_t getInstructionCount() = 0;
    virtual tango::ISetPtr getResultSet(size_t idx) = 0;
};

XCM_DECLARE_SMARTPTR(ICopyJob)


class CopyJob : public TangoJobBase,
                public ICopyJob
{
    XCM_CLASS_NAME("appmain.CopyJob")
    XCM_BEGIN_INTERFACE_MAP(CopyJob)
        XCM_INTERFACE_ENTRY(ICopyJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    CopyJob();
    virtual ~CopyJob();

    void addCopyInstruction(tango::IDatabasePtr source_db,
                            const wxString& query,
                            tango::IDatabasePtr dest_db,
                            const wxString& dest_path = wxEmptyString);
                                                        
    void addCopyInstruction(tango::IIteratorPtr source_iter,
                            const wxString& condition,
                            const wxString& columns,
                            tango::IDatabasePtr dest_db,
                            const wxString& dest_path = wxEmptyString);

    void addCopyInstruction(tango::IDatabasePtr source_db,
                            tango::ISetPtr source_set,
                            const wxString& columns,
                            const wxString& order,
                            const wxString& filter,
                            tango::IDatabasePtr dest_db,
                            const wxString& dest_path = wxEmptyString);

    size_t getInstructionCount();
    tango::ISetPtr getResultSet(size_t idx);

    int runJob();

private:

    xcm::mutex m_obj_mutex;
    std::vector<CopyInstruction> m_instructions;
};



#endif

