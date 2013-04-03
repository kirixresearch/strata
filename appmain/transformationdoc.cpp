/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-27
 *
 */


#include "appmain.h"
#include "toolbars.h"
#include "textdoc.h"
#include "tabledoc.h"
#include "transformationdoc.h"
#include "structurevalidator.h"
#include "fieldtypechoice.h"
#include "../kl/include/kl/regex.h"


const wxString EMPTY_SOURCENAME_STR = wxT("--");

enum
{
    ID_InsertField = wxID_HIGHEST + 1
};

// the following enums must be kept in sync with their corresponding
// dropdown label vectors in TransformationDoc (see the constructor)

enum ExpressionFormatIndexes
{
    charfmtBegin = 0,
    charfmtTrimLeadingSpaces = charfmtBegin,
    charfmtTrimLeadingZeros,
    charfmtTrimLeadingSpacesAndZeros,
    charfmtOther,
    // this index is past the dropdown last index (won't show up as an option)
    charfmtEmpty,
    
    numfmtBegin = 20,
    numfmtLeadingNegativeEnglish = numfmtBegin,
    numfmtLeadingNegativeEuropean,
    numfmtTrailingNegativeEnglish,
    numfmtTrailingNegativeEuropean,
    numfmtParenthesisNegativeEnglish,
    numfmtParenthesisNegativeEuropean,
    numfmtOther,
    // this index is past the dropdown last index (won't show up as an option)
    numfmtEmpty,

    datefmtBegin = 40,
    datefmtYYYYMMDD = datefmtBegin,
    datefmtYYYYDDMM,
    datefmtMMDDYYYY,
    datefmtDDMMYYYY,
    datefmtYYMMDD,
    datefmtYYDDMM,
    datefmtMMDDYY,
    datefmtDDMMYY,
    datefmtYYYYDDD,
    datefmtDDDYYYY,
    datefmtJulian,
    datefmtOther,
    // this index is past the dropdown last index (won't show up as an option)
    datefmtEmpty,
    
    boolfmtBegin = 60,
    boolfmtTF = boolfmtBegin,
    boolfmtTrueFalse,
    boolfmtYN,
    boolfmtYesNo,
    boolfmt10,
    boolfmtNot00,
    boolfmtOther,
    // this index is past the dropdown last index (won't show up as an option)
    boolfmtEmpty,
};

enum GridColumnIndexes
{
    colRowNumber = 0,
    colFieldName,
    colFieldType,
    colFieldWidth,
    colFieldScale,
    colSourceName,
    colFieldFormula
};
    
// storage class for source name and type format lookup

struct ExpressionLookupInfo
{
    int format;
    const wchar_t* expr;
    klregex::wregex* regex;
    
    ~ExpressionLookupInfo()
    {
        delete regex;
        regex = NULL;
    }
};

static ExpressionLookupInfo expr_lookup_arr[] =
{
    { charfmtTrimLeadingSpaces          , L"((LTRIM(%s)))"        , NULL },
    { charfmtTrimLeadingZeros           , L"((LTRIM(%s,\"0\")))"  , NULL },
    { charfmtTrimLeadingSpacesAndZeros  , L"((LTRIM(%s,\"0 \")))" , NULL },
    { charfmtEmpty                      , L"((%s))"               , NULL },
    
    { numfmtLeadingNegativeEuropean     , L"((IIF(LTRIM(TRIM(TRIM(TRANSLATE(%s,\".\",\"\"))),\"- \")=TRIM(TRANSLATE(%s,\".\",\"\")),VAL(TRIM(TRANSLATE(%s,\".\",\"\"))),-1*VAL(LTRIM(TRIM(TRANSLATE(%s,\".\",\"\")) , \"- \")))))" , NULL },
    { numfmtLeadingNegativeEnglish      , L"((IIF(LTRIM(TRIM(TRIM(TRANSLATE(%s,\",\",\"\"))),\"- \")=TRIM(TRANSLATE(%s,\",\",\"\")),VAL(TRIM(TRANSLATE(%s,\",\",\"\"))),-1*VAL(LTRIM(TRIM(TRANSLATE(%s,\",\",\"\")) , \"- \")))))" , NULL },
    { numfmtTrailingNegativeEuropean    , L"((IIF(RTRIM(TRIM(TRIM(TRANSLATE(%s,\".\",\"\"))),\"- \")=TRIM(TRANSLATE(%s,\".\",\"\")),VAL(TRIM(TRANSLATE(%s,\".\",\"\"))),-1*VAL(RTRIM(TRIM(TRANSLATE(%s,\".\",\"\")),\"- \")))))"   , NULL },
    { numfmtTrailingNegativeEnglish     , L"((IIF(RTRIM(TRIM(TRIM(TRANSLATE(%s,\",\",\"\"))),\"- \")=TRIM(TRANSLATE(%s,\",\",\"\")),VAL(TRIM(TRANSLATE(%s,\",\",\"\"))),-1*VAL(RTRIM(TRIM(TRANSLATE(%s,\",\",\"\")),\"- \")))))"   , NULL },
    { numfmtParenthesisNegativeEuropean , L"((IIF(TRIM(TRIM(TRANSLATE(%s,\".\",\"\")),\"() \")=TRIM(TRANSLATE(%s,\".\",\"\")),VAL(TRIM(TRANSLATE(%s,\".\",\"\"))),-1*VAL(TRIM(TRIM(TRANSLATE(%s,\".\",\"\")),\"() \")))))"         , NULL },
    { numfmtParenthesisNegativeEnglish  , L"((IIF(TRIM(TRIM(TRANSLATE(%s,\",\",\"\")),\"() \")=TRIM(TRANSLATE(%s,\",\",\"\")),VAL(TRIM(TRANSLATE(%s,\",\",\"\"))),-1*VAL(TRIM(TRIM(TRANSLATE(%s,\",\",\"\")),\"() \")))))"         , NULL },
    { numfmtEmpty                       , L"((VAL(TRIM(%s))))"                                                                                                                                                                     , NULL },
    
    { datefmtYYYYMMDD  , L"((DATE(TRIM(%s),'YYYYMMDD')))"                                       , NULL },
    { datefmtYYYYDDMM  , L"((DATE(TRIM(%s),'YYYYDDMM')))"                                       , NULL },
    { datefmtMMDDYYYY  , L"((DATE(TRIM(%s),'MMDDYYYY')))"                                       , NULL },
    { datefmtDDMMYYYY  , L"((DATE(TRIM(%s),'DDMMYYYY')))"                                       , NULL },
    { datefmtYYMMDD    , L"((DATE(TRIM(%s),'YYMMDD')))"                                         , NULL },
    { datefmtYYDDMM    , L"((DATE(TRIM(%s),'YYDDMM')))"                                         , NULL },
    { datefmtMMDDYY    , L"((DATE(TRIM(%s),'MMDDYY')))"                                         , NULL },
    { datefmtDDMMYY    , L"((DATE(TRIM(%s),'DDMMYY')))"                                         , NULL },
    { datefmtYYYYDDD   , L"((DATE(VAL(SUBSTR(TRIM(%s),1,4)),1,1)+VAL(SUBSTR(TRIM(%s),5,3))-1))" , NULL },
    { datefmtDDDYYYY   , L"((DATE(VAL(SUBSTR(TRIM(%s),4,4)),1,1)+VAL(SUBSTR(TRIM(%s),1,3))-1))" , NULL },
    { datefmtJulian    , L"((DATE(0,0,0)+VAL(TRIM(%s))+1))"                                     , NULL },
    { datefmtEmpty     , L"((DATE(TRIM(%s))))"                                                  , NULL },
    
    { boolfmtTF        , L"((UPPER(TRIM(%s))=\"T\"))"                                                                                                                          , NULL },
    { boolfmtTrueFalse , L"((UPPER(TRIM(%s))=\"TRUE\"))"                                                                                                                       , NULL },
    { boolfmtYN        , L"((UPPER(TRIM(%s))=\"Y\"))"                                                                                                                          , NULL },
    { boolfmtYesNo     , L"((UPPER(TRIM(%s))=\"YES\"))"                                                                                                                        , NULL },
    { boolfmt10        , L"((VAL(TRIM(%s))=1))"                                                                                                                                , NULL },
    { boolfmtNot00     , L"((VAL(TRIM(%s))!=0))"                                                                                                                               , NULL },
    { boolfmtEmpty     , L"((VAL(TRIM(%s))=1 OR TRIM(%s)=\"T\" OR TRIM(%s)=\"t\" OR UPPER(TRIM(%s))=\"TRUE\" OR UPPER(TRIM(%s))=\"YES\" OR TRIM(%s)=\"Y\" OR TRIM(%s)=\"y\"))" , NULL },
};
    
// -- utility functions --

static const int format2comboIdx(int tango_type, int format_idx)
{
    switch (tango_type)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
            return (format_idx-charfmtBegin);
        
        case tango::typeNumeric:
        case tango::typeInteger:
        case tango::typeDouble:
            return (format_idx-numfmtBegin);
        
        case tango::typeDateTime:
        case tango::typeDate:
            return (format_idx-datefmtBegin);
        
        case tango::typeBoolean:
            return (format_idx-boolfmtBegin);
            
        default:
            return -1;
    }
    
    return -1;
}

static const int combo2formatIdx(int tango_type, int combo_idx)
{
    switch (tango_type)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
            return (combo_idx == -1) ? charfmtEmpty : (charfmtBegin+combo_idx);
        
        case tango::typeNumeric:
        case tango::typeInteger:
        case tango::typeDouble:
            return (combo_idx == -1) ? numfmtEmpty : (numfmtBegin+combo_idx);
        
        case tango::typeDateTime:
        case tango::typeDate:
            return (combo_idx == -1) ? datefmtEmpty : (datefmtBegin+combo_idx);
        
        case tango::typeBoolean:
            return (combo_idx == -1) ? boolfmtEmpty : (boolfmtBegin+combo_idx);
            
        default:
            return -1;
    }
    
    return -1;
}

static const std::wstring expr2regex(const std::wstring& expr)
{
    // even though it is a little slower to convert to wxString and back,
    // there are far more string translation functions to help us
    wxString e = towx(expr);
    e.Replace(wxT("("), wxT("\\("), true);
    e.Replace(wxT(")"), wxT("\\)"), true);
    e.Replace(wxT("+"), wxT("\\+"), true);
    e.Replace(wxT("="), wxT("\\="), true);
    e.Replace(wxT("%s"), wxT("([^,\"()]+)"), true);
    return towstr(e);
}
    
static bool isFieldDynamic(kcl::Grid* grid, int row)
{
    TransformField* f = (TransformField*)grid->getRowData(row);
    if (!f)
        return false;

    return f->dynamic;
}

static std::vector<RowErrorChecker> getRowErrorCheckerVector(
                                        kcl::Grid* grid,
                                        bool include_empty_fieldnames = true)
{
    std::vector<RowErrorChecker> vec;
    
    int row, row_count = grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        wxString fieldname = grid->getCellString(row, colFieldName);
        
        if (fieldname.IsEmpty() && !include_empty_fieldnames)
            continue;
            
        vec.push_back(RowErrorChecker(row, fieldname));
    }

    return vec;
}

tango::IStructurePtr getTextSourceStructure(IDocumentSitePtr doc_site)
{
    tango::IStructurePtr source_struct;
    
    // get the source structure so we can check the expression return type
    ITextDocPtr textdoc = lookupOtherDocument(doc_site, "appmain.TextDoc");
    if (textdoc.isOk())
    {
        tango::ISetPtr set = textdoc->getTextSet();
        tango::IFixedLengthDefinitionPtr fset = set;
        tango::IDelimitedTextSetPtr tset = set;
        if (fset.isOk())
            source_struct = fset->getSourceStructure();
             else if (tset.isOk())
            source_struct = tset->getSourceStructure();
    }
    
    return source_struct;
}


    
    
BEGIN_EVENT_TABLE(TransformationDoc, wxWindow)
    EVT_SIZE(TransformationDoc::onSize)
    EVT_ERASE_BACKGROUND(TransformationDoc::onEraseBackground)
    EVT_MENU(ID_Edit_SelectAll, TransformationDoc::onSelectAll)
    EVT_MENU(ID_File_Save, TransformationDoc::onSave)
    EVT_MENU(ID_Edit_Copy, TransformationDoc::onCopy)
    EVT_MENU(ID_Edit_Delete, TransformationDoc::onDeleteField)
    EVT_MENU(ID_InsertField, TransformationDoc::onInsertField)
    EVT_KCLGRID_NEED_TOOLTIP_TEXT(TransformationDoc::onGridNeedTooltipText)
    EVT_KCLGRID_BEGIN_EDIT(TransformationDoc::onGridBeginEdit)
    EVT_KCLGRID_END_EDIT(TransformationDoc::onGridEndEdit)
    EVT_KCLGRID_EDIT_CHANGE(TransformationDoc::onGridEditChange)
    EVT_KCLGRID_CELL_RIGHT_CLICK(TransformationDoc::onGridCellRightClick)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(TransformationDoc::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(TransformationDoc::onGridPreInvalidAreaInsert)
    EVT_MENU(ID_View_SwitchToDesignView, TransformationDoc::onToggleView)

    // disable data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, TransformationDoc::onUpdateUI_DisableAlways)

    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, TransformationDoc::onUpdateUI_DisableAlways)
    
    // disable format items
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, TransformationDoc::onUpdateUI_DisableAlways)

    // disable table items
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, TransformationDoc::onUpdateUI_DisableAlways)

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_SaveAs, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_SaveAsExternal, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Bookmark, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_PageSetup, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Print, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Run, TransformationDoc::onUpdateUI_DisableAlways)

    // disable the zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomIn, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomOut, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToActual, TransformationDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, TransformationDoc::onUpdateUI_DisableAlways)

    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, TransformationDoc::onUpdateUI)

END_EVENT_TABLE()


TransformationDoc::TransformationDoc()
{
    m_dirty = false;
    m_grid = NULL;
    m_last_selected_fieldtype = -1;
    m_init_set = xcm::null;
    
    m_char_format_choices.push_back(_("Trim leading spaces"));
    m_char_format_choices.push_back(_("Trim leading zeros"));
    m_char_format_choices.push_back(_("Trim leading spaces and zeros"));
    m_char_format_choices.push_back(_("Other..."));

    m_numeric_format_choices.push_back(wxT("-1000.00"));
    m_numeric_format_choices.push_back(wxT("-1000,00"));
    m_numeric_format_choices.push_back(wxT("1000.00-"));
    m_numeric_format_choices.push_back(wxT("1000,00-"));
    m_numeric_format_choices.push_back(wxT("(1000.00)"));
    m_numeric_format_choices.push_back(wxT("(1000,00)"));
    m_numeric_format_choices.push_back(  _("Other..."));

    m_date_format_choices.push_back(wxT("YYYYMMDD"));
    m_date_format_choices.push_back(wxT("YYYYDDMM"));
    m_date_format_choices.push_back(wxT("MMDDYYYY"));
    m_date_format_choices.push_back(wxT("DDMMYYYY"));
    m_date_format_choices.push_back(wxT("YYMMDD"));
    m_date_format_choices.push_back(wxT("YYDDMM"));
    m_date_format_choices.push_back(wxT("MMDDYY"));
    m_date_format_choices.push_back(wxT("DDMMYY"));
    m_date_format_choices.push_back(wxT("YYYYDDD"));
    m_date_format_choices.push_back(wxT("DDDYYYY"));
    m_date_format_choices.push_back(  _("Julian Date"));
    m_date_format_choices.push_back(  _("Other..."));

    m_bool_format_choices.push_back(wxT("T/F"));
    m_bool_format_choices.push_back(wxT("TRUE/FALSE"));
    m_bool_format_choices.push_back(wxT("Y/N"));
    m_bool_format_choices.push_back(wxT("YES/NO"));
    m_bool_format_choices.push_back(wxT("1/0"));
    m_bool_format_choices.push_back(  _("Not 0/0"));
    m_bool_format_choices.push_back(  _("Other..."));
}

TransformationDoc::~TransformationDoc()
{
}


bool TransformationDoc::open(const wxString& path)
{
    m_path = path;
    return true;
}


inline void resizeAllGridColumnsToFitDoc(kcl::Grid* grid)
{
    // this code changes the two proportional columns to be non-proportionally
    // sized, which will make the grid fill the client window size initially,
    // but still allow the user to resize each column
    int w, h;
    grid->GetClientSize(&w, &h);
    w -= grid->getColumnSize(colRowNumber);
    w -= grid->getColumnSize(colSourceName);
    w -= grid->getColumnSize(colFieldType);
    w -= grid->getColumnSize(colFieldWidth);
    w -= grid->getColumnSize(colFieldScale);
    grid->setColumnSize(colFieldName, w*30/100);
    grid->setColumnSize(colFieldFormula, w*70/100);

    // refresh the row selection grid
    grid->moveCursor(0, colFieldName);
    grid->refresh(kcl::Grid::refreshAll);
}

bool TransformationDoc::initDoc(IFramePtr frame,
                                IDocumentSitePtr doc_site,
                                wxWindow* docsite_wnd,
                                wxWindow* panesite_wnd)
{
    // store ptrs
    m_frame = frame;
    m_doc_site = doc_site;
    
    // set the document's caption and icon
    wxString caption;
    if (m_path.Length() > 0)
        caption = m_path.AfterLast(wxT('/'));
         else
        caption = _("(Untitled)");
    
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

    wxFont overlay_font = wxFont(16, wxSWISS, wxNORMAL, wxNORMAL, false);
    
    // create transformation grid
    m_grid = new kcl::RowSelectionGrid(this,
                                       -1,
                                       wxPoint(0,0),
                                       docsite_wnd->GetClientSize(),
                                       wxBORDER_NONE);
    m_grid->setDragFormat(wxT("transformationdoc"));
    m_grid->setRowSelectionGridFlags(kcl::RowSelectionGrid::refreshNone);
    m_grid->setOverlayFont(overlay_font);
    m_grid->setCursorType(kcl::Grid::cursorThin);
    m_grid->setAllowInvalidAreaInserts(true);
    m_grid->setAllowKeyboardInserts(true);
    m_grid->setAllowDeletes(true);
    m_grid->setCursorVisible(true);

    m_grid->createModelColumn(colRowNumber, wxEmptyString, kcl::Grid::typeCharacter, 5, 0);
    m_grid->createModelColumn(colFieldName, _("Field Name"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(colFieldType, _("Type"), kcl::Grid::typeCharacter, 1, 0);
    m_grid->createModelColumn(colFieldWidth, _("Width"), kcl::Grid::typeInteger, 5, 0);
    m_grid->createModelColumn(colFieldScale, _("Decimals"), kcl::Grid::typeInteger, 5, 0);
    m_grid->createModelColumn(colSourceName, _("Source Name"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(colFieldFormula, _("Format/Formula"), kcl::Grid::typeCharacter, 4096, 0);
    m_grid->createDefaultView();
    
    m_grid->setColumnSize(colRowNumber, 30);
    m_grid->setColumnSize(colFieldName, 140);
    m_grid->setColumnSize(colFieldType, 110);
    m_grid->setColumnSize(colFieldWidth, 55);
    m_grid->setColumnSize(colFieldScale, 55);
    m_grid->setColumnSize(colSourceName, 120);
    m_grid->setColumnSize(colFieldFormula, 160);

    // set cell properties for source fields choice control
    populateSourceFieldDropDown();
    
    // set cell properties for field type choice control
    kcl::CellProperties props;
    props.mask = kcl::CellProperties::cpmaskCtrlType |
                 kcl::CellProperties::cpmaskCbChoices;
    props.ctrltype = kcl::Grid::ctrltypeDropList;
    props.cbchoices.push_back(tango2text(tango::typeCharacter));
    props.cbchoices.push_back(tango2text(tango::typeWideCharacter));
    props.cbchoices.push_back(tango2text(tango::typeNumeric));
    props.cbchoices.push_back(tango2text(tango::typeDouble));
    props.cbchoices.push_back(tango2text(tango::typeInteger));
    props.cbchoices.push_back(tango2text(tango::typeDate));
    props.cbchoices.push_back(tango2text(tango::typeDateTime));
    props.cbchoices.push_back(tango2text(tango::typeBoolean));
    m_grid->setModelColumnProperties(colFieldType, &props);
    
    props.mask = kcl::CellProperties::cpmaskEditable |
                 kcl::CellProperties::cpmaskAlignment |
                 kcl::CellProperties::cpmaskBitmapAlignment;
    props.editable = false;
    props.alignment = kcl::Grid::alignRight;
    props.bitmap_alignment = kcl::Grid::alignRight;
    m_grid->setModelColumnProperties(colRowNumber, &props);

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_grid, 1, wxEXPAND);
    SetSizer(main_sizer);
    Layout();

    // fits all of the grid's columns to the size of the document
    resizeAllGridColumnsToFitDoc(m_grid);

    // make this grid a drop target and connect the signal
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_grid);
    drop_target->setGridDataObjectFormats(wxT("transformationdoc"), wxT("fieldspanel"));
    drop_target->sigDropped.connect(this, &TransformationDoc::onGridDataDropped);
    m_grid->SetDropTarget(drop_target);

    // add frame event handlers
    frame->sigFrameEvent().connect(this, &TransformationDoc::onFrameEvent);

    // connect row selection grid signals
    m_grid->sigInsertingRows.connect(this, &TransformationDoc::onInsertingRows);
    m_grid->sigDeletedRows.connect(this, &TransformationDoc::onDeletedRows);

    // create the statusbar items for this document
    IStatusBarItemPtr item;

    item = addStatusBarItem(wxT("transformationdoc_field_count"));
    item->setWidth(120);
    
    item = addStatusBarItem(wxT("transformationdoc_row_width"));
    item->setWidth(150);
    
    return true;
}

wxString TransformationDoc::getDocumentLocation()
{
    IDocumentPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
    if (textdoc)
        return textdoc->getDocumentLocation();

    return urlToFilename(m_path);
}

void TransformationDoc::setDocumentFocus()
{
    m_grid->SetFocus();
}

bool TransformationDoc::onSiteClosing(bool force)
{
    if (force)
    {
        return true;
    }

    if (m_dirty)
    {
        int result = appMessageBox(_("Would you like to save the changes made to the table's structure?"),
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
        doSave();
    }

    return true;
}

void TransformationDoc::onSiteActivated()
{
    if (m_grid)
        updateStatusBar();
}

void TransformationDoc::initFromSet(tango::ISetPtr set)
{
    tango::IStructurePtr source_structure;
    tango::IStructurePtr structure;
    tango::IColumnInfoPtr colinfo;

    tango::IFixedLengthDefinitionPtr fset = set;
    if (fset)
    {
        source_structure = fset->getSourceStructure();
        structure = fset->getDestinationStructure();
    }
    
    tango::IDelimitedTextSetPtr tset = set;
    if (tset)
    {
        source_structure = tset->getSourceStructure();
        structure = tset->getDestinationStructure();
    }

    // store the set we used to initialize the grid
    m_init_set = set;
    
    // clear the grid
    m_grid->deleteAllRows();
    
    // set the input structure from the set's source structure
    // (this will also populate the source field dropdown)
    setInputStructure(source_structure);

    // populate the grid from the set's destination structure
    int row = 0;
    int i, col_count = structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        colinfo = structure->getColumnInfoByIdx(i);
        insertRowFromColumnInfo(row, colinfo, source_structure);
        row++;
    }
    
    updateNumberColumn();
    resizeAllGridColumnsToFitDoc(m_grid);

    // refresh the row selection grid
    if (m_grid->getRowCount() > 0)
    {
        m_grid->setRowSelected(0, true);
        m_grid->moveCursor(0, colFieldName);
    }
    m_grid->refresh(kcl::Grid::refreshAll);

    // update the status bar
    updateStatusBar();
}

void TransformationDoc::close()
{
    m_init_set.clear();
}

void TransformationDoc::getTransformation(std::vector<TransformField>& result)
{
    wxString input_name;
    wxString name;
    int type;
    int width;
    int scale;
    wxString expression;
    bool dynamic;

    int row, row_count = m_grid->getRowCount();
    
    result.clear();
    result.reserve(row_count);
    
    for (row = 0; row < row_count; ++row)
    {
        input_name = m_grid->getCellString(row, colSourceName);
        name = m_grid->getCellString(row, colFieldName);
        type = m_grid->getCellComboSel(row, colFieldType);
        width = m_grid->getCellInteger(row, colFieldWidth);
        scale = m_grid->getCellInteger(row, colFieldScale);
        expression = m_grid->getCellString(row, colFieldFormula);
        dynamic = isFieldDynamic(m_grid, row);
        
        TransformField field = getInputFieldByName(input_name);
        field.output_name = name;
        field.output_type = choice2tango(type);
        field.output_width = width;
        field.output_scale = scale;
        field.output_expression = expression;
        field.dynamic = dynamic;
        
        result.push_back(field);
    }
}

void TransformationDoc::setInputStructure(const std::vector<TransformField>& input_fields)
{
    m_source_fields = input_fields;
    populateSourceFieldDropDown();
}

void TransformationDoc::setInputStructure(tango::IStructurePtr structure)
{
    m_source_fields.clear();
    
    TransformField field;
    
    int i, count = structure->getColumnCount();
    for (i = 0; i < count; ++i)
    {
        tango::IColumnInfoPtr col = structure->getColumnInfoByIdx(i);
        if (col)
        {
            field.input_name = towx(col->getName());
            field.input_type = col->getType();
            field.input_width = col->getWidth();
            field.input_scale = col->getScale();
            field.input_offset = col->getOffset();
            m_source_fields.push_back(field);
        }
    }
    
    populateSourceFieldDropDown();
}

void TransformationDoc::getColumnListItems(std::vector<ColumnListItem>& items)
{
    // get the column list from the text doc (it will figure out the
    // set type for us and return the appropriate source structure)
    
    ITextDocPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
    if (textdoc.isOk())
    {
        IColumnListTargetPtr columnlist_target = textdoc;
        if (columnlist_target.isOk())
            columnlist_target->getColumnListItems(items);
    }
}

void TransformationDoc::onColumnListDblClicked(const std::vector<wxString>& items)
{
    tango::IStructurePtr s = getTextSourceStructure(m_doc_site);
    if (s.isNull())
        return;
        
    m_grid->Freeze();

    int row = m_grid->getRowCount();

    // inserting fields in from the field panel
    std::vector<wxString>::const_iterator it;
    for (it = items.begin(); it != items.end(); ++it)
    {
        // get the column info from the column name we double-clicked
        tango::IColumnInfoPtr colinfo = s->getColumnInfo(towstr(*it));
        if (colinfo.isNull())
            continue;
        
        // we need to set the expression to the source column name
        // so that we know which source column this column refers to
        colinfo->setExpression(colinfo->getName());
        
        insertRowFromColumnInfo(row, colinfo);
        row++;
    }

    updateNumberColumn();
    m_grid->moveCursor(m_grid->getRowCount()-1, colFieldName);
    m_grid->scrollVertToCursor();
    m_grid->refresh(kcl::Grid::refreshAll);
    
    m_grid->Thaw();
    
    updateStatusBar();
}

TransformField TransformationDoc::getInputFieldByName(const wxString& input_name)
{
    std::vector<TransformField>::iterator it;
    for (it = m_source_fields.begin(); it != m_source_fields.end(); ++it)
    {
        if (it->input_name == input_name)
            return *it;
    }
    
    TransformField f;
    return f;
}

void TransformationDoc::checkOverlayText()
{
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(_("To add fields to this table, drag in fields\nfrom the Fields Panel or double-click here"));
         else
        m_grid->setOverlayText(wxEmptyString);
}

void TransformationDoc::insertRow(int row, bool dynamic)
{
    if (row == -1)
        row = m_grid->getRowCount();
    
    TransformField* f = new TransformField;
    f->input_name = wxEmptyString;
    f->input_type = tango::typeCharacter;
    f->input_width = 20;
    f->input_scale = 0;
    f->input_offset = 0;
    f->output_name = wxEmptyString;
    f->output_type = tango::typeCharacter;
    f->output_width = 20;
    f->output_scale = 0;
    f->output_expression = wxT("\"\"");
    f->dynamic = dynamic;
    f->original = false;
    
    m_grid->insertRow(row);
    m_grid->setRowData(row, (long)f);
    m_grid->setCellBitmap(row, colRowNumber, dynamic ? GETBMP(gf_lightning_16)
                                                     : GETBMP(xpm_blank_16));
    m_grid->setCellComboSel(row, colFieldType, tango2choice(f->output_type));
    m_grid->setCellString(row, colFieldName, f->output_name);
    m_grid->setCellInteger(row, colFieldWidth, f->output_width);
    m_grid->setCellInteger(row, colFieldScale, f->output_scale);
    m_grid->setCellString(row, colSourceName, EMPTY_SOURCENAME_STR);
    m_grid->setCellString(row, colFieldFormula, f->output_expression);
    m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));

    // make sure either a source field or an expression is specified
    int valid_res = validateExpression(getSourceStructure(), f->output_expression, f->output_type);
    updateExpressionIcon(row, valid_res);

    updateRowCellProps(row);
    checkOverlayText();
}

void TransformationDoc::insertSelectedRows(bool dynamic)
{
    kcl::SelectionRect rect;
    int sel_count = m_grid->getSelectionCount();
        
    // insert the selected number of rows into the grid
    for (int i = 0; i < sel_count; ++i)
    {
        m_grid->getSelection(i, &rect);
        int start_row = rect.m_start_row;
        int row = rect.m_end_row;
        
        while (row-- >= start_row)
        {
            insertRow(start_row, dynamic);
        }
    }
    
    // the grid is empty, insert a starter row
    if (sel_count == 0 && m_grid->getRowCount() == 0)
    {
        insertRow(0, dynamic);
        m_grid->setRowSelected(0, true);
    }

    updateNumberColumn();
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void TransformationDoc::insertRowFromColumnInfo(int row,
                                                tango::IColumnInfoPtr colinfo,
                                                tango::IStructurePtr validate_against)
{
    if (row == -1)
        row = m_grid->getRowCount();
        
    TransformField* f = new TransformField;
    f->input_name = towx(colinfo->getName());
    f->input_type = tango::typeCharacter;
    f->input_width = colinfo->getWidth();
    f->input_scale = colinfo->getScale();
    f->input_offset = colinfo->getOffset();
    f->output_name = makeProper(towx(colinfo->getName()));
    f->output_type = colinfo->getType();
    f->output_width = colinfo->getWidth();
    f->output_scale = colinfo->getScale();
    f->output_expression = towx(colinfo->getExpression());
    f->dynamic = colinfo->getCalculated() ? true : false;
    f->original = true;
    
    m_grid->insertRow(row);
    m_grid->setRowData(row, (long)f);
    m_grid->setCellString(row, colFieldName, f->output_name);
    m_grid->setCellComboSel(row, colFieldType, tango2choice(f->output_type));
    m_grid->setCellInteger(row, colFieldWidth, f->output_width);
    m_grid->setCellInteger(row, colFieldScale, f->output_scale);
    if (f->dynamic)
        m_grid->setCellBitmap(row, colRowNumber, GETBMP(gf_lightning_16));
     else
        m_grid->setCellBitmap(row, colRowNumber, GETBMP(xpm_blank_16));
    
    // we need to update the row's cell props here so that the expression
    // cell's combobox is populated before we set its cell info
    updateRowCellProps(row);
    
    wxString source_name;
    int format_comboidx;
    bool res = getInfoFromDestinationExpression(towx(colinfo->getExpression()),
                                                colinfo->getType(),
                                                &source_name,
                                                &format_comboidx);
    if (!res)
    {
        // there were no matches, which means it is a custom expression
        
        m_grid->setCellString(row, colSourceName, source_name);
        
        // if the source name cell's dropdown returns -1 after setting
        // the source name, then the expression is not a source name
        if (m_grid->getCellComboSel(row, colSourceName) == -1)
            m_grid->setCellString(row, colSourceName, EMPTY_SOURCENAME_STR);
            
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
        
        // only set the expression cell's text if the column info's
        // expression was not a source name
        wxString wx_expr = towx(colinfo->getExpression());
        if (wx_expr.CmpNoCase(m_grid->getCellString(row, colSourceName)) != 0)
            m_grid->setCellString(row, colFieldFormula, wx_expr);
    }
     else
    {
        // we found a match, so populate the cells with the matched info
        m_grid->setCellString(row, colSourceName, source_name);
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
        m_grid->setCellComboSel(row, colFieldFormula, format_comboidx);
    }
    
    // get (or create) the field's expression
    wxString field_expr = getFieldExpression(row);
    
    // make sure either a source field or an expression is specified
    int valid_res = validateExpression(validate_against, field_expr, f->output_type);
    updateExpressionIcon(row, valid_res);

    checkOverlayText();
}

void TransformationDoc::updateRowCellProps(int row)
{
    int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
    
    bool width_editable = true;
    bool decimal_editable = true;

    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskCtrlType |
                     kcl::CellProperties::cpmaskCbChoices;
    cellprops.ctrltype = kcl::Grid::ctrltypeComboBox;

    // determine if the width and decimal cells are editable
    switch (type)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
            decimal_editable = false;
            cellprops.cbchoices = m_char_format_choices;
            break;
            
        case tango::typeNumeric:
            cellprops.cbchoices = m_numeric_format_choices;
            break;

        case tango::typeInteger:
            width_editable = false;
            decimal_editable = false;
            cellprops.cbchoices = m_numeric_format_choices;
            break;
            
        case tango::typeDouble:
            width_editable = false;
            cellprops.cbchoices = m_numeric_format_choices;
            break;

        case tango::typeDateTime:
        case tango::typeDate:
            width_editable = false;
            decimal_editable = false;
            cellprops.cbchoices = m_date_format_choices;
            break;

        case tango::typeBoolean:
            width_editable = false;
            decimal_editable = false;
            cellprops.cbchoices = m_bool_format_choices;
            break;
        
        default:
            break;
    }

    // set the format/formula cell properties
    m_grid->setCellProperties(row, colFieldFormula, &cellprops);
    
    cellprops.mask = kcl::CellProperties::cpmaskEditable |
                     kcl::CellProperties::cpmaskVisible;

    // if the width can't be edited, don't show it
    cellprops.visible = width_editable;
    cellprops.editable = width_editable;
    m_grid->setCellProperties(row, colFieldWidth, &cellprops);

    // if the scale can't be edited, don't show it
    cellprops.visible = decimal_editable;
    cellprops.editable = decimal_editable;
    m_grid->setCellProperties(row, colFieldScale, &cellprops);
}

void TransformationDoc::updateNumberColumn()
{
    // resize row number column based on it's max text size
    wxClientDC cdc(this);
    wxString text;
    int w, h, descent, leading;
    int max_width = 0;
    wxFont font = m_grid->GetFont();
    
    int row_count = m_grid->getRowCount();
    for (int i = 0; i < row_count; ++i)
    {
        text = wxString::Format(wxT("%d"), i+1);
        m_grid->setCellString(i, colRowNumber, text);
        cdc.GetTextExtent(text, &w, &h, &descent, &leading, &font);
        if (w > max_width)
            max_width = w;
    }
    
    if (row_count == 0)
    {
        cdc.GetTextExtent(wxT("1"), &w, &h, &descent, &leading, &font);
        if (w > max_width)
            max_width = w;
    }
    
    max_width += 16;    // this accounts for the bitmap width
    max_width += 12;    // this accounts for the bitmap and cell padding
    
    if (m_grid->getColumnSize(colRowNumber) < max_width)
        m_grid->setColumnSize(colRowNumber, max_width);
}

void TransformationDoc::updateExpressionIcon(int row, int validation_res)
{
    if (validation_res == StructureValidator::ExpressionInvalid)
    {
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_x_16));
    }
     else if (validation_res == StructureValidator::ExpressionTypeMismatch)
    {
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_exclamation_16));
    }
     else if (validation_res == StructureValidator::ExpressionValid)
    {
        int cursor_col = m_grid->getCursorColumn();
        if (m_grid->isEditing() && cursor_col == colFieldFormula)
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_checkmark_16));
         else
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
    }
     else
    {
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
    }
}

void TransformationDoc::updateRowWidthAndScale(int row)
{
    // NOTE: now that we hide default field widths and scales (e.g. width
    //       is hidden for date, double, etc. because it is standard), we
    //       actually don't need to update these values until we actually
    //       save the structure (this will help greatly when switching
    //       back and forth between types because existing info will
    //       not be lost every time the type changes
    
    if (m_last_selected_fieldtype == -1)
        return;
    
    int last_tango_type = choice2tango(m_last_selected_fieldtype);
    int tango_type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
    
    TransformField* f = (TransformField*)(m_grid->getRowData(row));
    
    if (tango_type == tango::typeCharacter ||
        tango_type == tango::typeWideCharacter)
    {
        if (last_tango_type == tango::typeCharacter ||
            last_tango_type == tango::typeWideCharacter)
        {
            return;
        }
        
        // if the width of the field was set to something less than
        // the original input width, revert back to the input width
        int width = m_grid->getCellInteger(row, colFieldWidth);
        if (width < f->output_width)
        {
            m_grid->setCellInteger(row, colFieldWidth, f->output_width);
        }
    }
    
    if (tango_type == tango::typeNumeric)
    {
        // if the width of the field was set to something above
        // the max numeric width, cap it off
        int width = m_grid->getCellInteger(row, colFieldWidth);
        if (width > tango::max_numeric_width)
        {
            m_grid->setCellInteger(row, colFieldWidth,
                                   tango::max_numeric_width);
        }
    }
}

void TransformationDoc::updateStatusBar()
{
    // -- if the grid hasn't been created yet, bail out --
    if (!m_grid)
        return;

    // -- field count --
    int row, row_count = m_grid->getRowCount();
    
    // -- row width --
    int total_width = 0;
    for (row = 0; row < row_count; ++row)
    {
        total_width += m_grid->getCellInteger(row, colFieldWidth);
    }
    
    wxString field_count_str = wxString::Format(_("Field Count: %d"), row_count);
    wxString row_width_str = wxString::Format(_("Record Width: %d"), total_width);
    
    IStatusBarItemPtr item;
    item = m_frame->getStatusBar()->getItem(wxT("transformationdoc_field_count"));
    if (item.isOk())
        item->setValue(field_count_str);
    item = m_frame->getStatusBar()->getItem(wxT("transformationdoc_row_width"));
    if (item.isOk())
        item->setValue(row_width_str);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}

wxString TransformationDoc::createDestinationExpression(int row)
{
    int tango_type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
    int format_comboidx = m_grid->getCellComboSel(row, colFieldFormula);
    wxString source_name = m_grid->getCellString(row, colSourceName);
    wxString quoted_source_name = towx(tango::quoteIdentifier(g_app->getDatabase(), towstr(source_name)));
    
    // translate from the combobox index and tango type
    // to the expression format index
    int expr_format = combo2formatIdx(tango_type, format_comboidx);
    wxString retval;
    
    // we couldn't find the expression index for the lookup below, so
    // we'll just use the source name of the field as the expression
    if (expr_format == -1)
        return quoted_source_name;
    
    // lookup the expression in our lookup array
    int i, count = sizeof(expr_lookup_arr)/sizeof(ExpressionLookupInfo);
    for (i = 0; i < count; ++i)
    {
        if (expr_lookup_arr[i].format == expr_format)
        {
            retval = towx(expr_lookup_arr[i].expr);
            retval.Replace(wxT("%s"), quoted_source_name, true);
            return retval;
        }
    }

    // something went wrong, return the source name as the expression
    return quoted_source_name;
}

bool TransformationDoc::getInfoFromDestinationExpression(
                                             const wxString& expression,
                                             int tango_type,
                                             wxString* source_name,
                                             int* format_comboidx)
{
    if (expression.Mid(0, 2) != wxT("((") &&
        expression.Mid(expression.Length()-2, 2) != wxT("))"))
    {
        // this is safe to do since the source name cell in the grid
        // is a wxChoice cell, so if the expression isn't a source name,
        // the cell's wxChoice index will show up as -1
        *source_name = expression;
        *format_comboidx = -1;
        return false;
    }

    std::wstring wexpression = towstr(expression);
    const wchar_t* match_expr = wexpression.c_str();
    const wchar_t* start = match_expr;
    const wchar_t* end = match_expr + wcslen(match_expr);

    klregex::wmatch matchres;

    // create the klregex::wregex for the array entry if it doesn't exist
    int i, count = sizeof(expr_lookup_arr)/sizeof(ExpressionLookupInfo);
    for (i = 0; i < count; ++i)
    {
        if (expr_lookup_arr[i].regex == NULL)
        {
            std::wstring regex_str = expr2regex(expr_lookup_arr[i].expr);
            expr_lookup_arr[i].regex = new klregex::wregex(regex_str);
        }
    }

    // now, try to match the expression using the regex
    for (i = 0; i < count; ++i)
    {
        if (expr_lookup_arr[i].regex->match(start, end, matchres))
        {
            klregex::wsubmatch name_match = matchres[1];
            
            // fill out the source name
            if (name_match.isValid())
                *source_name = towx(name_match.str());
            source_name->Trim(true);
            source_name->Trim(false);
            
            // fill out the format combobox's index
            *format_comboidx = format2comboIdx(tango_type,
                                               expr_lookup_arr[i].format);
            return true;
        }
    }

    *source_name = wxEmptyString;
    *format_comboidx = -1;
    return false;
}

wxString TransformationDoc::getFieldExpression(int row)
{
    int format_comboidx = m_grid->getCellComboSel(row, colFieldFormula);
    wxString expression = m_grid->getCellString(row, colFieldFormula);
    
    if (format_comboidx == -1 && expression.Length() > 0)
    {
        // the user has typed in an expression, so don't worry
        // about creating a destination expression
        return expression;
    }
     else
    {
        // either a format was selected or the expression was
        // left blank, so we need to create a destination expression
        expression = createDestinationExpression(row);
    }
    
    return expression;
}

void TransformationDoc::populateSourceFieldDropDown()
{
    if (!m_grid)
        return;

    kcl::CellProperties props;
    props.mask = kcl::CellProperties::cpmaskCtrlType |
                 kcl::CellProperties::cpmaskCbChoices;
    props.ctrltype = kcl::Grid::ctrltypeDropList;
    props.cbchoices.clear();
    props.cbchoices.push_back(EMPTY_SOURCENAME_STR);
    
    std::vector<TransformField>::iterator it;
    for (it = m_source_fields.begin(); it != m_source_fields.end(); ++it)
    {
        props.cbchoices.push_back(it->input_name);
    }
    
    m_grid->setModelColumnProperties(colSourceName, &props);
    m_grid->refresh(kcl::Grid::refreshAll);
}

void TransformationDoc::clearProblemRows()
{
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        if (isFieldDynamic(m_grid, row))
            m_grid->setCellBitmap(row, colRowNumber, GETBMP(gf_lightning_16));
         else
            m_grid->setCellBitmap(row, colRowNumber, GETBMP(xpm_blank_16));
    }
}

void TransformationDoc::markProblemRow(int row, bool scroll_to)
{
    m_grid->setCellBitmap(row, colRowNumber, GETBMP(gf_exclamation_16));

    if (scroll_to)
    {
        m_grid->moveCursor(row, colFieldName, false);
        if (!m_grid->isCursorVisible())
        {
            m_grid->scrollVertToCursor();
        }
    }
}

int TransformationDoc::checkDuplicateFieldnames(int check_flags)
{
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    bool include_empty_fieldnames = false;
    if (check_flags & CheckEmptyFieldnames)
        include_empty_fieldnames = true;
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, include_empty_fieldnames);

    bool mark_rows = (check_flags & CheckMarkRows);
    bool errors_found = StructureValidator::findDuplicateFieldNames(check_rows);
    if (errors_found && mark_rows)
    {
        std::vector<RowErrorChecker>::iterator it;
        for (it = check_rows.begin(); it != check_rows.end(); ++it)
        {
            if (it->errors != StructureValidator::ErrorNone)
                markProblemRow(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorDuplicateFieldNames
                         : StructureValidator::ErrorNone);
}

int TransformationDoc::checkInvalidFieldnames(int check_flags)
{
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    bool include_empty_fieldnames = false;
    if (check_flags & CheckEmptyFieldnames)
        include_empty_fieldnames = true;
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, include_empty_fieldnames);

    bool mark_rows = (check_flags & CheckMarkRows);
    bool errors_found = StructureValidator::findInvalidFieldNames(check_rows);
    if (errors_found && mark_rows)
    {
        std::vector<RowErrorChecker>::iterator it;
        for (it = check_rows.begin(); it != check_rows.end(); ++it)
        {
            if (it->errors != StructureValidator::ErrorNone)
                markProblemRow(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorInvalidFieldNames
                         : StructureValidator::ErrorNone);
}

int TransformationDoc::validateExpression(tango::IStructurePtr structure, const wxString& expr, int type)
{
    if (structure.isNull())
        structure = getSourceStructure();

    if (structure.isNull())
        return StructureValidator::ErrorInvalidStructure;

    return StructureValidator::validateExpression(structure, expr, type);
}

int TransformationDoc::validateStructure()
{
    // CHECK: check for empty structure
    if (m_grid->getRowCount() == 0)
        return StructureValidator::ErrorNoFields;

    tango::IStructurePtr source_structure = getSourceStructure();
    
    // CHECK: check for invalid expressions
    wxString expr;
    int type, valid;
    bool type_mismatch = false;
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
        expr = getFieldExpression(row);
        
        valid = validateExpression(source_structure, expr, type);
        if (valid == StructureValidator::ExpressionValid)
            continue;
        
        if (valid == StructureValidator::ExpressionTypeMismatch)
        {
            type_mismatch = true;
            continue;
        }
        
        return StructureValidator::ErrorInvalidExpressions;
    }
    
    // CHECK: check for expression/field type mismatches
    if (type_mismatch)
        return StructureValidator::ErrorExpressionTypeMismatch;

    // clear rows that have exlamation mark icons in them
    clearProblemRows();

    // CHECK: check for duplicate and invalid field names
    int duplicatefields_errorcode = checkDuplicateFieldnames(
                                        CheckMarkRows | CheckEmptyFieldnames);
    int invalidfields_errorcode = checkInvalidFieldnames(
                                        CheckMarkRows | CheckEmptyFieldnames);
    
    if (duplicatefields_errorcode != StructureValidator::ErrorNone)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
        return duplicatefields_errorcode;
    }
    
    if (invalidfields_errorcode != StructureValidator::ErrorNone)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
        return invalidfields_errorcode;
    }

    m_grid->refreshColumn(kcl::Grid::refreshAll, colRowNumber);
    return StructureValidator::ErrorNone;
}


// this function encapsulates all of the logic/error checking
// for when we want to save a document or switch views
bool TransformationDoc::doErrorCheck()
{
    bool block = false;
    int errorcode = validateStructure();
    StructureValidator::showErrorMessage(errorcode, &block);
    
    // there is an error in the structure that must be fixed
    if (block)
        return false;
    
    return true;
}

tango::IStructurePtr TransformationDoc::createStructureFromGrid()
{
    // -- create the tango::IStructure --
    tango::IStructurePtr s = g_app->getDatabase()->createStructure();

    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        tango::IColumnInfoPtr col = s->createColumn();
        col->setName(towstr(m_grid->getCellString(row, colFieldName)));
        col->setType(choice2tango(m_grid->getCellComboSel(row, colFieldType)));
        col->setWidth(m_grid->getCellInteger(row, colFieldWidth));
        col->setScale(m_grid->getCellInteger(row, colFieldScale));
        col->setCalculated(isFieldDynamic(m_grid, row));
    }
    
    return s;
}

tango::IStructurePtr TransformationDoc::getSourceStructure()
{
    tango::IStructurePtr s;
    if (m_init_set.isOk())
    {
        tango::IFixedLengthDefinitionPtr fset = m_init_set;
        tango::IDelimitedTextSetPtr tset = m_init_set;
        if (fset)
            return fset->getSourceStructure();
         else if (tset)
            return tset->getSourceStructure();
    }
    
    return xcm::null;
}


// -- event handling --

void TransformationDoc::onInsertingRows(std::vector<int> rows)
{
    // set the dirty flag
    m_dirty = true;

    // if multiple rows are selected, clear the selection and
    // select only the newly inserted row
    if (rows.size() > 1)
    {
        m_grid->clearSelection();
        m_grid->setRowSelected(rows[0], true);
        m_grid->moveCursor(rows[0], m_grid->getCursorColumn());
    }
    
    // insert the selected rows
    insertSelectedRows(false /* dynamic */);
}

void TransformationDoc::onDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
    updateNumberColumn();
    updateStatusBar();
}

void TransformationDoc::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGING)
    {
        int id = (int)(evt.l_param);
        
        // -- make sure we are in the active container --
        IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        if (active_site.isNull() || m_doc_site.isNull())
            return;
        if (active_site->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
        
        if (id == ID_View_SwitchToLayoutView)
        {
            // if we are on transformation doc, we might need to prompt for saving
            IDocumentSitePtr tabledoc_site;

            tabledoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.TableDoc");
            active_site = g_app->getMainFrame()->getActiveChild();

            if (active_site == m_doc_site)
            {
                if (m_dirty)
                {
                    int result = appMessageBox(_("Would you like to save the changes made to the table's structure?"),
                                                    APPLICATION_NAME,
                                                    wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);
                    if (result == wxCANCEL)
                    {
                        // don't allow the view switcher to change
                        *(bool*)evt.l_param2 = false;
                        return;
                    }

                    if (result == wxYES)
                    {
                        if (!doSave())
                        {
                            // don't allow the view switcher to change
                            *(bool*)evt.l_param2 = false;
                            return;
                        }
                    }
                    
                    // revert the grid to the original structure
                    if (result == wxNO)
                    {
                        initFromSet(m_init_set);
                        updateNumberColumn();
                        checkOverlayText();
                        
                        // we'll refresh the grid below, after we've switched
                        // views, so we don't see any flicker
                    }
                }
                
                // if we don't have a tabledoc yet, we need to create one so
                // that we can switch to it (this is most likely the case
                // because we are creating a table)
                if (!tabledoc_site)
                {
                    // create a tabledoc and open it
                    ITableDocPtr doc = TableDocMgr::createTableDoc();
                    doc->open(m_init_set->getObjectPath());

                    if (doc->getCaption().Length() == 0)
                    {
                        //doc->setCaption(m_path, wxEmptyString);
                    }
                    
                    unsigned int site_type = sitetypeNormal;

                    IDocumentSitePtr doc_site;
                    doc_site = g_app->getMainFrame()->createSite(
                                                  m_doc_site->getContainerWindow(),
                                                  doc,
                                                  false);
                }
            }
        }
        
        switch (id)
        {
            case ID_View_SwitchToDesignView:
                switchToOtherDocument(m_doc_site, "appmain.TransformationDoc");
                return;
                
            case ID_View_SwitchToSourceView:
                switchToOtherDocument(m_doc_site, "appmain.TextDoc");
                
                // refresh the grid after we've completed the switch,
                // so we can avoid flicker
                m_grid->scrollToTop();
                m_grid->moveCursor(0, colFieldName, false);
                m_grid->clearSelection();
                if (m_grid->getRowCount() > 0)
                    m_grid->setRowSelected(0, true);
                m_grid->refresh(kcl::Grid::refreshAll);
                return;
                
            case ID_View_SwitchToLayoutView:
                switchToOtherDocument(m_doc_site, "appmain.TableDoc");
                
                // refresh the grid after we've completed the switch,
                // so we can avoid flicker
                m_grid->scrollToTop();
                m_grid->moveCursor(0, colFieldName, false);
                m_grid->clearSelection();
                if (m_grid->getRowCount() > 0)
                    m_grid->setRowSelected(0, true);
                m_grid->refresh(kcl::Grid::refreshAll);
                
                return;
                
            default:
                // something went wrong, do nothing
                return;
        }
    }
}

void TransformationDoc::onSize(wxSizeEvent& evt)
{
    Layout();
}

void TransformationDoc::onEraseBackground(wxEraseEvent& evt)
{

}

void TransformationDoc::onSelectAll(wxCommandEvent& evt)
{
    m_grid->selectAll();
}

void TransformationDoc::onCopy(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }

    AppBusyCursor c;
    m_grid->copySelection();
}

void TransformationDoc::onSave(wxCommandEvent& evt)
{
    if (m_dirty)
        doSave();
}

bool TransformationDoc::doSave()
{
    // make sure we're not editing the grid when we do the save
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // check the output structure to make sure it's valid
    if (!doErrorCheck())
        return false;

    // since we're not modifying width/scale based on type dynamically --
    // see note for updateWidthAndScale() -- we need to make sure they
    // are set in conformity with our structure standards now
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
        int width = -1;
        int scale = -1;
        
        if (type == tango::typeCharacter ||
            type == tango::typeWideCharacter)
        {
            scale = 0;
        }
        
        if (type == tango::typeDouble)
        {
            width = 8;
        }
        
        if (type == tango::typeDateTime)
        {
            width = 8;
            scale = 0;
        }
        
        if (type == tango::typeInteger ||
            type == tango::typeDate)
        {
            width = 4;
            scale = 0;
        }
        
        if (type == tango::typeBoolean)
        {
            width = 1;
            scale = 0;
        }
        
        if (width != -1)
            m_grid->setCellInteger(row, colFieldWidth, width);
        if (scale != -1)
            m_grid->setCellInteger(row, colFieldScale, scale);
    }

    tango::ISetPtr text_set;
    tango::IFixedLengthDefinitionPtr fset;
    tango::IDelimitedTextSetPtr tset;

    wxString name, source_name, expression;
    int type, width, scale, format_sel;

    ITextDocPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
    
    #ifdef _DEBUG
    if (textdoc.isNull())
    {
        appMessageBox(wxT("Debug Message from Ben: The bug that happens while closing text doc has occurred. Set your breakpoint."));
    }
    #else
    if (textdoc.isNull())
        return false;
    #endif
    
    if (textdoc.isOk())
    {
        // get the proper set from the TextDoc
        fset = text_set;
        tset = text_set;
        
        // get a structure that we can configure
        tango::IStructurePtr new_struct = textdoc->getStructure();
        tango::IColumnInfoPtr colinfo;
        
        // populate the new structure
        int row, row_count = m_grid->getRowCount();
        for (row = 0; row < row_count; ++row)
        {
            name = m_grid->getCellString(row, colFieldName);
            type = m_grid->getCellComboSel(row, colFieldType);
            width = m_grid->getCellInteger(row, colFieldWidth);
            scale = m_grid->getCellInteger(row, colFieldScale);
            source_name = m_grid->getCellString(row, colSourceName);
            format_sel = m_grid->getCellComboSel(row, colFieldFormula);
            expression = getFieldExpression(row);

            colinfo = new_struct->createColumn();
            colinfo->setName(towstr(name));
            colinfo->setType(choice2tango(type));
            colinfo->setWidth(width);
            colinfo->setScale(scale);
            colinfo->setExpression(towstr(expression));
        }

        // we're working on a fixed-length text set
        if (fset)
        {
            // purge the old destination structure
            tango::IStructurePtr s = fset->getDestinationStructure();
            
            int col, col_count = s->getColumnCount();
            for (col = 0; col < col_count; ++col)
            {
                colinfo = s->getColumnInfoByIdx(col);
                s->deleteColumn(colinfo->getName());
            }

            // remove all columns from the structure and repopulate it
            fset->modifyDestinationStructure(s, NULL);
            fset->modifyDestinationStructure(new_struct, NULL);
            
            // save the set configuration
            fset->saveConfiguration();
        }
        
        // we're working on a text-delimited set
        if (tset)
        {
            // purge the old destination structure
            tango::IStructurePtr s = tset->getDestinationStructure();
            tango::IColumnInfoPtr colinfo;
            
            int col, col_count = s->getColumnCount();
            for (col = 0; col < col_count; ++col)
            {
                colinfo = s->getColumnInfoByIdx(col);
                s->deleteColumn(colinfo->getName());
            }

            // remove all columns from the structure and repopulate it
            tset->modifyDestinationStructure(s, NULL);
            tset->modifyDestinationStructure(new_struct, NULL);
            
            // save the set configuration
            tset->saveConfiguration();
        }
        
        // store the updated set we used to initialize the grid
        m_init_set = text_set;
    }

    // update the TableDoc's view
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        // update the TableDoc's base set
        tabledoc->open(text_set->getObjectPath());
        
        ITableDocViewPtr tabledocview = tabledoc->getActiveView();
        if (tabledocview)
        {
            tango::IStructurePtr s;
            if (fset)
                s = fset->getDestinationStructure();
            if (tset)
                s = tset->getDestinationStructure();
            
            int i, col_count = s->getColumnCount();
            for (i = 0; i < col_count; ++i)
            {
                tango::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
                int viewidx = tabledocview->getColumnIdx(towx(colinfo->getName()));
                
                // only add the column to our view if it doesn't already exist
                if (viewidx == -1)
                {
                    // add the column to the TableDoc's view
                    ITableDocViewColPtr viewcol = tabledocview->createColumn(i);
                    viewcol->setName(colinfo->getName());
                    viewcol->setSize(80);
                }
            }
            
            // refresh the TableDoc's view
            tabledoc->refreshActiveView();

            // save the view information in the OFS
            ITableDocModelPtr model = tabledoc->getModel();
            if (model)
                model->writeObject(tabledocview);
        }
    }
    
    m_dirty = false;
    return true;
}

void TransformationDoc::onInsertField(wxCommandEvent& evt)
{
    // set the dirty flag
    m_dirty = true;

    // insert the selected rows
    insertSelectedRows(false /* dynamic */);
}

void TransformationDoc::onDeleteField(wxCommandEvent& evt)
{
    // set the dirty flag
    m_dirty = true;

    // delete the selected rows
    m_grid->deleteSelectedRows();
    checkOverlayText();
    updateNumberColumn();
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void TransformationDoc::onGridNeedTooltipText(kcl::GridEvent& evt)
{
    int row_count = m_grid->getRowCount();
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col != colRowNumber && col != colFieldName && col != colFieldFormula)
        return;
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, false /* don't check empty fieldnames */);
    StructureValidator::findDuplicateFieldNames(check_rows);
    StructureValidator::findInvalidFieldNames(check_rows);
    
    std::vector<RowErrorChecker>::iterator it;
    for (it = check_rows.begin(); it != check_rows.end(); ++it)
    {
        wxString msg;
        
        if (col == colRowNumber || col == colFieldName)
        {
            if (it->row == row && it->errors != StructureValidator::ErrorNone)
            {
                if (it->errors & StructureValidator::ErrorDuplicateFieldNames &&
                    it->errors & StructureValidator::ErrorInvalidFieldNames)
                {
                    msg = _("This field has the same name as another field\nin this list and has an invalid name");
                }
                 else
                {
                    if (it->errors & StructureValidator::ErrorDuplicateFieldNames)
                        msg = _("This field has the same name as another field in this list");
                     else if (it->errors & StructureValidator::ErrorInvalidFieldNames)
                        msg = _("This field has an invalid name");
                }
                
                evt.SetString(msg);
            }
        }
         else if (col == colFieldFormula)
        {
            if (it->row == row)
            {
                int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
                wxString expr = getFieldExpression(row);
                int res = validateExpression(getSourceStructure(), expr, type);
                
                if (res == StructureValidator::ExpressionTypeMismatch)
                    msg = _("This formula has a return type that does not match the field type");
                 else if (res == StructureValidator::ExpressionInvalid)
                    msg = _("This formula has invalid syntax");
                
                evt.SetString(msg);
            }
        }
    }
}

void TransformationDoc::onGridBeginEdit(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    // store the last selected field type for comparison later on
    if (col == colFieldType)
        m_last_selected_fieldtype = m_grid->getCellComboSel(row, colFieldType);

    // NOTE: Normally, we'd update the expression icon here, but because
    //       the edit control is a combobox, it draws itself over the where
    //       the icon would draw itself, so there's no point in drawing
    //       the icon until the edit is over
}

void TransformationDoc::onGridEndEdit(kcl::GridEvent& evt)
{
    int col = evt.GetColumn();
    int row = evt.GetRow();
    int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));

    m_last_selected_fieldtype = -1;

    if (col == colFieldName)
    {
        if (evt.GetEditCancelled())
            return;

        // this will force the cell's text to be the text that we just
        // entered (this is necessary because we use getCellString()
        // in the checkDuplicateFieldnames() function below and the
        // cell's text has not yet changed)
        m_grid->setCellString(row, colFieldName, evt.GetString());
        
        clearProblemRows();
        checkDuplicateFieldnames(CheckMarkRows);
        checkInvalidFieldnames(CheckMarkRows);
        m_grid->refreshColumn(kcl::Grid::refreshAll, colRowNumber);
    }
     else if (col == colFieldWidth)
    {
        if (evt.GetEditCancelled())
            return;

        // conform to max character field width
        if (type == tango::typeCharacter || type == tango::typeWideCharacter)
        {
            int width = evt.GetInt();
            if (width > tango::max_character_width)
            {
                evt.Veto();
                width = tango::max_character_width;
                m_grid->setCellInteger(row, colFieldWidth, width);
                m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldWidth);
            }

            if (width < 1)
            {
                evt.Veto();
                width = 1;
                m_grid->setCellInteger(row, colFieldWidth, width);
                m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldWidth);
            }
        }

        if (type == tango::typeNumeric)
        {
            int width = evt.GetInt();
            int scale = m_grid->getCellInteger(row, colFieldScale);

            // conform to max numeric field width
            if (width > tango::max_numeric_width)
            {
                evt.Veto();
                width = tango::max_numeric_width;
                m_grid->setCellInteger(row, colFieldWidth, width);
                m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldWidth);
            }
            
            // conform to min numeric field width
            if (width < 1)
            {
                evt.Veto();
                width = 1;
                m_grid->setCellInteger(row, colFieldWidth, width);
                m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldWidth);
            }
            
            // make sure scale doesn't exceed width
            if (width < scale)
            {
                scale = width-1;
                m_grid->setCellInteger(row, colFieldScale, scale);
                m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldScale);
            }
        }
    }
     else if (col == colFieldScale)
    {
        if (evt.GetEditCancelled())
            return;

        if (type == tango::typeDouble || type == tango::typeNumeric)
        {
            int scale = evt.GetInt();
            int width = m_grid->getCellInteger(row, colFieldWidth);

            // conform to max double/numeric field scale
            if (scale > tango::max_numeric_scale)
            {
                evt.Veto();
                scale = tango::max_numeric_scale;
                m_grid->setCellInteger(row, colFieldScale, scale);
                m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldScale);
            }
            
            // conform to min double/numeric field scale
            if (scale < 0)
            {
                evt.Veto();
                scale = 0;
                m_grid->setCellInteger(row, colFieldScale, scale);
                m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldScale);
            }
            
            // make sure scale doesn't exceed width
            if (width < scale)
            {
                evt.Veto();
                scale = width-1;
                m_grid->setCellInteger(row, colFieldScale, scale);
                m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldScale);
            }
        }
    }
     else if (col == colSourceName)
    {
        wxString source_name = evt.GetString();
        wxString field_name = m_grid->getCellString(row, colFieldName);
        wxString expr_text = m_grid->getCellString(row, colFieldFormula);
        
        if (source_name != EMPTY_SOURCENAME_STR)
        {
            // fill out an empty fieldname with the selected source name by default
            if (field_name.IsEmpty())
                m_grid->setCellString(row, colFieldName, source_name);
            
            // remove default double-quoted expression when selecting a source name
            if (expr_text == wxT("\"\""))
                m_grid->setCellString(row, colFieldFormula, wxEmptyString);
        }
    }
     else if (col == colFieldFormula)
    {
        wxString source_name = m_grid->getCellString(row, colSourceName);
        wxString expr = evt.GetString();
        int expr_combosel = evt.GetExtraLong();

        // if we can't match the cell's string to a combo selection
        // then we know we're typing in a custom expression
        if (!expr.IsEmpty() && expr_combosel == -1)
            m_grid->setCellString(row, colSourceName, EMPTY_SOURCENAME_STR);
        
        // if the expression is empty, construct it from the grid cells
        if (expr.IsEmpty())
            expr = getFieldExpression(row);
        
        if (expr_combosel == -1)
        {
            int res = validateExpression(getSourceStructure(), expr, type);
            updateExpressionIcon(row, res);
        }
         else
        {
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
        }
    }
    
    m_grid->refresh(kcl::Grid::refreshAll);
    m_dirty = true;
}

void TransformationDoc::onGridEditChange(kcl::GridEvent& evt)
{
    // set the dirty flag
    m_dirty = true;
    
    if (evt.GetEditCancelled())
    {
        m_last_selected_fieldtype = -1;
        return;
    }
    
    int col = evt.GetColumn();
    int row = evt.GetRow();

    if (col == colFieldType)
    {
        if (m_grid->getCellComboSel(row, colFieldFormula) == -1)
        {
            int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
            wxString expr = getFieldExpression(row);
        
            int res = validateExpression(getSourceStructure(), expr, type);
            updateExpressionIcon(row, res);
        }
         else
        {
            // this field doesn't have a custom expression,
            // so don't show any icon next to the expression
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
        }
        
        updateRowWidthAndScale(row);
        updateRowCellProps(row);
        m_grid->refresh(kcl::Grid::refreshAll);
        
        updateStatusBar();
        
        // continually update the last selected type so we can compare
        // the current field type selection with the last selection --
        // we need to do this so we know how to update the column's row width
        m_last_selected_fieldtype = m_grid->getCellComboSel(row, colFieldType);
    }
     else if (col == colSourceName)
    {
        int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
        wxString expr = getFieldExpression(row);
        
        // make sure either a source field or an expression is specified
        int res = validateExpression(getSourceStructure(), expr, type);
        updateExpressionIcon(row, res);
        m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldFormula);
    }
     else if (col == colFieldFormula)
    {
        int tango_type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
        int combo_idx  = evt.GetExtraLong();
        int format_idx = combo2formatIdx(tango_type, combo_idx);
        
        if (format_idx == charfmtOther ||
            format_idx == numfmtOther  ||
            format_idx == datefmtOther ||
            format_idx == boolfmtOther)
        {
            // if we selected an "Other..." entry, clear out the combobox
            // so the user can begin typing in a custom expression
            m_grid->setCellComboSel(row, colFieldFormula, -1);
            m_grid->setCellString(row, colFieldFormula, wxEmptyString);
            return;
        }
        
        // NOTE: Normally, we'd update the expression icon here, but because
        //       the edit control is a combobox, it draws itself over the where
        //       the icon would draw itself, so there's no point in drawing
        //       the icon until the edit is over
    }
}

void TransformationDoc::onGridCellRightClick(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (row < 0 || row >= m_grid->getRowCount() ||
        col < 0 || col >= m_grid->getColumnCount())
        return;

    // -- if the user clicked on a row that was not
    //    previously selected, select the row --
    if (!m_grid->isRowSelected(row))
    {
        m_grid->clearSelection();
        m_grid->setRowSelected(row, true);
        m_grid->refresh(kcl::Grid::refreshAll);
    }
    
    wxString insert_field_str = _("&Insert Field");
    
    int selected_row_count = m_grid->getSelectedRowCount();
    if (selected_row_count > 1)
        insert_field_str = _("&Insert Fields");
    
    wxMenu menuPopup;
    menuPopup.Append(ID_InsertField, insert_field_str);
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Edit_Copy, _("&Copy"));
    menuPopup.Append(ID_Edit_Delete, _("&Delete"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Edit_SelectAll, _("Select &All"));

    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);
    CommandCapture* cc = new CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);

    // -- post the event to the event handler --
    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, command);
    ::wxPostEvent(this, e);
}

void TransformationDoc::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();
    
    // make sure we're not editing the grid
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // set the dirty flag
    m_dirty = true;

    insertRow(-1);
    updateNumberColumn();
    int row = m_grid->getRowCount()-1;
    m_grid->moveCursor(row, evt.GetColumn(), false);
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void TransformationDoc::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();
    
    // make sure we're not editing the grid
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // set the dirty flag
    m_dirty = true;

    insertRow(-1);
    updateNumberColumn();
    int row = m_grid->getRowCount()-1;
    m_grid->moveCursor(row, evt.GetColumn(), false);
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void TransformationDoc::onToggleView(wxCommandEvent& evt)
{
    // make sure we're not editing the grid
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // don't allow the user to switch to another document view
    // if the output structure isn't valid
    if (!validateStructure())
        return;

    // pass the event through to other event handlers
    evt.Skip();
}

void TransformationDoc::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    wxDataObject* obj = drop_target->GetDataObject();
    if (!obj)
        return;
    
    // determine which type of data object was dropped
    kcl::GridDataObjectComposite* drop_data = (kcl::GridDataObjectComposite*)obj;
    wxDataFormat fmt = drop_data->GetReceivedFormat();
    
    // check to see if what type of data object we're dealing with
    if (drop_data->isGridData())
    {
        if (fmt.GetId() == kcl::getGridDataFormat(wxT("transformationdoc")))
        {
            // if we've reordered the rows, we need to renumber the rows
            drop_target->doRowDrag(false);
            updateNumberColumn();
            m_grid->refresh(kcl::Grid::refreshAll);
        }
         else if (fmt.GetId() == kcl::getGridDataFormat(wxT("fieldspanel")))
        {
            tango::IStructurePtr s = getTextSourceStructure(m_doc_site);
            if (s.isNull())
                return;
                
            tango::IColumnInfoPtr colinfo;
            
            int drop_row = drop_target->getDropRow();

            // dragging fields in from the field panel
            kcl::GridDraggedCells cells = drop_target->getDraggedCells();
            kcl::GridDraggedCells::iterator it;
            for (it = cells.begin(); it != cells.end(); ++it)
            {
                // only look at the first column from the fields panel
                if ((*it)->m_col != 0)
                    continue;
                
                // get the column info from the column name we dragged in
                colinfo = s->getColumnInfo(towstr((*it)->m_strvalue));
                if (colinfo.isNull())
                    continue;
                
                // we need to set the expression to the source column name
                // so that we know which source column this column refers to
                colinfo->setExpression(colinfo->getName());
                
                insertRowFromColumnInfo(drop_row, colinfo);
                drop_row++;
            }
            
            // if nothing was selected when we dropped the data,
            // select the row with the cursor in it now
            if (m_grid->getSelectionCount() == 0)
            {
                int cursor_row = m_grid->getCursorRow();
                m_grid->setRowSelected(cursor_row, true);
            }

            updateNumberColumn();
            checkDuplicateFieldnames(CheckMarkRows);
            checkInvalidFieldnames(CheckMarkRows);
            m_grid->refresh(kcl::Grid::refreshAll);
            updateStatusBar();
        }

        m_dirty = true;
    }
}

void TransformationDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void TransformationDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

void TransformationDoc::onUpdateUI(wxUpdateUIEvent& evt)
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
        id == ID_Edit_Paste)
    {
        evt.Enable(false);
        return;
    }
    
    // enable copy/delete if there's a selection
    if (id == ID_Edit_Copy ||
        id == ID_Edit_Delete)
    {
        if (m_grid->getSelectionCount() > 0)
            evt.Enable(true);
        else
            evt.Enable(false);
        return;
    }
    
    // disable find/replace (for now)
    if (id == ID_Edit_Find ||
        id == ID_Edit_Replace)
    {
        evt.Enable(false);
        return;
    }
    
    // disable find next/find prev
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

