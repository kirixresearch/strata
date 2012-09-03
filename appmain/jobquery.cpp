/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-30
 *
 */


#include "appmain.h"
#include "jobquery.h"
#include "appcontroller.h"


wxString writeQueryFromFilterSort(tango::IDatabasePtr db,
                                  const wxString& table_name,
                                  const wxString& filter,
                                  const wxString& sort)
{
    wxString query = wxT("SELECT * FROM ");
    query += quoteIdentifier(db, table_name);

    if (filter.Length() > 0)
    {
        query += wxT(" WHERE ");
        query += filter;
    }
    
    if (sort.Length() > 0)
    {
        query += wxT(" ORDER BY ");
        query += sort;
    }
    
    return query;
}






// -- QueryJob implementation --

QueryJob::QueryJob()
{
    m_flags = 0;
}

QueryJob::~QueryJob()
{
}

void QueryJob::setDatabase(tango::IDatabasePtr db)
{
    m_database = db;
}

void QueryJob::setQuery(const wxString& query, unsigned int flags)
{
    m_query = query;
    m_flags = flags;
}

void QueryJob::setQueryParts(tango::ISetPtr set,
                   const wxString& columns,
                   const wxString& filter,
                   const wxString& sort)
{
}
                   
int QueryJob::runJob()
{
    // if a database to use is specified, use it; otherwise use
    // the global database
    tango::IDatabasePtr db;

    if (m_database.isOk())
        db = m_database;
          else
        db = g_app->getDatabase();

    if (db.isNull())
    {
        m_job_info->setState(cfw::jobStateFailed);
        return 0;
    }
    
    tango::IJobPtr tango_job = g_app->getDatabase()->createJob();
    setTangoJob(tango_job);

    m_obj_mutex.lock();


    
    m_result_iter.clear();
    m_result_set.clear();
    
    
    xcm::IObjectPtr result;
    if (db->execute(towstr(m_query), m_flags, result, tango_job))
    {
        // command succeeded;  check if there's an iterator available
        m_result_iter = result;

        if (m_result_iter.isOk())
            m_result_set = m_result_iter->getSet();
    }
     else
    {
        // command failed
        
        if (tango_job->getStatus() != tango::jobCancelled)
        {
            m_job_info->setState(cfw::jobStateFailed);
            m_job_info->setError(db->getErrorCode(), db->getErrorString());
        }
    }
    
    
    m_obj_mutex.unlock();

    return 0;
}

void QueryJob::runPostJob()
{
}


tango::ISetPtr QueryJob::getResultSet()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_result_set;
}


tango::IIteratorPtr QueryJob::getResultIterator()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_result_iter;
}




// -- SortFilterJob implementation --

SortFilterJob::SortFilterJob()
{
    m_job_info->setTitle(_("Query"));
}

SortFilterJob::~SortFilterJob()
{
}

void SortFilterJob::setInstructions(tango::ISetPtr source_set,
                                    const wxString& condition,
                                    const wxString& order)
{
    m_source_set = source_set;
    m_condition = condition;
    m_order = order;
}

wxString SortFilterJob::getCondition()
{
    return m_condition;
}

wxString SortFilterJob::getOrder()
{
    return m_order;
}

tango::ISetPtr SortFilterJob::getResultSet()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_result_set;
}

tango::IIteratorPtr SortFilterJob::getResultIterator()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_result_iter;
}

int SortFilterJob::runJob()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        m_job_info->setState(cfw::jobStateFailed);
        return 0;
    }
            
    wxString table_name = towx(m_source_set->getObjectPath());
    wxString query = writeQueryFromFilterSort(db, table_name, m_condition, m_order);
    
    tango::IJobPtr tango_job = db->createJob();
    setTangoJob(tango_job);


    xcm::IObjectPtr result;
    db->execute(towstr(query), tango::sqlPassThrough, result, tango_job);
    tango::IIteratorPtr result_iter = result;

    if (result_iter.isOk())
    {
        m_obj_mutex.lock();
        m_result_iter = result_iter;
        m_result_set = m_result_iter->getSet();
        m_obj_mutex.unlock();
    }
     else
    {
        m_obj_mutex.lock();
        m_result_iter.clear();
        m_result_set.clear();
        m_obj_mutex.unlock();

        if (tango_job->getStatus() != tango::jobCancelled)
        {
            m_job_info->setState(cfw::jobStateFailed);
        }
    }

    return 0;
}

void SortFilterJob::runPostJob()
{
}






