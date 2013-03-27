/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-03-03
 *
 */


#include "appmain.h"
#include "dlgdatabasefile.h"
#include "moduleremoveduprec.h"
#include "tabledoc.h"
#include "jobquery.h"
#include "appcontroller.h"
#include "dbdoc.h"



enum
{
    ID_BrowseInputButton = wxID_HIGHEST + 1,
    ID_BrowseOutputButton
};


class DupRecInputOutputPage : public kcl::WizardPage,
                              public xcm::signal_sink
{
public:

    DupRecInputOutputPage(kcl::Wizard *parent,
                          DupRecInfo* info) : kcl::WizardPage(parent)
    {
        SetSize(parent->GetClientSize());

        m_info = info;

        // create the page's banner
        kcl::BannerControl* banner = createModuleBanner(this, _("File Selection"));
        
        // create the page's message
        wxStaticText* message = new wxStaticText(this, -1,
            _("Please specify the table on which you would like to remove duplicate records."));
        
        // create input file sizer
        wxStaticText* label_inputfile = new wxStaticText(this, -1, _("Input File:"));
        m_input_textctrl = new wxTextCtrl(this,
                                          -1,
                                          wxEmptyString,
                                          wxDefaultPosition,
                                          wxSize(200,21));
        
        wxButton* input_browse_button = new wxButton(this, ID_BrowseInputButton, _("Browse..."));
        
        wxBoxSizer* inputfile_sizer = new wxBoxSizer(wxHORIZONTAL);
        inputfile_sizer->Add(label_inputfile, 0, wxALIGN_CENTER);
        inputfile_sizer->Add(m_input_textctrl, 1, wxALIGN_CENTER);
        inputfile_sizer->AddSpacer(8);
        inputfile_sizer->Add(input_browse_button);

        // create output file sizer
        wxStaticText* label_outputfile = new wxStaticText(this, -1, _("Output File:"));
        m_output_textctrl = new wxTextCtrl(this,
                                           -1,
                                           wxEmptyString,
                                           wxDefaultPosition,
                                           wxSize(200,21));
        
        wxButton* output_browse_button = new wxButton(this, ID_BrowseOutputButton, _("Browse..."));
        
        wxBoxSizer* outputfile_sizer = new wxBoxSizer(wxHORIZONTAL);
        outputfile_sizer->Add(label_outputfile, 0, wxALIGN_CENTER);
        outputfile_sizer->Add(m_output_textctrl, 1, wxALIGN_CENTER);
        outputfile_sizer->AddSpacer(8);
        outputfile_sizer->Add(output_browse_button);

        wxSize label_size = getMaxTextSize(label_inputfile,
                                                label_outputfile);
        label_size.x += 8;
        
        inputfile_sizer->SetItemMinSize(label_inputfile, label_size);
        outputfile_sizer->SetItemMinSize(label_outputfile, label_size);

        // create main sizer
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->Add(banner, 0, wxEXPAND);
        main_sizer->AddSpacer(20);
        main_sizer->Add(message, 0, wxEXPAND | wxLEFT | wxRIGHT, 40);
        main_sizer->AddSpacer(4);
        main_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                        0, wxEXPAND | wxLEFT | wxRIGHT, 40);
        main_sizer->AddSpacer(12);
        main_sizer->Add(inputfile_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 40);
        main_sizer->AddSpacer(10);
        main_sizer->Add(outputfile_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 40);
        main_sizer->AddStretchSpacer(1);
        SetSizer(main_sizer);
        Layout();

        // make the text controls drop targets for the project panel
        FsDataDropTarget* input_drop_target = new FsDataDropTarget;
        input_drop_target->sigDragDrop.connect(this, &DupRecInputOutputPage::onInputTextDropped);
        m_input_textctrl->SetDropTarget(input_drop_target);
        
        FsDataDropTarget* output_drop_target = new FsDataDropTarget;
        output_drop_target->sigDragDrop.connect(this, &DupRecInputOutputPage::onOutputTextDropped);
        m_output_textctrl->SetDropTarget(output_drop_target);

        setFocusDeferred(m_input_textctrl);
        resizeStaticText(message, message->GetClientSize().GetWidth());
    }

    void onInputTextDropped(wxDragResult& drag_result, FsDataObject* data)
    {
        IFsItemEnumPtr items = data->getFsItems();
        std::vector<wxString> res;
        DbDoc::getFsItemPaths(items, res, true);

        if (res.size() > 1)
        {
            appMessageBox(_("You have selected either a folder or more than one data set.  Please select only one data set."),
                               APPLICATION_NAME,
                               wxOK | wxICON_INFORMATION | wxCENTER);
            return;
        }


        DbDoc* dbdoc = g_app->getDbDoc();
        IFsItemPtr item = items->getItem(0);

        if (res.size() == 0)
            return;

        m_input_textctrl->SetValue(res[0]);
        m_input_textctrl->SetInsertionPointEnd();
        m_input_textctrl->SetFocus();
    }


    void onOutputTextDropped(wxDragResult& drag_result, FsDataObject* data)
    {
        IFsItemEnumPtr items = data->getFsItems();
        std::vector<wxString> res;
        DbDoc::getFsItemPaths(items, res, true);

        if (res.size() > 1)
        {
            appMessageBox(_("You have selected either a folder or more than one data set.  Please select only one data set."),
                               APPLICATION_NAME,
                               wxOK | wxICON_INFORMATION | wxCENTER);
            return;
        }


        DbDoc* dbdoc = g_app->getDbDoc();
        IFsItemPtr item = items->getItem(0);

        if (res.size() == 0)
            return;

        m_output_textctrl->SetValue(res[0]);
        m_output_textctrl->SetInsertionPointEnd();
        m_output_textctrl->SetFocus();
    }

    void onBrowseInput(wxCommandEvent& event)
    {
        DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeOpen);
        dlg.setCaption(_("Select Input File"));
        dlg.setAffirmativeButtonLabel(_("OK"));
        if (dlg.ShowModal() != wxID_OK)
            return;
            
        m_input_textctrl->ChangeValue(dlg.getPath());
        m_input_textctrl->SetInsertionPointEnd();
        m_input_textctrl->SetFocus();
    }

    void onBrowseOutput(wxCommandEvent& event)
    {
        DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeOpen);
        dlg.setCaption(_("Select Output File"));
        dlg.setAffirmativeButtonLabel(_("OK"));
        if (dlg.ShowModal() != wxID_OK)
            return;
            
        m_output_textctrl->ChangeValue(dlg.getPath());
        m_output_textctrl->SetInsertionPointEnd();
        m_output_textctrl->SetFocus();
    }


    // -- wizard event handlers --

    void onPageChanged()
    {
        m_input_textctrl->SetValue(m_info->m_input_path);
        m_input_textctrl->SetInsertionPointEnd();
        m_input_textctrl->SetSelection(0, m_input_textctrl->GetLastPosition());
        setFocusDeferred(m_input_textctrl);
    }

    bool onPageChanging(bool forward)
    {
        // -- store the output set paths in m_info --
        m_info->m_output_path = m_output_textctrl->GetValue();

        if (m_info->m_output_path.length() > 0 &&
            !doOutputPathCheck(m_info->m_output_path))
        {
            return false;
        }

        wxString new_input_path = m_input_textctrl->GetValue();

        m_info->m_input_changed = false;
        if (m_info->m_input_path.CmpNoCase(new_input_path) != 0)
        {
            m_info->m_input_changed = true;
            m_info->m_input_path = new_input_path;
        }

        // -- try to open the input set, if it's not already open --
        if (m_info->m_input_changed || m_info->m_input_set.isNull())
        {
            m_info->m_input_changed = true;
            m_info->m_input_set = g_app->getDatabase()->openSet(towstr(m_info->m_input_path));
        }

        if (m_info->m_input_set.isNull())
        {
            m_info->m_input_path = wxT("");

            // -- could not open input transation file, bail out. --
            appMessageBox(_("The specified input transaction set could not be opened."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            return false;
        }


        return true;
    }

private:

    DupRecInfo* m_info;

    wxTextCtrl* m_input_textctrl;
    wxTextCtrl* m_output_textctrl;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(DupRecInputOutputPage, kcl::WizardPage)
    EVT_BUTTON(ID_BrowseInputButton, DupRecInputOutputPage::onBrowseInput)
    EVT_BUTTON(ID_BrowseOutputButton, DupRecInputOutputPage::onBrowseOutput)
END_EVENT_TABLE()






// -- wizard event handlers --

BEGIN_EVENT_TABLE(RemoveDupRecWizard, wxWindow)
    EVT_SIZE(RemoveDupRecWizard::onSize)
END_EVENT_TABLE()


RemoveDupRecWizard::RemoveDupRecWizard()
{
    m_info = new DupRecInfo;
}

RemoveDupRecWizard::~RemoveDupRecWizard()
{
    delete m_info;
}

bool RemoveDupRecWizard::initDoc(IFramePtr frame,
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
        return false;

    doc_site->setCaption(_("Duplicate Record Removal"));
    wxSize min_site_size = doc_site->getContainerWindow()->GetSize();
    doc_site->setMinSize(min_site_size.x, min_site_size.y);
    m_doc_site = doc_site;

    // grab default input file
    IDocumentSitePtr site = g_app->getMainFrame()->getActiveChild();
    if (site.isOk())
    {
        ITableDocPtr table_doc = site->getDocument();
        if (table_doc.isOk())
        {
            tango::ISetPtr set = table_doc->getBaseSet();

            if (!set->isTemporary())
                m_info->m_input_path = towx(set->getObjectPath());
        }        
    }

    // create wizard
    m_wizard = new kcl::Wizard(this);
    m_wizard->setFinishButtonLabel(_("Run"));
    
    // create main sizer
    m_sizer = new wxBoxSizer(wxVERTICAL);
    m_sizer->Add(m_wizard, 1, wxEXPAND);
    SetSizer(m_sizer);
    Layout();
    
    // create and add pages to the wizard
    DupRecInputOutputPage* page0 = new DupRecInputOutputPage(m_wizard, m_info);
    m_wizard->addPage(page0);
    m_wizard->startWizard(0);

    // -- connect signals --
    m_wizard->sigCancelPressed.connect(this, &RemoveDupRecWizard::onWizardCancelled);
    m_wizard->sigFinishPressed.connect(this, &RemoveDupRecWizard::onWizardFinished);

    return true;
}

wxWindow* RemoveDupRecWizard::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void RemoveDupRecWizard::setDocumentFocus()
{

}

void RemoveDupRecWizard::onWizardCancelled(kcl::Wizard* wizard)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}


static void onRemoveDupRecJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobStateFinished)
        return;

    bool success = false;

    // if there's an output set, open it
    tango::IIteratorPtr result_iter = job->getResultObject();
    if (result_iter.isOk())
    {
        tango::ISetPtr result_set = result_iter->getSet();
        if (result_set.isOk())
        {
            std::wstring output_path;
            output_path= result_set->getObjectPath();

            // if the object path begins with xtmp_, we have a temporary
            // table, so open it up
            if (isTemporaryTable(output_path))
            {
                ITableDocPtr doc = TableDocMgr::createTableDoc();
                doc->open(result_set, xcm::null);
                g_app->getMainFrame()->createSite(doc,
                                                  sitetypeNormal,
                                                  -1, -1, -1, -1);
            }
             else
            {
                DbDoc* dbdoc = g_app->getDbDoc();
                dbdoc->refresh();
            }

            success = true;

        }
    }

    if (!success)
    {
        appMessageBox(_("An output set could not be created."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
    }
}

void RemoveDupRecWizard::onWizardFinished(kcl::Wizard* wizard)
{
    if (m_info->m_input_set.isNull())
        return;

    // close the site
    g_app->getMainFrame()->closeSite(m_doc_site);


    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.query-job");

    // configure the job parameters
    kl::JsonNode params;
    params["input"].setString(towstr(m_info->m_input_path));
    params["output"].setString(L"xtmp_" + kl::getUniqueString());

    if (m_info->m_output_path.Length() > 0)
        params["output"].setString(towstr(m_info->m_output_path));

    params["distinct"].setBoolean(true);

    // set the job parameters and start the job
    job->getJobInfo()->setTitle(towstr(_("Remove Duplicate Records")));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(&onRemoveDupRecJobFinished);
    g_app->getJobQueue()->addJob(job, jobStateRunning);

    // TODO: see if tango::sqlAlwaysCopy flag still needs to be set;
    // this was set in the old query job:  job->setQuery(sql, tango::sqlAlwaysCopy);
    // if it still needs to be set, then figure out how to set the tango

}

void RemoveDupRecWizard::onSize(wxSizeEvent& event)
{
    Layout();
}

