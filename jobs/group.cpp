/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-01-24
 *
 */


#include "jobspch.h"
#include "group.h"


namespace jobs
{


// GroupJob implementation

GroupJob::GroupJob() : XdJobBase(XdJobBase::useTangoCurrentCount)
{
}

GroupJob::~GroupJob()
{
}

bool GroupJob::isInputValid()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.group-data",
            "version" : 1,
            "description" : ""
        },
        "input" : <path>,
        "output" : <path>,
        "group" : <string>,
        "columns" : <string>,
        "where" : <string>,      // not required; default = ""
        "having" : <string>,     // not required; default = ""
        "unique" : <boolean>    // not required; default = false
    }
*/
    if (m_config.isNull())
        return false;

    // TODO: check job type and version

    if (!m_config.childExists("input"))
        return false;

    if (!m_config.childExists("output"))
        return false;

    if (!m_config.childExists("group"))
        return false;

    if (!m_config.childExists("columns"))
        return false;

    // TODO: check for file existence?  in general, how much
    // work should the validator do?

    return true;
}

int GroupJob::runJob()
{
    // make sure we have a valid input
    if (!isInputValid())
    {
        // TODO: correlate input validation with job error information
    
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }

    // make sure the database is valid
    if (m_db.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }    

    // get the input parameters
    std::wstring input_path = m_config["input"].getString();
    std::wstring output_path = m_config["output"].getString();
    std::wstring group_params = m_config["group"].getString();
    std::wstring column_params = m_config["columns"].getString();

    std::wstring where_params;
    if (m_config.childExists("where"))
        where_params = m_config["where"].getString();

    std::wstring having_params;
    if (m_config.childExists("having"))
        having_params = m_config["having"].getString();

    bool unique_records = false;
    if (m_config.childExists("unique"))
        unique_records = m_config["unique"].getBoolean();


    tango::IJobPtr tango_job;
    tango::ISetPtr output_set;
    tango::ISetPtr input_set = m_db->openSet(input_path);

    if (!unique_records)
    {
        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        output_set = m_db->runGroupQuery(input_set,
                                         group_params,
                                         column_params,
                                         where_params,
                                         having_params,
                                         tango_job.p);

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);

            // TODO: need to decide how to handle error strings; these need to 
            // be translated, so shouldn't be in this class
            //m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));

            return 0;
        }
    }
     else
    {
        // this whole section is specialized code to optimize the option
        // for unique records:  rather than selecting distinct first, then
        // doing a group operation, this first groups with the related
        // "where" and "having", then removes duplicates, then regroups
        // it's optimized for instances where "where" and "having" are
        // specified and the expected output size is small

        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        tango::ISetPtr intermediate_output_set1;
        intermediate_output_set1 = m_db->runGroupQuery(input_set,
                                                       group_params,
                                                       L"[DETAIL]",
                                                       where_params,
                                                       having_params,
                                                       tango_job.p);

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);

            // TODO: need to decide how to handle error strings; these need to 
            // be translated, so shouldn't be in this class
            //m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));

            return 0;
        }

        if (output_set.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }


        std::wstring sql = L"SELECT DISTINCT * FROM ";
        sql += intermediate_output_set1->getObjectPath();

        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        xcm::IObjectPtr result;
        m_db->execute(sql, 0, result, tango_job.p);

        tango::IIteratorPtr iter = result;

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (iter.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        tango::ISetPtr intermediate_output_set2 = iter->getSet();
        if (intermediate_output_set2.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }



        tango_job = m_db->createJob();
        setTangoJob(tango_job);

        output_set = m_db->runGroupQuery(intermediate_output_set2,
                                         group_params,
                                         column_params,
                                         where_params,
                                         having_params,
                                         tango_job.p);

        if (tango_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(jobStateFailed);

            // TODO: need to decide how to handle error strings; these need to 
            // be translated, so shouldn't be in this class
            //m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));

            return 0;
        }
    }


//    if (output_path.length() > 0)
//        m_db->storeObject(output_set, output_path);


    return 0;


/*
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
*/
}

void GroupJob::runPostJob()
{
}


};  // namespace jobs

