/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-12-09
 *
 */


#include "appmain.h"

#include "app.h"
#include "appcontroller.h"
#include "tabledoc.h"
#include "editordoc.h"
#include "querydoc.h"
#include "reportdoc.h"
#include "exportwizard.h"
#include "panelfileinfo.h"
#include "jsonconfig.h"
#include "dlgdatabasefile.h"
#include "projectmgr.h"
#include "importtemplate.h"
#include "dbdoc.h"
#include "structuredoc.h"
#include "connectionwizard.h"

#include <wx/generic/dirctrlg.h>

#ifdef WIN32
#include <windows.h>
#include <commctrl.h>
#endif


std::map<std::wstring, wxBitmap> g_custom_icons; // image md5/bitmap hash
std::vector<IFsItemPtr> g_cutcopy_items;
int g_cutcopy_action = 0;




// -- utility functions --

static wxString appendPath(const wxString& path1,
                           const wxString& path2)
{
    wxString result;
    result = path1;

    if (result.Length() > 0)
    {
        if (result.Last() == wxT('/') ||
            result.Last() == wxT('\\'))
        {
            result.RemoveLast();
        }
    }


    if (result.IsEmpty())
        result = wxT("/");

    if (result.Last() != wxT('/'))
        result += wxT("/");

    result += path2;

    result.Replace(wxT("\\"), wxT("/"));

    return result;
}

static std::wstring stripExtension(const std::wstring& s)
{
    if (s.find('.') == s.npos)
        return s;

    std::wstring ext = kl::afterLast(s, '.');
    kl::makeUpper(ext);
    
    if (ext == L"DBF" || ext == L"TXT" || ext == L"CSV" || ext == L"ICSV")
        return kl::beforeLast(s, '.');

    return s;
}

static wxString getDatabaseNameProjectLabel()
{
    xd::IDatabasePtr database = g_app->getDatabase();
    if (database.isNull())
        return wxT("");

    xd::IAttributesPtr attr = database->getAttributes();
    if (attr.isNull())
        return wxT("");

    wxString project_name = attr->getStringAttribute(xd::dbattrDatabaseName);

    wxString label;
    if (project_name.Length() > 0)
        label = wxString::Format(_("Project '%s'"), project_name.c_str());
         else
        label = _("Project");
    
    return label;
}


static std::wstring getDbDriver(xd::IDatabasePtr& db)
{
    if (db.isNull())
        return wxEmptyString;
    
    xcm::class_info* class_info = xcm::get_class_info(db.p);
    std::wstring s = towstr(class_info->get_name());
    return kl::beforeFirst(s, '.');
}



static wxMenu* createProjectsMenu(const std::vector<ProjectInfo>& projects,
                                  const wxString& active_project_location,
                                  int base_id)
{
    wxMenu* menu = new wxMenu;
    
    std::vector<ProjectInfo>::const_iterator it, it_end;
    it_end = projects.end();
    
    int i = 0;
    for (it = projects.begin(); it != it_end; ++it)
    {
        
        std::wstring location = it->location;
        if (kl::icontains(location, L"xdprovider="))
        {
            xd::ConnectionString cstr(location);
            std::wstring provider = cstr.getLowerValue(L"xdprovider");

            if (provider == L"xdnative" || provider == L"xdfs")
            {
                location = cstr.getLowerValue(L"database");
            }
        }

        wxString menu_name = it->name;
        menu_name.Append(L" - ");
        menu_name.Append(location);

        menu->AppendCheckItem(base_id+i, menu_name);
        
        // check the active project in the menu
        if (active_project_location == it->location)
            menu->Check(base_id+i, true);
            
        ++i;
    }

    menu->AppendSeparator();
    menu->Append(base_id+i, _("&Edit..."));
    return menu;
}



std::vector<std::pair<wxBitmap, wxBitmap> > g_shortcut_bitmaps;

wxBitmap getShortcutBitmap(const wxBitmap& input_bmp)
{
    static const char* xpm_shortcut[] = {
    "7 7 6 1",
    "X c #303030",
    "Y c #949494",
    "Z c #888888",
    "A c #DEDEDE",
    "B c #ECECEC",
    "  c #FFFFFF",
    "XXXXXXX",
    "X     X",
    "X  XX X",
    "X ZYX X",
    "X XA  X",
    "X BB  X",
    "XXXXXXX"};

    
    // check to see if we've already made a shortcut bitmap
    
    std::vector<std::pair<wxBitmap, wxBitmap> >::iterator it;
    for (it = g_shortcut_bitmaps.begin(); it != g_shortcut_bitmaps.end(); ++it)
    {
        if (it->first.IsSameAs(input_bmp))
            return it->second;
    }


    wxImage image = input_bmp.ConvertToImage();
    wxImage shortcut_image(xpm_shortcut);
    
    int x,y;
    int width = image.GetWidth();
    int height = image.GetHeight();
    
    for (y = 0; y < 7; ++y)
    {
        for (x = 0; x < 7; ++x)
        {
            unsigned char r = shortcut_image.GetRed(x,y);
            unsigned char g = shortcut_image.GetGreen(x,y);
            unsigned char b = shortcut_image.GetBlue(x,y);

            image.SetRGB(x, height+y-7, r, g, b);
            if (image.HasAlpha())
                image.SetAlpha(x, height+y-7, 255);
        }
    }
    
    wxBitmap result = wxBitmap(image);
    g_shortcut_bitmaps.push_back(std::pair<wxBitmap,wxBitmap>(input_bmp, result));
    
    return result;
}

static double blendColour(double fg, double bg, double alpha)
{
    double result = bg + (alpha * (fg - bg));
    if (result < 0.0)
        result = 0.0;
    if (result > 255)
        result = 255;
    return result;
}

// snagged from our toolbar code
static wxBitmap lightenBitmap(const wxBitmap& bmp)
{
    wxImage image = bmp.ConvertToImage();
    
    int mr, mg, mb;
    mr = image.GetMaskRed();
    mg = image.GetMaskGreen();
    mb = image.GetMaskBlue();

    unsigned char* data = image.GetData();
    int width = image.GetWidth();
    int height = image.GetHeight();
    bool has_mask = image.HasMask();

    for (int y = height-1; y >= 0; --y)
    {
        for (int x = width-1; x >= 0; --x)
        {
            data = image.GetData() + (y*(width*3))+(x*3);
            unsigned char* r = data;
            unsigned char* g = data+1;
            unsigned char* b = data+2;

            if (has_mask && *r == mr && *g == mg && *b == mb)
                continue;

            *r = (unsigned char)blendColour((double)*r, 255.0, 0.4);
            *g = (unsigned char)blendColour((double)*g, 255.0, 0.4);
            *b = (unsigned char)blendColour((double)*b, 255.0, 0.4);
        }
    }

    return wxBitmap(image);
}


// functor for sorting IFileInfoPtr objects

class FileInfoLess
{
public:

    bool isCloudFolder(const xd::IFileInfoPtr& f1) const
    {
        if (!f1->isMount())
            return false;

        std::wstring cstr, rpath;
        f1->getMountInfo(cstr, rpath);
        if (kl::icontains(cstr, L"dbdoccloudfolder="))
            return true;

        return false;
    }

    bool operator()(const xd::IFileInfoPtr& f1,
                    const xd::IFileInfoPtr& f2) const                
    {
        int f1_type = f1->getType();
        int f2_type = f2->getType();

        bool f1_cloud = isCloudFolder(f1);
        bool f2_cloud = isCloudFolder(f2);

        // we want cloud folders to be on top
        if (f1_cloud && f2_cloud)
            return false;
        if (f1_cloud && !f2_cloud)
            return true;
        if (f2_cloud)
            return false;
       
        const std::wstring& f1_name = f1->getName();
        const std::wstring& f2_name = f2->getName();
       
        if (f1_name == f2_name)
            return false;
       
        // folders always float to the top
       
        if (f1_type == xd::filetypeFolder || f2_type == xd::filetypeFolder)
        {
            if (f1_type == f2_type)
            {

                return wcscasecmp(f1_name.c_str(), f2_name.c_str()) < 0 ? true : false;
            }
             else if (f1_type == xd::filetypeFolder)
            {
                return true;
            }
             else
            {
                return false;
            }
        }
        
            
        return wcscasecmp(f1_name.c_str(), f2_name.c_str()) < 0 ? true : false;
    }
};




// helper function for sorting IFileInfoPtr objects

static void sortFolderInfo(xd::IFileInfoEnumPtr& f)
{
    if (f.isNull())
        return;

    std::vector<xd::IFileInfoPtr> vec;
    std::vector<xd::IFileInfoPtr>::iterator it;
    
    size_t i, count = f->size();
    
    for (i = 0; i < count; ++i)
        vec.push_back(f->getItem(i));
    
    std::sort(vec.begin(), vec.end(), FileInfoLess());
    
    f->clear();
    for (it = vec.begin(); it != vec.end(); ++it)
        f->append(*it);
}





// DbFolderFsItem class implementation

DbFolderFsItem::DbFolderFsItem()
{
    m_path = wxT("/");
    m_is_mount = false;
    m_only_folders = false;
    m_only_tables = false;
}

DbFolderFsItem::~DbFolderFsItem()
{
}

void DbFolderFsItem::setConnection(IConnectionPtr conn)
{
    m_conn = conn;
}

IConnectionPtr DbFolderFsItem::getConnection()
{
    return m_conn;
}

void DbFolderFsItem::setDatabase(xd::IDatabasePtr db)
{
    m_db = db;
}

xd::IDatabasePtr DbFolderFsItem::getDatabase()
{
    return m_db;
}

void DbFolderFsItem::setPath(const wxString& s)
{
    m_path = s;
}

void DbFolderFsItem::setType(int new_val)
{
    wxFAIL_MSG(wxT("Can't change a folder's type"));
}

int DbFolderFsItem::getType()
{
    return dbobjtypeFolder;
}

void DbFolderFsItem::setIsMount(bool b)
{
    m_is_mount = b;
}

bool DbFolderFsItem::getIsMount()
{
    return m_is_mount;
}
    
wxString DbFolderFsItem::getPath()
{
    return m_path;
}

void DbFolderFsItem::setOwner(IDbFolderFsItemPtr owner)
{
    m_owner = owner;
}

IDbFolderFsItemPtr DbFolderFsItem::getOwner()
{
    return m_owner;
}

void DbFolderFsItem::setFoldersOnly(bool new_val)
{
    m_only_folders = new_val;
}

void DbFolderFsItem::setTablesOnly(bool new_val)
{
    m_only_tables = new_val;
}

// this function sets a specific file info enumerator to use for this folder,
// which will be used in getChildren() as an override instead of actually
// trying to get the children of the folder item
void DbFolderFsItem::setChildrenOverride(xd::IFileInfoEnumPtr children_override)
{
    m_children_override = children_override;
}


#ifdef WIN32
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commoncontrols.h>
#endif

class ExtensionBitmapMgr
{
public:

    wxBitmap getBitmap(const wxString& ext)
    {
        std::map<wxString, wxBitmap>::iterator it;
        it = m_ext_bitmaps.find(ext);
        if (it != m_ext_bitmaps.end())
            return it->second;

        wxBitmap bmp;
        
        int id = wxTheFileIconsTable->GetIconID(ext);
        wxImageList* image_list = wxTheFileIconsTable->GetSmallImageList();
        bmp = image_list->GetBitmap(id);

        if (!bmp.IsOk())
        {
            bmp = GETBMPSMALL(gf_blank);
        }

        int x = bmp.GetWidth();
        int y = bmp.GetHeight();

        static int iconsize = g_app->getMainWindow()->FromDIP(100) > 100 ? 24 : 16;
        if (iconsize != x || iconsize != y)
        {
            wxClientDC dc(g_app->getMainWindow());
            wxImage img = bmp.ConvertToImage();
            img.Rescale(iconsize, iconsize);
            bmp = wxBitmap(img, dc);
        }

        m_ext_bitmaps[ext] = bmp;
        return bmp;
    }
    
private:

    std::map<wxString, wxBitmap> m_ext_bitmaps;
};

ExtensionBitmapMgr g_ext_bitmaps;



// DECIDE_BMP() simply chooses a shortcut version of the
// bitmap, if the file is a mount, or the regular bitmap if not
#define DECIDE_BMP(bmp) ((info.isOk() && info->isMount()) ? (getShortcutBitmap(GETBMPSMALL(bmp))) : (GETBMPSMALL(bmp)))

IFsItemEnumPtr DbFolderFsItem::getChildren()
{
    xcm::IVectorImpl<IFsItemPtr>* vec;
    vec = new xcm::IVectorImpl<IFsItemPtr>;

    if (m_conn)
    {
        if (!m_conn->isOpen())
        {
            m_conn->open();
        }

        m_db = m_conn->getDatabasePtr();
    }


    if (m_db.isNull())
    {
        return vec;
    }

    // if we're overriding the children enum, use that
    xd::IFileInfoEnumPtr files;
    if (m_children_override.isOk())
        files = m_children_override;
         else
        files = m_db->getFolderInfo(towstr(m_path));
    
    if (files.isNull())
    {
        return vec;
    }
    

    // regular tree mode is always alphabetical
    sortFolderInfo(files);

    size_t i, count = files->size();


    // get filters into vector form
    std::vector<wxString> exts;
    if (m_conn)
    {
        wxString filters = m_conn->getFilter();

        if (!filters.IsEmpty())
        {
            wxStringTokenizer t(filters, wxT(" ,;"));
            
            while (t.HasMoreTokens())
            {
                wxString token = t.GetNextToken();
                exts.push_back(token);
            }
        }
    }

    // now add other items
    for (i = 0; i < count; ++i)
    {
        xd::IFileInfoPtr info = files->getItem(i);

        wxString item_name = info->getName();
        int item_type = info->getType();
        
        if ((item_type != xd::filetypeTable && item_type != xd::filetypeFolder) && m_only_tables)
            continue;
        
        if (item_type != xd::filetypeFolder && m_only_folders)
            continue;

        // don't show hidden files (hidden files begin
        // with a period)
        if (*(item_name.c_str()) == wxT('.'))
            continue;



        // if there is an extension filter, make sure the
        // files shown are in that filter

        if (exts.size() > 0 && m_conn->getType() == dbtypeFilesystem)
        {
            bool ext_found = false;
            wxString ext = item_name.AfterLast('.');

            std::vector<wxString>::iterator it;
            for (it = exts.begin(); it != exts.end(); ++it)
            {
                if (!ext.CmpNoCase(*it))
                {
                    ext_found = true;
                    break;
                }
            }

            // if the file extension is not included in the filter,
            // do not add it to the list of items in the tree
            if (!ext_found)
                continue;
        }

        if (item_type == xd::filetypeFolder)
        {
            DbFolderFsItem* item = new DbFolderFsItem;
            item->setBitmap(DECIDE_BMP(gf_folder_closed), fsbmpSmall);
            item->setBitmap(DECIDE_BMP(gf_folder_open), fsbmpSmallExpanded);
            item->setBitmap(DECIDE_BMP(gf_folder_closed), fsbmpLarge);
            item->setLabel(item_name);
            item->setPath(appendPath(m_path, item_name));
            item->setDatabase(m_db);
            item->setConnection(m_conn);
            item->setFoldersOnly(m_only_folders);
            item->setTablesOnly(m_only_tables);
            item->setOwner(this);
            if (info->isMount())
            {
                item->setIsMount(true);
                item->setDeferred(true);
            }
            

            if (info->isMount())
            {
                std::wstring cstr, rpath;
                info->getMountInfo(cstr, rpath);

                if (kl::icontains(cstr, L"dbdoccloudfolder="))
                {
                    item->setBitmap(GETBMPSMALL(gf_globe), fsbmpSmall);
                    item->setBitmap(GETBMPSMALL(gf_globe), fsbmpSmallExpanded);
                }
            }
                
            vec->append(static_cast<IFsItem*>(item));
        }
         else if (item_type == xd::filetypeTable)
        {
            // it appears that all files that are in mounted folders
            // except scripts (which are of type xd::filetypeStream)
            // are of type xd::filetypeTable, so we need to determine
            // the db object type based on the file's extension

            wxBitmap bmp = DECIDE_BMP(gf_table);
                
            int dbobject_type = dbobjtypeSet;

            int period_idx = item_name.Find(wxT('.'), true);
            if (period_idx != -1)
            {
                wxString ext = item_name.AfterLast(wxT('.'));
                ext.MakeUpper();

                if (ext != wxT("EXE"))
                {
                    bmp = g_ext_bitmaps.getBitmap(ext);
                }
                
                if (ext == wxT("JS"))
                {
                    dbobject_type = dbobjtypeScript;
                }

                if (ext == wxT("HTM") ||
                    ext == wxT("HTML") ||
                    ext == wxT("XML"))
                {
                    dbobject_type = dbobjtypeBookmark;
                }
            }


            DbObjectFsItem* item = new DbObjectFsItem;
            item->setBitmap(bmp, fsbmpSmall);
            item->setBitmap(bmp, fsbmpLarge);
            item->setLabel(item_name);
            item->setPath(appendPath(m_path, item_name));
            item->setType(dbobject_type);
            item->setOwner(this);
            vec->append(static_cast<IFsItem*>(item));
        }
         else if (item_type == xd::filetypeNode)
        {
            // we have an old nodefile; open it as a stream and get what we need
            wxString path = appendPath(m_path, item_name);
            kl::JsonNode node;

            node = JsonConfig::loadFromDb(m_db, towstr(path));
            if (!node.isOk())
                continue;

            kl::JsonNode root_node = node["root"];
            if (!root_node.isOk())
                continue;


            bool script = false;
            bool report = false;
            bool query = false;


            kl::JsonNode script_node = root_node["kpp_script"];
            script = script_node.isOk();

            kl::JsonNode report_node = root_node["kpp_report"];
            report = report_node.isOk();

            kl::JsonNode template_node = root_node["kpp_template"];
            if (template_node.isOk())
            {
                kl::JsonNode type_node = template_node["type"];
                if (type_node.isOk() && type_node.getString() == L"query")
                    query = true;
            }

            DbObjectFsItem* item = new DbObjectFsItem;
            item->setLabel(item_name);
            item->setPath(path);
            item->setOwner(this);

            if (script)
            {
                item->setBitmap(DECIDE_BMP(gf_script), fsbmpSmall);
                item->setBitmap(DECIDE_BMP(gf_script), fsbmpLarge);
                item->setType(dbobjtypeScript);
            }
             else if (report)
            {
                item->setBitmap(DECIDE_BMP(gf_report), fsbmpSmall);
                item->setBitmap(DECIDE_BMP(gf_report), fsbmpLarge);
                item->setType(dbobjtypeReport);
            }
             else if (query)
            {
                item->setBitmap(DECIDE_BMP(gf_query), fsbmpSmall);
                item->setBitmap(DECIDE_BMP(gf_query), fsbmpLarge);
                item->setType(dbobjtypeTemplate);
            }
             else
            {
                item->setBitmap(DECIDE_BMP(gf_gear), fsbmpSmall);
                item->setBitmap(DECIDE_BMP(gf_gear), fsbmpLarge);
                item->setType(dbobjtypeTemplate);
            }

            vec->append(static_cast<IFsItem*>(item));
        }
         else if (item_type == xd::filetypeStream)
        {
            const std::wstring& mime_type = info->getMimeType();
            if (mime_type == L"application/vnd.kx.report")
            {
                DbObjectFsItem* item = new DbObjectFsItem;
                item->setLabel(item_name);
                item->setPath(appendPath(m_path, item_name));
                item->setOwner(this);
                item->setBitmap(DECIDE_BMP(gf_report), fsbmpSmall);
                item->setBitmap(DECIDE_BMP(gf_report), fsbmpLarge);
                item->setType(dbobjtypeReport);
                vec->append(static_cast<IFsItem*>(item));
            }
             else if (mime_type == L"application/vnd.kx.query")
            {
                DbObjectFsItem* item = new DbObjectFsItem;
                item->setLabel(item_name);
                item->setPath(appendPath(m_path, item_name));
                item->setOwner(this);
                item->setBitmap(DECIDE_BMP(gf_query), fsbmpSmall);
                item->setBitmap(DECIDE_BMP(gf_query), fsbmpLarge);
                item->setType(dbobjtypeTemplate);
                vec->append(static_cast<IFsItem*>(item));
            }
             else if (mime_type.substr(0, 19) == L"application/vnd.kx.")
            {
                DbObjectFsItem* item = new DbObjectFsItem;
                item->setLabel(item_name);
                item->setPath(appendPath(m_path, item_name));
                item->setOwner(this);
                item->setBitmap(DECIDE_BMP(gf_gear), fsbmpSmall);
                item->setBitmap(DECIDE_BMP(gf_gear), fsbmpLarge);
                item->setType(dbobjtypeTemplate);
                vec->append(static_cast<IFsItem*>(item));
            }
             else
            {
                DbObjectFsItem* item = new DbObjectFsItem;
                item->setLabel(item_name);
                item->setPath(appendPath(m_path, item_name));
                item->setOwner(this);
                item->setBitmap(DECIDE_BMP(gf_script), fsbmpSmall);
                item->setBitmap(DECIDE_BMP(gf_script), fsbmpLarge);
                item->setType(dbobjtypeScript);
                vec->append(static_cast<IFsItem*>(item));
            }
        }
    }
    
    return vec;
}



// DbObjectFsItem class implementation

DbObjectFsItem::DbObjectFsItem()
{
    m_path = wxT("");
    m_type = dbobjtypeSet;
    m_is_mount = false;
}

DbObjectFsItem::~DbObjectFsItem()
{

}

void DbObjectFsItem::setPath(const wxString& s)
{
    m_path = s;
}

wxString DbObjectFsItem::getPath()
{
    return m_path;
}


void DbObjectFsItem::setType(int new_val)
{
    m_type = new_val;
}

int DbObjectFsItem::getType()
{
    return m_type;
}

void DbObjectFsItem::setIsMount(bool b)
{
    m_is_mount = b;
}

bool DbObjectFsItem::getIsMount()
{
    return m_is_mount;
}

void DbObjectFsItem::setOwner(IDbFolderFsItemPtr owner)
{
    m_owner = owner;
}

IDbFolderFsItemPtr DbObjectFsItem::getOwner()
{
    return m_owner;
}



// -- DbDoc class implementation --

enum
{
    ID_ItemProperties = wxID_HIGHEST+1,
    ID_ExpandItem,
    ID_RemoveItem,
    ID_RenameItem,
    ID_RefreshItem,
    ID_Cut,
    ID_Copy,
    ID_Paste,    
    ID_Open,
    ID_OpenAsTable,
    ID_OpenAsText,
    ID_OpenAsWeb,
    ID_RunQuery,
    ID_RunScript,
    ID_RunReport,
    ID_PrintReport,
    ID_ModifyStructure,
    ID_NewShortcut,    
    ID_NewFolder,
    ID_NewTable,
    ID_NewReport,
    ID_NewQuery,
    ID_NewScript,
    ID_NewItemFinished
};


BEGIN_EVENT_TABLE(DbDoc, wxEvtHandler)
    EVT_MENU(ID_Project_ConnectExternal, DbDoc::onCreateExternalConnection)
    EVT_MENU(ID_Project_NewTable, DbDoc::onCreateTable)
    EVT_MENU(ID_Project_Import, DbDoc::onImportData)
    EVT_MENU(ID_Project_Export, DbDoc::onExportData)
    EVT_MENU(ID_App_OpenProject, DbDoc::onOpenProject)
    EVT_MENU(ID_Project_ProjectProperties, DbDoc::onProjectProperties)
    EVT_MENU(ID_ItemProperties, DbDoc::onItemProperties)
    EVT_MENU(ID_Project_RefreshProject, DbDoc::onRefreshProject)
    EVT_MENU(ID_RefreshItem, DbDoc::onRefreshItem)
    EVT_MENU(ID_ExpandItem, DbDoc::onExpandItem)
    EVT_MENU(ID_RemoveItem, DbDoc::onRemoveItem)
    EVT_MENU(ID_RenameItem, DbDoc::onRenameItem) 
    EVT_MENU(ID_Cut, DbDoc::onCut)
    EVT_MENU(ID_Copy, DbDoc::onCopy)
    EVT_MENU(ID_Paste, DbDoc::onPaste)    
    EVT_MENU(ID_Open, DbDoc::onOpen)
    EVT_MENU(ID_OpenAsTable, DbDoc::onOpenAsTable)
    EVT_MENU(ID_OpenAsText, DbDoc::onOpenAsText)
    EVT_MENU(ID_OpenAsWeb, DbDoc::onOpenAsWeb)
    EVT_MENU(ID_RunQuery, DbDoc::onRunQuery)
    EVT_MENU(ID_RunScript, DbDoc::onRunScript)
    EVT_MENU(ID_RunReport, DbDoc::onRunReport)
    EVT_MENU(ID_PrintReport, DbDoc::onPrintReport)
    EVT_MENU(ID_ModifyStructure, DbDoc::onModifyStructure)
    EVT_MENU(ID_NewShortcut, DbDoc::onNewShortcut)    
    EVT_MENU(ID_NewFolder, DbDoc::onNewFolder)
    EVT_MENU(ID_NewTable, DbDoc::onNewTable)
    EVT_MENU(ID_NewReport, DbDoc::onNewReport)
    EVT_MENU(ID_NewQuery, DbDoc::onNewQuery)
    EVT_MENU(ID_NewScript, DbDoc::onNewScript)
    EVT_MENU(ID_NewItemFinished, DbDoc::onNewItemFinished)               
    EVT_SET_FOCUS(DbDoc::onSetFocus)
    EVT_KILL_FOCUS(DbDoc::onKillFocus)
END_EVENT_TABLE()




DbDoc::DbDoc()
{
    m_edit_mode = editNone;
    m_no_edit = false;
    m_style = 0;
    m_ref_count = 1;
}


DbDoc::~DbDoc()
{
}

void DbDoc::ref()
{
    m_ref_count++;
}

void DbDoc::unref()
{
    if (--m_ref_count <= 0)
    {
        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);
    }
}


bool DbDoc::initAsWindow(wxWindow* window, 
                         wxWindowID id,
                         const wxPoint& position,
                         const wxSize& size,
                         int flags,
                         IFsItemPtr item)
{
    // set the root item
    IDbFolderFsItemPtr db_item = item;
    
    wxString caption = wxT("");
    
    if (db_item)
    {
        caption = db_item->getPath();
        m_root_item = item;
    }

    // create the FsPanel
    m_fspanel = createFsPanelObject();
    
    if (m_fspanel.isNull())
    {
        wxFAIL_MSG(wxT("Could not create FsPanel object"));
        return false;
    }
    
    m_fspanel->setStyle(m_style);
    if (!m_fspanel->create(window, id, position, size, flags))
    {
        wxFAIL_MSG(wxT("Could not create FsPanel window"));
        return false;
    }

    // connect FsPanel's signals
    m_fspanel->sigItemActivated().connect(this, &DbDoc::onFsItemActivated);
    m_fspanel->sigItemBeginLabelEdit().connect(this, &DbDoc::onFsItemBeginLabelEdit);
    m_fspanel->sigItemEndLabelEdit().connect(this, &DbDoc::onFsItemEndLabelEdit);
    m_fspanel->sigItemRightClicked().connect(this, &DbDoc::onFsItemRightClicked);
    m_fspanel->sigItemHighlightRequest().connect(this, &DbDoc::onFsItemHighlightRequest);
    m_fspanel->sigKeyDown().connect(this, &DbDoc::onKeyDown);
    m_fspanel->sigDragDrop().connect(this, &DbDoc::onDragDrop);

    m_fspanel->setRootItem(m_root_item);
    m_fspanel->setView(fsviewTree);
    m_fspanel->refresh();
    
    return true;
}


bool DbDoc::initAsDocument(IFsItemPtr item)
{
    // set the root item
    IDbFolderFsItemPtr db_item = item;
    
    wxString caption = wxEmptyString;
    
    if (db_item)
    {
        caption = db_item->getPath();
        m_root_item = item;
    }
    
    
    // create the FsPanel
    m_fspanel = createFsPanelObject();
    
    if (m_fspanel.isNull())
    {
        wxFAIL_MSG(wxT("Could not create FsPanel object"));
        return false;
    }
    
    int x = 0, y = 0, w = 220, h = 80, state = sitetypeDockable | dockLeft;
    m_fspanel->setStyle(m_style);
    m_dbdoc_site = g_app->getMainFrame()->createSite(m_fspanel, state, x, y, w, h);
    
    if (m_dbdoc_site.isNull())
    {
        wxFAIL_MSG(wxT("Could not create site for FsPanel"));
        return false;
    }
    
    m_dbdoc_site->setBitmap(GETBMPSMALL(gf_project));
    m_dbdoc_site->setMinSize(200, 400);
    m_dbdoc_site->setCaption(caption);
    m_dbdoc_site->setVisible(true);

    // connect FsPanel's signals
    m_fspanel->sigItemActivated().connect(this, &DbDoc::onFsItemActivated);
    m_fspanel->sigItemBeginLabelEdit().connect(this, &DbDoc::onFsItemBeginLabelEdit);
    m_fspanel->sigItemEndLabelEdit().connect(this, &DbDoc::onFsItemEndLabelEdit);
    m_fspanel->sigItemRightClicked().connect(this, &DbDoc::onFsItemRightClicked);
    m_fspanel->sigItemHighlightRequest().connect(this, &DbDoc::onFsItemHighlightRequest);
    m_fspanel->sigKeyDown().connect(this, &DbDoc::onKeyDown);
    m_fspanel->sigDragDrop().connect(this, &DbDoc::onDragDrop);

    m_fspanel->setRootItem(m_root_item);
    m_fspanel->setView(fsviewTree);
    m_fspanel->refresh();
    
    return true;
}


IDocumentSitePtr DbDoc::getDbDocSite()
{
    return m_dbdoc_site;
}

IFsPanelPtr DbDoc::getFsPanel()
{
    return m_fspanel;
}

wxWindow* DbDoc::getDocWindow()
{
    IDocumentPtr doc = m_fspanel;
    if (doc.isNull())
        return NULL;
    return doc->getDocumentWindow();
}

void DbDoc::setDatabase(xd::IDatabasePtr db, const wxString& root_path)
{
    if (db)
    {
        DbFolderFsItem* root = new DbFolderFsItem;
        root->setDatabase(db);
        if (root_path.Length() > 0)
            root->setPath(root_path);
        
        wxString db_label = getDatabaseNameProjectLabel();
        
        root->setLabel(db_label);
        root->setBitmap(GETBMPSMALL(gf_project));
        m_fspanel->setRootItem(root);
    }
     else
    {
        m_fspanel->setRootItem(xcm::null);
    }

    m_fspanel->refresh();
    g_cutcopy_items.clear();
}

// this function sets a specific root item for the DbDoc, insteaad of
// creating a root item based on the path as is the case in setDatabase()

void DbDoc::setRootItem(IDbFolderFsItemPtr root_folder)
{
    m_fspanel->setRootItem(root_folder);
    m_fspanel->refresh();
    g_cutcopy_items.clear();
}

bool DbDoc::isFsItemExternal(IFsItemPtr item)
{
    xd::IDatabasePtr db = getItemDatabase(item);
    if (db != g_app->getDatabase())
        return true;

    return false;
}


void DbDoc::refresh()
{
    if (!m_fspanel)
        return;
        
    // make sure the root label is updated
    IFsItemPtr root = m_fspanel->getRootItem();
    
    if (!root)
        return;
        
    wxString cur_label = root->getLabel();
    wxString new_label = getDatabaseNameProjectLabel();
    if (cur_label.CmpNoCase(new_label) != 0)
        root->setLabel(new_label);

    m_fspanel->refresh();
}


void DbDoc::actionActivate(IFsItemPtr item, int open_mask)
{
    AppBusyCursor bc;

    // if the item is not a Database Object Item, bail out
    IDbObjectFsItemPtr obj = item;
    if (!obj)
        return;

    if (obj->isFolder())
    {
        // don't activate a folder
        return;
    }

    AppController* app_cont = g_app->getAppController();
    wxString obj_path = obj->getPath();
    int obj_type = obj->getType();

    if (!app_cont->openAny(obj_path, open_mask))
    {
        appMessageBox(_("The file cannot be opened.  Please check to make sure that the file exists,\nthat you have the necessary permissions to access it, and it is not currently being modified."),
                       APPLICATION_NAME,
                       wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    
    g_macro << "";
    g_macro << "// open a file";
    g_macro << wxString::Format(wxT("HostApp.open('%s');"), wxcstr(obj_path));
}






class DbDocItemActivate : public wxEvtHandler
{
friend class DbDoc;

private:
    
    DbDocItemActivate(IFsItemPtr item, int open_mask)
    {
        m_item = item;
        m_open_mask = open_mask;
    }
        
    bool ProcessEvent(wxEvent& event)
    {
        DbDoc::actionActivate(m_item, m_open_mask);
        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);
        return true;
    }
    
private:

    IFsItemPtr m_item;
    int m_open_mask;
};


void DbDoc::actionActivateDeferred(IFsItemPtr item, int open_mask)
{
    DbDocItemActivate* act = new DbDocItemActivate(item, open_mask);
    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, 10000);
    ::wxPostEvent(act, e);
}


void DbDoc::getFsItemPaths(IFsItemEnumPtr source,
                           std::vector<wxString>& result,
                           bool expand_subfolders)
{
    std::vector<wxString>::iterator it;
    wxString path;
    
    size_t i, size = source->size();
    for (i = 0; i < size; ++i)
    {
        IFsItemPtr item = source->getItem(i);
        IDbFolderFsItemPtr db_folder_item = item;
        
        if (db_folder_item.isOk())
        {
            if (expand_subfolders && !DbDoc::isItemMount(item))
            {
                getFsItemPaths(item->getChildren(), result, expand_subfolders);
                continue;
            }
             else
            {
                path = db_folder_item->getPath();
            }
        }
         else
        {
            IDbObjectFsItemPtr db_object_item;
            db_object_item = item;

            if (!db_object_item)
                continue;

            path = db_object_item->getPath();
        }
        
        // make sure we don't have any duplicate entries in the vector
        bool found = false;
        for (it = result.begin(); it != result.end(); ++it)
        {
            if (path.CmpNoCase(*it) == 0)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
            result.push_back(path);
    }


    for (auto& path : result)
    {
        if (path.StartsWith('/') && path.Freq('/') == 1)
        {
            path.erase(0, 1);
        }
    }

}

wxString DbDoc::getFsItemPath(IFsItemPtr source)
{
    IDbFolderFsItemPtr db_folder_item = source;
    if (db_folder_item)
    {
        return db_folder_item->getPath();
    }

    IDbObjectFsItemPtr db_object_item = source;
    if (db_object_item)
    {
        return db_object_item->getPath();
    }

    return wxEmptyString;
}


bool DbDoc::isItemMount(IFsItemPtr _item)
{
    IDbObjectFsItemPtr item = _item;
    if (item.isNull())
        return false;
    return item->getIsMount();
}

bool DbDoc::isItemInMount(IFsItemPtr item)
{
    IDbObjectFsItemPtr dbitem = item;
    
    while (1)
    {
        if (dbitem.isNull())
            return false;
        
        IDbObjectFsItemPtr dbitem2 = dbitem->getOwner();
        if (dbitem2.isNull())
            return false;
            
        if (dbitem2->getIsMount())
            return true;
            
        dbitem = dbitem2;
    }
}

xd::IDatabasePtr DbDoc::getItemDatabase(IFsItemPtr _item)
{
    IDbObjectFsItemPtr item = _item;
    
    
    // specific to the tree -- this section won't work for the link bar
    // but it's necessary for the 'New Folder' to work
    if (item.isNull())
    {
        DbDoc* dbdoc = g_app->getDbDoc();
        if (!dbdoc)
            return xcm::null;
            
        IFsPanelPtr fspanel = dbdoc->getFsPanel();
        if (fspanel.isNull())
            return xcm::null;
            
        IFsItemPtr parent = fspanel->getItemParent(_item);
        if (!parent)
            return xcm::null;
        
        item = parent;
    }
 
    while (1)
    {
        if (item.isNull())
            return xcm::null;
        
        IDbFolderFsItemPtr folder = item;
        if (folder.isOk())
        {
            xd::IDatabasePtr db = folder->getDatabase();
            if (db.isOk())
                return db;
        }
        
        IDbObjectFsItemPtr parent = item->getOwner();
        item = parent;
    }
}

IFsItemPtr DbDoc::getFsItemFromPath(const wxString& path)
{
    wxString s = path;
    s.Trim();
    s.Trim(TRUE);

    return _findFsItem(m_fspanel->getRootItem(), s);
}


void DbDoc::onKeyDown(const wxKeyEvent& evt, bool* handled)
{
    if (evt.GetKeyCode() == WXK_RETURN)
    {
        openSelectedItems();
        toggleExpandOnSelectedItems();
        *handled = true;
    }
     else if (evt.GetKeyCode() == WXK_DELETE)
    {
        wxCommandEvent empty;
        onRemoveItem(empty);
        *handled = true;
    }
     else
    {
        *handled = false;
    }
}

void DbDoc::onCreateExternalConnection(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;

    IFsItemPtr item = items->getItem(0);

    IDbFolderFsItemPtr folder = item;
    if (!folder)
        return;

    g_app->getAppController()->showCreateExternalConnectionWizard();
}

void DbDoc::onCreateTable(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;

    IFsItemPtr item = items->getItem(0);

    IDbFolderFsItemPtr folder = item;
    if (!folder)
        return;

    g_app->getAppController()->showCreateTable();
}

void DbDoc::onImportData(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;

    IFsItemPtr item = items->getItem(0);

    IDbFolderFsItemPtr folder = item;
    if (!folder)
        return;

    ImportInfo info;
    info.base_path = getFsItemPath(item);

    g_app->getAppController()->showImportWizard(info);
}

void DbDoc::onExportData(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();

    int i;
    int count = items->size();

    ExportInfo info;
    ExportTableSelection ts;

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        wxString path = getFsItemPath(item);

        ts.input_tablename = path;
        ts.output_tablename = path.AfterLast(wxT('/'));
        info.tables.push_back(ts);
    }

    g_app->getAppController()->showExportWizard(info);
}

void DbDoc::onOpenProject(wxCommandEvent& evt)
{
    g_app->getAppController()->showProjectManager();
}

void DbDoc::onProjectProperties(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;

    IFsItemPtr item = items->getItem(0);

    IDbFolderFsItemPtr folder = item;
    if (!folder)
        return;

    g_app->getAppController()->showProjectProperties();
}

void DbDoc::onItemProperties(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    size_t i,count = items->size();

    if (count == 1)
    {
        IDbFolderFsItemPtr folder = items->getItem(0);
        if (folder)
        {
            std::wstring path = towstr(folder->getPath());
            
            xd::IDatabasePtr db = getItemDatabase(folder);
            if (db.isNull())
                return;
                
            xd::IFileInfoPtr file_info = db->getFileInfo(path);
            
            if (file_info.isOk() && file_info->isMount())
            {
                // get connection string
                std::wstring cstr, rpath;
                if (!db->getMountPoint(path, cstr, rpath))
                    return;

                IDocumentSitePtr site;

                m_edit_item = folder;

                /*
                DlgConnection* dlg = new DlgConnection(g_app->getMainWindow(), wxID_ANY, _("Connect To Database"), DlgConnection::optionFolder);
                dlg->getConnectionInfo().fromConnectionString(cstr);
                //dlg->sigFinished.connect(&onConnectExternalDatabaseWizardFinished);
                dlg->Show();
                */

                ConnectionWizard* wizard = new ConnectionWizard;
                wizard->setTitle(_("Connection Properties"));
                wizard->setMode(ConnectionWizard::ModeProperties);
                wizard->setConnectionString(cstr);
                wizard->sigConnectionWizardFinished.connect(this, &DbDoc::onSetConnectionPropertiesFinished);

                site = g_app->getMainFrame()->createSite(wizard, sitetypeModeless,
                    -1, -1, 540, 480);
                site->setMinSize(540, 480);
                site->setName(wxT("ConnectionPropertiesPanel"));
                return;
            }
        }
    }


    AppBusyCursor bc;

    MultiFileInfoPanel* panel = new MultiFileInfoPanel;

    for (i = 0; i < count; ++i)
        panel->addFile(getFsItemPath(items->getItem(i)));

    IDocumentSitePtr site;
    site = g_app->getMainFrame()->createSite(panel, sitetypeModeless,
                                             -1, -1, 600, 520);
    site->setMinSize(600,320);
    site->setName(wxT("FilePropertiesPanel"));
}

void DbDoc::onRefreshProject(wxCommandEvent& evt)
{
    g_app->getAppController()->refreshDbDoc();
}

void DbDoc::onRefreshItem(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;
    IFsItemPtr item = items->getItem(0);

    m_fspanel->refreshItem(item);
}

void DbDoc::onExpandItem(wxCommandEvent& evt)
{
    toggleExpandOnSelectedItems();
}

void DbDoc::onRemoveItem(wxCommandEvent& evt)
{
    bool external_database_item = false;
    bool external = false;
    bool all_mounts = true;
    wxString message;
    int i;

    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    int count = items->size();
    if (count < 1)
        return;

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        
        if (!isItemMount(item))
            all_mounts = false;
            
        external = isFsItemExternal(item);
        if (!external)
        {
            if (getFsItemPath(item) == wxT("/"))
                return;
        }
         else
        {
            IDbFolderFsItemPtr db_folder_item = item;
            if (db_folder_item)
            {
                external_database_item = true;
            }
        }
    }

    if (all_mounts)
    {
        message += _("You are about to remove the following connection(s).\nAre you sure you want to continue?\n\n");
    }
     else
    {
        message += _("You are about to delete the following item(s).\nAre you sure you want to continue?\n\n");
    }
    
    int mcount = 0;

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);

        wxString path = getFsItemPath(item);
        path = path.AfterLast(L'/');
        
        if (mcount < 10)
        {
            message += wxT("     ");

            message += path;
            message += wxT("\n");
        }
         else if (mcount == 10)
        {   
            message += wxT("     ");
            message += _("(More files...)");
        }

        mcount++;
    }


    if (mcount > 0 && !external_database_item)
    {
        int result = appMessageBox(message,
                                        APPLICATION_NAME,
                                        wxYES_NO | wxICON_EXCLAMATION | wxCENTER);
        if (result != wxYES)
            return;
    }


    // if the user is trying to remove an external connection,
    // let's ask them if they really want to do this just to make sure
    
    if (external_database_item)
    {
        IFsItemPtr item = items->getItem(0);
        
        message = _("You are about to remove the following external connection.\nAre you sure?\n\n");
        message += wxT("     ");
        message += item->getLabel();

        int result = appMessageBox(message,
                                        APPLICATION_NAME,
                                        wxYES_NO | wxICON_EXCLAMATION | wxCENTER);
        if (result != wxYES)
            return;
    }


    std::vector<wxString> problem_items;


    {
        AppBusyCursor bc;

        for (i = 0; i < count; ++i)
        {
            deleteFsItem(m_fspanel, items->getItem(i), problem_items);
        }
    }


    if (problem_items.size() > 0)
    {
        if (!external)
        {
            message = _("One or more of the following items could not be deleted.  They may be in use.\n\n");
        }
         else
        {
            message = _("One or more of the following items could not be deleted.  They may be in use or read-only,\nor you may not have user privileges to delete them.\n\n");
        }

        for (std::vector<wxString>::iterator it = problem_items.begin();
             it != problem_items.end(); ++it)
        {        
            message += wxT("     ");
            message += *it;
            message += wxT("\n");
        }

        appMessageBox(message,
                           APPLICATION_NAME,
                           wxOK| wxICON_EXCLAMATION | wxCENTER);
    }
    
    
    // refocus the project tree; note: deleting a bookmarked webpage from a
    // folder in the link bar folder was causing a crash since doc was null; 
    // check to see if doc is null before deleting to prevent the crash
    IDocumentPtr doc = m_fspanel;
    
    if (doc.isOk())
        doc->setDocumentFocus();
}

void DbDoc::onRenameItem(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;
    IFsItemPtr item = items->getItem(0);

    m_edit_mode = editRename;
    m_fspanel->editLabel(item);
}

void DbDoc::onOpen(wxCommandEvent& evt)
{
    openSelectedItems();
}

void DbDoc::onOpenAsTable(wxCommandEvent& evt)
{
    openSelectedItems(appOpenDefault | appOpenAsTable);
}

void DbDoc::onOpenAsText(wxCommandEvent& evt)
{
    openSelectedItems(appOpenDefault | appOpenAsText);
}

void DbDoc::onOpenAsWeb(wxCommandEvent& evt)
{
    openSelectedItems(appOpenDefault | appOpenAsWeb);
}

void DbDoc::onRunQuery(wxCommandEvent& event)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();

    int i, count;
    count = items->size();

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);

        IDbObjectFsItemPtr obj;
        obj = item;

        if (obj)
        {
            g_app->getAppController()->execute(obj->getPath());
        }
    }
}

void DbDoc::onRunScript(wxCommandEvent& event)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;
    IFsItemPtr item = items->getItem(0);
    wxString item_path = getFsItemPath(item);
    
    IDocumentSiteEnumPtr docsites;
    IDocumentSitePtr site;
    IEditorDocPtr editor_doc;
    
    // if we're running a script from the project panel, make sure
    // all open scripts (except untitled scripts) are saved first
    docsites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);
    int site_count = docsites->size();
    for (int i = 0; i < site_count; ++i)
    {
        site = docsites->getItem(i);
        editor_doc = site->getDocument();
        if (editor_doc.isOk() && !editor_doc->isTemporary())
            editor_doc->doSave();
    }
    
    // now we can run the script
    AppScriptError error;
    
    if (g_app->getAppController()->executeScript(item_path, NULL, &error).isNull())
    {
        // open the script that has an error in it
        
        wxString script_path;
        if (!error.file.IsEmpty())
            script_path = error.file;
             else
            script_path = item_path;
        
        int doc_id;
        if (!g_app->getAppController()->openScript(script_path, &doc_id))
            return;
        
        IDocumentSitePtr site = g_app->getMainFrame()->lookupSiteById(doc_id);
        if (site.isNull())
            return;
        
        IEditorDocPtr doc_ptr = site->getDocument();
        if (doc_ptr.isNull())
            return;
        
        // show the error in the statusbar
        if (doc_ptr.isOk())
            doc_ptr->reportError(error.offset, error.line, error.message);
        
        wxString error_str = wxString::Format(_("Compiler error (line %d): %s"),
                                              error.line,
                                              error.message.c_str());
        
        wxString message = wxString::Format(_("The selected script could not be run because there are errors in it:\n\n%s"),
                                            error_str.c_str());
        
        // alert the user that the script couldn't be run
        appMessageBox(message,
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
    }
}

void DbDoc::onRunReport(wxCommandEvent& evt)
{
    openSelectedItems(appOpenDefault | appOpenInLayout);
}

void DbDoc::onPrintReport(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();

    int i, count;
    count = items->size();

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);

        IDbObjectFsItemPtr obj;
        obj = item;

        if (obj)
        {
            g_app->getAppController()->print(obj->getPath());
        }
    }
}

void DbDoc::onModifyStructure(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;
    IFsItemPtr item = items->getItem(0);
    
    IDbObjectFsItemPtr obj = item;
    if (!obj)
        return;

    std::wstring path = towstr(obj->getPath());

    StructureDoc* doc = new StructureDoc;
    doc->setModifySet(path);
    g_app->getMainFrame()->createSite(static_cast<IDocument*>(doc),
                                      sitetypeNormal,
                                      -1, -1, -1, -1);
}


void DbDoc::doCut()
{
    g_cutcopy_items.clear();
    g_cutcopy_action = actionCut;
    
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    
    size_t i, count = items->size();

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        g_cutcopy_items.push_back(item);
        m_fspanel->setItemBitmap(item, lightenBitmap(item->getBitmap()));
    }
}

void DbDoc::doCopy()
{
    g_cutcopy_items.clear();
    g_cutcopy_action = actionCopy;

    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    int count = items->size();
    int i;

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        g_cutcopy_items.push_back(item);
    }
}

void DbDoc::doPaste()
{
    xd::IDatabasePtr db = g_app->getDatabase();


    if (g_cutcopy_items.size() == 0)
        return;

    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;

    IFsItemPtr target = items->getItem(0);
    
    while (target)
    {
        IDbFolderFsItemPtr f = target;
        if (f.isOk())
            break;
        target = m_fspanel->getItemParent(target);
    }
    
    if (target.isNull())
        return; // can't find parent folder item
    
    
    wxString target_location = getFsItemPath(target);
    
    
    xd::IDatabasePtr target_database = getItemDatabase(target);

    std::vector<IFsItemPtr>::iterator it;

    if (g_cutcopy_action == actionCut)
    {
        // don't allow cut/paste of same items in the same location
        for (it = g_cutcopy_items.begin(); it != g_cutcopy_items.end(); ++it)
        {
            if (target == *it)
            {
                appMessageBox(_("One or more source and/or destination files are the same."),
                              APPLICATION_NAME,
                              wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }
        }
    


        for (it = g_cutcopy_items.begin(); it != g_cutcopy_items.end(); ++it)
        {
            wxString src_path = getFsItemPath(*it);
            wxString fname = src_path.AfterLast(wxT('/'));
            wxString target_path;
    
            int counter = 0;
            do
            {
                target_path = target_location;
                if (target_path.IsEmpty() || target_path.Last() != wxT('/'))
                {
                    target_path += wxT("/");
                }

                target_path += fname;
                if (counter > 0)
                {
                    target_path += wxString::Format(wxT("_%d"), counter+1);
                }

                counter++;

            } while (db->getFileExist(towstr(target_path)));


            if (db->moveFile(towstr(src_path), towstr(target_path)))
            {
                m_fspanel->remove(*it);

                // post a command that lets everyone know that
                // an ofs item has been moved

                FrameworkEvent* cfw_event = new FrameworkEvent(FRAMEWORK_EVT_TREEPANEL_OFS_FILE_RENAMED);
                cfw_event->s_param = src_path;
                cfw_event->s_param2 = target_path;
                g_app->getMainFrame()->postEvent(cfw_event);
            }
        }

        m_fspanel->refreshItem(target);

        IDbFolderFsItemPtr folder = getFsItemFromPath(target_location);
        if (folder)
        {
            m_fspanel->expand(folder);
        }

        g_cutcopy_items.clear();
    }
     else if (g_cutcopy_action == actionCopy)
    {
        std::vector<jobs::IJobPtr> jobs;
        for (it = g_cutcopy_items.begin(); it != g_cutcopy_items.end(); ++it)
        {
            std::wstring input = towstr(getFsItemPath(*it));
            std::wstring fname = stripExtension(kl::afterLast(input, '/'));
            std::wstring output;

            int counter = 0;
            do
            {
                output = towstr(target_location);
                if (output.empty() || output[output.length()-1] != '/')
                    output += L"/";

                output += fname;
                if (counter > 0)
                {
                    output += kl::stdswprintf(L"_%d", counter+1);
                }

                // if we're saving the file to a filesystem mount and no
                // extension is specified, then automatically add a 'csv'
                // or 'js' extension; this is a usability issue since without 
                // the extension, the user usually ends up adding this as the 
                // first item of business after saving
                IDbObjectFsItemPtr dbobject_item = *it;
                if (dbobject_item.isOk())
                {
                    if (dbobject_item->getType() == dbobjtypeSet)
                        output = addExtensionIfExternalFsDatabase(output, L".csv");
                    if (dbobject_item->getType() == dbobjtypeScript)
                        output = addExtensionIfExternalFsDatabase(output, L".js");
                }

                counter++;
            } while (db->getFileExist(output));


            jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.copy-job");

            kl::JsonNode params;
            params["input"] = input;
            params["output"] = output;

            job->setParameters(params.toString());
            jobs.push_back(job);
        }



        if (g_cutcopy_items.size() > jobs.size())
        {
            m_fspanel->refreshItem(target);
        }

        if (jobs.size() > 0)
        {
            jobs::IJobPtr aggregate_job = jobs::createAggregateJob(jobs);
            aggregate_job->getJobInfo()->setTitle(towstr(_("Copying Records")));
            aggregate_job->setDatabase(g_app->getDatabase());
            aggregate_job->setExtraValue(L"refresh-folder", towstr(target_location));
            aggregate_job->sigJobFinished().connect(this, &DbDoc::onCopyJobFinished);
            g_app->getJobQueue()->addJob(aggregate_job, jobs::jobStateRunning);
        }
    }
}

void DbDoc::onCut(wxCommandEvent& evt)
{
    doCut();
}

void DbDoc::onCopy(wxCommandEvent& evt)
{
    doCopy();
}

void DbDoc::onPaste(wxCommandEvent& evt)
{
    doPaste();
}

void DbDoc::onNewShortcut(wxCommandEvent& evt)
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;

    IFsItemPtr parent_item = items->getItem(0);

    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeOpen);
    dlg.setCaption(_("Select Item Location"));
    dlg.setAffirmativeButtonLabel(_("OK"));
    if (dlg.ShowModal() != wxID_OK)
        return;


    xd::IDatabasePtr db = getItemDatabase(parent_item);
    wxString path = getFsItemPath(parent_item);
    path += wxT("/New_Shortcut");


    db->setMountPoint(towstr(path), L"", towstr(dlg.getPath()));
    
    m_fspanel->refreshItem(parent_item);

    IFsItemPtr target = getFsItemFromPath(path);
    if (target.isOk())
    {
        m_fspanel->unselectAll();
        m_fspanel->selectItem(target);
    }
    
}

IFsItemPtr DbDoc::getNewFileParent()
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return xcm::null;
    
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return xcm::null;

    return items->getItem(0);
}
    
wxString DbDoc::getDefaultNewFileName(IFsItemPtr parent, const wxString& name)
{
    wxASSERT(parent.isOk());
    
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return wxEmptyString;

    wxString path = getFsItemPath(parent);
    
    if (path.IsEmpty() || path.Last() != '/')
        path += wxT("/");
        
    wxString result;
    
    int i = 1;
    do
    {
        result = path;
        result += name;
        
        if (i > 1)
            result += wxString::Format(wxT(" %d"), i);
        ++i;
    } while (db->getFileExist(towstr(result)));

    return result;
}



void DbDoc::onNewFolder(wxCommandEvent& evt)
{
/*
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();
    if (items->size() != 1)
        return;

    m_newitem_parent = items->getItem(0);
    
    // find the last folder and put our temporary
    // edit item directly under there

    IFsItemPtr previous_item;
    IFsItemEnumPtr all_items;
    
    all_items = m_fspanel->getItemChildren(m_newitem_parent);

    int count = all_items->size();
    int i;

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = all_items->getItem(i);
        IDbFolderFsItemPtr folder = item;
        if (!folder)
            break;

        previous_item = item;
    }

    IFsItemPtr new_item;
    new_item = m_fspanel->insertItem(m_newitem_parent,
                                     previous_item,
                                     wxT(""),
                                     GETBMPSMALL(gf_folder_open));
    m_edit_mode = editNewFolder;

    m_fspanel->expand(m_newitem_parent);
    m_fspanel->editLabel(new_item);
*/

    IFsItemPtr parent = getNewFileParent();
    if (parent.isNull())
        return;
        
    wxString path = getDefaultNewFileName(parent, _("New Folder"));
    
    xd::IDatabasePtr db = g_app->getDatabase();
    if (!db->createFolder(towstr(path)))
    {
        appMessageBox(_("A folder could not be created in this location."),
                           APPLICATION_NAME,
                           wxOK | wxICON_ERROR | wxCENTER);
        return;
    }
    
    m_fspanel->refreshItem(parent);
    
    if (!m_fspanel->isItemExpanded(parent))
        m_fspanel->expand(parent);
    
    IFsItemPtr new_item = getFsItemFromPath(path);
    if (new_item)
    {
        m_fspanel->expand(new_item);
        m_edit_mode = editRename;
        m_fspanel->editLabel(new_item);
    }
    
}

void DbDoc::onNewTable(wxCommandEvent& evt)
{
    IFsItemPtr parent = getNewFileParent();
    if (parent.isNull())
        return;
        
    wxString path = getDefaultNewFileName(parent, _("New Table"));
    
    if (!TableDocMgr::newFile(towstr(path)))
    {
        appMessageBox(_("A file could not be created in this folder."),
                           APPLICATION_NAME,
                           wxOK | wxICON_ERROR | wxCENTER);
        return;
    }
    
    m_fspanel->refreshItem(parent);
    
    if (!m_fspanel->isItemExpanded(parent))
        m_fspanel->expand(parent);
    
    IFsItemPtr new_item = getFsItemFromPath(path);
    if (new_item)
    {
        m_edit_mode = editRename;
        m_fspanel->editLabel(new_item);
    }
}

void DbDoc::onNewReport(wxCommandEvent& evt)
{
    IFsItemPtr parent = getNewFileParent();
    if (parent.isNull())
        return;
        
    wxString path = getDefaultNewFileName(parent, _("New Report"));
    if (!ReportDoc::newFile(path))
    {
        appMessageBox(_("A file could not be created in this folder."),
                           APPLICATION_NAME,
                           wxOK | wxICON_ERROR | wxCENTER);
        return;
    }
    
    m_fspanel->refreshItem(parent);
    
    if (!m_fspanel->isItemExpanded(parent))
        m_fspanel->expand(parent);
    
    IFsItemPtr new_item = getFsItemFromPath(path);
    if (new_item)
    {
        m_edit_mode = editRename;
        m_fspanel->editLabel(new_item);
    }
}

void DbDoc::onNewQuery(wxCommandEvent& evt)
{
    IFsItemPtr parent = getNewFileParent();
    if (parent.isNull())
        return;
        
    wxString path = getDefaultNewFileName(parent, _("New Query"));
    if (!QueryDoc::newFile(path))
    {
        appMessageBox(_("A file could not be created in this folder."),
                           APPLICATION_NAME,
                           wxOK | wxICON_ERROR | wxCENTER);
        return;
    }
    
    m_fspanel->refreshItem(parent);
    
    if (!m_fspanel->isItemExpanded(parent))
        m_fspanel->expand(parent);
    
    IFsItemPtr new_item = getFsItemFromPath(path);
    if (new_item)
    {
        m_edit_mode = editRename;
        m_fspanel->editLabel(new_item);
    }
}

void DbDoc::onNewScript(wxCommandEvent& evt)
{
    IFsItemPtr parent = getNewFileParent();
    if (parent.isNull())
        return;
        
    wxString path = getDefaultNewFileName(parent, _("New Script"));
    if (!EditorDoc::newFile(path))
    {
        deferredAppMessageBox(_("A file could not be created in this folder."),
                           APPLICATION_NAME,
                           wxOK | wxICON_ERROR | wxCENTER);
        return;
    }
    
    m_fspanel->refreshItem(parent);
    
    if (!m_fspanel->isItemExpanded(parent))
        m_fspanel->expand(parent);
    
    IFsItemPtr new_item = getFsItemFromPath(path);
    if (new_item)
    {
        m_edit_mode = editRename;
        m_fspanel->editLabel(new_item);
    }
}

void DbDoc::onNewItemFinished(wxCommandEvent& evt)
{
    wxString label = evt.GetString();
    
    m_fspanel->refreshItem(m_newitem_parent);
    updateCutCopyItems();
    
    // now select the new folder
    IFsItemEnumPtr children = m_fspanel->getItemChildren(m_newitem_parent);
    int child_count = children->size();
    int i;
    
    for (i = 0; i < child_count; ++i)
    {
        IFsItemPtr child = children->getItem(i);
        IDbFolderFsItemPtr folder = child;
        if (folder.isNull())
            continue;

        if (child->getLabel().CmpNoCase(label) == 0)
        {
            m_fspanel->unselectAll();
            m_fspanel->selectItem(child);
            m_fspanel->expand(child);
            break;
        }
    }
    
    m_newitem_parent.clear();
}

void DbDoc::onSetFocus(wxFocusEvent& evt)
{
    wxWindow* win = getDocWindow();
    if (win == NULL)
        return;

    win->SetFocus();
}

void DbDoc::onKillFocus(wxFocusEvent& evt)
{
}

void DbDoc::openSelectedItems(int open_mask)
{
    // if no mask is specified, open the selected
    // item with the default
    if (open_mask == -1)
        open_mask = appOpenDefault;

    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();

    int i, count;
    count = items->size();

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);

        IDbObjectFsItemPtr obj;
        obj = item;

        if (obj)
        {
            actionActivateDeferred(item, open_mask);
        }
    }
}

void DbDoc::toggleExpandOnSelectedItems()
{
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();

    size_t i, count = items->size();
    if (count < 1)
        return;

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        if (m_fspanel->isItemExpanded(item))
            m_fspanel->collapse(item);
             else
            m_fspanel->expand(item);
    }
}

void DbDoc::updateCutCopyItems()
{
    std::vector<IFsItemPtr> new_cutcopy_items;

    std::vector<IFsItemPtr>::iterator it;
    for (it = g_cutcopy_items.begin(); it != g_cutcopy_items.end(); ++it)
    {
        wxString path = getFsItemPath(*it);
        IFsItemPtr new_item = getFsItemFromPath(path);
        if (new_item)
        {
            if (new_item != *it)
            {
                m_fspanel->setItemBitmap(new_item, lightenBitmap(new_item->getBitmap()));
            }

            new_cutcopy_items.push_back(new_item);
        }
    }

    g_cutcopy_items.clear();
    g_cutcopy_items = new_cutcopy_items;
}



// This function deletes a folder in the tree as well
// as recursively deletes its contents

void DbDoc::deleteFsItem(IFsPanelPtr tree,
                         IFsItemPtr item,
                         std::vector<wxString>& problem_items)
{    
    IDbFolderFsItemPtr folder = item;
    IDbObjectFsItemPtr obj = item;



    xd::IDatabasePtr db = getItemDatabase(item);

    if (db.isNull())
    {
        problem_items.push_back(getFsItemPath(item));
        return;
    }

    if (folder)
    {
        // first check if we're deleting a mount
        {
            xd::IDatabasePtr db = g_app->getDatabase();
            xd::IFileInfoPtr file_info = db->getFileInfo(towstr(folder->getPath()));
            
            if (file_info.isOk() && file_info->isMount())
            {
                if (!db->deleteFile(towstr(folder->getPath())))
                {
                    problem_items.push_back(folder->getPath());
                }
                 else
                {
                    if (tree)
                        tree->remove(item);
                }
                
                return;
            }
        }

    
    
        size_t problem_item_count = problem_items.size();

        IFsItemEnumPtr child_items = tree->getItemChildren(item);
        
        size_t i, child_count = child_items->size();

        for (i = 0; i < child_count; ++i)
        {
            deleteFsItem(tree, child_items->getItem(i), problem_items);
        }

        // if there were no further problems deleting child
        // sets, then proceed to delete the folder

        if (problem_items.size() == problem_item_count)
        {
            if (!db->deleteFile(towstr(folder->getPath())))
            {
                problem_items.push_back(folder->getPath());
            }
             else
            {
                if (tree)
                    tree->remove(item);
            }
        }
    }
     else if (obj)
    {

        int obj_type = obj->getType();
        if (obj_type == dbobjtypeSet && db == g_app->getDatabase())
        {
            // local set deletion: we must delete
            // the tabledoc model as well

            bool problem = false;

            xd::IFileInfoPtr info = db->getFileInfo(towstr(obj->getPath()));

            if (info.isOk())
            {
                std::wstring set_id = info->getObjectId();

                if (db->deleteFile(towstr(obj->getPath())))
                {
                    // delete the tabledoc model
                    TableDocMgr::deleteModel(set_id);
                }
                 else
                {
                    problem = true;
                }
            }
             else
            {
                if (!db->deleteFile(towstr(obj->getPath())))
                {
                    problem = true;
                }
            }

            if (problem)
            {
                problem_items.push_back(obj->getPath());
            }
             else
            {
                if (tree)
                    tree->remove(item);
            }
        }
         else
        {
            if (!db->deleteFile(towstr(obj->getPath())))
            {
                problem_items.push_back(obj->getPath());
            }
             else
            {
                if (tree)
                    tree->remove(item);
            }
        }
    }
}

IFsItemPtr DbDoc::_findFsItem(IFsItemPtr item,
                                   const wxString& _path)
{
    wxString path = _path;
    
    if (getFsItemPath(item) == path)
        return item;
    
    // if there's a slash at the beginning, remove it
    while (path.Length() > 0 && path.GetChar(0) == L'/')
        path.Remove(0, 1);
            
    // get next chunk
    wxString piece = path.BeforeFirst(L'/');
    
    bool last_piece = false;
    if (piece == path)
    {
        last_piece = true;
    }
    
    IFsItemEnumPtr items = m_fspanel->getItemChildren(item);
    if (items.isNull())
        return xcm::null;

    int i, count = items->size();
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr t = items->getItem(i);

        wxString label = t->getLabel();
        if (0 == label.CmpNoCase(piece))
        {
            if (last_piece)
            {
                return t;
            }
             else
            {
                return _findFsItem(t, path.AfterFirst(L'/'));
            }
        }
    }
    
    return xcm::null;
}

void DbDoc::onFsItemActivated(IFsItemPtr item)
{
    int open_mask = appOpenDefault;

    IDbObjectFsItemPtr obj = item;
    if (obj.isOk() && obj->getType() == dbobjtypeSet)
        open_mask = appOpenAsTable;

    actionActivateDeferred(item, open_mask);
}

void DbDoc::onFsItemBeginLabelEdit(IFsItemPtr item,
                                   bool* allow)
{
    if (m_no_edit)
    {
        *allow = false;
        return;
    }
    
    IDbFolderFsItemPtr folder = item;

    if (isFsItemExternal(item))
    {
        // user may change the description of the
        // connection, but not any of the tables inside the
        // connection (for now)

        if (folder.isOk() && folder->getPath() == wxT("/"))
        {
            *allow = true;
            return;
        }
         else
        {
            *allow = false;
            return;
        }
    }

    // do not allow renaming of the root item of the project database
    if (folder.isOk() && folder->getPath() == wxT("/"))
    {
        *allow = false;
        return;
    }
    
    if (m_edit_mode == editNone)
    {
        m_edit_mode = editRename;
    }
}

void DbDoc::onFsItemEndLabelEdit(IFsItemPtr item,
                                 wxString text,
                                 bool cancelled,
                                 bool* allow)
{
    if (cancelled)
    {
        if (m_edit_mode == editNewFolder)
        {
            m_edit_mode = editNone;
            if (m_fspanel)
                m_fspanel->remove(item);
        }

        *allow = false;

        return;
    }
    

    // user may be renaming a connection; check this case first

    if (isFsItemExternal(item))
    {
        IDbFolderFsItemPtr folder = item;
        if (folder.isNull())
        {
            *allow = false;
            return;
        }

        IConnectionPtr conn = folder->getConnection();
        if (conn.isNull())
        {
            *allow = false;
            return;
        }

        conn->setDescription(towstr(text));

        *allow = true;
        return;
    }


    wxString label = text;

    if (!isValidObjectName(label))
    {
        if (m_edit_mode == editNewFolder)
        {
            if (m_fspanel)
                m_fspanel->remove(item);
        }

        *allow = false;

        appInvalidObjectMessageBox();
        return;
    }

    if (m_edit_mode == editNewFolder)
    {
        if (text.Length() == 0)
        {
            if (m_fspanel)
                m_fspanel->remove(item);
            *allow = false;
            return;
        }
                        
        // create new folder
        IDbFolderFsItemPtr folder = m_newitem_parent;
        
        if (folder.isNull())
        {
            if (m_fspanel)
                m_fspanel->remove(item);
            *allow = false;
            return;
        }

        wxString filename;
        filename = folder->getPath();
        if (filename.IsEmpty() || filename.Last() != wxT('/'))
            filename += wxT("/");
        filename += label;

        xd::IDatabasePtr db = g_app->getDatabase();

        // find out if a file with the new name already exists
        if (db->getFileExist(towstr(filename)))
        {
            if (m_fspanel)
                m_fspanel->remove(item);
            *allow = false;
            
            wxString message = wxString::Format(_("An object with the name '%s' already exists.  Please choose a different name."),
                                                label.c_str());
            deferredAppMessageBox(message,
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        bool result = db->createFolder(towstr(filename));
        if (!result)
        {               
            // failed
            if (m_fspanel)
                m_fspanel->remove(item);
            *allow = false;
            return;
        }

        // the rest of this processing must be executed
        // after this event is done
        
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, ID_NewItemFinished);
        e.SetString(label);
        ::wxPostEvent(this, e);
    }
     else if (m_edit_mode == editRename)
    {
        xd::IDatabasePtr db = g_app->getDatabase();
        wxString old_path;

        // check if edit was cancelled or was empty
        if (label.Length() == 0 || cancelled)
        {
            *allow = false;
            return;
        }

        old_path = getFsItemPath(item);
        if (old_path.IsEmpty())
        {
            *allow = false;
            return;
        }

        // find out if a file with the new name already exists
        wxString new_path = old_path;
        int slash_pos = new_path.Find(wxT('/'), TRUE);
        if (slash_pos == -1)
        {
            return;
        }
        new_path = new_path.BeforeLast(wxT('/'));
        new_path += wxT("/");
        new_path += label;
        
        wxString old_label = old_path.AfterLast(wxT('/'));

        // the first part of the following 'if' condition checks
        // that we are assigning a new filename to the object;
        // it could be that we are simply changing the case of the
        // existing filename.
        
        if (0 != old_label.CmpNoCase(label) && db->getFileExist(towstr(new_path)))
        {
            wxString message = wxString::Format(_("An object with the name '%s' already exists.  Please choose a different name."),
                                                label.c_str());

            deferredAppMessageBox(message,
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            
            *allow = false;
            return;
        }


        bool rename_result;
        
        {
            AppBusyCursor bc;
            rename_result = db->renameFile(towstr(old_path), towstr(label));
        }
        
        if (rename_result)
        {
            // we'll handle the tree updating ourselves
            *allow = false;

            IDbFolderFsItemPtr folder = item;
            IDbObjectFsItemPtr obj = item;

            item->setLabel(label);
            
            if (folder)
            {
                folder->setPath(new_path);
                if (m_fspanel)
                    m_fspanel->refreshItem(item);
            }
             else if (obj)
            {
                obj->setPath(new_path);
                if (m_fspanel)
                    m_fspanel->refreshItem(item);
            }

            updateCutCopyItems();

            // post a command that lets everyone know that
            // an ofs item has been renamed

            FrameworkEvent* evt = new FrameworkEvent(FRAMEWORK_EVT_TREEPANEL_OFS_FILE_RENAMED);
            evt->s_param = old_path;
            evt->s_param2 = new_path;
            g_app->getMainFrame()->postEvent(evt);
        }
         else
        {
            wxString message = _("The file or table cannot be renamed right now.  The file or table may be in use or otherwise locked.   Please try again later.");
            message += "\n";
            message += db->getErrorString();
            
            deferredAppMessageBox(message,
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            
            *allow = false;
        }
    }
     else
    {
    }

    m_edit_mode = editNone;
}

void DbDoc::onFsItemRightClicked(IFsItemPtr item)
{
    // if we've right-clicked on an item that was not already selected,
    // we need to deselect all selected items and select the right-click
    // item because, for some reason, the FsPanelTreeView does not
    // already take care of this for us
    
    IFsItemEnumPtr items;
    items = m_fspanel->getSelectedItems();

    int i, count;
    count = items->size();
    
    bool rightclick_item_selected = false;
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr selected_item = items->getItem(i);

        if (selected_item == item)
            rightclick_item_selected = true;
    }
    
    if (!rightclick_item_selected)
    {
        m_fspanel->unselectAll();
        m_fspanel->selectItem(item);
    }

    wxMenu menuPopup, menuNew, menuProjects;
    bool root_selected = false;

    IFsItemEnumPtr tree_items = m_fspanel->getSelectedItems();
    int selected_count = tree_items->size();


    // ---------- multiple object right-click menus ----------

    if (selected_count > 1)
    {
        int db_count = 0;
        bool folder_selected = false;
        bool all_mounts = true;

        int i;
        for (i = 0; i < selected_count; ++i)
        {
            IFsItemPtr item = tree_items->getItem(i);
            xd::IDatabasePtr db = getItemDatabase(item);
            
            if (!isItemMount(item))
                all_mounts = false;

            IDbFolderFsItemPtr folder = item;
            if (folder)
            {
                folder_selected = true;

                if (folder->getPath() == wxT("/"))
                {
                    root_selected = true;
                    db_count++;
                    break;
                }
            }
        }


        // if more that one database has been selected, there should not be
        // a right-click menu offered to the user
        if (db_count > 1)
            return;


        // if a database is selected along with some of its objects, there
        // should not be a right-click menu offered to the user
        if (db_count == 1 && selected_count > 1)
            return;


        if (!folder_selected)
        {
            menuPopup.Append(ID_Open, _("&Open"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Cut, _("Cu&t"));
            menuPopup.Append(ID_Copy, _("&Copy"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RemoveItem, _("&Delete"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_ItemProperties, _("Properti&es"));
        }
         else
        {
            if (all_mounts)
            {
                menuPopup.Append(ID_RemoveItem, _("Re&move"));
            }
             else
            {
                menuPopup.Append(ID_RemoveItem, _("&Delete"));
            }
            
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_ItemProperties, _("Properti&es"));
        }

        int result = m_fspanel->popupMenu(&menuPopup);
        
        if (result != 0)
        {
            wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, result);
            ::wxPostEvent(this, e);
        }
        
        return;
    }


    // ---------- single object right-click menus ----------

    IDbFolderFsItemPtr folder = item;
    std::vector<ProjectInfo> projects;

    if (folder)
    {
        // we have clicked on a folder object

        if (folder->getPath() == wxT("/"))
        {
            root_selected = true;
        }

        // new submenu
        wxMenu* submenuNew = new wxMenu;
        submenuNew->Append(ID_NewFolder, _("&Folder"));
        submenuNew->AppendSeparator();
        submenuNew->Append(ID_NewTable, _("&Table"));
        submenuNew->Append(ID_NewReport, _("&Report"));
        submenuNew->Append(ID_NewQuery, _("&Query"));
        submenuNew->Append(ID_NewScript, _("&Script"));

        if (root_selected)
        {
            ProjectMgr projmgr;
            projects = projmgr.getProjectEntries();
            std::sort(projects.begin(), projects.end(), ProjectInfoLess());

            menuPopup.AppendSubMenu(submenuNew, _("&New"));
            menuPopup.Append(ID_Project_RefreshProject, _("Refres&h"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Project_Import, _("&Import..."));
            menuPopup.Append(ID_Project_Export, _("&Export..."));
            menuPopup.Append(ID_Project_ConnectExternal, _("Create Co&nnection..."));
            //menuPopup.Append(ID_Project_ConnectExternalDatabase, _("Create Co&nnection to Database..."));
            menuPopup.AppendSeparator();            

            menuPopup.Append(ID_App_OpenProject, _("P&rojects..."));
            menuPopup.Append(27600, _("&Switch Project"),
                         createProjectsMenu(projects, g_app->getDatabaseLocation(), 27600));
            
            menuPopup.AppendSeparator();            
            menuPopup.Append(ID_Paste, _("&Paste"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Project_ProjectProperties, _("Properti&es"));
        }
         else
        {
            bool is_mount = isItemMount(item);
            bool is_expanded = m_fspanel->isItemExpanded(item);

            if (is_mount)
            {
                if (!is_expanded)
                    menuPopup.Append(ID_ExpandItem, _("&Expand"));
                     else
                    menuPopup.Append(ID_ExpandItem, _("C&ollapse"));

                menuPopup.Append(ID_RefreshItem, _("Refre&sh"));
                menuPopup.AppendSeparator();
                menuPopup.Append(ID_ItemProperties, _("Edit &Connection..."));
                menuPopup.AppendSeparator();
                menuPopup.Append(ID_Paste, _("&Paste"));
                menuPopup.AppendSeparator();
                menuPopup.Append(ID_RemoveItem, _("Re&move"));
                menuPopup.Append(ID_RenameItem, _("&Rename"));
                menuPopup.AppendSeparator();
                menuPopup.Append(ID_ItemProperties, _("Properti&es"));
            }
             else
            {
                if (!is_expanded)
                    menuPopup.Append(ID_ExpandItem, _("&Expand"));
                     else
                    menuPopup.Append(ID_ExpandItem, _("C&ollapse"));

                menuPopup.Append(ID_RefreshItem, _("Refre&sh"));
                menuPopup.AppendSeparator();
                menuPopup.AppendSubMenu(submenuNew, _("&New"));
                //menuPopup.Append(ID_Project_ConnectExternalDatabase, _("Cre&ate Connection to Database..."));
                menuPopup.Append(ID_Project_ConnectExternal, _("Cre&ate Connection..."));
                menuPopup.AppendSeparator();
                menuPopup.Append(ID_Paste, _("&Paste"));
                menuPopup.AppendSeparator();
                menuPopup.Append(ID_RemoveItem, _("&Delete"));
                menuPopup.Append(ID_RenameItem, _("&Rename"));
                menuPopup.AppendSeparator();
                menuPopup.Append(ID_ItemProperties, _("P&roperti&es"));
            }
        }

        if (g_cutcopy_items.size() == 0)
        {
            if (menuPopup.FindItem(ID_Paste))
            {
                menuPopup.Enable(ID_Paste, false);
            }
        }

        int result = m_fspanel->popupMenu(&menuPopup);
        
        if (result != 0)
        {
            // handle menu commands for changing projects
            if (result >= 27600 && result <= 27699)
            {
                size_t i = result-27600;
                if (i >= projects.size())
                {
                    // user clicked the "Edit..." menu item
                    g_app->getAppController()->showProjectManager();
                }
                 else
                {
                    // if the project we're switching to is a different project
                    // and the previous project is successfully closed, open
                    // the new project
                    ProjectInfo info = projects[i];
                    if (info.location != g_app->getDatabaseLocation() && 
                        g_app->getAppController()->closeProject())
                    {
                        // user clicked on one of the projects;
                        // set that project to the active project
                        g_app->getAppController()->openProject(info);
                    }
                }
            }
            else
            {
                wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, result);
                ::wxPostEvent(this, e);
            }
        }

        return;
    }


    IDbObjectFsItemPtr obj = item;
    if (obj)
    {
    
        wxMenu* submenuOpenWith = new wxMenu;
        submenuOpenWith->Append(ID_OpenAsText, _("Text &Editor"));
        submenuOpenWith->Append(ID_OpenAsTable, _("&Table Browser"));
        submenuOpenWith->Append(ID_OpenAsWeb, _("&Web Browser"));
    
        // we have clicked on a non-folder object
        
        if (obj->getType() == dbobjtypeSet)
        {
            // if we're on a set, distinguish between database
            // sets and external file sets; allow external file
            // sets to be opened in text mode, since they might
            // be regular text files
            
            // TODO: is there a better way to find out if the
            // current item is part of a filesystem?
            bool fs_mount = false;
            xd::IDatabasePtr db = g_app->getDatabase()->getMountDatabase(towstr(getFsItemPath(item)));
            if (db.isOk() && db->getDatabaseType() == xd::dbtypeFilesystem)
                fs_mount = true;

            menuPopup.Append(ID_Open, _("&Open"));

            if (!fs_mount)
            {
                menuPopup.Append(ID_ModifyStructure, _("&Edit Structure"));
                delete submenuOpenWith; // we aren't using this submenu, so delete it to prevent memory leak
            }
             else
            {
                menuPopup.AppendSubMenu(submenuOpenWith, _("Open &With"));
            }

            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Cut, _("Cu&t"));
            menuPopup.Append(ID_Copy, _("&Copy"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RemoveItem, _("&Delete"));
            menuPopup.Append(ID_RenameItem, _("&Rename"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_ItemProperties, _("P&roperti&es"));
        }
         else if (obj->getType() == dbobjtypeScript)
        {
            menuPopup.Append(ID_Open, _("&Open"));
            menuPopup.AppendSubMenu(submenuOpenWith, _("Open &With"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RunScript, _("Ru&n"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Cut, _("Cu&t"));
            menuPopup.Append(ID_Copy, _("&Copy"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RemoveItem, _("&Delete"));
            menuPopup.Append(ID_RenameItem, _("&Rename"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_ItemProperties, _("Properti&es"));
        }
         else if (obj->getType() == dbobjtypeTemplate)
        {
            menuPopup.Append(ID_Open, _("&Open"));
            menuPopup.AppendSubMenu(submenuOpenWith, _("Open &With"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RunQuery, _("Ru&n"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Cut, _("Cu&t"));
            menuPopup.Append(ID_Copy, _("&Copy"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RemoveItem, _("&Delete"));
            menuPopup.Append(ID_RenameItem, _("&Rename"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_ItemProperties, _("Properti&es"));
        }
         else if (obj->getType() == dbobjtypeReport)
        {
            menuPopup.Append(ID_Open, _("&Open"));
            menuPopup.AppendSubMenu(submenuOpenWith, _("Open &With"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RunReport, _("Ru&n"));
            //menuPopup.Append(ID_PrintReport, _("Pri&nt"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Cut, _("Cu&t"));
            menuPopup.Append(ID_Copy, _("&Copy"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RemoveItem, _("&Delete"));
            menuPopup.Append(ID_RenameItem, _("&Rename"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_ItemProperties, _("Properti&es"));
        }
         else if (obj->getType() == dbobjtypeBookmark)
        {
            menuPopup.Append(ID_Open, _("&Open"));
            menuPopup.AppendSubMenu(submenuOpenWith, _("Open &With"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Cut, _("Cu&t"));
            menuPopup.Append(ID_Copy, _("&Copy"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_RemoveItem, _("&Delete"));
            menuPopup.Append(ID_RenameItem, _("&Rename"));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_ItemProperties, _("Properti&es"));
        }

        int result = m_fspanel->popupMenu(&menuPopup);

        if (result != 0)
        {
            wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, result);
            ::wxPostEvent(this, e);
        }
    }
}

void DbDoc::onFsItemHighlightRequest(IFsItemPtr& item)
{
    IDbFolderFsItemPtr folder = item;
    if (!folder)
    {
        item = m_fspanel->getItemParent(item);
    }
}

void DbDoc::onDragDrop(IFsItemPtr target,
                       wxDataObject* data,
                       wxDragResult* result)
{

    if (data->GetPreferredFormat() != wxDataFormat(wxT("application/vnd.kx.fspanel")))
    {
        *result = wxDragNone;
        return;
    }

    
    IDbFolderFsItemPtr folder = target;
    if (!folder)
    {
        appMessageBox(_("Files can only be dragged into folders"),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);

        *result = wxDragNone;
        return;
    }


    wxString target_path = getFsItemPath(target);


    FsDataObject* tree_data = (FsDataObject*)data;
    IFsItemEnumPtr items = tree_data->getFsItems();

    size_t i, count = items->size();


    // if one or more of the items is being dragged onto
    // itself cancel the drag operation
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        if (item == target)
        {
            *result = wxDragNone;
            return;
        }
    }


    // check if the items have been moved at all

    bool ok = false;
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        if (m_fspanel->getItemParent(item) != target)
        {
            ok = true;
            break;
        }
    }

    if (!ok)
    {
        *result = wxDragNone;
        return;
    }



    // perform the copy or move operation

    std::vector<jobs::IJobPtr> jobs;


    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);

        std::wstring src_path = towstr(getFsItemPath(item));
        std::wstring src_name = kl::afterLast(src_path, '/');
        std::wstring dest_folder = towstr(getFsItemPath(target));

        xd::IDatabasePtr db = g_app->getDatabase();
        xd::IDatabasePtr source_db = getItemDatabase(item);
        xd::IDatabasePtr dest_db = getItemDatabase(target);
        std::wstring dest_driver = getDbDriver(dest_db);


        std::wstring src_cstr, src_rpath;
        std::wstring dest_cstr, dest_rpath;

        bool src_ismount = getMountPointHelper(db, src_path, src_cstr, src_rpath);
        bool dest_ismount = getMountPointHelper(db, dest_folder, dest_cstr, dest_rpath);
        bool cross_mount = (src_ismount != dest_ismount || src_cstr != dest_cstr) ? true : false;

        std::wstring dest_name = src_name;
        if (g_app->getDbDriver() != L"xdfs" && dest_driver == L"xdfs")
        {
            // if we're saving the file to a filesystem mount and no extension
            // is specified, then automatically add a 'csv' or 'js' extension; this
            // is a usability issue since without the extension, the user usually
            // ends up adding this as the first order of business after saving
            if (getExtensionFromPath(dest_name).length() == 0)
            {
                IDbObjectFsItemPtr dbobject_item = item;
                if (dbobject_item.isOk() && dbobject_item->getType() == dbobjtypeSet)
                    dest_name += L".csv";
                else if (dbobject_item.isOk() && dbobject_item->getType() == dbobjtypeScript)
                    dest_name += L".js";
            }
        }
         else
        {
            // dest database is not a filesystem
            if (cross_mount)
            {
                dest_name = stripExtension(dest_name);
                kl::replaceStr(dest_name, L".", L"_");
                dest_name = makeValidObjectName(dest_name, dest_db);
            }
        }

        std::wstring dest_path = dest_folder;
        if (dest_path.empty() || dest_path[dest_path.length()-1] != '/')
            dest_path += L"/";
        dest_path += dest_name;


        if (cross_mount)
        {
            // the file is being dragged across a mount barrier, which
            // precludes a simple move
            jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.copy-job");

            kl::JsonNode params;
            params["input"] = src_path;
            params["output"] = dest_path;

            std::wstring definition_json;
            if (readStreamTextFile(g_app->getDatabase(), src_path + L".def", definition_json))
            {
                kl::JsonNode input_format;

                if (input_format.fromString(definition_json))
                {
                    params["input_format"].copyFrom(input_format);
                }
            }

            job->setParameters(params.toString());
            jobs.push_back(job);
        }
         else
        {
            // the file is being dragged within a mount, which allows
            // us to perform a simple move
            if (db->moveFile(src_path, dest_path))
            {
                if (tree_data->getSourceId() != ID_Toolbar_Link)
                {
                    m_fspanel->remove(item);
                }
                 else
                {
                    g_app->getAppController()->refreshLinkBar();
                }

                // post a command that lets everyone know that
                // an ofs item has been moved

                FrameworkEvent* evt = new FrameworkEvent(FRAMEWORK_EVT_TREEPANEL_OFS_FILE_RENAMED);
                evt->s_param = src_path;
                evt->s_param2 = dest_path;
                g_app->getMainFrame()->postEvent(evt);
            }
        }
    }

    if (count > jobs.size())
    {
        m_fspanel->refreshItem(target);
    }

    if (jobs.size() > 0)
    {
        jobs::IJobPtr aggregate_job = jobs::createAggregateJob(jobs);
        aggregate_job->getJobInfo()->setTitle(towstr(_("Copying Records")));
        aggregate_job->setDatabase(g_app->getDatabase());
        aggregate_job->setExtraValue(L"refresh-folder", towstr(target_path));

        g_app->getJobQueue()->addJob(aggregate_job, jobs::jobStateRunning);
                
        aggregate_job->sigJobFinished().connect(this, &DbDoc::onCopyJobFinished);
    }
     else
    {
        IFsItemPtr target = getFsItemFromPath(target_path);
        if (target)
        {
            m_fspanel->expand(target);
            m_fspanel->unselectAll();
            
            // this if says 'don't select root item if it's hidden'
            if ((m_style & fsstyleTreeHideRoot) == 0 ||
                 m_fspanel->getRootItem() != target)
            {
                m_fspanel->selectItem(target);
            }
        }
    }

}

void DbDoc::onCopyJobFinished(jobs::IJobPtr job)
{
    // first copy any tabledoc models

    std::wstring params = job->getParameters();

    kl::JsonNode params_node;
    params_node.fromString(params);

    if (params_node.childExists(L"jobs"))
    {
        kl::JsonNode jobs_node = params_node["jobs"];

        // create job objects from each parameter json node

        size_t i, cnt = jobs_node.getChildCount();

        xd::IDatabasePtr db = g_app->getDatabase();
        if (db.isNull())
        {
            cnt = 0;
        }

        for (i = 0; i < cnt; ++i)
        {
            kl::JsonNode job_node = jobs_node[(int)i];

            kl::JsonNode metadata = job_node["metadata"];
            if (!metadata.isOk())
                continue;

            kl::JsonNode type = metadata["type"];
            if (type.isOk() && type.getString() == L"application/vnd.kx.copy-job")
            {
                kl::JsonNode copy_params = job_node["params"];
                if (copy_params.isOk())
                {
                    kl::JsonNode input = copy_params["input"];
                    kl::JsonNode output = copy_params["output"];

                    if (input.isOk() && output.isOk())
                    {
                        std::wstring input_path = input.getString();
                        std::wstring output_path = output.getString();

                        std::wstring input_id, output_id;
                        xd::IFileInfoPtr finfo;

                        finfo = db->getFileInfo(input_path);
                        if (finfo) input_id = finfo->getObjectId();

                        finfo = db->getFileInfo(output_path);
                        if (finfo) output_id = finfo->getObjectId();

                        if (input_id.length() > 0 && output_id.length() > 0)
                        {
                            TableDocMgr::copyModel(input_id, output_id);
                        }
                    }
                }
            }
        }
    }


    //  now refresh the database tree

    std::wstring folder_to_refresh = job->getExtraValue(L"refresh-folder");
    if (folder_to_refresh.length() > 0)
    {
        IFsItemPtr item = getFsItemFromPath(folder_to_refresh);
        if (item)
        {
            m_fspanel->refreshItem(item);
            return;
        }
    }

    m_fspanel->refresh();
}

void DbDoc::onSetConnectionPropertiesFinished(ConnectionWizard* dlg)
{
    // get the connection string
    wxString conn_str = dlg->getConnectionString();
    xd::IDatabasePtr db = getItemDatabase(m_edit_item);
    wxASSERT_MSG(db.p, wxT("Missing db"));

    // set the mount point
    wxString path = getFsItemPath(m_edit_item);
    g_app->getDatabase()->setMountPoint(towstr(path), towstr(conn_str), L"/");

    // refresh the project
    refresh();
}

