/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-11
 *
 */


#include "appmain.h"
#include "extensionmgr.h"
#include "panelextensionmgr.h"


enum
{
    ID_AddExtensionsButton = wxID_HIGHEST + 1,
    ID_RunAtStartupCheckBox,
    ID_CancelUninstallButton,
    ID_UninstallButton,
    ID_StartNowButton,
};


// utility function for handling showing/hiding of selected/unselected items

bool isExtensionManaged(const wxString& extension_location)
{
    wxString extensions_dir = g_app->getAppDataPath();
    if (extensions_dir.Length() == 0 || extensions_dir.Last() != PATH_SEPARATOR_CHAR)
        extensions_dir += PATH_SEPARATOR_CHAR;
    extensions_dir += wxT("Extensions");

    // since extensions are stored as <APPDATA>/Extensions/<GUID>/<GUID>.kxt,
    // we need to go back two path separators to get the extensions folder path
    wxString folder = extension_location;
    folder = folder.BeforeLast(PATH_SEPARATOR_CHAR);
    folder = folder.BeforeLast(PATH_SEPARATOR_CHAR);
    
    // the extension is stored in the extensions folder, so it's managed
    if (folder.CmpNoCase(extensions_dir) == 0)
        return true;
    
    return false;
}

void selectItem(kcl::ScrollListItem* item, bool select = true)
{
    if (item == NULL)
        return;
    
    wxString guid = item->getExtraString();
    ExtensionInfo& info = g_app->getExtensionMgr()->getExtension(guid);
    
    bool managed = false;
    bool running = false;
    bool to_uninstall = false;
    if (info.isOk())
    {
        managed = isExtensionManaged(info.path);
        running = (info.state == ExtensionInfo::stateRunning) ? true : false;
        to_uninstall = info.to_uninstall;
    }
    
    item->setSelected(select);
    item->getElement(wxT("description"))->setTextWrap(select ? true : false);
    item->getElement(wxT("location"))->setVisible((select && !managed) ? true : false);
    item->getElement(wxT("filename"))->setVisible((select && !managed) ? true : false);
    item->getElement(wxT("startnow_button"))->setVisible((select && !running && !to_uninstall) ? true : false);
    
    kcl::ScrollListElement* ub = item->getElement(wxT("uninstall_button"));
    ub->setVisible(select && !to_uninstall ? true : false);
    
    kcl::ScrollListElement* cub = item->getElement(wxT("cancel_uninstall_button"));
    cub->setVisible(select && to_uninstall ? true : false);
}


BEGIN_EVENT_TABLE(ExtensionManagerPanel, wxPanel)
    EVT_BUTTON(ID_AddExtensionsButton, ExtensionManagerPanel::onAddExtensionsButtonClicked)
    EVT_BUTTON(ID_StartNowButton, ExtensionManagerPanel::onStartNowButtonClicked)
    EVT_BUTTON(ID_CancelUninstallButton, ExtensionManagerPanel::onCancelUninstallButtonClicked)
    EVT_BUTTON(ID_UninstallButton, ExtensionManagerPanel::onUninstallButtonClicked)
    EVT_CHECKBOX(ID_RunAtStartupCheckBox, ExtensionManagerPanel::onRunAtStartupChecked)
END_EVENT_TABLE()




ExtensionManagerPanel::ExtensionManagerPanel()
{
    m_doc_site = xcm::null;
}

ExtensionManagerPanel::~ExtensionManagerPanel()
{

}

// -- IDocument --
bool ExtensionManagerPanel::initDoc(IFramePtr frame,
                              IDocumentSitePtr site,
                              wxWindow* docsite_wnd,
                              wxWindow* panesite_wnd)
{
    if (!Create(docsite_wnd,
                -1,
                wxPoint(0,0),
                docsite_wnd->GetClientSize(),
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
    {
        return false;
    }

    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);

    m_doc_site = site;
    m_doc_site->setMinSize(200,150);
    m_doc_site->setCaption(_("Extensions"));

    // -- create controls for the panel --
    m_extension_list = new kcl::ScrollListControl(this, -1);
    m_addextensions_button = new wxButton(this, ID_AddExtensionsButton, _("Add Extensions..."));
    
    // this button is created so that the ESC key can be used to
    // close the panel -- it is completely hidden to the user
    wxButton* close_button;
    close_button = new wxButton(this, wxID_CANCEL, wxEmptyString, wxDefaultPosition, wxSize(0,0));
    
    // -- create the button sizer --
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->Add(m_addextensions_button, 0, wxALIGN_CENTER);
    button_sizer->Add(close_button, 0, wxALIGN_CENTER);
    
    // -- create the main sizer --
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_extension_list, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);

    SetSizer(main_sizer);
    Layout();
    
    // connect signals
    m_extension_list->sigSelectedItemChanged.connect(this, &ExtensionManagerPanel::onSelectedItemChanged);
    g_app->getExtensionMgr()->sigExtensionInstalled.connect(this, &ExtensionManagerPanel::onExtensionInstalled);
    g_app->getExtensionMgr()->sigExtensionStarted.connect(this, &ExtensionManagerPanel::onExtensionStarted);
    
    // populate the extension list
    populate();
    
    // show overlay text if we don't have any extensions
    checkOverlayText();
    
    // select the first item
    size_t first_item_idx = m_extension_list->getItemCount()-1;
    kcl::ScrollListItem* first_item = m_extension_list->getItem(first_item_idx);
    if (first_item != NULL)
        selectItem(first_item, true);
    
    m_extension_list->refresh();
    return true;
}

wxWindow* ExtensionManagerPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ExtensionManagerPanel::setDocumentFocus()
{

}

static wxBitmap getExtensionBitmap(const wxString& path)
{
    ExtensionInfo info = g_app->getExtensionMgr()->getInfo(path);
    if (info.bitmap.IsOk())
        return info.bitmap;
    return GETBMP(kx_extension_32);
}

void ExtensionManagerPanel::addItem(ExtensionInfo& info)
{
    // if we haven't retrieved the extension's bitmap
    // from the extension package, do so now
    if (!info.bitmap.IsOk())
        info.bitmap = getExtensionBitmap(info.path);

    wxPoint create_pt(-10000,-10000);
    
    kcl::ScrollListItem* item = new kcl::ScrollListItem;
    
    // -- create uninstall text element --
    wxString appname = APPLICATION_NAME;
    wxString uninstall_text = wxString::Format(_("This extension will be uninstalled when %s is restarted."),
                                               appname.c_str());
    kcl::ScrollListElement* uninstall;
    uninstall = item->addElement(uninstall_text, wxPoint(15,15));
    uninstall->setPadding(0,0,15,15);
    uninstall->setName(wxT("uninstall"));
    uninstall->setTextBold(true);
    uninstall->setTextWrap(false);
    uninstall->setVisible(false);
    
    // -- create bitmap element --
    kcl::ScrollListElement* bitmap;
    bitmap = item->addElement(info.bitmap);
    bitmap->setPadding(0,0,15,15);
    bitmap->setRelativePosition(uninstall,
                                kcl::ScrollListElement::positionBelow);
    bitmap->setName(wxT("bitmap"));

    // -- create name text element --
    kcl::ScrollListElement* name;
    name = item->addElement(info.name);
    name->setPadding(0,0,10,8);
    name->setRelativePosition(bitmap,
                              kcl::ScrollListElement::positionOnRight);
    name->setTextBold(true);
    name->setTextWrap(false);
    name->setName(wxT("name"));

    // -- create version text element --
    wxString version_text = wxString::Format(wxT("%d.%d.%d"), info.major_version,
                                                              info.minor_version,
                                                              info.subminor_version);
    kcl::ScrollListElement* version;
    version = item->addElement(version_text);
    version->setPadding(0,0,15,8);
    version->setRelativePosition(name,
                                 kcl::ScrollListElement::positionOnRight);
    version->setTextWrap(false);
    version->setName(wxT("version"));

    // -- create description text element --
    kcl::ScrollListElement* desc;
    desc = item->addElement(info.description);
    desc->setPadding(0,0,15,8);
    desc->setRelativePosition(name, kcl::ScrollListElement::positionBelow);
    desc->setStretchable(true);
    desc->setName(wxT("description"));
    
    // -- create location text element --
    wxString location_text = wxT("Location:");
    kcl::ScrollListElement* location;
    location = item->addElement(location_text);
    location->setPadding(0,0,5,0);
    location->setRelativePosition(desc,
                                  kcl::ScrollListElement::positionBelow);
    location->setVisible(false);
    location->setName(wxT("location"));
    
    kcl::ScrollListElement* filename;
    filename = item->addElement(info.path);
    filename->setPadding(0,0,15,0);
    filename->setRelativePosition(location,
                                  kcl::ScrollListElement::positionOnRight);
    filename->setStretchable(true);
    filename->setTextWrap(false);
    filename->setVisible(false);
    filename->setName(wxT("filename"));

    // -- create 'uninstall' button element --
    wxButton* uninstall_button = new wxButton(m_extension_list,
                                              ID_UninstallButton,
                                              _("Uninstall"),
                                              create_pt,
                                              wxDefaultSize,
                                              wxBU_EXACTFIT);
    uninstall_button->SetExtraStyle((long)item);
    
    kcl::ScrollListElement* uninstall2;
    uninstall2 = item->addElement(uninstall_button);
    uninstall2->setPadding(8,8,15,15);
    uninstall2->setRelativePosition(filename,
                                   kcl::ScrollListElement::positionBelow,
                                   kcl::ScrollListElement::alignRight);
    uninstall2->setVisible(false);
    uninstall2->setName(wxT("uninstall_button"));
    
    // -- create 'don't uninstall' button element --
    wxButton* cancel_uninstall_button = new wxButton(m_extension_list,
                                              ID_CancelUninstallButton,
                                              _("Don't Uninstall"),
                                              create_pt,
                                              wxDefaultSize,
                                              wxBU_EXACTFIT);
    cancel_uninstall_button->SetExtraStyle((long)item);
    
    kcl::ScrollListElement* cancel_uninstall;
    cancel_uninstall = item->addElement(cancel_uninstall_button);
    cancel_uninstall->setPadding(8,8,15,15);
    cancel_uninstall->setRelativePosition(filename,
                                   kcl::ScrollListElement::positionBelow,
                                   kcl::ScrollListElement::alignRight);
    cancel_uninstall->setVisible(false);
    cancel_uninstall->setName(wxT("cancel_uninstall_button"));
    
    // -- create 'start now' button element --
    wxButton* startnow_button = new wxButton(m_extension_list,
                                             ID_StartNowButton,
                                             _("Start Now"),
                                             create_pt,
                                             wxDefaultSize,
                                             wxBU_EXACTFIT);
    startnow_button->SetExtraStyle((long)item);
    
    kcl::ScrollListElement* startnow;
    startnow = item->addElement(startnow_button);
    startnow->setPadding(0,8,0,15);
    startnow->setRelativePosition(uninstall2,
                             kcl::ScrollListElement::positionOnLeft);
    startnow->setVisible(false);
    startnow->setName(wxT("startnow_button"));
        
    // -- create 'run at startup' checkbox element --
    wxCheckBox* runatstartup_checkbox = new wxCheckBox(m_extension_list,
                                                       ID_RunAtStartupCheckBox,
                                                       _("Run at startup"));
    runatstartup_checkbox->SetValue(info.run_at_startup);
    runatstartup_checkbox->SetExtraStyle((long)item);
    
    kcl::ScrollListElement* startup;
    startup = item->addElement(runatstartup_checkbox);
    startup->setRelativePosition(location,
                                 kcl::ScrollListElement::positionBelow);
    startup->setName(wxT("runatstartup_checkbox"));
    
    // make sure the 'run at startup' checkbox is middle-aligned
    // with the 'uninstall' or 'don't uninstall' buttons
    int tp = uninstall2->getPaddingTop() + 
               (uninstall2->getHeight()-startup->getHeight())/2;
    int bp = uninstall2->getPaddingBottom() +
               (uninstall2->getHeight()-startup->getHeight())/2;
    
    startup->setPadding(0,tp,8,bp);
    
    
    // assign the extension id and default value to the item
    item->setExtraString(info.guid);
    m_extension_list->addItem(item);
    
    // show overlay text if we don't have any extensions
    checkOverlayText();
    
    // make sure the item is not selected (handles bottom padding)
    selectItem(item, false);
    updateItem(item, info);
}

void ExtensionManagerPanel::updateItem(kcl::ScrollListItem* item,
                                       ExtensionInfo& info)
{
    wxString version_text = wxString::Format(wxT("%d.%d.%d"), info.major_version,
                                                              info.minor_version,
                                                              info.subminor_version);
    if (info.bitmap.IsOk())
        item->getElement(wxT("bitmap"))->setBitmap(info.bitmap);
    item->getElement(wxT("name"))->setText(info.name);
    item->getElement(wxT("version"))->setText(version_text);
    item->getElement(wxT("description"))->setText(info.description);
    
    // for pending extension uninstalls, gray out
    // the item and show a notification
    bool selected = item->isSelected();
    item->getElement(wxT("cancel_uninstall_button"))->setVisible(selected && info.to_uninstall);
    item->getElement(wxT("startnow_button"))->setVisible(selected && !info.to_uninstall);
    item->getElement(wxT("uninstall_button"))->setVisible(selected && !info.to_uninstall);
    item->getElement(wxT("uninstall"))->setVisible(info.to_uninstall);
    item->setEnabled(!info.to_uninstall);
}

bool ExtensionManagerPanel::selectExtension(const wxString& guid)
{
    size_t i, count = m_extension_list->getItemCount();
    for (i = 0; i < count; ++i)
    {
        kcl::ScrollListItem* item = m_extension_list->getItem(i);
        if (!item)
            continue;
        
        wxString item_guid = item->getExtraString();
        if (item_guid == guid)
            selectItem(item, true);
             else
            selectItem(item, false);
    }
    
    m_extension_list->refresh();
    return true;
}

void ExtensionManagerPanel::checkOverlayText()
{
    size_t count = m_extension_list->getItemCount();
    if (count == 0)
        m_extension_list->setOverlayText(wxEmptyString);
         else
        m_extension_list->setOverlayText(wxEmptyString);
}

void ExtensionManagerPanel::populate()
{
    m_extension_list->clearItems();
    
    ExtensionMgr* ext_mgr = g_app->getExtensionMgr();
    
    std::vector<ExtensionInfo>::iterator it;
    std::vector<ExtensionInfo>& extensions = ext_mgr->getExtensions();
    for (it = extensions.begin(); it != extensions.end(); ++it)
        addItem(*it);
}

void ExtensionManagerPanel::onExtensionInstalled(ExtensionInfo& info)
{
    selectExtension(info.guid);
}

void ExtensionManagerPanel::onExtensionStarted(ExtensionInfo& info)
{
    selectExtension(info.guid);
}

void ExtensionManagerPanel::onSelectedItemChanged(kcl::ScrollListItem* old_item,
                                                  kcl::ScrollListItem* new_item)
{
    selectItem(old_item, false);
    selectItem(new_item, true);
    m_extension_list->refresh();
}

void ExtensionManagerPanel::onAddExtensionsButtonClicked(wxCommandEvent& evt)
{
    wxString filter;
    filter  = _("Application Extension Files");
    filter += wxT(" (*.kxt)|*.kxt|");
    filter += _("Script Files");
    filter += wxT(" (*.js)|*.js|");
    filter += _("All Files");
    filter += wxT(" (*.*)|*.*|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxFileDialog dlg(this,
                     _("Choose File"),
                     wxT(""),
                     wxT(""),
                     filter,
                     wxFD_OPEN|wxFD_MULTIPLE|wxFD_FILE_MUST_EXIST);

    int res = dlg.ShowModal();
    if (res != wxID_OK)
        return;

    // the call to installExtension() will actually select the extension
    // that we just installed, so we need to freeze this window just in
    // case the user installs multiple extensions at the same time
    Freeze();

    std::vector<wxString> problem_extensions;
    bool refresh = false;
    
    wxArrayString as;
    dlg.GetPaths(as);

    ExtensionMgr* extmgr = g_app->getExtensionMgr();
    
    size_t i, count = as.GetCount();
    for (i = 0; i < count; ++i)
    {
        wxString s = as.Item(i);
        ExtensionInfo info = extmgr->installExtension(s);
        if (!info.isOk())
        {
            problem_extensions.push_back(s);
            continue;
        }
    
        // search through the extensions that have already been added
        // to the list and determine if we need to add this extension
        // to the list or update an item that already is in the list
        
        bool found = false;
        size_t j, list_count = m_extension_list->getItemCount();
        for (j = 0; j < list_count; ++j)
        {
            kcl::ScrollListItem* item = m_extension_list->getItem(j);
            wxString guid = item->getExtraString();
            
            // this extension is already in the list, update it
            if (guid.CmpNoCase(info.guid) == 0)
            {
                found = true;
                updateItem(item, info);
                break;
            }
        }
        
        // we couldn't find the extension in the list, add it
        if (!found)
        {
            addItem(info);
            selectExtension(info.guid);
        }
    } 
    
    // refresh the extension list
    m_extension_list->refresh();
    
    // thaw the window after the final refresh
    Thaw();
    
    if (problem_extensions.size() > 0)
    {
        wxString message = _("The following extensions were not installed because their extension format could not be determined:");
        message += wxT("\n");
        
        std::vector<wxString>::iterator it;
        for (it = problem_extensions.begin();
             it != problem_extensions.end(); ++it)
        {
            message += wxT("\n\t");
            message += *it;
        }

        appMessageBox(message,
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION,
                           g_app->getMainWindow());
        return;
    }
}

void ExtensionManagerPanel::onCancelUninstallButtonClicked(wxCommandEvent& evt)
{
    wxButton* button = (wxButton*)(evt.GetEventObject());
    kcl::ScrollListItem* item = (kcl::ScrollListItem*)(button->GetExtraStyle());

    // probably never happens.  If it does, something is not right, we have
    // a 'don't uninstall' button, but we can't find the extension item
    if (!item)
        return;

    wxString guid = item->getExtraString();
    ExtensionInfo& info = g_app->getExtensionMgr()->getExtension(guid);
    if (info.isOk())
    {
        info.to_uninstall = false;
        updateItem(item, info);
        m_extension_list->refresh();
        return;
    }
}

void ExtensionManagerPanel::onUninstallButtonClicked(wxCommandEvent& evt)
{
    wxButton* button = (wxButton*)(evt.GetEventObject());
    kcl::ScrollListItem* item = (kcl::ScrollListItem*)(button->GetExtraStyle());

    // probably never happens.  If it does, something is not right, we
    // have an 'uninstall' button, but we can't find the extension item
    if (!item)
        return;

    wxString guid = item->getExtraString();
    ExtensionInfo& info = g_app->getExtensionMgr()->getExtension(guid);
    if (info.isOk())
    {
        wxString message = wxString::Format(_("Are you sure you want to uninstall the '%s' extension?"),
                                            info.name.c_str());
        int result = appMessageBox(message,
                                        _("Extension Manager"),
                                        wxYES_NO | wxICON_QUESTION);
        if (result == wxYES)
        {
            info.to_uninstall = true;
            updateItem(item, info);
            m_extension_list->refresh();
            return;
        }
    }
}

void ExtensionManagerPanel::onStartNowButtonClicked(wxCommandEvent& evt)
{
    wxButton* button = (wxButton*)(evt.GetEventObject());
    kcl::ScrollListItem* item = (kcl::ScrollListItem*)(button->GetExtraStyle());

    // probably never happens.  If it does, something is not right, we
    // have a 'start now' button, but we can't find the extension item
    if (!item)
        return;

    wxString guid = item->getExtraString();
    ExtensionInfo& info = g_app->getExtensionMgr()->getExtension(guid);
    if (info.isOk())
        g_app->getExtensionMgr()->startExtension(guid);
}

void ExtensionManagerPanel::onRunAtStartupChecked(wxCommandEvent& evt)
{
    wxCheckBox* checkbox = (wxCheckBox*)(evt.GetEventObject());
    kcl::ScrollListItem* item = (kcl::ScrollListItem*)(checkbox->GetExtraStyle());

    // probably never happens.  If it does, something is not right, we have
    // a 'run at startup' checkbox, but we can't find the extension item
    if (!item)
        return;

    wxString guid = item->getExtraString();
    ExtensionInfo& info = g_app->getExtensionMgr()->getExtension(guid);
    if (info.isOk())
        g_app->getExtensionMgr()->enableExtension(guid, checkbox->IsChecked());
}


