/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-09-29
 *
 */


#ifndef __APP_PANELJOBSCHEDULER_H
#define __APP_PANELJOBSCHEDULER_H


#include <wx/datectrl.h>
#include <wx/dateevt.h>
class wxSpinEvent;


class JobSchedulerPanelEntry
{
public:

    enum Schedule
    {
        Once = 0,
        Daily,
        Weekly,
        Monthly
    };

    enum Days
    {
        InvalidDay = 0x00,
        Mon        = 0x01,
        Tue        = 0x02,
        Wed        = 0x04,
        Thu        = 0x08,
        Fri        = 0x10,
        Sat        = 0x20,
        Sun        = 0x40,
        Weekdays   = Mon|Tue|Wed|Thu|Fri,
        Weekend    = Sat|Sun,
        AllDays    = Mon|Tue|Wed|Thu|Fri|Sat|Sun,
    };

public:

    JobSchedulerPanelEntry()
    {
        name = wxEmptyString;
        schedule = JobSchedulerPanelEntry::Once;
        active = true;
        
        start_time = wxDateTime::Now();
        start_time.SetHour(8);
        start_time.SetMinute(0);
        start_time.SetSecond(0);
        finish_time = wxDateTime::Now();
        finish_time.SetHour(8);
        finish_time.SetMinute(0);
        finish_time.SetSecond(0);
        finish_active = false;
        
        daily_repeat_active = false;
        daily_repeat_interval = 30;
        weekly_days = JobSchedulerPanelEntry::Mon;
        monthly_daynumber = 1;
    }

public:

    wxString name;

    Schedule schedule;          // daily, weekly, etc.
    bool active;                // only active jobs will run
    
    wxDateTime start_time;
    wxDateTime finish_time;
    bool finish_active;
    
    bool daily_repeat_active;
    int daily_repeat_interval;  // in minutes
    int weekly_days;            // day bitmask
    int monthly_daynumber;      // 1-13
    
    std::vector<wxString> commands;
};


class JobSchedulerPanel : public wxPanel,
                          public cfw::IDocument,
                          public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.JobSchedulerPanel")
    XCM_BEGIN_INTERFACE_MAP(JobSchedulerPanel)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
    XCM_END_INTERFACE_MAP()

public:

    JobSchedulerPanel();
    ~JobSchedulerPanel();

private:

    // -- cfw::IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    void insertScheduledJob(int row = -1);
    void checkOverlayText();
    void updateJobList(int row, bool refresh = false);
    void populateCommandList(JobSchedulerPanelEntry& entry);
    void updateJobParameters(JobSchedulerPanelEntry& entry);
    void resetJobParameters();
    
private: // layout functions

    void doLayout();
    wxBoxSizer* createVerticalSizer();
    wxStaticBoxSizer* createDailySizer();
    wxStaticBoxSizer* createWeeklySizer();
    wxStaticBoxSizer* createMonthlySizer();

private:

    void onJobListInsertingRows(std::vector<int> rows);
    void onJobListDeletedRows(std::vector<int> rows);
    void onCommandListDeletedRows(std::vector<int> rows);

private:

    // -- event handlers --
    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onFrequencySelected(wxCommandEvent& evt);
    void onGridEndEdit(kcl::GridEvent& evt);
    void onGridCursorMove(kcl::GridEvent& evt);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    void onFinishTimeActiveChecked(wxCommandEvent& evt);
    void onDailyRepeatActiveChecked(wxCommandEvent& evt);
    void onDailyRepeatIntervalText(wxCommandEvent& evt);
    void onMonthlyDayNumberText(wxCommandEvent& evt);
    void onWeeklyDayChecked(wxCommandEvent& evt);
    void onStartDateChanged(wxDateEvent& evt);
    void onFinishDateChanged(wxDateEvent& evt);
    void onStartTimeSpunUp(wxSpinEvent& evt);
    void onStartTimeSpunDown(wxSpinEvent& evt);
    void onFinishTimeSpunUp(wxSpinEvent& evt);
    void onFinishTimeSpunDown(wxSpinEvent& evt);

private: // utilities

    wxString createJobDescription(JobSchedulerPanelEntry* entry);
    void setTextCtrlTime(wxTextCtrl* ctrl, int& hour, int& min, bool inc_val);
    void getTextCtrlTime(wxTextCtrl* ctrl, int* hour, int* min);
    
private:

    cfw::IDocumentSitePtr m_doc_site;
    
    kcl::RowSelectionGrid* m_joblist_grid;
    kcl::RowSelectionGrid* m_commandlist_grid;

    // general
    wxChoice* m_frequency_choice;
    
    wxTextCtrl*       m_starttime_textctrl;
    wxSpinButton*     m_starttime_spinbutton;
    wxDatePickerCtrl* m_starttime_datectrl;
    
    wxCheckBox*       m_finishtime_active_checkbox;
    wxTextCtrl*       m_finishtime_textctrl;
    wxSpinButton*     m_finishtime_spinbutton;
    wxDatePickerCtrl* m_finishtime_datectrl;

    // daily sizer
    wxCheckBox* m_daily_repeat_active_checkbox;
    wxSpinCtrl* m_daily_repeat_interval_spinctrl;

    // weekly sizer
    wxCheckBox* m_weekly_mon_checkbox;
    wxCheckBox* m_weekly_tue_checkbox;
    wxCheckBox* m_weekly_wed_checkbox;
    wxCheckBox* m_weekly_thu_checkbox;
    wxCheckBox* m_weekly_fri_checkbox;
    wxCheckBox* m_weekly_sat_checkbox;
    wxCheckBox* m_weekly_sun_checkbox;

    // monthly sizer
    wxSpinCtrl* m_monthly_daynumber_spinctrl;

    // sizers
    wxBoxSizer* m_vert_sizer;
    wxBoxSizer* m_top_sizer;
    wxBoxSizer* m_starttime_sizer;
    wxBoxSizer* m_finishtime_sizer;
    wxStaticBoxSizer* m_daily_sizer;
    wxStaticBoxSizer* m_weekly_sizer;
    wxStaticBoxSizer* m_monthly_sizer;
    
    bool m_has_ampm;
    
    JobSchedulerPanelEntry* m_current_job_entry;

    DECLARE_EVENT_TABLE()
};




#endif

