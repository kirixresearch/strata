/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-04-26
 *
 */


#include "panelconsole.h"
#include "appcontroller.h"
#include "treecontroller.h"
#include "griddoc.h"


#define KPP_USE_STC


#ifdef KPP_USE_STC

#include <wx/stc/stc.h>


wxChar* keywords =
wxT("all into and is any join as left asc like avg \
     local between match by max cast min corresponding \
     natural count not create null cross on delete or \
     desc order distinct outer drop right escape \
     select except set exists some false sum from \
     table full temporary global true group union \
     having unique in unknown inner update insert \
     using intersect values alter column");


class CommandTextCtrl : public wxStyledTextCtrl
{
private:
    std::vector<wxString> m_commands;
    wxString m_text;
    bool m_populating;

public:

    CommandTextCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
                  : wxStyledTextCtrl(parent, 21520, pos, size, style)
    {
        wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
        StyleSetFont(wxSTC_STYLE_DEFAULT, font);
        StyleClearAll();

        // -- set selection foreground and background color --
        SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
        SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));

        StyleSetForeground(0,  wxColour(128, 128, 128));
        StyleSetForeground(1,  wxColour(000, 128, 000));
        StyleSetForeground(2,  wxColour(000, 128, 000));
        StyleSetForeground(3,  wxColour(128, 128, 128));
        StyleSetForeground(4,  wxColour(000, 128, 128));
        StyleSetForeground(5,  wxColour(000, 000, 255));
        StyleSetForeground(6,  wxColour(128, 000, 128));
        StyleSetForeground(7,  wxColour(128, 000, 128));
        StyleSetForeground(8,  wxColour(000, 128, 128));
        StyleSetForeground(9,  wxColour(128, 128, 128));
        StyleSetForeground(10, wxColour(000, 000, 000));
        StyleSetForeground(11, wxColour(000, 000, 000));

        MarkerDefine(0, wxSTC_MARK_ARROW);
        //MarkerDefineBitmap(35, GET_XPM(xpm_error));

        //#ifdef __WXMSW__
        //StyleSetSpec(2, _T("fore:#007f00,bold,face:Arial,size:9"));
        //#else
        //StyleSetSpec(2, _T("fore:#007f00,bold,face:Helvetica,size:9"));
        //#endif

        EmptyUndoBuffer();

        SetLexer(wxSTC_LEX_CPPNOCASE);
        SetBufferedDraw(true);
        SetUseVerticalScrollBar(true);
        SetUseHorizontalScrollBar(false);
        SetKeyWords(0, keywords);
        SetUseTabs(false);
        SetMarginWidth(1,0);
        SetIndent(4);

        m_text = wxT("");
        m_populating = false;
    }

    ~CommandTextCtrl()
    {

    }

    void SetValue(const wxString& value)
    {
        SetText(value);
    }

    wxString GetValue()
    {
        return GetText();
    }

    bool IsModified()
    {
        return GetModify();
    }

    void DiscardEdits()
    {
        SetSavePoint();
    }

    bool CanCut()
    {
        return true;
    }

    bool CanCopy()
    {
        return true;
    }

    void SetCursorPosition(int pos)
    {
        GotoPos(pos);
    }

    void RemoveMarkers()
    {
        MarkerDeleteAll(0);
    }

    void SetMarker(int line)
    {
        MarkerAdd(line, 0);
    }

    void populateText()
    {
        m_populating = true;

        Freeze();
        ClearAll();

        int i = 0;
        m_text = wxT("");
        std::vector<wxString>::iterator it;
        for (it = m_commands.begin(); it != m_commands.end(); ++it)
        {
            m_text += *it;
            m_text += '\r';
            m_text += '\n';

            i++;
        }

        SetValue(m_text);
        GotoLine(i);
        Thaw();

        m_populating = false;
    }

    void onCharAdded(wxStyledTextEvent& event)
    {
        if (m_populating)
        {
            return;
        }

        if (event.GetKey() == wxT('\r'))
        {
            int cur_line = GetCurrentLine();
            int line_count = GetLineCount();

            // -- this accounts for if the user hits enter in the middle of a line --
            wxString last_line = GetLine(cur_line-1);
            wxString this_line = GetLine(cur_line);

            wxString command = last_line+this_line;
            command.Trim();

            // -- remove the carriage return and line feed --
            int idx = command.Find('\n');
            if (idx != -1)
            {
                command.Remove(idx-1, 2);
            }
            
            m_commands.push_back(command);
            populateText();

            command.Trim(false);
            wxString command_word = command;
            command_word = command_word.BeforeFirst(wxT(' '));

            if (!command_word.CmpNoCase(wxT("SELECT")))
            {
                wxBusyCursor bc;

                IConnectionPtr c = g_app->getConnectionMgr()->getActiveConnection();
                if (!c)
                {
                    return;
                }

                // -- if the connection is not open, attempt to open it --
                if (!c->isOpen())
                {
                    if (!c->open())
                    {
                        return;
                    }
                }

                tango::IDatabasePtr db = c->getDatabasePtr();
                if (!db)
                {
                    return;
                }

                xcm::IObjectPtr result;
                db->execute(towstr(command), 0, result, NULL);
                tango::IIteratorPtr iter = result;
                if (!iter)
                    return;

                cfw::IDocumentSitePtr site;
                GridDoc* gd = new GridDoc;
                site = g_app->getMainFrame()->createSite(gd, cfw::sitetypeNormal, -1, -1, -1, -1);

                gd->setName(_("Query"));

                if (!gd->setIterator(iter))
                {
                    gd->Destroy();
                    return;
                }
            }
             else
            {
                wxBusyCursor bc;

                IConnectionPtr c = g_app->getConnectionMgr()->getActiveConnection();
                if (!c)
                {
                    return;
                }

                tango::IDatabasePtr db = c->getDatabasePtr();
                if (!db)
                {
                    return;
                }

                if (!(db->execute(towstr(command), NULL)))
                {
                    cfw::appMessageBox(_("An error occurred while processing your SQL statement."),
                                       PRODUCT_NAME, wxOK);
                    SetFocus();
                    return;
                }

                g_app->getAppController()->getTreeController()->refreshAllItems();
            }

            SetFocus();
            return;
        }
    }


    DECLARE_EVENT_TABLE()
};




BEGIN_EVENT_TABLE(CommandTextCtrl, wxStyledTextCtrl)
    EVT_STC_CHARADDED(21520, CommandTextCtrl::onCharAdded)
END_EVENT_TABLE()




#else


class CommandTextCtrl : public wxTextCtrl
{

public:

    CommandTextCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
                  : wxTextCtrl(parent, id, wxEmptyString, pos, size, style)
    {
    }

    void SetCursorPosition(int pos)
    {
    }

    void RemoveMarkers()
    {
    }

    void SetMarker(int line)
    {
    }
};


#endif




enum
{
    ID_Text = wxID_HIGHEST + 1
};


BEGIN_EVENT_TABLE(ConsolePanel, wxWindow)
    EVT_SIZE(ConsolePanel::onSize)
END_EVENT_TABLE()


ConsolePanel::ConsolePanel()
{
    m_text = NULL;
}

ConsolePanel::~ConsolePanel()
{

}

bool ConsolePanel::initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd)
{
    // -- create document's window --
    bool result = Create(docsite_wnd, -1, wxDefaultPosition, docsite_wnd->GetClientSize(), wxNO_FULL_REPAINT_ON_RESIZE);
    
    if (!result)
    {
        return false;
    }

    doc_site->setCaption(_("Command Console"));
    wxSize min_site_size = doc_site->getSiteWindow()->GetSize();
    doc_site->setMinSize(100, 50);

    m_frame = frame;
    refresh();

    return true;
}

wxWindow* ConsolePanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ConsolePanel::setDocumentFocus()
{

}

void ConsolePanel::refresh()
{
    // -- create the text control --
    if (!m_text)
    {
        m_text = new CommandTextCtrl(this, ID_Text, wxPoint(0,0), GetClientSize(), wxTE_MULTILINE | wxNO_BORDER | wxHSCROLL | wxVSCROLL);
        m_text->SetFont(wxFont(10, wxMODERN, wxNORMAL, wxNORMAL));
        m_text->Show();
        m_text->Freeze();
    }
     else
    {
        m_text->Freeze();
    }

    m_text->Thaw();

    doSizing();
}

void ConsolePanel::doSizing()
{
    if (m_text)
    {
        wxSize size = GetClientSize();
        m_text->SetSize(0, 0, size.GetWidth(), size.GetHeight());
    }
}

void ConsolePanel::onSize(wxSizeEvent& event)
{
    doSizing();
}





