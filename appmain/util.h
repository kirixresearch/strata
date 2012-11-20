/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-05-19
 *
 */


#ifndef __APP_UTIL_H
#define __APP_UTIL_H


#include <cmath>
#include <string>
#include <kl/thread.h>



// -- platform definitions ----------------------------------------------------

#ifdef WIN32
#define PATH_SEPARATOR_CHAR    wxT('\\')
#define PATH_SEPARATOR_STR     wxT("\\")
#else
#define PATH_SEPARATOR_CHAR    wxT('/')
#define PATH_SEPARATOR_STR     wxT("/")
#endif



// -- conversion functions ----------------------------------------------------

inline wxString towx(const char* s)
{
#ifdef _UNICODE
    return wxString::From8BitData(s);
#else
    return s;
#endif
}

inline wxString towx(const wchar_t* s)
{
#ifdef _UNICODE
    return s;
#else
    wxString result;
    int len = wcslen(s);
    result.Alloc(len);
    for (int i = 0; i < len; ++i)
    {
        if (s[i] > 255)
            result += '?';
             else
            result += ((char)s[i]);
    }
    return result;
#endif
}

inline wxString towx(const std::string& s)
{
#ifdef _UNICODE
    return wxString::From8BitData(s.c_str());
#else
    return s.c_str();
#endif
}

inline wxString towx(const std::wstring& s)
{
#ifdef _UNICODE
    return s.c_str();
#else
    wxString result;
    int len = s.length();
    result.Alloc(len);
    for (int i = 0; i < len; ++i)
    {
        if (s[i] > 255)
            result += '?';
             else
            result += ((char)s[i]);
    }
    return result;
#endif
}

inline wxString towx(char c)
{
    wxChar s[2];
    s[0] = (unsigned char)c;
    s[1] = 0;
    return s;
}

inline wxString towx(wchar_t c)
{
    wxChar s[2];
    s[0] = c;
    s[1] = 0;
    return s;
}

inline const wxChar* wxcstr(const wxString& s)
{
    return (const wxChar*)s.c_str();
}

inline std::wstring towstr(const wxString& s)
{
#ifdef _UNICODE
    return (const wchar_t*)s.wc_str();
#else
    std::wstring result;
    int i, len;
    len = s.Length();
    result.resize(len);
    for (i = 0; i < len; ++i)
        result[i] = (unsigned char)s[i];
    return result;
#endif
}

inline std::wstring towstr(const char* s)
{
    std::wstring ret;
    int i, len = strlen(s);
    ret.resize(len);
    for (i = 0; i < len; ++i)
        ret[i] = (unsigned char)(*(s+i));
    return ret;
}

inline std::string tostr(const wxString& s)
{
#ifdef _UNICODE
    return (const char*)s.mbc_str();
#else
    return s.c_str();
#endif
}

inline std::string tostr(const std::wstring& s)
{
    std::string ret;
    int i, len = s.length();
    ret.resize(len);
    for (i = 0; i < len; ++i)
        ret[i] = (char)s[i];
    return ret;
}

inline int color2int(const wxColor& color)
{
    if (color.Ok())
        return (color.Red() << 16) | (color.Green() << 8) | color.Blue();

    return -1;
}

inline wxColor int2color(int int_color)
{
    if (int_color == -1)
        return wxNullColour;

    return wxColor(int_color >> 16, (int_color >> 8) & 0xff, int_color & 0xff);
}



// -- string helper functions -------------------------------------------------

wxString dbl2fstr(double d, int dec_places = 0);

wxString doubleQuote(const wxString& src, wxChar quote = L'\'');
wxString makeProper(const wxString& input);
wxString makeProperIfNecessary(const wxString& input);

bool isUnicodeString(const std::wstring& val);

wxString filenameToUrl(const wxString& _filename);
wxString urlToFilename(const wxString& _url);

void trimUnwantedUrlChars(wxString& str);

wxString jsEscapeString(const wxString& input, wxChar quote);
wxString urlEscape(const wxString& input);
wxString multipartEncode(const wxString& input);

wxString removeChar(const wxString& s, wxChar c);
wxChar* zl_strchr(wxChar* str, wxChar ch);

wxString makeUniqueString();

void int2buf(unsigned char* buf, unsigned int i);
int buf2int(const unsigned char* buf);

wxString vectorToString(const std::vector<wxString>& list);
std::vector<wxString> stringToVector(const wxString& string);
std::vector< std::pair<wxString, bool> > sortExprToVector(const wxString& sort_expr);



// -- database helper functions -----------------------------------------------

// checks if a string is a keyword
bool isKeyword(const wxString& str,
               tango::IDatabasePtr db = xcm::null);

// checks if a field name is valid
bool isValidFieldName(
               const wxString& str,
               tango::IDatabasePtr db = xcm::null,
               int* err_idx = NULL);

// checks if an object name is valid
bool isValidObjectName(
               const wxString& str,
               tango::IDatabasePtr db = xcm::null,
               int* err_idx = NULL);

// checks is a path name is valid (same as isObjectNameValid() + slashes)
bool isValidObjectPath(
               const wxString& str,
               tango::IDatabasePtr db = xcm::null,
               int* err_idx = NULL);

// makes a valid field name out of the given string
wxString makeValidFieldName(
               const wxString& str,
               tango::IDatabasePtr db = xcm::null);

// makes a valid object name out of the given string
wxString makeValidObjectName(
               const wxString& str,
               tango::IDatabasePtr db = xcm::null);

// returns a localized name for a given type
wxString getTypeText(int tango_type);

// for mounted folders or singleton mounts, these functions get
// the actual object path or actual filename, respectively
wxString getObjectPathFromMountPath(const wxString& database_path);
wxString getPhysPathFromMountPath(const wxString& database_path);
std::wstring getMountRoot(tango::IDatabasePtr db, const std::wstring path);
bool getMountPointHelper(tango::IDatabasePtr& db, const wxString& _path, wxString& cstr, wxString& rpath);

std::wstring getDbDriverFromSet(tango::ISetPtr set);

// gets the filename from the path
wxString getFilenameFromPath(const wxString& path, bool include_extension = true);
wxString getDirectoryFromPath(const wxString& path);
wxString getExtensionFromPath(const wxString& path);
wxString getMimeTypeFromExtension(const wxString& path);
wxString determineMimeType(const wxString& path);

// utility for adding a filesystem extension to a path if the path is a
// mounted filesystem database and the path doesn't specify an extension
wxString addExtensionIfExternalFsDatabase(const wxString& path, const wxString& ext);

// stream text file loading and saving routines
bool readStreamTextFile(tango::IDatabasePtr db,
                        const std::wstring& path,
                        std::wstring& result_text);

bool writeStreamTextFile(tango::IDatabasePtr db,
                        const std::wstring& path,
                        const std::wstring& text,
                        const std::wstring& mime_type = L"text/plain");

// function to handle (sub)folder creation gracefully
bool tryCreateFolderStructure(const wxString& folder_path);
bool createFolderStructure(const wxString& folder_path,
                           bool delete_on_success = false);

// returns the approximate size of a project
double getProjectSize(const wxString& project_path);

// returns the default location where projects are created
wxString getDefaultProjectsPath();

tango::IIndexInfoPtr lookupIndex(tango::IIndexInfoEnumPtr indexes, 
                                 const std::wstring& fields, bool order_matters);



// -- message box helpers -----------------------------------------------------

void appInvalidObjectMessageBox(const wxString& name = wxEmptyString,
                                wxWindow* parent = NULL);

void appInvalidFieldMessageBox(const wxString& name = wxEmptyString,
                               wxWindow* parent = NULL);

int appMessageBox(const wxString& message,
                  const wxString& caption = wxT("Message"),
                  int style = wxOK | wxCENTRE,
                  wxWindow *parent = NULL,
                  int x = -1, int y = -1);

void deferredAppMessageBox(const wxString& message,
                           const wxString& caption = wxT("Message"),
                           int style = wxOK | wxCENTRE,
                           wxWindow *parent = NULL,
                           int x = -1, int y = -1);



// -- utilities which look for and/or activate a specified document site ------

IDocumentSitePtr lookupOtherDocumentSite(
                           IDocumentSitePtr site,
                           const std::string& doc_class_name);

IDocumentPtr lookupOtherDocument(
                           IDocumentSitePtr site,
                           const std::string& doc_class_name);

void switchToOtherDocument(IDocumentSitePtr site,
                           const std::string& doc_class_name);

wxWindow* getDocumentSiteWindow(IDocumentSitePtr site);



// -- other utility functions -------------------------------------------------

// get small web files, returning them as strings
wxString getWebFile(const wxString& urlstring);

// checks an output path for g_app->getDatabase().  It will
// pop up an error message if something is wrong.
bool doOutputPathCheck(const wxString& output_path, wxWindow* parent = NULL);

// creates a standard-looking banner control for all modules
kcl::BannerControl* createModuleBanner(wxWindow* parent, const wxString& title);

void setFocusDeferred(wxWindow* focus);
bool windowOrChildHasFocus(wxWindow* wnd);

int getTaskBarHeight();  // get OS's taskbar height

wxFont getDefaultWindowFont();
wxString getUserDocumentFolder();

void limitFontSize(wxWindow* wnd, int size = 12);
void makeTextCtrlLowerCase(wxTextCtrl* text);
void makeTextCtrlUpperCase(wxTextCtrl* text);
void makeFontBold(wxWindow* window);
void setTextWrap(wxStaticText* text, bool wrap);
void resizeStaticText(wxStaticText* text, int width = -1);
wxSize getMaxTextSize(wxStaticText* st0,
                      wxStaticText* st1,
                      wxStaticText* st2 = NULL,
                      wxStaticText* st3 = NULL,
                      wxStaticText* st4 = NULL,
                      wxStaticText* st5 = NULL,
                      wxStaticText* st6 = NULL);

void suppressConsoleLogging();
wxString getProxyServer();



// -- utility classes ---------------------------------------------------------

// AppBusyCursor implements wxBusyCursor, but only when APP_GUI==1;
// i.e. if the console mode is active, this does nothing
class AppBusyCursor
{
public:
    AppBusyCursor() {}
private:

#if APP_GUI==1
    wxBusyCursor m_bc;
#endif
};


// this class shows a busy cursor if the code is running in the
// main thread.  If it's running in a thread which isn't the main 
// thread, no action is performed
class AppBusyCursorIfMainThread
{
public:

    AppBusyCursorIfMainThread()
    {
#if APP_GUI==1
        m_bc = NULL;
        if (kl::Thread::isMain())
            m_bc = new wxBusyCursor;
#endif
    }
    
    ~AppBusyCursorIfMainThread()
    {
#if APP_GUI==1
        delete m_bc;
#endif
    }
    
private:

#if APP_GUI==1
    wxBusyCursor* m_bc;
#endif
};


class CommandCapture : public wxEvtHandler
{
    int m_last_id;

public:

    // signals 
    xcm::signal2<int, int> sigCommandFired;

public:
    
    CommandCapture()
    {
        m_last_id = 0;
    }

    int getLastCommandId()
    {
        return m_last_id;
    }

    bool ProcessEvent(wxEvent& evt)
    {
        bool processed = false;

        int event_type = evt.GetEventType();

        if (event_type == wxEVT_COMMAND_MENU_SELECTED ||
            event_type == wxEVT_COMMAND_BUTTON_CLICKED ||
            event_type == wxEVT_COMMAND_COMBOBOX_SELECTED ||
            event_type == wxEVT_COMMAND_TEXT_UPDATED)
        {
            processed = true;
            m_last_id = evt.GetId();

            sigCommandFired(evt.GetEventType(), evt.GetId());
        }

        if (processed)
            return true;

        if (GetNextHandler())
        {
            return GetNextHandler()->ProcessEvent(evt);
        }

        return false;
    }
};


class wxBitmapMenu : public wxMenu
{

public:

    void Append(int id,
                const wxString& text,
                const wxBitmap& bitmap = wxNullBitmap,
                const wxString& helpString = wxEmptyString,
                bool checkable = false)
    {
        #ifdef __WXMAC__
        wxMenuItem* m =  new wxMenuItem(this, id, text, helpString);
        #else
        wxMenuItem* m =  new wxMenuItem(this, id, text, helpString, checkable ? wxITEM_CHECK : wxITEM_NORMAL);
        #endif

        m->SetBitmap(bitmap);
        #ifdef WIN32
        m->SetMarginWidth(20);
        #endif
        wxMenu::Append(m);
    }

    void Append(int id,
                const wxString& text,
                wxMenu* submenu,
                const wxBitmap& bitmap = wxNullBitmap,
                const wxString& helpString = wxEmptyString,
                bool checkable = false)
    {
        #ifdef __WXMAC__
        wxMenuItem* m = new wxMenuItem(this, id, text, helpString);
        #else
        wxMenuItem* m = new wxMenuItem(this, id, text, helpString, checkable ? wxITEM_CHECK : wxITEM_NORMAL);
        #endif

        m->SetBitmap(bitmap);
        #ifdef WIN32
        m->SetMarginWidth(20);
        #endif
        m->SetSubMenu(submenu);
        wxMenu::Append(m);
    }
};


class Locale
{
public:

    enum DateOrder
    {
        DateOrderMMDDYY = 0,
        DateOrderDDMMYY = 1,
        DateOrderYYMMDD = 2
    };

public:

    static const Locale* getSettings();

    static wxChar getDecimalPoint()        { return getSettings()->decimal_point; }
    static wxChar getThousandsSeparator()  { return getSettings()->thousands_separator; }
    static wxChar getDateSeparator()       { return getSettings()->date_separator; }
    static DateOrder getDateOrder()        { return getSettings()->date_order; }
    static int getDefaultPaperType()       { return getSettings()->paper_type; }

    static wxString formatDate(int year,
                               int month,
                               int day,
                               int hour = -1,
                               int min = 0,
                               int sec = 0);

    static bool parseDateTime(const wxString& input,
                              int* year,
                              int* month,
                              int* day,
                              int* hour,
                              int* minute,
                              int* second);
public:

    Locale();
    ~Locale();

public:

    wxChar decimal_point;
    wxChar thousands_separator;
    wxChar date_separator;
    DateOrder date_order;
    int paper_type;
};


class wxDoubleClickGauge : public wxGauge
{
public:

    wxDoubleClickGauge(wxWindow* parent,
            wxWindowID id,
            int range,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxGA_HORIZONTAL) : wxGauge(parent, id, range, pos, size, style)
    {
        m_last_click = 0;
    }

private:

#ifdef WIN32
    WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
    {
        if (message == WM_LBUTTONDOWN)
        {
            clock_t click_time = clock();
            
            double ms = click_time - m_last_click;
            ms = (ms/((double)CLOCKS_PER_SEC))*1000;
            
            m_last_click = click_time;
            
            if (ms <= GetDoubleClickTime())
            {
                wxMouseEvent evt(wxEVT_LEFT_DCLICK);
                InitMouseEvent(evt, LOWORD(lParam), HIWORD(lParam), wParam);
                GetEventHandler()->ProcessEvent(evt);
                return 0;
            }
        }
        
        return wxGauge::MSWWindowProc(message, wParam, lParam);
    }
#endif

    clock_t m_last_click;
};


class JobGaugeUpdateTimer : public wxTimer,
                            public xcm::signal_sink
{
public:

    JobGaugeUpdateTimer(IStatusBarPtr _statusbar,
                        IJobQueuePtr _job_queue,
                        wxGauge* _gauge);
    
    ~JobGaugeUpdateTimer();

    void hideJobFailedIcon();
    void showJobFailedIcon();
    
    void UnInit();
    void Notify();
    
    void onJobAdded(IJobInfoPtr job_info);
    void onJobStateChanged(IJobInfoPtr job_info);

    void onJobAddedInMainThread(wxCommandEvent& evt);
    void onJobStateChangedInMainThread(wxCommandEvent& evt);
    
private:

    IStatusBarPtr m_statusbar;
    IJobQueuePtr m_job_queue;
    
    IStatusBarItemPtr m_job_text_item;
    IStatusBarItemPtr m_job_separator_item;
    IStatusBarItemPtr m_job_gauge_item;
    IStatusBarItemPtr m_job_failed_item;

    wxGauge* m_gauge;
    
    DECLARE_EVENT_TABLE()
};


#endif // __APP_UTIL_H

