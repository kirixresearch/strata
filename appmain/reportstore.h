/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2004-01-26
 *
 */


#ifndef __APP_REPORTSTORE_H
#define __APP_REPORTSTORE_H


#include "../kcanvas/kcanvas.h"
#include "../kcanvas/property.h"


class ReportValue : public kcanvas::IStoreValue
{
    XCM_CLASS_NAME("appmain.ReportValue")
    XCM_BEGIN_INTERFACE_MAP(ReportValue)
        XCM_INTERFACE_ENTRY(kcanvas::IStoreValue)
    XCM_END_INTERFACE_MAP()

public:

    ReportValue();
    virtual ~ReportValue();

    wxString getName();

    void setString(const wxString& value);
    wxString getString();

    void setDouble(double value);
    double getDouble();

    void setInteger(int value);
    int getInteger();

    void setBoolean(bool value);
    bool getBoolean();

    void setColor(const kcanvas::Color& color);
    kcanvas::Color getColor();

    unsigned int getChildCount();
    wxString getChildName(unsigned int idx);
    kcanvas::IStoreValuePtr getChildByIdx(unsigned int idx);
    kcanvas::IStoreValuePtr getChild(const wxString& name, bool create_if_not_exist);
    kcanvas::IStoreValuePtr createChild(const wxString& name);
    bool getChildExist(const wxString& name);
    bool deleteChild(const wxString& name);
    bool deleteAllChildren();

    void setNode(tango::INodeValuePtr node);
    tango::INodeValuePtr getNode();

private:

    tango::INodeValuePtr m_node;
};

class ReportStore : public kcanvas::IStore
{
    XCM_CLASS_NAME("appmain.ReportStore")
    XCM_BEGIN_INTERFACE_MAP(ReportStore)
        XCM_INTERFACE_ENTRY(kcanvas::IStore)
    XCM_END_INTERFACE_MAP()

public:

    ReportStore();
    virtual ~ReportStore();

    kcanvas::IStoreValuePtr createFile(const wxString& file);
    kcanvas::IStoreValuePtr loadFile(const wxString& file);

private:

    tango::INodeValuePtr m_file;
};


#endif

