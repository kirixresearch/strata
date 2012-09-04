/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-05-20
 *
 */


#include "appmain.h"
#include "panelindex.h"
#include "tabledoc.h"
#include "fieldlistcontrol.h"


enum
{
    ID_AvailableFieldsList = wxID_HIGHEST + 1,
    ID_AddIndexButton,
    ID_DeleteIndexButton,
    ID_IndexesList
};


// rule definition functions for validating a GridActionRule

static void onCheckInvalidChars(kcl::Grid* grid,
                         int col,
                         std::vector<int>& retval)
{
    retval.clear();
    
    std::wstring s, bad_chars;
    bad_chars = g_app->getDatabase()->getAttributes()->
                getStringAttribute(tango::dbattrTableInvalidChars);
    
    int row, row_count = grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        s = towstr(grid->getCellString(row, col));
        
        // if the cell contains an invalid character,
        // add it to the return vector
        if (wcspbrk(s.c_str(), bad_chars.c_str()))
            retval.push_back(row);
    }
}

static void onCheckInvalidStartingChars(kcl::Grid* grid,
                                        int col,
                                        std::vector<int>& retval)
{
    retval.clear();
    
    wchar_t* c;
    std::wstring s, bad_chars;
    bad_chars = g_app->getDatabase()->getAttributes()->
                getStringAttribute(tango::dbattrTableInvalidStartingChars);
    
    int row, row_count = grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        s = towstr(grid->getCellString(row, col));
        c = (wchar_t*)(s.c_str());
        
        // if the cell contains an invalid starting character,
        // add it to the return vector
        if (wcschr(bad_chars.c_str(), *c))
            retval.push_back(row);
    }
}


// functions to add a GridActionRule to a validator

static kcl::GridActionRule* addCheckInvalidCharsRule(kcl::GridActionValidator* validator,
                                              int col)
{
    wxString error_text = _("Index names cannot contain the following characters:");
    error_text += wxT(" ");
    error_text += towx(g_app->getDatabase()->getAttributes()->
                       getStringAttribute(tango::dbattrTableInvalidChars));
    
    kcl::GridActionRule* rule = validator->addRule();
    rule->setColumn(col);
    rule->setErrorColumn(col);
    rule->setErrorText(error_text);
    rule->sigRuleCheckRequested.connect(&onCheckInvalidChars);
    return rule;
}

static kcl::GridActionRule* addCheckInvalidStartingCharsRule(kcl::GridActionValidator* validator,
                                                      int col)
{
    wxString error_text = _("Index names cannot begin with the following characters:");
    error_text += wxT(" ");
    error_text += towx(g_app->getDatabase()->getAttributes()->
                       getStringAttribute(tango::dbattrTableInvalidStartingChars));
    
    kcl::GridActionRule* rule = validator->addRule();
    rule->setColumn(col);
    rule->setErrorColumn(col);
    rule->setErrorText(error_text);
    rule->sigRuleCheckRequested.connect(&onCheckInvalidStartingChars);
    return rule;
}




BEGIN_EVENT_TABLE(IndexPanel, wxPanel)
    EVT_BUTTON(wxID_OK, IndexPanel::onOK)
    EVT_BUTTON(wxID_CANCEL, IndexPanel::onCancel)
    EVT_BUTTON(ID_AddIndexButton, IndexPanel::onAddIndex)
    EVT_BUTTON(ID_DeleteIndexButton, IndexPanel::onDeleteIndex)
    EVT_KCLGRID_CURSOR_MOVE(IndexPanel::onGridCursorMove)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(IndexPanel::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(IndexPanel::onGridPreInvalidAreaInsert)
END_EVENT_TABLE()


IndexPanel::IndexPanel(ITableDocPtr tabledoc)
{
    m_selected_index_info = NULL;
    m_available_fields = NULL;
    m_index_fields = NULL;
    
    m_set = tabledoc->getBaseSet();
    if (m_set.isOk())
    {
        wxString temps = towx(m_set->getObjectPath());
        if (m_set->isTemporary())
            m_set_path = wxEmptyString;
             else
            m_set_path = temps;
        
        // store the set structure
        m_structure = m_set->getStructure();
    }
}

IndexPanel::~IndexPanel()
{
    m_ok_button = NULL;
}


bool IndexPanel::initDoc(cfw::IFramePtr frame,
                         cfw::IDocumentSitePtr site,
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
    
    wxString caption = _("Indexes");
    if (m_set_path.Length() > 0)
    {
        caption += wxT(" - [");
        caption += m_set_path;
        caption += wxT("]");
    }
    
    m_doc_site = site;
    m_doc_site->setCaption(caption);
    
    // create indexes list

    m_indexes_list = new kcl::RowSelectionGrid(this,
                                               ID_IndexesList,
                                               wxDefaultPosition,
                                               wxDefaultSize,
                                               kcl::DEFAULT_BORDER,
                                               false,
                                               false);
    m_indexes_list->setAllowInvalidAreaInserts(true);
    m_indexes_list->setAllowDeletes(true);
    m_indexes_list->setOptionState(kcl::Grid::optColumnResize, false);
    m_indexes_list->createModelColumn(0, _("Indexes"), kcl::Grid::typeCharacter, 512, 0);
    m_indexes_list->createDefaultView();
    m_indexes_list->setColumnProportionalSize(0, 1);
    populateIndexesList();
    
    // add rules for validating index names (and do an initial check)
    
    m_indexes_list_validator = new kcl::GridActionValidator(m_indexes_list, GETBMP(gf_exclamation_16));
    
    kcl::GridActionRule* rule;
    rule = m_indexes_list_validator->addRule(0, kcl::GridActionValidator::RuleNoDuplicates);
    rule->setErrorText(_("This index has the same name as another index in this list"));
    
    rule = m_indexes_list_validator->addRule(0, kcl::GridActionValidator::RuleNoSpaces);
    rule->setErrorText(_("Index names cannot contain spaces"));
    
    addCheckInvalidCharsRule(m_indexes_list_validator, 0);
    addCheckInvalidStartingCharsRule(m_indexes_list_validator, 0);
    
    m_indexes_list_validator->validate();
    
    // create available fields list
    m_available_fields = new FieldListControl(this, ID_AvailableFieldsList);
    m_available_fields->setDragFormat(wxT("indexpanel_fields"));
    m_available_fields->setColumnCaption(FieldListControl::ColNameIdx, _("Available Fields"));
    m_available_fields->setStructure(m_structure);
    
    // create index fields list
    m_index_fields = new kcl::RowSelectionGrid(this);
    m_index_fields->setAllowDeletes(true);
    m_index_fields->setDragFormat(wxT("indexpanel_output"));
    m_index_fields->setGreenBarInterval(0);
    m_index_fields->createModelColumn(-1,
                                      _("Index Fields"),
                                      kcl::Grid::typeCharacter,
                                      256,
                                      0);
    m_index_fields->createModelColumn(-1,
                                      _("Sort Direction"),
                                      kcl::Grid::typeCharacter,
                                      256,
                                      0);
    m_index_fields->createDefaultView();
    
    // the list of field names should not be editable
    kcl::CellProperties props;
    props.mask = kcl::CellProperties::cpmaskEditable;
    props.editable = false;
    m_index_fields->setModelColumnProperties(0, &props);
    
    // the sort order should be a drop list
    props.mask = kcl::CellProperties::cpmaskCtrlType |
                 kcl::CellProperties::cpmaskCbChoices;
    props.ctrltype = kcl::Grid::ctrltypeDropList;
    props.cbchoices.push_back(_("Ascending"));
    props.cbchoices.push_back(_("Descending"));
    m_index_fields->setModelColumnProperties(1, &props);

    int index_row_height = m_index_fields->getRowHeight()-2;
    m_index_fields->setRowHeight(index_row_height);
    
    // create add index button
    wxButton* add_index_button = new wxButton(this,
                                             ID_AddIndexButton,
                                             _("Add"),
                                             wxDefaultPosition,
                                             wxDefaultSize,
                                             wxBU_EXACTFIT);
    wxSize s = add_index_button->GetSize();
    s.SetWidth(s.GetWidth()+10);
    add_index_button->SetMinSize(s);
    
    // create delete index button
    m_delete_index_button = new wxButton(this,
                                         ID_DeleteIndexButton,
                                         _("Delete"),
                                         wxDefaultPosition,
                                         wxDefaultSize,
                                         wxBU_EXACTFIT);
    s = m_delete_index_button->GetSize();
    s.SetWidth(s.GetWidth()+10);
    m_delete_index_button->SetMinSize(s);
    
    // create the drop target for the grid and connect the signal
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_index_fields);
    drop_target->sigDropped.connect(this, &IndexPanel::onGridDataDropped);
    drop_target->setGridDataObjectFormats(wxT("indexpanel_fields"), wxT("indexpanel_output"));
    m_index_fields->SetDropTarget(drop_target);
    
    // create horizontal sizer
    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer->Add(m_indexes_list, 1, wxEXPAND);
    horz_sizer->AddSpacer(8);
    horz_sizer->Add(m_available_fields, 1, wxEXPAND);
    horz_sizer->AddSpacer(8);
    horz_sizer->Add(m_index_fields, 1, wxEXPAND);
    
    
    // create a platform standards-compliant OK/Cancel sizer --
    
    m_ok_button = new wxButton(this, wxID_OK);
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(m_ok_button);
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->Prepend(m_delete_index_button, 0, wxALIGN_CENTER | wxLEFT, 4);
    ok_cancel_sizer->Prepend(add_index_button, 0, wxALIGN_CENTER | wxLEFT, 8);
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

    // create pseudo-proportional columns in the index fields list
    int w, h;
    m_index_fields->GetClientSize(&w, &h);
    m_index_fields->setColumnSize(0, w/2);
    m_index_fields->setColumnSize(1, w-(w/2));
    
    // update the delete index button and the index field list
    checkEnabled();
    
    // check overlay text
    checkOverlayText();
    
    // refresh the index info based on the row selection in the indexes list
    m_indexes_list->clearSelection();
    m_indexes_list->setRowSelected(0, true);
    m_indexes_list->moveCursor(0, 0);
    m_indexes_list->refresh(kcl::Grid::refreshAll);
    refreshIndexInfo();
    
    // connect signals
    m_available_fields->sigFieldDblClicked.connect(this, &IndexPanel::onAvailableFieldsDblClicked);
    m_index_fields->sigDeletedRows.connect(this, &IndexPanel::onDeletedIndexFields);
    m_indexes_list->sigDeletingRows.connect(this, &IndexPanel::onDeletingIndexes);
    m_indexes_list->sigDeletedRows.connect(this, &IndexPanel::onDeletedIndexes);
    
    m_ok_button->SetDefault();
    
    return true;
}

wxWindow* IndexPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void IndexPanel::setDocumentFocus()
{

}

static void expr2vec(IndexInfo* info)
{
    // parse the index expression into a vector of IndexColumnInfo
    wxArrayString arr;
    wxStringTokenizer tkz(info->expr, wxT(","));
    while (tkz.HasMoreTokens())
    {
        IndexColumnInfo c;
        c.ascending = true;
        c.name = tkz.GetNextToken();
        c.name.Trim(false);
        c.name.Trim();
        
        if (c.name.Length() > 0)
        {
            // handle "DESC" in the expression
            if (c.name.AfterLast(wxT(' ')).CmpNoCase(wxT("DESC")) == 0)
            {
                c.name = c.name.BeforeLast(wxT(' '));
                c.ascending = false;
            }
            
            info->cols.push_back(c);
        }
    }
}

static void vec2expr(IndexInfo* info)
{
    wxString expr;
    
    // create the index expression from the vector of IndexColumnInfo
    std::vector<IndexColumnInfo>::iterator it;
    for (it = info->cols.begin(); it != info->cols.end(); ++it)
    {
        expr += it->name;
        
        if (!it->ascending)
            expr += wxT(" DESC");
        
        if (it != info->cols.end()-1)
            expr += wxT(",");
    }
    
    info->expr = expr;
}

std::vector<IndexInfo*> IndexPanel::getAllIndexes()
{
    std::vector<IndexInfo*> retval;
    
    int row, row_count = m_indexes_list->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        IndexInfo* info = (IndexInfo*)m_indexes_list->getRowData(row);
        info->name = m_indexes_list->getCellString(row, 0);
        
        // create the index expression from the vector of IndexColumnInfo
        vec2expr(info);
        
        retval.push_back(info);
    }
    
    return retval;
}

void IndexPanel::populateIndexesList()
{
    if (m_set.isNull())
        return;
    
    tango::IIndexInfoPtr index;
    tango::IIndexInfoEnumPtr indexes;
    indexes = g_app->getDatabase()->getIndexEnum(m_set->getObjectPath());
    
    // temporary vector for sorting
    std::vector<IndexInfo*> info_vec;
    
    int i, count = indexes->size();
    for (i = 0; i < count; ++i)
    {
        // get the index we're going to store
        index = indexes->getItem(i);
        
        // create the index info to be stored as row data
        IndexInfo* info = new IndexInfo;
        info->orig_name = towx(index->getTag());
        info->name = towx(index->getTag());
        info->expr = towx(index->getExpression());
        
        // parse the index expression into a vector of IndexColumnInfo
        expr2vec(info);
        
        // insert the row in the indexes list
        m_indexes_list->insertRow(-1);
        
        // store the name and the row data
        m_indexes_list->setCellString(i, 0, info->name);
        m_indexes_list->setRowData(i, (long)info);
    }
}

void IndexPanel::populateIndexFieldsList()
{
    // clear out the index fields list
    m_index_fields->deleteAllRows();
    
    IndexInfo* info = m_selected_index_info;
    if (!info)
    {
        // refresh the index fields list
        checkOverlayText();
        m_index_fields->refresh(kcl::Grid::refreshAll);
        return;
    }
    
    // populate the index fields list with the index info
    int i, count = info->cols.size();
    for (i = 0; i < count; ++i)
    {
        wxString col_name = cfw::makeProper(info->cols[i].name);
        insertIndexColumn(i, col_name, &(info->cols[i]));
    }
    
    // refresh the index fields list
    checkOverlayText();
    m_index_fields->refresh(kcl::Grid::refreshAll);
}

void IndexPanel::checkEnabled()
{
    // enable/disable the delete index button and the index fields list
    
    if (m_indexes_list->getRowCount() == 0)
    {
        m_delete_index_button->Enable(false);
        
        m_index_fields->deleteAllRows();
        m_index_fields->setOverlayText(wxEmptyString);
        m_index_fields->refresh(kcl::Grid::refreshAll);
        m_index_fields->Enable(false);
        m_index_fields->setVisibleState(kcl::Grid::stateDisabled);
    }
     else
    {
        m_delete_index_button->Enable(true);
        
        m_index_fields->Enable(true);
        checkOverlayText();
        m_index_fields->refresh(kcl::Grid::refreshAll);
        m_index_fields->setVisibleState(kcl::Grid::stateVisible);
    }
}

void IndexPanel::checkOverlayText()
{
    if (m_indexes_list->getRowCount() == 0)
        m_indexes_list->setOverlayText(_("To create an index on this table,\nclick the 'Add' button below"));
         else
        m_indexes_list->setOverlayText(wxEmptyString);
    
    // set the overlay text for the index fields list
    if (m_index_fields->IsEnabled())
    {
        if (m_index_fields->getRowCount() == 0)
            m_index_fields->setOverlayText(_("Select fields from the\n'Available Fields' list\nand drag them here"));
             else
            m_index_fields->setOverlayText(wxEmptyString);
    }
}

void IndexPanel::insertIndexColumn(int row,
                                   const wxString& col_name,
                                   IndexColumnInfo* col_info)
{
    // insert a new row in the index fields grid
    m_index_fields->insertRow(row);
    if (row == -1)
        row = m_index_fields->getRowCount()-1;
    
    // determine if this field is a dynamic field
    bool dynamic = false;
    tango::IColumnInfoPtr colinfo = m_structure->getColumnInfo(towstr(col_name));
    if (colinfo.isOk())
        dynamic = colinfo->getCalculated();
    
    // set the cell text and bitmap information
    
    m_index_fields->setCellString(row, 0, col_name);
    m_index_fields->setCellBitmap(row, 0, dynamic ? GETBMP(gf_lightning_16) :
                                                    GETBMP(gf_field_16),
                                                    kcl::Grid::alignLeft);
    if (col_info)
        m_index_fields->setCellComboSel(row, 1, col_info->ascending ? 0 : 1);
         else
        m_index_fields->setCellComboSel(row, 1, 0); // ascending
}

void IndexPanel::refreshIndexInfo()
{
    // save the selected index info before we move to a different index
    if (m_selected_index_info)
    {
        IndexInfo* info = m_selected_index_info;
        
        info->cols.clear();
        
        int i, count = m_index_fields->getRowCount();
        for (i = 0; i < count; ++i)
        {
            IndexColumnInfo c;
            c.name = m_index_fields->getCellString(i,0);
            c.ascending = (m_index_fields->getCellComboSel(i,1) == 0) ? true : false;
            info->cols.push_back(c);
        }
    }
    
    
    // load (and store) the index info for the selected row
    if (m_indexes_list->getRowCount() > 0)
    {
        int row = m_indexes_list->getCursorRow();
        m_selected_index_info = (IndexInfo*)m_indexes_list->getRowData(row);
    }
     else
    {
        // no rows in indexes list means no selected index info
        m_selected_index_info = NULL;
    }
    
    // populate the index fields list
    populateIndexFieldsList();
}

void IndexPanel::onOK(wxCommandEvent& evt)
{
    if (!m_indexes_list_validator->validate())
    {
        cfw::appMessageBox(_("One or more of the index names is invalid.  Please make sure all index names are valid to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    if (m_indexes_list->getRowCount() > 0 &&
        m_index_fields->getRowCount() == 0)
    {
        cfw::appMessageBox(_("No fields exist in the selected index.  Please add at least one field to the index to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    // make sure we save the index info to the row data
    refreshIndexInfo();
    
    sigOkPressed(this);
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void IndexPanel::onCancel(wxCommandEvent& evt)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}

wxString IndexPanel::getUniqueIndexName()
{
    wxString base = _("index");
    wxString retval;
    
    int row, row_count = m_indexes_list->getRowCount();
    int counter = row_count+1;
    
    while (1)
    {
        retval = base + wxString::Format(wxT("%d"), counter);
        bool dup = false;
        
        for (row = 0; row < row_count; ++row)
        {
            wxString name = m_indexes_list->getCellString(row, 0);
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

void IndexPanel::onAddIndex(wxCommandEvent& evt)
{
    m_indexes_list->Freeze();
    
    // create the new index's row data
    IndexInfo* info = new IndexInfo;
    info->name = getUniqueIndexName();
    
    // add a new row to the indexes list
    int row = m_indexes_list->getRowCount();
    m_indexes_list->insertRow(-1);
    m_indexes_list->setCellString(row, 0, info->name);
    m_indexes_list->setRowData(row, (long)info);
    m_indexes_list->clearSelection();
    m_indexes_list->setRowSelected(row, true);
    m_indexes_list->moveCursor(row, 0);
    
    // save the selected index info
    m_selected_index_info = info;
    
    // update the delete index button and the index fields list
    checkEnabled();
    
    // validate the new entry in the indexes list
    m_indexes_list_validator->validate(true);
    
    m_indexes_list->refresh(kcl::Grid::refreshAll);
    m_indexes_list->Thaw();
}

void IndexPanel::onDeleteIndex(wxCommandEvent& evt)
{
    // don't allow deletes if more than one row is selected
    if (m_indexes_list->getSelectedRowCount() > 1)
        return;
    
    // don't allow deletes if there's only one index left
    if (m_indexes_list->getRowCount() == 0)
        return;
    
    // delete the selected row
    m_indexes_list->deleteSelectedRows();
    
    // validate the new entry in the indexes list
    m_indexes_list_validator->validate(true);
    
    // refresh the indexes list
    m_indexes_list->refresh(kcl::Grid::refreshAll);
    
    // since we've deleted a row, the row that is selected has new index info
    // associated with it, so we need to refresh the index info
    refreshIndexInfo();
}

void IndexPanel::onGridCursorMove(kcl::GridEvent& evt)
{
    // only process this event for the indexes list
    if (evt.GetId() != ID_IndexesList)
    {
        evt.Skip();
        return;
    }
    
    refreshIndexInfo();
    
    evt.Skip();
}

void IndexPanel::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    // only process this event for the indexes list
    if (evt.GetId() != ID_IndexesList)
    {
        evt.Skip();
        return;
    }
    
    // we'll handle the processing of this event ourselves
    evt.Veto();
    
    // add an index to the end of the indexes list
    wxCommandEvent unused;
    onAddIndex(unused);
}

void IndexPanel::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    // only process this event for the indexes list
    if (evt.GetId() != ID_IndexesList)
    {
        evt.Skip();
        return;
    }
    
    // we'll handle the processing of this event ourselves
    evt.Veto();
    
    // add an index to the end of the indexes list
    wxCommandEvent unused;
    onAddIndex(unused);
}

void IndexPanel::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    int drop_row = drop_target->getDropRow();
    wxWindowID source_id = drop_target->getSourceGridId();
    
    if (source_id == ID_AvailableFieldsList)
    {
        kcl::GridDraggedCells cells = drop_target->getDraggedCells();
        kcl::GridDraggedCells::iterator it;
        for (it = cells.begin(); it != cells.end(); ++it)
        {
            insertIndexColumn(drop_row, cfw::makeProper((*it)->m_strvalue));
            drop_row++;
        }
    }
     else
    {
        drop_target->doRowDrag(false);
    }
    
    checkOverlayText();
    m_index_fields->refresh(kcl::Grid::refreshAll);
}

void IndexPanel::onAvailableFieldsDblClicked(int row, const wxString& text)
{
    // double-clicked on a field item
    insertIndexColumn(-1, cfw::makeProper(text));
    checkOverlayText();
    m_index_fields->refresh(kcl::Grid::refreshAll);
}

void IndexPanel::onDeletingIndexes(std::vector<int> rows, bool* allow)
{
    // don't allow the delete if we only have one index left
    if (m_indexes_list->getRowCount() == 0)
        *allow = false;
}

void IndexPanel::onDeletedIndexes(std::vector<int> rows)
{
    // check to see if we should enable/disable the delete button
    // and the index fields list
    checkEnabled();
    
    // NOTE: only the indexes list will be refreshed by the
    //       kcl::RowSelectedGrid after this signal is processed,
    //       so we need to refresh the index fields list here ourselves
    checkOverlayText();
    m_index_fields->refresh(kcl::Grid::refreshAll);
}

void IndexPanel::onDeletedIndexFields(std::vector<int> rows)
{
    // NOTE: the index fields list will be refreshed by the
    //       kcl::RowSelectedGrid after this signal is processed
    checkOverlayText();
}




