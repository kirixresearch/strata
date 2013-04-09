/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2004-01-06
 *
 */


#include "appmain.h"
#include "jobexport.h"
#include "appcontroller.h"
#include <set>


ExportJob::ExportJob()
{
    m_job_info->setTitle(L"");

    m_export_type = dbtypeUndefined;
    m_fix_invalid_fieldnames = false;

    m_filename = L"";
    m_overwrite_file = true;
    
    m_host = L"";
    m_database = L"";
    m_username = L"";
    m_password = L"";
    m_port = 0;
    
    m_delimiters = L",";
    m_text_qualifier = L"\"";
    m_first_row_header = true;
}

ExportJob::~ExportJob()
{

}

void ExportJob::setExportType(int type)
{
    m_export_type = type;
}

void ExportJob::setFixInvalidFieldnames(bool val)
{
    m_fix_invalid_fieldnames = val;
}

void ExportJob::setFilename(const std::wstring& filename, bool overwrite)
{
    m_filename = filename;
    m_overwrite_file = overwrite;
}

void ExportJob::setConnectionInfo(const std::wstring& host,
                                  int port,
                                  const std::wstring& database,
                                  const std::wstring& username,
                                  const std::wstring& password)
{
    m_host = host;
    m_port = port;
    m_database = database;
    m_username = username;
    m_password = password;
}

void ExportJob::addExportSet(const ExportJobInfo& info)
{
    m_exports.push_back(info);
}

void ExportJob::setDelimiters(const std::wstring& delimiters)
{
    m_delimiters = delimiters;
}

void ExportJob::setTextQualifier(const std::wstring& text_qualifier)
{
    m_text_qualifier = text_qualifier;
}

void ExportJob::setFirstRowHeader(bool first_row_header)
{
    m_first_row_header = first_row_header;
}

static bool isValidExportType(int export_type)
{
    switch (export_type)
    {
        case dbtypeAccess:
        case dbtypeExcel:
        case dbtypeSqlServer:
        case dbtypeMySql:
        case dbtypeOracle:
        case dbtypeDb2:
        case dbtypeOdbc:
        case dbtypeXbase:
        case dbtypeDelimitedText:
        case dbtypeFixedLengthText:
            return true;
    }

    return false;
}

static std::wstring getDefaultExportTitle(int export_type)
{
/*
    switch (export_type)
    {
        case dbtypeAccess:
            return _("Exporting to Microsoft Access File");
        case dbtypeExcel:
            return _("Exporting to Microsoft Excel File");
        case dbtypeSqlServer:
            return _("Exporting to SQL Server");
        case dbtypeMySql:
            return _("Exporting to MySQL");
        case dbtypeOracle:
            return _("Exporting to Oracle");
        case dbtypeOdbc:
            return _("Exporting to ODBC");
        case dbtypeDb2:
            return _("Exporting to DB2");
        case dbtypeXbase:
            return _("Exporting Xbase File(s)");
        case dbtypeDelimitedText:
            return _("Exporting Text-Delimited File(s)");
        case dbtypeFixedLengthText:
            return _("Exporting Fixed-Length File(s)");
    }

    return L"";
    */
    return L"Exporting Data";
}

static std::wstring getExportTitle(int export_type,
                                   const std::wstring& tablename = L"",
                                   const std::wstring& filename = L"",
                                   const std::wstring& server = L"")
{
    if (!isValidExportType(export_type))
        return L"";
            
    if (tablename.empty() && filename.empty() && server.empty())
        return getDefaultExportTitle(export_type);
    
    if (export_type == dbtypeSqlServer ||
        export_type == dbtypeMySql     ||
        export_type == dbtypeOracle    ||
        export_type == dbtypeOdbc      ||
        export_type == dbtypeDb2)
    {
        if (server.empty())
            return getDefaultExportTitle(export_type);
        
        if (tablename.length() > 0)
            return kl::stdswprintf(L"Exporting '%ls' to '%ls'", tablename.c_str(), server.c_str());
             else
            return kl::stdswprintf(L"Exporting to '%ls'", server.c_str());
    }
     else if (export_type == dbtypeAccess ||
              export_type == dbtypeExcel)
    {
        std::wstring fn = kl::afterLast(filename, PATH_SEPARATOR_CHAR);
        if (fn.empty())
            return getDefaultExportTitle(export_type);

        std::wstring tn = kl::afterLast(tablename, '/');
        if (tn.length() > 0)
            return kl::stdswprintf(L"Exporting '%ls' to '%ls'", tn.c_str(), fn.c_str());
             else
            return kl::stdswprintf(L"Exporting to '%ls'", fn.c_str());
    }
     else if (export_type == dbtypeFixedLengthText ||
              export_type == dbtypeDelimitedText   ||
              export_type == dbtypeXbase)
    {
        std::wstring fn = kl::afterLast(filename, PATH_SEPARATOR_CHAR);
        if (fn.empty())
            return getDefaultExportTitle(export_type);

        std::wstring tn = kl::afterLast(tablename, PATH_SEPARATOR_CHAR);
        if (tn.length() > 0)
            return kl::stdswprintf(L"Exporting '%ls' to '%ls'", tn.c_str(), fn.c_str());
             else
            return getDefaultExportTitle(export_type);
    }
    
    return L"";
}

void ExportJob::updateJobTitle(ExportJobInfo* info)
{
    std::wstring tablename, filename, title;
    
    if (!info)
    {
        title = getExportTitle(m_export_type);
        m_job_info->setTitle(title);
        return;
    }
       
    if (m_export_type == dbtypeSqlServer ||
        m_export_type == dbtypeMySql     ||
        m_export_type == dbtypeOracle    ||
        m_export_type == dbtypeOdbc      ||
        m_export_type == dbtypeDb2)
    {
        tablename = kl::afterLast(info->input_path, '/');
        title = getExportTitle(m_export_type, tablename, L"", m_host);
        m_job_info->setTitle(title);
    }
     else if (m_export_type == dbtypeAccess ||
              m_export_type == dbtypeExcel)
    {
        tablename = kl::afterLast(info->input_path, '/');
        filename = kl::afterLast(m_filename, PATH_SEPARATOR_CHAR);
        title = getExportTitle(m_export_type, tablename, filename);
        m_job_info->setTitle(title);
    }
     else if (m_export_type == dbtypeFixedLengthText ||
              m_export_type == dbtypeDelimitedText   ||
              m_export_type == dbtypeXbase)
    {
        tablename = kl::afterLast(info->input_path, '/');
        filename = kl::afterLast(info->output_path, PATH_SEPARATOR_CHAR);
        title = getExportTitle(m_export_type, tablename, filename);
        m_job_info->setTitle(title);
    }              
}

int ExportJob::runJob()
{
    if (!isValidExportType(m_export_type))
    {
        // unknown export type -- fail the job
        m_job_info->setState(jobStateFailed);
        return 0;
    }
    
    // only use default job title if the job info's title is empty
    std::wstring job_title = m_job_info->getTitle();
    if (job_title.empty())
        updateJobTitle(NULL);

    if (m_export_type == dbtypeAccess ||
        m_export_type == dbtypeExcel)
    {
        // if we selected a Microsoft Access or Microsoft Excel
        // export, overwrite it if that was the selected option
        if (m_overwrite_file)
        {
            tango::IDatabaseMgrPtr db_mgr;
            db_mgr.create_instance("xdodbc.DatabaseMgr");

            if (db_mgr.isNull())
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            xf_remove(m_filename);
        
            tango::IDatabasePtr db = db_mgr->createDatabase(m_filename, L"");
            if (db.isNull())
            {
                // could not create mdb database.  permissions problem maybe?
                m_job_info->setState(jobStateFailed);
                return 0;
            }
        }
    }


    IConnectionPtr conn = createUnmanagedConnection();
    conn->setType(m_export_type);
    conn->setHost(m_host);
    conn->setPort(m_port);
    conn->setDatabase(m_database);
    conn->setUsername(m_username);
    conn->setPassword(m_password);
    conn->setPath(m_filename);

    if (!conn->open())
    {
        // we couldn't open the connection, bail out
        m_job_info->setState(jobStateFailed);
        return 0;
    }


    tango::IDatabasePtr src_db = g_app->getDatabase();
    tango::IDatabasePtr dest_db = conn->getDatabasePtr();

    if (src_db.isNull() || dest_db.isNull())
    {
        // no source database available
        m_job_info->setState(jobStateFailed);
        return 0;
    }


    long long max_count = 0;
    bool max_count_known = true;
    max_count = 0.0;


    std::vector<ExportJobInfo>::iterator it;
    for (it = m_exports.begin(); it != m_exports.end(); ++it)
    {
        // get total row count
        tango::IFileInfoPtr finfo = src_db->getFileInfo(it->input_path);
        if (finfo.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        if (finfo->getFlags() & tango::sfFastRowCount)
        {
            m_obj_mutex.lock();
            max_count += finfo->getRowCount();
            m_obj_mutex.unlock();
        }
         else
        {
            max_count_known = false;
        }
    }

    if (max_count_known)
    {
        m_job_info->setMaxCount(max_count);
    }

    for (it = m_exports.begin(); it != m_exports.end(); ++it)
    {
        updateJobTitle(&(*it));
        
        tango::IStructurePtr src_struct;
        tango::IIteratorPtr src_iter;
        tango::IStructurePtr dest_struct;

        std::vector<ExportCopyInfo> copy_info;
        std::vector<ExportCopyInfo>::iterator copyinfo_it;


        // now, do the insert
        src_iter = src_db->createIterator(it->input_path, L"", L"", NULL);
        if (src_iter.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }


        src_struct = src_iter->getStructure();
        if (src_struct.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        int src_col_count = src_struct->getColumnCount();

        // try to open the set

        

        tango::IFileInfoPtr dest_finfo = dest_db->getFileInfo(it->output_path);

        if (it->append && dest_finfo.isOk())
        {
            // if we are appending to an existing table exists;
            // get the structure from that table

            dest_struct = dest_db->describeTable(it->output_path);
            if (dest_struct.isNull())
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            ExportCopyInfo ci;
            int i;
            for (i = 0; i < src_col_count; ++i)
            {
                tango::IColumnInfoPtr src_colinfo;
                tango::IColumnInfoPtr dest_colinfo;

                src_colinfo = src_struct->getColumnInfoByIdx(i);
                dest_colinfo = dest_struct->getColumnInfo(src_colinfo->getName());

                if (src_colinfo.isNull() || dest_colinfo.isNull())
                    continue;

                ci.src_name = src_colinfo->getName();
                ci.dest_name = dest_colinfo->getName();
                copy_info.push_back(ci);
            }
        }
         else
        {
            int i;
            ExportCopyInfo ci;


            std::set<std::wstring> dup_check;

            // we are not appending to a set, so create a new set
            // and fill out it's structure from the FieldTransInfo vector

            dest_struct = conn->getDatabasePtr()->createStructure();

            int conn_type = conn->getType();

            for (i = 0; i < src_col_count; ++i)
            {
                tango::IColumnInfoPtr src_colinfo = src_struct->getColumnInfoByIdx(i);

                ci.src_name = src_colinfo->getName();
                
                // xbase field names can only be 10 characters long
                std::wstring s;
                if (conn_type == dbtypeXbase)
                    s = ci.src_name.substr(0, 10);
                     else
                    s = ci.src_name;
                
                // field names with spaces were causing problems
                // on export for these types
                if (conn_type == dbtypeXbase ||
                    conn_type == dbtypeSqlServer ||
                    conn_type == dbtypeMySql ||
                    conn_type == dbtypeOracle ||
                    conn_type == dbtypeOdbc ||
                    conn_type == dbtypeDb2)
                {
                    kl::replaceStr(s, L" ", L"_", true);
                }
                    
                ci.dest_name = s;

                int counter = 2;
                kl::makeUpper(s);
                while (dup_check.find(s) != dup_check.end())
                {
                    if (conn_type == dbtypeXbase)
                        s = ci.src_name.substr(0, 10 - (counter/10) + 1);
                         else
                        s = ci.src_name;
                    s += kl::stdswprintf(L"%d", counter++);
                    ci.dest_name = s;

                    kl::makeUpper(s);
                }

                dup_check.insert(s);
                copy_info.push_back(ci);


                // create destination column
                tango::IColumnInfoPtr dest_colinfo = dest_struct->createColumn();
                dest_colinfo->setName(ci.dest_name);
                dest_colinfo->setType(src_colinfo->getType());
                dest_colinfo->setWidth(src_colinfo->getWidth());
                dest_colinfo->setScale(src_colinfo->getScale());
            }

            // if the table already exists, then we are overwriting it, so drop the
            // existing table before we create a new one
            if (dest_finfo.isOk())
            {
                conn->getDatabasePtr()->deleteFile(it->output_path);
            }

            // even if the table couldn't be opened, the file may still exist
            // (if it was corrupt, etc.), so make sure the file is deleted
            if (!it->append)
            {
                if (conn_type == dbtypeXbase ||
                    conn_type == dbtypeFilesystem ||
                    conn_type == dbtypeDelimitedText ||
                    conn_type == dbtypeFixedLengthText)
                {
                    if (xf_get_file_exist(it->output_path))
                    {
                        xf_remove(it->output_path);
                    }
                }
            }
            
            
            std::wstring delimiters = m_delimiters;
            std::wstring text_qualifiers = m_text_qualifier;
            
            int format;
            switch (conn_type)
            {
                default:
                    format = tango::formatNative;
                    break;
                case dbtypeFixedLengthText:  format = tango::formatFixedLengthText; break;
                case dbtypeXbase:            format = tango::formatXbase; break;
                case dbtypeDelimitedText:    format = tango::formatDelimitedText; break;
                case dbtypeXdnative:         format = tango::formatNative; break;
                case dbtypeFilesystem:
                {
                    // determine the file format from the output filename's extension
                    std::wstring ext;
                    if (it->output_path.find(L".") != it->output_path.npos)
                        ext = kl::afterLast(it->output_path, '.');                    
                    kl::makeLower(ext);
                    
                    if (ext == L"dbf")
                    {
                       format = tango::formatXbase;
                    }
                     else if (ext == L"csv" || ext == L"icsv" || ext == L"txt")
                    {
                        format = tango::formatDelimitedText;
                        delimiters = L",";
                        text_qualifiers = L"\"";
                    }
                     else if (ext == L"tsv")
                    {
                        format = tango::formatDelimitedText;
                        delimiters = L"\t";
                        text_qualifiers = L"";
                    }
                     else
                    {
                        format = tango::formatNative;
                    }
                }
            }
            
            
            switch (format)
            {
                default:
                    format = tango::formatNative;
                case tango::formatNative:
                case tango::formatXbase:
                case tango::formatFixedLengthText:
                {
                    tango::FormatInfo fi;
                    fi.table_format = format;

                    dest_db->createTable(it->output_path,
                                         dest_struct,
                                         &fi);
                }
                break;

                case tango::formatDelimitedText:
                {
                    std::wstring ext;
                    if (it->output_path.find(L".") != it->output_path.npos)
                        ext = kl::afterLast(it->output_path, '.');                    
                    kl::makeLower(ext);
                    
                    if (ext == wxT("icsv"))
                    {
                        tango::FormatInfo fi;
                        fi.table_format = tango::formatDelimitedText;

                        dest_db->createTable(it->output_path,
                                             dest_struct,
                                             &fi);
                    }
                     else
                    {
                        tango::FormatInfo fi;
                        fi.table_format = tango::formatDelimitedText;
                        fi.delimiters = delimiters;
                        fi.text_qualifiers = text_qualifiers;
                        fi.line_delimiters = L"\n";
                        fi.first_row_column_names = m_first_row_header;

                        dest_db->createTable(it->output_path,
                                             dest_struct,
                                             &fi);
                    }
                    
                    break;
                }
            }


            // first, update the structure; even though we tried to create
            // a table with particular types, the columns may not actually have 
            // those types; in particular, this happens with the Access and Excel 
            // ODBC drivers, that always export to wide character fields; without
            // updating the destination structure, the following insertion routine 
            // tries to insert text into a regular character field, while the handle, 
            // which knows that the output field is a wide character, sets the wide
            // character parameter, leaving the regular character parameter blank,
            //  resulting in no values being inserted into character fields

            dest_struct = dest_db->describeTable(it->output_path);
            if (!dest_struct)
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }
        }

        // now that we have created the destination set,
        // insert rows into that set

        tango::IRowInserterPtr ri;
        ri = dest_db->bulkInsert(it->output_path);
        ri->startInsert(L"*");

        // get input and output handles
        for (copyinfo_it = copy_info.begin();
             copyinfo_it != copy_info.end();
             ++copyinfo_it)
        {
            tango::IColumnInfoPtr dest_colinfo;
            dest_colinfo = dest_struct->getColumnInfo(copyinfo_it->dest_name);
            if (dest_colinfo.isNull())
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            copyinfo_it->src_handle = src_iter->getHandle(copyinfo_it->src_name);
            copyinfo_it->dest_handle = ri->getHandle(copyinfo_it->dest_name);
            copyinfo_it->dest_type = dest_struct->getColumnInfo(copyinfo_it->dest_name)->getType();

            if (!copyinfo_it->src_handle || !copyinfo_it->dest_handle)
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }
        }

     
        src_iter->goFirst();
        while (!src_iter->eof())
        {
            if (isCancelling())
            {
                break;
            }

            for (copyinfo_it = copy_info.begin();
                 copyinfo_it != copy_info.end();
                 ++copyinfo_it)
            {
                switch (copyinfo_it->dest_type)
                {
                    case tango::typeCharacter:
                    {
                        ri->putString(copyinfo_it->dest_handle,
                            src_iter->getString(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeWideCharacter:
                    {
                        ri->putWideString(copyinfo_it->dest_handle,
                            src_iter->getWideString(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeNumeric:
                    {
                        ri->putDouble(copyinfo_it->dest_handle,
                            src_iter->getDouble(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeDouble:
                    {
                        ri->putDouble(copyinfo_it->dest_handle,
                            src_iter->getDouble(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeInteger:
                    {
                        ri->putInteger(copyinfo_it->dest_handle,
                            src_iter->getInteger(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeDateTime:
                    {
                        ri->putDateTime(copyinfo_it->dest_handle,
                            src_iter->getDateTime(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeDate:
                    {
                        ri->putDateTime(copyinfo_it->dest_handle,
                            src_iter->getDateTime(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeBoolean:
                    {
                        ri->putBoolean(copyinfo_it->dest_handle,
                            src_iter->getBoolean(copyinfo_it->src_handle));
                        break;
                    }
                }
            }

            ri->insertRow();
          
            src_iter->skip(1);
            m_job_info->incrementCurrentCount(1.0);
        }

        ri->finishInsert();
    }

    return 0;
}

void ExportJob::runPostJob()
{
    g_app->getAppController()->refreshDbDoc();
}






