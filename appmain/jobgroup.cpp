/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-30
 *
 */


#include "appmain.h"
#include "jobgroup.h"
#include "appcontroller.h"
#include "tabledoc.h"


// -- GroupJob implementation --

GroupJob::GroupJob()
{
    m_job_info->setTitle(towstr(_("Grouping Records")));
    m_unique_records_only = false;
}

GroupJob::~GroupJob()
{
}


void GroupJob::setInstructions(tango::ISetPtr source_set,
                               const wxString& group,
                               const wxString& columns,
                               const wxString& where,
                               const wxString& having)
{
    m_source_set = source_set;
    m_group = group;
    m_columns = columns;
    m_where = where;
    m_having = having;
}

void GroupJob::setUniqueRecordsOnly(bool val)
{
    m_unique_records_only = val;
}

void GroupJob::setOutputPath(const wxString& output_path)
{
    m_output_path = output_path;
}

wxString GroupJob::getOutputPath()
{
    return m_output_path;
}


tango::ISetPtr GroupJob::getResultSet()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_result_set;
}

tango::IIteratorPtr GroupJob::getResultIterator()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    return m_result_iter;
}

int GroupJob::runJob()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    tango::IJobPtr tango_job;

    // set the job title
    wxString path = towx(m_source_set->getObjectPath());
    path = path.AfterLast(wxT('/'));
    m_job_info->setTitle(towstr(wxString::Format(_("Grouping '%s'"), path.c_str())));

    if (!m_unique_records_only)
    {
        tango_job = g_app->getDatabase()->createJob();
        setTangoJob(tango_job);

        m_result_set = db->runGroupQuery(m_source_set,
                                         towstr(m_group),
                                         towstr(m_columns),
                                         towstr(m_where),
                                         towstr(m_having),
                                         tango_job.p);

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }
    }
     else
    {
        // -- the following is special code for the dup pay wizard --
        wxString columns = m_columns;
        if (m_unique_records_only)
        {
            columns = wxT("[DETAIL]");
        }

        tango_job = g_app->getDatabase()->createJob();
        setTangoJob(tango_job);

        m_result_set = db->runGroupQuery(m_source_set,
                                         towstr(m_group),
                                         towstr(columns),
                                         towstr(m_where),
                                         towstr(m_having),
                                         tango_job.p);

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));
            return 0;
        }

        // -- the following is special code for the dup pay wizard --
        if (m_result_set.isNull())
        {
            m_job_info->setState(jobStateFailed);
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }



        wxString sql = wxT("SELECT DISTINCT * FROM ");

        sql += towx(m_result_set->getObjectPath());

        tango_job = g_app->getDatabase()->createJob();
        setTangoJob(tango_job);

        xcm::IObjectPtr result;
        db->execute(towstr(sql), 0, result, tango_job.p);
        
        tango::IIteratorPtr iter = result;

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }

        if (iter.isNull())
        {
            m_job_info->setState(jobStateFailed);
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }

        tango::ISetPtr set = iter->getSet();
        if (set.isNull())
        {
            m_job_info->setState(jobStateFailed);
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }



        tango_job = g_app->getDatabase()->createJob();
        setTangoJob(tango_job);

        m_result_set = db->runGroupQuery(set,
                                         towstr(m_group),
                                         towstr(m_columns),
                                         towstr(m_where),
                                         towstr(m_having),
                                         tango_job.p);

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);
            m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));
            m_result_set.clear();
            m_result_iter.clear();
            return 0;
        }

    }


    return 0;
}

void GroupJob::runPostJob()
{
    if (m_job_info->getState() == jobStateCancelled ||
        m_job_info->getState() == jobStateFailed)
    {
        m_source_set = xcm::null;
        m_result_set = xcm::null;
        m_result_iter = xcm::null;
        return;
    }

    tango::IDatabasePtr database = g_app->getDatabase();

    if (database.isOk() && m_output_path.length() > 0)
    {
        database->storeObject(m_result_set, towstr(m_output_path));
    }
}


