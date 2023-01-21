/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-02-26
 *
 */


#include "appmain.h"
#include "toolbars.h"
#include "paneloptions.h"
#include "tabledoc.h"
#include "webdoc.h"
#include "appcontroller.h"
#include "projectmgr.h"
#include "dlgpagesetup.h"
#include "../kcanvas/kcanvas.h"
#include "../kcanvas/property.h"
#include "../kcanvas/graphicsobj.h"


enum
{
    // buttonbar button ids
    ID_GeneralOptionsButton = wxID_HIGHEST + 1,
    ID_InternetOptionsButton,
    ID_PrivacyOptionsButton,
    ID_DatasheetOptionsButton,
    ID_ReportOptionsButton,
    ID_ScriptOptionsButton,
    
    ID_RestoreDefaultsButton
};

inline wxString getLastOpenProjectLabel() { return _("<Last Open Project>"); }


wxBoxSizer* createLabelTextControlSizer(wxWindow* parent,
                                        const wxString& label,
                                        wxTextCtrl** textctrl,
                                        wxWindowID textctrl_id,
                                        const wxString& textctrl_text,
                                        int spacer)
{
    // we have to do this because of the way wx handles events...
    // we don't want the text changed event to work yet
    *textctrl = NULL;
    *textctrl = new wxTextCtrl(parent, textctrl_id, textctrl_text);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(new wxStaticText(parent, -1, label), 0, wxALIGN_CENTER);
    if (spacer > 0)
        sizer->AddSpacer(spacer);
    sizer->Add(*textctrl, 1, wxEXPAND);
    
    return sizer;
}

void makeSizerLabelsSameSize(wxBoxSizer* sizer1,
                             wxBoxSizer* sizer2,
                             wxBoxSizer* sizer3 = NULL,
                             wxBoxSizer* sizer4 = NULL,
                             wxBoxSizer* sizer5 = NULL,
                             wxBoxSizer* sizer6 = NULL,
                             wxBoxSizer* sizer7 = NULL)
{
    size_t item0 = 0;
    wxSizerItem* item;
    wxWindow* wnd;
    
    wxStaticText *st1 = NULL,
                 *st2 = NULL,
                 *st3 = NULL,
                 *st4 = NULL,
                 *st5 = NULL,
                 *st6 = NULL,
                 *st7 = NULL;
    
    if (sizer1) { item = sizer1->GetItem(item0);
                  if (item) { wnd = item->GetWindow(); }
                  if (item && wnd && wnd->IsKindOf(CLASSINFO(wxStaticText))) { st1 = (wxStaticText*)wnd; }
                }
    if (sizer2) { item = sizer2->GetItem(item0);
                  if (item) { wnd = item->GetWindow(); }
                  if (item && wnd && wnd->IsKindOf(CLASSINFO(wxStaticText))) { st2 = (wxStaticText*)wnd; }
                }
    if (sizer3) { item = sizer3->GetItem(item0);
                  if (item) { wnd = item->GetWindow(); }
                  if (item && wnd && wnd->IsKindOf(CLASSINFO(wxStaticText))) { st3 = (wxStaticText*)wnd; }
                }
    if (sizer4) { item = sizer4->GetItem(item0);
                  if (item) { wnd = item->GetWindow(); }
                  if (item && wnd && wnd->IsKindOf(CLASSINFO(wxStaticText))) { st4 = (wxStaticText*)wnd; }
                }
    if (sizer5) { item = sizer5->GetItem(item0);
                  if (item) { wnd = item->GetWindow(); }
                  if (item && wnd && wnd->IsKindOf(CLASSINFO(wxStaticText))) { st5 = (wxStaticText*)wnd; }
                }
    if (sizer6) { item = sizer6->GetItem(item0);
                  if (item) { wnd = item->GetWindow(); }
                  if (item && wnd && wnd->IsKindOf(CLASSINFO(wxStaticText))) { st6 = (wxStaticText*)wnd; }
                }
    if (sizer7) { item = sizer7->GetItem(item0);
                  if (item) { wnd = item->GetWindow(); }
                  if (item && wnd && wnd->IsKindOf(CLASSINFO(wxStaticText))) { st7 = (wxStaticText*)wnd; }
                }
    
    wxSize s = getMaxTextSize(st1, st2, st3, st4, st5, st6, st7);
    if (sizer1 && st1)
        sizer1->SetItemMinSize(st1, s);
    if (sizer2 && st2)
        sizer2->SetItemMinSize(st2, s);
    if (sizer3 && st3)
        sizer3->SetItemMinSize(st3, s);
    if (sizer4 && st4)
        sizer4->SetItemMinSize(st4, s);
    if (sizer5 && st5)
        sizer5->SetItemMinSize(st5, s);
    if (sizer6 && st6)
        sizer6->SetItemMinSize(st6, s);
    if (sizer7 && st7)
        sizer7->SetItemMinSize(st7, s);
}


// General Options page

class GeneralOptionsPage : public wxPanel
{
private:

    enum
    {
        ID_StartupLocationTextCtrl = wxID_HIGHEST+1,
        ID_UseCurrentPageButton,
        ID_UseBlankPageButton,
        ID_ResetPanelsButton,
        ID_StartupProjectMgrRadio,
        ID_StartupOpenProjectRadio,
        ID_OpenProjectChoice,
        ID_ToolbarStyleChoice,
        ID_CheckForUpdatesCheckBox
    };
    
    enum ToolbarChoice
    {
        ToolbarChoice_SmallIcons = 0,
        ToolbarChoice_LargeIcons = 1,
        ToolbarChoice_LargeIconsWithText = 2
    };
    
public:

    GeneralOptionsPage(wxWindow* parent, PrefInfo* pi) : wxPanel(parent), m_pi(pi)
    {
        SetWindowStyle(GetWindowStyle() | wxTAB_TRAVERSAL);


        m_startup_projectmgr = new wxRadioButton(this,
                                                 ID_StartupProjectMgrRadio,
                                                 _("Show Project Manager"),
                                                 wxDefaultPosition,
                                                 wxDefaultSize,
                                                 wxRB_GROUP);
        

        // create open project sizer
        
        m_startup_openproject = new wxRadioButton(this,
                                             ID_StartupOpenProjectRadio,
                                             _("Automatically Load Project:"));

        m_openproject = new wxChoice(this,
                                     ID_OpenProjectChoice,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     0,
                                     NULL);
        
        // add the 'Last Open Project' item to the dropdown list
        m_openproject->Append(getLastOpenProjectLabel());
        
        // sort and add projects to the dropdown list
        m_projects = m_projmgr.getProjectEntries();
        std::sort(m_projects.begin(), m_projects.end(), ProjectInfoLess());

        std::vector<ProjectInfo>::iterator it;
        for (it = m_projects.begin(); it != m_projects.end(); ++it)
            m_openproject->Append(it->name);
        
        // select the first item in the dropdown list by default
        m_openproject->SetSelection(0);
        
        wxBoxSizer* openproject_sizer = new wxBoxSizer(wxHORIZONTAL);
        openproject_sizer->Add(m_startup_openproject, 0, wxALIGN_CENTER);
        openproject_sizer->AddSpacer(10);
        openproject_sizer->Add(m_openproject, 1, wxALIGN_CENTER);
        
        
        // create startup sizer
        
        wxStaticBox* startup_static_box = new wxStaticBox(this, 
                                                -1,
                                                _("Startup"));
        wxStaticBoxSizer* startup_sizer;
        startup_sizer = new wxStaticBoxSizer(startup_static_box,
                                             wxVERTICAL);
        startup_sizer->AddSpacer(10);
        startup_sizer->Add(m_startup_projectmgr, 0, wxLEFT | wxRIGHT, 10);
        startup_sizer->AddSpacer(8);
        startup_sizer->Add(openproject_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        startup_sizer->AddSpacer(10);
        
        
        // create startup location sizer
        
        wxBoxSizer* loc_text_sizer = createLabelTextControlSizer(
                                        this,
                                        _("Location:"),
                                        &m_startup_location_textctrl,
                                        ID_StartupLocationTextCtrl,
                                        wxEmptyString,
                                        10);
        
        m_use_current_page_button = new wxButton(this, ID_UseCurrentPageButton,
                                                 _("Use &Current Page"));
        m_use_blank_page_button = new wxButton(this, ID_UseBlankPageButton,
                                               _("Use &Blank Page"));
        
        wxBoxSizer* loc_button_sizer = new wxBoxSizer(wxHORIZONTAL);
        loc_button_sizer->AddStretchSpacer(1);
        loc_button_sizer->Add(m_use_current_page_button, 0, wxEXPAND);
        loc_button_sizer->AddSpacer(8);
        loc_button_sizer->Add(m_use_blank_page_button, 0, wxEXPAND);
        
        wxStaticBox* location_static_box = new wxStaticBox(this, 
                                                         -1,
                                                         _("Home Page"));
        wxStaticBoxSizer* startup_location_sizer;
        startup_location_sizer = new wxStaticBoxSizer(location_static_box,
                                                      wxVERTICAL);
        startup_location_sizer->AddSpacer(10);
        startup_location_sizer->Add(loc_text_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        startup_location_sizer->AddSpacer(8);
        startup_location_sizer->Add(loc_button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        startup_location_sizer->AddSpacer(10);
        
        
        // create toolbar look and feel sizer
        
        wxStaticText* label_style = new wxStaticText(this,
                                                     -1,
                                                     _("Navigation Toolbar Style:"),
                                                     wxDefaultPosition,
                                                     wxDefaultSize);
        
        wxString toolbar_styles[3];
        toolbar_styles[ToolbarChoice_SmallIcons] = _("Small Icons");
        toolbar_styles[ToolbarChoice_LargeIcons] = _("Large Icons");
        toolbar_styles[ToolbarChoice_LargeIconsWithText] = _("Large Icons with Text");
        
        m_toolbar_style = new wxChoice(this,
                                       ID_ToolbarStyleChoice,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       3,
                                       toolbar_styles);
        
        wxBoxSizer* toolbar_laf_sizer = new wxBoxSizer(wxHORIZONTAL);
        toolbar_laf_sizer->Add(label_style, 0, wxALIGN_CENTER);
        toolbar_laf_sizer->AddSpacer(10);
        toolbar_laf_sizer->Add(m_toolbar_style, 1, wxALIGN_CENTER);
        
        // create reset sizer
        
        wxButton* m_reset_panels_button = new wxButton(this, ID_ResetPanelsButton, _("Reset Toolbars and Panels"));
        wxBoxSizer* reset_sizer = new wxBoxSizer(wxHORIZONTAL);
        reset_sizer->AddStretchSpacer();
        reset_sizer->Add(m_reset_panels_button);
        
        // create look and feel sizer
        
        wxStaticBox* laf_static_box = new wxStaticBox(this, -1, _("Toolbars and Panels"));
        wxStaticBoxSizer* laf_sizer = new wxStaticBoxSizer(laf_static_box, wxVERTICAL);
        laf_sizer->AddSpacer(10);
        laf_sizer->Add(toolbar_laf_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        laf_sizer->AddSpacer(8);
        laf_sizer->Add(reset_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        laf_sizer->AddSpacer(10);
        
        m_check_for_updates = new wxCheckBox(this,
                                             ID_CheckForUpdatesCheckBox,
                                             _("Automatically check for updates (recommended)"));
        
        
        // create main sizer
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->Add(startup_sizer, 0, wxEXPAND);
        main_sizer->AddSpacer(10);
        main_sizer->Add(startup_location_sizer, 0, wxEXPAND);
        main_sizer->AddSpacer(10);
        main_sizer->Add(laf_sizer, 0, wxEXPAND);
        main_sizer->AddSpacer(15);
        main_sizer->Add(m_check_for_updates, 0, wxLEFT, 10);

        if (wxString(APP_UPDATE_URL).Length() == 0)
        {
            main_sizer->Hide(m_check_for_updates);
        }

        SetSizer(main_sizer);
        Layout();
        
        initControlValues();
    }
    
    int toolbarChoice2pref(int choice_idx)
    {
        switch (choice_idx)
        {
            case ToolbarChoice_SmallIcons:
                return prefToolbarSmallIcons;

            case ToolbarChoice_LargeIcons:
                return prefToolbarLargeIcons;

            case ToolbarChoice_LargeIconsWithText:
                return prefToolbarLargeIconsWithText;
        }
        
        return getAppPrefsDefaultLong(wxT("general.toolbar.style"));
    }
    
    int pref2toolbarChoice(int pref_val)
    {
        switch (pref_val)
        {
            case prefToolbarSmallIcons:
                return ToolbarChoice_SmallIcons;

            case prefToolbarLargeIcons:
                return ToolbarChoice_LargeIcons;

            case prefToolbarLargeIconsWithText:
                return ToolbarChoice_LargeIconsWithText;
        }
        
        return ToolbarChoice_LargeIcons;
    }
    
    void updateEnabled()
    {
        m_openproject->Enable(m_startup_openproject->GetValue());
    }
    
    void initControlValues()
    {
        // we've removed the 'Do Nothing' startup action
        if (m_pi->general_startup_action == prefStartupNone)
            m_pi->general_startup_action = prefStartupProjectMgr;
        
        // select the appropriate radio button based
        // on the current preference settings
        switch (m_pi->general_startup_action)
        {
            case prefStartupOpenProject:
                {
                    m_startup_openproject->SetValue(true);
                    break;
                }
                
            default:
            case prefStartupProjectMgr:
                {
                    m_startup_projectmgr->SetValue(true);
                    break;
                }
        }
        
        if (m_pi->general_startup_open_last_project)
        {
            m_openproject->SetSelection(0);
        }
         else
        {
            // fill out the startup project combobox and select the
            // appropriate entry based on the current preference settings
            // (NOTE: project indexes start at 1 because the first item
            //        in the dropdown is the 'Last Open Project' item)
            int idx = 1;
            bool found = false;
            std::vector<ProjectInfo>::iterator it;
            for (it = m_projects.begin(); it != m_projects.end(); ++it)
            {
                if (kl::iequals(it->entry_name, towstr(m_pi->general_startup_connection)))
                {
                    found = true;
                    break;
                }
                
                ++idx;
            }
            
            if (found)
                m_openproject->SetSelection(idx);
        }
        
        // set the toolbar selection
        int toolbar_idx = pref2toolbarChoice(m_pi->general_toolbar_style);
        m_toolbar_style->SetSelection(toolbar_idx);
        
        // set the startup location text control's value
        m_startup_location_textctrl->SetValue(m_pi->general_startup_location);
        
        // set the check for updates checkbox
        m_check_for_updates->SetValue(m_pi->general_check_for_updates);
        
        // make sure the project choice control is enabled/disabled
        // based on which radio button is selected
        updateEnabled();
    }
    
    void serializeValues()
    {
        // startup action
        if (m_startup_openproject->GetValue())
            m_pi->general_startup_action = prefStartupOpenProject;
             else
            m_pi->general_startup_action = prefStartupProjectMgr;
        
        // startup project
        int sel = m_openproject->GetSelection();
        if (sel == 0 || sel == -1)
        {
            m_pi->general_startup_open_last_project = true;
            m_pi->general_startup_connection = L"";
        
            // 'Last Open Project' dropdown item was selected; set
            // the startup connection based on the current project's location
            std::wstring project_loc = towstr(g_app->getDatabaseLocation());
            std::vector<ProjectInfo>::iterator it;
            for (it = m_projects.begin(); it != m_projects.end(); ++it)
            {
                if (kl::iequals(it->location, project_loc))
                {
                    m_pi->general_startup_connection = it->entry_name;
                    break;
                }
            }
            
        }
         else
        {
            m_pi->general_startup_open_last_project = false;
            
            if (sel != -1)
                m_pi->general_startup_connection = m_projects[sel-1].entry_name;
                 else
                m_pi->general_startup_connection = wxEmptyString;
        }
        
        // startup location (web pages only)
        m_pi->general_startup_location = m_startup_location_textctrl->GetValue();
        
        // navigation toolbar style
        switch (m_toolbar_style->GetSelection())
        {
            case ToolbarChoice_SmallIcons:
                m_pi->general_toolbar_style = prefToolbarSmallIcons;
                break;
            
            default:
            case ToolbarChoice_LargeIcons:
                m_pi->general_toolbar_style = prefToolbarLargeIcons;
                break;

            case ToolbarChoice_LargeIconsWithText:
                m_pi->general_toolbar_style = prefToolbarLargeIconsWithText;
                break;
        }
        
        // check for updates
        m_pi->general_check_for_updates = m_check_for_updates->GetValue();
    }
    
    void onUseCurrentPageButtonClicked(wxCommandEvent& evt)
    {
        IDocumentSitePtr active_child;
        active_child = g_app->getMainFrame()->getActiveChild();
        if (active_child.isNull())
            return;
        
        IDocumentPtr doc = active_child->getDocument();
        if (doc.isNull())
            return;

        IWebDocPtr webdoc = doc;
        if (webdoc.isNull())
        {
            appMessageBox(_("The current page is not a web document so it cannot be used as your home page."));
            return;
        }
                        
        m_startup_location_textctrl->SetValue(doc->getDocumentLocation());
    }
    
    void onUseBlankPageButtonClicked(wxCommandEvent& evt)
    {
        m_startup_location_textctrl->SetValue(wxT("about:blank"));
    }
    
    void onResetPanelsButtonClicked(wxCommandEvent& evt)
    {
        int res = appMessageBox(_("Are you sure you want to reset the location and size of all toolbars and panels?"),
                                     _("Reset View?"),
                                     wxYES_NO |
                                     wxNO_DEFAULT |
                                     wxICON_QUESTION |
                                     wxCENTER,
                                     g_app->getMainWindow());
        if (res == wxNO)
            return;
        
        // reset the application to the default perspective
        g_app->getAppController()->resetToDefaultPerspective();
        
        // make sure the navigation toolbar style dropdown is updated
        m_pi->general_toolbar_style = getAppPrefsDefaultLong(wxT("general.toolbar.style"));
        m_toolbar_style->SetSelection(pref2toolbarChoice(m_pi->general_toolbar_style));
        
        // do preference refresh
        FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_PREFERENCES_SAVED);
        g_app->getMainFrame()->sendEvent(e);
    }
    
    void onStartupRadioClicked(wxCommandEvent& evt)
    {
        updateEnabled();
    }
    
    void restoreDefaultPrefs()
    {
        // get the default values from the default app preferences
        m_pi->general_startup_open_last_project = getAppPrefsDefaultBoolean(wxT("general.startup.open_last_project"));
        m_pi->general_startup_location = getAppPrefsDefaultString(wxT("general.location.home"));
        m_pi->general_startup_action = getAppPrefsDefaultLong(wxT("general.startup.default_action"));
        m_pi->general_toolbar_style = getAppPrefsDefaultLong(wxT("general.toolbar.style"));
        m_pi->general_check_for_updates = getAppPrefsDefaultBoolean(wxT("general.updater.check_for_updates"));
        
        // restore the default settings to controls
        m_openproject->SetSelection(m_pi->general_startup_open_last_project ? 0 : 1);
        m_startup_location_textctrl->SetValue(m_pi->general_startup_location);
        m_startup_openproject->SetValue(m_pi->general_startup_action == prefStartupOpenProject ? true : false);
        m_startup_projectmgr->SetValue(m_pi->general_startup_action == prefStartupProjectMgr ? true : false);
        m_toolbar_style->SetSelection(pref2toolbarChoice(m_pi->general_toolbar_style));
        m_check_for_updates->SetValue(m_pi->general_check_for_updates);
        updateEnabled();
    }

private:

    PrefInfo* m_pi;
    
    wxTextCtrl* m_startup_location_textctrl;
    wxButton* m_use_current_page_button;
    wxButton* m_use_blank_page_button;
    wxButton* m_reset_panels_button;
    
    wxChoice* m_openproject;
    wxChoice* m_toolbar_style;

    wxRadioButton* m_startup_projectmgr;
    wxRadioButton* m_startup_openproject;
    
    wxCheckBox* m_check_for_updates;
    
    ProjectMgr m_projmgr;
    std::vector<ProjectInfo> m_projects;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(GeneralOptionsPage, wxPanel)
    EVT_BUTTON(ID_UseCurrentPageButton, GeneralOptionsPage::onUseCurrentPageButtonClicked)
    EVT_BUTTON(ID_UseBlankPageButton, GeneralOptionsPage::onUseBlankPageButtonClicked)
    EVT_BUTTON(ID_ResetPanelsButton, GeneralOptionsPage::onResetPanelsButtonClicked)
    EVT_RADIOBUTTON(ID_StartupProjectMgrRadio, GeneralOptionsPage::onStartupRadioClicked)
    EVT_RADIOBUTTON(ID_StartupOpenProjectRadio, GeneralOptionsPage::onStartupRadioClicked)
END_EVENT_TABLE()








// helper function

void doColorPanelPopup(wxWindow* parent,
                       kcl::PopupContainer** container,
                       kcl::ColorPanel** panel,
                       const wxColor& default_color);

const int BUTTON_SIZE = 20;
const int COLOR_SQUARE_SIZE = 14;
const int DROPDOWN_WIDTH = 9;

// from kcl::Button
static const char* xpm_dropdown_arrow[] = {
"5 3 2 1",
"  c None",
"! c #000000",
"!!!!!",
" !!! ",
"  !  "};

wxBitmap g_droparrow_bmp;




// Datasheet Options page

class DatasheetOptionsPage : public wxPanel,
                             public xcm::signal_sink
{
private:

    enum
    {
        ID_HorzLineCheckBox = wxID_HIGHEST + 1,
        ID_VertLineCheckBox,
        ID_GreenBarCheckBox,
        ID_GreenBarSpinCtrl,
        ID_TextWrapCheckBox,
        ID_BooleanCheckBoxCheckBox,
        ID_BackgroundColorButton,
        ID_ForegroundColorButton,
        ID_LineColorButton,
        ID_InvalidAreaColorButton,
        ID_GreenBarColorButton,
        ID_FontCombo,
        ID_FontSizeCombo,

        // parameter of the grid we are changing
        gridBackgroundColor,
        gridForegroundColor,
        gridLineColor,
        gridInvalidAreaColor,
        gridGreenBarColor
    };

private:

    kcl::Button* createColorButton(wxWindow* parent, int id, const wxColor& color)
    {
        kcl::Button* b = new kcl::Button(this, id, wxDefaultPosition,
                                         wxSize(BUTTON_SIZE+DROPDOWN_WIDTH,BUTTON_SIZE));
        b->setMode(kcl::Button::modeFlat);
        b->setRightClickAllowed(true);
        b->setColor(kcl::Button::colorNormalFg, color);
        b->sigCustomRender.connect(this, &DatasheetOptionsPage::onColorButtonRender);
        return b;
    }

    void onColorButtonRender(kcl::Button* button,
                             wxDC* dc,
                             int flags)
    {
        int xoff = 3, yoff = 3;
        int cli_width, cli_height;
        button->GetClientSize(&cli_width, &cli_height);
        wxColour highlight_color = kcl::getHighlightColor();
        
        // clear old button rendering
        dc->Clear();

        if (flags & kcl::Button::buttonDepressed)
        {
            // draw selected background (darker blue)
            dc->SetPen(highlight_color);
            dc->SetBrush(kcl::stepColor(highlight_color, 140));
            dc->DrawRectangle(0, 0, cli_width, cli_height);
            
            // draw drop arrow
            dc->DrawBitmap(g_droparrow_bmp,
                           xoff+COLOR_SQUARE_SIZE+xoff,
                           (cli_height-g_droparrow_bmp.GetHeight())/2 + 1,
                           true);
        }
         else
        {
            wxPoint pt_mouse = ::wxGetMousePosition();
            pt_mouse = ScreenToClient(pt_mouse);

            wxRect r(button->GetPosition(), button->GetClientSize());
            if (r.Contains(pt_mouse))
            {
                // draw hover background (lighter blue)
                dc->SetPen(highlight_color);
                dc->SetBrush(kcl::stepColor(highlight_color, 170));
                dc->DrawRectangle(0, 0, cli_width, cli_height);
                
                // draw drop arrow
                dc->DrawBitmap(g_droparrow_bmp,
                               xoff+COLOR_SQUARE_SIZE+xoff,
                               (cli_height-g_droparrow_bmp.GetHeight())/2 + 1,
                               true);
            }
        }

        // draw the color square button
        dc->SetPen(*wxGREY_PEN);
        dc->SetBrush(button->getColor(kcl::Button::colorNormalFg));
        dc->DrawRectangle(xoff, yoff, COLOR_SQUARE_SIZE, COLOR_SQUARE_SIZE);
    }

public:

    DatasheetOptionsPage(wxWindow* parent, PrefInfo* pi) : wxPanel(parent), m_pi(pi)
    {
        SetWindowStyle(GetWindowStyle() | wxTAB_TRAVERSAL);

        m_button_selected = 0;
        m_color_panel_cont = NULL;
        m_color_panel = NULL;
        
        
        // create top left options sizer
        
        m_horz_line_check = new wxCheckBox(this, ID_HorzLineCheckBox, _("Show horizontal grid lines"));
        m_vert_line_check = new wxCheckBox(this, ID_VertLineCheckBox, _("Show vertical grid lines"));
        m_greenbar_check = new wxCheckBox(this, ID_GreenBarCheckBox, _("Show shaded rows every..."));

        wxString val = wxString::Format(wxT("%d"), m_pi->grid_greenbar_interval);
        m_greenbar_spin_ctrl = new wxSpinCtrl(this, ID_GreenBarSpinCtrl, val, wxDefaultPosition, wxSize(65,-1));

        m_text_wrap_check = new wxCheckBox(this, ID_TextWrapCheckBox, _("Wrap text in character fields"));
        m_boolean_checkbox_check = new wxCheckBox(this, ID_BooleanCheckBoxCheckBox, _("Use check boxes for boolean fields"));
        
        wxStaticBox* left_box = new wxStaticBox(this, -1, _("View and Formatting"));
        wxStaticBoxSizer* top_left_sizer = new wxStaticBoxSizer(left_box, wxVERTICAL);
        top_left_sizer->AddSpacer(10);
        top_left_sizer->Add(m_horz_line_check, 0, wxLEFT | wxRIGHT, 10);
        top_left_sizer->AddSpacer(8);
        top_left_sizer->Add(m_vert_line_check, 0, wxLEFT | wxRIGHT, 10);
        top_left_sizer->AddSpacer(8);
        top_left_sizer->Add(m_greenbar_check, 0, wxLEFT | wxRIGHT, 10);
        top_left_sizer->AddSpacer(8);
        top_left_sizer->Add(m_greenbar_spin_ctrl, 0, wxLEFT, 25);
        top_left_sizer->AddSpacer(8);
        top_left_sizer->Add(m_text_wrap_check, 0, wxLEFT | wxRIGHT, 10);
        top_left_sizer->AddSpacer(8);
        top_left_sizer->Add(m_boolean_checkbox_check, 0, wxLEFT | wxRIGHT, 10);
        top_left_sizer->AddSpacer(10);


        // create foreground color sizer
        
        m_foreground_color_button = createColorButton(this,
                                          ID_ForegroundColorButton,
                                          m_pi->grid_foreground_color);

        wxStaticText* foreground_static_text = new wxStaticText(this,
                                                     -1, 
                                                     _("Text Color"));

        wxBoxSizer* foreground_color_sizer = new wxBoxSizer(wxHORIZONTAL);
        foreground_color_sizer->Add(m_foreground_color_button, 0, wxEXPAND | wxRIGHT, 4);
        foreground_color_sizer->Add(foreground_static_text, 0, wxALIGN_CENTER);


        // create background color sizer
        
        m_background_color_button = createColorButton(this,
                                          ID_BackgroundColorButton,
                                          m_pi->grid_background_color);
        
        wxStaticText* background_static_text = new wxStaticText(this,
                                                     -1,
                                                     _("Background Color"));

        wxBoxSizer* background_color_sizer = new wxBoxSizer(wxHORIZONTAL);
        background_color_sizer->Add(m_background_color_button, 0, wxEXPAND | wxRIGHT, 4);
        background_color_sizer->Add(background_static_text, 0, wxALIGN_CENTER);


        // create grid line color sizer
        
        m_gridline_color_button = createColorButton(this,
                                          ID_LineColorButton,
                                          m_pi->grid_line_color);

        wxStaticText* gridline_static_text = new wxStaticText(this,
                                                   -1,
                                                   _("Line Color"));

        wxBoxSizer* gridline_color_sizer = new wxBoxSizer(wxHORIZONTAL);
        gridline_color_sizer->Add(m_gridline_color_button, 0, wxEXPAND | wxRIGHT, 4);
        gridline_color_sizer->Add(gridline_static_text, 0, wxALIGN_CENTER);


        // create invalid area color sizer
        
        m_invalid_area_color_button = createColorButton(this,
                                          ID_InvalidAreaColorButton,
                                          m_pi->grid_invalid_area_color);

        wxStaticText* invalid_area_static_text = new wxStaticText(this,
                                                      -1,
                                                      _("Invalid Area Color"));

        wxBoxSizer* invalid_area_color_sizer = new wxBoxSizer(wxHORIZONTAL);
        invalid_area_color_sizer->Add(m_invalid_area_color_button, 0, wxEXPAND | wxRIGHT, 4);
        invalid_area_color_sizer->Add(invalid_area_static_text, 0, wxALIGN_CENTER);


        // create greenbar color sizer
        
        m_greenbar_color_button = createColorButton(this,
                                          ID_GreenBarColorButton,
                                          m_pi->grid_greenbar_color);

        wxStaticText* greenbar_static_text = new wxStaticText(this,
                                                   -1,
                                                   _("Shaded Row Color"));

        wxBoxSizer* greenbar_color_sizer = new wxBoxSizer(wxHORIZONTAL);
        greenbar_color_sizer->Add(m_greenbar_color_button, 0, wxEXPAND | wxRIGHT, 4);
        greenbar_color_sizer->Add(greenbar_static_text, 0, wxALIGN_CENTER);
        
        
        // create font sizer
        
        m_fontface_combo = new FontComboControl(this, ID_FontCombo);
        m_fontface_combo->SetStringSelection(m_pi->grid_font_facename);
        
        m_fontsize_combo = new FontSizeComboControl(this, ID_FontSizeCombo);
        m_fontsize_combo->SetStringSelection(wxString::Format(wxT("%d"), m_pi->grid_font_ptsize));
        
        // make sure we don't lose our tab traversal ability
        m_fontface_combo->GetEventHandler()->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(OptionsPanel::onComboTabPressed));
        m_fontsize_combo->GetEventHandler()->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(OptionsPanel::onComboTabPressed));
        
        // when this control loses focus, make sure we update the font info and preview grid
        m_fontsize_combo->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(DatasheetOptionsPage::onKillFocus), NULL, this);
        
        wxBoxSizer* font_sizer = new wxBoxSizer(wxHORIZONTAL);
        font_sizer->Add(m_fontface_combo, 1);
        font_sizer->AddSpacer(8);
        font_sizer->Add(m_fontsize_combo, 0);
        
        
        // create top right options sizer
        
        wxStaticBox* right_box = new wxStaticBox(this, -1, _("Colors and Font"));
        wxStaticBoxSizer* top_right_sizer = new wxStaticBoxSizer(right_box, wxVERTICAL);
        top_right_sizer->AddSpacer(7);  // 7 instead of 10 (matches the left sizer)
        top_right_sizer->Add(gridline_color_sizer, 0, wxLEFT | wxRIGHT, 10);
        top_right_sizer->AddSpacer(2);
        top_right_sizer->Add(foreground_color_sizer, 0, wxLEFT | wxRIGHT, 10);
        top_right_sizer->AddSpacer(2);
        top_right_sizer->Add(background_color_sizer, 0, wxLEFT | wxRIGHT, 10);
        top_right_sizer->AddSpacer(2);
        top_right_sizer->Add(greenbar_color_sizer, 0, wxLEFT | wxRIGHT, 10);
        top_right_sizer->AddSpacer(2);
        top_right_sizer->Add(invalid_area_color_sizer, 0, wxLEFT | wxRIGHT, 10);
        top_right_sizer->AddSpacer(5);
        top_right_sizer->Add(font_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        top_right_sizer->AddSpacer(10);



        // create options sizer
        
        wxFlexGridSizer* top_sizer = new wxFlexGridSizer(1, 2, 10, 10);
        top_sizer->Add(top_left_sizer, 0, wxEXPAND);
        top_sizer->Add(top_right_sizer, 0, wxEXPAND);
        top_sizer->AddGrowableCol(1, 1);


        // create grid preview sizer

        m_preview_grid = new kcl::Grid;
        m_preview_grid->setCursorType(kcl::Grid::cursorRowHighlight);
        m_preview_grid->setBorderType(wxSIMPLE_BORDER);
        m_preview_grid->setOptionState(kcl::Grid::optColumnMove |
                                       kcl::Grid::optColumnResize |
                                       kcl::Grid::optRowResize |
                                       kcl::Grid::optEdit |
                                       kcl::Grid::optSelect,
                                       false);
        m_preview_grid->createModelColumn(-1, _("Text"), kcl::Grid::typeCharacter, 80, 0);
        m_preview_grid->createModelColumn(-1, _("Numeric"), kcl::Grid::typeDouble, 18, 3);
        m_preview_grid->createModelColumn(-1, _("Date/Time"), kcl::Grid::typeDateTime, 80, 0);
        m_preview_grid->createModelColumn(-1, _("Boolean"), kcl::Grid::typeBoolean, 80, 0);
    
        m_preview_grid->createDefaultView();
        m_preview_grid->setColumnSize(0, 150);
        m_preview_grid->setColumnSize(1, 75);
        m_preview_grid->setColumnSize(2, 75);
        m_preview_grid->setColumnSize(3, 75);
        m_preview_grid->Create(this, -1);

        m_preview_grid->insertRow(-1);
        m_preview_grid->insertRow(-1);
        m_preview_grid->insertRow(-1);

        m_preview_grid->setCellString(0, 0, _("Text Cell (0,0)"));
        m_preview_grid->setCellString(1, 0, wxT("Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo."));
        m_preview_grid->setCellString(2, 0, _("Text Cell (2,0)"));

        m_preview_grid->setCellDouble(0, 1, 0.123);
        m_preview_grid->setCellDouble(1, 1, 456.789);
        m_preview_grid->setCellDouble(2, 1, -789.000);

        m_preview_grid->setCellString(0, 2, Locale::formatDate(1970,01,01));
        m_preview_grid->setCellString(1, 2, Locale::formatDate(2000,02,28));
        m_preview_grid->setCellString(2, 2, Locale::formatDate(1979,12,25));

        m_preview_grid->setCellBoolean(0, 3, true);
        m_preview_grid->setCellBoolean(1, 3, false);
        m_preview_grid->setCellBoolean(2, 3, true);

        // load all the settings from the registry and
        // set the values of the controls accordingly
        
        m_horz_line_check->SetValue(m_pi->grid_horzlines_visible);
        m_vert_line_check->SetValue(m_pi->grid_vertlines_visible);

        m_greenbar_check->SetValue(m_pi->grid_greenbar_visible);
        m_greenbar_spin_ctrl->Enable(m_pi->grid_greenbar_visible ? true : false);

        m_text_wrap_check->SetValue(m_pi->grid_text_wrap);
        m_boolean_checkbox_check->SetValue(m_pi->grid_boolean_checkbox);

        wxFont cur_font(m_pi->grid_font_ptsize,
                        m_pi->grid_font_family,
                        m_pi->grid_font_style,
                        m_pi->grid_font_weight,
                        m_pi->grid_font_underline,
                        m_pi->grid_font_facename,
                        (wxFontEncoding)m_pi->grid_font_encoding);

        m_preview_grid->setDefaultBackgroundColor(m_pi->grid_background_color);
        m_preview_grid->setDefaultForegroundColor(m_pi->grid_foreground_color);
        m_preview_grid->setDefaultLineColor(m_pi->grid_line_color);
        m_preview_grid->setDefaultInvalidAreaColor(m_pi->grid_invalid_area_color);
        m_preview_grid->setGreenBarColor(m_pi->grid_greenbar_color);
        m_preview_grid->setOptionState(kcl::Grid::optHorzGridLines,
                                       m_pi->grid_horzlines_visible ?
                                       true : false);
        m_preview_grid->setOptionState(kcl::Grid::optVertGridLines,
                                       m_pi->grid_vertlines_visible ?
                                       true : false);
        m_preview_grid->setGreenBarInterval(m_pi->grid_greenbar_visible ?
                                            m_pi->grid_greenbar_interval : 0);
        m_preview_grid->setColumnTextWrapping(0, m_pi->grid_text_wrap ? kcl::Grid::wrapOn : kcl::Grid::wrapOff);

        // show a "T" or a checkbox for the boolean column
        kcl::CellProperties colprops;
        colprops.mask = kcl::CellProperties::cpmaskCtrlType;

        if (m_pi->grid_boolean_checkbox)
            colprops.ctrltype = kcl::Grid::ctrltypeCheckBox;
             else
            colprops.ctrltype = kcl::Grid::ctrltypeText;

        m_preview_grid->setModelColumnProperties(3, &colprops);

        m_preview_grid->SetFont(cur_font);
        m_preview_grid->moveCursor(1,1);
        m_preview_grid->setCursorVisible(false);
        m_preview_grid->setRowHeight(m_pi->grid_row_height);
        m_preview_grid->refresh(kcl::Grid::refreshAll);

        wxStaticBox* sb2 = new wxStaticBox(this, -1, _("Preview"));
        wxStaticBoxSizer* grid_sizer = new wxStaticBoxSizer(sb2, wxHORIZONTAL);
        grid_sizer->Add(m_preview_grid, 1, wxEXPAND | wxALL, 10);

        
        /*
        // even thought the right top sizer is growable, make sure the left top
        // sizer starts out taking up half the top sizer's width by default
        wxSize top_left_minsize = parent->GetClientSize();
        top_left_minsize.x -= 16; // left and right padding
        top_left_minsize.x -= 10; // middle padding between static box sizers
        top_left_minsize.x /= 2;
        top_left_minsize.y = -1;
        top_sizer->SetItemMinSize(top_left_sizer, top_left_minsize);
        */
        
        // create main sizer

        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->Add(top_sizer, 0, wxEXPAND);
        main_sizer->AddSpacer(10);
        main_sizer->Add(grid_sizer, 1, wxEXPAND);
        main_sizer->AddSpacer(2); // combined with the 8px spacer above the
                                  // ok/cancel sizer, this will give us the
                                  // standard 10px space we desire between
                                  // static box sizers and other elements
        SetSizer(main_sizer);
        Layout();
    }

    void onShowHorzGridLines(wxCommandEvent& evt)
    {
        bool val = m_horz_line_check->GetValue();
        m_pi->grid_horzlines_visible = val;
        m_preview_grid->setOptionState(kcl::Grid::optHorzGridLines, val);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }

    void onShowVertGridLines(wxCommandEvent& evt)
    {
        bool val = m_vert_line_check->GetValue();
        m_pi->grid_vertlines_visible = val;
        m_preview_grid->setOptionState(kcl::Grid::optVertGridLines, val);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }

    void onShowGreenBar(wxCommandEvent& evt)
    {
        bool val = m_greenbar_check->GetValue();
        m_pi->grid_greenbar_visible = val;
        m_greenbar_spin_ctrl->Enable(val);
        m_preview_grid->setGreenBarInterval(val ? m_pi->grid_greenbar_interval : 0);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }

    void onUseTextWrap(wxCommandEvent& evt)
    {
        bool val = m_text_wrap_check->GetValue();
        m_pi->grid_text_wrap = val;
        m_preview_grid->setColumnTextWrapping(0, val ? kcl::Grid::wrapOn : kcl::Grid::wrapOff);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }

    void onUseBooleanCheckbox(wxCommandEvent& evt)
    {
        bool val = m_boolean_checkbox_check->GetValue();
        m_pi->grid_boolean_checkbox = val;
        
        kcl::CellProperties colprops;
        colprops.mask = kcl::CellProperties::cpmaskCtrlType;
        val ? (colprops.ctrltype = kcl::Grid::ctrltypeCheckBox) :
              (colprops.ctrltype = kcl::Grid::ctrltypeText);
        m_preview_grid->setModelColumnProperties(3, &colprops);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }

    void onShowBackgroundColorPanel(wxCommandEvent& evt)
    {
        wxPoint pt_mouse = ::wxGetMousePosition();
        pt_mouse = ScreenToClient(pt_mouse);

        if (m_color_panel)
        {
            m_color_panel_cont->Destroy();
            m_color_panel = NULL;
        }

        m_button_selected = gridBackgroundColor;

        doColorPanelPopup(this,
                          &m_color_panel_cont,
                          &m_color_panel,
                          getAppPrefsDefaultColor(wxT("grid.background_color")));
    }

    void onShowForegroundColorPanel(wxCommandEvent& evt)
    {
        wxPoint pt_mouse = ::wxGetMousePosition();
        pt_mouse = ScreenToClient(pt_mouse);

        if (m_color_panel)
        {
            m_color_panel_cont->Destroy();
            m_color_panel = NULL;
        }

        m_button_selected = gridForegroundColor;

        doColorPanelPopup(this,
                          &m_color_panel_cont,
                          &m_color_panel,
                          getAppPrefsDefaultColor(wxT("grid.foreground_color")));
    }

    void onShowGridLineColorPanel(wxCommandEvent& evt)
    {
        wxPoint pt_mouse = ::wxGetMousePosition();
        pt_mouse = ScreenToClient(pt_mouse);

        if (m_color_panel)
        {
            m_color_panel_cont->Destroy();
            m_color_panel = NULL;
        }

        m_button_selected = gridLineColor;

        doColorPanelPopup(this,
                          &m_color_panel_cont,
                          &m_color_panel,
                          getAppPrefsDefaultColor(wxT("grid.line_color")));
    }

    void onShowGridInvalidAreaColorPanel(wxCommandEvent& evt)
    {
        wxPoint pt_mouse = ::wxGetMousePosition();
        pt_mouse = ScreenToClient(pt_mouse);

        if (m_color_panel)
        {
            m_color_panel_cont->Destroy();
            m_color_panel = NULL;
        }

        m_button_selected = gridInvalidAreaColor;

        doColorPanelPopup(this,
                          &m_color_panel_cont,
                          &m_color_panel,
                          getAppPrefsDefaultColor(wxT("grid.invalid_area_color")));
    }

    void onShowGridGreenBarColorPanel(wxCommandEvent& evt)
    {
        wxPoint pt_mouse = ::wxGetMousePosition();
        pt_mouse = ScreenToClient(pt_mouse);

        if (m_color_panel)
        {
            m_color_panel_cont->Destroy();
            m_color_panel = NULL;
        }

        m_button_selected = gridGreenBarColor;

        doColorPanelPopup(this,
                          &m_color_panel_cont,
                          &m_color_panel,
                          getAppPrefsDefaultColor(wxT("grid.greenbar_color")));
    }

    void onGreenBarIntervalSpin(wxSpinEvent& evt)
    {
        m_pi->grid_greenbar_interval = m_greenbar_spin_ctrl->GetValue();
        m_preview_grid->setGreenBarInterval(m_pi->grid_greenbar_interval);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }

    void onGreenBarIntervalChanged(wxCommandEvent& evt)
    {
        if (!m_greenbar_check->IsChecked())
            return;

        m_pi->grid_greenbar_interval = evt.GetInt();

        if (m_pi->grid_greenbar_interval < 0)
            m_pi->grid_greenbar_interval = 0;

        m_preview_grid->setGreenBarInterval(m_pi->grid_greenbar_interval);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }
    
    void updateFont()
    {
        wxFont font(wxAtoi(m_fontsize_combo->GetValue()),
                    wxFONTFAMILY_DEFAULT,
                    wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL,
                    false,
                    m_fontface_combo->GetValue());
        m_pi->grid_font_ptsize = font.GetPointSize();
        m_pi->grid_font_family = font.GetFamily();
        m_pi->grid_font_style = font.GetStyle();
        m_pi->grid_font_weight = font.GetWeight();
        m_pi->grid_font_encoding = font.GetDefaultEncoding();
        m_pi->grid_font_underline = font.GetUnderlined();
        m_pi->grid_font_facename = font.GetFaceName();

        m_preview_grid->SetFont(font);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }
    
    void onFontChanged(wxCommandEvent& evt)
    {
        updateFont();
    }
    
    void onKillFocus(wxFocusEvent& evt)
    {
        updateFont();
        evt.Skip();
    }
    
    // manage color panel events

    void onColorPanelDestructing(kcl::ColorPanel* panel)
    {
        m_color_panel = NULL;

        switch (m_button_selected)
        {
            case gridBackgroundColor:   m_background_color_button->Refresh();   break;
            case gridForegroundColor:   m_foreground_color_button->Refresh();   break;
            case gridLineColor:         m_gridline_color_button->Refresh();     break;
            case gridInvalidAreaColor:  m_invalid_area_color_button->Refresh(); break;
            case gridGreenBarColor:     m_greenbar_color_button->Refresh();     break;
        }
    }

    void onColorSelected(wxColor color, int button)
    {
        switch (m_button_selected)
        {
            case gridBackgroundColor:
                m_background_color_button->setColor(
                                          kcl::Button::colorNormalFg, color);
                m_preview_grid->setDefaultBackgroundColor(color);
                m_pi->grid_background_color = color;
                break;

            case gridForegroundColor:
                m_foreground_color_button->setColor(
                                          kcl::Button::colorNormalFg, color);
                m_preview_grid->setDefaultForegroundColor(color);
                m_pi->grid_foreground_color = color;
                break;

            case gridLineColor:
                m_gridline_color_button->setColor(
                                          kcl::Button::colorNormalFg, color);
                m_preview_grid->setDefaultLineColor(color);
                m_pi->grid_line_color = color;
                break;

            case gridInvalidAreaColor:
                m_invalid_area_color_button->setColor(
                                          kcl::Button::colorNormalFg, color);
                m_preview_grid->setDefaultInvalidAreaColor(color);
                m_pi->grid_invalid_area_color = color;
                break;

            case gridGreenBarColor:
                m_greenbar_color_button->setColor(
                                          kcl::Button::colorNormalFg, color);
                m_preview_grid->setGreenBarColor(color);
                m_pi->grid_greenbar_color = color;
                break;
        }

        m_preview_grid->refresh(kcl::Grid::refreshAll);

        m_color_panel_cont->Show(false);
        m_color_panel_cont->Destroy();
    }

    void restoreDefaultPrefs()
    {
        // get the default values from the default app preferences
        m_pi->grid_horzlines_visible = getAppPrefsDefaultBoolean(wxT("grid.horzlines_visible"));
        m_pi->grid_vertlines_visible = getAppPrefsDefaultBoolean(wxT("grid.vertlines_visible"));
        m_pi->grid_greenbar_visible = getAppPrefsDefaultBoolean(wxT("grid.greenbar_visible"));
        m_pi->grid_boolean_checkbox = getAppPrefsDefaultBoolean(wxT("grid.boolean_checkbox"));
        m_pi->grid_text_wrap = getAppPrefsDefaultBoolean(wxT("grid.text_wrap"));
        
        m_pi->grid_greenbar_interval = getAppPrefsDefaultLong(wxT("grid.greenbar_interval"));
        m_pi->grid_row_height = getAppPrefsDefaultLong(wxT("grid.row_height"));
        
        m_pi->grid_background_color = getAppPrefsDefaultColor(wxT("grid.background_color"));
        m_pi->grid_foreground_color = getAppPrefsDefaultColor(wxT("grid.foreground_color"));
        m_pi->grid_line_color = getAppPrefsDefaultColor(wxT("grid.line_color"));
        m_pi->grid_invalid_area_color = getAppPrefsDefaultColor(wxT("grid.invalid_area_color"));
        m_pi->grid_greenbar_color = getAppPrefsDefaultColor(wxT("grid.greenbar_color"));
        
        m_pi->grid_font_facename = getAppPrefsDefaultString(wxT("grid.font.face_name"));
        m_pi->grid_font_ptsize = getAppPrefsDefaultLong(wxT("grid.font.point_size"));
        m_pi->grid_font_family = getAppPrefsDefaultLong(wxT("grid.font.family"));
        m_pi->grid_font_style = getAppPrefsDefaultLong(wxT("grid.font.style"));
        m_pi->grid_font_weight = getAppPrefsDefaultLong(wxT("grid.font.weight"));
        m_pi->grid_font_encoding = getAppPrefsDefaultLong(wxT("grid.font.encoding"));
        m_pi->grid_font_underline = getAppPrefsDefaultBoolean(wxT("grid.font.underline"));
        
        // restore the default settings to controls
        m_background_color_button->setColor(kcl::Button::colorNormalFg,
                                        m_pi->grid_background_color);
        m_foreground_color_button->setColor(kcl::Button::colorNormalFg,
                                        m_pi->grid_foreground_color);
        m_gridline_color_button->setColor(kcl::Button::colorNormalFg,
                                        m_pi->grid_line_color);
        m_invalid_area_color_button->setColor(kcl::Button::colorNormalFg,
                                        m_pi->grid_invalid_area_color);
        m_greenbar_color_button->setColor(kcl::Button::colorNormalFg,
                                        m_pi->grid_greenbar_color);

        m_vert_line_check->SetValue(m_pi->grid_vertlines_visible);
        m_horz_line_check->SetValue(m_pi->grid_horzlines_visible);
        m_greenbar_check->SetValue(m_pi->grid_greenbar_visible);
        m_greenbar_spin_ctrl->SetValue(wxString::Format(wxT("%d"), m_pi->grid_greenbar_interval));
        m_greenbar_spin_ctrl->Enable(m_pi->grid_greenbar_visible);
        m_text_wrap_check->SetValue(m_pi->grid_text_wrap);
        m_boolean_checkbox_check->SetValue(m_pi->grid_boolean_checkbox);
        
        m_fontface_combo->SetStringSelection(m_pi->grid_font_facename);
        m_fontsize_combo->SetStringSelection(wxString::Format(wxT("%d"), m_pi->grid_font_ptsize));
        
        // restore the default settings to the grid
        m_preview_grid->setDefaultBackgroundColor(m_pi->grid_background_color);
        m_preview_grid->setDefaultForegroundColor(m_pi->grid_foreground_color);
        m_preview_grid->setDefaultLineColor(m_pi->grid_line_color);
        m_preview_grid->setDefaultInvalidAreaColor(m_pi->grid_invalid_area_color);
        m_preview_grid->setGreenBarColor(m_pi->grid_greenbar_color);

        m_preview_grid->SetFont(wxFont(m_pi->grid_font_ptsize,
                                       m_pi->grid_font_family,
                                       m_pi->grid_font_style,
                                       m_pi->grid_font_weight,
                                       m_pi->grid_font_underline,
                                       m_pi->grid_font_facename));
        m_preview_grid->setRowHeight(m_pi->grid_row_height);
        m_preview_grid->setGreenBarInterval(m_pi->grid_greenbar_visible ? m_pi->grid_greenbar_interval : 0);
        m_preview_grid->refresh(kcl::Grid::refreshAll);
    }

private:

    kcl::Grid* m_preview_grid;
    PrefInfo* m_pi;

    kcl::PopupContainer* m_color_panel_cont;
    kcl::ColorPanel* m_color_panel;

    wxCheckBox* m_horz_line_check;
    wxCheckBox* m_vert_line_check;
    wxCheckBox* m_greenbar_check;
    wxSpinCtrl* m_greenbar_spin_ctrl;
    wxCheckBox* m_text_wrap_check;
    wxCheckBox* m_boolean_checkbox_check;

    kcl::Button* m_background_color_button;
    kcl::Button* m_foreground_color_button;
    kcl::Button* m_gridline_color_button;
    kcl::Button* m_invalid_area_color_button;
    kcl::Button* m_greenbar_color_button;
    FontComboControl* m_fontface_combo;
    FontSizeComboControl* m_fontsize_combo;
    
    int m_button_selected;

    DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(DatasheetOptionsPage, wxPanel)
    EVT_CHECKBOX(ID_HorzLineCheckBox, DatasheetOptionsPage::onShowHorzGridLines)
    EVT_CHECKBOX(ID_VertLineCheckBox, DatasheetOptionsPage::onShowVertGridLines)
    EVT_CHECKBOX(ID_GreenBarCheckBox, DatasheetOptionsPage::onShowGreenBar)
    EVT_SPINCTRL(ID_GreenBarSpinCtrl, DatasheetOptionsPage::onGreenBarIntervalSpin)
    EVT_TEXT(ID_GreenBarSpinCtrl, DatasheetOptionsPage::onGreenBarIntervalChanged)
    EVT_CHECKBOX(ID_TextWrapCheckBox, DatasheetOptionsPage::onUseTextWrap)
    EVT_CHECKBOX(ID_BooleanCheckBoxCheckBox, DatasheetOptionsPage::onUseBooleanCheckbox)
    EVT_BUTTON(ID_BackgroundColorButton, DatasheetOptionsPage::onShowBackgroundColorPanel)
    EVT_BUTTON(ID_ForegroundColorButton, DatasheetOptionsPage::onShowForegroundColorPanel)
    EVT_BUTTON(ID_LineColorButton, DatasheetOptionsPage::onShowGridLineColorPanel)
    EVT_BUTTON(ID_InvalidAreaColorButton, DatasheetOptionsPage::onShowGridInvalidAreaColorPanel)
    EVT_BUTTON(ID_GreenBarColorButton, DatasheetOptionsPage::onShowGridGreenBarColorPanel)
    EVT_COMBOBOX(ID_FontCombo, DatasheetOptionsPage::onFontChanged)
    EVT_COMBOBOX(ID_FontSizeCombo, DatasheetOptionsPage::onFontChanged)
    EVT_TEXT_ENTER(ID_FontSizeCombo, DatasheetOptionsPage::onFontChanged)
END_EVENT_TABLE()


void doColorPanelPopup(wxWindow* parent,
                       kcl::PopupContainer** container,
                       kcl::ColorPanel** panel,
                       const wxColor& default_color)
{
    *container = new kcl::PopupContainer(::wxGetMousePosition());
    *panel = new kcl::ColorPanel(*container);

    kcl::ColorPanel* color_panel = (*panel);
    color_panel->setMode(kcl::ColorPanel::ModeNone);
    color_panel->setDefaultColor(default_color);
    color_panel->sigDestructing.connect((DatasheetOptionsPage*)parent,
                          &DatasheetOptionsPage::onColorPanelDestructing);
    color_panel->sigColorSelected.connect((DatasheetOptionsPage*)parent,
                          &DatasheetOptionsPage::onColorSelected);
    color_panel->sigCustomColorsRequested.connect(g_app->getAppController(),
                          &AppController::onCustomColorsRequested);
    color_panel->sigCustomColorsChanged.connect(g_app->getAppController(),
                          &AppController::onCustomColorsChanged);
    (*container)->doPopup(color_panel);
}








// Internet Options page

class InternetOptionsPage : public wxPanel
{

private:

    enum
    {
        ID_DirectProxyRadio = wxID_HIGHEST + 1,
        ID_AutoDetectProxyRadio,
        ID_ManualProxyRadio,
        ID_HttpProxyTextCtrl,
        ID_HttpProxyPortTextCtrl,
        ID_FtpProxyTextCtrl,
        ID_FtpProxyPortTextCtrl,
        ID_SslProxyTextCtrl,
        ID_SslProxyPortTextCtrl,
        ID_SocksProxyTextCtrl,
        ID_SocksProxyPortTextCtrl
    };

public:

    InternetOptionsPage(wxWindow* parent, PrefInfo* pi) : wxPanel(parent), m_pi(pi)
    {
        SetWindowStyle(GetWindowStyle() | wxTAB_TRAVERSAL);

        // create proxy radio buttons

        m_proxy_direct_radio = new wxRadioButton(this, 
                                                 ID_DirectProxyRadio,
                                                 _("Directly connect to the Internet"),
                                                 wxDefaultPosition,
                                                 wxDefaultSize,
                                                 wxRB_GROUP);

        m_proxy_autodetect_radio = new wxRadioButton(this, 
                                                 ID_AutoDetectProxyRadio,
                                                 _("Automatically detect proxy settings for this network"),
                                                 wxDefaultPosition,
                                                 wxDefaultSize);

        m_proxy_manual_radio = new wxRadioButton(this, 
                                                 ID_ManualProxyRadio,
                                                 _("Manually configure proxy settings"),
                                                 wxDefaultPosition,
                                                 wxDefaultSize);

        // create http proxy settings sizer
        
        m_proxy_http_sizer = createLabelTextControlSizer(
                                this,
                                _("HTTP Proxy:"),
                                &m_proxy_http_textctrl,
                                ID_HttpProxyTextCtrl,
                                m_pi->internet_proxy_http,
                                10);
        wxBoxSizer* httpport_sizer = createLabelTextControlSizer(
                                this,
                                _("Port:"),
                                &m_proxy_httpport_textctrl,
                                ID_HttpProxyPortTextCtrl,
                                wxString::Format(wxT("%d"), m_pi->internet_proxy_http_port),
                                10);
        m_proxy_http_sizer->AddSpacer(10);
        m_proxy_http_sizer->Add(httpport_sizer);
        
        // create ftp proxy settings sizer
        
        m_proxy_ftp_sizer = createLabelTextControlSizer(
                                this,
                                _("FTP Proxy:"),
                                &m_proxy_ftp_textctrl,
                                ID_FtpProxyTextCtrl,
                                m_pi->internet_proxy_ftp,
                                10);
        wxBoxSizer* ftpport_sizer = createLabelTextControlSizer(
                                this,
                                _("Port:"),
                                &m_proxy_ftpport_textctrl,
                                ID_FtpProxyPortTextCtrl,
                                wxString::Format(wxT("%d"), m_pi->internet_proxy_ftp_port),
                                10);
        m_proxy_ftp_sizer->AddSpacer(10);
        m_proxy_ftp_sizer->Add(ftpport_sizer);
        
        // create ssl proxy settings sizer
        
        m_proxy_ssl_sizer = createLabelTextControlSizer(
                                this,
                                _("SSL Proxy:"),
                                &m_proxy_ssl_textctrl,
                                ID_SslProxyTextCtrl,
                                m_pi->internet_proxy_ssl,
                                10);
        wxBoxSizer* sslport_sizer = createLabelTextControlSizer(
                                this,
                                _("Port:"),
                                &m_proxy_sslport_textctrl,
                                ID_SslProxyPortTextCtrl,
                                wxString::Format(wxT("%d"), m_pi->internet_proxy_ssl_port),
                                10);
        m_proxy_ssl_sizer->AddSpacer(10);
        m_proxy_ssl_sizer->Add(sslport_sizer);
        
        // create socks proxy settings sizer
        
        m_proxy_socks_sizer = createLabelTextControlSizer(
                                this,
                                _("SOCKS Proxy:"),
                                &m_proxy_socks_textctrl,
                                ID_SocksProxyTextCtrl,
                                m_pi->internet_proxy_socks,
                                10);
        wxBoxSizer* socksport_sizer = createLabelTextControlSizer(
                                this,
                                _("Port:"),
                                &m_proxy_socksport_textctrl,
                                ID_SocksProxyPortTextCtrl,
                                wxString::Format(wxT("%d"), m_pi->internet_proxy_socks_port),
                                10);
        m_proxy_socks_sizer->AddSpacer(10);
        m_proxy_socks_sizer->Add(socksport_sizer);
        
        // make the labels line up in the proxy settings sizer
        makeSizerLabelsSameSize(m_proxy_http_sizer,
                                m_proxy_ssl_sizer,
                                m_proxy_ftp_sizer,
                                m_proxy_socks_sizer);

        // this must be done after the call to makeSizerLabelsSameSize()
        // because that function is expecting the first sizer item to be
        // a wxStaticText control
        m_proxy_http_sizer->Prepend(40,1);
        m_proxy_ftp_sizer->Prepend(40,1);
        m_proxy_ssl_sizer->Prepend(40,1);
        m_proxy_socks_sizer->Prepend(40,1);

        // make sure the proxy type radio buttons are set properly
        m_proxy_direct_radio->SetValue(m_pi->internet_proxy_type == prefProxyDirect ? true : false);
        m_proxy_autodetect_radio->SetValue(m_pi->internet_proxy_type == prefProxyAutoDetect ? true : false);
        m_proxy_manual_radio->SetValue(m_pi->internet_proxy_type == prefProxyManual ? true : false);
        
        // create the proxy settings sizer
        
        wxStaticBox* ps_box = new wxStaticBox(this, -1, _("Proxy Settings (Advanced)"));
        wxStaticBoxSizer* proxy_sizer = new wxStaticBoxSizer(ps_box, wxVERTICAL);
        proxy_sizer->AddSpacer(10);
        proxy_sizer->Add(m_proxy_direct_radio, 0, wxLEFT | wxRIGHT, 10);
        proxy_sizer->AddSpacer(8);
        proxy_sizer->Add(m_proxy_autodetect_radio, 0, wxLEFT | wxRIGHT, 10);
        proxy_sizer->AddSpacer(8);
        proxy_sizer->Add(m_proxy_manual_radio, 0, wxLEFT | wxRIGHT, 10);
        proxy_sizer->AddSpacer(8);
        proxy_sizer->Add(m_proxy_http_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        proxy_sizer->AddSpacer(8);
        proxy_sizer->Add(m_proxy_ftp_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        proxy_sizer->AddSpacer(8);
        proxy_sizer->Add(m_proxy_ssl_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        proxy_sizer->AddSpacer(8);
        proxy_sizer->Add(m_proxy_socks_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        proxy_sizer->AddSpacer(10);

        // create main sizer
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->Add(proxy_sizer, 0, wxEXPAND);
        SetSizer(main_sizer);
        Layout();
        
        // fire this event to enable/disable proxy text controls
        wxCommandEvent empty;
        onProxyTypeRadioChanged(empty);
    }

    static void enableSizerChildren(wxSizer* sizer, bool enable)
    {
        wxSizerItemList item_list = sizer->GetChildren();
        size_t i, count = item_list.GetCount();
        for (i = 0; i < count; ++i)
        {
            wxSizerItem* item = item_list.Item(i)->GetData();
            wxWindow* wnd = item->GetWindow();
            if (wnd != NULL && wnd->IsEnabled() != enable)
                wnd->Enable(enable);
            
            // make sure we recursively enable/disable elements
            // in sizers that are contained in this sizer
            if (item->GetSizer() != NULL)
                enableSizerChildren(item->GetSizer(), enable);
        }
    }
    
    void onProxyTypeRadioChanged(wxCommandEvent& evt)
    {
        if (m_proxy_direct_radio->GetValue())
        {
            enableSizerChildren(m_proxy_http_sizer, false);
            enableSizerChildren(m_proxy_ftp_sizer, false);
            enableSizerChildren(m_proxy_ssl_sizer, false);
            enableSizerChildren(m_proxy_socks_sizer, false);
            m_pi->internet_proxy_type = prefProxyDirect;
        }
         else if (m_proxy_autodetect_radio->GetValue())
        {
            enableSizerChildren(m_proxy_http_sizer, false);
            enableSizerChildren(m_proxy_ftp_sizer, false);
            enableSizerChildren(m_proxy_ssl_sizer, false);
            enableSizerChildren(m_proxy_socks_sizer, false);
            m_pi->internet_proxy_type = prefProxyAutoDetect;
        }
         else
        {
            enableSizerChildren(m_proxy_http_sizer, true);
            enableSizerChildren(m_proxy_ftp_sizer, true);
            enableSizerChildren(m_proxy_ssl_sizer, true);
            enableSizerChildren(m_proxy_socks_sizer, true);
            m_pi->internet_proxy_type = prefProxyManual;
            
            m_proxy_http_textctrl->SetFocus();
            m_proxy_http_textctrl->SetInsertionPointEnd();
        }
    }
    
    void onHttpProxyTextChanged(wxCommandEvent& evt)
    {
        if (m_proxy_http_textctrl)
            m_pi->internet_proxy_http = evt.GetString();
    }

    void onHttpProxyPortTextChanged(wxCommandEvent& evt)
    {
        if (m_proxy_httpport_textctrl)
            m_pi->internet_proxy_http_port = wxAtoi(evt.GetString());
    }

    void onFtpProxyTextChanged(wxCommandEvent& evt)
    {
        if (m_proxy_ftp_textctrl)
            m_pi->internet_proxy_ftp = evt.GetString();
    }

    void onFtpProxyPortTextChanged(wxCommandEvent& evt)
    {
        if (m_proxy_ftpport_textctrl)
            m_pi->internet_proxy_ftp_port = wxAtoi(evt.GetString());
    }

    void onSslProxyTextChanged(wxCommandEvent& evt)
    {
        if (m_proxy_ssl_textctrl)
            m_pi->internet_proxy_ssl = evt.GetString();
    }

    void onSslProxyPortTextChanged(wxCommandEvent& evt)
    {
        if (m_proxy_sslport_textctrl)
            m_pi->internet_proxy_ssl_port = wxAtoi(evt.GetString());
    }

    void onSocksProxyTextChanged(wxCommandEvent& evt)
    {
        if (m_proxy_socks_textctrl)
            m_pi->internet_proxy_socks = evt.GetString();
    }

    void onSocksProxyPortTextChanged(wxCommandEvent& evt)
    {
        if (m_proxy_socksport_textctrl)
            m_pi->internet_proxy_socks_port = wxAtoi(evt.GetString());
    }
    
    void restoreDefaultPrefs()
    {
        // get the default values from the default app preferences
        m_pi->internet_proxy_type = getAppPrefsDefaultLong(wxT("internet.proxy.type"));
        m_pi->internet_proxy_http_port = getAppPrefsDefaultLong(wxT("internet.proxy.http_port"));
        m_pi->internet_proxy_ftp_port = getAppPrefsDefaultLong(wxT("internet.proxy.ftp_port"));
        m_pi->internet_proxy_ssl_port = getAppPrefsDefaultLong(wxT("internet.proxy.ssl_port"));
        m_pi->internet_proxy_socks_port = getAppPrefsDefaultLong(wxT("internet.proxy.socks_port"));
        m_pi->internet_proxy_http = getAppPrefsDefaultString(wxT("internet.proxy.http"));
        m_pi->internet_proxy_ftp = getAppPrefsDefaultString(wxT("internet.proxy.ftp"));
        m_pi->internet_proxy_ssl = getAppPrefsDefaultString(wxT("internet.proxy.ssl"));
        m_pi->internet_proxy_socks = getAppPrefsDefaultString(wxT("internet.proxy.socks"));
        
        // restore the default settings to controls
        m_proxy_http_textctrl->SetValue(m_pi->internet_proxy_http);
        m_proxy_ftp_textctrl->SetValue(m_pi->internet_proxy_ftp);
        m_proxy_ssl_textctrl->SetValue(m_pi->internet_proxy_ssl);
        m_proxy_socks_textctrl->SetValue(m_pi->internet_proxy_socks);
        m_proxy_httpport_textctrl->SetValue(wxString::Format(wxT("%d"), m_pi->internet_proxy_http_port));
        m_proxy_ftpport_textctrl->SetValue(wxString::Format(wxT("%d"), m_pi->internet_proxy_ftp_port));
        m_proxy_sslport_textctrl->SetValue(wxString::Format(wxT("%d"), m_pi->internet_proxy_ssl_port));
        m_proxy_socksport_textctrl->SetValue(wxString::Format(wxT("%d"), m_pi->internet_proxy_socks_port));
        m_proxy_direct_radio->SetValue(m_pi->internet_proxy_type == prefProxyDirect ? true : false);
        m_proxy_manual_radio->SetValue(m_pi->internet_proxy_type == prefProxyManual ? true : false);
        m_proxy_autodetect_radio->SetValue(m_pi->internet_proxy_type == prefProxyAutoDetect ? true : false);

        // fire this event to enable/disable proxy text controls
        wxCommandEvent empty;
        onProxyTypeRadioChanged(empty);
    }
    
private:

    PrefInfo* m_pi;
    
    wxBoxSizer* m_proxy_http_sizer;
    wxBoxSizer* m_proxy_ftp_sizer;
    wxBoxSizer* m_proxy_ssl_sizer;
    wxBoxSizer* m_proxy_socks_sizer;
    
    wxTextCtrl* m_homepage_textctrl;
    wxButton* m_use_current_page_button;
    wxButton* m_use_blank_page_button;

    wxRadioButton* m_proxy_direct_radio;
    wxRadioButton* m_proxy_autodetect_radio;
    wxRadioButton* m_proxy_manual_radio;
    
    wxTextCtrl* m_proxy_http_textctrl;
    wxTextCtrl* m_proxy_httpport_textctrl;
    wxTextCtrl* m_proxy_ftp_textctrl;
    wxTextCtrl* m_proxy_ftpport_textctrl;
    wxTextCtrl* m_proxy_ssl_textctrl;
    wxTextCtrl* m_proxy_sslport_textctrl;
    wxTextCtrl* m_proxy_socks_textctrl;
    wxTextCtrl* m_proxy_socksport_textctrl;
    
    DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(InternetOptionsPage, wxPanel)
    EVT_RADIOBUTTON(ID_DirectProxyRadio, InternetOptionsPage::onProxyTypeRadioChanged)
    EVT_RADIOBUTTON(ID_AutoDetectProxyRadio, InternetOptionsPage::onProxyTypeRadioChanged)
    EVT_RADIOBUTTON(ID_ManualProxyRadio, InternetOptionsPage::onProxyTypeRadioChanged)
    EVT_TEXT(ID_HttpProxyTextCtrl, InternetOptionsPage::onHttpProxyTextChanged)
    EVT_TEXT(ID_HttpProxyPortTextCtrl, InternetOptionsPage::onHttpProxyPortTextChanged)
    EVT_TEXT(ID_FtpProxyTextCtrl, InternetOptionsPage::onFtpProxyTextChanged)
    EVT_TEXT(ID_FtpProxyPortTextCtrl, InternetOptionsPage::onFtpProxyPortTextChanged)
    EVT_TEXT(ID_SslProxyTextCtrl, InternetOptionsPage::onSslProxyTextChanged)
    EVT_TEXT(ID_SslProxyPortTextCtrl, InternetOptionsPage::onSslProxyPortTextChanged)
    EVT_TEXT(ID_SocksProxyTextCtrl, InternetOptionsPage::onSocksProxyTextChanged)
    EVT_TEXT(ID_SocksProxyPortTextCtrl, InternetOptionsPage::onSocksProxyPortTextChanged)
END_EVENT_TABLE()








class PrivacyOptionsPage : public wxPanel
{
private:

    enum
    {
        ID_AllowPopupsCheckBox = wxID_HIGHEST + 1,
        ID_AllowCookiesCheckBox,
        ID_KeepCookiesUntilExpiredRadio,
        ID_KeepCookiesUntilCloseRadio,
        ID_ClearPrivateDataButton
    };
    
public:

    PrivacyOptionsPage(wxWindow* parent, PrefInfo* pi) : wxPanel(parent), m_pi(pi)
    {
        SetWindowStyle(GetWindowStyle() | wxTAB_TRAVERSAL);


        // create controls
        
        m_block_popups = new wxCheckBox(this,
                                        ID_AllowPopupsCheckBox,
                                        _("Block pop-up windows"));
        
        m_accept_cookies = new wxCheckBox(this,
                                        ID_AllowCookiesCheckBox,
                                        _("Accept cookies from sites"));
        
        m_keep_cookies_until_expired = new wxRadioButton(this,
                                        ID_KeepCookiesUntilExpiredRadio,
                                        _("Keep until they expire"),
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxRB_GROUP);

        wxString appname = APPLICATION_NAME;
        wxString label = wxString::Format(_("Keep until %s is closed"), appname.c_str());
        m_keep_cookies_until_close = new wxRadioButton(this,
                                        ID_KeepCookiesUntilCloseRadio,
                                        label);
        
        m_clear_private_data = new wxButton(this,
                                            ID_ClearPrivateDataButton,
                                            _("Clear Private Data"));
        
        
        // create popups sizer

        wxStaticBox* popup_static_box = new wxStaticBox(this, 
                                                -1,
                                                _("Pop-ups"));
        wxStaticBoxSizer* popup_sizer;
        popup_sizer = new wxStaticBoxSizer(popup_static_box,
                                             wxVERTICAL);
        popup_sizer->AddSpacer(10);
        popup_sizer->Add(m_block_popups, 0, wxLEFT | wxRIGHT, 10);
        popup_sizer->AddSpacer(10);
        
        
        // create cookies sizer

        wxStaticBox* cookies_static_box = new wxStaticBox(this, 
                                                -1,
                                                _("Cookies"));
        wxStaticBoxSizer* cookies_sizer;
        cookies_sizer = new wxStaticBoxSizer(cookies_static_box,
                                             wxVERTICAL);
        cookies_sizer->AddSpacer(10);
        cookies_sizer->Add(m_accept_cookies, 0, wxLEFT | wxRIGHT, 10);
        cookies_sizer->AddSpacer(8);
        cookies_sizer->Add(m_keep_cookies_until_expired, 0, wxLEFT, 25);
        cookies_sizer->AddSpacer(8);
        cookies_sizer->Add(m_keep_cookies_until_close, 0, wxLEFT, 25);
        cookies_sizer->AddSpacer(10);
        
        
        // create clear private data sizer

        wxBoxSizer* clear_private_data_sizer = new wxBoxSizer(wxHORIZONTAL);
        clear_private_data_sizer->Add(m_clear_private_data);
        
        
        // create main sizer
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->Add(popup_sizer, 0, wxEXPAND);
        main_sizer->AddSpacer(10);
        main_sizer->Add(cookies_sizer, 0, wxEXPAND);
        main_sizer->AddSpacer(10);
        main_sizer->Add(clear_private_data_sizer, 0, wxEXPAND);
        SetSizer(main_sizer);
        Layout();
        
        initControlValues();
    }
    
    void initControlValues()
    {
        m_block_popups->SetValue(m_pi->privacy_block_popups);
        m_accept_cookies->SetValue(m_pi->privacy_accept_cookies);
        
        // select the appropriate radio button based
        // on the current preference settings
        switch (m_pi->privacy_cookie_lifetime_policy)
        {
            default:
            case prefKeepCookiesUntilExpired:
                {
                    m_keep_cookies_until_expired->SetValue(true);
                    break;
                }
                
            case prefKeepCookiesUntilExit:
                {
                    m_keep_cookies_until_close->SetValue(true);
                    break;
                }
        }

        wxCommandEvent unused;
        onAllowCookiesChecked(unused);
    }
    
    void serializeValues()
    {
        m_pi->privacy_block_popups = m_block_popups->GetValue();
        m_pi->privacy_accept_cookies = m_accept_cookies->GetValue();
        
        if (m_keep_cookies_until_expired->GetValue())
            m_pi->privacy_cookie_lifetime_policy = prefKeepCookiesUntilExpired;
             else if (m_keep_cookies_until_close->GetValue())
            m_pi->privacy_cookie_lifetime_policy = prefKeepCookiesUntilExit;
    }
    
    void onAllowCookiesChecked(wxCommandEvent& evt)
    {
        bool enable = m_accept_cookies->GetValue();
        m_keep_cookies_until_expired->Enable(enable);
        m_keep_cookies_until_close->Enable(enable);
    }
    
    void onClearPrivateData(wxCommandEvent& evt)
    {
        int result = appMessageBox(_("Are you sure you want to clear all private data?"),
                                        APPLICATION_NAME,
                                        wxYES_NO | wxICON_QUESTION | wxCENTER);
        if (result == wxYES)
        {
            wxWebControl::ClearCache();
        }
    }
    
    void restoreDefaultPrefs()
    {
        // get the default values from the default app preferences
        m_pi->privacy_block_popups = getAppPrefsDefaultBoolean(wxT("privacy.popups.block"));
        m_pi->privacy_accept_cookies = getAppPrefsDefaultBoolean(wxT("privacy.cookie.accept"));
        m_pi->privacy_cookie_lifetime_policy = getAppPrefsDefaultLong(wxT("privacy.cookie.lifetime_policy"));
        m_pi->privacy_cookie_lifetime_days = getAppPrefsDefaultLong(wxT("privacy.cookie.lifetime_days"));
        
        // restore the default settings to controls
        m_block_popups->SetValue(m_pi->privacy_block_popups);
        m_accept_cookies->SetValue(m_pi->privacy_accept_cookies);
        m_keep_cookies_until_expired->SetValue(m_pi->privacy_cookie_lifetime_policy == prefKeepCookiesUntilExpired ? true : false);
        m_keep_cookies_until_close->SetValue(m_pi->privacy_cookie_lifetime_policy == prefKeepCookiesUntilExit ? true : false);
        
        wxCommandEvent unused;
        onAllowCookiesChecked(unused);
    }
    
private:

    PrefInfo* m_pi;
    
    wxCheckBox* m_block_popups;
    wxCheckBox* m_accept_cookies;
    wxRadioButton* m_keep_cookies_until_expired;
    wxRadioButton* m_keep_cookies_until_close;
    wxButton* m_clear_private_data;
    
    DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(PrivacyOptionsPage, wxPanel)
    EVT_CHECKBOX(ID_AllowCookiesCheckBox, PrivacyOptionsPage::onAllowCookiesChecked)
    EVT_BUTTON(ID_ClearPrivateDataButton, PrivacyOptionsPage::onClearPrivateData)
END_EVENT_TABLE()








class ReportOptionsPage : public wxPanel
{
public:

    ReportOptionsPage(wxWindow* parent, PrefInfo* pi) : wxPanel(parent), m_pi(pi)
    {
        SetWindowStyle(GetWindowStyle() | wxTAB_TRAVERSAL);

        // create panels
        m_orientation_panel = new OrientationPanel(this);
        m_pagesize_panel = new PageSizePanel(this);
        
        
        // create font sizer
        
        m_fontface_combo = new FontComboControl(this, -1);
        m_fontsize_combo = new FontSizeComboControl(this, -1);
        
        // make sure we don't lose our tab traversal ability
        m_fontface_combo->GetEventHandler()->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(OptionsPanel::onComboTabPressed));
        m_fontsize_combo->GetEventHandler()->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(OptionsPanel::onComboTabPressed));

        wxStaticBox* box = new wxStaticBox(this, -1, _("Font"));
        wxStaticBoxSizer* font_sizer = new wxStaticBoxSizer(box, wxHORIZONTAL);
        font_sizer->AddSpacer(10);
        font_sizer->Add(m_fontface_combo, 1, wxTOP | wxBOTTOM, 10);
        font_sizer->AddSpacer(8);
        font_sizer->Add(m_fontsize_combo, 0, wxTOP | wxBOTTOM, 10);
        font_sizer->AddSpacer(10);

        
        // create margin panel (created here so it is in
        // the correct location for tab traversal)
        m_margins_panel = new MarginsPanel(this);

        
        // create top sizer
        
        wxFlexGridSizer* top_sizer = new wxFlexGridSizer(2,2,10,10);
        top_sizer->Add(m_orientation_panel, 0, wxEXPAND);
        top_sizer->Add(m_pagesize_panel, 0, wxEXPAND);
        top_sizer->Add(font_sizer, 0, wxEXPAND);
        top_sizer->Add(m_margins_panel, 0, wxEXPAND);
        top_sizer->AddGrowableCol(0, 1);
        
        
        // create main sizer
            
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->Add(top_sizer, 0, wxEXPAND);
        SetSizer(main_sizer);
        Layout();
        
        // connect signals
        m_orientation_panel->sigOrientationChanged.connect(m_pagesize_panel, &PageSizePanel::onOrientationChanged);
        m_pagesize_panel->sigDimensionsChanged.connect(m_orientation_panel, &OrientationPanel::onPageDimensionsChanged);

        initControlValues();
    }
    
    double toPage(long val)
    {
        double retval = val;
        retval /= 100.0;
        retval = kl::dblround(retval, 2);
        return retval;
    }
    
    long toPref(double val)
    {
        val *= 100.0;
        floor(val);
        long retval = val;
        return retval;
    }
    
    void initControlValues()
    {
        m_pagesize_panel->setPageDimensions(toPage(m_pi->report_page_width),
                                            toPage(m_pi->report_page_height));
        
        m_margins_panel->setMargins(toPage(m_pi->report_margin_left),
                                    toPage(m_pi->report_margin_right),
                                    toPage(m_pi->report_margin_top),
                                    toPage(m_pi->report_margin_bottom));
        
        m_fontface_combo->SetStringSelection(m_pi->report_font_facename);
        m_fontsize_combo->SetStringSelection(wxString::Format(wxT("%d"), m_pi->report_font_ptsize));
    }
    
    void serializeValues()
    {
        double page_width, page_height;
        m_pagesize_panel->getPageDimensions(&page_width, &page_height);
        
        double margin_left, margin_right, margin_top, margin_bottom;
        m_margins_panel->getMargins(&margin_left,
                                    &margin_right,
                                    &margin_top,
                                    &margin_bottom);
        
        m_pi->report_page_width = toPref(page_width);
        m_pi->report_page_height = toPref(page_height);
        m_pi->report_margin_left = toPref(margin_left);
        m_pi->report_margin_right = toPref(margin_right);
        m_pi->report_margin_top = toPref(margin_top);
        m_pi->report_margin_bottom = toPref(margin_bottom);

        kcanvas::Font font(m_fontface_combo->GetValue(),
                           kcanvas::FONT_STYLE_NORMAL,
                           kcanvas::FONT_WEIGHT_NORMAL,
                           kcanvas::FONT_UNDERSCORE_NORMAL,
                           wxAtoi(m_fontsize_combo->GetValue()));

        m_pi->report_font_ptsize = font.getSize();
        m_pi->report_font_style = font.getStyle();
        m_pi->report_font_weight = font.getWeight();
        m_pi->report_font_underscore = font.getUnderscore();
        m_pi->report_font_facename = font.getFaceName();
    }
    
    void restoreDefaultPrefs()
    {
        // get the default values from the default app preferences
        m_pi->report_page_width = getAppPrefsDefaultLong(wxT("report.page.width"));
        m_pi->report_page_height = getAppPrefsDefaultLong(wxT("report.page.height"));
        m_pi->report_margin_left = getAppPrefsDefaultLong(wxT("report.margin.left"));
        m_pi->report_margin_right = getAppPrefsDefaultLong(wxT("report.margin.right"));
        m_pi->report_margin_top = getAppPrefsDefaultLong(wxT("report.margin.top"));
        m_pi->report_margin_bottom = getAppPrefsDefaultLong(wxT("report.margin.bottom"));
        
        m_pi->report_font_facename = getAppPrefsDefaultString(wxT("report.font.facename"));
        m_pi->report_font_ptsize = getAppPrefsDefaultLong(wxT("report.font.size"));
        m_pi->report_font_style = getAppPrefsDefaultString(wxT("report.font.style"));
        m_pi->report_font_weight = getAppPrefsDefaultString(wxT("report.font.weight"));
        m_pi->report_font_underscore = getAppPrefsDefaultString(wxT("report.font.underscore"));

        // restore the default settings to controls
        m_pagesize_panel->setPageDimensions(toPage(m_pi->report_page_width),
                                            toPage(m_pi->report_page_height));
        
        m_margins_panel->setMargins(toPage(m_pi->report_margin_left),
                                    toPage(m_pi->report_margin_right),
                                    toPage(m_pi->report_margin_top),
                                    toPage(m_pi->report_margin_bottom));
        
        m_fontface_combo->SetValue(m_pi->report_font_facename);
        m_fontsize_combo->SetValue(wxString::Format(wxT("%d"), m_pi->report_font_ptsize));
    }
    
private:

    PrefInfo* m_pi;
    
    OrientationPanel* m_orientation_panel;
    PageSizePanel* m_pagesize_panel;
    MarginsPanel* m_margins_panel;
    
    FontComboControl* m_fontface_combo;
    FontSizeComboControl* m_fontsize_combo;
    
    DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(ReportOptionsPage, wxPanel)
END_EVENT_TABLE()








class ScriptOptionsPage : public wxPanel
{
private:

    enum
    {
        ID_ShowLineNumbersCheckBox = wxID_HIGHEST + 1,
        ID_ShowSyntaxHighlightCheckBox,
        ID_ShowWhitespaceCheckBox,
        ID_UseSpacesRadioButton,
        ID_UseTabsRadioButton,
        ID_SpacesSpinCtrl,
        ID_TabSpinCtrl
    };
    
public:

    ScriptOptionsPage(wxWindow* parent, PrefInfo* pi) : wxPanel(parent), m_pi(pi)
    {
        SetWindowStyle(GetWindowStyle() | wxTAB_TRAVERSAL);
        
        m_show_line_numbers = NULL;
        m_show_syntax_highlight = NULL;
        m_show_whitespace = NULL;
        m_use_spaces = NULL;
        m_use_tabs = NULL;
        m_spaces_size = NULL;
        m_tab_size = NULL;

        // create controls
        
        m_show_line_numbers = new wxCheckBox(this,
                                    ID_ShowLineNumbersCheckBox,
                                    _("Show line numbers"));
        
        m_show_syntax_highlight = new wxCheckBox(this,
                                    ID_ShowSyntaxHighlightCheckBox,
                                    _("Show syntax highlighting"));
        
        m_show_whitespace = new wxCheckBox(this,
                                    ID_ShowWhitespaceCheckBox,
                                    _("Show whitespace characters (spaces, tabs, etc.)"));
        
        m_use_spaces = new wxRadioButton(this,
                                    ID_UseSpacesRadioButton,
                                    _("Insert spaces"),
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    wxRB_GROUP);
        
        m_use_tabs = new wxRadioButton(this,
                                    ID_UseTabsRadioButton,
                                    _("Keep tabs"));
        
        const int MIN_SPACES = 1;
        const int MIN_TABS = 1;
        const int MAX_SPACES = 60;
        const int MAX_TABS = 60;
        
        m_spaces_size = new wxSpinCtrl(this,
                                    ID_SpacesSpinCtrl,
                                    wxEmptyString,
                                    wxDefaultPosition,
                                    wxSize(65,-1),
                                    wxSP_ARROW_KEYS,
                                    MIN_SPACES, MAX_SPACES);
        
        m_tab_size = new wxSpinCtrl(this,
                                    ID_TabSpinCtrl,
                                    wxEmptyString, 
                                    wxDefaultPosition,
                                    wxSize(65,-1),
                                    wxSP_ARROW_KEYS,
                                    MIN_TABS, MAX_TABS);
        
        
        // create general sizer
        
        wxStaticBox* general_box = new wxStaticBox(this, -1, _("General"));
        wxStaticBoxSizer* general_sizer = new wxStaticBoxSizer(general_box, wxVERTICAL);
        general_sizer->AddSpacer(10);
        general_sizer->Add(m_show_line_numbers, 0, wxLEFT | wxRIGHT, 10);
        general_sizer->AddSpacer(8);
        general_sizer->Add(m_show_syntax_highlight, 0, wxLEFT | wxRIGHT, 10);
        general_sizer->AddSpacer(8);
        general_sizer->Add(m_show_whitespace, 0, wxLEFT | wxRIGHT, 10);
        general_sizer->AddSpacer(10);
        
        
        // create tabs sizer
        
        wxStaticBox* tabs_box = new wxStaticBox(this, -1, _("Tabs"));
        wxStaticBoxSizer* tabs_sizer = new wxStaticBoxSizer(tabs_box, wxVERTICAL);
        tabs_sizer->AddSpacer(10);
        tabs_sizer->Add(m_use_spaces, 0, wxLEFT | wxRIGHT, 10);
        tabs_sizer->AddSpacer(8);
        tabs_sizer->Add(m_spaces_size, 0, wxLEFT | wxRIGHT, 25);
        tabs_sizer->AddSpacer(15);
        tabs_sizer->Add(m_use_tabs, 0, wxLEFT | wxRIGHT, 10);
        tabs_sizer->AddSpacer(8);
        tabs_sizer->Add(m_tab_size, 0, wxLEFT | wxRIGHT, 25);
        tabs_sizer->AddSpacer(10);


        // create main sizer
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->Add(general_sizer, 0, wxEXPAND);
        main_sizer->AddSpacer(10);
        main_sizer->Add(tabs_sizer, 0, wxEXPAND);
        SetSizer(main_sizer);
        Layout();
        
        initControlValues();
    }
    
    void updateEnabled()
    {
        m_spaces_size->Enable(m_use_spaces->GetValue());
        m_tab_size->Enable(m_use_tabs->GetValue());
    }
    
    void initControlValues()
    {
        m_show_line_numbers->SetValue(m_pi->script_show_line_numbers);
        m_show_syntax_highlight->SetValue(m_pi->script_show_syntax_highlight);
        m_show_whitespace->SetValue(m_pi->script_show_whitespace);
        
        m_spaces_size->SetValue(m_pi->script_spaces_size);
        m_tab_size->SetValue(m_pi->script_tab_size);
        
        if (m_pi->script_insert_spaces)
            m_use_spaces->SetValue(true);
             else
            m_use_tabs->SetValue(true);
        
        // make sure the spaces or tab size
        // text controls are enabled/disabled
        updateEnabled();
    }
    
    void restoreDefaultPrefs()
    {
        // get the default values from the default app preferences
        m_pi->script_show_syntax_highlight = getAppPrefsDefaultBoolean(wxT("script.show_syntax_highlighting"));
        m_pi->script_show_line_numbers = getAppPrefsDefaultBoolean(wxT("script.show_line_numbers"));
        m_pi->script_show_whitespace = getAppPrefsDefaultBoolean(wxT("script.show_whitespace"));
        m_pi->script_insert_spaces = getAppPrefsDefaultBoolean(wxT("script.insert_spaces"));
        m_pi->script_spaces_size = getAppPrefsDefaultLong(wxT("script.insert_spaces_count"));
        m_pi->script_tab_size = getAppPrefsDefaultLong(wxT("script.tab_size"));
        
        // restore the default settings to controls
        m_show_syntax_highlight->SetValue(m_pi->script_show_syntax_highlight);
        m_show_line_numbers->SetValue(m_pi->script_show_line_numbers);
        m_show_whitespace->SetValue(m_pi->script_show_whitespace);
        m_use_spaces->SetValue(m_pi->script_insert_spaces);
        m_spaces_size->SetValue(m_pi->script_spaces_size);
        m_tab_size->SetValue(m_pi->script_tab_size);
    }

    void onShowLineNumbers(wxCommandEvent& evt)
    {
        if (!m_show_line_numbers) return;
        bool checked = m_show_line_numbers->GetValue();
        m_pi->script_show_line_numbers = checked;
    }
    
    void onShowSyntaxHighlighting(wxCommandEvent& evt)
    {
        if (!m_show_syntax_highlight) return;
        bool checked = m_show_syntax_highlight->GetValue();
        m_pi->script_show_syntax_highlight = checked;
    }
    
    void onShowWhitespace(wxCommandEvent& evt)
    {
        if (!m_show_whitespace) return;
        bool checked = m_show_whitespace->GetValue();
        m_pi->script_show_whitespace = checked;
    }
    
    void onSpacesTabsChanged(wxCommandEvent& evt)
    {
        if (!m_use_spaces) return;
        bool spaces = m_use_spaces->GetValue();
        m_pi->script_insert_spaces = spaces;
        updateEnabled();
    }
    
    void onSpacesSpun(wxSpinEvent& evt)
    {
        if (!m_spaces_size) return;
        m_pi->script_spaces_size = m_spaces_size->GetValue();
    }
    
    void onSpacesChanged(wxCommandEvent& evt)
    {
        if (!m_spaces_size) return;
        m_pi->script_spaces_size = m_spaces_size->GetValue();
    }
    
    void onTabSpun(wxSpinEvent& evt)
    {
        if (!m_tab_size) return;
        m_pi->script_tab_size = m_tab_size->GetValue();
    }
    
    void onTabChanged(wxCommandEvent& evt)
    {
        if (!m_tab_size) return;
        m_pi->script_tab_size = m_tab_size->GetValue();
    }

private:

    PrefInfo* m_pi;
    
    wxCheckBox* m_show_line_numbers;
    wxCheckBox* m_show_syntax_highlight;
    wxCheckBox* m_show_whitespace;
    wxRadioButton* m_use_spaces;
    wxRadioButton* m_use_tabs;
    wxSpinCtrl* m_spaces_size;
    wxSpinCtrl* m_tab_size;
    
    DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(ScriptOptionsPage, wxPanel)
    EVT_CHECKBOX(ID_ShowLineNumbersCheckBox, ScriptOptionsPage::onShowLineNumbers)
    EVT_CHECKBOX(ID_ShowSyntaxHighlightCheckBox, ScriptOptionsPage::onShowSyntaxHighlighting)
    EVT_CHECKBOX(ID_ShowWhitespaceCheckBox, ScriptOptionsPage::onShowWhitespace)
    EVT_RADIOBUTTON(ID_UseSpacesRadioButton, ScriptOptionsPage::onSpacesTabsChanged)
    EVT_RADIOBUTTON(ID_UseTabsRadioButton, ScriptOptionsPage::onSpacesTabsChanged)
    EVT_SPINCTRL(ID_SpacesSpinCtrl, ScriptOptionsPage::onSpacesSpun)
    EVT_TEXT(ID_SpacesSpinCtrl, ScriptOptionsPage::onSpacesChanged)
    EVT_SPINCTRL(ID_TabSpinCtrl, ScriptOptionsPage::onTabSpun)
    EVT_TEXT(ID_TabSpinCtrl, ScriptOptionsPage::onTabChanged)
END_EVENT_TABLE()








// OptionsPanel implementation

BEGIN_EVENT_TABLE(OptionsPanel, wxPanel)
    EVT_SIZE(OptionsPanel::onSize)
    EVT_BUTTON(ID_GeneralOptionsButton, OptionsPanel::onButtonBarClicked)
    EVT_BUTTON(ID_InternetOptionsButton, OptionsPanel::onButtonBarClicked)
    EVT_BUTTON(ID_PrivacyOptionsButton, OptionsPanel::onButtonBarClicked)
    EVT_BUTTON(ID_DatasheetOptionsButton, OptionsPanel::onButtonBarClicked)
    EVT_BUTTON(ID_ReportOptionsButton, OptionsPanel::onButtonBarClicked)
    EVT_BUTTON(ID_ScriptOptionsButton, OptionsPanel::onButtonBarClicked)
    EVT_BUTTON(ID_RestoreDefaultsButton, OptionsPanel::onRestoreDefaultsClicked)
    EVT_BUTTON(wxID_OK, OptionsPanel::onOK)
    EVT_BUTTON(wxID_CANCEL, OptionsPanel::onCancel)
    EVT_BUTTON(wxID_APPLY, OptionsPanel::onApply)
END_EVENT_TABLE()


OptionsPanel::OptionsPanel()
{
    // load all preferences
    m_pi = new PrefInfo;
    loadPrefs(m_pi);
}

OptionsPanel::~OptionsPanel()
{
    if (m_pi)
        delete m_pi;
}


bool OptionsPanel::initDoc(IFramePtr frame,
                           IDocumentSitePtr doc_site,
                           wxWindow* docsite_wnd,
                           wxWindow* panesite_wnd)
{
    // create document's window
    bool result = Create(docsite_wnd,
                         -1,
                         wxDefaultPosition,
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxTAB_TRAVERSAL);
    if (!result)
        return false;

    // load bitmaps that prefs pages use
    g_droparrow_bmp = wxBitmap(xpm_dropdown_arrow);

    doc_site->setCaption(_("Options"));
    m_doc_site = doc_site;

    m_general_page = new GeneralOptionsPage(this, m_pi);
    m_internet_page = new InternetOptionsPage(this, m_pi);
    m_privacy_page = new PrivacyOptionsPage(this, m_pi);
    m_datasheet_page = new DatasheetOptionsPage(this, m_pi);
    m_report_page = new ReportOptionsPage(this, m_pi);
    m_script_page = new ScriptOptionsPage(this, m_pi);

    
    wxSize page_min_size, aggregate_min_size(0,0);
    page_min_size = m_general_page->GetSizer()->GetMinSize();
    aggregate_min_size = page_min_size;

    page_min_size = m_internet_page->GetSizer()->GetMinSize();
    aggregate_min_size.x = wxMax(aggregate_min_size.x, page_min_size.x);
    aggregate_min_size.y = wxMax(aggregate_min_size.y, page_min_size.y);

    page_min_size = m_privacy_page->GetSizer()->GetMinSize();
    aggregate_min_size.x = wxMax(aggregate_min_size.x, page_min_size.x);
    aggregate_min_size.y = wxMax(aggregate_min_size.y, page_min_size.y);

    page_min_size = m_datasheet_page->GetSizer()->GetMinSize();
    aggregate_min_size.x = wxMax(aggregate_min_size.x, page_min_size.x);
    aggregate_min_size.y = wxMax(aggregate_min_size.y, page_min_size.y);

    page_min_size = m_report_page->GetSizer()->GetMinSize();
    aggregate_min_size.x = wxMax(aggregate_min_size.x, page_min_size.x);
    aggregate_min_size.y = wxMax(aggregate_min_size.y, page_min_size.y);

    page_min_size = m_script_page->GetSizer()->GetMinSize();
    aggregate_min_size.x = wxMax(aggregate_min_size.x, page_min_size.x);
    aggregate_min_size.y = wxMax(aggregate_min_size.y, page_min_size.y);


    kcl::ButtonBar* button_bar = new kcl::ButtonBar(this, -1);
    button_bar->setItemMinSize(FromDIP(64), -1);
    
    button_bar->addItem(ID_GeneralOptionsButton, GETBMPMEDIUM(gf_switch),  _("General"));
    button_bar->addItem(ID_InternetOptionsButton, GETBMPMEDIUM(gf_globe), _("Internet"));
    button_bar->addItem(ID_PrivacyOptionsButton, GETBMPMEDIUM(gf_lock), _("Privacy"));
    button_bar->addItem(ID_DatasheetOptionsButton, GETBMPMEDIUM(gf_table), _("Datasheet"));
    button_bar->addItem(ID_ReportOptionsButton, GETBMPMEDIUM(gf_report), _("Report"));
    button_bar->addItem(ID_ScriptOptionsButton, GETBMPMEDIUM(gf_script), _("Script"));
    

    wxButton* restore_defaults_button = new wxButton(this,
                                              ID_RestoreDefaultsButton,
                                              _("Restore Defaults"));
    wxButton* ok_button = new wxButton(this, wxID_OK);


    // create a platform standards-compliant OK/Cancel sizer
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(ok_button);
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_APPLY));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->Prepend(restore_defaults_button, 0, wxALIGN_CENTER | wxLEFT, FromDIP(8));
    ok_cancel_sizer->AddSpacer(FromDIP(5));
    
    // this code is necessary to get the sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+FromDIP(16));
    ok_cancel_sizer->SetMinSize(min_size);
    
    
    // create main sizer
    
    m_sizer = new wxBoxSizer(wxVERTICAL);
    m_sizer->Add(button_bar, 0, wxEXPAND);
    m_sizer->AddSpacer(FromDIP(8));
    m_sizer->Add(m_general_page, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8));
    m_sizer->Add(m_internet_page, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8));
    m_sizer->Add(m_privacy_page, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8));
    m_sizer->Add(m_datasheet_page, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8));
    m_sizer->Add(m_report_page, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8));
    m_sizer->Add(m_script_page, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8));
    m_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);


    m_sizer->SetItemMinSize(m_general_page, aggregate_min_size);
    m_sizer->SetItemMinSize(m_internet_page, aggregate_min_size);
    m_sizer->SetItemMinSize(m_privacy_page, aggregate_min_size);
    m_sizer->SetItemMinSize(m_datasheet_page, aggregate_min_size);
    m_sizer->SetItemMinSize(m_report_page, aggregate_min_size);
    m_sizer->SetItemMinSize(m_script_page, aggregate_min_size);

    SetSizer(m_sizer);
    SetClientSize(m_sizer->GetMinSize());

    Layout();
    
    // start out on the general options page
    button_bar->selectItem(ID_GeneralOptionsButton);
    
    // make sure the ok button is selected by default
    ok_button->SetDefault();
    
    return true;
}

wxWindow* OptionsPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void OptionsPanel::setDocumentFocus()
{

}

void OptionsPanel::loadPrefs(PrefInfo* info)
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    
    // general preferences
    info->general_startup_location = getAppPrefsString(wxT("general.location.home"));
    info->general_startup_connection = getAppPrefsString(wxT("general.startup.connection"));
    info->general_startup_open_last_project = getAppPrefsBoolean(wxT("general.startup.open_last_project"));
    info->general_startup_action = getAppPrefsLong(wxT("general.startup.default_action"));
    info->general_toolbar_style = getAppPrefsLong(wxT("general.toolbar.style"));
    info->general_check_for_updates = getAppPrefsBoolean(wxT("general.updater.check_for_updates"));

    // internet preferences
    info->internet_proxy_type = getAppPrefsLong(wxT("internet.proxy.type"));
    info->internet_proxy_http = getAppPrefsString(wxT("internet.proxy.http"));
    info->internet_proxy_http_port = getAppPrefsLong(wxT("internet.proxy.http_port"));
    info->internet_proxy_ftp = getAppPrefsString(wxT("internet.proxy.ftp"));
    info->internet_proxy_ftp_port = getAppPrefsLong(wxT("internet.proxy.ftp_port"));
    info->internet_proxy_ssl = getAppPrefsString(wxT("internet.proxy.ssl"));
    info->internet_proxy_ssl_port = getAppPrefsLong(wxT("internet.proxy.ssl_port"));
    info->internet_proxy_socks = getAppPrefsString(wxT("internet.proxy.socks"));
    info->internet_proxy_socks_port = getAppPrefsLong(wxT("internet.proxy.socks_port"));

    // privacy preferences
    info->privacy_block_popups = getAppPrefsBoolean(wxT("privacy.popups.block"));
    info->privacy_accept_cookies = getAppPrefsBoolean(wxT("privacy.cookie.accept"));
    info->privacy_cookie_lifetime_policy = getAppPrefsLong(wxT("privacy.cookie.lifetime_policy"));
    info->privacy_cookie_lifetime_days = getAppPrefsLong(wxT("privacy.cookie.lifetime_days"));

    // grid preferences
    info->grid_font_ptsize = getAppPrefsLong(wxT("grid.font.point_size"));
    info->grid_font_family = getAppPrefsLong(wxT("grid.font.family"));
    info->grid_font_style = getAppPrefsLong(wxT("grid.font.style"));
    info->grid_font_weight = getAppPrefsLong(wxT("grid.font.weight"));
    info->grid_font_encoding = getAppPrefsLong(wxT("grid.font.encoding"));
    info->grid_font_underline = getAppPrefsBoolean(wxT("grid.font.underline"));
    info->grid_font_facename = getAppPrefsString(wxT("grid.font.face_name"));
    info->grid_greenbar_interval = getAppPrefsLong(wxT("grid.greenbar_interval"));
    info->grid_row_height = getAppPrefsLong(wxT("grid.row_height"));
    info->grid_background_color = getAppPrefsColor(wxT("grid.background_color"));
    info->grid_foreground_color = getAppPrefsColor(wxT("grid.foreground_color"));
    info->grid_line_color = getAppPrefsColor(wxT("grid.line_color"));
    info->grid_invalid_area_color = getAppPrefsColor(wxT("grid.invalid_area_color"));
    info->grid_greenbar_color = getAppPrefsColor(wxT("grid.greenbar_color"));
    info->grid_text_wrap = getAppPrefsBoolean(wxT("grid.text_wrap"));
    info->grid_boolean_checkbox = getAppPrefsBoolean(wxT("grid.boolean_checkbox"));
    info->grid_horzlines_visible = getAppPrefsBoolean(wxT("grid.horzlines_visible"));
    info->grid_vertlines_visible = getAppPrefsBoolean(wxT("grid.vertlines_visible"));
    info->grid_greenbar_visible = getAppPrefsBoolean(wxT("grid.greenbar_visible"));
    
    // report preferences (page dimensions and margin values are stored in hundreths of an inch)
    info->report_page_width = getAppPrefsLong(wxT("report.page.width"));
    info->report_page_height = getAppPrefsLong(wxT("report.page.height"));
    info->report_margin_left = getAppPrefsLong(wxT("report.margin.left"));
    info->report_margin_right = getAppPrefsLong(wxT("report.margin.right"));
    info->report_margin_top = getAppPrefsLong(wxT("report.margin.top"));
    info->report_margin_bottom = getAppPrefsLong(wxT("report.margin.bottom"));
    info->report_font_ptsize = getAppPrefsLong(wxT("report.font.size"));
    info->report_font_style = getAppPrefsString(wxT("report.font.style"));
    info->report_font_weight = getAppPrefsString(wxT("report.font.weight"));
    info->report_font_underscore = getAppPrefsString(wxT("report.font.underscore"));
    info->report_font_facename = getAppPrefsString(wxT("report.font.facename"));
    
    // script preferences
    info->script_show_syntax_highlight = getAppPrefsBoolean(wxT("script.show_syntax_highlighting"));
    info->script_show_line_numbers = getAppPrefsBoolean(wxT("script.show_line_numbers"));
    info->script_show_whitespace = getAppPrefsBoolean(wxT("script.show_whitespace"));
    info->script_insert_spaces = getAppPrefsBoolean(wxT("script.insert_spaces"));
    info->script_spaces_size = getAppPrefsLong(wxT("script.insert_spaces_count"));
    info->script_tab_size = getAppPrefsLong(wxT("script.tab_size"));
}

void OptionsPanel::savePrefs(PrefInfo* info)
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();

    // general preferences
    m_general_page->serializeValues();
    prefs->setString(wxT("general.location.home"), info->general_startup_location);
    prefs->setLong(wxT("general.startup.default_action"), info->general_startup_action);
    prefs->setString(wxT("general.startup.connection"), info->general_startup_connection);
    prefs->setBoolean(wxT("general.startup.open_last_project"), info->general_startup_open_last_project);
    prefs->setLong(wxT("general.toolbar.style"), info->general_toolbar_style);
    prefs->setBoolean(wxT("general.updater.check_for_updates"), info->general_check_for_updates);

    // internet preferences
    prefs->setLong(wxT("internet.proxy.type"), info->internet_proxy_type);
    prefs->setString(wxT("internet.proxy.http"), info->internet_proxy_http);
    prefs->setLong(wxT("internet.proxy.http_port"), info->internet_proxy_http_port);
    prefs->setString(wxT("internet.proxy.ftp"), info->internet_proxy_ftp);
    prefs->setLong(wxT("internet.proxy.ftp_port"), info->internet_proxy_ftp_port);
    prefs->setString(wxT("internet.proxy.ssl"), info->internet_proxy_ssl);
    prefs->setLong(wxT("internet.proxy.ssl_port"), info->internet_proxy_ssl_port);
    prefs->setString(wxT("internet.proxy.socks"), info->internet_proxy_socks);
    prefs->setLong(wxT("internet.proxy.socks_port"), info->internet_proxy_socks_port);

    // privacy preferences
    m_privacy_page->serializeValues();
    prefs->setBoolean(wxT("privacy.popups.block"), info->privacy_block_popups);
    prefs->setBoolean(wxT("privacy.cookie.accept"), info->privacy_accept_cookies);
    prefs->setLong(wxT("privacy.cookie.lifetime_policy"), info->privacy_cookie_lifetime_policy);
    prefs->setLong(wxT("privacy.cookie.lifetime_days"), info->privacy_cookie_lifetime_days);

    // grid preferences
    prefs->setColor(wxT("grid.background_color"), info->grid_background_color);
    prefs->setColor(wxT("grid.foreground_color"), info->grid_foreground_color);
    prefs->setColor(wxT("grid.line_color"), info->grid_line_color);
    prefs->setColor(wxT("grid.invalid_area_color"), info->grid_invalid_area_color);
    prefs->setColor(wxT("grid.greenbar_color"), info->grid_greenbar_color);
    prefs->setLong(wxT("grid.font.point_size"), info->grid_font_ptsize);
    prefs->setLong(wxT("grid.font.family"), info->grid_font_family);
    prefs->setLong(wxT("grid.font.style"), info->grid_font_style);
    prefs->setLong(wxT("grid.font.weight"), info->grid_font_weight);
    prefs->setLong(wxT("grid.font.encoding"), info->grid_font_encoding);
    prefs->setBoolean(wxT("grid.font.underline"), info->grid_font_underline);
    prefs->setString(wxT("grid.font.face_name"), info->grid_font_facename);
    prefs->setBoolean(wxT("grid.horzlines_visible"), info->grid_horzlines_visible);
    prefs->setBoolean(wxT("grid.vertlines_visible"), info->grid_vertlines_visible);
    prefs->setBoolean(wxT("grid.greenbar_visible"), info->grid_greenbar_visible);
    prefs->setLong(wxT("grid.greenbar_interval"), info->grid_greenbar_interval);
    prefs->setLong(wxT("grid.row_height"), info->grid_row_height);
    prefs->setBoolean(wxT("grid.text_wrap"), info->grid_text_wrap);
    prefs->setBoolean(wxT("grid.boolean_checkbox"), info->grid_boolean_checkbox);
    
    // report preferences (page dimensions and margin values are stored in hundreths of an inch)
    m_report_page->serializeValues();
    prefs->setLong(wxT("report.page.width"), info->report_page_width);
    prefs->setLong(wxT("report.page.height"), info->report_page_height);
    prefs->setLong(wxT("report.margin.left"), info->report_margin_left);
    prefs->setLong(wxT("report.margin.right"), info->report_margin_right);
    prefs->setLong(wxT("report.margin.top"), info->report_margin_top);
    prefs->setLong(wxT("report.margin.bottom"), info->report_margin_bottom);
    prefs->setLong(wxT("report.font.size"), info->report_font_ptsize);
    prefs->setString(wxT("report.font.style"), info->report_font_style);
    prefs->setString(wxT("report.font.weight"), info->report_font_weight);
    prefs->setString(wxT("report.font.underscore"), info->report_font_underscore);
    prefs->setString(wxT("report.font.facename"), info->report_font_facename);
    
    // script preferences
    prefs->setBoolean(wxT("script.show_syntax_highlighting"), info->script_show_syntax_highlight);
    prefs->setBoolean(wxT("script.show_line_numbers"), info->script_show_line_numbers);
    prefs->setBoolean(wxT("script.show_whitespace"), info->script_show_whitespace);
    prefs->setBoolean(wxT("script.insert_spaces"), info->script_insert_spaces);
    prefs->setLong(wxT("script.insert_spaces_count"), info->script_spaces_size);
    prefs->setLong(wxT("script.tab_size"), info->script_tab_size);
    
    prefs->flush();
    
    FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_PREFERENCES_SAVED);
    g_app->getMainFrame()->sendEvent(e);
}

void OptionsPanel::onOK(wxCommandEvent& evt)
{
    savePrefs(m_pi);
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void OptionsPanel::onCancel(wxCommandEvent& evt)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void OptionsPanel::onApply(wxCommandEvent& evt)
{
    savePrefs(m_pi);
}

void OptionsPanel::onSize(wxSizeEvent& evt)
{
    Layout();
}

void OptionsPanel::onButtonBarClicked(wxCommandEvent& evt)
{
    int id = evt.GetId();
    m_sizer->Show(m_general_page,   (id == ID_GeneralOptionsButton)   ? true : false, true);
    m_sizer->Show(m_internet_page,  (id == ID_InternetOptionsButton)  ? true : false, true);
    m_sizer->Show(m_privacy_page,   (id == ID_PrivacyOptionsButton)   ? true : false, true);
    m_sizer->Show(m_datasheet_page, (id == ID_DatasheetOptionsButton) ? true : false, true);
    m_sizer->Show(m_report_page,    (id == ID_ReportOptionsButton)    ? true : false, true);
    m_sizer->Show(m_script_page,    (id == ID_ScriptOptionsButton)    ? true : false, true);
    m_sizer->Layout();
}

void OptionsPanel::onRestoreDefaultsClicked(wxCommandEvent& evt)
{
    if (m_general_page->IsShown())
    {
        int res = appMessageBox(_("Restoring the default general settings will cause you to lose your current settings.\nDo you want to continue?"),
                                     _("Restore Defaults?"),
                                     wxYES_NO |
                                     wxNO_DEFAULT |
                                     wxICON_EXCLAMATION |
                                     wxCENTER,
                                     g_app->getMainWindow());
        if (res == wxYES)
            m_general_page->restoreDefaultPrefs();
        
        return;
    }
     else if (m_internet_page->IsShown())
    {
        int res = appMessageBox(_("Restoring the default Internet settings will cause you to lose your current settings.\nDo you want to continue?"),
                                     _("Restore Defaults?"),
                                     wxYES_NO |
                                     wxNO_DEFAULT |
                                     wxICON_EXCLAMATION |
                                     wxCENTER,
                                     g_app->getMainWindow());
        if (res == wxYES)
            m_internet_page->restoreDefaultPrefs();
        
        return;
    }
     else if (m_privacy_page->IsShown())
    {
        int res = appMessageBox(_("Restoring the default privacy settings will cause you to lose your current settings.\nDo you want to continue?"),
                                     _("Restore Defaults?"),
                                     wxYES_NO |
                                     wxNO_DEFAULT |
                                     wxICON_EXCLAMATION |
                                     wxCENTER,
                                     g_app->getMainWindow());
        if (res == wxYES)
            m_privacy_page->restoreDefaultPrefs();
        
        return;
    }
     else if (m_datasheet_page->IsShown())
    {
        int res = appMessageBox(_("Restoring the default datasheet settings will cause you to lose your current settings.\nDo you want to continue?"),
                                     _("Restore Defaults?"),
                                     wxYES_NO |
                                     wxNO_DEFAULT |
                                     wxICON_EXCLAMATION |
                                     wxCENTER,
                                     g_app->getMainWindow());
        if (res == wxYES)
            m_datasheet_page->restoreDefaultPrefs();
        
        return;
    }
     else if (m_report_page->IsShown())
    {
        int res = appMessageBox(_("Restoring the default report settings will cause you to lose your current settings.\nDo you want to continue?"),
                                     _("Restore Defaults?"),
                                     wxYES_NO |
                                     wxNO_DEFAULT |
                                     wxICON_EXCLAMATION |
                                     wxCENTER,
                                     g_app->getMainWindow());
        if (res == wxYES)
            m_report_page->restoreDefaultPrefs();
        
        return;
    }
     else if (m_script_page->IsShown())
    {
        int res = appMessageBox(_("Restoring the default script settings will cause you to lose your current settings.\nDo you want to continue?"),
                                     _("Restore Defaults?"),
                                     wxYES_NO |
                                     wxNO_DEFAULT |
                                     wxICON_EXCLAMATION |
                                     wxCENTER,
                                     g_app->getMainWindow());
        if (res == wxYES)
            m_script_page->restoreDefaultPrefs();
        
        return;
    }
}

void OptionsPanel::onComboTabPressed(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_TAB)
    {
        if (evt.ShiftDown())
            Navigate(wxNavigationKeyEvent::IsBackward);
             else
            Navigate(wxNavigationKeyEvent::IsForward);
    }
     else
    {
        evt.Skip();
    }
}


