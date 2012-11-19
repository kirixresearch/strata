/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-10-08
 *
 */


#include "appmain.h"
#include "toolbars.h"
#include "panelfind.h"
#include "panelconsole.h"
#include "exprbuilder.h"
#include "appcontroller.h"
#include "tabledoc.h"
#include "tangogridmodel.h"
#include "jobfindinfiles.h"
#include "dlgdatabasefile.h"


// -- FindPanel class implementation --

enum
{
    ID_FindModeCombo = 10000,
    ID_FindCombo,
    ID_FindInCombo,
    ID_ReplaceCombo,
    ID_FindNext,
    ID_FindPrev,
    ID_FindAll,
    ID_FindInBrowse,
    ID_Replace,
    ID_ReplaceAll,
    ID_MatchCase,
    ID_MatchWhole,
    ID_AllOpen,
    ID_SelectAll,
    ID_SelectNone
};


BEGIN_EVENT_TABLE(FindPanel, wxPanel)
    EVT_MENU(ID_Edit_Find, FindPanel::onModeChanged)
    EVT_MENU(ID_Edit_Replace, FindPanel::onModeChanged)
    EVT_MENU(ID_Edit_FindInFiles, FindPanel::onModeChanged)
    EVT_BUTTON(ID_FindPrev, FindPanel::onFindPrevNext)
    EVT_BUTTON(ID_FindNext, FindPanel::onFindPrevNext)
    EVT_BUTTON(ID_FindAll, FindPanel::onFindAll)
    EVT_BUTTON(ID_Replace, FindPanel::onReplace)
    EVT_BUTTON(ID_ReplaceAll, FindPanel::onReplaceAll)
    EVT_BUTTON(ID_FindInBrowse, FindPanel::onFindInBrowse)
    EVT_CHECKBOX(ID_MatchCase, FindPanel::onFindPrefsChanged)
    EVT_CHECKBOX(ID_MatchWhole, FindPanel::onFindPrefsChanged)
    EVT_CHECKBOX(ID_AllOpen, FindPanel::onFindPrefsChanged)
    EVT_CHOICE(ID_FindModeCombo, FindPanel::onModeCombo)
    EVT_TEXT(ID_FindCombo, FindPanel::onComboChanged)
    EVT_TEXT_ENTER(ID_FindCombo, FindPanel::onComboEnter)
    EVT_TEXT(ID_ReplaceCombo, FindPanel::onComboChanged)
    EVT_TEXT_ENTER(ID_ReplaceCombo, FindPanel::onComboEnter)
END_EVENT_TABLE()


FindPanel::FindPanel()
{
    m_find_sizer = NULL;
    m_replace_sizer = NULL;
    m_button_sizer = NULL;
    m_main_sizer = NULL;
    
    m_find_label = NULL;
    m_replace_label = NULL;
    
    m_find_combo = NULL;
    m_replace_combo = NULL;
    m_matchcase_checkbox = NULL;
    m_wholecell_checkbox = NULL;
    m_allopen_checkbox = NULL;
    
    m_findprev_button = NULL;
    m_findnext_button = NULL;
    m_replace_button = NULL;
    m_replaceall_button = NULL;

    m_caption = _("Find");
    
    wxString mode = g_app->getAppPreferences()->getString(wxT("find.default_mode"), wxT("find_in_files"));
    if (mode == wxT("find_in_files"))
        m_mode = ModeFindInFiles;
    else if (mode == wxT("find"))
        m_mode = ModeFind;
    else if (mode == wxT("find_and_replace"))
        m_mode = ModeFindReplace;
    else
        m_mode = ModeFind;
}

FindPanel::~FindPanel()
{
}

// -- IDocument --
bool FindPanel::initDoc(IFramePtr frame,
                        IDocumentSitePtr site,
                        wxWindow* docsite_wnd,
                        wxWindow* panesite_wnd)
{
    if (!Create(docsite_wnd,
                -1,
                wxDefaultPosition,
                wxDefaultSize,
                wxNO_FULL_REPAINT_ON_RESIZE |
                wxCLIP_CHILDREN |
                wxTAB_TRAVERSAL))
    {
        return false;
    }

    m_doc_site = site;

    // make sure that Ctrl-F and Ctrl-H work in this panel
    wxAcceleratorEntry entries[6];
    entries[0].Set(wxACCEL_CTRL, (int)'F', ID_Edit_Find);
    entries[1].Set(wxACCEL_CTRL, (int)'H', ID_Edit_Replace);
    entries[2].Set(wxACCEL_CTRL, WXK_F9, ID_Edit_FindInFiles);
    entries[3].Set(wxACCEL_NORMAL, WXK_F3, ID_FindNext);
    entries[4].Set(wxACCEL_SHIFT, WXK_F3, ID_FindPrev);
    entries[5].Set(wxACCEL_SHIFT, WXK_RETURN, ID_FindPrev);
    wxAcceleratorTable accel(6, entries);
    SetAcceleratorTable(accel);
    
    
    // -- create 'find mode' sizer --
    wxArrayString choices;
    choices.Add(_("Find In Files"));
    choices.Add(_("Find In Current Window"));
    choices.Add(_("Find And Replace"));
    
    m_find_mode_label = new wxStaticText(this, -1, _("Mode:"));
    m_find_mode_combo = new wxChoice(this,
                                  ID_FindModeCombo,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  choices,
                                  wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB);

    m_find_mode_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_find_mode_sizer->Add(m_find_mode_label, 0, wxALIGN_CENTER | wxLEFT, 2);
    m_find_mode_sizer->AddSpacer(5);
    m_find_mode_sizer->Add(m_find_mode_combo, 1);

    
    
    // -- create 'find' sizer --
    
    m_find_label = new wxStaticText(this, -1, _("Fi&nd:"));
    m_find_combo = new wxComboBox(this,
                                  ID_FindCombo,
                                  wxEmptyString,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  0,
                                  NULL,
                                  wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB);

    m_find_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_find_sizer->Add(m_find_label, 0, wxALIGN_CENTER | wxLEFT, 2);
    m_find_sizer->AddSpacer(5);
    m_find_sizer->Add(m_find_combo, 1);


    // -- create 'find in' sizer --
    
    m_find_in_label = new wxStaticText(this, -1, _("Look in:"));
    m_find_in_combo = new wxComboBox(this,
                                  ID_FindInCombo,
                                  wxEmptyString,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  0,
                                  NULL,
                                  wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB);
    
    wxString default_location = g_app->getAppPreferences()->getString(wxT("find.default_location"), wxT(""));
    if (default_location.Length() > 0)
    {
        m_find_in_combo->Append(default_location);
        m_find_in_combo->SetSelection(0);
    }
    
    wxButton* find_in_browse = new wxButton(this,
                                    ID_FindInBrowse,
                                    wxT("..."),
                                    wxDefaultPosition,
                                    wxSize(28,20));

    m_find_in_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_find_in_sizer->Add(m_find_in_label, 0, wxALIGN_CENTER | wxLEFT, 2);
    m_find_in_sizer->AddSpacer(5);
    m_find_in_sizer->Add(m_find_in_combo, 1);
    m_find_in_sizer->Add(find_in_browse, 0, wxALIGN_CENTER | wxLEFT, 2);


    // -- create replace sizer --
    
    m_replace_label = new wxStaticText(this, -1, _("Re&place with:"));
    m_replace_combo = new wxComboBox(this,
                                     ID_ReplaceCombo,
                                     wxEmptyString,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     0,
                                     NULL,
                                     wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB);
    
    m_replace_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_replace_sizer->Add(m_replace_label, 0, wxALIGN_CENTER | wxLEFT, 2);
    m_replace_sizer->AddSpacer(5);
    m_replace_sizer->Add(m_replace_combo, 1);

    
    // -- create options sizer --
    
    m_matchcase_checkbox = new wxCheckBox(this,
                                    ID_MatchCase,
                                    _("Match &case"));
    
    m_wholecell_checkbox = new wxCheckBox(this,
                                    ID_MatchWhole,
                                    _("Match &whole word"));
    
    m_allopen_checkbox = new wxCheckBox(this,
                                    ID_AllOpen,
                                    _("Look in all &open documents"));
    
    m_findprev_button = new wxButton(this,
                                    ID_FindPrev,
                                    _("Find &Previous"));

    m_findnext_button = new wxButton(this,
                                    ID_FindNext,
                                    _("&Find Next"));

    m_replace_button = new wxButton(this,
                                    ID_Replace,
                                    _("&Replace"));
    
    m_findall_button = new wxButton(this,
                                    ID_FindAll,
                                    _("Find &All"));
                                    
    m_replaceall_button = new wxButton(this,
                                    ID_ReplaceAll,
                                    _("Replace &All"));
    
    wxStaticBox* option_caption = new wxStaticBox(this, -1, _("Options"));
    wxStaticBoxSizer* option_sizer = new wxStaticBoxSizer(option_caption, wxVERTICAL);
    option_sizer->AddSpacer(8);
    option_sizer->Add(m_matchcase_checkbox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    option_sizer->Add(m_wholecell_checkbox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    option_sizer->Add(m_allopen_checkbox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    // we don't support multi-doc find yet
    option_sizer->Show(m_allopen_checkbox, false);
    
    // -- create button sizer --
    
    m_button_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_button_sizer->AddStretchSpacer();
    m_button_sizer->Add(m_findprev_button, 0, wxRIGHT, 8);
    m_button_sizer->Add(m_findnext_button, 0, wxRIGHT, 8);
    m_button_sizer->Add(m_replace_button, 0, wxRIGHT, 8);
    m_button_sizer->Add(m_replaceall_button, 0, wxRIGHT, 8);
    m_button_sizer->Add(m_findall_button, 0, wxRIGHT, 8);

    // create main sizer
    
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(8);
    m_main_sizer->Add(m_find_mode_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    m_main_sizer->Add(m_find_sizer,      0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    m_main_sizer->Add(m_find_in_sizer,   0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    m_main_sizer->Add(m_replace_sizer,   0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    m_main_sizer->Add(option_sizer,      1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    m_main_sizer->AddSpacer(8);
    m_main_sizer->Add(m_button_sizer,    0, wxEXPAND);
    m_main_sizer->AddSpacer(8);
    SetSizer(m_main_sizer);
    
    // this function takes care of laying out the panel, setting
    // the caption and also calls Layout() for us
    setMode(m_mode, true);

    // fill out option check box values from preferences
    bool match_case, whole_word, all_open_documents;
    getAppPrefsFindMatchCase(&match_case);
    getAppPrefsFindMatchWholeWord(&whole_word);
    getAppPrefsFindInAllOpenDocuments(&all_open_documents);

    m_matchcase_checkbox->SetValue(match_case);
    m_wholecell_checkbox->SetValue(whole_word);
    m_allopen_checkbox->SetValue(all_open_documents);

    g_app->getMainFrame()->sigFrameEvent().connect(this, &FindPanel::onFrameEvent);
    
    // make sure we don't lose our tab traversal ability
    m_find_combo->GetEventHandler()->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FindPanel::onComboTabPressed));
    m_replace_combo->GetEventHandler()->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FindPanel::onComboTabPressed));
    
    // make sure we can press ESC to close (hide) the panel
    const wxWindowList& children = GetChildren();
    int i, count = children.GetCount();
    for (i = 0; i < count; ++i)
    {
        wxWindow* wnd = children.Item(i)->GetData();
        if (wnd && !wnd->IsKindOf(CLASSINFO(wxComboBox)))
            wnd->GetEventHandler()->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FindPanel::onKeyDown));
    }

    // make sure the buttons are enabled/disabled on startup
    wxCommandEvent e;
    e.SetId(ID_FindCombo);
    onComboChanged(e);
    
    return true;
}


void FindPanel::setMode(int mode, bool layout)
{
    Freeze();
    
    m_mode = mode;
    
    // set the caption
    m_caption = _("Find");
    if (mode == FindPanel::ModeFindInFiles)
        m_caption = _("Find In Files");
    if (mode == FindPanel::ModeFindReplace)
        m_caption = _("Find and Replace");
    
    if (m_doc_site.isOk())
        m_doc_site->setCaption(m_caption);
    
    m_find_mode_combo->SetSelection(m_mode);

    // figure out label widths
    
    if (m_find_sizer != NULL && m_replace_sizer != NULL)
    {
        wxSize label_size;
        if (m_mode == ModeFindReplace)
        {
            label_size = getMaxTextSize(m_find_mode_label, m_find_label, m_replace_label);
            m_find_mode_sizer->SetItemMinSize(m_find_mode_label, label_size);
            m_find_sizer->SetItemMinSize(m_find_label, label_size);
            m_replace_sizer->SetItemMinSize(m_replace_label, label_size);
        }
         else if (m_mode == ModeFindInFiles)
        {
            label_size = getMaxTextSize(m_find_mode_label, m_find_label, m_find_in_label);
            m_find_mode_sizer->SetItemMinSize(m_find_mode_label, label_size);
            m_find_sizer->SetItemMinSize(m_find_label, label_size);
            m_find_in_sizer->SetItemMinSize(m_find_in_label, label_size);
        }
         else
        {
            label_size = getMaxTextSize(m_find_mode_label, m_find_label);
            m_find_mode_sizer->SetItemMinSize(m_find_mode_label, label_size);
            m_find_sizer->SetItemMinSize(m_find_label, label_size);
        }
    }
    
    // -- layout the button sizer --
    
    if (m_button_sizer != NULL)
    {
        if (m_mode == ModeFind)
        {
            if (!m_button_sizer->IsShown(m_findprev_button))
                m_button_sizer->Show(m_findprev_button, true);
            if (!m_button_sizer->IsShown(m_findnext_button))
                m_button_sizer->Show(m_findnext_button, true);
            if (m_button_sizer->IsShown(m_replace_button))
                m_button_sizer->Show(m_replace_button, false);
            if (m_button_sizer->IsShown(m_replaceall_button))
                m_button_sizer->Show(m_replaceall_button, false);
            if (m_button_sizer->IsShown(m_findall_button))
                m_button_sizer->Show(m_findall_button, false);
        }
         else if (m_mode == ModeFindInFiles)
        {
            if (m_button_sizer->IsShown(m_findprev_button))
                m_button_sizer->Show(m_findprev_button, false);
            if (m_button_sizer->IsShown(m_findnext_button))
                m_button_sizer->Show(m_findnext_button, false);
            if (m_button_sizer->IsShown(m_replace_button))
                m_button_sizer->Show(m_replace_button, false);
            if (m_button_sizer->IsShown(m_replaceall_button))
                m_button_sizer->Show(m_replaceall_button, false);
            if (!m_button_sizer->IsShown(m_findall_button))
                m_button_sizer->Show(m_findall_button, true);
            m_findall_button->SetDefault();
        }
         else if (m_mode == ModeFindReplace)
        {
            if (m_button_sizer->IsShown(m_findprev_button))
                m_button_sizer->Show(m_findprev_button, false);
            if (!m_button_sizer->IsShown(m_findnext_button))
                m_button_sizer->Show(m_findnext_button, true);
            if (!m_button_sizer->IsShown(m_replace_button))
                m_button_sizer->Show(m_replace_button, true);
            if (!m_button_sizer->IsShown(m_replaceall_button))
                m_button_sizer->Show(m_replaceall_button, true);
            if (m_button_sizer->IsShown(m_findall_button))
                m_button_sizer->Show(m_findall_button, false);
        }
    }
    
    // -- layout main sizer --
    
    if (m_main_sizer != NULL)
    {
        if (m_mode == ModeFind)
        {
            if (m_main_sizer->IsShown(m_replace_sizer))
                m_main_sizer->Show(m_replace_sizer, false, true);
            if (m_main_sizer->IsShown(m_find_in_sizer))
                m_main_sizer->Show(m_find_in_sizer, false, true);
        }
         else if (m_mode == ModeFindInFiles)
        {
            if (!m_main_sizer->IsShown(m_find_in_sizer))
                m_main_sizer->Show(m_find_in_sizer, true, true);
            if (m_main_sizer->IsShown(m_replace_sizer))
                m_main_sizer->Show(m_replace_sizer, false, true);
        }
         else if (m_mode == ModeFindReplace)
        {
            if (m_main_sizer->IsShown(m_find_in_sizer))
                m_main_sizer->Show(m_find_in_sizer, false, true);
            if (!m_main_sizer->IsShown(m_replace_sizer))
                m_main_sizer->Show(m_replace_sizer, true, true);
        }
         else
        {
            wxASSERT(0);
        }
    }
    
    // if we don't have a main sizer, we can't call Layout()
    if (layout && m_main_sizer != NULL)
        Layout();
    
    // set focus and selection to the find combobox control
    if (m_find_combo)
    {
        m_find_combo->SetFocus();
        m_find_combo->SetSelection(-1, -1);
    }
    
    if (m_findnext_button)
        m_findnext_button->SetDefault();
    
    if (m_doc_site.isOk())
    {
        int x,y,w,h;
        m_doc_site->getPosition(&x,&y,&w,&h);
        
        if (m_mode == FindPanel::ModeFind)
        {
            m_doc_site->setMinSize(340,215);
            m_doc_site->setPosition(x,y,340,225);
        }
         else if (m_mode == FindPanel::ModeFindInFiles)
        {
            m_doc_site->setMinSize(340,245);
            m_doc_site->setPosition(x,y,340,245);
        }
         else if (m_mode == FindPanel::ModeFindReplace)
        {
            m_doc_site->setMinSize(340,245);
            m_doc_site->setPosition(x,y,340,245);
        }
    }
    
    Thaw();
}

wxWindow* FindPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void FindPanel::setDocumentFocus()
{
    m_find_combo->SetFocus();
}

bool FindPanel::onSiteClosing(bool force)
{
    // NOTE: it is important to always return false in this function
    //       since we don't ever want to destroy the FindPanel until
    //       we close the application since we don't want to lose
    //       the values in the find and replace combobox dropdowns
    
    bool quickfilter_on = g_app->getAppController()->
                                 getStandardToolbar()->
                                 GetToolToggled(ID_Data_QuickFilter);
    
    // if the quick filter is turned on, just hide ourself, so we don't
    // mess up the existing value in the global find/filter combobox
    if (quickfilter_on)
    {
        m_doc_site->setVisible(false);
        return false;
    }
    
    // make sure this panel and the global find/filter box are in sync
    g_app->getAppController()->
           getStandardToolbar()->
           getFindCombo()->
           SetValue(m_find_combo->GetValue());
    
    // we'll just hide ourselves instead of closing ourselves
    m_doc_site->setVisible(false);
    return false;
}

void FindPanel::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == wxT("appmain.addFindComboItem"))
    {
        wxString val = evt.s_param;
        
        // if the find string doesn't exist
        // in the find combobox, add it
        if (m_find_combo->FindString(val) == wxNOT_FOUND && val.Length() > 0)
            m_find_combo->Insert(val, 0);
        
        // update the find combobox
        m_find_combo->SetValue(val);
        m_find_combo->SetSelection(-1, -1);
        m_findnext_button->SetDefault();
    }
}


void FindPanel::setFindValue(const wxString& val)
{
    bool enabled = (val.Length() > 0 ? true : false);
    m_findnext_button->Enable(enabled);
    m_findprev_button->Enable(enabled);
    m_replace_button->Enable(enabled);
    m_replaceall_button->Enable(enabled);
    
    // set focus and selection to the find combobox control
    if (m_find_combo)
    {
        m_find_combo->SetValue(val);
        m_find_combo->SetFocus();
        m_find_combo->SetSelection(-1, -1);
    }
}

void FindPanel::addReplaceComboItem()
{
    wxString val = m_replace_combo->GetValue();
    
    // if the replace string doesn't exist
    // in the replace combobox, add it
    if (m_replace_combo->FindString(val) == wxNOT_FOUND && val.Length() > 0)
        m_replace_combo->Insert(val, 0);
}

void FindPanel::savePrefs()
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();

    prefs->setBoolean(wxT("find.match_case"), m_matchcase_checkbox->IsChecked());
    prefs->setBoolean(wxT("find.whole_cells"), m_wholecell_checkbox->IsChecked());
    //prefs->setBoolean(wxT("find.all_open_documents"), m_allopen_checkbox->IsChecked());
    prefs->flush();
}

void FindPanel::onFindPrefsChanged(wxCommandEvent& evt)
{
    savePrefs();
}

void FindPanel::onComboChanged(wxCommandEvent& evt)
{
    if (evt.GetId() == ID_FindCombo)
    {
        bool enabled = (evt.GetString().Length() > 0 ? true : false);
        m_findnext_button->Enable(enabled);
        m_findprev_button->Enable(enabled);
        m_replace_button->Enable(enabled);
        m_replaceall_button->Enable(enabled);
    }
}

void FindPanel::onComboEnter(wxCommandEvent& evt)
{
    if (evt.GetId() == ID_FindCombo)
    {
        wxCommandEvent e;
        
        if (m_mode == FindPanel::ModeFindInFiles)
        {
            onFindAll(e);
        }
         else
        {
            e.SetId(ID_FindNext);
            onFindPrevNext(e);
        }
    }
     else if (evt.GetId() == ID_ReplaceCombo)
    {
        // add the current replace combobox string to its dropdown list
        addReplaceComboItem();
    
        // do a find next on the value in the find combobox
        wxCommandEvent e;
        e.SetId(ID_FindNext);
        onFindPrevNext(e);
        
        // set the focus to the find combobox
        m_find_combo->SetFocus();
    }
}

void FindPanel::onComboTabPressed(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_TAB)
    {
        if (evt.ShiftDown())
            Navigate(wxNavigationKeyEvent::IsBackward);
             else
            Navigate(wxNavigationKeyEvent::IsForward);
    }
     else if (evt.GetKeyCode() == WXK_ESCAPE)
    {
        // NOTE: it is important to do a lookup here instead of using
        // the m_doc_site member variable as we're not connecting the
        // events to a specific instance of the FindPanel -- since this
        // is the case, m_doc_site is always NULL here
        IDocumentSitePtr site;
        site = g_app->getMainFrame()->lookupSite(wxT("FindPanel"));
        if (site.isOk())
            site->setVisible(false);
    }
     else
    {
        evt.Skip();
    }
}

void FindPanel::onFindInBrowse(wxCommandEvent& evt)
{
    DlgDatabaseFile dlg(this, DlgDatabaseFile::modeSelectFolder);
    if (dlg.ShowModal() != wxID_OK)
        return;

    m_find_in_combo->SetValue(dlg.getPath());
    m_find_combo->SetFocus();
}

void FindPanel::onModeCombo(wxCommandEvent& evt)
{
    int mode = m_find_mode_combo->GetSelection();
    setMode(mode, true);
    
    wxString mode_str;
    if (mode == ModeFindInFiles)
        mode_str = wxT("find_in_files");
    else if (mode == ModeFind)
        mode_str = wxT("find");
    else if (mode == ModeFindReplace)
        mode_str = wxT("find_and_replace");
    else
        mode_str = wxT("find");
        
    g_app->getAppPreferences()->setString(wxT("find.default_mode"), mode_str);
}


void FindPanel::onModeChanged(wxCommandEvent& evt)
{
    if (evt.GetId() == ID_Edit_Find)
        setMode(ModeFind, true);
    else if (evt.GetId() == ID_Edit_FindInFiles)
        setMode(ModeFindInFiles, true);
    else if (evt.GetId() == ID_Edit_Replace)
        setMode(ModeFindReplace, true);
}

void FindPanel::onFindPrevNext(wxCommandEvent& evt)
{
    wxString find_val = m_find_combo->GetValue();
    
    // fire an event to add this string to the find combobox dropdowns
    FrameworkEvent* cfw_evt = new FrameworkEvent(wxT("appmain.addFindComboItem"));
    cfw_evt->s_param = find_val;
    g_app->getMainFrame()->postEvent(cfw_evt);

    bool forward_direction;

    if (evt.GetId() == ID_FindPrev)
        forward_direction = false;
         else
        forward_direction = true;

    // TODO: we have to save the prefs even if we don't use the find panel
    // to find anything because these preferences are used by the toolbar
    // find and the user may use these to set those preferences without
    // actually using this panel to do the finding
    savePrefs();

    IDocumentSitePtr site;
    site = g_app->getMainFrame()->getActiveChild();
    if (site.isNull())
        return;

    IFindTargetPtr find_target = site->getDocument();
    if (find_target.isNull())
        return;

    find_target->findNextMatch(find_val,
                               forward_direction,
                               m_matchcase_checkbox->IsChecked(),
                               m_wholecell_checkbox->IsChecked());
}

void FindPanel::onFindAll(wxCommandEvent& evt)
{
    // clear console
    IDocumentSitePtr site = g_app->getMainFrame()->lookupSite(wxT("ConsolePanel"));
    if (site.isOk())
    {
        IConsolePanelPtr console = site->getDocument();
        if (console.isOk())
            console->clear();
    }
    
    wxString find_val = m_find_combo->GetValue();
    wxString find_in = m_find_in_combo->GetValue();
    
    if (find_in.Length() == 0)
    {
        appMessageBox(_("Please enter a search location."),
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION | wxCENTER);
        return;
    }

    // remember the file in location
    g_app->getAppPreferences()->setString(wxT("find.default_location"), find_in);
    
    std::vector<wxString> vec;
    
    wxStringTokenizer tkz(find_in, wxT(";"));
    while (tkz.HasMoreTokens())
    {
        wxString t = tkz.GetNextToken();
        t.Trim(false);
        t.Trim(true);
        vec.push_back(t);
    }
    
    FindInFilesJob* job = new FindInFilesJob;
    job->setInstructions(find_val,
                         m_matchcase_checkbox->IsChecked(),
                         m_wholecell_checkbox->IsChecked(),
                         vec);
    
    g_app->getJobQueue()->addJob(job, jobStateRunning);
    
    site = g_app->getMainFrame()->lookupSite(wxT("FindPanel"));
    if (site)
        site->setVisible(false);
}


void FindPanel::onReplace(wxCommandEvent& evt)
{
    // add the current replace combobox string to its dropdown list
    addReplaceComboItem();
    
    wxString find_val = m_find_combo->GetValue();
    wxString replace_val = m_replace_combo->GetValue();
    
    // fire an event to add this string to the find combobox dropdowns
    FrameworkEvent* cfw_evt = new FrameworkEvent(wxT("appmain.addFindComboItem"));
    cfw_evt->s_param = find_val;
    g_app->getMainFrame()->postEvent(cfw_evt);
    
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->getActiveChild();
    if (site.isNull())
        return;

    IFindTargetPtr find_target = site->getDocument();
    if (find_target.isNull())
        return;

    find_target->findReplaceWith(find_val,
                                 replace_val,
                                 true /* forward */,
                                 m_matchcase_checkbox->IsChecked(),
                                 m_wholecell_checkbox->IsChecked());
}

void FindPanel::onReplaceAll(wxCommandEvent& evt)
{
    // add the current replace combobox string to its dropdown list
    addReplaceComboItem();
    
    wxString find_val = m_find_combo->GetValue();
    wxString replace_val = m_replace_combo->GetValue();
    
    // fire an event to add this string to the find combobox dropdowns
    FrameworkEvent* cfw_evt = new FrameworkEvent(wxT("appmain.addFindComboItem"));
    cfw_evt->s_param = find_val;
    g_app->getMainFrame()->postEvent(cfw_evt);
    
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->getActiveChild();
    if (site.isNull())
        return;

    IFindTargetPtr find_target = site->getDocument();
    if (find_target.isNull())
        return;

    find_target->findReplaceAll(find_val,
                                replace_val,
                                m_matchcase_checkbox->IsChecked(),
                                m_wholecell_checkbox->IsChecked());
}

void FindPanel::onKeyDown(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_ESCAPE)
    {
        // NOTE: it is important to do a lookup here instead of using
        // the m_doc_site member variable as we're not connecting the
        // events to a specific instance of the FindPanel -- since this
        // is the case, m_doc_site is always NULL here
        IDocumentSitePtr site;
        site = g_app->getMainFrame()->lookupSite(wxT("FindPanel"));
        if (site.isOk())
            site->setVisible(false);
    }
     else
    {
        evt.Skip();
    }
}




BEGIN_EVENT_TABLE(FindComboControl, kcl::BitmapComboControl)
    EVT_TEXT_ENTER(-1, FindComboControl::onFind)
    EVT_COMBOBOX(-1, FindComboControl::onFind)
    EVT_SET_FOCUS(FindComboControl::onSetFocus)
    EVT_KILL_FOCUS(FindComboControl::onKillFocus)
END_EVENT_TABLE()

FindComboControl::FindComboControl(wxWindow* parent,
                 wxWindowID id,
                 kcl::BitmapComboPopup* popup)
                    : kcl::BitmapComboControl(parent, id, popup)
{
    setShowBitmap(false);
}

FindComboControl::~FindComboControl()
{
}

void FindComboControl::onFind(wxCommandEvent& evt)
{
    wxString val = GetValue();
    
    // fire an event to add this string to the find combobox dropdowns
    
    FrameworkEvent* cfw_evt = new FrameworkEvent(wxT("appmain.addFindComboItem"));
    cfw_evt->s_param = val;
    g_app->getMainFrame()->postEvent(cfw_evt);

    IDocumentSitePtr site;
    site = g_app->getMainFrame()->getActiveChild();
    if (site.isNull())
        return;
    
    // if the active child is a table and the quick filter is toggled on,
    // use the find/filter combobox as a quick filter instead of find
    
    bool quickfilter_on = g_app->getAppController()->
                                 getStandardToolbar()->
                                 GetToolToggled(ID_Data_QuickFilter);
    
    ITableDocPtr tabledoc = site->getDocument();
    if (quickfilter_on && tabledoc.isOk())
    {
        // remove the filter if the find/filter combobox is empty
        if (val.IsEmpty())
        {
            tabledoc->removeFilter();
            return;
        }
        
        // do a quick filter operation
        tabledoc->setQuickFilter(val);
    }
     else
    {
        // do a find operation
        IFindTargetPtr find_target = site->getDocument();
        if (find_target.isNull())
            return;

        bool forward_direction = true;
        if (::wxGetKeyState(WXK_SHIFT))
            forward_direction = false;

        bool match_case, whole_word;
        getAppPrefsFindMatchCase(&match_case);
        getAppPrefsFindMatchWholeWord(&whole_word);
        
        find_target->findNextMatch(val,
                                   forward_direction,   // true if forward
                                   match_case,
                                   whole_word);
    }
}
