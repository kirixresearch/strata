/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-29
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "jobscript.h"


ScriptJob::ScriptJob()
{
    m_script_host = NULL;
    m_job_info->setTitle(towstr(_("Script")));

    m_job_info->setInfoMask(cfw::jobMaskTitle |
                            cfw::jobMaskStartTime |
                            cfw::jobMaskFinishTime |
                            cfw::jobMaskProgressString |
                            cfw::jobMaskProgressBar);

    m_job_info->sigOnGetCurrentCount().connect(this, &ScriptJob::onGetCurrentCount);
    m_job_info->sigOnGetMaxCount().connect(this, &ScriptJob::onGetMaxCount);
    m_job_info->sigOnGetPercentage().connect(this, &ScriptJob::onGetPercentage);
}

ScriptJob::~ScriptJob()
{
    delete m_script_host;

    m_script_job.clear();
}


void ScriptJob::onGetCurrentCount(double* d)
{
    if (m_script_jobinfo.isNull())
    {
        *d = 0.0;
        return;
    }

    *d = m_script_jobinfo->getCurrentCount();
}

void ScriptJob::onGetMaxCount(double* d)
{
    if (m_script_jobinfo.isNull())
    {
        *d = 0.0;
        return;
    }

    *d = m_script_jobinfo->getMaxCount();
}

void ScriptJob::onGetPercentage(double* d)
{
    if (m_script_jobinfo.isNull())
    {
        *d = 0.0;
        return;
    }

    *d = m_script_jobinfo->getPercentage();
}


void ScriptJob::onScriptHostJobChanged()
{
    wxString caption = _("Script");

    m_script_job = m_script_host->getCurrentJob();
    m_script_jobinfo.clear();

    if (m_script_job)
    {
        m_script_jobinfo = m_script_job->getJobInfo();

        if (m_script_jobinfo)
        {
            caption += wxT(" [");
            caption += m_script_jobinfo->getTitle();
            caption += wxT("]");
        }
    }

    m_job_info->setTitle(towstr(caption));
}


void ScriptJob::setScriptHost(ScriptHost* script_host)
{
    m_script_host = script_host;
    m_script_host->sigJobChanged.connect(this, &ScriptJob::onScriptHostJobChanged);
}

bool ScriptJob::cancel()
{
    JobBase::cancel();

    m_script_host->cancel();

    if (m_script_job)
    {
        m_script_job->cancel();
        m_script_job.clear();
    }

    return true;
}

int ScriptJob::runJob()
{
    bool res = m_script_host->run();

    if (!res)
    {
        m_rterror_code = m_script_host->getErrorCode();
        m_rterror_string = m_script_host->getErrorString();
    }

    return 0;
}

void ScriptJob::runPostJob()
{
    if (!m_rterror_string.IsEmpty())
    {
        wxString s = _("Script Runtime Error");
        s += wxT(": ");
        s += m_rterror_string;
        s += wxT("\n");
        
        g_app->getAppController()->showConsolePanel();
        g_app->getAppController()->printConsoleText(s);
    }
}





