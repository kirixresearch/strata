/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-08-17
 *
 */


#ifndef H_APP_PANELJOBMGR_H
#define H_APP_PANELJOBMGR_H


class JobListCtrlTimer;
class JobListCtrl : public kcl::ScrollListControl
{
public:

    JobListCtrl(wxWindow* parent);
    ~JobListCtrl();
    
    void refreshItems();
    void checkOverlayText();
    void clearInactiveJobs();
    
private:

    void populate();
    void addJobItem(jobs::IJobInfoPtr job_info);
    void updateJobItem(kcl::ScrollListItem* item);
    
    // signal events
    void onJobQueueChanged();
    
    // events
    void onCancelButtonClicked(wxCommandEvent& evt);
    void onDoJobQueueRefresh(wxCommandEvent& evt);
    
private:

    JobListCtrlTimer* m_timer;
    jobs::IJobQueuePtr m_job_queue;
    std::map<wxButton*, kcl::ScrollListItem*> m_cancel_button_map;

    DECLARE_EVENT_TABLE()
};




class JobManagerPanel : public wxPanel,
                        public IDocument,
                        public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.JobManagerPanel")
    XCM_BEGIN_INTERFACE_MAP(JobManagerPanel)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:
    JobManagerPanel();
    ~JobManagerPanel();

public:

    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    
private:

    // -- events --
    void onCleanupButtonClicked(wxCommandEvent& event);

private:

    IDocumentSitePtr m_doc_site;
    
    JobListCtrl* m_job_list;
    wxButton* m_cleanup_button;
    
    DECLARE_EVENT_TABLE()
};


#endif


