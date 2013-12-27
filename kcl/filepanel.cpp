/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2013-12-12
 *
 */



#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/filectrl.h>
#include <wx/treectrl.h>
#include <wx/dir.h>
#include <wx/tokenzr.h>
#include <wx/imaglist.h>
#include <wx/stdpaths.h>
#include <wx/wupdlock.h>
#include <wx/generic/dirctrlg.h>
#include <wx/numformatter.h>
#include <wx/mimetype.h>
#include <wx/artprov.h>
#include "filepanel.h"

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#endif


extern size_t wxGetAvailableDrives(wxArrayString &paths, wxArrayString &names, wxArrayInt &icon_ids);



IMPLEMENT_DYNAMIC_CLASS(kcl::FilePanelEvent, wxEvent)
wxDEFINE_EVENT( wxEVT_FILEPANEL_ITEM_ACTIVATED, kcl::FilePanelEvent );




namespace kcl
{

/* XPM */
static char* goparent_xpm[] = {
"16 16 5 1",
"  c None",
". c Black",
"O c #FFFF00",
"o c #EFE7D6",
"X c #FFF200",
"                ",
"  .....         ",
" .XoXoX.        ",
"..............  ",
".oXoXoXoOoXoXo. ",
".XoXo.oXoXoXoX. ",
".oXo...oXoXoXo. ",
".Xo.....oXoXoX. ",
".oXoO.OoXoXoXo. ",
".XoXo.oOoOoXoX. ",
".oXoX.......Xo. ",
".XoXoXoXoXoXoX. ",
".oXoXoXoXoXoXo. ",
"............... ",
"                ",
"                "
};

class DividerLine : public wxControl
{
public:

    DividerLine(wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize)
                     : wxControl(parent, id, pos, size, wxNO_BORDER) {}
private:

    void onPaint(wxPaintEvent& evt)
    {
        wxPaintDC dc(this);
        wxSize size = GetClientSize();
        dc.SetPen(*wxLIGHT_GREY_PEN);
        dc.DrawLine(0, 0, 0, size.y);
    }

    void onEraseBackground(wxEraseEvent& evt) { }
    void onSize(wxSizeEvent& evt) { Refresh(); }
    bool AcceptsFocus() const { return false; }

private:

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(DividerLine, wxControl)
    EVT_PAINT(DividerLine::onPaint)
    EVT_SIZE(DividerLine::onSize)
    EVT_ERASE_BACKGROUND(DividerLine::onEraseBackground)
END_EVENT_TABLE()






FileCtrl::FileCtrl(wxWindow* parent,
                   wxWindowID id,
                   const wxPoint& pos,
                   const wxSize& size,
                   long style) : wxListCtrl(parent, id, pos, size, style)
{
    m_folder_only = false;

    SetImageList(wxTheFileIconsTable->GetSmallImageList(), wxIMAGE_LIST_SMALL);

    InsertColumn(0, _("Name"),          wxLIST_FORMAT_LEFT, 230);
    InsertColumn(1, _("Size"),          wxLIST_FORMAT_RIGHT, 80);
    InsertColumn(2, _("Date Modified"), wxLIST_FORMAT_LEFT, 135);
    InsertColumn(3, _("Type"),          wxLIST_FORMAT_LEFT, 80);
}


bool FileCtrl::goToDir(const wxString& dir)
{
    if (!::wxDirExists(dir))
        return false;

    m_curdir = dir;
    populate();
    return true;
}



bool FileCtrl::populate()
{
    wxBusyCursor bc;
    wxLogNull nulllog;
    wxWindowUpdateLocker freeze(this);

    DeleteAllItems();
    m_files.clear();

    wxDir dir(m_curdir);

    if (!dir.IsOpened())
        return false;



    // Get the directories first (not matched against wildcards):
    wxString fname;
    bool more = dir.GetFirst(&fname, "", wxDIR_DIRS);
    while (more)
    {
        wxFileName fn;
        fn.AssignDir(m_curdir);
        fn.AppendDir(fname);

        FileInfo fi;
        fi.folder = true;
        fi.name = fname;
        fi.size = 0;
        fi.datetime = fn.GetModificationTime();
        m_files.push_back(fi);

        more = dir.GetNext(&fname);
    }

    if (!m_folder_only)
    {
        wxString filespec = m_filespec;
        if (filespec.IsEmpty())
            filespec = "*.*";

        wxStringTokenizer t(filespec, wxT(";"));
        while (t.HasMoreTokens())
        {
            bool more = dir.GetFirst(&fname, t.GetNextToken(), wxDIR_FILES);
            while (more)
            {
                wxFileName fn(m_curdir, fname);
            

                FileInfo fi;
                fi.folder = false;
                fi.name = fname;
                fi.size = fn.GetSize();
                fi.datetime = fn.GetModificationTime();
                m_files.push_back(fi);


                more = dir.GetNext(&fname);
            }
        }
    }


    // populate control
    int idx = 0;
    int image_id;
    wxString ext;

    std::vector<FileInfo>::iterator it;
    for (it = m_files.begin(); it != m_files.end(); ++it)
    {
        image_id = wxFileIconsTable::folder;


        if (it->name.find('.') != it->name.npos)
        {
            ext = it->name.AfterLast('.');
            image_id = wxTheFileIconsTable->GetIconID(ext);
        }
         else
        {
            ext = L"";
        }


        if (it->folder)
            image_id = wxFileIconsTable::folder;
             else
            image_id = wxFileIconsTable::file;

        this->InsertItem(idx, it->name, image_id);

        this->SetItemData(idx, idx);

        if (it->folder)
        {
            this->SetItem(idx, 3, _("File folder"));
        }
         else
        {
            if (ext.length() > 0)
            {
                wxString desc;
                wxFileType* ft = wxTheMimeTypesManager->GetFileTypeFromExtension(ext);
                if (ft)
                {
                    ft->GetDescription(&desc);
                }
                 else
                {
                    desc = ext;
                    if (desc.length() <= 3)
                        desc.MakeUpper();
                         else
                        desc.MakeCapitalized();
                    desc += " File";
                    this->SetItem(idx, 3, ext + " File");
                }

                this->SetItem(idx, 3, desc);
            }
             else
            {
                this->SetItem(idx, 3, _("Data File"));
            }
        }


        if (it->datetime.IsValid())
        {
            this->SetItem(idx, 2, it->datetime.Format());
        }
        
        if (!it->folder)
        {
            wxULongLong kb = it->size / 1024;
            wxString sizestr = wxNumberFormatter::ToString((long)kb.GetValue());
            sizestr += " KB";
            this->SetItem(idx, 1, sizestr);
        }

        idx++;
    }

    return true;
}



void FileCtrl::getSelection(std::vector<FileInfo>& files)
{
    files.clear();

    std::vector<wxString> selected;

    long item = -1;
    while (true)
    {
        item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1)
            break;

        long idx = GetItemData(item);
        files.push_back(m_files[idx]);
    }
}


void FileCtrl::setWildcard(const wxString& val)
{
    m_filespec = val;
    populate();
}

void FileCtrl::setFolderOnly(bool value)
{
    m_folder_only = value;
    populate();
}



class LocationTreeData : public wxTreeItemData
{
public:
    LocationTreeData(const wxString& _loc) { loc = _loc; }
    wxString loc;
};

enum
{
    ID_First = wxID_HIGHEST+1,
    ID_Location_TreeCtrl,
    ID_Path_TextCtrl,
    ID_File_Ctrl,
    ID_Filename_TextCtrl,
    ID_Filter_Choice,
    ID_GoParent_Button
};


BEGIN_EVENT_TABLE(FilePanel, wxNavigationEnabled<wxPanel>)
    EVT_TREE_SEL_CHANGING(ID_Location_TreeCtrl, FilePanel::onTreeSelectionChanging)
    EVT_TREE_SEL_CHANGED(ID_Location_TreeCtrl, FilePanel::onTreeSelectionChanged)
    EVT_LIST_ITEM_SELECTED(ID_File_Ctrl, FilePanel::onFileCtrlItemSelected)
    EVT_LIST_ITEM_ACTIVATED(ID_File_Ctrl, FilePanel::onFileCtrlItemActivated)
    EVT_TEXT_ENTER(ID_Filename_TextCtrl, FilePanel::onPathCtrlEnterPressed)
    EVT_CHILD_FOCUS(FilePanel::onChildFocus)
    EVT_IDLE(FilePanel::onIdle)
    EVT_BUTTON(ID_GoParent_Button, FilePanel::onGoParentClicked)
    EVT_CHOICE(ID_Filter_Choice, FilePanel::onFilterChoice)
END_EVENT_TABLE()


FilePanel::FilePanel(wxWindow* parent, wxWindowID id) : wxPanel(parent,
                                                                id,
                                                                wxDefaultPosition,
                                                                wxDefaultSize,
                                                                wxTAB_TRAVERSAL | wxCLIP_CHILDREN)
{
    m_filter_index = 0;
    m_filename_ctrl_focus_received = false;
    m_folder_only = false;

    wxBusyCursor bc;
    wxStandardPaths& paths = wxStandardPaths::Get();

    wxString home_dir = ::wxGetHomeDir();
    wxString documents_dir = paths.GetDocumentsDir();
#ifdef WIN32
    wxString desktop_dir = paths.MSWGetShellDir(CSIDL_DESKTOP);
    wxString downloads_dir = home_dir + "\\Downloads";
    wxString music_dir = paths.MSWGetShellDir(CSIDL_MYMUSIC);
    wxString pictures_dir = paths.MSWGetShellDir(CSIDL_MYPICTURES);
    wxString videos_dir = paths.MSWGetShellDir(CSIDL_MYVIDEO);
#else
    wxString desktop_dir = home_dir + "/Desktop";
    wxString downloads_dir = home_dir + "/Downloads";
    wxString music_dir = home_dir + "/Music";
    wxString videos_dir = home_dir + "/Videos";
#endif

    m_path_ctrl = new wxTextCtrl(this,
                                 ID_Path_TextCtrl, 
                                 "",
                                 wxDefaultPosition,
                                 wxSize(200,23),
                                 wxTE_PROCESS_ENTER);

    wxBitmapButton* goparent_button = new wxBitmapButton(this, ID_GoParent_Button, wxBitmap(goparent_xpm), wxDefaultPosition, wxSize(24,24));

    m_location_tree = new wxTreeCtrl(this, ID_Location_TreeCtrl, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTR_NO_BUTTONS | wxTR_NO_LINES | wxTR_HIDE_ROOT);
    m_location_tree->SetIndent(10);
    m_location_tree->SetImageList(wxTheFileIconsTable->GetSmallImageList());



    wxTreeItemId root_id = m_location_tree->AddRoot("root");

    wxTreeItemId favorites_id = m_location_tree->AppendItem(root_id, _("Favorites"), wxFileIconsTable::folder);
    m_location_tree->AppendItem(favorites_id, _("Desktop"), wxFileIconsTable::folder, -1, new LocationTreeData(desktop_dir));
    m_location_tree->AppendItem(favorites_id, _("Downloads"), wxFileIconsTable::folder, -1, new LocationTreeData(downloads_dir));

    m_location_tree->AppendItem(root_id, ""); // spacer

    wxTreeItemId libraries_id = m_location_tree->AppendItem(root_id, _("Libraries"), wxFileIconsTable::folder);
    m_location_tree->AppendItem(libraries_id, _("Documents"), wxFileIconsTable::folder, -1, new LocationTreeData(documents_dir));
    m_location_tree->AppendItem(libraries_id, _("Music"), wxFileIconsTable::folder, -1, new LocationTreeData(music_dir));
    m_location_tree->AppendItem(libraries_id, _("Pictures"), wxFileIconsTable::folder, -1, new LocationTreeData(pictures_dir));
    m_location_tree->AppendItem(libraries_id, _("Videos"), wxFileIconsTable::folder, -1, new LocationTreeData(videos_dir));

    m_location_tree->AppendItem(root_id, ""); // spacer

    wxTreeItemId computer_id = m_location_tree->AppendItem(root_id, _("Computer"), wxFileIconsTable::computer);

    wxArrayString drives, drive_names;
    wxArrayInt drive_icons;
    wxGetAvailableDrives(drives, drive_names, drive_icons);

    for (size_t i = 0; i < drives.GetCount(); ++i)
    {
        m_location_tree->AppendItem(computer_id, drive_names[i], drive_icons[i],  -1, new LocationTreeData(drives[i]));
    }

    m_location_tree->ExpandAll();

    m_file_ctrl = new FileCtrl(this, ID_File_Ctrl, wxDefaultPosition, wxSize(150,150), wxLC_REPORT | wxBORDER_NONE);


    m_filename_ctrl = new wxTextCtrl(this,
                                 ID_Filename_TextCtrl, 
                                 "",
                                 wxDefaultPosition,
                                 wxSize(200,23),
                                 wxTE_PROCESS_ENTER);

    m_filter_ctrl = new wxChoice(this,
                                 ID_Filter_Choice,
                                 wxDefaultPosition,
                                 wxSize(185,21),
                                 0,
                                 NULL);
    
    m_filter_ctrl->Append(_("All Files (*.*)"), (void*)0);  
    m_filter_ctrl->SetSelection(0);   


    // default to documents dir
    setDirectory(documents_dir);


    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxSizerItem* label_sizer_item = top_sizer->Add(new wxStaticText(this, -1, _("Look in:"), wxDefaultPosition, wxSize(175, -1), wxALIGN_RIGHT|wxST_NO_AUTORESIZE), 0, wxALIGN_CENTER);
    top_sizer->AddSpacer(5);
    top_sizer->Add(m_path_ctrl, 1, wxALIGN_CENTER);
    top_sizer->AddSpacer(2);
    top_sizer->Add(goparent_button, 0);
    top_sizer->AddSpacer(5);

    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxSizerItem* tree_sizer_item = horz_sizer->Add(m_location_tree, 0, wxEXPAND);
    tree_sizer_item->SetMinSize(180,100);
    horz_sizer->Add(new DividerLine(this, -1, wxDefaultPosition, wxSize(1,1)), 0, wxEXPAND);
    horz_sizer->Add(m_file_ctrl, 1, wxEXPAND);

    wxBoxSizer* path_sizer = new wxBoxSizer(wxHORIZONTAL);
    path_sizer->AddSpacer(15);
    path_sizer->Add(new wxStaticText(this, -1, _("File name:")), 0, wxALIGN_CENTER);
    path_sizer->AddSpacer(5);
    path_sizer->Add(m_filename_ctrl, 1, wxEXPAND);
    path_sizer->AddSpacer(5);
    path_sizer->Add(m_filter_ctrl, 0, wxEXPAND);
    path_sizer->AddSpacer(5);

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(top_sizer, 0, wxEXPAND);
    main_sizer->AddSpacer(5);
    main_sizer->Add(horz_sizer, 1, wxEXPAND);
    main_sizer->AddSpacer(8);
    main_sizer->Add(path_sizer, 0, wxEXPAND);

    SetSizer(main_sizer);
}

FilePanel::~FilePanel()
{
}


void FilePanel::setFolderOnly(bool value)
{
    m_folder_only = value;
    m_file_ctrl->setFolderOnly(value);
}


void FilePanel::setFilterString(const wxString& value)
{
    m_filter_string = value;

    wxWindowUpdateLocker freeze(m_filter_ctrl);

    m_wildcards.clear();
    while (m_filter_ctrl->GetCount())
        m_filter_ctrl->Delete(0);

    wxStringTokenizer t(m_filter_string, wxT("|"));
    while (t.HasMoreTokens())
    {
        wxString desc = t.GetNextToken();
        if (!t.HasMoreTokens())
            break;
        wxString wildcard = t.GetNextToken();

        m_filter_ctrl->Append(desc, (void*)m_wildcards.size());
        m_wildcards.push_back(wildcard);
    }

    if (m_filter_index >= 0 && m_filter_index < (int)m_filter_ctrl->GetCount())
        m_filter_ctrl->SetSelection(m_filter_index);
}

void FilePanel::setFilterIndex(int value)
{
    m_filter_index = value;
    if (m_filter_index >= 0 && m_filter_index < (int)m_filter_ctrl->GetCount())
        m_filter_ctrl->SetSelection(m_filter_index);
}


void FilePanel::setDirectory(const wxString& value)
{
    m_path_ctrl->SetValue(value);
    m_file_ctrl->goToDir(value);
}


void FilePanel::onTreeSelectionChanging(wxTreeEvent& evt)
{
    wxTreeItemId id = evt.GetItem();
    if (id.IsOk())
    {
        if (m_location_tree->GetItemText(id).Length() == 0)
        {
            evt.Veto();
        }
    }
}

void FilePanel::onGoParentClicked(wxCommandEvent& evt)
{
    wxString path = m_path_ctrl->GetValue();
    wxChar sep = wxFileName::GetPathSeparator();
    if (path.Last() == sep)
        path.RemoveLast();
    if (path.find(sep) == path.npos)
        return;
    path = path.BeforeLast(sep);
    
    if (path.length() == 2 && path[1] == ':')
        path += sep;

    wxDir dir(path);
    if (dir.IsOpened())
        setDirectory(path);
}

void FilePanel::onTreeSelectionChanged(wxTreeEvent& evt)
{
    wxTreeItemId id = evt.GetItem();
    if (id.IsOk())
    {
        LocationTreeData* item = (LocationTreeData*)m_location_tree->GetItemData(id);
        if (item)
        {
            setDirectory(item->loc);
        }
    }
}


wxString FilePanel::getFilename()
{
    return m_filename_ctrl->GetValue();
}

void FilePanel::getFilenames(std::vector<wxString>& result)
{
    result.clear();

    wxString str = m_filename_ctrl->GetValue();
    str.Trim(true);
    str.Trim(false);

    if (str.IsEmpty())
        return;

    bool quote = false;

    wxString cur;
    wxString::iterator it;

    for (it = str.begin(); it != str.end(); ++it)
    {
        if (*it == '"')
        {
            quote = !quote;
            continue;
        }

        if (!quote && *it == ' ')
        {
            if (cur.length() > 0)
            {
                result.push_back(cur);
                cur = "";
            }
            continue;
        }

        cur += *it;
    }

    if (cur.length() > 0)
        result.push_back(cur);
}

wxString FilePanel::getPath()
{
    wxString res = getFilename();
    wxFileName fn(m_file_ctrl->m_curdir, res);
    return fn.GetFullPath();
}

void FilePanel::getPaths(std::vector<wxString>& result)
{
    getFilenames(result);

    std::vector<wxString>::iterator it;
    for (it = result.begin(); it != result.end(); ++it)
    {
        wxFileName fn(m_file_ctrl->m_curdir, *it);
        *it = fn.GetFullPath();
    }
}


void FilePanel::onFileCtrlItemActivated(wxListEvent& evt)
{
    wxBusyCursor bc;

    std::vector<FileInfo> files;
    std::vector<FileInfo>::iterator it;

    m_file_ctrl->getSelection(files);

    if (files.size() == 1)
    {
        if (files[0].folder)
        {
            wxString path = m_path_ctrl->GetValue();
            wxChar sep = wxFileName::GetPathSeparator();
            if (path.Last() != sep)
                path += sep;
            path += files[0].name;

            setDirectory(path);
        }
         else
        {
            FilePanelEvent evt(wxEVT_FILEPANEL_ITEM_ACTIVATED, GetId());
            GetParent()->GetEventHandler()->ProcessEvent(evt);
        }
    }
}

void FilePanel::onFileCtrlItemSelected(wxListEvent& evt)
{
    std::vector<FileInfo> files;
    std::vector<FileInfo>::iterator it;

    m_file_ctrl->getSelection(files);

    wxString path_value;

    if (files.size() == 1)
    {
        if (m_folder_only)
        {
            if (files[0].folder)
                path_value = files[0].name;
        }
         else
        {
            if (!files[0].folder)
                path_value = files[0].name;
        }
    }
     else
    {
        for (it = files.begin(); it != files.end(); ++it)
        {
            if (it->folder)
                continue;
            if (path_value.length() > 0)
                path_value += " ";
            path_value += '"';
            path_value += it->name;
            path_value += '"';
        }
    }

    m_filename_ctrl->SetValue(path_value);
}


void FilePanel::onPathCtrlEnterPressed(wxCommandEvent& evt)
{
    wxString val = m_filename_ctrl->GetValue();
 
    if (val.find(wxFileName::GetPathSeparator()) != val.npos)
    {
        // path separator found; if the text control contains a valid directory,
        // update the current path

        wxDir dir(val);
        if (dir.IsOpened())
        {
            m_file_ctrl->goToDir(val);
        }
    }

}

void FilePanel::onFilterChoice(wxCommandEvent& evt)
{
    int sel = m_filter_ctrl->GetSelection();
    if (sel >= 0 && sel < (int)m_wildcards.size())
    {
        m_file_ctrl->setWildcard(m_wildcards[sel]);
    }
}


void FilePanel::onChildFocus(wxChildFocusEvent& evt)
{
    if (evt.GetWindow() == m_filename_ctrl)
    {
         m_filename_ctrl_focus_received = true;
    }

    evt.Skip();
}

void FilePanel::onIdle(wxIdleEvent& evt)
{
    if (m_filename_ctrl_focus_received)
    {
        m_filename_ctrl->SelectAll();
        m_filename_ctrl_focus_received = false;
    }
}


}; // namespace kcl




