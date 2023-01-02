/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2003-04-09
 *
 */


#include "limits.h"
#include "appmain.h"
#include "appprint.h"
#include "appcontroller.h"
#include "dlgdatabasefile.h"
#include "dlgpagesetup.h"
#include "dlgreportprops.h"
#include "dbdoc.h"
#include "panelcolprops.h"
#include "panelcolordoc.h"
#include "querytemplate.h"
#include "reportdoc.h"

#include "../kcanvas/component.h"
#include "../kcanvas/componentimage.h"
#include "../kcanvas/componentpage.h"
#include "../kcanvas/componenttable.h"
#include "../kcanvas/canvashistory.h"
#include "../kcanvas/util.h"


#include <wx/tooltip.h>
#include <wx/renderer.h>
#include <wx/paper.h>
#include <wx/print.h>


enum
{
    ID_FormulaCustom = wxID_HIGHEST + 1,
    ID_FormulaPageNumber,
    ID_FormulaPageCount,
    ID_FormulaPagePageNumber,
    ID_FormulaPageNumberOfPageCount,
    ID_FormulaPagePageNumberOfPageCount,
    ID_FormulaCurrentDate,
    ID_FormulaDataSource 
};


/* XPM */
static const char* xpm_cursor_cross[] = {
"32 32 3 1",
"  c #000000",
"! c #C0C0C0",
"# c #FFFFFF",
"!!!!!!###!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"####### #######!!!!!!!!!!!!!!!!!",
"#             #!!!!!!!!!!!!!!!!!",
"####### #######!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!# #!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!###!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"};


// utility functions
static double in2mm(double in) { return (in*25.4); }
static double mm2in(double mm) { return (mm/25.4); }

static int in2model(double in) { return in*kcanvas::CANVAS_MODEL_DPI; }
static double model2in(int model) { return ((double)(model))/kcanvas::CANVAS_MODEL_DPI; }


// this art provider is exactly the same as wxAuiDefaultToolbarArt,
// with the exception that it doesn't draw a gradient background
class ReportDocFormatBarArt : public wxAuiDefaultToolBarArt
{
public:

    void DrawBackground(wxDC& dc,
                        wxWindow* wnd,
                        const wxRect& rect)
    {
        wxColor c = kcl::getBaseColor();
        dc.SetPen(c);
        dc.SetBrush(c);
        dc.DrawRectangle(rect);
    }
};


BEGIN_EVENT_TABLE(ReportDoc, wxWindow)

    EVT_MENU(ID_File_Save, ReportDoc::onSave)
    EVT_MENU(ID_File_SaveAs, ReportDoc::onSaveAs)
    EVT_MENU(ID_File_SaveAsExternal, ReportDoc::onSaveAsExternal)
    EVT_MENU(ID_File_Reload, ReportDoc::onReload)
    EVT_MENU(ID_File_Print, ReportDoc::onPrint)
    EVT_MENU(ID_File_PageSetup, ReportDoc::onPageSetup)
    
    EVT_MENU(ID_View_ZoomIn, ReportDoc::onZoomIn)
    EVT_MENU(ID_View_ZoomOut, ReportDoc::onZoomOut)
    EVT_MENU(ID_View_ZoomToActual, ReportDoc::onZoomToActual)
    EVT_MENU(ID_View_ZoomToWidth, ReportDoc::onZoomToWidth)
    EVT_MENU(ID_View_ZoomToFit, ReportDoc::onZoomToSize)

    EVT_COMBOBOX(ID_View_ZoomCombo, ReportDoc::onZoomChanged)
    EVT_TEXT_ENTER(ID_View_ZoomCombo, ReportDoc::onZoomChanged)

    EVT_MENU(ID_Edit_Undo, ReportDoc::onUndo)
    EVT_MENU(ID_Edit_Redo, ReportDoc::onRedo)
    EVT_MENU(ID_Edit_Cut, ReportDoc::onCut)
    EVT_MENU(ID_Edit_Copy, ReportDoc::onCopy)
    EVT_MENU(ID_Edit_Paste, ReportDoc::onPaste)
    EVT_MENU(ID_Edit_GoTo, ReportDoc::onGoTo)
    EVT_MENU(ID_Edit_SelectAll, ReportDoc::onSelectAll)
    EVT_MENU(ID_Edit_SelectNone, ReportDoc::onSelectNone)
    EVT_MENU(ID_Table_ClearCells, ReportDoc::onClearText)
    EVT_MENU(ID_Edit_Delete, ReportDoc::onClearText)

    EVT_COMBOBOX(ID_Format_FontFace_Combo, ReportDoc::onFontFaceChanged)
    EVT_TEXT_ENTER(ID_Format_FontFace_Combo, ReportDoc::onFontFaceChanged)

    EVT_COMBOBOX(ID_Format_FontSize_Combo, ReportDoc::onFontSizeChanged)
    EVT_TEXT_ENTER(ID_Format_FontSize_Combo, ReportDoc::onFontSizeChanged)
        
    EVT_MENU(ID_Format_Settings, ReportDoc::onReportSettings)
    EVT_MENU(ID_Format_ToggleWrapText, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_ToggleMergeCells, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_ToggleFontWeight, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_ToggleFontStyle, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_ToggleFontUnderline, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignTextLeft, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignTextCenter, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignTextRight, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignTop, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignMiddle, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignBottom, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_FillColor, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_TextColor, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_LineColor, ReportDoc::onFormatChanged)
    EVT_MENU(ID_Format_Border, ReportDoc::onFormatChanged)

    EVT_MENU(ID_Table_InsertColumns, ReportDoc::onInsertColumns)
    EVT_MENU(ID_Table_DeleteColumns, ReportDoc::onDeleteColumns)
    EVT_MENU(ID_Table_InsertRows, ReportDoc::onInsertRows)
    EVT_MENU(ID_Table_DeleteRows, ReportDoc::onDeleteRows)

    EVT_MENU(ID_Frame_ShowColumnList, ReportDoc::onShowColumnPanel)
    EVT_MENU(ID_Frame_ToggleFormatToolbar, ReportDoc::onToggleFormatToolbar)
    
    // map these events to AppController's event handlers
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_Format_TextColor, ReportDoc::onTextColorDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_Format_FillColor, ReportDoc::onFillColorDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_Format_LineColor, ReportDoc::onLineColorDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_Format_Border, ReportDoc::onBorderDropDown)

    EVT_SIZE(ReportDoc::onSize)
    EVT_SCROLLWIN(ReportDoc::onScroll)
    EVT_MOUSE_EVENTS(ReportDoc::onMouse)

    // disable data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, ReportDoc::onUpdateUI_DisableAlways)
    
    // disable some of the table items
    EVT_UPDATE_UI(ID_Table_InsertColumnBreak, ReportDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Table_RemoveColumnBreak, ReportDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Table_InsertGroupBreak, ReportDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Table_RemoveGroupBreak, ReportDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Table_EditViews, ReportDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Table_HideColumn, ReportDoc::onUpdateUI_DisableAlways)

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_Run, ReportDoc::onUpdateUI_DisableAlways)
    
    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, ReportDoc::onUpdateUI)

    // set formatting items based on conditions
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, ReportDoc::onUpdateUI)
    
END_EVENT_TABLE()



ReportDoc::ReportDoc()
{
    // format state
    m_format_halignment = wxT("");
    m_format_valignment = wxT("");
    m_format_fontfacename = wxT("");
    m_format_fontweight = wxT("");
    m_format_fontstyle = wxT("");
    m_format_fontunderscore = wxT("");
    m_format_fontsize = -1;
    m_format_cellsmerged = false;
    m_format_textwrapped = false;

    // mouse cursor variables
    wxImage image_cross = wxBitmap(xpm_cursor_cross).ConvertToImage();
    image_cross.SetMaskColour(192, 192, 192);
    image_cross.SetMask(true);
    image_cross.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 8);
    image_cross.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 8);

    // TODO: for some reason, there's an annoying flicker when
    // the cursor changes between cursors created with images 
    // and the stock cursors; for now, minimize this by using
    // some stock cursors in place of some image cursors for 
    // a few of the cursor types
    
    m_cursors[cursorInvalid] = wxNullCursor;
    m_cursors[cursorStandard] = *wxSTANDARD_CURSOR;
    m_cursors[cursorSizeNS] = wxCursor(wxCURSOR_SIZENS);
    m_cursors[cursorSizeWE] = wxCursor(wxCURSOR_SIZEWE);
    m_cursors[cursorSizeNWSE] = wxCursor(wxCURSOR_SIZENWSE);
    m_cursors[cursorSizeNESW] = wxCursor(wxCURSOR_SIZENESW);
    m_cursors[cursorMove] = wxCursor(wxCURSOR_SIZING);
    m_cursors[cursorMoveSelect] = wxCursor(wxCURSOR_SIZING);
    m_cursors[cursorTextEdit] = wxCursor(wxCURSOR_IBEAM);
    m_cursors[cursorTable] = *wxSTANDARD_CURSOR;
    m_cursors[cursorCross] = wxCursor(image_cross);

    // print progress listenter for job queue
    m_print_progress = NULL;

    // layout/design view toggle
    m_design_view = true;
    
    // page index for printing status
    m_printing_page_idx = 0;

    // saved zoom state
    m_last_layout_zoom = PROP_REPORT_DEFAULT_ZOOM;
    m_last_design_zoom = PROP_REPORT_DEFAULT_ZOOM;
    m_refresh_on_zoom = true;

    // flags to indicate whether or not the document has been 
    // created or changed, whether it's being edited, or
    // wether it's temporary (untitled)
    m_created = false;
    m_changed = false;
    m_editing = false;
    m_temporary = true;

    // flag to indicate whether or not changes are being
    // tracked; for undo/redo
    m_tracking = false;
}

ReportDoc::~ReportDoc()
{
}

// static
bool ReportDoc::newFile(const wxString& path)
{
    // note: parallels initComponents(), except without
    // canvas element

    // initialize design component
    kcanvas::IComponentPtr design_component = CompReportDesign::create();
    if (design_component.isNull())
        return false;

    // every report has a report header, report footer, page header,
    // page footer, and detail section; groups are added separately
    CompReportDesign* design_component_raw;
    design_component_raw = static_cast<CompReportDesign*>(design_component.p);
    design_component_raw->addSection(PROP_REPORT_HEADER, PROP_REPORT_HEADER, 2, false);
    design_component_raw->addSection(PROP_REPORT_PAGE_HEADER, PROP_REPORT_PAGE_HEADER, 2, true);
    design_component_raw->addSection(PROP_REPORT_DETAIL, PROP_REPORT_DETAIL, 6, true);
    design_component_raw->addSection(PROP_REPORT_PAGE_FOOTER, PROP_REPORT_PAGE_FOOTER, 2, true);
    design_component_raw->addSection(PROP_REPORT_FOOTER, PROP_REPORT_FOOTER, 2, false);

    // set the active section to the detail
    design_component_raw->setActiveSectionByName(PROP_REPORT_DETAIL);

    // set the default page settings based on the preferences
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isNull())
        return false;

    if (prefs->exists(wxT("report.page.width")))
    {
        long t = getAppPrefsLong(wxT("report.page.width"));
        int width_new = in2model((double)t/100);
        
        int width, height;
        design_component_raw->getPageSize(&width, &height);
        design_component_raw->setPageSize(width_new, height);
    }

    if (prefs->exists(wxT("report.page.height")))
    {
        long t = getAppPrefsLong(wxT("report.page.height"));
        int height_new = in2model((double)t/100);

        int width, height;
        design_component_raw->getPageSize(&width, &height);
        design_component_raw->setPageSize(width, height_new);
    }

    if (prefs->exists(wxT("report.margin.left")))
    {
        long t = getAppPrefsLong(wxT("report.margin.left"));
        int left_new = in2model((double)t/100);

        int left, right, top, bottom;
        design_component_raw->getPageMargins(&left, &right, &top, &bottom);
        design_component_raw->setPageMargins(left_new, right, top, bottom);
    }
    
    if (prefs->exists(wxT("report.margin.right")))
    {
        long t = getAppPrefsLong(wxT("report.margin.right"));
        int right_new = in2model((double)t/100);

        int left, right, top, bottom;
        design_component_raw->getPageMargins(&left, &right, &top, &bottom);
        design_component_raw->setPageMargins(left, right_new, top, bottom);
    }

    if (prefs->exists(wxT("report.margin.top")))
    {
        long t = getAppPrefsLong(wxT("report.margin.top"));
        int top_new = in2model((double)t/100);

        int left, right, top, bottom;
        design_component_raw->getPageMargins(&left, &right, &top, &bottom);
        design_component_raw->setPageMargins(left, right, top_new, bottom);
    }

    if (prefs->exists(wxT("report.margin.bottom")))
    {
        long t = getAppPrefsLong(wxT("report.margin.bottom"));
        int bottom_new = in2model((double)t/100);

        int left, right, top, bottom;
        design_component_raw->getPageMargins(&left, &right, &top, &bottom);
        design_component_raw->setPageMargins(left, right, top, bottom_new);
    }

    return design_component_raw->save(path);
}

bool ReportDoc::create(const ReportCreateInfo& data)
{
    // set the created/changed/editing/temporary flags
    m_created = true;
    m_changed = false;
    m_editing = false;
    m_temporary = true;

    // if the report generation data doesn't contain a
    // path, create a blank report in design view;
    // otherwise create a report from the specified
    // data and open it in layout view
    if (data.path.length() == 0)
    {
        // load a blank design view
        loadDesignView();
    }
    else
    {
        // set the created state to false, load the generation 
        // data and show the report in layout view
        m_created = false;
        m_changed = true;
        getDesignComponentRaw()->load(data);
        resetReportLayout();
        loadLayoutView();
    }

    // set the default zoom
    getCanvas()->setZoom(PROP_REPORT_DEFAULT_ZOOM);

    // update caption and status bar
    updateCaption();
    updateStatusBar();

    // set the initial focus component
    kcanvas::IComponentPtr focus;
    focus = getDesignComponentRaw()->getActiveSectionTable();
    getCanvas()->setFocus(focus);

    // layout and render the canvas
    updateCanvas();

    // update the format items
    updateFormatItems();

    // update the column list
    g_app->getMainFrame()->postEvent(new FrameworkEvent(FRAMEWORK_EVT_COLUMNLISTPANEL_UPDATE));

    return true;
}

bool ReportDoc::print(bool show_print_dialog)
{
    // find out the paper size
    wxPaperSize paper_size;
    int orientation;

    // find out the paper size
    int page_width, page_height;
    getPageSize(&page_width, &page_height);

    double width = ((double)page_width)/kcanvas::CANVAS_MODEL_DPI;
    double height = ((double)page_height)/kcanvas::CANVAS_MODEL_DPI;
    
    // get paper orientation; if the orientation is landscape,
    // then reverse the width and the height so that the
    // paper database can find the correct paper size
    orientation = wxPORTRAIT;
    if (width > height)
    {
        orientation = wxLANDSCAPE;
        double t = width;
        width = height;
        height = t;
    }
    
    // get paper size
    wxSize psize = wxSize(in2mm(width)*10.0,in2mm(height)*10.0);
    wxPrintPaperDatabase* paper_db = g_app->getPaperDatabase();
    wxPrintPaperType* paper_type = paper_db->FindPaperType(psize);
    if (!paper_type)
        paper_size = wxPAPER_NONE;
         else
        paper_size = paper_type->GetId();

    // set the print data
    wxPrintData printdata;
    printdata.SetOrientation(orientation);
    printdata.SetQuality(600);
    printdata.SetPaperId(paper_size);

    // get the page count; if the page count is zero, then
    // the layout component hasn't yet been layed out and
    // we don't know the number of pages; in this case, set
    // the page count to one so the print dialog will be
    // set to "print all pages" mode
    int page_count = getPageCount();
    if (page_count == 0)
        page_count = 1;

    // create an initial print dialog data object
    wxPrintDialogData printdialogdata(printdata);
    printdialogdata.SetMinPage(1);
    printdialogdata.SetMaxPage(page_count);
    printdialogdata.SetFromPage(1);
    printdialogdata.SetToPage(page_count);

    if (!show_print_dialog)
    {
        // if we're not showing the dialog, print all the pages
        printdialogdata.SetAllPages(true);
    }
    else
    {
        wxPrintDialog dialog(this, &printdialogdata);
        if (dialog.ShowModal() != wxID_OK)
            return false;

        // get the updated print data from the print dialog
        printdialogdata = dialog.GetPrintDialogData();
    }

    // GetPaperSize() only in mm; use GetPaperId() for dimensions in mm*10
    wxPaperSize paper_id = printdialogdata.GetPrintData().GetPaperId();
    wxSize dialog_paper_size = paper_db->GetSize(paper_id);
    dialog_paper_size.x = mm2in((double)dialog_paper_size.x/10)*kcanvas::CANVAS_MODEL_DPI;
    dialog_paper_size.y = mm2in((double)dialog_paper_size.y/10)*kcanvas::CANVAS_MODEL_DPI;
    if (printdialogdata.GetPrintData().GetOrientation() == wxLANDSCAPE)
    {
        int t = dialog_paper_size.x;
        dialog_paper_size.x = dialog_paper_size.y;
        dialog_paper_size.y = t;
    }

    ReportPrintInfo info;
    info.setPageSize(dialog_paper_size.x, dialog_paper_size.y);
    info.setQuality(printdialogdata.GetPrintData().GetQuality());
    info.setNumberCopies(printdialogdata.GetPrintData().GetNoCopies());    
    info.setMinPage(printdialogdata.GetMinPage());
    info.setMaxPage(printdialogdata.GetMaxPage());
    info.setFromPage(printdialogdata.GetFromPage());
    info.setToPage(printdialogdata.GetToPage());
    info.setAllPages(printdialogdata.GetAllPages());

    // create a new report print pdf; it will destroy 
    // itself when done
    ReportLayoutEngine layout_engine;
    initReportLayoutEngine(layout_engine, true);

    ReportOutputPrinter* printout = new ReportOutputPrinter(layout_engine, info, m_doc_site->getCaption(), false);
    printout->sigOutputBegin().connect(this, &ReportDoc::onPrintingBegin);
    printout->sigOutputEnd().connect(this, &ReportDoc::onPrintingEnd);
    printout->sigOutputPage().connect(this, &ReportDoc::onPrintingPage);

    printout->create();
    return true;
}

bool ReportDoc::saveFile(const wxString& path)
{
    // save the report info
    if (!getDesignComponentRaw()->save(path))
        return false;

    // reset the state flags
    m_changed = false;
    m_temporary = false;

    // set the file path
    m_file_path = path;
      
    // update the caption
    updateCaption();

    // fire this event so that the URL will be updated with the report's path
    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));

    // render the canvas
    getCanvas()->render();
    return true;
}

bool ReportDoc::loadFile(const wxString& path, bool layout)
{
    // set the created/changed/editing flags
    m_created = false;
    m_changed = false;
    m_editing = false;
    m_temporary = false;

    // set an initial caption
    m_file_path = path;
    updateCaption();

    // fire this event so that the URL will be updated with the report's path
    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));

    // clear the layout component
    getLayoutComponentRaw()->clear();

    // load the report info
    if (!getDesignComponentRaw()->load(path))
        return false;
 
    // set the origin and size of the design component, regardless of 
    // what it may have been saved as; this ensures that new reports
    // that are created and saved without first saving them are displayed
    // properly the first time they are loaded; offset the design component 
    // slightly so the gray borders of the table portions of the component 
    // are hidden under the other parts of the window
    int x_adj = m_canvas->dtom_x(0);
    int y_adj = m_canvas->dtom_y(0);
    getDesignComponent()->setOrigin(-x_adj, -y_adj);
    getDesignComponent()->setWidth(m_canvas->getWidth());
    getDesignComponent()->setHeight(m_canvas->getHeight());
 
    // set the active table to the detail
    getDesignComponentRaw()->setActiveSectionByName(PROP_REPORT_DETAIL);

    // set the default zoom
    getCanvas()->setZoom(PROP_REPORT_DEFAULT_ZOOM);

    // update scrollbars, caption and status bar; note the caption
    // and status bars may have changed based on the load; we update
    // it earlier to put an initial title up while the document is
    // loading and here to make sure it's correct after the load 
    updateCaption();
    updateStatusBar();

    // set the initial focus component
    kcanvas::IComponentPtr focus;
    focus = getDesignComponentRaw()->getActiveSectionTable();
    getCanvas()->setFocus(focus);

    // if the layout flag is set, load the layout view;
    // otherwise load the design view
    if (layout)
        loadLayoutView();
          else
        loadDesignView();

    // update the format items
    updateFormatItems();

    // update the column list
    g_app->getMainFrame()->postEvent(new FrameworkEvent(FRAMEWORK_EVT_COLUMNLISTPANEL_UPDATE));
    
    return true;
}

void ReportDoc::onSave(wxCommandEvent& evt)
{
    doSave(false);
}

void ReportDoc::onSaveAs(wxCommandEvent& evt)
{
    // set the temporary flag to true to trigger
    // the "save-as" dialog
    m_temporary = true;
    
    // save
    doSave(false);
}

void ReportDoc::onSaveAsExternal(wxCommandEvent& evt)
{    
    wxString filter;

    // NOTE: if you add or remove items from this
    // list, make sure you adjust the case statement
    // below, because it affects which file type the
    // target will be
    filter += _("PDF Files");
    filter += wxT(" (*.pdf)|*.pdf|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxString filename = getFilenameFromPath(m_file_path, false);
    wxFileDialog dlg(g_app->getMainWindow(),
                     _("Save As"),
                     wxT(""),
                     filename,
                     filter,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() != wxID_OK)
        return;

    int type = 0;
    
    switch (dlg.GetFilterIndex())
    {
        case 0: type = 0; break;    // TODO: implement proper filter indexes
        default:
            wxFAIL_MSG(wxT("invalid filter index"));
            return;
    }

    // save as a pdf file
    doSaveAsPdf(dlg.GetPath());
}

void ReportDoc::onReload(wxCommandEvent& evt)
{
    getLayoutComponentRaw()->refresh();
    updateCanvas();

    // update the column list
    g_app->getMainFrame()->postEvent(new FrameworkEvent(FRAMEWORK_EVT_COLUMNLISTPANEL_UPDATE));
}

void ReportDoc::onPrint(wxCommandEvent& evt)
{
    print(true);
}

void ReportDoc::onPageSetup(wxCommandEvent& evt)
{
    // TODO: add undo/redo

    // set the dialog page size from the current page size
    int page_width, page_height;
    getPageSize(&page_width, &page_height);

    PageSetupDialog dlg(this);
    dlg.setPageDimensions(model2in(page_width),
                          model2in(page_height));

    // set the dialog margins from the current page margins
    int margin_left, margin_right, margin_top, margin_bottom;
    getPageMargins(&margin_left, &margin_right, &margin_top, &margin_bottom);

    dlg.setMargins(model2in(margin_left),
                   model2in(margin_right),
                   model2in(margin_top),
                   model2in(margin_bottom));

    if (dlg.ShowModal() != wxID_OK)
    {
        // if we press anything besides OK, don't
        // do anything
        setDocumentFocus();
        return;
    }

    // set the page size variables    
    double dlg_page_width, dlg_page_height;
    dlg.getPageDimensions(&dlg_page_width, 
                          &dlg_page_height);

    setPageSize(in2model(dlg_page_width),
                in2model(dlg_page_height));

    // set the page margin variables
    double dlg_margin_left, dlg_margin_right, dlg_margin_top, dlg_margin_bottom;
    dlg.getMargins(&dlg_margin_left,
                   &dlg_margin_right,
                   &dlg_margin_top,
                   &dlg_margin_bottom);

    setPageMargins(in2model(dlg_margin_left),
                   in2model(dlg_margin_right),
                   in2model(dlg_margin_top),
                   in2model(dlg_margin_bottom));

    // reset the report layout
    resetReportLayout();

    // update the canvas
    updateCanvas();
    
    // set the focus back to the report
    setDocumentFocus();
}

void ReportDoc::onReportSettings(wxCommandEvent& evt)
{
    showReportSettings();
}

void ReportDoc::onGoTo(wxCommandEvent& evt)
{
    gotoPage();
}

void ReportDoc::onZoomIn(wxCommandEvent& evt)
{
    getCanvas()->setZoom(getCanvas()->getZoom() + kcanvas::ZOOM_CHANGE);
}

void ReportDoc::onZoomOut(wxCommandEvent& evt)
{
    getCanvas()->setZoom(getCanvas()->getZoom() - kcanvas::ZOOM_CHANGE);
}

void ReportDoc::onZoomToActual(wxCommandEvent& evt)
{
    getCanvas()->setZoom(kcanvas::ZOOM_SCALE);
}

void ReportDoc::onZoomToWidth(wxCommandEvent& evt)
{
    // get the view size
    int view_w, view_h;
    getCanvas()->getViewSize(&view_w, &view_h);
    
    // get the model size
    int model_w, model_h;
    getCanvas()->getSize(&model_w, &model_h);
   
    int zoom = (view_w*getCanvas()->getZoom())/model_w;
    getCanvas()->setZoom(zoom);
}

void ReportDoc::onZoomToHeight(wxCommandEvent& evt)
{
    // get the view size
    int view_w, view_h;
    getCanvas()->getViewSize(&view_w, &view_h);
    
    // get the model size
    int model_w, model_h;
    getCanvas()->getSize(&model_w, &model_h);
    
    int zoom = (view_h*getCanvas()->getZoom())/model_h;
    getCanvas()->setZoom(zoom);
}

void ReportDoc::onZoomToSize(wxCommandEvent& evt)
{
    // get the view size
    int view_w, view_h;
    getCanvas()->getViewSize(&view_w, &view_h);
    
    // get the model size
    int model_w, model_h;
    getCanvas()->getSize(&model_w, &model_h);

    int zoom_width = (view_w*getCanvas()->getZoom())/model_w;
    int zoom_height = (view_h*getCanvas()->getZoom())/model_h;
    int zoom = (zoom_width > zoom_height ? zoom_height : zoom_width);
    
    getCanvas()->setZoom(zoom);
}

void ReportDoc::onZoomChanged(wxCommandEvent& evt)
{
    // set the canvas zoom
    long size;
    evt.GetString().ToLong(&size);
    getCanvas()->setZoom(size);
    
    // select the text in the zoom combo
    m_format_bar->m_zoom_combo->SelectAll();
}

void ReportDoc::onSetToolSelect(wxCommandEvent& evt)
{
    // TODO: setTool() removed from Canvas interface; add to CanvasControl?
    //getCanvas()->setTool(kcanvas::toolSelect);
}

void ReportDoc::onSetToolNavigate(wxCommandEvent& evt)
{
    // TODO: setTool() removed from Canvas interface; add to CanvasControl?
    //getCanvas()->setTool(kcanvas::toolNavigate);
}

void ReportDoc::onCreateTextBox(wxCommandEvent& evt)
{
    // TODO: setTool() removed from Canvas interface; add to CanvasControl?
    //getCanvas()->setTool(kcanvas::toolCreate);
}

void ReportDoc::onCreateImage(wxCommandEvent& evt)
{
    // TODO: setTool() removed from Canvas interface; add to CanvasControl?
    //getCanvas()->setTool(kcanvas::toolSelect);

    wxString filter;
    filter  = _("All Image Files");
    filter += wxT(" (*.bmp, *.jpg, *.png, *.gif)|*.bmp;*.jpg;*.png;*.gif|");
    filter += _("Bitmap Files");
    filter += wxT(" (*.bmp)|*.bmp|");
    filter += _("JPEG Images");
    filter += wxT(" (*.jpg, *.jpeg)|*.jpg;*.jpeg|");
    filter += _("PNG Images");
    filter += wxT(" (*.png)|*.png|");
    filter += _("Graphics Interface Files");
    filter += wxT(" (*.gif)|*.gif|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxFileDialog dlg(g_app->getMainWindow(),
                     _("Choose File(s)"),
                     wxT(""),
                     wxT(""),
                     filter,
                     wxFD_OPEN | wxFD_CHANGE_DIR | wxFD_FILE_MUST_EXIST);

    if (dlg.ShowModal() == wxID_OK)
    {
        // load the file
        wxImage wx_image;
        wxString path = dlg.GetPath();
        wx_image.LoadFile(path);

        if (!wx_image.Ok())
            return;

        // create an image component
        kcanvas::IComponentPtr comp;
        comp = kcanvas::CompImage::create();

        if (comp.isNull())
            return;

        // set the image
        kcanvas::ICompImagePtr image = comp;
        if (image.isNull())
            return;

        image->setImage(wx_image);

        // set an initial width and height
        int x = getCanvas()->dtom_x(0);
        int y = getCanvas()->dtom_y(0);
        int w = getCanvas()->dtom_x(wx_image.GetWidth());
        int h = getCanvas()->dtom_y(wx_image.GetHeight());

        comp->setOrigin(x, y);
        comp->setSize(w, h);

        // get the view location
        int view_x, view_y, view_w, view_h;
        getCanvas()->getViewOrigin(&view_x, &view_y);
        getCanvas()->getViewSize(&view_w, &view_h);

        // TODO: get the design table location closest to the
        // middle of the screen and drop the image onto this table
        x += (view_w - w)/2;
        y += (view_h - h)/2;

        // TODO: drop image

        // update the canvas
        updateCanvas();
    }

    setDocumentFocus();
}

void ReportDoc::onFontFaceChanged(wxCommandEvent& evt)
{
    // if the font face name is different from the
    // current one, set the new font face name

    wxString fontfacename = evt.GetString();
    if (!fontfacename.IsEmpty() && fontfacename != m_format_fontfacename)
    {
        m_format_fontfacename = fontfacename;
        setSelectedCellProperty(kcanvas::PROP_FONT_FACENAME, m_format_fontfacename);
    }
}

void ReportDoc::onFontSizeChanged(wxCommandEvent& evt)
{
    // if the font size is different from the
    // current one, set the new font size

    long size;
    evt.GetString().ToLong(&size);

    if (size != -1 && size != m_format_fontsize)
    {
        m_format_fontsize = (int)size;
        setSelectedCellProperty(kcanvas::PROP_FONT_SIZE, m_format_fontsize);
    }
    
    // select the text in the size combo
    m_format_bar->m_fontsize_combo->SelectAll();
}

void ReportDoc::onFormatChanged(wxCommandEvent& evt)
{
    if (evt.GetId() == ID_Format_ToggleFontWeight)
    {
        if (m_format_fontweight != kcanvas::FONT_WEIGHT_NORMAL)
            m_format_fontweight = kcanvas::FONT_WEIGHT_NORMAL;
        else
            m_format_fontweight = kcanvas::FONT_WEIGHT_BOLD;

        setSelectedCellProperty(kcanvas::PROP_FONT_WEIGHT, m_format_fontweight);
    }
    
    if (evt.GetId() == ID_Format_ToggleFontStyle)
    {   
        if (m_format_fontstyle != kcanvas::FONT_STYLE_NORMAL)
            m_format_fontstyle = kcanvas::FONT_STYLE_NORMAL;
        else
            m_format_fontstyle = kcanvas::FONT_STYLE_ITALIC;

        setSelectedCellProperty(kcanvas::PROP_FONT_STYLE, m_format_fontstyle);
    }
    
    if (evt.GetId() == ID_Format_ToggleFontUnderline)
    {
        if (m_format_fontunderscore != kcanvas::FONT_UNDERSCORE_NORMAL)
            m_format_fontunderscore = kcanvas::FONT_UNDERSCORE_NORMAL;
        else
            m_format_fontunderscore = kcanvas::FONT_UNDERSCORE_LINE;

        setSelectedCellProperty(kcanvas::PROP_FONT_UNDERSCORE, m_format_fontunderscore);
    }

    if (evt.GetId() == ID_Format_AlignTextLeft)
    {
        if (m_format_halignment != kcanvas::ALIGNMENT_LEFT)
        {
            m_format_halignment = kcanvas::ALIGNMENT_LEFT;
            setSelectedCellProperty(kcanvas::PROP_TEXT_HALIGN, m_format_halignment);
        }
    }
    
    if (evt.GetId() == ID_Format_AlignTextCenter)
    {
        if (m_format_halignment != kcanvas::ALIGNMENT_CENTER)
        {
            m_format_halignment = kcanvas::ALIGNMENT_CENTER;
            setSelectedCellProperty(kcanvas::PROP_TEXT_HALIGN, m_format_halignment);
        }
    }
    
    if (evt.GetId() == ID_Format_AlignTextRight)
    {
        if (m_format_halignment != kcanvas::ALIGNMENT_RIGHT)
        {
            m_format_halignment = kcanvas::ALIGNMENT_RIGHT;
            setSelectedCellProperty(kcanvas::PROP_TEXT_HALIGN, m_format_halignment);
        }
    }

    if (evt.GetId() == ID_Format_AlignTop)
    {
        if (m_format_valignment != kcanvas::ALIGNMENT_TOP)
        {
            m_format_valignment = kcanvas::ALIGNMENT_TOP;
            setSelectedCellProperty(kcanvas::PROP_TEXT_VALIGN, m_format_valignment);
        }
    }
    
    if (evt.GetId() == ID_Format_AlignMiddle)
    {
        if (m_format_valignment != kcanvas::ALIGNMENT_MIDDLE)
        {
            m_format_valignment = kcanvas::ALIGNMENT_MIDDLE;
            setSelectedCellProperty(kcanvas::PROP_TEXT_VALIGN, m_format_valignment);
        }
    }
    
    if (evt.GetId() == ID_Format_AlignBottom)
    {
        if (m_format_valignment != kcanvas::ALIGNMENT_BOTTOM)
        {
            m_format_valignment = kcanvas::ALIGNMENT_BOTTOM;
            setSelectedCellProperty(kcanvas::PROP_TEXT_VALIGN, m_format_valignment);
        }
    }
    
    if (evt.GetId() == ID_Format_TextColor)
    {
        wxColor* color = (wxColor*)evt.GetExtraLong();
        if (color)
            setSelectedCellProperty(kcanvas::PROP_TEXT_COLOR, kcanvas::tokccolor(*color));            
    }
    
    if (evt.GetId() == ID_Format_FillColor)
    {
        wxColor* color = (wxColor*)evt.GetExtraLong();
        if (color)
            setSelectedCellProperty(kcanvas::PROP_COLOR_BG, kcanvas::tokccolor(*color));
    }
    
    if (evt.GetId() == ID_Format_LineColor)
    {
        wxColor* color = (wxColor*)evt.GetExtraLong();
        if (color)
            setSelectedCellProperty(kcanvas::PROP_COLOR_FG, kcanvas::tokccolor(*color));
    }

    if (evt.GetId() == ID_Format_Border)
    {
        // TODO: use button that was clicked to set the
        // appropriate border

        wxString border_string = evt.GetString();
        if (border_string.Length() > 0)
        {
            kcanvas::Properties props;

            // add the styles
            wxStringTokenizer tokenizer(border_string, wxT(";"));
            while (tokenizer.HasMoreTokens())
            {
                wxString border_token = tokenizer.GetNextToken();
                wxString border = border_token.BeforeFirst(wxT(':'));
                wxString style = border_token.AfterFirst(wxT(':'));

                props.add(border, style);
            }
 
            setSelectedCellBorderProperties(props);
        }
    }
    
    if (evt.GetId() == ID_Format_ToggleWrapText)
    {
        m_format_textwrapped = !m_format_textwrapped;
        setSelectedCellProperty(kcanvas::PROP_TEXT_WRAP, m_format_textwrapped);
    }
    
    if (evt.GetId() == ID_Format_ToggleMergeCells)
    {
        m_format_cellsmerged = !m_format_cellsmerged;
        setMergedCells(m_format_cellsmerged);
    }    
}

void ReportDoc::onUndo(wxCommandEvent& evt)
{
    if (m_history.isNull())
        return;

    m_history->undo();
    
    updateCanvas();
    updateFormatItems();
}

void ReportDoc::onRedo(wxCommandEvent& evt)
{
    if (m_history.isNull())
        return;

    m_history->redo();

    updateFormatItems();
    updateCanvas();
}

void ReportDoc::onCut(wxCommandEvent& evt)
{
    kcanvas::IEditPtr edit = getCanvas();
    if (edit.isNull())
        return;

    // track the changes
    track();

    // cut        
    edit->cut();
    getCanvas()->layout();
    getCanvas()->render();
    
    // commit the changes
    commit();
}

void ReportDoc::onCopy(wxCommandEvent& evt)
{
    kcanvas::IEditPtr edit = getCanvas();
    if (edit.isNull())
        return;

    // note: no need to track changes or update the
    // canvas since there's no change in the canvas

    // copy the changes  
    edit->copy();
}

void ReportDoc::onPaste(wxCommandEvent& evt)
{
    kcanvas::IEditPtr edit = getCanvas();
    if (edit.isNull())
        return;

    // track the changes
    track();

    // paste        
    edit->paste();
    getCanvas()->layout();
    getCanvas()->render();
    
    // commit the changes
    commit();
}

void ReportDoc::onSelectAll(wxCommandEvent& evt)
{
    kcanvas::IEditPtr edit = getCanvas();
    if (edit.isNull())
        return;

    // note: no need to track the changes, since
    // there's no change in the canvas
    
    edit->selectAll();
    getCanvas()->render();
}

void ReportDoc::onSelectNone(wxCommandEvent& evt)
{
    kcanvas::IEditPtr edit = getCanvas();
    if (edit.isNull())
        return;

    // note: no need to track the changes, since
    // there's no change in the canvas 
  
    edit->selectNone();
    getCanvas()->render();
}

void ReportDoc::onClearText(wxCommandEvent& evt)
{
    clearText();
}

void ReportDoc::onInsertColumns(wxCommandEvent& evt)
{
    // track the changes
    track();

    // insert the columns
    if (!getDesignComponentRaw()->insertColumns())
    {
        rollback();
        return;
    }

    // update the canvas
    updateCanvas();

    // commit the changes
    commit();
}

void ReportDoc::onDeleteColumns(wxCommandEvent& evt)
{
    // track the changes
    track();
    
    // delete the columns
    if (!getDesignComponentRaw()->deleteColumns())
    {
        rollback();
        return;
    }

    // update the canvas
    updateCanvas();

    // commit the changes
    commit();
}

void ReportDoc::onInsertRows(wxCommandEvent& evt)
{
    // track the changes
    track();
    
    // insert the rows
    if (!getDesignComponentRaw()->insertRows())
    {
        rollback();
        return;
    }

    // update the canvas
    updateCanvas();

    // commit the changes
    commit();
}

void ReportDoc::onDeleteRows(wxCommandEvent& evt)
{
    // track the changes
    track();
    
    // delete the rows
    if (!getDesignComponentRaw()->deleteRows())
    {
        rollback();
        return;
    }

    // update the canvas
    updateCanvas();

    // commit the changes
    commit();
}

void ReportDoc::onShowColumnPanel(wxCommandEvent& evt)
{
    g_app->getAppController()->showColumnListPanel();
}

void ReportDoc::onTextColorDropDown(wxAuiToolBarEvent& evt)
{
    // post this event to AppController
    ::wxPostEvent(g_app->getAppController(), evt);
}

void ReportDoc::onFillColorDropDown(wxAuiToolBarEvent& evt)
{
    // post this event to AppController
    ::wxPostEvent(g_app->getAppController(), evt);
}

void ReportDoc::onLineColorDropDown(wxAuiToolBarEvent& evt)
{
    // post this event to AppController
    ::wxPostEvent(g_app->getAppController(), evt);
}

void ReportDoc::onBorderDropDown(wxAuiToolBarEvent& evt)
{
    // post this event to AppController
    ::wxPostEvent(g_app->getAppController(), evt);
}

void ReportDoc::onToggleFormatToolbar(wxCommandEvent& evt)
{
    // process this event elsewhere (in AppController::onToggleToolbar)
    evt.Skip();
}

void ReportDoc::onSize(wxSizeEvent& evt)
{
    Layout();
}

void ReportDoc::onScroll(wxScrollWinEvent& evt)
{
    if (evt.GetOrientation() == wxHORIZONTAL)
    {
        // note: to calculate the line and page movement
        // offsets, make sure to multiple the percentage
        // by the width before dividing to avoid dividing
        // an integer by another integer of about the same 
        // size, resulting in significant rounding errors
    
        int view_width = getCanvas()->getViewWidth();
        int line = (view_width*kcanvas::SCROLL_LINE_PERC)/100;
        int page = (view_width*kcanvas::SCROLL_PAGE_PERC)/100;

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            getCanvas()->scrollBy(-line, 0);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            getCanvas()->scrollBy(line, 0);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            getCanvas()->scrollBy(-page, 0);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            getCanvas()->scrollBy(page, 0);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
        {        
            int x, y;
            getCanvas()->getViewOrigin(&x, &y);
            getCanvas()->scrollTo(evt.GetPosition(), y);
        }
    }

    if (evt.GetOrientation() == wxVERTICAL)
    {
        // note: to calculate the line and page movement
        // offsets, make sure to multiple the percentage
        // by the height before dividing to avoid dividing
        // an integer by another integer of about the same 
        // size, resulting in significant rounding errors
    
        int view_height = getCanvas()->getViewHeight();
        int line = (view_height*kcanvas::SCROLL_LINE_PERC)/100;
        int page = (view_height*kcanvas::SCROLL_PAGE_PERC)/100;

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            getCanvas()->scrollBy(0, -line);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            getCanvas()->scrollBy(0, line);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            getCanvas()->scrollBy(0, -page);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            getCanvas()->scrollBy(0, page);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
        {   
            int x, y;
            getCanvas()->getViewOrigin(&x, &y);
            getCanvas()->scrollTo(x, evt.GetPosition());
        }
    }
}

void ReportDoc::onMouse(wxMouseEvent& evt)
{
    // set the cursor to the standard cursor as the default;
    // we'll set the specific canvas cursors in a canvas 
    // event handler
    SetCursor(*wxSTANDARD_CURSOR);

    if (evt.GetEventType() == wxEVT_LEFT_DOWN ||
        evt.GetEventType() == wxEVT_RIGHT_DOWN ||
        evt.GetEventType() == wxEVT_MIDDLE_DOWN)
    {    
        // set the focus to the canvas
        setDocumentFocus();
    }
}

void ReportDoc::onUpdateUI(wxUpdateUIEvent& evt)
{
    // update UI elements
    int id = evt.GetId();
    switch (id)
    {
        /*
        // remove data source
        case ID_RemoveDataSource:
            {
                wxString data_path = getDataSource();
                evt.Enable(!data_path.IsEmpty());
            }
            break;
        */

        // undo
        case ID_Edit_Undo:
            evt.Enable(m_history.isOk() && m_history->canUndo());
            break;

        // redo
        case ID_Edit_Redo:
            evt.Enable(m_history.isOk() && m_history->canRedo());
            break;

        // cut
        case ID_Edit_Cut:
            {
                kcanvas::IEditPtr edit = getCanvas();
                evt.Enable(edit.isOk() && edit->canCut());
            }
            break;

        // copy    
        case ID_Edit_Copy:
            {
                kcanvas::IEditPtr edit = getCanvas();
                evt.Enable(edit.isOk() && edit->canCopy());
            }
            break;

        // paste            
        case ID_Edit_Paste:
            {
                kcanvas::IEditPtr edit = getCanvas();
                evt.Enable(edit.isOk() && edit->canPaste());
            }
            break;
            
        // find: TODO: enable when implemented
        case ID_Edit_Find:
            evt.Enable(false);
            break;

        // replace; TODO: enable when implemented
        case ID_Edit_Replace:
            evt.Enable(false);
            break;

        // goto
        case ID_Edit_GoTo:
            evt.Enable(!isDesignView());
            break;

        // find next/find prev; TODO: enable when implemented
        case ID_Edit_FindPrev:
        case ID_Edit_FindNext:
            evt.Enable(false);
            break;

        // formatting items
        case ID_Format_ToggleWrapText:
            evt.Enable(true);
            evt.Check(m_format_textwrapped);
            break;
            
        case ID_Format_ToggleMergeCells:
            evt.Enable(true);
            evt.Check(m_format_cellsmerged);
            break;
            
        case ID_Format_ToggleFontWeight:
            evt.Enable(true);
            evt.Check(m_format_fontweight == kcanvas::FONT_WEIGHT_BOLD);
            break;
            
        case ID_Format_ToggleFontStyle:
            evt.Enable(true);
            evt.Check(m_format_fontstyle == kcanvas::FONT_STYLE_ITALIC);
            break;
            
        case ID_Format_ToggleFontUnderline:
            evt.Enable(true);
            evt.Check(m_format_fontunderscore == kcanvas::FONT_UNDERSCORE_LINE);
            break;
            
        case ID_Format_AlignTextLeft:
            evt.Enable(true);
            evt.Check(m_format_halignment == kcanvas::ALIGNMENT_LEFT);
            break;
            
        case ID_Format_AlignTextCenter:
            evt.Enable(true);
            evt.Check(m_format_halignment == kcanvas::ALIGNMENT_CENTER);
            break;
                    
        case ID_Format_AlignTextRight:
            evt.Enable(true);
            evt.Check(m_format_halignment == kcanvas::ALIGNMENT_RIGHT);
            break;

        case ID_Format_AlignTop:
            evt.Enable(true);
            evt.Check(m_format_valignment == kcanvas::ALIGNMENT_TOP);
            break;

        case ID_Format_AlignMiddle:
            evt.Enable(true);
            evt.Check(m_format_valignment == kcanvas::ALIGNMENT_MIDDLE);
            break;

        case ID_Format_AlignBottom:
            evt.Enable(true);
            evt.Check(m_format_valignment == kcanvas::ALIGNMENT_BOTTOM);
            break;

        // selected components
        case ID_Canvas_ObjectAlignLeft:
        case ID_Canvas_ObjectAlignCenter:
        case ID_Canvas_ObjectAlignRight:
        case ID_Canvas_ObjectAlignTop:
        case ID_Canvas_ObjectAlignMiddle:
        case ID_Canvas_ObjectAlignBottom:
        case ID_Canvas_ObjectBringToFront:
        case ID_Canvas_ObjectSendToBack:
        case ID_Canvas_ObjectBringForward:
        case ID_Canvas_ObjectSendBackward:        
            evt.Enable(true);
            break;
            
        default:
            evt.Enable(true);
            break;
    }
}

void ReportDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void ReportDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

bool ReportDoc::initCanvas(kcanvas::ICanvasPtr canvas)
{
    if (canvas.isNull())
        return false;

    // set the default canvas control color
    wxColour c;
    kcanvas::Color base_color;

#ifdef __WXMAC__
/*
    wxBrush toolbarbrush;
    toolbarbrush.MacSetTheme( kThemeBrushToolbarBackground );
    c = toolbarbrush.GetColour();
    base_color = kcanvas::Color(c.Red(), c.Green(), c.Blue());
*/
    c = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
    base_color = kcanvas::Color(c.Red(), c.Green(), c.Blue());    
#else
    c = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
    base_color = kcanvas::Color(c.Red(), c.Green(), c.Blue());    
#endif

    // the base_colour is too pale to use as our base colour,
    // so darken it a bit --
    if ((255-base_color.Red()) +
        (255-base_color.Green()) +
        (255-base_color.Blue()) < 60)
    {
        base_color = kcanvas::stepColor(kcanvas::COLOR_BLACK, base_color, 8);
    }

    canvas->addProperty(kcanvas::PROP_COLOR_3DFACE, base_color);

    // set the default canvas highlight colors
    c = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    canvas->addProperty(kcanvas::PROP_COLOR_HIGHLIGHT, kcanvas::Color(c.Red(), c.Green(), c.Blue()));    

    c = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
    canvas->addProperty(kcanvas::PROP_COLOR_HIGHLIGHTTEXT, kcanvas::Color(c.Red(), c.Green(), c.Blue()));

    return true;
}

bool ReportDoc::initComponents(kcanvas::ICanvasPtr canvas)
{
    // if we don't have a canvas, we're done
    if (canvas.isNull())
        return false;

    // initialize design component
    m_design_component = CompReportDesign::create();
    if (m_design_component.isNull())
        return false;

    // every report has a report header, report footer, page header,
    // page footer, and detail section; groups are added separately
    CompReportDesign* design_component;
    design_component = getDesignComponentRaw();
    design_component->addSection(PROP_REPORT_HEADER, PROP_REPORT_HEADER, 2, false);
    design_component->addSection(PROP_REPORT_PAGE_HEADER, PROP_REPORT_PAGE_HEADER, 2, true);
    design_component->addSection(PROP_REPORT_DETAIL, PROP_REPORT_DETAIL, 6, true);
    design_component->addSection(PROP_REPORT_PAGE_FOOTER, PROP_REPORT_PAGE_FOOTER, 2, true);
    design_component->addSection(PROP_REPORT_FOOTER, PROP_REPORT_FOOTER, 2, false);

    // set the active section to the detail    
    design_component->setActiveSectionByName(PROP_REPORT_DETAIL);
    
    // set the origin of the design component; offset the
    // design component slightly so the gray borders of
    // the table portions of the component are hidden
    // under the other parts of the window
    int x_adj = m_canvas->dtom_x(0);
    int y_adj = m_canvas->dtom_y(0);
    design_component->setOrigin(-x_adj, -y_adj);

    // initialize layout component
    m_layout_component = CompReportLayout::create();
    if (m_layout_component.isNull())
        return false;

    // TODO: for now, set the canvas manually; we have to do 
    // this because the canvas element is normally set when a 
    // component is added to the canvas; however, the report 
    // may be created in layout view, at which time, the design 
    // component won't yet have a valid canvas because it hasn't 
    // yet been added to the canvas; as a result, the layout 
    // component itself won't have the correct layout because 
    // it relies on the design component for its layout, and 
    // the layout component is not able to update the design
    // component without the design component having a valid 
    // canvas
    getDesignComponentRaw()->setCanvas(m_canvas);
    getLayoutComponentRaw()->setCanvas(m_canvas);

    return true;
}

bool ReportDoc::initEventHandlers(kcanvas::ICanvasPtr canvas)
{
    // note: adds a default event handler for the canvas

    // if we don't have a valid canvas we're done
    if (canvas.isNull())
        return false;

    // target for following event handlers
    kcanvas::IEventTargetPtr target = canvas;
    if (target.isNull())
        return false;

    // disable certain events based on a condition in onCanvasDisableEvent
    kcanvas::EventHandler* prehandler = new kcanvas::EventHandler;
    prehandler->sigEvent().connect(this, &ReportDoc::onCanvasDisableEvent);

    target->addEventHandler(kcanvas::EVENT_MOUSE_LEFT_DOWN, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_LEFT_UP, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_DCLICK, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_DOWN, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_UP, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_DCLICK, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_DOWN, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_UP, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_MOTION, prehandler, true);
    target->addEventHandler(kcanvas::EVENT_MOUSE_OVER, prehandler, true);

    // connect the event handler
    kcanvas::EventHandler* handler = new kcanvas::EventHandler;
    handler->sigEvent().connect(this, &ReportDoc::onCanvasEvent);

    // capturing phase events
    target->addEventHandler(kcanvas::EVENT_KEY, handler, true);
    target->addEventHandler(kcanvas::EVENT_KEY_DOWN, handler, true);
    target->addEventHandler(kcanvas::EVENT_KEY_UP, handler, true);

    // target and bubbling phase events
    target->addEventHandler(kcanvas::EVENT_KEY, handler);
    target->addEventHandler(kcanvas::EVENT_KEY_DOWN, handler);
    target->addEventHandler(kcanvas::EVENT_KEY_UP, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_LEFT_DCLICK, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_LEFT_DOWN, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_LEFT_UP, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_DCLICK, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_DOWN, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_UP, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_DCLICK, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_DOWN, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_UP, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_MOTION, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_WHEEL, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_ENTER, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_LEAVE, handler);
    target->addEventHandler(kcanvas::EVENT_MOUSE_OVER, handler);
    target->addEventHandler(kcanvas::EVENT_FOCUS_SET, handler);
    target->addEventHandler(kcanvas::EVENT_FOCUS_KILL, handler);
    target->addEventHandler(kcanvas::EVENT_DROP, handler);
    target->addEventHandler(kcanvas::EVENT_PRE_SELECT, handler);
    target->addEventHandler(kcanvas::EVENT_SELECT, handler);
    target->addEventHandler(kcanvas::EVENT_PRE_LAYOUT, handler);
    target->addEventHandler(kcanvas::EVENT_LAYOUT, handler);
    target->addEventHandler(kcanvas::EVENT_PRE_RENDER, handler);
    target->addEventHandler(kcanvas::EVENT_RENDER, handler);
    target->addEventHandler(kcanvas::EVENT_CANVAS_SCROLL, handler);
    target->addEventHandler(kcanvas::EVENT_CANVAS_ZOOM, handler);
    target->addEventHandler(kcanvas::EVENT_CANVAS_CURSOR, handler);
    target->addEventHandler(kcanvas::EVENT_EDIT_BEGIN, handler);
    target->addEventHandler(kcanvas::EVENT_EDIT_END, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_CURSOR_MOVE, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_CURSOR_MOVE, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_SELECTION_CLEAR, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_SELECTION_CLEAR, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_SELECT_CELL, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_SELECT_CELL, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_SELECT_CELL, handler);    
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_RESIZE_COLUMN, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_RESIZING_COLUMN, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_RESIZE_ROW, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_RESIZING_ROW, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_POPULATE_CELL, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_POPULATE_CELL, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_MOVE_CELL, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_MOVE_CELL, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_CELL_EDIT, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_CELL_EDIT, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_PRE_CELL_CLEAR, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_CELL_CLEAR, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_CELL_LEFT_CLICK, handler);
    target->addEventHandler(kcanvas::EVENT_TABLE_CELL_RIGHT_CLICK, handler);

    // report design/layout events
    target->addEventHandler(EVENT_DESIGN_PRE_GROUP_RESIZE, handler);
    target->addEventHandler(EVENT_DESIGN_GROUP_RESIZE, handler);
    target->addEventHandler(EVENT_REPORT_UPDATED, handler);

    return true;
}

bool ReportDoc::initDragAndDrop(kcanvas::ICanvasPtr canvas)
{
    // note: adds the drag and drop handlers to the canvas

    // if we don't have a canvas, we're done
    if (canvas.isNull())
        return false;

    // if we don't have a canvas control, we're done
    kcanvas::ICanvasControlPtr canvas_control = canvas;
    if (canvas_control.isNull())
        return false;

    // add the fields panel and file system data objects to 
    // the canvas drop target
    wxWindow* canvas_window = canvas_control->getWindow();
    kcanvas::CanvasDropTarget* drop_target = 
             static_cast<kcanvas::CanvasDropTarget*>(canvas_window->GetDropTarget());
             
    m_grid_data_object = new kcl::GridDataObject(NULL, wxT("fieldspanel"));
    m_fs_data_object = new FsDataObject;
    m_canvas_data_object = drop_target->GetDataObject();

    m_canvas_data_object->AddDataObject(kcl::getGridDataFormat(wxT("fieldspanel")), m_grid_data_object);
    m_canvas_data_object->AddDataObject(FS_DATA_OBJECT_FORMAT, m_fs_data_object);
    
    return true;
}

bool ReportDoc::initControls(kcanvas::ICanvasPtr canvas)
{
    // if we don't have a canvas, we're done
    if (canvas.isNull())
        return false;

    // if we don't have a canvas control, we're done
    kcanvas::ICanvasControlPtr canvas_control = canvas;
    if (canvas_control.isNull())
        return false;

    // create the format toolbar 
    m_format_bar = new FormatToolbar(this, -1);
    m_format_bar->setEmbedded(true);
    m_format_bar->SetArtProvider(new ReportDocFormatBarArt);
    m_format_bar->SetMargins(0,0,0,0);

    // create the settings sizer
    m_settings_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_settings_sizer->Add(m_format_bar, 1, wxALIGN_CENTER);
    m_settings_sizer->SetMinSize(100, 31);

    // create the canvas sizer
    wxBoxSizer* canvas_sizer = new wxBoxSizer(wxHORIZONTAL);
    canvas_sizer->Add(canvas_control->getWindow(), 1, wxEXPAND);

    // create the main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->Add(m_settings_sizer, 0, wxEXPAND);
    m_main_sizer->Add(canvas_sizer, 1, wxEXPAND);
    SetSizer(m_main_sizer);
    Layout();

    return true;
}

bool ReportDoc::initUndoRedo()
{
    // initialize history object for storing changes
    // in the table template
    kcanvas::IComponentPtr design_component = getDesignComponent();
    m_history = kcanvas::CanvasHistory::create(design_component);
    return true;
}

bool ReportDoc::initFormatToolbar()
{
    FormatToolbar* format_toolbar;
    format_toolbar = g_app->getAppController()->getFormatToolbar();
    
    // show or hide the settings sizer depending
    // on if the global format toolbar is visible
    showFormatBar(!format_toolbar->IsShown());

    return true;
}

bool ReportDoc::initStatusBar()
{
    // create the statusbar items for this document
    IStatusBarItemPtr item;
    
    item = addStatusBarItem(wxT("reportdoc_data_source"));
    item->setWidth(200);
    
    item = addStatusBarItem(wxT("reportdoc_page_number"));
    item->setWidth(120);
    
    item = addStatusBarItem(wxT("reportdoc_page_count"));
    item->setWidth(150);

    // connect the status bar signal
    g_app->getMainFrame()->getStatusBar()->sigItemLeftDblClick().connect(
                                this, &ReportDoc::onStatusBarItemLeftDblClick);

    return true;
}

bool ReportDoc::initSettings()
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isNull())
        return false;

    if (prefs->exists(wxT("report.page.width")))
    {
        long t = getAppPrefsLong(wxT("report.page.width"));
        int width_new = in2model((double)t/100);
        
        int width, height;
        getPageSize(&width, &height);
        setPageSize(width_new, height);
    }

    if (prefs->exists(wxT("report.page.height")))
    {
        long t = getAppPrefsLong(wxT("report.page.height"));
        int height_new = in2model((double)t/100);

        int width, height;
        getPageSize(&width, &height);
        setPageSize(width, height_new);
    }

    if (prefs->exists(wxT("report.margin.left")))
    {
        long t = getAppPrefsLong(wxT("report.margin.left"));
        int left_new = in2model((double)t/100);

        int left, right, top, bottom;
        getPageMargins(&left, &right, &top, &bottom);
        setPageMargins(left_new, right, top, bottom);
    }
    
    if (prefs->exists(wxT("report.margin.right")))
    {
        long t = getAppPrefsLong(wxT("report.margin.right"));
        int right_new = in2model((double)t/100);

        int left, right, top, bottom;
        getPageMargins(&left, &right, &top, &bottom);
        setPageMargins(left, right_new, top, bottom);
    }

    if (prefs->exists(wxT("report.margin.top")))
    {
        long t = getAppPrefsLong(wxT("report.margin.top"));
        int top_new = in2model((double)t/100);

        int left, right, top, bottom;
        getPageMargins(&left, &right, &top, &bottom);
        setPageMargins(left, right, top_new, bottom);
    }

    if (prefs->exists(wxT("report.margin.bottom")))
    {
        long t = getAppPrefsLong(wxT("report.margin.bottom"));
        int bottom_new = in2model((double)t/100);

        int left, right, top, bottom;
        getPageMargins(&left, &right, &top, &bottom);
        setPageMargins(left, right, top, bottom_new);
    }

    // note: font initialization in ReportDesign::addSection()

    return true;
}

bool ReportDoc::initDoc(IFramePtr frame,
                        IDocumentSitePtr doc_site,
                        wxWindow* docsite_wnd,
                        wxWindow* panesite_wnd)
{
    // create document's window
    bool result = Create(docsite_wnd,
                         doc_site,
                         wxPoint(0,0),
                         docsite_wnd->GetClientSize(),
                         wxNO_BORDER |
                         wxHSCROLL | wxVSCROLL |
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN);

    if (!result)
        return false;

    // set the default background color
    SetBackgroundColour(kcl::getBaseColor());

    // set the docsite
    m_doc_site = doc_site;
    m_doc_site->setUIContext(getUserInterface());
    m_doc_site->setBitmap(GETBMPSMALL(gf_report));
    updateCaption();
    
    // set the frame and connect the handlers
    m_frame = frame;
    m_frame->sigFrameEvent().connect(this, &ReportDoc::onFrameEvent);

    // create the design and layout view canvas
    m_canvas = kcanvas::CanvasControl::create(this,
                                              -1,
                                              wxDefaultPosition,
                                              wxDefaultSize,
                                              wxNO_BORDER);

    // initialize the canvas with various preferences
    initCanvas(m_canvas);

    // initialize the design and layout components;
    // note: initialize these before initializing
    // anything else, since further initialization
    // depends on having valid components
    initComponents(m_canvas);

    // add handlers for the canvas and initialize the
    // drag&drop, the table controls, and the design
    // and layout components
    initEventHandlers(m_canvas);
    initDragAndDrop(m_canvas);
    initControls(m_canvas);

    // initialize the undo/redo history
    initUndoRedo();

    // initialize the format toolbar
    initFormatToolbar();

    // initialize the status bar
    initStatusBar();

    // initialize the defaut settings
    initSettings();

    // initialize the format bar
    bool show = (g_app->getAppController()->getFormatToolbar()->IsShown()) ? false : true;
    showFormatBar(show);

    // set focus to report
    setDocumentFocus();
    return true;
}

wxWindow* ReportDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

wxString ReportDoc::getDocumentLocation()
{
    return m_file_path;
}

void ReportDoc::setDocumentFocus()
{
    kcanvas::ICanvasControlPtr canvas_control = getCanvas();
    if (canvas_control.isNull())
        return;

    canvas_control->getWindow()->SetFocus();
}

bool ReportDoc::onSiteClosing(bool force)
{
    if (force)
        return true;

    if (isChanged())
    {
        int result;
        result = appMessageBox(_("Would you like to save the changes made to this document?"),
                                    APPLICATION_NAME,
                                    wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);

        // if we want to save the changes, try to save them and return
        // whatever value the doSave() function returns
        if (result == wxYES)
            return doSave(false);

        // if we cancel, do not save and do not close the site
        if (result == wxCANCEL)
            return false;
    }

    // if we're neither saving nor canceling, we want to close the
    // site without saving; return true
    return true;
}

void ReportDoc::onSiteDeactivated()
{
}

void ReportDoc::onSiteActivated()
{
    updateStatusBar();
}

IUIContextPtr ReportDoc::getUserInterface()
{
    IUIContextPtr ui_context;

    // see if a user interface of this type already exists
    ui_context = g_app->getMainFrame()->lookupUIContext(wxT("ReportUI"));
    if (ui_context.isOk())
        return ui_context;
    
    // no, so create one
    ui_context = g_app->getMainFrame()->createUIContext(wxT("ReportUI"));
    return ui_context;
}

void ReportDoc::track()
{
    // set the created flag to false
    m_created = false;

    // if we don't have a valid canvas history
    // tracker, don't do anything else
    if (m_history.isNull())
        return;

    // the canvas undo/redo reference counts track(), commit(),
    // and rollback() statements so that only the changes
    // between the very first and last track() and commit()/
    // rollback() pairs are saved; there are instances where 
    // this is desirable, but it requires that a track() is
    // always matched with a corresponding commit() or rollback();
    // however, here track() is sometimes called in an event
    // handler that is then vetoed, so that no corresponding
    // commit() will ever be called, preventing future track()
    // statements from properly marking the beginning of a
    // new series of changes and causing undo to revert all
    // the way back to the first track statement; so, for the 
    // report doc, if we're tracking changes, close any previous 
    // track statements before starting a new tracking statement;
    // this disables the ability to nest track()/commit() pairs
    // in the report doc, but parallels the way these are being 
    // used here
    if (m_tracking)
        rollback();

    // begin the tracking the canvas state
    m_history->track();

    // set the tracking flag
    m_tracking = true;
}

void ReportDoc::commit()
{
    // set the changed flag
    bool old_changed_state = m_changed;
    m_changed = true;

    // if we don't have a history, we're done
    if (m_history.isNull())
        return;

    // otherwise commit the changes, per the flag
    m_history->commit();

    // set the tracking flag
    m_tracking = false;

    // if the changed status has changed, update
    // the caption
    if (old_changed_state != m_changed)
        updateCaption();
}

void ReportDoc::rollback()
{
    // note: don't set the changed flag, since 
    // we're rolling back the changes

    if (m_history.isNull())
        return;

    // rollback the changes
    m_history->rollback();

    // set the tracking flag
    m_tracking = false;
}

bool ReportDoc::isCreated()
{
    return m_created;
}

bool ReportDoc::isChanged()
{
    return m_changed;
}

bool ReportDoc::isEditing()
{
    return m_editing;
}

bool ReportDoc::isTemporary()
{
    return m_temporary;
}

void ReportDoc::getColumnListItems(std::vector<ColumnListItem>& list)
{
    // note: returns a list of the columns available for the
    // current data source

    // make sure the column list is clear
    list.clear();

    kcanvas::IComponentPtr comp_design = getDesignComponent();
    if (comp_design.isNull())
        return;

    // if we don't have a data source, we're done        
    wxString data_source = getDesignComponentRaw()->getDataSource();
    if (data_source.Length() == 0)
        return;

    xd::Structure structure;
    structure = g_app->getDatabase()->describeTable(towstr(data_source));
    if (structure.isOk())
    {
        // add the columns to the list
        size_t i, col_count = structure.getColumnCount();
        list.reserve(col_count);
        
        for (i = 0; i < col_count; i++)
        {
            const xd::ColumnInfo& colinfo = structure.getColumnInfoByIdx(i);

            ColumnListItem item;
            item.text = colinfo.name;
            if (colinfo.calculated)
                item.bitmap = GETBMPSMALL(gf_lightning);
                 else
                item.bitmap = GETBMPSMALL(gf_field);

            item.active = true;
            list.push_back(item);
        }

        return;
    }
    
    // if the data source isn't a data set, load the query, 
    // then add the columns from the query output to the list 
    // of available fields
    
    // TODO: right now, we're loading the query parameters
    // each time; this ensures that if the query is changed
    // and saved, that the latest query output fields are 
    // always available in the column list; the tradeoff
    // is this may slow down the interface when switching 
    // to a report tab that is based on a larger query;
    // ideally, we would only load the query parameters if
    // the query has changed, but this can be hard to monitor
    // across all open docs
    QueryTemplate query_template;
    query_template.load(data_source);

    std::vector<QueryBuilderParam>::iterator it, it_end;
    it_end = query_template.m_params.end();
    
    list.reserve(query_template.m_params.size());
    for (it = query_template.m_params.begin(); it != it_end; ++it)
    {
        // if the field isn't selected for output,
        // move on
        if (!it->output)
            continue;

        // ad the field to the list
        ColumnListItem item;
        item.text = it->output_field;
        item.bitmap = GETBMPSMALL(gf_field);  // query output always static

        item.active = true;
        list.push_back(item);
    }
}

void ReportDoc::onColumnListDblClicked(const std::vector<wxString>& list)
{
    kcanvas::ICompTablePtr table;
    table = getDesignComponentRaw()->getActiveSectionTable();

    if (table.isNull())
        return;

    // track the changes
    track();

    // get the cursor position
    int row, col;
    table->getCursorPos(&row, &col);

    // insert the strings at the row
    std::vector<wxString>::const_iterator it, it_end;
    it_end = list.end();

    int idx = 0;
    for (it = list.begin(); it != it_end; ++it)
    {
        wxString value = wxT("=") + *it;
        value.Trim(true);
        value.Trim(false);
        kcanvas::CellRange range(row, col);
        table->setCellProperty(range, kcanvas::PROP_CONTENT_VALUE, value);
        col++;
    }

    // move the cursor over
    table->setCursorPos(row, col);

    // update the canvas
    updateCanvas();

    // commit the changes
    commit();
    
    // set the document focus to the report
    setDocumentFocus();
}

void ReportDoc::onFrameEvent(FrameworkEvent& evt)
{
    // if a file is renamed, update this file with the new file path
    if (evt.name == FRAMEWORK_EVT_TREEPANEL_OFS_FILE_RENAMED)
    {
        if (evt.s_param == m_file_path)
        {
            m_file_path = evt.s_param2;
            updateCaption();
            
            IDocumentSitePtr doc_site = m_frame->getActiveChild();
            if (doc_site.isOk() && doc_site == m_doc_site)
            {
                // fire this event so that the URL combobox will be updated
                // with the new path if this is the active child
                m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));
            }
        }
    }

    // populate the view switcher with the available views
    if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_QUERY_AVAILABLE_VIEW)
    {
        // make sure we are in the active container before replying
        IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        if (active_site.isNull() || m_doc_site.isNull())
            return;
        if (active_site->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
             
        // populate the view switcher list with the available views
        ViewSwitcherList* list = (ViewSwitcherList*)(evt.o_param);

        // design view
        list->addItem(ID_View_SwitchToDesignView, _("Design View"),
                      isDesignView() ? true : false);

        // layout view
        list->addItem(ID_View_SwitchToLayoutView, _("Layout View"),
                      !isDesignView() ? true : false);
    }

    // if the view has changed, switch views
    if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGED)
    {
        // -- make sure we are in the active container --
        IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        if (active_site.isNull() || m_doc_site.isNull())
            return;
        if (active_site->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
        
        // find out what view to switch to and do the switch
        int id = (int)(evt.l_param);
        if (id == ID_View_SwitchToLayoutView)
        {
            // TODO: prompt for save?
            // doSave();
        }

        switch (id)
        {
            // switching the view triggers an onSize event,
            // which triggers the layout, so when switching
            // views we don't need to call updateCanvas()
        
            case ID_View_SwitchToDesignView:
                loadDesignView();
                return;

            case ID_View_SwitchToLayoutView:
                loadLayoutView();
                return;

            default:
                // something went wrong, do nothing
                return;
        }
    }
    
    if (evt.name == FRAMEWORK_EVT_APPMAIN_FORMAT_TOOLBAR_TOGGLED)
    {
        // if the global toolbar is on, don't show the embedded toolbar
        long format_toolbar_shown = evt.l_param;
        if (format_toolbar_shown == 1)
            showFormatBar(false);
             else
            showFormatBar(true);
    }
}

void ReportDoc::onStatusBarItemLeftDblClick(IStatusBarItemPtr item)
{
    // if the data source status bar item is double-clicked, open
    // the report settings dialog
    IDocumentSitePtr docsite = m_frame->getActiveChild();
    if (docsite.isOk() && docsite == m_doc_site)
    {
        if (item->getName() == wxT("reportdoc_data_source"))
        {
            showReportSettings();
        }
         else if (item->getName() == wxT("reportdoc_page_number") ||
                  item->getName() == wxT("reportdoc_page_count"))
        {
            if (!isDesignView())
                gotoPage();
        }
    }
}

void ReportDoc::onCustomFormulaOk(ExprBuilderPanel* expr_panel)
{
    // close panel site
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("Formula"));
    if (site.isOk())
    {
        g_app->getMainFrame()->closeSite(site);
    }

    // insert the text and return the document focus
    wxString formula = expr_panel->getExpression();
    if (!formula.IsEmpty())
        insertText(wxT("=") + formula);

    setDocumentFocus();
}

void ReportDoc::onPrintingBegin(ReportOutputBase* output_engine, kcanvas::Properties& props)
{
    // get the number pages being printed
    kcanvas::PropertyValue prop_total_pages;
    props.get(kcanvas::EVENT_PROP_PAGES, prop_total_pages);
    int total_pages = prop_total_pages.getInteger();

    // set the current page of the printing process; note: the number 
    // of the page being printed is the actual page number, and is not 
    // the same as m_printing_page_idx which is the number of pages 
    // printed (including the one about to be printed), which is used 
    // for determining the progress of the print job
    m_printing_page_idx = 0;

    // start the printing process listener
    m_print_progress = new AppPrintProgressListener(total_pages);
    m_print_progress->Init(m_file_path, m_doc_site->getCaption());
    m_print_progress->OnStart();
}

void ReportDoc::onPrintingEnd(ReportOutputBase* output_engine, kcanvas::Properties& props)
{
    if (!m_print_progress)
        return;

    m_print_progress->OnFinish();
    m_print_progress = NULL;
}

void ReportDoc::onPrintingPage(ReportOutputBase* output_engine, kcanvas::Properties& props)
{
    // get the number of pages being printed
    kcanvas::PropertyValue prop_total_pages;
    props.get(kcanvas::EVENT_PROP_PAGES, prop_total_pages);
    int total_pages = prop_total_pages.getInteger();

    // increment the number of pages we've printed (including
    // the one we're about to print); note: this isn't the actual 
    // index of the page, but the index of the page being printed 
    // relative to the total number of pages that need to be printed;
    // so if we're printing pages 2-3 a total of 3 times, we have 
    // 6 pages to print, and m_printing_page index will cycle from 
    // 1 to 6
    m_printing_page_idx++;

    // sanity check; make sure the page_index never goes above the
    // total pages
    if (m_printing_page_idx > total_pages)
        m_printing_page_idx = total_pages;

    // update the job status
    if (m_print_progress)
        m_print_progress->OnProgressChange(m_printing_page_idx, total_pages);

    // if the job is cancelled, cancel the output job
    if (m_print_progress->IsCancelled())
        output_engine->cancel();

    // TODO: handle job fail
}

void ReportDoc::updateCanvas()
{
    // if the document is newly created and we're 
    // in the design view, turn the design component 
    // cursor off; otherwise, turn it on
    if (m_created && isDesignView())
        getDesignComponentRaw()->setCursorVisible(false);
         else
        getDesignComponentRaw()->setCursorVisible(true);

    // depending on the view, add either the
    // design component or the layout component
    m_canvas->removeAll();
    if (isDesignView())
        m_canvas->add(getDesignComponent());
         else
        m_canvas->add(getLayoutComponent());

    // update the canvas
    getCanvas()->layout();
    getCanvas()->render();
}

void ReportDoc::updateCanvasSize()
{
    int width, height;
    if (isDesignView())
    {
        getDesignComponent()->getSize(&width, &height);
    }
    else
    {
        getLayoutComponent()->getSize(&width, &height);
    }

    m_canvas->setSize(width, height);
}

void ReportDoc::updateCaption()
{
    if (isTemporary())
        m_doc_site->setCaption(_("(Untitled)"));
     else
     {
        wxString caption;
        caption.Append(m_file_path.AfterLast(wxT('/')));
        caption.Append(isChanged() ? wxT("*") : wxT(""));

        m_doc_site->setCaption(caption);
     }
}

void ReportDoc::updateScrollBars()
{
    if (getCanvas().isNull())
        return;

    // get the canvas size
    int width, height;
    getCanvas()->getSize(&width, &height);

    // get the canvas view size
    int view_x, view_y, view_w, view_h;
    getCanvas()->getViewOrigin(&view_x, &view_y);
    getCanvas()->getViewSize(&view_w, &view_h);

    // update the scrollbars
    width = wxMax(view_w, width);
    height = wxMax(view_h, height);

    if (wxHSCROLL & GetWindowStyleFlag())
        SetScrollbar(wxHORIZONTAL, view_x, view_w, width + 1, true);

    if (wxVSCROLL & GetWindowStyleFlag())
        SetScrollbar(wxVERTICAL, view_y, view_h, height + 1, true);
}

void ReportDoc::updateStatusBar()
{
    // if we're in layout view, calculate the page
    // we're one; TODO: may want to do some caching
    // of page idx here; however, shouldn't be a
    // problem for reports < 1000 pages
    int page_idx = 1;
    if (!isDesignView())
    {
        int x, y;
        m_canvas->getViewOrigin(&x, &y);
        getLayoutComponentRaw()->getPageIdxByPos(x, y, &page_idx);
    }

    // get the page count
    int page_count = getPageCount();
    page_count = wxMax(page_count, 1);

    wxString data_source = _("Source: ") + getDataSource();

    wxString page_number_text = wxString::Format(_("Page: %s"),
                                    kl::formattedNumber(page_idx).c_str());
                                    
    wxString page_count_text = wxString::Format(_("Page Count: %s"),
                                    kl::formattedNumber(page_count).c_str());

    IStatusBarItemPtr item;
    IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
    if (!statusbar)
        return;

    // update the data source item
    item = statusbar->getItem(wxT("reportdoc_data_source"));
    if (item.isNull())
        return;

    item->setValue(data_source);

    // update the page number item
    item = statusbar->getItem(wxT("reportdoc_page_number"));
    if (item.isNull())
        return;

    item->setValue(page_number_text);

    // update the page count item
    item = statusbar->getItem(wxT("reportdoc_page_count"));
    if (item.isNull())
        return;

    item->setValue(page_count_text);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}

void ReportDoc::updateDesignComponentMargins()
{
    int page_width, page_height;
    getPageSize(&page_width, &page_height);
    
    int left_margin, right_margin, top_margin, bottom_margin;
    getPageMargins(&left_margin, &right_margin, &top_margin, &bottom_margin);
    
    getDesignComponentRaw()->setPageSize(page_width, page_height);
    getDesignComponentRaw()->setPageMargins(left_margin,
                                            right_margin,
                                            top_margin,
                                            bottom_margin);
}

void ReportDoc::updateFormatItems()
{
    // set the format state to their
    // indeterminate values
    m_format_halignment = wxT("");
    m_format_valignment = wxT("");
    m_format_fontfacename = wxT("");
    m_format_fontweight = wxT("");
    m_format_fontstyle = wxT("");
    m_format_fontunderscore = wxT("");
    m_format_fontsize = -1;
    m_format_cellsmerged = true;
    m_format_textwrapped = false;

    kcanvas::ICompTablePtr design_table;
    design_table = getDesignComponentRaw()->getActiveSectionTable();
    
    if (design_table.isNull())
        return;

    // get the selections
    std::vector<kcanvas::CellRange> selections;
    design_table->getCellSelections(selections);
    
    // if the selection size is zero, the cursor may
    // be hidden; in this implementation, this should
    // only happen when teh report is newly created
    // and we want to show overlay text; however,
    // we still want to set the active font based on
    // the report default settings, so add in the
    // cursor cell, even if it's hidden
    if (selections.size() == 0)
    {
        int x, y;
        design_table->getCursorPos(&x, &y);
        selections.push_back(kcanvas::CellRange(x, y));
    }

    // if one of the format items is constant across 
    // the selection area, set it
    kcanvas::Properties props;
    props.add(kcanvas::PROP_FONT_FACENAME);
    props.add(kcanvas::PROP_FONT_SIZE);
    props.add(kcanvas::PROP_FONT_STYLE);
    props.add(kcanvas::PROP_FONT_WEIGHT);
    props.add(kcanvas::PROP_FONT_UNDERSCORE);
    props.add(kcanvas::PROP_TEXT_WRAP);
    
    std::vector<kcanvas::CellRange>::iterator it, it_end;
    it_end = selections.end();
    
    for (it = selections.begin(); it != it_end; ++it)
    {
        design_table->getCellProperties(*it, props);
        
        kcanvas::CellRange merged_range = *it;
        if (m_format_cellsmerged &&
             (!design_table->isMerged(merged_range) ||
              !merged_range.contains(*it)))
        {
            m_format_cellsmerged = false;
        }
    }
    
    kcanvas::PropertyValue value;
    
    if (props.get(kcanvas::PROP_TEXT_HALIGN, value))
        m_format_halignment = value.getString();
        
    if (props.get(kcanvas::PROP_TEXT_VALIGN, value))
        m_format_valignment = value.getString();
    
    if (props.get(kcanvas::PROP_FONT_FACENAME, value))
        m_format_fontfacename = value.getString();
    
    if (props.get(kcanvas::PROP_FONT_SIZE, value))
        m_format_fontsize = value.getInteger();
        
    if (props.get(kcanvas::PROP_FONT_WEIGHT, value))
        m_format_fontweight = value.getString();

    if (props.get(kcanvas::PROP_FONT_STYLE, value))
        m_format_fontstyle = value.getString();

    if (props.get(kcanvas::PROP_FONT_UNDERSCORE, value))
        m_format_fontunderscore = value.getString();
    
    if (props.get(kcanvas::PROP_TEXT_WRAP, value) &&
        value.getBoolean())
    {
        m_format_textwrapped = true;
    }

    // update the format toolbar
    FormatToolbar* toolbar = g_app->getAppController()->getFormatToolbar();
    if (toolbar)
    {
        toolbar->m_fontface_combo->SetValue(m_format_fontfacename);
        
        wxString format_fontsize = wxT("");
        if (m_format_fontsize != -1)
            format_fontsize = wxString::Format(wxT("%d"), m_format_fontsize);
            
        toolbar->m_fontsize_combo->SetValue(format_fontsize);
    }

    // update the embedded format controls
    if (m_format_bar)
    {
        m_format_bar->m_fontface_combo->SetValue(m_format_fontfacename);

        wxString format_fontsize = wxT("");
        if (m_format_fontsize != -1)
            format_fontsize = wxString::Format(wxT("%d"), m_format_fontsize);

        m_format_bar->m_fontsize_combo->SetValue(format_fontsize);
    }
}

void ReportDoc::onCanvasDisableEvent(kcanvas::IEventPtr evt)
{
    // if we're just starting up, stop the event propogation
    if (isCreated() && isDesignView())
        evt->stopPropogation();
}

void ReportDoc::onCanvasEvent(kcanvas::IEventPtr evt)
{
    wxString name = evt->getName();

    if (isCreated() && isDesignView())
    {
        // if the document is newly created, we're in the
        // design view, and the canvas is double-clicked,
        // then show a database file dialog, so the user
        // can set the data source
        if (evt->getName() == kcanvas::EVENT_MOUSE_LEFT_DCLICK )
        {
            DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeOpen);
            dlg.setCaption(_("Select Source Table"));
            dlg.setAffirmativeButtonLabel(_("OK"));
            if (dlg.ShowModal() != wxID_OK)
                return;

            // track the changes
            track();
            
            // set the data source
            setDataSource(dlg.getPath());
            resetReportLayout();
            
            // update the canvas
            updateCanvas();
            
            // commit the changes
            commit();

            return;
        }

        // however, if the document is newly created and
        // we're in design view, disable mouse events
        // so the user can double-click on the overlay text
        if (name == kcanvas::EVENT_TABLE_PRE_SELECT_CELL ||
            name == kcanvas::EVENT_TABLE_PRE_CURSOR_MOVE ||
            name == kcanvas::EVENT_TABLE_PRE_CELL_EDIT)
        {
            kcanvas::INotifyEventPtr notify_evt = evt;
            notify_evt->veto();
            return;
        }
    }

    // TODO: need a better way of doing this; this is called
    // at the end of ReportLayout::populateCache() so we know
    // to rerender the report after updating the cache, which
    // happens when a query is run; the problem is that this
    // incurs a "double-render" when we're not running a query
    if (name == EVENT_REPORT_UPDATED)
    {
        updateCanvas();
    }

    // key events
    if (name == kcanvas::EVENT_KEY_DOWN ||
        name == kcanvas::EVENT_KEY_UP ||
        name == kcanvas::EVENT_KEY)
    {
        onCanvasKeyEvent(evt);
        return;
    }
    
    // mouse events
    if (name == kcanvas::EVENT_MOUSE_LEFT_DCLICK || 
        name == kcanvas::EVENT_MOUSE_LEFT_DOWN || 
        name == kcanvas::EVENT_MOUSE_LEFT_UP || 
        name == kcanvas::EVENT_MOUSE_RIGHT_DCLICK || 
        name == kcanvas::EVENT_MOUSE_RIGHT_DOWN || 
        name == kcanvas::EVENT_MOUSE_RIGHT_UP || 
        name == kcanvas::EVENT_MOUSE_MIDDLE_DCLICK || 
        name == kcanvas::EVENT_MOUSE_MIDDLE_DOWN || 
        name == kcanvas::EVENT_MOUSE_MIDDLE_UP || 
        name == kcanvas::EVENT_MOUSE_MOTION || 
        name == kcanvas::EVENT_MOUSE_WHEEL ||
        name == kcanvas::EVENT_MOUSE_ENTER ||
        name == kcanvas::EVENT_MOUSE_LEAVE ||
        name == kcanvas::EVENT_MOUSE_OVER)
    {
        onCanvasMouseEvent(evt);
        return;
    }

    // focus events
    if (name == kcanvas::EVENT_FOCUS_SET ||
        name == kcanvas::EVENT_FOCUS_KILL)
    {
        onCanvasFocusEvent(evt);
    }

    // drop event
    if (name == kcanvas::EVENT_DROP)
    {
        onCanvasDropEvent(evt);
        return;
    }
    
    // select events
    if (name == kcanvas::EVENT_PRE_SELECT ||
        name == kcanvas::EVENT_SELECT)
    {
        onCanvasSelectEvent(evt);
        return;
    }
    
    // layout events
    if (name == kcanvas::EVENT_PRE_LAYOUT ||
        name == kcanvas::EVENT_LAYOUT)
    {
        onCanvasLayoutEvent(evt);
        return;
    }
    
    // render event
    if (name == kcanvas::EVENT_PRE_RENDER ||
        name == kcanvas::EVENT_RENDER)
    {
        onCanvasRenderEvent(evt);
        return;
    }

    if (name == kcanvas::EVENT_CANVAS_SCROLL)
    {
        onCanvasScrollEvent(evt);
        return;
    }
    
    if (name == kcanvas::EVENT_CANVAS_ZOOM)
    {
        onCanvasZoomEvent(evt);
        return;
    }

    if (name == kcanvas::EVENT_CANVAS_CURSOR)
    {
        onCanvasCursorEvent(evt);
    }

    // begin edit event
    if (name == kcanvas::EVENT_EDIT_BEGIN)
    {
        onCanvasEditBeginEvent(evt);
        return;
    }
    
    // end edit event
    if (name == kcanvas::EVENT_EDIT_END)
    {
        onCanvasEditEndEvent(evt);
        return;
    }

    if (name == kcanvas::EVENT_TABLE_CELL_RIGHT_CLICK)
    {
        onCanvasRightClick(evt);
    }

    // table selection and cursor movement for
    // formatting panel updates
    if (name == kcanvas::EVENT_TABLE_SELECT_CELL ||
        name == kcanvas::EVENT_TABLE_CURSOR_MOVE)
    {
        updateFormatItems();
    }

    // if we're in the layout view, we need to update the design view
    // for appropriate table events
    if (!isDesignView())
    {
        if (name == kcanvas::EVENT_TABLE_CURSOR_MOVE ||
            name == kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN ||
            name == kcanvas::EVENT_TABLE_RESIZE_COLUMN ||
            name == kcanvas::EVENT_TABLE_RESIZING_COLUMN ||
            name == kcanvas::EVENT_TABLE_PRE_RESIZE_ROW ||
            name == kcanvas::EVENT_TABLE_RESIZE_ROW ||
            name == kcanvas::EVENT_TABLE_RESIZING_ROW ||
            name == kcanvas::EVENT_TABLE_PRE_SELECT_CELL ||
            name == kcanvas::EVENT_TABLE_SELECT_CELL ||
            name == kcanvas::EVENT_TABLE_SELECTING_CELL ||
            name == kcanvas::EVENT_TABLE_CELL_EDIT)
        {
            onCanvasLayoutTableEvent(evt);
        }
    }

    // send everything else to a special component 
    // event handler function
    onCanvasComponentEvent(evt);
}

void ReportDoc::onCanvasScrollEvent(kcanvas::IEventPtr evt)
{
    // update the canvas; because the canvas update may
    // take a small amount of time, update the scrollbar 
    // after the canvas to keep the two in sync visibly;
    // if we do it the other way around, the scrollbars 
    // update while the canvas may lag slightly, causing 
    // the two to look slightly out of sync
    getCanvas()->layout();
    getCanvas()->render();

    // update the scrollbars
    updateScrollBars();
}

void ReportDoc::onCanvasZoomEvent(kcanvas::IEventPtr evt)
{
    // update the canvas; because the canvas update may
    // take a small amount of time, update the scrollbar 
    // after the canvas to keep the two in sync visibly;
    // if we do it the other way around, the scrollbars 
    // update while the canvas may lag slightly, causing 
    // the two to look slightly out of sync
    if (m_refresh_on_zoom)
    {
        getCanvas()->layout();
        getCanvas()->render();
    }

    // update the format zoom combo with the current zoom
    wxString zoom_percent;
    zoom_percent = wxString::Format(wxT("%d"), getCanvas()->getZoom());
    m_format_bar->m_zoom_combo->SetValue(zoom_percent + wxT("%"));

    // update the scrollbars
    updateScrollBars();
}

void ReportDoc::onCanvasCursorEvent(kcanvas::IEventPtr evt)
{
    if (evt->getPhase() != kcanvas::EVENT_BUBBLING_PHASE)
        return;

    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;
        
    kcanvas::PropertyValue prop;
    notify_evt->getProperty(kcanvas::EVENT_PROP_CURSOR, prop);
    wxString cursor = prop.getString();
    
    if (cursor == kcanvas::CURSOR_INVALID)
        SetCursor(m_cursors[cursorInvalid]);
        
    if (cursor == kcanvas::CURSOR_STANDARD)
        SetCursor(m_cursors[cursorStandard]);

    // TODO: re-enable; for now, don't show NS cursor in
    // layout mode        
    if (cursor == kcanvas::CURSOR_SIZENS && isDesignView())
        SetCursor(m_cursors[cursorSizeNS]);
        
    if (cursor == kcanvas::CURSOR_SIZEWE)
        SetCursor(m_cursors[cursorSizeWE]);
        
    if (cursor == kcanvas::CURSOR_SIZENWSE)
        SetCursor(m_cursors[cursorSizeNWSE]);

    if (cursor == kcanvas::CURSOR_SIZENESW)
        SetCursor(m_cursors[cursorSizeNESW]);

    if (cursor == kcanvas::CURSOR_MOVE)
        SetCursor(m_cursors[cursorMove]);

    if (cursor == kcanvas::CURSOR_TEXTEDIT)
        SetCursor(m_cursors[cursorTextEdit]);

    if (cursor == kcanvas::CURSOR_CROSS)
        SetCursor(m_cursors[cursorCross]);
}

void ReportDoc::onCanvasKeyEvent(kcanvas::IEventPtr evt)
{
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    if (evt->getName() != kcanvas::EVENT_KEY)
        return;

    if (evt->getPhase() != kcanvas::EVENT_CAPTURING_PHASE)
        return;

    int line = wxMax(canvas->dtom_y(5), canvas->getViewHeight()*kcanvas::SCROLL_LINE_PERC/100);
    int page = wxMax(canvas->dtom_y(5), canvas->getViewHeight()*kcanvas::SCROLL_PAGE_PERC/100);

    kcanvas::IKeyEventPtr key_evt = evt;
    if (key_evt.isNull())
        return;

    bool handled = false;        
    int key_code = key_evt->getKeyCode();
    switch (key_code)
    {
        case WXK_UP:
        case WXK_NUMPAD_UP:
        {
            if (!isDesignView())
            {
                // scroll the canvas if we're in the layout view;
                // if we're in the design view, we want the cursor
                // to move instead
                canvas->scrollBy(0, -line);
                handled = true;
            }
        }
        break;
            
        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
        {
            if (!isDesignView())
            {
                // scroll the canvas if we're in the layout view;
                // if we're in the design view, we want the cursor
                // to move instead
                canvas->scrollBy(0, line);
                handled = true;
            }
        }
        break;

        case WXK_PAGEUP:
        {
            canvas->scrollBy(0, -page);
            handled = true;
        }
        break;

        case WXK_PAGEDOWN:
        {
            canvas->scrollBy(0, page);
            handled = true;
        }
        break;
        
        case WXK_DELETE:
        {
            clearText();
            handled = true;
        }
        break;
    }
    
    // if we handled the event, stop the default handling
    if (handled)
        evt->stopPropogation();
}

void ReportDoc::onCanvasMouseEvent(kcanvas::IEventPtr evt)
{
}

void ReportDoc::onCanvasFocusEvent(kcanvas::IEventPtr evt)
{
}

void ReportDoc::onCanvasDropEvent(kcanvas::IEventPtr evt)
{
    kcanvas::IDragDropEventPtr drop_evt = evt;
    if (drop_evt.isNull())
        return;

    // if the drop target isn't a table, we're done
    kcanvas::ICompTablePtr table = evt->getTarget();
    if (table.isNull())
        return;

    // TODO: seems like the event should carry the format

    // get the received data format
    wxDataFormat format = m_canvas_data_object->GetReceivedFormat();
    
    // if it's invalid, we're done
    if (format == wxDF_INVALID)
        return;

    // track the changes
    track();

    // find the cell we're dropping into
    int row, col;
    kcanvas::IDragDropEventPtr drop_event = evt;
    table->getCellIdxByPos(drop_evt->getX(), drop_evt->getY(), &row, &col);

    // if the drop data is from the file system, get the 
    // file info
    if (format == wxDataFormat(FS_DATA_OBJECT_FORMAT))
    {
        // get the file system drop item
        IFsItemEnumPtr fs_items = m_fs_data_object->getFsItems();
        IFsItemPtr fs_item = fs_items->getItem(0);
        IDbObjectFsItemPtr item = fs_item;
        
        std::vector<wxString> res;
        std::vector<wxString>::iterator it;
        DbDoc::getFsItemPaths(fs_items, res, true);
        
        
        if (res.size() != 1)
        {
            // more than one item was dragged in, bail out
            appMessageBox(_("Only one item can be selected as the source for a report."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            
            // rollback the changes
            rollback();
            
            // set the document focus to the report
            setDocumentFocus();
            return;
        }

        wxString path = res[0];
        if (!g_app->getDatabase()->getFileExist(towstr(path)) ||
            item.isNull())
        {
            // the path couldn't be found, bail out
            appMessageBox(_("The selected item could not be found in the project."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            
            // rollback the changes
            rollback();

            // set the document focus to the report
            setDocumentFocus();
            return;
        }
        
        /*
        // TODO: reactivate: try to set the data source; if it
        // fails, trigger an error message
        
        if (item->getType() != dbobjtypeSet)
        {
            // the item is not a table, bail out
            appMessageBox(_("Only tables can be selected as the source for a report."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            
            // rollback the changes
            rollback();
            
            // set the document focus to the report
            setDocumentFocus();
            return;
        }
        */

        // set the data source and reset the report layout
        setDataSource(path);
        resetReportLayout();
        
        // if we're in the design view, set the focus to
        // the detail component detail view
        if (isDesignView())
        {
            kcanvas::ICompTablePtr section_table;
            section_table = getDesignComponentRaw()->getSectionTableByName(PROP_REPORT_DETAIL);

            if (section_table.isOk())
                m_canvas->setFocus(section_table);

        }

        // if we're in layout view, reset the y position
        if (!isDesignView())
        {
            int view_x, view_y;
            m_canvas->getViewOrigin(&view_x, &view_y);
            m_canvas->setViewOrigin(view_x, 0);
        }
    }
  
    // if the drop data is from the column list, find the cell
    // where the column is being dropped and fill it with a
    // data source tag
    if (format == wxDataFormat(kcl::getGridDataFormat(wxT("fieldspanel"))))
    {
        // get the value of the column we're dropping
        kcl::GridDraggedCells cells = m_grid_data_object->getDraggedCells();
        kcl::GridDraggedCells::iterator it, it_end;
        it_end = cells.end();
        
        int row_end = row;
        int col_end = col;
        
        int idx = 0;
        for (it = cells.begin(); it != it_end; ++it)
        {
            wxString fieldname = (*it)->m_strvalue;
            fieldname.Trim(true);
            fieldname.Trim(false);
            
            wxString qfieldname = xd::quoteIdentifier(g_app->getDatabase(), towstr(fieldname));
            wxString value = wxT("=") + qfieldname;

            kcanvas::CellRange range(row_end, col_end);
            table->setCellProperty(range, kcanvas::PROP_CONTENT_VALUE, value);
            col_end++;
        }

        // move the cursor to the start of the newly inserted values 
        // and select the newly inserted values 
        kcanvas::CellRange s(row, col, row_end, col_end-1);
        table->setCursorPos(row, col);
        
        table->removeAllCellSelections();
        table->selectCells(s);
    }

    // update the canvas
    updateCanvas();

    // commit the changes
    commit();
    
    // set the document focus to the report
    setDocumentFocus();
}

void ReportDoc::onCanvasSelectEvent(kcanvas::IEventPtr evt)
{
    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    wxString name = evt->getName();
    if (name == kcanvas::EVENT_PRE_SELECT)
    {
        // don't allow any selections    
        notify_evt->veto();
    }
}

void ReportDoc::onCanvasLayoutEvent(kcanvas::IEventPtr evt)
{
    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    kcanvas::ICanvasPtr canvas = evt->getTarget();

    // if we're at the beginning of the layout sequence
    // update the layout of the design component regardless
    // of what view we're in; this will ensure that if we
    // start in the layout view, that the design component 
    // is ready for use (the layout component uses the
    // design component for the layout); however, for the 
    // layout component, we need only refresh it if we're 
    // in the layout view
    if (evt->getName() == kcanvas::EVENT_PRE_LAYOUT)
    {
        // note: the canvas limits the view position to the
        // model at the end of the layout; we do it here
        // since the layout of the components depends on
        // the view
        int x_view_origin, y_view_origin;
        m_canvas->getViewOrigin(&x_view_origin, &y_view_origin);
        
        int x_view_size, y_view_size;
        m_canvas->getViewSize(&x_view_size, &y_view_size);

        int x_model_size, y_model_size;
        m_canvas->getSize(&x_model_size, &y_model_size);

        if (x_view_origin > x_model_size - x_view_size)
            x_view_origin = x_model_size - x_view_size;
        if (y_view_origin > y_model_size - y_view_size)
            y_view_origin = y_model_size - y_view_size;
        if (x_view_origin < 0)
            x_view_origin = 0;
        if (y_view_origin < 0)
            y_view_origin = 0;
        
        m_canvas->setViewOrigin(x_view_origin, y_view_origin);
    
        // layout the design component
        getDesignComponentRaw()->layout();
        updateDesignComponentMargins();

        if (!isDesignView())
            getLayoutComponentRaw()->layout();
    }

    // if we're at the end of the layout sequence, update the
    // table controls and the scrollbars; note: we have to update
    // the scrollbars manually since we're using our own scrollbars
    // rather than the ones provided by the canvas
    if (evt->getName() == kcanvas::EVENT_LAYOUT &&
        !canvas.isNull())
    {
        // update the canvas size, table controls and 
        // the scrollbars
        updateCanvasSize();
        updateScrollBars();
    }
}

void ReportDoc::onCanvasRenderEvent(kcanvas::IEventPtr evt)
{
    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    // if we're not rendering the canvas, don't do
    // anything else
    kcanvas::ICanvasPtr canvas = evt->getTarget();
    if (canvas.isNull())
        return;

    // if the document is newly created, and we're in
    // design view, draw the overlay text
    if (isCreated() && isDesignView())
    {
        // get the dimensions of the detail portion
        // of the design component

        kcanvas::IComponentPtr detail_section;
        detail_section = getDesignComponentRaw()->getSectionTableByName(PROP_REPORT_DETAIL);

        if (!detail_section.isNull())
        {
            // default overlay settings; note: set these before getting
            // the font dimensions so that the font positioning is
            // accurate
            
            kcanvas::Font overlay_font;

            #ifdef WIN32
                overlay_font.setFaceName(wxT("Arial"));
            #else 
                overlay_font.setFaceName(wxT("Bitstream Vera Sans"));
            #endif

            overlay_font.setStyle(kcanvas::FONT_STYLE_NORMAL);
            overlay_font.setWeight(kcanvas::FONT_WEIGHT_NORMAL);
            overlay_font.setUnderscore(kcanvas::FONT_UNDERSCORE_NORMAL);
            overlay_font.setSize(16);            

            wxString overlay_text1 = _("To set the data source for this report, drag in");
            wxString overlay_text2 = _("a table from the Project Panel or double-click here");
            m_canvas->setFont(overlay_font);
            m_canvas->setTextForeground(kcanvas::COLOR_BLACK);

            // calculate the overlay position
            int x, y, width, height;
            detail_section->getSize(&width, &height);
            m_canvas->getComponentPos(detail_section, &x, &y);

            int text1_width, text1_height, text2_width, text2_height;
            m_canvas->getTextExtent(overlay_text1, &text1_width, &text1_height);
            m_canvas->getTextExtent(overlay_text2, &text2_width, &text2_height);

            int text_height = (text1_height + text2_height);
            int text1_xpos = (m_canvas->getViewWidth() - text1_width)/2;
            int text2_xpos = (m_canvas->getViewWidth() - text2_width)/2;
            int text1_ypos = y + (height - text_height)/2;
            int text2_ypos = y + (height - text_height)/2 + text1_height*1.125;

            // draw the overlay text; make sure the draw origin is at (0,0)
            m_canvas->setDrawOrigin(0, 0);
            m_canvas->drawText(overlay_text1, text1_xpos, text1_ypos);
            m_canvas->drawText(overlay_text2, text2_xpos, text2_ypos);
        }
    }

    // update the status bar
    updateStatusBar();
}

void ReportDoc::onCanvasEditBeginEvent(kcanvas::IEventPtr evt)
{
    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    // track the changes
    track();

    // set the editing flag
    m_editing = true;
}

void ReportDoc::onCanvasEditEndEvent(kcanvas::IEventPtr evt)
{
    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    // set the editing flag
    m_editing = false;

    // see if the edit event is accepted
    kcanvas::PropertyValue value;
    notify_evt->getProperty(kcanvas::EVENT_PROP_EDIT_ACCEPTED, value);
    bool is_accepted = value.getBoolean();

    // if we didn't accept the change, we're done
    if (!is_accepted)
    {
        rollback();
        return;
    }

    // commit the changes
    commit();
}

void ReportDoc::onCanvasRightClick(kcanvas::IEventPtr evt)
{
    // get the event name
    wxString name = evt->getName();

    // see if we clicked on a row header or a column header
    bool column_header = false;
    bool row_header = false;

    kcanvas::IComponentPtr comp = evt->getTarget();

    kcanvas::PropertyValue value;
    if (!comp.isNull() && comp->getProperty(NAME_TABLE_ROW_HEADER, value))
        row_header = true;
        
    if (!comp.isNull() && comp->getProperty(NAME_TABLE_COLUMN_HEADER, value))
        column_header = true;
    
    // popup menu
    wxMenu menuPopup;
    menuPopup.Append(ID_Edit_Cut, _("Cu&t"));
    menuPopup.Append(ID_Edit_Copy, _("&Copy"));
    menuPopup.Append(ID_Edit_Paste, _("&Paste"));
    menuPopup.AppendSeparator();
    
    if (isDesignView())
    {
        menuPopup.Append(ID_Frame_ShowColumnList, _("Insert &Fields..."));

        if (name == kcanvas::EVENT_TABLE_CELL_RIGHT_CLICK &&
            !row_header && !column_header)
        {
            wxMenu* submenuFormula = new wxMenu;
            submenuFormula->Append(ID_FormulaCurrentDate, _("<Current Date>"));
            submenuFormula->Append(ID_FormulaDataSource, _("<Data Source>"));
            submenuFormula->AppendSeparator();
            submenuFormula->Append(ID_FormulaPageNumber, _("<Page Number>"));
            submenuFormula->Append(ID_FormulaPageCount, _("<Page Count>"));
            submenuFormula->Append(ID_FormulaPageNumberOfPageCount, _("<Page Number> of <Page Count>"));
            submenuFormula->Append(ID_FormulaPagePageNumber, _("Page <Page Number>"));
            submenuFormula->Append(ID_FormulaPagePageNumberOfPageCount, _("Page <Page Number> of <Page Count>"));
            submenuFormula->AppendSeparator();
            submenuFormula->Append(ID_FormulaCustom, _("Custom Formula..."));
            
            menuPopup.AppendSubMenu(submenuFormula, _("Insert For&mula"));
        }

        menuPopup.AppendSeparator();

        if (name == kcanvas::EVENT_TABLE_CELL_RIGHT_CLICK)
        {
            // if we clicked on a row, add menu items to insert 
            // and delete rows
            if (row_header)
            {
                menuPopup.Append(ID_Table_InsertRows, _("&Insert Row"));
                menuPopup.Append(ID_Table_DeleteRows, _("&Delete Row"));
            }

            // if we clicked on a column, add menu items to 
            // insert and delete columns
            if (column_header)
            {
                menuPopup.Append(ID_Table_InsertColumns, _("&Insert Column"));
                menuPopup.Append(ID_Table_DeleteColumns, _("&Delete Column"));
            }
        }

        menuPopup.Append(ID_Table_ClearCells, _("Clear Co&ntents"));
        menuPopup.AppendSeparator();
    }
    
    menuPopup.Append(ID_Format_Settings, _("&Report Settings..."));

    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);

    // show the popup menu and capture the result
    CommandCapture* cc = new CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);
    
    switch (command)
    {
        case ID_FormulaCustom:
        case ID_FormulaPageNumber:
        case ID_FormulaPageCount:
        case ID_FormulaPagePageNumber:
        case ID_FormulaPageNumberOfPageCount:
        case ID_FormulaPagePageNumberOfPageCount:
        case ID_FormulaCurrentDate:
        case ID_FormulaDataSource:
            insertFormula(command);
            break;

        default:
            {
                wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, command);
                ::wxPostEvent(this, e);
            }
            break;
    }

    // return the focus back to the report
    setDocumentFocus();
}

void ReportDoc::onCanvasComponentEvent(kcanvas::IEventPtr evt)
{
    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    wxString name = evt->getName();

    // start tracking changes at the beginning of canvas-changing events
    if (name == kcanvas::EVENT_TABLE_PRE_POPULATE_CELL ||
        name == kcanvas::EVENT_TABLE_PRE_MOVE_CELL)
    {
        // track the changes
        track();
    }

    // commit changes at the end of canvas-changing events
    if (name == kcanvas::EVENT_TABLE_POPULATE_CELL ||
        name == kcanvas::EVENT_TABLE_MOVE_CELL)
    {
        // commit the changes
        commit();
    }

    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN ||
        name == kcanvas::EVENT_TABLE_PRE_RESIZE_ROW)
    {
        // show a tooltip
        wxString text = _("Size");

        kcanvas::PropertyValue prop;
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, prop);
        text += wxString::Format(wxT(": %4.2f"), (double)prop.getInteger()/kcanvas::CANVAS_MODEL_DPI);

        // set the default tootip delay; note: other parts of the 
        // application use different delays (e.g. report writer), 
        // so set the delay each time the tooltip is used
        wxToolTip::SetDelay(10);
        setToolTip(text);
    
        // track the changes
        track();
    }

    if (name == kcanvas::EVENT_TABLE_RESIZING_COLUMN ||
        name == kcanvas::EVENT_TABLE_RESIZING_ROW)
    {
        // set the tooltip, refreshing the position
        // show a tooltip
        wxString text = _("Size");

        kcanvas::PropertyValue prop;
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, prop);
        text += wxString::Format(wxT(": %4.2f"), (double)prop.getInteger()/kcanvas::CANVAS_MODEL_DPI);

        setToolTip(text);
    }

    if (name == kcanvas::EVENT_TABLE_RESIZE_COLUMN ||
        name == kcanvas::EVENT_TABLE_RESIZE_ROW)
    {
        // remove the tooltip
        removeToolTip();
    
        // commit the changes
        commit();
    }

    if (name == kcanvas::EVENT_TABLE_PRE_CELL_CLEAR)
    {
        // track the changes
        track();
    }
    
    if (name == kcanvas::EVENT_TABLE_CELL_CLEAR)
    {
        // commit the changes()
        commit();
    }

    if (name == EVENT_DESIGN_PRE_GROUP_RESIZE)
    {
        // track the changes
        track();
    }

    if (name == EVENT_DESIGN_GROUP_RESIZE)
    {
        // commit the changes
        commit();
    }

    if (name == kcanvas::EVENT_TABLE_PRE_CELL_EDIT)
    {
        // for now, don't do anything
    }

    if (name == kcanvas::EVENT_TABLE_CELL_EDIT)
    {
    }
}

void ReportDoc::onCanvasLayoutTableEvent(kcanvas::IEventPtr evt)
{
    // if we don't have a notify event, we're done
    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    // if the event target isn't a component, we're done
    kcanvas::IComponentPtr comp = evt->getTarget();
    if (comp.isNull())
        return;


    CompReportDesign* design_table_raw = getDesignComponentRaw();

    // get the event name
    wxString name = evt->getName();

    // move the cursor between the sections
    if (name == kcanvas::EVENT_TABLE_CURSOR_MOVE)
    {
        kcanvas::ICompTablePtr table = evt->getTarget();
        if (table.isNull())
            return;
    
        int cursor_row, cursor_col;
        table->getCursorPos(&cursor_row, &cursor_col);
    
        // get the properties of the cursor cell
        kcanvas::Properties props;
        table->getCellProperties(kcanvas::CellRange(cursor_row, cursor_col), props);

        // get the row and template section corresponding to the
        // layout row
        kcanvas::PropertyValue template_row, template_section;
        props.get(PROP_TABLE_TEMPLATE_ROW_IDX, template_row);
        props.get(PROP_TABLE_TEMPLATE_SECTION_IDX, template_section);

        design_table_raw->setActiveSectionByIdx(template_section.getInteger());
        kcanvas::ICompTablePtr section_table = design_table_raw->getActiveSectionTable();
        if (!section_table.isNull())
        {
            int row, col;
            row = template_row.getInteger();
            col = cursor_col;

            design_table_raw->removeAllCellSelections();

            section_table->setCursorPos(row, col);
            section_table->selectCells(kcanvas::CellRange(row, col));
        }
    }

    if (name == kcanvas::EVENT_TABLE_CELL_EDIT)
    {
        // if we're in the layout view, put the properties of the
        // changed cell back into the template cell corresponding 
        // to it
        kcanvas::IComponentPtr design_table = getDesignComponent();
        if (design_table.isNull())
            return;

        kcanvas::PropertyValue layout_row;
        kcanvas::PropertyValue layout_col;
        
        kcanvas::PropertyValue template_row;
        kcanvas::PropertyValue template_section;
    
        // get the properties of the cell we just edited
        notify_evt->getProperty(kcanvas::EVENT_PROP_ROW, layout_row);
        notify_evt->getProperty(kcanvas::EVENT_PROP_COLUMN, layout_col);
        
        kcanvas::Properties props;
        kcanvas::ICompTablePtr table = evt->getTarget();
        table->getCellProperties(kcanvas::CellRange(layout_row.getInteger(), 
                                                    layout_col.getInteger()), props);

        // get the row and template section corresponding to the
        // layout row
        props.get(PROP_TABLE_TEMPLATE_ROW_IDX, template_row);
        props.get(PROP_TABLE_TEMPLATE_SECTION_IDX, template_section);

        // set the template table text cell property corresponding to the
        // layout table cell that was edited; note: while the rows
        // between the layout and template tables are different, the
        // columns are the same        
        kcanvas::ICompTablePtr template_table;
        template_table = design_table_raw->getSectionTableByIdx(template_section.getInteger());
        if (!template_table.isNull())
        {
            kcanvas::PropertyValue text_prop;
            props.get(kcanvas::PROP_CONTENT_VALUE, text_prop);

            // set the text value
            kcanvas::CellRange cell(template_row.getInteger(), 
                                    layout_col.getInteger());
            template_table->setCellProperty(cell, kcanvas::PROP_CONTENT_VALUE, text_prop);

            // layout and render
            getCanvas()->layout();
            getCanvas()->render();
        }
    }

    if (!isRowHeader(comp) && !isColumnHeader(comp))
        return;

    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_ROW)
    {
        // for now, don't allow row resizing
        notify_evt->veto();
    }

    if (name == kcanvas::EVENT_TABLE_RESIZE_ROW)
    {
        // for now, do nothing
    }

    if (name == kcanvas::EVENT_TABLE_RESIZING_ROW)
    {
        // for now, do nothing 
    }

    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN)
    {
        // before we do anything, tag the design table so
        // that we can undo the temporary size properties
        // that accumulate during the real-time resizing
        design_table_raw->tag(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN);
    }

    if (name == kcanvas::EVENT_TABLE_RESIZE_COLUMN)
    {
        // restore the design table to what it was before
        // we started the real-time resizing, then add
        // on the final size properties
        design_table_raw->restore(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN);
    
        // get the column index and size that's changed
        kcanvas::INotifyEventPtr notify_evt = evt;

        kcanvas::PropertyValue index, size;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, index);
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, size);

        // update the column sizes, including selected rows
        design_table_raw->syncColumnSizes(evt->getTarget(), 
                                          index.getInteger(), 
                                          size.getInteger(), 
                                          true);

        // update the design layout so it's ready to go when
        // we switch back
        design_table_raw->setUpdateTableHeadersFlag();
        design_table_raw->layout();

        // reset the layout engine with the new layout info, but don't 
        // refresh the data
        ReportLayoutEngine& engine = getLayoutComponentRaw()->getReportLayoutEngineRef();
        initReportLayoutEngine(engine, false);

        // TODO: set the focus back to the active table

        // if one of the table columns was resized, layout 
        // and render the canvas
        getCanvas()->layout();
        getCanvas()->render();
        return;
    }
    
    if (name == kcanvas::EVENT_TABLE_RESIZING_COLUMN)
    {
        // do real-time column resizing

        // get the column index and size that's changed
        kcanvas::INotifyEventPtr notify_evt = evt;

        kcanvas::PropertyValue index, size;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, index);
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, size);

        // sync up the column that's being resized
        design_table_raw->syncColumnSizes(evt->getTarget(), 
                                          index.getInteger(), 
                                          size.getInteger(), 
                                          false);

        // reset the layout engine with the new layout info, but don't 
        // refresh the data
        ReportLayoutEngine& engine = getLayoutComponentRaw()->getReportLayoutEngineRef();
        initReportLayoutEngine(engine, false);

        // if one of the table columns was resized, layout 
        // and render the canvas
        getCanvas()->layout();
        getCanvas()->render();
        return;
    }

    if (name == kcanvas::EVENT_TABLE_PRE_SELECT_CELL)
    {
        // TODO: for now, don't allow selections on rows
        // or columns
        notify_evt->veto();
    }

    if (name == kcanvas::EVENT_TABLE_SELECT_CELL)
    {
        // update the selections
        design_table_raw->syncSelections(evt->getTarget());

        // render the canvas
        getCanvas()->layout();
        getCanvas()->render();
        return;
    }

    if (name == kcanvas::EVENT_TABLE_SELECTING_CELL)
    {
        // update the selections
        design_table_raw->syncSelections(evt->getTarget());

        // render the canvas
        getCanvas()->layout();
        getCanvas()->render();
        return;
    }
}

kcanvas::ICanvasPtr ReportDoc::getCanvas()
{
    return m_canvas;
}

kcanvas::IComponentPtr ReportDoc::getDesignComponent()
{
    return m_design_component;
}

kcanvas::IComponentPtr ReportDoc::getLayoutComponent()
{
    return m_layout_component;
}

CompReportDesign* ReportDoc::getDesignComponentRaw()
{
    CompReportDesign* comp;
    comp = static_cast<CompReportDesign*>(m_design_component.p);
    return comp;
}

CompReportLayout* ReportDoc::getLayoutComponentRaw()
{
    CompReportLayout* comp;
    comp = static_cast<CompReportLayout*>(m_layout_component.p);
    return comp;
}

void ReportDoc::loadDesignView()
{
    // set the design view flag
    m_design_view = true;

    // save the layout zoom state
    m_last_layout_zoom = getCanvas()->getZoom();

    // if we're editing and the view is changing, dispatch an end 
    // edit event so that edits stop when the view is changed;
    // TODO: we should use setFocus() instead, but this is easier 
    // right now since we don't have to save/restore the focus
    if (isEditing())
    {
        kcanvas::INotifyEventPtr notify_edit_evt;
        notify_edit_evt = kcanvas::NotifyEvent::create(kcanvas::EVENT_EDIT_END, getCanvas());
        notify_edit_evt->addProperty(kcanvas::EVENT_PROP_EDIT_ACCEPTED, true);
        
        kcanvas::IEventTargetPtr target = getCanvas();
        if (!target.isNull())
        {
            target->dispatchEvent(notify_edit_evt);
        }
    }

    // set the view origin to the origin of the design view
    int origin_x, origin_y;
    if (m_design_component.isOk())
    {
        m_design_component->getOrigin(&origin_x, &origin_y);
        m_canvas->setViewOrigin(origin_x, origin_y);
    }

    // restore the zoom; temporarily disable screen
    // refreshing so we don't see a resized view
    // of the previous view
    m_refresh_on_zoom = false;
    m_canvas->setZoom(m_last_design_zoom);
    m_refresh_on_zoom = true;

    // update the canvas
    updateCanvas();
}

void ReportDoc::loadLayoutView()
{
    // show a busy cursor
    AppBusyCursor bc;

    // set the view change flag
    bool view_change = false;
    if (isDesignView())
        view_change = true;

    // save the design zoom state
    m_last_design_zoom = getCanvas()->getZoom();

    // set the design view flag
    m_design_view = false;

    // if we're editing and the view is changing, dispatch an end 
    // edit event so that edits stop when the view is changed;
    // TODO: we should use setFocus() instead, but this is easier 
    // right now since we don't have to save/restore the focus
    if (isEditing())
    {
        kcanvas::INotifyEventPtr notify_edit_evt;
        notify_edit_evt = kcanvas::NotifyEvent::create(kcanvas::EVENT_EDIT_END, getCanvas());
        notify_edit_evt->addProperty(kcanvas::EVENT_PROP_EDIT_ACCEPTED, true);
        
        kcanvas::IEventTargetPtr target = getCanvas();
        if (!target.isNull())
        {
            target->dispatchEvent(notify_edit_evt);
        }
    }

    // if the view is changing, reset the report layout
    if (view_change)
    {
        m_canvas->setViewOrigin(0,0);
        resetReportLayout();
    }

    // restore the zoom; temporarily disable screen
    // refreshing so we don't see a resized view
    // of the previous view
    m_refresh_on_zoom = false;
    m_canvas->setZoom(m_last_layout_zoom);
    m_refresh_on_zoom = true;

    // update the canvas
    updateCanvas();
}

bool ReportDoc::isDesignView()
{
    return m_design_view;
}

void ReportDoc::resetReportLayout()
{
    // set the engine for the layout component
    ReportLayoutEngine layout_engine;
    initReportLayoutEngine(layout_engine, true);
    getLayoutComponentRaw()->setReportLayoutEngine(layout_engine);
}

void ReportDoc::initReportLayoutEngine(ReportLayoutEngine& engine, bool refresh_data)
{
    std::vector<ReportSection> sections;
    getDesignComponentRaw()->getSections(sections);
    
    wxString data_source = getDesignComponentRaw()->getDataSource();
    wxString data_filter = getDesignComponentRaw()->getDataFilter();
    
    int page_width, page_height;
    getDesignComponentRaw()->getPageSize(&page_width, &page_height);
    
    int margin_left, margin_right, margin_top, margin_bottom;
    getDesignComponentRaw()->getPageMargins(&margin_left, &margin_right, &margin_top, &margin_bottom);

    engine.init(sections,
                data_source,
                data_filter,
                page_width,
                page_height,
                margin_left,
                margin_right,
                margin_top,
                margin_bottom,
                refresh_data);
}

bool ReportDoc::setDataSource(const wxString& path)
{
    // set the data source
    getDesignComponentRaw()->setDataSource(path);

    // update the column list
    g_app->getMainFrame()->postEvent(new FrameworkEvent(FRAMEWORK_EVT_COLUMNLISTPANEL_UPDATE));
    
    return true;
}

wxString ReportDoc::getDataSource()
{
    return getDesignComponentRaw()->getDataSource();
}

void ReportDoc::setPageSize(int width, int height)
{
    getDesignComponentRaw()->setPageSize(width, height);
    resetReportLayout();
}

void ReportDoc::getPageSize(int* width, int* height)
{
    getDesignComponentRaw()->getPageSize(width, height);
}

void ReportDoc::setPageMargins(int left_margin, 
                               int right_margin, 
                               int top_margin, 
                               int bottom_margin)
{
    getDesignComponentRaw()->setPageMargins(left_margin, 
                                            right_margin, 
                                            top_margin, 
                                            bottom_margin);
    resetReportLayout();
}

void ReportDoc::getPageMargins(int* left_margin, 
                               int* right_margin, 
                               int* top_margin, 
                               int* bottom_margin)
{
    getDesignComponentRaw()->getPageMargins(left_margin, 
                                            right_margin, 
                                            top_margin, 
                                            bottom_margin);
}

int ReportDoc::getPageCount()
{
    return getLayoutComponentRaw()->getPageCount();
}

void ReportDoc::gotoPage()
{
    int page_count = getPageCount();

    // if we're in layout view, calculate the page
    // we're one; TODO: may want to do some caching
    // of page idx here; however, shouldn't be a
    // problem for reports < 1000 pages
    int page_idx = 1;
    if (!isDesignView())
    {
        int x, y;
        m_canvas->getViewOrigin(&x, &y);
        getLayoutComponentRaw()->getPageIdxByPos(x, y, &page_idx);
    }

    wxString default_val = wxString::Format(wxT("%d"), page_idx);
    
    wxString message = wxString::Format(_("Page number (1 - %d):"), page_count);
    wxTextEntryDialog dlg(this, message, _("Go To Page"), default_val);
    dlg.SetSize(260,143);
    
    if (dlg.ShowModal() == wxID_OK)
    {
        int page = wxAtoi(dlg.GetValue());
        if (page < 1)
            page = 1;
         else if (page > page_count)
            page = page_count;
        
        // get the page position
        int x, y;
        getLayoutComponentRaw()->getPagePosByIdx(page, &x, &y);

        // subtract off the page spacing
        y -= m_canvas->dtom_x(PROP_REPORT_PAGE_SPACING);
        
        // set the canvas view origin to the page; only
        // set the y parameter, since pages are arranged
        // vertically; TODO: if we ever allow multiple
        // pages horizontally, we'll have to also set
        // the x position
        int view_x, view_y;
        m_canvas->getViewOrigin(&view_x, &view_y);
        m_canvas->setViewOrigin(view_x, y);
        
        // update the canvas; note: status bar update
        // happens at the end of the canvas rendering,
        // so we don't need to explicitly update the
        // status bar here
        updateCanvas();
    }
}

bool ReportDoc::setSelectedCellProperty(const wxString& prop_name, 
                                        const kcanvas::PropertyValue& value)
{
    // start tracking the changes
    track();

    // copy the value
    kcanvas::PropertyValue value_updated = value;
    
    // see if the property transparent background color (wxNullColour)
    // if so, change the color so the table doesn't show through to
    // invalid area; TODO: if transparency is set, the table should
    // show the default background color; here, we set it to white
    // for convenience

    if (prop_name == kcanvas::PROP_COLOR_BG &&
        value_updated.getColor() == kcanvas::COLOR_NULL)
    {
        value_updated.setColor(kcanvas::COLOR_WHITE);
    }
    
    bool changed = false;
    changed = getDesignComponentRaw()->setSelectedCellProperty(prop_name, value_updated);

    // if none of the properties were set, rollback
    // the history to the state when we first began
    // tracking them
    if (!changed)
    {
        rollback();
        return false;
    }
    
    // update the canvas
    updateCanvas();
    
    // commit the changes
    commit();
    
    return true;
}

bool ReportDoc::setSelectedCellBorderProperties(const kcanvas::Properties& props)
{
    // start tracking the changes
    track();

    bool changed = false;
    changed = getDesignComponentRaw()->setSelectedCellBorderProperties(props);

    // if none of the properties were set, rollback
    // the history to the state when we first began
    // tracking them
    if (!changed)
    {
        rollback();
        return false;
    }
    
    // update the canvas
    updateCanvas();
    
    // commit the changes
    commit();
    
    return true;
}

bool ReportDoc::setMergedCells(bool merge)
{
    // start tracking the changes
    track();
    
    bool changed = false;
    changed = getDesignComponentRaw()->setMergedCells(merge);

    // if none of the properties were set, rollback
    // the history to the state when we first began
    // tracking them
    if (!changed)
    {
        rollback();
        return false;
    }

    // update the canvas
    updateCanvas();
    
    // commit the changes
    commit();
    
    return true;
}

bool ReportDoc::doSave(bool save_as)
{
    if (!isTemporary() && !save_as)
    {
        // saveFile() calls updateCaption();
        saveFile(m_file_path);
        return true;
    }

    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSave);
    dlg.setCaption(_("Save As"));

    if (dlg.ShowModal() == wxID_OK)
    {
        // saveFile() calls updateCaption();
        saveFile(dlg.getPath());
        m_temporary = false;

        g_app->getAppController()->refreshDbDoc();
        return true;
    }
     else
    {
        return false;
    }
}

bool ReportDoc::doSaveAsPdf(const wxString& location)
{
    int page_width, page_height;
    getDesignComponentRaw()->getPageSize(&page_width, &page_height);

    double width = ((double)page_width)/kcanvas::CANVAS_MODEL_DPI;
    double height = ((double)page_height)/kcanvas::CANVAS_MODEL_DPI;

    ReportPrintInfo info;
    info.setPageSize(width, height);
    info.setQuality(600);
    info.setMinPage(1);
    info.setMaxPage(1);
    info.setFromPage(1);
    info.setToPage(1);
    info.setAllPages(true);

    // create a new report output pdf; it will destroy 
    // itself when done
    ReportLayoutEngine layout_engine;
    initReportLayoutEngine(layout_engine, true);
    
    ReportOutputPdf* pdf = new ReportOutputPdf(layout_engine, info, location, false);
    pdf->sigOutputBegin().connect(this, &ReportDoc::onPrintingBegin);
    pdf->sigOutputEnd().connect(this, &ReportDoc::onPrintingEnd);
    pdf->sigOutputPage().connect(this, &ReportDoc::onPrintingPage);

    pdf->create();
    return true;
}

static bool populateReportGroupSettings(CompReportDesign* design, 
                                        ReportSettings* settings)
{
    // note: this function populates the report group settings;
    // it does this by matching the group header and footer
    // sections, and combining their info into a single report 
    // settings group and adding it to the report settings

    // split the group headers and footers into two lists
    std::vector<ReportSection> group_headers, group_footers;
    
    std::vector<ReportSection> sections;
    design->getSections(sections);
    
    std::vector<ReportSection>::iterator it, it_end;
    it_end = sections.end();

    for (it = sections.begin(); it != sections.end(); ++it)
    {
        if (it->m_type == PROP_REPORT_GROUP_HEADER)
            group_headers.push_back(*it);

        if (it->m_type == PROP_REPORT_GROUP_FOOTER)
            group_footers.push_back(*it);
    }

    // a header should always be matched with a footer; if
    // it isn't, something is wrong; return false
    if (group_headers.size() != group_footers.size())
        return false;
        
    // combine the report group header and footer into a 
    // settings group, and add it to the report settings
    int idx = 0;
    int count = group_headers.size();
    for (idx = 0; idx < count; ++idx)
    {
        ReportSection header = group_headers[idx];
        ReportSection footer = group_footers[count-idx-1];
        wxString group_id = header.m_name.BeforeFirst(wxT('.'));

        // determine the sort order
        int sort_order = ReportGroupItem::SortAsc;
        if (header.m_sort_desc)
            sort_order = ReportGroupItem::SortDesc;
    
        settings->addGroup(header.m_group_field,    // group field based on header
                           group_id,                // group id, from outermost to innermost
                           sort_order,              // sort order based on header
                           header.m_active,         // whether or not group header is visible
                           footer.m_active,         // whether or not group footer is visible
                           footer.m_page_break);    // page break based on footer
    }

    return true;
}

void ReportDoc::showReportSettings()
{
    ReportSettings settings;

    // set the properties for each of the sections
    CompReportDesign* table = getDesignComponentRaw();

    ReportPropsDialog dlg(this);
    dlg.SetTitle(_("Report Settings"));

    int page_width, page_height;
    getPageSize(&page_width, &page_height);

    int margin_left, margin_right, margin_top, margin_bottom;
    getPageMargins(&margin_left, &margin_right, &margin_top, &margin_bottom);

    settings.source = getDataSource();
    settings.orientation = (page_width > page_height) ? wxLANDSCAPE : wxPORTRAIT;
    settings.page_width = model2in(page_width);
    settings.page_height = model2in(page_height);
    settings.margin_left = model2in(margin_left);
    settings.margin_right = model2in(margin_right);
    settings.margin_top = model2in(margin_top);
    settings.margin_bottom = model2in(margin_bottom);
    settings.report_header = table->getSectionVisible(PROP_REPORT_HEADER);
    settings.report_footer = table->getSectionVisible(PROP_REPORT_FOOTER);
    settings.page_header = table->getSectionVisible(PROP_REPORT_PAGE_HEADER);
    settings.page_footer = table->getSectionVisible(PROP_REPORT_PAGE_FOOTER);

    // populate the groups in the report settings panel from
    // the design template
    populateReportGroupSettings(getDesignComponentRaw(), &settings);

    // set the dialog settings before we show it
    dlg.setSettings(settings);

    // show the dialog; if we don't press OK,
    // we're done
    if (dlg.ShowModal() != wxID_OK)
        return;

    // track the changes
    track();

    // get the report settings
    settings = dlg.getSettings();

    // set the data source
    setDataSource(settings.source);

    // set the page dimensions and margins
    setPageSize(in2model(settings.page_width), 
                in2model(settings.page_height));
    
    setPageMargins(in2model(settings.margin_left),
                   in2model(settings.margin_right),
                   in2model(settings.margin_top),
                   in2model(settings.margin_bottom));

    // set the page header/footer info
    table->setSectionVisible(PROP_REPORT_HEADER, settings.report_header);
    table->setSectionVisible(PROP_REPORT_FOOTER, settings.report_footer);
    table->setSectionVisible(PROP_REPORT_PAGE_HEADER, settings.page_header);
    table->setSectionVisible(PROP_REPORT_PAGE_FOOTER, settings.page_footer);


    // update the report sections
    std::vector<ReportSection> report_sections;
    int idx, count = settings.getGroupCount();

    for (idx = 0; idx < count; ++idx)
    {
        // report headers
        ReportGroupItem group = settings.getGroup(idx);

        // TODO: for now only allow page break on footers;
        // the algorithm supports page breaks on both, but
        // the interface currently does not
        ReportSection section;        
        section.m_name = group.id + wxT(".header");
        section.m_type = PROP_REPORT_GROUP_HEADER;
        section.m_group_field = group.name;
        //section.m_page_break = group.page_break;
        section.m_page_break = false;
        section.m_sort_desc = (group.sort == ReportGroupItem::SortDesc);
        section.m_active = group.header;

        report_sections.push_back(section);
    }

    for (idx = 0; idx < count; ++idx)
    {
        // report footers
    
        // match the top-most header with the bottom-most footer,
        // so go from larger indexes to smaller indexes
        size_t reverse_idx = count - idx - 1;
        ReportGroupItem group = settings.getGroup(reverse_idx);

        // TODO: for now only allow page break on footers;
        // the algorithm supports page breaks on both, but
        // the interface currently does not
        ReportSection section;
        section.m_name = group.id + wxT(".footer");
        section.m_type = PROP_REPORT_GROUP_FOOTER;
        section.m_group_field = group.name;
        section.m_page_break = group.page_break;
        section.m_sort_desc = (group.sort == ReportGroupItem::SortDesc);
        section.m_active = group.footer;

        report_sections.push_back(section);        
    }

    // update the report sections
    getDesignComponentRaw()->updateSections(report_sections);


    // set the focus to the active table
    m_canvas->setFocus(getDesignComponentRaw()->getActiveSectionTable());

    // if we're in the layout mode, reset the view origin
    if (!isDesignView())
        m_canvas->setViewOrigin(0,0);

    // reset the report layout
    resetReportLayout();

    // update the canvas
    updateCanvas();
    
    // commit the changes
    commit();

    // return the focus back to the report
    setDocumentFocus();
}

void ReportDoc::showFormatBar(bool show)
{
    m_main_sizer->Show(m_settings_sizer, show, true);
    Layout();
}

void  ReportDoc::insertText(const wxString& text)
{
    // if we're not in design view, we're done;
    // TODO: make this work with layout view also
    if (!isDesignView())
        return;
        
    // if we don't have an active table, we're done
    kcanvas::ICompTablePtr table = getDesignComponentRaw()->getActiveSectionTable();
    if (table.isNull())
        return;

    // if the cursor isn't visible, we're done
    if (!table->isPreference(kcanvas::CompTable::prefCursor))
        return;

    // start tracking the changes for undo/redo
    track();
 
    // get the cursor cell
    int row, col;
    table->getCursorPos(&row, &col);
    
    // set the text
    table->setCellProperty(kcanvas::CellRange(row, col), kcanvas::PROP_CONTENT_VALUE, text);
    
    // commit the changes
    commit();
    
    // render
    getCanvas()->render();
}

void ReportDoc::insertFormula(int command_id)
{
    // construct a string of text to insert
    wxString text;

    switch (command_id)
    {
        case ID_FormulaPageNumber:
            text = wxT("=PageNumber()");
            break;

        case ID_FormulaPageCount:
            text = wxT("=PageCount()");
            break;
            
        case ID_FormulaPagePageNumber:
            text = wxT("=\"Page \" + STR(PageNumber())");
            break;
            
        case ID_FormulaPageNumberOfPageCount:
            text = wxT("=STR(PageNumber()) + \" of \" + STR(PageCount())");
            break;
            
        case ID_FormulaPagePageNumberOfPageCount:
            text = wxT("=\"Page \" + STR(PageNumber()) + \" of \" + STR(PageCount())");
            break;
            
        case ID_FormulaCurrentDate:
            text = wxT("=CurDate()");
            break;
            
        case ID_FormulaDataSource:
            text = wxT("=DataSource()");
            break;

        default:
            break;
    }

    // if we're not on a custom formula, set the
    // text and we're done
    if (command_id != ID_FormulaCustom)
    {
        insertText(text);
        return;
    }
    
    // if we're on a custom formula, show the expression builder
    wxString cell_text;
    kcanvas::ICompTablePtr table = getDesignComponentRaw()->getActiveSectionTable();
    if (!table.isNull())
    {
        // if the cursor isn't visible, we're done
        if (table->isPreference(kcanvas::CompTable::prefCursor))
        {
            // get the text
            int row, col;
            table->getCursorPos(&row, &col);
            
            kcanvas::PropertyValue value;
            table->getCellProperty(kcanvas::CellRange(row, col), kcanvas::PROP_CONTENT_VALUE, value);
            cell_text = value.getString();
            
            // strip off leading/trailing spaces and the leading "="
            cell_text.Trim(true);
            cell_text.Trim(false);
            cell_text = cell_text.Mid(1);
        }
    }
    
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("FormulaBuilder"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            ExprBuilderDocPanel* panel = new ExprBuilderDocPanel;
            site = m_frame->createSite(panel,
                                       sitetypeModeless |
                                       siteHidden,
                                       -1, -1, 560, 300);
            site->setMinSize(560,300);
            site->setCaption(_("Formula"));
            site->setName(wxT("Formula"));

            panel->sigOkPressed.connect(this, &ReportDoc::onCustomFormulaOk);
            panel->setOKText(_("OK"));
            panel->setEmptyOk(true);

            // set the iterator; in the case of tables, we can get
            // it from the set; TODO: in the case of queries, 
            // there's no way to get it without running the query

            wxString data_source = getDesignComponentRaw()->getDataSource();

            xd::Structure structure = g_app->getDatabase()->describeTable(towstr(data_source));

            if (structure.isOk())
                panel->setStructure(structure);

            // set the initial expression
            panel->setExpression(cell_text);
        }

        // show the site
        site->setVisible(true);
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void ReportDoc::clearText()
{
    kcanvas::IEditPtr edit = getCanvas();
    if (edit.isNull())
        return;
    
    // track the changes
    track();
    
    // clear the selected cells in the design component
    if (m_design_component.isOk())
        getDesignComponentRaw()->clearContent(true);

    getCanvas()->layout();
    getCanvas()->render();
    
    // commit the changes
    commit();
}

void ReportDoc::setToolTip(const wxString& text)
{
    kcanvas::ICanvasControlPtr control = m_canvas;
    if (control.isNull())
        return;

    wxWindow* c = control->getWindow();        
    c->SetToolTip(text);
    
    // set the delay to zero so it opens right away
    // note: other parts of the application use different 
    // delays (e.g. report writer), so set the delay
    // each time the tooltip is used
    //wxToolTip::SetDelay(50);
}

void ReportDoc::removeToolTip()
{
    kcanvas::ICanvasControlPtr control = m_canvas;
    if (control.isNull())
        return;

    wxWindow* c = control->getWindow();        
    wxToolTip* tip = c->GetToolTip();
    
    if (tip)
    {
        #ifdef WIN32
        tip->SetWindow(NULL);
        #endif
        c->SetToolTip(NULL);
    }
}

