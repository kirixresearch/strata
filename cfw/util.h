/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Benjamin I. Williams
 * Created:  2003-05-19
 *
 */


#ifndef __CFW_UTIL_H
#define __CFW_UTIL_H


#include <cmath>
#include <string>


class wxTreeCtrl;


namespace cfw
{


// -- GUI helper classes and functions --

class CommandCapture : public wxEvtHandler
{
    int m_last_id;

public:

    // -- signals --
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

    bool ProcessEvent(wxEvent& event)
    {
        bool processed = false;

        int event_type = event.GetEventType();

        if (event_type == wxEVT_COMMAND_MENU_SELECTED ||
            event_type == wxEVT_COMMAND_BUTTON_CLICKED ||
            event_type == wxEVT_COMMAND_COMBOBOX_SELECTED ||
            event_type == wxEVT_COMMAND_TEXT_UPDATED)
        {
            processed = true;
            m_last_id = event.GetId();

            sigCommandFired(event.GetEventType(), event.GetId());
        }

        if (processed)
            return true;

        if (GetNextHandler())
        {
            return GetNextHandler()->ProcessEvent(event);
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



// -- locale settings class --

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

    static const Locale* getSettings()
    {
        static Locale* s = NULL;
        if (!s)
        {
            s = new Locale;
        }

        return s;
    }

    static wxChar getDecimalPoint()
    {
        return getSettings()->decimal_point;
    }

    static wxChar getThousandsSeparator()
    {
        return getSettings()->thousands_separator;
    }

    static wxChar getDateSeparator()
    {
        return getSettings()->date_separator;
    }

    static DateOrder getDateOrder()
    {
        return getSettings()->date_order;
    }

    static int getDefaultPaperType()
    {
        return getSettings()->paper_type;
    }


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

    Locale()
    {
        lconv* l = localeconv();
        decimal_point = (unsigned char)*l->decimal_point;
        thousands_separator = (unsigned char)*l->thousands_sep;
        paper_type = wxPAPER_LETTER;

        // get date format
#ifdef WIN32
        TCHAR s[32];
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDATE, s, 32);
        if (*s == wxT('1'))
            date_order = DateOrderDDMMYY;
             else
            date_order = DateOrderMMDDYY;

        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDATE, s, 32);
        date_separator = s[0];
#else
        date_separator = '/';
        date_order = DateOrderMMDDYY;
#endif

        // get default paper size
#ifdef WIN32
        GetLocaleInfo(LOCALE_USER_DEFAULT, 0x0000100A, s, 32);
        if (*s == wxT('1'))
            paper_type = wxPAPER_LETTER;
             else
            paper_type = wxPAPER_A4;
#endif

    }

public:

    wxChar decimal_point;
    wxChar thousands_separator;
    wxChar date_separator;
    DateOrder date_order;
    int paper_type;
};



// -- other functions --

wxFont getDefaultWindowFont();
wxString getUserDocumentFolder();
wxString dbl2fstr(double d, int dec_places = 0);
wxString doubleQuote(const wxString& src, wxChar quote = L'\'');
wxString makeProper(const wxString& input);
wxString makeProperIfNecessary(const wxString& input);

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
void setTreeItemHeight(wxTreeCtrl* ctrl, int height);
int getTreeItemHeight(wxTreeCtrl* ctrl);
wxString simpleEncryptString(const wxString& s);
wxString simpleDecryptString(const wxString& s);

void suppressConsoleLogging();
wxString getProxyServer();


};   // namespace cfw



// -- platform definitions --


#ifdef WIN32
#define PATH_SEPARATOR_CHAR    wxT('\\')
#define PATH_SEPARATOR_STR     wxT("\\")
#else
#define PATH_SEPARATOR_CHAR    wxT('/')
#define PATH_SEPARATOR_STR     wxT("/")
#endif



// -- string conversion functions --


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





#endif

