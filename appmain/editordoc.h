/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-28
 *
 */


#ifndef __APP_EDITORDOC_H
#define __APP_EDITORDOC_H


#include "panelfind.h"


class PageSetupPanel;
class EditorCtrl;


xcm_interface IEditorDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IEditorDoc")

public:
    
    virtual void goLine(int line) = 0;
    virtual void reportError(size_t offset,
                             int line,
                             const wxString& message) = 0;
    virtual bool doSave() = 0;
    virtual bool isModified() = 0;
    virtual bool isTemporary() = 0;
    virtual void setText(const wxString& text) = 0;
};

XCM_DECLARE_SMARTPTR(IEditorDoc)


class EditorDoc : public wxWindow,
                  public cfw::IDocument,
                  public IEditorDoc,
                  public IFindTarget,
                  public cfw::StatusBarProviderBase,
                  public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.EditorDoc")
    XCM_BEGIN_INTERFACE_MAP(EditorDoc)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
        XCM_INTERFACE_ENTRY(IEditorDoc)
        XCM_INTERFACE_ENTRY(IFindTarget)
        XCM_INTERFACE_CHAIN(cfw::StatusBarProviderBase)
    XCM_END_INTERFACE_MAP()

public:

    static bool newFile(const wxString& path);

public:

    EditorDoc();
    virtual ~EditorDoc();

    bool loadFile(const wxString& path);
    bool saveFile();

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    wxString getDocumentTitle();
    wxString getDocumentLocation();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    void onSiteActivated();

    // -- IFindTarget --
    bool findNextMatch(
                    const wxString& text,
                    bool forward,
                    bool match_case,
                    bool whole);
    bool findReplaceWith(
                    const wxString& find_val,
                    const wxString& replace_val,
                    bool forward,
                    bool match_case,
                    bool whole);
    bool findReplaceAll(
                    const wxString& find_val,
                    const wxString& replace_val,
                    bool match_case,
                    bool whole);
    bool findIsReplaceAllowed();

    // -- IEditorDoc --
    void goLine(int line);
    void reportError(size_t offset,
                     int line,
                     const wxString& message);
    bool doSave();
    bool isTemporary();
    bool isModified();

    void setText(const wxString& text);

    // -- checks for external changes; public simply so
    //    it can work with timer class --
    void checkForExternalChanges();
    
private:

    void refreshControlPreferences();
    void updateStatusBar();
    void updateCaption();
    void updateContent();
    void gotoLine();
    void setCurrentEolMode(const wxString& value);

    void onExecute(wxCommandEvent& evt);
    void onSave(wxCommandEvent& evt);
    void onSaveAs(wxCommandEvent& evt);
    void onSaveAsExternal(wxCommandEvent& evt);
    void onPrint(wxCommandEvent& evt);
    void onPageSetup(wxCommandEvent& evt);
    void onUndo(wxCommandEvent& evt);
    void onRedo(wxCommandEvent& evt);
    void onCut(wxCommandEvent& evt);
    void onCopy(wxCommandEvent& evt);
    void onPaste(wxCommandEvent& evt);
    void onSelectAll(wxCommandEvent& evt);
    void onGoTo(wxCommandEvent& evt);
    void onZoomIn(wxCommandEvent& evt);
    void onZoomOut(wxCommandEvent& evt);
    void onZoomToActual(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI(wxUpdateUIEvent& evt);

    // frame event handlers
    void onFrameEvent(cfw::Event& evt);
    
    // statusbar event handlers
    void onStatusBarItemLeftDblClick(cfw::IStatusBarItemPtr item);

    // function for reading a file
    bool readFile(const wxString _path, 
                  wxString& value,
                  wxString& mime_type,
                  bool& external);

    // gets the hash of a file based on a path
    bool getFileHash(const wxString path, std::wstring& hash);

private:

    EditorCtrl* m_text;
    bool m_temporary;
    bool m_external;
    wxString m_path;
    wxString m_error_message;
    wxString m_mime_type;
    int m_eol_mode;           // end of line mode -- CR, LF, or CRLF

    cfw::IFramePtr m_frame;
    cfw::IDocumentSitePtr m_doc_site;

    // stores a hash of the file when it was last loaded
    // or saved to determine if the file has changed
    std::wstring m_file_hash;

private:

    // page dimensions for printing in inches
    double m_page_width;
    double m_page_height;
    double m_page_margin_left;
    double m_page_margin_right;
    double m_page_margin_top;
    double m_page_margin_bottom;

    DECLARE_EVENT_TABLE()
};






#endif

