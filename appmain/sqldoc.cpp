/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2008-02-28
 *
 */


#include "appmain.h"
#include "sqldoc.h"
#include "querydoc.h"
#include "tabledoc.h"
#include "appcontroller.h"
#include "toolbars.h"
#include <wx/stc/stc.h>


enum
{
    ID_Text = wxID_HIGHEST + 1
};


// NOTE: these colors and this control were ripped from the editordoc.cpp...
//       this stuff show really be factored out
#define NORMAL_CHAR_COLOR     wxColour(0x00, 0x00, 0x00)
#define UNICODE_CHAR_COLOR    wxColour(0x00, 0x00, 0x00)
#define KEYWORD_COLOR         wxColour(0x00, 0x00, 0xff)
#define OPERATOR_COLOR        wxColour(0x00, 0x00, 0x00)
#define SINGLE_QUOTED_COLOR   wxColour(0xa0, 0x00, 0x00)
#define DOUBLE_QUOTED_COLOR   wxColour(0xa0, 0x00, 0x00)
#define NUMBER_COLOR          wxColour(0x00, 0x00, 0x00)
#define COMMENT_COLOR         wxColour(0x00, 0x80, 0x00)
#define PREPROCESSOR_COLOR    wxColour(0x80, 0x80, 0x80)


// ripped from xdodbc/database.cpp
static const char* sql92_keywords =
         "ABSOLUTE ACTION ADA ADD ALL ALLOCATE ALTER AND "
         "ANY ARE AS ASC ASSERTION AT AUTHORIZATION AVG "
         "BEGIN BETWEEN BIT BIT_LENGTH BOTH BY CASCADE "
         "CASCADED CASE CAST CATALOG CHAR CHAR_LENGTH "
         "CHARACTER CHARACTER_LENGTH CHECK CLOSE COALESCE "
         "COLLATE COLLATION COLUMN COMMIT CONNECT "
         "CONNECTION CONSTRAINT CONSTRAINTS CONTINUE "
         "CONVERT CORRESPONDING COUNT CREATE CROSS CURRENT "
         "CURRENT_DATE CURRENT_TIME CURRENT_TIMESTAMP "
         "CURRENT_USER CURSOR DATE DAY DEALLOCATE DEC "
         "DECIMAL DECLARE DEFAULT DEFERRABLE DEFERRED "
         "DELETE DESC DESCRIBE DESCRIPTOR DIAGNOSTICS "
         "DISCONNECT DISTINCT DOMAIN DOUBLE DROP ELSE END "
         "END-EXEC ESCAPE EXCEPT EXCEPTIONEXEC EXECUTE "
         "EXISTS EXTERNAL EXTRACT FALSE FETCH FIRST FLOAT "
         "FOR FOREIGN FORTRAN FOUND FROM FULL GET GLOBAL "
         "GO GOTO GRANT GROUP HAVING HOUR IDENTITY "
         "IMMEDIATE IN INCLUDE INDEX INDICATOR INITIALLY "
         "INNER INPUT INSENSITIVE INSERT INT INTEGER "
         "INTERSECT INTERVAL INTO IS ISOLATION JOIN KEY "
         "LANGUAGE LAST LEADING LEFT LEVEL LIKE LOCAL "
         "LOWER MATCH MAX MIN MINUTE MODULE MONTH NAMES "
         "NATIONAL NATURAL NCHAR NEXT NO NONE NOT NULL "
         "NULLIF NUMERIC OCTET_LENGTH OF ON ONLY OPEN "
         "OPTION OR ORDER OUTER OUTPUT OVERLAPS PAD "
         "PARTIAL PASCAL POSITION PRECISION PREPARE "
         "PRESERVE PRIMARY PRIOR PRIVILEGES PROCEDURE "
         "PUBLIC READ REAL REFERENCES RELATIVE RESTRICT "
         "REVOKE RIGHT ROLLBACK ROWS SCHEMA SCROLL "
         "SECOND SECTION SELECT SESSION SESSION_USER SET "
         "SIZE SMALLINT SOME SPACE SQL SQLCA SQLCODE "
         "SQLERROR SQLSTATE SQLWARNING SUBSTRING SUM "
         "SYSTEM_USER TABLE TEMPORARY THEN TIME TIMESTAMP "
         "TIMEZONE_HOUR TIMEZONE_MINUTE TO TRAILING "
         "TRANSACTION TRANSLATE TRANSLATION TRIM TRUE "
         "UNION UNIQUE UNKNOWN UPDATE UPPER USAGE USER "
         "USING VALUE VALUES VARCHAR VARYING VIEW WHEN "
         "WHENEVER WHERE WITH WORK WRITE YEAR ZONE";


class SqlTextCtrl : public wxStyledTextCtrl
{
public:
    
    SqlTextCtrl(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
            : wxStyledTextCtrl(parent, 18543, pos, size, style)
    {
        wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
        StyleSetFont(wxSTC_STYLE_DEFAULT, font);
        StyleClearAll();

        // -- set selection foreground and background color --
        SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
        SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));

        StyleSetForeground(wxSTC_SQL_DEFAULT, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_SQL_COMMENT, COMMENT_COLOR);
        StyleSetForeground(wxSTC_SQL_COMMENTLINE, COMMENT_COLOR);
        StyleSetForeground(wxSTC_SQL_COMMENTDOC, COMMENT_COLOR);
        StyleSetForeground(wxSTC_SQL_NUMBER, NUMBER_COLOR);
        StyleSetForeground(wxSTC_SQL_WORD, KEYWORD_COLOR);
        StyleSetForeground(wxSTC_SQL_STRING, DOUBLE_QUOTED_COLOR);
        StyleSetForeground(wxSTC_SQL_CHARACTER, SINGLE_QUOTED_COLOR);
        StyleSetForeground(wxSTC_SQL_OPERATOR, OPERATOR_COLOR);
        StyleSetForeground(wxSTC_SQL_IDENTIFIER, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_SQL_COMMENTLINEDOC, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_SQL_WORD2, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_SQL_COMMENTDOCKEYWORD, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_SQL_COMMENTDOCKEYWORDERROR, NORMAL_CHAR_COLOR);

        // wxStyledTextCtrl requires lowercase keywords for the sql lexer
        wxString keywords = wxString::From8BitData(sql92_keywords);
        keywords.MakeLower();
        
        SetLexer(wxSTC_LEX_SQL);
        SetKeyWords(0, keywords.c_str());
        SetWrapMode(wxSTC_WRAP_WORD);
        SetUseTabs(false);
        SetIndent(4);

        // set the left and right margins
        SetMargins(2,2);
        
        // make sure margin 1 has no width
        SetMarginWidth(1,0);
        
        // show line numbers in the first left margin
        SetMarginType(0, wxSTC_MARGIN_NUMBER);
        
        // hide the caret (since we can't edit the SQL right now)
        SetCaretForeground(*wxWHITE);
        
        // horizontal scrollbars weren't being shown
        // on Linux... perhaps this helps???
        SetUseVerticalScrollBar(true);
        SetUseHorizontalScrollBar(true);
    }
};




BEGIN_EVENT_TABLE(SqlDoc, wxWindow)

    EVT_SIZE(SqlDoc::onSize)
    
    EVT_MENU(ID_Edit_Copy, SqlDoc::onCopy)
    EVT_MENU(ID_Edit_SelectAll, SqlDoc::onSelectAll)
    EVT_MENU(ID_Edit_SelectNone, SqlDoc::onSelectNone)
    
    // disable data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, SqlDoc::onUpdateUI_DisableAlways)

    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, SqlDoc::onUpdateUI_DisableAlways)

    // disable format items
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, SqlDoc::onUpdateUI_DisableAlways)

    // disable table items
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, SqlDoc::onUpdateUI_DisableAlways)

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_Save, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_SaveAs, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_SaveAsExternal, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_PageSetup, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Print, SqlDoc::onUpdateUI_DisableAlways)

    // disable the zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomIn, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomOut, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToActual, SqlDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, SqlDoc::onUpdateUI_DisableAlways)

    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, SqlDoc::onUpdateUI)

END_EVENT_TABLE()


SqlDoc::SqlDoc()
{
    m_textctrl = NULL;
}

SqlDoc::~SqlDoc()
{

}


// -- IDocument --

bool SqlDoc::initDoc(cfw::IFramePtr frame,
                     cfw::IDocumentSitePtr doc_site,
                     wxWindow* docsite_wnd,
                     wxWindow* panesite_wnd)
{
    bool result = Create(docsite_wnd,
                         -1,
                         wxPoint(0,0),
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN |
                         wxNO_BORDER);
    if (!result)
        return false;

    m_frame = frame;
    m_doc_site = doc_site;

    SetBackgroundColour(*wxWHITE);
    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);

    wxString path = getDocumentLocation();
    if (path.Length() > 0)
    {
        wxString caption = path.AfterLast(wxT('/'));
        m_doc_site->setCaption(caption);
    }
     else
    {
        m_doc_site->setCaption(_("(Untitled)"));
    }
    
    m_doc_site->setBitmap(GETBMP(gf_query_16));
    
    
    // -- create text control --
    m_textctrl = new SqlTextCtrl(this,
                                 ID_Text,
                                 wxDefaultPosition,
                                 wxDefaultSize,
                                 wxTE_MULTILINE | wxNO_BORDER |
                                 wxHSCROLL | wxVSCROLL);
    m_textctrl->SetFont(wxFont(wxNORMAL_FONT->GetPointSize()+2, wxMODERN, wxNORMAL, wxNORMAL));
    m_textctrl->SetText(m_sql_text);
    m_textctrl->SetReadOnly(true);

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_textctrl, 1, wxEXPAND);
    SetSizer(main_sizer);
    Layout();

    // add frame event handlers
    frame->sigFrameEvent().connect(this, &SqlDoc::onFrameEvent);

    return true;
}

wxString SqlDoc::getDocumentLocation()
{
    cfw::IDocumentPtr doc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
    if (doc.isOk())
        return doc->getDocumentLocation();

    return wxEmptyString;
}

wxString SqlDoc::getDocumentTitle()
{
    return wxEmptyString;
}

wxWindow* SqlDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void SqlDoc::setDocumentFocus()
{
    m_textctrl->SetFocus();
}

bool SqlDoc::onSiteClosing(bool force)
{
    if (force)
        return true;
    
    // user the querydoc's site closing code,
    // so we don't have to duplicate code
    IQueryDocPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
    if (querydoc.isOk())
    {
        cfw::IDocumentPtr doc = querydoc;
        return doc->onSiteClosing(force);
    }

    return true;
}

void SqlDoc::onSiteActivated()
{

}

void SqlDoc::onSelectAll(wxCommandEvent& evt)
{
    if (!m_textctrl)
        return;
    
    m_textctrl->SetSelection(-1,-1);
}

void SqlDoc::onCopy(wxCommandEvent& evt)
{
    if (!m_textctrl)
        return;
    
    m_textctrl->Copy();
}

void SqlDoc::onSelectNone(wxCommandEvent& evt)
{
    if (!m_textctrl)
        return;
    
    m_textctrl->SetSelection(0,0);
}

void SqlDoc::onSize(wxSizeEvent& evt)
{
    Layout();
}

void SqlDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void SqlDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

void SqlDoc::onUpdateUI(wxUpdateUIEvent& evt)
{
    int id = evt.GetId();
    
    // disable undo/redo
    if (id == ID_Edit_Undo ||
        id == ID_Edit_Redo)
    {
        evt.Enable(false);
        return;
    }
    
    // disable cut/copylink/paste (for now)
    if (id == ID_Edit_Cut ||
        id == ID_Edit_CopyLink ||
        id == ID_Edit_Paste ||
        id == ID_Edit_Delete)
    {
        evt.Enable(false);
        return;
    }
    
    // enable copy if it's available
    if (id == ID_Edit_Copy)
    {
        evt.Enable(true);
        return;
    }
    
    // enable select all and select none
    if (id == ID_Edit_SelectAll ||
        id == ID_Edit_SelectNone)
    {
        evt.Enable(true);
        return;
    }
    
    // disable find/replace (for now)
    if (id == ID_Edit_Find ||
        id == ID_Edit_Replace)
    {
        evt.Enable(false);
        return;
    }
    
    if (id == ID_Edit_FindPrev || id == ID_Edit_FindNext)
    {
        evt.Enable(false);
        return;
    }
    
    // disable goto
    if (id == ID_Edit_GoTo)
    {
        evt.Enable(false);
        return;
    }
    
    // enable other items by default
    evt.Enable(true);
    return;
}

void SqlDoc::setText(const wxString& text)
{
    m_sql_text = text;
    
    // basic SQL formatting
    m_sql_text.Replace(wxT("SELECT "), wxT("\nSELECT\n    "));
    m_sql_text.Replace(wxT("FROM "), wxT("\nFROM\n    "));
    m_sql_text.Replace(wxT("INTO "), wxT("\nINTO \n    "));
    m_sql_text.Replace(wxT("WHERE "), wxT("\nWHERE\n    "));
    m_sql_text.Replace(wxT("INNER JOIN "), wxT("\nINNER JOIN\n    "));
    m_sql_text.Replace(wxT("LEFT OUTER JOIN "), wxT("\nLEFT OUTER JOIN\n    "));
    m_sql_text.Replace(wxT("RIGHT OUTER JOIN "), wxT("\nRIGHT OUTER JOIN\n    "));
    m_sql_text.Replace(wxT("FULL OUTER JOIN "), wxT("\nFULL OUTER JOIN\n    "));
    m_sql_text.Replace(wxT("GROUP BY "), wxT("\nGROUP BY\n    "));
    m_sql_text.Replace(wxT("HAVING "), wxT("\nHAVING\n    "));
    m_sql_text.Replace(wxT("ORDER BY "), wxT("\nORDER BY\n    "));
    m_sql_text.Replace(wxT(", "), wxT(",\n    "));
    m_sql_text += wxT("\n");
    
    // set the SQL text
    if (m_textctrl)
    {
        m_textctrl->SetReadOnly(false);
        m_textctrl->SetText(m_sql_text);
        m_textctrl->SetReadOnly(true);
    }
}

void SqlDoc::onFrameEvent(cfw::Event& evt)
{
    // if a file is renamed, update this file with the new file path
    if (evt.name == wxT("treepanel.ofsFileRenamed"))
    {
        if (evt.s_param == getDocumentLocation())
        {
            wxString path = evt.s_param2;

            // update caption;
            wxString caption = path.AfterLast(wxT('/'));
            m_doc_site->setCaption(caption);
            
            cfw::IDocumentSitePtr doc_site = m_frame->getActiveChild();
            if (doc_site.isOk() && doc_site == m_doc_site)
            {
                // fire this event so that the URL combobox will be updated
                // with the new path if this is the active child
                m_frame->postEvent(new cfw::Event(wxT("cfw.locationChanged")));
            }
        }
    }

    if (evt.name == wxT("appmain.view_switcher.query_available_views"))
    {
        cfw::IDocumentSitePtr active_child;
        active_child = g_app->getMainFrame()->getActiveChild();
        
        if (active_child.isNull() || m_doc_site.isNull())
            return;
            
        if (active_child != m_doc_site)
            return;
        
        if (active_child->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
        
        // site ptrs to check the active site
        cfw::IDocumentSitePtr active_site;
        cfw::IDocumentSitePtr querydoc_site;
        cfw::IDocumentSitePtr tabledoc_site;
        querydoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.QueryDoc");
        tabledoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.TableDoc");
        active_site = g_app->getMainFrame()->getActiveChild();
        
        // populate the view switcher list with the available views
        ViewSwitcherList* list = (ViewSwitcherList*)(evt.o_param);
        
        // normally, we populate the view switcher from the TableDoc's
        // onFrameEvent() handler, but no tabledoc exists in this container
        // (most likely because we are createing a new query), so make sure
        // we populate the list ourselves
        ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
        if (tabledoc.isNull())
        {
            list->addItem(ID_View_SwitchToSourceView, _("SQL View"),
                          (m_doc_site == active_site) ? true : false);
            
            list->addItem(ID_View_SwitchToDesignView, _("Design View"),
                          (querydoc_site == active_site) ? true : false);
                          
            list->addItem(ID_View_SwitchToLayoutView, _("Table View"),
                          (tabledoc_site == active_site) ? true : false);
        }
    }
     else if (evt.name == wxT("appmain.view_switcher.active_view_changing"))
    {
        // the QueryDoc handles this case for us...
    }
     else if (evt.name == wxT("appmain.view_switcher.active_view_changed"))
    {
        int id = (int)(evt.l_param);
        
        // -- make sure we are in the active container --
        cfw::IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        if (active_site.isNull() || m_doc_site.isNull())
            return;
        if (active_site->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
        if (active_site != m_doc_site)
            return;
        
        if (id == ID_View_SwitchToSourceView)
            return;
        
        if (id == ID_View_SwitchToDesignView)
        {
            switchToOtherDocument(m_doc_site, "appmain.QueryDoc");
            return;
        }
        
        if (id == ID_View_SwitchToLayoutView)
        {
            // always let querydoc handle this switch
            return;
        }
    }
}



