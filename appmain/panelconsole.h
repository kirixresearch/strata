/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-04-26
 *
 */


#ifndef __APP_PANELCONSOLE_H
#define __APP_PANELCONSOLE_H


// interface

xcm_interface IConsolePanel;
XCM_DECLARE_SMARTPTR(IConsolePanel)


xcm_interface IConsolePanel : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IConsolePanel")


public:

    virtual void refresh() = 0;
    virtual void print(const wxString& str) = 0;
    virtual void clear() = 0;

    virtual void cut() = 0;
    virtual void copy() = 0;
    virtual void paste() = 0;
};


// class definitions

class CommandTextCtrl;
class wxStyledTextEvent;

class ConsolePanel : public wxWindow,
                     public IConsolePanel,
                     public IDocument,
                     public xcm::signal_sink
{

XCM_CLASS_NAME_NOREFCOUNT("appmain.ConsolePanel")
XCM_BEGIN_INTERFACE_MAP(ConsolePanel)
    XCM_INTERFACE_ENTRY(IDocument)
    XCM_INTERFACE_ENTRY(IConsolePanel)
XCM_END_INTERFACE_MAP()

XCM_IMPLEMENT_SIGNAL1(sigCommand, wxString&)

public:

    ConsolePanel();
    ~ConsolePanel();

    // IDocument interface implementation
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // IConsolePanel interface
    void refresh();
    void clear();
    void print(const wxString& str);

    void cut();
    void copy();
    void paste();

private:

    // event handlers
    void onSize(wxSizeEvent& evt);
    void onSetFocus(wxFocusEvent& evt);
    void onKillFocus(wxFocusEvent& evt);
    void onLeftDblClick(wxStyledTextEvent& evt);
    
    // signal handlers
    void onCommandEntered(wxString& command);
    void onCommand(wxString& command);
    void onQueryJobFinished(IJobPtr job);

private:

    // commands
    bool processClear(const std::vector<wxString> tokens);
    bool processOpen(const std::vector<wxString> tokens);
    bool processClose(const std::vector<wxString> tokens);
    bool processSet(const std::vector<wxString> tokens);    
    bool processUse(const std::vector<wxString> tokens);
    bool processPwd(const std::vector<wxString> tokens);
    bool processEval(const std::vector<wxString> tokens);
    bool processExit(const std::vector<wxString> tokens);

private:

    void runCommand(wxString& command);
    bool processNonSqlCommand(wxString& command);   

    void tokenizeCommand(const wxString& _command,
                         std::vector<wxString>& tokens, 
                         wchar_t lquote = L'[', wchar_t rquote = L']');
    void removeQuotes(wxString& command, 
                      wchar_t lquote = L'[', wchar_t rquote = L']');

    void echo(const wxString& text);
    void echoDatabaseInfo(tango::IDatabasePtr command_db, const wxString& command_path);

    void doSizing();

private:

    // controls
    IFramePtr m_frame;
    CommandTextCtrl* m_text_ctrl;

    // database to use for processing commands
    wxString m_command_db_path;
    
    // command
    wxString m_command_text;
    
    // quote characters to use for command
    wchar_t m_lquote;
    wchar_t m_rquote;

private:

    DECLARE_EVENT_TABLE()
};




#endif


