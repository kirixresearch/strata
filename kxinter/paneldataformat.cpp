/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-19
 *
 */


#include "kxinter.h"
#include "paneldataformat.h"
#include "datadoc.h"


enum
{
    ColName = 0,
    ColFormat = 1,
    ColType = 2,
    ColOffset = 3,
    ColWidth = 4,
    ColDec = 5
};



static wxString getFormatString(int type)
{
    switch (type)
    {
        case ddformatASCII:        return _("ASCII Text");
        case ddformatEBCDIC:       return _("EBCDIC Text");
        case ddformatCompNumeric:  return _("COBOL COMP");
        case ddformatComp3Numeric: return _("COBOL COMP-3");
    }

    return wxT("");
}


static wxString getTypeString(int type)
{
    switch (type)
    {
        case ddtypeCharacter:       return _("Character");
        case ddtypeDecimal:         return _("Decimal Number");
        case ddtypeNumeric:         return _("Numeric (Implied Decimal)");
        case ddtypeDateYYMMDD:      return _("Date (YYMMDD)");
        case ddtypeDateMMDDYY:      return _("Date (MMDDYY)");
        case ddtypeDateCCYYMMDD:    return _("Date (CCYYMMDD)");
        case ddtypeDateCCYY_MM_DD:  return _("Date (CCYY-MM-DD)");
    }

    return wxT("");
}



// -- Record Defintion Page class --

class RecDefPage : public wxWindow
{

public:

    RecDefPage(wxWindow* parent);
    ~RecDefPage();

    void setRecordDefinition(RecordDefinition* rd);
    void populate();
    void editFieldName(int field);

private:

    void onSize(wxSizeEvent& event);

    void onGridEditChange(kcl::GridEvent& event);
    void onGridEndEdit(kcl::GridEvent& event);
    void onTableNameChanged(wxCommandEvent& event);
    void onRecIdOffsetChanged(wxCommandEvent& event);
    void onRecIdByteChanged(wxCommandEvent& event);
    void onAppendField(wxCommandEvent& event);
    void onInsertField(wxCommandEvent& event);
    void onDeleteField(wxCommandEvent& event);
    void onShiftDown(wxCommandEvent& event);
    void onShiftUp(wxCommandEvent& event);

private:

    kcl::Grid* m_grid;
    RecordDefinition* m_rd;
    wxTextCtrl* m_name_text;
    wxTextCtrl* m_reclen_text;
    wxTextCtrl* m_recid_offset_text;
    wxTextCtrl* m_recid_byte_text;

    DECLARE_EVENT_TABLE()
};



enum
{
    ID_TableName = 8000,
    ID_RecLength,
    ID_RecIdOffset,
    ID_RecIdByte,
    ID_AppendField,
    ID_InsertField,
    ID_DeleteField,
    ID_ShiftDown,
    ID_ShiftUp
};

BEGIN_EVENT_TABLE(RecDefPage, wxWindow)
    EVT_SIZE(RecDefPage::onSize)
    EVT_TEXT(ID_TableName, RecDefPage::onTableNameChanged)
    EVT_TEXT(ID_RecIdOffset, RecDefPage::onRecIdOffsetChanged)
    EVT_TEXT(ID_RecIdByte, RecDefPage::onRecIdByteChanged)
    EVT_BUTTON(ID_AppendField, RecDefPage::onAppendField)
    EVT_BUTTON(ID_InsertField, RecDefPage::onInsertField)
    EVT_BUTTON(ID_DeleteField, RecDefPage::onDeleteField)
    EVT_BUTTON(ID_ShiftDown, RecDefPage::onShiftDown)
    EVT_BUTTON(ID_ShiftUp, RecDefPage::onShiftUp)
    EVT_KCLGRID_EDIT_CHANGE(RecDefPage::onGridEditChange)
    EVT_KCLGRID_END_EDIT(RecDefPage::onGridEndEdit)
END_EVENT_TABLE()



RecDefPage::RecDefPage(wxWindow* parent) : wxWindow(parent, -1)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);

    // -- top sizer --

    wxStaticBox* general_static_box = new wxStaticBox(this, -1, _("General"));
    wxStaticBoxSizer* general_sizer = new wxStaticBoxSizer(general_static_box, wxVERTICAL);
    
    wxBoxSizer* name_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* name_static = new wxStaticText(this, -1, _("Table Name:"));
    m_name_text = new wxTextCtrl(this, ID_TableName);
    name_sizer->Add(name_static, 0, wxALIGN_CENTER);
    name_sizer->Add(m_name_text, 1, wxEXPAND | wxLEFT, 5);

    wxBoxSizer* reclen_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* reclen_static = new wxStaticText(this, -1, _("Record Length:"));
    m_reclen_text = new wxTextCtrl(this, ID_RecLength);
    reclen_sizer->Add(reclen_static, 0, wxALIGN_CENTER);
    reclen_sizer->Add(m_reclen_text, 0, wxEXPAND | wxLEFT, 5);

    general_sizer->Add(name_sizer, 1, wxEXPAND);
    general_sizer->Add(reclen_sizer, 1, wxEXPAND);
    
    wxSize general_max_size = cfw::getMaxTextSize(name_static, reclen_static);
    name_sizer->SetItemMinSize(name_static, general_max_size);
    reclen_sizer->SetItemMinSize(reclen_static, general_max_size);



    // -- rec id static box --

    wxStaticBox* recid_static_box = new wxStaticBox(this, -1, _("Record ID"));
    wxStaticBoxSizer* recid_sizer = new wxStaticBoxSizer(recid_static_box, wxVERTICAL);
    
    wxBoxSizer* recid_offset_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* recid_offset_static = new wxStaticText(this, -1, _("Record ID Offset:"));
    m_recid_offset_text = new wxTextCtrl(this, ID_RecIdOffset);
    recid_offset_sizer->Add(recid_offset_static, 0, wxALIGN_CENTER);
    recid_offset_sizer->Add(m_recid_offset_text, 1, wxEXPAND | wxLEFT, 5);

    wxBoxSizer* recid_byte_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* recid_byte_static = new wxStaticText(this, -1, _("Record ID Byte:"));
    m_recid_byte_text = new wxTextCtrl(this, ID_RecIdByte);
    recid_byte_sizer->Add(recid_byte_static, 0, wxALIGN_CENTER);
    recid_byte_sizer->Add(m_recid_byte_text, 1, wxEXPAND | wxLEFT, 5);

    recid_sizer->Add(recid_offset_sizer);
    recid_sizer->Add(recid_byte_sizer);
    
    wxSize recid_max_size = cfw::getMaxTextSize(recid_offset_static, recid_byte_static);
    recid_offset_sizer->SetItemMinSize(recid_offset_static, recid_max_size);
    recid_byte_sizer->SetItemMinSize(recid_byte_static, recid_max_size);

    top_sizer->Add(general_sizer, 1, wxRIGHT, 5);
    top_sizer->Add(recid_sizer);



    // -- button sizer --

    kcl::Button* append_button = new kcl::Button(this, ID_AppendField, wxDefaultPosition, wxDefaultSize,
                                            _("Append"), GET_XPM(xpm_field_add));
    kcl::Button* insert_button = new kcl::Button(this, ID_InsertField, wxDefaultPosition, wxDefaultSize,
                                            _("Insert"), GET_XPM(xpm_field_insert));
    kcl::Button* delete_button = new kcl::Button(this, ID_DeleteField, wxDefaultPosition, wxDefaultSize,
                                               _("Delete"), GET_XPM(xpm_field_delete));
    kcl::Button* shiftdown_button = new kcl::Button(this, ID_ShiftDown, wxDefaultPosition, wxDefaultSize,
                                               _("Shift Down"), GET_XPM(xpm_downarrow));
    kcl::Button* shiftup_button = new kcl::Button(this, ID_ShiftUp, wxDefaultPosition, wxDefaultSize,
                                               _("Shift Up"), GET_XPM(xpm_uparrow));

    button_sizer->Add(2,2);
    button_sizer->Add(append_button, 0, wxEXPAND | wxRIGHT, 5);
    button_sizer->Add(insert_button, 0, wxEXPAND | wxRIGHT, 5);
    button_sizer->Add(delete_button, 0, wxEXPAND | wxRIGHT, 5);
    button_sizer->Add(1,1, 1, wxEXPAND);
    button_sizer->Add(shiftdown_button, 0, wxEXPAND | wxRIGHT, 5);
    button_sizer->Add(shiftup_button, 0, wxEXPAND | wxRIGHT, 5);


    // -- grid --

    m_grid = new kcl::Grid;

    m_grid->setOptionState(kcl::Grid::optHorzGridLines |
                           kcl::Grid::optVertGridLines |
                           kcl::Grid::optSelect |
                           kcl::Grid::optColumnMove |
                           kcl::Grid::optEditBoxBorder, false);

    m_grid->setGreenBarInterval(2);
    m_grid->setOptionState(kcl::Grid::optGhostRow |
                           kcl::Grid::optContinuousScrolling, true);

    m_grid->setCursorType(kcl::Grid::cursorRowOutline);
    m_grid->setRowLabelSize(0);

    m_grid->createModelColumn(-1, _("Name"), kcl::Grid::typeCharacter, 256, 0);
    m_grid->createModelColumn(-1, _("Format"), kcl::Grid::typeCharacter, 50, 0);
    m_grid->createModelColumn(-1, _("Type"), kcl::Grid::typeCharacter, 50, 0);
    m_grid->createModelColumn(-1, _("Offset"), kcl::Grid::typeInteger, 4, 0);
    m_grid->createModelColumn(-1, _("Width"), kcl::Grid::typeInteger, 4, 0);
    m_grid->createModelColumn(-1, _("Dec"), kcl::Grid::typeInteger, 4, 0);
    m_grid->createDefaultView();
    m_grid->Create(this, -1);


    // -- set cell properties for field format column --
    kcl::CellProperties format_props;
    format_props.mask = kcl::CellProperties::cpmaskCtrlType | kcl::CellProperties::cpmaskCbChoices;
    format_props.ctrltype = kcl::Grid::ctrltypeDropList;
    format_props.cbchoices.push_back(getFormatString(ddformatASCII));
    format_props.cbchoices.push_back(getFormatString(ddformatEBCDIC));
    format_props.cbchoices.push_back(getFormatString(ddformatCompNumeric));
    format_props.cbchoices.push_back(getFormatString(ddformatComp3Numeric));
    m_grid->setModelColumnProperties(ColFormat, &format_props);

    // -- set cell properties for field type column --
    kcl::CellProperties type_props;
    type_props.mask = kcl::CellProperties::cpmaskCtrlType | kcl::CellProperties::cpmaskCbChoices;
    type_props.ctrltype = kcl::Grid::ctrltypeDropList;
    type_props.cbchoices.push_back(getTypeString(ddtypeCharacter));
    type_props.cbchoices.push_back(getTypeString(ddtypeDecimal));
    type_props.cbchoices.push_back(getTypeString(ddtypeNumeric));
    type_props.cbchoices.push_back(getTypeString(ddtypeDateYYMMDD));
    type_props.cbchoices.push_back(getTypeString(ddtypeDateMMDDYY));
    type_props.cbchoices.push_back(getTypeString(ddtypeDateCCYYMMDD));
    type_props.cbchoices.push_back(getTypeString(ddtypeDateCCYY_MM_DD));
    m_grid->setModelColumnProperties(ColType, &type_props);

    m_grid->setColumnProportionalSize(ColName, 1);
    m_grid->setColumnSize(ColFormat, 95);
    m_grid->setColumnSize(ColType, 130);
    m_grid->setColumnSize(ColOffset, 35);
    m_grid->setColumnSize(ColWidth, 35);
    m_grid->setColumnSize(ColDec, 35);


    sizer->Add(top_sizer, 0, wxEXPAND | wxALL, 5);
    sizer->Add(button_sizer, 0, wxEXPAND);
    sizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);
    SetSizer(sizer);

    Layout();
}



RecDefPage::~RecDefPage()
{
}


void RecDefPage::setRecordDefinition(RecordDefinition* rd)
{
    m_rd = rd;
}

void RecDefPage::populate()
{
    m_name_text->SetValue(m_rd->name);
    m_reclen_text->SetValue(wxString::Format(wxT("%d"), m_rd->record_length));
    m_recid_offset_text->SetValue(wxString::Format(wxT("%d"), m_rd->id_offset));
    m_recid_byte_text->SetValue(wxString::Format(wxT("%d"), m_rd->id_value));

    m_grid->deleteAllRows();

    int row = 0;

    std::vector<FieldDefinition>::iterator f_it;
    for (f_it = m_rd->field_defs.begin();
         f_it != m_rd->field_defs.end(); ++f_it)
    {
        m_grid->insertRow(-1);

        m_grid->setCellString(row, ColName, f_it->name);
        m_grid->setCellComboSel(row, ColFormat, f_it->format);
        m_grid->setCellComboSel(row, ColType, f_it->type);
        m_grid->setCellInteger(row, ColOffset, f_it->offset);
        m_grid->setCellInteger(row, ColWidth, f_it->length);
        m_grid->setCellInteger(row, ColDec, f_it->scale);

        row++;
    }

    m_grid->refresh(kcl::Grid::refreshAll);
}


void RecDefPage::editFieldName(int field)
{
    m_grid->moveCursor(field, 0);
    m_grid->beginEdit(wxEmptyString);
}



void RecDefPage::onSize(wxSizeEvent& event)
{
    Layout();
}


void RecDefPage::onGridEditChange(kcl::GridEvent& event)
{
    int row = event.GetRow();
    int col = event.GetColumn();

    if (row >= m_rd->field_defs.size())
    {
        return;
    }

    if (col == ColFormat)
    {
        m_rd->field_defs[row].format = m_grid->getCellComboSel(row, col);
    }
     else if (col == ColType)
    {
        m_rd->field_defs[row].type = m_grid->getCellComboSel(row, col);
    }
}


void RecDefPage::onGridEndEdit(kcl::GridEvent& event)
{
    int row = event.GetRow();
    int col = event.GetColumn();

    if (row >= m_rd->field_defs.size())
    {
        event.Veto();
        return;
    }

    if (col == ColName)
    {
        m_rd->field_defs[row].name = event.GetString();
    }
     else if (col == ColOffset)
    {
        m_rd->field_defs[row].offset = event.GetInt();
    }
     else if (col == ColWidth)
    {
        m_rd->field_defs[row].length = event.GetInt();
    }
     else if (col == ColDec)
    {
        m_rd->field_defs[row].scale = event.GetInt();
    }
}

void RecDefPage::onTableNameChanged(wxCommandEvent& event)
{
    m_rd->name = event.GetString();
}

void RecDefPage::onRecIdOffsetChanged(wxCommandEvent& event)
{
    m_rd->id_offset = wxAtoi(event.GetString());
}

void RecDefPage::onRecIdByteChanged(wxCommandEvent& event)
{
    m_rd->id_value = wxAtoi(event.GetString());
}

void RecDefPage::onAppendField(wxCommandEvent& event)
{
    FieldDefinition f;
    f.offset = 0;
    f.length = 0;
    f.format = ddformatASCII;
    f.type = ddtypeCharacter;
    f.scale = 0;
    m_rd->field_defs.push_back(f);

    m_grid->insertRow(-1);
    int row = m_grid->getRowCount()-1;

    m_grid->setCellString(row, ColName, f.name);
    m_grid->setCellComboSel(row, ColFormat, f.format);
    m_grid->setCellComboSel(row, ColType, f.type);
    m_grid->setCellInteger(row, ColOffset, f.offset);
    m_grid->setCellInteger(row, ColWidth, f.length);
    m_grid->setCellInteger(row, ColDec, f.scale);

    m_grid->scrollToBottom();
    m_grid->moveCursor(row, ColName);

    m_grid->refresh(kcl::Grid::refreshAll);

    m_grid->beginEdit();
}

void RecDefPage::onInsertField(wxCommandEvent& event)
{

}

void RecDefPage::onDeleteField(wxCommandEvent& event)
{
    if (m_rd->field_defs.size() > 0)
    {
        int row = m_grid->getCursorRow();
        
        if (row < m_rd->field_defs.size())
        {
            m_rd->field_defs.erase(m_rd->field_defs.begin() + row);
            m_grid->deleteRow(row);
            m_grid->refresh(kcl::Grid::refreshAll);
        }
    }
}

void RecDefPage::onShiftDown(wxCommandEvent& event)
{
    int row = m_grid->getCursorRow();
    int field_count = m_rd->field_defs.size();

    for (int i = row; i < field_count; ++i)
    {
        m_rd->field_defs[i].offset++;
        m_grid->setCellInteger(i, ColOffset, m_rd->field_defs[i].offset);
    }

    m_grid->refresh(kcl::Grid::refreshAll);
}

void RecDefPage::onShiftUp(wxCommandEvent& event)
{
    int row = m_grid->getCursorRow();
    int field_count = m_rd->field_defs.size();

    for (int i = row; i < field_count; ++i)
    {
        m_rd->field_defs[i].offset--;
        m_grid->setCellInteger(i, ColOffset, m_rd->field_defs[i].offset);
    }

    m_grid->refresh(kcl::Grid::refreshAll);
}




// -- Data Format Panel class --

enum
{
    ID_AddRecDef = wxID_HIGHEST+1,
    ID_DeleteRecDef
};


BEGIN_EVENT_TABLE(DataFormatPanel, wxWindow)
    EVT_SIZE(DataFormatPanel::onSize)
END_EVENT_TABLE()


DataFormatPanel::DataFormatPanel()
{
    m_dfd = NULL;
    m_notebook = NULL;
}

DataFormatPanel::~DataFormatPanel()
{
}


// -- IDocument --
bool DataFormatPanel::initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd)
{
    if (!Create(docsite_wnd, -1, wxDefaultPosition, wxDefaultSize, wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
        return false;

    doc_site->setCaption(_("Data Format Editor"));

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    m_notebook = new kcl::NotebookControl(this);
    m_notebook->setScrollMode(kcl::notebookScrollAuto);


    wxPanel* extra_panel = new wxPanel(m_notebook, -1);
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);

    kcl::Button* add_button = new kcl::Button(extra_panel, ID_AddRecDef, wxDefaultPosition, wxDefaultSize,
                            _("Add"), GET_XPM(xpm_plus_small));
    kcl::Button* delete_button = new kcl::Button(extra_panel, ID_DeleteRecDef, wxDefaultPosition, wxDefaultSize,
                            _("Delete"), GET_XPM(xpm_delete_small));

    add_button->setDimensions(-1, 16);
    delete_button->setDimensions(-1, 16);

    button_sizer->Add(add_button);
    button_sizer->Add(delete_button);
    button_sizer->Add(1,1, 1, wxEXPAND);

    extra_panel->SetSizer(button_sizer);
    extra_panel->SetAutoLayout(true);

    cfw::CommandCapture* cc = new cfw::CommandCapture;
    extra_panel->PushEventHandler(cc);
    cc->sigCommandFired.connect(this, &DataFormatPanel::onExtraPanelCommand);

    m_notebook->setExtraPanel(extra_panel);


    main_sizer->Add(m_notebook, 1, wxEXPAND);

    SetSizer(main_sizer);
    Layout();

    refresh();

    return true;
}

wxWindow* DataFormatPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}



void DataFormatPanel::onExtraPanelCommand(int command_type, int command_id)
{
    if (command_id == ID_AddRecDef)
    {
        m_dfd->rec_defs.push_back(RecordDefinition());
        refresh();
        setActivePage(m_dfd->rec_defs.size()-1);
    }
     else if (command_id == ID_DeleteRecDef)
    {
        int old_page = m_notebook->getSelection();
        m_dfd->rec_defs.erase(m_dfd->rec_defs.begin() + m_notebook->getSelection());
        refresh();

        if (old_page-1 >= m_dfd->rec_defs.size())
        {
            setActivePage(old_page-1);
        }
    }
}



void DataFormatPanel::setDocumentFocus()
{
}

bool DataFormatPanel::onSiteClosing(bool force)
{
    return true;
}


void DataFormatPanel::setDataFormat(DataFormatDefinition* dfd)
{
    m_dfd = dfd;
    refresh();
}


void DataFormatPanel::refresh()
{
    if (m_notebook)
    {
        std::vector<RecordDefinition>::iterator r_it;

        if (m_notebook->getPageCount() == m_dfd->rec_defs.size())
        {
            int idx = 0;

            for (r_it = m_dfd->rec_defs.begin();
                 r_it != m_dfd->rec_defs.end();
                 ++r_it)
            {
                RecDefPage* page = (RecDefPage*)m_notebook->getPage(idx);
                idx++;
                page->setRecordDefinition(&(*r_it));
                page->populate();
            }
        }
         else
        {
            m_notebook->deleteAllPages();

            for (r_it = m_dfd->rec_defs.begin();
                 r_it != m_dfd->rec_defs.end();
                 ++r_it)
            {
                RecDefPage* page = new RecDefPage(m_notebook);
                page->setRecordDefinition(&(*r_it));
                page->populate();
                m_notebook->addPage(page, _("Record Definition"), wxNullBitmap, false);
            }
        }


        m_notebook->Refresh(TRUE);
    }
}


void DataFormatPanel::onSize(wxSizeEvent& event)
{
    Layout();
}

void DataFormatPanel::setActivePage(int page)
{
    if (m_notebook)
    {
        m_notebook->setSelection(page);
    }
}

void DataFormatPanel::editFieldName(int field)
{
    if (m_notebook)
    {
        RecDefPage* page = (RecDefPage*)m_notebook->getPage(m_notebook->getSelection());
        page->editFieldName(field);
    }
}

