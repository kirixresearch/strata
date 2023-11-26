/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-11-29
 *
 */


#ifndef H_APP_TABLEDOCMODEL_H
#define H_APP_TABLEDOCMODEL_H


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

    bool fromJson(const std::wstring& json);
    std::wstring toJson();

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

    bool saveJson();
    bool loadJson();
    bool loadAndConvertOldVersionToNewJson();

private:

    kl::mutex m_obj_mutex;
    std::wstring m_id;
    bool m_convert_old_version;

    std::vector<ITableDocObjectPtr> m_marks;
    std::vector<ITableDocObjectPtr> m_views;
    std::vector<std::wstring> m_to_delete;
};


#endif
