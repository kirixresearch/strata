/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-06-30
 *
 */


#include "kxinter.h"
#include "appcontroller.h"
#include "treecontroller.h"
#include "treemodel.h"
#include "jobconvert.h"
#include "../kappcmn/dlgconnectionwizard.h"
#include "structwindow.h"
#include "datadoc.h"


enum
{
    // -- Tree Menu Items --
    ID_Menu_NewConnection = wxID_HIGHEST + 1,
    ID_Menu_NewFolder,
    ID_Menu_SetAsActive,
    ID_Menu_Remove,
    ID_Menu_Rename,
    ID_Menu_Delete,
    ID_Menu_Open,
    ID_Menu_CreateTable,
    ID_Menu_EditStructure,
    ID_Menu_Properties,
    ID_SetFileLayout,
    ID_Convert
};


BEGIN_EVENT_TABLE(TreeController, wxEvtHandler)
    EVT_MENU(ID_Menu_Remove, TreeController::onItemRemove)
    EVT_MENU(ID_Menu_Rename, TreeController::onItemRename)
    EVT_MENU(ID_Menu_Delete, TreeController::onItemDelete)
    EVT_MENU(ID_Menu_Properties, TreeController::onItemProperties)
    EVT_MENU(ID_Menu_NewConnection, TreeController::onNewConnection)
    EVT_MENU(ID_Menu_NewFolder, TreeController::onNewFolder)
    EVT_MENU(ID_Menu_SetAsActive, TreeController::onSetAsActiveDatabase)
    EVT_MENU(ID_Menu_Open, TreeController::onOpen)
    EVT_MENU(ID_Menu_CreateTable, TreeController::onCreateTable)
    EVT_MENU(ID_Menu_EditStructure, TreeController::onEditStructure)
    EVT_MENU(ID_SetFileLayout, TreeController::onSetFileLayout)
    EVT_MENU(ID_Convert, TreeController::onConvert)
END_EVENT_TABLE()


TreeController::TreeController()
{
    m_tree.create_instance("cfw.TreePanel");

    g_app->getMainFrame()->createSite(m_tree,
                                      cfw::sitetypeDockable | cfw::dockLeft,
                                      0, 0, 220, 220);

    m_tree->sigItemActivated().connect(this, &TreeController::onItemActivated);
    m_tree->sigItemRightClicked().connect(this, &TreeController::onItemRightClicked);
    m_tree->sigItemEndLabelEdit().connect(this, &TreeController::onItemEndLabelEdit);
    m_tree->sigDragDrop().connect(this, &TreeController::onDragDrop);

    populate();
}

cfw::ITreePanelPtr TreeController::getTreePanel()
{
    return m_tree;
}

void TreeController::populate()
{
    // -- create the root item for the tree --

    cfw::IGenericTreeItemPtr root_item;
    root_item.create_instance("cfw.GenericTreeItem");
    cfw::ITreeItemPtr root_treeitem = root_item;
    root_treeitem->setBitmap(GET_XPM(xpm_workspace));
    root_treeitem->setItemStringData(wxT("RootTreeItem"));
    root_treeitem->setLabel(_("Workspace"));

    // -- add all of our non-directory connections to the tree --

    cfw::ITreeItemPtr active_item;
    ConnectionMgr* mgr = g_app->getConnectionMgr();
    int connection_count = mgr->getConnectionCount();

    int i;
    for (i = 0; i < connection_count; ++i)
    {
        IConnectionPtr c = mgr->getConnection(i);
     
        if (c->getType() != dbtypeFilesystem)
        {
            DatabaseFolderTreeItem* item = new DatabaseFolderTreeItem;
            item->setBitmap(GET_XPM(xpm_database));
            item->setLabel(c->getDescription());
            item->setItemData((long)i);
            item->setConnection(c);

            if (connection_count == 1)
            {
                g_app->getConnectionMgr()->setActiveConnection(item->getConnection());
                g_app->getAppController()->saveRegistry();
            }

            if (c == g_app->getConnectionMgr()->getActiveConnection())
            {
                active_item = item;
            }

            root_item->addTreeItem(static_cast<cfw::ITreeItem*>(item));
        }
    }

    // -- add all of our directories to the tree --

    for (i = 0; i < connection_count; ++i)
    {
        IConnectionPtr c = mgr->getConnection(i);

        if (c->getType() == dbtypeFilesystem)
        {
            cfw::IDirectoryTreeItemPtr dir_item;
            dir_item.create_instance("cfw.DirectoryTreeItem");
            dir_item->setPath(c->getPath());
            dir_item->setVisibleFileTypes(c->getFilter());

            cfw::ITreeItemPtr item = dir_item;
            item->setLabel(c->getDescription());
            item->setItemData((long)i);
        
            root_item->addTreeItem(dir_item);
        }
    }

    m_tree->setRootItem(root_item);
    m_tree->refresh();

    if (active_item)
    {
        m_tree->setItemBold(active_item, true);
    }
}


void TreeController::refreshItem(cfw::ITreeItemPtr item)
{
    if (item)
    {
        m_tree->refresh(item);
    }
}


void TreeController::refreshAllItems()
{
    m_tree->refreshAll();
}


void TreeController::onItemRemove(wxCommandEvent& event)
{
    IConnectionPtr c;

    ConnectionMgr* conn_mgr = g_app->getConnectionMgr();

    IDatabaseFolderTreeItemPtr db_item = m_rightclick_item;
    if (db_item)
    {
        c = db_item->getConnection();
    }

    cfw::IDirectoryTreeItemPtr dir_item = m_rightclick_item;
    if (dir_item)
    {
        c = conn_mgr->getConnection(m_rightclick_item->getItemData());
    }

    if (c)
    {
        // -- if we are removing the current active connection, set the active connection
        //    to the next item up on the tree which is a database folder tree item --
        if (c == conn_mgr->getActiveConnection())
        {
            int i;
            int conn_count = conn_mgr->getConnectionCount();
            bool cur_active_conn_found = false;
            bool new_active_conn_set = false;
            IConnectionPtr new_active_conn;

            // -- search up the tree for the next database folder tree item --
            for (i = (conn_count-1); i >= 0; --i)
            {
                new_active_conn = conn_mgr->getConnection(i);

                if (new_active_conn == c)
                {
                    cur_active_conn_found = true;
                    continue;
                }

                if (cur_active_conn_found)
                {
                    if (new_active_conn->getType() != dbtypeUndefined &&
                        new_active_conn->getType() != dbtypeFilesystem)
                    {
                        conn_mgr->setActiveConnection(new_active_conn);
                        new_active_conn_set = true;
                        break;
                    }
                }
            }

            // -- search up the tree for the next database folder tree item --
            if (!new_active_conn_set)
            {
                for (i = 0; i < conn_count; ++i)
                {
                    new_active_conn = conn_mgr->getConnection(i);

                    if (new_active_conn == c)
                    {
                        cur_active_conn_found = true;
                        continue;
                    }

                    if (cur_active_conn_found)
                    {
                        if (new_active_conn->getType() != dbtypeUndefined &&
                            new_active_conn->getType() != dbtypeFilesystem)
                        {
                            conn_mgr->setActiveConnection(new_active_conn);
                            break;
                        }
                    }
                }
            }
        }

        conn_mgr->removeConnection(c);

        cfw::IGenericTreeItemPtr parent_item = m_tree->getItemParent(m_rightclick_item);
        parent_item->deleteTreeItem(m_rightclick_item);

        g_app->getAppController()->saveRegistry();

        // -- I would prefer to do the following and just refresh the tree instead of repopulate it,
        //    however, that is not possible because we need to set the bold item --
        //m_tree->refresh();
        populate();
    }
}

void TreeController::onItemRename(wxCommandEvent& event)
{
    m_tree->editLabel(m_rightclick_item);
}

void TreeController::onItemDelete(wxCommandEvent& event)
{
    int i;
    cfw::ITreeItemEnumPtr items;
    items = m_tree->getSelectedItems();

    for (i = 0; i < items->size(); ++i)
    {
        IDatabaseObjectTreeItemPtr db_obj = items->getItem(i);
        if (db_obj)
        {
            db_obj->getConnection()->getDatabasePtr()->deleteFile(towstr(db_obj->getPath()));
        }
    }

    m_tree->refresh(m_tree->getItemParent(m_rightclick_item));
}


// -- implemented in appcontroller.cpp --
void onDlgConnectionWizardFinished(DlgConnectionWizard* dlg);


void TreeController::onItemProperties(wxCommandEvent& event)
{
    cfw::IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("DlgConnectionWizard"));
    if (site.isNull())
    {
        DlgConnectionWizard* cw = new DlgConnectionWizard(dbtypeExternalConnection|dbtypeXdnative);
        cw->sigConnectionWizardFinished.connect(&onDlgConnectionWizardFinished);
        
        site = g_app->getMainFrame()->createSite(cw, cfw::sitetypeModeless, -1, -1, 430, 480);
        site->setName(wxT("DlgConnectionWizard"));
    }
     else
    {
        if (!site->getVisible())
        {
            site->setVisible(true);
        }
    }

    IConnectionWizardPtr cw = site->getDocument();
    if (cw)
    {
        IConnectionPtr c;


        IDatabaseFolderTreeItemPtr db_item = m_rightclick_item;
        if (db_item)
        {
            c = db_item->getConnection();
        }

        cfw::IDirectoryTreeItemPtr dir_item = m_rightclick_item;
        if (dir_item)
        {
            c = g_app->getConnectionMgr()->getConnection(m_rightclick_item->getItemData());
        }

        if (c)
        {
            cw->setData((long)c);

            if (c->getType() == dbtypeXdnative ||
                c->getType() == dbtypeFilesystem)
            {
                // -- the path selection page --
                //cw->goPage(1);
            }
             else
            {
                // -- the server properties page --
                //cw->goPage(3);
            }
        }
    }
}

void TreeController::onNewConnection(wxCommandEvent& event)
{
    wxCommandEvent e;
    g_app->getAppController()->onNewConnection(e);
}

void TreeController::onNewFolder(wxCommandEvent& event)
{

}

void TreeController::onSetAsActiveDatabase(wxCommandEvent& event)
{
    cfw::ITreeItemPtr root = m_tree->getRootItem();
    cfw::ITreeItemEnumPtr children = root->getChildren();

    int i;
    int count = children->size();
    for (i = 0; i < count; ++i)
    {
        m_tree->setItemBold(children->getItem(i), false);
    }
    m_tree->setItemBold(m_rightclick_item, true);

    IDatabaseFolderTreeItemPtr db_item = m_rightclick_item;
    if (db_item)
    {
        g_app->getConnectionMgr()->setActiveConnection(db_item->getConnection());
    }
    g_app->getAppController()->saveRegistry();
}

void TreeController::onOpen(wxCommandEvent& event)
{
    int i;
    cfw::ITreeItemEnumPtr items;
    items = m_tree->getSelectedItems();

    for (i = 0; i < items->size(); ++i)
    {
        IDatabaseObjectTreeItemPtr db_obj = items->getItem(i);
        onItemActivated(db_obj);
    }
}

void TreeController::onCreateTable(wxCommandEvent& event)
{
    StructWindow* sw = new StructWindow;
    g_app->getMainFrame()->createSite(sw, cfw::sitetypeModeless, -1, -1, 500, 400);
}

void TreeController::onEditStructure(wxCommandEvent& event)
{
    cfw::ITreeItemEnumPtr items;
    items = m_tree->getSelectedItems();
    if (items->size() != 1)
        return;
    cfw::ITreeItemPtr item = items->getItem(0);
    

    tango::ISetPtr modify_set;
    wxString set_path;

    IDatabaseObjectTreeItemPtr obj = item;
    if (obj)
    {
        IConnectionPtr c = obj->getConnection();
        modify_set = c->getDatabasePtr()->openSet(towstr(obj->getPath()));
        set_path = obj->getPath();
    }

    cfw::IFileTreeItemPtr file = item;
    if (file)
    {
        modify_set = g_app->getXbaseDatabase()->openSet(towstr(file->getPath()));
        set_path = file->getPath();
    }

    if (!modify_set)
    {
        return;
    }

    StructWindow* sw = new StructWindow;
    sw->setModifySet(modify_set);
    g_app->getMainFrame()->createSite(sw, cfw::sitetypeModeless, -1, -1, 500, 400);
}

void TreeController::onItemEndLabelEdit(cfw::ITreeItemPtr item, wxString label, bool cancelled, bool* allow)
{
    if (cancelled)
    {
        *allow = false;
        return;
    }

    item->setLabel(label);

    // -- renaming a database folder label --
    IDatabaseFolderTreeItemPtr db_folder_item = item;
    if (db_folder_item)
    {
        int idx = item->getItemData();
        IConnectionPtr conn = g_app->getConnectionMgr()->getConnection(idx);
        conn->setDescription(label);
    }

    // -- renaming a database table on the server --
    IDatabaseObjectTreeItemPtr db_object_item = item;
    if (db_object_item)
    {
        db_object_item->getConnection()->getDatabasePtr()->renameFile(towstr(db_object_item->getPath()), towstr(label));
    }

    // -- renaming a file on the hard drive --
    cfw::IFileTreeItemPtr file_item = item;
    if (file_item)
    {

    }

    // -- renaming a directory label --
    cfw::IDirectoryTreeItemPtr dir_item = item;
    if (dir_item)
    {
        int idx = item->getItemData();
        IConnectionPtr conn = g_app->getConnectionMgr()->getConnection(idx);
        conn->setDescription(label);
    }

    g_app->getAppController()->saveRegistry();
}

void TreeController::onItemActivated(cfw::ITreeItemPtr item)
{
    // -- find out the item type --
    cfw::IFileTreeItemPtr file_item = item;
    if (file_item)
    {
        wxString path = file_item->getPath();
        path = path.AfterLast(wxT('.'));
        if (path.CmpNoCase(wxT("ebc")) == 0 ||
            path.CmpNoCase(wxT("txt")) == 0 ||
            path.CmpNoCase(wxT("asc")) == 0)
        {
            g_app->getAppController()->openDataDoc(file_item->getPath());
        }
         else if (path.CmpNoCase(wxT("dbf")) == 0)
        {
            g_app->getAppController()->openTableDoc(g_app->getXbaseDatabase(), file_item->getPath());
        }
         else
        {
            g_app->getAppController()->openTableDoc(g_app->getDelimitedTextDatabase(), file_item->getPath());
        }

        return;
    }

    IDatabaseObjectTreeItemPtr table_item = item;
    if (table_item)
    {
        g_app->getAppController()->openTableDoc(table_item->getConnection()->getDatabasePtr(), table_item->getPath());
    }
}


void TreeController::onItemRightClicked(cfw::ITreeItemPtr item)
{
    cfw::ITreeItemEnumPtr tree_items = m_tree->getSelectedItems();

    if (tree_items->size() > 1)
    {
        // -- multiple tree items selected --
        bool all_filetree_items = true;
        
        int selected_count = tree_items->size();
        int i;
        for (i = 0; i < selected_count; ++i)
        {
            cfw::ITreeItemPtr item = tree_items->getItem(i);
            
            cfw::IFileTreeItemPtr f = item;
            if (!f)
            {
                all_filetree_items = false;
            }
        }

        wxMenu menuPopup;

        if (all_filetree_items)
        {
            menuPopup.Append(ID_SetFileLayout, _("Set File Layout..."));
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_Convert, _("Convert"));
            menuPopup.AppendSeparator();
            menuPopup.Append(0, _("Cancel"));
        }


        int result = m_tree->popupMenu(&menuPopup);

        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, result);
        ::wxPostEvent(this, e);

        return;
    }



    m_rightclick_item = item;

    cfw::wxBitmapMenu menu;
    int res = 0;

    // -- find out the item type, and display the appropriate right-click menu --

    cfw::IGenericTreeItemPtr gen_item = item;
    if (gen_item)
    {
        // -- right-clicked on the root treeitem --
        if (item->getItemStringData() == wxT("RootTreeItem"))
        {
            menu.Append(ID_Menu_NewConnection, _("New &Connection..."));
            menu.AppendSeparator();
            menu.Append(ID_Menu_Properties, _("&Properties..."));
        }
    }

    IDatabaseFolderTreeItemPtr db_folder_item = item;
    if (db_folder_item)
    {
        IConnectionPtr c = db_folder_item->getConnection();

        if (c->getType() == dbtypeXdnative)
        {
            menu.Append(ID_Menu_NewFolder, _("&New Folder"));
        }
        menu.Append(ID_Menu_CreateTable, _("&Create Table..."));
        menu.AppendSeparator(); 
        menu.Append(ID_Menu_Remove, _("&Remove From Workspace"));
        menu.Append(ID_Menu_Rename, _("Re&name"));
        menu.AppendSeparator(); 
        menu.Append(ID_Menu_SetAsActive, _("Set As Acti&ve Database"));
        menu.AppendSeparator(); 
        menu.Append(ID_Menu_Properties, _("&Properties..."));
    }

    cfw::IDirectoryTreeItemPtr dir_item = item;
    if (dir_item)
    {
        menu.Append(ID_Menu_NewFolder, _("&New Folder"));
        menu.AppendSeparator();
        menu.Append(ID_Menu_CreateTable, _("&Create Table..."));
        menu.AppendSeparator();
        menu.Append(ID_Menu_Remove, _("&Remove From Workspace"));
        menu.Append(ID_Menu_Rename, _("Re&name"));
        menu.AppendSeparator();
        menu.Append(ID_Menu_Properties, _("&Properties..."));
    }

    IDatabaseObjectTreeItemPtr db_object_item = item;
    if (db_object_item)
    {
        menu.Append(ID_Menu_Open, _("&Open"));
        menu.AppendSeparator();
        menu.Append(ID_Menu_EditStructure, _("&Edit Structure..."));
        menu.AppendSeparator();
        menu.Append(ID_Menu_Delete, _("&Delete"));
        menu.Append(ID_Menu_Rename, _("Re&name"));
        menu.AppendSeparator();
        menu.Append(ID_Menu_Properties, _("&Properties..."));

    }

    cfw::IFileTreeItemPtr file_item = item;
    if (file_item)
    {
        menu.Append(ID_Menu_Open, _("&Open"));
        menu.AppendSeparator();
        menu.Append(ID_Menu_EditStructure, _("&Edit Structure..."));
        menu.AppendSeparator();
        menu.Append(ID_Menu_Delete, _("&Delete"));
        menu.Append(ID_Menu_Rename, _("Re&name"));
        menu.AppendSeparator();
        menu.Append(ID_Menu_Properties, _("&Properties..."));
    }

    res = m_tree->popupMenu(&menu);

    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, res);
    ::wxPostEvent(this, e);
}


static wxString _getItemPath(cfw::ITreeItemPtr item)
{
    // -- get item path --
    IDatabaseFolderTreeItemPtr folder;
    IDatabaseObjectTreeItemPtr obj;
    cfw::IFileTreeItemPtr file;

    folder = item;
    obj = item;
    file = item;

    if (folder)
    {
        return folder->getPath();
    }
     else if (obj)
    {
        return obj->getPath();
    }
     else if (file)
    {
        return file->getPath();
    }

    return wxEmptyString;
}


void TreeController::onDragDrop(cfw::ITreeItemPtr target, wxDataObject* data, wxDragResult* result)
{
    IDatabaseFolderTreeItemPtr folder = target;
    if (!folder)
    {
        cfw::appMessageBox(_("Files can only be dragged into folders."),
                        PRODUCT_NAME,
                        wxOK);

        *result = wxDragNone;
        return;
    }


    // -- make sure the connection is open --

    IConnectionPtr c = folder->getConnection();

    if (!c)
        return;

    if (!c->isOpen())
    {
        if (!c->open())
        {
            return;
        }
    }


    cfw::TreeDataObject* tree_data = (cfw::TreeDataObject*)data;
    cfw::ITreeItemEnumPtr items = tree_data->getTreeItems();


    int count = items->size();
    int i;

    for (i = 0; i < count; ++i)
    {
        cfw::ITreeItemPtr item = items->getItem(i);

        wxString src_path = _getItemPath(item);
        wxString dest_location = _getItemPath(target);



        IDatabaseObjectTreeItemPtr src_db_item = item;
        cfw::IFileTreeItemPtr src_file_item = item;


        tango::IDatabasePtr dest_db = folder->getConnection()->getDatabasePtr();
        tango::IDatabasePtr src_db;

        if (!src_file_item)
        {
            src_db = src_db_item->getConnection()->getDatabasePtr();
        }
         else
        {
            // -- determine which file type it is and assign
            //    the appropriate tango::IDatabasePtr --
            src_db = g_app->getXbaseDatabase();
        }


        // -- compare the two item's tango::IDatabasePtr and if they are the same,
        //    do not perform a conversion, instead just move the table --
        if (src_db == dest_db)
        {
            if (src_db->moveFile(towstr(src_path), towstr(dest_location)))
            {
                m_tree->remove(item);
                //refresh();
            }
        }



        // -- database item --
        if (src_db_item)
        {
            wxString fname = src_path.AfterLast(wxT('/'));

            wxString dest_path;
            dest_path = dest_location;
            if (dest_path.IsEmpty() || dest_path.Last() != wxT('/'))
            {
                dest_path += wxT("/");
            }
            dest_path += fname;



            ConvertJob* job = new ConvertJob;

            job->setInstructions(src_db, src_path,
                                 dest_db, dest_path,
                                 item, target);

            g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
            g_app->getAppController()->showJobManager();            
        }


        // -- file item --
        if (src_file_item)
        {
            // -- trim the path --
            wxString fname = src_path.AfterLast(wxT('\\'));
            fname = fname.BeforeLast(wxT('.'));

            wxString dest_path;
            dest_path = dest_location;
            if (dest_path.IsEmpty() || dest_path.Last() != wxT('/'))
            {
                dest_path += wxT("/");
            }
            dest_path += fname;



            ConvertJob* job = new ConvertJob;

            job->setInstructions(src_db, src_path,
                                 dest_db, dest_path,
                                 item, target);

            g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
            g_app->getAppController()->showJobManager();            
        }
    }
}


void TreeController::onSetFileLayout(wxCommandEvent& event)
{
    wxString filter;
    filter =  _("Layout Files (*.kfl)|*.kfl|All Files (*.*)|*.*|");

    wxFileDialog dlg(g_app->getMainFrame()->getFrameWindow(),
                     _("Set Active Layout"),
                     wxT(""), wxT(""),
                     filter,
                     wxHIDE_READONLY);

    if (dlg.ShowModal() != wxID_OK)
    {
        return;
    }


    wxString layout_file = dlg.GetPath();


    cfw::ITreeItemEnumPtr tree_items = m_tree->getSelectedItems();

    // -- multiple tree items selected --
    bool all_filetree_items = true;
    
    int selected_count = tree_items->size();
    int i;
    for (i = 0; i < selected_count; ++i)
    {
        cfw::ITreeItemPtr item = tree_items->getItem(i);
        cfw::IFileTreeItemPtr f = item;

        if (f)
        {
            setActiveLayoutFile(f->getPath(), layout_file);
        }
    }
}





void TreeController::onConvert(wxCommandEvent& event)
{

    // -- get active connection --

    IConnectionPtr conn = g_app->getConnectionMgr()->getActiveConnection();

    if (!conn)
    {
        cfw::appMessageBox(_("In order to convert data, you must first set an active connection."),
                      PRODUCT_NAME,
                      wxOK | wxCENTER | wxICON_EXCLAMATION,
                      g_app->getMainWindow());
        return;
    }

    if (!conn->isOpen())
    {
        if (!conn->open())
        {
            cfw::appMessageBox(_("The active connection could not be opened."),
                          PRODUCT_NAME,
                          wxOK | wxCENTER | wxICON_EXCLAMATION,
                          g_app->getMainWindow());
            return;
        }
    }


    tango::IDatabasePtr db = conn->getDatabasePtr();

    if (!db)
        return;


    ConvertDataJob* job = new ConvertDataJob;


    cfw::ITreeItemEnumPtr tree_items = m_tree->getSelectedItems();

    // -- multiple tree items selected --
    bool all_filetree_items = true;
    
    int selected_count = tree_items->size();
    int i;
    for (i = 0; i < selected_count; ++i)
    {
        cfw::ITreeItemPtr item = tree_items->getItem(i);
        cfw::IFileTreeItemPtr f = item;

        if (!f)
        {
            delete job;

            cfw::appMessageBox(_("Not all of the selected items are able to be converted."),
                          PRODUCT_NAME,
                          wxOK | wxICON_EXCLAMATION,
                          g_app->getMainWindow());
            return;
        }


        wxString data_file = f->getPath();
        wxString layout_file = getActiveLayoutFile(data_file);
        wxString output_table;
        DataFormatDefinition dfd;

        if (layout_file.IsEmpty())
        {
            delete job;

            wxString message = wxString::Format(_("The file '%s' has no associated layout and therefore cannot be converted."), data_file.c_str());

            cfw::appMessageBox(message,
                          PRODUCT_NAME,
                          wxOK | wxICON_EXCLAMATION,
                          g_app->getMainWindow());
            return;                                
        }

        if (!dfd.load(layout_file))
        {
            delete job;

            wxString message = wxString::Format(_("The layout file '%s' could not be opened."), layout_file.c_str());

            cfw::appMessageBox(message,
                          PRODUCT_NAME,
                          wxOK | wxICON_EXCLAMATION,
                          g_app->getMainWindow());
            return;                                
        }


#ifdef WIN32
        output_table = data_file.AfterLast(wxT('\\'));
#else
        output_table = data_file.AfterLast(wxT('/'));
#endif

        output_table = output_table.BeforeLast(wxT('.'));

        if (output_table.IsEmpty())
        {
            delete job;

            cfw::appMessageBox(_("One or more of the selected files have an invalid filename."),
                          PRODUCT_NAME,
                          wxOK | wxICON_EXCLAMATION,
                          g_app->getMainWindow());

            return;
        }

        output_table.Replace(wxT("/"), wxT("_"));
        output_table.Replace(wxT("\\"), wxT("_"));
        output_table.Replace(wxT(":"), wxT("_"));
        output_table.Replace(wxT("."), wxT("_"));
        output_table.Replace(wxT(" "), wxT("_"));

        IDataStreamPtr data = openFileStream(data_file);

        if (data.isNull())
        {
            delete job;

            wxString message = wxString::Format(_("The file '%s' could not be opened for input."), data_file.c_str());

            cfw::appMessageBox(message,
                          PRODUCT_NAME,
                          wxOK | wxICON_EXCLAMATION,
                          g_app->getMainWindow());
            return;
        }


        job->addInstruction(data, dfd, db, output_table);
    }

    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
    g_app->getAppController()->showJobManager();    
}






