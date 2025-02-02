#ifndef JOBS_ITERTEMPTABLE_H
#define JOBS_ITERTEMPTABLE_H


namespace jobs
{

class XdJobInfo;

class ConvertIterToTempTable
{
public:
    ConvertIterToTempTable();
    ~ConvertIterToTempTable();

    bool execute(xd::IDatabasePtr db, xd::IIteratorPtr _iter, XdJobInfo* job_info);
    const std::wstring& getTempTablePath() const;

private:
    xd::IDatabasePtr m_db;
    std::wstring m_temp_table_path;
};

} // namespace jobs

#endif // JOBS_ITERTEMPTABLE_H 