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
#include "filepanel.h"

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#endif


extern size_t wxGetAvailableDrives(wxArrayString &paths, wxArrayString &names, wxArrayInt &icon_ids);


namespace kcl
{



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
    SetImageList(wxTheFileIconsTable->GetSmallImageList(), wxIMAGE_LIST_SMALL);

    InsertColumn(0, _("Name"),          wxLIST_FORMAT_LEFT, 230);
    InsertColumn(1, _("Size"),          wxLIST_FORMAT_RIGHT, 80);
    InsertColumn(2, _("Date Modified"), wxLIST_FORMAT_LEFT, 135);
    InsertColumn(3, _("Type"),          wxLIST_FORMAT_LEFT, 80);

    
    goToDir("C:\\");
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
    ID_Filter_Choice
};


BEGIN_EVENT_TABLE(FilePanel, wxNavigationEnabled<wxPanel>)
    EVT_TREE_SEL_CHANGED(ID_Location_TreeCtrl, FilePanel::onTreeSelectionChanged)
END_EVENT_TABLE()


FilePanel::FilePanel(wxWindow* parent, wxWindowID id) : wxPanel(parent,
                                                                id,
                                                                wxDefaultPosition,
                                                                wxDefaultSize,
                                                                wxTAB_TRAVERSAL | wxCLIP_CHILDREN)
{
    m_filter_index = 0;

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



    m_location_tree = new wxTreeCtrl(this, ID_Location_TreeCtrl, wxDefaultPosition, wxSize(180,180), wxBORDER_NONE | wxTR_NO_BUTTONS | wxTR_NO_LINES | wxTR_HIDE_ROOT);
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

    m_file_ctrl = new FileCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(150,150), wxLC_REPORT | wxBORDER_NONE);


    m_path_ctrl = new wxTextCtrl(this,
                                 ID_Path_TextCtrl, 
                                 "",
                                 wxDefaultPosition,
                                 wxSize(200,23));

    m_filter_ctrl = new wxChoice(this,
                                 ID_Filter_Choice,
                                 wxDefaultPosition,
                                 wxSize(185,21),
                                 0,
                                 NULL);
    
    m_filter_ctrl->Append(_("All Files (*.*)"), (void*)0);  
    m_filter_ctrl->SetSelection(0);   

    

    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer->Add(m_location_tree, 0, wxEXPAND);
    horz_sizer->Add(new DividerLine(this, -1, wxDefaultPosition, wxSize(1,1)), 0, wxEXPAND);
    horz_sizer->Add(m_file_ctrl, 1, wxEXPAND);


    wxBoxSizer* path_sizer = new wxBoxSizer(wxHORIZONTAL);
    path_sizer->AddSpacer(15);
    path_sizer->Add(new wxStaticText(this, -1, _("File name:")), 0, wxALIGN_CENTER);
    path_sizer->AddSpacer(5);
    path_sizer->Add(m_path_ctrl, 1, wxEXPAND);
    path_sizer->AddSpacer(5);
    path_sizer->Add(m_filter_ctrl, 0, wxEXPAND);
    path_sizer->AddSpacer(5);

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(horz_sizer, 1, wxEXPAND);
    main_sizer->AddSpacer(8);
    main_sizer->Add(path_sizer, 0, wxEXPAND);

    SetSizer(main_sizer);
}

FilePanel::~FilePanel()
{
}


void FilePanel::setFilterString(const wxString& value)
{
    m_filter_string = value;

    wxWindowUpdateLocker freeze(m_filter_ctrl);

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

void FilePanel::onTreeSelectionChanged(wxTreeEvent& evt)
{
    wxTreeItemId id = evt.GetItem();
    if (id.IsOk())
    {
        LocationTreeData* item = (LocationTreeData*)m_location_tree->GetItemData(id);
        if (item)
        {
            m_file_ctrl->goToDir(item->loc);
        }
    }

}



}; // namespace kcl




