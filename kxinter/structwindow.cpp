/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-08-27
 *
 */


#include "kxinter.h"
#include "app.h"
#include "structwindow.h"
#include "appcontroller.h"
#include "griddoc.h"
#include "jobmodifystruct.h"


enum
{
    ID_Execute = 10000,
    ID_InsertField,
    ID_DeleteField,
    ID_MakePermanent
};

enum
{
    comboCharacter = 0,
    comboNumeric,
    comboDouble,
    comboInteger,
    comboDate,
    comboDateTime,
    comboBoolean
};

// -- Utility Functions --

int tango2comboType(int tango_type)
{
    switch(tango_type)
    {
        case tango::typeCharacter:
            return comboCharacter;

        case tango::typeNumeric:
            return comboNumeric;

        case tango::typeDouble:
            return comboDouble;

        case tango::typeInteger:
            return comboInteger;

        case tango::typeDate:
            return comboDate;

        case tango::typeDateTime:
            return comboDateTime;

        case tango::typeBoolean:
            return comboBoolean;
    }

    return comboCharacter;
}

int combo2tangoType(int combo_type)
{
    switch(combo_type)
    {
        case comboCharacter:
            return tango::typeCharacter;

        case comboNumeric:
            return tango::typeNumeric;

        case comboDouble:
            return tango::typeDouble;

        case comboInteger:
            return tango::typeInteger;

        case comboDate:
            return tango::typeDate;

        case comboDateTime:
            return tango::typeDateTime;

        case comboBoolean:
            return tango::typeBoolean;
    }

    return tango::typeUndefined;
}





BEGIN_EVENT_TABLE(StructWindow, wxWindow)
    EVT_BUTTON(ID_Execute, StructWindow::onExecute)
    EVT_BUTTON(ID_InsertField, StructWindow::onInsertField)
    EVT_BUTTON(ID_DeleteField, StructWindow::onDeleteField)
    EVT_SIZE(StructWindow::onSize)
    EVT_KCLGRID_END_EDIT(StructWindow::onGridEndEdit)
    EVT_KCLGRID_EDIT_CHANGE(StructWindow::onGridEditChange)
    EVT_KCLGRID_CURSOR_MOVE(StructWindow::onGridCursorMove)
END_EVENT_TABLE()



StructWindow::StructWindow()
{
    m_new_field_counter = 1;
    m_set_name = wxT("/NewTable");
    m_modify = false;
    m_status_bar = NULL;
}

StructWindow::~StructWindow()
{
}

bool StructWindow::initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd)
{
    // -- create document's window --
    bool result = Create(docsite_wnd, -1, wxDefaultPosition, docsite_wnd->GetClientSize(), wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN);
    if (!result)
        return false;

    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxString caption = _("Structure Editor");
    caption += wxT(" - [");
    caption += m_set_name;
    caption += wxT("]");

    m_doc_site = doc_site;

    m_doc_site->setCaption(caption);
    m_doc_site->setBitmap(GET_XPM(xpm_modifystructure));

    m_grid = new kcl::Grid;
    m_grid->setGreenBarInterval(2);
    m_grid->setOptionState(kcl::Grid::optGhostRow | kcl::Grid::optContinuousScrolling, true);
    m_grid->setOptionState(kcl::Grid::optColumnResize | kcl::Grid::optColumnMove | kcl::Grid::optSelect |
                           kcl::Grid::optHorzGridLines | kcl::Grid::optVertGridLines, false);
    m_grid->setCursorType(kcl::Grid::cursorThin);
    m_grid->setBorderType(wxSUNKEN_BORDER);

    m_grid->createModelColumn(-1, _("Field Name"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Type"), kcl::Grid::typeCharacter, 1, 0);
    m_grid->createModelColumn(-1, _("Width"), kcl::Grid::typeInteger, 9, 0);
    m_grid->createModelColumn(-1, _("Decimals"), kcl::Grid::typeInteger, 9, 0);

    m_grid->setRowLabelSize(0);
    m_grid->createDefaultView();

    m_grid->setColumnProportionalSize(0, 6);
    m_grid->setColumnProportionalSize(1, 3);
    m_grid->setColumnProportionalSize(2, 2);
    m_grid->setColumnProportionalSize(3, 2);

    // -- set cell properties for field type column --
    kcl::CellProperties props;
    props.mask = kcl::CellProperties::cpmaskCtrlType | kcl::CellProperties::cpmaskCbChoices;

    props.ctrltype = kcl::Grid::ctrltypeDropList;
    props.cbchoices.push_back(_("Character"));
    props.cbchoices.push_back(_("Numeric"));
    props.cbchoices.push_back(_("Double"));
    props.cbchoices.push_back(_("Integer"));
    props.cbchoices.push_back(_("Date"));
    props.cbchoices.push_back(_("Date/Time"));
    props.cbchoices.push_back(_("Boolean"));
    m_grid->setModelColumnProperties(1, &props);

    m_grid->Create(this, -1, wxPoint(0,0), docsite_wnd->GetClientSize(), 0, kcl::Grid::stateHidden);


    // -- create button sizer --
    kcl::Button* add_button = new kcl::Button(this, ID_InsertField, wxDefaultPosition, wxDefaultSize,
                                            _("Add Field"), GET_XPM(xpm_field_add));
    kcl::Button* delete_button = new kcl::Button(this, ID_DeleteField, wxDefaultPosition, wxDefaultSize,
                                               _("Delete Field"), GET_XPM(xpm_field_delete));
    m_execute_button = new kcl::Button(this, ID_Execute, wxDefaultPosition, wxDefaultSize,
                                               m_modify ? _("Save Changes") : _("Save Structure"), GET_XPM(xpm_save));

    button_sizer->Add(2,2);
    button_sizer->Add(add_button, 0, wxEXPAND | wxRIGHT, 5);
    button_sizer->Add(delete_button, 0, wxEXPAND);
    button_sizer->Add(1,1, 1, wxEXPAND);
    button_sizer->Add(m_execute_button, 0, wxEXPAND);
    button_sizer->Add(2,2);

    // -- create status bar --

    m_status_bar = new wxStatusBar(this, -1);


    main_sizer->Add(button_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);
    main_sizer->Add(m_grid, 1, wxEXPAND, 0);
    main_sizer->Add(m_status_bar, 0, wxEXPAND, 0);

    SetSizer(main_sizer);

    Layout();

    populate();

    m_grid->setVisibleState(kcl::Grid::stateVisible);

    updateStatusBar();

    m_execute_button->Enable(m_actions.size() == 0 ? false : true);

    return true;
}

void StructWindow::onSize(wxSizeEvent& event)
{
    Layout();
}

void StructWindow::setDocumentFocus()
{
    m_grid->SetFocus();
}

bool StructWindow::onSiteClosing(bool force)
{
    if (force)
    {
        return true;
    }

    if (m_actions.size() == 0)
    {
        return true;
    }

    int result = cfw::appMessageBox(_("Would you like to save the changes you made to the table's structure?"),
                               PRODUCT_NAME, wxYES_NO | wxCANCEL | wxCENTER | wxICON_INFORMATION,
                               this);

    if (result == wxYES)
    {
        // -- fire an event to ourselves --
        wxCommandEvent e(wxEVT_COMMAND_BUTTON_CLICKED, ID_Execute);
        ::wxPostEvent(this, e);
        return false;
    }
     else if (result == wxNO)
    {
        return true;
    }
     else
    {
        return false;
    }
}


void StructWindow::setModifySet(tango::ISetPtr modify_set)
{
    m_modify_set = modify_set;
    tango::IStorablePtr sp = m_modify_set;
    if (sp.isOk())
    {
        if (sp->isTemporary())
        {
            m_set_name = _("Untitled");
        }
         else
        {
            m_set_name = towx(sp->getObjectPath());
        }
    }

    m_modify = true;

    updateStatusBar();
}


void StructWindow::populate()
{
    if (m_modify)
    {
        tango::IStructurePtr structure = m_modify_set->getStructure();

        int col_count = structure->getColumnCount();
        for (int i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr column;
            column = structure->getColumnInfoByIdx(i);

            StructField s;
            s.name = towx(column->getName());
            s.type = column->getType();
            s.width = column->getWidth();
            s.scale = column->getScale();
            s.dynamic = column->getCalculated() ? 1 : 0;
            s.dyn_state = column->getCalculated();
            s.pos = i;

            m_fields.push_back(s);
        }
    }

    std::list<StructField>::iterator it;

    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        m_grid->insertRow(it->pos);
        m_grid->setRowData(it->pos, (long)&(*it));
        m_grid->setCellString(it->pos, 0, cfw::makeProper(towx(it->name)));
        m_grid->setCellComboSel(it->pos, 1, tango2comboType(it->type));
        m_grid->setCellInteger(it->pos, 2, it->width);
        m_grid->setCellInteger(it->pos, 3, it->scale);
        m_grid->setCellBitmap(it->pos, 0, it->dyn_state ? GET_XPM(xpm_lightning) : GET_XPM(xpm_field), kcl::Grid::alignLeft);

        setRowCellProps(it->pos);
    }

    m_grid->refresh(kcl::Grid::refreshAll);

    // -- update make-permanent-button's enabled status --
    kcl::GridEvent evt;
    evt.SetDestinationRow(0);
    onGridCursorMove(evt);
}


void StructWindow::updateStatusBar()
{
    if (!m_status_bar)
        return;

    if (m_status_bar->GetFieldsCount() != 3)
    {
        int widths[3] = { 120, 120, -1 };
        m_status_bar->SetFieldsCount(3, widths);
    }


    // -- field count --
    int row_count = m_grid->getRowCount();
    wxString fcount = wxString::Format(_("Field Count: %d"), row_count);
    m_status_bar->SetStatusText(fcount, 0);


    // -- row width --
    int total_width = 0;
    for (int i = 0; i < row_count; ++i)
    {
        total_width += m_grid->getCellInteger(i, 2);
    }

    wxString rowwidth = wxString::Format(_("Record Width: %d"), total_width);
    m_status_bar->SetStatusText(rowwidth, 1);


    if (m_modify)
    {
        m_status_bar->SetStatusText(m_set_name.c_str(), 2);
    }
}


void StructWindow::setRowCellProps(int row)
{
    int type = combo2tangoType(m_grid->getCellComboSel(row, 1));
    
    bool width_editable = true;
    bool decimal_editable = true;

    switch (type)
    {
        case tango::typeCharacter:
            decimal_editable = false;
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
        case tango::typeNumeric:
            width_editable = true;
            decimal_editable = true;
            break;
    }

    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskEditable |
                     kcl::CellProperties::cpmaskFgColor;

    cellprops.fgcolor = width_editable ? wxColor(0,0,0) : wxColor(128,128,128);
    cellprops.editable = width_editable;

    m_grid->setCellProperties(row, 2, &cellprops);

    cellprops.fgcolor = decimal_editable ? wxColor(0,0,0) : wxColor(128,128,128);
    cellprops.editable = decimal_editable;

    m_grid->setCellProperties(row, 3, &cellprops);
}


void StructWindow::onGridCursorMove(kcl::GridEvent& event)
{

}


void StructWindow::onGridEndEdit(kcl::GridEvent& event)
{
    if (event.GetEditCancelled())
        return;

    wxString orig_name;
    bool modify_row = false;
    bool res = false;

    int col = event.GetColumn();
    int row = event.GetRow();

    // -- check if the row is new-- if it's not, we are modifying an
    //    existing field.  If it is new, then we are adding a new field. --

    StructField* s = (StructField*)m_grid->getRowData(event.GetRow());
    if (s)
    {
        orig_name = s->name;
        modify_row = true;
    }

    // -- modifying a field in the table --
    if (modify_row)
    {

        switch (col)
        {
            case 0:
            {
                wxString new_name = event.GetString();
                res = modifyField(orig_name, new_name, -1, -1, -1, row);
            }
            break;

            case 1:
                res = modifyField(orig_name, wxT(""), combo2tangoType(m_grid->getCellComboSel(row, 1)), -1, -1, row);
                break;

            case 2:
                res = modifyField(orig_name, wxT(""), -1, event.GetInt(), -1, row);
                break;

            case 3:
                res = modifyField(orig_name, wxT(""), -1, -1, event.GetInt(), row);
                break;
        }
    }
     else
    {
        switch (col)
        {
            case 0:
                res = addField(event.GetString(), -1, -1, -1, row);
                break;
            case 1:
                res = addField(wxT(""), combo2tangoType(m_grid->getCellComboSel(row, 1)), -1, -1, row);
                break;
            case 2:
                res = addField(wxT(""), -1, event.GetInt(), -1, row);
                break;
            case 3:
                res = addField(wxT(""), -1, -1, event.GetInt(), row);
                break;
        }
    }

    if (!res)
    {
        // --if something went wrong, do not allow the user to add the field --
        event.Veto();
        return;
    }

    setRowCellProps(row);

    m_grid->refresh(kcl::Grid::refreshAll);

    m_execute_button->Enable(m_actions.size() == 0 ? false : true);

    updateStatusBar();
}


void StructWindow::onGridEditChange(kcl::GridEvent& event)
{
    int col = event.GetColumn();
    int row = event.GetRow();

    StructField* s = (StructField*)m_grid->getRowData(row);
    if (!s)
    {
        return;
    }

    if (col == 1)
    {
        int tango_type = combo2tangoType(m_grid->getCellComboSel(row, 1));
        int width = -1;
        int scale = -1;

        switch (tango_type)
        {
            case tango::typeCharacter:
                if (s->type == tango::typeDate)
                {
                    width = 10;
                }
                 else if (s->type == tango::typeDateTime)
                {
                    width = 20;
                }

                scale = 0;
                break;

            case tango::typeNumeric:
                width = 18;
                break;

            case tango::typeDouble:
                width = 8;
                break;

            case tango::typeInteger:
                width = 4;
                scale = 0;
                break;

            case tango::typeDate:
                width = 4;
                scale = 0;
                break;

            case tango::typeDateTime:
                width = 8;
                scale = 0;
                break;

            case tango::typeBoolean:
                width = 1;
                scale = 0;
                break;

            default:
                width = -1;
                break;
        }

        if (width != -1)
        {
            m_grid->setCellInteger(row, 2, width);
        }

        if (scale != -1)
        {
            m_grid->setCellInteger(row, 3, scale);
        }

        modifyField(s->name, wxT(""), tango_type, width, scale, row);
        setRowCellProps(row);
        m_execute_button->Enable(m_actions.size() == 0 ? false : true);

        m_grid->refresh(kcl::Grid::refreshAll);
    }
}


void StructWindow::onInsertField(wxCommandEvent& event)
{
    insertField(-1);
    m_execute_button->Enable(m_actions.size() == 0 ? false : true);
}


void StructWindow::onDeleteField(wxCommandEvent& event)
{
    deleteField(m_grid->getCursorRow());
    m_execute_button->Enable(m_actions.size() == 0 ? false : true);

    updateStatusBar();
}


bool StructWindow::addField(const wxString& name,
                            int type,
                            int width,
                            int scale,
                            int pos)
{
    std::list<StructAction>::iterator it;
    for (it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        // -- found it --
        if (it->new_field.pos == pos)
        {
            if (!name.IsEmpty())
            {
                it->new_field.name = name;
            }

            if (type != -1)
            {
                it->new_field.type = type;
            }

            if (width != -1)
            {
                it->new_field.width = width;
            }

            if (scale != -1)
            {
                it->new_field.scale = scale;
            }

            return true;
        }
    }

    StructAction s;
    s.action = StructAction::addField;
    s.new_field.name = name;
    if (type == -1)
    {
        s.new_field.type = tango::typeCharacter;
        m_grid->setCellComboSel(pos, 1, tango2comboType(tango::typeCharacter));
    }
     else
    {
        s.new_field.type = type;
    }

    if (width == -1)
    {
        s.new_field.width = 20;
        m_grid->setCellInteger(pos, 2, 20);
    }
     else
    {
        s.new_field.width = width;
    }

    if (scale == -1)
    {
        s.new_field.scale = 0;
        m_grid->setCellInteger(pos, 3, 0);
    }
     else
    {
        s.new_field.scale = scale;
    }

    s.new_field.pos = pos;

    m_actions.push_back(s);

    m_grid->refresh(kcl::Grid::refreshAll);


    return true;
}


bool StructWindow::modifyField(const wxString& oldname,
                               const wxString& name,
                               int type,
                               int width,
                               int scale,
                               int pos,
                               int dynamic)
{
    // -- first, try to find an existing action for this field --
    if (!oldname.IsEmpty())
    {
        std::list<StructAction>::iterator it;
        for (it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            if (it->action == StructAction::modifyField &&
                !it->old_field.name.CmpNoCase(oldname))
            {
                // -- found it --
                if (!name.IsEmpty())
                {
                    if (!it->old_field.name.CmpNoCase(name))
                    {
                        it->new_field.name = wxT("");
                    }
                     else
                    {
                        it->new_field.name = name;
                    }
                }

                if (type != -1)
                {
                    if (it->old_field.type == type)
                    {
                        it->new_field.type = -1;
                    }
                     else
                    {
                        it->new_field.type = type;
                    }
                }

                if (width != -1)
                {
                    if (it->old_field.width == width)
                    {
                        it->new_field.width = -1;
                    }
                     else
                    {
                        it->new_field.width = width;
                    }
                }

                if (scale != -1)
                {
                    if (it->old_field.scale == scale)
                    {
                        it->new_field.scale = -1;
                    }
                     else
                    {
                        it->new_field.scale = scale;
                    }
                }

                if (dynamic != -1)
                {
                    if (it->old_field.dynamic == dynamic)
                    {
                        it->new_field.dynamic = -1;
                    }
                     else
                    {
                        it->new_field.dynamic = dynamic;
                    }
                }

                if (it->new_field.name.IsEmpty() &&
                    it->new_field.type == -1 &&
                    it->new_field.width == -1 &&
                    it->new_field.scale == -1 &&
                    it->new_field.dynamic == -1)
                {
                    m_actions.erase(it);
                }

                return true;
            }
        }
    }

    StructAction s;

    s.action = StructAction::modifyField;

    std::list<StructField>::iterator field_it;
    for (field_it = m_fields.begin(); field_it != m_fields.end(); ++field_it)
    {
        if (!field_it->name.CmpNoCase(oldname))
        {
            s.old_field = *field_it;
            break;
        }
    }


    bool add_action = false;

    s.new_field.name = wxT("");
    s.new_field.type = -1;
    s.new_field.width = -1;
    s.new_field.scale = -1;
    s.new_field.dynamic = -1;
    s.new_field.pos = pos;

    if (!name.IsEmpty())
    {
        if (s.old_field.name.CmpNoCase(name) != 0)
        {
            s.new_field.name = name;
            add_action = true;
        }
    }

    if (type != -1)
    {
        if (s.old_field.type != type)
        {
            s.new_field.type = type;
            add_action = true;
        }
    }

    if (width != -1)
    {
        if (s.old_field.width != width)
        {
            s.new_field.width = width;
            add_action = true;
        }
    }

    if (scale != -1)
    {
        if (s.old_field.scale != scale)
        {
            s.new_field.scale = scale;
            add_action = true;
        }
    }

    if (dynamic != -1)
    {
        if (s.old_field.dynamic != dynamic)
        {
            s.new_field.dynamic = dynamic;
            add_action = true;
        }
    }

    if (add_action)
    {
        m_actions.push_back(s);
    }

    return true;
}


bool StructWindow::moveField(int pos)
{

    return true;
}


bool StructWindow::insertField(int pos)
{
    if (pos == -1)
    {
        // -- append a field --

        StructField sf;
        m_fields.insert(m_fields.end(), sf);
        m_grid->insertRow(-1);
        m_grid->scrollToBottom();
        m_grid->moveCursor(m_grid->getRowCount()-1, 0);
        m_grid->refresh(kcl::Grid::refreshAll);
        m_grid->beginEdit(wxT(""));

        // -- update the last row's cell props --
        setRowCellProps(m_grid->getRowCount()-1);
        return true;
    }

    std::list<StructField>::iterator it2;
    StructField sf;

    int i = 0;
    for (it2 = m_fields.begin(); it2 != m_fields.end(); it2++)
    {
        if (i == pos)
        {
            it2->pos = i+1;

            sf.pos = i;
            m_fields.insert(it2, sf);
        }

        if (i > pos)
        {
            it2->pos = i+1;
        }

        i++;
    }

    m_grid->insertRow(pos);
    m_grid->moveCursor(pos, m_grid->getCursorColumn());
    m_grid->refresh(kcl::Grid::refreshAll);

    // -- update the last row's cell props --
    setRowCellProps(pos);

    return true;
}


bool StructWindow::deleteField(int pos)
{
    int i = 0;
    std::list<StructField>::iterator it;
    StructAction sa;

    sa.action = StructAction::deleteField;

    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (i == pos)
        {
            StructField* s = (StructField*)m_grid->getRowData(i);
            if (s)
            {
                sa.old_field = *s;
            }
             else
            {
                sa.new_field = *it;
            }
            
            it = m_fields.erase(it);
            it->pos = i;
        }

        if (i > pos)
        {
            it->pos = i;
        }

        i++;
    }

    m_actions.push_back(sa);

    m_grid->deleteRow(pos);
    m_grid->moveCursor(pos, m_grid->getCursorColumn());
    m_grid->refresh(kcl::Grid::refreshAll);

    return true;
}


bool StructWindow::markDuplicateFields()
{
    bool field_dups_exist = false;
    
    kcl::CellProperties props;
    props.mask = kcl::CellProperties::cpmaskBgColor;

    // -- find and mark duplicate fields --
    int row_count = m_grid->getRowCount();
    for (int i = 0; i < row_count; ++i)
    {
        for (int k = 0; k < m_grid->getColumnCount(); k++)
        {
            if (i % 2 == 0)
            {
                props.bgcolor = wxColour(255, 255, 255);
            }
             else
            {
                props.bgcolor = wxColour(232, 232, 232);
            }

            m_grid->setCellProperties(i, k, &props);
        }

        for (int j = 0; j < row_count; ++j)
        {
            if (i == j)
            {
                continue;
            }

            if (!m_grid->getCellString(i, 0).CmpNoCase(m_grid->getCellString(j,0)) && !m_grid->getCellString(i, 0).IsEmpty())
            {
                props.bgcolor = wxColor(255, 128, 128);

                for (int k = 0; k < m_grid->getColumnCount(); k++)
                {
                    m_grid->setCellProperties(i, k, &props);
                    m_grid->setCellProperties(j, k, &props);
                }

                field_dups_exist = true;
            }
        }
    }

    m_grid->refresh(kcl::Grid::refreshAll);

    if (field_dups_exist)
    {
        cfw::appMessageBox(_("The table contains duplicate field names.  Remove the duplicate fields in order to save changes to the table structure."),
                      _("Duplicate Field Exist"),
                      wxOK|wxICON_EXCLAMATION|wxCENTRE,
                      this);
    }

    return field_dups_exist ? false : true;
}


bool StructWindow::validateStructure()
{
    // -- first, check for any invalid field names --
    wxString s;
    
    int type;
    int width;
    int scale;

    int row_count = m_grid->getRowCount();
    for (int i = 0; i < row_count; ++i)
    {
        s = m_grid->getCellString(i, 0);
        s.Trim(true);

        /*
        if (!isFieldNameOk(s))
        {
            appMessageBox(_("A field with an invalid name has been found.  Field names must contain only alphanumeric (0-9, A-Z) characters, begin with a letter, and be between 1 and 80 characters in length."),
                          _("Invalid Field Name"),
                          wxOK | wxICON_EXCLAMATION | wxCENTRE,
                          this);
            return false;
        }
        */

        type = combo2tangoType(m_grid->getCellComboSel(i, 1));
        width = m_grid->getCellInteger(i, 2);
        scale = m_grid->getCellInteger(i, 3);

        if (width <= 0)
        {
            cfw::appMessageBox(_("A numeric field with a width less than or equal to zero has been found. Fields must have a width of at least one."),
                          _("Invalid Field Width"),
                          wxOK | wxICON_EXCLAMATION | wxCENTRE,
                          this);
            return false;
        }

        if (type == tango::typeNumeric || type == tango::typeDouble)
        {
            if (scale > tango::max_numeric_scale)
            {
                wxString message = wxString::Format(_("A numeric field with an invalid number of decimal places has been found.  The number of decimal places for a numeric field must not exceed %d digits."),
                                                    tango::max_numeric_scale);

                cfw::appMessageBox(message,
                              _("Invalid Field Scale"),
                              wxOK | wxICON_EXCLAMATION | wxCENTRE,
                              this);

                return false;
            }

            if (type == tango::typeNumeric && width > tango::max_numeric_width)
            {
                wxString message = wxString::Format(_("A numeric field with an invalid width has been found.  The width for a numeric field must not exceed %d digits."),
                                                    tango::max_numeric_width);

                cfw::appMessageBox(message,
                              _("Invalid Field Width"),
                              wxOK | wxICON_EXCLAMATION | wxCENTRE,
                              this);

                return false;
            }

            if (type == tango::typeNumeric && scale >= width)
            {
                cfw::appMessageBox(_("The number of decimal places must be less than the field width."),
                              _("Invalid Field Scale"),
                              wxOK | wxICON_EXCLAMATION | wxCENTRE,
                              this);

                return false;
            }
        }
    }

    if (!markDuplicateFields())
    {
        return false;
    }

    return true;
}


void StructWindow::onExecute(wxCommandEvent& event)
{
    m_grid->endEdit(true);

    if (m_grid->getRowCount() == 0)
    {
        cfw::appMessageBox(_("The structure cannot be modified because no fields would exist in the output table."),
                        PRODUCT_NAME,
                        wxOK | wxCENTER | wxICON_INFORMATION,
                        this);
        return;
    }

    if (m_actions.size() == 0)
    {
        cfw::appMessageBox(_("The structure cannot be modified because no changes have been made to the table."),
                        PRODUCT_NAME,
                        wxOK | wxCENTER | wxICON_INFORMATION,
                        this);
        return;
    }


    if (!validateStructure())
    {
        return;
    }


    if (m_modify)
    {
        // -- warn user against possible data loss (like when a numeric field with
        //    decimal places is converted to an integer field) --

        bool data_loss_warning = false;

        std::list<StructAction>::iterator it;
        for (it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            if (it->action == StructAction::modifyField)
            {
                if (it->old_field.scale > 0)
                {
                    if (it->new_field.type == tango::typeInteger)
                    {
                        data_loss_warning = true;
                        break;
                    }
                }

                if (it->old_field.type == tango::typeNumeric ||
                    it->old_field.type == tango::typeDouble ||
                    it->old_field.type == tango::typeInteger)
                {
                    if (it->new_field.type == tango::typeDate ||
                        it->new_field.type == tango::typeDateTime ||
                        it->new_field.type == tango::typeBoolean)
                    {
                        data_loss_warning = true;
                    }
                }

                if (it->old_field.type == tango::typeCharacter &&
                    it->new_field.type != -1)
                {
                    data_loss_warning = true;
                }
            }
        }

        if (data_loss_warning)
        {
            int result = cfw::appMessageBox(_("One or more structure modifications might result in data truncation (data loss).  Are you sure you want to continue?"),
                                    PRODUCT_NAME,
                                    wxYES_NO | wxCENTER | wxICON_QUESTION,
                                    this);

            if (result == wxNO)
            {
                return;
            }

        }
    }



    // -- close the structure editor window --
    m_doc_site->closeSite();


    if (m_modify)
    {
        // -- close all sets of the same path which are open --
        cfw::IDocumentSitePtr target_site;

        IGridDocPtr grid_doc;
        cfw::IDocumentSitePtr site;
        cfw::IDocumentSiteEnumPtr sites = g_app->getMainFrame()->getDocumentSites(cfw::sitetypeNormal);
        int site_count = sites->size();

        for (int i = 0; i < site_count; ++i)
        {
            site = sites->getItem(i);
            if (site.isNull())
                continue;
            grid_doc = site->getDocument();
            if (grid_doc.isNull())
                continue;
            if (m_modify_set == grid_doc->getSet())
            {
                if (target_site.isNull())
                {
                    target_site = site;
                }
                 else
                {
                    site->closeSite();
                }
            }
        }


        // -- submit structure modify job --
        modifyTable(target_site);
    }
     else
    {
        // -- submit structure create job --
        createTable();
    }
}


bool StructWindow::modifyTable(cfw::IDocumentSitePtr target_site)
{
    tango::IStructurePtr structure = m_modify_set->getStructure();
    tango::IColumnInfoPtr colinfo;
    std::list<StructAction>::iterator it;
    StructField* s;

    ModifyStructJob* job = new ModifyStructJob;

    for (it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        switch(it->action)
        {
            case StructAction::undefined:
                break;

            case StructAction::addField:
                colinfo = structure->createColumn();
                colinfo->setName(towstr(it->new_field.name));
                colinfo->setType(it->new_field.type);
                colinfo->setWidth(it->new_field.width);
                colinfo->setScale(it->new_field.scale);
                colinfo->setColumnOrdinal(it->new_field.pos);

                job->addToInsert(it->new_field.name);
                break;

            case StructAction::deleteField:
                // -- deleting a field which was in the orginal table --
                s = &(it->old_field);
                if (s)
                {
                    colinfo = structure->deleteColumn(towstr(it->old_field.name));
                }
                 else
                {
                    // -- deleting a field which was added to the structure --
                    
                }

                job->addToDelete(it->old_field.name);
                break;

            case StructAction::moveField:
                // colinfo = structure->moveColumn(it->old_field.name)
                break;

            case StructAction::modifyField:
                colinfo = structure->modifyColumn(towstr(it->old_field.name));
                colinfo->setName(towstr(it->new_field.name));
                colinfo->setType(it->new_field.type);
                colinfo->setWidth(it->new_field.width);
                colinfo->setScale(it->new_field.scale);
                colinfo->setColumnOrdinal(it->new_field.pos);

                if (it->new_field.dynamic == 0)
                {
                    colinfo->setCalculated(false);
                }

                if (it->new_field.name.length() > 0 &&
                    it->old_field.name.CmpNoCase(it->new_field.name) != 0)
                {
                    job->addToRename(it->old_field.name, it->new_field.name);
                }
                break;
        }
    }

    job->setInstructions(m_modify_set, structure);
    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
    g_app->getAppController()->showJobManager();    

    return true;
}


bool StructWindow::createTable()
{
    std::list<StructAction>::iterator it;

    IConnectionPtr c = g_app->getConnectionMgr()->getActiveConnection();
    tango::IStructurePtr sc = c->getDatabasePtr()->createStructure();

    for (it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        tango::IColumnInfoPtr ci = sc->createColumn();
        ci->setName(towstr(it->new_field.name));
        ci->setType(it->new_field.type);
        ci->setWidth(it->new_field.width);
        ci->setScale(it->new_field.scale);
    }

    tango::ISetPtr new_set = c->getDatabasePtr()->createSet(L"", sc, NULL);

    return true;
}




// -- utility function used to view vector --

void StructWindow::outputFieldList()
{
#ifdef _DEBUG
    ::wxLogTrace(wxT("kxinter"), wxT("----FIELD DUMP BEGIN----"));
    wxString s;
    for (std::list<StructField>::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        s = wxString::Format(wxT("name: %s; type: %d; width: %d; scale: %d; pos = %d"),
                     towx(it->name).c_str(), it->type, it->width, it->scale, it->pos);

        ::wxLogTrace(wxT("kxinter"), s);
    }
    ::wxLogTrace(wxT("kxinter"), wxT("------------------------\n"));
#endif
}

void StructWindow::outputActionList()
{
#ifdef _DEBUG
    ::wxLogTrace(wxT("kxinter"), wxT("----ACTION DUMP BEGIN----"));
    for (std::list<StructAction>::iterator it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        ::wxLogTrace(wxT("kxinter"), wxT("action: %d; name: %s; type: %d; width: %d; scale: %d; pos: %d; old_name: %s; old_type: %d; old_width: %d; old_scale: %d; old_pos = %d"),
                         it->action, towx(it->new_field.name).c_str(), it->new_field.type, it->new_field.width, it->new_field.scale, it->new_field.pos,
                                     towx(it->old_field.name).c_str(), it->old_field.type, it->old_field.width, it->old_field.scale, it->old_field.pos);
    }
    ::wxLogTrace(wxT("kxinter"), wxT("-------------------------\n"));
#endif
}


