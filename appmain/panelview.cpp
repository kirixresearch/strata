/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-10-31
 *
 */


#include "appmain.h"
#include "panelview.h"
#include "tabledoc.h"
#include "fieldlistcontrol.h"


enum
{
    ID_AvailableFieldsList = wxID_HIGHEST + 1,
    ID_AddViewButton,
    ID_DeleteViewButton,
    ID_ViewsList
};

wxString label_separator; // see the ViewPanel constructor for the value of this


BEGIN_EVENT_TABLE(ViewPanel, wxPanel)
    EVT_BUTTON(wxID_OK, ViewPanel::onOK)
    EVT_BUTTON(wxID_CANCEL, ViewPanel::onCancel)
    EVT_BUTTON(ID_AddViewButton, ViewPanel::onAddView)
    EVT_BUTTON(ID_DeleteViewButton, ViewPanel::onDeleteView)
    EVT_KCLGRID_CURSOR_MOVE(ViewPanel::onGridCursorMove)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(ViewPanel::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(ViewPanel::onGridPreInvalidAreaInsert)
END_EVENT_TABLE()


ViewPanel::ViewPanel(ITableDocPtr tabledoc)
{
    if (label_separator.IsEmpty())
    {
        label_separator = _("-- Column Break --");
        #ifdef _UNICODE
        // this is like this for two reasons
        //    1) VC6 doesn't like \u escape sequences
        //    2) non-unicode builds don't support characters over 255
        wxChar solid_dash[2];
        solid_dash[0] = 0x2014;
        solid_dash[1] = 0;
        label_separator.Replace(wxT("-"), solid_dash);
        #endif
    }
    
    m_selected_view_info = NULL;
    m_available_fields = NULL;
    m_visible_fields = NULL;
    
    tango::ISetPtr set = tabledoc->getBaseSet();
    if (set.isOk())
    {
        if (isTemporaryTable(towstr(tabledoc->getPath())))
            m_set_path = "";
             else
            m_set_path = tabledoc->getPath();
        
        m_structure = set->getStructure();
    }
    
    // store all existing views
    ITableDocModelPtr model = tabledoc->getModel();
    if (model.isOk())
        m_views = model->getViewEnum();
    
    // store the original view we started with
    ITableDocViewPtr view = tabledoc->getActiveView();
    if (view.isOk())
        m_original_view = view;
}

ViewPanel::~ViewPanel()
{
    m_ok_button = NULL;
}

// -- IDocument --
bool ViewPanel::initDoc(IFramePtr frame,
                        IDocumentSitePtr site,
                        wxWindow* docsite_wnd,
                        wxWindow* panesite_wnd)
{
    bool result = Create(docsite_wnd,
                         -1,
                         wxDefaultPosition,
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN);
    if (!result)
        return false;
    
    wxString caption = _("Column Views");
    if (m_set_path.Length() > 0)
    {
        caption += wxT(" - [");
        caption += m_set_path;
        caption += wxT("]");
    }
    
    m_doc_site = site;
    m_doc_site->setCaption(caption);
    
    // create views list
    m_views_list = new kcl::RowSelectionGrid(this,
                                             ID_ViewsList,
                                             wxDefaultPosition,
                                             wxDefaultSize,
                                             kcl::DEFAULT_BORDER,
                                             false,
                                             false);
    m_views_list->setAllowInvalidAreaInserts(true);
    m_views_list->setAllowDeletes(true);
    m_views_list->setOptionState(kcl::Grid::optColumnResize, false);
    m_views_list->createModelColumn(0, _("Views"), kcl::Grid::typeCharacter, 512, 0);
    m_views_list->createDefaultView();
    m_views_list->setColumnProportionalSize(0, 1);
    m_views_list->setAllowInvalidAreaInserts(false);
    populateViewsList();
    
    // create available fields list
    m_available_fields = new FieldListControl(this, ID_AvailableFieldsList);
    m_available_fields->setDragFormat(wxT("viewpanel_fields"));
    m_available_fields->setColumnCaption(FieldListControl::ColNameIdx, _("Available Fields"));
    m_available_fields->addCustomItem(label_separator);
    m_available_fields->setStructure(m_structure);
    
    // create visible fields list
    m_visible_fields = new kcl::RowSelectionGrid(this);
    m_visible_fields->setOptionState(kcl::Grid::optColumnResize |
                                     kcl::Grid::optEdit, false);
    m_visible_fields->setAllowDeletes(true);
    m_visible_fields->setDragFormat(wxT("viewpanel_output"));
    m_visible_fields->setGreenBarInterval(0);
    m_visible_fields->createModelColumn(-1,
                                        _("Visible Fields"),
                                        kcl::Grid::typeCharacter,
                                        256,
                                        0);
    m_visible_fields->createDefaultView();

    int view_row_height = m_visible_fields->getRowHeight()-2;
    m_visible_fields->setRowHeight(view_row_height);
    m_visible_fields->setColumnProportionalSize(0, 1);
    
    // create add view button
    wxButton* add_view_button = new wxButton(this,
                                             ID_AddViewButton,
                                             _("Add"),
                                             wxDefaultPosition,
                                             wxDefaultSize,
                                             wxBU_EXACTFIT);
    wxSize s = add_view_button->GetSize();
    s.SetWidth(s.GetWidth()+10);
    add_view_button->SetMinSize(s);
    
    // create delete view button
    m_delete_view_button = new wxButton(this,
                                        ID_DeleteViewButton,
                                        _("Delete"),
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxBU_EXACTFIT);
    s = m_delete_view_button->GetSize();
    s.SetWidth(s.GetWidth()+10);
    m_delete_view_button->SetMinSize(s);
    
    // update the delete view button
    if (m_views_list->getRowCount() <= 1)
        m_delete_view_button->Enable(false);
    
    // create the drop target for the grid and connect the signal
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_visible_fields);
    drop_target->sigDropped.connect(this, &ViewPanel::onGridDataDropped);
    drop_target->setGridDataObjectFormats(wxT("viewpanel_fields"), wxT("viewpanel_output"));
    m_visible_fields->SetDropTarget(drop_target);
    
    // create horizontal sizer
    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer->Add(m_views_list, 1, wxEXPAND);
    horz_sizer->AddSpacer(8);
    horz_sizer->Add(m_available_fields, 1, wxEXPAND);
    horz_sizer->AddSpacer(8);
    horz_sizer->Add(m_visible_fields, 1, wxEXPAND);
    
    
    // -- create a platform standards-compliant OK/Cancel sizer --
    
    m_ok_button = new wxButton(this, wxID_OK);
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(m_ok_button);
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->Prepend(m_delete_view_button, 0, wxALIGN_CENTER | wxLEFT, 4);
    ok_cancel_sizer->Prepend(add_view_button, 0, wxALIGN_CENTER | wxLEFT, 8);
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the OK/Cancel sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(horz_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(main_sizer);
    Layout();
    
    // if we've been supplied with an edit set, populate the view grid
    int row = 0;
    if (m_original_view.isOk())
    {
        ITableDocObjectPtr v1, v2 = m_original_view;
        
        int i, count = m_views->size();
        for (i = 0; i < count; ++i)
        {
            ITableDocViewPtr view = m_views->getItem(i);
            v1 = view;
            
            if (v1->getObjectId() == v2->getObjectId())
            {
                row = i;
                break;
            }
        }
    }
    
    // refresh the view info based on the row selection in the views list
    m_views_list->clearSelection();
    m_views_list->setRowSelected(row, true);
    m_views_list->moveCursor(row, 0);
    m_views_list->refresh(kcl::Grid::refreshAll);
    refreshViewInfo();
    
    // connect signals
    m_available_fields->sigFieldDblClicked.connect(this, &ViewPanel::onAvailableFieldsDblClicked);
    m_visible_fields->sigDeletedRows.connect(this, &ViewPanel::onDeletedVisibleFields);
    m_views_list->sigDeletingRows.connect(this, &ViewPanel::onDeletingViews);
    m_views_list->sigDeletedRows.connect(this, &ViewPanel::onDeletedViews);
    
    m_ok_button->SetDefault();
    
    return true;
}

wxWindow* ViewPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ViewPanel::setDocumentFocus()
{
}

void ViewPanel::updateTableDocViewEnum(ITableDocPtr tabledoc)
{
    // make sure the view info is up-to-date
    refreshViewInfo();
    
    ITableDocModelPtr model = tabledoc->getModel();
    if (model.isNull())
        return;
    
    ITableDocViewEnumPtr views = model->getViewEnum();
    if (views.isNull())
        return;
    
    // clear out all existing views
    int i, view_count = (int)views->size();
    for (i = view_count-1; i >= 0 ; --i)
    {
        ITableDocViewPtr view = views->getItem(i);
        model->deleteObject(view);
    }
    
    // repopulate the views enum
    int cursor_row = m_views_list->getCursorRow();
    int row, row_count = m_views_list->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        ViewInfo* info = (ViewInfo*)m_views_list->getRowData(row);
        
        ITableDocViewPtr view = model->createViewObject();
        view->setDescription(towstr(m_views_list->getCellString(row, 0)));
        
        // don't save views that had no columns
        if (info->cols.size() == 0)
            continue;
        
        i = 0;
        std::vector<ColumnViewInfo>::iterator it;
        for (it = info->cols.begin(); it != info->cols.end(); ++it)
        {
            ITableDocViewColPtr col = view->createColumn(i++);
            col->setName(towstr(it->name));
            col->setSize(it->size);
            col->setAlignment(it->alignment);
            col->setForegroundColor(it->fg_color);
            col->setBackgroundColor(it->bg_color);
        }
        
        // save the view
        model->writeObject(view);
        
        // set the active view
        if (row == cursor_row)
            tabledoc->setActiveView(view);
    }
}

void ViewPanel::checkOverlayText()
{
    // this isn't the best place for this, but it's convenient
    if (m_ok_button)
        m_ok_button->Enable(m_visible_fields->getRowCount() > 0 ? true : false);
    
    // set the overlay text
    if (m_visible_fields->getRowCount() == 0)
        m_visible_fields->setOverlayText(_("Select fields from the\n'Available Fields' list\nand drag them here"));
         else
        m_visible_fields->setOverlayText(wxEmptyString);
}

void ViewPanel::insertViewColumn(int row,
                                 const wxString& col_name,
                                 ColumnViewInfo* col_info)
{
    // insert a new row in the view fields grid
    m_visible_fields->insertRow(row);
    if (row == -1)
        row = m_visible_fields->getRowCount()-1;
    
    // determine if this field is a dynamic field
    bool dynamic = false;
    tango::IColumnInfoPtr colinfo = m_structure->getColumnInfo(towstr(col_name));
    if (colinfo.isOk())
        dynamic = colinfo->getCalculated();
    
    // -- set the cell text and bitmap information --
    
    if (col_name.CmpNoCase(label_separator) == 0 || col_name.IsEmpty())
    {
        m_visible_fields->setCellString(row, 0, label_separator);
        m_visible_fields->setCellBitmap(row, 0, wxNullBitmap,
                                        kcl::Grid::alignLeft);
    }
     else
    {
        m_visible_fields->setCellString(row, 0, col_name);
        m_visible_fields->setCellBitmap(row, 0, dynamic ? GETBMP(gf_lightning_16) :
                                                          GETBMP(gf_field_16),
                                                          kcl::Grid::alignLeft);
    }
    
    // -- set the row data for the output view --
    
    if (col_info != NULL)
    {
        ColumnViewInfo* c = new ColumnViewInfo;
        c->name = col_info->name;
        c->size = col_info->size;
        c->alignment = col_info->alignment;
        c->fg_color = col_info->fg_color;
        c->bg_color = col_info->bg_color;
        c->separator = col_info->separator;
        
        // set the visible fields row data
        m_visible_fields->setRowData(row, (long)c);
        return;
    }
     else
    {
        ColumnViewInfo* c = new ColumnViewInfo;
        if (col_name.CmpNoCase(label_separator) == 0 || col_name.IsEmpty())
        {
            // column separator row
            c->name = wxEmptyString;
            c->size = 10;
            c->separator = true;
        }
         else
        {
            c->name = col_name;
            c->size = 80;
            c->alignment = tabledocAlignDefault;
            c->fg_color = wxNullColour;
            c->bg_color = wxNullColour;
            c->separator = false;
        }
        
        // set the visible fields row data
        m_visible_fields->setRowData(row, (long)c);
    }
}

void ViewPanel::populateViewsList()
{
    ITableDocViewPtr view;
    ITableDocViewColPtr col;
    
    int i, count = m_views->size();
    for (i = 0; i < count; ++i)
    {
        // get the view we're going to store
        view = m_views->getItem(i);
        
        // insert the row in the views list
        m_views_list->insertRow(-1);
        
        // create the view info to be stored as row data
        ViewInfo* info = new ViewInfo;
        info->name = view->getDescription();
        
        // store all of the column view info in each row of the list
        int j, col_count = view->getColumnCount();
        for (j = 0; j < col_count; ++j)
        {
            col = view->getColumn(j);
            
            ColumnViewInfo colinfo;
            colinfo.name = col->getName();
            colinfo.alignment = col->getAlignment();
            colinfo.size = col->getSize();
            colinfo.bg_color = col->getBackgroundColor();
            colinfo.fg_color = col->getForegroundColor();
            colinfo.separator = (col->getName().empty() ? true : false);
            info->cols.push_back(colinfo);
        }
        
        // store the name and the row data
        m_views_list->setCellString(i, 0, view->getDescription());
        m_views_list->setRowData(i, (long)info);
    }
}

void ViewPanel::populateVisibleFieldsList()
{
    // clear out the visible fields list
    m_visible_fields->deleteAllRows();
    
    ViewInfo* info = m_selected_view_info;
    if (!info)
    {
        // refresh the visible fields list
        checkOverlayText();
        m_visible_fields->refresh(kcl::Grid::refreshAll);
        return;
    }
    
    // populate the visible fields list with the column view info
    int i, count = info->cols.size();
    for (i = 0; i < count; ++i)
    {
        wxString col_name = makeProper(info->cols[i].name);
        insertViewColumn(i, col_name, &(info->cols[i]));
    }
    
    // refresh the visible fields list
    checkOverlayText();
    m_visible_fields->refresh(kcl::Grid::refreshAll);
}

void ViewPanel::updateAvailableFieldsList()
{
    std::vector<FieldListItem> items = m_available_fields->getAllItems();
    
    std::vector<wxString> visible_fields;
    int row, row_count = m_visible_fields->getRowCount();
    for (row = 0; row < row_count; ++row)
        visible_fields.push_back(m_visible_fields->getCellString(row, 0));
    
    bool found;
    std::vector<FieldListItem>::iterator item_it;
    for (item_it = items.begin(); item_it != items.end(); ++item_it)
    {
        found = false;
        std::vector<wxString>::iterator it;
        for (it = visible_fields.begin(); it != visible_fields.end(); ++it)
        {
            if (item_it->name.CmpNoCase(*it) == 0)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
            m_available_fields->setItemEnabled(item_it->name, false);
             else
            m_available_fields->setItemEnabled(item_it->name, true);
    }
    
    m_available_fields->refresh();
}

void ViewPanel::refreshViewInfo()
{
    // save the selected view info before we move to a different view
    if (m_selected_view_info)
    {
        ViewInfo* info = m_selected_view_info;
        
        info->cols.clear();
        
        int i, count = m_visible_fields->getRowCount();
        for (i = 0; i < count; ++i)
        {
            ColumnViewInfo* col_info;
            col_info = (ColumnViewInfo*)m_visible_fields->getRowData(i);
            
            // store all of the column view info
            ColumnViewInfo c;
            c.name = col_info->name;
            c.alignment = col_info->alignment;
            c.size = col_info->size;
            c.bg_color = col_info->bg_color;
            c.fg_color = col_info->fg_color;
            c.separator = col_info->separator;
            info->cols.push_back(c);
        }
    }
    
    // load (and store) the view info for the selected row
    int row = m_views_list->getCursorRow();
    m_selected_view_info = (ViewInfo*)m_views_list->getRowData(row);
    
    // populate the visible fields list
    populateVisibleFieldsList();
    
    // make sure we update the enabled/disabled field bitmaps
    // in the available fields list
    updateAvailableFieldsList();
}


wxString ViewPanel::getUniqueViewName()
{
    wxString base = _("view");
    wxString retval;
    
    int row, row_count = m_views_list->getRowCount();
    int counter = row_count+1;
    
    while (1)
    {
        retval = base + wxString::Format(wxT("%d"), counter);
        bool dup = false;
        
        for (row = 0; row < row_count; ++row)
        {
            wxString name = m_views_list->getCellString(row, 0);
            if (retval.CmpNoCase(name) == 0)
            {
                dup = true;
                break;
            }
        }
        
        counter++;
        
        // index name is unique, we're done
        if (!dup)
            break;
    }
    
    return retval;
}

void ViewPanel::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    int drop_row = drop_target->getDropRow();
    wxWindowID source_id = drop_target->getSourceGridId();
    
    if (source_id == ID_AvailableFieldsList)
    {
        kcl::GridDraggedCells cells = drop_target->getDraggedCells();
        kcl::GridDraggedCells::iterator it;
        for (it = cells.begin(); it != cells.end(); ++it)
        {
            insertViewColumn(drop_row, (*it)->m_strvalue);
            drop_row++;
        }
    }
     else
    {
        drop_target->doRowDrag(false);
    }
    
    checkOverlayText();
    m_visible_fields->refresh(kcl::Grid::refreshAll);
    
    // make sure we update the enabled/disabled field bitmaps
    // in the available fields list
    updateAvailableFieldsList();
}

void ViewPanel::onAvailableFieldsDblClicked(int row, const wxString& text)
{
    // double-clicked on the separator item
    if (text.CmpNoCase(label_separator) == 0)
    {
        insertViewColumn(-1, label_separator);
        checkOverlayText();
        m_visible_fields->refresh(kcl::Grid::refreshAll);
        return;
    }

    // double-clicked on a field item
    insertViewColumn(-1, text);
    checkOverlayText();
    m_visible_fields->refresh(kcl::Grid::refreshAll);
    
    // make sure we update the enabled/disabled field bitmaps
    // in the available fields list
    updateAvailableFieldsList();
}

void ViewPanel::onDeletingViews(std::vector<int> rows, bool* allow)
{
    // don't allow the delete if we only have one view left
    if (m_views_list->getRowCount() <= 1)
        *allow = false;
}

void ViewPanel::onDeletedViews(std::vector<int> rows)
{
    // update the delete view button
    if (m_views_list->getRowCount() <= 1)
        m_delete_view_button->Enable(false);
    
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
}

void ViewPanel::onDeletedVisibleFields(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
    
    // make sure we update the enabled/disabled field bitmaps
    // in the available fields list
    updateAvailableFieldsList();
}

void ViewPanel::onOK(wxCommandEvent& evt)
{
    if (m_visible_fields->getRowCount() == 0)
    {
        appMessageBox(_("No fields exist in the selected view.  Please add at least one field to the view to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    // make sure we save the view info to the row data
    refreshViewInfo();

    sigOkPressed(this);
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void ViewPanel::onCancel(wxCommandEvent& evt)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void ViewPanel::onAddView(wxCommandEvent& evt)
{
    m_views_list->Freeze();
    
    int row = m_views_list->getRowCount();
    
    // create the new view's row data
    ViewInfo* info = new ViewInfo;
    info->name = getUniqueViewName();
    
    // add a new row to the views list
    m_views_list->insertRow(-1);
    m_views_list->setCellString(row, 0, info->name);
    m_views_list->setRowData(row, (long)info);
    m_views_list->clearSelection();
    m_views_list->setRowSelected(row, true);
    m_views_list->moveCursor(row, 0);
    
    // save the selected view info
    m_selected_view_info = info;
    
    // update the delete view button
    if (m_views_list->getRowCount() > 1)
        m_delete_view_button->Enable(true);
    
    m_views_list->refresh(kcl::Grid::refreshAll);
    m_views_list->Thaw();
    
    // make sure we update the enabled/disabled field bitmaps
    // in the available fields list
    updateAvailableFieldsList();
}

void ViewPanel::onDeleteView(wxCommandEvent& evt)
{
    // don't allow deletes if more than one row is selected
    if (m_views_list->getSelectedRowCount() > 1)
        return;
    
    // don't allow deletes if there's only one view left
    if (m_views_list->getRowCount() == 1)
        return;
    
    // delete the selected row
    m_views_list->deleteSelectedRows();
    
    // since we've deleted a row, the row that is selected has new view info
    // associated with it, so we need to refresh the view info
    refreshViewInfo();
    
    // make sure we update the enabled/disabled field bitmaps
    // in the available fields list
    updateAvailableFieldsList();
}

void ViewPanel::onGridCursorMove(kcl::GridEvent& evt)
{
    // only process this event for the views list
    if (evt.GetId() != ID_ViewsList)
    {
        evt.Skip();
        return;
    }
    
    refreshViewInfo();
    
    // make sure we update the enabled/disabled field bitmaps
    // in the available fields list
    updateAvailableFieldsList();
    
    evt.Skip();
}

void ViewPanel::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    // only process this event for the views list
    if (evt.GetId() != ID_ViewsList)
    {
        evt.Skip();
        return;
    }
    
    // we'll handle the processing of this event ourselves
    evt.Veto();
    
    // add a new view to the end of the grid
    wxCommandEvent unused;
    onAddView(unused);
}

void ViewPanel::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    // only process this event for the views list
    if (evt.GetId() != ID_ViewsList)
    {
        evt.Skip();
        return;
    }
    
    // we'll handle the processing of this event ourselves
    evt.Veto();
    
    // add a new view to the end of the grid
    wxCommandEvent unused;
    onAddView(unused);
}




