/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-08
 *
 */


#include "appmain.h"
#include "updater.h"
#include "curlutil.h"
#include <curl/curl.h>
#include <kl/crypt.h>
#include <kl/thread.h>
#include "../jobs/jobbase.h"


// encryption key for the update service



class CheckUpdatesWaitDlg : public wxDialog
{
public:

    CheckUpdatesWaitDlg(wxWindow* parent) : wxDialog(parent, -1, APPLICATION_NAME,
                 wxDefaultPosition,
                 wxSize(300, 100),
                  wxDEFAULT_DIALOG_STYLE |
                  wxNO_FULL_REPAINT_ON_RESIZE |
                  wxCLIP_CHILDREN |
                  wxCENTER)
    {
        wxStaticText* text = new wxStaticText(this, -1, _("Checking for updates..."));
        resizeStaticText(text);

        m_gauge = new wxGauge(this, -1, 0, wxPoint(0,0), wxSize(20,18));
        m_gauge->Pulse();
        
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddStretchSpacer();
        sizer->Add(text, 0, wxEXPAND | wxLEFT, 20);
        sizer->AddSpacer(8);
        sizer->Add(m_gauge, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
        sizer->AddStretchSpacer();

        CenterOnParent();
        
        SetSizer(sizer);
        Layout();
        
        m_pulse_timer.SetOwner(this, 8001);
        m_pulse_timer.Start(50);
    }

protected:

    void onTimer(wxTimerEvent& evt)
    {
        m_gauge->Pulse();
    }
    
private:

    wxTimer m_pulse_timer;
    wxGauge* m_gauge;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(CheckUpdatesWaitDlg, wxDialog)
    EVT_TIMER(8001, CheckUpdatesWaitDlg::onTimer)
END_EVENT_TABLE()








class FileCloser
{
public:
    FileCloser(xf_file_t _f) : f(_f) { }
    ~FileCloser() { if (f) xf_close(f); }
    void closeNow() { if (f) xf_close(f); f = 0; }
    xf_file_t f;
};




// -- update check job --


xcm_interface IUpdateCheckJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IUpdateCheckJob")

public:
    virtual const UpdaterInfo& getUpdaterInfo() = 0;
};

XCM_DECLARE_SMARTPTR(IUpdateCheckJob)



class UpdateCheckJob : public jobs::JobBase,
                       public IUpdateCheckJob
{
    XCM_CLASS_NAME("appmain.UpdateCheckJob")
    XCM_BEGIN_INTERFACE_MAP(UpdateCheckJob)
        XCM_INTERFACE_ENTRY(IUpdateCheckJob)
        XCM_INTERFACE_CHAIN(jobs::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    UpdateCheckJob()
    {
    }
    
    virtual ~UpdateCheckJob()
    {
    }

    const UpdaterInfo& getUpdaterInfo()
    {
        return m_update_info;
    }
    
    int runJob()
    {
        wxStopWatch sw;
        sw.Start();
        
        wxString update_xml = doHttpRequest(APP_UPDATE_URL);
    
        if (update_xml.IsEmpty())
            return 0;

        if (!Updater::parseUpdateFile(update_xml, m_update_info))
            return 0;
    
        if (m_update_info.fetch_url.IsEmpty())
            return 0;
        
        
        // make sure job runs at least 2 seconds so it appears
        // that check actually worked
        
        while (sw.Time() < 2000)
            kl::thread::sleep(100);
        
        return 0;
    }
    
    void runPostJob()
    {
    }
    
private:

    UpdaterInfo m_update_info;
};






// -- update download job --

xcm_interface IUpdateDownloadJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IUpdateDownloadJob")

public:

    virtual std::wstring getOutputFilename() = 0;
};

XCM_DECLARE_SMARTPTR(IUpdateDownloadJob)

class UpdateDownloadJob : public jobs::JobBase,
                          public IUpdateDownloadJob
{
    XCM_CLASS_NAME("appmain.UpdateDownloadJob")
    XCM_BEGIN_INTERFACE_MAP(UpdateDownloadJob)
        XCM_INTERFACE_ENTRY(IUpdateDownloadJob)
        XCM_INTERFACE_CHAIN(jobs::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    UpdateDownloadJob()
    {
        m_f = 0;
        m_curl = (CURL*)0;
        m_fetched_content_length = false;
        m_total_downloaded = 0.0;
        
        // set the job title and format the job progress string
        m_job_info->setTitle(towstr(_("Downloading Update")));
        m_job_info->setProgressStringFormat(
                                    towstr(_("$C KB downloaded")),
                                    towstr(_("$C of $M KB downloaded ($p1%)")));
        
        if (getAppPrefsLong(wxT("internet.proxy.type")) == prefProxyManual)
        {
            // get ftp proxy info from the registry
            m_proxy = tostr(getAppPrefsString(wxT("internet.proxy.http")));
            m_proxy_port = g_app->getAppPreferences()->getLong(wxT("internet.proxy.http_port"), 8080);
        }
    }
    
    virtual ~UpdateDownloadJob()
    {
    }
    
    std::wstring getOutputFilename()
    {
        return m_output_path;
    }

    void setInformation(const wxString& fetch_url,
                        const wxString& username,
                        const wxString& password)
    {
        m_fetch_url = tostr(fetch_url);
        m_basic_auth = "";
        if (username.Length() > 0)
        {
            m_basic_auth = tostr(username);
            m_basic_auth += ":";
            m_basic_auth += tostr(password);
        }
    }
    
    int runJob()
    {
        m_output_path = L"";
        m_fetched_content_length = false;
        m_total_downloaded = 0.0;
        
        m_curl = curlCreateHandle();
        if (!m_curl)
        {
            m_job_info->setState(jobs::jobStateFailed);
            return 0;
        }
        
        CurlAutoDestroy ad(m_curl);

        wxString temps = m_fetch_url;
        if (temps.Find(wxT('?')) != -1)
            temps = temps.BeforeLast(wxT('?'));
        if (temps.Length() > 0 && temps.Last() == wxT('/'))
            temps.RemoveLast();
        
        wxString filename = temps.AfterLast(wxT('/'));
        
        
        std::wstring output_filename = xf_get_temp_path();
        output_filename += towstr(filename);

        xf_remove(output_filename);
        
        
        m_f = xf_open(output_filename, xfCreate, xfWrite, xfShareNone);
        if (!m_f)
        {
            m_job_info->setState(jobs::jobStateFailed);
            return 0;
        }
        
        FileCloser fc(m_f);
        
        
        CURLcode curl_result;
        curl_result = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, FALSE);
        curl_result = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_result = curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, TRUE);
        curl_result = curl_easy_setopt(m_curl, CURLOPT_AUTOREFERER, TRUE);
        curl_result = curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, -1);
        curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
        curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        curl_result = curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");
        curl_result = curl_easy_setopt(m_curl, CURLOPT_URL, m_fetch_url.c_str());
        
        // set curl proxy info
        curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXY, m_proxy.c_str());
        curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYPORT, m_proxy_port);

        if (m_basic_auth.length() > 0)
            curl_result = curl_easy_setopt(m_curl, CURLOPT_USERPWD, m_basic_auth.c_str());

        // set the result functions
        curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)this);
        if (curl_result != CURLE_OK)
            return 0;
    
        curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, (void*)update_writer);
        if (curl_result != CURLE_OK)
            return 0;
        
        // set the GET option   
        curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPGET, TRUE);
        if (curl_result != CURLE_OK)
            return 0;

        // get the full body    
        curl_result = curl_easy_setopt(m_curl, CURLOPT_NOBODY, FALSE);
        if (curl_result != CURLE_OK)
            return 0;
        
        // retrieve the data from the URL
        curl_result = curl_easy_perform(m_curl);

        m_curl = (CURL*)0;
        
        m_output_path = output_filename;
        
        if (isCancelling())
        {
            fc.closeNow();
            xf_remove(output_filename);
        }
        
        return 0;
    }
    
    void runPostJob()
    {
    }
    
    static size_t update_writer(void* ptr, size_t size, size_t nmemb, void* data_struct)
    {
        UpdateDownloadJob* j = (UpdateDownloadJob*)data_struct;

        if (j->isCancelling())
            return 0;
            
        unsigned int amt = xf_write(j->m_f, ptr, size, nmemb);
        
        if (!j->m_fetched_content_length)
        {
            double content_length = 0.0;
            curl_easy_getinfo(j->m_curl,
                              CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                              &content_length);
            j->m_job_info->setMaxCount(content_length);
        }
        
        j->m_total_downloaded += ((double)amt * (double)size);
        j->m_job_info->setCurrentCount(j->m_total_downloaded);
        
        return amt*size;
    }

private:

    xf_file_t m_f;
    bool m_fetched_content_length;
    double m_total_downloaded;
    CURL* m_curl;
    
    std::string m_fetch_url;
    std::string m_basic_auth;
    std::wstring m_output_path;
    std::string m_proxy;
    long m_proxy_port;
};





Updater::Updater()
{
}

Updater::~Updater()
{
}



static int versionToNumber(const std::wstring& _ver)
{
    std::vector<int> parts;

    std::wstring ver = _ver;
    while (1)
    {
        if (ver.empty())
            break;
        if (!iswdigit(ver[0]))
            break;
        
        std::wstring part = kl::beforeFirst(ver, L'.');
        int period = ver.find(L'.');
        ver = kl::afterFirst(ver, L'.');
        
        parts.push_back(kl::wtoi(part));
        
        if (period == -1)
            break;
    }
    
    while (parts.size() < 4)
        parts.push_back(0);
    
    wchar_t buf[255];
    swprintf(buf, 254, L"%2d%01d%01d%04d", parts[0], parts[1], parts[2], parts[3]);
    
    return kl::wtoi(buf);
}

static bool isVersionHit(const std::wstring& app,
                         const std::wstring& platform,
                         const std::wstring& min_ver,
                         const std::wstring& max_ver)
{
    std::wstring cur_app = kl::towstring(PALADIN_APP_TAG);
    
    if (0 != wcscasecmp(app.c_str(), cur_app.c_str()))
        return false;

    std::wstring uplatform = platform;
    kl::makeUpper(uplatform);
    
    #if defined(_MSC_VER)

        if (sizeof(void*) == 8)
        {
            // 64-bit windows
            if (uplatform != L"WIN64")
                return false;
        }
         else
        {
            // 32-bit windows
            if (uplatform != L"WIN32")
                return false;
        }

    #elif defined(__linux__)

        if (sizeof(void*) == 8)
        {
            // 64-bit linux
            if (uplatform != L"LINUX64")
                return false;
        }
         else
        {
            // 32-bit linux
            if (uplatform != L"LINUX32")
                return false;
        }

    #else
        // unknown platform
        return false;
    #endif


    wchar_t cur_ver[99];
    swprintf(cur_ver, 99, L"%d.%d.%d.%d",
                    APP_VERSION_MAJOR,
                    APP_VERSION_MINOR,
                    APP_VERSION_SUBMINOR,
                    APP_VERSION_BUILDSERIAL);
                    
    int v_cur, v_min, v_max;
    
    if (min_ver == L"*" || min_ver == L"")
    {
        v_min = 0;
    }
     else
    {    
        v_min = versionToNumber(min_ver);
    }
    
    
    if (max_ver == L"*" || max_ver == L"")
    {
        v_max = 99999999;
    }
     else
    {    
        v_max = versionToNumber(max_ver);
    }
    
    
    v_cur = versionToNumber(cur_ver);
    
    return ((v_cur >= v_min && v_cur <= v_max) ? true : false);
}

bool Updater::parseUpdateFile(const wxString& xml, UpdaterInfo& info)
{
    kl::xmlnode node;
    if (!node.parse(towstr(xml)))
        return false;
    
    size_t i, entry_count = node.getChildCount();
    for (i = 0; i < entry_count; ++i)
    {
        kl::xmlnode& update_node = node.getChild(i);
        
        if (update_node.getPropertyIdx(L"minversion") == -1 ||
            update_node.getPropertyIdx(L"maxversion") == -1 ||
            update_node.getPropertyIdx(L"platform") == -1 ||
            update_node.getPropertyIdx(L"app") == -1)
        {
            // missing information in this entry; skip it
            continue;
        }
        
        std::wstring min_ver = update_node.getProperty(L"minversion").value;
        std::wstring max_ver = update_node.getProperty(L"maxversion").value;
        std::wstring app = update_node.getProperty(L"app").value;
        std::wstring platform = update_node.getProperty(L"platform").value;

        if (isVersionHit(app, platform, min_ver, max_ver))
        {
            wxString user, pw, fetch_url;
            
            kl::xmlnode& uid = update_node.getChild(L"username");
            if (!uid.isEmpty())
            {
                std::wstring val = uid.getNodeValue();
                kl::trim(val);

                user = kl::decryptString(val, APP_UPDATE_ENCRYPTIONKEY);
                user.Trim(true);
                user.Trim(false);
            }
        
            kl::xmlnode& p = update_node.getChild(L"password");
            if (!p.isEmpty())
            {
                std::wstring val = p.getNodeValue();
                kl::trim(val);

                pw = kl::decryptString(val, APP_UPDATE_ENCRYPTIONKEY);
                pw.Trim(true);
                pw.Trim(false);
            }
        
            kl::xmlnode& location = update_node.getChild(L"url");
            if (!location.isEmpty())
            {
                std::wstring val = location.getNodeValue();
                kl::trim(val);
                fetch_url = kl::decryptString(val, APP_UPDATE_ENCRYPTIONKEY);
                fetch_url.Trim(true);
                fetch_url.Trim(false);
                
                
                info.fetch_url = fetch_url;
                info.user_name = user;
                info.password = pw;
                return true;
            }
        }
    }
    
    return false;
}


static void onUpdateDownloadFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;
        
    IUpdateDownloadJobPtr u = job;
    wxASSERT(u.p);
    
    
    jobs::IJobQueuePtr job_queue = g_app->getJobQueue();
    if (job_queue.isOk() && job_queue->getJobsActive())
    {
        wxString appname = APPLICATION_NAME;
        wxString message = wxString::Format(_("There are currently jobs running.  Please update %s after all running jobs are completed."),
                                            appname.c_str());
        appMessageBox(message, APPLICATION_NAME);
        return;
    }
    
    
    
    int result = appMessageBox(_("The update has finished downloading.  Would you like to install the update now?"),
                                    APPLICATION_NAME,
                                    wxYES_NO | wxICON_QUESTION,
                                    g_app->getMainWindow());
    
    if (result == wxYES)
    {
        wxString path = u->getOutputFilename();

        #ifdef __WXGTK__

            // linux version unpacks archive and shows folder
            wxString cmd;

            if (path.Find(wxT(".tar.gz")) != -1)
            {
                cmd = wxT("tar zxvf ");
                cmd += path;
                cmd += wxT(" ");
                cmd += wxT("-C ");
                cmd += xf_get_temp_path();

                cmd = wxT("xdg-open ");
                cmd += xf_get_temp_path();

                ::wxExecute(cmd);
            }
             else
            {
                cmd = wxT("xdg-open ");
                cmd += xf_get_temp_path();
                ::wxExecute(cmd);
            }

            // close the application so the install works well
            g_app->getMainFrame()->closeAll(true);
            ::wxYield();
            g_app->getMainWindow()->Close(true);

        #else

            // win32 version closes app and runs installer -- note that previously,
            // wxExecute was being used, however, for Win32 we now use ShellExecute
            // because on Vista and Windows 7 it will properly prompt the user
            // for elevated privileges to execute the installer.            
            ::ShellExecute(NULL, _T("open"), (LPCTSTR)path.c_str(), _T(""), NULL, SW_SHOWNORMAL);
            
            ::wxYield();
            AppBusyCursor bc;
            kl::thread::sleep(2000);
            
            // close the application so the install works well
            g_app->getMainFrame()->closeAll(true);
            ::wxYield();
            g_app->getMainWindow()->Close(true);

        #endif
    }
}


CheckUpdatesWaitDlg* g_dlg = NULL;


static void onUpdateCheckFinished(jobs::IJobPtr job)
{
    bool full_gui = false;
    if (g_dlg)
    {
        full_gui = true;
        g_dlg->Close();
        g_dlg = NULL;
    }
    
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;
    
    IUpdateCheckJobPtr u = job;
    wxASSERT(u.p);

    UpdaterInfo info = u->getUpdaterInfo();
    
    if (info.fetch_url.IsEmpty())
    {
        if (full_gui)
            Updater::showNoUpdates();
        return;
    }
    
    if (Updater::showAskForUpdate())
    {
        jobs::IJobQueuePtr job_queue = g_app->getJobQueue();
        if (job_queue.isOk() && job_queue->getJobsActive())
        {
            wxString appname = APPLICATION_NAME;
            wxString message = wxString::Format(_("There are currently jobs running.  Please update %s after all running jobs are completed."),
                                                appname.c_str());
            appMessageBox(message, APPLICATION_NAME);
            return;
        }
        
        // do update
        UpdateDownloadJob* job = new UpdateDownloadJob;
        job->setInformation(info.fetch_url, info.user_name, info.password);
        job->sigJobFinished().connect(&onUpdateDownloadFinished);
        g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
    }
}


// |full_gui| pops up a dialog which says 'checking for updates'
// specifying false for this parmeter will cause the program to
// only show a gui if there is indeed an update available

void Updater::checkForUpdates(bool full_gui)
{
    if (g_dlg != NULL)
        return;

    // do update -- add to the script job queue (doesn't really matter)
    // so that it won't display in the main queue or in the progress bar
    // at the bottom
    UpdateCheckJob* job = new UpdateCheckJob;
    job->sigJobFinished().connect(&onUpdateCheckFinished);
    g_app->getScriptJobQueue()->addJob(job, jobs::jobStateRunning);

    
    if (full_gui)
    {
        g_dlg = new CheckUpdatesWaitDlg(g_app->getMainWindow());
        g_dlg->Show();
    }


    // make sure we update the 'last checked' time in the registry
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isOk())
    {
        time_t now_time = time(NULL);

        prefs->setLong(wxT("general.updater.last_check"), now_time);
        prefs->flush();
    }
}



void Updater::showNoUpdates()
{
    int maj, min, sub;
    maj = APP_VERSION_MAJOR;
    min = APP_VERSION_MINOR;
    sub = APP_VERSION_SUBMINOR;
    
    // format the shortest version string possible
    wxString version_str;
    if (sub == 0)
        version_str = wxString::Format(wxT("%d.%d"), maj, min);
         else
        version_str = wxString::Format(wxT("%d.%d.%d"), maj, min, sub);
    
    wxString appname = APPLICATION_NAME;
    wxString message = wxString::Format(_("This version of %s (%s) is the current version."),
                                        appname.c_str(),
                                        version_str.c_str());
    
    appMessageBox(message, APPLICATION_NAME);
}


bool Updater::showAskForUpdate()
{
    wxString appname = APPLICATION_NAME;
    wxString message = wxString::Format(_("A new version of %s is available.  Would you like to download it?"),
                                        appname.c_str());
                                        
    int result = appMessageBox(message,
                                    APPLICATION_NAME,
                                    wxYES_NO | wxICON_QUESTION,
                                    g_app->getMainWindow());
    if (result == wxYES)
        return true;
        
    return false;
}


