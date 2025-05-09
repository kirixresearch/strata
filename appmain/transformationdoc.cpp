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
    colSourceOffset,
    colSourceWidth,
    colSourceEncoding,
    colFieldFormula
};
    
// storage class for source name and type format lookup

struct ExpressionLookupInfo
{
    int format;
    const wchar_t* expr;
    std::wregex* regex;

    ~ExpressionLookupInfo()
    {
        delete regex;
        regex = NULL;
    }
};

static ExpressionLookupInfo expr_lookup_arr[] =
{
    { charfmtTrimLeadingSpaces,          L"(( LTRIM(%s) ))", nullptr },
    { charfmtTrimLeadingZeros,           L"(( LTRIM(%s,'0') ))", nullptr },
    { charfmtTrimLeadingSpacesAndZeros,  L"(( LTRIM(%s,'0 ') ))", nullptr },
    { charfmtEmpty,                      L"(( %s ))", nullptr },
                                              
    { numfmtLeadingNegativeEuropean,     L"(( IIF(LTRIM(TRIM(TRIM(TRANSLATE(%s,'.',''))),'- ')=TRIM(TRANSLATE(%s,'.','')),VAL(TRIM(TRANSLATE(%s,'.',''))),-1*VAL(LTRIM(TRIM(TRANSLATE(%s,'.','')) , '- '))) ))", nullptr },
    { numfmtLeadingNegativeEnglish,      L"(( IIF(LTRIM(TRIM(TRIM(TRANSLATE(%s,',',''))),'- ')=TRIM(TRANSLATE(%s,',','')),VAL(TRIM(TRANSLATE(%s,',',''))),-1*VAL(LTRIM(TRIM(TRANSLATE(%s,',','')) , '- '))) ))", nullptr },
    { numfmtTrailingNegativeEuropean,    L"(( IIF(RTRIM(TRIM(TRIM(TRANSLATE(%s,'.',''))),'- ')=TRIM(TRANSLATE(%s,'.','')),VAL(TRIM(TRANSLATE(%s,'.',''))),-1*VAL(RTRIM(TRIM(TRANSLATE(%s,'.','')),'- '))) ))", nullptr },
    { numfmtTrailingNegativeEnglish,     L"(( IIF(RTRIM(TRIM(TRIM(TRANSLATE(%s,',',''))),'- ')=TRIM(TRANSLATE(%s,',','')),VAL(TRIM(TRANSLATE(%s,',',''))),-1*VAL(RTRIM(TRIM(TRANSLATE(%s,',','')),'- '))) ))", nullptr },
    { numfmtParenthesisNegativeEuropean, L"(( IIF(TRIM(TRIM(TRANSLATE(%s,'.','')),'() ')=TRIM(TRANSLATE(%s,'.','')),VAL(TRIM(TRANSLATE(%s,'.',''))),-1*VAL(TRIM(TRIM(TRANSLATE(%s,'.','')),'() '))) ))", nullptr },
    { numfmtParenthesisNegativeEnglish,  L"(( IIF(TRIM(TRIM(TRANSLATE(%s,',','')),'() ')=TRIM(TRANSLATE(%s,',','')),VAL(TRIM(TRANSLATE(%s,',',''))),-1*VAL(TRIM(TRIM(TRANSLATE(%s,',','')),'() '))) ))", nullptr },
    { numfmtEmpty,                       L"(( VAL(TRIM(%s)) ))", nullptr },
                                              
    { datefmtYYYYMMDD,                   L"(( DATE(TRIM(%s),'YYYYMMDD') ))", nullptr },
    { datefmtYYYYDDMM,                   L"(( DATE(TRIM(%s),'YYYYDDMM') ))", nullptr },
    { datefmtMMDDYYYY,                   L"(( DATE(TRIM(%s),'MMDDYYYY') ))", nullptr },
    { datefmtDDMMYYYY,                   L"(( DATE(TRIM(%s),'DDMMYYYY') ))", nullptr },
    { datefmtYYMMDD,                     L"(( DATE(TRIM(%s),'YYMMDD') ))", nullptr },
    { datefmtYYDDMM,                     L"(( DATE(TRIM(%s),'YYDDMM') ))", nullptr },
    { datefmtMMDDYY,                     L"(( DATE(TRIM(%s),'MMDDYY') ))", nullptr },
    { datefmtDDMMYY,                     L"(( DATE(TRIM(%s),'DDMMYY') ))", nullptr },
    { datefmtYYYYDDD,                    L"(( DATE(VAL(SUBSTR(TRIM(%s),1,4)),1,1)+VAL(SUBSTR(TRIM(%s),5,3))-1 ))", nullptr },
    { datefmtDDDYYYY,                    L"(( DATE(VAL(SUBSTR(TRIM(%s),4,4)),1,1)+VAL(SUBSTR(TRIM(%s),1,3))-1 ))", nullptr },
    { datefmtJulian,                     L"(( DATE(0,0,0)+VAL(TRIM(%s))+1 ))", nullptr },
    { datefmtEmpty,                      L"(( DATE(TRIM(%s)) ))", nullptr },
                                              
    { boolfmtTF,                         L"(( UPPER(TRIM(%s))='T' ))", nullptr },
    { boolfmtTrueFalse,                  L"(( UPPER(TRIM(%s))='TRUE' ))", nullptr },
    { boolfmtYN,                         L"(( UPPER(TRIM(%s))='Y' ))", nullptr },
    { boolfmtYesNo,                      L"(( UPPER(TRIM(%s))='YES' ))", nullptr },
    { boolfmt10,                         L"(( VAL(TRIM(%s))=1 ))", nullptr },
    { boolfmtNot00,                      L"(( VAL(TRIM(%s))!=0 ))", nullptr },
    { boolfmtEmpty,                      L"(( VAL(TRIM(%s))=1 OR TRIM(%s)='T' OR TRIM(%s)='t' OR UPPER(TRIM(%s))='TRUE' OR UPPER(TRIM(%s))='YES' OR TRIM(%s)='Y' OR TRIM(%s)='y' ))", nullptr },
};

// utility functions

static int format2comboIdx(int xd_type, int format_idx)
{
    switch (xd_type)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
            return (format_idx-charfmtBegin);
        
        case xd::typeNumeric:
        case xd::typeInteger:
        case xd::typeDouble:
            return (format_idx-numfmtBegin);
        
        case xd::typeDateTime:
        case xd::typeDate:
            return (format_idx-datefmtBegin);
        
        case xd::typeBoolean:
            return (format_idx-boolfmtBegin);
            
        default:
            return -1;
    }
    
    return -1;
}

static int combo2formatIdx(int xd_type, int combo_idx)
{
    switch (xd_type)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
            return (combo_idx == -1) ? charfmtEmpty : (charfmtBegin+combo_idx);
        
        case xd::typeNumeric:
        case xd::typeInteger:
        case xd::typeDouble:
            return (combo_idx == -1) ? numfmtEmpty : (numfmtBegin+combo_idx);
        
        case xd::typeDateTime:
        case xd::typeDate:
            return (combo_idx == -1) ? datefmtEmpty : (datefmtBegin+combo_idx);
        
        case xd::typeBoolean:
            return (combo_idx == -1) ? boolfmtEmpty : (boolfmtBegin+combo_idx);
            
        default:
            return -1;
    }
    
    return -1;
}

enum EncodingComboIndexes
{
    comboUndefined = 0,
    comboASCII,
    comboEBCDIC,
    comboCOMP,
    comboCOMP3
};

wxString choice2encodingLabel(int choice_idx)
{
    switch (choice_idx)
    {
        case comboUndefined:  return _("Default");
        case comboASCII:      return _("ASCII");
        case comboEBCDIC:     return _("EBCDIC");
        case comboCOMP:       return _("COMP");
        case comboCOMP3:      return _("COMP-3");
    }
    return _("Undefined");
}

inline int choice2xdencoding(int choice_idx)
{
    switch (choice_idx)
    {
        case comboUndefined:  return xd::encodingUndefined;
        case comboASCII:      return xd::encodingASCII;
        case comboEBCDIC:     return xd::encodingEBCDIC;
        case comboCOMP:       return xd::encodingCOMP;
        case comboCOMP3:      return xd::encodingCOMP3;
    }

    return xd::encodingUndefined; // Return undefined if choice not found
}

inline int xdencoding2choice(int encoding)
{
    switch (encoding)
    {
        case xd::encodingInvalid:
        case xd::encodingUndefined: return comboUndefined;
        case xd::encodingASCII:     return comboASCII;
        case xd::encodingEBCDIC:    return comboEBCDIC;
        case xd::encodingCOMP:      return comboCOMP;
        case xd::encodingCOMP3:     return comboCOMP3;
    }

    return comboASCII; // Default to ASCII if encoding not found
}




static std::wstring expr2regex(const std::wstring& expr)
{
    // even though it is a little slower to convert to wxString and back,
    // there are far more string translation functions to help us

    wxString e = expr;

    e.Replace("(", "\\(", true);
    e.Replace(")", "\\)", true);
    e.Replace("+", "\\+", true);
    e.Replace("*", "\\*", true);
    e.Replace("=", "\\=", true);
    e.Replace("\"", "~~~QUOTE~~~", true);
    e.Replace("'", "~~~QUOTE~~~", true);
    e.Replace("~~~QUOTE~~~", "[\"']", true);
    e.Replace("%s", "([^,\"]+)", true);

    return towstr(e);
}
    
static bool isFieldCalculated(kcl::Grid* grid, int row)
{
    return false;
    /*
    ransformField* f = (ransformField*)grid->getRowData(row);
    if (!f)
        return false;

    return f->calculated;
    */
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


TransformationDoc::TransformationDoc(xd::FormatDefinition& def)
    : m_def(def)
{
    m_dirty = false;
    m_grid = NULL;
    m_last_selected_fieldtype = -1;
    
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


void TransformationDoc::resizeAllGridColumnsToFitDoc()
{
    // this code changes the two proportional columns to be non-proportionally
    // sized, which will make the grid fill the client window size initially,
    // but still allow the user to resize each column

    m_grid->createDefaultView();

    int tiny_x = 0, tiny_y = 0;
    m_grid->getTextExtent("XXXXX", &tiny_x, &tiny_y);

    int small_x=0, small_y=0;
    m_grid->getTextExtent("DecimalsX", &small_x, &small_y);

    int medium_x=0, medium_y=0;
    m_grid->getTextExtent("Source EncodingX", &medium_x, &medium_y);

    m_grid->setColumnSize(colRowNumber, tiny_x);
    m_grid->setColumnSize(colFieldName, 100);
    m_grid->setColumnSize(colFieldType, medium_x);
    m_grid->setColumnSize(colFieldWidth, small_x);
    m_grid->setColumnSize(colFieldScale, small_x);
    m_grid->setColumnSize(colSourceOffset, medium_x);
    m_grid->setColumnSize(colSourceWidth, medium_x);
    m_grid->setColumnSize(colSourceEncoding, medium_x);
    m_grid->setColumnSize(colFieldFormula, 200);

    if (m_def.format == xd::formatFixedLengthText)
    {
    }
    else
    {
        m_grid->hideColumn(colSourceEncoding);
        m_grid->hideColumn(colSourceWidth);
        m_grid->hideColumn(colSourceOffset);
    }

    int colidxRowNumber = m_grid->getColumnViewIdx(colRowNumber);
    int colidxFieldName = m_grid->getColumnViewIdx(colFieldName);
    int colidxFieldType = m_grid->getColumnViewIdx(colFieldType);
    int colidxFieldWidth = m_grid->getColumnViewIdx(colFieldWidth);
    int colidxFieldScale = m_grid->getColumnViewIdx(colFieldScale);
    int colidxSourceOffset = m_grid->getColumnViewIdx(colSourceOffset);
    int colidxSourceWidth = m_grid->getColumnViewIdx(colSourceWidth);
    int colidxSourceEncoding = m_grid->getColumnViewIdx(colSourceEncoding);
    int colidxFieldFormula = m_grid->getColumnViewIdx(colFieldFormula);


    int w, h;
    m_grid->GetClientSize(&w, &h);
    w -= colidxRowNumber >= 0 ? m_grid->getColumnSize(colidxRowNumber) : 0;
    w -= colidxFieldType >= 0 ? m_grid->getColumnSize(colFieldType) : 0;
    w -= colidxFieldWidth >= 0 ? m_grid->getColumnSize(colFieldWidth) : 0;
    w -= colidxFieldScale >= 0 ? m_grid->getColumnSize(colFieldScale) : 0;
    w -= colidxSourceOffset >= 0 ? m_grid->getColumnSize(colSourceOffset) : 0;
    w -= colidxSourceWidth >= 0 ? m_grid->getColumnSize(colSourceWidth) : 0;
    w -= colidxSourceEncoding >= 0 ? m_grid->getColumnSize(colSourceEncoding) : 0;
    m_grid->setColumnSize(colidxFieldName, w * 30 / 100);
    m_grid->setColumnSize(colidxFieldFormula, w * 70 / 100);


    // refresh the row selection grid
    m_grid->moveCursor(0, colFieldName);
    m_grid->refresh(kcl::Grid::refreshAll);
}

bool cell_render_hook(kcl::Grid* grid, int row, int col, int model_col, kcl::CellData& cell_data)
{
    if (model_col == colFieldFormula)
    {
        if (cell_data.m_strvalue == "")
        {
            cell_data.m_strvalue = _("None");
            cell_data.fgcolor = *wxLIGHT_GREY;
        }

        return true;
    }

    return false;
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
    wxString caption = _("Structure");
    m_doc_site->setCaption(caption);
    m_doc_site->setBitmap(GETBMPSMALL(gf_table));

    Hide();     // prevent flicker when creating TextDoc

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

    wxFont overlay_font = wxFont(16, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false);

    // create transformation grid
    m_grid = new kcl::RowSelectionGrid(this,
                                       -1,
                                       wxPoint(0,0),
                                       docsite_wnd->GetClientSize(),
                                       wxBORDER_NONE,
                                       false,
                                       false);
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
    m_grid->createModelColumn(colSourceOffset, _("Source Offset"), kcl::Grid::typeInteger, 10, 0);
    m_grid->createModelColumn(colSourceWidth, _("Source Width"), kcl::Grid::typeInteger, 5, 0);
    m_grid->createModelColumn(colSourceEncoding, _("Source Encoding"), kcl::Grid::typeCharacter, 20, 0);
    m_grid->createModelColumn(colFieldFormula, _("Format/Formula"), kcl::Grid::typeCharacter, 4096, 0);
    m_grid->createDefaultView();

    m_grid->setCellRenderHook(cell_render_hook);

    // set properties for row number field
    {
        kcl::CellProperties props;
        props.mask = kcl::CellProperties::cpmaskEditable |
            kcl::CellProperties::cpmaskAlignment |
            kcl::CellProperties::cpmaskBitmapAlignment;
        props.editable = false;
        props.alignment = kcl::Grid::alignRight;
        props.bitmap_alignment = kcl::Grid::alignRight;
        m_grid->setModelColumnProperties(colRowNumber, &props);
    }
    
    // set cell properties for field type choice control
    {
        kcl::CellProperties props;
        props.mask = kcl::CellProperties::cpmaskCtrlType |
                     kcl::CellProperties::cpmaskCbChoices;
        props.ctrltype = kcl::Grid::ctrltypeDropList;
        props.cbchoices.push_back(xdtype2text(xd::typeCharacter));
        props.cbchoices.push_back(xdtype2text(xd::typeWideCharacter));
        props.cbchoices.push_back(xdtype2text(xd::typeNumeric));
        props.cbchoices.push_back(xdtype2text(xd::typeDouble));
        props.cbchoices.push_back(xdtype2text(xd::typeInteger));
        props.cbchoices.push_back(xdtype2text(xd::typeDate));
        props.cbchoices.push_back(xdtype2text(xd::typeDateTime));
        props.cbchoices.push_back(xdtype2text(xd::typeBoolean));
        m_grid->setModelColumnProperties(colFieldType, &props);
    }


    // set cell properties for the encoding choice control
    {
        kcl::CellProperties props;
        props.mask = kcl::CellProperties::cpmaskCtrlType |
                     kcl::CellProperties::cpmaskCbChoices;
        props.ctrltype = kcl::Grid::ctrltypeDropList;

        props.cbchoices.push_back(choice2encodingLabel(comboUndefined));
        props.cbchoices.push_back(choice2encodingLabel(comboASCII));
        props.cbchoices.push_back(choice2encodingLabel(comboEBCDIC));
        props.cbchoices.push_back(choice2encodingLabel(comboCOMP));
        props.cbchoices.push_back(choice2encodingLabel(comboCOMP3));

        m_grid->setModelColumnProperties(colSourceEncoding, &props);
    }
   

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_grid, 1, wxEXPAND);
    SetSizer(main_sizer);
    Layout();

    // fits all of the grid's columns to the size of the document
    resizeAllGridColumnsToFitDoc();

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
    
    Show();     // prevent flicker when creating TextDoc

    return true;
}

wxString TransformationDoc::getDocumentLocation()
{
    IDocumentPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
    if (textdoc)
        return textdoc->getDocumentLocation();

    return "";
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
    {
        resizeAllGridColumnsToFitDoc();
        updateStatusBar();
    }
}

void TransformationDoc::populate()
{
    const xd::FormatDefinition& def = m_def;

    std::vector<xd::ColumnInfo>::const_iterator cit;

    // clear the grid
    m_grid->deleteAllRows();
    
    // populate the grid from the table's destination structure
    int row = 0;

    for (cit = def.columns.cbegin(); cit != def.columns.cend(); ++cit)
    {
        insertRowFromColumnInfo(row, *cit);
        row++;
    }
    
    updateNumberColumn();

    resizeAllGridColumnsToFitDoc();

    // refresh the row selection grid
    if (m_grid->getRowCount() > 0)
    {
        m_grid->setRowSelected(0, true);
        m_grid->moveCursor(0, colFieldName);
    }
    m_grid->refresh(kcl::Grid::refreshAll);

    // update the status bar
    updateStatusBar();

    m_dirty = false;
}

void TransformationDoc::close()
{
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
}

void TransformationDoc::checkOverlayText()
{
}

void TransformationDoc::insertRow(int row, bool calculated)
{
    if (row == -1)
        row = m_grid->getRowCount();
    
    int xd_type = xd::typeCharacter;

    m_grid->insertRow(row);
    m_grid->setCellBitmap(row, colRowNumber, calculated ? GETBMP(gf_lightning_16) : GETBMP(gf_blank_16));
    m_grid->setCellComboSel(row, colFieldType, xdtype2choice(xd_type));
    m_grid->setCellString(row, colFieldName, "");
    m_grid->setCellInteger(row, colFieldWidth, 20);
    m_grid->setCellInteger(row, colFieldScale, 0);
    m_grid->setCellString(row, colFieldFormula, "");
    m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_blank_16));

    // make sure either a source field or an expression is specified
    int valid_res = validateExpression(getSourceStructure(), "", xd_type);
    updateExpressionIcon(row, valid_res);

    updateRowCellProps(row);
    checkOverlayText();
}

void TransformationDoc::insertSelectedRows(bool calculated)
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
            insertRow(start_row, calculated);
        }
    }
    
    // the grid is empty, insert a starter row
    if (sel_count == 0 && m_grid->getRowCount() == 0)
    {
        insertRow(0, calculated);
        m_grid->setRowSelected(0, true);
    }

    updateNumberColumn();
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void TransformationDoc::insertRowFromColumnInfo(int row, const xd::ColumnInfo& colinfo)
{
    if (row == -1)
        row = m_grid->getRowCount();
    
    m_grid->insertRow(row);
    m_grid->setCellString(row, colFieldName, colinfo.name);
    m_grid->setCellComboSel(row, colFieldType, xdtype2choice(colinfo.type));
    m_grid->setCellInteger(row, colFieldWidth, colinfo.width);
    m_grid->setCellInteger(row, colFieldScale, colinfo.scale);

    m_grid->setCellInteger(row, colSourceOffset, colinfo.source_offset);
    m_grid->setCellInteger(row, colSourceWidth, colinfo.source_width);
    m_grid->setCellComboSel(row, colSourceEncoding, xdencoding2choice(colinfo.source_encoding));

    if (/*caculated*/ false)
        m_grid->setCellBitmap(row, colRowNumber, GETBMP(gf_lightning_16));
     else
        m_grid->setCellBitmap(row, colRowNumber, GETBMP(gf_blank_16));
    
    // we need to update the row's cell props here so that the expression
    // cell's combobox is populated before we set its cell info
    updateRowCellProps(row);
    

    wxString expr = colinfo.expression;
    wxString source_name;
    int format_comboidx;

    //expr.Replace(wxString::Format(L"rawvalue(%d)", (row + 1)), wxString::Format(L"field(%d)", (row + 1)));
    //expr.Replace(wxString::Format(L"field(%d)", (row + 1)), colinfo.name);

    bool res = getInfoFromDestinationExpression(expr,
                                                colinfo.type,
                                                &source_name,
                                                &format_comboidx);
    if (!res)
    {
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_blank_16));
        m_grid->setCellString(row, colFieldFormula, expr);
        int valid_res = StructureValidator::ErrorNone;
        if (expr.Length() > 0)
        {
            valid_res = validateExpression(m_def.columns, expr, colinfo.type);
            updateExpressionIcon(row, valid_res);
        }
    }
     else
    {
        // we found a match, so populate the cells with the matched info
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_blank_16));
        m_grid->setCellComboSel(row, colFieldFormula, format_comboidx);
    }
    
    // get (or create) the field's expression
    //wxString field_expr = getFieldExpression(row);
    
    /*
    // make sure either a source field or an expression is specified
    int valid_res = validateExpression(validate_against, field_expr, f->output_type);
    updateExpressionIcon(row, valid_res);
    */

    checkOverlayText();
}

void TransformationDoc::updateRowCellProps(int row)
{
    int type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));
    
    bool width_editable = true;
    bool decimal_editable = true;

    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskCtrlType |
                     kcl::CellProperties::cpmaskCbChoices;
    cellprops.ctrltype = kcl::Grid::ctrltypeComboBox;

    // determine if the width and decimal cells are editable
    switch (type)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
            decimal_editable = false;
            cellprops.cbchoices = m_char_format_choices;
            break;
            
        case xd::typeNumeric:
            cellprops.cbchoices = m_numeric_format_choices;
            break;

        case xd::typeInteger:
            width_editable = false;
            decimal_editable = false;
            cellprops.cbchoices = m_numeric_format_choices;
            break;
            
        case xd::typeDouble:
            width_editable = false;
            cellprops.cbchoices = m_numeric_format_choices;
            break;

        case xd::typeDateTime:
        case xd::typeDate:
            width_editable = false;
            decimal_editable = false;
            cellprops.cbchoices = m_date_format_choices;
            break;

        case xd::typeBoolean:
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
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_blank_16));
    }
     else
    {
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_blank_16));
    }
}


void TransformationDoc::updateStatusBar()
{
    // if the grid hasn't been created yet, bail out
    if (!m_grid)
        return;

    // field count
    int row, row_count = m_grid->getRowCount();
    
    // row width
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
    {
        item->setValue(field_count_str);
    }

    item = m_frame->getStatusBar()->getItem(wxT("transformationdoc_row_width"));
    if (item.isOk())
    {
        item->setValue(row_width_str);
    }

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}

wxString TransformationDoc::createDestinationExpression(int row)
{
    int xd_type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));
    int format_comboidx = m_grid->getCellComboSel(row, colFieldFormula);
    std::wstring source_name = L"";
   
    if (format_comboidx == -1 && source_name == L"")
    {
        return "";
    }

    std::wstring quoted_source_name = kl::stdswprintf(L"field(%d)", row+1);

    // translate from the combobox index and xd type
    // to the expression format index
    int expr_format = combo2formatIdx(xd_type, format_comboidx);
    wxString retval;
    
    // we couldn't find the expression index for the lookup below, so
    // we'll just use the source name of the field as the expression
    if (expr_format == -1)
        return quoted_source_name;
    
    // lookup the expression in our lookup array
    int i, count = sizeof(expr_lookup_arr) / sizeof(ExpressionLookupInfo);
    for (i = 0; i < count; ++i)
    {
        if (expr_lookup_arr[i].format == expr_format)
        {
            retval = expr_lookup_arr[i].expr;
            retval.Replace(wxT("%s"), quoted_source_name, true);
            return retval;
        }
    }

    // something went wrong, return the source name as the expression
    return quoted_source_name;
}

static bool isSpecialExpression(const wxString& expression)
{
    bool is_special = expression.Length() >= 3 &&
                      expression.Mid(0, 3) == wxT("(( ") && 
                      expression.Mid(expression.Length()-3, 3) == wxT(" ))");
    return is_special;
}

bool TransformationDoc::getInfoFromDestinationExpression(
                                          const wxString& expression,
                                          int xd_type,
                                          wxString* source_name,
                                          int* format_comboidx)
{
    if (!isSpecialExpression(expression))
    {
        // this is safe to do since the source name cell in the grid
        // is a wxChoice cell, so if the expression isn't a source name,
        // the cell's wxChoice index will show up as -1
        *source_name = expression;
        *format_comboidx = -1;
        return false;
    }

    std::wstring wexpression = towstr(expression);

    // create the std::wregex for the array entry if it doesn't exist
    int i, count = sizeof(expr_lookup_arr)/sizeof(ExpressionLookupInfo);
    for (i = 0; i < count; ++i)
    {
        if (expr_lookup_arr[i].regex == NULL)
        {
            std::wstring regex_str = expr2regex(expr_lookup_arr[i].expr);
            expr_lookup_arr[i].regex = new std::wregex(regex_str);
        }
    }

    // now, try to match the expression using the regex
    std::wsmatch matchres;
    for (i = 0; i < count; ++i)
    {
        if (std::regex_match(wexpression, matchres, *expr_lookup_arr[i].regex))
        {
            // fill out the source name
            if (matchres.size() > 1)
            {
                *source_name = matchres[1].str();
            }
            source_name->Trim(true);
            source_name->Trim(false);
            
            // fill out the format combobox's index
            *format_comboidx = format2comboIdx(xd_type,
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


void TransformationDoc::clearProblemRows()
{
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        if (isFieldCalculated(m_grid, row))
            m_grid->setCellBitmap(row, colRowNumber, GETBMP(gf_lightning_16));
         else
            m_grid->setCellBitmap(row, colRowNumber, GETBMP(gf_blank_16));
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

int TransformationDoc::validateExpression(const xd::Structure& structure, const wxString& expr, int type)
{
    if (structure.isNull())
        return StructureValidator::ErrorInvalidStructure;

    return StructureValidator::validateExpression(structure, expr, type);
}

int TransformationDoc::validateStructure()
{
    // CHECK: check for empty structure
    if (m_grid->getRowCount() == 0)
        return StructureValidator::ErrorNoFields;

    xd::Structure source_structure = getSourceStructure();
    
    // CHECK: check for invalid expressions
    wxString expr;
    int type, valid;
    bool type_mismatch = false;
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));
        expr = getFieldExpression(row);

        if (expr.Length() == 0)
            continue;

        xd::Structure source_structure_without_own_fieldname = source_structure;
        wxString field_name = m_grid->getCellString(row, colFieldName);

        // don't allow the field to use its own column name (to avoid circular references)
        source_structure_without_own_fieldname.deleteColumn(field_name.ToStdWstring());

        valid = validateExpression(source_structure_without_own_fieldname, expr, type);
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

xd::Structure TransformationDoc::createStructureFromGrid()
{
    xd::Structure s;

    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        xd::ColumnInfo col;

        col.name = towstr(m_grid->getCellString(row, colFieldName));
        col.type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));
        col.width = m_grid->getCellInteger(row, colFieldWidth);
        col.scale = m_grid->getCellInteger(row, colFieldScale);
        col.calculated = isFieldCalculated(m_grid, row);

        s.createColumn(col);
    }
    
    return s;
}

xd::Structure TransformationDoc::getSourceStructure()
{
    return createStructureFromGrid();
}


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
    insertSelectedRows(false /* calculated */);
}

void TransformationDoc::onDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
    updateNumberColumn();
    updateStatusBar();
    m_dirty = true;
}

void TransformationDoc::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGING)
    {
        int id = (int)(evt.l_param);
        
        // make sure we are in the active container
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
                // if we're editing the grid, end the edit
                if (m_grid->isEditing())
                    m_grid->endEdit(true);

                if (!doErrorCheck())
                {
                    // don't allow the view switcher to change
                    *(bool*)evt.o_param = false;
                    return;
                }

                if (m_dirty)
                {
                    int result = appMessageBox(_("Would you like to save the changes made to the table's structure?"),
                                                    APPLICATION_NAME,
                                                    wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);
                    if (result == wxCANCEL)
                    {
                        // don't allow the view switcher to change
                        *(bool*)evt.o_param = false;
                        return;
                    }

                    if (result == wxYES)
                    {
                        if (!doSave())
                        {
                            // don't allow the view switcher to change
                            *(bool*)evt.o_param = false;
                            return;
                        }
                    }
                    
                    // revert the grid to the original structure
                    if (result == wxNO)
                    {
                        populate();
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
                /*
                    // TODO: implement

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
                */
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
    {
        doSave();
    }
     else
    {
        // give TextDoc a chance to save
        ITextDocPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
        if (textdoc)
        {
            textdoc->save(false);
        }
    }
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
        int type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));
        int width = -1;
        int scale = -1;
        
        if (type == xd::typeCharacter ||
            type == xd::typeWideCharacter)
        {
            scale = 0;
        }
        
        if (type == xd::typeDouble)
        {
            width = 8;
        }
        
        if (type == xd::typeDateTime)
        {
            width = 8;
            scale = 0;
        }
        
        if (type == xd::typeInteger ||
            type == xd::typeDate)
        {
            width = 4;
            scale = 0;
        }
        
        if (type == xd::typeBoolean)
        {
            width = 1;
            scale = 0;
        }
        
        if (width != -1)
            m_grid->setCellInteger(row, colFieldWidth, width);
        if (scale != -1)
            m_grid->setCellInteger(row, colFieldScale, scale);
    }


    ITextDocPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
    if (textdoc)
    {
        m_def.columns.clear();

        std::wstring name, expression;
        int type, width, scale, format_sel;
        int source_offset, source_width, source_encoding;


        // populate the new structure
        int row, row_count = m_grid->getRowCount();
        for (row = 0; row < row_count; ++row)
        {
            name = m_grid->getCellString(row, colFieldName);
            type = m_grid->getCellComboSel(row, colFieldType);
            width = m_grid->getCellInteger(row, colFieldWidth);
            scale = m_grid->getCellInteger(row, colFieldScale);
            source_offset = m_grid->getCellInteger(row, colSourceOffset);
            source_width = m_grid->getCellInteger(row, colSourceWidth);
            source_encoding = choice2xdencoding(m_grid->getCellComboSel(row, colSourceEncoding));
            format_sel = m_grid->getCellComboSel(row, colFieldFormula);
            expression = getFieldExpression(row);

            xd::ColumnInfo colinfo;
            colinfo.name = name;
            colinfo.type = choice2xdtype(type);
            colinfo.width = width;
            colinfo.scale = scale;
            colinfo.expression = expression;
            colinfo.source_offset = source_offset;
            colinfo.source_width = source_width;
            colinfo.source_encoding = source_encoding;

            m_def.createColumn(colinfo);
        }

        textdoc->save(false);
        textdoc->refreshFromDefinition();
        textdoc->refreshDocuments();
    }



    // update the TableDoc's view
    ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (tabledoc)
    {
        /* TODO: implement

        // update the TableDoc's base set
        tabledoc->open(text_set->getObjectPath());


        
        ITableDocViewPtr tabledocview = tabledoc->getActiveView();
        if (tabledocview)
        {
            xd::Structure s;
            if (fset)
                s = fset->getDestinationStructure();
            if (tset)
                s = tset->getDestinationStructure();
            
            int i, col_count = s->getColumnCount();
            for (i = 0; i < col_count; ++i)
            {
                const xd::ColumnInfo& colinfo = s->getColumnInfoByIdx(i);
                int viewidx = tabledocview->getColumnIdx(colinfo.name);
                
                // only add the column to our view if it doesn't already exist
                if (viewidx == -1)
                {
                    // add the column to the TableDoc's view
                    ITableDocViewColPtr viewcol = tabledocview->createColumn(i);
                    viewcol->setName(colinfo.name);
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

        */
    }
    
    m_dirty = false;
    return true;
}

void TransformationDoc::onInsertField(wxCommandEvent& evt)
{
    // set the dirty flag
    m_dirty = true;

    // insert the selected rows
    insertSelectedRows(false /* calculated */);
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
    xd::Structure source_structure;
    
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
            if (it->row == row && false)
            {
                int type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));
                wxString expr = getFieldExpression(row);

                if (source_structure.isNull())
                {
                    source_structure = getSourceStructure();
                }

                int res = validateExpression(source_structure, expr, type);
                
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
    int view_col = evt.GetColumn();
    int col = m_grid->getColumnModelIdx(view_col);
    
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
    int view_col = evt.GetColumn();
    int col = m_grid->getColumnModelIdx(view_col);
    int row = evt.GetRow();
    int type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));

    m_last_selected_fieldtype = -1;

    if (col == colFieldName)
    {
        if (evt.GetEditCancelled())
            return;

        if (m_grid->getCellString(row, colFieldName) == evt.GetString())
        {
            evt.Veto();
            return;
        }

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

        if (m_grid->getCellInteger(row, colFieldWidth) == evt.GetInt())
        {
            evt.Veto();
            return;
        }

        // conform to max character field width
        if (type == xd::typeCharacter || type == xd::typeWideCharacter)
        {
            int width = evt.GetInt();
            if (width > xd::max_character_width)
            {
                evt.Veto();
                width = xd::max_character_width;
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

        if (type == xd::typeNumeric)
        {
            int width = evt.GetInt();
            int scale = m_grid->getCellInteger(row, colFieldScale);

            // conform to max numeric field width
            if (width > xd::max_numeric_width)
            {
                evt.Veto();
                width = xd::max_numeric_width;
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

        if (m_grid->getCellInteger(row, colFieldScale) == evt.GetInt())
        {
            evt.Veto();
            return;
        }

        if (type == xd::typeDouble || type == xd::typeNumeric)
        {
            int scale = evt.GetInt();
            int width = m_grid->getCellInteger(row, colFieldWidth);

            // conform to max double/numeric field scale
            if (scale > xd::max_numeric_scale)
            {
                evt.Veto();
                scale = xd::max_numeric_scale;
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
     else if (col == colFieldFormula)
    {
        wxString expr = evt.GetString();
        int expr_combosel = evt.GetExtraLong();

        if (expr.Length() > 0 && expr_combosel == -1)
        {
            xd::Structure source_structure = getSourceStructure();
            wxString field_name = m_grid->getCellString(row, colFieldName);

            // don't allow the field to use its own column name (to avoid circular references)
            source_structure.deleteColumn(field_name.ToStdWstring());

            int res = validateExpression(source_structure, expr, type);
            updateExpressionIcon(row, res);
        }
         else
        {
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_blank_16));
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
    
    int view_col = evt.GetColumn();
    int col = m_grid->getColumnModelIdx(view_col);
    int row = evt.GetRow();

    if (col == colFieldType)
    {
        if (m_grid->getCellComboSel(row, colFieldFormula) == -1)
        {
            int type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));
            wxString expr = getFieldExpression(row);
            
            if (expr.Length() > 0)
            {
                xd::Structure source_structure = getSourceStructure();
                int res = validateExpression(source_structure, expr, type);
                updateExpressionIcon(row, res);
            }
            else
            {
                m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_blank_16));
            }
        }
         else
        {
            // this field doesn't have a custom expression,
            // so don't show any icon next to the expression
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_blank_16));
        }
        
        updateRowCellProps(row);
        m_grid->refresh(kcl::Grid::refreshAll);
        
        updateStatusBar();
        
        // continually update the last selected type so we can compare
        // the current field type selection with the last selection --
        // we need to do this so we know how to update the column's row width
        m_last_selected_fieldtype = m_grid->getCellComboSel(row, colFieldType);
    }
     else if (col == colFieldFormula)
    {
        int xd_type = choice2xdtype(m_grid->getCellComboSel(row, colFieldType));
        int combo_idx  = evt.GetExtraLong();
        int format_idx = combo2formatIdx(xd_type, combo_idx);
        
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

    // if the user clicked on a row that was not previously selected, select the row
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

    // post the event to the event handler 
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

