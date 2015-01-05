/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#include "xdfs.h"
#include "database.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/formatdefinition.h"


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
        xd::ConnectionString c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return xcm::null;
        
        // check if the provider refers to us, or a different dll/shared lib

        if (provider != L"xdfs")
            return false;

        //  parse the connection string
        std::wstring location = c.getValue(L"database");
        if (location.empty())
            return false;

        // if directory already exists, use it with its contents
        if (xf_get_directory_exist(location))
            return true;

        return xf_mkdir(location);
    }
    
    xd::IDatabasePtr open(const std::wstring& connection_str)
    {
        xd::ConnectionString c(connection_str);
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
    

    xd::DatabaseEntryEnum getDatabaseList(const std::wstring& host,
                                          int port,
                                          const std::wstring& uid,
                                          const std::wstring& password)
    {
        return xd::DatabaseEntryEnum();
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





class XdUtil : public xd::IXdUtil
{
    XCM_CLASS_NAME("xdfs.XdUtil")
    XCM_BEGIN_INTERFACE_MAP(XdUtil)
        XCM_INTERFACE_ENTRY(xd::IXdUtil)
    XCM_END_INTERFACE_MAP()

public:

    std::wstring saveDefinitionToString(const xd::FormatDefinition& def) { return ::saveDefinitionToString(def); }
    bool loadDefinitionFromString(const std::wstring& str, xd::FormatDefinition* def) { return ::loadDefinitionFromString(str, def); }

};



// -- these are the publicly creatable classes --

XCM_BEGIN_DYNAMIC_MODULE(xdfs)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
        XCM_CLASS_ENTRY(FsDatabase)
        XCM_CLASS_ENTRY(XdUtil)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdfs)

