/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-08-17
 *
 */


#include "appmain.h"
#include "paneljobmgr.h"
#include "appcontroller.h"
#include <kl/thread.h>
#include <set>


enum
{
    ID_CancelButton = wxID_HIGHEST + 1,
    ID_DoJobQueueRefresh = 413,
    ID_CleanupButton = 414
};


const int SCROLL_STEP = 10;


class JobListCtrlTimer : public wxTimer
{
public:
    
    JobListCtrl* m_owner;

    void Notify()
    {
        m_owner->refreshItems();
        m_owner->checkOverlayText();
        m_owner->refresh();
    }
};



#ifdef _DEBUG
void checkIfInMainThread()
{
    if (!::wxIsMainThread())
    {
        wxFAIL_MSG(wxT("JobListCtrl method being called from a non-gui thread!  Please fix."));
        return;
    }
}
#else
#define checkIfInMainThread()
#endif




    
BEGIN_EVENT_TABLE(JobListCtrl, kcl::ScrollListControl)
    EVT_BUTTON(ID_CancelButton, JobListCtrl::onCancelButtonClicked)
    EVT_MENU(ID_DoJobQueueRefresh, JobListCtrl::onDoJobQueueRefresh)
END_EVENT_TABLE()



static wxString getJobElapsedTimeString(jobs::IJobInfoPtr job_info)
{
    int job_state = job_info->getState();
    if (job_state == jobs::jobStatePaused)
        return _("Paused");
         else if (job_state == jobs::jobStateQueued)
        return _("Queued");

    // calcuate the duration of the job
    time_t start_time = job_info->getStartTime();
    time_t finish_time = job_info->getFinishTime();
    time_t duration, current_time = time(NULL);
    
    if (job_state == jobs::jobStateCancelled ||
        job_state == jobs::jobStateFinished ||
        job_state == jobs::jobStateFailed)
    {
        time_t finish_time = job_info->getFinishTime();
        duration = finish_time - start_time;
    }
     else if (job_state == jobs::jobStateQueued)
    {
        duration = 0;
    }
     else
    {
        duration = current_time - start_time;
    }
    
    // create the elapsed time string
    int hours = (duration/3600);
    duration -= (hours*3600);
    int minutes = (duration/60);
    int seconds = duration%60;
    
    // make sure there is at least a 1 second duration
    if (hours == 0 && minutes == 0 && seconds == 0)
        seconds = 1;
        
    wxString time_str = wxString::Format(_("%02d:%02d:%02d elapsed"),
                                         hours, minutes, seconds);
    
    // create the finish date and time string
    wxString finish_time_str, finish_date_str;
    if (finish_time > 0)
    {
        wxDateTime now = wxDateTime(finish_time);
        finish_date_str = now.Format(wxT("%A, %B %d, %Y"));
        finish_time_str = now.Format(wxT("%X"));
    }
    
    switch (job_state)
    {
        case jobs::jobStateRunning:
            return wxString::Format(_("Running (%s)"), time_str.c_str());

        case jobs::jobStateFinished:
            return wxString::Format(_("Finished on %s at %s (%s)"),
                                    finish_date_str.c_str(),
                                    finish_time_str.c_str(),
                                    time_str.c_str());

        case jobs::jobStateCancelling:
            return wxString::Format(_("Cancelling (%s)"), time_str.c_str());

        case jobs::jobStateCancelled:
            return wxString::Format(_("Cancelled on %s at %s (%s)"),
                                    finish_date_str.c_str(),
                                    finish_time_str.c_str(),
                                    time_str.c_str());
        case jobs::jobStateFailed:
            return wxString::Format(_("Failed on %s at %s (%s)"),
                                    finish_date_str.c_str(),
                                    finish_time_str.c_str(),
                                    time_str.c_str());
    }

    return kl::formattedNumber(job_info->getPercentage(), 1) + wxT("%");
}


JobListCtrl::JobListCtrl(wxWindow* parent) : kcl::ScrollListControl(parent)
{
    checkIfInMainThread();

    // connect job queue update signal
    m_job_queue = g_app->getJobQueue();
    m_job_queue->sigQueueChanged().connect(this, &JobListCtrl::onJobQueueChanged);

    // start an update timer
    m_timer = new JobListCtrlTimer;
    m_timer->m_owner = this;
    m_timer->Start(100);
    
    populate();
    refreshItems();
    checkOverlayText();
}

JobListCtrl::~JobListCtrl()
{
    checkIfInMainThread();

    if (m_timer)
    {
        delete m_timer;
        m_timer = NULL;
    }
}

void JobListCtrl::addJobItem(jobs::IJobInfoPtr job_info)
{
    checkIfInMainThread();

    wxPoint create_pt(-10000,-10000);
    
    kcl::ScrollListItem* item = new kcl::ScrollListItem;
    
    // create bitmap element
    kcl::ScrollListElement* bitmap;
    bitmap = item->addElement(GETBMP(gf_gear_32), wxPoint(15,15));
    bitmap->setPadding(0,0,0,15);
    bitmap->setName(wxT("bitmap"));

    // create title text element
    kcl::ScrollListElement* title;
    title = item->addElement(job_info->getTitle(), wxPoint(60,15));
    title->setPadding(0,0,15,8);
    title->setTextBold(true);
    title->setTextWrap(false);
    title->setName(wxT("title"));
    
    // create progress gauge element
    wxGauge* progress_gauge = new wxGauge(this,
                                          -1,
                                          100,
                                          create_pt,
                                          wxSize(200,16));

    // the job's finish time is indeterminate
    if (kl::dblcompare(job_info->getMaxCount(), 0.0) == 0)
        progress_gauge->Pulse();

    kcl::ScrollListElement* gauge;
    gauge = item->addElement(progress_gauge);
    gauge->setPadding(0,0,15,8);
    gauge->setRelativePosition(title, kcl::ScrollListElement::positionBelow);
    gauge->setStretchable(true);
    gauge->setName(wxT("progress_gauge"));

    // create cancel button element
    wxButton* cancel_button = new wxButton(this,
                                           ID_CancelButton,
                                           _("Cancel"),
                                           create_pt,
                                           wxDefaultSize,
                                           wxBU_EXACTFIT);
    cancel_button->SetExtraStyle((long)item);
    
    kcl::ScrollListElement* cancel;
    cancel = item->addElement(cancel_button);
    cancel->setPadding(0,0,15,15);
    cancel->setRelativePosition(gauge,
                                kcl::ScrollListElement::positionBelow,
                                kcl::ScrollListElement::alignRight);
    cancel->setName(wxT("cancel_button"));

    // create record count text element
    kcl::ScrollListElement* record_count;
    record_count = item->addElement(job_info->getProgressString());
    record_count->setPadding(0,0,0,5);
    record_count->setRelativePosition(gauge,
                                      kcl::ScrollListElement::positionBelow);
    record_count->setName(wxT("record_count_text"));
    
    // create description text element
    kcl::ScrollListElement* job_state;
    job_state = item->addElement(wxEmptyString);
    job_state->setPadding(0,0,0,15);
    job_state->setRelativePosition(record_count,
                                     kcl::ScrollListElement::positionBelow);
    job_state->setStretchable(true);
    job_state->setName(wxT("job_state_text"));
    
    // assign the job id to the item
    item->setExtraLong(job_info->getJobId());
    addItem(item);
}

void JobListCtrl::updateJobItem(kcl::ScrollListItem* item)
{
    checkIfInMainThread();

    int job_id = item->getExtraLong();
    jobs::IJobInfoPtr job_info = m_job_queue->getJobInfo(job_id);
    int job_state = job_info->getState();
    
    kcl::ScrollListElement* element;
    
    // update title
    element = item->getElement(wxT("title"));
    element->setText(job_info->getTitle());
    
    // update bitmap
    element = item->getElement(wxT("bitmap"));
    if (job_state == jobs::jobStateFinished)
        element->setBitmap(GETBMP(gf_checkmark_32));
         else if (job_state == jobs::jobStateCancelled)
        element->setBitmap(GETBMP(gf_x_32));
         else if (job_state == jobs::jobStateFailed)
        element->setBitmap(GETBMP(gf_exclamation_32));
    
    // update progress gauge
    if (job_state == jobs::jobStateRunning)
    {
        double pct = job_info->getPercentage();
        bool is_indeterminate = false;
        
        if (kl::dblcompare(pct, 0.0) == 0)
        {
            is_indeterminate = true;
        }
         else
        {
            time_t start_time = job_info->getStartTime();
            time_t current_time = time(NULL);
            
            // the job has been running for more than 3 seconds and
            // is still stuck at 0%, so it's indeterminate right now
            if ((current_time - start_time) >= 3 && pct == 0.0)
                is_indeterminate = true;
        }
        
        if (is_indeterminate)
        {
            // the job's progress is indeterminate
            element = item->getElement(wxT("progress_gauge"));
            wxGauge* g = (wxGauge*)element->getControl();
            g->Pulse();
        }
         else
        {
            element = item->getElement(wxT("progress_gauge"));
            wxGauge* g = (wxGauge*)element->getControl();
            g->SetValue((int)pct);
        }
    }
    
    // update job state and elapsed time
    element = item->getElement(wxT("job_state_text"));
    element->setText(getJobElapsedTimeString(job_info));
    
    // update record count
    element = item->getElement(wxT("record_count_text"));
    element->setText(job_info->getProgressString());
}

void JobListCtrl::refreshItems()
{
    checkIfInMainThread();
    
    std::set<int> processed;
    int job_id;
    
    // first process jobs already in the job list control
    std::vector<kcl::ScrollListItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        if (!(*it)->getUpdated())
            continue;
            
        job_id = (*it)->getExtraLong();
        
        // try to locate the job info in the job queue
        jobs::IJobInfoPtr info;
        info = m_job_queue->getJobInfo(job_id);
        if (!info)
            continue;

        updateJobItem(*it);
        
        int state = info->getState();
        if (state == jobs::jobStateFinished ||
            state == jobs::jobStateCancelled ||
            state == jobs::jobStateFailed)
        {
            // make job list not update this job anymore
            (*it)->setUpdated(false);
            
            // hide the progress gauge and cancel button 
            kcl::ScrollListElement* element;
            element = (*it)->getElement(wxT("progress_gauge"));
            element->setVisible(false);
            element = (*it)->getElement(wxT("cancel_button"));
            element->setVisible(false);
        }

        // don't look for this job below
        processed.insert(job_id);
    }

    // now look for new jobs
    jobs::IJobInfoEnumPtr jobs = m_job_queue->getJobInfoEnum(jobs::jobStateAll);
    bool item_count_changed = false;
    int job_count = jobs->size();
    int i;

    for (i = 0; i < job_count; i++)
    {
        jobs::IJobInfoPtr job_info = jobs->getItem(i);

        job_id = job_info->getJobId();
        int state = job_info->getState();

        if (state == jobs::jobStateFinished ||
            state == jobs::jobStateCancelled ||
            state == jobs::jobStateFailed)
        {
            continue;
        }
        
        // this job has already been added
        if (processed.find(job_id) != processed.end())
            continue;

        addJobItem(job_info);
        item_count_changed = true;
    }

    if (item_count_changed)
    {
        // calculate the virtual height for the control
        calcVirtualHeight();
    }
}

void JobListCtrl::clearInactiveJobs()
{
    checkIfInMainThread();

    jobs::IJobInfoEnumPtr jobs = m_job_queue->getJobInfoEnum(jobs::jobStateAll);
    jobs::IJobInfoPtr job_info;
    int job_count = jobs->size();
    int i;

    for (i = 0; i < job_count; i++)
    {
        // for all jobs that are done (meaning cancelled, finished, etc.),
        // set the job info's visible flag to false, since we'll use that
        // to determine whether to create a kcl::ScrollList item or not
        jobs::IJobInfoPtr job_info = jobs->getItem(i);
        int state = job_info->getState();
        
        if (state == jobs::jobStateFinished ||
            state == jobs::jobStateCancelling ||
            state == jobs::jobStateCancelled ||
            state == jobs::jobStateFailed)
        {
            job_info->setVisible(false);
        }
    }
    
    // populate the job info vector from the job queue
    populate();
    refreshItems();
    checkOverlayText();
    refresh();
}

void JobListCtrl::checkOverlayText()
{
    // TODO: add overlay text here if we want it for the job list
    if (m_items.size() == 0)
        setOverlayText(wxEmptyString);
         else
        setOverlayText(wxEmptyString);
}

void JobListCtrl::populate()
{
    checkIfInMainThread();

    jobs::IJobInfoEnumPtr jobs = m_job_queue->getJobInfoEnum(jobs::jobStateAll);
    jobs::IJobInfoPtr job_info;
    size_t i, job_count = jobs->size();

    // clear out the job items vector
    clearItems();
    
    // populate the job info vector from the job queue
    for (i = 0; i < job_count; i++)
    {
        // only populate the control with visible job infos
        jobs::IJobInfoPtr job_info = jobs->getItem(i);
        if (!job_info->getVisible())
            continue;

        addJobItem(job_info);
    }
    
    // calculate the virtual height for the control
    calcVirtualHeight();

    // scroll to the top of the job list
    Scroll(0,0);
}

void JobListCtrl::onCancelButtonClicked(wxCommandEvent& evt)
{
    checkIfInMainThread();

    wxButton* button = (wxButton*)(evt.GetEventObject());
    kcl::ScrollListItem* item = (kcl::ScrollListItem*)(button->GetExtraStyle());

    if (!item)
    {
        // something is not right, we still have a cancel button,
        // but we can't find the job list item, so bail out
        return;
    }

    int job_id = item->getExtraLong();
    jobs::IJobPtr job = m_job_queue->lookupJob(job_id);

    if (job)
    {
        jobs::IJobInfoPtr info = job->getJobInfo();
        if (info.isOk())
        {
            info->setFinishTime(time(NULL));
            info.clear();
        }

        job->cancel();
        m_job_queue->sigQueueChanged();
    }
     else
    {
        jobs::IJobInfoPtr job_info = m_job_queue->getJobInfo(job_id);
        if (job_info)
        {
            job_info->setFinishTime(time(NULL));
            job_info->setState(jobs::jobStateCancelling);
        }
    }
}


void JobListCtrl::onDoJobQueueRefresh(wxCommandEvent&)
{
    // when threads fire something that changes the job queue, the
    // queue gui component gets refreshed, and this needs to happen
    // in the gui/main thread.
    
    wxASSERT(kl::Thread::isMain());
    
    refreshItems();
    checkOverlayText();
    refresh();
}

void JobListCtrl::onJobQueueChanged()
{
    if (kl::Thread::isMain())
    {
        refreshItems();
        checkOverlayText();
        refresh();
    }
     else
    {
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, ID_DoJobQueueRefresh);
        ::wxPostEvent(this, e);
    }
}


    
    
BEGIN_EVENT_TABLE(JobManagerPanel, wxPanel)
    EVT_BUTTON(ID_CleanupButton, JobManagerPanel::onCleanupButtonClicked)
END_EVENT_TABLE()


JobManagerPanel::JobManagerPanel()
{
    checkIfInMainThread();

    m_doc_site = xcm::null;
}

JobManagerPanel::~JobManagerPanel()
{

}


bool JobManagerPanel::initDoc(IFramePtr frame,
                              IDocumentSitePtr site,
                              wxWindow* docsite_wnd,
                              wxWindow* panesite_wnd)
{
    checkIfInMainThread();

    if (!Create(docsite_wnd,
                -1,
                wxPoint(0,0),
                docsite_wnd->GetClientSize(),
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
    {
        return false;
    }

    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);

    m_doc_site = site;
    m_doc_site->setMinSize(200,150);
    m_doc_site->setCaption(_("Jobs"));

    // create controls for the panel
    m_job_list = new JobListCtrl(this);
    m_cleanup_button = new wxButton(this, ID_CleanupButton, _("Clean Up"));
    
    // this button is created so that the ESC key can be used to
    // close the panel -- it is completely hidden to the user
    wxButton* close_button;
    close_button = new wxButton(this, wxID_CANCEL, wxEmptyString, wxDefaultPosition, wxSize(0,0));
    
    // create the button sizer
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->Add(m_cleanup_button, 0, wxALIGN_CENTER);
    button_sizer->Add(close_button, 0, wxALIGN_CENTER);
    
    // create the main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_job_list, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);
    
    SetSizer(main_sizer);
    Layout();
    return true;
}

wxWindow* JobManagerPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void JobManagerPanel::setDocumentFocus()
{

}

void JobManagerPanel::onCleanupButtonClicked(wxCommandEvent& event)
{
    checkIfInMainThread();

    m_job_list->clearInactiveJobs();
}


