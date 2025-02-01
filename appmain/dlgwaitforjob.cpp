/*!
 *
 * Copyright (c) 2024, fopen GmbH  All rights reserved.
 *
 * Project:  Application Client
 * Created:  2025-02-01
 *
 */

#include "appmain.h"
#include "dlgwaitforjob.h"



BEGIN_EVENT_TABLE(DlgWaitForJob, wxDialog)
    EVT_TIMER(-1, DlgWaitForJob::onTimer)
    EVT_BUTTON(wxID_CANCEL, DlgWaitForJob::onCancel)
END_EVENT_TABLE()

void DlgWaitForJob::start(jobs::IJobInfoPtr job_info)
{
    if (!job_info)
        return;

    // Create and show the dialog hidden initially
    DlgWaitForJob* dlg = new DlgWaitForJob(g_app->getMainWindow(), job_info);
    dlg->Show(false);
}

DlgWaitForJob::DlgWaitForJob(wxWindow* parent, jobs::IJobInfoPtr job_info)
    : wxDialog(parent, wxID_ANY, _("Please Wait..."), 
               wxDefaultPosition, wxSize(parent->FromDIP(300), parent->FromDIP(200)),
               wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
{
    m_job_info = job_info;
    m_start_time = time(NULL);

    // Create dialog layout
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Add spacer to push button to center
    sizer->AddStretchSpacer(1);
    
    // Add cancel button
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Cancel"));
    sizer->Add(cancel_btn, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    
    // Add spacer to push button to center
    sizer->AddStretchSpacer(1);

    SetSizer(sizer);
    Center();

    // Connect to job state changes
    m_job_info->sigStateChanged().connect(this, &DlgWaitForJob::onJobStateChanged);

    // Start timer for initial wait period and state checking
    m_timer.SetOwner(this);
    m_timer.Start(200); // Check every 200ms

    // Show wait cursor
    wxBeginBusyCursor();
}

DlgWaitForJob::~DlgWaitForJob()
{
    if (wxIsBusy())
        wxEndBusyCursor();
}

void DlgWaitForJob::onTimer(wxTimerEvent& WXUNUSED(event))
{
    checkJobState();
}

void DlgWaitForJob::onCancel(wxCommandEvent& WXUNUSED(event))
{
    if (m_job_info && m_job_info->getCancelAllowed())
    {
        m_job_info->setState(jobs::jobStateCancelling);
    }
}

void DlgWaitForJob::onJobStateChanged(jobs::IJobInfoPtr job_info)
{
    checkJobState();
}

void DlgWaitForJob::checkJobState()
{
    if (!m_job_info)
    {
        Destroy();
        return;
    }

    int state = m_job_info->getState();
    time_t current_time = time(NULL);
    bool job_completed = (state & (jobs::jobStateFinished | jobs::jobStateCancelled | jobs::jobStateFailed));

    if (job_completed)
    {
        Destroy();
        return;
    }

    // If job is still running after 5 seconds, show the dialog
    if (!IsShown() && (current_time - m_start_time >= 5))
    {
        if (wxIsBusy())
            wxEndBusyCursor();
        Show(true);
    }
}
