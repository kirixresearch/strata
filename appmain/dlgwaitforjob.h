/*!
 *
 * Copyright (c) 2024, fopen GmbH  All rights reserved.
 *
 * Project:  Application Client
 * Created:  2025-02-01
 *
 */

#ifndef H_APP_DLGWAITFORJOB_H
#define H_APP_DLGWAITFORJOB_H


class DlgWaitForJob : public wxDialog, public xcm::signal_sink
{
public:
    static void start(jobs::IJobInfoPtr job_info);

private:
    DlgWaitForJob(wxWindow* parent, jobs::IJobInfoPtr job_info);
    virtual ~DlgWaitForJob();

    void onTimer(wxTimerEvent& event);
    void onCancel(wxCommandEvent& event);
    void onJobStateChanged(jobs::IJobInfoPtr job_info);
    void checkJobState();

    jobs::IJobInfoPtr m_job_info;
    wxTimer m_timer;
    time_t m_start_time;

    DECLARE_EVENT_TABLE()
};


#endif 