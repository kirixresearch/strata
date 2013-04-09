/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-13
 *
 */


#include "appmain.h"
#include "dbcombo.h"


const int BMP_INDENT = 3;
const int BMP_PADDING = 5;


/* XPM */
static const char* xpm_path_arrow[] = {
"17 16 2 1",
"x c #000000",
"  c None",
"                 ",
"                 ",
"                 ",
"                 ",
"                 ",
"       x         ",
"       xx        ",
"       xxx       ",
"       xxxx      ",
"       xxx       ",
"       xx        ",
"       x         ",
"                 ",
"                 ",
"                 ",
"                 ",
};


// -- DbComboPopup class implementation --

void DbComboPopup::Init()
{
    // initialize member variables
    m_dbdoc = NULL;
}

bool DbComboPopup::Create(wxWindow* parent)
{
    // create popup control
    m_dbdoc = new DbDoc;
    m_dbdoc->setStyle(fsstyleTrackSelect);
    m_dbdoc->initAsWindow(parent,
                       -1,
                       wxPoint(0,0),
                       wxDefaultSize,
                       0);
    m_dbdoc->setDatabase(g_app->getDatabase(), wxT("/"));
    
    m_dbdoc->refresh();
    m_dbdoc->getFsPanel()->setDragDrop(false);
    m_dbdoc->getFsPanel()->sigItemActivated().disconnect();
    m_dbdoc->getFsPanel()->sigItemSelected().disconnect();
    m_dbdoc->getFsPanel()->sigItemSelected().connect(this, &DbComboPopup::onItemActivated);

    if (m_init_value.IsEmpty() || m_init_value == wxT("/"))
    {
        IFsItemPtr item = m_dbdoc->getFsPanel()->getRootItem();
        if (item.isOk())
        {
            m_dbdoc->getFsPanel()->selectItem(item);
            m_selected_item = item;
        }
    }
     else
    {
        IFsItemPtr item = m_dbdoc->getFsItemFromPath(m_init_value);
        if (item.isOk())
        {
            m_dbdoc->getFsPanel()->selectItem(item);
            m_selected_item = item;
        }
    }

    // make sure the dbdoc has a border
    wxWindow* wnd = m_dbdoc->getDocWindow();
    long flags = wnd->GetWindowStyle();
    wnd->SetWindowStyle(flags | kcl::DEFAULT_BORDER);
    
    // don't show the full path in the text control by default
    m_show_full_path = false;
    m_show_root_label_in_path = true;
    
    return true;
}

wxWindow* DbComboPopup::GetControl()
{
    if (!m_dbdoc)
        return NULL;

    // return pointer to the created control
    return m_dbdoc->getDocWindow();
}

void DbComboPopup::SetStringValue(const wxString& s)
{
    m_init_value = s;
    
    if (!m_dbdoc)
        return;
    
    IFsPanelPtr fspanel = m_dbdoc->getFsPanel();
    if (fspanel.isNull())
        return;
    
    if (s.IsEmpty() || s == wxT("/"))
    {
        IFsItemPtr item = fspanel->getRootItem();
        if (item.isOk())
        {
            fspanel->selectItem(item);
            m_selected_item = item;
        }
    }
     else
    {
        IFsItemPtr item = m_dbdoc->getFsItemFromPath(s);
        if (item.isOk())
        {
            fspanel->selectItem(item);
            m_selected_item = item;
        }
    }
}

wxString DbComboPopup::GetStringValue() const
{
    if (!m_dbdoc)
        return wxEmptyString;
    
    IFsPanelPtr fspanel = m_dbdoc->getFsPanel();
    if (fspanel.isNull())
        return wxEmptyString;
    
    // get the selected item path
    wxString obj_path = getPath();
    
    // get the root path
    IFsItemPtr root = fspanel->getRootItem();
    IDbObjectFsItemPtr obj = root;
    if (obj.isNull())
        return wxEmptyString;
    wxString root_path = obj->getPath();
    
    // trim the root path from the object path
    wxString retval;
    obj_path.StartsWith(root_path.c_str(), &retval);
    if (retval.IsEmpty())
        retval = obj_path;
    
    return retval;
}

static wxArrayString getSlashTokens(const wxString& text)
{
    wxArrayString retval;
    wxStringTokenizer tkz(text, wxT("/"));
    while (tkz.HasMoreTokens())
    {
        wxString t = tkz.GetNextToken();
        if (t.Length() > 0)
            retval.Add(t);
    }
    
    return retval;
}

void DbComboPopup::PaintComboControl(wxDC& dc, const wxRect& rect)
{
    if (!m_dbdoc)
        return;
    
    IFsPanelPtr fspanel = m_dbdoc->getFsPanel();
    if (fspanel.isNull())
        return;
    
    // get the selected item's bitmap and label
    IFsItemPtr item = m_selected_item;
    if (item.isNull())
        return;
    
    // for paths that have slashes in them
    wxArrayString text_parts;
    
    wxBitmap bmp  = item->getBitmap();
    wxString text;
    if (m_show_full_path)
    {
        text = getFsItemPath(item);
        
        // still show the root label
        if (text.CmpNoCase(m_root_path) == 0)
            text = item->getLabel();
        
        // remove root path from the text
        if (text.Length() > 0 && text.Find(m_root_path) != wxNOT_FOUND)
            text.Remove(0, m_root_path.Length());
        
        text_parts = getSlashTokens(text);
    }
     else
    {
        text = item->getLabel();
    }
    
    // get the text's width and height
    int text_w, text_h;
    dc.GetTextExtent(text, &text_w, &text_h);
    
    // calculate the bitmap position
    int bmp_x = rect.x+BMP_INDENT;
    int bmp_y = rect.y+(rect.height-bmp.GetHeight())/2;
    
    // this is totally custom and should be reviewed at some point:
    // the project icon is drawn a little too high in its 16x16 box,
    // so move it down a pixel
    if (item == fspanel->getRootItem())
        bmp_y++;
    
    // draw text parts with black triangle separators if there
    // were slashes in the path (this is only the case if the
    // m_show_full_path parameter is true)
    if (text_parts.GetCount() > 0)
    {
        int x = bmp_x + bmp.GetWidth();
        wxBitmap arrow_bmp  = wxBitmap(xpm_path_arrow);
        size_t count = text_parts.GetCount();
        
        if (m_show_root_label_in_path &&
            m_root_label.Length() > 0 &&
            m_root_label.CmpNoCase(text_parts.Item(0)) != 0)
        {
            // draw the black triangle separator after the root label
            dc.DrawBitmap(arrow_bmp, x, bmp_y, true);
            x += arrow_bmp.GetWidth();
            
            // draw the root label
            dc.GetTextExtent(m_root_label, &text_w, &text_h);
            dc.DrawText(m_root_label, x, rect.y+(rect.height-text_h)/2);
            x += text_w;
        }
        
        for (size_t i = 0; i < count; ++i)
        {
            // draw the black triangle separator after the root label
            dc.DrawBitmap(arrow_bmp, x, bmp_y, true);
            x += arrow_bmp.GetWidth();
            
            // draw the folder label (one of the text parts)
            wxString s = text_parts.Item(i);
            dc.GetTextExtent(s, &text_w, &text_h);
            dc.DrawText(s, x, rect.y+(rect.height-text_h)/2);
            x += text_w;
        }
    }
     else
    {
        // calculate the text offset
        int text_x = bmp_x + bmp.GetWidth() + BMP_PADDING;
    
        // just draw the item's text
        dc.DrawText(text, text_x, rect.y+(rect.height-text_h)/2);
    }
    
    // draw the item's bitmap
    dc.DrawBitmap(bmp, bmp_x, bmp_y, true);
}

// this function was copied from DlgDatabaseFile
void DbComboPopup::setRootFolder(const wxString& path, const wxString& root_label)
{
    if (!m_dbdoc)
        return;
    
    m_root_path = path;
    m_root_label = root_label;
    
    DbFolderFsItem* root = new DbFolderFsItem;
    root->setPath(m_root_path);
    root->setDatabase(g_app->getDatabase());
    root->setFoldersOnly(true);
    
    wxString label = root_label;
    if (label.IsEmpty())
    {
        tango::IDatabasePtr db = g_app->getDatabase();
        tango::IAttributesPtr attr = db->getAttributes();
        
        wxString project_name;
        
        if (attr)
            project_name = attr->getStringAttribute(tango::dbattrDatabaseName);

        if (project_name.Length() > 0)
            label = wxString::Format(_("Project '%s'"), project_name.c_str());
             else
            label = _("Project");
        
        m_root_label = label;
    }
    
    root->setLabel(label);
    root->setBitmap(GETBMP(gf_project_16));

    m_selected_item = root;

    m_dbdoc->getFsPanel()->setRootItem(root);
    m_dbdoc->getFsPanel()->setMultiSelect(false);
    m_dbdoc->getFsPanel()->setView(fsviewTree);
    m_dbdoc->getFsPanel()->refresh();
}

void DbComboPopup::setShowFullPath(bool show_full_path,
                                   bool show_root_label_in_path)
{
    m_show_full_path = show_full_path;
    m_show_root_label_in_path = show_root_label_in_path;
}

wxString DbComboPopup::getPath() const
{
    // get the object path
    IDbObjectFsItemPtr obj = m_selected_item;
    if (obj.isNull())
        return wxEmptyString;
    wxString obj_path = obj->getPath();
    return obj_path;
}

void DbComboPopup::onItemActivated(IFsItemPtr item)
{
    m_selected_item = item;
    Dismiss();
    
    sigItemSelected();
}




// -- DbComboCtrl class implementation --

DbComboCtrl::DbComboCtrl(wxWindow* parent, wxWindowID id)
                        : wxComboCtrl(parent,
                                      id,
                                      wxEmptyString,
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      wxCB_READONLY | wxCC_STD_BUTTON)
{
    m_popup = new DbComboPopup;
    m_popup->sigItemSelected.connect(this, &DbComboCtrl::onItemSelected);
    SetPopupControl(m_popup);
    SetPopupMaxHeight(240);
    SetPopupMinWidth(160);
}

void DbComboCtrl::SetValue(const wxString& value)
{
    wxComboCtrl::SetValue(value);
    wxString combo_value = value.AfterLast(wxT('/'));
    if (combo_value.IsEmpty())
        combo_value = m_root_label;
    wxComboCtrl::SetText(combo_value);
}

void DbComboCtrl::setPopupRootFolder(const wxString& path,
                                     const wxString& label)
{
    m_root_path = path;
    m_root_label = label;
    m_popup->setRootFolder(path, label);
    SetValue(wxT("/"));
}

void DbComboCtrl::setShowFullPath(bool show_full_path,
                                  bool show_root_label_in_path)
{
    m_popup->setShowFullPath(show_full_path, show_root_label_in_path);
}

void DbComboCtrl::setPath(const wxString& new_path)
{
    m_popup->SetStringValue(new_path);
}

wxString DbComboCtrl::getPath()
{
    if (!m_popup)
        return wxEmptyString;
    
    return m_popup->getPath();
}

void DbComboCtrl::onItemSelected()
{
    sigItemSelected();
}


