/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-12-09
 *
 */


#ifndef __APP_DBDOC_H
#define __APP_DBDOC_H



enum
{
    dbobjtypeFolder = 1,
    dbobjtypeSet = 2,
    dbobjtypeTemplate = 3,
    dbobjtypeScript = 4,
    dbobjtypeReport = 5,
    dbobjtypeBookmark = 6
};



xcm_interface IDbFolderFsItem;
xcm_interface IDbObjectFsItem;

XCM_DECLARE_SMARTPTR(IDbFolderFsItem)
XCM_DECLARE_SMARTPTR(IDbObjectFsItem)

xcm_interface IDbFolderFsItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IDbFolderFsItem")

public:

    virtual void setPath(const wxString& s) = 0;
    virtual wxString getPath() = 0;
    
    virtual void setType(int type) = 0;
    virtual int getType() = 0;
    
    virtual void setOwner(IDbFolderFsItemPtr owner) = 0;
    virtual IDbFolderFsItemPtr getOwner() = 0;
    
    virtual void setDatabase(tango::IDatabasePtr db) = 0;
    virtual tango::IDatabasePtr getDatabase() = 0;

    virtual void setConnection(IConnectionPtr conn) = 0;
    virtual IConnectionPtr getConnection() = 0;
};





xcm_interface IDbObjectFsItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IDbObjectFsItem")

public:

    virtual bool isFolder() = 0;

    virtual void setPath(const wxString& s) = 0;
    virtual wxString getPath() = 0;
    
    virtual void setType(int type) = 0;
    virtual int getType() = 0;

    virtual void setIsMount(bool b) = 0;
    virtual bool getIsMount() = 0;

    virtual void setOwner(IDbFolderFsItemPtr owner) = 0;
    virtual IDbFolderFsItemPtr getOwner() = 0;
};





class ConnectionWizard;

class DbFolderFsItem : public FsItemBase,
                       public IDbFolderFsItem,
                       public IDbObjectFsItem
{
    XCM_CLASS_NAME("appmain.DbFolderFsItem")
    XCM_BEGIN_INTERFACE_MAP(DbFolderFsItem)
        XCM_INTERFACE_ENTRY(IDbFolderFsItem)
        XCM_INTERFACE_ENTRY(IDbObjectFsItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()

public:

    DbFolderFsItem();
    ~DbFolderFsItem();

    void setLinkBarMode(bool b);
    
    void setConnection(IConnectionPtr conn);
    IConnectionPtr getConnection();

    void setDatabase(tango::IDatabasePtr db);
    tango::IDatabasePtr getDatabase();

    wxString getPath();
    void setPath(const wxString& s);

    void setType(int new_val);
    int getType();

    void setIsMount(bool b);
    bool getIsMount();
    
    void setOwner(IDbFolderFsItemPtr owner);
    IDbFolderFsItemPtr getOwner();
    
    void setChildrenOverride(tango::IFileInfoEnumPtr children_override);
    void setFoldersOnly(bool new_val);
    void setTablesOnly(bool new_val);

    IFsItemEnumPtr getChildren();

    bool isDeferred() { return true; }
    bool isFolder() { return true; }

private:

    bool checkForDisplayOrder(tango::IFileInfoEnumPtr& file_info);

private:

    wxString m_path;
    tango::IDatabasePtr m_db;
    tango::IFileInfoEnumPtr m_children_override;
    IConnectionPtr m_conn;
    IDbFolderFsItemPtr m_owner;
    bool m_only_folders;
    bool m_only_tables;
    bool m_is_mount;
    bool m_link_bar_mode;
};




class DbObjectFsItem : public FsItemBase,
                       public IDbObjectFsItem
{
    XCM_CLASS_NAME("appmain.DbObjectFsItem")
    XCM_BEGIN_INTERFACE_MAP(DbObjectFsItem)
        XCM_INTERFACE_ENTRY(IDbObjectFsItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()

public:

    DbObjectFsItem();
    ~DbObjectFsItem();

    void setPath(const wxString& s);
    wxString getPath();

    void setType(int new_val);
    int getType();

    void setIsMount(bool b);
    bool getIsMount();

    void setOwner(IDbFolderFsItemPtr owner);
    IDbFolderFsItemPtr getOwner();

    bool isFolder() { return false; }

private:

    IDbFolderFsItemPtr m_owner;
    wxString m_path;
    int m_type;
    bool m_is_mount;
};





class DbDoc : public wxEvtHandler,
              public xcm::signal_sink
{
friend class LinkBar;

public:

    // public utility API for DbDoc
    static void actionActivate(IFsItemPtr item, int open_mask);
    static void actionActivateDeferred(IFsItemPtr item, int open_mask);
    static void setFileVisualLocation(const wxString& path, int insert_index);
    static tango::IDatabasePtr getItemDatabase(IFsItemPtr item);
    static bool isItemMount(IFsItemPtr item);
    static bool isItemInMount(IFsItemPtr item);
    static wxString getFsItemPath(IFsItemPtr source);
    static void getFsItemPaths(IFsItemEnumPtr source,
                               std::vector<wxString>& result,
                               bool expand_subfolders);
public:

    DbDoc();
    ~DbDoc();
    
    void ref();
    void unref();
    
    void setStyle(int fspanel_style) { m_style = fspanel_style; }
    void setLinkBarMode(bool value) { m_link_bar_mode = value; }
    void setNoEdit(bool value) { m_no_edit = value; }
    
    // call initAsDocument() if you want to use dbdoc as a cfw document,
    // or call initAsWindow() if you just want a raw window
    bool initAsDocument(IFsItemPtr item = xcm::null);
    bool initAsWindow(wxWindow* window, 
                      wxWindowID id,
                      const wxPoint& position,
                      const wxSize& size,
                      int flags,
                      IFsItemPtr item = xcm::null);
    
    IDocumentSitePtr getDbDocSite();
    IFsPanelPtr getFsPanel();
    wxWindow* getDocWindow();
    
    IFsItemPtr getFsItemFromPath(const wxString& string);
    bool isFsItemExternal(IFsItemPtr item);
    void setDatabase(tango::IDatabasePtr db, const wxString& root_path = wxEmptyString);
    void setRootItem(IDbFolderFsItemPtr root_folder);
    void refresh();

    void doCut();
    void doCopy();
    void doPaste();
    
private:

    // event handlers

    void onCreateExternalConnection(wxCommandEvent& evt);
    void onCreateTable(wxCommandEvent& evt);
    void onImportData(wxCommandEvent& evt);
    void onExportData(wxCommandEvent& evt);
    void onOpenProject(wxCommandEvent& evt);    
    void onProjectProperties(wxCommandEvent& evt);
    void onItemProperties(wxCommandEvent& evt);
    void onRefreshProject(wxCommandEvent& evt);
    void onRefreshItem(wxCommandEvent& evt);
    void onExpandItem(wxCommandEvent& evt);
    void onRemoveItem(wxCommandEvent& evt);
    void onRenameItem(wxCommandEvent& evt);
        
    void onOpen(wxCommandEvent& evt);
    void onOpenAsTable(wxCommandEvent& evt);    
    void onOpenAsText(wxCommandEvent& evt);
    void onOpenAsWeb(wxCommandEvent& evt);
    void onRunQuery(wxCommandEvent& evt);
    void onRunScript(wxCommandEvent& evt);
    void onRunReport(wxCommandEvent& evt);    
    void onPrintReport(wxCommandEvent& evt);
    void onModifyStructure(wxCommandEvent& evt);
    void onCut(wxCommandEvent& evt);
    void onCopy(wxCommandEvent& evt);
    void onPaste(wxCommandEvent& evt);
    void onNewShortcut(wxCommandEvent& evt);
    void onNewFolder(wxCommandEvent& evt);
    void onNewTable(wxCommandEvent& evt);
    void onNewReport(wxCommandEvent& evt);
    void onNewQuery(wxCommandEvent& evt);
    void onNewScript(wxCommandEvent& evt);
    void onNewItemFinished(wxCommandEvent& evt);

    void onSetFocus(wxFocusEvent& evt);
    void onKillFocus(wxFocusEvent& evt);
    void onKeyDown(const wxKeyEvent& evt);

private:

    void openSelectedItems(int open_mask = -1);
    void toggleExpandOnSelectedItems();

    void updateCutCopyItems();

    void deleteFsItem(IFsPanelPtr tree,
                        IFsItemPtr item,
                        std::vector<wxString>& problem_items);

    IFsItemPtr _findFsItem(IFsItemPtr item,
                                const wxString& path);

    wxString getDefaultNewFileName(IFsItemPtr parent,
                                   const wxString& name);
    IFsItemPtr getNewFileParent();

    void onFsItemActivated(IFsItemPtr item);
    void onFsItemBeginLabelEdit(IFsItemPtr item,
                                bool* allow);
    void onFsItemEndLabelEdit(IFsItemPtr item,
                              wxString text,
                              bool cancelled,
                              bool* allow);
    void onFsItemRightClicked(IFsItemPtr item);
    void onFsItemHighlightRequest(IFsItemPtr& item);

    void onDragDrop(IFsItemPtr target,
                    wxDataObject* data,
                    wxDragResult* result);
    
    void onCopyJobFinished(jobs::IJobPtr job);
    void onSetConnectionPropertiesFinished(ConnectionWizard* dlg);

    
private:

    enum
    {
        editNone = 0,
        editNewFolder = 1,
        editRename = 2
    };

    enum
    {
        actionCut = 1,
        actionCopy = 2
    };

private:

    IDocumentSitePtr m_dbdoc_site;
    
    IFsPanelPtr m_fspanel;
    IFsItemPtr m_root_item;
    IFsItemPtr m_newitem_parent;
    IFsItemPtr m_edit_item;
    
    bool m_no_edit;
    bool m_link_bar_mode;
    int m_edit_mode;
    int m_style;

    int m_ref_count;
    
    DECLARE_EVENT_TABLE()
};


#endif

