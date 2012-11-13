/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#ifndef __APP_JOBQUERY_H
#define __APP_JOBQUERY_H


#include "tangojobbase.h"


wxString writeQueryFromFilterSort(tango::IDatabasePtr db,
                                  const wxString& table_name,
                                  const wxString& sort,
                                  const wxString& filter);
                                  


xcm_interface IQueryJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IQueryJob")

public:

    virtual tango::ISetPtr getResultSet() = 0;
    virtual tango::IIteratorPtr getResultIterator() = 0;
};


XCM_DECLARE_SMARTPTR(IQueryJob)



class QueryJob : public TangoJobBase,
                 public IQueryJob
{
    XCM_CLASS_NAME("appmain.QueryJob")
    XCM_BEGIN_INTERFACE_MAP(QueryJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
        XCM_INTERFACE_ENTRY(IQueryJob)
    XCM_END_INTERFACE_MAP()

public:

    QueryJob();
    virtual ~QueryJob();

    void setDatabase(tango::IDatabasePtr db);
    void setQuery(const wxString& query, unsigned int flags);

    void setQueryParts(tango::ISetPtr set,
                       const wxString& columns,
                       const wxString& filter,
                       const wxString& sort);

    tango::ISetPtr getResultSet();
    tango::IIteratorPtr getResultIterator();

    int runJob();
    void runPostJob();

private:

    xcm::mutex m_obj_mutex;
    wxString m_query;
    unsigned int m_flags;

    tango::IDatabasePtr m_database;
    tango::ISetPtr m_result_set;
    tango::IIteratorPtr m_result_iter;
};








xcm_interface ISortFilterJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ISortFilterJob")

public:

    virtual wxString getCondition() = 0;
    virtual wxString getOrder() = 0;
    virtual tango::ISetPtr getResultSet() = 0;
    virtual tango::IIteratorPtr getResultIterator() = 0;
};

XCM_DECLARE_SMARTPTR(ISortFilterJob)


class SortFilterJob : public TangoJobBase,
                      public ISortFilterJob
{
    XCM_CLASS_NAME("appmain.SortFilterJob")
    XCM_BEGIN_INTERFACE_MAP(SortFilterJob)
        XCM_INTERFACE_ENTRY(ISortFilterJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    SortFilterJob();
    virtual ~SortFilterJob();

    void setInstructions(tango::ISetPtr source_set,
                         const wxString& condition,
                         const wxString& order);

    wxString getCondition();
    wxString getOrder();
    tango::ISetPtr getResultSet();
    tango::IIteratorPtr getResultIterator();

    int runJob();
    void runPostJob();

private:

    xcm::mutex m_obj_mutex;

    wxString m_condition;
    wxString m_order;
    tango::ISetPtr m_source_set;

    tango::ISetPtr m_result_set;
    tango::IIteratorPtr m_result_iter;
};







#endif

