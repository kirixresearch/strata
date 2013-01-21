/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-09-29
 *
 */


#include "appmain.h"
#include "paneljobscheduler.h"
#include "jobscheduler.h"
#include "dbdoc.h"


enum
{
    ID_Frequency_Choice = wxID_HIGHEST + 1,
    ID_StartTime_TextCtrl,
    ID_StartTime_SpinButton,
    ID_StartDate_DateCtrl,
    ID_FinishActive_CheckBox,
    ID_FinishTime_TextCtrl,
    ID_FinishTime_SpinButton,
    ID_FinishDate_DateCtrl,
    
    // daily
    ID_Daily_Repeat_Active_CheckBox,
    ID_Daily_Repeat_Interval_SpinCtrl,
    
    // weekly
    ID_Weekly_Mon_CheckBox,
    ID_Weekly_Tue_CheckBox,
    ID_Weekly_Wed_CheckBox,
    ID_Weekly_Thu_CheckBox,
    ID_Weekly_Fri_CheckBox,
    ID_Weekly_Sat_CheckBox,
    ID_Weekly_Sun_CheckBox,
    
    // monthly
    ID_Monthly_DayNumber_SpinCtrl
};

enum JobListColumnIndexes
{
    colActive = 0,
    colName = 1,
    colDescription = 2
};

enum ScheduleChoiceIndexes
{
    ScheduleChoice_Once = 0,
    ScheduleChoice_Daily,
    ScheduleChoice_Weekly,
    ScheduleChoice_Monthly
};

// -- utility functions --

void populateJobSchedulerEntry(JobSchedulerPanelEntry* panel_entry, JobSchedulerEntry& e)
{
    e.name   = panel_entry->name;
    e.active = panel_entry->active;
    
    switch (panel_entry->schedule)
    {
        default:
        case JobSchedulerPanelEntry::Once:      e.schedule = JobSchedulerEntry::Once;       break;
        case JobSchedulerPanelEntry::Daily:     e.schedule = JobSchedulerEntry::Daily;      break;
        case JobSchedulerPanelEntry::Weekly:    e.schedule = JobSchedulerEntry::Weekly;     break;
        case JobSchedulerPanelEntry::Monthly:   e.schedule = JobSchedulerEntry::Monthly;    break;
    }
    
    e.start_time    = panel_entry->start_time.GetTicks();
    e.finish_time   = panel_entry->finish_time.GetTicks();
    e.finish_active = panel_entry->finish_active;
    
    e.daily_repeat_active   = panel_entry->daily_repeat_active;
    e.daily_repeat_interval = panel_entry->daily_repeat_interval;
    
    e.weekly_days = JobSchedulerEntry::InvalidDay;
    e.weekly_days |= (panel_entry->weekly_days & JobSchedulerPanelEntry::Mon) ? JobSchedulerEntry::Mon : JobSchedulerEntry::InvalidDay;
    e.weekly_days |= (panel_entry->weekly_days & JobSchedulerPanelEntry::Tue) ? JobSchedulerEntry::Tue : JobSchedulerEntry::InvalidDay;
    e.weekly_days |= (panel_entry->weekly_days & JobSchedulerPanelEntry::Wed) ? JobSchedulerEntry::Wed : JobSchedulerEntry::InvalidDay;
    e.weekly_days |= (panel_entry->weekly_days & JobSchedulerPanelEntry::Thu) ? JobSchedulerEntry::Thu : JobSchedulerEntry::InvalidDay;
    e.weekly_days |= (panel_entry->weekly_days & JobSchedulerPanelEntry::Fri) ? JobSchedulerEntry::Fri : JobSchedulerEntry::InvalidDay;
    e.weekly_days |= (panel_entry->weekly_days & JobSchedulerPanelEntry::Sat) ? JobSchedulerEntry::Sat : JobSchedulerEntry::InvalidDay;
    e.weekly_days |= (panel_entry->weekly_days & JobSchedulerPanelEntry::Sun) ? JobSchedulerEntry::Sun : JobSchedulerEntry::InvalidDay;

    e.monthly_daynumber = panel_entry->monthly_daynumber;
    
    e.commands = panel_entry->commands;
}

void populateJobSchedulerPanelEntry(JobSchedulerEntry& e, JobSchedulerPanelEntry** entry)
{
    JobSchedulerPanelEntry* panel_entry = *entry;
    panel_entry->name   = e.name;
    panel_entry->active = e.active;
    
    switch (e.schedule)
    {
        default:
        case JobSchedulerEntry::Once:       panel_entry->schedule = JobSchedulerPanelEntry::Once;       break;
        case JobSchedulerEntry::Daily:      panel_entry->schedule = JobSchedulerPanelEntry::Daily;      break;
        case JobSchedulerEntry::Weekly:     panel_entry->schedule = JobSchedulerPanelEntry::Weekly;     break;
        case JobSchedulerEntry::Monthly:    panel_entry->schedule = JobSchedulerPanelEntry::Monthly;    break;
    }
    
    panel_entry->start_time    = wxDateTime(e.start_time);
    panel_entry->finish_time   = wxDateTime(e.finish_time);
    panel_entry->finish_active = e.finish_active;
    
    panel_entry->daily_repeat_active   = e.daily_repeat_active;
    panel_entry->daily_repeat_interval = e.daily_repeat_interval;
    
    panel_entry->weekly_days = JobSchedulerPanelEntry::InvalidDay;
    panel_entry->weekly_days |= (e.weekly_days & JobSchedulerEntry::Mon) ? JobSchedulerPanelEntry::Mon : JobSchedulerPanelEntry::InvalidDay;
    panel_entry->weekly_days |= (e.weekly_days & JobSchedulerEntry::Tue) ? JobSchedulerPanelEntry::Tue : JobSchedulerPanelEntry::InvalidDay;
    panel_entry->weekly_days |= (e.weekly_days & JobSchedulerEntry::Wed) ? JobSchedulerPanelEntry::Wed : JobSchedulerPanelEntry::InvalidDay;
    panel_entry->weekly_days |= (e.weekly_days & JobSchedulerEntry::Thu) ? JobSchedulerPanelEntry::Thu : JobSchedulerPanelEntry::InvalidDay;
    panel_entry->weekly_days |= (e.weekly_days & JobSchedulerEntry::Fri) ? JobSchedulerPanelEntry::Fri : JobSchedulerPanelEntry::InvalidDay;
    panel_entry->weekly_days |= (e.weekly_days & JobSchedulerEntry::Sat) ? JobSchedulerPanelEntry::Sat : JobSchedulerPanelEntry::InvalidDay;
    panel_entry->weekly_days |= (e.weekly_days & JobSchedulerEntry::Sun) ? JobSchedulerPanelEntry::Sun : JobSchedulerPanelEntry::InvalidDay;

    panel_entry->monthly_daynumber = e.monthly_daynumber;
    
    panel_entry->commands = e.commands;
}

wxString JobSchedulerPanel::createJobDescription(JobSchedulerPanelEntry* entry)
{
    wxString start_timestr, finish_timestr;
    wxString start_datestr, finish_datestr;
    wxString weekly_days;
    wxString monthly_daynumber;
    
    // we can't format the string if we haven't
    // selected any days to run the job
    if (entry->schedule == JobSchedulerPanelEntry::Weekly &&
        entry->weekly_days == JobSchedulerPanelEntry::InvalidDay)
    {
        return wxEmptyString;
    }
    
    // if the job is set to be inactive, don't populate
    // the description with the schedule text below
    if (!entry->active)
    {
        return _("This job is inactive and will not run until activated.");
    }
    
    // format the time strings
    start_timestr = entry->start_time.Format(m_has_ampm ? wxT("%I") : wxT("%H"));
    start_timestr.StartsWith(wxT("0"), &start_timestr);
    start_timestr += entry->start_time.Format(m_has_ampm ? wxT(":%M %p") : wxT(":%M"));
    
    finish_timestr = entry->finish_time.Format(m_has_ampm ? wxT("%I") : wxT("%H"));
    finish_timestr.StartsWith(wxT("0"), &finish_timestr);
    finish_timestr += entry->finish_time.Format(m_has_ampm ? wxT(":%M %p") : wxT(":%M"));
    
    // format the date strings
    wxString nozero_daynumber;
    start_datestr = entry->start_time.Format(wxT("%A, %B "));
    nozero_daynumber = entry->start_time.Format(wxT("%d, %Y"));
    nozero_daynumber.StartsWith(wxT("0"), &nozero_daynumber);
    start_datestr += nozero_daynumber;
    
    finish_datestr = entry->finish_time.Format(wxT("%A, %B "));
    nozero_daynumber = entry->finish_time.Format(wxT("%d, %Y"));
    nozero_daynumber.StartsWith(wxT("0"), &nozero_daynumber);
    finish_datestr += nozero_daynumber;
    
    // for weekly intervals
    weekly_days = wxEmptyString;
    if (entry->weekly_days & JobSchedulerPanelEntry::Mon)
        weekly_days += _("Monday, ");
    if (entry->weekly_days & JobSchedulerPanelEntry::Tue)
        weekly_days += _("Tuesday, ");
    if (entry->weekly_days & JobSchedulerPanelEntry::Wed)
        weekly_days +=_("Wednesday, ");
    if (entry->weekly_days & JobSchedulerPanelEntry::Thu)
        weekly_days += _("Thursday, ");
    if (entry->weekly_days & JobSchedulerPanelEntry::Fri)
        weekly_days += _("Friday, ");
    if (entry->weekly_days & JobSchedulerPanelEntry::Sat)
        weekly_days +=  _("Saturday, ");
    if (entry->weekly_days & JobSchedulerPanelEntry::Sun) 
        weekly_days += _("Sunday, ");
    weekly_days = weekly_days.BeforeLast(wxT(','));
    
    // format the end of the string properly
    if (entry->weekly_days > JobSchedulerPanelEntry::Mon)
    {
        wxString find_str = wxT(", ");
        wxString repl_str = _(" and ");
        
        size_t idx = weekly_days.find_last_of(find_str);
        if (idx != -1)
            weekly_days.replace(idx-1, find_str.Length(), repl_str, 0, repl_str.Length());
    }
    
    // for monthly intervals
    monthly_daynumber = wxString::Format(wxT("%d"), entry->monthly_daynumber);
    
    // create the job description string
    wxString retval;
    switch (entry->schedule)
    {
        case JobSchedulerPanelEntry::Once:
            retval = wxString::Format(_("At %s on %s"), start_timestr.c_str(), start_datestr.c_str());
            break;
        case JobSchedulerPanelEntry::Daily:
            retval = wxString::Format(_("Every day at %s, beginning on %s"), start_timestr.c_str(), start_datestr.c_str());
            break;
        case JobSchedulerPanelEntry::Weekly:
            retval = wxString::Format(_("Every %s at %s, beginning on %s"), weekly_days.c_str(), start_timestr.c_str(), start_datestr.c_str());
            break;
        case JobSchedulerPanelEntry::Monthly:
            retval = wxString::Format(_("On day %s of every month at %s, beginning on %s"), monthly_daynumber.c_str(), start_timestr.c_str(), start_datestr.c_str());
            break;
    }
    
    if (entry->schedule != JobSchedulerPanelEntry::Once && entry->finish_active)
    {
        retval += wxString::Format(_(" and ending at %s on %s"), finish_timestr.c_str(), finish_datestr.c_str());
    }
    
    retval += wxT(".");
    return retval;
}


BEGIN_EVENT_TABLE(JobSchedulerPanel, wxPanel)
    EVT_BUTTON(wxID_OK, JobSchedulerPanel::onOK)
    EVT_BUTTON(wxID_CANCEL, JobSchedulerPanel::onCancel)
    EVT_CHOICE(ID_Frequency_Choice, JobSchedulerPanel::onFrequencySelected)
    EVT_KCLGRID_END_EDIT(JobSchedulerPanel::onGridEndEdit)
    EVT_KCLGRID_CURSOR_MOVE(JobSchedulerPanel::onGridCursorMove)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(JobSchedulerPanel::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(JobSchedulerPanel::onGridPreInvalidAreaInsert)
    EVT_DATE_CHANGED(ID_StartDate_DateCtrl, JobSchedulerPanel::onStartDateChanged)
    EVT_DATE_CHANGED(ID_FinishDate_DateCtrl, JobSchedulerPanel::onFinishDateChanged)
    EVT_SPIN_UP(ID_StartTime_SpinButton, JobSchedulerPanel::onStartTimeSpunUp)
    EVT_SPIN_DOWN(ID_StartTime_SpinButton, JobSchedulerPanel::onStartTimeSpunDown)
    EVT_SPIN_UP(ID_FinishTime_SpinButton, JobSchedulerPanel::onFinishTimeSpunUp)
    EVT_SPIN_DOWN(ID_FinishTime_SpinButton, JobSchedulerPanel::onFinishTimeSpunDown)
    EVT_TEXT(ID_Monthly_DayNumber_SpinCtrl, JobSchedulerPanel::onMonthlyDayNumberText)
    EVT_TEXT(ID_Daily_Repeat_Interval_SpinCtrl, JobSchedulerPanel::onDailyRepeatIntervalText)
    EVT_CHECKBOX(ID_Daily_Repeat_Active_CheckBox, JobSchedulerPanel::onDailyRepeatActiveChecked)
    EVT_CHECKBOX(ID_FinishActive_CheckBox, JobSchedulerPanel::onFinishTimeActiveChecked)
    EVT_CHECKBOX(ID_Weekly_Mon_CheckBox, JobSchedulerPanel::onWeeklyDayChecked)
    EVT_CHECKBOX(ID_Weekly_Tue_CheckBox, JobSchedulerPanel::onWeeklyDayChecked)
    EVT_CHECKBOX(ID_Weekly_Wed_CheckBox, JobSchedulerPanel::onWeeklyDayChecked)
    EVT_CHECKBOX(ID_Weekly_Thu_CheckBox, JobSchedulerPanel::onWeeklyDayChecked)
    EVT_CHECKBOX(ID_Weekly_Fri_CheckBox, JobSchedulerPanel::onWeeklyDayChecked)
    EVT_CHECKBOX(ID_Weekly_Sat_CheckBox, JobSchedulerPanel::onWeeklyDayChecked)
    EVT_CHECKBOX(ID_Weekly_Sun_CheckBox, JobSchedulerPanel::onWeeklyDayChecked)
END_EVENT_TABLE()


JobSchedulerPanel::JobSchedulerPanel()
{
    m_joblist_grid = NULL;
    m_commandlist_grid = NULL;

    m_frequency_choice = NULL;
    
    m_starttime_textctrl = NULL;
    m_starttime_spinbutton = NULL;
    m_starttime_datectrl = NULL;

    m_finishtime_active_checkbox = NULL;
    m_finishtime_textctrl = NULL;
    m_finishtime_spinbutton = NULL;
    m_finishtime_datectrl = NULL;

    m_daily_repeat_active_checkbox = NULL;
    m_daily_repeat_interval_spinctrl = NULL;
    m_weekly_mon_checkbox = NULL;
    m_weekly_tue_checkbox = NULL;
    m_weekly_wed_checkbox = NULL;
    m_weekly_thu_checkbox = NULL;
    m_weekly_fri_checkbox = NULL;
    m_weekly_sat_checkbox = NULL;
    m_weekly_sun_checkbox = NULL;
    m_monthly_daynumber_spinctrl = NULL;

    m_current_job_entry = NULL;
    
    
    // find out if the current locale uses AM and PM
    wxDateTime dt = wxDateTime::Now();
    wxString am_str, pm_str;
    dt.GetAmPmStrings(&am_str, &pm_str);
    m_has_ampm = (am_str.Length() + pm_str.Length() > 0) ? true : false;
}

JobSchedulerPanel::~JobSchedulerPanel()
{

}

bool JobSchedulerPanel::initDoc(IFramePtr frame,
                                IDocumentSitePtr site,
                                wxWindow* doc_site,
                                wxWindow* pane_site)
{
    if (!Create(doc_site,
                -1,
                wxPoint(0,0),
                doc_site->GetClientSize(),
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
    {
        return false;
    }

    site->setCaption(_("Job Scheduler"));
    m_doc_site = site;

    // create job list grid
    m_joblist_grid = new kcl::RowSelectionGrid(this);
    m_joblist_grid->setDragFormat(wxT("jobscheduler_joblist"));
    m_joblist_grid->setAllowInvalidAreaInserts(true);
    m_joblist_grid->setAllowKeyboardInserts(true);
    m_joblist_grid->setAllowDeletes(true);
    
    m_joblist_grid->createModelColumn(colActive, wxEmptyString, kcl::Grid::typeBoolean, 1024, 0);
    m_joblist_grid->createModelColumn(colName, _("Job Name"), kcl::Grid::typeCharacter, 1024, 0);
    m_joblist_grid->createModelColumn(colDescription, _("Schedule"), kcl::Grid::typeCharacter, 1024, 0);
    m_joblist_grid->createDefaultView();
    m_joblist_grid->setRowHeight(45);

    // set cell properties for field type column
    kcl::CellProperties props;
    props.mask = kcl::CellProperties::cpmaskAlignment;
    props.alignment = kcl::Grid::alignRight;
    m_joblist_grid->setModelColumnProperties(colActive, &props);

    props.mask = kcl::CellProperties::cpmaskEditable;
    props.editable = false;
    m_joblist_grid->setModelColumnProperties(colDescription, &props);

    m_joblist_grid->setColumnTextWrapping(colName, kcl::Grid::wrapOn);
    m_joblist_grid->setColumnTextWrapping(colDescription, kcl::Grid::wrapOn);

    // create command list row selection grid
    m_commandlist_grid = new kcl::RowSelectionGrid(this);
    m_commandlist_grid->setOptionState(kcl::Grid::optEdit, false);
    m_commandlist_grid->setDragFormat(wxT("jobscheduler_commandlist"));
    m_commandlist_grid->setAllowDeletes(true);
    m_commandlist_grid->setGreenBarInterval(0);
    m_commandlist_grid->createModelColumn(-1, _("Processes"), kcl::Grid::typeCharacter, 1024, 0);
    m_commandlist_grid->createDefaultView();
    m_commandlist_grid->setColumnProportionalSize(0, 1);
    
    // create a platform standards-compliant OK/Cancel sizer
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_OK));
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the OK/Cancel sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);

    // create horizontal sizer
    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer->Add(m_joblist_grid, 1, wxEXPAND);
    horz_sizer->AddSpacer(8);
    horz_sizer->Add(createVerticalSizer(), 0, wxEXPAND);

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(horz_sizer, 1, wxLEFT | wxRIGHT | wxEXPAND, 8);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(main_sizer);
    Layout();
    
    
    // populate the job list grid with the jobs that
    // are in the application-wide job scheduler
    
    JobScheduler* scheduler = g_app->getJobScheduler();
    std::vector<JobSchedulerEntry> jobs = scheduler->getJobs();
    std::vector<JobSchedulerEntry>::iterator it;

    int row = 0;
    for (it = jobs.begin(); it != jobs.end(); ++it)
    {
        m_joblist_grid->insertRow(row);
    
        JobSchedulerPanelEntry* panel_entry = new JobSchedulerPanelEntry;
        populateJobSchedulerPanelEntry(*it, &panel_entry);

        m_joblist_grid->setRowData(row, (long)panel_entry);
        updateJobList(row, false);

        row++;
    }

    // this function takes care of resetting the job settings
    // area (all controls and layout) to their default state
    resetJobParameters();

    // size columns in the job list grid
    int w = m_joblist_grid->GetClientSize().GetWidth();
    m_joblist_grid->setColumnSize(colActive, 23);
    w -= 23;
    m_joblist_grid->setColumnSize(colName, w/4);
    w -= w/4;
    m_joblist_grid->setColumnSize(colDescription, w);
    m_joblist_grid->refresh(kcl::Grid::refreshAll);

    // size column in the command list grid
    w = m_commandlist_grid->GetClientSize().GetWidth();
    m_commandlist_grid->setColumnSize(0, w);
    m_commandlist_grid->refresh(kcl::Grid::refreshAll);


    // -- set our drop targets --

    kcl::GridDataDropTarget* drop_target1 = new kcl::GridDataDropTarget(m_joblist_grid);
    drop_target1->sigDropped.connect(this, &JobSchedulerPanel::onGridDataDropped);
    drop_target1->setGridDataObjectFormats(wxT("jobscheduler_joblist"));
    m_joblist_grid->SetDropTarget(drop_target1);

    kcl::GridDataObjectComposite* drop_data2;
    drop_data2 = new kcl::GridDataObjectComposite(NULL, wxT("jobscheduler_commandlist"));
    drop_data2->Add(new FsDataObject);
    
    kcl::GridDataDropTarget* drop_target2 = new kcl::GridDataDropTarget(m_commandlist_grid);
    drop_target2->SetDataObject(drop_data2);
    drop_target2->sigDropped.connect(this, &JobSchedulerPanel::onGridDataDropped);
    m_commandlist_grid->SetDropTarget(drop_target2);

    // -- connect row selection grid signals --
    
    m_joblist_grid->sigInsertingRows.connect(this, &JobSchedulerPanel::onJobListInsertingRows);
    m_joblist_grid->sigDeletedRows.connect(this, &JobSchedulerPanel::onJobListDeletedRows);
    m_commandlist_grid->sigDeletedRows.connect(this, &JobSchedulerPanel::onCommandListDeletedRows);

    return true;
}

wxStaticBoxSizer* JobSchedulerPanel::createDailySizer()
{
    m_daily_repeat_active_checkbox = new wxCheckBox(this,
                                    ID_Daily_Repeat_Active_CheckBox,
                                    wxEmptyString);
    m_daily_repeat_interval_spinctrl = new wxSpinCtrl(this,
                                    ID_Daily_Repeat_Interval_SpinCtrl,
                                    wxT("30"),
                                    wxDefaultPosition, 
                                    wxSize(65,21));
    m_daily_repeat_interval_spinctrl->SetRange(1,1440);
    
    // create the horizontal "every 'blank' days" sizer
    wxBoxSizer* daily_repeat_sizer = new wxBoxSizer(wxHORIZONTAL);
    daily_repeat_sizer->Add(m_daily_repeat_active_checkbox, 0, wxALIGN_CENTER);
    daily_repeat_sizer->AddSpacer(5);
    daily_repeat_sizer->Add(new wxStaticText(this, -1, _("Repeat every")), 0, wxALIGN_CENTER);
    daily_repeat_sizer->AddSpacer(5);
    daily_repeat_sizer->Add(m_daily_repeat_interval_spinctrl, 0, wxALIGN_CENTER);
    daily_repeat_sizer->AddSpacer(5);
    daily_repeat_sizer->Add(new wxStaticText(this, -1, _("minute(s)")), 0, wxALIGN_CENTER);

    // create the vertical "daily" sizer
    wxStaticBox* box = new wxStaticBox(this, -1, _("Schedule this job daily"));
    m_daily_sizer = new wxStaticBoxSizer(box, wxVERTICAL);
    m_daily_sizer->AddSpacer(5);
    m_daily_sizer->Add(daily_repeat_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_daily_sizer->AddSpacer(5);
    m_daily_sizer->Layout();
    return m_daily_sizer;
}

wxStaticBoxSizer* JobSchedulerPanel::createWeeklySizer()
{
    m_weekly_mon_checkbox = new wxCheckBox(this, ID_Weekly_Mon_CheckBox, _("Monday"));
    m_weekly_tue_checkbox = new wxCheckBox(this, ID_Weekly_Tue_CheckBox, _("Tuesday"));
    m_weekly_wed_checkbox = new wxCheckBox(this, ID_Weekly_Wed_CheckBox, _("Wednesday"));
    m_weekly_thu_checkbox = new wxCheckBox(this, ID_Weekly_Thu_CheckBox, _("Thursday"));
    m_weekly_fri_checkbox = new wxCheckBox(this, ID_Weekly_Fri_CheckBox, _("Friday"));
    m_weekly_sat_checkbox = new wxCheckBox(this, ID_Weekly_Sat_CheckBox, _("Saturday"));
    m_weekly_sun_checkbox = new wxCheckBox(this, ID_Weekly_Sun_CheckBox, _("Sunday"));
    
    // create the vertical "weekdays" sizer
    wxBoxSizer* weekday_sizer = new wxBoxSizer(wxVERTICAL);
    weekday_sizer->Add(m_weekly_mon_checkbox, 0);
    weekday_sizer->AddSpacer(5);
    weekday_sizer->Add(m_weekly_tue_checkbox, 0);
    weekday_sizer->AddSpacer(5);
    weekday_sizer->Add(m_weekly_wed_checkbox, 0);
    weekday_sizer->AddSpacer(5);
    weekday_sizer->Add(m_weekly_thu_checkbox, 0);
    weekday_sizer->AddSpacer(5);
    weekday_sizer->Add(m_weekly_fri_checkbox, 0);
    
    // create the vertical "weekend" sizer
    wxBoxSizer* weekend_sizer = new wxBoxSizer(wxVERTICAL);
    weekend_sizer->Add(m_weekly_sat_checkbox, 0);
    weekend_sizer->AddSpacer(5);
    weekend_sizer->Add(m_weekly_sun_checkbox, 0);
    
    // create the horizontal "weekly" sizer
    wxStaticBox* box = new wxStaticBox(this, -1, _("Schedule this job weekly"));
    m_weekly_sizer = new wxStaticBoxSizer(box, wxHORIZONTAL);
    m_weekly_sizer->AddSpacer(10);
    m_weekly_sizer->Add(weekday_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 5);
    m_weekly_sizer->AddSpacer(10);
    m_weekly_sizer->Add(weekend_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 5);
    m_weekly_sizer->Layout();
    return m_weekly_sizer;
}

wxStaticBoxSizer* JobSchedulerPanel::createMonthlySizer()
{
    m_monthly_daynumber_spinctrl = new wxSpinCtrl(this,
                                    ID_Monthly_DayNumber_SpinCtrl,
                                    wxT("1"),
                                    wxDefaultPosition,
                                    wxSize(65,21));
    m_monthly_daynumber_spinctrl->SetRange(1,31);

    // create the horizontal "day 'blank' of the month" sizer
    wxBoxSizer* day_of_month_sizer = new wxBoxSizer(wxHORIZONTAL);
    day_of_month_sizer->Add(new wxStaticText(this, -1, _("Day")), 0, wxALIGN_CENTER);
    day_of_month_sizer->AddSpacer(5);
    day_of_month_sizer->Add(m_monthly_daynumber_spinctrl, 0, wxALIGN_CENTER);
    day_of_month_sizer->AddSpacer(5);
    day_of_month_sizer->Add(new wxStaticText(this, -1, _("of the month")), 0, wxALIGN_CENTER);

    // create the vertical "monthly" sizer
    wxStaticBox* box = new wxStaticBox(this, -1, _("Schedule this job monthly"));
    m_monthly_sizer = new wxStaticBoxSizer(box, wxVERTICAL);
    m_monthly_sizer->AddSpacer(5);
    m_monthly_sizer->Add(day_of_month_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_monthly_sizer->AddSpacer(5);
    m_monthly_sizer->Layout();
    return m_monthly_sizer;
}

wxBoxSizer* JobSchedulerPanel::createVerticalSizer()
{
    // -- create frequency sizer --
    wxStaticText* label_frequency = new wxStaticText(this, -1, _("Schedule Job:"));
    m_frequency_choice = new wxChoice(this,
                                    ID_Frequency_Choice,
                                    wxDefaultPosition,
                                    wxSize(100,21),
                                    0,
                                    NULL);
    m_frequency_choice->Append(_("Once"));
    m_frequency_choice->Append(_("Daily"));
    m_frequency_choice->Append(_("Weekly"));
    m_frequency_choice->Append(_("Monthly"));
    m_frequency_choice->SetSelection(ScheduleChoice_Once);

    wxBoxSizer* frequency_sizer = new wxBoxSizer(wxHORIZONTAL);
    frequency_sizer->Add(label_frequency, 0, wxALIGN_CENTER);
    frequency_sizer->AddSpacer(5);
    frequency_sizer->Add(m_frequency_choice, 1, wxEXPAND);
    
    // -- create end sizer --
    wxStaticText* label_finish = new wxStaticText(this, -1, _("End:"));
    m_finishtime_active_checkbox = new wxCheckBox(this,
                                    ID_FinishActive_CheckBox,
                                    wxEmptyString);
    m_finishtime_textctrl = new wxTextCtrl(this,
                                    ID_FinishTime_TextCtrl,
                                    wxT("08:00 AM"),
                                    wxDefaultPosition,
                                    wxSize(65,21));
    m_finishtime_textctrl->SetCursor(*wxSTANDARD_CURSOR);
    m_finishtime_textctrl->SetEditable(false);

    m_finishtime_spinbutton = new wxSpinButton(this,
                                    ID_FinishTime_SpinButton,
                                    wxDefaultPosition,
                                    wxSize(17,21));
    m_finishtime_datectrl = new wxDatePickerCtrl(this, ID_FinishDate_DateCtrl);

    m_finishtime_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_finishtime_sizer->Add(m_finishtime_active_checkbox, 0, wxALIGN_CENTER);
    m_finishtime_sizer->AddSpacer(5);
    m_finishtime_sizer->Add(label_finish, 0, wxALIGN_CENTER);
    m_finishtime_sizer->Add(m_finishtime_textctrl, 0, wxEXPAND);
    m_finishtime_sizer->AddSpacer(1);
    m_finishtime_sizer->Add(m_finishtime_spinbutton, 0, wxEXPAND);
    m_finishtime_sizer->AddSpacer(2);
    m_finishtime_sizer->Add(m_finishtime_datectrl, 1, wxEXPAND);

    // -- create start sizer --
    wxStaticText* label_start = new wxStaticText(this, -1, _("Begin:"));
    m_starttime_textctrl = new wxTextCtrl(this,
                                    ID_StartTime_TextCtrl,
                                    wxT("08:00 AM"),
                                    wxDefaultPosition,
                                    wxSize(65,21));
    m_starttime_textctrl->SetCursor(*wxSTANDARD_CURSOR);
    m_starttime_textctrl->SetEditable(false);

    m_starttime_spinbutton = new wxSpinButton(this,
                                    ID_StartTime_SpinButton,
                                    wxDefaultPosition,
                                    wxSize(17,21));
    m_starttime_datectrl = new wxDatePickerCtrl(this, ID_StartDate_DateCtrl);
    
    // make sure the labels are lined up by adding this space
    // to the beginning of the start time sizer
    int w = m_finishtime_active_checkbox->GetClientSize().GetWidth()+5;
    
    m_starttime_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_starttime_sizer->AddSpacer(w);
    m_starttime_sizer->Add(label_start, 0, wxALIGN_CENTER);
    m_starttime_sizer->Add(m_starttime_textctrl, 0, wxEXPAND);
    m_starttime_sizer->AddSpacer(1);
    m_starttime_sizer->Add(m_starttime_spinbutton, 0, wxEXPAND);
    m_starttime_sizer->AddSpacer(2);
    m_starttime_sizer->Add(m_starttime_datectrl, 1, wxEXPAND);
    
    // measure the label widths
    wxSize label_size = getMaxTextSize(label_start,
                                            label_finish);
    label_size.x += 5;
    m_starttime_sizer->SetItemMinSize(label_start, label_size);
    m_finishtime_sizer->SetItemMinSize(label_finish, label_size);

    // create top sizer
    m_top_sizer = new wxBoxSizer(wxVERTICAL);
    m_top_sizer->Add(frequency_sizer, 0, wxEXPAND);
    m_top_sizer->AddSpacer(8);
    m_top_sizer->Add(m_starttime_sizer, 0, wxEXPAND);
    m_top_sizer->Add(m_finishtime_sizer, 0, wxEXPAND | wxTOP, 8);
    
    wxBoxSizer* daily_sizer = createDailySizer();
    wxBoxSizer* weekly_sizer = createWeeklySizer();
    wxBoxSizer* monthly_sizer = createMonthlySizer();
    
    // -- create the vertical sizer --
    m_vert_sizer = new wxBoxSizer(wxVERTICAL);
    m_vert_sizer->Add(m_top_sizer, 0, wxEXPAND);
    m_vert_sizer->AddSpacer(8);
    m_vert_sizer->Add(daily_sizer, 0, wxEXPAND | wxBOTTOM, 8);
    m_vert_sizer->Add(weekly_sizer, 0, wxEXPAND | wxBOTTOM, 8);
    m_vert_sizer->Add(monthly_sizer, 0, wxEXPAND | wxBOTTOM, 8);
    m_vert_sizer->Add(m_commandlist_grid, 1, wxEXPAND);
    m_vert_sizer->Layout();
    
    // make sure the minimum size of the vertical sizer
    // is equal to the biggest minimum size of its children
    wxSize min_size = m_vert_sizer->GetMinSize();
    min_size.IncTo(daily_sizer->GetMinSize());
    min_size.IncTo(weekly_sizer->GetMinSize());
    min_size.IncTo(monthly_sizer->GetMinSize());
    m_vert_sizer->SetMinSize(min_size);
    
    return m_vert_sizer;
}

wxWindow* JobSchedulerPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void JobSchedulerPanel::setDocumentFocus()
{
}

void JobSchedulerPanel::doLayout()
{
    Freeze();
    int sel = m_frequency_choice->GetSelection();
    
    size_t spacer_idx = 0;
    if (sel == ScheduleChoice_Once)
    {
        wxSizerItem* item = m_starttime_sizer->GetItem(spacer_idx);
        if (item->IsSpacer())
            m_starttime_sizer->Remove(spacer_idx);
    }
     else
    {
        wxSizerItem* item = m_starttime_sizer->GetItem(spacer_idx);
        if (!item->IsSpacer())
        {
            // make sure the labels are lined up by adding this space
            // to the beginning of the start time sizer
            int w = m_finishtime_active_checkbox->GetClientSize().GetWidth()+5;
            m_starttime_sizer->PrependSpacer(w);
        }
    }
    
    m_top_sizer->Show(m_finishtime_sizer, (sel != JobSchedulerPanelEntry::Once), true);
    m_vert_sizer->Show(m_daily_sizer, (sel == JobSchedulerPanelEntry::Daily), true);
    m_vert_sizer->Show(m_weekly_sizer, (sel == JobSchedulerPanelEntry::Weekly), true);
    m_vert_sizer->Show(m_monthly_sizer, (sel == JobSchedulerPanelEntry::Monthly), true);
    m_vert_sizer->Layout();
    Thaw();
}

void JobSchedulerPanel::checkOverlayText()
{
    // -- job list grid --

    if (m_joblist_grid->getRowCount() == 0)
        m_joblist_grid->setOverlayText(_("To add a scheduled job,\ndouble-click here"));
         else
        m_joblist_grid->setOverlayText(wxEmptyString);

    // -- command list grid --

    if (m_current_job_entry == NULL)
    {
        // let's hide the overlay text here until we can figure out
        // why the overlay text in the grid doesn't vertically center
        // itself correctly when the grid is disabled
        //m_commandlist_grid->setOverlayText(_("A job must be selected from the\nlist on the left before any scripts\nor queries can be added here"));
        m_commandlist_grid->setOverlayText(wxEmptyString);
    }
     else
    {
        if (m_commandlist_grid->getRowCount() == 0)
            m_commandlist_grid->setOverlayText(_("Select scripts or queries from the\nProject Panel and drag them here\nto add processes to this job"));
             else
            m_commandlist_grid->setOverlayText(wxEmptyString);
    }
}

void JobSchedulerPanel::populateCommandList(JobSchedulerPanelEntry& entry)
{
    m_commandlist_grid->deleteAllRows();

    DbDoc* dbdoc = g_app->getDbDoc();

    int row = 0;
    std::vector<wxString>::iterator it;
    for (it = entry.commands.begin(); it != entry.commands.end(); ++it)
    {
        IFsItemPtr item;
        if (dbdoc)
            item = dbdoc->getFsItemFromPath(*it);
            
        m_commandlist_grid->insertRow(row);
        if (item.isOk())
            m_commandlist_grid->setCellBitmap(row, 0, item->getBitmap());
         else
            m_commandlist_grid->setCellBitmap(row, 0, GETBMP(xpm_blank_16));
        m_commandlist_grid->setCellString(row, 0, *it);
        row++;
    }
    
    checkOverlayText();
    m_commandlist_grid->refresh(kcl::Grid::refreshAll);
}

void JobSchedulerPanel::updateJobList(int row, bool refresh)
{
    if (row == -1)
        return;
        
    JobSchedulerPanelEntry* entry;
    entry = (JobSchedulerPanelEntry*)m_joblist_grid->getRowData(row);
    
    if (!entry)
        return;
    
    wxString desc = createJobDescription(entry);
    m_joblist_grid->setCellBoolean(row, colActive, entry->active);
    m_joblist_grid->setCellString(row, colName, entry->name);
    m_joblist_grid->setCellString(row, colDescription, desc);

    if (refresh)
        m_joblist_grid->refresh(kcl::Grid::refreshAll);
}

static void enableSizerChildren(wxSizer* sizer, bool enable)
{
    wxSizerItemList item_list = sizer->GetChildren();
    size_t i, count = item_list.GetCount();
    for (i = 0; i < count; ++i)
    {
        wxSizerItem* item = item_list.Item(i)->GetData();
        wxWindow* wnd = item->GetWindow();
        if (wnd != NULL && wnd->IsEnabled() != enable)
            wnd->Enable(enable);
        
        // make sure we recursively enable/disable elements
        // in sizers that are contained in this sizer
        if (item->GetSizer() != NULL)
            enableSizerChildren(item->GetSizer(), enable);
    }
}

void JobSchedulerPanel::updateJobParameters(JobSchedulerPanelEntry& entry)
{
    Freeze();
    
    checkOverlayText();
    m_commandlist_grid->deleteAllRows();
    m_commandlist_grid->refresh(kcl::Grid::refreshAll);
    
    if (m_current_job_entry == NULL)
    {
        enableSizerChildren(m_vert_sizer, false);
        m_commandlist_grid->setVisibleState(kcl::Grid::stateDisabled);
    }
     else
    {
        enableSizerChildren(m_vert_sizer, true);
        m_commandlist_grid->setVisibleState(kcl::Grid::stateVisible);
    }
    
    // this eliminates a nasty flicker in the date controls
    // that is not present in any of the other controls
    m_starttime_datectrl->Freeze();
    m_finishtime_datectrl->Freeze();
    m_commandlist_grid->Freeze();

    switch (entry.schedule)
    {
        default:
        case JobSchedulerPanelEntry::Once:      m_frequency_choice->SetSelection(ScheduleChoice_Once);      break;
        case JobSchedulerPanelEntry::Daily:     m_frequency_choice->SetSelection(ScheduleChoice_Daily);     break;
        case JobSchedulerPanelEntry::Weekly:    m_frequency_choice->SetSelection(ScheduleChoice_Weekly);    break;
        case JobSchedulerPanelEntry::Monthly:   m_frequency_choice->SetSelection(ScheduleChoice_Monthly);   break;
    }
    
    wxString start_timestr = entry.start_time.Format(m_has_ampm ? wxT("%I:%M %p") : wxT("%H:%M"));
    wxString finish_timestr = entry.finish_time.Format(m_has_ampm ? wxT("%I:%M %p") : wxT("%H:%M"));
    m_starttime_textctrl->ChangeValue(start_timestr);
    m_starttime_textctrl->SetInsertionPoint(0);
    m_starttime_datectrl->SetValue(entry.start_time);
    m_finishtime_active_checkbox->SetValue(entry.finish_active);
    m_finishtime_textctrl->ChangeValue(finish_timestr);
    m_finishtime_textctrl->SetInsertionPoint(0);
    m_finishtime_datectrl->SetValue(entry.finish_time);

    m_daily_repeat_active_checkbox->SetValue(entry.daily_repeat_active);
    m_daily_repeat_interval_spinctrl->SetValue(entry.daily_repeat_interval);

    int days = entry.weekly_days;
    m_weekly_mon_checkbox->SetValue((days & JobSchedulerPanelEntry::Mon) ? true : false);
    m_weekly_tue_checkbox->SetValue((days & JobSchedulerPanelEntry::Tue) ? true : false);
    m_weekly_wed_checkbox->SetValue((days & JobSchedulerPanelEntry::Wed) ? true : false);
    m_weekly_thu_checkbox->SetValue((days & JobSchedulerPanelEntry::Thu) ? true : false);
    m_weekly_fri_checkbox->SetValue((days & JobSchedulerPanelEntry::Fri) ? true : false);
    m_weekly_sat_checkbox->SetValue((days & JobSchedulerPanelEntry::Sat) ? true : false);
    m_weekly_sun_checkbox->SetValue((days & JobSchedulerPanelEntry::Sun) ? true : false);

    m_monthly_daynumber_spinctrl->SetValue(entry.monthly_daynumber);
    
    // populate the command list grid for this job
    populateCommandList(entry);

    // enable/disable the finish date and time controls
    wxCommandEvent unused;
    onFinishTimeActiveChecked(unused);
    
    // enable/disable the daily interval control
    onDailyRepeatActiveChecked(unused);
    
    // layout the vertical sizer based on the job type (daily, weekly, etc.)
    doLayout();

    m_starttime_datectrl->Thaw();
    m_finishtime_datectrl->Thaw();
    m_commandlist_grid->Thaw();

    Thaw();
}

void JobSchedulerPanel::resetJobParameters()
{
    // create an empty entry and use the default values from
    // the constructor to reset the job parameters
    JobSchedulerPanelEntry entry;
    updateJobParameters(entry);
}

void JobSchedulerPanel::onFrequencySelected(wxCommandEvent& evt)
{
    doLayout();

    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;

    int sel = m_frequency_choice->GetSelection();
    switch (sel)
    {
        default:
        case ScheduleChoice_Once:       m_current_job_entry->schedule = JobSchedulerPanelEntry::Once;       break;
        case ScheduleChoice_Daily:      m_current_job_entry->schedule = JobSchedulerPanelEntry::Daily;      break;
        case ScheduleChoice_Weekly:     m_current_job_entry->schedule = JobSchedulerPanelEntry::Weekly;     break;
        case ScheduleChoice_Monthly:    m_current_job_entry->schedule = JobSchedulerPanelEntry::Monthly;    break;
    }
    
    updateJobList(row, true);
}

void JobSchedulerPanel::insertScheduledJob(int row)
{
    int row_count = m_joblist_grid->getRowCount();
    if (row > row_count || row == -1)
        row = row_count;
        
    m_joblist_grid->Freeze();
    m_joblist_grid->insertRow(row);
    
    JobSchedulerPanelEntry* entry = new JobSchedulerPanelEntry;
    entry->name = wxString::Format(_("Job #%d"), row_count+1);

    m_joblist_grid->setRowData(row, (long)entry);
    updateJobList(row, false);
    
    // set the job parameter area to the same default values
    // that the new job will be starting with
    resetJobParameters();
    
    checkOverlayText();
    m_joblist_grid->moveCursor(row, colName);
    m_joblist_grid->clearSelection();
    m_joblist_grid->setRowSelected(row, true);
    m_joblist_grid->scrollVertToCursor();
    m_joblist_grid->refresh(kcl::Grid::refreshAll);
    m_joblist_grid->Thaw();
}

void JobSchedulerPanel::onFinishTimeActiveChecked(wxCommandEvent& evt)
{
    bool checked = m_finishtime_active_checkbox->GetValue();
    m_finishtime_textctrl->Enable(checked);
    m_finishtime_spinbutton->Enable(checked);
    m_finishtime_datectrl->Enable(checked);

    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;

    m_current_job_entry->finish_active = checked;
    updateJobList(row, true);
}

void JobSchedulerPanel::onDailyRepeatActiveChecked(wxCommandEvent& evt)
{
    bool checked = m_daily_repeat_active_checkbox->GetValue();
    m_daily_repeat_interval_spinctrl->Enable(checked);
    
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;

    m_current_job_entry->daily_repeat_active = checked;
    updateJobList(row, true);
}

void JobSchedulerPanel::onDailyRepeatIntervalText(wxCommandEvent& evt)
{
    // NOTE: we don't need to change the job description text
    //       when this value is edited
    
    if (!m_current_job_entry)
        return;

    int val = m_daily_repeat_interval_spinctrl->GetValue();
    int max = m_daily_repeat_interval_spinctrl->GetMax();
    if (val > max)
        val = max;
    
    m_current_job_entry->daily_repeat_interval = val;
}

void JobSchedulerPanel::onMonthlyDayNumberText(wxCommandEvent& evt)
{
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;

    int val = m_monthly_daynumber_spinctrl->GetValue();
    int max = m_monthly_daynumber_spinctrl->GetMax();
    if (val > max)
        val = max;
    
    m_current_job_entry->monthly_daynumber = val;
    updateJobList(row, true);
}

void JobSchedulerPanel::onWeeklyDayChecked(wxCommandEvent& evt)
{
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;
    
    bool mon_checked = m_weekly_mon_checkbox->GetValue();
    bool tue_checked = m_weekly_tue_checkbox->GetValue();
    bool wed_checked = m_weekly_wed_checkbox->GetValue();
    bool thu_checked = m_weekly_thu_checkbox->GetValue();
    bool fri_checked = m_weekly_fri_checkbox->GetValue();
    bool sat_checked = m_weekly_sat_checkbox->GetValue();
    bool sun_checked = m_weekly_sun_checkbox->GetValue();
    
    int days = JobSchedulerPanelEntry::InvalidDay;
    days |= (mon_checked) ? JobSchedulerPanelEntry::Mon : JobSchedulerPanelEntry::InvalidDay;
    days |= (tue_checked) ? JobSchedulerPanelEntry::Tue : JobSchedulerPanelEntry::InvalidDay;
    days |= (wed_checked) ? JobSchedulerPanelEntry::Wed : JobSchedulerPanelEntry::InvalidDay;
    days |= (thu_checked) ? JobSchedulerPanelEntry::Thu : JobSchedulerPanelEntry::InvalidDay;
    days |= (fri_checked) ? JobSchedulerPanelEntry::Fri : JobSchedulerPanelEntry::InvalidDay;
    days |= (sat_checked) ? JobSchedulerPanelEntry::Sat : JobSchedulerPanelEntry::InvalidDay;
    days |= (sun_checked) ? JobSchedulerPanelEntry::Sun : JobSchedulerPanelEntry::InvalidDay;
    m_current_job_entry->weekly_days = days;
    
    updateJobList(row, true);
}

void JobSchedulerPanel::onStartDateChanged(wxDateEvent& evt)
{
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;
        
    wxDateTime dt = evt.GetDate();
    m_current_job_entry->start_time.SetDay(dt.GetDay());
    m_current_job_entry->start_time.SetMonth(dt.GetMonth());
    m_current_job_entry->start_time.SetYear(dt.GetYear());
    
    updateJobList(row, true);
}

void JobSchedulerPanel::onFinishDateChanged(wxDateEvent& evt)
{
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;
    
    wxDateTime dt = evt.GetDate();
    m_current_job_entry->finish_time.SetDay(dt.GetDay());
    m_current_job_entry->finish_time.SetMonth(dt.GetMonth());
    m_current_job_entry->finish_time.SetYear(dt.GetYear());
    
    updateJobList(row, true);
}

void JobSchedulerPanel::getTextCtrlTime(wxTextCtrl* ctrl, int* hour, int* min)
{
    wxString str = ctrl->GetValue();
    *hour = wxAtoi(str.SubString(0,2));
    *min  = wxAtoi(str.SubString(3,5));
    
    // get the "AM" and "PM" strings (takes into account different locales)
    wxString am_str, pm_str;
    wxDateTime dt = wxDateTime::Now();
    dt.GetAmPmStrings(&am_str, &pm_str);
    
    // some locales have AM or PM, others don't.  For those
    // that have AM and PM, we need to adjust our numbers
    if (am_str.Length() + pm_str.Length() > 0)
    {
        bool am  = (str.SubString(6,str.Length()).CmpNoCase(am_str) == 0) ? true : false;
        
        // 1:00 PM, 2:00 PM, etc.
        if ((*hour) < 12 && !am)
            (*hour) += 12;
            
        // 12:00 AM
        if ((*hour) == 12 && am)
            (*hour) -= 12;
    }
}

void JobSchedulerPanel::setTextCtrlTime(wxTextCtrl* ctrl, int& hour, int& min, bool inc_val)
{
    wxString str = ctrl->GetValue();
    int idx = ctrl->GetInsertionPoint();
    int start_sel, end_sel;
    
    if (idx >= 0 && idx <= 2)
    {
        if (inc_val)
            hour++;
         else
            hour--;
        
        if (hour >= 24)
            hour = 0;
        if (hour <= -1)
            hour = 23;
            
        start_sel = 0;
        end_sel = 2;
    }
     else if (idx >= 3 && idx <= 5)
    {
        if (inc_val)
            min++;
         else
            min--;

        if (min >= 60)
            min = 0;
        if (min <= -1)
            min = 59;
            
        start_sel = 3;
        end_sel = 5;
    }
     else
    {
        if (hour < 12)
            hour += 12;
         else
            hour -= 12;
            
        start_sel = 6;
        end_sel = str.Length();
    }

    // format the string for the text control
    wxDateTime dt = wxDateTime::Now();
    dt.SetHour(hour);
    dt.SetMinute(min);
        
    wxString timestr;
    if (m_has_ampm)
        timestr = dt.Format(wxT("%I:%M %p"));
         else
        timestr = dt.Format(wxT("%H:%M"));
        
    ctrl->ChangeValue(timestr);
    ctrl->SetSelection(start_sel, end_sel);
}

void JobSchedulerPanel::onStartTimeSpunUp(wxSpinEvent& evt)
{
    int hour, min;
    getTextCtrlTime(m_starttime_textctrl, &hour, &min);
    setTextCtrlTime(m_starttime_textctrl, hour, min, true);
    
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;

    m_current_job_entry->start_time.SetHour(hour);
    m_current_job_entry->start_time.SetMinute(min);
    
    updateJobList(row, true);
}

void JobSchedulerPanel::onStartTimeSpunDown(wxSpinEvent& evt)
{
    int hour, min;
    getTextCtrlTime(m_starttime_textctrl, &hour, &min);
    setTextCtrlTime(m_starttime_textctrl, hour, min, false);
    
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;

    m_current_job_entry->start_time.SetHour(hour);
    m_current_job_entry->start_time.SetMinute(min);
    
    updateJobList(row, true);
}

void JobSchedulerPanel::onFinishTimeSpunUp(wxSpinEvent& evt)
{
    int hour, min;
    getTextCtrlTime(m_finishtime_textctrl, &hour, &min);
    setTextCtrlTime(m_finishtime_textctrl, hour, min, true);
    
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;

    m_current_job_entry->finish_time.SetHour(hour);
    m_current_job_entry->finish_time.SetMinute(min);
    
    updateJobList(row, true);
}

void JobSchedulerPanel::onFinishTimeSpunDown(wxSpinEvent& evt)
{
    int hour, min;
    getTextCtrlTime(m_finishtime_textctrl, &hour, &min);
    setTextCtrlTime(m_finishtime_textctrl, hour, min, false);
    
    int row = m_joblist_grid->getCursorRow();
    if (!m_current_job_entry)
        return;

    m_current_job_entry->finish_time.SetHour(hour);
    m_current_job_entry->finish_time.SetMinute(min);
    
    updateJobList(row, true);
}

void JobSchedulerPanel::onGridEndEdit(kcl::GridEvent& evt)
{
    if (evt.GetEventObject() != m_joblist_grid)
        return;

    int col = evt.GetColumn();
    int row = evt.GetRow();
    
    JobSchedulerPanelEntry* entry;
    entry = (JobSchedulerPanelEntry*)m_joblist_grid->getRowData(row);

    switch (col)
    {
        case colActive:  entry->active = evt.GetBoolean();   break;
        case colName:    entry->name   = evt.GetString();    break;
    }
    
    updateJobList(row, true);
}

void JobSchedulerPanel::onGridCursorMove(kcl::GridEvent& evt)
{
    if (evt.GetEventObject() != m_joblist_grid)
        return;

    int row = m_joblist_grid->getCursorRow();
    
    int row_count = m_joblist_grid->getRowCount();
    if (row < 0 || row >= row_count || row_count == 0)
    {
        m_current_job_entry = NULL;
        resetJobParameters();
        checkOverlayText();
        m_commandlist_grid->refresh(kcl::Grid::refreshAll);
        return;
    }
    
    JobSchedulerPanelEntry* old_entry = m_current_job_entry;
    JobSchedulerPanelEntry* entry;
    entry = (JobSchedulerPanelEntry*)m_joblist_grid->getRowData(row);
    m_current_job_entry = entry;
    
    if (entry != old_entry)
        updateJobParameters(*m_current_job_entry);
}

void JobSchedulerPanel::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();

    if (evt.GetEventObject() != m_joblist_grid)
        return;

    // make sure we're not editing the grid
    if (m_joblist_grid->isEditing())
        m_joblist_grid->endEdit(true);

    // add a scheduled job to the end of the job list grid
    insertScheduledJob(-1);
}

void JobSchedulerPanel::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();

    if (evt.GetEventObject() != m_joblist_grid)
        return;

    // make sure we're not editing the grid
    if (m_joblist_grid->isEditing())
        m_joblist_grid->endEdit(true);

    // add a scheduled job to the end of the job list grid
    insertScheduledJob(-1);
}

void JobSchedulerPanel::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    wxDataObject* obj = drop_target->GetDataObject();
    if (!obj)
        return;

    // determine which type of data object was dropped
    kcl::GridDataObjectComposite* drop_data = (kcl::GridDataObjectComposite*)obj;
    wxDataFormat fmt = drop_data->GetReceivedFormat();

    if (drop_data->isGridData())
    {
        // do the row drag in the job list
        if (fmt == wxDataFormat(kcl::getGridDataFormat(wxT("jobscheduler_joblist"))))
        {
            drop_target->doRowDrag(true);
            return;
        }

        // if we haven't selected a job, we can't
        // reorder commands into the command list
        if (!m_current_job_entry)
            return;

        // do the row drag in the command list
        if (fmt == wxDataFormat(kcl::getGridDataFormat(wxT("jobscheduler_commandlist"))))
        {
            drop_target->doRowDrag(true);
            
            // since we've reordered the commands into the command list grid,
            // we need to update the current job's command list vector
            m_current_job_entry->commands.clear();
            int i, count = m_commandlist_grid->getRowCount();
            for (i = 0; i < count; ++i)
            {
                wxString command = m_commandlist_grid->getCellString(i, 0);
                m_current_job_entry->commands.push_back(command);
            }
            
            return;
        }

        return;
    }

    // -- we're dragging from the project panel --
    
    
    // only accept tree data objects here
    if (fmt.GetId().CmpNoCase(FS_DATA_OBJECT_FORMAT) != 0)
        return;

    // get the row number where we dropped the data
    int drop_row = drop_target->getDropRow();
    
    // get the data out of the composite data object
    size_t len = drop_data->GetDataSize(fmt);
    unsigned char* data = new unsigned char[len];
    drop_data->GetDataHere(fmt, data);
    
    // copy the data from the wxDataObjectComposite to this new
    // FsDataObject so we can use it's accessor functions
    FsDataObject* fs_data_obj = new FsDataObject;
    fs_data_obj->SetData(fmt, len, data);



    // if we haven't selected a job, we can't
    // drag commands into the command list
    if (!m_current_job_entry)
        return;
        

    IFsItemEnumPtr items = fs_data_obj->getFsItems();

    std::vector<wxString>::iterator it;
    std::vector<wxString> res;
    DbDoc::getFsItemPaths(items, res, true);

    DbDoc* dbdoc = g_app->getDbDoc();

    int i, count = items->size();
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);

        if (dbdoc->isFsItemExternal(item))
        {
            appMessageBox(_("One or more of the items dragged from the project panel is an external object and cannot be added to the command list."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            delete[] data;
            delete fs_data_obj;
            
            return;
        }

        bool bad = false;

        IDbFolderFsItemPtr folder = item;
        IDbObjectFsItemPtr obj = item;
        if (folder)
        {
            bad = true;
        }

        if (!bad)
        {
            if (!obj)
            {
                bad = true;
            }
             else
            {
                int type = obj->getType();

                if (!(type == dbobjtypeTemplate || type == dbobjtypeScript))
                {
                    bad = true;
                }
            }
        }
            
        if (bad)
        {
            appMessageBox(_("One or more of the items dragged from the project panel is not a batchable command.  Only scripts, queries and job templates can been added to a scheduled job."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
    }

    // using res[idx] below should be OK because there should be a 1 to 1
    // mapping between them items vector and the paths vector
    int insert_row = drop_row;
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);

        m_commandlist_grid->insertRow(insert_row);
        if (item.isOk())
            m_commandlist_grid->setCellBitmap(insert_row, 0, item->getBitmap());
         else
            m_commandlist_grid->setCellBitmap(insert_row, 0, GETBMP(xpm_blank_16));
        m_commandlist_grid->setCellString(insert_row, 0, res[i]);
        insert_row++;
    }
    
    checkOverlayText();
    m_commandlist_grid->refresh(kcl::Grid::refreshAll);


    // since we've dropped some commands into the command list grid,
    // we need to update the current job's command list vector
    m_current_job_entry->commands.clear();
    count = m_commandlist_grid->getRowCount();
    for (i = 0; i < count; ++i)
    {
        wxString command = m_commandlist_grid->getCellString(i, 0);
        m_current_job_entry->commands.push_back(command);
    }
    

    delete[] data;
    delete fs_data_obj;
}

void JobSchedulerPanel::onJobListInsertingRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us

    int cursor_row = m_joblist_grid->getCursorRow();
    
    if (rows.size() > 1)
    {
        m_joblist_grid->clearSelection();
        m_joblist_grid->setRowSelected(cursor_row, true);
    }
    
    insertScheduledJob(cursor_row);
}

void JobSchedulerPanel::onJobListDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us

    int row = m_joblist_grid->getCursorRow();
    int row_count = m_joblist_grid->getRowCount();
    if (row < 0 || row >= row_count || row_count == 0)
    {
        m_current_job_entry = NULL;
        resetJobParameters();
        checkOverlayText();
        m_commandlist_grid->refresh(kcl::Grid::refreshAll);
        return;
    }
    
    JobSchedulerPanelEntry* entry;
    entry = (JobSchedulerPanelEntry*)m_joblist_grid->getRowData(row);
    m_current_job_entry = entry;
    
    updateJobParameters(*m_current_job_entry);

    checkOverlayText();
    m_commandlist_grid->refresh(kcl::Grid::refreshAll);
}

void JobSchedulerPanel::onCommandListDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us

    checkOverlayText();
    m_commandlist_grid->refresh(kcl::Grid::refreshAll);

    if (!m_current_job_entry)
        return;
    
    std::vector<wxString>::iterator entry_it;
    
    int row;
    std::vector<int>::reverse_iterator rit;
    for (rit = rows.rbegin(); rit != rows.rend(); ++rit)
    {
        row = (*rit);
        entry_it = m_current_job_entry->commands.begin()+row;
        m_current_job_entry->commands.erase(entry_it);
    
    }
}

void JobSchedulerPanel::onOK(wxCommandEvent& evt)
{
    // save the panel's scheduled jobs back
    // to the application-wide job scheduler
    
    JobScheduler* scheduler = g_app->getJobScheduler();
    scheduler->clearJobs();
    
    int row, row_count = m_joblist_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        JobSchedulerPanelEntry* panel_entry;
        panel_entry = (JobSchedulerPanelEntry*)m_joblist_grid->getRowData(row);

        JobSchedulerEntry e;
        populateJobSchedulerEntry(panel_entry, e);
        scheduler->addJob(e);
    }

    scheduler->save();

    // if there are any scheduled jobs, show a job icon in the statusbar
    IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
    bool show = false;
    
    /*
    // REMOVED ON 12/06/2007: Too many items being added to the statusbar;
    //                         we can add it back later, if we want
    
    IStatusBarItemPtr job_schedule_item;
    job_schedule_item = statusbar->getItem(wxT("app_job_schedule"));
    if (job_schedule_item.isOk())
    {
        std::vector<JobSchedulerEntry> scheduled_jobs;
        scheduled_jobs = g_app->getJobScheduler()->getJobs();
        std::vector<JobSchedulerEntry>::iterator jit;
        for (jit = scheduled_jobs.begin(); jit != scheduled_jobs.end(); ++jit)
        {
            if (jit->active)
            {
                show = true;
                break;
            }
        }
    }
    
    if ((job_schedule_item->isShown() && !show) ||
        (!job_schedule_item->isShown() && show))
    {
        job_schedule_item->show(show);

        // populate the statusbar
        g_app->getMainFrame()->getStatusBar()->populate();
    }
    */
    
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void JobSchedulerPanel::onCancel(wxCommandEvent& evt)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}




