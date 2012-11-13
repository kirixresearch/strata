/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-03-16
 *
 */


#ifndef __APP_JOBINDEX_H
#define __APP_JOBINDEX_H


#include "tangojobbase.h"


class IndexJobInstruction
{
public:
    tango::ISetPtr set;
    wxString tag;
    wxString expr;
};



xcm_interface IIndexJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IIndexJob")

public:

    virtual std::vector<IndexJobInstruction> getInstructions() = 0;
};

XCM_DECLARE_SMARTPTR(IIndexJob)





class IndexJob : public TangoJobBase,
                 public IIndexJob
{
    XCM_CLASS_NAME("appmain.IndexJob")
    XCM_BEGIN_INTERFACE_MAP(IndexJob)
        XCM_INTERFACE_ENTRY(IIndexJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    IndexJob();
    virtual ~IndexJob();

    void addInstruction(tango::ISetPtr set,
                        const wxString& tag,
                        const wxString& expr);

    std::vector<IndexJobInstruction> getInstructions();

    int runJob();
    void runPostJob();

private:

    std::vector<IndexJobInstruction> m_instructions;
};



#endif
