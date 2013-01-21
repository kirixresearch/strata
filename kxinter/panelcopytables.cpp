/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-08-30
 *
 */


#include "kxinter.h"
#include "panelcopytables.h"
#include "appcontroller.h"
#include "treecontroller.h"
#include "treemodel.h"
#include "jobconvert.h"


// -- control ids --

enum
{
    ID_AddOne = wxID_HIGHEST+1,
    ID_RemoveOne,
    ID_AddAll,
    ID_RemoveAll,
    ID_SourceDatabaseCombo,
    ID_TargetDatabaseCombo
};


// -- option ids --

enum
{
    ID_OverwriteYes = 31675,
    ID_OverwriteAll,
    ID_OverwriteNo
};




class DlgOverwriteConfirm : public wxDialog
{

private:
    bool m_cancelled;

public:

    DlgOverwriteConfirm(wxWindow* parent, const wxString& filename, bool multiple_files, bool is_file = true) : wxDialog(parent, -1, _("Overwrite Table(s)"),
                                  wxDefaultPosition, wxSize(370, 140),
                                  wxDEFAULT_DIALOG_STYLE |
                                  wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN | wxCENTER)
    {
        m_cancelled = false;

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        wxStaticBitmap* bitmap = new wxStaticBitmap(this, -1, GET_XPM(xpm_overwritefile_big));
        wxStaticText* text = new wxStaticText(this, -1, wxT(""));
        
        wxString overwrite_text;
        overwrite_text = wxString::Format(_("'%s' already exists.  Continuing with this operation will overwrite this table.  Do you wish to continue?"),
                                    filename.c_str());

        text->SetLabel(overwrite_text);
        resizeStaticText(text, 300);
        
        if (!is_file)
        {
            SetTitle(_("Overwrite Table(s)"));
        }

        wxButton* overwrite_yes_button = new wxButton(this, ID_OverwriteYes, _("Yes"));
        wxButton* overwrite_all_button;
        wxButton* overwrite_no_button = new wxButton(this, ID_OverwriteNo, _("No"));
        wxButton* cancel_button;

        wxBoxSizer* text_sizer = new wxBoxSizer(wxHORIZONTAL);
        text_sizer->Add(bitmap, 0, wxALIGN_CENTER | wxRIGHT, 10);
        text_sizer->Add(text, 0, wxEXPAND);

        wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
        button_sizer->Add(1,1, 1);
        button_sizer->Add(overwrite_yes_button, 0);
        button_sizer->Add(5,5, 0);

        if (multiple_files)
        {
            overwrite_all_button = new wxButton(this, ID_OverwriteAll, _("Yes to All"));
            button_sizer->Add(overwrite_all_button, 0);
            button_sizer->Add(5,5, 0);
        }

        button_sizer->Add(overwrite_no_button, 0);
        button_sizer->Add(5,5, 0);

        if (multiple_files)
        {
            cancel_button = new wxButton(this, wxID_CANCEL, _("Cancel"));
            button_sizer->Add(cancel_button, 0);
            button_sizer->Add(5,5, 0);
        }

        sizer->Add(7,7, 0, wxEXPAND);
        sizer->Add(text_sizer, 0, wxEXPAND | wxALL, 7);
        sizer->Add(1,1, 1, wxEXPAND);
        sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 7);

        CenterOnParent();
        
        SetSizer(sizer);
        Layout();
    }

    ~DlgOverwriteConfirm()
    {
    }

    void onYes(wxCommandEvent& event)
    {
        EndModal(ID_OverwriteYes);
    }

    void onYesToAll(wxCommandEvent& event)
    {
        EndModal(ID_OverwriteAll);
    }

    void onNo(wxCommandEvent& event)
    {
        EndModal(ID_OverwriteNo);
    }

    void onCancel(wxCommandEvent& event)
    {
        m_cancelled = true;
        EndModal(wxID_CANCEL);
    }

    bool getCancelled()
    {
        return m_cancelled;
    }

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(DlgOverwriteConfirm, wxDialog)
    EVT_BUTTON(ID_OverwriteYes, DlgOverwriteConfirm::onYes)
    EVT_BUTTON(ID_OverwriteAll, DlgOverwriteConfirm::onYesToAll)
    EVT_BUTTON(ID_OverwriteNo, DlgOverwriteConfirm::onNo)
    EVT_BUTTON(wxID_CANCEL, DlgOverwriteConfirm::onCancel)
END_EVENT_TABLE()




BEGIN_EVENT_TABLE(CopyTablesPanel, wxWindow)
    EVT_BUTTON(ID_AddOne, CopyTablesPanel::onAddOne)
    EVT_BUTTON(ID_RemoveOne, CopyTablesPanel::onRemoveOne)
    EVT_BUTTON(ID_AddAll, CopyTablesPanel::onAddAll)
    EVT_BUTTON(ID_RemoveAll, CopyTablesPanel::onRemoveAll)
    EVT_BUTTON(wxID_OK, CopyTablesPanel::onOk)
    EVT_BUTTON(wxID_CANCEL, CopyTablesPanel::onCancel)
    EVT_COMBOBOX(ID_SourceDatabaseCombo, CopyTablesPanel::onSourceDatabaseChanged)
    EVT_SIZE(CopyTablesPanel::onSize)
END_EVENT_TABLE()



CopyTablesPanel::CopyTablesPanel()
{

}

CopyTablesPanel::~CopyTablesPanel()
{

}

bool CopyTablesPanel::initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd)
{
    // -- create document's window --
    bool result = Create(docsite_wnd, -1, wxDefaultPosition, docsite_wnd->GetClientSize(), wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN);
    if (!result)
        return false;

    doc_site->setCaption(_("Copy Tables"));
    wxSize min_site_size = doc_site->getSiteWindow()->GetSize();
    doc_site->setMinSize(min_site_size.x, min_site_size.y);
    m_doc_site = doc_site;



    m_source_combo = new wxComboBox(this, ID_SourceDatabaseCombo, wxT(""), wxDefaultPosition, wxDefaultSize,
                                    0, NULL, wxCB_READONLY);
    m_target_combo = new wxComboBox(this, ID_TargetDatabaseCombo, wxT(""), wxDefaultPosition, wxDefaultSize,
                                    0, NULL, wxCB_READONLY);

    m_source_grid = new kcl::Grid;
    m_source_grid->setCursorType(kcl::Grid::cursorRowHighlight);
    m_source_grid->setBorderType(wxSUNKEN_BORDER);
    m_source_grid->setOptionState(kcl::Grid::optHorzGridLines | kcl::Grid::optVertGridLines |
                           kcl::Grid::optColumnResize | kcl::Grid::optColumnMove |
                           kcl::Grid::optEdit, false);
    m_source_grid->createModelColumn(-1, _("Source Tables"), kcl::Grid::typeCharacter, 256, 0);
    m_source_grid->setRowLabelSize(0);
    m_source_grid->createDefaultView();
    m_source_grid->setColumnProportionalSize(0, 1);
    m_source_grid->Create(this, -1);

    m_target_grid = new kcl::Grid;
    m_target_grid->setCursorType(kcl::Grid::cursorRowHighlight);
    m_target_grid->setBorderType(wxSUNKEN_BORDER);
    m_target_grid->setOptionState(kcl::Grid::optHorzGridLines | kcl::Grid::optVertGridLines |
                           kcl::Grid::optColumnResize | kcl::Grid::optColumnMove, false);
    m_target_grid->createModelColumn(-1, _("Destination Tables"), kcl::Grid::typeCharacter, 256, 0);
    m_target_grid->setRowLabelSize(0);
    m_target_grid->createDefaultView();
    m_target_grid->setColumnProportionalSize(0, 1);
    m_target_grid->Create(this, -1);

    cfw::ITreeItemPtr root = g_app->getTreeController()->getTreePanel()->getRootItem();
    cfw::ITreeItemEnumPtr children = root->getChildren();
    int i;
    int count = children->size();
    for (i = 0; i < count; ++i)
    {
        cfw::ITreeItemPtr item = children->getItem(i);

        IConnectionPtr c;
        IDatabaseFolderTreeItemPtr folder = item;
        if (folder)
        {
            c = folder->getConnection();
        }

        cfw::IDirectoryTreeItemPtr dir = item;
        if (dir)
        {
            c = g_app->getConnectionMgr()->getConnection(i);
        }

        m_source_combo->Append(item->getLabel(), (void*)c.p);
        m_target_combo->Append(item->getLabel(), (void*)c.p);
    }
    m_source_combo->SetSelection(0);
    m_target_combo->SetSelection(0);

    wxBoxSizer* source_sizer = new wxBoxSizer(wxVERTICAL);
    source_sizer->Add(m_source_combo, 0, wxEXPAND);
    source_sizer->Add(5,5, 0);
    source_sizer->Add(m_source_grid, 1, wxEXPAND);

    wxBoxSizer* target_sizer = new wxBoxSizer(wxVERTICAL);
    target_sizer->Add(m_target_combo, 0, wxEXPAND);
    target_sizer->Add(5,5, 0);
    target_sizer->Add(m_target_grid, 1, wxEXPAND);



    kcl::Button* rightarrow_button = new kcl::Button(this, ID_AddOne);
    rightarrow_button->setBitmap(GET_XPM(xpm_rightarrow));
    rightarrow_button->makeMinSize();
    rightarrow_button->setTooltip(_("Add Table"));

    kcl::Button* leftarrow_button = new kcl::Button(this, ID_RemoveOne);
    leftarrow_button->setBitmap(GET_XPM(xpm_leftarrow));
    leftarrow_button->makeMinSize();
    leftarrow_button->setTooltip(_("Remove Table"));

    kcl::Button* rightdblarrow_button = new kcl::Button(this, ID_AddAll);
    rightdblarrow_button->setBitmap(GET_XPM(xpm_rightdblarrow));
    rightdblarrow_button->makeMinSize();
    rightdblarrow_button->setTooltip(_("Add All Tables"));

    kcl::Button* leftdblarrow_button = new kcl::Button(this, ID_RemoveAll);
    leftdblarrow_button->setBitmap(GET_XPM(xpm_leftdblarrow));
    leftdblarrow_button->makeMinSize();
    leftdblarrow_button->setTooltip(_("Remove All Tables"));

    wxBoxSizer* arrow_button_sizer = new wxBoxSizer(wxVERTICAL);
    arrow_button_sizer->Add(1,1, 1, wxEXPAND, 0);
    arrow_button_sizer->Add(rightarrow_button, 0, wxEXPAND, 0);
    arrow_button_sizer->Add(leftarrow_button, 0, wxEXPAND, 0);
    arrow_button_sizer->Add(rightdblarrow_button, 0, wxEXPAND, 0);
    arrow_button_sizer->Add(leftdblarrow_button, 0, wxEXPAND, 0);
    arrow_button_sizer->Add(1,1, 1, wxEXPAND, 0);



    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer->Add(source_sizer, 1, wxEXPAND | wxRIGHT, 5);
    horz_sizer->Add(arrow_button_sizer, 0, wxEXPAND | wxRIGHT, 5);
    horz_sizer->Add(target_sizer, 1, wxEXPAND);



    kcl::Button* cancel_button = new kcl::Button(this, wxID_CANCEL, wxDefaultPosition, wxDefaultSize,
                            _("Cancel"), GET_XPM(xpm_cancel));

    kcl::Button* ok_button = new kcl::Button(this, wxID_OK, wxDefaultPosition, wxDefaultSize,
                            _("OK"), GET_XPM(xpm_execute));

    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->Add(1,1, 1, wxEXPAND);
    button_sizer->Add(cancel_button, 0, wxEXPAND);
    button_sizer->Add(5,5, 0);
    button_sizer->Add(ok_button, 0, wxEXPAND);


    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(horz_sizer, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 5);
    main_sizer->Add(5,5, 0);
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 5);
    SetSizer(main_sizer);
    Layout();

    populate();

    return true;
}

wxWindow* CopyTablesPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void CopyTablesPanel::setDocumentFocus()
{

}

void CopyTablesPanel::onSize(wxSizeEvent& event)
{
    Layout();
}

void CopyTablesPanel::onSourceDatabaseChanged(wxCommandEvent& event)
{
    m_to_convert.clear();
    populate();
}

void CopyTablesPanel::onAddOne(wxCommandEvent& event)
{
    wxString label = m_source_grid->getCellString(m_source_grid->getCursorRow(), 0);

    StringPair sp;
    sp.first = label;
    sp.second = label;
    m_to_convert.push_back(sp);

    populate();
}

void CopyTablesPanel::onRemoveOne(wxCommandEvent& event)
{
    int row_count = m_target_grid->getRowCount();

    if (row_count > 0)
    {
        int row = m_target_grid->getCursorRow();
        m_to_convert.erase(m_to_convert.begin() + row);
        populate();

        row_count--;
        if (row < row_count)
        {
            m_target_grid->moveCursor(row, 0);
        }
         else
        {
            m_target_grid->moveCursor(row_count-1, 0);
        }
    }
}

void CopyTablesPanel::onAddAll(wxCommandEvent& event)
{
    wxString label;
    StringPair sp;
    
    int i;
    int count = m_source_grid->getRowCount();

    for (i = 0; i < count; ++i)
    {
        label = m_source_grid->getCellString(i, 0);
        sp.first = label;
        sp.second = label;
        m_to_convert.push_back(sp);
    }

    populate();
}

void CopyTablesPanel::onRemoveAll(wxCommandEvent& event)
{
    m_to_convert.clear();
    populate();
}

void CopyTablesPanel::onOk(wxCommandEvent& event)
{
    wxString src_path;
    wxString dest_path;

    IConnectionPtr c;

    c = (IConnection*)m_source_combo->GetClientData(m_source_combo->GetSelection());
    tango::IDatabasePtr src_db = c->getDatabasePtr();

    c = (IConnection*)m_target_combo->GetClientData(m_target_combo->GetSelection());
    tango::IDatabasePtr dest_db = c->getDatabasePtr();

    std::vector<StringPair>::iterator it;
    for (it = m_to_convert.begin(); it != m_to_convert.end(); ++it)
    {
        src_path = wxT("/") + (*it).first;
        dest_path = wxT("/") + (*it).second;

        ConvertJob* job = new ConvertJob;

        job->setInstructions(src_db, src_path,
                             dest_db, dest_path,
                             xcm::null, xcm::null);

        g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
        g_app->getAppController()->showJobManager();
    }

    m_doc_site->closeSite();
}

void CopyTablesPanel::onCancel(wxCommandEvent& event)
{
    m_doc_site->closeSite();
}

void CopyTablesPanel::populate()
{
    m_source_grid->deleteAllRows();
    m_target_grid->deleteAllRows();

    m_source_tables.clear();

    int i;
    int count;
    IConnectionPtr c;
    tango::IDatabasePtr db;
    tango::IFileInfoEnumPtr files;

    // -- populate the source tables grid --
    c = (IConnection*)m_source_combo->GetClientData(m_source_combo->GetSelection());
    db = c->getDatabasePtr();
    files = db->getFolderInfo(L"/");
    if (files.isOk())
    {
        count = files->size();

        for (i = 0; i < count; ++i)
        {
            tango::IFileInfoPtr info = files->getItem(i);

            if (info->getType() == tango::filetypeSet)
            {
                m_source_grid->insertRow(-1);
                m_source_grid->setCellString(i, 0, cfw::makeProper(towx(info->getName())));
                m_source_grid->setCellBitmap(i, 0, GET_XPM(xpm_table), kcl::Grid::alignLeft);
            }
        }
    }

    // -- populate the target tables grid with tables to convert --
    i = 0;
    std::vector<StringPair>:: iterator it;
    for (it = m_to_convert.begin(); it != m_to_convert.end(); ++it)
    {
        m_target_grid->insertRow(-1);
        m_target_grid->setCellString(i, 0, cfw::makeProper((*it).second));
        m_target_grid->setCellBitmap(i, 0, GET_XPM(xpm_table), kcl::Grid::alignLeft);

        i++;
    }

    if (m_to_convert.size() == 0)
    {
        m_target_grid->setOverlayText(_("No tables are currently\nselected for conversion."));
    }
     else
    {
        m_target_grid->setOverlayText(wxT(""));
    }


    m_source_grid->refresh(kcl::Grid::refreshAll);
    m_target_grid->refresh(kcl::Grid::refreshAll);
}







