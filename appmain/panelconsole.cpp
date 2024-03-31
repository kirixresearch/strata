/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-04-26
 *
 */


#include "appmain.h"
#include "panelconsole.h"
#include "editordoc.h"
#include "appcontroller.h"
#include "scripthostapp.h"
#include "tabledoc.h"
#include <kl/regex.h>
#include <wx/stc/stc.h>


// NOTE: the default colors for the editor doc, sql doc, 
// expression builder, and console are the same;
// TODO: probably should factor these out
#define NORMAL_CHAR_COLOR     wxColour(0x00, 0x00, 0x00)
#define UNICODE_CHAR_COLOR    wxColour(0x00, 0x00, 0x00)
#define KEYWORD_COLOR         wxColour(0x00, 0x00, 0xff)
#define OPERATOR_COLOR        wxColour(0x00, 0x00, 0x00)
#define SINGLE_QUOTED_COLOR   wxColour(0xa0, 0x00, 0x00)
#define DOUBLE_QUOTED_COLOR   wxColour(0xa0, 0x00, 0x00)
#define NUMBER_COLOR          wxColour(0x00, 0x00, 0x00)
#define COMMENT_COLOR         wxColour(0x00, 0x80, 0x00)
#define PREPROCESSOR_COLOR    wxColour(0x80, 0x80, 0x80)


// command keywords
static const char* command_keywords =
         "USE PWD CLEAR OPEN CLOSE";

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


static std::wstring getTableNameFromSql(const std::wstring& str)
{
    // TODO: replace this with something more substantial
    
    std::wstring s = str;
    kl::makeUpper(s);
    int pos = s.find(L"FROM ");
    if (pos == -1)
        pos = s.find(L"FROM\t");
        
    if (pos != s.npos)
    {
        const wchar_t* p = str.c_str() + pos + 5;
        while (iswspace(*p))
            p++;
        
        std::wstring res;
        while (*p && *p != ';' && !iswspace(*p))
        {
            if (*p == '[')
            {
                while (*p && *p != ']')
                {
                    res += *p;
                    p++;
                }
                if (!*p)
                    break;
            }
            
            res += *p;
            ++p;
        }
        
        // dequote
        kl::trim(res);
        if (res.length() > 0 && res[0] == '[' && res[res.length()-1] == ']')
        {
            res.erase(0,1);
            if (res.length() > 0)
                res.erase(res.length()-1,1);
        }
        
        return res;
    }
    
    return L"";
}



class CommandHistoryItem
{
public:

    CommandHistoryItem()
    {
        m_start_command = -1;
        m_end_command = -1;
    }

public:

    wxString m_command;
    int m_start_command;
    int m_end_command;
};



enum
{
    ID_DoEchoResult = 19425
};

class CommandTextCtrl : public wxStyledTextCtrl
{
public:

    XCM_IMPLEMENT_SIGNAL1(sigCommandEntered, wxString&)

public:

    CommandTextCtrl(wxWindow* parent,
                    wxWindowID id,
                    const wxPoint& pos,
                    const wxSize& size,
                    long style)
                  : wxStyledTextCtrl(parent, id, pos, size, style | wxWANTS_CHARS)
    {
        wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
        StyleSetFont(wxSTC_STYLE_DEFAULT, font);
        StyleClearAll();

        // -- set selection foreground and background color --
        SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
        SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));

        StyleSetForeground(wxSTC_C_DEFAULT,  NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_COMMENT,  COMMENT_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTLINE,  COMMENT_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTDOC,  COMMENT_COLOR);
        StyleSetForeground(wxSTC_C_NUMBER,  NUMBER_COLOR);
        StyleSetForeground(wxSTC_C_WORD,  KEYWORD_COLOR);
        StyleSetForeground(wxSTC_C_STRING,  DOUBLE_QUOTED_COLOR);
        StyleSetForeground(wxSTC_C_CHARACTER,  SINGLE_QUOTED_COLOR);
        StyleSetForeground(wxSTC_C_UUID,  NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_PREPROCESSOR,  PREPROCESSOR_COLOR);
        StyleSetForeground(wxSTC_C_OPERATOR, OPERATOR_COLOR);
        StyleSetForeground(wxSTC_C_IDENTIFIER, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_STRINGEOL, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_VERBATIM, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_REGEX, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTLINEDOC, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_WORD2, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORD, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORDERROR, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_GLOBALCLASS, NORMAL_CHAR_COLOR);

        MarkerDefine(0, wxSTC_MARK_ARROW);

        EmptyUndoBuffer();

        SetMargins(2,2);
        SetMarginWidth(1,0);

        SetLexer(wxSTC_LEX_CPPNOCASE);
        SetBufferedDraw(true);
        SetUseVerticalScrollBar(true);
        SetUseHorizontalScrollBar(true);

        wxString keywords;
        keywords += wxString::From8BitData(sql92_keywords) + L" ";
        keywords += wxString::From8BitData(command_keywords);
        keywords = keywords.MakeLower();

        SetKeyWords(0, keywords.c_str());

        SetUseTabs(false);
        SetIndent(4);

        // command history index
        m_command_history_idx = -1;
        
        // command start position
        m_start_command_pos = 0;
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

    void SetText(const wxString& text)
    {
        wxStyledTextCtrl::SetText(text);
        EmptyUndoBuffer();
    }

    void AppendText(const wxString& text)
    {
        wxStyledTextCtrl::AppendText(text);
        EmptyUndoBuffer();
    }

    void EchoText(const wxString& text)
    {
        // same as AppendText(), except cursor is
        // positioned at end of echoed text
        wxStyledTextCtrl::AppendText(text);
        GoToEnd();
        EmptyUndoBuffer();    
    }

    wxString GetCurrentLineText()
    {
        int current_line = GetCurrentLine();
        int command_start = PositionFromLine(current_line);
        int command_end = GetLineEndPosition(current_line);
        return GetTextRange(command_start, command_end);
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
    
    void GoToEnd()
    {
        GotoLine(10000000);
    }

private:

    bool ProcessCommand()
    {
        // process the command
        wxString command;

        // for now, treat the current cursor line as the command
        // TODO: allow multi-line commands
        int current_line = GetCurrentLine();
        int command_start = PositionFromLine(current_line);
        int command_end = GetLineEndPosition(current_line);
        command = GetTextRange(command_start, command_end);

        // if we're not on the last line, get the current
        // line and reappend it to the end with a carriage
        // return; otherwise, simply add a carriage return
        int last_line = LineFromPosition(GetTextLength());
        if (current_line == last_line)
            EchoText(wxT("\n"));
             else
            EchoText(command + wxT("\n"));

        // if the command doesn't any text, simply advance
        // the line, and we're done; makes sure control is
        // responsive when simply pressing return
        if (command.Length() == 0)
            return false;

        // signal the the command has been entered
        sigCommandEntered().fire(command);

        // post a signal to finish the command, at which point
        // we'll echo any results; this allows time for jobs that 
        // are launched from commands to return errors so those 
        // errors can get added right after the command text
        ::wxMilliSleep(50);
        wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, ID_DoEchoResult);
        ::wxPostEvent(this, evt);
        return true;
    }

    void AddCommandToHistory(const wxString& command, 
                             int start_command, int end_command)
    {    
        // if the command is empty, don't add it
        if (command.Length() == 0)
            return;
            
        // if the command is the same as the last
        // command stored in the history, don't add it
        if (m_command_history.size() > 0)
        {
            if (command == m_command_history.back().m_command)
                return;
        }

        // add the command to the history
        CommandHistoryItem command_item;
        command_item.m_command = command;
        command_item.m_start_command = start_command;
        command_item.m_end_command = end_command;

        m_command_history.push_back(command_item);
    }

    bool GetPreviousCommand(wxString& command)
    {
        // note: when m_command_history == -1, we're not on
        // any of the command history items, but rather, on
        // the current command line
    
        // if there's no items in the history, we're done
        if (m_command_history.size() == 0)
            return false;
    
        // if the command history index is zero,
        // there's no more items to get
        if (m_command_history_idx == 0)
            return false;
            
        // if the command history is less than zero, we're
        // on the current command line; get the last item in 
        // the history
        if (m_command_history_idx < 0)
        {
            command = m_command_history.back().m_command;
            m_command_history_idx = m_command_history.size() - 1;
            return true;
        }    

        // decrement the index and return the item
        m_command_history_idx--;
        command = m_command_history[m_command_history_idx].m_command;
        return true;
    }
    
    bool GetNextCommand(wxString& command)
    {
        // if the command history index is -1, there's
        // no more commands to get
        if (m_command_history_idx < 0)
            return false;

        // if the command history index is on the last
        // history item, set the command index to -1
        if (m_command_history_idx == m_command_history.size() - 1)
        {
            m_command_history_idx = -1;
            command = wxT("");            
            return true;
        }
        
        m_command_history_idx++;
        command = m_command_history[m_command_history_idx].m_command;
        return true;
    }
    
    int GetStartCommandPosition()
    {
        return m_start_command_pos;
    }

    void ResetCommandStartPosition()
    {
        m_start_command_pos = GetTextLength();
    }

    void OnKeyDown(wxKeyEvent& evt)
    {
        // get the cursor position and keycode
        int current_pos = GetCurrentPos();
        int keycode = evt.GetKeyCode();

        switch (keycode)
        {
            case WXK_RETURN:
            case WXK_NUMPAD_ENTER:
                ProcessCommand();
                return;

            default:
            case WXK_BACK:
            case WXK_HOME:
            case WXK_NUMPAD_HOME:
            case WXK_END:
            case WXK_NUMPAD_END:
            case WXK_UP:
            case WXK_NUMPAD_UP:
            case WXK_DOWN:
            case WXK_NUMPAD_DOWN:
            case WXK_LEFT:
            case WXK_NUMPAD_LEFT:
            case WXK_RIGHT:
            case WXK_NUMPAD_RIGHT:
                break;
        }

        // allow the text control to process the event
        evt.Skip();
    }

    void OnEchoResult(wxCommandEvent& evt)
    {
        // TODO: echo the result
    }

private:

    // history of commands
    std::vector<CommandHistoryItem> m_command_history;
    int m_command_history_idx;

    // start of new command
    int m_start_command_pos;
        
private:

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CommandTextCtrl, wxStyledTextCtrl)
    EVT_KEY_DOWN(CommandTextCtrl::OnKeyDown)
    EVT_MENU(ID_DoEchoResult, CommandTextCtrl::OnEchoResult)
END_EVENT_TABLE()



enum
{
    ID_Text = wxID_HIGHEST + 1
};


BEGIN_EVENT_TABLE(ConsolePanel, wxWindow)
    EVT_SIZE(ConsolePanel::onSize)
    EVT_SET_FOCUS(ConsolePanel::onSetFocus)
    EVT_KILL_FOCUS(ConsolePanel::onKillFocus)
    EVT_STC_DOUBLECLICK(-1, ConsolePanel::onLeftDblClick)
END_EVENT_TABLE()


ConsolePanel::ConsolePanel()
{
    m_text_ctrl = NULL;
    m_lquote = L'[';
    m_rquote = L']';
}

ConsolePanel::~ConsolePanel()
{
}

bool ConsolePanel::initDoc(IFramePtr frame,
                           IDocumentSitePtr doc_site,
                           wxWindow* docsite_wnd,
                           wxWindow* panesite_wnd)
{
    // create document's window
    bool result = Create(docsite_wnd,
                         -1,
                         wxDefaultPosition,
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxNO_BORDER |
                         wxWANTS_CHARS);
    
    if (!result)
        return false;

    // connect the signal
    sigCommand().connect(this, &ConsolePanel::onCommand);

    // set the doc site info
    doc_site->setCaption(_("Console"));
    wxSize min_site_size = doc_site->getContainerWindow()->GetSize();
    doc_site->setMinSize(100, 50);
    m_frame = frame;

    // refresh the control
    refresh();

    // database to use for processing commands
    m_command_db_path = wxT("");
    
    // command
    m_command_text = wxT("");

    // quote characters to use for command
    m_lquote = L'[';
    m_rquote = L']';

    return true;
}

wxWindow* ConsolePanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ConsolePanel::setDocumentFocus()
{
    if (m_text_ctrl)
        m_text_ctrl->SetFocus();
}

void ConsolePanel::refresh()
{
    // create the text control
    if (!m_text_ctrl)
    {
        m_text_ctrl = new CommandTextCtrl(this,
                                          ID_Text,
                                          wxPoint(0,0),
                                          GetClientSize(),
                                          wxTE_MULTILINE |
                                          wxNO_BORDER |
                                          wxHSCROLL |
                                          wxVSCROLL);

        m_text_ctrl->SetUseHorizontalScrollBar(false);
        
        // connect the entered-pressed signal
        m_text_ctrl->sigCommandEntered().connect(this, &ConsolePanel::onCommandEntered);

        // set the font
        m_text_ctrl->SetFont(wxFont(10, wxMODERN, wxNORMAL, wxNORMAL));
        m_text_ctrl->Show();
        m_text_ctrl->Freeze();
    }
     else
    {
        m_text_ctrl->Freeze();
    }

    m_text_ctrl->Thaw();

    doSizing();
}

void ConsolePanel::clear()
{
    m_text_ctrl->SetText(wxT(""));
}

void ConsolePanel::print(const wxString& str)
{
    m_text_ctrl->AppendText(str);
    m_text_ctrl->GoToEnd();
}

void ConsolePanel::cut()
{
    m_text_ctrl->Cut();
}

void ConsolePanel::copy()
{
    m_text_ctrl->Copy();
}

void ConsolePanel::paste()
{
    m_text_ctrl->Paste();
}

void ConsolePanel::onSize(wxSizeEvent& evt)
{
    doSizing();
}

void ConsolePanel::onSetFocus(wxFocusEvent& evt)
{
    setDocumentFocus();
}

void ConsolePanel::onKillFocus(wxFocusEvent& evt)
{
}

void ConsolePanel::onLeftDblClick(wxStyledTextEvent& evt)
{
    if (!m_text_ctrl)
        return;

    klregex::wregex regex(L"^(.+?)\\((\\d+)\\):.*$");
    klregex::wmatch matchres;
    std::wstring str = towstr(m_text_ctrl->GetCurrentLineText());
    
    if (regex.match(str.c_str(), matchres))
    {
        std::wstring filename = matchres.getSubMatch(1).str();
        std::wstring line_str = matchres.getSubMatch(2).str();
        int line = kl::wtoi(line_str);
        
        
        int site_id = 0;
        IEditorDocPtr editor;
        IDocumentSitePtr site;
        
        g_app->getAppController()->setActiveChildByLocation(filename, &site_id);

        if (site_id)
        {
            site = g_app->getMainFrame()->lookupSiteById(site_id);
            if (site)
            {
                editor = site->getDocument();
                if (editor)
                {
                    editor->goLine(line);
                    IDocumentPtr doc = editor;
                    doc->setDocumentFocus();
                    return;
                }
            }
        }
        
        g_app->getAppController()->openScript(filename, &site_id);
        site = g_app->getMainFrame()->lookupSiteById(site_id);
        if (site)
        {
            editor = site->getDocument();
            if (editor)
            {
                editor->goLine(line);
                IDocumentPtr doc = editor;
                doc->setDocumentFocus();
            }
        }
    }
}

void ConsolePanel::onCommandEntered(wxString& command)
{
    if (!m_text_ctrl)
        return;

    // run the command
    runCommand(command);
}

void ConsolePanel::onCommand(wxString& command)
{
    if (m_frame.isOk())
        m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CONSOLEPANEL_COMMAND, 0, command));
}

void ConsolePanel::onQueryJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() == jobs::jobStateFailed)
    {
        // the job failed; add an error code and populate the text
        wxString error_string = job->getJobInfo()->getErrorString();
        if (error_string.Length() == 0)
            error_string = wxT("Error: Command failed for an unknown reason");
             else
            error_string = wxT("Error: ") + error_string;

        echo(error_string);

        // if the command failed as a sql statement, treat the
        // text entered as a command and fire the command signal
        sigCommand().fire(m_command_text);
        return;
    }

    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;


    bool success = false;

    xd::IIteratorPtr result_iter = job->getResultObject();
    if (result_iter.isOk())
    {
        std::wstring table_path = result_iter->getTable();

        if (table_path.length() > 0 || m_command_db_path.length() > 0)
        {
            ITableDocPtr doc = TableDocMgr::createTableDoc();
            doc->setTemporaryModel(true);
            doc->open(table_path, result_iter);
            g_app->getMainFrame()->createSite(doc,
                                              sitetypeNormal,
                                              -1, -1, -1, -1);
        }

        // TODO: we should do a better job checking for success;
        // however, we can't base the test on a result set since 
        // some types of sql statements don't return a result set
        // (i.e. drop table), and the db doc should refresh for
        // these the same as when a result is returned
        success = true;
    }
    
    if (success)
    {
        g_app->getAppController()->refreshDbDoc();

        IDocumentSitePtr site;
        site = g_app->getMainFrame()->lookupSite(wxT("ConsolePanel"));
        if (site)
        {
            site->getContainerWindow()->SetFocus();
            site->getDocument()->setDocumentFocus();
        }
    }
}

bool ConsolePanel::processClear(const std::vector<wxString>& tokens)
{
    // syntax: CLEAR
    // description: clears the console
    // params: no parameters

    clear();
    return true;
}

bool ConsolePanel::processOpen(const std::vector<wxString>& tokens)
{
    // syntax: OPEN <file>
    // description: opens a file in the current project
    // params <file>: the path of the file to open; either
    //     a relative path in the project, or a URL

    // if we don't have a parameter, we're done
    if (tokens.size() < 2)
    {
        wxString response = wxT("Error: need to specify a location to open");
        echo(response);
        return true;
    }

    // get the parameter, which is the location to open;
    // if it's quoted, remove the quotes
    wxString path = tokens[1];
    removeQuotes(path, m_lquote, m_rquote);

    // try to open the location
    if (!g_app->getAppController()->openAny(path))
    {
        wxString response = wxT("Error: could not open the specified location");
        echo(response);
    }

    return true;
}

bool ConsolePanel::processClose(const std::vector<wxString>& tokens)
{
    // syntax: CLOSE
    // description: closes the active document; particularly
    //     useful for closing queries that have just been created
    //     from the command line
    // params: no parameters

    IDocumentSitePtr site = g_app->getMainFrame()->getActiveChild();
    if (site.isOk())
        g_app->getMainFrame()->closeSite(site);

    return true;
}

bool ConsolePanel::processUse(const std::vector<wxString>& tokens)
{
    // syntax: USE <directory>
    // description: changes the active database to use when processing 
    //     SQL statements; if no parameters are entered, selects the 
    //     top-level project database
    // params <directory>: specifies the mounted directory to use when
    //     processing queries that are entered    

    // reset the command db
    m_command_db_path = wxT("");
  
    // if we don't have a parameter, we're done
    if (tokens.size() < 2)
    {
        echoDatabaseInfo(xcm::null, m_command_db_path);
        return true;
    }
    
    // get the parameter, which is the path of the database
    // to use; if it's quoted, remove the quotes
    wxString path = tokens[1];
    removeQuotes(path, m_lquote, m_rquote);

    // if a mount is specified, make we can mount the db
    xd::IDatabasePtr db = g_app->getDatabase();
    xd::IDatabasePtr mount_db;
    if (db.isOk())
    {
        mount_db = db->getMountDatabase(towstr(path));

        if (mount_db.isOk())
        {
            // note: don't store the xd::IDatabasePtr directly
            // since if the user changes the mount information
            // to another database in the project panel, we want 
            // the command to go to the new database
            m_command_db_path = path;
        }
    }

    echoDatabaseInfo(mount_db, m_command_db_path);
    return true;
}

bool ConsolePanel::processPwd(const std::vector<wxString>& tokens)
{
    // syntax: PWD
    // description: echos the currently selected database to 
    //     which to pass any query that is entered
    // params: no parameters


    // if a mount is specified, make we can mount the db
    xd::IDatabasePtr db = g_app->getDatabase();
    xd::IDatabasePtr mount_db;
    if (db.isOk())
    {
        mount_db = db->getMountDatabase(towstr(m_command_db_path));
    }

    echoDatabaseInfo(mount_db, m_command_db_path);
    return true;
}

bool ConsolePanel::processEval(const std::vector<wxString>& tokens)
{
    // syntax: EVAL <code>
    // description: evaluates the input javascript code and returns
    //     a result
    // params <code>: the code to execute

    // if we don't have a parameter, we're done
    if (tokens.size() < 2)
    {
        wxString response = wxT("Error: no code to execute");
        echo(response);
        return true;
    }

    // if we have two tokens, the second token is properly escaped
    // syntax; extract the command
    wxString command = wxT("");
    if (tokens.size() == 2)
    {
        command = tokens[1];
        removeQuotes(command);
    }
    else
    {
        // the code to run isn't quoted and is stored in multiple
        // tokens (e.g. EVAL 1 + 1;); so, get the raw command string,
        // extract everything after the eval, and treat it as a
        // single parameter

        // get everything after the keyword
        wxString t = tokens[0];
        int offset = m_command_text.Find(t) + t.Length() + 1;
        command = m_command_text.Mid(offset);
    }

    // create a script host object
    ScriptHost* script_host = new ScriptHost;
    
    // compile the script
    if (!script_host->compile(command))
    {
        echo(script_host->getErrorString());    

        delete script_host;
        return true;
    }

    // run the script
    script_host->run();

    // if there's a result, return it
    kscript::Value* result = script_host->getRetval();
    if (!result->isUndefined())
        echo(script_host->getRetval()->getString());

    return true;
}

bool ConsolePanel::processExit(const std::vector<wxString>& tokens)
{
    // syntax: EXIT
    // description: exits the application
    // params: no parameters

    m_frame->getFrameWindow()->Close();
    return true;
}

void ConsolePanel::runCommand(wxString& command)
{
    // if we don't have a command, we're done
    if (command.Length() == 0)
        return;

    // save the command text
    m_command_text = command;

    // process non-Sql commands; if the command is processed,
    // we're done
    if (processNonSqlCommand(command))
    {    
        SetFocus();
        return;
    }

    // process SQL commands
    int flags = xd::sqlPassThrough;
    if (command[0] == '~')
    {
        // for debugging purposes (not specifying sqlAlwaysCopy)
        command.erase(0,1);
        flags = 0;
    }

    std::wstring table = getTableNameFromSql(towstr(command));
    if (table.length() > 0)
    {
        // find out if it's a local table, and if it
        // is, turn on 'always copy';  tabledoc can't yet
        // directly handle certain types of queries like
        // select field1,field2 FROM tbl
        xd::IDatabasePtr db = g_app->getDatabase();
        if (db.isOk())
        {
            xd::IDatabasePtr mount_db;                    
            mount_db = db->getMountDatabase(table);

           // if (mount_db.isNull())
            {
                flags |= xd::sqlAlwaysCopy;
            }
        }
    }


    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.execute-job");


    if (m_command_db_path.length() > 0)
    {

        // if a command database is specified, set the database so
        // the command are passed directly through to that database
        xd::IDatabasePtr db = g_app->getDatabase();
        xd::IDatabasePtr mount_db;
        if (db.isOk())
        {
            mount_db = db->getMountDatabase(towstr(m_command_db_path));
            if (mount_db.isOk())
            {
                // note: we get the mount db each command since if the user
                // changes the mount information to another database in the
                // project panel, we want the command to use the new database;
                // if we cache the mount database when it's changed using 'use',
                // it's difficult to invalidate the cache when the db mount
                // information is changed in the project panel

                mount_db = db->getMountDatabase(towstr(m_command_db_path));
            }
        }

        if (mount_db.isOk())
            job->setDatabase(mount_db);
    }





    if (flags & xd::sqlAlwaysCopy)
        job->setExtraValue(L"xd.sqlAlwaysCopy", L"true");


    // run the job
    kl::JsonNode params;
    params["command"].setString(towstr(command));

    job->getJobInfo()->setTitle(towstr(_("Query")));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(this, &ConsolePanel::onQueryJobFinished);
    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);


    SetFocus();
}

bool ConsolePanel::processNonSqlCommand(wxString& command)
{
    // returns true if command was processed; false otherwise

    // first, tokenize the command
    std::vector<wxString> tokens;
    tokenizeCommand(command, tokens, m_lquote, m_rquote);

    // if we don't have any tokens, we're done
    if (tokens.size() < 1)
        return false;

    // get the command keyword
    wxString command_keyword = tokens[0];

    // process the commands
    if (0 == command_keyword.CmpNoCase(wxT("OPEN")))
        return processOpen(tokens);

    if (0 == command_keyword.CmpNoCase(wxT("CLOSE")))
        return processClose(tokens);

    if (0 == command_keyword.CmpNoCase(wxT("CLEAR")))
        return processClear(tokens);

    if (0 == command_keyword.CmpNoCase(wxT("PWD")))
        return processPwd(tokens);

    if (0 == command_keyword.CmpNoCase(wxT("USE")))
        return processUse(tokens);

    if (0 == command_keyword.CmpNoCase(wxT("EVAL")))
        return processEval(tokens);

    if (0 == command_keyword.CmpNoCase(wxT("EXIT")))
        return processExit(tokens);

    // command wasn't processed
    return false;
}

void ConsolePanel::tokenizeCommand(const wxString& _command,
                                   std::vector<wxString>& tokens, 
                                   wchar_t lquote, wchar_t rquote)
{
    // take the command and tokenize it into parts based on space,
    // but keeping quoted items together; so COMMAND PARAM1 [PARAM 2]
    // would tokenize into three parts, while COMMAND PARAM1 PARAM 2
    // would tokenize into four parts

    wxString command = _command;
        
    // trim leading and trailing space
    command.Trim(true);
    command.Trim(false);

    // if there's no command, we're done
    if (command.Length() == 0)
        return;
    
    // trim any trailing semi-colon
    if (command.Last() == L';')
    {
        command.RemoveLast();
        command.Trim(true);
    }

    // after trimming the semi-colon, if there's no command, 
    // we're done
    if (command.Length() == 0)
        return;
    
    // break the string up into parts, based on space,
    // but keeping quoted items as one unit, even if
    // they contain a space
    const wchar_t* expr = command.c_str();
    wchar_t* ch = (wchar_t*)expr;
    
    while (1)
    {
        // since we're trimmed leading/trailing spaces
        // and the string has length, the first character
        // is either a left quote or a regular character
        
        // if we're at the end of the string, add the string 
        // as the token, and we're done
        if (!*ch)
        {
            tokens.push_back(command);
            return;
        }

        // if we're on a left quote, advance to the right quote; 
        // if we can't find it, add the string as the token, and 
        // we're done
        if (*ch == lquote)
        {
            ch = zl_strchr((wchar_t*)ch, rquote);

            if (!ch)
            {
                tokens.push_back(command);
                return;
            }
        }
        
        // if we're on a white space, add the string up to the
        // whitespace, then call the function recursively on 
        // the remainder of the string
        if (::iswspace(*ch))
        {
            int offset = (int)(ch - expr);
            wxString token = command.substr(0, offset);
            tokens.push_back(token);
            tokenizeCommand(command.substr(offset), tokens, lquote, rquote);
            return;
        }

        // advance to the next character
        ++ch;
    }
}

void ConsolePanel::removeQuotes(wxString& command, 
                                wchar_t lquote, wchar_t rquote)
{
    if (command.Length() < 2)
        return;

    if (command[0] == lquote && command[command.Length()-1] == rquote)
        command = command.Mid(1, command.Length()-2);
}

void ConsolePanel::echo(const wxString& text)
{
    // append the echoed item manually to the text
    m_text_ctrl->EchoText(text + wxT("\n\n"));
}

void ConsolePanel::echoDatabaseInfo(xd::IDatabasePtr command_db, const wxString& command_path)
{
    // echo the new database to use
    wxString response;
    if (command_db.isNull())
    {
        response += wxT("Result: Using Local Database");
    }
     else
    {
        response += wxT("Result: Using ");
        response += g_app->getProjectName();
        response += wxT(" from ");
        response += L"[";
        response += command_path;
        response += L"]";
    }

    echo(response);
}

void ConsolePanel::doSizing()
{
    if (m_text_ctrl)
    {
        wxSize size = GetClientSize();
        m_text_ctrl->SetSize(0, 0, size.GetWidth(), size.GetHeight());
    }
}

