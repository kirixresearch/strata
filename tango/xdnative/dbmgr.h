/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-10
 *
 */


#ifndef __XDNATIVE_DBMGR_H
#define __XDNATIVE_DBMGR_H


#include "tango.h"
#include "../xdcommon/errorinfo.h"


class DatabaseMgr : public tango::IDatabaseMgr,
                    public tango::IDatabaseParserBinder
{
    XCM_CLASS_NAME("xdnative.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(tango::IDatabaseMgr)
        XCM_INTERFACE_ENTRY(tango::IDatabaseParserBinder)
    XCM_END_INTERFACE_MAP()

public:

    tango::IDatabasePtr createDatabase(const std::wstring& location,
                                       const std::wstring& dbname);

    bool createDatabase(const std::wstring& location, int db_type);

    tango::IDatabasePtr open(const std::wstring& connection_str);

    tango::IDatabaseEntryEnumPtr getDatabaseList(const std::wstring& host,
                                                int port,
                                                const std::wstring& uid,
                                                const std::wstring& password);

    std::wstring getErrorString();
    int getErrorCode();
    
    
    // -- IDatabaseParserBinder --
    
    void bindExprParser(void* parser);
    
private:

    ThreadErrorInfo m_error;

};





#endif

