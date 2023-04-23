/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-09-19
 *
 */


#ifndef __APP_TEXTDOC_H
#define __APP_TEXTDOC_H



#include "xdgridmodel.h"
#include "panelcolumnlist.h"


xcm_interface ITextDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ITextDoc")

public:

    virtual bool open(const wxString& filename) = 0;
    virtual void close() = 0;
    virtual wxString getPath() = 0;
    virtual xd::Structure getStructure() = 0;
    virtual void setSourceUrl(const wxString& source_url) = 0; // allows override of displayed url
    virtual xd::FormatDefinition& getDefinition() = 0;
    virtual void refreshFromDefinition() = 0;
    virtual void refreshDocuments() = 0;
    virtual bool save(bool do_refresh) = 0;
    virtual bool checkSave() = 0;
};

XCM_DECLARE_SMARTPTR(ITextDoc)



class TextView;
class TextViewColumn;

class TextDoc : public wxWindow,
                public IDocument,
                public StatusBarProviderBase,
                public ITextDoc,
                public IColumnListTarget,
                public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.TextDoc")
    XCM_BEGIN_INTERFACE_MAP(TextDoc)
        XCM_INTERFACE_ENTRY(IDocument)
        XCM_INTERFACE_ENTRY(ITextDoc)
        XCM_INTERFACE_ENTRY(IColumnListTarget)
        XCM_INTERFACE_CHAIN(StatusBarProviderBase)
    XCM_END_INTERFACE_MAP()

public:

    enum
    {
        FixedLengthView = 0,
        TextDelimitedView
    };
    
    enum
    {
        StandardEncoding = 0,
        EbcdicEncoding
    };
    
public:

    TextDoc();
    virtual ~TextDoc();

    // IDocument
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    wxString getDocumentLocation();
    void setDocumentFocus();
    bool checkSave(); // saves the definition file (after asking user if m_dirty is true)
    bool onSiteClosing(bool force);
    void onSiteDeactivated();
    void onSiteActivated();

    void updateCaption();

    xd::FormatDefinition& getDefinition() { return m_def; }

    // ITextDoc
    void setSourceUrl(const wxString& source_url);
    xd::Structure getStructure();
    bool open(const wxString& filename);
    void close();
    bool save(bool do_refresh = false);
    wxString getPath();

    void refreshFromDefinition();
    void refreshDocuments();
    
    // IColumnListTarget
    void getColumnListItems(std::vector<ColumnListItem>& items);
    
private:
    
    void doTextModeLayout();    // changes the layout based on the file type
    bool updateColumnList();
    void updateStatusBar();
    void refreshGrid();

    bool initFixedLengthView();
    bool initDelimitedTextView();
    
    wxBoxSizer* createMainSettingsSizer();
    wxBoxSizer* createFixedLengthSettingsSizer();
    wxBoxSizer* createTextDelimitedSettingsSizer();

    // only to be used with fixed-length text files
    int getRowWidth();
    int getBeginningSkipCharacters();
    bool isLineDelimited();
    
    // only to be used with text-delimited text files
    wxString getFieldDelimiters();
    wxString getTextQualifier();
    bool isFirstRowFieldNames();
    
    // layout storage
    bool saveLayoutTemplate(const wxString& path);

private:

    // event handlers
    void onOptionsMenu(wxCommandEvent& evt);
    void onSave(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onFileTypeChanged(wxCommandEvent& evt);
    void onEncodingChanged(wxCommandEvent& evt);
    void onFixedLengthSkipCharTextEnter(wxCommandEvent& evt);
    void onFixedLengthRowWidthTextEnter(wxCommandEvent& evt);
    void onFixedLengthSkipCharSpun(wxSpinEvent& evt);
    void onFixedLengthRowWidthSpun(wxSpinEvent& evt);
    void onFixedLengthLineDelimitedChecked(wxCommandEvent& evt);
    void onTextDelimitedFieldDelimiterTextEnter(wxCommandEvent& evt);
    void onTextDelimitedFieldDelimiterCombo(wxCommandEvent& evt);
    void onTextDelimitedTextQualifierTextEnter(wxCommandEvent& evt);
    void onTextDelimitedTextQualifierTextChanged(wxCommandEvent& evt);
    void onTextDelimitedTextQualifierCombo(wxCommandEvent& evt);
    void onTextDelimitedFirstRowFieldNamesChecked(wxCommandEvent& evt);
    void onTextDelimitedCaptionBeginEdit(kcl::GridEvent& evt);
    void onTextDelimitedCaptionEndEdit(kcl::GridEvent& evt);
    void onToggleView(wxCommandEvent& evt);

    // signal handlers
    void onTextViewColumnAdded(TextViewColumn& col);
    void onTextViewColumnDeleted(TextViewColumn& col);
    void onTextViewColumnModified(TextViewColumn& col, TextViewColumn& new_settings);
    void onTextViewCursorPositionChanged(
                                  int new_cursor_offset,
                                  int new_cursor_row);

    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI(wxUpdateUIEvent& evt);
    
private:

    IFramePtr m_frame;             // ptr to the application frame
    IDocumentSitePtr m_doc_site;   // ptr to our document site

    wxBoxSizer* m_main_sizer;
    wxBoxSizer* m_main_settings_sizer;
    wxBoxSizer* m_fixedlength_settings_sizer;
    wxBoxSizer* m_textdelimited_settings_sizer;
    
    // main controls
    wxChoice* m_filetype_choice;
    wxChoice* m_encoding_choice;
    TextView* m_textview;
    kcl::Grid* m_grid;
    
    // fixed-length settings sizer controls
    wxSpinCtrl* m_skipchars_spinctrl;
    wxSpinCtrl* m_rowwidth_spinctrl;
    wxCheckBox* m_linedelimited_checkbox;
    
    // text-delimited settings sizer controls
    wxComboBox* m_delimiters_combobox;
    wxComboBox* m_textqualifier_combobox;
    wxCheckBox* m_firstrowfieldnames_checkbox;
    

    xd::IIteratorPtr m_textdelimited_iter;
    IXdGridModelPtr m_grid_model;
    
    wxString m_path;
    wxString m_source_url;          // source url (can override what's displayed in the url bar)
    std::wstring m_definition_file; // filename that stores the xd definition
    int m_view;                     // fixed-length or text-delimited
    int m_encoding;                 // 
    bool m_dirty;                   // changes have been made to the document
    bool m_loading_definition;      // if we're loading a defintion, don't fire any signals
    bool m_path_is_datafile;        // true if m_path is a datafile, or a path to a definition file
    int m_last_textqualifier_sel;   // last text qualifier combobox selection
    int m_last_delimiters_sel;      // last delimiters combobox selection
    wxString m_last_textqualifier;  // last text qualifier that was selected or typed in
    wxString m_last_delimiters;     // last delimiters that were selected or typed in

    std::vector<wxString> m_filetype_labels;        // labels for filetype choice
    std::vector<wxString> m_encoding_labels;        // labels for encoding choice
    std::vector<wxString> m_delimiters_labels;      // labels for delimiters combobox
    std::vector<wxString> m_textqualifier_labels;   // labels for text qualifier combobox

    xd::FormatDefinition m_def;

    xd::FormatDefinition m_def_frc;    // m_def copy with first-row-columns = true
    xd::FormatDefinition m_def_nofrc;  // m_def copy with first-row-columns = false

    DECLARE_EVENT_TABLE()
};


ITextDocPtr createTextDoc(const std::wstring& filename, wxWindow* container_wnd, int* site_id);


#endif  // __APP_TEXTDOC_H


