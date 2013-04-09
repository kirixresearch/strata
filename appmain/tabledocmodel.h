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
    void init(const std::wstring& id);

    const std::wstring& getId();

    bool load();
    bool save();

    bool writeObject(ITableDocObjectPtr obj, bool save_to_store = true);
    bool writeMultipleObjects(ITableDocObjectEnumPtr obj);

    bool deleteObject(ITableDocObjectPtr obj);

public:
    xcm::signal1<TableDocModel*> sigDeleted;

private:

    ITableDocMarkPtr createMarkObject();
    ITableDocViewPtr createViewObject();

    ITableDocMarkEnumPtr getMarkEnum();
    ITableDocViewEnumPtr getViewEnum();
    
    wxColor getNextMarkColor();

    ITableDocObjectPtr lookupObject(const std::wstring& id);

    bool saveJson(const std::wstring& path);
    bool loadJson(const std::wstring& path);
    bool loadAndConvertOldVersionToNewJson(const std::wstring& id);

private:

    xcm::mutex m_obj_mutex;
    std::wstring m_id;

    std::vector<ITableDocObjectPtr> m_marks;
    std::vector<ITableDocObjectPtr> m_views;
    std::vector<std::wstring> m_to_delete;
};


#endif
