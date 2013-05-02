/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-05
 *
 */


#ifndef __XDCOMMON_CONNECTIONSTR_H
#define __XDCOMMON_CONNECTIONSTR_H


#include <map>
#include <vector>
#include <kl/string.h>
#include <xcm/xcm.h>


namespace xdcommon
{


// this class can be used as an xcm object
// or as a regular stack object

class ConnectionStringParser : public tango::IConnectionStringParser
{
    XCM_CLASS_NAME("xdnative.ConnectionStringParser")
    XCM_BEGIN_INTERFACE_MAP(ConnectionStr)
        XCM_INTERFACE_ENTRY(tango::IConnectionStringParser)
    XCM_END_INTERFACE_MAP()

public:

    ConnectionStr();
    ConnectionStr(const std::wstring& str);

    void parse(const std::wstring& str);
    std::wstring getConnectionString();
    
    // returns the value of the parameter, in lowercase
    std::wstring getLowerValue(const std::wstring& param);
    
    // returns the value of the parameter
    std::wstring getValue(const std::wstring& param);
    
    // returns true if the value exists
    bool getValueExist(const std::wstring& param);
    
    // sets a value in the connection string
    void setValue(const std::wstring& param, const std::wstring& value);
    
private:

    std::map<std::wstring, std::wstring, kl::cmp_nocase> m_map;
    std::vector<std::wstring> m_keys;
};



std::wstring urlToConnectionStr(const std::wstring& url);
std::wstring decryptConnectionStringPassword(const std::wstring& connection_str);
std::wstring encryptConnectionStringPassword(const std::wstring& connection_str);


}; // namespace xdcommon


#endif



