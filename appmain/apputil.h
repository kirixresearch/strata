/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-05-23
 *
 */


#ifndef __APP_APPUTIL_H
#define __APP_APPUTIL_H


// checks an output path for g_app->getDatabase().  It will
// pop up an error message if something is wrong.

bool doOutputPathCheck(const wxString& output_path, wxWindow* parent = NULL);

// displays a generic 'invalid' message box

void appInvalidObjectMessageBox(const wxString& name = wxEmptyString,
                                wxWindow* parent = NULL);

void appInvalidFieldMessageBox(const wxString& name = wxEmptyString,
                               wxWindow* parent = NULL);

// creates a standard-looking banner control for all modules

kcl::BannerControl* createModuleBanner(wxWindow* parent, const wxString& title);

// creates a standard label-spacer-text control sizer 

wxBoxSizer* createLabelTextControlSizer(wxWindow* parent,
                                        const wxString& label,
                                        wxTextCtrl** textctrl,
                                        wxWindowID textctrl_id,
                                        const wxString& textctrl_text,
                                        int spacer = 0);

void makeSizerLabelsSameSize(wxBoxSizer* sizer1,
                             wxBoxSizer* sizer2,
                             wxBoxSizer* sizer3 = NULL,
                             wxBoxSizer* sizer4 = NULL,
                             wxBoxSizer* sizer5 = NULL,
                             wxBoxSizer* sizer6 = NULL,
                             wxBoxSizer* sizer7 = NULL);



int color2int(const wxColor& color);     // convert wxColor to int
wxColor int2color(int int_color);        // convert int to wxColor
void setFocusDeferred(wxWindow* focus);
void autoSizeListHeader(wxListCtrl* ctrl);
bool windowOrChildHasFocus(wxWindow* wnd);

int getTaskBarHeight();  // get OS's taskbar height


// utilities which looking and/or activate a specified documentsite

cfw::IDocumentSitePtr lookupOtherDocumentSite(
                           cfw::IDocumentSitePtr site,
                           const std::string& doc_class_name);

cfw::IDocumentPtr lookupOtherDocument(
                           cfw::IDocumentSitePtr site,
                           const std::string& doc_class_name);

void switchToOtherDocument(cfw::IDocumentSitePtr site,
                           const std::string& doc_class_name);

wxWindow* getDocumentSiteWindow(cfw::IDocumentSitePtr site);




// -- database helper classes and functions ----------------------------------


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

// checks is a path name is valid
// (same as isObjectNameValid() + slashes)
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

// determines if two tango types are compatible
bool isTypeCompatible(int type1, int type2);

// returns a localized name for a given type
wxString getTypeText(int tango_type);

// quotes a database identifier
wxString quoteIdentifier(tango::IDatabasePtr db, const wxString& identifier);
wxString dequoteIdentifier(tango::IDatabasePtr db, const wxString& identifier);

// for mounted folders or singleton mounts, these functions get
// the actual object path or actual filename, respectively

wxString getObjectPathFromMountPath(const wxString& database_path);
wxString getPhysPathFromMountPath(const wxString& database_path);
std::wstring getMountRoot(tango::IDatabasePtr db, const std::wstring path);
bool getMountPointHelper(tango::IDatabasePtr& db, const wxString& _path, wxString& cstr, wxString& rpath);

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

tango::IIndexInfoPtr lookupIndex(tango::IIndexInfoEnumPtr indexes, const std::wstring& fields, bool order_matters);


// string helper functions

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

// helper functions to convert vectors of wxStrings
// into comma-delimited lists, and vice versa

wxString vectorToString(const std::vector<wxString>& list);
std::vector<wxString> stringToVector(const wxString& string);
std::vector< std::pair<wxString, bool> > sortExprToVector(const wxString& sort_expr);

wxString getWebFile(const wxString& urlstring);      // get small web files, returning them as strings
double getProjectSize(const wxString& project_path); // returns the approximate size of a project
wxString getDefaultProjectsPath();                   // returns the default location where projects are created



void int2buf(unsigned char* buf, unsigned int i);
int buf2int(const unsigned char* buf);





//  utility classes

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

    JobGaugeUpdateTimer(cfw::IStatusBarPtr _statusbar,
                        cfw::IJobQueuePtr _job_queue,
                        wxGauge* _gauge);
    
    ~JobGaugeUpdateTimer();

    void hideJobFailedIcon();
    void showJobFailedIcon();
    
    void UnInit();
    void Notify();
    
    void onJobAdded(cfw::IJobInfoPtr job_info);
    void onJobStateChanged(cfw::IJobInfoPtr job_info);

    void onJobAddedInMainThread(wxCommandEvent& evt);
    void onJobStateChangedInMainThread(wxCommandEvent& evt);
    
private:

    cfw::IStatusBarPtr m_statusbar;
    cfw::IJobQueuePtr m_job_queue;
    
    cfw::IStatusBarItemPtr m_job_text_item;
    cfw::IStatusBarItemPtr m_job_separator_item;
    cfw::IStatusBarItemPtr m_job_gauge_item;
    cfw::IStatusBarItemPtr m_job_failed_item;

    wxGauge* m_gauge;
    
    DECLARE_EVENT_TABLE()
};


class CustomPromptDlg : public wxDialog
{
public:
    
    enum Bitmaps
    {
        bitmapError = 0,
        bitmapQuestion,
        bitmapWarning,
        bitmapInformation
    };
    
    enum ButtonFlags
    {
        showButton1 = 0x01,
        showButton2 = 0x02,
        showCancel  = 0x04
    };
    
    CustomPromptDlg(wxWindow* parent, 
                    const wxString& caption,
                    const wxString& message,
                    const wxSize& size = wxDefaultSize);
    ~CustomPromptDlg();
    
    int ShowModal();
    
    // for bitmap ids, reference the list in the ArtProvider class
    void setBitmap(int bitmap_id);
    void setButton1(int id, const wxString& label);
    void setButton2(int id, const wxString& label);
    void showButtons(int button_flags);

private:

    // event handlers
    void onChoice1Clicked(wxCommandEvent& evt);
    void onChoice2Clicked(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);

private:

    int m_bitmap_id;
    int m_button_flags;
    int m_button1_id;
    int m_button2_id;
    wxString m_button1_label;
    wxString m_button2_label;
    wxString m_message;
    
    DECLARE_EVENT_TABLE()
};




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
        if (wxThread::IsMain())
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





#endif

