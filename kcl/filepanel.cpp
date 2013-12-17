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
#include <wx/generic/dirctrlg.h>
#include "filepanel.h"



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

    InsertColumn(0, _("Name"),          wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
    InsertColumn(1, _("Date Modified"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
    InsertColumn(2, _("Type"),          wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);
    InsertColumn(3, _("Size"),          wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);

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

    DeleteAllItems();

    wxDir dir(m_curdir);

    if (!dir.IsOpened())
        return false;



    // Get the directories first (not matched against wildcards):
    wxString fname;
    bool more = dir.GetFirst(&fname, "", wxDIR_DIRS);
    while (more)
    {
        FileInfo fi;
        fi.folder = true;
        fi.name = fname;
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
            FileInfo fi;
            fi.folder = false;
            fi.name = fname;
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

        if (it->folder)
        {
            image_id = wxFileIconsTable::folder;
        }
         else
        {
            image_id = wxFileIconsTable::file;

            if (it->name.find('.') != it->name.npos)
            {
                ext = it->name.AfterLast('.');
                image_id = wxTheFileIconsTable->GetIconID(ext);
            }
        }

        this->InsertItem(idx, it->name, image_id);


        idx++;
    }

    return true;
}










enum
{
    ID_First = wxID_HIGHEST+1
};


BEGIN_EVENT_TABLE(FilePanel, wxNavigationEnabled<wxPanel>)
END_EVENT_TABLE()


FilePanel::FilePanel(wxWindow* parent, wxWindowID id) : wxPanel(parent,
                                                                id,
                                                                wxDefaultPosition,
                                                                wxDefaultSize,
                                                                wxTAB_TRAVERSAL | wxCLIP_CHILDREN | wxBORDER)
{
    wxBusyCursor bc;

    m_location_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(180,180), wxBORDER_NONE | wxTR_NO_BUTTONS | wxTR_NO_LINES | wxTR_HIDE_ROOT);

    m_location_tree->SetImageList(wxTheFileIconsTable->GetSmallImageList());


    wxTreeItemId root_id = m_location_tree->AddRoot("root");

    wxTreeItemId favorites_id = m_location_tree->AppendItem(root_id, _("Favorites"), wxFileIconsTable::folder);
    m_location_tree->AppendItem(favorites_id, _("Desktop"), wxFileIconsTable::folder);
    m_location_tree->AppendItem(favorites_id, _("Downloads"), wxFileIconsTable::folder);

    m_location_tree->AppendItem(root_id, ""); // spacer

    wxTreeItemId libraries_id = m_location_tree->AppendItem(root_id, _("Libraries"), wxFileIconsTable::folder);
    m_location_tree->AppendItem(libraries_id, _("Documents"), wxFileIconsTable::folder);
    m_location_tree->AppendItem(libraries_id, _("Music"), wxFileIconsTable::folder);
    m_location_tree->AppendItem(libraries_id, _("Pictures"), wxFileIconsTable::folder);
    m_location_tree->AppendItem(libraries_id, _("Videos"), wxFileIconsTable::folder);

    m_location_tree->AppendItem(root_id, ""); // spacer

    wxTreeItemId computer_id = m_location_tree->AppendItem(root_id, _("Computer"), wxFileIconsTable::computer);

    wxArrayString drives, drive_names;
    wxArrayInt drive_icons;
    wxGetAvailableDrives(drives, drive_names, drive_icons);

    for (size_t i = 0; i < drives.GetCount(); ++i)
    {
        m_location_tree->AppendItem(computer_id, drive_names[i], drive_icons[i]);
    }

    m_location_tree->ExpandAll();


    m_file_ctrl = new FileCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(150,150), wxLC_REPORT | wxBORDER_NONE);


    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer->Add(m_location_tree, 0, wxEXPAND);
    horz_sizer->Add(new DividerLine(this, -1, wxDefaultPosition, wxSize(1,1)), 0, wxEXPAND);
    horz_sizer->Add(m_file_ctrl, 1, wxEXPAND);

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(horz_sizer, 1, wxEXPAND);

    SetSizer(main_sizer);
}

FilePanel::~FilePanel()
{
}



}; // namespace kcl




