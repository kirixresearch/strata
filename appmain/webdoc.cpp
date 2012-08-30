/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2006-09-22
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "webdoc.h"
#include "toolbars.h"
#include "tabledoc.h"
#include "textdoc.h"
#include "extensionmgr.h"
#include "feedparser.h"
#include "bookmark.h"
#include "dlgpagesetup.h"
#include "dbdoc.h"
#include "../webconnect/webcontrol.h"
#include <wx/fs_inet.h>
#include <wx/stdpaths.h>
#include <wx/artprov.h>
#include <wx/webview.h>
#include <kl/regex.h>
#include <kl/md5.h>


const int wxID_WEB = 9001;
const int wxID_WEBVIEW = 9002;

const int ID_Open = 18001;
const int ID_OpenNewTab = 18002;
const int ID_ImportTable = 18003;
const int ID_RotateLeft = 18004;
const int ID_RotateRight = 18005;
const int ID_ViewPageSource = 18006;
const int ID_ViewWebPage = 18007;
const int ID_SaveLinkAs = 18008;
const int ID_SavePageAs = 18009;

const float ZOOM_CHANGE_PERC = 0.10f;
const float ZOOM_DEFAULT = 1.0f;


// used to patch through job info from the web control
// to our job queue
class DownloadProgressListener : public wxWebProgressBase
{
public:

    DownloadProgressListener(cfw::IJobInfoPtr job_info = xcm::null)
    {
        m_job_id = -1;
        
        if (job_info)
        {
            m_job_info = job_info;
        }
         else
        {
            m_job_info.create_instance("cfw.JobInfo");
        }
        
        m_job_info->setTitle(wxT("Download"));
        m_job_info->setInfoMask(cfw::jobMaskTitle |
                            cfw::jobMaskStartTime |
                            cfw::jobMaskFinishTime |
                            cfw::jobMaskPercentage |
                            cfw::jobMaskProgressString |
                            cfw::jobMaskProgressBar |
                            cfw::jobMaskCurrentCount);
    }

    virtual void Init(const wxString& url,
                      const wxString& suggested_filename)
    {
        m_url = url;
        m_filename = suggested_filename;
        
        wxString filename = m_filename.AfterLast(PATH_SEPARATOR_CHAR);
        wxString title = wxString::Format(_("Downloading '%s' to '%s'"),
                                          filename.c_str(),
                                          suggested_filename.c_str());
        m_job_info->setTitle(title);
        m_job_info->setProgressStringFormat(
                                    _("$C KB downloaded"),
                                    _("$C of $M KB downloaded ($p1%)"));
    }

    virtual void OnStart()
    {
        m_job_info->setStartTime(time(NULL));
        m_job_id = g_app->getJobQueue()->addJobInfo(m_job_info, cfw::jobStateRunning);
    }

    virtual void OnFinish()
    {
        // set job to finished
        m_job_info->setFinishTime(time(NULL));
        m_job_info->setState(cfw::jobStateFinished);

        delete this;
    }
    
    virtual void OnError(const wxString& message)
    {
        m_job_info->setFinishTime(time(NULL));
        m_job_info->setState(cfw::jobStateFailed);
    }
    
    virtual void OnProgressChange(wxLongLong cur_progress,
                                  wxLongLong max_progress)
    {
        m_job_info->setMaxCount(max_progress.ToDouble());
        m_job_info->setCurrentCount(cur_progress.ToDouble());
        
        if (m_job_info->getState() == cfw::jobStateCancelling ||
            m_job_info->getState() == cfw::jobStateCancelled)
        {
            m_job_info->setFinishTime(time(NULL));
            m_job_info->setState(cfw::jobStateCancelled);
            Cancel();
        }
    }
    
private:

    wxString m_url;
    wxString m_filename;
    cfw::IJobInfoPtr m_job_info;
    int m_job_id;
};






class AppExtensionContentHandler : public wxWebContentHandler
{
public:
    
    enum
    {
        modeInstall = 0,
        modeDownload = 1
    };
    
    AppExtensionContentHandler()
    {
        m_f = 0;
        m_mode = modeDownload;
    }
    
    ~AppExtensionContentHandler()
    {
    }
    
    bool CanHandleContent(const wxString& url, const wxString& mime_type)
    {        
        wxString ext = url.AfterLast(wxT('.'));
        ext.MakeLower();
        
        if (ext == wxT("kxt"))
        {
            // handle the data
            return true;
        }

        return false;
    }
    
    void OnStartRequest(const wxString& url)
    {
        int ID_InstallExtension = 100;
        int ID_DownloadExtension = 101;
        
        CustomPromptDlg dlg(g_app->getMainWindow(), 
                            _("Extension Download"),
                            _("Would you like to download this extension or install it now?\n\nPlease be aware that extensions can contain routines that may damage or destroy information on your computer and/or could violate your privacy.  We strongly recommend that you only install extensions from a trusted source."),
                            wxSize(420,220));
        dlg.setBitmap(CustomPromptDlg::bitmapQuestion);
        dlg.setButton1(ID_DownloadExtension, _("Download"));
        dlg.setButton2(ID_InstallExtension, _("Install Now"));
        dlg.showButtons(CustomPromptDlg::showButton1 |
                        CustomPromptDlg::showButton2 |
                        CustomPromptDlg::showCancel);
        
        int result = dlg.ShowModal();
        if (result != ID_InstallExtension && result != ID_DownloadExtension)
            return;
        
        if (result == ID_InstallExtension)
        {
            // set the handler mode
            m_mode = modeInstall;
            
            // save the url
            m_url = url;
            
            // create a temporary filename
            m_filename = towx(xf_get_temp_filename(L"ext", L"kxt"));
            
            // remove the file if it exists
            xf_remove(towstr(m_filename));
            
            // create the temporary file
            m_f = xf_open(towstr(m_filename),
                          xfOpenCreateIfNotExist,
                          xfReadWrite,
                          xfShareNone);
            return;
        }
         else if (result == ID_DownloadExtension)
        {
            // set the handler mode
            m_mode = modeDownload;
            
            // save the url
            m_url = url;
            
            // get the filename from the dialog
            
            wxString filter;
            filter  = _("Application Extension Files");
            filter += wxT(" (*.kxt)|*.kxt|");
            filter.RemoveLast(); // get rid of the last pipe sign

            wxString filename = getFilenameFromPath(m_url, true);   // include extension
            wxFileDialog dlg2(g_app->getMainWindow(),
                              _("Save As"),
                              wxEmptyString,
                              filename,
                              filter,
                              wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            
            if (dlg2.ShowModal() != wxID_OK)
                return;
            
            // store the filename from the dialog
            m_filename = dlg2.GetPath();
            
            // remove the file if it exists
            xf_remove(towstr(m_filename));
            
            // create the file
            m_f = xf_open(towstr(m_filename),
                          xfOpenCreateIfNotExist,
                          xfReadWrite,
                          xfShareNone);
            return;
        }
    }
    
    void OnStopRequest()
    {
        if (m_f)
        {
            xf_close(m_f);
            m_f = 0;
            
            if (m_mode == modeInstall)
            {
                // add an extension
                ExtensionMgr* extmgr = g_app->getExtensionMgr();
                ExtensionInfo info = extmgr->installExtension(m_filename);
                if (!info.isOk())
                {
                    cfw::appMessageBox(_("Installation of the extension was not possible, because the extension's format could not be determined."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION,
                                       g_app->getMainWindow());
                    return;
                }
                
                g_app->getAppController()->showExtensionManager();
            }
        }
        
    }
    
    void OnData(const unsigned char* buf, size_t size)
    {
        if (!m_f)
            return;
        
        xf_write(m_f, buf, size, 1);
    }
    
private:
    
    int m_mode;
    xf_file_t m_f;
    wxString m_url;
    wxString m_filename;
};




/*
// DelimitedTextContentHandler is a handler for delimited text files
// which are clicked in webdoc.  It handles csv file links,
// not csv content downloads.  See below for more information.

class DelimitedTextContentHandler : public wxWebContentHandler
{
public:

    DelimitedTextContentHandler()
    {
        m_f = 0;
        m_extension = wxT("csv");
    
        m_site_id = -1;
        m_opened_with_middle_click = false;
    }

    ~DelimitedTextContentHandler()
    {
    }
    
    void setTargetSiteId(int site_id)
    {
        m_site_id = site_id;
    }
    
    void setOpenedWithMiddleClick(bool b)
    {
        m_opened_with_middle_click = b;
    }
    
    bool CanHandleContent(const wxString& url, const wxString& mime_type)
    {
        if (mime_type == wxT("application/excel") ||
            mime_type == wxT("application/vnd.ms-excel") ||
            mime_type == wxT("application/vnd.msexcel") ||
            mime_type == wxT("application/x-excel") ||
            mime_type == wxT("application/x-msexcel"))
        {
            wxString ext = url.AfterLast(wxT('.'));
            ext.MakeLower();
            if (ext != wxT("csv"))
            {
                m_extension = wxT("xls");
            }
        }
          
        if (mime_type == wxT("text/comma-separated-values") ||
            mime_type == wxT("text/csv") ||
            mime_type == wxT("application/csv"))
        {
            m_extension = wxT("csv");
            return true;
        }
        
        if (mime_type == wxT("text/tab-separated-values") ||
            mime_type == wxT("text/tsv") ||
            mime_type == wxT("application/tsv"))
        {
            m_extension = wxT("tsv");
            return true;
        }
        
        if (mime_type == wxT("application/dbase") ||
            mime_type == wxT("application/x-dbase") ||
            mime_type == wxT("application/dbf") ||
            mime_type == wxT("application/x-dbf") ||
            mime_type == wxT("zz-application/zz-winassoc-dbf"))
        {
            m_extension = wxT("dbf");
            return true;
        }
        
        

        wxString ext = url.AfterLast(wxT('.'));
        ext.MakeLower();
        
        if (ext == wxT("csv") ||
            ext == wxT("tsv") ||
            ext == wxT("dbf") ||
            ext == wxT("xls"))
        {
            m_extension = ext;
            // handle the data
            return true;
        }

        
        // let somebody else handle the request
        return false;
    }
    
    void OnStartRequest(const wxString& url)
    {
        m_url = url;
        
        m_filename = towx(xf_get_temp_filename(L"cch", towstr(m_extension)));
        
        xf_remove(towstr(m_filename));
        
        m_f = xf_open(towstr(m_filename),
                 xfOpenCreateIfNotExist,
                 xfReadWrite,
                 xfShareNone);
    }
    
    void OnStopRequest()
    {
        if (!m_f)
            return;
        
        xf_close(m_f);
        m_f = 0;
        
        
        // find out where and how to open up the new window,
        // then reset the values for next time
        int target_id = m_site_id;
        bool opened_with_middle_click = m_opened_with_middle_click;
        
        m_site_id = -1;  // reset it for next time
        m_opened_with_middle_click = false;


        wxWindow* container_wnd = NULL;
        
        if (target_id != -1)
        {
            cfw::IDocumentSitePtr target_site;
            target_site = g_app->getMainFrame()->lookupSiteById(target_id);
            if (target_site.isOk())
            {
                IWebDocPtr webdoc = target_site->getDocument();
                if (webdoc.isOk() && 
                      (webdoc->getWebControl()->GetCurrentURI() == wxT("") ||
                       webdoc->getWebControl()->GetCurrentURI() == wxT("about:blank")))
                {
                    container_wnd = target_site->getContainerWindow();
                }
            }
        }
        
        
        
        wxString file_url = filenameToUrl(m_filename);
        int site_id = 0;
        if (createTextDoc(file_url, container_wnd, &site_id))
        {
            // make sure the site has the source url in the url bar
            cfw::IDocumentSitePtr site = g_app->getMainFrame()->lookupSiteById(site_id);

            ITextDocPtr text_doc = lookupOtherDocument(site, "appmain.TextDoc");
            if (text_doc)
            {
                text_doc->setSourceUrl(m_url);
            }
            
            ITableDocPtr table_doc = lookupOtherDocument(site, "appmain.TableDoc");
            if (table_doc)
            {
                table_doc->setSourceUrl(m_url);
            }
            
            g_app->getAppController()->updateURLToolbar();
        }
    }
    
    void OnData(const unsigned char* buf, size_t size)
    {
        if (!m_f)
            return;
        
        xf_write(m_f, buf, size, 1);
    }
    
private:

    xf_file_t m_f;
    wxString m_url;
    wxString m_filename;
    wxString m_extension;

    int m_site_id;
    bool m_opened_with_middle_click;
};
*/




// used for download-style delimited text loading
class DelimitedTextProgressListener : public wxWebProgressBase,
                                      public xcm::signal_sink
{
public:

    DelimitedTextProgressListener()
    {
        m_site_id = -1;
        m_cancelled = false;
        
        m_job_info.create_instance("cfw.JobInfo");
        m_job_info->setTitle(wxT("Download"));
        m_job_info->setInfoMask(cfw::jobMaskTitle |
                            cfw::jobMaskStartTime |
                            cfw::jobMaskFinishTime |
                            cfw::jobMaskPercentage |
                            cfw::jobMaskProgressString |
                            cfw::jobMaskProgressBar |
                            cfw::jobMaskCurrentCount);
    }

    virtual void Init(const wxString& url,
                      const wxString& suggested_filename)
    {
        m_url = url;
        m_filename = suggested_filename;
        
        wxString filename = m_filename.AfterLast(PATH_SEPARATOR_CHAR);
        wxString title = wxString::Format(_("Downloading '%s' to '%s'"),
                                          filename.c_str(),
                                          suggested_filename.c_str());
        m_job_info->setTitle(title);
        m_job_info->setProgressStringFormat(
                                    _("$C KB downloaded"),
                                    _("$C of $M KB downloaded ($p1%)"));
    }

    void setTargetSiteId(int site_id)
    {
        m_site_id = site_id;
    }
    
    virtual void OnStart()
    {
        // start the progress bitmap updater
        cfw::IDocumentSitePtr target_site;
        target_site = g_app->getMainFrame()->lookupSiteById(m_site_id);
        if (target_site.isOk())
        {
            m_bitmap_updater.setDocSite(target_site);
            m_bitmap_updater.setFinishBitmap(GETBMP(gf_table_16));
            m_bitmap_updater.start();
        }
        
        // connect signals
        g_app->getMainFrame()->sigSiteClose().connect(this, &DelimitedTextProgressListener::onSiteClose);
        m_job_info->sigStateChanged().connect(this, &DelimitedTextProgressListener::onJobInfoStateChanged);
        
        m_job_info->setStartTime(time(NULL));
        g_app->getJobQueue()->addJobInfo(m_job_info, cfw::jobStateRunning);
    }

    virtual void OnFinish()
    {
        // stop the progress bitmap updater
        m_bitmap_updater.stop();

        int site_id = 0;
        wxWindow* container_wnd = NULL;
        
        if (m_site_id != -1)
        {
            cfw::IDocumentSitePtr target_site;
            target_site = g_app->getMainFrame()->lookupSiteById(m_site_id);
            if (target_site.isOk())
            {
                container_wnd = target_site->getContainerWindow();
            }
        }
        
        
        
        wxString file_url = filenameToUrl(m_filename);
        createTextDoc(file_url, container_wnd, &site_id);
        
        cfw::IDocumentSitePtr site = g_app->getMainFrame()->lookupSiteById(site_id);
        ITextDocPtr text_doc = lookupOtherDocument(site, "appmain.TextDoc");
        if (text_doc)
        {
            text_doc->setSourceUrl(m_url);
            g_app->getAppController()->updateURLToolbar();
        }
        
        ITableDocPtr table_doc = lookupOtherDocument(site, "appmain.TableDoc");
        if (table_doc)
        {
            table_doc->setSourceUrl(m_url);
            table_doc->setSourceMimeType(wxT("text/csv"));
        }
        
        /*
        if (m_site_id != -1)
        {
            cfw::IDocumentSitePtr webdoc_site = lookupOtherDocumentSite(site, "appmain.WebDoc");
            if (webdoc_site)
            {
                webdoc_site->setVisible(false);
                //g_app->getMainFrame()->closeSite(webdoc_site, cfw::closeForce);
            }
        }
        */
        
        // set job to finished
        m_job_info->setFinishTime(time(NULL));
        m_job_info->setState(cfw::jobStateFinished);

        
        delete this;
    }
    
    virtual void OnError(const wxString& message)
    {
        // stop the progress bitmap updater
        m_bitmap_updater.Stop();

        m_job_info->setFinishTime(time(NULL));
        m_job_info->setState(cfw::jobStateFailed);
    }
    
    virtual void OnProgressChange(wxLongLong cur_progress,
                                  wxLongLong max_progress)
    {
        double cur_count = cur_progress.ToDouble();
        double max_count = max_progress.ToDouble();
        
        // for some reason, the last time OnProgressChange() gets called,
        // the current progress and max progress are both -1.  This code
        // prevents the progress from ever reporting something like
        // "-1 of -1 records processed"
        if (cur_count > 0.0)
            m_job_info->setCurrentCount(cur_count);
        if (max_count > 0.0)
            m_job_info->setMaxCount(max_count);
    }
    
    // cancel the job if the site is closed
    void onSiteClose(cfw::IDocumentSitePtr doc_site)
    {
        if (doc_site->getId() == m_site_id)
            cancelDownload(false);
    }
    
    // close the site if the job is cancelled
    void onJobInfoStateChanged(cfw::IJobInfoPtr job_info)
    {
        if (job_info->getState() == cfw::jobStateCancelling)
            cancelDownload(true);
    }

    void cancelDownload(bool close_site)
    {
        if (m_cancelled)
            return;
        
        // stop the progress bitmap updater
        m_bitmap_updater.stop();
        
        // cancel the download job
        Cancel();
        
        // update the job info
        if (m_job_info->getState() != cfw::jobStateCancelled)
        {
            m_job_info->setFinishTime(time(NULL));
            m_job_info->setState(cfw::jobStateCancelled);
        }
        
        // make sure the associated document site is closed
        if (close_site)
        {
            cfw::IDocumentSitePtr doc_site;
            doc_site = g_app->getMainFrame()->lookupSiteById(m_site_id);
            if (doc_site.isOk())
                g_app->getMainFrame()->closeSite(doc_site, cfw::closeForce);
        }
        
        m_cancelled = true;
    }
    
private:

    wxString m_url;
    wxString m_filename;
    cfw::IJobInfoPtr m_job_info;
    ProgressBitmapUpdater m_bitmap_updater;
    int m_site_id;
    bool m_cancelled;
};





class FeedContentHandler : public wxWebContentHandler
{
public:

    FeedContentHandler()
    {
        m_site_id = -1;
        m_opened_with_middle_click = false;
    }
    
    ~FeedContentHandler()
    {
    }
    
    void setTargetSiteId(int site_id)
    {
        m_site_id = site_id;
    }
    
    void setOpenedWithMiddleClick(bool b)
    {
        m_opened_with_middle_click = b;
    }
    
    bool CanHandleContent(const wxString& url, const wxString& mime_type)
    {
        if (mime_type == wxT("application/rss+xml") ||
            mime_type == wxT("application/rdf+xml") ||
            mime_type == wxT("application/atom+xml") ||
            mime_type == wxT("text/xml"))
        {
            return true;
        }
        
/*
        // also check the file extension
        wxString ext = url.AfterLast(wxT('.'));
        ext.MakeLower();
        if (ext == wxT("rss") || ext == wxT("atom"))
            return true;
*/

        return false;
    }
    
    void OnStartRequest(const wxString& url)
    {
        m_url = url;
        m_buf.SetDataLen(0);
    }
    
    void OnStopRequest()
    {
        // write out the memory buffer to a file
        wxString filename = towx(xf_get_temp_filename(L"cch", L"tmp"));

        xf_file_t f = xf_open(towstr(filename),
                              xfOpenCreateIfNotExist,
                              xfReadWrite,
                              xfShareNone);
                              
        xf_write(f, m_buf.GetData(), m_buf.GetDataLen(), 1);
        
        xf_close(f);
        
        
        // parse the file
        
        FeedParser parser;
        if (!parser.loadAndParse(filename))
        {
            xf_remove(towstr(filename));
            // can't parse feed file
            return;
        }
        
        xf_remove(towstr(filename));

        
        tango::ISetPtr set = parser.convertToSet();
        
        if (set.isNull())
        {
            // bad set
            return;
        }
        

        // find out where and how to open up the new window,
        // then reset the values for next time
        
        int target_id = m_site_id;
        bool opened_with_middle_click = m_opened_with_middle_click;
        
        m_site_id = -1;  // reset it for next time
        m_opened_with_middle_click = false;



        cfw::IDocumentSitePtr site;
        ITableDocPtr doc = TableDocMgr::createTableDoc();
        doc->open(set, xcm::null);
        doc->setSourceUrl(m_url);
        doc->setSourceMimeType(wxT("application/rss+xml"));

        
        bool created_in_existing_container = false;
        
        if (target_id != -1)
        {
            site = g_app->getMainFrame()->lookupSiteById(target_id);
            if (site.isOk())
            {
                IWebDocPtr webdoc = site->getDocument();
                if (webdoc.isOk() && 
                      (webdoc->getWebControl()->GetCurrentURI() == wxT("") ||
                       webdoc->getWebControl()->GetCurrentURI() == wxT("about:blank")))
                {                
                    g_app->getMainFrame()->createSite(site->getContainerWindow(),
                                                      doc,
                                                      true);
                    created_in_existing_container = true;
                }
            }
        }

        if (!created_in_existing_container)
        {
            site = g_app->getMainFrame()->createSite(doc,
                                                     cfw::sitetypeNormal,
                                                     -1, -1, -1, -1);
        }
        
        
        int width, height;
        cfw::IDocumentPtr cdoc = doc;
        cdoc->getDocumentWindow()->GetClientSize(&width, &height);
        



        
        // set some view columns to make it look nice
        ITableDocViewPtr view;
   
        view = doc->getActiveView();
        if (view.isOk())
        {
            int unit_size = width/10;
            
            int idx;
            ITableDocViewColPtr col;
            
            idx = view->getColumnIdx(L"link");
            if (idx >= 0)
            {
                col = view->getColumn((unsigned int)idx);
                col->setSize(unit_size);
            }
            
            idx = view->getColumnIdx(L"title");
            if (idx >= 0)
            {
                col = view->getColumn((unsigned int)idx);
                col->setTextWrap(tabledocWrapOn);
                col->setSize(unit_size*2);
            }
            
            idx = view->getColumnIdx(L"description");
            if (idx >= 0)
            {
                col = view->getColumn((unsigned int)idx);
                col->setTextWrap(tabledocWrapOn);
                col->setSize(unit_size*4);
            }

            // note: setting the following row size to 80 increases the
            // cell size so that feed content is easier to read; however,
            // it doesn't allow the user to select relevant articles as 
            // quickly because they can no longer see as many articles;
            // consequently, if a user saves a link to an RSS and sets
            // up a mark/dynamic field combination to highlight links
            // with keywords, they can't simply middle-click on the links
            // in the highlighted records without having to scroll down
            // several times; the tradeoff here is whether the RSS table
            // is used for reading or whether it's used as a means to
            // filter articles based on keyword to get to the actual 
            // articles; for now, favor the later use-case

            //view->setRowSize(80);

            doc->refreshActiveView();
        }
        
        if (created_in_existing_container &&
            !opened_with_middle_click &&
            site.isOk())
        {
            g_app->getMainFrame()->setActiveChild(site);
        }
    }
    
    void OnData(const unsigned char* buf, size_t size)
    {
        m_buf.AppendData((void*)buf, size);
    }
      
private:

    wxMemoryBuffer m_buf;
    wxString m_url;
    int m_site_id;
    bool m_opened_with_middle_click;
};








#ifdef __WXGTK__
extern "C" {
typedef void* (*ft_gconf_client_get_default)();
void g_free(void* mem);
void g_object_unref(void* object);
typedef char* (*ft_gconf_client_get_string)(
                  void* client,
                  const char* key,
                  void** err);
}

static wxString getGnomeEmailCommand()
{
    static void* gconf_lib = NULL;
    static ft_gconf_client_get_default fp_gconf_client_get_default = NULL;
    static ft_gconf_client_get_string fp_gconf_client_get_string = NULL;

    if (!gconf_lib)
    {
        gconf_lib = dlopen("libgconf-2.so", RTLD_LAZY);
        if (!gconf_lib)
            return wxEmptyString;
        fp_gconf_client_get_default = (ft_gconf_client_get_default)dlsym(gconf_lib, "gconf_client_get_default");
        fp_gconf_client_get_string = (ft_gconf_client_get_string)dlsym(gconf_lib, "gconf_client_get_string");
    }

    if (fp_gconf_client_get_default == NULL ||
        fp_gconf_client_get_string == NULL)
    {
        return wxEmptyString;
    }
        
    void* cli = fp_gconf_client_get_default();
    if (!cli)
        return wxEmptyString;
    char* c = fp_gconf_client_get_string(cli, "/desktop/gnome/url-handlers/mailto/command", NULL);
    if (!c)
        return wxEmptyString;

    wxString ret = wxString::From8BitData(c);
    g_free(c);
    g_object_unref(cli);
    return ret;
}

#endif


static bool composeMail(const wxString& mailto)
{
    // get default mail app
    wxString default_mail_app;
    wxString open_cmd;
    
    #if defined  __WXMSW__

        wxRegKey dk2(wxT("HKEY_LOCAL_MACHINE\\Software\\Clients\\Mail"));
        if (dk2.Exists() && dk2.Open(wxRegKey::Read))
        {
            dk2.QueryValue(wxEmptyString, default_mail_app);
        }

        if (default_mail_app.IsEmpty())
            return false;
        
        wxString key = wxT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Clients\\Mail\\");
        key += default_mail_app;
        key += wxT("\\shell\\open\\command");
        
        wxRegKey okey(key);
        if (okey.Exists() && okey.Open(wxRegKey::Read))
        {
            okey.QueryValue(wxEmptyString, open_cmd);
        }
    #elif defined __WXGTK__
        open_cmd = getGnomeEmailCommand().BeforeFirst(wxT(' '));
        wxString test = open_cmd;
        test.MakeLower();
        test.Trim(true);
        test.Trim(false);
        if (test == wxT("evolution"))
            default_mail_app = wxT("Evolution");
             else
            default_mail_app = wxT("Mozilla");
    #endif
    
    if (default_mail_app.CmpNoCase(wxT("Outlook Express")) == 0)
    {
        open_cmd += wxT(" /mailurl:");
        open_cmd += mailto;
    }
     else if (default_mail_app.CmpNoCase(wxT("Microsoft Outlook")) == 0)
    {
        open_cmd.MakeUpper();
        open_cmd.Replace(wxT("/RECYCLE"), wxT(""));
        open_cmd += wxT(" /c ipm.note /m \"");
        open_cmd += mailto;
        open_cmd += wxT("\"");
    }
     else if (default_mail_app.CmpNoCase(wxT("Mozilla Thunderbird")) == 0 ||
              default_mail_app.CmpNoCase(wxT("Netscape Mail")) == 0 ||
              default_mail_app.CmpNoCase(wxT("Mozilla")) == 0)
    {
        open_cmd += wxT(" -compose \"");
        open_cmd += mailto;
        open_cmd += wxT("\"");
    }
     else
    {
        open_cmd += wxT(" ");
        open_cmd += mailto;
    }
    
    ::wxExecute(open_cmd);
    
    return true;
}





class OpenOrSaveDlg : public wxDialog
{
    
public:

    OpenOrSaveDlg(wxWindow* parent, const wxString& filename)
                         : wxDialog(parent,
                                    -1,
                                    _("File Download"),
                                    wxDefaultPosition,
                                    wxSize(360, 175),
                                    wxDEFAULT_DIALOG_STYLE |
                                    wxCENTER)
    {
        wxStaticText* label_message = new wxStaticText(this, -1, _("Would you like to open or save this file?"));
        
        // create filename text
        
        wxString fn = _("Filename:");
        fn += wxT(" ");
        fn += filename;
        wxStaticText* label_filename = new wxStaticText(this, -1, fn);
        
        // create a platform standards-compliant OK/Cancel sizer
        wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
        ok_cancel_sizer->SetAffirmativeButton(new wxButton(this, wxID_OPEN));
        ok_cancel_sizer->SetNegativeButton(new wxButton(this, wxID_SAVE));
        ok_cancel_sizer->SetCancelButton(new wxButton(this, wxID_CANCEL));
        ok_cancel_sizer->Realize();
        ok_cancel_sizer->AddSpacer(5);
        
        // this code is necessary to get the OK/Cancel sizer's bottom margin to 8
        wxSize min_size = ok_cancel_sizer->GetMinSize();
        min_size.SetHeight(min_size.GetHeight()+16);
        ok_cancel_sizer->SetMinSize(min_size);

        // create username/password sizer
        
        wxBitmap bmp = wxArtProvider::GetBitmap(wxART_QUESTION, wxART_MESSAGE_BOX);
        wxStaticBitmap* bitmap_question = new wxStaticBitmap(this, -1, bmp);
        
        wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
        vert_sizer->Add(label_message, 0, wxEXPAND);
        vert_sizer->AddSpacer(16);
        vert_sizer->Add(label_filename, 0, wxEXPAND);
        
        // create top sizer
        
        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        top_sizer->AddSpacer(7);
        top_sizer->Add(bitmap_question, 0, wxTOP, 7);
        top_sizer->AddSpacer(15);
        top_sizer->Add(vert_sizer, 1, wxEXPAND | wxTOP, 7);
        
        // create main sizer
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->AddSpacer(8);
        main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        main_sizer->AddStretchSpacer();
        main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
                
        SetSizer(main_sizer);
        Layout();
    }
    
    ~OpenOrSaveDlg()
    {
    }

private:

    // event handlers
    
    void OnOpen(wxCommandEvent& evt)
    {
        EndModal(wxID_OPEN);
    }
    
    void OnSave(wxCommandEvent& evt)
    {
        EndModal(wxID_SAVE);
    }
    
    void OnCancel(wxCommandEvent& evt)
    {
        EndModal(wxID_CANCEL);
    }
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(OpenOrSaveDlg, wxDialog)
    EVT_BUTTON(wxID_OPEN, OpenOrSaveDlg::OnOpen)
    EVT_BUTTON(wxID_SAVE, OpenOrSaveDlg::OnSave)
    EVT_BUTTON(wxID_CANCEL, OpenOrSaveDlg::OnCancel)
END_EVENT_TABLE()





struct HtmlTableFieldInfo
{
    wxString name;
    size_t max_len;
    tango::objhandle_t handle;
    int type;
    int max_dec;
    int date_faults;
    int numeric_faults;
    bool empty;
        
    HtmlTableFieldInfo()
    {
        name = wxT("");
        max_len = 0;
        handle = 0;
        type = tango::typeDate; // fall back in this order Date -> Numeric -> Character
        max_dec = 0;
        date_faults = 0;
        numeric_faults = 0;
        empty = true;
    }
    
    HtmlTableFieldInfo(const HtmlTableFieldInfo& c)
    {
        name = c.name;
        max_len = c.max_len;
        handle = c.handle;
        type = c.type;
        max_dec = c.max_dec;
        date_faults = c.date_faults;
        numeric_faults = c.numeric_faults;
        empty = c.empty;
    }
    
    HtmlTableFieldInfo& operator=(const HtmlTableFieldInfo& c)
    {
        name = c.name;
        max_len = c.max_len;
        handle = c.handle;
        type = c.type;
        max_dec = c.max_dec;
        date_faults = c.date_faults;
        numeric_faults = c.numeric_faults;
        empty = c.empty;
        return *this;
    }
};


// NOTE: this code is from xdfs, if you modify/improve it here, merge
// your changes in xdfs as well.  Thanks.

static bool extractMatchResults(klregex::match_results<wxChar>& matchres,
                                int* yy,
                                int* mm,
                                int* dd)
{
    const klregex::sub_match<wxChar>& year_match = matchres[wxT("year")];
    const klregex::sub_match<wxChar>& month_match = matchres[wxT("month")];
    const klregex::sub_match<wxChar>& day_match = matchres[wxT("day")];

    if (!year_match.isValid())
        return false;
    if (!month_match.isValid())
        return false;
    if (!day_match.isValid())
        return false;
    
    static const wxChar* months[] = { wxT("JAN"), wxT("FEB"), wxT("MAR"),
                                      wxT("APR"), wxT("MAY"), wxT("JUN"),
                                      wxT("JUL"), wxT("AUG"), wxT("SEP"),
                                      wxT("OCT"), wxT("NOV"), wxT("DEC") };
                                

    if (yy)
    {
        *yy = wxAtoi(year_match.str().c_str());
        if (*yy < 100)
        {
            if (*yy < 70)
                *yy += 2000;
                 else
                *yy += 1900;
        }
    }

    if (mm)
    {
        wxString month = wxString(towx(month_match.str())).Left(3);
        if (month.length() == 0)
            return false;
        month.MakeUpper();
         
        if (iswdigit(month[0]))
        {
            *mm = wxAtoi(month.c_str());
        }
         else
        {            
            int j;
            bool found = false;
            for (j = 0; j < 12; ++j)
            {
                if (0 == month.Cmp(months[j]))
                {
                    *mm = j+1;
                    found = true;
                    break;
                }
            }
            
            if (!found)
            {
                // unknown month name
                return false;
            }
        }
    }
    
    if (dd)
    {
        *dd = wxAtoi(day_match.str().c_str());
    }

    return true;
}




static bool parseDelimitedStringDate(const wxString& str,
                                     int* year = NULL,
                                     int* month = NULL,
                                     int* day = NULL)
{
    if (str.length() == 0)
        return false;

    if (year)
    {
        *year = 0;
        *month = 0;
        *day = 0;
    }
    
    // CCYY-MM-DD this format is so commonly a date format that
    // we don't need to do further validation.  It does however check to make
    // sure the first digit is between 0 and 3 (should be enough for years 0 - 3999)
    static const klregex::basic_regex<wxChar> fmt1(wxT("(?<year>[0-3]\\d{3})[-/. ](?<month>\\d{1,2})[-/. ](?<day>\\d{1,2})"));
    
    // this format checks for CCYYMMDD, but does numerical range checking on the month and day portions
    static const klregex::basic_regex<wxChar> fmt2(wxT("(?<year>[[0-9]{4})(?<month>0[1-9]|1[0-2])(?<day>0[1-9]|[12][0-9]|3[01])"));

    // this format checks for MM/DD/CCYY
    static const klregex::basic_regex<wxChar> fmt3(wxT("(?<month>0?[1-9]|1[0-2])[-/. ](?<day>0?[1-9]|[12][0-9]|3[01])[-/. ](?<year>[0-9]{2,4})"));

    // this format checks for DD/MM/CCYY
    static const klregex::basic_regex<wxChar> fmt4(wxT("(?<day>0?[1-9]|[12][0-9]|3[01])[-/. ](?<month>0?[1-9]|1[0-2])[-/. ](?<year>[0-9]{2,4})"));

    // this format checks for DD-MMM-CCYY
    static const klregex::basic_regex<wxChar> fmt5(wxT("(?<day>0?[1-9]|[12][0-9]|3[01])[-/., ]+(?<month>[A-Za-z]+)[-/., ]+(?<year>[0-9]{2,4})"));
    
    // this format checks for MMM DD CCYY
    static const klregex::basic_regex<wxChar> fmt6(wxT("(?i)(?<month>jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)\\w*[-/., ]+(?<day>0?[1-9]|[12][0-9]|3[01])[-/., ]+(?<year>[0-9]{2,4})"));

    // this format checks for Friday, 9 July, 11:10:00 GMT+0130
    static const klregex::basic_regex<wxChar> fmt7(L"^((?<dow>\\[A-Za-z]*)[, ]+)?(?<day>0?[1-9]|[12][0-9]|3[01])\\s+(?<month>[A-Za-z]*)(\\s+(?<year>\\d{4}))?[ ,]+(?<hour>\\d+)[:. ](?<minute>\\d+)(?:[:. ](?<second>\\d+))?");

    klregex::match_results<wxChar> matchres;
    
    const wxChar* start = str.c_str();
    const wxChar* end = start + str.length();
    
    if (fmt1.match(start, end, matchres) || 
        fmt2.match(start, end, matchres))
    {
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt3.match(start, end, matchres))
    {
        // using MDY order
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt4.match(start, end, matchres))
    {
        // using YMD order
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt5.match(start, end, matchres) ||
        fmt6.match(start, end, matchres) ||
        fmt7.match(start, end, matchres)) 
    {
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
  
    return false;
}


static void nodeToString(wxDOMNode& cell, wxString& output)
{
    if (cell.GetNodeType() == wxWEB_NODETYPE_COMMENT_NODE)
        return;
    output += cell.GetNodeValue();
    wxDOMNodeList children = cell.GetChildNodes();
    size_t i, count = children.GetLength();
    for (i = 0; i < count; ++i)
    {
        wxDOMNode node = children.Item(i);
        nodeToString(node, output);
    }
    
    // now get rid of space(s) used to format html code
    output.Replace(wxT("\t"), wxT(" "));
    output.Replace(wxT("\n"), wxT(" "));
    output.Replace(wxT("\r"), wxT(" "));
    
    while (output.Find(wxT("  ")) != -1)
        output.Replace(wxT("  "), wxT(" "));
    output.Replace(wxT("\t"), wxT(" "));
}




// HTML Table to Data Table routines

static int lookupNodeIdx(wxDOMNodeList& list, const wxString& node_name)
{
    size_t i, len = list.GetLength();
    for (i = 0; i < len; ++i)
    {
        wxDOMNode node = list.Item(i);
        if (0 == node.GetNodeName().CmpNoCase(node_name))
            return (int)i;
    }
    
    return -1;
}


static bool isNumericValue(const wxString& s)
{
    int dec_count = 0;
    
    size_t i, len = s.Length();
    for (i = 0; i < len; ++i)
    {
        wxChar ch = s.GetChar(i);
        
        if (ch == '-')
        {
            // minus is ok if it's at the beginning
            if (i == 0)
                continue;
        }
                
        
        if (ch == '.')
            dec_count++;
            
        if (dec_count > 1)
        {
            // strings with more than one period aren't to be
            // treated as numbers
            return false;
        }
        
        if (!wcschr(L"0123456789$%,.+", ch))
            return false;
    }
    
    return true;
}

static double extractNumber(const wxString& num, int* dec_places = NULL)
{
    wxString s = num;
    s.Replace(wxT("$"), wxT(""));
    s.Replace(wxT("%"), wxT(""));
    s.Replace(wxT(","), wxT(""));
    
    int dec_pos = s.Find(wxT('.'), true);
    if (dec_places)
    {
        if (dec_pos == -1)
            *dec_places = 0;
             else
            *dec_places = (s.Length() - dec_pos - 1);
    }
    
    return kl::nolocale_wtof(towstr(s));
}



tango::ISetPtr makeTableFromDom(wxDOMNode& _node)
{
    std::vector<HtmlTableFieldInfo> fields;
    tango::IStructurePtr dest_struct;
    tango::ISetPtr dest_set;
    wxDOMNode node = _node;
    wxDOMNode table_node;
    size_t i, count;
    
    
    tango::IDatabasePtr dest_db = g_app->getDatabase();
    
    if (dest_db.isNull())
        return xcm::null;
    
    // first, find the table element
    while (1)
    {
        wxString node_name = node.GetNodeName();
        
        if (0 == node_name.CmpNoCase(wxT("TABLE")))
            break;
            
        node = node.GetParentNode();
        if (!node.IsOk())
            return xcm::null;
    }
    
    table_node = node;
    
    
    // first make sure we have the node that contains the TR entries
    wxDOMNodeList table_node_children = table_node.GetChildNodes();
    
    std::vector<wxDOMNode> row_nodes;
    
    
    count = table_node_children.GetLength();
    for (i = 0; i < count; ++i)
    {
        wxDOMNode node = table_node_children.Item(i);
        if (0 == node.GetNodeName().CmpNoCase(wxT("TBODY")))
        {
            wxDOMNodeList rows= node.GetChildNodes();

            size_t rown, row_count = rows.GetLength();
            for (rown = 0; rown < row_count; ++rown)
            {
                wxDOMNode row = rows.Item(rown);
                
                // only take TR's
                if (0 == row.GetNodeName().CmpNoCase(wxT("TR")))
                {
                    row_nodes.push_back(row);
                }
            }
        }
    }




    // measure the table's data, so we can create a data table of the right
    // size.  We will also measure the field types, to determine whether
    // certain fields should be numeric
    
    size_t rown, row_count = row_nodes.size();
    for (rown = 0; rown < row_count; ++rown)
    {
        wxDOMNode row = row_nodes[rown];
        
        // only take TR's
        if (0 != row.GetNodeName().CmpNoCase(wxT("TR")))
            continue;
        
        wxDOMNodeList col_nodes = row.GetChildNodes();
        size_t coln, col_count = col_nodes.GetLength();
        

        size_t td_count = 0;
        for (coln = 0; coln < col_count; ++coln)
        {
            wxDOMNode cell = col_nodes.Item(coln);
            
            // only take TD's
            if (0 != cell.GetNodeName().CmpNoCase(wxT("TD")))
                continue;
                
            // make sure our fields vector can fit all the columns
            td_count++;
            if (td_count >= fields.size())
                fields.resize(td_count);
                      
            HtmlTableFieldInfo& info = fields[td_count-1];
             
            wxString val;
            nodeToString(cell, val);

            // trim leading and trailing spaces before determining the type
            while (val.Length() > 0 && iswspace(val[0]))
                val.Remove(0, 1);
            while (val.Length() > 0 && iswspace(val.Last()))
                val.RemoveLast();

            if (!val.IsEmpty())
                info.empty = false;


            info.name = wxString::Format(wxT("FIELD%d"), td_count);
            
            
            if (info.type == tango::typeDate && !parseDelimitedStringDate(val))
            {
                if (rown > 0) // first row might/likely is a header, so don't count it as a fault
                    info.date_faults++;
                if (info.date_faults >= 2 || row_count <= 3)
                {
                    info.type = tango::typeNumeric;
                }
            }
            
            if (info.type == tango::typeNumeric && !isNumericValue(val))
            {      
                if (rown > 0) // first row might/likely is a header, so don't count it as a fault
                    info.numeric_faults++;
                if (info.numeric_faults >= 2 || row_count <= 3)
                {
                    info.type = tango::typeWideCharacter;
                }
            }
            
            // determine the number of decimal places in this cell value;
            // update the field's maximum # of decimal places
            if (info.type == tango::typeNumeric)
            {
                int dec_places = 0;
                double d = extractNumber(val, &dec_places);
                if (dec_places > info.max_dec)
                {
                    info.max_dec = dec_places;
                    if (info.max_dec > tango::max_numeric_scale)
                        info.max_dec = tango::max_numeric_scale;
                }
            }
            
            if (val.Length() > info.max_len)
                info.max_len = val.Length();
            
            if (info.type == tango::typeNumeric)
            {
                if (info.max_len > tango::max_numeric_width)
                    info.max_len = tango::max_numeric_width;
            }
            
            // fields should always be at least one character
            if (info.max_len <= 0)
                info.max_len = 1;
        }
    }
    
    
    // find table column headers, if any
    
    
    // lazy header is set to true below if the html code
    // uses regular <TR><TD>'s for field headers; normally
    // the html programmer should use THEAD
    bool lazy_header = false;

    wxDOMNode thead_node;
    wxDOMNode thead_row;
    
    count = table_node_children.GetLength();
    for (i = 0; i < count; ++i)
    {
        wxDOMNode node = table_node_children.Item(i);
        if (0 == node.GetNodeName().CmpNoCase(wxT("THEAD")))
        {
            thead_node = node;
            break;
        }
    }
    
    if (thead_node.IsOk())
    {
        // we have a THEAD -- look for a TR underneath
        wxDOMNodeList thead_children = thead_node.GetChildNodes();
        count = thead_children.GetLength();
        for (i = 0; i < count; ++i)
        {
            wxDOMNode node = thead_children.Item(i);
            if (0 == node.GetNodeName().CmpNoCase(wxT("TR")))
            {
                thead_row = node;
                break;
            }
        }
        
        if (!thead_row.IsOk())
        {
            thead_node = wxDOMNode();
        }
    }
    
    if (!thead_node.IsOk())
    {
        std::vector<wxString> first_row_values;
        
        // there is no THEAD, find TH's in the table rows
        // find the row with the most TH's in it
        
        size_t thead_max_cell_count = 0;
        
        for (rown = 0; rown < row_count; ++rown)
        {
            wxDOMNode row = row_nodes[rown];
        
            // only take TR's
            if (0 != row.GetNodeName().CmpNoCase(wxT("TR")))
                continue;
            
            wxDOMNodeList col_nodes = row.GetChildNodes();
            size_t coln, col_count = col_nodes.GetLength();
            for (coln = 0; coln < col_count; ++coln)
            {
                wxDOMNode cell = col_nodes.Item(coln);
                if (0 == cell.GetNodeName().CmpNoCase(wxT("TH")))
                {
                    if (thead_max_cell_count < col_count)
                    {
                        thead_row = row;
                        thead_max_cell_count = col_count;
                    }
                    break;
                }
            }
        }
        
        
        if (!thead_row.IsOk())
        {
            // no headers (THs) -- check if perhaps it could be
            // a lazy table header (headers in the first row using TR)
            
            std::vector<wxString> first_row_values;
            bool first_row_data_captured = false;
            wxDOMNode row;
            
            // get the first row
            for (rown = 0; rown < row_count; ++rown)
            {
                wxDOMNode test_row = row_nodes[rown];
            
                // only take TR's
                if (0 != test_row.GetNodeName().CmpNoCase(wxT("TR")))
                    continue;
                row = test_row;
                break;
            }
            
            if (row.IsOk())
            {      
                wxDOMNodeList col_nodes = row.GetChildNodes();
                size_t coln, col_count = col_nodes.GetLength();
                for (coln = 0; coln < col_count; ++coln)
                {
                    wxDOMNode cell = col_nodes.Item(coln);
                    wxString node_name = cell.GetNodeName();
                    
                    if (0 == node_name.CmpNoCase(wxT("TD")))
                    {
                        wxString str;
                        nodeToString(cell, str);
                        first_row_values.push_back(str);
                    }
                }
            }
                
            
            size_t i = 0, field_count = fields.size();
            std::vector<wxString>::iterator it;
            for (it = first_row_values.begin(); it != first_row_values.end(); ++it, ++i)
            {
                if (i >= field_count)
                    break;
                    
                if (fields[i].type == tango::typeNumeric)
                {
                    if (!isNumericValue(*it))
                    {
                        lazy_header = true;
                        thead_row = row;
                        break;
                    }
                }
                
                if (fields[i].type == tango::typeDate)
                {
                    if (!parseDelimitedStringDate(*it))
                    {
                        lazy_header = true;
                        thead_row = row;
                        break;
                    }
                }
            }     
        }
    }
    
    
    if (thead_row.IsOk())
    {
        // we have row headers, let's extract them into our
        // output field structure
        size_t header_count = 0;
        
        wxDOMNodeList col_nodes = thead_row.GetChildNodes();
        size_t coln, col_count = col_nodes.GetLength();
        for (coln = 0; coln < col_count; ++coln)
        {
            wxDOMNode cell = col_nodes.Item(coln);
            
            if (0 != cell.GetNodeName().CmpNoCase(lazy_header ? wxT("TD") : wxT("TH")))
                continue;
            
            if (header_count < fields.size())
            {
                wxString s;
                nodeToString(cell, s);
                
                // this is the same as trim, but better because
                // iswspace catches more blank characters
                while (s.Length() > 0 && iswspace(s[0]))
                    s.Remove(0, 1);
                while (s.Length() > 0 && iswspace(s.Last()))
                    s.RemoveLast();
                    
                size_t i, l = s.Length();
                for (i = 0; i < l; ++i)
                {
                    if (iswspace(s[i]))
                        s.SetChar(i, wxT('_'));
                }
                
                // for now, make the field name more 'usable'

                s.Replace(wxT(" "), wxT("_"));
                s.Replace(wxT("\r"), wxT("_"));
                s.Replace(wxT("\n"), wxT("_"));
                s.Replace(wxT(","), wxT("_"));
                s.Replace(wxT("."), wxT("_"));
                s.Replace(wxT("("), wxT("_"));
                s.Replace(wxT(")"), wxT("_"));
                s.Replace(wxT("'"), wxT("_"));
                s.Replace(wxT(":"), wxT("_"));
                s.Replace(wxT("\""), wxT("_"));
                s.Replace(wxT("-"), wxT("_"));
                s.Replace(wxT("+"), wxT("_"));
                s.Replace(wxT("/"), wxT("_"));
                s.Replace(wxT("*"), wxT("_"));
                s.Replace(wxT("%"), wxT("_"));
                
                // if first digit is number, prepend an underscore
                if (s.Length() > 0 && ::iswdigit(s[0]))
                    s.Prepend(wxT("_"));
                              
                // get rid of duplicate __
                while (s.Find(wxT("__")) != -1)
                    s.Replace(wxT("__"), wxT("_"));
   
                fields[header_count].name = s;
            }
            
            header_count++;
        }
    }
    
    
    // make sure there are no empty field names
    
    count = fields.size();
    for (i = 0; i < count; ++i)
    {
        fields[i].name.Trim(false);
        fields[i].name.Trim(true);
        
        if (fields[i].name.IsEmpty())
        {
            fields[i].name = wxString::Format(wxT("FIELD%d"), i+1);
        } 
    }
    
    // make sure there are no super-long field names
    for (i = 0; i < count; ++i)
    {
        if (fields[i].name.Length() > 75)
            fields[i].name = fields[i].name.Left(75);
    }
    
    // fix up field names that are key words
    
    std::set<wxString> keywords;
    tango::IAttributesPtr attr = dest_db->getAttributes();
    wxStringTokenizer t(towx(attr->getStringAttribute(tango::dbattrKeywords)), wxT(","));
    while (t.HasMoreTokens())
    {
        wxString s = t.GetNextToken();
        s.Trim();
        s.Trim(FALSE);
        s.MakeUpper();
        keywords.insert(s);
    }
    
    for (i = 0; i < count; ++i)
    {
        wxString s = fields[i].name;
        s.MakeUpper();
        if (keywords.find(s) != keywords.end())
            fields[i].name += wxT("_");
    }
    
    // make sure there are no duplicated field names
    std::set<wxString> dup_check;
    for (i = 0; i < count; ++i)
    {
        wxString base_name = fields[i].name;
        wxString new_name = base_name;
        
        wxString s = new_name;
        s.MakeUpper();
        int cnt = 1;
        while (dup_check.find(s) != dup_check.end())
        {
            new_name = wxString::Format(wxT("%s%d"), base_name.c_str(), ++cnt);
            s = new_name;
            s.MakeUpper();
        }
        
        dup_check.insert(s);
        fields[i].name = new_name;
    }
    
    
    // force empty fields to have a character type
    for (i = 0; i < count; ++i)
    {
        if (fields[i].empty)
        {
            fields[i].type = tango::typeWideCharacter;
            fields[i].max_len = 1;
            fields[i].max_dec = 0;
        }
    } 
    
    
       
    // create the output data table
    
    dest_struct = dest_db->createStructure();

                                           
    std::vector<HtmlTableFieldInfo>::iterator field_it;
    for (field_it = fields.begin(); field_it != fields.end(); ++field_it)
    {
        tango::IColumnInfoPtr col_info = dest_struct->createColumn();
        col_info->setName(towstr(field_it->name));
        col_info->setType(field_it->type);
        col_info->setWidth(field_it->max_len);
        col_info->setScale(field_it->type == tango::typeNumeric ? field_it->max_dec : 0);
    }

    dest_set = dest_db->createSet(L"", dest_struct, NULL);
    
    if (dest_set.isNull())
        return xcm::null;
    
    tango::IRowInserterPtr row_inserter = dest_set->getRowInserter();
    if (row_inserter.isNull())
        return xcm::null;
        
    row_inserter->startInsert(L"");
                                       
    for (field_it = fields.begin(); field_it != fields.end(); ++field_it)
    {
        field_it->handle = row_inserter->getHandle(towstr(field_it->name));
        if (!field_it->handle)
            return xcm::null;
    }
    
    // populate the data table rows from the html table
    
    bool first_row = true;
    
    for (rown = 0; rown < row_count; ++rown)
    {
        wxDOMNode row = row_nodes[rown];
        
        // only take TR's
        if (0 != row.GetNodeName().CmpNoCase(wxT("TR")))
            continue;
        
        // lazy header tables must have the first row skipped
        // as this is the header row
        if (first_row)
        {
            first_row = false;
            if (lazy_header)
                continue;
        }
        
        // assume that the row is empty
        bool row_empty = true;
        
         
        wxDOMNodeList col_nodes = row.GetChildNodes();
        size_t coln, col_count = col_nodes.GetLength();

        size_t td_count = 0;
        for (coln = 0; coln < col_count; ++coln)
        {
            wxDOMNode cell = col_nodes.Item(coln);
            
            
            // skip rows with header elements (TH)
            if (0 == cell.GetNodeName().CmpNoCase(wxT("TH")))
            {
                td_count = 0;
                break;
            }
            
            // only take TD's
            if (0 != cell.GetNodeName().CmpNoCase(wxT("TD")))
                continue;
            
            // skip any cell with a rowspan attribute
            wxDOMNamedNodeMap attr = cell.GetAttributes();
            wxDOMNode rowspan_node = attr.GetNamedItem(wxT("rowspan"));
            if (rowspan_node.IsOk())
                continue;
            
            
            
            td_count++;

            HtmlTableFieldInfo& info = fields[td_count-1];
            
            wxString val;
            nodeToString(cell, val);

            // next, trim leading and trailing spaces; in a strict conversion
            // from a database system, we wouldn't necessarily want to remove
            // padding; however, in the case of an HTML table, spaces/tabs are
            // sometimes used to format the values and adjust the spacing,
            // leaving large amounts of space after the table is imported and 
            // making the data less immediately useful without this step; note: 
            // the following is the same as trim, but better because iswspace 
            // catches more blank characters
            while (val.Length() > 0 && iswspace(val[0]))
                val.Remove(0, 1);
            while (val.Length() > 0 && iswspace(val.Last()))
                val.RemoveLast();


            if (!val.IsEmpty())
                row_empty = false;
                
            switch (info.type)
            {
                case tango::typeNumeric:
                {
                    row_inserter->putDouble(info.handle, extractNumber(val));
                    break;
                }
                
                case tango::typeDate:
                case tango::typeDateTime:
                {
                    int y = 0, m = 0, d = 0;
                    if (parseDelimitedStringDate(val, &y, &m, &d))
                    {
                        tango::DateTime dt(y, m, d);
                        row_inserter->putDateTime(info.handle, dt);
                    }
                    break;
                }
                
                default:
                {
                    row_inserter->putWideString(info.handle, towstr(val));
                    break;
                }

            }
        }
        
        
        // write out the row, if there were TD entries; if there
        // were no TD entries in the row, don't write it out
        if (td_count > 0 && !row_empty)
        {
            if (!row_inserter->insertRow())
                break;
        }
         else
        {
            // clear out write buffer
            for (field_it = fields.begin(); field_it != fields.end(); ++field_it)
            {
                if (field_it->type == tango::typeNumeric)
                    row_inserter->putDouble(field_it->handle, 0.0);
                     else
                    row_inserter->putWideString(field_it->handle, L"");
            }
        }
    }
    
    
    row_inserter->finishInsert();
    
    return dest_set;
}











static FeedContentHandler* g_feed_content_handler = NULL;
static AppExtensionContentHandler* g_app_extension_content_handler = NULL;



BEGIN_EVENT_TABLE(WebDoc, wxWindow)
    EVT_MENU(ID_Edit_Cut, WebDoc::onCutSelection)
    EVT_MENU(ID_Edit_Copy, WebDoc::onCopySelection)
    EVT_MENU(ID_Edit_CopyLink, WebDoc::onCopyLinkLocation)
    EVT_MENU(ID_Edit_Paste, WebDoc::onPaste)
    EVT_MENU(ID_Edit_SelectAll, WebDoc::onSelectAll)
    EVT_MENU(ID_Edit_Delete, WebDoc::onDelete)
    EVT_MENU(ID_File_Back, WebDoc::onGoBackward)
    EVT_MENU(ID_File_Forward, WebDoc::onGoForward)
    EVT_MENU(ID_File_Reload, WebDoc::onReload)
    EVT_MENU(ID_File_Stop, WebDoc::onStop)
    EVT_MENU(ID_File_SaveAsExternal, WebDoc::onSaveAsExternal)
    EVT_MENU(ID_File_Print, WebDoc::onPrint)
    EVT_MENU(ID_File_PageSetup, WebDoc::onPageSetup)
    EVT_MENU(ID_View_ZoomIn, WebDoc::onZoomIn)
    EVT_MENU(ID_View_ZoomOut, WebDoc::onZoomOut)
    EVT_MENU(ID_View_ZoomToActual, WebDoc::onZoomToActual)
    EVT_MENU(ID_View_ZoomToWidth, WebDoc::onZoomToActual)
    EVT_MENU(ID_View_ZoomToFit, WebDoc::onZoomToActual)
    EVT_SIZE(WebDoc::onSize)
    EVT_WEB_STATUSTEXT(wxID_WEB, WebDoc::onStatusChange)
    EVT_WEB_STATUSCHANGE(wxID_WEB, WebDoc::onStatusChange)
    EVT_WEB_STATECHANGE(wxID_WEB, WebDoc::onStateChange)
    EVT_WEB_LOCATIONCHANGE(wxID_WEB, WebDoc::onLocationChange)
    EVT_WEB_TITLECHANGE(wxID_WEB, WebDoc::onTitleChange)
    EVT_WEB_SHOWCONTEXTMENU(wxID_WEB, WebDoc::onShowContextMenu)
    EVT_WEB_LEFTUP(wxID_WEB, WebDoc::onLeftUp)
    EVT_WEB_MIDDLEUP(wxID_WEB, WebDoc::onMiddleUp)
    EVT_WEB_CREATEBROWSER(wxID_WEB, WebDoc::onCreateBrowserRequest)
    EVT_WEB_INITDOWNLOAD(wxID_WEB, WebDoc::onInitDownload)
    EVT_WEB_SHOULDHANDLECONTENT(wxID_WEB, WebDoc::onShouldHandleContent)
    EVT_WEB_OPENURI(wxID_WEB, WebDoc::onOpenURI)
    EVT_WEB_FAVICONAVAILABLE(wxID_WEB, WebDoc::onFavIconAvailable)
    EVT_WEB_DOMCONTENTLOADED(wxID_WEB, WebDoc::onDOMContentLoaded)
    EVT_WEB_DRAGDROP(wxID_WEB, WebDoc::onFsDataDropped)
    
    // for wxWebView alternative component
    EVT_WEB_VIEW_LOADED(wxID_WEBVIEW, WebDoc::onWebViewDocumentLoaded)
    EVT_WEB_VIEW_TITLE_CHANGED(wxID_WEBVIEW, WebDoc::onWebViewTitleChanged)

    // disable the data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, WebDoc::onUpdateUI_DisableAlways)

    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, WebDoc::onUpdateUI_DisableAlways)
    
    // disable format items
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, WebDoc::onUpdateUI_DisableAlways)

    // disable table items
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, WebDoc::onUpdateUI_DisableAlways)

    // disable the scale-related zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, WebDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, WebDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, WebDoc::onUpdateUI_DisableAlways) 

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_Save, WebDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_SaveAs, WebDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Run, WebDoc::onUpdateUI_DisableAlways)

    // enable/disable some of the file items based on conditions
    EVT_UPDATE_UI(ID_File_Reload, WebDoc::onUpdateUI)
    EVT_UPDATE_UI(ID_File_Stop, WebDoc::onUpdateUI)
    
    // enable/disable these item based on if we're already viewing the source
    EVT_UPDATE_UI(ID_ImportTable, WebDoc::onUpdateUI)
    EVT_UPDATE_UI(ID_ViewPageSource, WebDoc::onUpdateUI)

    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, WebDoc::onUpdateUI)

END_EVENT_TABLE()




WebDoc::WebDoc()
{
    m_webcontrol = NULL;
    m_webview = NULL;
    m_web = NULL;
}

WebDoc::~WebDoc()
{
}


void WebDoc::setLastClickedBookmarkPath(const wxString& path)
{
    m_last_clicked_bookmark_path = path;
}

bool WebDoc::downloadFile(const wxString& url,
                          const wxString& filename,
                          cfw::IJobInfoPtr job_info)
{
    DownloadProgressListener* listener = new DownloadProgressListener(job_info);

    return wxWebControl::SaveRequest(url, filename, NULL, listener);
}



bool WebDoc::initDoc(cfw::IFramePtr frame,
                     cfw::IDocumentSitePtr doc_site,
                     wxWindow* docsite_wnd,
                     wxWindow* panesite_wnd)
{
    // create document window
    bool result = Create(docsite_wnd,
                         doc_site,
                         wxPoint(0,0),
                         docsite_wnd->GetClientSize(),
                         wxNO_BORDER |
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN);

    if (!result)
    {
        return false;
    }

    m_frame = frame;
    m_site_id = doc_site->getId();

    frame->sigFrameEvent().connect(this, &WebDoc::onFrameEvent);

    // set the caption
    doc_site->setCaption(_("(Untitled)"));
    doc_site->setBitmap(GETBMP(gf_document_16));
    m_doc_site = doc_site;
    
    m_bitmap_updater.setDocSite(m_doc_site);
    
    // create the web component itself
    if (wxWebControl::IsEngineOk())
    {
        m_webcontrol = new wxWebControl(this, wxID_WEB, wxPoint(0,0), docsite_wnd->GetClientSize());
        m_web = m_webcontrol;

        if (!g_app_extension_content_handler)
        {
            //g_delimited_text_content_handler = new DelimitedTextContentHandler;
            g_feed_content_handler = new FeedContentHandler;
            g_app_extension_content_handler = new AppExtensionContentHandler;
        
            //wxWebControl::AddContentHandler(g_delimited_text_content_handler, true);
            wxWebControl::AddContentHandler(g_feed_content_handler, true);
            wxWebControl::AddContentHandler(g_app_extension_content_handler, true);
        }
    }
     else
    {

#ifdef __WXMSW__

        // on windows, we want to be using at least IE version 9.0 or later.  By default
        // wxWebView will use IE 7.  We need to set the following registry value to
        // make wxWebView use the newer embedded browser components

        static bool first_time = true;
        if (first_time)
        {
            first_time = false;
         
            wxRegKey* pRegKey = new wxRegKey(wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION"));

            if (!pRegKey->Exists())
            {
                pRegKey->Create();
            }

            wxStandardPaths& stdpaths = wxStandardPaths::Get();
            wxString prog = stdpaths.GetExecutablePath();
            prog = prog.AfterLast(PATH_SEPARATOR_CHAR);
            pRegKey->SetValue(prog, (long)9000);

            delete pRegKey;
        }
#endif

        m_webview = wxWebView::New(this, wxID_WEBVIEW, wxWebViewDefaultURLStr, wxPoint(0,0), docsite_wnd->GetClientSize(), wxWEB_VIEW_BACKEND_DEFAULT, wxBORDER_NONE);
        m_web = m_webview;
    }


    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_web, 1, wxEXPAND);
    SetSizer(main_sizer);
    Layout();
    
    return true;
}

wxString WebDoc::getDocumentTitle()
{
    return m_title;
}

wxString WebDoc::getDocumentLocation()
{
    return m_url;
}

wxWindow* WebDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void WebDoc::setDocumentFocus()
{
    m_web->SetFocus();
}

static void clearStatusBarTextItem()
{
    // clear out statusbar text
    cfw::IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
    cfw::IStatusBarItemPtr item = statusbar->getItem(wxT("app_statusbar_text"));
    if (item.isNull())
        return;

    // doing the following will ensure that the item's dirty flag
    // is set -- if we don't do this, there is a chance that the
    // item will not be refreshed because it is not dirty
    wxString val = item->getValue();
    item->setValue(val + wxT("dirty"));
    item->setValue(wxEmptyString);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}

bool WebDoc::onSiteClosing(bool force)
{
    clearStatusBarTextItem();
    return true;
}

bool WebDoc::isWebBrowserOk() const
{
    if (m_webcontrol)
    {
        return m_webcontrol->IsOk();
    }
     else
    {
        return m_webview ? true : false;
    }
}

bool WebDoc::findNextMatch(const wxString& text,
                           bool forward,
                           bool match_case,
                           bool whole)
{
    unsigned int flags = 0;
    
    if (!forward)
        flags |= wxWEB_FIND_BACKWARDS;
    if (match_case)
        flags |= wxWEB_FIND_MATCH_CASE;
    if (whole)
        flags |= wxWEB_FIND_ENTIRE_WORD;
        
    flags |= wxWEB_FIND_WRAP;
    flags |= wxWEB_FIND_SEARCH_FRAMES;
    
    if (m_webcontrol)
    {
        return m_webcontrol->Find(text, flags);
    }
     else if (m_webview)
    {
        return false;
    }
     else
    {
        return false;
    }
}

bool WebDoc::findReplaceWith(const wxString& find_val,
                             const wxString& replace_val,
                             bool forward,
                             bool match_case,
                             bool whole)
{
    if (!findIsReplaceAllowed())
        return false;
    
    return false;
}

bool WebDoc::findReplaceAll(const wxString& find_val,
                            const wxString& replace_val,
                            bool match_case,
                            bool whole)
{
    if (!findIsReplaceAllowed())
        return false;
    
    return false;
}

bool WebDoc::findIsReplaceAllowed()
{
    return false;
}



void WebDoc::onFrameEvent(cfw::Event& evt)
{
    if (evt.name == wxT("appmain.view_switcher.query_available_views"))
    {
        // make sure we are the same site
        if (g_app->getMainFrame()->getActiveChild() != m_doc_site)
            return;

        evt.name = wxT("");
        
        // populate the view switcher list with the available views
        ViewSwitcherList* list = (ViewSwitcherList*)(evt.o_param);
        
        bool is_source = isViewingSource();

        list->addItem(ID_View_SwitchToSourceView, _("Source View"),
                      is_source ? true : false);
        
        list->addItem(ID_View_SwitchToLayoutView, _("Web Page View"),
                      is_source ? false : true);
    }
     else if (evt.name == wxT("appmain.view_switcher.active_view_changed"))
    {
        int id = (int)(evt.l_param);
        
        // make sure we are the same site
        if (g_app->getMainFrame()->getActiveChild() != m_doc_site)
            return;
        
        // make the event only fire once
        evt.name = wxT("");
        
        switch (id)
        {
            case ID_View_SwitchToLayoutView:
                switchToWeb();
                break;
                
            case ID_View_SwitchToSourceView:
                switchToSource();
                break;
        }
    }
}

void WebDoc::onFsDataDropped(wxWebEvent& evt)
{
    if (!m_webcontrol)
        return;

    // for now, assume that the dragging was done from the tree --
    // The drag drop event was initialized by gecko, so we'll
    // need to work harder to be able to get the data object
    // where the drag was initialized from
    
    wxDOMDocument doc = m_webcontrol->GetDOMDocument();
    wxDOMElement element = doc.GetElementById(wxT("app-server-tag"));
    if (!element.IsOk())
    {
        // site is not an application server
        return;
    }
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;
    

    wxString request_url = m_webcontrol->GetCurrentURI();
    size_t loc = request_url.find(wxT("://"));
    if (loc == request_url.npos)
        return;
        
    loc = request_url.find(wxT("/"), loc+3);
    if (loc != request_url.npos)
        request_url = request_url.substr(0, loc);
    request_url += wxT("/process?action=addsource");
    
    
    





    DbDoc* dbdoc = g_app->getDbDoc();
    if (!dbdoc)
        return;
        
    cfw::IFsItemEnumPtr items = dbdoc->getFsPanel()->getSelectedItems();
    
    wxString table_to_load;
    
    size_t i, count = items->size();
    for (i = 0; i < count; ++i)
    {
        cfw::IFsItemPtr item = items->getItem(i);
        wxString path = DbDoc::getFsItemPath(item);
        
        wxString cstr, rpath;
        if (!getMountPointHelper(db, path, cstr, rpath))
            continue;
        
        
        wxWebPostData post_data;
        post_data.Add(wxT("server_ip"), wxT("localhost"));
        post_data.Add(wxT("server_port"), wxT("80"));
        post_data.Add(wxT("connection_str"), cstr);
        post_data.Add(wxT("remote_path"), rpath);
        post_data.Add(wxT("object_name"), item->getLabel());

        wxWebControl::SaveRequestToString(request_url, NULL, &post_data);
                
        table_to_load = item->getLabel();
    }
    
    if (table_to_load.Length() > 0)
        m_webcontrol->Execute(wxString::Format(wxT("clicallLoadTable('%s');"), (const wxChar*)table_to_load.c_str()));
}


bool WebDoc::isViewingSource() const
{
    return (m_url.Left(12) == wxT("view-source:") ? true : false);
}

void WebDoc::switchToWeb()
{
    cfw::IDocumentSitePtr other_site;
    other_site = lookupOtherDocumentSite(
                            m_doc_site,
                            "appmain.WebDoc");
                            
    if (other_site.isNull() || other_site == m_doc_site)
    {
        wxString regular_url = m_url;
        if (isViewingSource())
            regular_url.Remove(0, 12);

        WebDoc* doc = new WebDoc;
        g_app->getMainFrame()->createSite(m_doc_site->getContainerWindow(),
                                          doc,
                                          true);
        doc->openURI(regular_url);
    }
     else
    {
        switchToOtherDocument(m_doc_site, "appmain.WebDoc");
    }
}

void WebDoc::switchToSource()
{
    // if we're already viewing the source, do nothing
    if (isViewingSource())
        return;
        
    wxString source_url = wxT("view-source:") + m_url;
    
    cfw::IDocumentSitePtr other_site;
    other_site = lookupOtherDocumentSite(
                            m_doc_site,
                            "appmain.WebDoc");
                            
    if (other_site.isNull() || other_site == m_doc_site)
    {
        WebDoc* doc = new WebDoc;

        g_app->getMainFrame()->createSite(m_doc_site->getContainerWindow(),
                                          doc,
                                          true);
                                          
        //doc->openURI(source_url);
        doc->m_webcontrol->ViewSource(m_webcontrol);
    }
     else
    {
        if (other_site.isOk())
        {
            IWebDocPtr web_doc = other_site->getDocument();
            if (web_doc)
            {
                //web_doc->openURI(source_url);
                web_doc->getWebControl()->ViewSource(m_webcontrol);
                
            }
            
            switchToOtherDocument(m_doc_site, "appmain.WebDoc");
        }
    }
}

void WebDoc::savePageAsExternal()
{
    if (!m_webcontrol)
        return;

    wxString filter;

    // NOTE: if you add or remove items from this
    // list, make sure you adjust the case statement
    // below, because it affects which file type the
    // target will be
    filter += _("All Files");
    filter += wxT(" (*.*)|*.*|");  // some html pages simply contain images
    filter += _("HTML Files");
    filter += wxT(" (*.html, *.htm)|*.html;*.htm|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxString filename = getFilenameFromPath(m_url, true);   // include extension
    wxFileDialog dlg(g_app->getMainWindow(),
                     _("Save As"),
                     wxT(""),
                     filename,
                     filter,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() != wxID_OK)
        return;

    m_webcontrol->SaveCurrent(dlg.GetPath());
}

// web doc api

void WebDoc::openURI(const wxString& uri, wxWebPostData* post_data)
{
    if (m_webview)
    {
        m_url = uri;

        if (m_frame)
        {
            cfw::IDocumentSitePtr doc_site = m_frame->getActiveChild();
            if (doc_site.isOk() && doc_site == m_doc_site)
            {
                m_frame->postEvent(new cfw::Event(wxT("cfw.locationChanged")));
            }
        }

        m_webview->LoadURL(uri);
        return;
    }


    // don't allow thse locations to be loaded -- most restricted locations are handled
    // in WebDoc::onOpenURI(), however, by the time these locations get to that function,
    // they've already been processed by the Gecko engine and translated to something like
    // "jar:file:///d:/devsrc/trunk/xr/chrome/toolkit.jar!/content/global/plugins.html"
    if (uri == wxT("about:plugins") || uri == wxT("about:credits"))
    {
        m_webcontrol->OpenURI(wxT("about:blank"), wxWEB_LOAD_LINKCLICK, post_data);
        return;
    }
    


    if (isViewingSource())
    {
        cfw::IDocumentSitePtr other_site;
        other_site = lookupOtherDocumentSite(
                                m_doc_site,
                                "appmain.WebDoc");
        
        if (other_site.isOk())
        {
            IWebDocPtr web_doc = other_site->getDocument();
            if (web_doc)
            {
                web_doc->getWebControl()->OpenURI(uri);
                switchToOtherDocument(m_doc_site, "appmain.WebDoc");
                return;
            }
        }
    }
    
    m_webcontrol->OpenURI(uri, wxWEB_LOAD_LINKCLICK, post_data);
    
    if (uri.Contains(wxT("webres.jar!/about.html")) ||
        uri.Contains(wxT("webres.jar!/license.html")))
    {
        // ripped from WebBrowser::waitUntilReady()

        wxStopWatch sw;
        sw.Start();
        
        // wait for the page to load
        while (!m_webcontrol->IsContentLoaded())
        {
            wxThread::Sleep(100);
            ::wxSafeYield();
            
            // 5-second timeout (these are local files...)
            if (sw.Time() > 5000)
                return;
        }
        
        // create strings for DOM injection
        
        wxString app_name = APPLICATION_NAME;

        wxString title_str;
        if (uri.Contains(wxT("webres.jar!/about.html")))
            title_str = wxString::Format(wxT("About %s"), app_name.c_str());
             else if (uri.Contains(wxT("webres.jar!/license.html")))
            title_str = wxString::Format(wxT("%s License"), app_name.c_str());
        
        wxString version_str = wxString::Format(wxT("Version %d.%d"),
                                                    APP_VERSION_MAJOR,
                                                    APP_VERSION_MINOR);
        
        wxString build_str = wxString::Format(wxT("%s %d.%d.%d (build %d)"),
                                                    app_name.c_str(),
                                                    APP_VERSION_MAJOR,
                                                    APP_VERSION_MINOR,
                                                    APP_VERSION_SUBMINOR,
                                                    APP_VERSION_BUILDSERIAL);
        
        // inject information into the about and license pages
        
        wxDOMDocument domdoc = m_webcontrol->GetDOMDocument();
        if (domdoc.IsOk())
        {
            wxDOMElement e1 = domdoc.GetElementById(wxT("title"));
            if (e1.IsOk())
            {
                wxDOMText t = e1.GetFirstChild();
                if (t.IsOk())
                    t.SetData(title_str);
            }
            
            wxDOMElement e2 = domdoc.GetElementById(wxT("app-name"));
            if (e2.IsOk())
            {
                wxDOMText t = e2.GetFirstChild();
                if (t.IsOk())
                    t.SetData(app_name);
            }
            
            wxDOMElement e3 = domdoc.GetElementById(wxT("version"));
            if (e3.IsOk())
            {
                wxDOMText t = e3.GetFirstChild();
                if (t.IsOk())
                    t.SetData(version_str);
            }
            
            wxDOMElement e4 = domdoc.GetElementById(wxT("build"));
            if (e4.IsOk())
            {
                wxDOMText t = e4.GetFirstChild();
                if (t.IsOk())
                    t.SetData(build_str);
            }
            
            wxDOMElement e5 = domdoc.GetElementById(wxT("xul-disclaimer-app-name"));
            if (e5.IsOk())
            {
                wxDOMText t = e5.GetFirstChild();
                if (t.IsOk())
                    t.SetData(app_name);
            }
            
            m_doc_site->setCaption(title_str);
        }
    }
}

wxWebControl* WebDoc::getWebControl()
{
    return m_webcontrol;
}


// web doc events

void WebDoc::onSaveAsExternal(wxCommandEvent& evt)
{
    savePageAsExternal();
}

void WebDoc::onPrint(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->Print();
    }
     else
    {
        m_webview->Print();
    }
}

void WebDoc::onPageSetup(wxCommandEvent& evt)
{
    // get the current page settings
    double page_width = 0;
    double page_height = 0;
    double left_margin = 0;
    double right_margin = 0;
    double top_margin = 0;
    double bottom_margin = 0;

    m_webcontrol->GetPageSettings(&page_width, &page_height,
                                  &left_margin, &right_margin, &top_margin, &bottom_margin);

    PageSetupDialog dlg(this);
    dlg.setPageDimensions(page_width, page_height);
    dlg.setMargins(left_margin,
                   right_margin,
                   top_margin,
                   bottom_margin);

    // set the new page settings
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.getPageDimensions(&page_width, &page_height);
        dlg.getMargins(&left_margin,
                       &right_margin,
                       &top_margin,
                       &bottom_margin);

        m_webcontrol->SetPageSettings(page_width, page_height,
                                      left_margin, right_margin, top_margin, bottom_margin);
    }

    setDocumentFocus();
}

void WebDoc::onCutSelection(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->CutSelection();
    }
     else if (m_webview)
    {
        m_webview->Cut();
    }
}

void WebDoc::onCopySelection(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->CopySelection();
    }
     else if (m_webview)
    {
        m_webview->Copy();
    }
}

void WebDoc::onCopyLinkLocation(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->CopyLinkLocation();
    }
}

void WebDoc::onPaste(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->Paste();
    }
     else if (m_webview)
    {
        m_webview->Paste();
    }
}

void WebDoc::onDelete(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->CutSelection();
    }
     else if (m_webview)
    {
        m_webview->DeleteSelection();
    }
}

void WebDoc::onSelectAll(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->SelectAll();
    }
     else if (m_webview)
    {
        m_webview->SelectAll();
    }
}

void WebDoc::onGoBackward(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->GoBack();
    }
     else if (m_webview)
    {
        m_webview->GoBack();
    }
}

void WebDoc::onGoForward(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->GoForward();
    }
     else if (m_webview)
    {
        m_webview->GoForward();
    }
}

void WebDoc::onReload(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->Reload();
    }
     else if (m_webview)
    {
        m_webview->Reload();
    }
}

void WebDoc::onStop(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->Stop();
    }
     else if (m_webview)
    {
        m_webview->Stop();
    }
}

void WebDoc::onZoomIn(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        float zoom;
        m_webcontrol->GetTextZoom(&zoom);
        m_webcontrol->SetTextZoom(zoom + ZOOM_CHANGE_PERC);
    }
     else if (m_webview)
    {
        switch (m_webview->GetZoom())
        {
            case wxWEB_VIEW_ZOOM_TINY:      m_webview->SetZoom(wxWEB_VIEW_ZOOM_SMALL);   break;
            case wxWEB_VIEW_ZOOM_SMALL:     m_webview->SetZoom(wxWEB_VIEW_ZOOM_MEDIUM);  break;
            default:
            case wxWEB_VIEW_ZOOM_MEDIUM:    m_webview->SetZoom(wxWEB_VIEW_ZOOM_LARGE);   break;
            case wxWEB_VIEW_ZOOM_LARGE:	    m_webview->SetZoom(wxWEB_VIEW_ZOOM_LARGEST); break;
            case wxWEB_VIEW_ZOOM_LARGEST:   break;
        }
    }
}

void WebDoc::onZoomOut(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        float zoom;
        m_webcontrol->GetTextZoom(&zoom);
        m_webcontrol->SetTextZoom(zoom - ZOOM_CHANGE_PERC);
    }
     else if (m_webview)
    {
        switch (m_webview->GetZoom())
        {
            case wxWEB_VIEW_ZOOM_TINY:      break;
            case wxWEB_VIEW_ZOOM_SMALL:     m_webview->SetZoom(wxWEB_VIEW_ZOOM_TINY);  break;
            default:
            case wxWEB_VIEW_ZOOM_MEDIUM:    m_webview->SetZoom(wxWEB_VIEW_ZOOM_SMALL);  break;
            case wxWEB_VIEW_ZOOM_LARGE:	    m_webview->SetZoom(wxWEB_VIEW_ZOOM_MEDIUM); break;
            case wxWEB_VIEW_ZOOM_LARGEST:   m_webview->SetZoom(wxWEB_VIEW_ZOOM_LARGE);  break;
        }
    }
}

void WebDoc::onZoomToActual(wxCommandEvent& evt)
{
    if (m_webcontrol)
    {
        m_webcontrol->SetTextZoom(ZOOM_DEFAULT);
    } 
     else
    {
        m_webview->SetZoom(wxWEB_VIEW_ZOOM_MEDIUM);
    }
}

void WebDoc::onSize(wxSizeEvent& evt)
{
    Layout();
}


void WebDoc::onStatusChange(wxWebEvent& evt)
{
    // wxEVT_WEB_STATUSTEXT is received when somebody hovers
    // the mouse over a link and the status text should
    // be updated
    
    // wxEVT_WEB_STATUSCHANGE is received when the status text
    // changes when a web page is loading

    cfw::IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
    cfw::IStatusBarItemPtr item = statusbar->getItem(wxT("app_statusbar_text"));
    if (item.isNull())
        return;

    // set the value for the cell
    item->setValue(evt.GetString());

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}




void WebDoc::onStateChange(wxWebEvent& evt)
{
    int state = evt.GetState();
    
    if ((state & wxWEB_STATE_START) && (state & wxWEB_STATE_IS_WINDOW))
        m_bitmap_updater.start();
    
    if ((state & wxWEB_STATE_STOP) && (state & wxWEB_STATE_IS_WINDOW))
    {
        m_bitmap_updater.stop();
    }
    
    // we need to do this here since the last call to
    // OnStatusChange() doesn't contain an empty string
    // and we don't want "stuck" text in the statusbar
    if ((state & wxWEB_STATE_STOP) && (state & wxWEB_STATE_IS_REQUEST))
        clearStatusBarTextItem();
}

void WebDoc::onLocationChange(wxWebEvent& evt)
{
        
    m_url = evt.GetString();
    
    if (m_url == wxT("about:blank"))
        m_url = wxT("");
        
    if (m_frame)
    {
        cfw::IDocumentSitePtr doc_site = m_frame->getActiveChild();
        if (doc_site.isOk() && doc_site == m_doc_site)
        {
            m_frame->postEvent(new cfw::Event(wxT("cfw.locationChanged")));
        }
    }
}


void WebDoc::onTitleChange(wxWebEvent& evt)
{
    wxString title = evt.GetString();

    if (title.IsEmpty())
        title = _("(Untitled)");
    
    cfw::IDocumentSitePtr doc_site = m_frame->lookupSiteById(m_site_id);
    if (doc_site.isOk())
        doc_site->setCaption(title);
    
    m_title = title;
    
    // make sure the application's titlebar is updated
    g_app->getAppController()->updateTitle();
}

void WebDoc::onShowContextMenu(wxWebEvent& evt)
{
    wxDOMNode node = evt.GetTargetNode();
    wxDOMElement img_node;
    
    while (node.IsOk())
    {        
        if (0 == node.GetNodeName().CmpNoCase(wxT("IMG")))
        {
            img_node = node;
            break; 
        }
        
        node = node.GetParentNode();
    }




    wxMenu menuPopup;

    wxString href = evt.GetHref();
    if (!href.IsEmpty())
    {
        menuPopup.Append(ID_Open, _("&Open"));
        menuPopup.Append(ID_OpenNewTab, _("Open in &New Tab"));
        menuPopup.AppendSeparator();
        menuPopup.Append(ID_SaveLinkAs, _("Save Lin&k As..."));
        menuPopup.AppendSeparator();
    }
    else
    {
        menuPopup.Append(ID_File_Back, _("&Back"));
        menuPopup.Append(ID_File_Forward, _("&Forward"));
        menuPopup.Append(ID_File_Reload, _("&Refresh"));
        menuPopup.Append(ID_File_Stop, _("&Stop"));
        menuPopup.AppendSeparator();
        menuPopup.Append(ID_SavePageAs, _("Save Pag&e As..."));
        menuPopup.AppendSeparator();
    }

    menuPopup.Append(ID_Edit_Cut, _("Cu&t"));
    menuPopup.Append(ID_Edit_Copy, _("&Copy"));
    menuPopup.Append(ID_Edit_CopyLink, _("Copy &Link"));
    menuPopup.Append(ID_Edit_Paste, _("&Paste"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Edit_SelectAll, _("Select &All"));
    menuPopup.AppendSeparator();
    
    if (img_node.IsOk())
    {
        // note: following accelerators of "o" and "h" are ok
        // since link menu items ("Open", etc), shouldn't
        // appear on right-click menu at the same time as
        // the rotate menu items
        menuPopup.Append(ID_RotateLeft, _("R&otate Left"));
        menuPopup.Append(ID_RotateRight, _("Rotate Rig&ht"));
    }
     else
    {
        menuPopup.Append(ID_ImportTable, _("&Import Table"));
    }
    
    menuPopup.AppendSeparator();
    
    if (!isViewingSource())
        menuPopup.Append(ID_ViewPageSource, _("&View Page Source"));
     else
        menuPopup.Append(ID_ViewWebPage, _("&View Web Page"));
    
    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = m_webcontrol->ScreenToClient(pt_mouse);

    // show the popup menu and capture the result
    cfw::CommandCapture* cc = new cfw::CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);
    
    
    switch (command)
    {
        case ID_Open:
            g_app->getAppController()->openWeb(evt.GetHref(), NULL);
            break;
            
        case ID_OpenNewTab:
            g_app->getAppController()->openWeb(evt.GetHref(), NULL, appOpenForceNewWindow);
            break;

        case ID_File_Back:
            m_webcontrol->GoBack();
            break;
            
        case ID_File_Forward:
            m_webcontrol->GoForward();
            break;
        
        case ID_File_Reload:
            m_webcontrol->Reload();
            break;
            
        case ID_File_Stop:
            m_webcontrol->Stop();
            break;
        
        case ID_Edit_Cut:
            m_webcontrol->CutSelection();
            break;
        
        case ID_Edit_Copy:
            m_webcontrol->CopySelection();
            break;

        case ID_Edit_CopyLink:
            m_webcontrol->CopyLinkLocation();
            break;

        case ID_Edit_Paste:
            m_webcontrol->Paste();
            break;
            
        case ID_Edit_SelectAll:
            m_webcontrol->SelectAll();
            break;

        case ID_SaveLinkAs:
            {
                wxString filter;

                // NOTE: if you add or remove items from this
                // list, make sure you adjust the case statement
                // below, because it affects which file type the
                // target will be
                filter += _("All Files");
                filter += wxT(" (*.*)|*.*|");
                filter.RemoveLast(); // get rid of the last pipe sign

                wxString filename = getFilenameFromPath(evt.GetHref(), true);   // include extension
                wxFileDialog dlg(g_app->getMainWindow(),
                                 _("Save As"),
                                 wxT(""),
                                 filename,
                                 filter,
                                 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
                
                if (dlg.ShowModal() == wxID_OK)
                {
                    cfw::IJobInfoPtr job_info;
                    job_info.create_instance("cfw.JobInfo");
                    downloadFile(evt.GetHref(), dlg.GetPath(), job_info);
                }
            }
            break;

        case ID_SavePageAs:
            savePageAsExternal();
            break;

        case ID_ImportTable:
            {
                if (g_app->getDatabase().isNull())
                {
                    cfw::appMessageBox(_("A project must be open to import a table."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION | wxCENTER);
                    return;
                }
            
            
                wxDOMNode target = evt.GetTargetNode();
        
                tango::ISetPtr set = makeTableFromDom(target);
                if (set.isNull())
                    return;
                    
                ITableDocPtr doc = TableDocMgr::createTableDoc();

                doc->open(set, xcm::null);
                doc->setSourceUrl(m_url);
                g_app->getMainFrame()->createSite(doc,
                                                  cfw::sitetypeNormal,
                                                  -1, -1, -1, -1);
            }
            break;
        
        case ID_RotateLeft:
        {
            std::wstring style = towstr(img_node.GetAttribute(wxT("style")));
            klregex::wregex regex(L"rotate\\(\\s*([0-9]{1,3})\\s*deg\\s*\\)");
            klregex::wmatch matchres;
            if (regex.search(style.c_str(), matchres))
            {
                int degrees = kl::wtoi(matchres.getSubMatch(1).str());
                degrees -= 90;
                if (degrees < 0)
                    degrees = 360+degrees;
                wchar_t buf[255];
                swprintf(buf, 255, L"rotate(%ddeg)", degrees);
                regex.replace(style, buf);
            }
             else
            {
                if (style.length() > 0 && style[style.length()-1] != ';')
                    style += L";";
                style += L"-moz-transform:rotate(270deg)";
            }
            
            img_node.SetAttribute(wxT("style"), towx(style));
            break;
        }
        
        case ID_RotateRight:
        {
            std::wstring style = towstr(img_node.GetAttribute(wxT("style")));
            klregex::wregex regex(L"rotate\\(\\s*([0-9]{1,3})\\s*deg\\s*\\)");
            klregex::wmatch matchres;
            if (regex.search(style.c_str(), matchres))
            {
                int degrees = kl::wtoi(matchres.getSubMatch(1).str());
                degrees += 90;
                if (degrees >= 360)
                    degrees = degrees-360;
                wchar_t buf[255];
                swprintf(buf, 255, L"rotate(%ddeg)", degrees);
                regex.replace(style, buf);
            }
             else
            {
                if (style.length() > 0 && style[style.length()-1] != ';')
                    style += L";";
                style += L"-moz-transform:rotate(90deg)";
            }
            
            img_node.SetAttribute(wxT("style"), towx(style));
            break;
        }
                
        case ID_ViewWebPage:
            switchToWeb();
            break;
        
        case ID_ViewPageSource:
            switchToSource();
            break;
    }
}

void WebDoc::onCreateBrowserRequest(wxWebEvent& evt)
{
    if (evt.GetCreateChromeFlags() & wxWEB_CHROME_MODAL)
    {
        // create a modal frame
        wxWebDialog* dlg = new wxWebDialog(g_app->getMainWindow(), -1, wxT(""), wxPoint(0,0), wxSize(800,600));
        evt.SetCreateBrowser(dlg->GetWebControl());
    }
     else
    {

        cfw::IDocumentSitePtr site;
        
        WebDoc* doc = new WebDoc;
        site = m_frame->createSite(static_cast<cfw::IDocument*>(doc),
                                   cfw::sitetypeNormal | cfw::siteNoInitialActivate,
                                   -1, -1, -1, -1);
        if (site.isOk())
        {
            evt.SetCreateBrowser(doc->m_webcontrol);
        }
    }
}


void WebDoc::onShouldHandleContent(wxWebEvent& evt)
{
    // if there is no database, let the web browser handle all types;
    // we don't want to intercept anything
    if (g_app->getDatabase().isNull())
    {
        evt.SetShouldHandle(true);
        return;
    }


    wxString input_content_type = evt.GetContentType();
    
    if (input_content_type == wxT("text/csv"))
    {
        // let it know where to put the output table
        //g_delimited_text_content_handler->setTargetSiteId(m_doc_site->getId());
        
        // false because we want the web control _not_ to handle
        // this type itself; we want it to go to our delimited
        // content listener instead
        evt.SetShouldHandle(false);
        return;
    }
     else if (input_content_type == wxT("text/plain"))
    {
        // convert text/plain into csv.  This will get re-routed to
        // our handler eventually
        evt.SetOutputContentType(wxT("text/csv"));
        evt.SetShouldHandle(true);
        return;
    }
     else if (input_content_type == wxT("application/rss+xml") ||
              input_content_type == wxT("application/rdf+xml") ||
              input_content_type == wxT("application/atom+xml") ||
              input_content_type == wxT("application/xml") ||
              input_content_type == wxT("text/xml"))
    {
        // let it know where to put the output table
        g_feed_content_handler->setTargetSiteId(m_doc_site->getId());
        
        // we want to handle rss feeds ourselves.  Browser
        // shouldn't handle it
        evt.SetShouldHandle(false);
        return;
    }
     else
    {

    }
            
            
    evt.Skip();
}

void WebDoc::onFavIconAvailable(wxWebEvent& evt)
{
    wxImage img = m_webcontrol->GetFavIcon();
    img.Rescale(16, 16);
    
    wxBitmap bitmap = wxBitmap(img);
    m_favicon = img;
    
    // this bitmap will display when the progress bitmap is
    // done (when the web page finishes loading)
    if (m_bitmap_updater.IsRunning())
    {
        m_bitmap_updater.setFinishBitmap(bitmap);
    }
     else
    {
        m_doc_site->setBitmap(bitmap);
    }
    
    // fire a 'document bitmap updated' event
    
    cfw::Event* cfw_evt = new cfw::Event(wxT("appmain.docBitmapUpdated"));
    cfw_evt->o_param = &bitmap;
    g_app->getMainFrame()->sendEvent(cfw_evt);
    
    // if we clicked on a bookmar in the linkbar, update its bitmap
    
    if (m_last_clicked_bookmark_path.IsEmpty())
        return;
    
    Bookmark bookmark;
    if (!bookmark.load(m_last_clicked_bookmark_path))
    {
        m_last_clicked_bookmark_path = wxEmptyString;
        return;
    }
    
    bookmark.setFavIcon(img);
    bookmark.save(m_last_clicked_bookmark_path);
    
    g_app->getAppController()->refreshLinkBar();
        
    m_last_clicked_bookmark_path = wxEmptyString;
}

void WebDoc::onDOMContentLoaded(wxWebEvent& evt)
{
}



class WebDocLoadDeferred : public wxEvtHandler
{
public:
    
    WebDocLoadDeferred(WebDoc* doc, const wxString& url)
    {
        m_webdoc = doc;
        m_url = url;
        
        wxCommandEvent cmd(wxEVT_COMMAND_MENU_SELECTED, 10000);
        AddPendingEvent(cmd);
    }
    
    bool ProcessEvent(wxEvent& evt)
    {
        m_webdoc->openURI(m_url);
        wxPendingDelete.Append(this);
        return true;
    }
    
    WebDoc* m_webdoc;
    wxString m_url;
};



void WebDoc::showWebRes(int webres_id)
{
    wxString path = g_app->getInstallPath();
    path = filenameToUrl(path);
    if (path.IsEmpty() || path[path.Length()-1] != wxT('/'))
        path += wxT('/');
    path += wxT("webres.jar!/");
    
    switch (webres_id)
    {
        default:
        case webresAbout:          path += wxT("about.html");            break;
        case webresLicense:        path += wxT("license.html");          break;
        case webresDownloading:    path += wxT("download.html");         break;
        case webresServerNotFound: path += wxT("server_not_found.html"); break;
    }
    
    path.Prepend(wxT("jar:"));
    //openURI(path);
    
    WebDocLoadDeferred* l = new WebDocLoadDeferred(this, path);
}

wxImage WebDoc::getFavIcon()
{
    return m_favicon;
}


void WebDoc::onOpenURI(wxWebEvent& evt)
{
    // reset favicon to nothing
    m_favicon = wxImage();
    m_bitmap_updater.setFinishBitmap(wxBitmap());
    
    
    wxString href = evt.GetHref();
    
    wxString loc = href;
    loc.MakeLower();
    loc.Trim(true);
    loc.Trim(false);

    if (loc.Left(7) == wxT("mailto:"))
    {
        composeMail(href);
        evt.Veto();
        return;
    }

    // handle feed protocol
    if (loc.Left(5) == wxT("feed:"))
    {
        wxString new_loc = href.Mid(5);
        if (new_loc.Left(6) == wxT("http//"))
        {
            // I've seen some weird URL's that look like
            // feed:http//www.example.com
            // (without a colon) -- these work in other browsers
            new_loc.Remove(0, 6);
        }
        
        if (new_loc.Find(wxT("://")) == -1)
        {
            // http is the default protocol
            new_loc = wxT("http://") + new_loc;
        }
        
        openURI(new_loc);
        evt.Veto();
        return;
    }
    
    if (loc.Left(7) == wxT("sdserv:") || loc.Left(8) == wxT("sdservs:"))
    {
        g_app->getAppController()->openDataLink(loc);
        evt.Veto();
        return;
    }


    // see WebDoc::openURI() for some oddball cases that
    // get handled before this function processes them
    
    // about: type pages get translated
    if (loc.Find(wxT("toolkit.jar")) != -1)
    {
        // about: pages in toolkit.jar
        
        if (loc.Find(wxT("aboutabout.")) != -1)             // "about:about"
        {
            openURI(wxT("about:blank"));
            evt.Veto();
        }
         else if (loc.Find(wxT("about.")) != -1)            // "about:"
        {
            showWebRes(webresAbout);
            evt.Veto();
        }
         else if (loc.Find(wxT("license.")) != -1 ||        // "about:license"
                  loc.Find(wxT("mpl-1.1.")) != -1)
        {
            showWebRes(webresLicense);
            evt.Veto();
        }
         else if (loc.Find(wxT("mozilla.")) != -1 ||        // "about:mozilla"
                  loc.Find(wxT("logo.")) != -1)             // "about:logo"
        {
            openURI(wxT("about:blank"));
            evt.Veto();
        }
    } 
     else
    {
        // about: pages not in toolkit.jar
        
        if (loc.Find(wxT("about:bloat")) != -1)             // "about:bloat"
        {
            openURI(wxT("about:blank"));
            evt.Veto();
        }
         else if (loc.Find(wxT("about:cache-entry")) != -1) // "about:cache-entry"
        {
            openURI(wxT("about:blank"));
            evt.Veto();
        }
    }
}



static wxString extensionFromMimeType(const wxString& _mime_type)
{
    wxString mime_type = _mime_type;
    mime_type.MakeLower();
    
    if (mime_type == wxT("application/excel") ||
        mime_type == wxT("application/vnd.ms-excel") ||
        mime_type == wxT("application/vnd.msexcel") ||
        mime_type == wxT("application/x-excel") ||
        mime_type == wxT("application/x-msexcel"))
    {
        return wxT("xls");
    }
    
    if (mime_type == wxT("application/vnd.interchange-csv"))
    {
        return wxT("icsv");
    }
    
    if (mime_type == wxT("text/comma-separated-values") ||
        mime_type == wxT("text/csv") ||
        mime_type == wxT("application/csv"))
    {
        return wxT("csv");
    }
    
    if (mime_type == wxT("text/tab-separated-values") ||
        mime_type == wxT("text/tsv") ||
        mime_type == wxT("application/tsv"))
    {
        return wxT("tsv");
    }
    
    if (mime_type == wxT("application/dbase") ||
        mime_type == wxT("application/x-dbase") ||
        mime_type == wxT("application/dbf") ||
        mime_type == wxT("application/x-dbf") ||
        mime_type == wxT("zz-application/zz-winassoc-dbf"))
    {
        return wxT("dbf");
    }
    
    return wxEmptyString;
}


void WebDoc::onInitDownload(wxWebEvent& evt)
{   
    wxString href = evt.GetHref();
    wxString filename = evt.GetFilename();
    wxString prot = href.BeforeFirst(L':');
    prot.MakeLower();
    
    wxString ext = filename;
    ext.MakeLower();
    
    wxString mime_type = evt.GetContentType();
    wxString mime_ext = extensionFromMimeType(mime_type);
    
    if (ext.Find(wxT(".csv")) >= 0 ||
        ext.Find(wxT(".tsv")) >= 0 ||
        mime_ext.Length() > 0)
    {
        // first, make sure the file has the right extension
        if (prot != wxT("file") && mime_ext.Length() > 0)
        {
            if (ext.AfterLast(wxT('.')) != mime_ext)
            {
                filename += wxT(".");
                filename += mime_ext;
            }
        }
    
        // to ensure a unique but consistent filename, put a bit of
        // an md5 string right before the extension
        
        wxString url_md5 = towx(kl::md5str(towstr(href)));
        int spot = filename.Find(wxT('.'), true);
        if (spot >= 0)
        {
            filename.insert((size_t)spot, url_md5.Right(6));
            filename.insert((size_t)spot, wxT("_"));
        }
        
        
        wxString loc = m_webcontrol->GetCurrentURI();
        
        wxString temp_path = towx(xf_get_temp_path());
        if (temp_path.IsEmpty() || temp_path.Last() != PATH_SEPARATOR_CHAR)
            temp_path += PATH_SEPARATOR_CHAR;
        temp_path += filename;
        
        
        // find out if the file is already in use
        if (!xf_remove(towstr(temp_path)))
        {
            // file is in use -- check if it's open somewhere
            
            cfw::IFramePtr frame = g_app->getMainFrame();
            if (frame.isOk())
            {
                cfw::IDocumentSiteEnumPtr docsites;
                docsites = frame->getDocumentSites(cfw::sitetypeNormal);

                size_t i, site_count = docsites->size();
                for (i = 0; i < site_count; ++i)
                {
                    cfw::IDocumentSitePtr site = docsites->getItem(i);
                    cfw::IDocumentPtr doc = site->getDocument();
                    if (doc.isNull())
                        continue;
                    ITableDocPtr tabledoc = doc;
                    if (tabledoc.isOk() && doc->getDocumentLocation() == evt.GetHref())
                    {
                        frame->setActiveChild(site);
                        return;
                    }
                }
            }
        }


        DelimitedTextProgressListener* listener = new DelimitedTextProgressListener;
        if (loc.IsEmpty() || loc == wxT("about:blank"))
        {
            // if this window is blank, use it as the target
            // for the output table
            //showWebRes(webresDownloading);
            m_doc_site->setCaption(wxString::Format(_("Downloading '%s'"), filename.c_str()));
            listener->setTargetSiteId(m_doc_site->getId());
        }
         else
        {
            WebDoc* doc = new WebDoc;
            cfw::IDocumentSitePtr site;
            site = m_frame->createSite(static_cast<cfw::IDocument*>(doc),
                                       cfw::sitetypeNormal,
                                       -1, -1, -1, -1);
            //doc->showWebRes(webresDownloading);
            site->setCaption(wxString::Format(_("Downloading '%s'"), filename.c_str()));
            listener->setTargetSiteId(site->getId());
        }
        
        evt.SetDownloadAction(wxWEB_DOWNLOAD_SAVEAS);
        evt.SetDownloadTarget(temp_path);
        evt.SetDownloadListener(listener);
    }
     else
    {
        // unknown download type, so prompt user with open/save dialog box

        OpenOrSaveDlg dlg(g_app->getMainWindow(), filename);
        int result = dlg.ShowModal();
        
        switch (result)
        {
            case wxID_OPEN:
                evt.SetDownloadAction(wxWEB_DOWNLOAD_OPEN);
                break;
            case wxID_SAVE:
                {
                    wxString filter;
                    filter += _("All Files");
                    filter += wxT(" (*.*)|*.*|");
                    filter.RemoveLast(); // get rid of the last pipe sign

                    wxFileDialog dlg(g_app->getMainWindow(),
                             _("Save As"),
                             wxT(""),
                             filename,
                             filter,
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
                    
                    if (dlg.ShowModal() != wxID_OK)
                    {
                        evt.SetDownloadAction(wxWEB_DOWNLOAD_CANCEL);
                        return;
                    }
                    
                    
                    DownloadProgressListener* listener = new DownloadProgressListener;
                    evt.SetDownloadAction(wxWEB_DOWNLOAD_SAVEAS);
                    evt.SetDownloadTarget(dlg.GetPath());
                    evt.SetDownloadListener(listener);
                }
                
                break;
            case wxID_CANCEL:
                evt.SetDownloadAction(wxWEB_DOWNLOAD_CANCEL);
                break;
        }


    }
}


void WebDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void WebDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

void WebDoc::onUpdateUI(wxUpdateUIEvent& evt)
{
    int id = evt.GetId();
    switch (id)
    {
        case ID_Edit_Undo:
            evt.Enable(false);
            break;
            
        case ID_Edit_Redo:
            evt.Enable(false);
            break;

        case ID_Edit_Cut:
            evt.Enable(m_webcontrol->CanCutSelection());
            break;
            
        case ID_Edit_Copy:
            evt.Enable(m_webcontrol->CanCopySelection());
            break;
            
        case ID_Edit_CopyLink:
            evt.Enable(m_webcontrol->CanCopyLinkLocation());
            break;
            
        case ID_Edit_Paste:
            evt.Enable(m_webcontrol->CanPaste());
            break;

        case ID_Edit_Delete:
            evt.Enable(m_webcontrol->CanCutSelection());
            break;
            
        case ID_Edit_SelectAll:
            evt.Enable(true);
            break;
            
        case ID_Edit_SelectNone:
            evt.Enable(true);
            break;
            
        case ID_Edit_Find:
            evt.Enable(true);
            break;
            
        case ID_Edit_FindPrev:
        case ID_Edit_FindNext:
        {
            wxString find_filter_text = g_app->getAppController()->
                                               getStandardToolbar()->
                                               getFindCombo()->
                                               GetValue();
            evt.Enable((find_filter_text.Length() > 0) ? true : false);
            break;
        }
        
        case ID_Edit_Replace:
            evt.Enable(false);
            break;

        case ID_Edit_GoTo:
            evt.Enable(false);
            break;

        case ID_File_Reload:
            evt.Enable(true);
            break;

        case ID_ImportTable:
        case ID_ViewPageSource:
            evt.Enable(isViewingSource() ? false : true);
            break;
            
        // TODO: enable only if site loading
        case ID_File_Stop:
            evt.Enable(true);
            break;
      
        default:
            evt.Enable(false);
    }
}


void WebDoc::onLeftUp(wxWebEvent& evt)
{
    if (::wxGetMouseState().ControlDown())
    {
        // ctrl + left click is the same as a middle click
        onMiddleUp(evt);
        return;
    }
    
    evt.Skip();
}

void WebDoc::onMiddleUp(wxWebEvent& evt)
{
    if (g_feed_content_handler)
    {
        g_feed_content_handler->setOpenedWithMiddleClick(true);
    }
    
    //if (g_delimited_text_content_handler)
    //{
    //    g_delimited_text_content_handler->setOpenedWithMiddleClick(true);
    //}
    
    
    // open elements that we're middle clicking on in a new tab
    wxString href = evt.GetHref();
    if (!href.IsEmpty())
    {
        g_app->getAppController()->openWeb(evt.GetHref(), NULL, appOpenForceNewWindow, NULL);
    }
}



void WebDoc::onWebViewTitleChanged(wxWebViewEvent& evt)
{
    wxString title = evt.GetString();

    if (title.IsEmpty())
        title = _("(Untitled)");
    
    cfw::IDocumentSitePtr doc_site = m_frame->lookupSiteById(m_site_id);
    if (doc_site.isOk())
        doc_site->setCaption(title);
    
    m_title = title;
    
    // make sure the application's titlebar is updated
    g_app->getAppController()->updateTitle();
}




void WebDoc::onWebViewDocumentLoaded(wxWebViewEvent& evt)
{
    m_url = m_webview->GetCurrentURL();

    if (m_frame)
    {
        cfw::IDocumentSitePtr doc_site = m_frame->getActiveChild();
        if (doc_site.isOk() && doc_site == m_doc_site)
        {
            m_frame->postEvent(new cfw::Event(wxT("cfw.locationChanged")));
        }
    }
}
