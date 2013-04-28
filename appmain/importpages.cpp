/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-03-01
 *
 */


#include "appmain.h"
#include "connectionwizard.h"
#include "importpages.h"
#include "importtemplate.h"
#include "dlgdatabasefile.h"
#include "jobimport.h"
#include "textview.h"
#include "tangogridmodel.h"
#include "structurevalidator.h"


// control IDs

enum
{
    // table selection page
    ID_BasePathTextCtrl,
    ID_BrowseButton,
    ID_ImportTablesList,
    ID_SelectAllButton,
    ID_SelectNoneButton
};


// table selection grid column indexes

enum
{
    ONOFF_IDX = 0,
    SOURCE_TABLENAME_IDX = 1,
    DEST_TABLENAME_IDX = 2,
    APPEND_IDX = 3
};


// -- utility functions --

static void getAllSets(tango::IDatabasePtr db_ptr,
                       const wxString& path,
                       std::vector<wxString>& retval)
{
    tango::IFileInfoEnumPtr items = db_ptr->getFolderInfo(towstr(path));

    if (items.isNull())
        return;

    int count = items->size();
    int item_type;
    int i;

    for (i = 0; i < count; ++i)
    {
        tango::IFileInfoPtr info = items->getItem(i);
        item_type = info->getType();

        if (item_type == tango::filetypeFolder)
        {
            wxString folder_path = path;
            folder_path += info->getName();
            folder_path += "/";

            // recursively traverse this folder
            getAllSets(db_ptr, folder_path, retval);
        }
         else if (item_type == tango::filetypeTable)
        {
            wxString name = path;
            
            if (name.length() == 0 || name.Last() != '/')
                name += wxT("/");
                
            name += info->getName();

            retval.push_back(name);
        }
    }
}

static std::vector<RowErrorChecker> getRowErrorCheckerVector(kcl::Grid* grid)
{
    std::vector<RowErrorChecker> vec;
    
    int row, row_count = grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        // don't include unselected rows in the check
        if (!grid->getCellBoolean(row, ONOFF_IDX))
            continue;
            
        wxString tablename = grid->getCellString(row, DEST_TABLENAME_IDX);
        
        // permit output tablenames to have slashes (subdirectories, especially regarding kpg imports)
        tablename = removeChar(tablename, wxT('/'));
        
        vec.push_back(RowErrorChecker(row, tablename));
    }

    return vec;
}




// -- ImportTableSelectionPage class implementation --

BEGIN_EVENT_TABLE(ImportTableSelectionPage, kcl::WizardPage)
    EVT_KCLGRID_END_EDIT(ImportTableSelectionPage::onGridEndEdit)
    EVT_KCLGRID_NEED_TOOLTIP_TEXT(ImportTableSelectionPage::onGridNeedTooltipText)
    EVT_BUTTON(ID_BrowseButton, ImportTableSelectionPage::onBrowse)
    EVT_TEXT(ID_BasePathTextCtrl, ImportTableSelectionPage::onBasePathChanged)
    EVT_BUTTON(ID_SelectAllButton, ImportTableSelectionPage::onSelectAllTables)
    EVT_BUTTON(ID_SelectNoneButton, ImportTableSelectionPage::onSelectNoTables)
END_EVENT_TABLE()


ImportTableSelectionPage::ImportTableSelectionPage(kcl::Wizard* parent,
                                                   ImportInfo* ii) :
                                                     kcl::WizardPage(parent),
                                                     m_ii(ii)
{
    SetSize(parent->GetClientSize());

    m_message = new wxStaticText(this,
                                 -1,
                                 _("Please select the tables from the list that you would like to import."));
    resizeStaticText(m_message);

    m_grid = new kcl::RowSelectionGrid(this,
                                       ID_ImportTablesList,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       kcl::DEFAULT_BORDER,
                                       false, false);
    m_grid->setGreenBarInterval(0);

    m_grid->createModelColumn(ONOFF_IDX, wxEmptyString, kcl::Grid::typeBoolean, 1, 0);
    m_grid->createModelColumn(SOURCE_TABLENAME_IDX, _("Source Name"), kcl::Grid::typeCharacter, 512, 0);
    m_grid->createModelColumn(DEST_TABLENAME_IDX, _("Destination Name"), kcl::Grid::typeCharacter, 512, 0);
    m_grid->createModelColumn(APPEND_IDX, _("Append"), kcl::Grid::typeBoolean, 1, 0);

    // set cell properties for the grid
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskAlignment;
    cellprops.alignment = kcl::Grid::alignRight;
    m_grid->setModelColumnProperties(ONOFF_IDX, &cellprops);
    
    cellprops.mask = kcl::CellProperties::cpmaskAlignment;
    cellprops.alignment = kcl::Grid::alignCenter;
    m_grid->setModelColumnProperties(APPEND_IDX, &cellprops);
    
    cellprops.mask = kcl::CellProperties::cpmaskEditable;
    cellprops.editable = false;
    m_grid->setModelColumnProperties(SOURCE_TABLENAME_IDX, &cellprops);
    
    m_grid->setRowLabelSize(0);
    m_grid->createDefaultView();
    m_grid->setColumnSize(ONOFF_IDX, 23);
    m_grid->setColumnSize(SOURCE_TABLENAME_IDX, 120);
    m_grid->setColumnSize(DEST_TABLENAME_IDX, 120);
    m_grid->setColumnSize(APPEND_IDX, 50);

    // create import location sizer
    
    wxStaticText* label_basepath = new wxStaticText(this,
                                                    -1,
                                                    _("Import to:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_basepath_textctrl = new wxTextCtrl(this,
                                         ID_BasePathTextCtrl,
                                         wxEmptyString,
                                         wxDefaultPosition,
                                         wxSize(200,21));
    m_basepath_textctrl->SetValue(m_ii->base_path);
    
    wxButton* browse_button = new wxButton(this, ID_BrowseButton, _("Browse..."));
    
    wxBoxSizer* dest_folder_sizer = new wxBoxSizer(wxHORIZONTAL);
    dest_folder_sizer->Add(label_basepath, 0, wxALIGN_CENTER);
    dest_folder_sizer->AddSpacer(5);
    dest_folder_sizer->Add(m_basepath_textctrl, 1, wxALIGN_CENTER);
    dest_folder_sizer->AddSpacer(5);
    dest_folder_sizer->Add(browse_button);
    
    // create button sizer
    
    wxButton* selectall_button = new wxButton(this, ID_SelectAllButton, _("Select All"));
    wxButton* selectnone_button = new wxButton(this, ID_SelectNoneButton, _("Select None"));
    
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->Add(selectall_button);
    button_sizer->AddSpacer(8);
    button_sizer->Add(selectnone_button);
    
    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(20);
    main_sizer->Add(m_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(4);
    main_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                    0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(12);
    main_sizer->Add(dest_folder_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(8);
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(12);

    SetSizer(main_sizer);
    Layout();
}


void ImportTableSelectionPage::loadPageData()
{
}

void ImportTableSelectionPage::savePageData()
{
    // populate the tables vector and attempt to begin the import
    
    bool append_column_visible =
            (m_grid->getColumnViewIdx(APPEND_IDX)) != -1 ? true : false;
    
    m_ii->tables.clear();
    
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        ImportTableSelection its;
        its.selected = m_grid->getCellBoolean(row, ONOFF_IDX);
        its.input_tablename = m_grid->getCellString(row, SOURCE_TABLENAME_IDX);
        its.output_tablename = m_grid->getCellString(row, DEST_TABLENAME_IDX);
        
        if (append_column_visible)
            its.append = m_grid->getCellBoolean(row, APPEND_IDX);
         else
            its.append = false;
            
        m_ii->tables.push_back(its);
    }
}


ImportTableSelection* ImportTableSelectionPage::lookupTemplateTable(const wxString& tbl)
{
    std::vector<ImportTableSelection>::iterator it;
    for (it = m_ii->tables.begin(); it != m_ii->tables.end(); ++it)
    {
        if (tbl.CmpNoCase(it->input_tablename) == 0)
            return &(*it);
        if (tbl.CmpNoCase(wxT("/") + it->input_tablename) == 0)
            return &(*it);
    }
    return NULL;
}



// wizard event handlers

void ImportTableSelectionPage::onPageChanged()
{
    //  handle grid layout
    m_grid->hideAllColumns();

    // get the grid's size for column size calculations
    int w, h, col_w;
    m_grid->GetClientSize(&w, &h);
    
    // all imports have these three columns in the table
    m_grid->insertColumn(ONOFF_IDX, ONOFF_IDX);
    m_grid->insertColumn(SOURCE_TABLENAME_IDX, SOURCE_TABLENAME_IDX);
    m_grid->insertColumn(DEST_TABLENAME_IDX, DEST_TABLENAME_IDX);

    if (m_ii->type == dbtypeUndefined)
        return;

    if (m_ii->type == dbtypePackage)
    {
        // resize columns to fit grid area
        m_grid->setColumnSize(ONOFF_IDX, 23);
        col_w = (w-23)/2;
        m_grid->setColumnSize(SOURCE_TABLENAME_IDX, col_w);
        col_w = w-23-col_w;
        m_grid->setColumnSize(DEST_TABLENAME_IDX, col_w);
    }
     else
    {
        // we're not importing a package file, show the append column
        m_grid->insertColumn(APPEND_IDX, APPEND_IDX);

        // resize columns to fit grid area
        m_grid->setColumnSize(ONOFF_IDX, 23);
        m_grid->setColumnSize(APPEND_IDX, 50);
        col_w = (w-23-50)/2;
        m_grid->setColumnSize(SOURCE_TABLENAME_IDX, col_w);
        col_w = w-23-50-col_w;
        m_grid->setColumnSize(DEST_TABLENAME_IDX, col_w);
    }

    AppBusyCursor bc;

    
    // XBASE/TEXT-DELIMITED: populate the grid from the tables vector
    
    if (m_ii->type == dbtypeXbase || m_ii->type == dbtypeDelimitedText)
    {
        // sort the table vector
        std::sort(m_ii->tables.begin(),
                  m_ii->tables.end(),
                  ImportTableSelectionLess());

        bool changes_made = false;
        
        // compare grid data with the vector to see if something's changed
        int row, row_count = m_grid->getRowCount();
        if (row_count != m_ii->tables.size())
        {
            changes_made = true;
        }
         else
        {
            for (row = 0; row < row_count; ++row)
            {
                wxString grid_source = m_grid->getCellString(row, SOURCE_TABLENAME_IDX);
                wxString vec_source = m_ii->tables[row].input_tablename;
                if (grid_source.CmpNoCase(vec_source) != 0)
                {
                    changes_made = true;
                    break;
                }
            }
        }
        
        if (changes_made || row_count == 0)
        {
            // populate the grid from the tables vector
            int row = 0;
            m_grid->deleteAllRows();
            std::vector<ImportTableSelection>::iterator it;
            for (it = m_ii->tables.begin(); it != m_ii->tables.end(); ++it)
            {
                m_grid->insertRow(-1);
                m_grid->setCellBoolean(row, ONOFF_IDX, it->selected);
                m_grid->setCellString(row, SOURCE_TABLENAME_IDX, it->input_tablename);
                m_grid->setCellString(row, DEST_TABLENAME_IDX, it->output_tablename);
                m_grid->setCellBitmap(row, DEST_TABLENAME_IDX, GETBMP(xpm_blank_16));
                m_grid->setCellBoolean(row, APPEND_IDX, it->append);
                row++;
            }
        }
        
        checkOverlayText();
        m_grid->refresh(kcl::Grid::refreshAll);
        return;
    }
    

    // ALL OTHER IMPORTS: get the list of tables from the import source
    //                    and populate the grid from this list
    
    IConnectionPtr conn;
    
    if (m_ii->m_conn_ptr.isOk())
    {
        conn = m_ii->m_conn_ptr;
    }
     else
    {
        conn = createUnmanagedConnection();
        conn->setType(m_ii->type);
        conn->setDescription(m_ii->description);
        conn->setHost(m_ii->server);
        conn->setPort(m_ii->port);
        conn->setDatabase(m_ii->database);
        conn->setUsername(m_ii->username);
        conn->setPassword(m_ii->password);
        conn->setPath(m_ii->path);

        // if we cannot open the connection, bail out
        if (!conn->open())
        {
            wxString msg = _("There was an error connecting to the specified database.");
            msg += wxT("  ");
            msg += conn->getErrorString();
            
            appMessageBox(msg,
                          _("Import Wizard"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
    }


    // get a list of all items in the source database
    std::vector<wxString> info;
    std::vector<wxString>::iterator info_it;
    getAllSets(conn->getDatabasePtr(), wxT("/"), info);


    // format oracle tables the way oracle users
    // would expect them: "owner.table_name"

    if (m_ii->type == dbtypeOracle || m_ii->type == dbtypeDb2)
    {
        for (info_it = info.begin(); info_it != info.end(); ++info_it)
        {
            wxString temps;
            temps = info_it->AfterFirst(wxT('/'));
            temps.Replace(wxT("/"), wxT("."));
            *info_it = temps;
        }
    }

    // sort the tablenames vector
    std::sort(info.begin(), info.end());

    // populate the grid from the info vector
    int row = 0;
    m_grid->deleteAllRows();
    for (info_it = info.begin(); info_it != info.end(); ++info_it)
    {
        bool selected = true;
        wxString src_tablename = *info_it;
        wxString out_tablename = info_it->AfterLast(wxT('/'));
        bool append = false;


        // remove slash from beginning
        if (src_tablename.Freq('/') == 1 && src_tablename[0] == '/')
            src_tablename = src_tablename.substr(1);

        if (out_tablename.CmpNoCase(*info_it) == 0)
            out_tablename = info_it->AfterLast(wxT('.'));
        out_tablename = makeValidObjectName(out_tablename,
                                            g_app->getDatabase());

        // correlate with the template
        if (m_ii->tables.size() > 0)
        {
            // tables already exist in the template.  "selected" is
            // by default off.  If the line exists in the template, use
            // the info stored there.
            selected = false;

            ImportTableSelection* s = lookupTemplateTable(src_tablename);
            if (s)
            {
                out_tablename = s->output_tablename;
                append = s->append;
                selected = true;
            }
        }
        
        m_grid->insertRow(-1);
        m_grid->setCellBoolean(row, ONOFF_IDX, selected);
        m_grid->setCellString(row, SOURCE_TABLENAME_IDX, src_tablename);
        m_grid->setCellString(row, DEST_TABLENAME_IDX, out_tablename);
        m_grid->setCellBitmap(row, DEST_TABLENAME_IDX, GETBMP(xpm_blank_16));
        m_grid->setCellBoolean(row, APPEND_IDX, append);
        row++;
    }
    
    
    m_basepath_textctrl->SetValue(m_ii->base_path);
    
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
    return;
}

bool ImportTableSelectionPage::onPageChanging(bool forward)
{
    // set the last kpg path for comparison purposes if we go back and forth
    if (m_ii->type == dbtypePackage)
        m_last_path = m_ii->path;

    // set the last type for comparison purposes if we go back and forth
    m_last_type = m_ii->type;

    if (m_grid->isEditing())
        m_grid->endEdit(true);

    if (!forward)
        return true;

    // CHECK: check for duplicate table names in the list
    bool block = false;
    int errorcode = checkDuplicateTablenames();
    m_grid->refreshColumn(kcl::Grid::refreshAll, DEST_TABLENAME_IDX);
    StructureValidator::showErrorMessage(errorcode, &block);
    
    // there is an error in the structure that must be fixed
    if (block)
        return false;

    // CHECK: check for invalid table names in the list
    errorcode = checkInvalidTablenames();
    m_grid->refreshColumn(kcl::Grid::refreshAll, DEST_TABLENAME_IDX);
    StructureValidator::showErrorMessage(errorcode, &block);
    
    // there is an error in the structure that must be fixed
    if (block)
        return false;

    // CHECK: check to make sure we've selected a table to import
    bool selected = false;
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        selected = m_grid->getCellBoolean(row, ONOFF_IDX);
        if (selected == true)
            break;
    }
    
    if (!selected)
    {
        appMessageBox(
            _("No items have been selected for import.  Please select at least one item to continue."),
            _("Import Wizard"),
            wxOK | wxICON_EXCLAMATION | wxCENTER);
        return false;
    }
    
    
    savePageData();
    
    return true;
}

void ImportTableSelectionPage::onBrowse(wxCommandEvent& evt)
{
    if (!g_app->isDatabaseOpen())
    {
        appMessageBox(_("To load an import template, please create or open a project"),
                      _("Import Wizard"),
                      wxOK | wxICON_EXCLAMATION | wxCENTER,
                      g_app->getMainWindow());

        return;
    }

    DlgDatabaseFile dlg(getWizard(), DlgDatabaseFile::modeSelectFolder);
    if (dlg.ShowModal() != wxID_OK)
        return;

    // this will call onBasePathChanged() which will update the import info
    m_basepath_textctrl->SetValue(dlg.getPath());
}

void ImportTableSelectionPage::onBasePathChanged(wxCommandEvent& evt)
{
    m_ii->base_path = evt.GetString();
}

void ImportTableSelectionPage::onGridNeedTooltipText(kcl::GridEvent& evt)
{
    int row_count = m_grid->getRowCount();
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col != DEST_TABLENAME_IDX)
        return;
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid);
    StructureValidator::findDuplicateObjectNames(check_rows);
    StructureValidator::findInvalidObjectNames(check_rows);
    
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

void ImportTableSelectionPage::onGridEndEdit(kcl::GridEvent& evt)
{
    if (evt.GetEditCancelled())
        return;
    
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col == DEST_TABLENAME_IDX)
    {
        // this will force the cell's text to be the text that we just
        // entered (this is necessary because we use getCellString()
        // in the checkDuplicateFieldnames() function below and the
        // cell's text has not yet changed)
        m_grid->setCellString(row, DEST_TABLENAME_IDX, evt.GetString());
        
        clearProblemRows();
        checkDuplicateTablenames();
        checkInvalidTablenames();
        m_grid->refreshColumn(kcl::Grid::refreshAll, DEST_TABLENAME_IDX);
    }
     else if (col == ONOFF_IDX)
    {
        // see above comment regarding cell updating
        m_grid->setCellBoolean(row, ONOFF_IDX, evt.GetBoolean());

        clearProblemRows();
        checkDuplicateTablenames();
        checkInvalidTablenames();
        m_grid->refreshColumn(kcl::Grid::refreshAll, DEST_TABLENAME_IDX);
    }
}

void ImportTableSelectionPage::checkOverlayText()
{
    // clear the overlay text from the grid
    m_grid->setOverlayText(wxEmptyString);

    // if there are no tables in the source database, show the overlay text
    if (m_grid->getRowCount() == 0)
    {
        if (m_ii->type == dbtypeXbase ||
            m_ii->type == dbtypeDelimitedText ||
            m_ii->type == dbtypeFixedLengthText)
        {
            m_grid->setOverlayText(_("No tables were specified for import"));
            return;
        }

        if (m_ii->type == dbtypeAccess ||
            m_ii->type == dbtypeExcel ||
            m_ii->type == dbtypePackage)
        {
            wxString message = wxString::Format(_("No tables were found in\n'%s'"), m_ii->path.c_str());
            m_grid->setOverlayText(message);
            return;
        }
        
        m_grid->setOverlayText(_("No tables were found in the source database"));
    }
}

void ImportTableSelectionPage::markProblemRow(int row, bool scroll_to)
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

int ImportTableSelectionPage::checkDuplicateTablenames(bool mark_rows)
{
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid);

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

int ImportTableSelectionPage::checkInvalidTablenames(bool mark_rows)
{
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid);

    bool errors_found = StructureValidator::findInvalidObjectNames(check_rows);
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
}

void ImportTableSelectionPage::clearProblemRows()
{
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
        m_grid->setCellBitmap(row, DEST_TABLENAME_IDX, GETBMP(xpm_blank_16));
}

void ImportTableSelectionPage::onSelectAllTables(wxCommandEvent& evt)
{
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    int row, count = m_grid->getRowCount();
    for (row = 0; row < count; ++row)
        m_grid->setCellBoolean(row, ONOFF_IDX, true);

    m_grid->refreshColumn(kcl::Grid::refreshAll, ONOFF_IDX);
}

void ImportTableSelectionPage::onSelectNoTables(wxCommandEvent& evt)
{
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    int row, count = m_grid->getRowCount();
    for (row = 0; row < count; ++row)
        m_grid->setCellBoolean(row, ONOFF_IDX, false);

    m_grid->refreshColumn(kcl::Grid::refreshAll, ONOFF_IDX);
}



BEGIN_EVENT_TABLE(ImportDelimitedTextSettingsPage, kcl::WizardPage)
END_EVENT_TABLE()


ImportDelimitedTextSettingsPage::ImportDelimitedTextSettingsPage(kcl::Wizard* parent,
                                                     ImportInfo* ii)
                                : kcl::WizardPage(parent), m_ii(ii)
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
                                          _("Modify the settings below to determine how the text-delimited files should be read."));
    resizeStaticText(text);


    // create main delimiters sizer
    
    m_comma_radio = new wxRadioButton(this,
                                      -1,
                                      _("Comma"),
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      wxRB_GROUP);

    m_tab_radio = new wxRadioButton(this, -1, _("Tab "));
    m_semicolon_radio = new wxRadioButton(this, -1, _("Semicolon"));
    m_pipe_radio = new wxRadioButton(this, -1, _("Pipe"));
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
    left_options_sizer->Add(m_pipe_radio, 0, wxEXPAND | wxLEFT, 10);
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

    loadPageData();
    
    SetSizer(main_sizer);
    Layout();
}

void ImportDelimitedTextSettingsPage::onPageChanged()
{

}

bool ImportDelimitedTextSettingsPage::onPageChanging(bool forward)
{
    savePageData();
    return true;
}

void ImportDelimitedTextSettingsPage::loadPageData()
{
    if (m_ii->delimiters == wxT(","))
    {
        m_comma_radio->SetValue(true);
    }
     else if (m_ii->delimiters == wxT("\t"))
    {
        m_tab_radio->SetValue(true);
    }
     else if (m_ii->delimiters == wxT(";"))
    {
        m_semicolon_radio->SetValue(true);
    }
     else if (m_ii->delimiters == wxT("|"))
    {
        m_pipe_radio->SetValue(true);
    }
     else if (m_ii->delimiters == wxT(" "))
    {
        m_space_radio->SetValue(true);
    }
     else if (m_ii->delimiters == wxT(""))
    {
        m_nodelimiters_radio->SetValue(true);
    }
     else
    {
        m_otherdelimiters_radio->SetValue(true);
        m_otherdelimiters_text->SetValue(m_ii->delimiters);
    }

    if (m_ii->text_qualifier == wxT("\""))
    {
        m_doublequote_radio->SetValue(true);
    }
     else if (m_ii->text_qualifier == wxT("'"))
    {
        m_singlequote_radio->SetValue(true);
    }
     else if (m_ii->text_qualifier == wxT(""))
    {
        m_notextqualifier_radio->SetValue(true);
    }
     else
    {
        m_othertextqualifier_radio->SetValue(true);
        m_othertextqualifier_text->SetValue(m_ii->text_qualifier);
    }

    m_firstrowheader_check->SetValue(m_ii->first_row_header);
}

void ImportDelimitedTextSettingsPage::savePageData()
{
    if (m_comma_radio->GetValue())
    {
        m_ii->delimiters = wxT(",");
    }
     else if (m_tab_radio->GetValue())
    {
        m_ii->delimiters = wxT("\t");
    }
     else if (m_semicolon_radio->GetValue())
    {
        m_ii->delimiters = wxT(";");
    }
     else if (m_pipe_radio->GetValue())
    {
        m_ii->delimiters = wxT("|");
    }
     else if (m_space_radio->GetValue())
    {
        m_ii->delimiters = wxT(" ");
    }
     else if (m_nodelimiters_radio->GetValue())
    {
        m_ii->delimiters = wxT("");
    }
     else
    {
        m_ii->delimiters = m_otherdelimiters_text->GetValue();
    }

    if (m_doublequote_radio->GetValue())
    {
        m_ii->text_qualifier = wxT("\"");
    }
     else if (m_singlequote_radio->GetValue())
    {
        m_ii->text_qualifier = wxT("'");
    }
     else if (m_notextqualifier_radio->GetValue())
    {
        m_ii->text_qualifier = wxT("");
    }
     else if (m_othertextqualifier_radio->GetValue())
    {
        m_ii->text_qualifier = m_othertextqualifier_text->GetValue();
    }

    m_ii->first_row_header = m_firstrowheader_check->GetValue();
}




