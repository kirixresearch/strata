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
#include "appcontroller.h"
#include "panelsplit.h"
#include "jobsplit.h"
#include "dbdoc.h"
#include "dlgdatabasefile.h"


enum TextControlState
{
    stateError,
    stateDisabled,
    stateNA
};

enum
{
    ID_SplitTypeChoice = wxID_HIGHEST + 1,
    ID_SourceTableTextCtrl,
    ID_PrefixTextCtrl,
    ID_RowCountTextCtrl,
    ID_TableCountTextCtrl,
    ID_TableSizeTextCtrl,
    ID_ExpressionTextCtrl,
    ID_BrowseButton
};

// helper functions

static int getSetColumnCount(tango::ISetPtr set)
{
    tango::IStructurePtr structure = set->getStructure();
    if (!structure)
        return 0;
        
    return structure->getColumnCount();
}

static int getSetRowWidth(tango::ISetPtr set)
{
    int row_width = 0;
    
    tango::IStructurePtr structure = set->getStructure();
    if (!structure)
        return 0;

    int i, col_count = structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr col = structure->getColumnInfoByIdx(i);
        if (col->getCalculated())
            continue;

        row_width += col->getWidth();
    }
    
    return row_width;
}

// return value is in bytes
static double getSetSize(tango::ISetPtr set)
{
    int col_count = getSetColumnCount(set);
    int row_width = getSetRowWidth(set);
    int row_count = set->getRowCount();
    
    int header_length = (col_count*98); // field header
    header_length += 1024; // file header
    
    return (header_length+(row_width*row_count));
}

// return value is in bytes
static double calculateSetSize(int col_count, int row_width, int row_count)
{
    return ((col_count*98)+1024+(row_width*row_count));
}

xf_off_t calculateSetRowCount(int col_count, int row_width, double file_size)
{
    return floor((file_size-(col_count*98)-1024)/row_width);
}


static void onDivideJobFinished(jobs::IJobPtr job)
{
    g_app->getAppController()->refreshDbDoc();
}



BEGIN_EVENT_TABLE(SplitPanel, wxPanel)
    EVT_CHOICE(ID_SplitTypeChoice, SplitPanel::onSplitTypeChanged)
    EVT_TEXT(ID_PrefixTextCtrl, SplitPanel::onPrefixTextChanged)
    EVT_TEXT(ID_SourceTableTextCtrl, SplitPanel::onSourceTableTextChanged)
    EVT_TEXT(ID_TableCountTextCtrl, SplitPanel::onTableCountTextChanged)
    EVT_TEXT(ID_TableSizeTextCtrl, SplitPanel::onTableSizeTextChanged)
    EVT_TEXT(ID_RowCountTextCtrl, SplitPanel::onRowCountTextChanged)
    EVT_BUTTON(ID_BrowseButton, SplitPanel::onBrowse)
    EVT_BUTTON(wxID_OK, SplitPanel::onOK)
    EVT_BUTTON(wxID_CANCEL, SplitPanel::onCancel)
END_EVENT_TABLE()



SplitPanel::SplitPanel()
{
    m_splittype_choice = NULL;
    m_sourcetable_textctrl = NULL;
    m_prefix_textctrl = NULL;
    m_rowcount_textctrl = NULL;
    m_tablecount_textctrl = NULL;
    m_tablesize_textctrl = NULL;
    m_expression_textctrl = NULL;
    m_browse_button = NULL;
    m_ok_button = NULL;
    m_prefix_edited = false;
}

SplitPanel::~SplitPanel()
{

}

// -- IDocument --

bool SplitPanel::initDoc(IFramePtr frame,
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

    doc_site->setCaption(_("Divide"));
    
    m_frame = frame;
    m_doc_site = doc_site;

    // create split type sizer
    wxStaticText* label_splitby = new wxStaticText(this, -1, _("Divide By:"));
    m_splittype_choice = new wxChoice(this, ID_SplitTypeChoice);
    m_splittype_choice->Append(_("Number of records"));
    m_splittype_choice->Append(_("Number of tables"));
    m_splittype_choice->Append(_("Table size"));
    //m_splittype_choice->Append(_("Formula"));
    m_splittype_choice->SetSelection(0);
    
    wxBoxSizer* splittype_sizer = new wxBoxSizer(wxHORIZONTAL);
    splittype_sizer->Add(label_splitby, 0, wxALIGN_CENTER);
    splittype_sizer->Add(m_splittype_choice, 1, wxALIGN_CENTER);

    // create source table sizer
    wxStaticText* label_sourcetable = new wxStaticText(this, -1, _("Source Table:"));
    m_sourcetable_textctrl = new wxTextCtrl(this, ID_SourceTableTextCtrl);
    m_browse_button = new wxButton(this, ID_BrowseButton, _("Browse..."));

    wxBoxSizer* sourcetable_sizer = new wxBoxSizer(wxHORIZONTAL);
    sourcetable_sizer = new wxBoxSizer(wxHORIZONTAL);
    sourcetable_sizer->Add(label_sourcetable, 0, wxALIGN_CENTER);
    sourcetable_sizer->Add(m_sourcetable_textctrl, 1, wxALIGN_CENTER);
    sourcetable_sizer->AddSpacer(5);
    sourcetable_sizer->Add(m_browse_button, 0, wxALIGN_CENTER);

    // create table prefix sizer
    wxStaticText* label_prefix = new wxStaticText(this, -1, _("Table Prefix:"));
    m_prefix_textctrl = new wxTextCtrl(this, ID_PrefixTextCtrl);

    wxBoxSizer* prefix_sizer = new wxBoxSizer(wxHORIZONTAL);
    prefix_sizer->Add(label_prefix, 0, wxALIGN_CENTER);
    prefix_sizer->Add(m_prefix_textctrl, 1, wxEXPAND);

    // create row count sizer
    wxStaticText* label_rowcount = new wxStaticText(this, -1, _("Record Count:"));
    m_rowcount_textctrl = new wxTextCtrl(this, ID_RowCountTextCtrl);
    wxStaticText* label_records = new wxStaticText(this, -1, _("Records"));

    m_rowcount_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_rowcount_sizer->AddSpacer(10);
    m_rowcount_sizer->Add(label_rowcount, 0, wxALIGN_CENTER);
    m_rowcount_sizer->Add(m_rowcount_textctrl, 1, wxALIGN_CENTER);
    m_rowcount_sizer->AddSpacer(5);
    m_rowcount_sizer->Add(label_records, 0, wxALIGN_CENTER);
    m_rowcount_sizer->AddSpacer(10);

    // create table count sizer
    wxStaticText* label_tablecount = new wxStaticText(this, -1, _("Table Count:"));
    wxStaticText* label_tables = new wxStaticText(this, -1, _("Tables"));
    m_tablecount_textctrl = new wxTextCtrl(this, ID_TableCountTextCtrl);

    m_tablecount_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_tablecount_sizer->AddSpacer(10);
    m_tablecount_sizer->Add(label_tablecount, 0, wxALIGN_CENTER);
    m_tablecount_sizer->Add(m_tablecount_textctrl, 1, wxALIGN_CENTER);
    m_tablecount_sizer->AddSpacer(5);
    m_tablecount_sizer->Add(label_tables, 0, wxALIGN_CENTER);
    m_tablecount_sizer->AddSpacer(10);

    // create table size sizer
    wxStaticText* label_tablesize = new wxStaticText(this, -1, _("Table Size:"));
    m_tablesize_textctrl = new wxTextCtrl(this, ID_TableSizeTextCtrl);
    wxStaticText* label_mb = new wxStaticText(this, -1, _("MB"));

    m_tablesize_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_tablesize_sizer->AddSpacer(10);
    m_tablesize_sizer->Add(label_tablesize, 0, wxALIGN_CENTER);
    m_tablesize_sizer->Add(m_tablesize_textctrl, 1, wxALIGN_CENTER);
    m_tablesize_sizer->AddSpacer(5);
    m_tablesize_sizer->Add(label_mb, 0, wxALIGN_CENTER);
    m_tablesize_sizer->AddSpacer(10);

    // create expression sizer
    wxStaticText* label_expression = new wxStaticText(this, -1, _("Formula:"));
    m_expression_textctrl = new wxTextCtrl(this,
                                           ID_ExpressionTextCtrl,
                                           wxEmptyString,
                                           wxDefaultPosition,
                                           wxDefaultSize,
                                           wxTE_MULTILINE | wxTE_BESTWRAP);

    m_expression_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_expression_sizer->AddSpacer(10);
    m_expression_sizer->Add(label_expression, 0, wxEXPAND | wxTOP, 4);
    m_expression_sizer->Add(m_expression_textctrl, 1, wxEXPAND);
    m_expression_sizer->AddSpacer(10);

    // -- create a platform standards-compliant OK/Cancel sizer --
    
    m_ok_button = new wxButton(this, wxID_OK, _("Run"));
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(m_ok_button);
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    // resize labels to same width
    wxSize max_size = getMaxTextSize(label_splitby, label_prefix,
                                          label_sourcetable, label_tablecount,
                                          label_rowcount, label_tablesize,
                                          label_expression);
    max_size.x += 10;
    
    sourcetable_sizer->SetItemMinSize(label_sourcetable, max_size);
    splittype_sizer->SetItemMinSize(label_splitby, max_size);
    prefix_sizer->SetItemMinSize(label_prefix, max_size);
    m_rowcount_sizer->SetItemMinSize(label_rowcount, max_size);
    m_tablecount_sizer->SetItemMinSize(label_tablecount, max_size);
    m_tablesize_sizer->SetItemMinSize(label_tablesize, max_size);
    m_expression_sizer->SetItemMinSize(label_expression, max_size);

    max_size = getMaxTextSize(label_tables, label_records, label_mb);
    m_rowcount_sizer->SetItemMinSize(label_records, max_size);
    m_tablecount_sizer->SetItemMinSize(label_tables, max_size);
    m_tablesize_sizer->SetItemMinSize(label_mb, max_size);
    
    // create input options sizer
    wxStaticBox* box1 = new wxStaticBox(this, -1, _("Input Options"));
    m_input_sizer = new wxStaticBoxSizer(box1, wxVERTICAL);
    m_input_sizer->AddSpacer(10);
    m_input_sizer->Add(sourcetable_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    m_input_sizer->AddSpacer(8);
    m_input_sizer->Add(splittype_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    m_input_sizer->AddSpacer(10);
    
    // create output options sizer
    wxStaticBox* box2 = new wxStaticBox(this, -1, _("Output Options"));
    m_output_sizer = new wxStaticBoxSizer(box2, wxVERTICAL);
    m_output_sizer->AddSpacer(10);
    m_output_sizer->Add(prefix_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    m_output_sizer->Add(m_rowcount_sizer, 0, wxEXPAND | wxTOP, 8);
    m_output_sizer->Add(m_tablecount_sizer, 0, wxEXPAND | wxTOP, 8);
    m_output_sizer->Add(m_tablesize_sizer, 0, wxEXPAND | wxTOP, 8);
    m_output_sizer->Add(m_expression_sizer, 1, wxEXPAND | wxTOP , 8);
    m_output_sizer->AddSpacer(10);

    // create main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(8);
    m_main_sizer->Add(m_input_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    m_main_sizer->AddSpacer(10);
    m_main_sizer->Add(m_output_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    m_spacer = m_main_sizer->AddStretchSpacer(1);
    m_main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(m_main_sizer);
    
    // -- connect signals --
    FsDataDropTarget* sourcetable_droptarget = new FsDataDropTarget;
    sourcetable_droptarget->sigDragDrop.connect(this, &SplitPanel::onSourceTableDropped);
    m_sourcetable_textctrl->SetDropTarget(sourcetable_droptarget);

    // calling this event function will call Layout() for us
    wxCommandEvent unused;
    onSplitTypeChanged(unused);
    onSourceTableTextChanged(unused);
    
    // no table is specified, so we always start out in invalid mode
    m_ok_button->Enable(false);
    
    m_sourcetable_textctrl->SetFocus();
    m_sourcetable_textctrl->SetInsertionPointEnd();
    return true;
}

wxWindow* SplitPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void SplitPanel::setDocumentFocus()
{

}

void SplitPanel::onBrowse(wxCommandEvent& evt)
{
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeOpenTable);
    dlg.setCaption(_("Select Source Table"));
    dlg.setAffirmativeButtonLabel(_("OK"));
    if (dlg.ShowModal() != wxID_OK)
        return;

    m_prefix_textctrl->ChangeValue(dlg.getPath());
    m_sourcetable_textctrl->SetValue(dlg.getPath());
}

static SplitType getSplitType(int splittype_choice_selection)
{
    if (splittype_choice_selection == 0)
        return splitByRowCount;
    if (splittype_choice_selection == 1)
        return splitByTableCount;
    if (splittype_choice_selection == 2)
        return splitByTableSize;
    if (splittype_choice_selection == 3)
        return splitByExpression;
    
    return splitUndefined;
}

void SplitPanel::onOK(wxCommandEvent& event)
{
    if (m_set.isNull())
    {
        appMessageBox(_("The source table specified is invalid.  Please specify a valid table to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    // create the split info for the split job
    SplitInfo si;
    si.type = getSplitType(m_splittype_choice->GetSelection());
    si.source_set = m_set;
    si.prefix = m_prefix_textctrl->GetValue();
    si.expression = m_expression_textctrl->GetValue();
    m_rowcount_textctrl->GetValue().ToLong(&si.row_count);
    m_tablecount_textctrl->GetValue().ToLong(&si.table_count);


    // check output paths
    wxString output_path;
    for (int i = 0; i < si.table_count; ++i)
    {
        output_path = si.prefix;
        
        if (si.table_count < 10)
            output_path += wxString::Format(wxT("_%01d"), i+1);
         else if (si.table_count >= 10 && si.table_count < 100)
            output_path += wxString::Format(wxT("_%02d"), i+1);
         else
            output_path += wxString::Format(wxT("_%03d"), i+1);

        if (!doOutputPathCheck(output_path))
            return;
    }


    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.divide-data");

    kl::JsonNode params;
    params["input"].setString(towstr(si.source_set->getObjectPath()));
    params["output"].setString(towstr(si.prefix));
    params["row_count"].setInteger(si.row_count);
    
    job->getJobInfo()->setTitle(towstr(_("Divide Table")));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(&onDivideJobFinished);
    g_app->getJobQueue()->addJob(job, jobStateRunning);

    m_frame->closeSite(m_doc_site);    
}

void SplitPanel::onCancel(wxCommandEvent& evt)
{
    m_frame->closeSite(m_doc_site);
}

void SplitPanel::onSplitTypeChanged(wxCommandEvent& event)
{
    Freeze();
    
    int sel = m_splittype_choice->GetSelection();
    
    // show/hide sizers based on split type
    m_output_sizer->Show(m_rowcount_sizer,   (sel < 3) ? true:false, true);
    m_output_sizer->Show(m_tablecount_sizer, (sel < 3) ? true:false, true);
    m_output_sizer->Show(m_tablesize_sizer,  (sel < 3) ? true:false, true);
    m_output_sizer->Show(m_expression_sizer, (sel < 3) ? false:true, true);
    
    // set the output sizer's proportion to 1 if
    // we're going to show the expression sizer
    wxSizerItem* output_sizer_item = m_main_sizer->GetItem(m_output_sizer);
    output_sizer_item->SetProportion((sel < 3) ? 0 : 1);
    
    // show the spacer if we're not showing  the expression sizer
    m_spacer->Show((sel < 3) ? true:false);
    
    // enable/disable controls based on if the set path is valid
    m_prefix_textctrl->Enable(m_set.isOk() ? true:false);
    m_rowcount_textctrl->Enable((sel == 0) && m_set.isOk() ? true:false);
    m_tablecount_textctrl->Enable((sel == 1) && m_set.isOk() ? true:false);
    m_tablesize_textctrl->Enable((sel == 2) && m_set.isOk() ? true:false);

    Layout();
    Thaw();
}

void SplitPanel::setTextControls(int state)
{
    int split_type = m_splittype_choice->GetSelection();
 
    wxString str;
    if (state == stateError)
        str = _("ERROR");
     else if (state == stateDisabled)
    {
        str = wxT("--");
        split_type = -1;
    }
     else if (state == stateNA)
        str = wxT("N/A");
    
    if (split_type != 0) m_rowcount_textctrl->ChangeValue(str);
    if (split_type != 1) m_tablecount_textctrl->ChangeValue(str);
    if (split_type != 2) m_tablesize_textctrl->ChangeValue(str);
}

bool SplitPanel::validate()
{
    if (!m_set)
        return false;
        
    bool retval = true;
    int split_type = m_splittype_choice->GetSelection();

    wxString row_count   = m_rowcount_textctrl->GetValue();
    wxString table_count = m_tablecount_textctrl->GetValue();
    wxString table_size  = m_tablesize_textctrl->GetValue();
    
    // if any of the values are empty,
    // don't allow the user to continue

    if (table_count.IsEmpty())
        retval = false;

    if (row_count.IsEmpty())
        retval = false;

    if (table_size.IsEmpty())
        retval = false;

    // if the table or row count is not a whole number,
    // don't allow the user to continue

    if (wxStrpbrk(row_count, wxT(",.")))
        retval = false;

    if (wxStrpbrk(table_count, wxT(",.")))
        retval = false;

    if (retval == false)
    {
        setTextControls(stateError);
        return retval;
    }

    // if any of the values are out of bounds,
    // don't allow the user to continue

    int table_count_val = wxAtoi(table_count);
    int row_count_val = wxAtoi(row_count);
    double table_size_val = wxAtof(table_size);

    // minimum row count value
    if (split_type == 0 && row_count_val < 1)
    {
        m_rowcount_textctrl->SetValue(wxString::Format(wxT("%d"), 1));
        m_rowcount_textctrl->SetInsertionPointEnd();
        return true;
    }
    
    // maximum row count value
    if (split_type == 0 && row_count_val > m_set->getRowCount())
    {
        m_rowcount_textctrl->SetValue(wxString::Format(wxT("%d"), m_set->getRowCount()));
        m_rowcount_textctrl->SetInsertionPointEnd();
        return true;
    }

    // absolute maximum row count value
    if (split_type == 0 && row_count_val > 2000000000)
    {
        m_rowcount_textctrl->SetValue(wxString::Format(wxT("%d"), 2000000000));
        m_rowcount_textctrl->SetInsertionPointEnd();
        return true;
    }

    // minimum split table value
    if (split_type == 1 && table_count_val < 1)
    {
        m_tablecount_textctrl->SetValue(wxString::Format(wxT("%d"), 1));
        m_tablecount_textctrl->SetInsertionPointEnd();
        return true;
    }
    
    // maximum split table value
    if (split_type == 1 && table_count_val > m_set->getRowCount())
    {
        m_tablecount_textctrl->SetValue(wxString::Format(wxT("%d"), m_set->getRowCount()));
        m_tablecount_textctrl->SetInsertionPointEnd();
        return true;
    }

    // absolute maximum split table value
    if (split_type == 1 && table_count_val > 1000)
    {
        m_tablecount_textctrl->SetValue(wxString::Format(wxT("%d"), 1000));
        m_tablecount_textctrl->SetInsertionPointEnd();
        return true;
    }
    
    // minimum table size value
    if (split_type == 2 && table_size_val < 0.01)
    {
        m_tablesize_textctrl->SetValue(wxString::Format(wxT("%.02f"), 0.01));
        m_tablesize_textctrl->SetInsertionPointEnd();
        return true;
    }
    
    // maximum table size value (make sure it's at least the minimum too to avoid an endless loop)
    double max_table_size = std::max(getSetSize(m_set)/1048576.0, 0.01);
    if (split_type == 2 && table_size_val > max_table_size)
    {
        m_tablesize_textctrl->SetValue(wxString::Format(wxT("%.02f"), max_table_size));
        m_tablesize_textctrl->SetInsertionPointEnd();
        return true;
    }
    
    // absolute maximum table size value
    if (split_type == 2 && table_size_val > 2048.0)
    {
        m_tablesize_textctrl->SetValue(wxString::Format(wxT("%.02f"), 2048.0));
        m_tablesize_textctrl->SetInsertionPointEnd();
        return true;
    }

    return true;
}

void SplitPanel::populate()
{
    xf_off_t table_row_count;
    xf_off_t split_row_count;
    int split_table_count;
    double split_table_size;

    if (!m_set)
    {
        m_rowcount_textctrl->ChangeValue(wxT("N/A"));
        m_tablecount_textctrl->ChangeValue(wxT("N/A"));
        m_tablesize_textctrl->ChangeValue(wxT("N/A"));
        return;
    }
    
    int sel = m_splittype_choice->GetSelection();
    switch (sel)
    {
        case 0: /* split by row count */
        {
            if (m_set->getSetFlags() & tango::sfFastRowCount)
            {
                table_row_count = m_set->getRowCount();
                split_row_count = wxAtoi(m_rowcount_textctrl->GetValue());

                if (split_row_count == 0)
                {
                    // row count may be uninitialized
                    split_row_count = table_row_count;
                    m_rowcount_textctrl->ChangeValue(wxString::Format(wxT("%d"), split_row_count));
                }
                
                if (split_row_count == 0)
                {
                    // row count is invalid
                    m_tablecount_textctrl->ChangeValue(wxT("N/A"));
                    m_tablesize_textctrl->ChangeValue(wxT("N/A"));
                    return;
                }
                
                split_table_count = (int)(ceil((double)table_row_count/(double)split_row_count));
                split_table_size = calculateSetSize(getSetColumnCount(m_set),
                                                    getSetRowWidth(m_set),
                                                    split_row_count)/1048576.0;
                
                m_tablecount_textctrl->ChangeValue(wxString::Format(wxT("%d"), split_table_count));
                m_tablesize_textctrl->ChangeValue(wxString::Format(wxT("%.02f"), split_table_size));
            }
             else
            {
                // this set does not report its row count
                m_tablecount_textctrl->ChangeValue(wxT("N/A"));
                m_tablesize_textctrl->ChangeValue(wxT("N/A"));
                return;
            }

            break;
        }
        
        case 1: /* split by table count */
        {
            if (m_set->getSetFlags() & tango::sfFastRowCount)
            {
                table_row_count = m_set->getRowCount();
                split_table_count = wxAtoi(m_tablecount_textctrl->GetValue());
                
                if (split_table_count == 0)
                {
                    // table count may be uninitialized
                    split_table_count = 1;
                    m_tablecount_textctrl->ChangeValue(wxString::Format(wxT("%d"), 1));
                }

                split_row_count = (int)(ceil((double)table_row_count/(double)split_table_count));
                if (split_row_count > table_row_count)
                    split_row_count = table_row_count;
                
                split_table_size = calculateSetSize(getSetColumnCount(m_set),
                                                    getSetRowWidth(m_set),
                                                    split_row_count)/1048576.0;
                
                wxString format = wxT("%");
                format += towx(wxLongLongFmtSpec);
                format += wxT("u");
                
                m_rowcount_textctrl->ChangeValue(wxString::Format((const wxChar*)format.c_str(), split_row_count));
                m_tablesize_textctrl->ChangeValue(wxString::Format(wxT("%.02f"), split_table_size));
            }
             else
            {
                // this set does not report its row count
                m_rowcount_textctrl->ChangeValue(wxT("N/A"));
                m_tablesize_textctrl->ChangeValue(wxT("N/A"));
                return;
            }

            break;
        }
        
        case 2: /* split by table size */
        {
            if (m_set->getSetFlags() & tango::sfFastRowCount)
            {
                table_row_count = m_set->getRowCount();
                split_table_size = wxAtof(m_tablesize_textctrl->GetValue());
                split_table_size *= 1048576.0;
                
                int row_width = getSetRowWidth(m_set);
                double table_file_size = getSetSize(m_set);
                
                split_row_count = calculateSetRowCount(getSetColumnCount(m_set),
                                                       getSetRowWidth(m_set),
                                                       split_table_size);
                if (split_row_count > table_row_count)
                    split_row_count = table_row_count;
                
                split_table_count = (int)(ceil((double)table_file_size/(double)split_table_size));
                
                wxString format = wxT("%");
                format += towx(wxLongLongFmtSpec);
                format += wxT("u");
                
                m_rowcount_textctrl->ChangeValue(wxString::Format((const wxChar*)format.c_str(), split_row_count));
                m_tablecount_textctrl->ChangeValue(wxString::Format(wxT("%d"), split_table_count));
            }
             else
            {
                // this set does not report its row count
                m_rowcount_textctrl->ChangeValue(wxT("N/A"));
                m_tablecount_textctrl->ChangeValue(wxT("N/A"));
                return;
            }
        }
        
        case 3: /* split by expression */
        {
            break;
        }
    }
}

void SplitPanel::onPrefixTextChanged(wxCommandEvent& event)
{
    m_prefix_edited = true;
}

void SplitPanel::onSourceTableTextChanged(wxCommandEvent& event)
{
    std::wstring set_path = towstr(m_sourcetable_textctrl->GetValue());
    m_set = g_app->getDatabase()->openSet(set_path);
    
    if (!m_prefix_edited)
        m_prefix_textctrl->ChangeValue(event.GetString());
        
    if (m_set.isNull())
    {
        m_prefix_textctrl->Enable(false);
        m_rowcount_textctrl->Enable(false);
        m_tablecount_textctrl->Enable(false);
        m_tablesize_textctrl->Enable(false);
        m_expression_textctrl->Enable(false);
        setTextControls(stateDisabled);
    }
     else
    {
        wxString format = wxT("%");
        format += towx(wxLongLongFmtSpec);
        format += wxT("u");
                
        wxString val = wxString::Format((const wxChar*)format.c_str(), m_set->getRowCount());
        m_rowcount_textctrl->SetValue(val);
        
        m_prefix_textctrl->Enable(true);
        m_expression_textctrl->Enable(true);
        wxCommandEvent unused;
        onSplitTypeChanged(unused);
    }
    
    bool valid = validate();
    if (valid)
        populate();
    
    m_ok_button->Enable(valid ? true : false);
}

void SplitPanel::onTableCountTextChanged(wxCommandEvent& event)
{
    bool valid = validate();
    if (valid)
        populate();
    
    m_ok_button->Enable(valid ? true : false);
}

void SplitPanel::onRowCountTextChanged(wxCommandEvent& event)
{
    bool valid = validate();
    if (valid)
        populate();
    
    m_ok_button->Enable(valid ? true : false);
}

void SplitPanel::onTableSizeTextChanged(wxCommandEvent& event)
{
    bool valid = validate();
    if (valid)
        populate();
    
    m_ok_button->Enable(valid ? true : false);
}

void SplitPanel::onSourceTableDropped(wxDragResult& drag_result, FsDataObject* data)
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
            appMessageBox(_("One or more of the items dragged from the project panel is an external table and cannot be specified as the source table."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

        return;
    }

    if (res.size() == 0)
        return;

    m_prefix_textctrl->ChangeValue(res[0]);
    m_sourcetable_textctrl->SetValue(res[0]);
    m_sourcetable_textctrl->SetInsertionPointEnd();
    m_sourcetable_textctrl->SetFocus();
}




