/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2004-04-03
 *
 */



#include "appmain.h"
#include <wx/tokenzr.h>
#include <wx/config.h>
#include <xcm/xcm.h>
#include <kl/math.h>
#include "util.h"

#ifdef WIN32
#include "wx/fontutil.h"
#endif

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif


// needed by getUserDocumentFolder on win32 systems
#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#endif



struct LocalePtrContainer
{
    LocalePtrContainer()
    {
        m_ptr = NULL;
    }

    ~LocalePtrContainer()
    {
        delete m_ptr;
    }

    Locale* getLocalePtr()
    {
        if (m_ptr)
            return m_ptr;
        m_ptr = new Locale;
        return m_ptr;
    }

    Locale* m_ptr;
};

LocalePtrContainer g_locale_ptr_container;


Locale::Locale()
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

Locale::~Locale()
{
}

// static
const Locale* Locale::getSettings()
{
    return g_locale_ptr_container.getLocalePtr();
}


wxString Locale::formatDate(int year,
                            int month,
                            int day,
                            int hour,
                            int min,
                            int sec)
{
    static int date_order = -1;
    static wxChar date_separator = 0;

    if (date_order == -1)
    {
        date_order = Locale::getSettings()->date_order;
        date_separator = Locale::getSettings()->date_separator;
    }

    if (date_order == DateOrderMMDDYY)
    {
        if (hour == -1)
        {
            return wxString::Format(wxT("%02d%c%02d%c%04d"),
                                    month, date_separator,
                                    day,   date_separator,
                                    year);
        }
         else
        {
            return wxString::Format(wxT("%02d%c%02d%c%04d %02d:%02d:%02d"),
                                    month, date_separator,
                                    day,   date_separator,
                                    year,
                                    hour, min, sec);
        }
    }
     else if (date_order == DateOrderDDMMYY)
    {
        if (hour == -1)
        {
            return wxString::Format(wxT("%02d%c%02d%c%04d"),
                                    day,   date_separator,
                                    month, date_separator,
                                    year);
        }
         else
        {
            return wxString::Format(wxT("%02d%c%02d%c%04d %02d:%02d:%02d"),
                                    day,   date_separator,
                                    month, date_separator,
                                    year,
                                    hour, min, sec);
        }
    }

    return wxEmptyString;
}


bool Locale::parseDateTime(const wxString& input,
                           int* year,
                           int* month,
                           int* day,
                           int* hour,
                           int* minute,
                           int* second)
{
    if (year)
        *year = -1;
    if (month)
        *month = -1;
    if (day)
        *day = -1;
    if (hour)
        *hour = -1;
    if (minute)
        *minute = -1;
    if (second)
        *second = -1;

    std::vector<int> parts;

    wxStringTokenizer tkz(input, wxT("/-.: "));
    while (tkz.HasMoreTokens())
    {
        wxString part = tkz.GetNextToken();
        if (part.IsEmpty())
            continue;
        parts.push_back(wxAtoi(part));
    }

    int part_count = parts.size();

    if (part_count < 3)
    {
        return false;
    }    

    int y = 0, m = 0, d = 0, hh = -1, mm = -1, ss = 0;


    // date part positions
    int year_pos = 2;
    int month_pos = 0;
    int day_pos = 1;

    if (Locale::getDateOrder() == Locale::DateOrderDDMMYY)
    {
        year_pos = 2;
        month_pos = 1;
        day_pos = 0;
    }


    // first assign date portion
    y = parts[year_pos];
    m = parts[month_pos];
    d = parts[day_pos];

    if (part_count == 4)
        return false;

    if (part_count >= 5)
    {
        // time is included
        hh = parts[3];
        mm = parts[4];

        if (part_count > 5)
        {
            ss = parts[5];
        }
    }


    // check validity
    if (m <= 0 || m > 12)
    {
        return false;
    }

    int month_days[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (d <= 0 || d > month_days[m-1])
    {
        return false;
    }

    if (y < 70)
    {
        y += 2000;
    }
     else if (y < 100)
    {
        y += 1900;
    }

    if (part_count >= 5)
    {
        if (hh == 24)
            hh = 0;

        if (hh < 0 || hh >= 24)
            return false;

        if (m < 0 || mm >= 60)
            return false;
        
        if (part_count > 5)
        {
            if (ss < 0 || ss >= 60)
                return false;
        }
    }

    if (year)
        *year = y;
    if (month)
        *month = m;
    if (day)
        *day = d;
    if (hour)
        *hour = hh;
    if (minute)
        *minute = mm;
    if (second)
        *second = ss;

    return true;
}





// getDefaultFont() returns the default font for window contents

wxFont getDefaultWindowFont()
{
#ifdef WIN32
    NONCLIENTMETRICS nm;
    nm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,0,&nm,0);

    wxNativeFontInfo info;
    memcpy(&info.lf, &nm.lfMessageFont, sizeof(LOGFONT));
    wxFont font;
    font.Create(info, 0);
    return font;
#else
    return wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
#endif
}


// getUserDocumentFolder() returns the default location for the user's
// document storage.  On Windows, this will normally be the user's
// "My Documents" directory.  On other systems, the user's home folder


wxString getUserDocumentFolder()
{
#ifdef WIN32
    TCHAR path[512];
    LPITEMIDLIST pidl = NULL;
    if (FAILED(SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl)))
    {
        return _T("C:\\");
    }
    if (!pidl)
    {
        return _T("C:\\");
    }
    SHGetPathFromIDList(pidl, path);
    LPMALLOC pIMalloc;
    SHGetMalloc(&pIMalloc);
    pIMalloc->Free(pidl);
    pIMalloc->Release();
    if (_tcslen(path) == 0)
        return _T("C:\\");
    return path;
#else
    char path[512];
    char* home = getenv("HOME");
    if (home == NULL || strlen(home) == 0)
    {
        return _T("/");
    }
    strcpy(path, home);
    return towx(path);
#endif

}




wxString dbl2fstr(double d, int dec_places)
{
    // the first time we are run, get locale information about
    // the system's decimal point character and thousands
    // separator character

    static wxChar thousands_sep = 0;
    static wxChar decimal_point = 0;
    if (!decimal_point)
    {
        struct lconv* l = localeconv();
        thousands_sep = (unsigned char)*(l->thousands_sep);
        decimal_point = (unsigned char)*(l->decimal_point);

        if (thousands_sep == 0)
            thousands_sep = wxT(',');
        if (decimal_point == 0)
            decimal_point = wxT('.');
    }

    // initialize result area
    wxChar result[128];
    wxChar* string_start;
    memset(result, 0, sizeof(wxChar)*80);

    double decp, intp;
    int digit;
    bool negative = false;

    if (d < 0.0)
    {
        negative = true;
        d = fabs(d);
    }

    // split the number up into integer and fraction portions
    d = kl::dblround(d, dec_places);
    decp = modf(d, &intp);
    //decp = kl::dblround(decp, dec_places);

    int i = 0;

    int pos = 40;
    while (1)
    {
        digit = (int)((modf(intp/10, &intp)+0.01)*10);

        result[pos] = wxT('0') + digit;
        pos--;

        if (intp < 1.0)
        {
            break;
        }

        if (++i == 3)
        {
            result[pos] = thousands_sep;
            pos--;
            i = 0;
        }
    }

    if (negative)
    {
        result[pos] = wxT('-');
        --pos;
    }


    string_start = result+pos+1;


    if (dec_places > 0)
    {
        pos = 41;
        result[pos] = decimal_point;
        pos++;
        wxSnprintf(result+pos, 40, wxT("%0*.0f"), dec_places, decp*kl::pow10(dec_places));
    }

    return string_start;
}


wxString doubleQuote(const wxString& src, wxChar quote)
{
    wxString ret;
    ret.Alloc(src.Length() + 10);
    const wxChar* ch;

    ch = src.c_str();
    while (*ch)
    {
        if (*ch != quote)
            ret += *ch;
              else
            {
                ret += quote;
                ret += quote;
            }

        ++ch;
    }

    return ret;
}

wxString makeProper(const wxString& input)
{
    if (input.Length() == 0)
        return input;
    
    wxString output = input;
    const wxChar* ch = output.c_str();
    int idx = 0;

    while (*ch && !wxIsalpha(*ch))
    {
        ch++;
        idx++;
    }

    if (!*ch)
    {
        return output;
    }
    
    // make sure that all characters are 7-bit
    // before trying to uppercase the first letter,
    // which can destroy some character data in
    // certain unicode cases
    
    ch = output.c_str();
    while (*ch)
    {
        ch++;
        if (*ch > 127)
            return output;
    }

    output.MakeLower();
    output.SetChar(idx, wxToupper(output.GetChar(idx)));

    return output;
}

wxString makeProperIfNecessary(const wxString& input)
{
    if (input.Length() == 0)
        return input;
        
    const wxChar* ch = input.c_str();
    
    while (*ch)
    {
        if (*ch > 127)
        {
            // unicode strings should not be made 'proper'
            return input;
        }
            
        if (wxIsalpha(*ch) && *ch != towupper(*ch))
        {
            // input is mixed-case, return original
            return input;
        }
        
        ++ch;
    }
    
    // if the string is all upper case, lower-case it
    // and capitalize the first letter
    return makeProper(input);
}




#ifdef __WXGTK__

int appMessageBox(const wxString& message,
                  const wxString& caption,
                  int style,
                  wxWindow *parent,
                  int x, int y)
{
    long decorated_style = style;

    if ( ( style & ( wxICON_EXCLAMATION |
                     wxICON_HAND |
                     wxICON_INFORMATION |
                     wxICON_QUESTION ) ) == 0 )
    {
        decorated_style |= ( style & wxYES ) ? wxICON_QUESTION : wxICON_INFORMATION ;
    }


    if (0 != gtk_check_version(2,4,0))
    {
        // we have an old version of GTK, we must
        // use a different parent window

        int i;
        int cnt = wxTopLevelWindows.GetCount();

        wxWindow* main_wnd = wxTheApp->GetTopWindow();

        if (parent == NULL)
            parent = main_wnd;

        for (i = 0; i < cnt; ++i)
        {
            wxWindow* wnd = (wxWindow*)wxTopLevelWindows.Item(i)->GetData();
            if (wnd != main_wnd)
                parent = wnd;
        }
    }


    wxMessageDialog dialog(parent, message, caption, decorated_style);


    if (0 == gtk_check_version(2,4,0))
    {
        gtk_window_set_keep_above(GTK_WINDOW(dialog.m_widget), true);
    }

    
    int ans = dialog.ShowModal();
    switch ( ans )
    {
        case wxID_OK:
            return wxOK;
        case wxID_YES:
            return wxYES;
        case wxID_NO:
            return wxNO;
        case wxID_CANCEL:
            return wxCANCEL;
    }

    return wxCANCEL;
}

#else

int appMessageBox(const wxString& message,
                  const wxString& caption,
                  int style,
                  wxWindow *parent,
                  int x, int y)
{
    if (parent == NULL)
    {
        parent = wxTheApp->GetTopWindow();
    }

    wxWindowDisabler wd((wxWindow*)NULL);

    return wxMessageBox(message, caption, style, parent, x, y);
}

#endif


class MsgBoxData : public wxEvtHandler
{
public:

    bool ProcessEvent(wxEvent& event)
    {
        appMessageBox(message, caption, style, parent, x, y);
        //delete this;
        return true;
    }
    
public:

    wxString message;
    wxString caption;
    int style;
    wxWindow* parent;
    int x;
    int y;
};

void deferredAppMessageBox(const wxString& message,
                           const wxString& caption,
                           int style,
                           wxWindow *parent,
                           int x, int y)
{
    MsgBoxData* data = new MsgBoxData;
    data->message = message;
    data->caption = caption;
    data->style = style;
    data->parent = parent;
    data->x = x;
    data->y = y;
    
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 9000);
    ::wxPostEvent(data, evt);
}



void limitFontSize(wxWindow* wnd, int size)
{
    wxFont font = wnd->GetFont();
    if (font.GetPointSize() > size)
    {
        font.SetPointSize(size);
        wnd->SetFont(font);
    }
}

void makeTextCtrlLowerCase(wxTextCtrl* text)
{
#ifdef WIN32
    HWND h = (HWND)text->GetHWND();
    LONG l = GetWindowLongA(h, GWL_STYLE);
    l |= ES_LOWERCASE;
    SetWindowLongA(h, GWL_STYLE, l);
#endif
}

void makeTextCtrlUpperCase(wxTextCtrl* text)
{
#ifdef WIN32
    HWND h = (HWND)text->GetHWND();
    LONG l = GetWindowLongA(h, GWL_STYLE);
    l |= ES_UPPERCASE;
    SetWindowLongA(h, GWL_STYLE, l);
#endif
}

void makeFontBold(wxWindow* window)
{
    wxFont font = window->GetFont();
    font.SetWeight(wxBOLD);
    window->SetFont(font);
}

void setTextWrap(wxStaticText* text, bool wrap)
{
#ifdef WIN32
#endif

#ifdef __WXGTK__
    text->GetBestSize();
    gtk_label_set_line_wrap(GTK_LABEL(text->m_widget), wrap);
#endif
}


#ifdef __WXGTK__
void null_log_function(const gchar* log_domain,
                       GLogLevelFlags log_level,
                       const gchar* message,
                       gpointer user_data)
{
}
#endif

void suppressConsoleLogging()
{
    // no logging on wx
    wxLog::EnableLogging(false);

#ifdef __WXGTK__
    // no logging on gtk -- but only disable it
    // if gtk is being used at all
    if (gdk_screen_get_default())
    {
        g_log_set_handler("Gdk",
                      (GLogLevelFlags)(
                      G_LOG_LEVEL_WARNING |
                      G_LOG_LEVEL_ERROR | 
                      G_LOG_LEVEL_CRITICAL |
                      G_LOG_FLAG_FATAL |
                      G_LOG_FLAG_RECURSION),
                      null_log_function,
                      NULL);
    }
#endif
}

void resizeStaticText(wxStaticText* text, int width)
{
    text->SetWindowStyleFlag(text->GetWindowStyleFlag() | wxST_NO_AUTORESIZE);
    setTextWrap(text, true);
    
    wxWindow* parent = text->GetParent();

    int intended_width;

    if (width == -1)
    {
        wxSize s = parent->GetClientSize();
        intended_width = s.GetWidth() - 10;
    }
     else
    {
        intended_width = width;
    }

    // by default
    wxString label = text->GetLabel();
    wxCoord text_width, text_height;
    
    wxClientDC dc(text->GetParent());
    dc.SetFont(text->GetFont());
    dc.GetTextExtent(label, &text_width, &text_height);

    // add 5% to the text width
    text_width = (text_width*100)/95;
    
    if (intended_width < 50)
        intended_width = 50;
        
    int row_count = (text_width/intended_width)+1;
    
    const wxChar* c = label.c_str();
    while (*c)
    {
        if (*c == L'\n')
            row_count++;
        ++c;
    }

    text->SetSize(intended_width, row_count*text_height);
    text->SetSizeHints(intended_width, row_count*text_height);
}

wxSize getMaxTextSize(wxStaticText* st0,
                      wxStaticText* st1,
                      wxStaticText* st2,
                      wxStaticText* st3,
                      wxStaticText* st4,
                      wxStaticText* st5,
                      wxStaticText* st6)
{
    wxStaticText* ctrls[7];

    ctrls[0] = st0;
    ctrls[1] = st1;
    ctrls[2] = st2;
    ctrls[3] = st3;
    ctrls[4] = st4;
    ctrls[5] = st5;
    ctrls[6] = st6;

    int sx, sy;
    int x, y;
    x = 0;
    y = 0;

    for (int i = 0; i < 7; i++)
    {
        if (!ctrls[i])
        {
            continue;
        }

        ctrls[i]->GetBestSize(&sx, &sy);
        if (sx > x)
            x = sx;
        if (sy > y)
            y = sy;
    }

    return wxSize(x, y);
}

// simple password encryption/decryption --
// please note that these functions are NOT unicode-friendly

const char* password_randomizer = "1lBgi3kjsAbv04386kDm,bvjG9604937L5ifk8lfmdjsuxy30dalskdjfurn2lfkgmbjgk4jfndkcjf2";
const char* hex_digits = "0123456789ABCDEF";


static wxString char2hex(char c)
{
    wxString retval;
    retval.Printf(wxT("%02X"), c);
    return retval;
}

static char hex2char(const wxString& s)
{
    int b1, b2;
    b1 = (strchr(hex_digits, toupper(s[0]))-hex_digits)*16;
    b2 = strchr(hex_digits, towupper(s[1]))-hex_digits;
    return (char)(b1+b2);
}

wxString simpleEncryptString(const wxString& s)
{
    wxString retval = wxT("X");

    int i;
    int len = s.Length();

    for (i = 0; i < len; ++i)
    {
        retval += char2hex(((unsigned char)s[i]) ^ password_randomizer[i]);
    }

    return retval;
}

wxString simpleDecryptString(const wxString& _s)
{
    wxString retval;
    
    wxString s;

    if (_s.Length() >= 1 && _s.GetChar(0) == wxT('X'))
    {
        s = _s.Mid(1);
    }
     else
    {
        s = _s;
    }

    int i;
    int len = s.Length() / 2;   // two chars for every hex value
    wxString hex_val;

    for (i = 0; i < len; ++i)
    {
        hex_val = s.Mid(i*2, 2);
        retval += hex2char(hex_val) ^ password_randomizer[i];
    }

    return retval;
}

wxString getProxyServer()
{
    //wxT("10.1.1.1:3128");
#ifdef __WXMSW__

    wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"));

    if(!pRegKey->Exists())
        return wxEmptyString;

    long enable = 0;

    if (!pRegKey->QueryValue(wxT("ProxyEnable"), &enable))
        return wxEmptyString;

    if (!enable)
        return wxEmptyString;

    wxString proxy;
    if (!pRegKey->QueryValue(wxT("ProxyServer"), proxy))
        return wxEmptyString;

    return proxy;

#else

    wxString val;
    if (!::wxGetEnv(wxT("HTTP_PROXY"), &val))
        return wxEmptyString;
    return val.AfterLast(wxT('/'));

#endif

}

