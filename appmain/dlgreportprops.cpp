/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-07
 *
 */


#include "appmain.h"
#include "dlgpagesetup.h"
#include "dlgdatabasefile.h"
#include "dlgreportprops.h"
#include "querytemplate.h"
#include "jsonconfig.h"


// these values correspond match 1-to-1 with the grid (and must stay that way)

enum SortDirection
{
    SortAsc = 0,
    SortDesc = 1
};

enum GroupColumnIndexes
{
    colHidden = 0,
    colGroupId = 1,
    colFieldName = 2,
    colSortDirection = 3,
    colGroupHeader = 4,
    colGroupFooter = 5,
    colPageBreak = 6
};

enum
{
    ID_Browse = wxID_HIGHEST + 1,
    ID_Source,
    ID_ReportHeader,
    ID_ReportFooter,
    ID_PageHeader,
    ID_PageFooter
};




// -- helper functions --

static int sort2combo(int sort)
{
    if (sort == ReportGroupItem::SortAsc)
        return SortAsc;
         else if (sort == ReportGroupItem::SortDesc)
        return SortDesc;
    
    return SortAsc;
}

static int combo2sort(int combo_idx)
{
    if (combo_idx == SortAsc)
        return ReportGroupItem::SortAsc;
         else if (combo_idx == SortDesc)
        return ReportGroupItem::SortDesc;
    
    return ReportGroupItem::SortAsc;
}

static std::vector<wxString> getColumnsFromSource(const std::wstring& source)
{
    std::vector<wxString> columns; 

    bool exists = g_app->getDatabase()->getFileExist(source);
    if (!exists)
        return columns;

    xd::IFileInfoPtr info = g_app->getDatabase()->getFileInfo(source);
    
    // table source
    if (info->getType() == xd::filetypeTable)
    {
        xd::IDatabasePtr db = g_app->getDatabase();
        if (db.isNull())
            return columns;

        if (!isValidTable(source, db))
            return columns;

        xd::Structure structure = db->describeTable(source);
        if (structure.isNull())
            return columns;

        size_t i, column_count = structure.getColumnCount();
        columns.reserve(column_count);

        for (i = 0; i < column_count; ++i)
        {
            columns.push_back(structure.getColumnName(i));
        }

        return columns;
    }

    // query source, node format
    if (info->getType() == xd::filetypeNode)
    {
        kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), source);
        if (!node.isOk())
            return columns;

        kl::JsonNode root_node = node["root"];
        if (!root_node.isOk())
            return columns;

        kl::JsonNode kpp_template_node = root_node["kpp_template"];
        if (!kpp_template_node.isOk())
            return columns;

        kl::JsonNode type_node = kpp_template_node["type"];
        if (!type_node.isOk())
            return columns;

        wxString type = type_node.getString();
        if (!type.CmpNoCase("query"))
        {
            QueryTemplate t;
            if (!t.load(source))
                return columns;

            return t.getOutputFields();       
        }
    }

    // query source, JSON format
    if (info->getType() == xd::filetypeStream)
    {
        std::wstring mime_type = info->getMimeType();
        if (mime_type == L"application/vnd.kx.query")
        {
            QueryTemplate t;
            if (!t.load(source))
                return columns;

            return t.getOutputFields();                
        }
    }

    // unhandled
    return columns;
}


// -- ReportPropsDialog class implementation --

BEGIN_EVENT_TABLE(ReportPropsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ReportPropsDialog::onOK)
    EVT_BUTTON(ID_Browse, ReportPropsDialog::onBrowse)
    EVT_TEXT(ID_Source, ReportPropsDialog::onSourceTextChanged)
    EVT_KCLGRID_END_EDIT(ReportPropsDialog::onGridEndEdit)
    EVT_KCLGRID_NEED_TOOLTIP_TEXT(ReportPropsDialog::onGridNeedTooltipText)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(ReportPropsDialog::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(ReportPropsDialog::onGridPreInvalidAreaInsert)
    EVT_SIZE(ReportPropsDialog::onSize)
END_EVENT_TABLE()


ReportPropsDialog::ReportPropsDialog(wxWindow* parent) :
                            wxDialog(parent, 
                                     -1,
                                     _("Report Settings"),
                                     wxDefaultPosition,
                                     wxSize(520,480),
                                     wxDEFAULT_DIALOG_STYLE |
                                     wxRESIZE_BORDER |
                                     wxCLIP_CHILDREN |
                                     wxNO_FULL_REPAINT_ON_RESIZE)
{
    SetMinSize(wxSize(520,480));
    SetMaxSize(wxSize(660,620));
}

ReportPropsDialog::~ReportPropsDialog()
{

}

int ReportPropsDialog::ShowModal()
{
    // -- create source sizer --
    
    m_source = new wxTextCtrl(this, ID_Source);
    wxStaticText* label_source = new wxStaticText(this, -1, _("Source:"));
    wxButton* browse_button = new wxButton(this, ID_Browse, _("Browse..."));
    
    wxBoxSizer* source_sizer = new wxBoxSizer(wxHORIZONTAL);
    source_sizer->Add(label_source, 0, wxALIGN_CENTER);
    source_sizer->AddSpacer(5);
    source_sizer->Add(m_source, 1, wxALIGN_CENTER);
    source_sizer->AddSpacer(5);
    source_sizer->Add(browse_button, 0, wxALIGN_CENTER);

        
    // create panels
    m_orientation_panel = new OrientationPanel(this);
    m_pagesize_panel = new PageSizePanel(this);

    
    // -- create header/footer sizer --
    
    m_report_header = new wxCheckBox(this, ID_ReportHeader, _("Show Report Header"));
    m_report_footer = new wxCheckBox(this, ID_ReportFooter, _("Show Report Footer"));
    m_page_header = new wxCheckBox(this, ID_PageHeader, _("Show Page Header"));
    m_page_footer = new wxCheckBox(this, ID_PageFooter, _("Show Page Footer"));
    
    wxBoxSizer* report_sizer = new wxBoxSizer(wxVERTICAL);
    report_sizer->AddStretchSpacer();
    report_sizer->Add(m_report_header, 0, wxEXPAND);
    report_sizer->AddStretchSpacer();
    report_sizer->AddSpacer(8);
    report_sizer->AddStretchSpacer();
    report_sizer->Add(m_report_footer, 0, wxEXPAND);
    report_sizer->AddStretchSpacer();
    
    wxBoxSizer* page_sizer = new wxBoxSizer(wxVERTICAL);
    page_sizer->AddStretchSpacer();
    page_sizer->Add(m_page_header, 0, wxEXPAND);
    page_sizer->AddStretchSpacer();
    page_sizer->AddSpacer(8);
    page_sizer->AddStretchSpacer();
    page_sizer->Add(m_page_footer, 0, wxEXPAND);
    page_sizer->AddStretchSpacer();
    
    wxStaticBox* box = new wxStaticBox(this, -1, _("Headers and Footers"));
    wxStaticBoxSizer* headerfooter_sizer = new wxStaticBoxSizer(box, wxHORIZONTAL);
    headerfooter_sizer->AddSpacer(10);
    headerfooter_sizer->Add(report_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
    headerfooter_sizer->AddSpacer(10);
    headerfooter_sizer->Add(page_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
    headerfooter_sizer->AddSpacer(10);

        
    // create margin panel (created here so it is in
    // the correct location for tab traversal)
    m_margins_panel = new MarginsPanel(this);


    // create top sizer
    wxFlexGridSizer* top_sizer = new wxFlexGridSizer(2,2,10,10);
    top_sizer->Add(m_orientation_panel, 0, wxEXPAND);
    top_sizer->Add(m_pagesize_panel, 0, wxEXPAND);
    top_sizer->Add(headerfooter_sizer, 0, wxEXPAND);
    top_sizer->Add(m_margins_panel, 0, wxEXPAND);
    top_sizer->AddGrowableCol(0, 1);


    // -- create group parameter grid --

    m_grid = new kcl::RowSelectionGrid(this);
    m_grid->setRowSelectionGridFlags(kcl::RowSelectionGrid::refreshNone);
    m_grid->setDragFormat(wxT("reportsettingsdialog_groupgrid"));
    m_grid->setAllowInvalidAreaInserts(true);
    m_grid->setAllowDeletes(true);
    m_grid->setOptionState(kcl::Grid::optGhostRow |
                           kcl::Grid::optInvalidAreaClickable, true);
    
    // we need to have a hidden column as the first column
    // in the grid because the kcl::RowSelectionGrid has some
    // issues when the first column is a dropdown...

    m_grid->createModelColumn(colHidden, wxT("Hidden"), kcl::Grid::typeCharacter, 1, 0);
    m_grid->createModelColumn(colGroupId, wxT("Group Id"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(colFieldName, _("Field Name"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(colSortDirection, _("Sort Direction"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(colGroupHeader, _("Group Header"), kcl::Grid::typeBoolean, 80, 0);
    m_grid->createModelColumn(colGroupFooter, _("Group Footer"), kcl::Grid::typeBoolean, 80, 0);
    m_grid->createModelColumn(colPageBreak, _("Page Break"), kcl::Grid::typeBoolean, 80, 0);
    m_grid->createDefaultView();
    
    // don't allow these columns to be resized
    m_grid->setColumnResizable(colHidden, false);
    m_grid->setColumnResizable(colGroupId, false);
    
    // set cell properties for the grid
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskCtrlType |
                     kcl::CellProperties::cpmaskCbChoices;
    cellprops.ctrltype = kcl::Grid::ctrltypeDropList;
    cellprops.cbchoices.push_back(_("Ascending"));
    cellprops.cbchoices.push_back(_("Descending"));
    m_grid->setModelColumnProperties(colSortDirection, &cellprops);

    // create the drop target for the grid and connect the signal
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_grid);
    drop_target->setGridDataObjectFormats(wxT("reportsettingsdialog_groupgrid"));
    drop_target->sigDropped.connect(this, &ReportPropsDialog::onGridDataDropped);
    m_grid->SetDropTarget(drop_target);


    wxButton* ok_button = new wxButton(this, wxID_OK);
    
    // create a platform standards-compliant OK/Cancel sizer
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(ok_button);
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the OK/Cancel sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    
    // create header labels
    wxStaticText* source_title = new wxStaticText(this, -1, _("Source and Layout"));
    wxStaticText* sorting_title = new wxStaticText(this, -1, _("Sorting and Grouping"));
    makeFontBold(source_title);
    makeFontBold(sorting_title);


    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(source_title, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    main_sizer->AddSpacer(8);
    main_sizer->Add(source_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    main_sizer->AddSpacer(8);
    main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(15);
    main_sizer->Add(sorting_title, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(main_sizer);
    Layout();
    
    // set the ok button as the default
    ok_button->SetDefault();
    
    // size columns in the group grid
    int w = m_grid->GetClientSize().GetWidth();
    m_grid->setColumnSize(colHidden, 0);
    m_grid->setColumnSize(colGroupId, 0);
    m_grid->setColumnSize(colSortDirection, 100);
    w -= 100;
    m_grid->setColumnSize(colGroupHeader, 85);
    w -= 85;
    m_grid->setColumnSize(colGroupFooter, 85);
    w -= 85;
    m_grid->setColumnSize(colPageBreak, 70);
    w -= 70;
    m_grid->setColumnSize(colFieldName, w);
    m_grid->refresh(kcl::Grid::refreshAll);

    // connect signals
    m_orientation_panel->sigOrientationChanged.connect(m_pagesize_panel, &PageSizePanel::onOrientationChanged);
    m_pagesize_panel->sigDimensionsChanged.connect(m_orientation_panel, &OrientationPanel::onPageDimensionsChanged);
    
    // -- set control values --
    
    m_source->SetValue(m_settings.source);
    
    m_orientation_panel->setOrientation(m_settings.orientation);
    
    m_pagesize_panel->setPageDimensions(m_settings.page_width,
                                        m_settings.page_height);
    
    m_margins_panel->setMargins(m_settings.margin_left,
                                m_settings.margin_right,
                                m_settings.margin_top,
                                m_settings.margin_bottom);
    
    m_report_header->SetValue(m_settings.report_header);
    m_report_footer->SetValue(m_settings.report_footer);
    m_page_header->SetValue(m_settings.page_header);
    m_page_footer->SetValue(m_settings.page_footer);
    
    // -- populate group grid --
    
    m_grid->deleteAllRows();
    
    size_t i, count = m_settings.getGroupCount();
    for (i = 0; i < count; ++i)
    {
        ReportGroupItem item = m_settings.getGroup(i);
        
        m_grid->insertRow(-1);
        m_grid->setCellString(i, colGroupId, item.id);
        m_grid->setCellString(i, colFieldName, item.name);
        m_grid->setCellComboSel(i, colSortDirection, sort2combo(item.sort));
        m_grid->setCellBoolean(i, colGroupHeader, item.header);
        m_grid->setCellBoolean(i, colGroupFooter, item.footer);
        m_grid->setCellBoolean(i, colPageBreak, item.page_break);
    }
    
    // update the grid based on the structure (these functions
    // will check the overlay text and refresh the grid)
    populateFieldNameDropDown();
    validateFieldNames();
    updateEnabled();
    
    // connect signals
    m_grid->sigDeletedRows.connect(this, &ReportPropsDialog::onDeletedRows);

    return wxDialog::ShowModal();
}

void ReportPropsDialog::setSettings(const ReportSettings& settings)
{
    m_settings = settings;
}

ReportSettings ReportPropsDialog::getSettings() const
{
    return m_settings;
}

void ReportPropsDialog::populateFieldNameDropDown()
{
    wxArrayString fieldnames;

    std::vector<wxString>::iterator it, it_end;
    it_end = m_source_columns.end();
    
    for (it = m_source_columns.begin(); it != it_end; ++it)
    {
        fieldnames.Add(makeProper(*it));
    }

    // populate the field name dropdown in the grid
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskCtrlType |
                     kcl::CellProperties::cpmaskCbChoices;
    cellprops.ctrltype = kcl::Grid::ctrltypeDropList;
    
    fieldnames.Sort();
    int count = fieldnames.GetCount();
    for (int i = 0; i < count; ++i)
        cellprops.cbchoices.push_back(fieldnames.Item(i));

    m_grid->setModelColumnProperties(colFieldName, &cellprops);
}

void ReportPropsDialog::validateFieldNames()
{
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
        validateFieldNameByRow(row);
    
    m_grid->refresh(kcl::Grid::refreshAll);
}

void ReportPropsDialog::validateFieldNameByRow(int row)
{
    bool found = false;
    wxString name = m_grid->getCellString(row, colFieldName);

    std::vector<wxString>::iterator it, it_end;
    it_end = m_source_columns.end();
    
    for (it = m_source_columns.begin(); it != it_end; ++it)
    {
        wxString col_name = *it;
        if (name.CmpNoCase(col_name) == 0)
        {
            found = true;
            break;
        }
    }
    
    // show an exclamation mark next to field names
    // that don't exist in the source table
    if (found)
        m_grid->setCellBitmap(row, colFieldName, wxNullBitmap);
         else
        m_grid->setCellBitmap(row, colFieldName, GETBMP(gf_exclamation_16));
}

void ReportPropsDialog::updateEnabled()
{
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void ReportPropsDialog::checkOverlayText()
{
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(_("Double-click here to add a group"));
         else
        m_grid->setOverlayText(wxEmptyString);
}

void ReportPropsDialog::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    drop_target->doRowDrag(false);
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void ReportPropsDialog::onDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
}

void ReportPropsDialog::onSourceTextChanged(wxCommandEvent& evt)
{
    m_settings.source = m_source->GetValue();
    std::wstring source = towstr(m_source->GetValue());

    m_source_columns = getColumnsFromSource(source);
    
    populateFieldNameDropDown();
    validateFieldNames();
    updateEnabled();
}

void ReportPropsDialog::onGridEndEdit(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col == colFieldName)
    {
        validateFieldNameByRow(row);
        m_grid->refresh(kcl::Grid::refreshAll);
    }
    
    evt.Skip();
}

void ReportPropsDialog::onGridNeedTooltipText(kcl::GridEvent& evt)
{
    if (m_grid->getVisibleState() == kcl::Grid::stateDisabled)
        return;
    
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    // don't know why, but for some reason this event's row number
    // is sometimes -1, which causes a crash
    if (row < 0 || col < 0)
        return;
    
    if (col != colFieldName)
        return;
    
    wxBitmap bitmap;
    int alignment;
    
    m_grid->getCellBitmap(row, colFieldName, &bitmap, &alignment);
    if (bitmap.IsSameAs(GETBMP(gf_exclamation_16)))
    {
        wxString msg = _("This field name does not exist in the specified source table");
        evt.SetString(msg);
    }
}

void ReportPropsDialog::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();

    m_grid->insertRow(-1);
    int row = m_grid->getRowCount()-1;
    
    // add a default field to the grid
    ReportGroupItem item;
    m_grid->setCellComboSel(row, colFieldName, 0);  // first valid field name
    m_grid->setCellComboSel(row, colSortDirection, item.sort);
    m_grid->setCellBoolean(row, colGroupHeader, item.header);
    m_grid->setCellBoolean(row, colGroupFooter, item.footer);
    m_grid->setCellBoolean(row, colPageBreak, item.page_break);
    
    checkOverlayText();

    m_grid->moveCursor(row, colHidden, false);
    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void ReportPropsDialog::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();

    m_grid->insertRow(-1);
    int row = m_grid->getRowCount()-1;
    
    // add a default field to the grid
    ReportGroupItem item;
    m_grid->setCellComboSel(row, colFieldName, 0);  // first valid field name
    m_grid->setCellComboSel(row, colSortDirection, item.sort);
    m_grid->setCellBoolean(row, colGroupHeader, item.header);
    m_grid->setCellBoolean(row, colGroupFooter, item.footer);
    m_grid->setCellBoolean(row, colPageBreak, item.page_break);
    
    checkOverlayText();

    m_grid->moveCursor(row, colHidden, false);
    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void ReportPropsDialog::onOK(wxCommandEvent& evt)
{
    // fill out the report settings from the interface
    
    m_settings.orientation = m_orientation_panel->getOrientation();
    
    m_pagesize_panel->getPageDimensions(&m_settings.page_width,
                                        &m_settings.page_height);
    
    m_margins_panel->getMargins(&m_settings.margin_left,
                                &m_settings.margin_right,
                                &m_settings.margin_top,
                                &m_settings.margin_bottom);
    
    m_settings.report_header = m_report_header->GetValue();
    m_settings.report_footer = m_report_footer->GetValue();
    m_settings.page_header = m_page_header->GetValue();
    m_settings.page_footer = m_page_footer->GetValue();
    
    // -- fill out the group info --
    
    m_settings.clearGroups();
    
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        wxString name = m_grid->getCellString(row, colFieldName);
        wxString id = m_grid->getCellString(row, colGroupId);
        int sort = combo2sort(m_grid->getCellComboSel(row, colSortDirection));
        bool header = m_grid->getCellBoolean(row, colGroupHeader);
        bool footer = m_grid->getCellBoolean(row, colGroupFooter);
        bool page_break = m_grid->getCellBoolean(row, colPageBreak);

        m_settings.addGroup(name, id, sort, header, footer, page_break);
    }
    
    evt.Skip();
}

void ReportPropsDialog::onBrowse(wxCommandEvent& evt)
{
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeOpen);
    dlg.setCaption(_("Select Source Table"));
    dlg.setAffirmativeButtonLabel(_("OK"));
    if (dlg.ShowModal() != wxID_OK)
        return;
        
    m_source->SetValue(dlg.getPath());
}

void ReportPropsDialog::onSize(wxSizeEvent& evt)
{
    Layout();
}


