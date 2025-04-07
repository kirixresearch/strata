/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-05-19
 *
 */


#ifndef H_APP_UTIL_H
#define H_APP_UTIL_H




// -- platform definitions ----------------------------------------------------

#ifdef WIN32
#define PATH_SEPARATOR_CHAR    wxT('\\')
#define PATH_SEPARATOR_STR     wxT("\\")
#else
#define PATH_SEPARATOR_CHAR    wxT('/')
#define PATH_SEPARATOR_STR     wxT("/")
#endif



// -- conversion functions ----------------------------------------------------

#ifndef _DEBUG

    inline std::wstring towstr(const wxString& s)
    {
        return s.ToStdWstring();
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

#else

    class towstr : public std::wstring
    {
        public:

            towstr(const wxString& s) : std::wstring(s.ToStdWstring()) {}

            towstr(const char* s) : std::wstring()
            {
                int i, len = strlen(s);
                resize(len);
                for (i = 0; i < len; ++i)
                    (*this)[i] = (unsigned char)(*(s+i));
            }

        private:
        
            towstr(const std::wstring& s) : std::wstring(s)
            {
                // string is already a std::wstring -- we want to disallow this
            }

            towstr(const wchar_t* s) : std::wstring(s)
            {
                // string is already a const wchar_t* -- we want to disallow this
            }
    };

#endif


inline std::string tostr(const wxString& s)
{
    return s.ToStdString();
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

inline const wxChar* wxcstr(const wxString& s)
{
    return (const wxChar*)s.c_str();
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

inline std::wstring color2string(const wxColor& color)
{
    if (!color.Ok())
        return L"null";
    wchar_t buf[25];
    swprintf(buf, 24, L"#%02x%02x%02x", color.Red(), color.Green(), color.Blue());
    buf[24] = 0;
    return buf;
}

inline wxColor string2color(const std::wstring& str)
{
    if (str == L"null" || str.length() != 7)
        return wxNullColour;

    return wxColor(str);
}


// -- string helper functions -------------------------------------------------


wxString doubleQuote(const wxString& src, wxChar quote = L'\'');

// shortcut so we don't have to use towx everywhere
// this should be able to go away in future versions of wx
wxString filenameToUrl(const wxString& _filename);
wxString urlToFilename(const wxString& _url);

void trimUnwantedUrlChars(wxString& str);

wxString jsEscapeString(const wxString& input, wxChar quote);
wxString urlEscape(const wxString& input);

// this function was necessary because wxString::Replace(..., wxEmptyString)
// was truncating strings
wxString removeChar(const wxString& s, wxChar c);

// TODO: borrowed from expr_util in xd; also exists in kscript
// and parts of kl; should factor
wxChar* zl_strchr(wxChar* str, wxChar ch);

void int2buf(unsigned char* buf, unsigned int i);
int buf2int(const unsigned char* buf);


// TODO: similar functions exist elsewhere; should factor
std::vector< std::pair<std::wstring, bool> > sortExprToVector(const std::wstring& sort_expr);


// -- database helper functions -----------------------------------------------

// checks if a string is a keyword
bool isKeyword(const wxString& str,
               xd::IDatabasePtr db = xcm::null);

// checks if a field name is valid
bool isValidFieldName(
               const wxString& str,
               xd::IDatabasePtr db = xcm::null,
               int* err_idx = NULL);

// checks if an object name is valid
bool isValidObjectName(
               const wxString& str,
               xd::IDatabasePtr db = xcm::null,
               int* err_idx = NULL);

// checks is a path name is valid (same as isObjectNameValid() + slashes)
bool isValidObjectPath(
               const wxString& str,
               xd::IDatabasePtr db = xcm::null,
               int* err_idx = NULL);

bool isValidTable(
               const std::wstring& str,
               xd::IDatabasePtr db = xcm::null);


// makes a valid field name out of the given string
wxString makeValidFieldName(
               const wxString& str,
               xd::IDatabasePtr db = xcm::null);

// makes a valid object name out of the given string
wxString makeValidObjectName(
               const wxString& str,
               xd::IDatabasePtr db = xcm::null);

// returns a localized name for a given type
wxString getDbColumnTypeText(int xd_type);

bool isSamePath(const std::wstring& path1, const std::wstring& path2);

// for mounted folders or singleton mounts, these functions get
// the actual object path or actual filename, respectively
wxString getObjectPathFromMountPath(const wxString& database_path);
wxString getPhysPathFromDatabasePath(const wxString& database_path);
std::wstring getMountRoot(xd::IDatabasePtr db, const std::wstring& path);
bool getMountPointHelper(xd::IDatabasePtr& db, const std::wstring& path, std::wstring& cstr, std::wstring& rpath);
bool xd::isTemporaryPath(const std::wstring& path);

// gets the filename from the path
wxString getFilenameFromPath(const wxString& path, bool include_extension = true);
wxString getDirectoryFromPath(const wxString& path);
std::wstring getExtensionFromPath(const std::wstring& path);
std::wstring determineMimeType(const std::wstring& path);

// utility for adding a filesystem extension to a path if the path is a
// mounted filesystem database and the path doesn't specify an extension
std::wstring addExtensionIfExternalFsDatabase(const std::wstring& path, const std::wstring& ext);

// stream text file loading and saving routines
bool readStreamTextFile(xd::IDatabasePtr db,
                        const std::wstring& path,
                        std::wstring& result_text);

bool writeStreamTextFile(xd::IDatabasePtr db,
                        const std::wstring& path,
                        const std::wstring& text,
                        const std::wstring& mime_type = L"text/plain");

// this function simply attempts to create a folder that can be as many layers
// deep as desired (a sub-sub-sub-folder) -- if the process fails or if the
// "delete_on_success" flag is set to true, any folders that have been
// created will be deleted before returning true or false
bool tryCreateFolderStructure(const wxString& folder_path);

// this function simply attempts to create a folder that can be as many layers
// deep as desired (a sub-sub-sub-folder) -- regardless of whether or not
// it succeeds, it deletes the folders it creates
bool createFolderStructure(const wxString& folder_path,
                           bool delete_on_success = false);

// this function does not recursively check the project's subfolders,
// instead it approximates the project's size by calculating the size
// of the "data" folder inside the project -- this is a fast operation
double getProjectSize(const wxString& project_path);

// returns the default location where projects are created
wxString getDefaultProjectsPath();

xd::IndexInfo lookupIndex(const xd::IndexInfoEnum& indexes, 
                          const std::wstring& fields,
                          bool order_matters);


// creates the job parameters for a sort/filter job
kl::JsonNode createSortFilterJobParams(const std::wstring& path,
                                       const std::wstring& filter,
                                       const std::wstring& order);


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

jobs::IJobPtr appCreateJob(const std::wstring& job_id);


// -- other utility functions -------------------------------------------------

bool isValidFileVersion(kl::JsonNode node, const std::wstring& type, int version);

// perform simple/small web requests, returning result as a string
std::wstring doHttpRequest(const std::wstring& url, const std::map<std::wstring,std::wstring>& post_params = std::map<std::wstring,std::wstring>(), const std::wstring& cookie_file = L"");

// checks an output path for g_app->getDatabase().  It will
// pop up an error message if something is wrong.
bool doOutputPathCheck(const wxString& output_path, wxWindow* parent = NULL);

// creates a standard-looking banner control for all modules
kcl::BannerControl* createModuleBanner(wxWindow* parent, const wxString& title);

void setFocusDeferred(wxWindow* focus);
bool windowOrChildHasFocus(wxWindow* wnd);

// this function returns the height of the taskbar
// on Windows or 50px for other operating systems
int getTaskBarHeight();

wxBitmap rescaleBitmap(wxWindow* wnd, wxBitmap bitmap);

// returns the default location for the user's document storage;
// on Windows, this will normally be the user's "My Documents" directory;
// on other systems, the user's home folder
wxString getUserDocumentFolder();

void limitFontSize(wxWindow* wnd, int size = 12);
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

void openExplorerWindow(const wxString& directoryPath);
void openExplorerWindowAndSelectPath(const wxString& path);


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
        if (kl::thread::isMain())
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
                        jobs::IJobQueuePtr _job_queue,
                        wxGauge* _gauge);
    
    ~JobGaugeUpdateTimer();

    void hideJobFailedIcon();
    void showJobFailedIcon();
    
    void UnInit();
    void Notify();
    
    void onJobAdded(jobs::IJobInfoPtr job_info);
    void onJobStateChanged(jobs::IJobInfoPtr job_info);

    void onJobAddedInMainThread(wxCommandEvent& evt);
    void onJobStateChangedInMainThread(wxCommandEvent& evt);
    
private:

    IStatusBarPtr m_statusbar;
    jobs::IJobQueuePtr m_job_queue;
    
    IStatusBarItemPtr m_job_text_item;
    IStatusBarItemPtr m_job_separator_item;
    IStatusBarItemPtr m_job_gauge_item;
    IStatusBarItemPtr m_job_failed_item;

    wxGauge* m_gauge;
    
    DECLARE_EVENT_TABLE()
};


class AppException : public std::exception
{
public:
    AppException(const wxString& message) 
        : m_message(message), 
          m_what(tostr(message)) 
    {}
    
    const wxString& GetMessage() const { return m_message; }
    const char* what() const noexcept override { return m_what.c_str(); }
    
private:
    wxString m_message;
    std::string m_what;
};


#endif // __APP_UTIL_H

