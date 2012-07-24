/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Process Monitor
 * Author:   Benjamin I. Williams
 * Created:  2007-04-02
 *
 */


#include <wx/wx.h>
#include <windows.h>
#include <vector>

typedef DWORD (WINAPI *PFN_GetProcessImageFileName)(HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);
typedef DWORD (WINAPI *PFN_GetVolumePathName)(LPCTSTR lpszFileName, LPTSTR lpszVolumePathName, DWORD cchBufferLength);


PFN_GetProcessImageFileName pGetProcessImageFileName;
PFN_GetVolumePathName       pGetVolumePathName;



std::vector<std::string> g_captions;
std::vector<std::string> g_crash_captions;


// -- application --

class MyApp : public wxApp
{
public:
    bool OnInit();
};

DECLARE_APP(MyApp);
IMPLEMENT_APP(MyApp);


// -- frame --

class MyFrame : public wxFrame
{
friend class MyApp;

    enum
    {
        ID_Exit = wxID_HIGHEST+1,
        ID_About,
        ID_Timer,
        ID_SetInterval,
        ID_SetExecutable
    };

public:

    MyFrame(wxWindow* parent,
            wxWindowID id,
            const wxString& title,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE | wxSUNKEN_BORDER);

    ~MyFrame();
    
    void StartApp();

private:

    void OnEraseBackground(wxEraseEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnTimer(wxTimerEvent& evt);
    void OnSetExecutable(wxCommandEvent& evt);
    void OnSetInterval(wxCommandEvent& evt);
    
private:

    wxTimer m_timer;
    wxTextCtrl* m_text;
    int m_hang_count;
    int m_notrunning_count;
    wxString m_image_name;
    bool m_last_state_running;
    
    DECLARE_EVENT_TABLE();
};



bool MyApp::OnInit()
{
    g_captions.push_back("Kirix Strata");

    g_crash_captions.push_back("appmain.exe");
    g_crash_captions.push_back("Kirix Strata Application");
    


    HMODULE hpsapi = LoadLibraryA("psapi.dll");
    HMODULE hKernel32 = LoadLibraryA("kernel32.dll");
    
    pGetProcessImageFileName = (PFN_GetProcessImageFileName)GetProcAddress(hpsapi, "GetProcessImageFileNameW");
    pGetVolumePathName = (PFN_GetVolumePathName)GetProcAddress(hKernel32, "GetVolumePathNameW");
    if (pGetProcessImageFileName == NULL || pGetVolumePathName == NULL)
    {
        wxMessageBox(wxT("Couldn't get procedure address..."));
        return false;
    }


    MyFrame* frame = new MyFrame(NULL,
                                 wxID_ANY,
                                 wxT("Kirix Process Monitor"),
                                 wxDefaultPosition,
                                 wxSize(800, 600));
                                                     
    SetTopWindow(frame);
    frame->Show();



    if (argc > 1)
    {
        frame->m_image_name = argv[1];
    }
    
    return true;
}



BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_ERASE_BACKGROUND(MyFrame::OnEraseBackground)
    EVT_SIZE(MyFrame::OnSize)
    EVT_TIMER(10001, MyFrame::OnTimer)
    EVT_MENU(ID_SetInterval, MyFrame::OnSetInterval)
    EVT_MENU(ID_SetExecutable, MyFrame::OnSetExecutable)
END_EVENT_TABLE()



MyFrame::MyFrame(wxWindow* parent,
                 wxWindowID id,
                 const wxString& title,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style)
        : wxFrame(parent, id, title, pos, size, style)
{
    m_hang_count = 0;
    m_notrunning_count = 0;
    m_last_state_running = false;
    
    // create menu
    wxMenuBar* mb = new wxMenuBar;

    wxMenu* file_menu = new wxMenu;
    file_menu->Append(wxID_EXIT, _("Exit"));

    wxMenu* settings_menu = new wxMenu;
    settings_menu->Append(ID_SetInterval, _("Set Interval..."));
    settings_menu->Append(ID_SetExecutable, _("Set Execute Command..."));

    wxMenu* help_menu = new wxMenu;
    help_menu->Append(ID_About, _("About..."));
    
    mb->Append(file_menu, _("File"));
    mb->Append(settings_menu, _("Settings"));
    mb->Append(help_menu, _("Help"));
    
    SetMenuBar(mb);

    CreateStatusBar();
    GetStatusBar()->SetStatusText(_("Ready"));


    m_text = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxTE_MULTILINE);
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_text, 1, wxEXPAND);
    SetSizer(sizer);
    
    
    wxLogTextCtrl* log = new wxLogTextCtrl(m_text);
    wxLog::SetActiveTarget(log);
    wxLog::SetTimestamp(wxT("%a %b %d %H:%M:%S %Y"));


    // min size for the frame itself isn't completely done.
    // see the end up wxFrameManager::Update() for the test
    // code. For now, just hard code a frame minimum size
    SetMinSize(wxSize(400,300));
    
    m_timer.SetOwner(this, 10001);
    m_timer.Start(60000);
}

MyFrame::~MyFrame()
{
}


void MyFrame::OnEraseBackground(wxEraseEvent& evt)
{
    evt.Skip();
}

void MyFrame::OnSize(wxSizeEvent& evt)
{
    evt.Skip();
}


void MyFrame::OnSetExecutable(wxCommandEvent& evt)
{
    wxTextEntryDialog dlg(this, _("Please enter the executable path:"));
    dlg.SetValue(m_image_name);
    if (dlg.ShowModal() != wxID_OK)
        return;

    m_image_name = dlg.GetValue();
}

void MyFrame::OnSetInterval(wxCommandEvent& evt)
{
    wxTextEntryDialog dlg(this, _("Please enter the new check interval in seconds:"));
    dlg.SetValue(wxString::Format(wxT("%d"), m_timer.GetInterval()/1000));
    if (dlg.ShowModal() != wxID_OK)
        return;

    int seconds = wxAtoi(dlg.GetValue().c_str());

    m_timer.Stop();
    m_timer.Start(seconds*1000);
}



// -- monitor section --



class Info
{
public:
    HWND application_wnd;
    HWND crash_wnd;
};



static BOOL CALLBACK EnumAllWindowsProc(HWND hwnd, LPARAM lParam)
{
    Info* info = (Info*)lParam;
    
    char buf[512];
    GetWindowTextA(hwnd, buf, 511);
    if (buf[0] == 0)
        return TRUE;
    
    std::vector<std::string>::iterator it;
    
    if (0 == strstr(buf, " Help"))
    {
        for (it = g_captions.begin(); it != g_captions.end(); ++it)
        {
            if (0 == memcmp(buf, it->c_str(), it->length()))
            {
                GetClassNameA(hwnd, buf, 511);

                if (!memcmp(buf, "wx", 2))
                {
                    info->application_wnd = hwnd;
                }
                break;
            }

        }
    }
    

    
    for (it = g_crash_captions.begin(); it != g_crash_captions.end(); ++it)
    {
        if (0 == memcmp(buf, it->c_str(), it->length()))
        {
            // on some systems, a crash window is shown
            // when the program crashes
            info->crash_wnd = hwnd;   
        }
    }

    return TRUE;
}





BOOL IsHung(HWND hwnd)
{
    DWORD d = 1;
    

    LRESULT res = SendMessageTimeout(hwnd,
                                     WM_NULL,
                                     0,
                                     0,
                                     SMTO_NORMAL,
                                     1000,
                                     &d);
    if (res == 0)
        return true;
        
    if (d != 0)
        return true;
        
    return false;
}



bool TerminateProcessId(DWORD process_id)
{
    bool success = false;
    
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, process_id);
    
    if (h)
    {
        success = (FALSE == TerminateProcess(h, (UINT)-1)) ? false : true;

        CloseHandle(h);
    }
    
    return success;
}



void MyFrame::StartApp()
{
    if (m_image_name.Length() > 0)
    {
        wxLogMessage(wxT("Running program..."));
        ::wxExecute(m_image_name);
        Sleep(2000);
    }
     else
    {
        wxLogMessage(wxT("No command specified to run.  Running nothing."));
    }
}


void MyFrame::OnTimer(wxTimerEvent& evt)
{
    Info info;
    info.application_wnd = NULL;
    info.crash_wnd = NULL;
    
    EnumWindows(EnumAllWindowsProc, (LPARAM)&info);

        
    if (info.application_wnd)
    {
        TCHAR class_name[255];
        ::GetClassName(info.application_wnd, class_name, 255);
        
        if (IsHung(info.application_wnd))
        {
            m_last_state_running = false;
            m_hang_count++;
            wxLogMessage(wxT("Application busy/hung/loading/crashed.  Waiting State #%d"), m_hang_count);
        }
         else
        {
            if (m_last_state_running == false)
            {
                wxLogMessage(wxT("Application running."));
            }
            
            m_notrunning_count = 0;
            m_hang_count = 0;
            m_last_state_running = true;
        }
        
    }
     else
    {
        m_last_state_running = false;
        m_notrunning_count++;
        wxLogMessage(wxT("Application not running.  Not Running Count #%d"), m_notrunning_count);
        m_hang_count = 0;
    }


    if (info.application_wnd && m_hang_count >= 4)
    {
        wxLogMessage(wxT("Application has been hanging for some time now.  Trying kill the hanging process..."));
        DWORD dwProcess;
        GetWindowThreadProcessId(info.application_wnd, &dwProcess);
        TerminateProcessId(dwProcess);
        
        if (info.crash_wnd)
        {
            ::SendMessage(info.crash_wnd, WM_CLOSE, 0, 0);
        }
        
        m_hang_count = 0;
        Sleep(5000);
        
        wxLogMessage(wxT("Application should be gone."));
        
        StartApp();
    }
    
    if (m_notrunning_count >= 4)
    {
        wxLogMessage(wxT("Application has not been running for some time now."));
        StartApp();
        m_notrunning_count = 0;
    }
}


