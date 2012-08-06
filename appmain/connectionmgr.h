/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-04-27
 *
 */


#ifndef __APP_CONNECTIONMGR_H
#define __APP_CONNECTIONMGR_H


// -- NOTE: these type numbers may NOT be changed.  You
//    may, however, add and remove items from the list.  Just
//    don't change the numbers around. --

enum
{
    dbtypeUndefined = 0x01,
    dbtypeXdnative = 0x02,
    dbtypePackage = 0x04,
    dbtypeAccess = 0x08,
    dbtypeExcel = 0x10,
    dbtypeSqlServer = 0x20,
    dbtypeMySql = 0x40,
    dbtypeOracle = 0x80,
    dbtypeOdbc = 0x100,
    dbtypeFilesystem = 0x200,
    dbtypeXbase = 0x400,
    dbtypeDelimitedText = 0x800,
    dbtypeDb2 = 0x1000,
    dbtypeFixedLengthText = 0x2000,
    dbtypeSqlite = 0x4000,
    dbtypePostgres = 0x8000,
    dbtypeClient = 0x100000,
    dbtypeAll = 0x0fffffff
};


xcm_interface IConnection : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IConnection")

public:

    virtual bool open() = 0;
    virtual bool isOpen() = 0;

    virtual int getType() = 0;
    virtual void setType(int type) = 0;

    virtual wxString getDescription() = 0;
    virtual void setDescription(const wxString& new_val) = 0;

    virtual wxString getErrorString() = 0;
    
    // -- used for local connections --

    virtual wxString getPath() = 0;
    virtual void setPath(const wxString& path) = 0;

    virtual wxString getFilter() = 0;
    virtual void setFilter(const wxString& filter) = 0;

    // -- used for remote connections --

    virtual wxString getHost() = 0;
    virtual void setHost(const wxString& new_val) = 0;

    virtual int getPort() = 0;
    virtual void setPort(int new_val) = 0;

    virtual wxString getDatabase() = 0;
    virtual void setDatabase(const wxString& new_val) = 0;

    virtual wxString getUsername() = 0;
    virtual void setUsername(const wxString& new_val) = 0;

    virtual wxString getPassword() = 0;
    virtual void setPassword(const wxString& new_val) = 0;

    virtual tango::IDatabasePtr getDatabasePtr() = 0;
    virtual void setDatabasePtr(tango::IDatabasePtr new_val) = 0;
};

XCM_DECLARE_SMARTPTR(IConnection)



IConnectionPtr createUnmanagedConnection();



#endif



