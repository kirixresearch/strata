#include "jobspch.h"
#include "kl/portable.h"
#include "xd/xd.h"
#include "itertemptable.h"

namespace jobs {

ConvertIterToTempTable::ConvertIterToTempTable()
{
}

ConvertIterToTempTable::~ConvertIterToTempTable()
{
    if (!m_temp_table_path.empty() && !m_db.isNull())
    {
        m_db->deleteFile(m_temp_table_path);
    }
}

bool ConvertIterToTempTable::execute(xd::IDatabasePtr db, xd::IIteratorPtr _iter, XdJobInfo* xd_job_info)
{
    IJobInfoPtr job_info = (IJobInfo*)xd_job_info;

    m_db = db;

    xd::IIteratorPtr iter = _iter.isOk() ? _iter->clone() : xcm::null;
    if (iter.isNull())
        return false;

    iter->goFirst();

    // Generate temp table name
    m_temp_table_path = xd::getTemporaryPath();

    xd::IJobPtr job = db->createJob();
    if (xd_job_info)
    {
        xd_job_info->setXdJob(job);
    }

    // Copy data to temp table
    xd::CopyParams info;
    info.iter_input = iter;
    info.output = m_temp_table_path;
    info.append = false;
    m_db->copyData(&info, job.p);

    if (job.isOk() && job->getCancelled())
    {
        return false;
    }

    return true;
}

const std::wstring& ConvertIterToTempTable::getTempTablePath() const
{
    return m_temp_table_path;
}

} // namespace jobs 