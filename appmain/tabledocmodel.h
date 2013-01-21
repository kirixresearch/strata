/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-11-29
 *
 */


#ifndef __APP_TABLEDOCMODEL_H
#define __APP_TABLEDOCMODEL_H


#include "tabledoc.h"


class TableDocModel : public ITableDocModel
{
    XCM_CLASS_NAME("appmain.TableDocModel")
    XCM_BEGIN_INTERFACE_MAP(TableDocModel)
        XCM_INTERFACE_ENTRY(ITableDocModel)
    XCM_END_INTERFACE_MAP()


public:

    TableDocModel();
    virtual ~TableDocModel();

    void setId(const wxString& id);
    wxString getId();

public:
    xcm::signal1<TableDocModel*> sigDeleted;

private:

    bool writeObject(ITableDocObjectPtr obj);
    bool writeMultipleObjects(ITableDocObjectEnumPtr obj);
    bool deleteObject(ITableDocObjectPtr obj);
    tango::INodeValuePtr flushObject(ITableDocObjectPtr obj);

    ITableDocWatchPtr createWatchObject();
    ITableDocMarkPtr createMarkObject();
    ITableDocViewPtr createViewObject();

    ITableDocWatchEnumPtr getWatchEnum();
    ITableDocMarkEnumPtr getMarkEnum();
    ITableDocViewEnumPtr getViewEnum();
    
    wxColor getNextMarkColor();

private:

    xcm::mutex m_obj_mutex;
    wxString m_id;

    std::vector<ITableDocObjectPtr> m_watches;
    std::vector<ITableDocObjectPtr> m_marks;
    std::vector<ITableDocObjectPtr> m_views;

    int m_watches_cache_time;
    int m_marks_cache_time;
    int m_views_cache_time;
};



#endif

