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



// other utility functions

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
wxString simpleEncryptString(const wxString& s);
wxString simpleDecryptString(const wxString& s);

void suppressConsoleLogging();
wxString getProxyServer();



#endif

