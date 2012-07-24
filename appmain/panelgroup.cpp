/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-06-28
 *
 */


#include "appmain.h"
#include "panelgroup.h"
#include "jobgroup.h"
#include "tabledoc.h"
#include "appcontroller.h"
#include "fieldlistcontrol.h"
#include <algorithm>
#include <set>


enum
{
    ID_FieldListCtrl = 10000,
    ID_AddParams,
    ID_RemoveParams,
    ID_RemoveAllParams,
    ID_AddCountParam,
    ID_AddGroupIDParam,
    ID_Advanced,
    ID_GroupFilter,
    ID_MoveParamUp,
    ID_MoveParamDown
};



enum
{
    GroupFunc_GroupBy = 0,
    GroupFunc_Min = 1,
    GroupFunc_Max = 2,
    GroupFunc_Sum = 3,
    GroupFunc_Avg = 4,
    GroupFunc_Count = 5,
    GroupFunc_Stddev = 6,
    GroupFunc_Variance = 7,
    GroupFunc_GroupID = 8
    
    //GroupFunc_MaxDistance = // maybe add later; see function list below
};


// grid column indexes
const int GroupCol_Hidden = 0;
const int GroupCol_GroupFunc = 1;
const int GroupCol_InputExpr = 2;
const int GroupCol_OutputField = 3;


inline wxString getCountLabel() { return _("<Count>"); }
inline wxString getGroupIdLabel() { return _("<Group ID>"); }


static wxString groupfunc2str(int group_func, bool format_nice = false)
{
    wxString text;

    switch (group_func)
    {
        case GroupFunc_Min:         text = wxT("min"); break;
        case GroupFunc_Max:         text = wxT("max"); break;
        case GroupFunc_Sum:         text = wxT("sum"); break;
        case GroupFunc_Avg:         text = wxT("avg"); break;
        case GroupFunc_Count:       text = wxT("count"); break;
        case GroupFunc_Stddev:      text = wxT("stddev"); break;
        case GroupFunc_Variance:    text = wxT("variance"); break;
/*
        // maybe add later        
        case GroupFunc_MaxDistance: 
        {
            if (format_nice)
            {
                text = wxT("Maximum Distance");
            }
             else
            {
                text = wxT("maxdistance");
            }
            break;
        }
*/
        case GroupFunc_GroupBy:
        {
            if (format_nice)
            {
                text = wxT("Group By");
            }
             else
            {
                // group by -- when it's actually put into
                // a query, uses first
                text = wxT("first");
            }
            break;
        }
        case GroupFunc_GroupID:
        {
            if (format_nice)
            {
                text = wxT("Group ID");
            }
             else
            {
                text = wxT("groupid");
            }
            break;
        }
    }

    if (format_nice)
    {
        if (!text.IsEmpty())
        {
            text.SetChar(0, wxToupper(text.GetChar(0)));
        }
    }

    return text;
}

void setGridInputOutputText(kcl::Grid* grid, int row, int function, bool refresh)
{
    // -- set the intpu and output field names --
    
    wxString func_str = groupfunc2str(function, true);

    if (function == GroupFunc_GroupBy)
    {
        wxString* input = (wxString*)(grid->getRowData(row));
        grid->setCellString(row, GroupCol_OutputField, *input);
        grid->setCellString(row, GroupCol_InputExpr, *input);
    }
     else if (function == GroupFunc_Count)
    {
        grid->setCellString(row, GroupCol_OutputField, wxT("Row_count"));
        grid->setCellString(row, GroupCol_InputExpr, wxEmptyString);
    }
     else if (function == GroupFunc_GroupID)
    {
        wxString input = grid->getCellString(row, GroupCol_InputExpr);
        
        // use underscores for default group_id output name 
        // until data engine handles spaces better
        wxString output = func_str;
        output.Replace(wxT(" "), wxT("_"), true);

        grid->setCellString(row, GroupCol_OutputField, output);
        grid->setCellString(row, GroupCol_InputExpr, wxEmptyString);
    }
     else
    {
        wxString* input = (wxString*)(grid->getRowData(row));
        wxString output = wxString::Format(wxT("%s_%s"), func_str.c_str(),
                                                         input->c_str());
        grid->setCellString(row, GroupCol_OutputField, output);
        grid->setCellString(row, GroupCol_InputExpr, *input);
    }
    
    if (refresh)
        grid->refresh(kcl::Grid::refreshAll);
}



BEGIN_EVENT_TABLE(GroupPanel, wxPanel)
    EVT_BUTTON(wxID_CANCEL, GroupPanel::onCancel)
    EVT_BUTTON(wxID_OK, GroupPanel::onExecute)
    EVT_TEXT(ID_GroupFilter, GroupPanel::onAdvancedQueryText)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(GroupPanel::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(GroupPanel::onGridPreInvalidAreaInsert)
    EVT_KCLGRID_EDIT_CHANGE(GroupPanel::onGridEditChange)
    EVT_KCLGRID_END_EDIT(GroupPanel::onGridEndEdit)
END_EVENT_TABLE()


GroupPanel::GroupPanel()
{
    m_ok_button = NULL;
}

GroupPanel::~GroupPanel()
{

}

// -- IDocument --
bool GroupPanel::initDoc(cfw::IFramePtr frame,
                         cfw::IDocumentSitePtr site,
                         wxWindow* docsite_wnd,
                         wxWindow* panesite_wnd)
{
    wxSize size = docsite_wnd->GetClientSize();

    if (!Create(docsite_wnd,
                -1,
                wxPoint(0,0),
                size,
                wxCLIP_CHILDREN | wxNO_FULL_REPAINT_ON_RESIZE))
    {
        return false;
    }

    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);

    m_doc_site = site;


    // -- create tablecol grid --
    
    m_tablecols = new FieldListControl(this, ID_FieldListCtrl);
    m_tablecols->setDragFormat(wxT("grouppanel_fields"));


    // -- create group parameter grid --
    
    m_grid = new kcl::RowSelectionGrid(this,
                                       -1,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       kcl::DEFAULT_BORDER,
                                       true, true);
    m_grid->setAllowDeletes(true);
    m_grid->setDragFormat(wxT("grouppanel_output"));
    
    // we need to have a hidden column as the first column
    // in the grid because the kcl::RowSelectionGrid has some
    // issues when the first column is a dropdown...
    
    m_grid->createModelColumn(GroupCol_Hidden, wxT("Hidden"), kcl::Grid::typeCharacter, 1, 0);
    m_grid->createModelColumn(GroupCol_GroupFunc, _("Function"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(GroupCol_InputExpr, _("Input Field"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(GroupCol_OutputField, _("Output Field"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createDefaultView();
    
    // don't allow the hidden column to be resized
    m_grid->setColumnResizable(GroupCol_Hidden, false);
    
    // -- drop target for moving rows up and down on the grid --
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_grid);
    drop_target->sigDropped.connect(this, &GroupPanel::onGridDataDropped);
    drop_target->setGridDataObjectFormats(wxT("grouppanel_fields"), wxT("grouppanel_output"));
    m_grid->SetDropTarget(drop_target);


    // -- create horizontal sizer --
    
    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer->Add(m_tablecols, 1, wxEXPAND);
    horz_sizer->AddSpacer(8);
    horz_sizer->Add(m_grid, 3, wxEXPAND);

    wxStaticText* adv_group_query_label = new wxStaticText(this, -1, _("Group Filter:"));
    m_adv_group_query = new wxTextCtrl(this, ID_GroupFilter);
    m_adv_checkbox = new wxCheckBox(this, -1, _("Include Detail Records"));
    m_adv_group_query_valid = new kcl::ValidControl(this,
                                                    -1,
                                                    wxDefaultPosition,
                                                    wxSize(16,16));
    m_adv_group_query_valid->setValidBitmap(GETBMP(gf_checkmark_16));
    m_adv_group_query_valid->setInvalidBitmap(GETBMP(gf_x_16));

    m_adv_group_query_valid->setBorderVisible(false);
    m_adv_group_query_valid->setLabelVisible(false);

    // create advanced options sizer
    wxBoxSizer* advanced_options_sizer = new wxBoxSizer(wxHORIZONTAL);
    advanced_options_sizer->Add(adv_group_query_label, 0, wxALIGN_CENTER);
    advanced_options_sizer->AddSpacer(5);
    advanced_options_sizer->Add(m_adv_group_query, 1, wxALIGN_CENTER);
    advanced_options_sizer->AddSpacer(4);
    advanced_options_sizer->Add(m_adv_group_query_valid, 0, wxALIGN_CENTER);
    advanced_options_sizer->AddSpacer(8);
    advanced_options_sizer->Add(m_adv_checkbox, 0, wxALIGN_CENTER);


    // -- create a platform standards-compliant OK/Cancel sizer --
    
    m_ok_button = new wxButton(this, wxID_OK, _("Run"));
    wxButton* cancel_button = new wxButton(this, wxID_CANCEL);
    m_ok_button->SetDefault();
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(m_ok_button);
    ok_cancel_sizer->AddButton(cancel_button);
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);

    // -- this code is necessary to get the sizer's bottom margin to 8 --
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(horz_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);
    main_sizer->Add(advanced_options_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);
    main_sizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);

    SetSizer(main_sizer);
    Layout();


    // -- size the colums in the grid --
    int w = m_grid->GetClientSize().GetWidth();
    m_grid->setColumnSize(GroupCol_Hidden, 0);
    m_grid->setColumnSize(GroupCol_GroupFunc, 90);
    w -= 90;
    m_grid->setColumnSize(GroupCol_InputExpr, w/2);
    w /= 2;
    m_grid->setColumnSize(GroupCol_OutputField, w);


    cfw::IDocumentSitePtr active_site;
    active_site = g_app->getMainFrame()->getActiveChild();
    if (active_site.isNull())
        return false;

    ITableDocPtr tabledoc = active_site->getDocument();
    if (tabledoc.isNull())
        return false;

    m_set = tabledoc->getBrowseSet();
    if (m_set.isNull())
        return false;

    wxString caption = _("Group");
    if (!m_set->isTemporary())
    {
        wxString name = towx(m_set->getObjectPath());
        name.AfterLast(wxT('/'));
        
        caption += wxT(" - [");
        caption += name;
        caption += wxT("]");
    }

    // -- save structure for later --
    m_structure = m_set->getStructure();
    m_tablecols->addCustomItem(getCountLabel(), GETBMP(xpm_blank_16));
    m_tablecols->addCustomItem(getGroupIdLabel(), GETBMP(xpm_blank_16));
    m_tablecols->setStructure(m_structure);
    m_doc_site->setCaption(caption);

    // refresh the row selection grid
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);

    // connect signals
    m_grid->sigDeletedRows.connect(this, &GroupPanel::onDeletedRows);
    m_tablecols->sigFieldDblClicked.connect(this, &GroupPanel::onFieldDblClicked);

    return true;
}

wxWindow* GroupPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void GroupPanel::setDocumentFocus()
{
    if (m_grid)
    {
        m_grid->SetFocus();
    }
}

void GroupPanel::insertOutputField(int row,
                                   const wxString& input,
                                   int function,
                                   bool refresh)
{
    // default is no group function
    if (function == -1)
        function = GroupFunc_GroupBy;

    // -- insert a row in the grid for this group parameter --
    m_grid->insertRow(row);
    row = (row == -1) ? m_grid->getRowCount() - 1 : row;

    // -- populate the group function dropdown --
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskCtrlType |
                     kcl::CellProperties::cpmaskCbChoices;
    cellprops.ctrltype = kcl::Grid::ctrltypeDropList;
    for (int i = 0; i <= GroupFunc_GroupID; ++i)
    {
        cellprops.cbchoices.push_back(groupfunc2str(i, true));
    }
    m_grid->setCellProperties(row, GroupCol_GroupFunc, &cellprops);

    // -- set the values in the grid --
    m_grid->setCellComboSel(row, GroupCol_GroupFunc, function);

    // save the input expression in the row data -- the input expression
    // cell string will be set in the setGridInputOutputText() function
    wxString* rowdata = new wxString(input);
    m_grid->setRowData(row, (long)rowdata);
    
    setGridInputOutputText(m_grid, row, function, refresh);
    checkOverlayText();
    
    if (refresh)
        m_grid->refresh(kcl::Grid::refreshAll);
}



// -- event handlers --

void GroupPanel::onAdvancedQueryText(wxCommandEvent& evt)
{
    validateGroupQuery();
}

void GroupPanel::onGridEndEdit(kcl::GridEvent& evt)
{
    if (evt.GetEditCancelled())
        return;

    if (evt.GetEventObject() != m_grid)
        return;
        
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col == GroupCol_OutputField)
    {
        m_grid->setCellBitmap(row,
                              GroupCol_OutputField,
                              wxNullBitmap);

        m_grid->refresh(kcl::Grid::refreshAll);
    }
}

void GroupPanel::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();

    if (evt.GetEventObject() != m_grid)
        return;

    // add an output field to the grid
    insertOutputField(-1,
                      wxEmptyString,
                      GroupFunc_GroupBy,
                      false);

    m_grid->moveCursor(m_grid->getRowCount()-1, evt.GetColumn(), false);
    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void GroupPanel::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();

    if (evt.GetEventObject() != m_grid)
        return;

    // add an output field to the grid
    insertOutputField(-1,
                      wxEmptyString,
                      GroupFunc_GroupBy,
                      false);

    m_grid->moveCursor(m_grid->getRowCount()-1, evt.GetColumn(), false);
    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void GroupPanel::onGridEditChange(kcl::GridEvent& evt)
{
    if (evt.GetEventObject() != m_grid)
        return;

    int row = evt.GetRow();
    
    if (evt.GetColumn() == GroupCol_GroupFunc)
    {
        int new_sel;
        new_sel = m_grid->getCellComboSel(row, GroupCol_GroupFunc);

        setGridInputOutputText(m_grid, row, new_sel, true);
    }
}

void GroupPanel::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    int drop_row = drop_target->getDropRow();
    wxWindowID source_id = drop_target->getSourceGridId();
    
    if (source_id == ID_FieldListCtrl)
    {
        kcl::GridDraggedCells cells = drop_target->getDraggedCells();
        kcl::GridDraggedCells::iterator it;
        for (it = cells.begin(); it != cells.end(); ++it)
        {
            wxString cell_str = (*it)->m_strvalue;
            
            if (cell_str.CmpNoCase(getCountLabel()) == 0)
            {
                // insert a count field
                insertOutputField(drop_row,
                                  (*it)->m_strvalue,
                                  GroupFunc_Count,
                                  false);
            }
             else if (cell_str.CmpNoCase(getGroupIdLabel()) == 0)
            {
                // insert a group id field
                insertOutputField(drop_row,
                                  (*it)->m_strvalue,
                                  GroupFunc_GroupID,
                                  false);
            }
             else
            {
                insertOutputField(drop_row,
                                  (*it)->m_strvalue,
                                  -1,
                                  false);
            }
            
            if (drop_row != -1)
                drop_row++;
        }
        
        m_grid->refresh(kcl::Grid::refreshAll);
        return;
    }
     else
    {
        drop_target->doRowDrag(true);
    }
}

void GroupPanel::onFieldDblClicked(int row, const wxString& field)
{
    if (field.CmpNoCase(getCountLabel()) == 0)
        insertOutputField(-1, field, GroupFunc_Count, false);
     else if (field.CmpNoCase(getGroupIdLabel()) == 0)
        insertOutputField(-1, field, GroupFunc_GroupID, false);
     else
        insertOutputField(-1, field, -1, false);
    
    m_grid->scrollVertToCursor();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void GroupPanel::onDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
}

void GroupPanel::checkOverlayText()
{
    // this isn't the best place for this, but it's convenient
    if (m_ok_button)
        m_ok_button->Enable(m_grid->getRowCount() > 0 ? true : false);
    
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(_("Select fields from the list on the left and\ndrag them here to add them to the output table"));
         else
        m_grid->setOverlayText(wxEmptyString);
}

bool GroupPanel::validateGroupQuery()
{
    wxString value = m_adv_group_query->GetValue();
    value.Trim();
    if (value.Length() == 0)
        return true;

    bool valid = (m_structure->getExprType(towstr(value)) == tango::typeBoolean);
    m_adv_group_query_valid->setValid(valid);

    return valid;
}

static void onGroupJobFinished(cfw::IJobPtr job)
{
    if (job->getJobInfo()->getState() != cfw::jobStateFinished)
        return;


    IGroupJobPtr group_job = job;

    bool success = false;

    // -- check if there is an output set --
    if (group_job)
    {
        tango::ISetPtr result_set = group_job->getResultSet();

        if (result_set)
        {
            ITableDocPtr doc = TableDocMgr::createTableDoc();
            doc->setBaseSet(result_set, xcm::null);
            g_app->getMainFrame()->createSite(doc, cfw::sitetypeNormal,
                                              -1, -1, -1, -1);
            success = true;
        }
    }
    

    if (!success)
    {
        cfw::appMessageBox(_("The output table could not be created."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
    }
}

void GroupPanel::markProblemRow(int row, bool scroll_to, bool refresh)
{
    m_grid->setCellBitmap(row,
                          GroupCol_OutputField,
                          GETBMP(gf_exclamation_16),
                          kcl::Grid::alignLeft);

    if (scroll_to)
    {
        m_grid->moveCursor(row, 0, false);
        if (!m_grid->isCursorVisible())
        {
            m_grid->scrollVertToCursor();
        }
    }

    if (refresh)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
    }
}

void GroupPanel::addGroupField(const wxString& input)
{
    insertOutputField(-1, input);
}

void GroupPanel::onExecute(wxCommandEvent& event)
{
    m_grid->endEdit(true);

    tango::IColumnInfoPtr colinfo;
    int i, row_count = m_grid->getRowCount();
    int combo_sel;

    // -- do some rudimentary checks --

    if (row_count == 0)
    {
        cfw::appMessageBox(_("Please specify at least one output field."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    if (!validateGroupQuery())
    {
        cfw::appMessageBox(_("The specified group query must return a boolean (true/false) result."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    
    std::set<wxString> output_field_set;
    std::set<wxString> orig_field_set;
    wxString str;
    int func;
    
    wxString output_name;
    wxString input_name;
    int input_type;

    bool include_detail = m_adv_checkbox->GetValue();
    
    // if detail fields are being included, our duplicate column check
    // must check the base structure fields also for duplicity
    if (include_detail)
    {
        int col_count = m_structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            wxString s = towx(m_structure->getColumnName(i));
            s.MakeUpper();
            output_field_set.insert(s);
            orig_field_set.insert(s);
        }
    }
    
    for (i = 0; i < row_count; ++i)
    {
        input_name = m_grid->getCellString(i, GroupCol_InputExpr);
        output_name = m_grid->getCellString(i, GroupCol_OutputField);
        func = m_grid->getCellComboSel(i, GroupCol_GroupFunc);
        colinfo = m_structure->getColumnInfo(towstr(input_name));
        
        // check for empty output field names
        if (output_name.IsEmpty())
        {
            wxString message = _("One or more of the output fields is empty.  Please specify a valid field name to continue.");
            cfw::appMessageBox(message,
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
        
        // check for empty input field names (excluding
        // 'group id' and 'count' grouping operations)
        if (input_name.IsEmpty())
        {
            if (func != GroupFunc_Count && func != GroupFunc_GroupID)
            {
                wxString message = _("One or more of the input fields is empty.  Please specify an existing field to continue.");
                cfw::appMessageBox(message,
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }
        }
        
        // check for invalid input expressions
        if (colinfo.isNull())
        {
            if (func != GroupFunc_Count && func != GroupFunc_GroupID)
            {
                wxString message = wxString::Format(_("The specified input field '%s' does not exist."),
                                                    input_name.c_str());
                                                
                cfw::appMessageBox(message,
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }
        }
         else
        {
            input_type = colinfo->getType();
        }

        if (func == GroupFunc_Count && input_name.length() > 0)
        {
            // check to make sure that count's parameter is boolean
            tango::IIteratorPtr iter = m_set->createIterator(L"", L"", NULL);
            if (iter.isNull())
                return;

            bool valid = false;

            tango::objhandle_t handle = iter->getHandle(towstr(input_name));
            if (handle)
            {
                valid = (iter->getType(handle) == tango::typeBoolean ? true : false);
                iter->releaseHandle(handle);
            }

            if (!valid)
            {
                cfw::appMessageBox(_("Parameters for the count function must evaluate to either true or false."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }
        }

        // -- check for incompatible input field type/group func type --
        if (func == GroupFunc_Sum || 
            func == GroupFunc_Avg || 
            func == GroupFunc_Stddev ||
            func == GroupFunc_Variance)
        {
            if (input_type != tango::typeDouble &&
                input_type != tango::typeNumeric &&
                input_type != tango::typeInteger)
            {
                cfw::appMessageBox(_("One or more group field definitions has an invalid function type."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }
        }

        // check for invalid output field names
        if (!isValidFieldName(output_name))
        {
            appInvalidFieldMessageBox();
            return;
        }
        
        str = output_name;
        str.MakeUpper();
        str.Trim(true);



        if (include_detail &&
            orig_field_set.find(str) != orig_field_set.end() &&
            func == GroupFunc_GroupBy)
        {
            // this is permissable because group_by fields with
            // 'include detail' checked are not duplicated in
            // the output structure -- this is taken care of below
            continue;
        }

        if (output_field_set.find(str) != output_field_set.end())
        {
            markProblemRow(i, true, true);

            if (m_adv_checkbox->GetValue())
            {
                cfw::appMessageBox(_("Two or more fields have the same output name, or may be conflicting with one of the fields in the detail output.  Please make sure that the name of each output field is unique."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
            }
             else
            {
                cfw::appMessageBox(_("Two or more fields have the same output name.  Please make sure that the name of each output field is unique."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
            }

            return;
        }

        output_field_set.insert(str);
    }



    // -- now create the GroupJob and its associated parameters --

    GroupJob* job = new GroupJob;

    wxString output_columns;
    wxString columns;
    wxString group;

    // -- create the group sort/break key --
    int keypart_count = 0;
    for (i = 0; i < row_count; ++i)
    {
        if (m_grid->getCellComboSel(i, GroupCol_GroupFunc) != GroupFunc_GroupBy)
            continue;

        colinfo = m_structure->getColumnInfo(towstr(m_grid->getCellString(i, GroupCol_InputExpr)));

        wxString temps;
        temps.Printf(wxT("%s%s"),
                     keypart_count == 0 ? wxT("") : wxT(","),
                     colinfo->getName().c_str());

        group += temps;
        keypart_count++;
    }

    if (keypart_count == 0)
    {
        int result = cfw::appMessageBox(_("No key grouping fields have been defined.  The group operation will treat the entire data set as one group. Click 'Ok' if you would like to continue, or 'Cancel' if you would like to go back and specify one or more group fields."),
                                        APPLICATION_NAME,
                                        wxOK | wxCANCEL | wxICON_INFORMATION | wxCENTER);

        if (result == wxCANCEL)
            return;
    }



    // -- create the output functions string --
    int outputpart_count = 0;
    for (i = 0; i < row_count; ++i)
    {
        combo_sel = m_grid->getCellComboSel(i, GroupCol_GroupFunc);
        if (combo_sel < 0)
            continue;

        // if detail records are included, all fields are brought in
        // in addition to the aggregate results.  This makes all "Group By"
        // fields redundant.  Therefore we want to leave those out,
        // as they are already taken care of by the 'include detail' function
        
        wxString str = m_grid->getCellString(i, GroupCol_OutputField);
        str.MakeUpper();
        str.Trim(true);
        
        if (include_detail &&
            combo_sel == GroupFunc_GroupBy &&
            orig_field_set.find(str) != orig_field_set.end())
        {
            continue;
        }

        // construct the output field specifier
        wxString temps;

        temps.Printf(wxT("%s%s=%s(%s)"),
                     outputpart_count == 0 ? wxT("") : wxT(","),        
                     m_grid->getCellString(i, GroupCol_OutputField).c_str(),
                     groupfunc2str(combo_sel).c_str(),
                     m_grid->getCellString(i, GroupCol_InputExpr).c_str());

        output_columns += temps;
        outputpart_count++;
    }


    if (include_detail)
    {
        if (output_columns.Length() > 0)
            output_columns += wxT(",");
            
        output_columns += wxT("[DETAIL]");
    }

    wxString group_query = m_adv_group_query->GetValue();
    group_query.Trim(true);

    job->setInstructions(m_set,
                         group,
                         output_columns,
                         wxT(""),
                         group_query);

    job->sigJobFinished().connect(&onGroupJobFinished);

    // -- add and start job --
    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
    
    // -- close the site --
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void GroupPanel::onCancel(wxCommandEvent& event)
{
    // -- close the site --
    g_app->getMainFrame()->closeSite(m_doc_site);
}


