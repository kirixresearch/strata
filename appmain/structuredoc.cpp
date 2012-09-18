/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2006-09-29
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "structuredoc.h"
#include "tabledoc.h"
#include "toolbars.h"
#include "jobmodifystruct.h"
#include "dlgdatabasefile.h"
#include "dlgcustomprompt.h"
#include "fieldtypechoice.h"
#include "structurevalidator.h"


// -- command ids --

enum
{
    ID_InsertField = 10000,
    ID_InsertDynamicField,
    ID_ConvertDynamicToFixed,
    ID_UpdateStatusBar
};

// -- grid column indexes --

enum
{
    colRowNumber = 0,
    colFieldName,
    colFieldType,
    colFieldWidth,
    colFieldScale,
    colFieldFormula
};

// -- utility functions --

static bool isFieldDynamic(kcl::Grid* grid, int row)
{
    StructureField* f = (StructureField*)grid->getRowData(row);
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
        wxString expression = grid->getCellString(row, colFieldFormula);
        int type = choice2tango(grid->getCellComboSel(row, colFieldType));
        bool calculated_field = false;
        StructureField* f = (StructureField*)(grid->getRowData(row));
        if (f->dynamic)
            calculated_field = true;
        
        if (fieldname.IsEmpty() && !include_empty_fieldnames)
            continue;
            
        vec.push_back(RowErrorChecker(row, fieldname, type, expression, calculated_field));
    }

    return vec;
}




BEGIN_EVENT_TABLE(StructureDoc, wxWindow)
    EVT_SIZE(StructureDoc::onSize)
    EVT_ERASE_BACKGROUND(StructureDoc::onEraseBackground)
    EVT_KCLGRID_NEED_TOOLTIP_TEXT(StructureDoc::onGridNeedTooltipText)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(StructureDoc::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(StructureDoc::onGridPreInvalidAreaInsert)
    EVT_KCLGRID_BEGIN_EDIT(StructureDoc::onGridBeginEdit)
    EVT_KCLGRID_END_EDIT(StructureDoc::onGridEndEdit)
    EVT_KCLGRID_EDIT_CHANGE(StructureDoc::onGridEditChange)
    EVT_KCLGRID_CELL_RIGHT_CLICK(StructureDoc::onGridCellRightClick)
    EVT_MENU(ID_File_Save, StructureDoc::onSave)
    EVT_MENU(ID_Edit_Copy, StructureDoc::onCopy)
    EVT_MENU(ID_InsertField, StructureDoc::onInsertField)
    EVT_MENU(ID_InsertDynamicField, StructureDoc::onInsertDynamicField)
    EVT_MENU(ID_ConvertDynamicToFixed, StructureDoc::onConvertDynamicToFixed)
    EVT_MENU(ID_Edit_Delete, StructureDoc::onDeleteField)    
    EVT_MENU(ID_Edit_SelectAll, StructureDoc::onSelectAll)
    EVT_MENU(ID_View_SwitchToDesignView, StructureDoc::onSave)

    // disable data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, StructureDoc::onUpdateUI_DisableAlways)

    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, StructureDoc::onUpdateUI_DisableAlways)

    // disable format items
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, StructureDoc::onUpdateUI_DisableAlways)

    // disable table items
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, StructureDoc::onUpdateUI_DisableAlways)

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_SaveAs, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_SaveAsExternal, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_PageSetup, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Print, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Run, StructureDoc::onUpdateUI_DisableAlways)

    // disable the zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomIn, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomOut, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToActual, StructureDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, StructureDoc::onUpdateUI_DisableAlways)

    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, StructureDoc::onUpdateUI)

END_EVENT_TABLE()


StructureDoc::StructureDoc()
{
    m_path = wxEmptyString;
    m_changed = false;
    m_modify = false;
    m_readonly = false;
    m_last_selected_fieldtype = -1;
    m_grid = NULL;
    
    m_modify_set = xcm::null;
    m_expr_edit_structure = xcm::null;
}

StructureDoc::~StructureDoc()
{
}

void StructureDoc::setModifySet(tango::ISetPtr modify_set)
{
    m_modify_set = modify_set;
    m_modify = true;
    
    // set the changed flag
    setChanged(false);

    if (m_modify_set.isOk())
    {
        if (m_modify_set->isTemporary())
        {
            m_path = _("(Untitled)");
        }
         else
        {
            m_path = towx(m_modify_set->getObjectPath());

            // fire this event so that the URL will be updated with the new path
            if (m_frame.isOk())
                m_frame->postEvent(new cfw::Event(wxT("cfw.locationChanged")));
        }

        m_readonly = false;
    }
     else
    {
        m_readonly = true;
    }
}

tango::ISetPtr StructureDoc::getModifySet()
{
    return m_modify_set;
}

bool StructureDoc::doSave()
{
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // do a read-only check
    if (!g_app->getAppController()->doReadOnlyCheck())
        return false;

    // make sure everthing checks out
    if (!doErrorCheck())
        return false;

    // as long as everything checks out, reset the changed flag 
    // so that the document is clean; do this here so that if
    // the document is dirtied simply by moving around the
    // editor, the changed state is reset, even if no actual
    // changes in the structure are saved
    setChanged(false);

    // since we're not modifying width/scale based on type dynamically 
    // (see note for updateWidthAndScale()) we need to make sure they
    // are set in conformity with our structure standards now
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
        int width = m_grid->getCellInteger(row, colFieldWidth);
        int scale = m_grid->getCellInteger(row, colFieldScale);

        StructureValidator::updateFieldWidthAndScale(type, &width, &scale);
        m_grid->setCellInteger(row, colFieldWidth, width);
        m_grid->setCellInteger(row, colFieldScale, scale);
    }
    
    // if we're not modifying an existing table, submit a create table job
    if (!m_modify)
        return createTable();
    
    
    // -- at this point, we know we're modifying an existing table --
    
    
    // create the modify job
    size_t action_count = 0;
    ModifyStructJob* job = createModifyJob(&action_count);

    // make sure there's something to do
    if (action_count == 0)
    {
        delete job;
        return true;
    }
    
    
    // -- make sure we've got a tabledoc in our container --
    ITableDocPtr table_doc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
    if (table_doc.isNull())
    {
        // -- this chunk of code exists in the view switcher code as well --
        
        // create a tabledoc and open it
        table_doc = TableDocMgr::createTableDoc();
        table_doc->open(m_modify_set, xcm::null);

        if (table_doc->getCaption().Length() == 0)
        {
            // update the caption manually
            wxString caption = m_path.AfterLast(wxT('/'));
            table_doc->setCaption(caption, wxEmptyString);
        }
        
        wxWindow* container = m_doc_site->getContainerWindow();
        g_app->getMainFrame()->createSite(container,
                                          table_doc, false);
    }
    
    // check to see if this table is a child in a relationship sync situation
    bool filtered_table = false;
    if (table_doc.isOk())
    {
        // VC6 Bug: spelling out the smart ptr assignment and comparison here
        // prevents a VC6 compiler bug.   When VC6 is no longer in use, this
        // can be compacted into:
        // if (m_modify_set == doc->getBaseSet() || m_modify_set == doc->getBrowseSet())

        tango::ISetPtr base_set = table_doc->getBaseSet();
        tango::ISetPtr browse_set = table_doc->getBrowseSet();

        if (m_modify_set == base_set || m_modify_set == browse_set)
        {
            if (table_doc->getIsChildSet())
            {
                cfw::appMessageBox(_("The structure cannot be modified while the table is showing filtered related records."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_INFORMATION | wxCENTER);
                delete job;
                return false;
            }
            
            if (table_doc->getFilter().Length() > 0)
                filtered_table = true;
        }
    }
    
    cfw::IDocumentSiteEnumPtr sites;
    sites = g_app->getMainFrame()->getDocumentSites(cfw::sitetypeNormal);
    size_t i, site_count = sites->size();
    for (i = 0; i < site_count; ++i)
    {
        cfw::IDocumentSitePtr site = sites->getItem(i);
        if (site.isNull())
            continue;
        
        ITableDocPtr doc = site->getDocument();
        if (doc.isNull())
            continue;
        

        {
            // VC6 Bug: spelling out the smart ptr assignment and comparison here
            // prevents a VC6 compiler bug.   When VC6 is no longer in use, this
            // can be compacted into:
            // if (m_modify_set == doc->getBaseSet() || m_modify_set == doc->getBrowseSet())

            tango::ISetPtr base_set = doc->getBaseSet();
            tango::ISetPtr browse_set = doc->getBrowseSet();

            if (m_modify_set == base_set || m_modify_set == browse_set)
            {
                if (doc->getIsChildSet())
                {
                    cfw::appMessageBox(_("The structure cannot be modified while the table is showing filtered related records."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_INFORMATION | wxCENTER);
                    delete job;
                    return false;
                }
            
                if (doc->getFilter().Length() > 0)
                    filtered_table = true;
            }
        }
    }

    // if we're modifying a table that we've filtered,
    // see if the user wants to continue
    if (filtered_table)
    {
        CustomPromptDlg dlg(g_app->getMainWindow(), 
                            APPLICATION_NAME,
                            _("The table you want to modify is currently showing filtered records.  Modifying the table will remove the filter.  Would you like to continue?"),
                            wxSize(380,200));
        dlg.setBitmap(CustomPromptDlg::bitmapQuestion);
        dlg.showButtons(CustomPromptDlg::showButton1 |
                        CustomPromptDlg::showButton2);
        
        int result = dlg.ShowModal();
        if (result != wxYES)
        {
            delete job;
            return false;
        }
    }


    // -- disable the structure editor grid --
    
    m_grid->setVisibleState(kcl::Grid::stateDisabled);
    m_grid->refresh(kcl::Grid::refreshAll);
    
    
    // -- close the sets --
    std::vector<ITableDoc*> to_connect;
    for (i = 0; i < site_count; ++i)
    {
        cfw::IDocumentSitePtr site = sites->getItem(i);
        if (site.isNull())
            continue;
        
        ITableDocPtr doc = site->getDocument();
        if (doc.isNull())
            continue;

        // VC6 Bug: spelling out the smart ptr assignment and comparison here
        // prevents a VC6 compiler bug.   When VC6 is no longer in use, this
        // can be compacted into:
        // if (m_modify_set == doc->getBaseSet() || m_modify_set == doc->getBrowseSet())
        // N.B. This is where the problem was manifesting itself.  The ISetPtr's were
        // being leaked and the set could not be deleted after structure modify

        tango::ISetPtr base_set = doc->getBaseSet();
        tango::ISetPtr browse_set = doc->getBrowseSet();
        
        if (m_modify_set == base_set || m_modify_set == browse_set)
        {
            to_connect.push_back(doc.p);
            doc->closeSet();
        }
    }

    // -- let the pending close events process --
    
    g_app->processIdle();
    ::wxSafeYield();

    
    // -- start the job --
    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);

    // connect the job finished signal to the structuredoc
    job->sigJobFinished().connect(this, &StructureDoc::onModifyStructJobFinished);

    // connect the job finished signal to the tabledoc
    if (table_doc.isOk())
        table_doc->connectModifyStructJob(job);
    
    // now, connect the job finished signal to all open
    // tabledocs that are showing this set
    std::vector<ITableDoc*>::iterator it;
    for (it = to_connect.begin(); it != to_connect.end(); ++it)
    {
        if (table_doc.p == (*it))
            continue;
        
        (*it)->connectModifyStructJob(job);
    }

    return true;
}

bool StructureDoc::initDoc(cfw::IFramePtr frame,
                           cfw::IDocumentSitePtr doc_site,
                           wxWindow* docsite_wnd,
                           wxWindow* panesite_wnd)
{
    // store ptrs
    m_frame = frame;
    m_doc_site = doc_site;

    // -- create document's window --
    bool result = Create(docsite_wnd,
                         -1,
                         wxDefaultPosition,
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE);
    if (!result)
        return false;

    SetBackgroundColour(wxColor(255,255,255));
    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);
    
    // update the caption
    updateCaption();
    m_doc_site->setBitmap(GETBMP(gf_table_16));

    wxFont overlay_font = wxFont(16, wxSWISS, wxNORMAL, wxNORMAL, false);
    
    // -- create grid --
    m_grid = new kcl::RowSelectionGrid(this,
                                       -1,
                                       wxPoint(0,0),
                                       docsite_wnd->GetClientSize(),
                                       wxBORDER_NONE);
    m_grid->setDragFormat(wxT("structuredoc"));
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
    m_grid->createModelColumn(colFieldFormula, _("Formula"), kcl::Grid::typeCharacter, 4096, 0);
    m_grid->createDefaultView();
    
    m_grid->setColumnSize(colRowNumber, 30);
    m_grid->setColumnSize(colFieldName, 140);
    m_grid->setColumnSize(colFieldType, 110);
    m_grid->setColumnSize(colFieldWidth, 55);
    m_grid->setColumnSize(colFieldScale, 55);
    m_grid->setColumnSize(colFieldFormula, 160);

    // -- set cell properties for field type choice control --
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
    
    // populate the grid
    if (m_modify)
        populateGridFromSet(m_modify_set);

    // number the row number column and calculate its width
    updateNumberColumn();

    // this code changes the two proportional columns to be
    // non-proportionally sized, which will make the grid fill
    // the client window size initially, but still allow the user
    // to resize each column)
    int w, h;
    m_grid->GetClientSize(&w, &h);
    w -= m_grid->getColumnSize(colRowNumber);
    w -= m_grid->getColumnSize(colFieldType);
    w -= m_grid->getColumnSize(colFieldWidth);
    w -= m_grid->getColumnSize(colFieldScale);
    m_grid->setColumnSize(colFieldName, w*30/100);
    m_grid->setColumnSize(colFieldFormula, w*70/100);
    
    checkOverlayText();
    checkInvalidExpressions(CheckMarkRows | CheckEmptyFieldnames);    
    checkInvalidFieldnames(CheckMarkRows | CheckEmptyFieldnames);    
    checkDuplicateFieldnames(CheckMarkRows | CheckEmptyFieldnames);

    // refresh the row selection grid
    if (m_grid->getRowCount() > 0)
        m_grid->setRowSelected(0, true);
    m_grid->moveCursor(0, colFieldName, false);
    m_grid->refresh(kcl::Grid::refreshAll);

    // create the statusbar items for this document
    cfw::IStatusBarItemPtr item;
    
    item = addStatusBarItem(wxT("structuredoc_field_count"));
    item->setWidth(120);
    
    item = addStatusBarItem(wxT("structuredoc_row_width"));
    item->setWidth(150);

    // make this grid a drop target and connect the signal
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_grid);
    drop_target->setGridDataObjectFormats(wxT("structuredoc"), wxT("fieldspanel"));
    drop_target->sigDropped.connect(this, &StructureDoc::onGridDataDropped);
    m_grid->SetDropTarget(drop_target);

    // add frame event handlers
    frame->sigFrameEvent().connect(this, &StructureDoc::onFrameEvent);
    
    // connect row selection grid signals
    m_grid->sigInsertingRows.connect(this, &StructureDoc::onInsertingRows);
    m_grid->sigDeletedRows.connect(this, &StructureDoc::onDeletedRows);

    return true;
}

wxString StructureDoc::getDocumentLocation()
{
    return m_path;
}

void StructureDoc::setDocumentFocus()
{
    m_grid->SetFocus();
}

bool StructureDoc::onSiteClosing(bool force)
{
    if (force)
        return true;

    if (!isChanged())
        return true;
        
    int result = cfw::appMessageBox(_("Would you like to save the changes made to the table's structure?"),
                                    APPLICATION_NAME,
                                    wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);
    
    if (result == wxYES)
    {
        doSave();
        return false;
    }
     else if (result == wxNO)
    {
        setChanged(false);
        return true;
    }
     else
    {
        return false;
    }
}

void StructureDoc::onSiteActivated()
{
    if (m_grid)
        updateStatusBar();
}

void StructureDoc::getColumnListItems(std::vector<ColumnListItem>& items)
{
    // clear the list
    items.clear();

    // make sure we have a modify set
    if (m_modify_set.isNull())
        return;
    
    // get the structure
    tango::IStructurePtr structure = m_modify_set->getStructure();
    if (structure.isNull())
        return;

    // get the column count and reserve space for the items
    int i, col_count = structure->getColumnCount();
    items.reserve(col_count);

    for (i = 0; i < col_count; i++)
    {
        tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);
     
        ColumnListItem item;
        item.text = cfw::makeProperIfNecessary(towx(colinfo->getName()));
        if (colinfo->getCalculated())
        {
            item.bitmap = GETBMP(gf_lightning_16);
        }
         else
        {
            item.bitmap = GETBMP(gf_field_16);
        }
        item.active = true;
        items.push_back(item);
    }
}

void StructureDoc::onColumnListDblClicked(const std::vector<wxString>& items)
{
    // we can only add fields from the fields panel
    // if we're modifying an existing table
    if (m_modify_set.isNull())
        return;

    // set the changed flag
    setChanged(true);

    m_grid->Freeze();

    tango::IStructurePtr s = m_modify_set->getStructure();
                
    int row = m_grid->getRowCount();

    // inserting fields in from the field panel
    std::vector<wxString>::const_iterator it;
    for (it = items.begin(); it != items.end(); ++it)
    {
        // get the column info from the column name we dragged in
        tango::IColumnInfoPtr colinfo = s->getColumnInfo(towstr(*it));
        if (colinfo.isNull())
            continue;
        
        insertRow(row, colinfo->getCalculated());
        m_grid->setCellString(row, colFieldName, towx(colinfo->getName()));
        m_grid->setCellComboSel(row, colFieldType, tango2choice(colinfo->getType()));
        m_grid->setCellInteger(row, colFieldWidth, colinfo->getWidth());
        m_grid->setCellInteger(row, colFieldScale, colinfo->getScale());
        m_grid->setCellString(row, colFieldFormula, towx(colinfo->getExpression()));
        updateRowCellProps(row);
        row++;
    }

    updateNumberColumn();
    m_grid->moveCursor(m_grid->getRowCount()-1, colFieldName);
    m_grid->scrollVertToCursor();
    m_grid->refresh(kcl::Grid::refreshAll);
    
    m_grid->Thaw();
    
    updateStatusBar();
}

void StructureDoc::insertRow(int row, bool dynamic)
{
    if (row == -1)
        row = m_grid->getRowCount();
    
    StructureField* f = new StructureField;
    f->name = wxEmptyString;
    f->type = tango::typeCharacter;
    f->width = 20;
    f->scale = 0;
    f->dynamic = dynamic;
    f->expr = wxEmptyString;
    f->original_dynamic = false;
    f->original = false;
    f->pos = -1;
    
    m_grid->insertRow(row);
    m_grid->setRowData(row, (long)f);
    m_grid->setCellBitmap(row, colRowNumber, dynamic ? GETBMP(gf_lightning_16)
                                                     : GETBMP(xpm_blank_16));
    m_grid->setCellComboSel(row, colFieldType, tango2choice(f->type));
    m_grid->setCellInteger(row, colFieldWidth, f->width);
    m_grid->setCellInteger(row, colFieldScale, f->scale);
    m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
    if (dynamic)
        m_grid->setCellString(row, colFieldFormula, wxT("\"\""));

    updateRowCellProps(row);
    checkOverlayText();
}

void StructureDoc::insertSelectedRows(bool dynamic)
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
            insertRow(start_row, dynamic);
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

void StructureDoc::updateNumberColumn()
{
    // -- resize row number column based on it's max text size --
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

void StructureDoc::updateExpressionIcon(int row, bool editing, int validation_res)
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
        if (editing && cursor_col == colFieldFormula)
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_checkmark_16));
         else
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
    }
     else
    {
        m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
    }
}

void StructureDoc::updateRowWidthAndScale(int row)
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
    
    StructureField* f = (StructureField*)(m_grid->getRowData(row));


    if (tango_type == tango::typeCharacter || 
        tango_type == tango::typeWideCharacter)
    {
        // if we're moving from one character type to another, leave
        // everything as the user set it    
        if (last_tango_type == tango::typeWideCharacter ||
            last_tango_type == tango::typeCharacter)
        {
            return;
        }
        
        // if the original type is a character type and we're coming from
        // a non-character type, restore the original width
        if (f->type == tango::typeCharacter || 
            f->type == tango::typeWideCharacter)
        {
            m_grid->setCellInteger(row, colFieldWidth, f->width);
        }
    }

    // if the original type is a numeric type and we're coming from
    // a different type, restore the original width
    if (f->type == tango::typeNumeric && last_tango_type != tango::typeNumeric)
    {
        m_grid->setCellInteger(row, colFieldWidth, f->width);
        return;
    }

    // handle default widths when we're converting to a
    // character type
    if (tango_type == tango::typeCharacter ||
        tango_type == tango::typeWideCharacter)
    {
        // if we're converting from a date type, allow enough room 
        // for the date to be represented as a string
        if (f->type == tango::typeDate)
        {
            // YYYY-MM-DD = 10 characters
            int new_width = 10;
            m_grid->setCellInteger(row, colFieldWidth, new_width);
        }

        // if we're converting from a datetime type, allow enough room 
        // for the datetime to be represented as a string
        if (f->type == tango::typeDateTime)
        {
            // YYYY-MM-DD HH:MM:SS = 19 characters; round to 20
            int new_width = 20;
            m_grid->setCellInteger(row, colFieldWidth, new_width);
        }

        // if we're converting from a number type, allow enough room 
        // for the number to be represented as a string
        if (f->type == tango::typeNumeric)
        { 
            // width = numeric_width + 2 (for decimal place and sign)
            int new_width = f->width + 2;
            m_grid->setCellInteger(row, colFieldWidth, new_width);
        }

        // if we're converting from a double type, allow enough room 
        // for the double to be represented as a string
        if (f->type == tango::typeDouble)
        {
            // width = 20 (large enough for double); parallels
            // code in tango\xdfs\delimitedtextset.cpp
            int new_width = 20;
            m_grid->setCellInteger(row, colFieldWidth, new_width);
        }

        // if we're converting from an integer type, allow enough room 
        // for the integer to be represented as a string
        if (f->type == tango::typeInteger)
        {
            // width = 12 (size of 2^32 + sign + 1 to round to 12);
            // parallels code in tango\xdfs\delimitedtextset.cpp
            int new_width = 12;
            m_grid->setCellInteger(row, colFieldWidth, new_width);
        }
    }

    if (tango_type == tango::typeNumeric)
    {
        // if we're converting from a double type, allow enough room
        // for the double to fit
        if (f->type == tango::typeDouble)
        {
            int new_width = 18;
            m_grid->setCellInteger(row, colFieldWidth, new_width);
        }

        // if we're converting from an integer type, allow enough room
        // for the integer to fit
        if (f->type == tango::typeInteger)
        {
            // width = 10 (size of 2^32)
            int new_width = 10;
            m_grid->setCellInteger(row, colFieldWidth, new_width);
        }    

        // in any case, if the width of the field was set to something 
        // above the max numeric width, cap it off
        int width = m_grid->getCellInteger(row, colFieldWidth);
        if (width > tango::max_numeric_width)
        {
            m_grid->setCellInteger(row, colFieldWidth,
                                   tango::max_numeric_width);
        }
    }
}

void StructureDoc::updateRowCellProps(int row)
{
    int combo_sel = m_grid->getCellComboSel(row, colFieldType);
    int type = choice2tango(combo_sel);
    
    bool width_editable = true;
    bool decimal_editable = true;
    bool formula_editable = true;

    switch (type)
    {
        case tango::typeWideCharacter:
        case tango::typeCharacter:
            decimal_editable = false;
            break;

        case tango::typeNumeric:
            break;

        case tango::typeDouble:
            width_editable = false;
            break;

        case tango::typeBoolean:
        case tango::typeDateTime:
        case tango::typeDate:
        case tango::typeInteger:
            width_editable = false;
            decimal_editable = false;
            break;

        default:
            break;
    }
    
    // gray out dynamic field formulas for dynamic fields
    // that are being converted to static fields
    wxString expr = m_grid->getCellString(row, colFieldFormula);
    StructureField* f = (StructureField*)m_grid->getRowData(row);
    if (!f->dynamic && expr.Length() > 0)
        formula_editable = false;
    
    kcl::CellProperties cellprops;
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
    
    // if the formula can't be edited, gray it out
    cellprops.mask = kcl::CellProperties::cpmaskEditable |
                     kcl::CellProperties::cpmaskFgColor;
    cellprops.editable = formula_editable;
    cellprops.fgcolor = formula_editable ? *wxBLACK : *wxLIGHT_GREY;
    m_grid->setCellProperties(row, colFieldFormula, &cellprops);
}

void StructureDoc::updateCaption()
{
    wxString caption = _("(Untitled)");
    if (m_path.Length() == 0)
        m_doc_site->setCaption(caption);
    else
    {
        caption = m_path.AfterLast(wxT('/'));
        caption.Append(isChanged() ? wxT("*") : wxT(""));

        m_doc_site->setCaption(caption);
    }
}

void StructureDoc::updateStatusBar()
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
        if (isFieldDynamic(m_grid, row))
            continue;
            
        total_width += m_grid->getCellInteger(row, colFieldWidth);
    }
    
    wxString field_count_str = wxString::Format(_("Field Count: %d"), row_count);
    wxString row_width_str = wxString::Format(_("Record Width: %d"), total_width);
    
    cfw::IStatusBarItemPtr item;
    item = m_frame->getStatusBar()->getItem(wxT("structuredoc_field_count"));
    if (item.isOk())
        item->setValue(field_count_str);
    item = m_frame->getStatusBar()->getItem(wxT("structuredoc_row_width"));
    if (item.isOk())
        item->setValue(row_width_str);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}

void StructureDoc::checkOverlayText()
{
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(_("To add fields to this table, drag in fields\nfrom the Fields Panel or double-click here"));
         else
        m_grid->setOverlayText(wxEmptyString);
}

// this function encapsulates all of the logic/error checking
// for when we want to save a document or switch views
bool StructureDoc::doErrorCheck()
{
    bool block = false;
    int errorcode = validateStructure();
    StructureValidator::showErrorMessage(errorcode, &block);
    
    // there is an error in the structure that must be fixed
    if (block)
        return false;
    
    return true;
}

int StructureDoc::validateStructure()
{
    // CHECK: check for empty structure
    if (m_grid->getRowCount() == 0)
        return StructureValidator::ErrorNoFields;

    // make sure we clear out the structure cache
    m_expr_edit_structure = xcm::null;

    // clear rows that have exlamation mark icons in them
    clearProblemRows();

    // CHECK: check for duplicate and invalid field names
    int invalidexpressions_errorcode = checkInvalidExpressions(
                                        CheckMarkRows | CheckEmptyFieldnames);    
    int invalidfields_errorcode = checkInvalidFieldnames(
                                        CheckMarkRows | CheckEmptyFieldnames);
    int duplicatefields_errorcode = checkDuplicateFieldnames(
                                        CheckMarkRows | CheckEmptyFieldnames);

    int error = invalidexpressions_errorcode | invalidfields_errorcode | duplicatefields_errorcode;

    if (error != StructureValidator::ErrorNone)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
        return error;
    }

    m_grid->refreshColumn(kcl::Grid::refreshAll, colRowNumber);
    return StructureValidator::ErrorNone;
}

int StructureDoc::validateExpression(const wxString& expr, int type)
{
    // this is a conventient pass-through function which will make sure we
    // have created a structure to pass to the structure validator function
    
    if (m_expr_edit_structure.isNull())
        m_expr_edit_structure = createStructureFromGrid();
    
    return StructureValidator::validateExpression(m_expr_edit_structure,
                                                  expr, type);
}

int StructureDoc::checkInvalidExpressions(int check_flags)
{
    // -- if we're editing, end the edit --
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // populate the validation structure
    if (m_expr_edit_structure.isNull())
        m_expr_edit_structure = createStructureFromGrid();

    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, true); // include expressions with empty fieldnames

    bool mark_rows = (check_flags & CheckMarkRows);
    bool errors_found = StructureValidator::findInvalidExpressions(check_rows, m_expr_edit_structure);
    if (errors_found && mark_rows)
    {
        std::vector<RowErrorChecker>::iterator it;
        for (it = check_rows.begin(); it != check_rows.end(); ++it)
        {
            if (it->errors != StructureValidator::ErrorNone)
                markProblemFormula(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorInvalidExpressions
                         : StructureValidator::ErrorNone);
}

int StructureDoc::checkInvalidFieldnames(int check_flags)
{
    // -- if we're editing, end the edit --
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
                markProblemField(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorInvalidFieldNames
                         : StructureValidator::ErrorNone);
}

int StructureDoc::checkDuplicateFieldnames(int check_flags)
{
    // -- if we're editing, end the edit --
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
                markProblemField(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorDuplicateFieldNames
                         : StructureValidator::ErrorNone);
}

void StructureDoc::markProblemField(int row, bool scroll_to)
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

void StructureDoc::markProblemFormula(int row, bool scroll_to)
{
    m_grid->setCellBitmap(row, colFieldFormula, GETBMP(gf_x_16));

    if (scroll_to)
    {
        m_grid->moveCursor(row, colFieldFormula, false);
        if (!m_grid->isCursorVisible())
        {
            m_grid->scrollVertToCursor();
        }
    }
}

void StructureDoc::clearProblemRows()
{
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        if (!isFieldDynamic(m_grid, row))
        {
            m_grid->setCellBitmap(row, colRowNumber, GETBMP(xpm_blank_16));
        }
         else
        {
            m_grid->setCellBitmap(row, colRowNumber, GETBMP(gf_lightning_16));
            m_grid->setCellBitmap(row, colFieldFormula, GETBMP(xpm_blank_16));
        }

    }
}

bool StructureDoc::createTable()
{
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSave);
    dlg.setCaption(_("Save As"));

    if (dlg.ShowModal() != wxID_OK)
        return false;

    // -- create the structure --
    tango::IStructurePtr structure = g_app->getDatabase()->createStructure();
    tango::IColumnInfoPtr col;

    wxString name;
    int type;
    int width;
    int scale;
    wxString expr;
    
    // -- add the fields to the structure --
    int row_count = m_grid->getRowCount();
    for (int row = 0; row < row_count; ++row)
    {
        name = m_grid->getCellString(row, colFieldName);
        type = m_grid->getCellComboSel(row, colFieldType);
        width = m_grid->getCellInteger(row, colFieldWidth);
        scale = m_grid->getCellInteger(row, colFieldScale);
        expr = m_grid->getCellString(row, colFieldFormula);
        
        col = structure->createColumn();
        col->setName(towstr(name));
        col->setType(choice2tango(type));
        col->setWidth(width);
        col->setScale(scale);
        col->setExpression(towstr(expr));
        col->setCalculated(isFieldDynamic(m_grid, row));
    }
    
    // -- get the path from the dialog and create the new set --
    m_path = dlg.getPath();
    tango::ISetPtr set = g_app->getDatabase()->createSet(towstr(m_path),
                                                         structure,
                                                         NULL);

    // -- update the project panel and the document caption --
    g_app->getAppController()->refreshDbDoc();

    // update caption
    updateCaption();

    // fire this event so that the URL will be updated with the new path
    if (m_frame.isOk())
        m_frame->postEvent(new cfw::Event(wxT("cfw.locationChanged")));

    // set the modify set in case the user wants to further modify the set
    // they just created... this would be a modify structure job
    setModifySet(set);
    
    // update the row data in the grid by repopulating it
    populateGridFromSet(m_modify_set);
    m_grid->moveCursor(0, colFieldName, false);
    m_grid->clearSelection();
    if (m_grid->getRowCount() > 0)
        m_grid->setRowSelected(0, true);
    updateNumberColumn();
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
    
    // set the changed flag
    setChanged(false);

    return true;
}

ModifyStructJob* StructureDoc::createModifyJob(size_t* _action_count)
{
    wxString caption = m_doc_site->getCaption();
    wxString title = wxString::Format(_("Modifying Structure of '%s'"),
                                      caption.c_str());
    
    ModifyStructJob* job = new ModifyStructJob;
    job->getJobInfo()->setTitle(title);
    
    tango::IStructurePtr structure = m_modify_set->getStructure();
    tango::IColumnInfoPtr col;
    
    int row_count = m_grid->getRowCount();
    size_t action_count = 0;
    
    // find deleted fields (i.e. fields in the original
    // structure that aren't found in the grid)
    for (int i = 0; i < structure->getColumnCount(); ++i)
    {
        col = structure->getColumnInfoByIdx(i);
        wxString old_name = towx(col->getName());
        bool found = false;
        
        // -- try to find the old field name in the grid's row data --
        for (int row = 0; row < row_count; ++row)
        {
            StructureField* f = (StructureField*)m_grid->getRowData(row);
            if (!f)
                continue;
            
            // only check fields in the original structure
            if (!f->original)
                continue;
                
            // -- if we find the name, we haven't deleted the field --
            if ((old_name.CmpNoCase(f->name) == 0))
            {
                found = true;
                break;
            }
        }
        
        // -- if we didn't find the old field in the grid's row data
        //    then the user has deleted the field --
        if (!found)
        {
            structure->deleteColumn(towstr(old_name));
            job->addToDelete(old_name);
            action_count++;
        }
    }
    
    
    wxString name;
    int type;
    int width;
    int scale;
    int row;
    wxString expr;
    
    
    // find the min and max boundries of all move operations.  We use
    // this information to reassign indexes of fields inside these bounds
    int phys_i = 0;
    
    bool moving_fields = false;
    for (row = 0; row < row_count; ++row)
    {
        StructureField* f = (StructureField*)m_grid->getRowData(row);
        if (!f)
            continue;
        
        if (f->original && !f->original_dynamic && f->pos != phys_i)
        {
            moving_fields = true;
            break;
        }
        
        if (!f->dynamic)
            phys_i++;
    }
    
    
    // tell the job which fields to create and modify
    for (row = 0; row < row_count; ++row)
    {
        name = m_grid->getCellString(row, colFieldName);
        type = m_grid->getCellComboSel(row, colFieldType);
        width = m_grid->getCellInteger(row, colFieldWidth);
        scale = m_grid->getCellInteger(row, colFieldScale);
        expr = m_grid->getCellString(row, colFieldFormula);


        StructureField* f = (StructureField*)m_grid->getRowData(row);
        if (!f)
            continue;
            
        if (!f->original)
        {
            // there's no row data, so we're inserting a new field
            col = structure->createColumn();
            col->setName(towstr(name));
            col->setType(choice2tango(type));
            col->setWidth(width);
            col->setScale(scale);
            col->setExpression(towstr(expr));
            col->setCalculated(f->dynamic);
            col->setColumnOrdinal(row); // row position
            
            job->addToInsert(name, row);
            action_count++;
        }
         else
        {
            // we're modifying an existing field
            if (f->name.Cmp(name) != 0 ||
                f->type != choice2tango(type) ||
                f->width != width ||
                f->scale != scale ||
                f->expr != expr ||
                f->dynamic != f->original_dynamic ||
                moving_fields)
            {
                col = structure->modifyColumn(towstr(f->name));
                
                if (f->name.Cmp(name) != 0)
                    col->setName(towstr(name));
                if (f->type != choice2tango(type))
                    col->setType(choice2tango(type));
                if (f->width != width)
                    col->setWidth(width);
                if (f->scale != scale)
                    col->setScale(scale);
                if (f->expr != expr)
                    col->setExpression(towstr(expr));
                if (moving_fields && f->pos != row)
                    col->setColumnOrdinal(row);
                col->setCalculated(f->dynamic);
                
                // check if the field has been renamed; allow changes
                // in field case, so use regular compare instead of a 
                // no case compare
                if (f->name.Cmp(name) != 0)
                    job->addToRename(f->name, name);
                
                action_count++;
            }
        }
    }
    
    // set the instructions for the modify job
    job->setInstructions(m_modify_set, structure);
    
    // report back how many actions we're going to perform
    if (_action_count)
        *_action_count = action_count;
    
    return job;
}

tango::IStructurePtr StructureDoc::createStructureFromGrid()
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
        col->setExpression(towstr(m_grid->getCellString(row, colFieldFormula)));
        col->setCalculated(isFieldDynamic(m_grid, row));
    }
    
    return s;
}

void StructureDoc::populateGridFromSet(tango::ISetPtr set)
{
    m_grid->deleteAllRows();
    
    if (set.isNull())
        return;
    
    // first, populate the grid with all the fields; do this before validating
    // any of the calculated fields since some of the fields may be dependent
    // on later fields in grid that need to be added to the validation structure
    // before validation on previous fields in the grid is possible
    tango::IStructurePtr structure = set->getStructure();
    int i, col_count = structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr col;
        col = structure->getColumnInfoByIdx(i);

        StructureField* f = new StructureField;
        f->name = towx(col->getName());
        f->type = col->getType();
        f->width = col->getWidth();
        f->scale = col->getScale();
        f->dynamic = col->getCalculated() ? true : false;
        f->expr = towx(col->getExpression());
        f->original_dynamic = col->getCalculated() ? true : false;
        f->original = true;
        f->pos = i;

        m_grid->insertRow(i);
        m_grid->setRowData(i, (long)f);
        m_grid->setCellBitmap(i, colRowNumber,
                         col->getCalculated() ? GETBMP(gf_lightning_16)
                                              : GETBMP(xpm_blank_16));
        m_grid->setCellString(i, colFieldName,
                         towx(col->getName()));
        m_grid->setCellComboSel(i, colFieldType,
                         tango2choice(col->getType()));
        m_grid->setCellInteger(i, colFieldWidth, col->getWidth());
        m_grid->setCellInteger(i, colFieldScale, col->getScale());
        m_grid->setCellString(i, colFieldFormula,
                         towx(col->getExpression()));
        m_grid->setCellBitmap(i, colFieldFormula, GETBMP(xpm_blank_16));

        updateRowCellProps(i);
    }

    validateStructure();
}

bool StructureDoc::isChanged()
{
    return m_changed;
}

void StructureDoc::setChanged(bool changed)
{
    // save the old changed flag
    bool old_changed = m_changed;

    // set the changed flag
    m_changed = changed;

    // if the changed flag has changed, update
    // the caption
    if (old_changed != m_changed)
        updateCaption();
}

void StructureDoc::onModifyStructJobFinished(cfw::IJobPtr job)
{
    // update the modify set
    IModifyStructJobPtr modify_job = job;
    m_modify_set = modify_job->getActionSet();
    
    // enable the structure editor grid
    m_grid->setVisibleState(kcl::Grid::stateVisible);

    // update the row data in the grid by repopulating it
    populateGridFromSet(m_modify_set);
    m_grid->moveCursor(0, colFieldName, false);
    m_grid->clearSelection();
    if (m_grid->getRowCount() > 0)
        m_grid->setRowSelected(0, true);
    updateNumberColumn();
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void StructureDoc::onFrameEvent(cfw::Event& evt)
{
    // if a file is renamed, update this file with the new file path
    if (evt.name == wxT("treepanel.ofsFileRenamed"))
    {
        if (evt.s_param == m_path)
        {
            m_path = evt.s_param2;

            // update caption;
            updateCaption();
            
            // fire a frame event
            cfw::IDocumentSitePtr doc_site = m_frame->getActiveChild();
            if (doc_site.isOk() && doc_site == m_doc_site)
            {
                // fire this event so that the URL combobox will be updated
                // with the new path if this is the active child
                m_frame->postEvent(new cfw::Event(wxT("cfw.locationChanged")));
            }
        }
    }

    if (evt.name == wxT("appmain.view_switcher.query_available_views"))
    {
        cfw::IDocumentSitePtr active_child;
        active_child = g_app->getMainFrame()->getActiveChild();
        
        if (active_child.isNull() || m_doc_site.isNull())
            return;
            
        if (active_child->getContainerWindow() != m_doc_site->getContainerWindow())
            return;

        // site ptrs to check the active site
        cfw::IDocumentSitePtr tabledoc_site;
        cfw::IDocumentSitePtr active_site;
        tabledoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.TableDoc");
        active_site = g_app->getMainFrame()->getActiveChild();
        
        // populate the view switcher list with the available views
        ViewSwitcherList* list = (ViewSwitcherList*)(evt.o_param);
        
        // normally, we populate the view switcher from the TableDoc's
        // onFrameEvent() handler, but no tabledoc exists in this container
        // (most likely because we are createing a new table), so make sure
        // we populate the list ourselves
        ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
        if (tabledoc.isNull())
        {
            list->addItem(ID_View_SwitchToDesignView, _("Design View"),
                          (m_doc_site == active_site) ? true : false);
                          
            // this item is always in the list
            list->addItem(ID_View_SwitchToLayoutView, _("Table View"),
                          (tabledoc_site == active_site) ? true : false);
        }
    }
     else if (evt.name == wxT("appmain.view_switcher.active_view_changing"))
    {
        int id = (int)(evt.l_param);
        
        // -- make sure we are in the active container --
        cfw::IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        if (active_site.isNull() || m_doc_site.isNull())
            return;
        if (active_site->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
        
        if (id == ID_View_SwitchToLayoutView)
        {
            // if we are on structure doc, we might need to prompt for saving
            cfw::IDocumentSitePtr tabledoc_site;

            tabledoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.TableDoc");
            active_site = g_app->getMainFrame()->getActiveChild();

            if (active_site == m_doc_site)
            {
                if (isChanged())
                {
                    int result = cfw::appMessageBox(_("Would you like to save the changes made to the table's structure?"),
                                                    APPLICATION_NAME,
                                                    wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);
                    if (result == wxCANCEL)
                    {
                        // don't allow the view switcher to change
                        *(bool*)evt.l_param2 = false;
                        return;
                    }
                     else if (result == wxYES)
                    {
                        if (!doSave())
                        {
                            // don't allow the view switcher to change
                            *(bool*)evt.l_param2 = false;
                            return;
                        }
                    }
                     else if (result == wxNO)
                    {   
                        // reset the changed state
                        setChanged(false);
                    
                        // revert the grid to the original structure
                        populateGridFromSet(m_modify_set);
                        updateNumberColumn();
                        checkOverlayText();
                        
                        // we'll refresh the grid below, after we've switched
                        // views, so we don't see any flicker
                    }
                }
                 else   // if (!isChanged())
                {
                    // we haven't made any changes to the structure and
                    // we're not modifying an exisiting table -- we're
                    // creating a new one -- so do error checking here
                    if (!m_modify)
                    {
                        // -- if we're editing, end the edit --
                        if (m_grid->isEditing())
                            m_grid->endEdit(true);
                        
                        if (!doErrorCheck())
                            return;
                    }
                }
                
                // if we don't have a tabledoc yet, we need to create one so
                // that we can switch to it (this is most likely the case
                // because we are creating a table)
                if (tabledoc_site.isNull())
                {
                    // -- this chunk of code exists in doSave() as well --
                    
                    // create a tabledoc and open it
                    ITableDocPtr doc = TableDocMgr::createTableDoc();
                    doc->open(m_modify_set, xcm::null);

                    if (doc->getCaption().Length() == 0)
                    {
                        // update the caption manually
                        wxString caption = m_path.AfterLast(wxT('/'));
                        doc->setCaption(caption, wxEmptyString);
                    }
                    
                    wxWindow* container = m_doc_site->getContainerWindow();
                    g_app->getMainFrame()->createSite(container,
                                                      doc, false);
                }
            }
        }
        
        switch (id)
        {
            case ID_View_SwitchToDesignView:
                switchToOtherDocument(m_doc_site, "appmain.StructureDoc");
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
     else if (evt.name == wxT("tabledoc.structureModified"))
    {
        // update the row data in the grid by repopulating it
        populateGridFromSet(m_modify_set);
        m_grid->moveCursor(0, colFieldName, false);
        m_grid->clearSelection();
        if (m_grid->getRowCount() > 0)
            m_grid->setRowSelected(0, true);
        updateNumberColumn();
        checkOverlayText();
        m_grid->refresh(kcl::Grid::refreshAll);
    }
}

void StructureDoc::onInsertingRows(std::vector<int> rows)
{
    // set the changed flag
    setChanged(true);

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

void StructureDoc::onDeletedRows(std::vector<int> rows)
{
    // set the changed flag
    setChanged(true);

    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
    updateNumberColumn();
    validateStructure();
    updateStatusBar();
}

void StructureDoc::onSize(wxSizeEvent& evt)
{
    Layout();
}

void StructureDoc::onEraseBackground(wxEraseEvent& evt)
{
}


void StructureDoc::onSave(wxCommandEvent& evt)
{
    // we must check for no rows in the grid here because if the structuredoc
    // has just been opened for the first time and no rows have been added,
    // the dirty flag will not be set, however we still need to try the
    // save here because of the error handling in the doSave() function
    if (isChanged() || m_grid->getRowCount() == 0)
        doSave();
}

void StructureDoc::onCopy(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }

    AppBusyCursor c;
    m_grid->copySelection();
}

void StructureDoc::onInsertField(wxCommandEvent& evt)
{
    // set the changed flag
    setChanged(true);

    // insert the selected rows
    insertSelectedRows(false /* dynamic */);
}

void StructureDoc::onInsertDynamicField(wxCommandEvent& evt)
{
    // set the changed flag
    setChanged(true);

    // insert the selected rows
    insertSelectedRows(true /* dynamic */);
}

void StructureDoc::onConvertDynamicToFixed(wxCommandEvent& evt)
{
    std::vector<int> selected_rows = m_grid->getSelectedRows();
    std::vector<int>::iterator it;
    for (it = selected_rows.begin(); it != selected_rows.end(); ++it)
    {
        int row = (*it);
        StructureField* f = (StructureField*)m_grid->getRowData(row);
        
        // for now, don't allow newly created dynamic fields
        // (in the StructureDoc) to be converted to fixed fields -- doing
        // so results in the field being empty since the dynamic field's
        // expression is disregarded in the ModifyStructJob
        if (f->dynamic && !f->original_dynamic)
        {
            cfw::appMessageBox(_("One or more of the calculated fields that is selected is new to the table's structure.  Only calculated fields that already exist in the table's structure can be converted to fixed fields."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
    }

    // set the changed flag
    setChanged(true);

    for (it = selected_rows.begin(); it != selected_rows.end(); ++it)
    {
        int row = (*it);
        if (isFieldDynamic(m_grid, row))
        {
            StructureField* f = (StructureField*)m_grid->getRowData(row);
            if (!f)
                continue;
            
            // update the row data
            f->dynamic = false;
            
            // remove the lightning bitmap and gray out the formula text
            m_grid->setCellBitmap(row, colRowNumber, GETBMP(xpm_blank_16));
            updateRowCellProps(row);
        }
    }
    
    // refresh the grid
    if (selected_rows.size() > 0)
        m_grid->refresh(kcl::Grid::refreshAll);
}

void StructureDoc::onDeleteField(wxCommandEvent& evt)
{
    // set the changed flag
    setChanged(true);

    // delete the selected rows
    m_grid->deleteSelectedRows(false);
    checkOverlayText();
    updateNumberColumn();
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void StructureDoc::onSelectAll(wxCommandEvent& evt)
{
    m_grid->selectAll();
}

void StructureDoc::onGridNeedTooltipText(kcl::GridEvent& evt)
{
    // NOTE: disable tooltip logic; a lot of overhead that isn't necessarily useful
    return;
/*
    int row_count = m_grid->getRowCount();
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col != colRowNumber && col != colFieldName && col != colFieldFormula)
        return;
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, false);    // don't check empty fieldnames
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
                // don't check empty expressions for static fields
                if (!isFieldDynamic(m_grid, row))
                    continue;
                
                int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
                wxString expr = m_grid->getCellString(row, colFieldFormula);
                int res = validateExpression(expr, type);
                
                if (res == StructureValidator::ExpressionTypeMismatch)
                    msg = _("This formula has a return type that does not match the field type");
                 else if (res == StructureValidator::ExpressionInvalid)
                    msg = _("This formula has invalid syntax");
                
                evt.SetString(msg);
            }
        }
    }
*/    
}

void StructureDoc::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    // -- we'll handle the processing of this event ourselves --
    evt.Veto();

    // make sure we're not editing the grid
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // set the changed flag
    setChanged(true);
    
    insertRow(-1);
    updateNumberColumn();
    int row = m_grid->getRowCount()-1;
    m_grid->moveCursor(row, evt.GetColumn(), false);
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void StructureDoc::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    // -- we'll handle the processing of this event ourselves --
    evt.Veto();

    // make sure we're not editing the grid
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // set the changed flag
    setChanged(true);
    
    insertRow(-1);
    updateNumberColumn();
    int row = m_grid->getRowCount()-1;
    m_grid->moveCursor(row, evt.GetColumn(), false);
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void StructureDoc::onGridBeginEdit(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    // store the last selected field type for comparison later on
    if (col == colFieldType)
    {
        m_last_selected_fieldtype = m_grid->getCellComboSel(row, colFieldType);
    }
     else if (col == colFieldFormula)
    {
        // don't allow the user to edit the formula for static fields
        if (!isFieldDynamic(m_grid, row))
        {
            evt.Veto();
            return;
        }
        
        int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
        wxString expr = m_grid->getCellString(row, colFieldFormula);
        
        int res = validateExpression(expr, type);
        updateExpressionIcon(row, true, res);
        m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldFormula);
    }
}

void StructureDoc::onGridEndEdit(kcl::GridEvent& evt)
{
    wxString expr = evt.GetString();
    if (evt.GetEditCancelled())
        return;

    int row = evt.GetRow();
    int col = evt.GetColumn();
    int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
    m_last_selected_fieldtype = -1;

    // set the changed flag
    setChanged(true);

    // NOTE: in following, force the cell's text to be the text that we 
    // just entered (this is necessary for the validation function below
    // to work properly since it gets it's value from the grid, which hasn't
    // updated yet)
    if (col == colFieldName)
    {
        wxString name = evt.GetString();
        m_grid->setCellString(row, colFieldName, name);
    }

    if (col == colFieldWidth)
    {
        // make sure the width falls with the acceptable limits; if
        // it doesn't, reset it and Veto() the event so the correct
        // value is set; note: in this situation the cursor won't
        // advance down, but that's ok since it gives users a chance
        // to change any value that's overridden
        int width = evt.GetInt();
        int scale = m_grid->getCellInteger(row, colFieldScale);
        bool updated = StructureValidator::updateFieldWidthAndScale(type, &width, &scale);
        
        // if it's been updated, the event is carrying the wrong value, so
        // we have to Veto() the event so the correct value is set; note: 
        // in this situation the cursor won't advance down, but that's ok 
        // since it gives users a chance to change any value that's overridden
        if (updated)
            evt.Veto();
        
        // set the width, scale manually, similar to name field
        m_grid->setCellInteger(row, colFieldWidth, width);
        m_grid->setCellInteger(row, colFieldScale, scale);
        m_grid->refresh(kcl::Grid::refreshAll);
    }

    if (col == colFieldScale)
    {
        // make sure the scale falls with the acceptable limits   
        int width = m_grid->getCellInteger(row, colFieldWidth);
        int scale = evt.GetInt();
        bool updated = StructureValidator::updateFieldWidthAndScale(type, &width, &scale);

        // if it's been updated, the event is carrying the wrong value, so
        // we have to Veto() the event so the correct value is set; note: 
        // in this situation the cursor won't advance down, but that's ok 
        // since it gives users a chance to change any value that's overridden
        if (updated)
            evt.Veto();
        
        // set the width, scale manually, similar to name field
        m_grid->setCellInteger(row, colFieldWidth, width);
        m_grid->setCellInteger(row, colFieldScale, scale);
        m_grid->refresh(kcl::Grid::refreshAll);
    }

    if (col == colFieldFormula)
    {
        wxString formula = evt.GetString();
        m_grid->setCellString(row, colFieldFormula, formula);
    }

    validateStructure();
    updateStatusBar();
}

void StructureDoc::onGridEditChange(kcl::GridEvent& evt)
{
    // set the changed flag
    setChanged(true);
    
    if (evt.GetEditCancelled())
    {
        m_last_selected_fieldtype = -1;
        return;
    }
    
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col == colFieldType)
    {
        int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
        wxString expr = m_grid->getCellString(row, colFieldFormula);
        
        // if the expression is valid, don't show
        // an icon next to the expression
        if (isFieldDynamic(m_grid, row))
        {
            int res = validateExpression(expr, type);
            updateExpressionIcon(row, true, res);
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
     else if (col == colFieldFormula)
    {
        int type = choice2tango(m_grid->getCellComboSel(row, colFieldType));
        wxString expr = evt.GetString();

        int res = validateExpression(expr, type);
        updateExpressionIcon(row, true, res);
        m_grid->refreshColumn(kcl::Grid::refreshAll, colFieldFormula);
    }
}

void StructureDoc::onGridCellRightClick(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (row < 0 || row >= m_grid->getRowCount() ||
        col < 0 || col >= m_grid->getColumnCount())
        return;
    
    // if the user clicked on a row that was
    // not previously selected, select only that row
    if (!m_grid->isRowSelected(row))
    {
        m_grid->clearSelection();
        m_grid->setRowSelected(row, true);
        m_grid->refresh(kcl::Grid::refreshAll);
    }
    
    // find out if all of the selected rows are dynamic fields
    bool all_dynamic = true;
    std::vector<int> selected_rows = m_grid->getSelectedRows();
    std::vector<int>::iterator it;
    for (it = selected_rows.begin(); it != selected_rows.end(); ++it)
    {
        int row = (*it);
        if (!isFieldDynamic(m_grid, row))
        {
            all_dynamic = false;
            break;
        }
    }
    
    wxMenu menuPopup;
    menuPopup.Append(ID_InsertField, _("&Insert Field"));
    menuPopup.Append(ID_InsertDynamicField, _("Insert &Calculated Field"));
    menuPopup.AppendSeparator();
    if (all_dynamic)
    {
        menuPopup.Append(ID_ConvertDynamicToFixed, _("Convert to Fixed Field"));
        menuPopup.AppendSeparator();
    }
    menuPopup.Append(ID_Edit_Copy, _("&Copy"));
    menuPopup.Append(ID_Edit_Delete, _("&Delete"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Edit_SelectAll, _("Select &All"));
    
    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);
    cfw::CommandCapture* cc = new cfw::CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);
    
    // -- post the event to the event handler --
    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, command);
    ::wxPostEvent(this, e);
}

void StructureDoc::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    wxDataObject* obj = drop_target->GetDataObject();
    if (!obj)
        return;

    // set the changed flag
    setChanged(true);
    
    // determine which type of data object was dropped
    kcl::GridDataObjectComposite* drop_data = (kcl::GridDataObjectComposite*)obj;
    wxDataFormat fmt = drop_data->GetReceivedFormat();
    
    // check to see if what type of data object we're dealing with
    if (drop_data->isGridData())
    {
        if (fmt.GetId() == kcl::getGridDataFormat(wxT("structuredoc")))
        {
            // if we've reordered the rows, we need to renumber the rows
            drop_target->doRowDrag(false);
            updateNumberColumn();
            m_grid->refresh(kcl::Grid::refreshAll);
        }
         else if (fmt.GetId() == kcl::getGridDataFormat(wxT("fieldspanel")))
        {
            // we can only drag in fields from the fields panel
            // if we're modifying an existing table
            if (m_modify_set.isNull())
                return;

            tango::IStructurePtr s = m_modify_set->getStructure();
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
                
                insertRow(drop_row, colinfo->getCalculated());
                m_grid->setCellString(drop_row, colFieldName, towx(colinfo->getName()));
                m_grid->setCellComboSel(drop_row, colFieldType, tango2choice(colinfo->getType()));
                m_grid->setCellInteger(drop_row, colFieldWidth, colinfo->getWidth());
                m_grid->setCellInteger(drop_row, colFieldScale, colinfo->getScale());
                m_grid->setCellString(drop_row, colFieldFormula, towx(colinfo->getExpression()));
                updateRowCellProps(drop_row);
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
            checkInvalidExpressions(CheckMarkRows);            
            checkInvalidFieldnames(CheckMarkRows);            
            checkDuplicateFieldnames(CheckMarkRows);
            m_grid->refresh(kcl::Grid::refreshAll);
            updateStatusBar();
        }
    }
}

void StructureDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void StructureDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

void StructureDoc::onUpdateUI(wxUpdateUIEvent& evt)
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



