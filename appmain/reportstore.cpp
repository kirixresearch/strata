/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2004-01-26
 *
 */


#include "appmain.h"
#include "reportstore.h"
#include "../kcanvas/graphicsobj.h"


ReportValue::ReportValue()
{
}

ReportValue::~ReportValue()
{
}

wxString ReportValue::getName()
{
    return towx(m_node->getName());
}

void ReportValue::setString(const wxString& value)
{
    m_node->setString(towstr(value));
}

wxString ReportValue::getString()
{
    return towx(m_node->getString());
}

void ReportValue::setDouble(double value)
{
    m_node->setDouble(value);
}

double ReportValue::getDouble()
{
    return m_node->getDouble();
}

void ReportValue::setInteger(int value)
{
    m_node->setInteger(value);
}

int ReportValue::getInteger()
{
    return m_node->getInteger();
}

void ReportValue::setBoolean(bool value)
{
    m_node->setBoolean(value);
}

bool ReportValue::getBoolean()
{
    return m_node->getBoolean();
}

void ReportValue::setColor(const kcanvas::Color& color)
{
    unsigned int val = (color.Red() << 16)  | 
                       (color.Green() << 8) | 
                        color.Blue();

    wxString temps;
    temps.Printf(wxT("%u"), val);
    m_node->setString(towstr(temps));
}

kcanvas::Color ReportValue::getColor()
{
    unsigned int val = kl::wtoi(m_node->getString());

    int b = (val & 0xff);
    int g = (val >> 8) & 0xff;
    int r = (val >> 16) & 0xff;

    return kcanvas::Color(r,g,b);
}

unsigned int ReportValue::getChildCount()
{
    return m_node->getChildCount();
}

wxString ReportValue::getChildName(unsigned int idx)
{
    return towx(m_node->getChildName(idx));
}

kcanvas::IStoreValuePtr ReportValue::getChildByIdx(unsigned int idx)
{
    tango::INodeValuePtr child;
    child = m_node->getChildByIdx(idx);

    if (child.isNull())
        return xcm::null;

    ReportValue* value = new ReportValue;
    value->setNode(child);

    return static_cast<kcanvas::IStoreValue*>(value);
}

kcanvas::IStoreValuePtr ReportValue::getChild(const wxString& name, bool create_if_not_exist)
{
    tango::INodeValuePtr child;
    child = m_node->getChild(towstr(name), create_if_not_exist);

    if (child.isNull())
        return xcm::null;

    ReportValue* value = new ReportValue;
    value->setNode(child);

    return static_cast<kcanvas::IStoreValue*>(value);
}

kcanvas::IStoreValuePtr ReportValue::createChild(const wxString& name)
{
    tango::INodeValuePtr child;
    child = m_node->createChild(towstr(name));

    if (child.isNull())
        return xcm::null;

    ReportValue* value = new ReportValue;
    value->setNode(child);

    return static_cast<kcanvas::IStoreValue*>(value);
}

bool ReportValue::getChildExist(const wxString& name)
{
    return m_node->getChildExist(towstr(name));
}

bool ReportValue::deleteChild(const wxString& name)
{
    return m_node->deleteChild(towstr(name));
}

bool ReportValue::deleteAllChildren()
{
    return m_node->deleteAllChildren();
}

void ReportValue::setNode(tango::INodeValuePtr node)
{
    m_node = node;
}

tango::INodeValuePtr ReportValue::getNode()
{
    return m_node;
}



ReportStore::ReportStore()
{
}

ReportStore::~ReportStore()
{
}

kcanvas::IStoreValuePtr ReportStore::createFile(const wxString& path)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return xcm::null;
        
    m_file = db->createNodeFile(towstr(path));
    
    if (m_file.isNull())
        return xcm::null;

    // -- set up report root --
    tango::INodeValuePtr kpp_report = m_file->createChild(L"kpp_report");
    
    tango::INodeValuePtr report_type = kpp_report->createChild(L"type");
    report_type->setString(L"report");

    // report_version: version 2 for v2005.1;
    //                 version 3 for v4.0 beta period
    //                 version 4 for v4.0 release
    tango::INodeValuePtr report_version = kpp_report->createChild(L"version");
    report_version->setInteger(4);

    tango::INodeValuePtr data = kpp_report->createChild(L"data");

    if (data.isNull())
        return xcm::null;

    ReportValue* value = new ReportValue;
    value->setNode(data);

    return static_cast<kcanvas::IStoreValue*>(value);
}

kcanvas::IStoreValuePtr ReportStore::loadFile(const wxString& path)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return xcm::null;
        
    m_file = db->openNodeFile(towstr(path));

    if (m_file.isNull())
        return xcm::null;

    // -- verify report format --
    tango::INodeValuePtr kpp_report = m_file->getChild(L"kpp_report", false);
    if (!kpp_report)
        return xcm::null;

    tango::INodeValuePtr report_type = kpp_report->getChild(L"type", false);
    if (!report_type)
        return xcm::null;

    if (report_type->getString() != L"report")
        return xcm::null;

    tango::INodeValuePtr report_version = kpp_report->getChild(L"version", false);
    if (!report_version)
        return xcm::null;

    // report_version: version 2 for v2005.1;
    //                 version 3 for v4.0 beta period
    //                 version 4 for v4.0 release
    if (report_version->getInteger() != 4 &&
        report_version->getInteger() != 3)  // allow reports created in beta; however, save them as version 4
        return xcm::null;

    tango::INodeValuePtr data = kpp_report->getChild(L"data", false);
    if (data.isNull())
        return xcm::null;

    ReportValue* value = new ReportValue;
    value->setNode(data);

    return static_cast<kcanvas::IStoreValue*>(value);
}
