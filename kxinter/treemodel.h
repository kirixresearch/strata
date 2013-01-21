/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-04-26
 *
 */


#ifndef __KXINTER_TREEMODEL_H
#define __KXINTER_TREEMODEL_H


xcm_interface IDatabaseFolderTreeItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("kxinter.IDatabaseFolderTreeItem")

public:

    virtual IConnectionPtr getConnection() = 0;
    virtual void setConnection(IConnectionPtr c) = 0;

    virtual wxString getPath() = 0;
    virtual void setPath(wxString s) = 0;
};

XCM_DECLARE_SMARTPTR(IDatabaseFolderTreeItem)




xcm_interface IDatabaseObjectTreeItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("kxinter.IDatabaseObjectTreeItem")

public:

    virtual IConnectionPtr getConnection() = 0;
    virtual void setConnection(IConnectionPtr c) = 0;

    virtual wxString getPath() = 0;
    virtual void setPath(wxString s) = 0;
};

XCM_DECLARE_SMARTPTR(IDatabaseObjectTreeItem)




class DatabaseFolderTreeItem : public cfw::TreeItemBase,
                               public IDatabaseFolderTreeItem
{
    XCM_CLASS_NAME("kxinter.DatabaseFolderTreeItem")
    XCM_BEGIN_INTERFACE_MAP(DatabaseFolderTreeItem)
        XCM_INTERFACE_ENTRY(IDatabaseFolderTreeItem)
        XCM_INTERFACE_CHAIN(cfw::TreeItemBase)
    XCM_END_INTERFACE_MAP()

public:
    DatabaseFolderTreeItem();
    ~DatabaseFolderTreeItem();

    bool isDeferred() { return true; }

    void setConnection(IConnectionPtr c);
    IConnectionPtr getConnection();

    void setPath(wxString s);
    wxString getPath();

    // -- TreeItem derived functions --

    cfw::ITreeItemEnumPtr getChildren();

private:

    IConnectionPtr m_connection;
    wxString m_path;
};




class DatabaseObjectTreeItem : public cfw::TreeItemBase,
                               public IDatabaseObjectTreeItem
{
    XCM_CLASS_NAME("kxinter.DatabaseObjectTreeItem")
    XCM_BEGIN_INTERFACE_MAP(DatabaseObjectTreeItem)
        XCM_INTERFACE_ENTRY(IDatabaseObjectTreeItem)
        XCM_INTERFACE_CHAIN(cfw::TreeItemBase)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseObjectTreeItem();
    ~DatabaseObjectTreeItem();

    void setConnection(IConnectionPtr c);
    IConnectionPtr getConnection();

    void setPath(wxString s);
    wxString getPath();

private:

    IConnectionPtr m_connection;
    wxString m_path;
};




#endif




