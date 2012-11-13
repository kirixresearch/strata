/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-29
 *
 */


#ifndef __APP_JOBSCRIPT_H
#define __APP_JOBSCRIPT_H


#include "scripthost.h"


class ScriptJob : public cfw::JobBase,
                  public xcm::signal_sink
{
    XCM_CLASS_NAME("appmain.ScriptJob")
    XCM_BEGIN_INTERFACE_MAP(ScriptJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ScriptJob();
    virtual ~ScriptJob();

    void setScriptHost(ScriptHost* script_host);
    bool cancel();
    int runJob();
    void runPostJob();    

private:

    void onGetCurrentCount(double* d);
    void onGetMaxCount(double* d);
    void onGetPercentage(double* d);
    void onScriptHostJobChanged();

private:

    ScriptHost* m_script_host;

    cfw::IJobInfoPtr m_script_jobinfo;
    cfw::IJobPtr m_script_job;

    int m_rterror_code;
    wxString m_rterror_string;
};


#endif



