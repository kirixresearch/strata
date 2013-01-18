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
#include "divide.h"
#include "index.h"

namespace jobs
{


IJobPtr createJob(const std::wstring job_class)
{
    if (job_class == L"application/vnd.kx.append-data")
        return static_cast<IJob*>(new AppendJob);
    if (job_class == L"application/vnd.kx.divide-data")
        return static_cast<IJob*>(new DivideJob);
    if (job_class == L"application/vnd.kx.index-data")
        return static_cast<IJob*>(new IndexJob);        

        return xcm::null;        
}


IJobInfoPtr createJobInfoObject()
{
    return static_cast<IJobInfo*>(new JobInfo);
}



};

