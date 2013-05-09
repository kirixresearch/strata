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
#include "appcontroller.h"
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

void ReplaceRowsPanel::setParameters(const wxString& path, const wxString& expr, const wxString& field)
{
    m_path = path;
    m_iter = g_app->getDatabase()->query(towstr(path), L"", L"", L"", NULL);
    m_structure = m_iter->getStructure();
    m_default_expr = expr;
    m_default_field = field;
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




    populate();
    
    // set the expression
    m_expr_panel->setExpression(m_default_expr);
    
    // set the initial field
    int idx = m_field_choice->FindString(m_default_field);
    if (m_default_field.Length() > 0 && idx >= 0)
        m_field_choice->Select(idx);


    //  make panel caption
    wxString caption = _("Update");

    if (!isTemporaryTable(towstr(m_path)))
    {
        caption += wxT(" - [");
        caption += m_path;
        caption += wxT("]");
    }


    m_doc_site->setCaption(caption);


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

    // populate choicebox
    tango::IColumnInfoPtr colinfo;

    std::vector<wxString> fields;

    m_field_choice->Clear();
    int i, col_count = m_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        colinfo = m_structure->getColumnInfoByIdx(i);
        fields.push_back(makeProper(colinfo->getName()));
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
    tango::IColumnInfoPtr colinfo = m_structure->getColumnInfo(towstr(replace_field));
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
            if (Locale::parseDateTime(replace_value, &y, &m, &d, &hh, &mm, &ss))
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

    wxString replace_field = m_field_choice->GetStringSelection();
    tango::IColumnInfoPtr colinfo = m_structure->getColumnInfo(towstr(replace_field));
    
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


    wxString replace_value = m_replace_text->GetValue();
    wxString replace_field = m_field_choice->GetStringSelection();

    tango::IColumnInfoPtr colinfo = m_structure->getColumnInfo(towstr(replace_field));
    if (colinfo.isOk())
    {
        int type = m_structure->getExprType(towstr(replace_value));
        valid = tango::isTypeCompatible(type, colinfo->getType());
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

static void onUpdateJobFinished(jobs::IJobPtr job)
{
    kl::JsonNode params;
    params.fromString(job->getParameters());
    std::wstring input = params["input"];

    FrameworkEvent* cfw_event = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_REFRESH);
    cfw_event->s_param = input;
    g_app->getMainFrame()->postEvent(cfw_event);
}

void ReplaceRowsPanel::onOKPressed(ExprBuilderPanel* panel)
{
    bool is_value = false;
    
    wxString replace_field = m_field_choice->GetStringSelection();
    tango::IColumnInfoPtr colinfo = m_structure->getColumnInfo(towstr(replace_field));
    
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

    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.update-job");

    kl::JsonNode params;
    params["input"].setString(towstr(m_path));
    if (condition.Length() > 0)
        params["where"].setString(towstr(condition));

    params["set"].setArray();
    kl::JsonNode update_info = params["set"].appendElement();
    update_info["column"].setString(towstr(replace_field));
    update_info["expression"].setString(towstr(replace_value));
    
    job->getJobInfo()->setTitle(towstr(_("Update Values")));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(&onUpdateJobFinished);
    g_app->getJobQueue()->addJob(job, jobStateRunning);

    g_app->getMainFrame()->closeSite(m_doc_site);
}

void ReplaceRowsPanel::onCancelPressed(ExprBuilderPanel* panel)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}

