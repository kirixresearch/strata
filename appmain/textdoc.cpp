/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-09-19
 *
 */


#include "appmain.h"
#include "app.h"
#include "appcontroller.h"
#include "dlgdatabasefile.h"
#include "toolbars.h"
#include "textdoc.h"
#include "transformationdoc.h"
#include "tabledoc.h"
#include "jsonconfig.h"
#include <wx/filesys.h>


#define MAX_SKIPCHARS 2000000000
#define MAX_ROWWIDTH  50000


enum
{
    ID_CharEncodingChoice = 22354,
    ID_FiletypeChoice,
    ID_EncodingChoice,
    ID_SkipCharsSpinCtrl,
    ID_RowWidthSpinCtrl,
    ID_LineDelimitedCheckBox,
    ID_DelimitersComboBox,
    ID_TextQualifierComboBox,
    ID_FirstRowFieldNamesCheckBox,
    ID_OptionsMenu
};


// the following enums must be kept in sync with their corresponding
// label vectors in TextDoc (see the constructor)

enum FileTypeIdx
{
    FileType_TextDelimited = 0,
    FileType_FixedLength = 1
};

enum DelimiterIdx
{
    DelimiterIdx_Comma = 0,
    DelimiterIdx_Tab = 1,
    DelimiterIdx_Semicolon = 2,
    DelimiterIdx_Space = 3,
    DelimiterIdx_Pipe = 4,
    DelimiterIdx_None = 5,
    DelimiterIdx_Other = 6
};

enum TextQualifierIdx
{
    TextQualifierIdx_DoubleQuote = 0,
    TextQualifierIdx_SingleQuote = 1,
    TextQualifierIdx_None = 2,
    TextQualifierIdx_Other = 3
};


// this function prompts the user as to whether or not
// they want to overwrite their existing transformation

static int showOverwriteTransformationChangesDialog()
{
    int result = appMessageBox(_("Performing this operation will overwrite any changes that have already been made to the table structure.\nAre you sure you want to continue?"),
                                    APPLICATION_NAME,
                                    wxYES_NO | wxICON_QUESTION | wxCENTER);
    return result;
}


// this function finds the best insert position in the destination structure
// based on the offset of the column to the left of the column that we
// inserted in the source structure

static int getBestDestinationInsertPosition(tango::IColumnInfoPtr src_coltoleft,
                                            tango::IStructurePtr dest_struct)
{
    if (!src_coltoleft)
        return 0;
        
    tango::IColumnInfoPtr dest_col;
    
    int i, col_count = dest_struct->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        dest_col = dest_struct->getColumnInfoByIdx(i);
        if (dest_col->getExpression() == src_coltoleft->getName())
            return i+1;
    }
    
    return -1;
}



// this function creates the TextDoc group of sites

ITextDocPtr createTextDoc(const wxString& filename,
                          wxWindow* container_wnd,
                          int* site_id)
{
    if (site_id)
        *site_id = 0;
    
    IFramePtr frame = g_app->getMainFrame();
    IDocumentSitePtr textdoc_site;
    IDocumentSitePtr transdoc_site;
    IDocumentSitePtr tabledoc_site;

    // create a new TextDoc
    TextDoc* textdoc = new TextDoc();
    if (!textdoc->open(filename))
        return xcm::null;
    
    // create a new TableDoc
    ITableDocPtr tabledoc = TableDocMgr::createTableDoc();
    tabledoc->open(filename);

    // create a new TransformationDoc
    TransformationDoc* transdoc = new TransformationDoc();
    if (!transdoc->open(filename))
        return xcm::null;

    if (container_wnd)
    {
        tabledoc_site = frame->createSite(container_wnd,
                                          tabledoc,
                                          false);
    }
     else
    {
        tabledoc_site = frame->createSite(tabledoc,
                                          sitetypeNormal,
                                         -1, -1, -1, -1);
        container_wnd = tabledoc_site->getContainerWindow();
    }


                          
    if (tabledoc_site.isOk() && site_id)
        *site_id = tabledoc_site->getId();



    textdoc_site = frame->createSite(container_wnd,
                                     static_cast<IDocument*>(textdoc),
                                     false);

    transdoc_site = frame->createSite(container_wnd,
                                      static_cast<IDocument*>(transdoc),
                                      false);
    transdoc->initFromSet(textdoc->getTextSet());



    // activate table doc site for starts if this textdoc is being created in a 
    // container which already exists

    // TODO: for now, always open the view in the table view; we used to have 
    // some logic for saving whether to open a table in the text view on a per-table 
    // basis based on the state in which it was last viewed, but this value was stored 
    // as a single value in the OFS on a per table and the implementation used a 
    // legacy approach that would be better to reimplement than adopt
    bool show_tabledoc = true;

    if (container_wnd)
    {
        if (show_tabledoc)
            frame->activateInPlace(tabledoc_site);
              else
            frame->activateInPlace(textdoc_site);
    }
     else
    {
        if (show_tabledoc)
            frame->setActiveChild(tabledoc_site);
             else
            frame->setActiveChild(textdoc_site);        
    }
    

    return static_cast<ITextDoc*>(textdoc);
}




BEGIN_EVENT_TABLE(TextDoc, wxWindow)
    EVT_SIZE(TextDoc::onSize)
    EVT_MENU(ID_File_Save, TextDoc::onSave)
    EVT_MENU(ID_View_SwitchToDesignView, TextDoc::onToggleView)
    EVT_BUTTON(ID_OptionsMenu, TextDoc::onOptionsMenu)
    EVT_CHOICE(ID_FiletypeChoice, TextDoc::onFileTypeChanged)
    EVT_CHOICE(ID_EncodingChoice, TextDoc::onEncodingChanged)
    EVT_TEXT_ENTER(ID_SkipCharsSpinCtrl, TextDoc::onFixedLengthSkipCharTextEnter)
    EVT_TEXT_ENTER(ID_RowWidthSpinCtrl, TextDoc::onFixedLengthRowWidthTextEnter)
    EVT_SPINCTRL(ID_SkipCharsSpinCtrl, TextDoc::onFixedLengthSkipCharSpun)
    EVT_SPINCTRL(ID_RowWidthSpinCtrl, TextDoc::onFixedLengthRowWidthSpun)
    EVT_CHECKBOX(ID_LineDelimitedCheckBox, TextDoc::onFixedLengthLineDelimitedChecked)
    EVT_TEXT_ENTER(ID_DelimitersComboBox, TextDoc::onTextDelimitedFieldDelimiterTextEnter)
    EVT_TEXT_ENTER(ID_TextQualifierComboBox, TextDoc::onTextDelimitedTextQualifierTextEnter)
    EVT_COMBOBOX(ID_DelimitersComboBox, TextDoc::onTextDelimitedFieldDelimiterCombo)
    EVT_COMBOBOX(ID_TextQualifierComboBox, TextDoc::onTextDelimitedTextQualifierCombo)
    EVT_TEXT(ID_TextQualifierComboBox, TextDoc::onTextDelimitedTextQualifierTextChanged)
    EVT_CHECKBOX(ID_FirstRowFieldNamesCheckBox, TextDoc::onTextDelimitedFirstRowFieldNamesChecked)
    EVT_KCLGRID_BEGIN_CAPTION_EDIT(TextDoc::onTextDelimitedCaptionBeginEdit)
    EVT_KCLGRID_END_CAPTION_EDIT(TextDoc::onTextDelimitedCaptionEndEdit)

    // disable data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, TextDoc::onUpdateUI_DisableAlways)

    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, TextDoc::onUpdateUI_DisableAlways)

    // disable format items
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, TextDoc::onUpdateUI_DisableAlways)

    // disable table items
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, TextDoc::onUpdateUI_DisableAlways)

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_SaveAs, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_SaveAsExternal, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Bookmark, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_PageSetup, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Print, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Run, TextDoc::onUpdateUI_DisableAlways)

    // disable the zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomIn, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomOut, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToActual, TextDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, TextDoc::onUpdateUI_DisableAlways)

    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, TextDoc::onUpdateUI_DisableAlways)

END_EVENT_TABLE()


TextDoc::TextDoc()
{
    m_fixedlength_set = xcm::null;
    m_textdelimited_set = xcm::null;
    m_textdelimited_iter = xcm::null;
    
    m_dirty = false;
    m_loading_definition = false;
    m_fixedlength_inited = false;
    m_textdelimited_inited = false;
    m_last_textqualifier = wxEmptyString;
    m_last_delimiters = wxEmptyString;
    
    m_textview = NULL;
    m_skipchars_spinctrl = NULL;
    m_rowwidth_spinctrl = NULL;
    m_linedelimited_checkbox = NULL;
    m_delimiters_combobox = NULL;
    m_encoding_choice = NULL;
    m_textqualifier_combobox = NULL;
    m_firstrowfieldnames_checkbox = NULL;

    // the following vectors labels must be kept in sync
    // with their corresponding enums above

    m_filetype_labels.push_back(_("Text-Delimited"));
    m_filetype_labels.push_back(_("Fixed-Length"));
    
    m_encoding_labels.push_back(_("ASCII/Unicode"));
    m_encoding_labels.push_back(_("EBCDIC"));

    m_delimiters_labels.push_back(_("Comma (,)"));
    m_delimiters_labels.push_back(_("Tab "));
    m_delimiters_labels.push_back(_("Semicolon (;)"));
    m_delimiters_labels.push_back(_("Space ( )"));
    m_delimiters_labels.push_back(_("Pipe (|)"));
    m_delimiters_labels.push_back(_("None"));
    m_delimiters_labels.push_back(_("Other..."));

    m_textqualifier_labels.push_back(_("Quotation Marks (\")"));
    m_textqualifier_labels.push_back(_("Single Quote (')"));
    m_textqualifier_labels.push_back(_("None"));
    m_textqualifier_labels.push_back(_("Other..."));

    m_encoding = TextDoc::StandardEncoding;
    m_view = -1;  // -1 means the view will be be determined by the file type in TextDoc::open()
}

TextDoc::~TextDoc()
{
    if (m_textview)
    {
        m_textview->closeFile();
        delete m_textview;
        m_textview = NULL;
    }
}


bool TextDoc::initDoc(IFramePtr frame,
                      IDocumentSitePtr doc_site,
                      wxWindow* docsite_wnd,
                      wxWindow* panesite_wnd)
{
    // store ptrs
    m_frame = frame;
    m_doc_site = doc_site;

    // set the document's caption and icon
    wxString caption;
    if (!m_path.IsEmpty())
    {
        caption = m_path.AfterLast(wxT('/'));
    }
     else
    {
        caption = _("(Untitled)");
    }

    m_doc_site->setCaption(caption);
    m_doc_site->setBitmap(GETBMP(gf_table_16));

    // create document's window
    bool result = Create(docsite_wnd,
                         -1,
                         wxPoint(0,0),
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN);

    if (!result)
        return false;

    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);

    // create the settings sizers
    m_fixedlength_settings_sizer = createFixedLengthSettingsSizer();
    m_textdelimited_settings_sizer = createTextDelimitedSettingsSizer();
    m_main_settings_sizer = createMainSettingsSizer();
    
    // create and initialize the text view
    m_textview = new TextView(this,
                              -1,
                              wxDefaultPosition,
                              wxDefaultSize,
                              wxNO_BORDER |
                              wxNO_FULL_REPAINT_ON_RESIZE |
                              wxHSCROLL |
                              wxVSCROLL |
                              wxALWAYS_SHOW_SB);

    // create and initialize the gridview
    m_grid = new kcl::Grid;
    m_grid->setCursorVisible(false);
    m_grid->setCaptionFont(*wxNORMAL_FONT);
    m_grid->setOptionState(kcl::Grid::optColumnMove |
                           kcl::Grid::optHorzGridLines |
                           kcl::Grid::optEdit |
                           kcl::Grid::optSelect |
                           kcl::Grid::optActivateHyperlinks,
                           false);
    m_grid->setOptionState(kcl::Grid::optCaptionsEditable |
                           kcl::Grid::optSingleClickEdit, true);
    m_grid->setDefaultLineColor(wxColour(100,100,100));     // hard-coded to match the TextView
    m_grid->Create(this,
                   -1,
                   wxDefaultPosition,
                   wxDefaultSize,
                   0,
                   kcl::Grid::stateVisible);

    // initialize the active view
    if (m_view == TextDoc::FixedLengthView)
        initFixedLengthView();
     else if (m_view == TextDoc::TextDelimitedView)    
        initTextDelimitedView();
    
    // create the main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->Add(m_main_settings_sizer, 0, wxEXPAND);
    m_main_sizer->Add(m_textview, 1, wxEXPAND);
    m_main_sizer->Add(m_grid, 1, wxEXPAND);

    // show and hide the appropriate controls
    doTextModeLayout();
    
    // set the main sizer to be the window's sizer
    SetSizer(m_main_sizer);
    Layout();
    
    // create the statusbar items for this document
    IStatusBarItemPtr item;

    item = addStatusBarItem(wxT("textdoc_column_offset"));
    item->setWidth(120);
    
    item = addStatusBarItem(wxT("textdoc_field_count"));
    item->setWidth(120);

    // connect signals
    m_textview->sigColumnAdded.connect(this, &TextDoc::onTextViewColumnAdded);
    m_textview->sigColumnDeleted.connect(this, &TextDoc::onTextViewColumnDeleted);
    m_textview->sigColumnModified.connect(this, &TextDoc::onTextViewColumnModified);
    m_textview->sigCursorPositionChanged.connect(this, &TextDoc::onTextViewCursorPositionChanged);
    return true;
}

wxWindow* TextDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

wxString TextDoc::getDocumentLocation()
{
    if (m_source_url.Length() > 0)
        return m_source_url;
        
    return urlToFilename(m_path);
}

void TextDoc::setDocumentFocus()
{

}

void TextDoc::setSourceUrl(const wxString& source_url)
{
    m_source_url = source_url;
}

bool TextDoc::onSiteClosing(bool force)
{
    if (force)
    {
        return true;
    }

    if (m_dirty)
    {
        int result = appMessageBox(_("Would you like to save the changes you made to the configuration?"),
                                        APPLICATION_NAME,
                                        wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);

        // cancel the site closing
        if (result == wxCANCEL)
            return false;
        
        // close site and don't perform a save operation
        if (result == wxNO)
            return true;
    }

    if (m_dirty)
    {
        // save the set's metadata
        wxCommandEvent e;
        onSave(e);
    }
    
    return true;
}

void TextDoc::onSiteDeactivated()
{
    if (m_dirty)
    {
        // save the set's metadata
        wxCommandEvent e;
        onSave(e);
    }
}

void TextDoc::onSiteActivated()
{
    updateStatusBar();
}

bool TextDoc::updateColumnList()
{
    g_app->getMainFrame()->postEvent(new FrameworkEvent(FRAMEWORK_EVT_COLUMNLISTPANEL_UPDATE));
    return true;
}

tango::ISetPtr TextDoc::getTextSet()
{
    if (m_view == TextDoc::TextDelimitedView)
    {
        return m_textdelimited_set;
    }
     else if (m_view == TextDoc::FixedLengthView)
    {
        return m_fixedlength_set;
    }
    
    return xcm::null;
}

tango::IStructurePtr TextDoc::getStructure()
{
    if (m_view == TextDoc::TextDelimitedView)
    {
        tango::IDelimitedTextSetPtr s = m_textdelimited_set;
        if (s)
            return s->getStructure();
    }
     else if (m_view == TextDoc::FixedLengthView)
    {
        tango::IFixedLengthDefinitionPtr s = m_fixedlength_set;
        if (s)
            return s->getStructure();
    }
    
    return xcm::null;
}

wxString TextDoc::getPath()
{
    return m_path;
}


// IColumnListTarget interface

void TextDoc::getColumnListItems(std::vector<ColumnListItem>& list)
{
    list.clear();
    
    tango::ISetPtr set = getTextSet();
    if (set.isNull())
        return;

    tango::IStructurePtr structure;
    tango::IFixedLengthDefinitionPtr fset = set;
    tango::IDelimitedTextSetPtr tset = set;
    if (fset)
        structure = fset->getSourceStructure();
    if (tset)
        structure = tset->getSourceStructure();
        
    if (structure.isNull())
        return;
        
    int i, col_count = structure->getColumnCount();
    list.reserve(col_count);
    
    for (i = 0; i < col_count; i++)
    {
        tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);
        if (colinfo.isNull())
            continue;
        
        ColumnListItem item;
        item.text = makeProperIfNecessary(colinfo->getName());
        item.bitmap = GETBMP(gf_field_16);
        item.active = true;
        list.push_back(item);
    }
}

bool TextDoc::initFixedLengthView()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;
    
    // open the set
    m_textdelimited_set.clear();
    m_fixedlength_set = db->openSetEx(towstr(m_path),
                                      tango::formatFixedLengthText);

    // if we don't have a set, bail out
    if (m_fixedlength_set.isNull())
        return false;

    // if the set is not a fixed-length text set, bail out
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isNull())
        return false;


    // load the fixed-length text metadata from the set

    m_loading_definition = true;

    // open the file in the TextView control
    wxString fn;
    if (m_path.Left(5).CmpNoCase(wxT("file:")) == 0)
        fn = urlToFilename(m_path);
     else
        fn = getPhysPathFromMountPath(m_path);
        
    if (!m_textview->openFile(fn))
        return false;

    // get the fixed-length text metadata from the set
    size_t rowwidth = fset->getRowWidth();
    size_t skipchars = fset->getBeginningSkipCharacterCount();
    bool line_delimited = fset->isLineDelimited();
    
    // set the controls' values
    
    // this code should be before the SetRange() calls below
    // becase SetRange() was updating the row width to 1 (because
    // it was its default 0).  This in turn caused on...RowWidthSpun()
    // to be called and overwrote the file's real row width...sad.
    
    m_rowwidth_spinctrl->SetValue(rowwidth);
    m_skipchars_spinctrl->SetValue(skipchars);
    m_linedelimited_checkbox->SetValue(line_delimited);
    
    // set the min/max values for the spin controls
    xf_off_t filesize = m_textview->getModelFileSize();
    int max_rowwidth = (filesize > MAX_ROWWIDTH) ? MAX_ROWWIDTH
                                                 : (int)filesize;
    int max_skipchars = (filesize > MAX_SKIPCHARS) ? MAX_SKIPCHARS
                                                   : (int)(filesize-1);
    m_rowwidth_spinctrl->SetRange(1, max_rowwidth);
    m_skipchars_spinctrl->SetRange(0, max_skipchars);
    

    // set the TextView's settings
    m_textview->setRowWidth(rowwidth);
    m_textview->setSkipChars(skipchars);
    m_textview->setFileType(line_delimited ? TextViewModel::lineDelimited :
                                             TextViewModel::lineFixed);
    
    // delete all columns in the TextView
    m_textview->deleteAllColumns();
    
    // get the source structure from the fixed-length text set
    tango::IStructurePtr fstruct = fset->getSourceStructure();
    if (fstruct.isNull())
    {
        m_loading_definition = false;
        return false;
    }
    
    // add columns to the TextView
    int i, field_count = fstruct->getColumnCount();
    for (i = 0; i < field_count; ++i)
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = fstruct->getColumnInfoByIdx(i);
        
        m_textview->addColumn(colinfo->getOffset(),
                              colinfo->getEncoding(),
                              colinfo->getName());
    }
    
    m_loading_definition = false;


    // no fields were added from the fixed-length text set's metadata
    if (m_textview->getColumnCount() == 0)
    {
        // add a default column to the TextView control (NOTE: this will
        // fire a signal to the TextDoc, which will, in turn, add a
        // column to fixed-length text set)
        m_textview->addColumn(0);
    }

    // refresh the view
    m_textview->refresh();
    
    m_fixedlength_inited = true;
    return true;
}

inline void setDelimitersComboBoxSelection(wxComboBox* combobox,
                                           const wxString& delimiters)
{
    if (delimiters == wxT(","))
        combobox->SetSelection(DelimiterIdx_Comma);
     else if (delimiters == wxT("\t"))
        combobox->SetSelection(DelimiterIdx_Tab);
     else if (delimiters == wxT(";"))
        combobox->SetSelection(DelimiterIdx_Semicolon);
     else if (delimiters == wxT(" "))
        combobox->SetSelection(DelimiterIdx_Space);
     else if (delimiters == wxT("|"))
        combobox->SetSelection(DelimiterIdx_Pipe);
     else if (delimiters.IsEmpty())
        combobox->SetSelection(DelimiterIdx_None);
     else
    {
        combobox->SetValue(delimiters);
        combobox->SetInsertionPointEnd();
    }
}

inline void setTextQualifierComboBoxSelection(wxComboBox* combobox,
                                              const wxString& text_qualifier)
{
    if (text_qualifier == wxT("\""))
        combobox->SetSelection(TextQualifierIdx_DoubleQuote);
     else if (text_qualifier == wxT("'"))
        combobox->SetSelection(TextQualifierIdx_SingleQuote);
     else if (text_qualifier.IsEmpty())
        combobox->SetSelection(TextQualifierIdx_None);
     else
    {
        combobox->SetValue(text_qualifier);
        combobox->SetInsertionPointEnd();
    }
}



bool TextDoc::open(const wxString& filename)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    // get the format of the file from the database
    tango::IFileInfoPtr file_info = db->getFileInfo(towstr(filename));
    if (file_info.isNull())
        return false;

    m_textdelimited_set.clear();
    m_fixedlength_set.clear();
    m_path = filename;
    
    if (m_view == -1)
    {
        if (file_info->getFormat() == tango::formatDelimitedText)
            m_view = TextDoc::TextDelimitedView;
             else
            m_view = TextDoc::FixedLengthView;
    }

    
    if (m_frame.isOk())
    {
        bool res;
        
        if (m_view == TextDoc::FixedLengthView)
            res = initFixedLengthView();
         else if (m_view == TextDoc::TextDelimitedView)    
            res = initTextDelimitedView();
            
        return res;
    }
     else
    {
        if (m_view == TextDoc::FixedLengthView)
        {
            m_fixedlength_set = db->openSetEx(towstr(m_path),
                                              tango::formatFixedLengthText);
            return m_fixedlength_set.isOk();
        }
         else if (m_view == TextDoc::TextDelimitedView)
        {
            m_textdelimited_set = db->openSetEx(towstr(m_path),
                                                tango::formatDelimitedText);
            return m_textdelimited_set.isOk();
        }
    }
    
    return false;
}

void TextDoc::close()
{
    // get rid of all references that are keeping the file open
    m_textdelimited_set.clear();
    m_fixedlength_set.clear();
    m_textdelimited_iter.clear();
    m_grid_model.clear();
    m_grid->setModel(xcm::null);
}


bool TextDoc::initTextDelimitedView()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;
    
    // open the set
    m_fixedlength_set.clear();
    m_textdelimited_set = db->openSetEx(towstr(m_path),
                                        tango::formatDelimitedText);

    // if we don't have a set, bail out
    if (m_textdelimited_set.isNull())
        return false;

    // if the set is not a text-delimited set, bail out
    tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
    if (tset.isNull())
        return false;


    // load the text-delimited metadata from the set

    m_loading_definition = true;

    // get the text-delimited metadata from the set
    bool firstrowfieldnames = tset->isFirstRowColumnNames();
    wxString delimiters = tset->getDelimiters();
    wxString textqualifier = tset->getTextQualifier();
    
    // set the field delimiter dropdown
    setDelimitersComboBoxSelection(m_delimiters_combobox,
                                   delimiters);

    // set the text qualifier dropdown
    setTextQualifierComboBoxSelection(m_textqualifier_combobox,
                                      textqualifier);

    // set the first row field names checkbox
    m_firstrowfieldnames_checkbox->SetValue(firstrowfieldnames);

    m_loading_definition = false;

    // store these for use in checking to see if the selection
    // or values have changed
    m_last_textqualifier_sel = m_textqualifier_combobox->GetSelection();
    m_last_delimiters_sel = m_delimiters_combobox->GetSelection();
    m_last_textqualifier = textqualifier;
    m_last_delimiters = delimiters;
    
    // set up the grid for the text-delimited set    
    m_textdelimited_iter = tset->createSourceIterator(NULL);
    m_grid_model = new TangoGridModel;
    m_grid_model->setIterator(m_textdelimited_iter);
    
    m_grid->setModel(m_grid_model);
    m_grid->createDefaultView();
    
    wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
    m_grid->SetFont(font);
    m_grid->setRowLabelSize(0);
    m_grid->setHeaderSize(20);      // hard-coded to match the TextView
    m_grid->setRowHeight(16);       // hard-coded to match the TextView
    m_grid->refresh(kcl::Grid::refreshAll);
    m_grid->autoColumnResize(-1);
    
    m_textdelimited_inited = true;
    return true;
}

wxBoxSizer* TextDoc::createMainSettingsSizer()
{
    wxStaticText* static_filetype = new wxStaticText(this,
                                                     -1,
                                                     _("File Type:"));
    
    size_t i;
                                               
    m_filetype_choice = new wxChoice(this,
                                     ID_FiletypeChoice,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     0,
                                     NULL);
    for (i = 0; i < m_filetype_labels.size(); ++i)
        m_filetype_choice->Append(m_filetype_labels[i]);
    
    
    wxStaticText* static_encoding = new wxStaticText(this,
                                                     -1,
                                                     _("Encoding:"));

    m_encoding_choice = new wxChoice(this,
                                 ID_EncodingChoice,
                                 wxDefaultPosition,
                                 wxDefaultSize,
                                 0,
                                 NULL);
    for (i = 0; i < m_encoding_labels.size(); ++i)
        m_encoding_choice->Append(m_encoding_labels[i]);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->AddSpacer(5);
    sizer->Add(static_filetype, 0, wxALIGN_CENTER);
    sizer->AddSpacer(5);
    sizer->Add(m_filetype_choice, 0, wxALIGN_CENTER);
    sizer->AddSpacer(10);
    sizer->Add(static_encoding, 0, wxALIGN_CENTER);
    sizer->AddSpacer(5);
    sizer->Add(m_encoding_choice, 0, wxALIGN_CENTER);
    sizer->AddStretchSpacer(1);
    sizer->Add(m_fixedlength_settings_sizer, 0, wxEXPAND);
    sizer->Add(m_textdelimited_settings_sizer, 0, wxEXPAND);
    sizer->AddSpacer(5);
    
    wxMouseState state = ::wxGetMouseState();
    
    if (state.ControlDown())
    {
        sizer->Add(new wxBitmapButton(this, ID_OptionsMenu, GETBMP(gf_gear_16)), 0);
        sizer->AddSpacer(5);
    }
    
    sizer->SetMinSize(100, 31);
    return sizer;
}

wxBoxSizer* TextDoc::createFixedLengthSettingsSizer()
{
    wxStaticText* static_skipchars = new wxStaticText(this,
                                                      -1,
                                                      _("Start Offset:"));

    wxStaticText* static_rowwidth = new wxStaticText(this,
                                                     -1,
                                                     _("Row Width:"));

    m_skipchars_spinctrl = new wxSpinCtrl(this,
                                          ID_SkipCharsSpinCtrl,
                                          wxT(""),
                                          wxDefaultPosition,
                                          wxSize(90,21),
                                          wxSP_ARROW_KEYS,
                                          0,
                                          MAX_SKIPCHARS);

    m_rowwidth_spinctrl = new wxSpinCtrl(this,
                                         ID_RowWidthSpinCtrl,
                                         wxT(""),
                                         wxDefaultPosition,
                                         wxSize(60,21),
                                         wxSP_ARROW_KEYS,
                                         1,
                                         MAX_ROWWIDTH);

    m_linedelimited_checkbox = new wxCheckBox(this,
                                              ID_LineDelimitedCheckBox,
                                              _("Rows end with CR/LF"));

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(static_skipchars, 0, wxALIGN_CENTER);
    sizer->AddSpacer(4);
    sizer->Add(m_skipchars_spinctrl, 0, wxALIGN_CENTER);
    sizer->AddSpacer(8);
    sizer->Add(static_rowwidth, 0, wxALIGN_CENTER);
    sizer->AddSpacer(4);
    sizer->Add(m_rowwidth_spinctrl, 0, wxALIGN_CENTER);
    sizer->AddSpacer(10);
    sizer->Add(m_linedelimited_checkbox, 0, wxALIGN_CENTER);
    sizer->SetMinSize(100, 31);
    return sizer;
}

wxBoxSizer* TextDoc::createTextDelimitedSettingsSizer()
{
    size_t i;

    wxStaticText* static_delimiters = new wxStaticText(this, -1,
                                                      _("Field Delimiters:"));

    wxStaticText* static_text_qualifier = new wxStaticText(this, -1,
                                                      _("Text Qualifier:"));
                                                           
    m_delimiters_combobox = new wxComboBox(this,
                                           ID_DelimitersComboBox,
                                           wxEmptyString,
                                           wxDefaultPosition,
                                           wxDefaultSize);
                                           
    for (i = 0; i < m_delimiters_labels.size(); ++i)
        m_delimiters_combobox->Append(m_delimiters_labels[i]);


    
    m_textqualifier_combobox = new wxComboBox(this,
                                              ID_TextQualifierComboBox,
                                              wxEmptyString,
                                              wxDefaultPosition,
                                              wxDefaultSize);
    for (i = 0; i < m_textqualifier_labels.size(); ++i)
        m_textqualifier_combobox->Append(m_textqualifier_labels[i]);
    
    
    m_firstrowfieldnames_checkbox = new wxCheckBox(this,
                                          ID_FirstRowFieldNamesCheckBox,
                                          _("First row contains field names"));


    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(static_delimiters, 0, wxALIGN_CENTER);
    sizer->AddSpacer(5);
    sizer->Add(m_delimiters_combobox, 0, wxALIGN_CENTER);
    sizer->AddSpacer(10);
    sizer->Add(static_text_qualifier, 0, wxALIGN_CENTER);
    sizer->AddSpacer(5);
    sizer->Add(m_textqualifier_combobox, 0, wxALIGN_CENTER);
    sizer->AddSpacer(10);
    sizer->Add(m_firstrowfieldnames_checkbox, 0, wxALIGN_CENTER);
    sizer->SetMinSize(100, 31);
    return sizer;
}

void TextDoc::doTextModeLayout()
{
    Freeze();
    
    m_main_settings_sizer->Show(m_fixedlength_settings_sizer, false, true);
    m_main_settings_sizer->Show(m_textdelimited_settings_sizer, false, true);
    m_main_sizer->Show(m_textview, false, true);
    m_main_sizer->Show(m_grid, false, true);
    
    if (m_view == TextDoc::TextDelimitedView)
    {
        m_filetype_choice->SetSelection(FileType_TextDelimited);
        m_main_settings_sizer->Show(m_textdelimited_settings_sizer, true, true);
        m_main_sizer->Show(m_grid, true, true);
    }
 
    if (m_view == TextDoc::FixedLengthView)
    {
        m_filetype_choice->SetSelection(FileType_FixedLength);
        m_main_settings_sizer->Show(m_fixedlength_settings_sizer, true, true);
        m_main_sizer->Show(m_textview, true, true);
    }
    
    if (m_encoding == TextDoc::StandardEncoding)
        m_encoding_choice->SetSelection(0);
         else
        m_encoding_choice->SetSelection(1);

    Layout();
    
    Thaw();
}

void TextDoc::refreshGrid()
{
    // if the set is not a text-delimited set, bail out
    tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
    if (tset.isNull())
        return;

    m_textdelimited_iter = tset->createSourceIterator(NULL);
    m_grid_model->setIterator(m_textdelimited_iter);
    
    m_grid->Freeze();
    m_grid->refreshModel();
    m_grid->createDefaultView();
    m_grid->refresh(kcl::Grid::refreshAll);
    m_grid->autoColumnResize(-1);
    m_grid->Thaw();
}

void TextDoc::updateStatusBar()
{
    // if the textview hasn't been created yet, bail out
    if (!m_textview)
        return;
        
    // if the grid hasn't been created yet, bail out
    if (!m_grid)
        return;

    if (m_view == TextDoc::TextDelimitedView)
    {
    
    }
     else if (m_view == TextDoc::FixedLengthView)
    {
        if (m_fixedlength_set.isNull())
            return;
        
        // field count
        int col = m_textview->getCursorOffset();
        int field_count = 0;
        
        tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
        if (fset)
            field_count = fset->getSourceStructure()->getColumnCount();
        
        wxString offset_str = wxString::Format(_("Offset: %d"), col);
        wxString field_count_str = wxString::Format(_("Field Count: %d"), field_count);
        
        IStatusBarItemPtr item;
        item = m_frame->getStatusBar()->getItem(wxT("textdoc_column_offset"));
        if (item.isOk())
        {
            if (col != -1)
                item->setValue(offset_str);
             else
                item->setValue(wxEmptyString);
        }
            
        item = m_frame->getStatusBar()->getItem(wxT("textdoc_field_count"));
        if (item.isOk())
            item->setValue(field_count_str);
    }

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}

int TextDoc::getRowWidth()
{
    if (m_rowwidth_spinctrl)
        return m_rowwidth_spinctrl->GetValue();
    
    return 0;
}

int TextDoc::getBeginningSkipCharacters()
{
    if (m_skipchars_spinctrl)
        return m_skipchars_spinctrl->GetValue();
    
    return 0;
}

bool TextDoc::isLineDelimited()
{
    if (m_linedelimited_checkbox)
        return m_linedelimited_checkbox->GetValue();
    
    return false;
}

wxString TextDoc::getFieldDelimiters()
{
    if (!m_delimiters_combobox)
        return wxEmptyString;
        
    int delimiters_selection = m_delimiters_combobox->GetSelection();
    wxString delimiters;
    switch (delimiters_selection)
    {
        case DelimiterIdx_Comma:
            delimiters = wxT(",");
            break;
            
        case DelimiterIdx_Tab:
            delimiters = wxT("\t");
            break;
            
        case DelimiterIdx_Semicolon:
            delimiters = wxT(";");
            break;
            
        case DelimiterIdx_Space:
            delimiters = wxT(" ");
            break;
            
        case DelimiterIdx_Pipe:
            delimiters = wxT("|");
            break;
            
        case DelimiterIdx_None:
            delimiters = wxEmptyString;
            break;
            
        case DelimiterIdx_Other:
            break;
            
        default:
            delimiters = m_delimiters_combobox->GetValue();
            break;
    }
    
    return delimiters;
}

wxString TextDoc::getTextQualifier()
{
    if (!m_textqualifier_combobox)
        return wxEmptyString;
        
    int textqualifier_selection = m_textqualifier_combobox->GetSelection();
    wxString text_qualifier;
    
    switch (textqualifier_selection)
    {
        case TextQualifierIdx_DoubleQuote:
            text_qualifier = wxT("\"");
            break;
            
        case TextQualifierIdx_SingleQuote:
            text_qualifier = wxT("'");
            break;
            
        case TextQualifierIdx_None:
            text_qualifier = wxEmptyString;
            break;
            
        case TextQualifierIdx_Other:
            break;
            
        default:
            text_qualifier = m_textqualifier_combobox->GetValue();
            break;
    }
    
    return text_qualifier;
}

bool TextDoc::isFirstRowFieldNames()
{
    if (m_firstrowfieldnames_checkbox)
        return m_firstrowfieldnames_checkbox->GetValue();

    return false;
}

void TextDoc::onOptionsMenu(wxCommandEvent& evt)
{
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSave);
    dlg.setCaption(_("Save As"));

    if (dlg.ShowModal() == wxID_OK)
    {
        saveLayoutTemplate(dlg.getPath());
        g_app->getAppController()->refreshDbDoc();
        
        g_app->getAppController()->openScript(dlg.getPath());
    }
}

void TextDoc::onTextViewColumnAdded(TextViewColumn col)
{
    // if we're loading a text definition, we don't want to process
    // the signals that the TextView is firing to us
    if (m_loading_definition)
        return;

    // get the index of the column from the offset
    size_t idx = m_textview->getColumnIdxFromOffset(col.offset);
     
    // insert the column into the fixed-length text set
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isOk())
    {
        tango::IStructurePtr ss = fset->getSourceStructure();
        tango::IColumnInfoPtr colinfo = ss->insertColumn(idx);
        colinfo->setOffset(col.offset);
        colinfo->setType(tango::typeCharacter);
        colinfo->setWidth(col.width);
        colinfo->setName(towstr(col.name));
        if (m_encoding == EbcdicEncoding)
            colinfo->setEncoding(tango::encodingEBCDIC);
        fset->modifySourceStructure(ss, NULL);
        
        colinfo = ss->getColumnInfoByIdx(idx-1);
        
        tango::IStructurePtr ds = fset->getDestinationStructure();
        int insert_idx = getBestDestinationInsertPosition(colinfo, ds);
        
        colinfo = ds->insertColumn(insert_idx);
        colinfo->setName(towstr(col.name));
        colinfo->setType(tango::typeCharacter);
        colinfo->setWidth(col.width);
        colinfo->setScale(0);
        colinfo->setExpression(towstr(col.name));
        fset->modifyDestinationStructure(ds, NULL);
    }

    // repopulate the TransformationDoc from the destination structure
    ITransformationDocPtr transdoc;
    transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
    if (transdoc)
        transdoc->initFromSet(m_fixedlength_set);
    
    // add the column to the TableDoc's view
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        // update the TableDoc's base set
        tabledoc->open(m_path);
        
        ITableDocViewPtr tabledocview = tabledoc->getActiveView();
        if (tabledocview)
        {
            int viewidx = tabledocview->getColumnIdx(towstr(col.name));
            
            // only add the column to our view if it doesn't already exist
            if (viewidx == -1)
            {
                // add the column to the TableDoc's view
                ITableDocViewColPtr viewcol = tabledocview->createColumn(idx);
                viewcol->setName(towstr(col.name));
                viewcol->setSize(80);
                
                // refresh the TableDoc's view
                tabledoc->refreshActiveView();
            }
        }
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextViewColumnDeleted(TextViewColumn col)
{
    // if we're loading a text definition, we don't want to process
    // the signals that the TextView is firing to us
    if (m_loading_definition)
        return;

    // get the index of the column from the offset
    size_t idx = m_textview->getColumnIdxFromOffset(col.offset);
    
    // delete the column into the fixed-length text set
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isOk())
    {
        tango::IStructurePtr s = fset->getSourceStructure();
        tango::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(idx);
        s->deleteColumn(colinfo->getName());
        fset->modifySourceStructure(s, NULL);
        
        // now, lookup and delete any columns in the destination
        // structure that have a corresponding expression
        s = fset->getDestinationStructure();
        int i, col_count = s->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            colinfo = s->getColumnInfoByIdx(i);
            
            if (col.name == colinfo->getExpression())
                s->deleteColumn(colinfo->getName());
        }
        fset->modifyDestinationStructure(s, NULL);
    }

    // repopulate the TransformationDoc from the destination structure
    ITransformationDocPtr transdoc;
    transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
    if (transdoc)
        transdoc->initFromSet(m_fixedlength_set);

    // delete the column from the TableDoc's view
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        ITableDocViewPtr tabledocview = tabledoc->getActiveView();
        if (tabledocview)
        {
            int viewidx = tabledocview->getColumnIdx(towstr(col.name));
            
            // only delete the column from our view if it exists in the view
            if (viewidx != -1)
            {
                tabledocview->deleteColumn(viewidx);
            }
        }
        
        // update the TableDoc's base set and refresh its view
        tabledoc->open(m_path);
        tabledoc->refreshActiveView();
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextViewColumnModified(TextViewColumn col,
                                       TextViewColumn new_settings)
{
    // if we're loading a text definition, we don't want to process
    // the signals that the TextView is firing to us
    if (m_loading_definition)
        return;

    // get the index of the column from the offset
    size_t idx = m_textview->getColumnIdxFromOffset(col.offset);
    
    // modify the column in the fixed-length text set
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isOk())
    {
        tango::IStructurePtr s = fset->getSourceStructure();
        tango::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(idx);
        tango::IColumnInfoPtr modinfo = s->modifyColumn(colinfo->getName());
        modinfo->setOffset(new_settings.offset);
        modinfo->setWidth(new_settings.width);
        modinfo->setEncoding(new_settings.encoding);
        modinfo->setName(towstr(new_settings.name));
        fset->modifySourceStructure(s, NULL);
        
        std::wstring src_colname = colinfo->getName();
        int src_width = colinfo->getWidth();
        
        // now, lookup and modify any columns in the destination
        // structure that have a corresponding expression
        s = fset->getDestinationStructure();
        int i, col_count = s->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            colinfo = s->getColumnInfoByIdx(i);
            
            if (colinfo->getExpression() == src_colname)
            {
                modinfo = s->modifyColumn(colinfo->getName());
                
                if (!new_settings.name.IsEmpty())
                {
                    modinfo->setExpression(towstr(new_settings.name));
                    
                    if (colinfo->getName() == src_colname)
                        modinfo->setName(towstr(new_settings.name));
                }
                
                if (new_settings.width != -1)
                {
                    if (colinfo->getWidth() == src_width)
                        modinfo->setWidth(new_settings.width);
                }
            }
        }
        fset->modifyDestinationStructure(s, NULL);
    }

    // repopulate the TransformationDoc from the destination structure
    ITransformationDocPtr transdoc;
    transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
    if (transdoc)
        transdoc->initFromSet(m_fixedlength_set);

    // update the TableDoc
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        // if the column was renamed, update the TableDoc's view
        if (!new_settings.name.IsEmpty())
        {
            ITableDocViewPtr tabledocview = tabledoc->getActiveView();
            if (tabledocview)
            {
                int viewidx = tabledocview->getColumnIdx(towstr(col.name));
                
                // only modify the column from our view if it exists in the view
                if (viewidx != -1)
                {
                    ITableDocViewColPtr viewcol = tabledocview->getColumn(viewidx);
                    viewcol->setName(towstr(new_settings.name));
                }
            }
        }

        // update the TableDoc's base set and refresh its view
        tabledoc->open(m_path);
        tabledoc->refreshActiveView();
    }
    
    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextViewCursorPositionChanged(int new_cursor_offset,
                                              int new_cursor_row)
{
    updateStatusBar();
}

void TextDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void TextDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

void TextDoc::onUpdateUI(wxUpdateUIEvent& evt)
{
    // disable by default
    evt.Enable(false);
    return;
}



void TextDoc::onSize(wxSizeEvent& evt)
{
    Layout();
}

void TextDoc::onSave(wxCommandEvent& evt)
{
    if (m_view == TextDoc::FixedLengthView)
    {
        tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
        if (fset)
        {
            fset->saveConfiguration();
        }
    }
     else if (m_view == TextDoc::TextDelimitedView)
    {
        tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
        if (tset)
        {
            tset->saveConfiguration();
        }
    }

    // save the view information in the OFS
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        ITableDocModelPtr model = tabledoc->getModel();
        if (model)
            model->writeObject(tabledoc->getActiveView());
    }
    
    m_dirty = false;
}

void TextDoc::onToggleView(wxCommandEvent& evt)
{
    updateStatusBar();

    // pass the event through to other event handlers
    evt.Skip();
}

void TextDoc::onFileTypeChanged(wxCommandEvent& evt)
{
    int sel = evt.GetSelection();

    // we've selected the same file type that we're already on
    if (sel == FileType_TextDelimited && m_view == TextDoc::TextDelimitedView)
        return;
        
    // we've selected the same file type that we're already on
    if (sel == FileType_FixedLength && m_view == TextDoc::FixedLengthView)
        return;
        
    // this is a pretty substantial operation the user is about to perform,
    // ask them if they are sure they want continue
    if (showOverwriteTransformationChangesDialog() != wxYES)
    {
        if (m_view == TextDoc::TextDelimitedView)
            m_filetype_choice->SetSelection(FileType_TextDelimited);
        
        if (m_view == TextDoc::FixedLengthView)
            m_filetype_choice->SetSelection(FileType_FixedLength);
        
        return;
    }
    
    if (sel == FileType_TextDelimited)
    {
        m_view = TextDoc::TextDelimitedView;
        
        //if (!m_textdelimited_inited)
            initTextDelimitedView();
    }
    
    if (sel == FileType_FixedLength)
    {
        m_view = TextDoc::FixedLengthView;
        
       //if (!m_fixedlength_inited)
            initFixedLengthView();
    }

    m_dirty = true;
    doTextModeLayout();
    
    // reset the TransformationDoc's grid and the TableDoc's grid
    resetTransformationDocAndTableDoc();
    updateStatusBar();
}


void TextDoc::onEncodingChanged(wxCommandEvent& evt)
{
    int sel = evt.GetSelection();
    int tango_encoding;
    
    if (sel == 0)
    {
        m_encoding = TextDoc::StandardEncoding;
        tango_encoding = tango::encodingASCII;
        m_textview->setCharEncoding(tango_encoding);
    }
     else
    {
        m_encoding = TextDoc::EbcdicEncoding;
        tango_encoding = tango::encodingEBCDIC;
        m_textview->setCharEncoding(tango_encoding);
    }
    
    
    if (m_textview->getColumnCount() == 1)
    {
        m_textview->modifyColumn(0, -1, -1, tango_encoding, wxT(""));
    }

    
}


void TextDoc::onFixedLengthSkipCharTextEnter(wxCommandEvent& evt)
{
    if (!m_skipchars_spinctrl)
        return;

    int val = m_skipchars_spinctrl->GetValue();
    xf_off_t filesize = m_textview->getModelFileSize();
    
    if (val < 0)
    {
        val = 0;
        m_skipchars_spinctrl->SetValue(val);
    }
    
    if (val > MAX_SKIPCHARS)
        val = MAX_SKIPCHARS;
        
    if ((xf_off_t)val >= filesize)
        val = (int)(filesize-1);
        
    m_textview->setSkipChars(val);
    m_textview->refresh();

    // update the fixed-length text set's metadata
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isOk())
        fset->setBeginningSkipCharacterCount(val);
    
    // update the TableDoc's base set
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        tabledoc->open(m_path);
    }
    
    m_dirty = true;
}

void TextDoc::onFixedLengthRowWidthTextEnter(wxCommandEvent& evt)
{
    if (!m_rowwidth_spinctrl)
        return;

    int val = m_rowwidth_spinctrl->GetValue();
    xf_off_t filesize = m_textview->getModelFileSize();

    if (val < 0)
    {
        val = 0;
        m_rowwidth_spinctrl->SetValue(val);
    }
    
    if (val > MAX_ROWWIDTH)
        val = MAX_ROWWIDTH;
        
    if ((xf_off_t)val > filesize)
        val = (int)(filesize);
        
    m_textview->setRowWidth(val);
    m_textview->refresh();
    
    // update the fixed-length text set's metadata
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isOk())
        fset->setRowWidth(val);
    
    // update the TableDoc's base set
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        tabledoc->open(m_path);
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onFixedLengthSkipCharSpun(wxSpinEvent& evt)
{
    if (!m_skipchars_spinctrl)
        return;
        
    int val = m_skipchars_spinctrl->GetValue();
    m_textview->setSkipChars(val);
    m_textview->refresh();

    // update the fixed-length text set's metadata
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isOk())
        fset->setBeginningSkipCharacterCount(val);
    
    // update the TableDoc's base set
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        tabledoc->open(m_path);
    }

    m_dirty = true;
}

void TextDoc::onFixedLengthRowWidthSpun(wxSpinEvent& evt)
{
    if (!m_rowwidth_spinctrl)
        return;
        
    int val = m_rowwidth_spinctrl->GetValue();
    m_textview->setRowWidth(val);
    m_textview->refresh();

    // update the fixed-length text set's metadata
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isOk())
        fset->setRowWidth(val);
    
    // update the TableDoc's base set
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        tabledoc->open(m_path);
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onFixedLengthLineDelimitedChecked(wxCommandEvent& evt)
{
    bool val = evt.IsChecked();
    if (val)
        m_textview->setFileType(TextViewModel::lineDelimited);
     else
        m_textview->setFileType(TextViewModel::lineFixed);

    m_textview->refresh();

    // update the fixed-length text set's metadata
    tango::IFixedLengthDefinitionPtr fset = m_fixedlength_set;
    if (fset.isOk())
        fset->setLineDelimited(val);
    
    // update the TableDoc's base set
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        tabledoc->open(m_path);
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextDelimitedFieldDelimiterTextEnter(wxCommandEvent& evt)
{
    wxString s = evt.GetString();
    
    if (s.IsNumber())
    {
        // users can optionall key in an character code as a decimal value
        wxChar ch = kl::wtoi(towstr(s));
        s = ch;
    }

    // we didn't change delimiters, so we're done
    if (s == m_last_delimiters)
        return;

    // if the user has edited the destination set structure in the
    // TransformationDoc, ask them if they are sure they want continue
    if (!isDestinationSameAsSourceStructure())
    {
        if (showOverwriteTransformationChangesDialog() != wxYES)
        {
            setDelimitersComboBoxSelection(m_delimiters_combobox,
                                           m_last_delimiters);
            return;
        }
    }
    
    // if the user typed in one of the default delimiters,
    // use that combobox entry instead
    setDelimitersComboBoxSelection(m_delimiters_combobox, s);
    m_last_delimiters = s;
    
    // set the parameters for a text-delimited set
    tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
    if (tset.isOk())
    {
        AppBusyCursor busy_cursor;
        tset->setDelimiters(towstr(getFieldDelimiters()), true);
        refreshGrid();

        // reset the TransformationDoc's grid and the TableDoc's grid
        resetTransformationDocAndTableDoc();
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextDelimitedTextQualifierTextEnter(wxCommandEvent& evt)
{
    wxString s = evt.GetString();
    
    // we didn't change the text qualifier, so we're done
    if (s == m_last_textqualifier)
        return;

    // if the user has edited the destination set structure in the
    // TransformationDoc, ask them if they are sure they want continue
    if (!isDestinationSameAsSourceStructure())
    {
        if (showOverwriteTransformationChangesDialog() != wxYES)
        {
            setTextQualifierComboBoxSelection(m_textqualifier_combobox,
                                              m_last_textqualifier);
            return;
        }
    }

    // if the user typed in one of the default text qualifiers,
    // use that combobox entry instead
    setTextQualifierComboBoxSelection(m_textqualifier_combobox, s);
    m_last_textqualifier = s;
    
    // set the parameters for a text-delimited set
    tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
    if (tset.isOk())
    {
        AppBusyCursor busy_cursor;
        tset->setTextQualifier(towstr(getTextQualifier()), true);
        refreshGrid();

        // reset the TransformationDoc's grid and the TableDoc's grid
        resetTransformationDocAndTableDoc();
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextDelimitedTextQualifierTextChanged(wxCommandEvent& evt)
{
    wxString s = evt.GetString();
    
    // don't chop off the text if we selected one of the combobox items
    size_t i;
    for (i = 0; i < m_textqualifier_labels.size(); ++i)
    {
        if (s == m_textqualifier_labels[i])
            return;
    }

    // text-delimited sets can only handle one text qualifier
    if (s.Length() > 1)
    {
        s = s.Mid(0,1);
        m_textqualifier_combobox->SetValue(s);
        m_textqualifier_combobox->SetInsertionPointEnd();
    }

    m_dirty = true;
}

void TextDoc::onTextDelimitedFieldDelimiterCombo(wxCommandEvent& evt)
{
    int sel = evt.GetSelection();
    
    // if "Other..." was selected, make sure the combobox text is empty
    if (sel == DelimiterIdx_Other)
    {
        m_delimiters_combobox->SetValue(wxEmptyString);
        m_delimiters_combobox->SetInsertionPointEnd();
        m_last_delimiters_sel = sel;
        return;
    }

    // we didn't change delimiters, so we're done
    if (sel == m_last_delimiters_sel)
        return;
        
    // if the user has edited the destination set structure in the
    // TransformationDoc, ask them if they are sure they want continue
    if (!isDestinationSameAsSourceStructure())
    {
        if (showOverwriteTransformationChangesDialog() != wxYES)
        {
            setDelimitersComboBoxSelection(m_delimiters_combobox,
                                           m_last_delimiters);
            return;
        }
    }
    
    m_last_delimiters_sel = sel;
    m_last_delimiters = getFieldDelimiters();
    
    // set the parameters for a text-delimited set
    tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
    if (tset.isOk())
    {
        AppBusyCursor busy_cursor;
        tset->setDelimiters(towstr(getFieldDelimiters()), true);
        refreshGrid();
        
        // when changing delimiters, the text-delimited set has to determine
        // the columns, and in so doing, re-senses if the first row is column
        // names or not, so we need to update this value appropriately
        m_firstrowfieldnames_checkbox->SetValue(tset->isFirstRowColumnNames());

        // reset the TransformationDoc's grid and the TableDoc's grid
        resetTransformationDocAndTableDoc();
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextDelimitedTextQualifierCombo(wxCommandEvent& evt)
{
    int sel = evt.GetSelection();

    // if "Other..." was selected, make sure the combobox text is empty
    if (sel == TextQualifierIdx_Other)
    {
        m_textqualifier_combobox->SetValue(wxEmptyString);
        m_textqualifier_combobox->SetInsertionPointEnd();
        m_last_textqualifier_sel = sel;
        return;
    }

    // we didn't change the text qualifier, so we're done
    if (sel == m_last_textqualifier_sel)
        return;

    // if the user has edited the destination set structure in the
    // TransformationDoc, ask them if they are sure they want continue
    if (!isDestinationSameAsSourceStructure())
    {
        if (showOverwriteTransformationChangesDialog() != wxYES)
        {
            setTextQualifierComboBoxSelection(m_textqualifier_combobox,
                                              m_last_textqualifier);
            return;
        }
    }

    m_last_textqualifier_sel = sel;
    m_last_textqualifier = getTextQualifier();

    // set the parameters for a text-delimited set
    tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
    if (tset.isOk())
    {
        AppBusyCursor busy_cursor;
        tset->setTextQualifier(towstr(getTextQualifier()), true);
        refreshGrid();

        // reset the TransformationDoc's grid and the TableDoc's grid
        resetTransformationDocAndTableDoc();
    }

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextDelimitedFirstRowFieldNamesChecked(wxCommandEvent& evt)
{
    std::vector< std::pair<wxString, wxString> > name_changes;
    std::pair<wxString, wxString> name_pair;
    
    // set the parameters for a text-delimited set
    tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
    if (tset.isOk())
    {
        tango::IStructurePtr s = tset->getSourceStructure();
        tango::IColumnInfoPtr colinfo;
        
        // get the old field names
        int i, col_count = s->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            colinfo = s->getColumnInfoByIdx(i);
            name_pair.first = colinfo->getName();
            name_changes.push_back(name_pair);
        }
        
        // set the first row field names in the text-delimited set
        // and refresh the grid
        tset->setFirstRowColumnNames(isFirstRowFieldNames());
        refreshGrid();
        
        // now, get the new field names
        s = tset->getSourceStructure();
        for (i = 0; i < col_count; ++i)
        {
            colinfo = s->getColumnInfoByIdx(i);
            name_changes[i].second = colinfo->getName();
        }
    }

    // this call will update both the TransformationDoc and the TableDoc
    doBulkFieldRename(name_changes);
    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

void TextDoc::onTextDelimitedCaptionBeginEdit(kcl::GridEvent& evt)
{

}

void TextDoc::onTextDelimitedCaptionEndEdit(kcl::GridEvent& evt)
{
    int col = evt.GetColumn();
    wxString new_name = evt.GetString();
    new_name.Trim(false);
    new_name.Trim(true);
    
    if (new_name.Length() == 0)
    {
        evt.Veto();
        return;
    }

    // we cannot set the name to an existing column name
    kcl::IModelPtr model = m_grid->getModel();
    int model_idx = model->getColumnIndex(new_name);
    if (model_idx != -1)
    {
        evt.Veto();
        return;
    }

    // get the model and view column indexes
    int mod_idx = m_grid->getColumnModelIdx(col);
    int view_idx = m_grid->getColumnViewIdx(mod_idx);
    if (mod_idx == -1)
    {
        evt.Veto();
        return;
    }

    // make sure we're dealing with a text-delimited set
    tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
    if (tset.isNull())
        return;

    // get the column's info
    tango::IStructurePtr src_struct = tset->getSourceStructure();
    if (src_struct.isNull())
        return;
    
    tango::IColumnInfoPtr colinfo = src_struct->getColumnInfoByIdx(mod_idx);
    if (colinfo.isNull())
        return;
    
    // the new name is the same as the old name
    std::wstring colname = colinfo->getName();
    std::wstring wnew_name = towstr(new_name);
    if (wnew_name == colname)
        return;
    
    // modify the column name in the source structure (this function
    // also updates the destination structure with any changes)
    tset->renameSourceColumn(colname, wnew_name);
    
    // repopulate the TransformationDoc from the destination structure
    ITransformationDocPtr transdoc;
    transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
    if (transdoc)
        transdoc->initFromSet(m_textdelimited_set);

    // update the TableDoc
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        // if the column was renamed, update the TableDoc's view
        ITableDocViewPtr tabledocview = tabledoc->getActiveView();
        if (tabledocview)
        {
            int viewidx = tabledocview->getColumnIdx(colname);
            
            // only modify the view column if it exists in the view
            if (viewidx != -1)
            {
                ITableDocViewColPtr viewcol = tabledocview->getColumn(viewidx);
                viewcol->setName(towstr(new_name));
            }
        }

        // update the TableDoc's base set and refresh its view
        tabledoc->open(m_path);
        tabledoc->refreshActiveView();
    }

    // refresh the iterator and the model for the grid
    m_textdelimited_iter->refreshStructure();
    m_grid->refreshModel();
    
    // set the view column info to match the model column info
    m_grid->setColumn(view_idx, mod_idx);

    // refresh the grid
    m_grid->refresh(kcl::Grid::refreshAll);

    updateColumnList();
    updateStatusBar();
    m_dirty = true;
}

bool TextDoc::isDestinationSameAsSourceStructure()
{
    // this function determines if the user has edited
    // anything in the TransformationDoc or not

    tango::IDelimitedTextSetPtr tset = getTextSet();
    tango::IFixedLengthDefinitionPtr fset = getTextSet();
    tango::IStructurePtr src_struct, dest_struct;
    
    if (tset)
    {
        src_struct = tset->getSourceStructure();
        dest_struct = tset->getDestinationStructure();
    }
    
    if (fset)
    {
        src_struct = fset->getSourceStructure();
        dest_struct = fset->getDestinationStructure();
    }
    
    int src_colcount = src_struct->getColumnCount();
    int dest_colcount = dest_struct->getColumnCount();
    if (src_colcount != dest_colcount)
        return false;
        
    tango::IColumnInfoPtr src_colinfo;
    tango::IColumnInfoPtr dest_colinfo;
    int i;
    
    for (i = 0; i < src_colcount; ++i)
    {
        src_colinfo = src_struct->getColumnInfoByIdx(i);
        dest_colinfo = dest_struct->getColumnInfoByIdx(i);
        
        if (src_colinfo->getName() != dest_colinfo->getName())
            return false;
            
        if (src_colinfo->getType() != dest_colinfo->getType())
            return false;
        
        if (src_colinfo->getWidth() != dest_colinfo->getWidth())
            return false;
            
        if (src_colinfo->getScale() != dest_colinfo->getScale())
            return false;
    }
    
    return true;
}

void TextDoc::resetTransformationDocAndTableDoc()
{
    // this function re-initializes both the TransformationDoc and the
    // TableDoc if a "big" change is made to the set (e.g. changing a
    // field delimiter, etc.)

    ITransformationDocPtr transdoc;
    transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
    if (transdoc)
        transdoc->initFromSet(getTextSet());
    
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        tango::ISetPtr set = getTextSet();
        if (set.isNull())
            return;

        tango::IFileInfoPtr finfo = g_app->getDatabase()->getFileInfo(towstr(m_path));
        if (finfo.isNull())
            return;

        ITableDocModelPtr model = TableDocMgr::loadModel(finfo->getObjectId());
        ITableDocViewEnumPtr views = model->getViewEnum();
        
        int i, count = views->size();
        for (i = 0; i < count; ++i)
        {
            ITableDocObjectPtr obj = views->getItem(i);
            model->deleteObject(obj);
        }

        // calling this after deleting all the views will cause
        // the TableDoc to "re-initialize" with a default view
        tabledoc->open(m_path);
    }
}

void TextDoc::doBulkFieldRename(std::vector< std::pair<wxString, wxString> > to_rename)
{
    // this function updates any columns in the TransformationDoc or TableDoc
    // which are associated with the source column that we're modifying

    // repopulate the TransformationDoc from the destination structure
    ITransformationDocPtr transdoc;
    transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
    if (transdoc)
        transdoc->initFromSet(getTextSet());

    // update any corresponding columns in the TableDoc's view
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        ITableDocViewPtr tabledocview = tabledoc->getActiveView();
        if (tabledoc.isOk() && tabledocview.isOk())
        {
            std::vector< std::pair<wxString, wxString> >::iterator it;
            for (it = to_rename.begin(); it != to_rename.end(); ++it)
            {
                // we couldn't find the column in the view, continue
                int colidx = tabledocview->getColumnIdx(towstr(it->first));
                if (colidx == -1)
                    continue;
                    
                ITableDocViewColPtr viewcol = tabledocview->getColumn(colidx);
                if (viewcol)
                    viewcol->setName(towstr(it->second));
            }

            // refresh the TableDoc's set and view
            tabledoc->open(m_path);
            tabledoc->refreshActiveView();
        }
    }
}


bool TextDoc::saveLayoutTemplate(const wxString& path)
{

    if (m_view == FixedLengthView)
    {
    }
     else if (m_view == TextDelimitedView)
    {
        kl::JsonNode root;
        
        kl::JsonNode metadata = root["metadata"];
        metadata["type"] = wxT("application/vnd.kx.text_format");
        metadata["version"] = 1;
        metadata["description"] = wxT("");

        root["type"] = wxT("delimited");
        root["delimiters"] = towstr(m_last_delimiters);
        root["line_delimiters"] = wxT("\n");
        root["first_row_column_names"] = m_firstrowfieldnames_checkbox->GetValue() ? true : false;

        kl::JsonNode fields = root["fields"];

        // if the set is not a text-delimited set, bail out
        tango::IDelimitedTextSetPtr tset = m_textdelimited_set;
        if (tset.isNull())
            return false;

        tango::IStructurePtr s = tset->getDestinationStructure();
        
        int i, count = s->getColumnCount();
        for (i = 0; i < count; ++i)
        {
            tango::IColumnInfoPtr e = s->getColumnInfoByIdx(i);
            
            kl::JsonNode field = fields.appendElement();
            
            field["name"] = e->getName();
            field["type"] = tango::dbtypeToString(e->getType());
            field["width"] = e->getWidth();
            field["scale"] = e->getScale();
        }

        return JsonConfig::saveToDb(root, g_app->getDatabase(), towstr(path), L"application/vnd.kx.text_format");
    }
    
    return false;
}



