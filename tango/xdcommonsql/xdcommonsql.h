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



xcm_interface IXdsqlDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IXdsqlDatabase")

public:

    virtual IXdsqlTablePtr openTable(const std::wstring& path) = 0;
};



xcm_interface IXdsqlTable : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IXdsqlTable")

public:

    virtual tango::IIteratorPtr createIterator(const std::wstring& columns,
                                               const std::wstring& expr,
                                               tango::IJob* job) = 0;
    virtual tango::IStructurePtr getStructure() = 0;

    virtual bool updateRow(tango::rowid_t rowid,
                           tango::ColumnUpdateInfo* info,
                           size_t info_size) = 0;
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

    static SqlIterator* createSqlIterator(tango::IIteratorPtr iter,
                                          const std::wstring& condition,
                                          tango::IJob* job);
    
    SqlIterator() { }
    virtual ~SqlIterator() { }
    
    virtual tango::IIterator* getIterator() = 0;
    virtual void goFirst() = 0;
    virtual void goNext() = 0;
    virtual bool isDone() = 0;
    virtual bool isCancelled() = 0;
};





// -- SQL function calls --

void splitSQL(const std::wstring& sql,
           std::vector<std::wstring>& commands);


bool doSQL(tango::IDatabasePtr db,
           const std::wstring& sql,
           unsigned int flags,
           xcm::IObjectPtr& result_obj,
           ThreadErrorInfo& error,
           tango::IJob* job);

tango::IIteratorPtr sqlSelect(
           tango::IDatabasePtr db,
           const std::wstring& command,
           unsigned int flags,
           ThreadErrorInfo& error,
           tango::IJob* job);
           
bool sqlInsert(
           tango::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           tango::IJob* job);

bool sqlCreate(
           tango::IDatabasePtr db,
           const std::wstring& command,
           xcm::IObjectPtr& result,
           ThreadErrorInfo& error,
           tango::IJob* job);

bool sqlDelete(
           tango::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           tango::IJob* job);

bool sqlUpdate(
           tango::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           tango::IJob* job);

bool sqlDrop(
           tango::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           tango::IJob* job);
           
bool sqlMove(
           tango::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           tango::IJob* job);
           
bool sqlAlter(
           tango::IDatabasePtr db,
           const std::wstring& command,
           ThreadErrorInfo& error,
           tango::IJob* job);

std::wstring popToken(std::wstring& str);
std::wstring peekToken(const std::wstring& str);
std::wstring getTableNameFromSql(const std::wstring& sql);
std::vector<std::wstring> getAllTableNamesFromSql(const std::wstring& sql);


#endif

