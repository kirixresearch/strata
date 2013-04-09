/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-04
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "panelfileinfo.h"
#include "tabledoc.h"
#include "jsonconfig.h"
#include <algorithm>


enum
{
    ID_Save = 20000
};

static bool sortByNameA(const ProjectFileInfo& l, const ProjectFileInfo& r)
{
    return l.name < r.name;
}

static bool sortByNameD(const ProjectFileInfo& l, const ProjectFileInfo& r)
{
    return l.name > r.name;
}

static bool sortByTypeA(const ProjectFileInfo& l, const ProjectFileInfo& r)
{
    return l.type < r.type;
}

static bool sortByTypeD(const ProjectFileInfo& l, const ProjectFileInfo& r)
{
    return l.type > r.type;
}

static bool sortBySizeA(const ProjectFileInfo& l, const ProjectFileInfo& r)
{
    return l.size < r.size;
}

static bool sortBySizeD(const ProjectFileInfo& l, const ProjectFileInfo& r)
{
    return l.size > r.size;
}

static bool sortByRecordsA(const ProjectFileInfo& l, const ProjectFileInfo& r)
{
    return l.records < r.records;
}

static bool sortByRecordsD(const ProjectFileInfo& l, const ProjectFileInfo& r)
{
    return l.records > r.records;
}

// function to determine the project info type for a given item
// (only needs to be used when the file info's type is tango::filetypeNode)
int determineProjectInfoType(const wxString& path)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    tango::IFileInfoPtr file = db->getFileInfo(towstr(path));
    if (file.isNull())
        return ProjectFileInfo::typeOther;
    
    int file_type = file->getType();
    
    if (file_type == tango::filetypeFolder)
        return ProjectFileInfo::typeFolder;
        
    if (file_type == tango::filetypeSet)
        return ProjectFileInfo::typeTable;
        
    if (file_type == tango::filetypeStream)
        return ProjectFileInfo::typeScript;
        
    if (file_type == tango::filetypeNode)
    {
        kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), towstr(path));
        if (node.isOk())
        {
            kl::JsonNode root_node = node["root"];
            if (root_node.isOk())
            {
                kl::JsonNode kpp_report_node = root_node["kpp_report"];
                if (kpp_report_node.isOk())
                    return ProjectFileInfo::typeReport;

                kl::JsonNode kpp_template_node = root_node["kpp_template"];
                if (kpp_template_node.isOk())
                {
                    kl::JsonNode kpp_template_type = kpp_template_node["type"];
                    if (kpp_template_type.isOk() && kpp_template_type.getString() == L"query")
                        return ProjectFileInfo::typeQuery;
                }
            }
        }
    }

    return ProjectFileInfo::typeOther;
}




BEGIN_EVENT_TABLE(MultiFileInfoPanel, wxPanel)
    EVT_BUTTON(wxID_OK, MultiFileInfoPanel::onSave)
    EVT_KCLGRID_COLUMN_LEFT_CLICK(MultiFileInfoPanel::onGridColumnLeftClick)
END_EVENT_TABLE()


MultiFileInfoPanel::MultiFileInfoPanel()
{
    m_sort_column = -1;
    m_sort_ascending = true;
}

MultiFileInfoPanel::~MultiFileInfoPanel()
{
}

void MultiFileInfoPanel::addFile(const wxString& path)
{
    wxString temps = path;
    temps.MakeUpper();
    if (m_inserted_items.find(temps) != m_inserted_items.end())
    {
        // we already have that item
        return;
    }

    m_inserted_items.insert(temps);

    // can't get file info; bail out
    tango::IDatabasePtr db = g_app->getDatabase();
    tango::IFileInfoPtr finfo = db->getFileInfo(towstr(path));
    if (finfo.isNull())
        return;
        
    switch (finfo->getType())
    {
        case tango::filetypeFolder:
        {
            // create the project file info
            ProjectFileInfo f;
            f.name = path;
            f.type = ProjectFileInfo::typeFolder;
            f.size = -2;
            f.records = -2;
            
            if (!finfo->isMount())
            {
                // native project folder; traverse the folder
                
                tango::IFileInfoEnumPtr files;
                files = db->getFolderInfo(towstr(path));

                int i, count = files->size();
                for (i = 0; i < count; ++i)
                {
                    wxString new_path = path;
                    if (new_path.Length() == 0 || new_path.Last() != wxT('/'))
                        new_path += wxT("/");
                    new_path += files->getItem(i)->getName();

                    addFile(new_path);
                }
            }
            
            // add the project file info to the vector
            m_info.push_back(f);
        }
        break;

        case tango::filetypeSet:
        {
            // get the record count
            long long rec_count = -1;
            if (finfo->getFlags() & tango::sfFastRowCount)
                rec_count = finfo->getRowCount();

            // create the project file info
            ProjectFileInfo f;
            f.name = path;
            f.type = ProjectFileInfo::typeTable;
            f.size = finfo->getSize();
            f.records = rec_count;

            // add the project file info to the vector
            m_info.push_back(f);
        }
        break;
        
        case tango::filetypeStream:
        {
            // create the project file info
            ProjectFileInfo f;
            f.name = path;
            f.type = ProjectFileInfo::typeScript;
            f.size = -2;
            f.records = -2;

            // add the project file info to the vector
            m_info.push_back(f);
        }
        break;
        
        case tango::filetypeNode:
        {
            // create the project file info
            ProjectFileInfo f;
            f.name = path;
            f.type = determineProjectInfoType(path);
            f.size = -2;
            f.records = -2;

            // add the project file info to the vector
            m_info.push_back(f);
        }
        break;
    }
}


// -- IDocument --
bool MultiFileInfoPanel::initDoc(IFramePtr frame,
                                 IDocumentSitePtr site,
                                 wxWindow* doc_site,
                                 wxWindow* pane_site)
{
    if (!Create(doc_site,
                -1,
                wxPoint(0,0),
                doc_site->GetClientSize(),
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
    {
        return false;
    }

    site->setCaption(_("File Properties"));
    m_doc_site = site;


    // create top sizer

    wxStaticBitmap* folder_bitmap = new wxStaticBitmap(this, -1, GETBMP(gf_folder_open_24));
    wxStaticBitmap* table_bitmap = new wxStaticBitmap(this, -1, GETBMP(gf_table_24));
    wxStaticBitmap* report_bitmap = new wxStaticBitmap(this, -1, GETBMP(gf_report_24));
    wxStaticBitmap* query_bitmap = new wxStaticBitmap(this, -1, GETBMP(gf_query_24));
    wxStaticBitmap* script_bitmap = new wxStaticBitmap(this, -1, GETBMP(gf_script_24));
    m_folder_text = new wxStaticText(this, -1, wxEmptyString);
    m_table_text = new wxStaticText(this, -1, wxEmptyString);
    m_report_text = new wxStaticText(this, -1, wxEmptyString);
    m_query_text = new wxStaticText(this, -1, wxEmptyString);
    m_script_text = new wxStaticText(this, -1, wxEmptyString);
    
    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_sizer->AddSpacer(5);
    top_sizer->Add(folder_bitmap, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(5);
    top_sizer->Add(m_folder_text, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(20);
    top_sizer->Add(table_bitmap, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(5);
    top_sizer->Add(m_table_text, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(20);
    top_sizer->Add(report_bitmap, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(5);
    top_sizer->Add(m_report_text, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(20);
    top_sizer->Add(query_bitmap, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(5);
    top_sizer->Add(m_query_text, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(20);
    top_sizer->Add(script_bitmap, 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(5);
    top_sizer->Add(m_script_text, 0, wxALIGN_CENTER);

    // -- create grid --

    m_grid = new kcl::Grid;
    m_grid->setOptionState(kcl::Grid::optHorzGridLines |
                           kcl::Grid::optVertGridLines |
                           kcl::Grid::optColumnMove |
                           kcl::Grid::optColumnResize |
                           kcl::Grid::optEdit |
                           kcl::Grid::optSelect |
                           kcl::Grid::optActivateHyperlinks, false);
    m_grid->setOptionState(kcl::Grid::optContinuousScrolling, true);
    m_grid->setCursorVisible(false);
    m_grid->setBorderType(kcl::DEFAULT_BORDER);
    m_grid->createModelColumn(-1, _("File Name"), kcl::Grid::typeCharacter, 256, 0);
    m_grid->createModelColumn(-1, _("Type"), kcl::Grid::typeCharacter, 256, 0);
    m_grid->createModelColumn(-1, _("Size"), kcl::Grid::typeCharacter, 256, 0);
    m_grid->createModelColumn(-1, _("Records"), kcl::Grid::typeCharacter, 28, 0);
    
    kcl::CellProperties align_right_props;
    align_right_props.mask = kcl::CellProperties::cpmaskAlignment;
    align_right_props.alignment = kcl::Grid::alignRight;

    m_grid->setModelColumnProperties(2, &align_right_props);
    m_grid->setModelColumnProperties(3, &align_right_props);

    m_grid->setRowLabelSize(0);
    m_grid->createDefaultView();
    m_grid->setColumnProportionalSize(0, 7);
    m_grid->setColumnProportionalSize(1, 2);
    m_grid->setColumnProportionalSize(2, 2);
    m_grid->setColumnProportionalSize(3, 2);
    m_grid->Create(this, -1);

    // -- create a platform standards-compliant OK/Cancel sizer --
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_OK, _("Save")));
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL, _("Close")));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // -- this code is necessary to get the sizer's bottom margin to 8 --
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    // create main sizer
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(main_sizer);
    Layout();
    
    std::sort(m_info.begin(), m_info.end(), sortByNameA);
    m_grid->setColumnBitmap(0,
                            GETBMP(xpm_columnsortup),
                            kcl::Grid::alignRight);
    m_sort_ascending = true;
    m_sort_column = 0;
    populate();
    
    return true;
}

wxWindow* MultiFileInfoPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void MultiFileInfoPanel::setDocumentFocus()
{
}

void MultiFileInfoPanel::populate()
{
    m_grid->deleteAllRows();

    int folder_count = 0;
    int table_count = 0;
    int report_count = 0;
    int query_count = 0;
    int script_count = 0;
    int other_count = 0;
    double total_records = 0.0;
    double total_bytes = 0.0;

    int row = 0;

    std::vector<ProjectFileInfo>::iterator it;
    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        m_grid->insertRow(-1);
        
        switch (it->type)
        {
            case ProjectFileInfo::typeFolder:
                folder_count++;
                m_grid->setCellBitmap(row, 0, GETBMP(gf_folder_open_16), kcl::Grid::alignLeft);
                m_grid->setCellString(row, 1, _("Folder"));
                break;
            
            case ProjectFileInfo::typeTable:
                table_count++;
                m_grid->setCellBitmap(row, 0, GETBMP(gf_table_16), kcl::Grid::alignLeft);
                m_grid->setCellString(row, 1, _("Table"));
                break;
            
            case ProjectFileInfo::typeReport:
                report_count++;
                m_grid->setCellBitmap(row, 0, GETBMP(gf_report_16), kcl::Grid::alignLeft);
                m_grid->setCellString(row, 1, _("Report"));
                break;
            
            case ProjectFileInfo::typeQuery:
                query_count++;
                m_grid->setCellBitmap(row, 0, GETBMP(gf_query_16), kcl::Grid::alignLeft);
                m_grid->setCellString(row, 1, _("Query"));
                break;
            
            case ProjectFileInfo::typeScript:
                script_count++;
                m_grid->setCellBitmap(row, 0, GETBMP(gf_script_16), kcl::Grid::alignLeft);
                m_grid->setCellString(row, 1, _("Script"));
                break;
            
            default:
                other_count++;
                m_grid->setCellBitmap(row, 0, GETBMP(xpm_blank_16), kcl::Grid::alignLeft);
                m_grid->setCellString(row, 1, _("Other"));
                break;
        }
        
        // set the name of the project item in the grid
        m_grid->setCellString(row, 0, it->name);
        
        // set the size of the project item in the grid
        if (it->size == -2)
            m_grid->setCellString(row, 2, wxEmptyString);
             else if (it->size == -1)
            m_grid->setCellString(row, 2, _("(Unknown)"));
             else
            m_grid->setCellString(row, 2, kl::formattedNumber(it->size/1024.0) + wxT(" KB"));
        
        // set the record count of the project item in the grid
        if (it->records == -2)
            m_grid->setCellString(row, 3, wxEmptyString);
             else if (it->records == -1)
            m_grid->setCellString(row, 3, _("(Unknown)"));
             else
            m_grid->setCellString(row, 3, kl::formattedNumber(it->records));
        
        row++;
    }
    
    // refresh the grid
    m_grid->refresh(kcl::Grid::refreshAll);
    
    // set folders label
    if (folder_count == 1)
        m_folder_text->SetLabel(wxString::Format(_("%d Folder"), folder_count));
         else
        m_folder_text->SetLabel(wxString::Format(_("%d Folders"), folder_count));
    
    // set tables label
    if (table_count == 1)
        m_table_text->SetLabel(wxString::Format(_("%d Table"), table_count));
         else
        m_table_text->SetLabel(wxString::Format(_("%d Tables"), table_count));
    
    // set reports label
    if (report_count == 1)
        m_report_text->SetLabel(wxString::Format(_("%d Report"), report_count));
         else
        m_report_text->SetLabel(wxString::Format(_("%d Reports"), report_count));
    
    // set query label
    if (query_count == 1)
        m_query_text->SetLabel(wxString::Format(_("%d Query"), query_count));
         else
        m_query_text->SetLabel(wxString::Format(_("%d Queries"), query_count));
    
    // set scripts label
    if (script_count == 1)
        m_script_text->SetLabel(wxString::Format(_("%d Script"), script_count));
         else
        m_script_text->SetLabel(wxString::Format(_("%d Scripts"), script_count));
    
    Layout();
}


void MultiFileInfoPanel::onOK(wxCommandEvent& event)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void MultiFileInfoPanel::onSave(wxCommandEvent& event)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    tango::IStructurePtr output_structure;
    tango::IColumnInfoPtr colinfo;
    output_structure = db->createStructure();

    colinfo = output_structure->createColumn();
    colinfo->setName(L"Filename");
    colinfo->setType(tango::typeWideCharacter);
    colinfo->setWidth(255);
    colinfo->setScale(0);

    colinfo = output_structure->createColumn();
    colinfo->setName(L"Type");
    colinfo->setType(tango::typeWideCharacter);
    colinfo->setWidth(30);
    colinfo->setScale(0);

    colinfo = output_structure->createColumn();
    colinfo->setName(L"Size");
    colinfo->setType(tango::typeDouble);
    colinfo->setWidth(8);
    colinfo->setScale(0);

    colinfo = output_structure->createColumn();
    colinfo->setName(L"Records");
    colinfo->setType(tango::typeDouble);
    colinfo->setWidth(8);
    colinfo->setScale(0);

    std::wstring output_path = L"xtmp_" + kl::getUniqueString();

    if (!db->createTable(output_path, output_structure, NULL))
        return;

    tango::IRowInserterPtr output_inserter = db->bulkInsert(output_path);
    if (!output_inserter)
        return;

    output_inserter->startInsert(L"*");

    tango::objhandle_t filename_handle = output_inserter->getHandle(L"filename");
    tango::objhandle_t type_handle = output_inserter->getHandle(L"type");
    tango::objhandle_t size_handle = output_inserter->getHandle(L"size");
    tango::objhandle_t records_handle = output_inserter->getHandle(L"records");

    std::vector<ProjectFileInfo>::iterator it;
    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        output_inserter->putWideString(filename_handle, towstr(it->name));

        switch (it->type)
        {
            case ProjectFileInfo::typeFolder:
                output_inserter->putWideString(type_handle, towstr(_("Folder")));
                break;

            case ProjectFileInfo::typeTable:
                output_inserter->putWideString(type_handle, towstr(_("Table")));
                break;

            case ProjectFileInfo::typeReport:
                output_inserter->putWideString(type_handle, towstr(_("Report")));
                break;

            case ProjectFileInfo::typeQuery:
                output_inserter->putWideString(type_handle, towstr(_("Query")));
                break;

            case ProjectFileInfo::typeScript:
                output_inserter->putWideString(type_handle, towstr(_("Script")));
                break;

            default:
                output_inserter->putWideString(type_handle, towstr(_("Other")));
                break;
        }
        
        if (it->size >= 0)
            output_inserter->putDouble(size_handle, it->size);
             else
            output_inserter->putDouble(size_handle, 0.0);
        
        if (it->records >= 0)
            output_inserter->putDouble(records_handle, it->records);
             else
            output_inserter->putDouble(records_handle, 0.0);
        
        output_inserter->insertRow();
    }

    output_inserter->finishInsert();
    
    g_app->getAppController()->openTable(output_path);

    g_app->getMainFrame()->closeSite(m_doc_site);
}

void MultiFileInfoPanel::onGridColumnLeftClick(kcl::GridEvent& event)
{
    int col = event.GetColumn();
    
    if (m_sort_column == col)
        m_sort_ascending = !m_sort_ascending;
         else
        m_sort_ascending = true;
    
    m_sort_column = col;
    
    if (m_sort_ascending)
    {
        if (col == 0)
            std::sort(m_info.begin(), m_info.end(), sortByNameA);
             else if (col == 1)
            std::sort(m_info.begin(), m_info.end(), sortByTypeA);
             else if (col == 2)
            std::sort(m_info.begin(), m_info.end(), sortBySizeA);
             else if (col == 3)
            std::sort(m_info.begin(), m_info.end(), sortByRecordsA);
        
        for (int i = 0; i < 4; ++i)
        {
            wxBitmap bmp;
            if (i == col)
                bmp = GETBMP(xpm_columnsortup);
                 else
                bmp = wxNullBitmap;
            
            m_grid->setColumnBitmap(i, bmp, kcl::Grid::alignRight);
        }
    }
     else
    {
        if (col == 0)
            std::sort(m_info.begin(), m_info.end(), sortByNameD);
             else if (col == 1)
            std::sort(m_info.begin(), m_info.end(), sortByTypeD);
             else if (col == 2)
            std::sort(m_info.begin(), m_info.end(), sortBySizeD);
             else if (col == 3)
            std::sort(m_info.begin(), m_info.end(), sortByRecordsD);
        
        for (int i = 0; i < 4; ++i)
        {
            wxBitmap bmp;
            if (i == col)
                bmp = GETBMP(xpm_columnsortdown);
                 else
                bmp = wxNullBitmap;
            
            m_grid->setColumnBitmap(i, bmp, kcl::Grid::alignRight);
        }
    }
    
    populate();
}



