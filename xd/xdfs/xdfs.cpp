/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <xd/xd.h>
#include "xdfs.h"
#include "database.h"
#include "../xdcommon/connectionstr.h"


/*
extern "C"
{

// test functions for NativeCall bindings
// they can be removed

int testFunc1()
{
    printf("testFunc1()\n");
    return 123;
}

int testFunc2(int val)
{
    printf("testFunc2(%d)\n", val);
    fflush(stdout);
    return 567;
}

void testFunc3(double dbl)
{
    printf("testFunc3(%.2f)\n", dbl);
}


};
*/


class DatabaseMgr : public xd::IDatabaseMgr
{
    XCM_CLASS_NAME("xdfs.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(xd::IDatabaseMgr)
    XCM_END_INTERFACE_MAP()

public:

    
    bool createDatabase(const std::wstring& connection_str)
    {
        return false;
    }
    
    xd::IDatabasePtr open(const std::wstring& connection_str)
    {
        xd::ConnectionStringParser c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return xcm::null;
        
        // check if the provider refers to us, or a different dll/shared lib

        if (provider != L"xdfs")
            return xcm::null;
        

        //  parse the connection string
        
        std::wstring location = c.getValue(L"database");

        FsDatabase* db = new FsDatabase;
        db->ref();

        if (!db->open(location))
        {
            db->unref();
            return xcm::null;
        }

        return xd::IDatabasePtr(db, false);
    }
    

    xd::IDatabaseEntryEnumPtr getDatabaseList(const std::wstring& host, int port,
                                                 const std::wstring& uid,
                                                 const std::wstring& password)
    {
        xcm::IVectorImpl<xd::IDatabaseEntryPtr>* vec = new xcm::IVectorImpl<xd::IDatabaseEntryPtr>;
        return vec;
    }

    std::wstring getErrorString()
    {
        return L"";
    }

    int getErrorCode()
    {
        return 0;
    }

};




// -- these are the publicly creatable classes --

XCM_BEGIN_DYNAMIC_MODULE(xdfs)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
        XCM_CLASS_ENTRY(FsDatabase)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdfs)
