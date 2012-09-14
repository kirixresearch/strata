/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-03-01
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "connectionwizard.h"
#include "exportpages.h"
#include "exportwizard.h"
#include "jobexport.h"
#include "jobexportpkg.h"
#include "structurevalidator.h"
#include "dbdoc.h"


enum
{
    // table selection page
    ID_BasePathTextCtrl,
    ID_BrowseButton,
    ID_ExportTablesList
};

// tableselection grid column indexes

enum
{
    SOURCE_TABLENAME_IDX = 0,
    DEST_TABLENAME_IDX = 1,
    APPEND_IDX = 2
};

// utility functions

inline wxString createFullFilePath(const wxString& base_path,
                                   const wxString& tablename)
{
    wxString retval = base_path;
    
    if (retval.Last() != PATH_SEPARATOR_CHAR &&
        !tablename.StartsWith(PATH_SEPARATOR_STR))
    {
        retval += PATH_SEPARATOR_CHAR;
    }

    retval += tablename;
    return retval;
}

static std::vector<RowErrorChecker> getRowErrorCheckerVector(kcl::Grid* grid,
                                                             int export_type,
                                                             const wxString& base_path)
{
    std::vector<RowErrorChecker> vec;
    
    int row, row_count = grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        wxString tablename = grid->getCellString(row, DEST_TABLENAME_IDX);

        if (export_type == dbtypeXbase ||
            export_type == dbtypeDelimitedText)
        {
            tablename = createFullFilePath(base_path, tablename);
        }

        vec.push_back(RowErrorChecker(row, tablename));
    }

    return vec;
}




// ExportTemplate class implementation

ExportTemplate::ExportTemplate()
{

}

bool ExportTemplate::load(const wxString& path)
{
    // create an ofs file with the run information
    tango::IDatabasePtr db = g_app->getDatabase();

    tango::INodeValuePtr file = db->openNodeFile(towstr(path));
    if (file.isNull())
        return false;

    tango::INodeValuePtr kpp_template = file->getChild(L"kpp_template", false);
    if (!kpp_template)
        return false;

    tango::INodeValuePtr template_type = kpp_template->getChild(L"type", false);
    if (!template_type)
        return false;

    if (template_type->getString() != L"export")
        return false;

    tango::INodeValuePtr data_root = kpp_template->getChild(L"data", false);
    if (!data_root)
        return false;


    // type
    tango::INodeValuePtr type_val = data_root->getChild(L"type", false);
    if (!type_val)
        return false;
    m_ei.type = type_val->getInteger();
    m_ei.last_type = type_val->getInteger();

    // description
    tango::INodeValuePtr desc_val = data_root->getChild(L"description", false);
    if (!desc_val)
        return false;
    m_ei.description = towx(desc_val->getString());

    // path
    tango::INodeValuePtr path_val = data_root->getChild(L"path", false);
    if (!path_val)
        return false;
    m_ei.path = towx(path_val->getString());

    // version (for package files)
    tango::INodeValuePtr kpg_version_val = data_root->getChild(L"kpg_version", false);
    if (!kpg_version_val)
        return false;
    m_ei.kpg_version = kpg_version_val->getInteger();

    // compressed (for package files)
    tango::INodeValuePtr kpg_compressed_val = data_root->getChild(L"kpg_compressed", true);
    if (!kpg_compressed_val)
        return false;
    m_ei.kpg_compressed = kpg_compressed_val->getBoolean();

    // filter
    tango::INodeValuePtr filter_val = data_root->getChild(L"filter", false);
    if (!filter_val)
        return false;
    m_ei.filter = towx(filter_val->getString());

    // server
    tango::INodeValuePtr server_val = data_root->getChild(L"server", false);
    if (!server_val)
        return false;
    m_ei.server = towx(server_val->getString());

    // port
    tango::INodeValuePtr port_val = data_root->getChild(L"port", false);
    if (!port_val)
        return false;
    m_ei.port = port_val->getInteger();
    
    // database
    tango::INodeValuePtr database_val = data_root->getChild(L"database", false);
    if (!database_val)
        return false;
    m_ei.database = towx(database_val->getString());

    // username
    tango::INodeValuePtr username_val = data_root->getChild(L"username", false);
    if (!username_val)
        return false;
    m_ei.username = towx(username_val->getString());

    // password
    tango::INodeValuePtr password_val = data_root->getChild(L"password", false);
    if (!password_val)
        return false;
    m_ei.password = towx(password_val->getString());

    // save password
    tango::INodeValuePtr save_password_val = data_root->getChild(L"save_password", false);
    if (!save_password_val)
        return false;
    m_ei.save_password = save_password_val->getBoolean();

    // base path (for table selection page)
    tango::INodeValuePtr basepath_val = data_root->getChild(L"base_path", false);
    if (!basepath_val)
        return false;
    m_ei.base_path = towx(basepath_val->getString());

    // tables base
    tango::INodeValuePtr tables_base = data_root->getChild(L"tables", false);
    if (!tables_base)
        return false;

    int table_count = tables_base->getChildCount();

    // try to load the tables that were saved

    int table_counter;
    for (table_counter = 0; table_counter < table_count; ++table_counter)
    {
        tango::INodeValuePtr table_node = tables_base->getChildByIdx(table_counter);

        // input tablename
        tango::INodeValuePtr input_tn = table_node->getChild(L"input_tablename", false);
        if (!input_tn)
            return false;

        // output tablename
        tango::INodeValuePtr output_tn = table_node->getChild(L"output_tablename", false);
        if (!output_tn)
            return false;

        // append
        tango::INodeValuePtr append = table_node->getChild(L"append", false);
        if (!append)
            return false;


        ExportTableSelection ts;
        ts.input_tablename = towx(input_tn->getString());
        ts.output_tablename = towx(output_tn->getString());
        ts.append = append->getBoolean();

        m_ei.tables.push_back(ts);
    }

    return true;
}

bool ExportTemplate::save(const wxString& path)
{
    // create an ofs file with the run information
    tango::IDatabasePtr db = g_app->getDatabase();

    wxString file_path = path;
    if (db->getFileExist(towstr(file_path)))
    {
        db->deleteFile(towstr(file_path));
    }

    // set up the template root
    tango::INodeValuePtr file = db->createNodeFile(towstr(file_path));

    tango::INodeValuePtr kpp_template = file->createChild(L"kpp_template");
    
    tango::INodeValuePtr template_type = kpp_template->createChild(L"type");
    template_type->setString(L"export");

    tango::INodeValuePtr template_version = kpp_template->createChild(L"version");
    template_version->setString(L"1");

    tango::INodeValuePtr data_root = kpp_template->createChild(L"data");


    // type
    tango::INodeValuePtr type_val = data_root->createChild(L"type");
    type_val->setInteger(m_ei.type);

    // description
    tango::INodeValuePtr desc_val = data_root->createChild(L"description");
    desc_val->setString(towstr(m_ei.description));

    // path
    tango::INodeValuePtr path_val = data_root->createChild(L"path");
    path_val->setString(towstr(m_ei.path));

    // version (for package files)
    tango::INodeValuePtr kpg_version_val = data_root->createChild(L"kpg_version");
    kpg_version_val->setInteger(m_ei.kpg_version);

    // compressed (for package files)
    tango::INodeValuePtr kpg_compressed_val = data_root->createChild(L"kpg_compressed");
    kpg_compressed_val->setBoolean(m_ei.kpg_compressed);

    // filter
    tango::INodeValuePtr filter_val = data_root->createChild(L"filter");
    filter_val->setString(towstr(m_ei.filter));

    // server
    tango::INodeValuePtr server_val = data_root->createChild(L"server");
    server_val->setString(towstr(m_ei.server));

    // port
    tango::INodeValuePtr port_val = data_root->createChild(L"port");
    port_val->setInteger(m_ei.port);
    
    // database
    tango::INodeValuePtr database_val = data_root->createChild(L"database");
    database_val->setString(towstr(m_ei.database));

    // username
    tango::INodeValuePtr username_val = data_root->createChild(L"username");
    username_val->setString(towstr(m_ei.username));

    // password
    tango::INodeValuePtr password_val = data_root->createChild(L"password");
    if (m_ei.save_password)
    {
        password_val->setString(towstr(m_ei.password));
    }
     else
    {
        password_val->setString(L"");
    }

    // save password
    tango::INodeValuePtr save_password_val = data_root->createChild(L"save_password");
    save_password_val->setBoolean(m_ei.save_password);

    // base path (for table selection page
    tango::INodeValuePtr basepath_val = data_root->createChild(L"base_path");
    basepath_val->setString(towstr(m_ei.base_path));

    // write out table info
    tango::INodeValuePtr tables_base = data_root->createChild(L"tables");

    int table_counter = 0;
    std::vector<ExportTableSelection>::iterator table_it;
    for (table_it = m_ei.tables.begin(); table_it != m_ei.tables.end(); ++table_it)
    {
        wchar_t table_buf[255];
        swprintf(table_buf, 255, L"table%d", table_counter++);

        tango::INodeValuePtr table_node = tables_base->createChild(table_buf);

        // input tablename
        tango::INodeValuePtr input_tn = table_node->createChild(L"input_tablename");
        input_tn->setString(towstr(table_it->input_tablename));

        // output tablename
        tango::INodeValuePtr output_tn = table_node->createChild(L"output_tablename");
        output_tn->setString(towstr(table_it->output_tablename));

        // append
        tango::INodeValuePtr append = table_node->createChild(L"append");
        append->setBoolean(table_it->append);
    }

    return true;
}

cfw::IJobPtr ExportTemplate::execute()
{
    // check for package file export
    if (m_ei.type == dbtypePackage)
    {
        ExportPkgJob* job = new ExportPkgJob;

        if (m_ei.overwrite_file)
        {
            job->setPkgFilename(m_ei.path,
                                ExportPkgJob::modeOverwrite,
                                m_ei.kpg_version);
        }
         else
        {
            job->setPkgFilename(m_ei.path, ExportPkgJob::modeAppend);
        }

        std::vector<ExportTableSelection>::iterator it;
        for (it = m_ei.tables.begin(); it != m_ei.tables.end(); ++it)
        {
            job->addExportObject(it->output_tablename,
                                 it->input_tablename,
                                 m_ei.kpg_compressed);
        }

        g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);

        return static_cast<cfw::IJob*>(job);
    }



    ExportJob* job = new ExportJob;
    job->setExportType(m_ei.type);
    job->setFilename(m_ei.path, m_ei.overwrite_file);
    job->setFixInvalidFieldnames(m_ei.fix_invalid_fieldnames);
    job->setConnectionInfo(m_ei.server,
                           m_ei.port,
                           m_ei.database,
                           m_ei.username,
                           m_ei.password);

    job->setDelimiters(m_ei.delimiters);
    job->setTextQualifier(m_ei.text_qualifier);
    job->setFirstRowHeader(m_ei.first_row_header);


    ExportJobInfo job_export_info;
    std::vector<ExportTableSelection>::iterator it;

    for (it = m_ei.tables.begin(); it != m_ei.tables.end(); ++it)
    {
        job_export_info.input_path = it->input_tablename;
        job_export_info.output_path = it->output_tablename;
        job_export_info.append = it->append;

        job->addExportSet(job_export_info);
    }

    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);

    return static_cast<cfw::IJob*>(job);
}



BEGIN_EVENT_TABLE(ExportTableSelectionPage, kcl::WizardPage)
    EVT_KCLGRID_END_EDIT(ExportTableSelectionPage::onGridEndEdit)
    EVT_KCLGRID_NEED_TOOLTIP_TEXT(ExportTableSelectionPage::onGridNeedTooltipText)
END_EVENT_TABLE()

ExportTableSelectionPage::ExportTableSelectionPage(kcl::Wizard* parent,
                                                   ExportInfo* ei) :
                                                     kcl::WizardPage(parent),
                                                     m_ei(ei)
{
    SetSize(parent->GetClientSize());

    m_message = new wxStaticText(this,
                                 -1,
                                 _("Add tables by dragging them from the project panel to the export list."));
    cfw::resizeStaticText(m_message);

    m_label_exportpath = new wxStaticText(this, -1, _("Export to"));
    
    m_path_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_path_sizer->Add(m_label_exportpath, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    
    m_grid = new kcl::RowSelectionGrid(this, ID_ExportTablesList);
    m_grid->setDragFormat(wxT("exporttables_list"));
    m_grid->setAllowDeletes(true);
    m_grid->setGreenBarInterval(0);

    m_grid->createModelColumn(SOURCE_TABLENAME_IDX, _("Source Name"), kcl::Grid::typeCharacter, 512, 0);
    m_grid->createModelColumn(DEST_TABLENAME_IDX, _("Destination Name"), kcl::Grid::typeCharacter, 512, 0);
    m_grid->createModelColumn(APPEND_IDX, _("Append"), kcl::Grid::typeBoolean, 1, 0);

    // set cell properties for the grid
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskAlignment;
    cellprops.alignment = kcl::Grid::alignCenter;
    m_grid->setModelColumnProperties(APPEND_IDX, &cellprops);

    cellprops.mask = kcl::CellProperties::cpmaskEditable;
    cellprops.editable = false;
    m_grid->setModelColumnProperties(SOURCE_TABLENAME_IDX, &cellprops);

    m_grid->setRowLabelSize(0);
    m_grid->createDefaultView();
    m_grid->setColumnSize(SOURCE_TABLENAME_IDX, 120);
    m_grid->setColumnSize(DEST_TABLENAME_IDX, 120);
    m_grid->setColumnSize(APPEND_IDX, 50);

    // set our drop targets

    kcl::GridDataObjectComposite* drop_data;
    drop_data = new kcl::GridDataObjectComposite(NULL, wxT("exporttables_list"));
    drop_data->Add(new cfw::FsDataObject);

    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_grid);
    drop_target->SetDataObject(drop_data);
    drop_target->sigDropped.connect(this, &ExportTableSelectionPage::onGridDataDropped);
    m_grid->SetDropTarget(drop_target);

    // create main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(20);
    m_main_sizer->Add(m_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_main_sizer->AddSpacer(4);
    m_main_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                      0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_main_sizer->AddSpacer(12);
    m_main_sizer->Add(m_path_sizer, 0, wxEXPAND | wxBOTTOM, 5);
    m_main_sizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_main_sizer->AddSpacer(20);

    SetSizer(m_main_sizer);
    Layout();

    // connect signals
    m_grid->sigDeletedRows.connect(this, &ExportTableSelectionPage::onDeletedRows);
    
    // set last type default to undefined
    m_last_type = dbtypeUndefined;
}


// wizard event handlers

void ExportTableSelectionPage::onPageChanged()
{
    if (m_ei->type == m_last_type)
        return;

    // handle grid layout
    
    m_grid->hideAllColumns();
    
    // get the grid's size for column size calculations
    int w, h, col_w;
    m_grid->GetClientSize(&w, &h);
    
    m_grid->insertColumn(SOURCE_TABLENAME_IDX, SOURCE_TABLENAME_IDX);
    m_grid->insertColumn(DEST_TABLENAME_IDX, DEST_TABLENAME_IDX);

    if (m_ei->type == dbtypePackage)
    {
        // resize columns to fit grid area
        col_w = w/2;
        m_grid->setColumnSize(SOURCE_TABLENAME_IDX, col_w);
        col_w = w-col_w;
        m_grid->setColumnSize(DEST_TABLENAME_IDX, col_w);
    }
     else
    {
        // if we're not importing from a package file, 
        // show the append column
        m_grid->insertColumn(APPEND_IDX, APPEND_IDX);

        // resize columns to fit grid area
        m_grid->setColumnSize(APPEND_IDX, 50);
        col_w = (w-50)/2;
        m_grid->setColumnSize(SOURCE_TABLENAME_IDX, col_w);
        col_w = w-50-col_w;
        m_grid->setColumnSize(DEST_TABLENAME_IDX, col_w);
    }
    
    
    if (m_ei->type == dbtypeMySql ||
        m_ei->type == dbtypeSqlServer ||
        m_ei->type == dbtypeOracle ||
        m_ei->type == dbtypeDb2 ||
        m_ei->type == dbtypeOdbc)
    {
        m_main_sizer->Show(m_path_sizer, false, true);
    }
     else if (m_ei->type == dbtypePackage ||
              m_ei->type == dbtypeAccess ||
              m_ei->type == dbtypeExcel)
    {
        m_main_sizer->Show(m_path_sizer, true, true);
    }
     else
    {
        m_main_sizer->Show(m_path_sizer, true, true);
    }

    Layout();
    
    // we've switch export types so delete all rows in the grid
    
    m_grid->deleteAllRows();
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
    
    // set the last type for comparison purposes if we go back and forth
    m_last_type = m_ei->type;
}

bool ExportTableSelectionPage::onPageChanging(bool forward)
{
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    if (!forward)
        return true;
    
    // CHECK: check for duplicate table names in the list
    bool block = false;
    int errorcode = checkDuplicateTablenames(false);
    StructureValidator::showErrorMessage(errorcode, &block);
    
    // there is an error in the structure that must be fixed
    if (block)
        return false;

    // CHECK: check for invalid table names in the list
    /*
    errorcode = checkInvalidTablenames(false);
    StructureValidator::showErrorMessage(errorcode, &block);
    
    // there is an error in the structure that must be fixed
    if (block)
        return false;
    */

    // populate the tables vector and attempt to begin the export
    
    bool append_column_visible =
            (m_grid->getColumnViewIdx(APPEND_IDX)) != -1 ? true : false;
    
    m_ei->tables.clear();
    
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        ExportTableSelection ets;
        ets.input_tablename = m_grid->getCellString(row, SOURCE_TABLENAME_IDX);
        ets.output_tablename = m_grid->getCellString(row, DEST_TABLENAME_IDX);
        
        if (append_column_visible)
            ets.append = m_grid->getCellBoolean(row, APPEND_IDX);
         else
            ets.append = false;
            
        m_ei->tables.push_back(ets);
    }
    
    return true;
}

void ExportTableSelectionPage::onDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
}

void ExportTableSelectionPage::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    wxDataObject* obj = drop_target->GetDataObject();
    if (!obj)
        return;

    // determine which type of data object was dropped
    kcl::GridDataObjectComposite* drop_data = (kcl::GridDataObjectComposite*)obj;
    wxDataFormat fmt = drop_data->GetReceivedFormat();

    if (drop_data->isGridData())
    {
        drop_target->doRowDrag(false);
        checkOverlayText();
        m_grid->refresh(kcl::Grid::refreshAll);
        return;
    }

    // we're dragging from the tree
    
    
    // only accept tree data objects here
    if (fmt.GetId().CmpNoCase(cfw::FS_DATA_OBJECT_FORMAT) != 0)
        return;

    // get the row number where we dropped the data
    int drop_row = drop_target->getDropRow();
    
    // get the data out of the composite data object
    size_t len = drop_data->GetDataSize(fmt);
    unsigned char* data = new unsigned char[len];
    drop_data->GetDataHere(fmt, data);
    
    // copy the data from the wxDataObjectComposite to this new
    // cfw::FsDataObject so we can use it's accessor functions
    cfw::FsDataObject* fs_data_obj = new cfw::FsDataObject;
    fs_data_obj->SetData(fmt, len, data);


    cfw::IFsItemEnumPtr items = fs_data_obj->getFsItems();

    bool dragging_only_folders = true;      // determine below if we are dragging only folders
    wxString base_path = wxT("");
    
    std::vector<wxString>::iterator it;
    std::vector<wxString> res;
    DbDoc::getFsItemPaths(items, res, true);

    DbDoc* dbdoc = g_app->getDbDoc();

    // do error checking
    
    int i, count = items->size();
    for (i = 0; i < count; ++i)
    {
        cfw::IFsItemPtr item = items->getItem(0);

        if (dbdoc->isFsItemExternal(item))
        {
            cfw::appMessageBox(_("One or more of the items dragged from the project panel is an external table.  Only native database\ntables can be dragged from the project panel."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
        
        // get the object_type
        int obj_type = -1;
        IDbObjectFsItemPtr obj = item;
        if (obj.isOk())
            obj_type = obj->getType();
                
        if (m_ei->type != dbtypePackage)
        {
            if (obj_type != dbobjtypeSet)
            {
                cfw::appMessageBox(_("One or more of the items dragged from the project panel is not a table.  If you would like to export objects other than tables,\nselect 'File' as your export type and export as 'Package File'."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }
        }
        
        if (obj_type == dbobjtypeFolder && base_path.Length() == 0)
        {
            base_path = dbdoc->getFsItemPath(item);
        }
        
        if (obj_type != dbobjtypeFolder)
        {
            dragging_only_folders = false;
        }
    }


    // when mounts are dragged in, they are not treated as dragging a normal
    // folder, which usually recursively adds the folder's contents.  Rather
    // the mount reference itself is added to the package file
    if (items->size() == 1 && DbDoc::isItemMount(items->getItem(0)))
        dragging_only_folders = false;
    

    // add tables to the grid
    
    for (it = res.begin(); it != res.end(); ++it)
    {
        wxString output, ext;

        switch (m_ei->type)
        {
            case dbtypeXbase:           ext = wxT(".dbf");  break;
            case dbtypeDelimitedText:   ext = wxT(".csv");  break;
            case dbtypeFixedLengthText: ext = wxT(".txt");  break;
        }

        if (m_ei->type == dbtypePackage)
        {
            if (dragging_only_folders)
            {
                output = *it;
                
                if (items->size() == 1)
                    output.Remove(0, base_path.Length());
                
                // remove preceeding slash, if present
                if (output.Length() > 0 && output.GetChar(0) == '/')
                    output = output.Remove(0, 1);
            }
             else
            {
                output = it->AfterLast(wxT('/'));
            }
        }
         else
        {
            output = it->AfterLast(wxT('/'));
            output += ext;
        }
        
        m_grid->insertRow(drop_row);
        m_grid->setCellString(drop_row, SOURCE_TABLENAME_IDX, *it);
        m_grid->setCellString(drop_row, DEST_TABLENAME_IDX, output);
        m_grid->setCellBitmap(drop_row, DEST_TABLENAME_IDX, GETBMP(xpm_blank_16));
        m_grid->setCellBoolean(drop_row, APPEND_IDX, false);
        drop_row++;
    }
    
    clearProblemRows();
    checkDuplicateTablenames();
    //checkInvalidTablenames();
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void ExportTableSelectionPage::onGridNeedTooltipText(kcl::GridEvent& evt)
{
    int row_count = m_grid->getRowCount();
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col != DEST_TABLENAME_IDX)
        return;
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, m_ei->type, m_ei->base_path);
    StructureValidator::findDuplicateObjectNames(check_rows);
    // NOTE: findInvalidObjectNames() needs a database as a second parameter
    //       so we know which database to check for invalid object name
    //StructureValidator::findInvalidObjectNames(check_rows, db);
    
    std::vector<RowErrorChecker>::iterator it;
    for (it = check_rows.begin(); it != check_rows.end(); ++it)
    {
        if (it->row == row && it->errors != StructureValidator::ErrorNone)
        {
            wxString msg;
            
            if (it->errors & StructureValidator::ErrorDuplicateTableNames &&
                it->errors & StructureValidator::ErrorInvalidTableNames)
            {
                msg = _("This table has the same name as another table\nin this list and has an invalid name");
            }
             else
            {
                if (it->errors & StructureValidator::ErrorDuplicateTableNames)
                    msg = _("This table has the same name as another table in this list");
                 else if (it->errors & StructureValidator::ErrorInvalidTableNames)
                    msg = _("This table has an invalid name");
            }
            
            evt.SetString(msg);
        }
    }
}

void ExportTableSelectionPage::onGridEndEdit(kcl::GridEvent& evt)
{
    if (evt.GetEditCancelled())
        return;
    
    if (evt.GetColumn() == DEST_TABLENAME_IDX)
    {
        // this will force the cell's text to be the text that we just
        // entered (this is necessary because we use getCellString()
        // in the checkDuplicateFieldnames() function below and the
        // cell's text has not yet changed)
        m_grid->setCellString(evt.GetRow(), DEST_TABLENAME_IDX, evt.GetString());
        
        clearProblemRows();
        checkDuplicateTablenames();
        //checkInvalidTablenames();
        m_grid->refreshColumn(kcl::Grid::refreshAll, DEST_TABLENAME_IDX);
    }
}

void ExportTableSelectionPage::checkOverlayText()
{
    // clear the overlay text from the grid
    m_grid->setOverlayText(wxEmptyString);

    // if there are no rows in the grid, show the overlay text
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(_("Drag items here from the Project Panel\nto add them to the export list"));
}

void ExportTableSelectionPage::markProblemRow(int row, bool scroll_to)
{
    m_grid->setCellBitmap(row, DEST_TABLENAME_IDX, GETBMP(gf_exclamation_16));

    if (scroll_to)
    {
        m_grid->moveCursor(row, DEST_TABLENAME_IDX, false);
        if (!m_grid->isCursorVisible())
        {
            m_grid->scrollVertToCursor();
        }
    }
}

int ExportTableSelectionPage::checkDuplicateTablenames(bool mark_rows)
{
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, m_ei->type, m_ei->base_path);

    bool errors_found = StructureValidator::findDuplicateObjectNames(check_rows);
    if (errors_found && mark_rows)
    {
        std::vector<RowErrorChecker>::iterator it;
        for (it = check_rows.begin(); it != check_rows.end(); ++it)
        {
            if (it->errors != StructureValidator::ErrorNone)
                markProblemRow(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorDuplicateTableNames
                         : StructureValidator::ErrorNone);
}

int ExportTableSelectionPage::checkInvalidTablenames(bool mark_rows)
{
    return StructureValidator::ErrorNone;
    
/*
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, m_ei->type, m_ei->base_path);

    // NOTE: findInvalidObjectNames() needs a database as a second parameter
    //       so we know which database to check for invalid object name
    
    bool errors_found = StructureValidator::findInvalidObjectNames(check_rows, db);
    if (errors_found && mark_rows)
    {
        std::vector<RowErrorChecker>::iterator it;
        for (it = check_rows.begin(); it != check_rows.end(); ++it)
        {
            if (it->errors != StructureValidator::ErrorNone)
                markProblemRow(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorInvalidTableNames
                         : StructureValidator::ErrorNone);
*/
}

void ExportTableSelectionPage::clearProblemRows()
{
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
        m_grid->setCellBitmap(row, DEST_TABLENAME_IDX, GETBMP(xpm_blank_16));
}

void ExportTableSelectionPage::setExportPathLabel(const wxString& label)
{
    m_label_exportpath->SetLabel(label);
}

void ExportTableSelectionPage::refreshGrid()
{
    m_grid->refresh(kcl::Grid::refreshAll);
}


// DelimitedTextSettingsPage class implementation


BEGIN_EVENT_TABLE(ExportDelimitedTextSettingsPage, kcl::WizardPage)
END_EVENT_TABLE()


ExportDelimitedTextSettingsPage::ExportDelimitedTextSettingsPage(kcl::Wizard* parent,
                                                                 ExportInfo* ei)
                                : kcl::WizardPage(parent), m_ei(ei)
{
    SetSize(parent->GetClientSize());

    wxStaticBox* delimiter_staticbox = new wxStaticBox(this,
                                                       -1,
                                                       _("Field Delimiters"));

    wxStaticBox* textqualifier_staticbox = new wxStaticBox(this,
                                                       -1,
                                                       _("Text Qualifier"));
    
    wxStaticText* text = new wxStaticText(this,
                                          -1,
                                          _("Modify the settings below to determine how the text-delimited files should be written."));
    cfw::resizeStaticText(text);


    // create main delimiters sizer
    
    m_comma_radio = new wxRadioButton(this,
                                      -1,
                                      _("Comma"),
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      wxRB_GROUP);

    m_tab_radio = new wxRadioButton(this, -1, _("Tab "));
    m_semicolon_radio = new wxRadioButton(this, -1, _("Semicolon"));
    m_space_radio = new wxRadioButton(this, -1, _("Space"));
    m_nodelimiters_radio = new wxRadioButton(this, -1, _("None"));
    m_otherdelimiters_radio = new wxRadioButton(this, -1, _("Other:"));
    m_otherdelimiters_text = new wxTextCtrl(this,
                                            -1,
                                            wxEmptyString,
                                            wxDefaultPosition,
                                            wxSize(50, 21));
    m_otherdelimiters_text->SetMaxLength(5);

    wxBoxSizer* other_delimiters_sizer = new wxBoxSizer(wxHORIZONTAL);
    other_delimiters_sizer->Add(m_otherdelimiters_radio, 0, wxALIGN_CENTER);
    other_delimiters_sizer->AddSpacer(5);
    other_delimiters_sizer->Add(m_otherdelimiters_text, 0, wxEXPAND);

    wxStaticBoxSizer* left_options_sizer = new wxStaticBoxSizer(delimiter_staticbox, wxVERTICAL);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_comma_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_tab_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_semicolon_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_space_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_nodelimiters_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(6);
    left_options_sizer->Add(other_delimiters_sizer, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);


    // create main text qualifier sizer
    
    m_doublequote_radio = new wxRadioButton(this,
                                            -1,
                                            _("Quotation Marks (\")"),
                                            wxDefaultPosition,
                                            wxDefaultSize,
                                            wxRB_GROUP);

    m_singlequote_radio = new wxRadioButton(this, -1, _("Single Quote (')"));
    m_notextqualifier_radio = new wxRadioButton(this, -1, _("None"));
    m_othertextqualifier_radio = new wxRadioButton(this, -1, _("Other:"));
    m_othertextqualifier_text = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxSize(50, 21));
    m_othertextqualifier_text->SetMaxLength(1);

    wxBoxSizer* other_textqualifier_sizer = new wxBoxSizer(wxHORIZONTAL);
    other_textqualifier_sizer->Add(m_othertextqualifier_radio, 0, wxALIGN_CENTER);
    other_textqualifier_sizer->AddSpacer(5);
    other_textqualifier_sizer->Add(m_othertextqualifier_text, 0, wxEXPAND);

    wxStaticBoxSizer* right_options_sizer = new wxStaticBoxSizer(textqualifier_staticbox, wxVERTICAL);
    right_options_sizer->AddSpacer(10);
    right_options_sizer->Add(m_doublequote_radio, 0, wxEXPAND | wxLEFT, 10);
    right_options_sizer->AddSpacer(10);
    right_options_sizer->Add(m_singlequote_radio, 0, wxEXPAND | wxLEFT, 10);
    right_options_sizer->AddSpacer(10);
    right_options_sizer->Add(m_notextqualifier_radio, 0, wxEXPAND | wxLEFT, 10);
    right_options_sizer->AddSpacer(6);
    right_options_sizer->Add(other_textqualifier_sizer, 0, wxEXPAND | wxLEFT, 10);
    right_options_sizer->AddSpacer(10);


    // create horizontal settings sizer
    wxBoxSizer* horz_sizer1 = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer1->Add(left_options_sizer, 1, wxEXPAND);
    horz_sizer1->AddSpacer(20);
    horz_sizer1->Add(right_options_sizer, 1, wxEXPAND);

    // create first row header sizer
    m_firstrowheader_check = new wxCheckBox(this, -1, _("First row contains field names"));
    m_firstrowheader_check->SetValue(true);

    wxBoxSizer* horz_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer2->Add(12,1);
    horz_sizer2->Add(m_firstrowheader_check, 1, wxALIGN_CENTER);

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(20);
    main_sizer->Add(text, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(4);
    main_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                    0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(12);
    main_sizer->Add(horz_sizer1, 0, wxEXPAND | wxLEFT | wxRIGHT, 40);
    main_sizer->AddSpacer(10);
    main_sizer->Add(horz_sizer2, 0, wxEXPAND | wxLEFT | wxRIGHT, 40);
    main_sizer->AddStretchSpacer(1);

    SetSizer(main_sizer);
    Layout();
}

void ExportDelimitedTextSettingsPage::onPageChanged()
{

}

bool ExportDelimitedTextSettingsPage::onPageChanging(bool forward)
{
    savePageData();
    return true;
}

void ExportDelimitedTextSettingsPage::loadPageData()
{
    if (m_ei->delimiters == wxT(","))
    {
        m_comma_radio->SetValue(true);
    }
     else if (m_ei->delimiters == wxT("\t"))
    {
        m_tab_radio->SetValue(true);
    }
     else if (m_ei->delimiters == wxT(";"))
    {
        m_semicolon_radio->SetValue(true);
    }
     else if (m_ei->delimiters == wxT(" "))
    {
        m_space_radio->SetValue(true);
    }
     else if (m_ei->delimiters == wxT(""))
    {
        m_nodelimiters_radio->SetValue(true);
    }
     else
    {
        m_otherdelimiters_radio->SetValue(true);
        m_otherdelimiters_text->SetValue(m_ei->delimiters);
    }

    if (m_ei->text_qualifier == wxT("\""))
    {
        m_doublequote_radio->SetValue(true);
    }
     else if (m_ei->text_qualifier == wxT("'"))
    {
        m_singlequote_radio->SetValue(true);
    }
     else if (m_ei->text_qualifier == wxT(""))
    {
        m_notextqualifier_radio->SetValue(true);
    }
     else
    {
        m_othertextqualifier_radio->SetValue(true);
        m_othertextqualifier_text->SetValue(m_ei->text_qualifier);
    }

    m_firstrowheader_check->SetValue(m_ei->first_row_header);
}

void ExportDelimitedTextSettingsPage::savePageData()
{
    if (m_comma_radio->GetValue())
    {
        m_ei->delimiters = wxT(",");
    }
     else if (m_tab_radio->GetValue())
    {
        m_ei->delimiters = wxT("\t");
    }
     else if (m_semicolon_radio->GetValue())
    {
        m_ei->delimiters = wxT(";");
    }
     else if (m_space_radio->GetValue())
    {
        m_ei->delimiters = wxT(" ");
    }
     else if (m_nodelimiters_radio->GetValue())
    {
        m_ei->delimiters = wxT("");
    }
     else
    {
        m_ei->delimiters = m_otherdelimiters_text->GetValue();
    }

    if (m_doublequote_radio->GetValue())
    {
        m_ei->text_qualifier = wxT("\"");
    }
     else if (m_singlequote_radio->GetValue())
    {
        m_ei->text_qualifier = wxT("'");
    }
     else if (m_notextqualifier_radio->GetValue())
    {
        m_ei->text_qualifier = wxT("");
    }
     else if (m_othertextqualifier_radio->GetValue())
    {
        m_ei->text_qualifier = m_othertextqualifier_text->GetValue();
    }

    m_ei->first_row_header = m_firstrowheader_check->GetValue();
}


