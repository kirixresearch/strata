/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-12
 *
 */


#include "appmain.h"
#include "panelreplacerows.h"
#include "exprbuilder.h"
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

void ReplaceRowsPanel::setParameters(const wxString& path, const wxString& tabledoc_path_to_refresh, const wxString& expr, const wxString& field)
{
    m_path = path;
    m_tabledoc_path_to_refresh = tabledoc_path_to_refresh;
    m_iter = g_app->getDatabase()->query(towstr(path), L"", L"", L"", NULL);
    setStructure(m_iter->getStructure());
    m_default_expr = expr;
    m_default_field = field;
}

void ReplaceRowsPanel::setStructure(const xd::Structure& structure)
{
    m_structure = structure;
}

void ReplaceRowsPanel::setValidationStructure(const xd::Structure& structure)
{
    m_structure_validation = structure;
}

bool ReplaceRowsPanel::initDoc(IFramePtr frame,
                               IDocumentSitePtr site,
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

    wxSize s1 = m_replace_text->GetSize();
    wxSize s2 = m_replace_text->GetTextExtent("XXXXXXXXXXXXXXX");
    s1.x = wxMax(s1.x, s2.x);
    replace_sizer->SetItemMinSize(m_replace_text, s1);


    m_expr_panel = new ExprBuilderPanel;
    m_expr_panel->setTypeOnly(xd::typeBoolean);
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




    populate();
    
    // set the expression
    m_expr_panel->setExpression(m_default_expr);
    
    // set the initial field
    int idx = m_field_choice->FindString(m_default_field);
    if (m_default_field.Length() > 0 && idx >= 0)
        m_field_choice->Select(idx);


    //  make panel caption
    wxString caption = _("Update");

    if (!xd::isTemporaryPath(towstr(m_path)))
    {
        caption += wxT(" - [");
        caption += m_path;
        caption += wxT("]");
    }


    m_doc_site->setCaption(caption);

    wxSize min_client_size = main_sizer->GetMinSize();
    m_doc_site->getContainerWindow()->SetClientSize(min_client_size);
    m_doc_site->getContainerWindow()->SetMinClientSize(min_client_size);


    // validate the expression builder and panel

    m_expr_panel->validate();
    validate();
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


void ReplaceRowsPanel::populate()
{
    m_expr_panel->setStructure(m_structure);

    if (m_structure_validation.isOk())
    {
        m_expr_panel->setValidationStructure(m_structure_validation);
    }

    // populate choicebox

    std::vector<wxString> fields;

    m_field_choice->Clear();
    size_t i, col_count = m_structure.getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        fields.push_back(m_structure.getColumnName(i));
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
    xd::ColumnInfo colinfo;
    
    if (m_structure_validation.isOk())
    {
        colinfo = m_structure_validation.getColumnInfo(towstr(replace_field));
    }
    else
    {
        colinfo = m_structure.getColumnInfo(towstr(replace_field));
    }

    if (colinfo.isNull())
        return false;

    switch (colinfo.type)
    {
        case xd::typeWideCharacter:
        case xd::typeCharacter:
            if (replace_value.Length() > (size_t)colinfo.width)
                return false;
            return true;

        case xd::typeDate:
        case xd::typeDateTime:
        {
            int y, m, d, hh, mm, ss;
            if (Locale::parseDateTime(replace_value, &y, &m, &d, &hh, &mm, &ss))
                return true;

            return false;
        }

        case xd::typeInteger:
        case xd::typeNumeric:
        case xd::typeDouble:
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

    wxString replace_field = m_field_choice->GetStringSelection();
    const xd::ColumnInfo& colinfo = m_structure.getColumnInfo(towstr(replace_field)); // the replace field should validate against the regular structure (not necessarily the validation structure)
    
    if (replace_field.IsEmpty() || colinfo.isNull())
    {
        m_expr_panel->setOKEnabled(false);
        return;
    }

    m_expr_panel->setOKEnabled(true);
}

bool ReplaceRowsPanel::validate(bool* is_value)
{
    bool valid = false;

    if (is_value)
        *is_value = false;

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    wxString replace_value = m_replace_text->GetValue();
    wxString replace_field = m_field_choice->GetStringSelection();

    if (m_structure_validation.isOk())
    {
        const xd::ColumnInfo& colinfo = m_structure_validation.getColumnInfo(towstr(replace_field));
        if (colinfo.isOk())
        {
            int type = db->validateExpression(towstr(replace_value), m_structure_validation).type;
            valid = xd::isTypeCompatible(type, colinfo.type);
        }
    }
    else
    {
        const xd::ColumnInfo& colinfo = m_structure.getColumnInfo(towstr(replace_field));
        if (colinfo.isOk())
        {
            int type = db->validateExpression(towstr(replace_value), m_structure).type;
            valid = xd::isTypeCompatible(type, colinfo.type);
        }
    }


    m_valid_control->setValidLabel(_("Valid"));

    if (!valid && isValidValue())
    {
        if (is_value)
            *is_value = true;

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

static void onUpdateJobFinished(jobs::IJobPtr job)
{
    kl::JsonNode params;
    params.fromString(job->getParameters());
    std::wstring tabledoc_path_to_refresh = params["tabledoc_path_to_refresh"];

    FrameworkEvent* cfw_event = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_REFRESH);
    cfw_event->s_param = tabledoc_path_to_refresh;
    g_app->getMainFrame()->postEvent(cfw_event);
}

void ReplaceRowsPanel::onOKPressed(ExprBuilderPanel* panel)
{
    bool is_value = false;
    
    wxString replace_field = m_field_choice->GetStringSelection();
    const xd::ColumnInfo& colinfo = m_structure.getColumnInfo(towstr(replace_field));
    
    if (replace_field.IsEmpty())
    {
        appMessageBox(_("No field has been selected to update.  Please select a valid field to update to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    if (colinfo.isNull())
    {
        appMessageBox(_("The selected field to update does not exist or is invalid.  Please select a valid field to update to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    if (!validate(&is_value))
    {
        appMessageBox(_("The replace value is not valid for the selected field.  Please enter a valid replace value for this field to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    wxString condition = m_expr_panel->getExpression();
    if (condition.IsEmpty())
    {
        int res =  appMessageBox(_("The formula entered for updating matching records is empty.  All values in the selected field will be replaced with\nthe specified replace value.  Are you sure you want to continue?"),
                                      APPLICATION_NAME,
                                      wxYES_NO | wxICON_EXCLAMATION | wxCENTER);
        if (res != wxYES)
            return;
    }

    wxString replace_value = m_replace_text->GetValue();
    wxString str;

    if (is_value)
    {
        // must adjust for value replace
        if (colinfo.type == xd::typeCharacter || colinfo.type == xd::typeWideCharacter)
        {
            int len = replace_value.Length();

            wxString temp;
            temp.Alloc(len + 20);

            temp = "'";

            wxChar c;
            for (int i = 0; i < len; ++i)
            {
                c = replace_value.GetChar(i);
                if (c == '\'')
                {
                    temp += "''";
                }
                 else
                {
                    temp += c;
                }
            }
            temp += "'";
            replace_value = temp;
        }
         else if (colinfo.type == xd::typeDate || colinfo.type == xd::typeDateTime)
        {
            int y, m, d, hh, mm, ss;

            if (!Locale::parseDateTime(replace_value,
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
         else if (colinfo.type == xd::typeInteger ||
                  colinfo.type == xd::typeDouble ||
                  colinfo.type == xd::typeNumeric)
        {
            replace_value.Replace(wxT(","), wxT("."));
        }
         else
        {
            return;
        }
    }

    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.update-job");

    kl::JsonNode params;
    params["input"].setString(towstr(m_path));
    params["tabledoc_path_to_refresh"].setString(towstr(m_tabledoc_path_to_refresh));

    if (condition.Length() > 0)
        params["where"].setString(towstr(condition));

    params["set"].setArray();
    kl::JsonNode update_info = params["set"].appendElement();
    update_info["column"].setString(towstr(replace_field));
    update_info["expression"].setString(towstr(replace_value));

    job->getJobInfo()->setTitle(towstr(_("Update Values")));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(&onUpdateJobFinished);
    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);

    g_app->getMainFrame()->closeSite(m_doc_site);
}

void ReplaceRowsPanel::onCancelPressed(ExprBuilderPanel* panel)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}

