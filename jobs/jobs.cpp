/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Benjamin I. Williams
 * Created:  2012-11-27
 *
 */


#include "jobspch.h"
#include "jobinfo.h"
#include "append.h"

namespace jobs
{


IJobPtr createJob(const std::wstring job_class)
{
    if (job_class == L"application/vnd.kx.append-data")
        return static_cast<IJob*>(new AppendJob);
         else
        return xcm::null;
}


IJobInfoPtr createJobInfoObject()
{
    return static_cast<IJobInfo*>(new JobInfo);
}



};

