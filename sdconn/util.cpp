/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2013-12-11
 *
 */


#include "app.h"
#include "util.h"
#include <wx/tokenzr.h>


// helper function
static bool isValid(const wxString& str,
                    const wxString& invalid_starting_chars,
                    const wxString& invalid_chars,
                    int* err_idx)
{
    if (str.IsEmpty())
    {
        if (err_idx)
        {
            *err_idx = 0;
        }
        return false;
    }

    // check to see if the first character is valid
    const wxChar* p = str.c_str();

    if (invalid_starting_chars.Find(*p) != -1)
    {
        if (err_idx)
        {
            *err_idx = 0;
        }
        return false;
    }

    ++p;
    int idx = 1;
    while (*p)
    {
        if (invalid_chars.Find(*p) != -1)
        {
            if (err_idx)
            {
                *err_idx = idx;
            }
            return false;
        }

        ++p;
        ++idx;
    }

    return true;
}

bool isKeyword(const wxString& str,
               xd::IDatabasePtr db)
{
    if (db.isNull())
        db = g_app->getDatabase();

    xd::IAttributesPtr attr = db->getAttributes();
    if (!attr)
    {
        // couldn't get attributes from database,
        // assume the worst
        return true;
    }

    std::vector<wxString> vec;

    wxStringTokenizer t(attr->getStringAttribute(xd::dbattrKeywords), ",");
    while (t.HasMoreTokens())
    {
        wxString s = t.GetNextToken();
        s.Trim();
        s.Trim(FALSE);
        vec.push_back(s);
    }

    std::vector<wxString>::iterator it;

    for (it = vec.begin(); it != vec.end(); ++it)
    {
        if (str.CmpNoCase(*it) == 0)
            return true;
    }

    return false;
}

bool isValidObjectName(const wxString& str,
                       xd::IDatabasePtr db,
                       int* err_idx)
{
    // if the string is empty, it's invalid
    if (str.Length() == 0)
        return false;

    if (db.isNull())
        db = g_app->getDatabase();

    xd::IAttributesPtr attr = db->getAttributes();
    if (!attr)
        return false;

    if (isKeyword(str, db))
        return false;

    wxString invalid_starting = attr->getStringAttribute(xd::dbattrTableInvalidStartingChars);
    wxString invalid = attr->getStringAttribute(xd::dbattrTableInvalidChars);

    // test the content
    if (!isValid(str, invalid_starting, invalid, err_idx))
        return false;


    // test the length
    if (str.Length() > (size_t)attr->getIntAttribute(xd::dbattrTableMaxNameLength))
    {
        if (err_idx)
        {
            *err_idx = str.Length()-1;
        }
        return false;
    }

    return true;
}

wxString makeValidObjectName(const wxString& str,
                             xd::IDatabasePtr db)
{
    if (db.isNull())
        db = g_app->getDatabase();

    wxString work_str = str;

    unsigned int i = 0;

    while (1)
    {
        int err_idx = 0;
        if (isValidObjectName(work_str, db, &err_idx))
        {
            return work_str;
        }
    
        if (err_idx == 0)
        {
            if (work_str.Length() > 0 && ::iswalpha(work_str[0]))
            {
                // probably a keyword
                work_str = work_str + wxT("_");
            }
             else
            {
                // probably a number at the beginning
                work_str = wxT("_") + work_str;
            }
        }
         else
        {
            work_str.SetChar(err_idx, wxT('_'));
        }

        // avoid infinite loop
        if (++i > str.Length()*2)
            return work_str;
    }

    return work_str;
}


void resizeStaticText(wxStaticText* text, int width)
{
    text->SetWindowStyleFlag(text->GetWindowStyleFlag() | wxST_NO_AUTORESIZE);
    //setTextWrap(text, true);
    
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
