/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-10
 *
 */


#ifndef H_XDNATIVE_DBMGR_H
#define H_XDNATIVE_DBMGR_H


#include <xd/xd.h>
#include "../xdcommon/errorinfo.h"


class DatabaseMgr : public xd::IDatabaseMgr,
                    public xd::IDatabaseParserBinder
{
    XCM_CLASS_NAME("xdnative.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(xd::IDatabaseMgr)
        XCM_INTERFACE_ENTRY(xd::IDatabaseParserBinder)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseMgr()
    {
    }

    virtual ~DatabaseMgr()
    {
    }

    bool createDatabase(const std::wstring& connection_str);

    xd::IDatabasePtr open(const std::wstring& connection_str);

    xd::DatabaseEntryEnum getDatabaseList(const std::wstring& host,
                                          int port,
                                          const std::wstring& uid,
                                          const std::wstring& password);

    std::wstring getErrorString();
    int getErrorCode();
    
    
    // IDatabaseParserBinder
    
    void bindExprParser(void* parser);
    
private:

    ThreadErrorInfo m_error;

};





#endif

