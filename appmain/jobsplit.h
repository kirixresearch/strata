/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-10-18
 *
 */


#ifndef __APP_JOBSPLIT_H
#define __APP_JOBSPLIT_H


#include "tangojobbase.h"


enum SplitType
{
    splitUndefined = 0,
    splitByRowCount,
    splitByTableCount,
    splitByTableSize,
    splitByExpression
};

struct SplitInfo
{
    tango::ISetPtr source_set;  // the table we are going to split

    SplitType type;      // how we splitting the table
    wxString prefix;     // prefix of the output tables
    wxString expression; // expression to split on
    long table_count;    // number of tables to split the table into
    long row_count;      // number of records to put into each output table
};


class SplitJob : public TangoJobBase
{
    XCM_CLASS_NAME("appmain.SplitJob")
    XCM_BEGIN_INTERFACE_MAP(SplitJob)
        XCM_INTERFACE_CHAIN(JobBase)
    XCM_END_INTERFACE_MAP()

public:

    SplitJob();
    virtual ~SplitJob();

    void setInstructions(SplitInfo* si);

    int runJob();
    void runPostJob();

private:
    SplitInfo* m_si;
};



#endif


