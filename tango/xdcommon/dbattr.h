/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2004-03-05
 *
 */


#ifndef __XDCOMMON_DBATTR_H
#define __XDCOMMON_DBATTR_H


#include <map>


class DatabaseAttributes : public tango::IAttributes
{
    XCM_CLASS_NAME("tango.DatabaseAttributes")
    XCM_BEGIN_INTERFACE_MAP(DatabaseAttributes)
        XCM_INTERFACE_ENTRY(tango::IAttributes)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseAttributes();
    ~DatabaseAttributes();

    bool getBoolAttribute(int attr_id);
    void setBoolAttribute(int attr_id, bool value);

    int getIntAttribute(int attr_id);
    void setIntAttribute(int attr_id, int value);

    std::wstring getStringAttribute(int attr_id);
    void setStringAttribute(int attr_id, const std::wstring& value);

private:

    std::map<int, bool> m_bool_attrs;
    std::map<int, int> m_int_attrs;
    std::map<int, std::wstring> m_str_attrs;
};



#endif

