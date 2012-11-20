/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2008-01-07
 *
 */


#ifndef __APP_APPPRINT_H
#define __APP_APPPRINT_H


#include <wx/timer.h>
#include <wx/print.h>
#include <wx/printdlg.h>


// application printer class; used to print pages from
// a wxPrintout-derived class at regular intervals using
// a timer; this allows the application to continue to
// function as well as provide printer feedback using
// the job queue
class AppPrinter : wxTimer
{
public:

    AppPrinter(wxPrintDialogData* printdata = NULL);
    ~AppPrinter();

    void PrintDialog(wxWindow* parent, wxPrintDialogData* printdata = NULL);
    bool Print(wxPrintout* printout);

public:

    // overridden wxTimer function to print pages at
    // specified intervals so main event loop has 
    // a chance to process other events
    void Notify();

private:

    void Destroy();

private:

    wxPrintDialogData m_printdata;
    wxPrinterError m_printerror;
    wxPrintout* m_printout;
    wxDC* m_dc;
    
    int m_min_page;
    int m_max_page;
    int m_start_page;
    int m_end_page;
    int m_number_copies;
    bool m_collate;
    
    int m_current_page;
    int m_current_copy;
    
    bool m_printing;
};


// application printer progress listener; used to patch 
// through print job info to the job queue; note: parallels 
// DownloadProgressListener in WebDoc
class AppPrintProgressListener
{
public:

    AppPrintProgressListener(int max_page_count = -1)
    {
        m_max_page_count = max_page_count;
        m_job_id = -1;
        m_cancelled = false;
        m_failed = false;
        
        m_job_info = createJobInfoObject();
        m_job_info->setTitle(wxT("Print"));
        m_job_info->setInfoMask(jobMaskTitle |
                            jobMaskStartTime |
                            jobMaskFinishTime |
                            jobMaskPercentage |
                            jobMaskProgressString |
                            jobMaskProgressBar |
                            jobMaskCurrentCount);
    }

    void Init(const wxString& url,
              const wxString& suggested_filename)
    {
        m_url = url;
        m_filename = suggested_filename;
        
        wxString filename = m_filename.AfterLast(PATH_SEPARATOR_CHAR);
        wxString title = wxString::Format(_("Printing '%s'"), filename.c_str());
        m_job_info->setTitle(towstr(title));
        if (m_max_page_count > 1)
        {
            m_job_info->setProgressStringFormat(
                                        towstr(_("$c pages printed")),
                                        towstr(_("$c of $m pages printed ($p1%)")));
        }
         else
        {
            m_job_info->setProgressStringFormat(
                                        towstr(_("$c page printed")),
                                        towstr(_("$c of $m pages printed ($p1%)")));
        }
    }

    void OnStart()
    {
        m_job_info->setStartTime(time(NULL));
        if (m_max_page_count != -1)
            m_job_info->setMaxCount(m_max_page_count);
        m_job_id = g_app->getJobQueue()->addJobInfo(m_job_info, jobStateRunning);
    }

    void OnFinish()
    {
        // set job to finished
        if (!IsCancelled() && !IsFailed())
        {
            m_job_info->setFinishTime(time(NULL));
            m_job_info->setState(jobStateFinished);
        }

        delete this;
    }
    
    void OnError(const wxString& message)
    {
        m_job_info->setFinishTime(time(NULL));
        m_job_info->setState(jobStateFailed);
    }
    
    void OnProgressChange(wxLongLong cur_progress,
                          wxLongLong max_progress)
    {
        m_job_info->setCurrentCount(cur_progress.ToDouble());
        
        if (m_job_info->getState() == jobStateCancelling ||
            m_job_info->getState() == jobStateCancelled)
        {
            m_job_info->setFinishTime(time(NULL));
            m_job_info->setState(jobStateCancelled);
            
            // set the cancelled flag
            m_cancelled = true;
        }
    }
    
    bool IsCancelled()
    {
        return m_cancelled;
    }
    
    bool IsFailed()
    {
        return m_failed;
    }
    
private:

    wxString m_url;
    wxString m_filename;
    IJobInfoPtr m_job_info;
    int m_job_id;
    int m_max_page_count;
    bool m_cancelled;
    bool m_failed;
};


#endif

