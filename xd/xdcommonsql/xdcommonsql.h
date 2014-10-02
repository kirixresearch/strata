/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-26
 *
 */


#ifndef __XDCOMMON_SQLCOMMON_H
#define __XDCOMMON_SQLCOMMON_H


#include <map>
#include <set>

class ThreadErrorInfo;


xcm_interface IXdsqlDatabase;
XCM_DECLARE_SMARTPTR(IXdsqlDatabase)

xcm_interface IXdsqlTable;
XCM_DECLARE_SMARTPTR(IXdsqlTable)

xcm_interface IXdsqlRowDeleter;
XCM_DECLARE_SMARTPTR(IXdsqlRowDeleter)


xcm_interface IXdsqlDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IXdsqlDatabase")

public:

    virtual IXdsqlTablePtr openTable(const std::wstring& path) = 0;
};


xcm_interface IXdsqlRowDeleter : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IXdsqlRowDeleter")

public:

    virtual void startDelete() = 0;
    virtual bool deleteRow(const xd::rowid_t& rowid) = 0;
    virtual void finishDelete() = 0;
    virtual void cancelDelete() = 0;
};

xcm_interface IXdsqlTable : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IXdsqlTable")

public:

    virtual xd::IIteratorPtr createIterator(const std::wstring& columns,
                                            const std::wstring& order,
                                            xd::IJob* job) = 0;
    virtual xd::Structure getStructure() = 0;

    virtual IXdsqlRowDeleterPtr getRowDeleter() = 0;
    virtual bool restoreDeleted() = 0;

    virtual bool updateRow(xd::rowid_t rowid,
                           xd::ColumnUpdateInfo* info,
                           size_t info_size) = 0;

    virtual bool modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job) = 0;
};








class SqlStatement
{
public:

    SqlStatement(const std::wstring& stmt);
    ~SqlStatement();

    void addKeyword(const std::wstring& keyword);
    bool getKeywordExists(const std::wstring& keyword);
    std::vector<std::wstring> getKeywordParams(const std::wstring& keyword);

    // these functions return the parameter (or position) of the first
    // instance of the keyword; subsequent instances are ignored
    std::wstring getKeywordParam(const std::wstring& keyword);
    size_t getKeywordPosition(const std::wstring& keyword);

private:

    wchar_t* m_stmt;
    std::map<std::wstring, std::vector<wchar_t*> > m_keywords;
    std::set<wchar_t*> m_stops;
};



class SqlIterator
{
public:

    static SqlIterator* createSqlIterator(xd::IIteratorPtr iter,
                                          const std::wstring& condition,
                                          xd::IJob* job);
    
    SqlIterator() { }
    virtual ~SqlIterator() { }
    
    virtual xd::IIterator* getIterator() = 0;
    virtual void goFirst() = 0;
    virtual void goNext() = 0;
    virtual bool isDone() = 0;
    virtual bool isCancelled() = 0;
};





// -- SQL function calls --

void splitSQL(const std::wstring& sql,
           std::vector<std::wstring>& commands);


bool doSQL(xd::IDatabasePtr db,
           const std::wstring& sql,
           unsigned int flags,
           xcm::IObjectPtr& result_obj,
           ThreadErrorInfo& error,
           xd::IJob* job);

xd::IIteratorPtr sqlSelect(
           xd::IDatabasePtr db,
           const std::wstring& command,
           unsigned int flags,
           ThreadErrorInfo& error,
           xd::IJob* job);
           
bool sqlInsert(
           xd::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           xd::IJob* job);

bool sqlCreate(
           xd::IDatabasePtr db,
           const std::wstring& command,
           xcm::IObjectPtr& result,
           ThreadErrorInfo& error,
           xd::IJob* job);

bool sqlDelete(
           xd::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           xd::IJob* job);

bool sqlUpdate(
           xd::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           xd::IJob* job);

bool sqlDrop(
           xd::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           xd::IJob* job);
           
bool sqlMove(
           xd::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           xd::IJob* job);
           
bool sqlAlter(
           xd::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           xd::IJob* job);

std::wstring popToken(std::wstring& str);
std::wstring peekToken(const std::wstring& str);
std::wstring getTableNameFromSql(const std::wstring& sql);
std::vector<std::wstring> getAllTableNamesFromSql(const std::wstring& sql);


#endif

