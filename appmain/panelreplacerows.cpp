/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-12
 *
 */


#include "appmain.h"
#include "panelreplacerows.h"
#include "exprbuilder.h"
#include "jobquery.h"
#include "appcontroller.h"
#include "tabledoc.h"
#include <algorithm>


enum
{
    ID_ReplaceText = wxID_HIGHEST + 1,
    ID_FieldChoice
};




BEGIN_EVENT_TABLE(ReplaceRowsPanel, wxPanel)
    EVT_CHOICE(ID_FieldChoice, ReplaceRowsPanel::onFieldChoiceChanged)
    EVT_TEXT(ID_ReplaceText, ReplaceRowsPanel::onReplaceTextChanged)
END_EVENT_TABLE()


ReplaceRowsPanel::ReplaceRowsPanel()
{
}

ReplaceRowsPanel::~ReplaceRowsPanel()
{
}

// -- IDocument --
bool ReplaceRowsPanel::initDoc(cfw::IFramePtr frame,
                               cfw::IDocumentSitePtr site,
                               wxWindow* docsite_wnd,
                               wxWindow* panesite_wnd)
{
    m_doc_site = site;

    if (!Create(docsite_wnd, -1))
        return false;

    site->setCaption(_("Update Records"));

    wxStaticText* static_replace_text = new wxStaticText(this, -1, _("When the following formula is true, replace values in"), wxDefaultPosition, wxDefaultSize);
    wxStaticText* static_replace_with = new wxStaticText(this, -1, _("with"), wxDefaultPosition, wxDefaultSize);
    m_field_choice = new wxChoice(this, ID_FieldChoice, wxDefaultPosition, wxSize(120,-1));
    m_replace_text = new wxTextCtrl(this, ID_ReplaceText);
    m_valid_control = new kcl::ValidControl(this, -1, wxDefaultPosition, wxSize(16,16));
    m_valid_control->setValidBitmap(GETBMP(gf_checkmark_16));
    m_valid_control->setInvalidBitmap(GETBMP(gf_x_16));
    m_valid_control->setBorderVisible(false);
    m_valid_control->setLabelVisible(false);

    wxBoxSizer* replace_sizer = new wxBoxSizer(wxHORIZONTAL);
    replace_sizer->Add(static_replace_text, 0, wxALIGN_CENTER);
    replace_sizer->AddSpacer(5);
    replace_sizer->Add(m_field_choice, 0, wxEXPAND);
    replace_sizer->AddSpacer(5);
    replace_sizer->Add(static_replace_with, 0, wxALIGN_CENTER);
    replace_sizer->AddSpacer(5);
    replace_sizer->Add(m_replace_text, 1, wxEXPAND);
    replace_sizer->AddSpacer(5);
    replace_sizer->Add(m_valid_control, 0, wxALIGN_CENTER);

    m_expr_panel = new ExprBuilderPanel;
    m_expr_panel->setTypeOnly(tango::typeBoolean);
    m_expr_panel->setEmptyOk(true);
    m_expr_panel->sigOkPressed.connect(this, &ReplaceRowsPanel::onOKPressed);
    m_expr_panel->sigCancelPressed.connect(this, &ReplaceRowsPanel::onCancelPressed);
    m_expr_panel->create(this);
    m_expr_panel->setOKText(_("Run"));

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(replace_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->Add(m_expr_panel, 1, wxEXPAND);
    SetSizer(main_sizer);
    Layout();

    checkEnableRun();
    
    return true;
}

wxWindow* ReplaceRowsPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ReplaceRowsPanel::setDocumentFocus()
{
    m_replace_text->SetFocus();
}

void ReplaceRowsPanel::setParams(tango::ISetPtr set, const wxString& expr, const wxString& field)
{
    m_set = set;
    m_iter = m_set->createIterator(L"", L"", NULL);
    populate();
    
    // set the expression
    m_expr_panel->setExpression(expr);
    
    // set the initial field
    int idx = m_field_choice->FindString(field);
    if (field.Length() > 0 && idx >= 0)
        m_field_choice->Select(idx);

    // validate the expression builder and panel
    m_expr_panel->validate();
    validate();
    checkEnableRun();

    // -- make panel caption --
    wxString caption = _("Update");

    if (set.isOk())
    {
        if (!set->isTemporary())
        {
            caption += wxT(" - [");
            caption += towx(set->getObjectPath());
            caption += wxT("]");
        }
    }

    m_doc_site->setCaption(caption);
    setDocumentFocus();
}


void ReplaceRowsPanel::populate()
{
    m_expr_panel->setIterator(m_iter);

    // -- populate choicebox --
    tango::IStructurePtr structure = m_set->getStructure();
    tango::IColumnInfoPtr colinfo;

    std::vector<wxString> fields;

    m_field_choice->Clear();
    int i, col_count = structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        colinfo = structure->getColumnInfoByIdx(i);
        fields.push_back(cfw::makeProper(towx(colinfo->getName())));
    }

    std::sort(fields.begin(), fields.end());

    std::vector<wxString>::iterator it;
    for (it = fields.begin(); it != fields.end(); ++it)
        m_field_choice->Append(*it);
}

bool ReplaceRowsPanel::isValidValue()
{
    wxString replace_value = m_replace_text->GetValue();
    replace_value.Trim();
    replace_value.Trim(TRUE);
    
    wxString replace_field = m_field_choice->GetStringSelection();
    tango::IStructurePtr structure = m_set->getStructure();
    tango::IColumnInfoPtr colinfo = structure->getColumnInfo(towstr(replace_field));
    if (colinfo.isNull())
        return false;

    switch (colinfo->getType())
    {
        case tango::typeWideCharacter:
        case tango::typeCharacter:
            if (replace_value.Length() > (size_t)colinfo->getWidth())
                return false;
            return true;

        case tango::typeDate:
        case tango::typeDateTime:
        {
            int y, m, d, hh, mm, ss;
            if (cfw::Locale::parseDateTime(replace_value, &y, &m, &d, &hh, &mm, &ss))
                return true;

            return false;
        }

        case tango::typeInteger:
        case tango::typeNumeric:
        case tango::typeDouble:
        {
            if (replace_value.length() == 0)
                return false;

            const wxChar* p = replace_value.c_str();

            while (*p)
            {
                if (!wxStrchr(wxT("0123456789.,-"), *p))
                    return false;

                p++;
            }

            return true;
        }

        default:
            return false;
    }

    return false;
}

void ReplaceRowsPanel::checkEnableRun()
{
    if (!m_expr_panel)
        return;
    
    if (m_set.isNull())
        return;
    
    wxString replace_field = m_field_choice->GetStringSelection();
    tango::IStructurePtr structure = m_set->getStructure();
    tango::IColumnInfoPtr colinfo = structure->getColumnInfo(towstr(replace_field));
    
    if (replace_field.IsEmpty() || colinfo.isNull())
    {
        m_expr_panel->setOKEnabled(false);
        return;
    }

    m_expr_panel->setOKEnabled(true);
}

bool ReplaceRowsPanel::validate(bool* value)
{
    bool valid = false;

    if (value)
        *value = false;

    if (m_iter.isOk())
    {
        wxString replace_value = m_replace_text->GetValue();
        wxString replace_field = m_field_choice->GetStringSelection();

        tango::IStructurePtr structure = m_iter->getStructure();
        tango::IColumnInfoPtr colinfo = structure->getColumnInfo(towstr(replace_field));
        if (colinfo.isOk())
        {
            tango::objhandle_t handle = m_iter->getHandle(towstr(replace_value));
            if (handle)
            {
                valid = isTypeCompatible(m_iter->getType(handle), colinfo->getType());
                m_iter->releaseHandle(handle);
            }
        }
    }

    m_valid_control->setValidLabel(_("Valid"));

    if (!valid && isValidValue())
    {
        if (value)
            *value = true;

        m_valid_control->setValidLabel(_("Value"));
        valid = true;
    }

    m_valid_control->setValid(valid);

    return valid;
}

void ReplaceRowsPanel::onReplaceTextChanged(wxCommandEvent& evt)
{
    validate();
}


void ReplaceRowsPanel::onFieldChoiceChanged(wxCommandEvent& evt)
{
    validate();
    checkEnableRun();
    m_replace_text->SetFocus();
}


static void onReplaceJobFinished(cfw::IJobPtr job)
{
    wxString target_set_id = job->getExtraString();

    // -- iterate through document sites, and update tabledocs --
    cfw::IDocumentSiteEnumPtr docsites;
    cfw::IDocumentSitePtr site;
    ITableDocPtr table_doc;

    docsites = g_app->getMainFrame()->getDocumentSites(cfw::sitetypeNormal);

    int site_count = docsites->size();
    for (int i = 0; i < site_count; ++i)
    {
        site = docsites->getItem(i);
        table_doc = site->getDocument();
        if (table_doc.isOk())
        {
            tango::ISetPtr browse_set = table_doc->getBrowseSet();
            tango::ISetPtr base_set = table_doc->getBaseSet();

            wxString base_setid;
            wxString browse_setid;
            if (base_set.isOk())
                base_setid = towx(base_set->getSetId());
            if (browse_set.isOk())
                browse_setid = towx(browse_set->getSetId());
            
            if ((browse_set.isOk() && browse_setid == target_set_id) ||
                (base_set.isOk() && base_setid == target_set_id))
            {
                table_doc->getGrid()->refresh(kcl::Grid::refreshAll);
            }
        }
    }
}


void ReplaceRowsPanel::onOKPressed(ExprBuilderPanel* panel)
{
    bool is_value = false;
    
    wxString replace_field = m_field_choice->GetStringSelection();
    tango::IStructurePtr structure = m_set->getStructure();
    tango::IColumnInfoPtr colinfo = structure->getColumnInfo(towstr(replace_field));
    
    if (replace_field.IsEmpty())
    {
        cfw::appMessageBox(_("No field has been selected to update.  Please select a valid field to update to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    if (colinfo.isNull())
    {
        cfw::appMessageBox(_("The selected field to update does not exist or is invalid.  Please select a valid field to update to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    if (!validate(&is_value))
    {
        cfw::appMessageBox(_("The replace value is not valid for the selected field.  Please enter a valid replace value for this field to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    wxString condition = m_expr_panel->getExpression();
    if (condition.IsEmpty())
    {
        int res =  cfw::appMessageBox(_("The formula entered for updating matching records is empty.  All values in the selected field will be replaced with\nthe specified replace value.  Are you sure you want to continue?"),
                                      APPLICATION_NAME,
                                      wxYES_NO | wxICON_EXCLAMATION | wxCENTER);
        if (res != wxYES)
            return;
    }

    wxString replace_value = m_replace_text->GetValue();
    wxString str;

    if (is_value)
    {
        // -- must adjust for value replace --

        int col_type = colinfo->getType();

        if (col_type == tango::typeCharacter ||
            col_type == tango::typeWideCharacter)
        {
            int len = replace_value.Length();

            wxString temp;
            temp.Alloc(len + 20);

            temp = wxT("\"");

            wxChar c;
            for (int i = 0; i < len; ++i)
            {
                c = replace_value.GetChar(i);
                if (c == wxT('"'))
                {
                    temp += wxT("\"\"");
                }
                 else
                {
                    temp += c;
                }
            }
            temp += wxT("\"");
            replace_value = temp;
        }
         else if (col_type == tango::typeDate ||
                  col_type == tango::typeDateTime)
        {
            int y, m, d, hh, mm, ss;

            if (!cfw::Locale::parseDateTime(replace_value,
                                  &y, &m, &d, &hh, &mm, &ss))
                return;

            if (hh == -1 || mm == -1)
            {
                replace_value = wxString::Format(wxT("DATE(%d,%d,%d)"),
                                                 y, m, d);
            }
             else
            {
                replace_value = wxString::Format(wxT("DATE(%d,%d,%d,%d,%d,%d)"),
                                       y, m, d, hh, mm, ss);
            }
        }
         else if (col_type == tango::typeInteger ||
                  col_type == tango::typeDouble ||
                  col_type == tango::typeNumeric)
        {
            replace_value.Replace(wxT(","), wxT("."));
        }
         else
        {
            return;
        }
    }

    wxString cmd = wxT("UPDATE ");
    cmd += towx(m_set->getObjectPath());
    cmd += wxT(" SET ");
    cmd += replace_field;
    cmd += wxT("=");
    cmd += replace_value;
    
    if (condition.Length() > 0)
    {
        cmd += wxT(" WHERE ");
        cmd += condition;
    }

    QueryJob* job = new QueryJob;
    job->getJobInfo()->setTitle(_("Update Values"));
    job->sigJobFinished().connect(&onReplaceJobFinished);
    job->setExtraString(towx(m_set->getSetId()));
    job->setQuery(cmd, tango::sqlPassThrough);
    
    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
    g_app->getMainFrame()->closeSite(m_doc_site);
}


void ReplaceRowsPanel::onCancelPressed(ExprBuilderPanel* panel)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}


