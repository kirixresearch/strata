/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#include <kl/klib.h>
#include <xd/xd.h>
#include "database.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/errorinfo.h"

#define XLNT_STATIC
#include <xlnt/xlnt.hpp>

class DatabaseMgr : public xd::IDatabaseMgr
{
    XCM_CLASS_NAME("xdexcel.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(xd::IDatabaseMgr)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseMgr()
    {
    }

    virtual ~DatabaseMgr()
    {
    }

    xd::IDatabasePtr open(const std::wstring& connection_str)
    {
        // parse the connection string
        xd::ConnectionString c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return xcm::null;
        
        // check if the provider refers to us, or a different dll/shared lib
        if (provider != L"xdexcel")
        {
            return xcm::null;
        }
        
 
        std::wstring location = c.getValue(L"database");

        if (c.getValue(L"create_if_not_exists") == L"true")
        {
            if (!xf_get_file_exist(location))
            {
                if (!this->createDatabase(connection_str))
                    return xcm::null;
            }
        }



        ExcelDatabase* db = new ExcelDatabase;
        db->ref();
        if (!db->open(location))
        {
            m_error.setError(db->getErrorCode(), db->getErrorString());

            db->unref();
            return xcm::null;
        }

        return xd::IDatabasePtr(db, false);
    }
    
    bool createDatabase(const std::wstring& connection_str)
    {
        xd::ConnectionString c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return false;
        
        // check if the provider refers to us, or a different dll/shared lib
        if (provider != L"xdexcel")
            return false;

        std::wstring location = c.getValue(L"database");


        xlnt::workbook wb;
        wb.save(location);

        //PkgFile file;
        //if (!file.create(location))
        //    return false;
        //file.close();

        return true;
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


private:

    ThreadErrorInfo m_error;

};




// -- these are the publicly creatable classes --

XCM_BEGIN_DYNAMIC_MODULE(xdexcel)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdexcel)


