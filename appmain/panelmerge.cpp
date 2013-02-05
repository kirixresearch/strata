/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-10-18
 *
 */


#include "appmain.h"
#include "panelmerge.h"
#include "appcontroller.h"
#include "tabledoc.h"
#include "dbdoc.h"
#include "dlgdatabasefile.h"


enum
{
    ID_OutputTableTextCtrl = wxID_HIGHEST + 1,
    ID_BrowseButton
};



BEGIN_EVENT_TABLE(MergePanel, wxPanel)
    EVT_BUTTON(wxID_OK, MergePanel::onOK)
    EVT_BUTTON(wxID_CANCEL, MergePanel::onCancel)
    EVT_BUTTON(ID_BrowseButton, MergePanel::onBrowse)
    EVT_SIZE(MergePanel::onSize)
END_EVENT_TABLE()


MergePanel::MergePanel()
{
    m_output_table = NULL;
    m_grid = NULL;
    m_ok_button = NULL;
    
    m_append = false;
    m_append_path = wxEmptyString;
    m_output_path = wxEmptyString;
}

MergePanel::~MergePanel()
{

}

// -- IDocument --
bool MergePanel::initDoc(IFramePtr frame,
                         IDocumentSitePtr doc_site,
                         wxWindow* docsite_wnd,
                         wxWindow* panesite_wnd)
{
    // -- create document's window --
    bool result = Create(docsite_wnd,
                         -1,
                         wxDefaultPosition,
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE);
    if (!result)
    {
        return false;
    }

    if (m_append)
    {
        doc_site->setCaption(_("Append"));
        m_output_path = m_append_path;
    }
     else
    {
        doc_site->setCaption(_("Merge"));
    }
    
    m_frame = frame;
    m_doc_site = doc_site;
    m_set = xcm::null;

    // create the grid
    m_grid = new kcl::RowSelectionGrid(this);
    m_grid->setAllowDeletes(true);
    m_grid->setOptionState(kcl::Grid::optColumnResize |
                           kcl::Grid::optGhostRow |
                           kcl::Grid::optInvalidAreaClickable |
                           kcl::Grid::optEdit, false);
    m_grid->setDragFormat(wxT("merge_tables"));
    m_grid->setGreenBarInterval(0);
    
    m_grid->createModelColumn(-1, _("Tables"), kcl::Grid::typeCharacter, 256, 0);
    m_grid->createDefaultView();
    m_grid->setColumnProportionalSize(0, 1);

    // create the output table sizer
    wxStaticText* label_output_table = new wxStaticText(this, -1, _("Output Table:"));
    wxStaticText* static_output_table = new wxStaticText(this, -1, m_output_path);
    wxString default_output_path = m_output_path.IsEmpty() ? wxT("/") : m_output_path;
    m_output_table = new wxTextCtrl(this, ID_OutputTableTextCtrl, default_output_path);
    wxButton* browse_button = new wxButton(this, ID_BrowseButton, _("Browse..."));

    wxBoxSizer* output_table_sizer = new wxBoxSizer(wxHORIZONTAL);
    output_table_sizer->AddSpacer(5);
    output_table_sizer->Add(label_output_table, 0, wxALIGN_CENTER);
    output_table_sizer->AddSpacer(5);
    output_table_sizer->Add(static_output_table, 1, wxALIGN_CENTER);
    output_table_sizer->Add(m_output_table, 1, wxALIGN_CENTER);
    output_table_sizer->Add(browse_button, 0, wxALIGN_CENTER | wxLEFT, 5);
    output_table_sizer->SetMinSize(100, 23);
    
    if (m_append)
    {
        output_table_sizer->Show(m_output_table, false);
        output_table_sizer->Show(browse_button, false);

        ITableDocPtr table_doc = TableDocMgr::getActiveTableDoc();
        if (table_doc.isNull())
            return false;

        m_set = table_doc->getBaseSet();

        // -- make panel caption --
        wxString caption = _("Append");
        if (m_set.isOk())
        {
            if (!m_set->isTemporary())
            {
                wxString name = towx(m_set->getObjectPath());
                name.AfterLast(wxT('/'));
                
                caption += wxT(" - [");
                caption += name;
                caption += wxT("]");
            }
        }

        m_doc_site->setCaption(caption);
    }
     else
    {
        output_table_sizer->Show(static_output_table, false);
    }
    
    // -- create a platform standards-compliant OK/Cancel sizer --

    m_ok_button = new wxButton(this, wxID_OK, _("Run"));
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(m_ok_button);
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // -- this code is necessary to get the sizer's bottom margin to 8 --
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->Add(output_table_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);
    if (!m_append)
        main_sizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);

    // only show the output table sizer when performing a merge operation
    if (m_append)
        main_sizer->Show(output_table_sizer, false, true);
    
    SetSizer(main_sizer);
    Layout();

    // -- set our drop targets --

    kcl::GridDataObjectComposite* drop_data;
    drop_data = new kcl::GridDataObjectComposite(NULL, wxT("merge_tables"));
    drop_data->Add(new FsDataObject);
    
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_grid);
    drop_target->SetDataObject(drop_data);
    drop_target->sigDropped.connect(this, &MergePanel::onGridDataDropped);
    m_grid->SetDropTarget(drop_target);

    FsDataDropTarget* output_drop_target = new FsDataDropTarget;
    output_drop_target->sigDragDrop.connect(this, &MergePanel::onOutputPathDropped);
    m_output_table->SetDropTarget(output_drop_target);

    // connect row selection grid signal
    m_grid->sigDeletedRows.connect(this, &MergePanel::onDeletedRows);
    
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
    
    return true;
}

wxWindow* MergePanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void MergePanel::setDocumentFocus()
{

}

void MergePanel::setAppend(const wxString& append_path)
{
    m_append_path = append_path;
    m_append = (m_append_path.IsEmpty() ? false : true);
}

void MergePanel::onBrowse(wxCommandEvent& evt)
{
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSaveSmall);
    dlg.setCaption(_("Select Output Table"));
    dlg.setAffirmativeButtonLabel(_("OK"));
    if (dlg.ShowModal() != wxID_OK)
        return;
        
    m_output_table->SetValue(dlg.getPath());
}

void MergePanel::checkOverlayText()
{
    // this isn't the best place for this, but it's convenient
    if (m_ok_button)
        m_ok_button->Enable(m_grid->getRowCount() > 0 ? true : false);
    
    m_grid->setOverlayText(wxEmptyString);
    
    if (m_grid->getRowCount() == 0)
    {
        if (m_append)
            m_grid->setOverlayText(_("Select tables from the Project Panel and\ndrag them here to append them to this table"));
             else
            m_grid->setOverlayText(_("Select tables from the Project Panel and\ndrag them here to add them to the output table"));
    }

    m_grid->refresh(kcl::Grid::refreshAll);
}

void MergePanel::onOutputPathDropped(wxDragResult& drag_result, FsDataObject* data)
{
    IFsItemEnumPtr items = data->getFsItems();
    std::vector<wxString> res;
    DbDoc::getFsItemPaths(items, res, true);

    if (res.size() > 1)
    {
        appMessageBox(_("You have selected either a folder or more than one table.  Please select only one table."),
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION | wxCENTER);
        return;
    }

    DbDoc* dbdoc = g_app->getDbDoc();
    IFsItemPtr item = items->getItem(0);

    if (dbdoc->isFsItemExternal(item))
    {
        appMessageBox(_("One or more of the items dragged from the project panel is an external table and cannot be specified as the output table."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    if (res.size() == 0)
        return;

    m_output_table->SetValue(res[0]);
    m_output_table->SetInsertionPointEnd();
    m_output_table->SetFocus();
}

void MergePanel::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    wxDataObject* obj = drop_target->GetDataObject();
    if (!obj)
        return;

    // determine which type of data object was dropped
    kcl::GridDataObjectComposite* drop_data = (kcl::GridDataObjectComposite*)obj;
    wxDataFormat fmt = drop_data->GetReceivedFormat();

    if (drop_data->isGridData())
    {
        if (fmt == wxDataFormat(kcl::getGridDataFormat(wxT("merge_tables"))))
        {
            drop_target->doRowDrag(true);
            return;
        }

        return;
    }

    // -- we're dragging tables from the project panel --
    

    // only accept tree data objects here
    if (fmt.GetId().CmpNoCase(FS_DATA_OBJECT_FORMAT) != 0)
        return;

    // get the row number where we dropped the data
    int drop_row = drop_target->getDropRow();
    
    // get the data out of the composite data object
    size_t len = drop_data->GetDataSize(fmt);
    unsigned char* data = new unsigned char[len];
    drop_data->GetDataHere(fmt, data);
    
    // -- copy the data from the wxDataObjectComposite to this new
    //    FsDataObject so we can use it's accessor functions --
    FsDataObject* fs_data_obj = new FsDataObject;
    fs_data_obj->SetData(fmt, len, data);


    IFsItemEnumPtr items = fs_data_obj->getFsItems();

    std::vector<wxString>::iterator it;
    std::vector<wxString> res;
    DbDoc::getFsItemPaths(items, res, true);

    DbDoc* dbdoc = g_app->getDbDoc();

    int i, count = items->size();
    for (i = 0; i < count; ++i)
    {
        if (dbdoc->isFsItemExternal(items->getItem(i)))
        {
            appMessageBox(_("One or more of the items dragged from the project panel is an external table and cannot be added to the output table."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            delete[] data;
            delete fs_data_obj;
            
            return;
        }
    }

    int insert_row = drop_row;
    
    for (it = res.begin(); it != res.end(); ++it)
    {
        m_grid->insertRow(insert_row);
        m_grid->setCellString(insert_row, 0, *it);
        insert_row++;
    }
    
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);

    delete[] data;
    delete fs_data_obj;
}

void MergePanel::onDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
}

void MergePanel::onSize(wxSizeEvent& evt)
{
    if (GetAutoLayout())
    {
        Layout();
    }
}


struct AppendInfo
{
    wxString output_path;
    tango::ISetPtr target_set;
};

static void onAppendJobFinished(jobs::IJobPtr job)
{
    g_app->getAppController()->refreshDbDoc();

    kl::JsonNode params;
    params.fromString(job->getParameters());

    if (params["mode"].getString() == L"append")
    {
        std::wstring output_path = params["output"];

        // iterate through document sites, and update tabledocs
        IDocumentSiteEnumPtr docsites;
        IDocumentSitePtr site;
        ITableDocPtr table_doc;

        docsites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);

        int site_count = docsites->size();
        for (int i = 0; i < site_count; ++i)
        {
            site = docsites->getItem(i);
            table_doc = site->getDocument();
            if (table_doc.isOk())
            {
                tango::ISetPtr base_set = table_doc->getBaseSet();
                if (base_set.isOk() && 0 == wcscasecmp(base_set->getObjectPath().c_str(), output_path.c_str()))
                {
                    table_doc->getGrid()->refresh(kcl::Grid::refreshAll);
                    table_doc->updateStatusBar();
                }
            }
        }
    }
     else if (params["mode"].getString() == L"overwrite")
    {
        // this is a merge job
        
        if (job->getJobInfo()->getState() != jobStateFinished)
            return;

        std::wstring output_path = params["output"];
        g_app->getAppController()->openSet(output_path);
    }
}

void MergePanel::onOK(wxCommandEvent& evt)
{
    if (m_grid->getRowCount() < 1)
    {
        appMessageBox(_("Please specify at least one input table to be added to the output table."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    wxString output_path = m_output_table->GetValue();
    if (!m_append)
    {
        // do an output path check only if we're merging tables
        if (!doOutputPathCheck(output_path))
            return;
    }
    
    tango::ISetPtr set;
    std::vector<tango::ISetPtr> set_ptrs;

    int col_idx = 0;
    int input_col_count = 0;
    int output_col_count = 0;

    int row = 0, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        wxString set_path = m_grid->getCellString(row, 0);
        set = g_app->getDatabase()->openSet(towstr(set_path));
        if (set.isNull())
        {
            wxString message = wxString::Format(_("'%s' could not be opened.  Please make sure this is a valid table."), set_path.c_str());

            appMessageBox(message,
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            
            return;
        }

        set_ptrs.push_back(set);
    }



    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.append-job");

    kl::JsonNode params;
    params["input"].setArray();

    if (m_append)
    {
        params["mode"].setString(L"append");
        params["output"] = m_set->getObjectPath(); // this is an append job
        job->getJobInfo()->setTitle(towstr(_("Append Records")));
    }
     else  
    {
        params["mode"].setString(L"overwrite");
        params["output"] = towstr(output_path);
        job->getJobInfo()->setTitle(towstr(_("Merge Tables")));
    }

    // add all of the sets we're going to append
    std::vector<tango::ISetPtr>::iterator set_it;
    for (set_it = set_ptrs.begin(); set_it != set_ptrs.end(); ++set_it)
    {
        kl::JsonNode input_element = params["input"].appendElement();
        input_element.setString((*set_it)->getObjectPath());
    }


    job->setParameters(params.toString());


    job->sigJobFinished().connect(&onAppendJobFinished);
    g_app->getJobQueue()->addJob(job, jobStateRunning);

    m_frame->closeSite(m_doc_site);
}


void MergePanel::onCancel(wxCommandEvent& evt)
{
    m_frame->closeSite(m_doc_site);
}

