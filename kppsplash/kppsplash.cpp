/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Splash Screen Application
 * Author:   Benjamin I. Williams
 * Created:
 *
 */


#include <wx/wx.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/sound.h>


#ifndef KPP_EVALUATION
#define KPP_EVALUATION  0
#endif


// -- resource IDs --
#define PNG_BACKGROUND_NORMAL           1001
#define PNG_BACKGROUND_EVAL             1002
#define PNG_BUTTON_NONE                 1003
#define PNG_BUTTON_HOVER                1004
#define PNG_BUTTON_DOWN                 1005


// -- bitmap objects --
wxBitmap g_bmp_background;
wxBitmap g_bmp_button_none;
wxBitmap g_bmp_button_hover;
wxBitmap g_bmp_button_down;

// -- Option map --

struct Option
{
    int x;
    int y;
    int w;
    int h;
    //wxBitmap* bmp;
    wxChar* text;
};



const int default_selection_index = 4;

Option options[] = { {  57,  50, 280,  31, wxT("") }, // install
                     { 119, 100, 292,  31, wxT("") }, // show demo
                     { 172, 150, 295,  31, wxT("") }, // quick start
                     { 222, 200, 331,  31, wxT("") }, // exit
                     {   0,   0,   0,   0, NULL }
                   };

bool isAdmin();

class MainFrame : public wxFrame
{
public:

    MainFrame(wxWindow* parent,
              wxWindowID id,
              const wxString& title,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize)
               : wxFrame(parent, id, title, pos, size,
                         wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX)
    {
        wxLogNull log;

        SetIcon(wxIcon(wxT("KPPICON")));

        m_selection = default_selection_index;
        m_button_down = false;
        
        int bmp_width = g_bmp_background.GetWidth();
        int bmp_height = g_bmp_background.GetHeight();

        SetClientSize(bmp_width, bmp_height);

        // -- create font --
        m_font = wxFont(16, wxDEFAULT, wxNORMAL, wxBOLD, false, wxT("Arial"));

        // -- create compatible bitmap for the screen --
        GetClientSize(&m_cliwidth, &m_cliheight);

        m_membmp.Create(bmp_width,
                        bmp_height, -1);
        m_memdc.SelectObject(m_membmp);

        #ifdef WIN32
        wxString path = ::wxGetOSDirectory();
        if (path.Length() > 0)
        {
            if (path.Last() != wxT('\\'))
                path += wxT('\\');
            path += wxT("Media\\start.wav");
        }
        
        if (::wxFileExists(path))
        {
            m_click_sound.Create(path);
        }

        #endif

        render();
    }

    ~MainFrame()
    {
    }

private:

    void render()
    {        
        m_memdc.DrawBitmap(g_bmp_background, 0, 0, false);
        
        m_memdc.SetFont(m_font);

        Option* opt;
        int i = 0;
        for (opt = options; opt->x != 0; ++opt)
        {
            if (i == m_selection)
            {
                if (m_button_down)
                {
                    m_memdc.DrawBitmap(g_bmp_button_down, opt->x, opt->y, true);
                }
                 else
                {
                    m_memdc.DrawBitmap(g_bmp_button_hover, opt->x, opt->y, true);
                }
            }
             else
            {
                m_memdc.DrawBitmap(g_bmp_button_none, opt->x, opt->y, true);
            }

            m_memdc.DrawText(opt->text, opt->x + 57, opt->y + 5);

            ++i;
        }
    }

    void update()
    {
        render();
        Refresh(FALSE);
        Update();
    }

    void onPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);
        dc.Blit(0, 0, m_cliwidth, m_cliheight, &m_memdc, 0, 0);
    }

    void onMouse(wxMouseEvent& event)
    {
        wxEventType type = event.GetEventType();

        if (type == wxEVT_LEFT_DOWN)
        {
            if (GetCapture() != this)
            {
                CaptureMouse();
            }

            m_button_down = true;
            update();

            if (m_selection != -1)
            {
                playClickSound();
            }
        }
         else if (type == wxEVT_LEFT_UP)
        {
            if (GetCapture() == this)
            {
                ReleaseMouse();
            }
            
            m_button_down = false;
            update();

            switch (m_selection)
            {
                case 0:
                {
                    // -- check operating system version --
                    if (::wxGetOsVersion() != wxWINDOWS_NT)
                    {
                        wxMessageBox(_("This application can only be installed on machines with Microsoft Windows NT, 2000, XP, or greater."),
                                     _("Welcome"),
                                     wxOK | wxICON_INFORMATION,
                                     this);
                        break;
                    }

                    if (!canInstall())
                    {
                        wxMessageBox(_("Your user account does not have the necessary privileges to install this application.  Your account must belong to either the Administrators and/or the Power Users group in order to complete this installation.  Please see your system administrator for more information."),
                                     _("Welcome"),
                                     wxOK | wxICON_INFORMATION,
                                     this);
                        break;
                    }

                    // -- install application --
                    wxString path = getExeDirectory();
                    path += wxT("\\setup.exe");
                    //ShellExecute(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    wxExecute(path);
                    break;
                }

                case 1:
                {
                    wxString avi = getExeDirectory();
                    avi += wxT("\\resource\\kppvideo.avi");

                    wxString cmd = getExeDirectory();
                    cmd += wxT("\\resource\\camplay.exe /A /D /E /M /play /S /T ");
                    cmd += avi;

                    // -- show video --
                    wxExecute(cmd);

                    break;
                }

                case 2:
                {
                    #if KPP_EVALUATION == 1
                    showEvaluationGuide();
                    #else
                    showOnlineHelp();
                    #endif

                    break;
                }

                case 3:
                {
                    wxUsleep(100);
                    Destroy();
                    break;
                }
            }
        }
         else if (type == wxEVT_MOTION)
        {
            int sel = getSelectionFromPosition(event.m_x, event.m_y);

            if (m_selection != sel)
            {
                m_selection = sel;
                update();
            }
        }
    }

    void onLeaveWindow(wxMouseEvent& event)
    {
        if (m_selection != default_selection_index)
        {
            m_selection = default_selection_index;
            Refresh(FALSE);
            Update();
        }
    }

    void onEraseBackground(wxEraseEvent& event)
    {
    }

    int getSelectionFromPosition(int x, int y)
    {
        Option* opt = options;
        int idx = 0;

        while (opt->x)
        {
            if (x >= opt->x &&
                y >= opt->y &&
                x <  opt->x + opt->w &&
                y <  opt->y + opt->h)
            {
                return idx;
            }

            ++idx;
            ++opt;
        }

        return -1;
    }
    
    wxString getExeDirectory()
    {
        TCHAR buf[512];
        GetModuleFileName(NULL, buf, 511);
        TCHAR* back_slash = _tcsrchr(buf, _T('\\'));
        if (back_slash)
        {
            *back_slash = 0;
        }
        return buf;
    }

    void playClickSound()
    {
        if (m_click_sound.IsOk())
        {
            m_click_sound.Play(wxSOUND_ASYNC);
        }
    }

    bool canInstall()
    {
        LONG result;
        HKEY hkey;

        result = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                _T("SOFTWARE\\TestCompany\\Test"),
                                0, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS, NULL, &hkey, NULL);

        if (result != ERROR_SUCCESS)
            return false;

        RegCloseKey(hkey);
        
        RegDeleteKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\TestCompany\\Test"));
        return true;
    }

    void showEvaluationGuide()
    {
        // -- evaluation guide --
        
        // -- install avi codec if we can (and if necessary) --
        bool need_codec = true;
        HINSTANCE h = LoadLibrary(_T("tsccvid.dll"));
        if (h)
        {
            FreeLibrary(h);
            need_codec = false;
        }

        if (need_codec && isAdmin())
        {
            wxString cmd = getExeDirectory();
            cmd += wxT("\\resource\\tscc.exe /s");
            wxExecute(cmd);
        }

        wxString path = getExeDirectory();
        path += "\\resource\\kppguide.pdf";
        HINSTANCE hinst;
        hinst = ShellExecute(NULL,
                             "open",
                             path.c_str(),
                             NULL,
                             NULL,
                             SW_SHOWNORMAL);

        if ((unsigned long)hinst <= 32)
        {
            int res = wxMessageBox(_("In order to view the Evaluation Guide, you will need to install Adobe Reader.  Would you like to download Adobe Reader?"),
                         _("Welcome"),
                         wxYES_NO | wxICON_QUESTION,
                         this);

            if (res == wxYES)
            {
                ShellExecute(NULL,
                             "open",
                             "http://www.adobe.com/products/acrobat/readstep2.html",
                             NULL,
                             NULL,
                             SW_SHOWNORMAL);
            }
        }
    }

    void showOnlineHelp()
    {
        // -- install application --
        wxString path = getExeDirectory();
        path += wxT("\\resource\\kpp.chm");
        ShellExecute(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

private:

    int m_selection;
    bool m_button_down;
    wxBitmap m_membmp;
    wxMemoryDC m_memdc;
    int m_cliwidth, m_cliheight;
    wxSound m_click_sound;
    wxFont m_font;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_PAINT(MainFrame::onPaint)
    EVT_LEAVE_WINDOW(MainFrame::onLeaveWindow)
    EVT_MOUSE_EVENTS(MainFrame::onMouse)
    EVT_ERASE_BACKGROUND(MainFrame::onEraseBackground)
END_EVENT_TABLE()




// -- SlashApp implementation --

class SplashApp : public wxApp
{

public:

    SplashApp()
    {
        wxImage::AddHandler(new wxPNGHandler);

#if KPP_EVALUATION == 1
        loadBitmap(g_bmp_background, PNG_BACKGROUND_EVAL);
#else
        loadBitmap(g_bmp_background, PNG_BACKGROUND_NORMAL);
#endif

        loadBitmap(g_bmp_button_none, PNG_BUTTON_NONE);
        loadBitmap(g_bmp_button_hover, PNG_BUTTON_HOVER);
        loadBitmap(g_bmp_button_down, PNG_BUTTON_DOWN);
    }

private:

    bool OnInit()
    {
        MainFrame* frame;

        int width, height;
        width = g_bmp_background.GetWidth();
        height = g_bmp_background.GetHeight();

        frame = new MainFrame(NULL,
                              -1,
                              _("Welcome"),
                              wxDefaultPosition,
                              wxDefaultSize);

        frame->Center();

        SetTopWindow(frame);
        frame->Show();

        return true;
    }

    bool loadBitmap(wxBitmap& bmp, int res_id)
    {
        HMODULE module = GetModuleHandle(NULL);

        HRSRC hrsrc = ::FindResource(module, MAKEINTRESOURCE(res_id), RT_RCDATA);
        if (!hrsrc)
            return false;
    
        DWORD size = ::SizeofResource(module, hrsrc);
        if (!size)
            return false;

        HGLOBAL res = ::LoadResource(module, hrsrc);
        if (!res)
            return false;

        const void* res_data = ::LockResource(res);

        if (!res)
            return false;

        HGLOBAL alloc = ::GlobalAlloc(GMEM_MOVEABLE, size);
        if (!alloc)
            return false;

        void* buf = ::GlobalLock(alloc);
        if (!buf)
        {
            ::GlobalFree(alloc);
            return false;
        }

        CopyMemory(buf, res_data, size);

        wxMemoryInputStream stream(buf, size);
        wxImage img;
        if (!img.LoadFile(stream, wxBITMAP_TYPE_PNG))
        {
            ::GlobalUnlock(alloc);
            ::GlobalFree(alloc);
            return false;
        }

        bmp = wxBitmap(img, -1);

        ::GlobalUnlock(alloc);
        ::GlobalFree(alloc);

        return true;
    }

    DECLARE_EVENT_TABLE()
};

IMPLEMENT_APP(SplashApp)

BEGIN_EVENT_TABLE(SplashApp, wxApp)
END_EVENT_TABLE()






// -- this function returns true if the user is the computer administrator --

bool isAdmin()
{
    HINSTANCE hadvapi32;

    BOOL (__stdcall *pfnOpenThreadToken)(HANDLE, DWORD, BOOL, PHANDLE);

    BOOL (__stdcall *pfnOpenProcessToken)(HANDLE, DWORD, PHANDLE);

    BOOL (__stdcall *pfnGetTokenInformation)(HANDLE, TOKEN_INFORMATION_CLASS,
                                             LPVOID, DWORD, PDWORD);

    BOOL (__stdcall *pfnAllocateAndInitializeSid)(PSID_IDENTIFIER_AUTHORITY,
                                                  BYTE, DWORD,DWORD, DWORD, DWORD,
                                                  DWORD,DWORD, DWORD, DWORD, PSID);

    BOOL (__stdcall *pfnEqualSid)(PSID, PSID);
    PVOID (__stdcall *pfnFreeSid)(PSID);


    HANDLE token;
    UCHAR buf[1024];
    PTOKEN_GROUPS groups = (PTOKEN_GROUPS)buf;
    SID_IDENTIFIER_AUTHORITY nt_auth = SECURITY_NT_AUTHORITY;
    PSID admin_sid;
    bool ret;

    // -- if we are running on Windows 9x, return true --
    if (::wxGetOsVersion() != wxWINDOWS_NT)
        return true;

    hadvapi32 = LoadLibrary("advapi32.dll");
    if (!hadvapi32)
        return false;

    pfnOpenThreadToken = (BOOL (__stdcall *)(HANDLE, DWORD, BOOL, PHANDLE))
                                GetProcAddress(hadvapi32, "OpenThreadToken");

    pfnOpenProcessToken = (BOOL (__stdcall *)(HANDLE, DWORD, PHANDLE))
                                GetProcAddress(hadvapi32, "OpenProcessToken");

    pfnGetTokenInformation = (BOOL (__stdcall *)(HANDLE,
                              TOKEN_INFORMATION_CLASS, LPVOID, DWORD, PDWORD))
                                GetProcAddress(hadvapi32, "GetTokenInformation");

    pfnAllocateAndInitializeSid = (BOOL (__stdcall *)(
                                  PSID_IDENTIFIER_AUTHORITY, BYTE, DWORD, DWORD, DWORD,
                                  DWORD, DWORD, DWORD, DWORD, DWORD, PSID))
                                GetProcAddress(hadvapi32, "AllocateAndInitializeSid");

    pfnEqualSid = (BOOL (__stdcall *)(PSID, PSID))
                                GetProcAddress(hadvapi32, "EqualSid");

    pfnFreeSid = (PVOID (__stdcall *)(PSID))
                                GetProcAddress(hadvapi32, "FreeSid");

    if (!pfnOpenThreadToken ||
        !pfnOpenProcessToken ||
        !pfnGetTokenInformation ||
        !pfnAllocateAndInitializeSid ||
        !pfnEqualSid ||
        !pfnFreeSid)
    {
        FreeLibrary(hadvapi32);
        return false;
    }

    if (!pfnOpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &token))
    {
        if (!pfnOpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        {
            FreeLibrary(hadvapi32);
            return false;
        }
    }

    DWORD buf_size;
    BOOL success = pfnGetTokenInformation(token,
                                          TokenGroups,
                                          buf,
                                          1024,
                                          &buf_size);

    CloseHandle(token);
    FreeLibrary(hadvapi32);

    if (!success)
        return false;

    if (!pfnAllocateAndInitializeSid(&nt_auth, 2,
                                     SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS,
                                     0, 0, 0, 0, 0, 0, &admin_sid))
    {
        return false;
    }

    ret = false;
    unsigned int i;
    for (i = 0; i < groups->GroupCount; ++i)
    {
       if (pfnEqualSid(groups->Groups[i].Sid, admin_sid))
       {
           ret = true;
           break;
       }
    }

    pfnFreeSid(admin_sid);

    return ret;
}
