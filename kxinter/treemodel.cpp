/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-04-26
 *
 */


#include "kxinter.h"
#include "treemodel.h"


// -- DatabaseFolderTreeItem class implementation --


DatabaseFolderTreeItem::DatabaseFolderTreeItem()
{
    m_path = wxT("/");
}

DatabaseFolderTreeItem::~DatabaseFolderTreeItem()
{

}

void DatabaseFolderTreeItem::setConnection(IConnectionPtr c)
{
    m_connection = c;
}

IConnectionPtr DatabaseFolderTreeItem::getConnection()
{
    return m_connection;
}

void DatabaseFolderTreeItem::setPath(wxString s)
{
    m_path = cfw::makeProper(s);
}

wxString DatabaseFolderTreeItem::getPath()
{
    return m_path;
}

cfw::ITreeItemEnumPtr DatabaseFolderTreeItem::getChildren()
{
    xcm::IVectorImpl<cfw::ITreeItemPtr>* vec = new xcm::IVectorImpl<cfw::ITreeItemPtr>;

    if (!m_connection)
        return vec;

    // -- make sure the connection is open --

    if (!m_connection->isOpen())
    {
        if (!m_connection->open())
        {
            return vec;
        }
    }

    tango::IDatabasePtr db = m_connection->getDatabasePtr();

    if (db.isNull())
    {
        return vec;
    }

    tango::IFileInfoEnumPtr files;
    files = db->getFolderInfo(towstr(m_path));
    int count = files->size();
    int i;

    if (files.isOk())
    {
        for (i = 0; i < count; ++i)
        {
            tango::IFileInfoPtr info = files->getItem(i);

            if (info->getType() == tango::filetypeFolder)
            {
                DatabaseFolderTreeItem* item = new DatabaseFolderTreeItem;
                item->setConnection(m_connection);
                item->setBitmap(GET_XPM(xpm_folder));
                item->setLabel(cfw::makeProper(towx(info->getName())));
                item->setPath(m_path + cfw::makeProper(towx(info->getName())) + wxT("/"));
                vec->append(static_cast<cfw::ITreeItem*>(item));
            }
        }

        for (i = 0; i < count; ++i)
        {
            tango::IFileInfoPtr info = files->getItem(i);

            if (info->getType() == tango::filetypeSet)
            {
                DatabaseObjectTreeItem* item = new DatabaseObjectTreeItem;
                item->setConnection(m_connection);
                item->setBitmap(GET_XPM(xpm_table));
                item->setLabel(cfw::makeProper(towx(info->getName())));
                item->setPath(m_path + cfw::makeProper(towx(info->getName())));
                vec->append(static_cast<cfw::ITreeItem*>(item));
            }
        }
    }

    return vec;
}




// -- DatabaseObjectTreeItem class implementation --


DatabaseObjectTreeItem::DatabaseObjectTreeItem()
{
    m_path = wxT("");
}

DatabaseObjectTreeItem::~DatabaseObjectTreeItem()
{

}

IConnectionPtr DatabaseObjectTreeItem::getConnection()
{
    return m_connection;
}

void DatabaseObjectTreeItem::setConnection(IConnectionPtr c)
{
    m_connection = c;
}

wxString DatabaseObjectTreeItem::getPath()
{
    return m_path;
}

void DatabaseObjectTreeItem::setPath(wxString s)
{
    m_path = cfw::makeProper(s);
}




