/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   David Z. Williams
 * Created:  2005-11-09
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <wx/wx.h>
#include <wx/tokenzr.h>
#include <xcm/xcm.h>
#include <kl/klib.h>
#include "fspanel.h"
#include "fsitems_private.h"
#include "fspaneltreeview.h"
#include "fspanellistview.h"
#include "util.h"
#include <algorithm>


static const char* xpm_folder[] = {
"16 16 6 1",
"  c None",
"! c #000000",
"# c #808080",
"$ c #C0C0C0",
"% c #FFFF00",
"& c #FFFFFF",
"                ",
"   #####        ",
"  #%$%$%#       ",
" #%$%$%$%###### ",
" #&&&&&&&&&&&&#!",
" #&%$%$%$%$%$%#!",
" #&$%$%$%$%$%$#!",
" #&%$%$%$%$%$%#!",
" #&$%$%$%$%$%$#!",
" #&%$%$%$%$%$%#!",
" #&$%$%$%$%$%$#!",
" #&%$%$%$%$%$%#!",
" ##############!",
"  !!!!!!!!!!!!!!",
"                ",
"                "};


static const char* xpm_file[] = {
"16 16 4 1",
"  c None",
"! c #000000",
"# c #FFFFFF",
"$ c #808080",
"                ",
"  $$$$$$$$$     ",
"  $######$#$    ",
"  $######$##$   ",
"  $######$$$$!  ",
"  $#########$!  ",
"  $#########$!  ",
"  $#########$!  ",
"  $#########$!  ",
"  $#########$!  ",
"  $#########$!  ",
"  $#########$!  ",
"  $#########$!  ",
"  $$$$$$$$$$$!  ",
"   !!!!!!!!!!!  ",
"                "};


namespace cfw
{


// -- GenericFsItem class implementation --


void GenericFsItem::addFsItem(IFsItemPtr item)
{
    m_fsitems.push_back(item);
}

void GenericFsItem::deleteFsItem(IFsItemPtr item)
{
    std::vector<IFsItemPtr>::iterator it;

    it = std::find(m_fsitems.begin(), m_fsitems.end(), item);
    if (it != m_fsitems.end())
    {
        m_fsitems.erase(it);
    }
}

IFsItemEnumPtr GenericFsItem::getChildren()
{
    xcm::IVectorImpl<IFsItemPtr>* vec = new xcm::IVectorImpl<IFsItemPtr>;

    std::vector<IFsItemPtr>::iterator it;
    
    for (it = m_fsitems.begin(); it != m_fsitems.end(); ++it)
    {
        vec->append(*it);
    }

    return vec;
}




// -- FileFsItem class implementation --


class FileFsItem : public FsItemBase,
                   public IFileFsItem
{

XCM_CLASS_NAME("cfw.FileFsItem")
XCM_BEGIN_INTERFACE_MAP(FileFsItem)
    XCM_INTERFACE_ENTRY(IFileFsItem)
    XCM_INTERFACE_CHAIN(FsItemBase)
XCM_END_INTERFACE_MAP()

private:

    wxString m_path;

public:

    void setPath(const wxString& path)
    {
        m_path = path;
    }

    wxString getPath()
    {
        return m_path;
    }
};




// -- DirectoryFsItem class implementation --


DirectoryFsItem::DirectoryFsItem()
{
    m_file_types = wxT("");

#ifdef WIN32
    m_path = wxT("C:\\");
#else
    m_path = wxT("/");
#endif

    m_default_file_bitmap = wxBitmap(xpm_file);
    m_default_folder_bitmap = wxBitmap(xpm_folder);

    setBitmap(m_default_folder_bitmap, fsbmpSmall);
}

DirectoryFsItem::~DirectoryFsItem()
{

}

void DirectoryFsItem::setPath(const wxString& path)
{
    setLabel(path);
    m_path = path;
}

void DirectoryFsItem::setVisibleFileTypes(const wxString& file_types)
{
    m_file_types = file_types;
}

void DirectoryFsItem::setDefaultFileBitmap(const wxBitmap& bitmap)
{
    m_default_file_bitmap = bitmap;
}

void DirectoryFsItem::setDefaultFolderBitmap(const wxBitmap& bitmap)
{
    m_default_folder_bitmap = bitmap;
}

void DirectoryFsItem::setFileTypeBitmap(const wxString& file_type, const wxBitmap& bitmap)
{
    wxString s = file_type;
    s.MakeUpper();
    m_file_type_bitmaps[s] = bitmap;
}

wxString DirectoryFsItem::getPath()
{
    return m_path;
}

wxString DirectoryFsItem::getLabel()
{
    wxChar slash_char;
    
    #ifdef WIN32
    slash_char = wxT('\\');
    #else
    slash_char = wxT('/');
    #endif
    
    wxString suffix_path = m_path.AfterLast(slash_char);
    
    if (suffix_path.IsEmpty())
        return m_path;
     else
        return suffix_path;
}

bool DirectoryFsItem::isDeferred()
{
    return true;
}

bool DirectoryFsItem::hasChildren()
{
    bool result = false;

    std::wstring wpath = towstr(m_path);

    if (!xf_get_directory_exist(wpath))
        return false;

    xf_dirhandle_t handle = xf_opendir(wpath);
    xf_direntry_t info;

    while (xf_readdir(handle, &info))
    {
        if (info.m_name == L"." ||
            info.m_name == L"..")
        {
            continue;
        }

        result = true;
        break;
    }

    xf_closedir(handle);

    return result;
}

IFsItemEnumPtr DirectoryFsItem::getChildren()
{
    xcm::IVectorImpl<IFsItemPtr>* vec = new xcm::IVectorImpl<IFsItemPtr>;

    // -- get the visible extensions --
    std::vector<wxString> extensions;
    wxStringTokenizer t(m_file_types, wxT(",;\n\t. "));
    while (t.HasMoreTokens())
    {
        wxString s = t.GetNextToken();
        s.MakeUpper();
        extensions.push_back(s);
    }


    std::vector<IFsItemPtr> folder_items;
    std::vector<IFsItemPtr> file_items;

    xf_dirhandle_t handle = xf_opendir(towstr(m_path));
    xf_direntry_t info;

    while (xf_readdir(handle, &info))
    {
        if (info.m_name == L"." ||
            info.m_name == L"..")
        {
            continue;
        }


        wxString new_path = m_path;

        if (new_path.Length() > 0)
        {
            wxChar slash_char;

            #ifdef WIN32
            slash_char = wxT('\\');
            #else
            slash_char = wxT('/');
            #endif

            if (new_path.Last() != slash_char)
            {
                new_path += slash_char;
            }
        }

        new_path += towx(info.m_name);


        if (info.m_type == xfFileTypeNormal)
        {
            bool visible = true;

            // -- get the file extension --
            const wchar_t* e;
            e = wcsrchr(info.m_name.c_str(), L'.');
            if (!e)
            {
                continue;
            }
            e++;

            wxString extension = towx(e);
            extension.MakeUpper();

            if (extensions.size() > 0)
            {
                visible = false;

                std::vector<wxString>::iterator it;
                for (it = extensions.begin();
                     it != extensions.end(); ++it)
                {
                    if (*it == extension)
                    {
                        visible = true;
                        break;
                    }
                }
            }

            wxBitmap bmp = m_file_type_bitmaps[extension];
            
            if (!bmp.Ok())
            {
                bmp = m_default_file_bitmap;
            }


            if (visible)
            {
                FileFsItem* item = new FileFsItem;
                item->setLabel(towx(info.m_name));
                item->setBitmap(bmp, fsbmpSmall);
                item->setPath(new_path);
                file_items.push_back(item);
            }
        }
         else if (info.m_type == xfFileTypeDirectory)
        {
            DirectoryFsItem* item = new DirectoryFsItem;
            item->setLabel(towx(info.m_name));
            item->setBitmap(m_default_folder_bitmap, fsbmpSmall);
            item->setPath(new_path);
            folder_items.push_back(item);
        }
    }

    xf_closedir(handle);


    std::vector<IFsItemPtr>::iterator it;

    for (it = folder_items.begin(); it != folder_items.end(); ++it)
    {
        vec->append(*it);
    }

    for (it = file_items.begin(); it != file_items.end(); ++it)
    {
        vec->append(*it);
    }

    return vec;
}


};  // namespace cfw


