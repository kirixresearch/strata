/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2003-04-09
 *
 */


#ifndef H_APP_REPORTDOC_H
#define H_APP_REPORTDOC_H


#include "reportdesign.h"
#include "reportlayout.h"
#include "reportmodel.h"
#include "toolbars.h"
#include "panelcolumnlist.h"
#include "panelexprbuilderdoc.h"
#include "../kcanvas/kcanvas.h"
#include "../kcanvas/canvas.h"
#include "../kcanvas/canvasdc.h"
#include "../kcanvas/canvascontrol.h"
#include "../kcanvas/property.h"
#include "../kcanvas/dnd.h"


// report default zoom
const int PROP_REPORT_DEFAULT_ZOOM = 100;


// forward declarations
namespace kcanvas
{
    class CanvasDataObjectComposite;
};


class AppPrintProgressListener;
class PageSetupPanel;
class FormatBar;


// canvas cursor enumerations
enum ReportCursors
{
    cursorInvalid = 0,
    cursorStandard = 1,
    cursorSizeNS = 2,
    cursorSizeWE = 3,    
    cursorSizeNWSE = 4,
    cursorSizeNESW = 5,
    cursorMove = 6,    
    cursorMoveSelect = 7,
    cursorTextEdit = 8,    
    cursorTable = 9,
    cursorCross = 10
};


// interface delcaration
xcm_interface IReportDoc;
XCM_DECLARE_SMARTPTR(IReportDoc)


xcm_interface IReportDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IReportDoc")

public:

    virtual bool isDesignView() = 0;
};


// class implementation
class ReportDoc : public wxWindow,
                  public IReportDoc,
                  public IColumnListTarget,
                  public IDocument,
                  public StatusBarProviderBase,
                  public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ReportDoc")
    XCM_BEGIN_INTERFACE_MAP(ReportDoc)
        XCM_INTERFACE_ENTRY(IReportDoc)
        XCM_INTERFACE_ENTRY(IColumnListTarget)
        XCM_INTERFACE_ENTRY(IDocument)
        XCM_INTERFACE_CHAIN(StatusBarProviderBase)
    XCM_END_INTERFACE_MAP()

public:

    static bool newFile(const wxString& path);

public:

    ReportDoc();
    virtual ~ReportDoc();

    bool create(const ReportCreateInfo& data = ReportCreateInfo());
    bool print(bool show_print_dialog = true);

    bool saveFile(const wxString& path);
    bool loadFile(const wxString& path, bool layout = false);

private:

    // note: these functions are the event handlers for wx events

    // report save/print/page/data setup
    void onSave(wxCommandEvent& evt);
    void onSaveAs(wxCommandEvent& evt);
    void onSaveAsExternal(wxCommandEvent& evt);
    void onReload(wxCommandEvent& evt);
    void onPrint(wxCommandEvent& evt);
    void onPageSetup(wxCommandEvent& evt);
    void onReportSettings(wxCommandEvent& evt);

    // report zoom
    void onZoomIn(wxCommandEvent& evt);
    void onZoomOut(wxCommandEvent& evt);
    void onZoomToActual(wxCommandEvent& evt);
    void onZoomToWidth(wxCommandEvent& evt);
    void onZoomToHeight(wxCommandEvent& evt);
    void onZoomToSize(wxCommandEvent& evt);
    void onZoomChanged(wxCommandEvent& evt);

    // report tool and component creation
    void onSetToolSelect(wxCommandEvent& evt);
    void onSetToolNavigate(wxCommandEvent& evt);
    void onCreateTextBox(wxCommandEvent& evt);
    void onCreateImage(wxCommandEvent& evt);

    // report formatting
    void onFontFaceChanged(wxCommandEvent& evt);
    void onFontSizeChanged(wxCommandEvent& evt);
    void onFormatChanged(wxCommandEvent& evt);

    // component edit
    void onUndo(wxCommandEvent& evt);
    void onRedo(wxCommandEvent& evt);
    void onCut(wxCommandEvent& evt);
    void onCopy(wxCommandEvent& evt);
    void onPaste(wxCommandEvent& evt);
    void onSelectAll(wxCommandEvent& evt);
    void onSelectNone(wxCommandEvent& evt);
    void onClearText(wxCommandEvent& evt);

    void onInsertColumns(wxCommandEvent& evt);
    void onDeleteColumns(wxCommandEvent& evt);
    void onInsertRows(wxCommandEvent& evt);
    void onDeleteRows(wxCommandEvent& evt);

    // panels and toolbars
    void onGoTo(wxCommandEvent& evt);
    void onShowColumnPanel(wxCommandEvent& evt);
    void onToggleFormatToolbar(wxCommandEvent& evt);
    void onTextColorDropDown(wxAuiToolBarEvent& evt);
    void onFillColorDropDown(wxAuiToolBarEvent& evt);
    void onLineColorDropDown(wxAuiToolBarEvent& evt);
    void onBorderDropDown(wxAuiToolBarEvent& evt);

    // window
    void onSize(wxSizeEvent& evt);
    void onScroll(wxScrollWinEvent& evt);
    void onMouse(wxMouseEvent& evt);

    void onUpdateUI(wxUpdateUIEvent& evt);
    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);

private:

    // handler initialization
    bool initCanvas(kcanvas::ICanvasPtr canvas);
    bool initComponents(kcanvas::ICanvasPtr canvas);
    bool initEventHandlers(kcanvas::ICanvasPtr canvas);
    bool initDragAndDrop(kcanvas::ICanvasPtr canvas);
    bool initControls(kcanvas::ICanvasPtr canvas);
    bool initUndoRedo();
    bool initFormatToolbar();
    bool initStatusBar();
    bool initSettings();

    // IDocument
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);

    wxWindow* getDocumentWindow();
    wxString getDocumentLocation();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    void onSiteDeactivated();
    void onSiteActivated();

    // IUIContext
    IUIContextPtr getUserInterface();

    // IColumnListTarget
    void getColumnListItems(std::vector<ColumnListItem>& list);
    void onColumnListDblClicked(const std::vector<wxString>& list);

    // signal sink for frame and status bar events
    void onFrameEvent(FrameworkEvent& evt);
    void onStatusBarItemLeftDblClick(IStatusBarItemPtr item);
    
    // signal sink for custom formula builder
    void onCustomFormulaOk(ExprBuilderPanel* expr_panel);

    // signal sinks for printing status
    void onPrintingBegin(ReportOutputBase* output_engine, kcanvas::Properties& props);
    void onPrintingEnd(ReportOutputBase* output_engine, kcanvas::Properties& props);
    void onPrintingPage(ReportOutputBase* output_engine, kcanvas::Properties& props);

private:

    // note: these are the update functions that update the application
    // interface in response to changes in the report
    void updateCanvas();
    void updateCanvasSize();
    void updateCaption();
    void updateScrollBars();
    void updateStatusBar();
    void updateDesignComponentMargins();
    void updateFormatItems();

private:

    // canvas event handler functions
    void onCanvasDisableEvent(kcanvas::IEventPtr evt);
    void onCanvasEvent(kcanvas::IEventPtr evt);
    void onCanvasScrollEvent(kcanvas::IEventPtr evt);
    void onCanvasZoomEvent(kcanvas::IEventPtr evt);
    void onCanvasCursorEvent(kcanvas::IEventPtr evt);
    void onCanvasKeyEvent(kcanvas::IEventPtr evt);
    void onCanvasMouseEvent(kcanvas::IEventPtr evt);
    void onCanvasFocusEvent(kcanvas::IEventPtr evt);
    void onCanvasDropEvent(kcanvas::IEventPtr evt);
    void onCanvasSelectEvent(kcanvas::IEventPtr evt);
    void onCanvasLayoutEvent(kcanvas::IEventPtr evt);
    void onCanvasRenderEvent(kcanvas::IEventPtr evt);
    void onCanvasEditBeginEvent(kcanvas::IEventPtr evt);
    void onCanvasEditEndEvent(kcanvas::IEventPtr evt);
    void onCanvasRightClick(kcanvas::IEventPtr evt);
    void onCanvasComponentEvent(kcanvas::IEventPtr evt);
    void onCanvasLayoutTableEvent(kcanvas::IEventPtr evt);

private:

    // note: these are functions used internally to control the report

    // utility functions for undo/redo
    void track();
    void commit();
    void rollback();

    // create state flag; true if the document is newly created
    // and false otherwise
    bool isCreated();
    
    // changed state flag for save; if the data has changed since the
    // document has been created, loaded or last saved, isChanged() 
    // returns true; used for asking the user if they want to save
    // the document (when they are closing it, but haven't saved it)
    bool isChanged();

    // editing state flag to indicate whether or not the document 
    // is being edited
    bool isEditing();

    // temporary state flag to indicate whether or not the document
    // is a temporary file (newly created untitled file)
    bool isTemporary();

    // function for getting the canvas and template table
    kcanvas::ICanvasPtr getCanvas();
    kcanvas::IComponentPtr getDesignComponent();
    kcanvas::IComponentPtr getLayoutComponent();
    CompReportDesign* getDesignComponentRaw();
    CompReportLayout* getLayoutComponentRaw();

    // functions for switching views
    void loadDesignView();
    void loadLayoutView();
    bool isDesignView();

    // function for initializing the report engine
    void resetReportLayout();
    void initReportLayoutEngine(ReportLayoutEngine& engine, bool refresh_data);

    // function for setting and accessing the data source
    bool setDataSource(const wxString& path);
    wxString getDataSource();

    // functions for setting default page settings
    // and getting the page count
    void setPageSize(int width, int height);
    void getPageSize(int* width, int* height);

    void setPageMargins(int left_margin,
                        int right_margin,
                        int top_margin,
                        int bottom_margin);
    
    void getPageMargins(int* left_margin,
                        int* right_margin,
                        int* top_margin,
                        int* bottom_margin);
    
    int getPageCount();
    void gotoPage();

    // utility function for setting the properties
    // on selected cells 
    bool setSelectedCellProperty(const wxString& prop_name, const kcanvas::PropertyValue& value);
    bool setSelectedCellBorderProperties(const kcanvas::Properties& props);
    bool setMergedCells(bool merge);

    // utility function for saving files
    bool doSave(bool save_as);
    bool doSaveAsPdf(const wxString& location);

    // report settings and format bar
    void showReportSettings();
    void showFormatBar(bool show = true);

    // utility functions for text
    void insertText(const wxString& text);
    void insertFormula(int command_id);
    void clearText();

    // tooltips
    void setToolTip(const wxString& text);
    void removeToolTip();
    
private:

    // document related variables

    // print progress listener for job queue
    AppPrintProgressListener* m_print_progress;

    // doc site and frame variables
    IDocumentSitePtr m_doc_site;
    IFramePtr m_frame;

    // drag and drop data data objects
    kcl::GridDataObject* m_grid_data_object;
    FsDataObject* m_fs_data_object;
    kcanvas::CanvasDataObjectComposite* m_canvas_data_object;

    // sizers
    wxBoxSizer* m_main_sizer;
    wxBoxSizer* m_settings_sizer;

    // format toolbar
    FormatToolbar* m_format_bar;

    // format state
    wxString m_format_halignment;
    wxString m_format_valignment;
    wxString m_format_fontfacename;
    wxString m_format_fontweight;
    wxString m_format_fontstyle;
    wxString m_format_fontunderscore;
    int m_format_fontsize;
    bool m_format_cellsmerged;
    bool m_format_textwrapped;

    // mouse cursor variables
    wxCursor m_cursors[13];

private:

    // report design/layout components
    kcanvas::IComponentPtr m_design_component;
    kcanvas::IComponentPtr m_layout_component;

    // file path
    wxString m_file_path;

    // canvas control
    kcanvas::ICanvasPtr m_canvas;

    // undo/redo
    kcanvas::ICanvasHistoryPtr m_history;

    // layout/design, page/nopage view toggle
    bool m_design_view;

    // page index for printing status
    int m_printing_page_idx;

    // saved zoom state
    int m_last_layout_zoom;
    int m_last_design_zoom;
    bool m_refresh_on_zoom;

    // flags to indicate whether or not the document has been 
    // created or changed, whether it's being edited, or
    // whether it's temporary (untitled)
    bool m_created;
    bool m_changed;
    bool m_editing;
    bool m_temporary;

    // flag to indicate whether or not changes are being
    // tracked; for undo/redo
    bool m_tracking;

    DECLARE_EVENT_TABLE()
};


#endif

