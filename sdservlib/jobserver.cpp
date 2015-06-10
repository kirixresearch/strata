/*!
 *
 * Copyright (c) 2015, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2015-05-26
 *
 */


#include "sdservlib.h"
#include "jobserver.h"
#include "controller.h"
#include <kl/url.h>
#include <kl/json.h>
#include <kl/file.h>
#include <kl/thread.h>
#include <cstdio>




class JobServerThread : public kl::thread
{
public:

    JobServerThread() : kl::thread()
    {
    }

    virtual ~JobServerThread()
    {
    }

    jobs::IJobPtr addJob(const std::wstring& job_type)
    {
        jobs::IJobPtr job = jobs::createJob(job_type);
        if (job.isOk())
            m_jobs.push_back(job);
        return job;
    }

    jobs::IJobInfoPtr getJobInfo()
    {
        if (m_jobs.size() == 1)
        {
            return m_jobs[0]->getJobInfo();
        }
         else
        {
            if (m_agg_jobinfo.isNull())
                m_agg_jobinfo = jobs::createAggregateJobInfoObject();
            m_agg_jobinfo->setCurrentJobIndex(0);
            m_agg_jobinfo->setJobCount(m_jobs.size());
            return m_agg_jobinfo;
        }
    }

    unsigned int entry()
    {
        std::vector<jobs::IJobPtr>::iterator it;

        size_t job_idx = 0;


		if (m_agg_jobinfo.isOk())
        {
			jobs::IJobInfoPtr j = m_agg_jobinfo;
            j->setState(jobs::jobStateRunning);
            j->setStartTime(time(NULL));
        }

        for (it = m_jobs.begin(); it != m_jobs.end(); ++it, ++job_idx)
        {
			jobs::IJobInfoPtr jobinfo = (*it)->getJobInfo();

            if (m_agg_jobinfo.isOk())
            {
                m_agg_jobinfo->setCurrentJobIndex(job_idx);
                m_agg_jobinfo->setJobCount(m_jobs.size());
				m_agg_jobinfo->setCurrentJobInfo(jobinfo);
            }

            jobinfo->setStartTime(time(NULL));
            jobinfo->setState(jobs::jobStateRunning);
            (*it)->runJob();
            (*it)->runPostJob();
            jobinfo->setState(jobs::jobStateFinished);
        }


		if (m_agg_jobinfo.isOk())
        {
			jobs::IJobInfoPtr j = m_agg_jobinfo;
            j->setState(jobs::jobStateFinished);
			j->setFinishTime(time(NULL));
        }

        return 0;
    }

public:

    std::vector<jobs::IJobPtr> m_jobs;
    jobs::IAggregateJobInfoPtr m_agg_jobinfo;
};



bool JobServer::run()
{
    std::wstring job_file = m_sdserv->getOption(L"sdserv.job_file");
    std::wstring json_params = xf_get_file_contents(job_file);

    if (json_params.empty())
        return false;

    kl::JsonNode template_node;
    template_node.fromString(json_params);

    kl::JsonNode metadata = template_node["metadata"];
    if (!metadata.isOk())
        return false;
    
    kl::JsonNode params = template_node["params"];
    if (!params.isOk())
        return false;

    std::wstring job_type = metadata["type"];


    JobServerThread* job_thread = new JobServerThread;
    jobs::IJobPtr job = job_thread->addJob(job_type);
    if (job.isNull())
    {
        delete job_thread;
        // returnApiError(req, "Invalid job type (2)");
        return false;
    }


    job->setParameters(params.toString());
    job->setDatabase(m_sdserv->m_database);
    jobs::IJobInfoPtr job_info = job_thread->getJobInfo();

    job_thread->create();


    int job_state;
    double percentage;
    double max_count;
    double cur_count;

    while (true)
    {
        kl::thread_sleep(1000);

        job_state = job_info->getState();
        if (job_state != jobs::jobStatePaused && job_state != jobs::jobStateRunning)
            break;
        if (job_state != jobs::jobStateRunning)
            continue;

        percentage = job_info->getPercentage();
        cur_count = job_info->getCurrentCount();
        max_count = job_info->getMaxCount();

        printf("running;%.0f;%.0f;%.0f\n", percentage, cur_count, max_count);
        fflush(stdout);
    }

    cur_count = job_info->getCurrentCount();
    max_count = job_info->getMaxCount();
    printf("finished;100;%.0f;%.0f\n",  cur_count, max_count);
    fflush(stdout);

    return true;
}
