/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Installation Utility
 * Author:   Benjamin I. Williams
 * Created:  2002-09-11
 *
 */


#include <wx/wx.h>
#include <wx/laywin.h>
#include <wx/panel.h>
#include <wx/config.h>
#include <wx/splash.h>
#include <wx/statline.h>
#include <wx/calctrl.h>
#include <wx/wizard.h>
#include <wx/fs_zip.h>

#include <xcm/xcm.h>
#include "../cfw/util.h"

#include "xpm_kirixlogo.xpm"
#include "xpm_setup.xpm"
#include "xpm_folder_large.xpm"


#include <kl/klib.h>

#ifdef WIN32
#define PATH_SEPARATOR_CHAR    wxT('\\')
#define PATH_SEPARATOR_STR     wxT("\\")
#else
#define PATH_SEPARATOR_CHAR    wxT('/')
#define PATH_SEPARATOR_STR     wxT("/")
#endif


wxString g_exe_path;        // -- directory where the installation is --
wxString g_eula_text;       // -- text of the eula --
wxString g_target_path;     // -- target directory for the installation --
bool g_all_users;           // -- true if install is for all users --


wxWizardPage* page_welcome;
wxWizardPage* page_eula;
wxWizardPage* page_insttype;
wxWizardPage* page_destination;
wxWizardPage* page_progress;
wxWizardPage* page_finished;
wxWizardPage* page_empty;
wxWizard* wizard;
const int wizard_width = 530; //480;
const int wizard_height = 380; //310;


static wxString getProgramPath()
{
    char buf[512];
    char* slash;

#if defined(WIN32)
    GetModuleFileNameA(NULL, buf, 511);
    slash = strrchr(buf, '\\');
    if (slash)
    {
        *slash = 0;
    }
#elif defined(__linux__)
    int res;
    res = readlink("/proc/self/exe", buf, 512);
    if (res == -1)
        return wxT("");
    buf[res] = 0;
    slash = strrchr(buf, '/');
    if (slash)
    {
        *slash = 0;
    }
#else
    return wxT("");
#endif

    return towx(buf);
}


static void limitTextSize(wxWindow* wnd)
{
    wxFont font = wnd->GetFont();
    if (font.GetPointSize() > 12)
    {
        font.SetPointSize(12);
        wnd->SetFont(font);
    }
}


static bool is64bit()
{
    FILE* f = popen("uname -m", "r");
    if (!f)
        return false;
        
    char s[80];
    fgets(s, 80, f);
    pclose(f);

    if (strncmp(s, "x86_64", 6) == 0)
        return true;

    return false;
}


class LogoHeader : public wxControl
{
public:
    
    LogoHeader(wxWindow* parent) : wxControl(parent,
                                             -1,
                                             wxPoint(0,0),
                                             wxSize(wizard_width, 40), wxNO_BORDER)
    {
        m_kirixlogo_bitmap = wxBitmap(xpm_kirixlogo);

        m_font = wxFont(13, wxSWISS, wxNORMAL, wxNORMAL, FALSE);
        m_text = wxT("Kirix Strata Installation");
        
        m_minWidth = wizard_width;
        m_minHeight = 40;
    }


    void onPaint(wxPaintEvent& event)
    {
        int cli_width, cli_height;
        int bmp_width, bmp_height;
        int text_width, text_height;
        
        GetClientSize(&cli_width, &cli_height);

        wxPaintDC dc(this);

        dc.SetPen(*wxWHITE_PEN);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(0, 0, cli_width, cli_height);


        // -- draw kirix logo --

        bmp_width = m_kirixlogo_bitmap.GetWidth();
        bmp_height = m_kirixlogo_bitmap.GetHeight();
        dc.DrawBitmap(m_kirixlogo_bitmap, 8, (cli_height-bmp_height)/2);


        // -- draw text --
        
        dc.SetFont(m_font);
        dc.GetTextExtent(m_text, &text_width, &text_height);
        

        dc.DrawText(m_text,
                    cli_width-text_width-8, ((cli_height-text_height)/2)+2);
    }
    
private:

    wxBitmap m_kirixlogo_bitmap;
    wxFont m_font;
    wxString m_text;
    
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(LogoHeader, wxControl)
    EVT_PAINT(LogoHeader::onPaint)
END_EVENT_TABLE()




class EmptyPage : public wxWizardPage
{
public:

    EmptyPage(wxWizard* parent) : wxWizardPage(parent)
    {
    }

    // -- GetNext()/GetPrev() --

    wxWizardPage* GetPrev() const
    {
        return NULL;
    }

    wxWizardPage* GetNext() const
    {
        return NULL;
    }
};




class WelcomePage : public wxWizardPage
{
public:

    WelcomePage(wxWizard* parent) : wxWizardPage(parent)
    {
        SetSize(wizard_width, wizard_height);

        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        wxStaticBitmap* setup_bitmap = new wxStaticBitmap(this, -1, wxBitmap(xpm_setup));

        wxBoxSizer* mid_sizer = new wxBoxSizer(wxHORIZONTAL);
        wxBoxSizer* mid_text_sizer = new wxBoxSizer(wxVERTICAL);


        // -- text controls --

        wxStaticText* text1 = new wxStaticText(this, -1,
                  _("Welcome to the Kirix Strata Installation Wizard.  This wizard will guide you through the process of setting up Kirix Strata on your computer."));
        limitTextSize(text1);
        cfw::resizeStaticText(text1, wizard_width-100);

        wxStaticText* text2 = new wxStaticText(this, -1,
                _("It is recommended that you exit all programs before running this Setup program."));
        limitTextSize(text2);
        cfw::resizeStaticText(text2, wizard_width-100);

        wxStaticText* text3 = new wxStaticText(this, -1,
                _("Click Cancel to quit Setup and then close any programs you have running.  Click Next to continue with the Setup program."));
        limitTextSize(text3);
        cfw::resizeStaticText(text3, wizard_width-100);


        wxStaticText* legal_text1 = new wxStaticText(this, -1,
                _("WARNING: This program is protected by copyright law and international treaties."));
        limitTextSize(legal_text1);
        cfw::resizeStaticText(legal_text1);

        wxStaticText* legal_text2 = new wxStaticText(this, -1,
                _("Unauthorized reproduction or distribution of this program, or any portion of it, may result in severe civil and criminal penalties, and will be prosecuted to the maximum extent possible under law."));
        limitTextSize(legal_text2);
        cfw::resizeStaticText(legal_text2);

        wxStaticText* legal_text3 = new wxStaticText(this, -1,
                _("Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved."));
        limitTextSize(legal_text3);
        cfw::resizeStaticText(legal_text3);


        // -- construct the mid-text sizer --

        mid_text_sizer->Add(text1, 0);
        mid_text_sizer->Add(1,10, 0, wxEXPAND);
        mid_text_sizer->Add(text2, 0);
        mid_text_sizer->Add(1,10, 0, wxEXPAND);
        mid_text_sizer->Add(text3, 0);

        mid_sizer->Add(setup_bitmap, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);
        mid_sizer->Add(mid_text_sizer, 1, wxEXPAND);

        main_sizer->Add(new LogoHeader(this), 0, wxBOTTOM, 20);

        main_sizer->Add(mid_sizer, 0, wxEXPAND);
        main_sizer->Add(1,1, 1, wxEXPAND, 0);
        main_sizer->Add(legal_text1, 0, wxEXPAND);
        main_sizer->Add(1,10, 0, wxEXPAND, 0);
        main_sizer->Add(legal_text2, 0, wxEXPAND);
        main_sizer->Add(1, 30, 0, wxEXPAND, 0);
        main_sizer->Add(legal_text3, 0, wxEXPAND);

        SetSizer(main_sizer);
        main_sizer->SetSizeHints(this);
        Layout();
    }

    // -- wizard event handlers --

    void onWizardCancel(wxWizardEvent& event)
    {
    }

    void onWizardPageChanging(wxWizardEvent& event)
    {

    }

    // -- GetNext()/GetPrev() --

    wxWizardPage* GetPrev() const
    {
        return NULL;
    }

    wxWizardPage* GetNext() const
    {
        return page_eula;
    }

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WelcomePage, wxWizardPage)
    EVT_WIZARD_PAGE_CHANGING(-1, WelcomePage::onWizardPageChanging)
    EVT_WIZARD_CANCEL(-1, WelcomePage::onWizardCancel)
END_EVENT_TABLE()




class EulaPage : public wxWizardPage
{

    enum
    {
        ID_AcceptButton = 9000,
        ID_RejectButton
    };

public:

    EulaPage(wxWizard* parent) : wxWizardPage(parent)
    {
        SetSize(wizard_width, wizard_height);

        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        

        wxStaticText* text1 = new wxStaticText(this, -1,
                  _("Please read the following license agreement carefully."));
        limitTextSize(text1);
        cfw::resizeStaticText(text1, 300);

        wxTextCtrl* eula_text = new wxTextCtrl(this,
                                               -1,
                                               g_eula_text,
                                               wxPoint(0,0),
                                               wxSize(100,100),
                                               wxTE_READONLY |
                                               wxTE_MULTILINE);

        //limitTextSize(eula_text);


        m_accept_button = new wxRadioButton(this,
                                            ID_AcceptButton,
                                            _("I accept the terms of the license agreement"),
                                            wxDefaultPosition,
                                            wxDefaultSize,
                                            wxRB_GROUP);

        m_reject_button = new wxRadioButton(this,
                                            ID_RejectButton,
                                            _("I do not accept the terms of the license agreement"));

        limitTextSize(m_accept_button);
        limitTextSize(m_reject_button);

        // -- construct the sizer --

        main_sizer->Add(new LogoHeader(this), 0, wxBOTTOM, 20);
        main_sizer->Add(text1, 0, wxEXPAND);
        main_sizer->Add(eula_text, 1, wxEXPAND);
        main_sizer->Add(m_accept_button, 0, wxEXPAND | wxTOP, 7);
        main_sizer->Add(m_reject_button, 0, wxEXPAND | wxTOP, 3);

        SetSizer(main_sizer);
        main_sizer->SetSizeHints(this);
        Layout();
    }

    // -- wizard event handlers --

    void onWizardCancel(wxWizardEvent& event)
    {
    }

    void onWizardPageChanged(wxWizardEvent& event)
    {
        m_reject_button->SetValue(true);
        enableForward(false);
    }

    void onWizardPageChanging(wxWizardEvent& event)
    {
        enableForward(true);
    }

    // -- GetNext()/GetPrev() --

    wxWizardPage* GetPrev() const
    {
        return page_welcome;
    }

    wxWizardPage* GetNext() const
    {
        return page_insttype;
    }

    void onAccept(wxCommandEvent& event)
    {
        enableForward(true);
    }

    void onReject(wxCommandEvent& event)
    {
        enableForward(false);
    }

    void enableForward(bool state)
    {
        wxWindow* forward_btn = GetParent()->FindWindow(wxID_FORWARD);
        if (forward_btn)
        {
            forward_btn->Enable(state);
        }
    }

public:

    wxRadioButton* m_accept_button;
    wxRadioButton* m_reject_button;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(EulaPage, wxWizardPage)
    EVT_WIZARD_PAGE_CHANGING(-1, EulaPage::onWizardPageChanging)
    EVT_WIZARD_PAGE_CHANGED(-1, EulaPage::onWizardPageChanged)
    EVT_RADIOBUTTON(EulaPage::ID_AcceptButton, EulaPage::onAccept)
    EVT_RADIOBUTTON(EulaPage::ID_RejectButton, EulaPage::onReject)
END_EVENT_TABLE()


class InstTypePage : public wxWizardPage
{
public:

    enum
    {
        ID_AllUsers = 8000,
        ID_CurUser
    };

public:

    InstTypePage(wxWizard* parent) : wxWizardPage(parent)
    {
        SetSize(wizard_width, wizard_height);

        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        
        wxStaticText* text1 = new wxStaticText(this, -1,
                  _("Install this application for:"));

        limitTextSize(text1);

        wxString user_str;
        user_str = _("Only for me");
        user_str += wxT(" (");
        user_str += ::wxGetUserName();
        user_str += wxT(")");

        m_all_users = new wxRadioButton(this,
                                       ID_AllUsers,
                                       _("Anyone who uses this computer"),
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxRB_GROUP);

        m_cur_user = new wxRadioButton(this,
                                       ID_CurUser,
                                       user_str);

        limitTextSize(m_all_users);
        limitTextSize(m_cur_user);

        // -- construct the sizer --

        main_sizer->Add(new LogoHeader(this), 0, wxBOTTOM, 20);
        main_sizer->Add(text1, 0, wxEXPAND | wxLEFT, 20);
        main_sizer->Add(10,10);
        main_sizer->Add(m_all_users, 0, wxEXPAND | wxLEFT, 70);
        main_sizer->Add(5,5);
        main_sizer->Add(m_cur_user, 0, wxEXPAND | wxLEFT, 70);
        SetSizer(main_sizer);
        main_sizer->SetSizeHints(this);
        Layout();

        if (getuid() == 0)
        {
            m_all_users->SetValue(true);
        }
         else
        {
            m_cur_user->SetValue(true);
        }
    }

    // -- GetNext()/GetPrev() --

    wxWizardPage* GetPrev() const
    {
        return page_welcome;
    }

    wxWizardPage* GetNext() const
    {
        return page_destination;
    }

    void onWizardPageChanged(wxWizardEvent& evt)
    {
    }

    void onWizardPageChanging(wxWizardEvent& evt)
    {
        #ifndef WIN32
        if (evt.GetDirection())
        {
            if (m_all_users->GetValue() && getuid() != 0)
            {
                cfw::appMessageBox(_("You may only install for all users is you are running this installation program as the system administrator (\"root\")."),
                                   _("Installation Wizard"),
                                   wxICON_EXCLAMATION | wxOK,
                                   GetParent());
                evt.Veto();
            }
        }
        #endif

        g_all_users = m_all_users->GetValue();

        if (g_all_users)
        {
            g_target_path = wxT("/opt/kstrata");
        }
         else
        {
            g_target_path = ::wxGetHomeDir();
            g_target_path += wxT("/kstrata");
        }
    }

public:

    wxRadioButton* m_all_users;
    wxRadioButton* m_cur_user;


    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(InstTypePage, wxWizardPage)
    EVT_WIZARD_PAGE_CHANGED(-1, InstTypePage::onWizardPageChanged)
    EVT_WIZARD_PAGE_CHANGING(-1, InstTypePage::onWizardPageChanging)
END_EVENT_TABLE()


class DestinationPage : public wxWizardPage
{
public:

    enum
    {
        ID_ChangeButton = 9000
    };

public:

    DestinationPage(wxWizard* parent) : wxWizardPage(parent)
    {
        SetSize(wizard_width, wizard_height);

        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);

        wxStaticBitmap* bmp = new wxStaticBitmap(this,
                                                 -1,
                                                 wxBitmap(xpm_folder_large));

        wxStaticText* text1 = new wxStaticText(this, -1,
                  _("Install the software into this folder:"));

        limitTextSize(text1);
        
        m_path_text = new wxTextCtrl(this,
                                     -1,
                                     wxEmptyString,
                                     wxPoint(0,0),
                                     wxSize(150,24));

        limitTextSize(m_path_text);

        wxButton* change_button = new wxButton(this,
                                               ID_ChangeButton,
                                               _("Change..."));

        limitTextSize(change_button);

        // -- construct the sizer --

        vert_sizer->Add(text1, 0, wxEXPAND | wxBOTTOM, 5);
        vert_sizer->Add(m_path_text, 1, wxEXPAND);

        horz_sizer->Add(bmp, 0, wxLEFT, 30);
        horz_sizer->Add(10,10);
        horz_sizer->Add(vert_sizer, 1, wxEXPAND);
        horz_sizer->Add(10,10);
        horz_sizer->Add(change_button, 0, wxALIGN_BOTTOM);

        main_sizer->Add(new LogoHeader(this), 0, wxBOTTOM, 40);
        main_sizer->Add(horz_sizer, 0, wxEXPAND);

        SetSizer(main_sizer);
        main_sizer->SetSizeHints(this);
        Layout();
    }

    // -- GetNext()/GetPrev() --

    wxWizardPage* GetPrev() const
    {
        return page_insttype;
    }

    wxWizardPage* GetNext() const
    {
        return page_progress;
    }

    void onWizardPageChanged(wxWizardEvent& event)
    {
        m_path_text->SetValue(g_target_path);
    }

    void onWizardPageChanging(wxWizardEvent& event)
    {
        wxString target_path = m_path_text->GetValue();

        std::wstring path = towstr(target_path);
        
        // -- if the directory exists, assume the target path is 
        //    good --
        if (xf_get_directory_exist(towstr(target_path)))
        {
            g_target_path = target_path;
            return;
        }
            
        // -- try to create target directory --
        if (!xf_mkdir(path))
        {
            cfw::appMessageBox(_("The target path could not be created."),
                               _("Installation Wizard"),
                               wxICON_EXCLAMATION | wxOK,
                               GetParent());
                                   
            event.Veto();
            return;
        }
        
        xf_rmdir(path);
        
        g_target_path = target_path;
    }

    void onDestinationClicked(wxCommandEvent& event)
    {
        wxDirDialog dlg(GetParent(), _("Choose an destination directory"));
        
        if (dlg.ShowModal() == wxID_OK)
        {
            wxString path = dlg.GetPath();
            path += PATH_SEPARATOR_STR;
            path += wxT("kstrata");

            m_path_text->SetValue(path);
        }
    }
    
public:

    wxTextCtrl* m_path_text;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(DestinationPage, wxWizardPage)
    EVT_WIZARD_PAGE_CHANGED(-1, DestinationPage::onWizardPageChanged)
    EVT_WIZARD_PAGE_CHANGING(-1, DestinationPage::onWizardPageChanging)
    EVT_BUTTON(ID_ChangeButton, DestinationPage::onDestinationClicked)
END_EVENT_TABLE()



class InstThread : public wxThread
{
public:

    InstThread() : wxThread(wxTHREAD_DETACHED)
    {
        m_cancelled = false;
        m_progress_pct = 0;
    }

    ~InstThread()
    {
    }

    wxThread::ExitCode Entry()
    {
        int i;
        
        std::vector<wxString> files;
        
        // -- read in a list of files from the installation
        //    control file, instinfo.xml --
        
        kl::xmlnode node;

        wxString path = g_exe_path;
        path += PATH_SEPARATOR_STR;
        path += wxT("instinfo.xml");
              
        if (!node.load(towstr(path)))
        {
            sendError(_("The installation configuration file could not be found."));
            return 0;
        }
               
        if (node.getNodeName() != L"inst_info")
        {
            sendError(_("The installation configuration file is not valid."));
            return 0;
        }
        
        
        // -- create the target directory --
        std::string temps = tostr(g_target_path);
        ::wxMkDir(temps.c_str(), makeMode(wxT("755")));
        
        // -- iterate through each command and execute it --
        
        int cmd_count = node.getChildCount();           
        for (i = 0; i < cmd_count; ++i)
        {
            // -- update the progress control --
            m_progress_pct = ((i*100)/cmd_count);
            m_progress_string = wxT("");
            
            // -- check whether the installation
            //    has been cancelled --
            m_obj_mutex.lock();
            if (m_cancelled)
            {
                m_obj_mutex.unlock();
                doCancel();
                return 0;
            }
            m_obj_mutex.unlock();

            // -- perform the next command --
            doCommand(node.getChild(i));
        }

        installDesktopIcon();
        //installSampleDataEntry();
        
        m_progress_string = wxT("");
        setProgress(100);
        
        wxThread::Sleep(1000);

        setFinished();

        return 0;
    }

    void cancel()
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        m_cancelled = true;
    }
    
private:

    bool installSampleDataEntry()
    {
        wxString location = g_target_path;
        location += wxT("/sample");
        
        wxConfig* config = new wxConfig(wxT("kirix-strata"), wxT("Kirix"));
        config->SetPath(wxT("/RecentDatabases"));
    
        config->SetPath(wxT("connection099"));
        config->Write(wxT("Local"), wxT("1"));
        config->Write(wxT("Name"), wxT("Sample"));
        config->Write(wxT("Location"), location);
        config->Write(wxT("User"), wxT("admin"));
        config->Write(wxT("Password"), wxT(""));
        
        delete config;
    }
    
    bool installDesktopIcon()
    {
        const char* text =
            "[Desktop Entry]\n"
            "Encoding=UTF-8\n"
            "Version=1.0\n"
            "Type=Application\n"
            "Exec=%s/bin/kstrata\n"
            "TryExec=\n"
            "Icon=%s/bin/strata48.png\n"
            "X-GNOME-DocPath=\n"
            "Terminal=false\n"
            "Name=Kirix Strata\n";
        
        std::string target_path;
        target_path = tostr(g_target_path);
        
        char buf[4096];
        sprintf(buf, text, target_path.c_str(), target_path.c_str());
        
        std::string desktop_file;
        desktop_file = getenv("HOME");
        desktop_file += "/Desktop/Kirix Strata.desktop";
        
        ::remove(desktop_file.c_str());
        FILE* f = fopen(desktop_file.c_str(), "wt");
        if (f)
        {
            fputs(buf, f);
            fclose(f);
            return true;
        }
        
        return false;
    }
    
    bool doCommand(kl::xmlnode& cmd)
    {
        std::wstring value = cmd.getNodeName();
        
        if (value == L"create_dir")
        {
            kl::xmlnode& target_node = cmd.getChild(L"target");
            if (target_node.isEmpty())
                return false;
            
            kl::xmlnode& mode_node = cmd.getChild(L"mode");
            if (mode_node.isEmpty())
                return false;
                
            wxString target = expandTokens(towx(target_node.getNodeValue()));
            wxString mode = towx(mode_node.getNodeValue());

            m_progress_string.Printf(wxT("Creating directory %s..."), target.c_str());
            setProgress(m_progress_pct, m_progress_string);
                    
            return createDir(target, mode);
        }
         else if (value == L"copy_file")
        {
            kl::xmlnode& src_node = cmd.getChild(L"src");
            if (src_node.isEmpty())
                return false;
                
            kl::xmlnode& target_node = cmd.getChild(L"target");
            if (target_node.isEmpty())
                return false;
            
            kl::xmlnode& mode_node = cmd.getChild(L"mode");
            if (mode_node.isEmpty())
                return false;

            bool overwrite = true;
            
            kl::xmlnode& overwrite_node = cmd.getChild(L"overwrite");
            if (!overwrite_node.isEmpty())
            {
                std::wstring val = overwrite_node.getNodeValue();
                kl::makeUpper(val);
                
                if (val == L"NO")
                    overwrite = false;
            }
            
            wxString src = expandTokens(towx(src_node.getNodeValue()));                
            wxString target = expandTokens(towx(target_node.getNodeValue()));
            wxString mode = towx(mode_node.getNodeValue());
            
            m_progress_string.Printf(wxT("Copying file %s..."), target.c_str());
            setProgress(m_progress_pct, m_progress_string);
            
            if (!overwrite)
            {
                std::wstring w_target = towstr(target);
                if (xf_get_file_exist(w_target))
                {
                    // -- don't overwrite it --
                    return true;
                }
            }
                             
            return copyFile(src, target, mode);
        }
         else if (value == L"unzip_file")
        {
            kl::xmlnode& src_node = cmd.getChild(L"src");
            if (src_node.isEmpty())
                return false;
                
            kl::xmlnode& target_node = cmd.getChild(L"target");
            if (target_node.isEmpty())
                return false;
            
            //kl::xmlnode& mode_node = cmd.getChild(L"mode");
            //if (mode_node.isEmpty())
            //    return false;

            wxString src = expandTokens(towx(src_node.getNodeValue()));                
            wxString target = expandTokens(towx(target_node.getNodeValue()));
            //wxString mode = towx(mode_node.getNodeValue());
            
            m_progress_string.Printf(wxT("Unzipping file %s..."), target.c_str());
            setProgress(m_progress_pct, m_progress_string);
                             
            return unzipFile(src, target);
        }
         else
        {
            return false;    
        }
        
        return true;
    }
    
    bool copyFile(const wxString& src,
                  const wxString& target,
                  const wxString& mode)
    {
        if (!::wxCopyFile(src, target, true))
        {
            return false;
        }
        
        #ifndef WIN32
        std::string t = tostr(target);
        chmod(t.c_str(), makeMode(mode));
        #endif
        
        return true;
    }
    
    bool createDir(const wxString& target,
                   const wxString& mode)
    {       
        std::string s = tostr(target);
        
        #ifdef WIN32
        return ::wxMkDir(s.c_str());
        #else
        return ::wxMkDir(s.c_str(), makeMode(mode));
        #endif
    }
    
    /*
    bool extractOne(wxFileSystem* fs,
                    const wxString& file,
                    const wxString& dest_file)
    {
        wxFSFile* fsfile = fs->OpenFile(file);
        if (!file)
            return false;
            
        unsigned char* buf = new unsigned char[32768];
        if (!buf)
            return false;
        
        std::string s_dest_file = kl::tostring(dest_file.c_str());
        FILE* f = fopen(s_dest_file.c_str(), "wb");
        if (!f)
            return false;
            
        wxInputStream* stream = fsfile->GetStream();
        
        bool err = false;
        bool done = false;
        
        while (!done)
        {
            stream->Read(buf, 32768);
            size_t last_read = stream->LastRead();
            
            done = (last_read != 32768) ? true : false;
            
            if (last_read <= 0)
            {
                done = true;
                break;
            }
                
            if (32768 != fwrite(buf, 1, last_read, f))
            {
                err = true;
                done = true;
            }
        }
        
        fclose(f);
        delete[] buf;
        delete file;
        
        return err;
    }
    
    bool extractAll(wxFileSystem* fs,
                    const wxString& src_url,
                    const wxString& dest_path)
    {
        printf("** path: '%ls'\n", src_url.c_str());
        
        fs->ChangePathTo(src_url, true);
        
        wxString s;
        
        std::wstring ws_dest_path = kl::towstring(dest_path.c_str());
        xf_mkdir(ws_dest_path);
        if (!xf_get_directory_exist(ws_dest_path))
            return false;
            
        s = fs->FindFirst(wxT("*"), 0);
        while (!s.IsEmpty())
        {            
            //if (!extractOne(fs, s, dest_file))
            //    return false;
            
            printf("Found File: '%ls'\n", s.c_str());
               
            s = fs->FindNext();
        }
        
        
        std::vector<wxString> dirs;
        
        s = fs->FindFirst(wxT("*"), wxDIR);
        while (!s.IsEmpty())
        {
            printf("Found Directory: '%ls'\n", s.c_str());
            s += wxT("/");
            dirs.push_back(s);               
            s = fs->FindNext();
        }
        
        std::vector<wxString>::iterator dir_it;
        for (dir_it = dirs.begin();
             dir_it != dirs.end();
             ++dir_it)
        {
            if (!extractAll(fs, *dir_it, dest_path))
                return false;
        }
        
        return true;
    }
    
    
    bool unzipFile(const wxString& src,
                   const wxString& target_path)
    {
        //wxString url = wxFileSystem::FileNameToURL(src);
        wxString url = wxT("file:");
        url += src;
        url += wxT("#zip:/");
        
        wxFileSystem fs;
        
        return extractAll(&fs, url, target_path);
    }
    */
    
    bool unzipFile(const wxString& src,
                   const wxString& target_path)
    {
        wxString command;
        command = wxT("unzip -qq -o ");
        command += src;
        command += wxT(" -d ");
        command += target_path;
        
        //wxExecute(command, wxEXEC_SYNC);
        std::string s = tostr(command);
        
        system(s.c_str());
        
        return true;
    }
    
    void doCancel()
    {
    }

    void setProgress(int pct, const wxString& str = wxEmptyString)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, 8000);
        event.SetInt(pct);
        event.SetString(str);
        ::wxPostEvent(page_progress, event);
    }

    void setFinished()
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, 8001);
        ::wxPostEvent(page_progress, event);
    }
    
    void sendError(const wxString& message)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, 8002);
        event.SetString(message);
        ::wxPostEvent(page_progress, event);
    }
    
    
    wxString expandTokens(const wxString& src)
    {
        wxString s = src.c_str();
        
        wxString home_dir = wxString::FromAscii(getenv("HOME"));
        
        s.Replace(wxT("[INSTPROG_PATH]"), g_exe_path);
        s.Replace(wxT("[TARGET]"), g_target_path);
        s.Replace(wxT("[HOME]"), home_dir);
        
        return s;
    }
    
    mode_t makeMode(const wxString& mode_str)
    {
        wxString str = mode_str.Right(3);
        if (str.Length() != 3)
            return S_IRWXU; // 700
            
        mode_t m = 0;
        
        wxChar u = str.GetChar(0);
        wxChar g = str.GetChar(1);
        wxChar o = str.GetChar(2);
        
        switch (u)
        {
            default:
            case '0': m |= 0; break;
            case '1': m |= S_IXUSR; break;
            case '2': m |= S_IWUSR; break;
            case '3': m |= (S_IWUSR|S_IXUSR); break;
            case '4': m |= S_IRUSR; break;
            case '5': m |= (S_IRUSR|S_IXUSR); break;
            case '6': m |= (S_IRUSR|S_IWUSR); break;
            case '7': m |= S_IRWXU; break;
        }
        switch (g)
        {
            default:
            case '0': m |= 0; break;
            case '1': m |= S_IXGRP; break;
            case '2': m |= S_IWGRP; break;
            case '3': m |= (S_IWGRP|S_IXGRP); break;
            case '4': m |= S_IRUSR; break;
            case '5': m |= (S_IRGRP|S_IXGRP); break;
            case '6': m |= (S_IRGRP|S_IWGRP); break;
            case '7': m |= S_IRWXG; break;
        }       
        switch (o)
        {
            default:
            case '0': m |= 0; break;
            case '1': m |= S_IXOTH; break;
            case '2': m |= S_IWOTH; break;
            case '3': m |= (S_IWOTH|S_IXOTH); break;
            case '4': m |= S_IROTH; break;
            case '5': m |= (S_IROTH|S_IXOTH); break;
            case '6': m |= (S_IROTH|S_IWOTH); break;
            case '7': m |= S_IRWXO; break;
        }       
        
        return m;
    }
    
private:

    xcm::mutex m_obj_mutex;
    bool m_cancelled;
    wxString m_progress_string;
    int m_progress_pct;
};




class ProgressPage : public wxWizardPage
{
public:

    ProgressPage(wxWizard* parent) : wxWizardPage(parent)
    {
        SetSize(wizard_width, wizard_height);

        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* text1 = new wxStaticText(this, -1,
                  _("Please wait while the installation wizard installs Kirix Strata.  This may take a few minutes."));
        
        limitTextSize(text1);
        cfw::resizeStaticText(text1, wizard_width-80);
        
        m_progress_text = new wxStaticText(this,
                                           -1,
                                           wxEmptyString,
                                           wxDefaultPosition,
                                           wxDefaultSize,
                                           wxST_NO_AUTORESIZE);
                                           
        m_progress = new wxGauge(this, -1, 100, wxPoint(0,0), wxSize(100,15));
              

        // -- construct the sizer --

        main_sizer->Add(new LogoHeader(this), 0, wxBOTTOM, 20);
        main_sizer->Add(text1);
        main_sizer->Add(1,15);
        main_sizer->Add(m_progress_text, 0, wxEXPAND | wxLEFT, 38);
        main_sizer->Add(1,2);
        main_sizer->Add(m_progress, 0, wxEXPAND | wxLEFT | wxRIGHT, 38);

        SetSizer(main_sizer);
        main_sizer->SetSizeHints(this);
        Layout();
        
        m_thread = NULL;
        
        m_timer.SetOwner(this, 8009);
        m_timer.Start(800);
    }

    // -- GetNext()/GetPrev() --

    wxWizardPage* GetPrev() const
    {
        return page_destination;
    }

    wxWizardPage* GetNext() const
    {
        return page_finished;
    }
    
    void enableButtons(bool state)
    {
        wxWindow* backward_btn = GetParent()->FindWindow(wxID_BACKWARD);
        if (backward_btn)
        {
            backward_btn->Enable(state);
        }
        
        wxWindow* forward_btn = GetParent()->FindWindow(wxID_FORWARD);
        if (forward_btn)
        {
            forward_btn->Enable(state);
        }
    }
    
    void onWizardPageChanged(wxWizardEvent& event)
    {
        // -- start worker thread which will do
        //    the actual installation --
    
        enableButtons(false);

        m_thread = new InstThread;
        m_thread->Create();
        m_thread->Run();
    }
    
    void onWizardCancel(wxWizardEvent& event)
    {
        if (m_thread)
        {
            m_thread->cancel();
            m_thread = NULL;
        }
    }
    
    void onProgress(wxCommandEvent& event)
    {
        int pct = event.GetInt();
        
        m_progress->SetValue(event.GetInt());
        m_progress_text->SetLabel(event.GetString());
    }
    
    void onFinished(wxCommandEvent& event)
    {
        enableButtons(true);

        // -- tell the wizard to go to the next page --
        wxWizard* wiz = (wxWizard*)GetParent();
        wiz->ShowPage(page_finished);
        m_thread = NULL;
    }

    void onError(wxCommandEvent& event)
    {
        cfw::appMessageBox(event.GetString());
        m_thread = NULL;
        enableButtons(true);
    }


    void onTimer(wxTimerEvent& event)
    {
        if (m_thread)
        {
            int new_pct =  m_progress->GetValue() + 1;
            if (new_pct > 100)
                new_pct = 100;

            m_progress->SetValue(new_pct);
        }
    }
    
public:

    wxGauge* m_progress;
    wxStaticText* m_progress_text;
    InstThread* m_thread;
    wxTimer m_timer;
    
    DECLARE_EVENT_TABLE()
};



BEGIN_EVENT_TABLE(ProgressPage, wxWizardPage)
    EVT_WIZARD_PAGE_CHANGED(-1, ProgressPage::onWizardPageChanged)
    EVT_WIZARD_CANCEL(-1, ProgressPage::onWizardCancel)
    EVT_MENU(8000, ProgressPage::onProgress)
    EVT_MENU(8001, ProgressPage::onFinished)
    EVT_MENU(8002, ProgressPage::onError)
    EVT_TIMER(8009, ProgressPage::onTimer)
END_EVENT_TABLE()


class FinishedPage : public wxWizardPage
{
public:

    FinishedPage(wxWizard* parent) : wxWizardPage(parent)
    {
        SetSize(wizard_width, wizard_height);

        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* text1 = new wxStaticText(this, -1,
                  _("The installation wizard has successfully installed the software.  Click Finish to exit the wizard."));  
        limitTextSize(text1);
        cfw::resizeStaticText(text1, wizard_width-120);

        wxStaticText* text2 = new wxStaticText(this, -1,
                  _("The software was installed in:"));      
        limitTextSize(text2);
        cfw::resizeStaticText(text2, wizard_width-120);

        m_target_text = new wxStaticText(this, -1,
                  g_target_path);      
        limitTextSize(m_target_text);
        cfw::makeFontBold(m_target_text);
        cfw::resizeStaticText(m_target_text, wizard_width-120);

        // -- construct the sizer --

        main_sizer->Add(new LogoHeader(this), 0, wxBOTTOM, 30);
        main_sizer->Add(text1, 0, wxLEFT, 50);
        main_sizer->Add(10,10);
        main_sizer->Add(text2, 0, wxLEFT, 50);
        main_sizer->Add(10,10);
        main_sizer->Add(m_target_text, 0, wxLEFT, 100);
        
        SetSizer(main_sizer);
        main_sizer->SetSizeHints(this);
        Layout();
    }

    // -- GetNext()/GetPrev() --

    wxWizardPage* GetPrev() const
    {
        return NULL;
    }

    wxWizardPage* GetNext() const
    {
        return NULL;
    }

    void onWizardPageChanged(wxWizardEvent& event)
    {
        wxWindow* backward_btn = GetParent()->FindWindow(wxID_BACKWARD);
        if (backward_btn)
        {
            backward_btn->Enable(false);
        }

        wxWindow* cancel_btn = GetParent()->FindWindow(wxID_CANCEL);
        if (cancel_btn)
        {
            cancel_btn->Enable(false);
        }

        m_target_text->SetLabel(g_target_path);
    }

private:

    wxStaticText* m_target_text;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(FinishedPage, wxWizardPage)
    EVT_WIZARD_PAGE_CHANGED(-1, FinishedPage::onWizardPageChanged)
END_EVENT_TABLE()




class KppInstApp : public wxApp
{
private:

    bool OnInit()
    {
        cfw::suppressConsoleLogging();
        
        // -- determine if we are on a 64-bit platform --
        
        //if (sizeof(long) == 4 && is64bit())
        //{
        //    printf("\n\n\nNOTE: 64-bit platforms are not supported by this build.\n\n");
        //    return false;
        //}
        
        
        // -- show splash screen --
        wxImage::AddHandler(new wxPNGHandler);

        wxFileSystem::AddHandler(new wxZipFSHandler);

        // -- set defaults --
        g_exe_path = getProgramPath();
        g_all_users = false;

        g_target_path = ::wxGetHomeDir();
        g_target_path += wxT("/kstrata");
        
        // -- display splashscreen, if available --

        wxString eula_path = g_exe_path;
        eula_path += PATH_SEPARATOR_STR;
        eula_path += wxT("eula.txt");

        // -- read in the text of the eula --
        char* buf = new char[500000];
        int b = 0;
        buf[0] = 0;
        FILE* f = fopen(tostr(eula_path).c_str(), "r");
        if (f)
        {
            b = fread(buf, 1, 499000, f);
            fclose(f);
        }
        buf[b] = 0;
        g_eula_text = towx(buf);
        delete[] buf;





/*
        wxBitmap bitmap;
        wxString path = g_exe_path;
        path += wxT("/data/splash.png");

        printf("%ls\n", path.c_str());

        if (xf_get_file_exist(kl::towstring(path)))
        {
            if (bitmap.LoadFile(path, wxBITMAP_TYPE_PNG))
            {
                wxSplashScreen* splash = new wxSplashScreen(bitmap,
                    wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_NO_TIMEOUT,
                    2000, NULL, -1, wxDefaultPosition, wxDefaultSize,
                    wxSIMPLE_BORDER|wxSTAY_ON_TOP);
                wxYield();
                wxSleep(2);
                delete splash;
            }
        }
*/


        // -- show wizard --
        wizard = new wxWizard(NULL,
                              -1,
                              _("Kirix Strata Setup"),
                              wxNullBitmap,
                              wxDefaultPosition,
                              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    
        page_welcome = new WelcomePage(wizard);
        page_eula = new EulaPage(wizard);
        page_insttype = new InstTypePage(wizard);
        page_destination = new DestinationPage(wizard);
        page_progress = new ProgressPage(wizard);
        page_finished = new FinishedPage(wizard);
        page_empty = new EmptyPage(wizard);
    
        wizard->SetPageSize(wxSize(wizard_width, wizard_height));

        wizard->RunWizard(page_welcome);

        //delete wizard;
        wizard->Destroy();


        ExitMainLoop();

        return true;
    }

    int OnExit()
    {
        return 0;
    }

};



IMPLEMENT_APP(KppInstApp)

