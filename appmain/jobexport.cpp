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
    m_job_info->setTitle(wxEmptyString);

    m_export_type = dbtypeUndefined;
    m_fix_invalid_fieldnames = false;

    m_filename = wxEmptyString;
    m_overwrite_file = true;
    
    m_host = wxEmptyString;
    m_database = wxEmptyString;
    m_username = wxEmptyString;
    m_password = wxEmptyString;
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

void ExportJob::setFilename(const wxString& filename, bool overwrite)
{
    m_filename = filename;
    m_overwrite_file = overwrite;
}

void ExportJob::setConnectionInfo(const wxString& host,
                                  int port,
                                  const wxString& database,
                                  const wxString& username,
                                  const wxString& password)
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

void ExportJob::setDelimiters(const wxString& delimiters)
{
    m_delimiters = delimiters;
}

void ExportJob::setTextQualifier(const wxString& text_qualifier)
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

static wxString getDefaultExportTitle(int export_type)
{
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

    return wxEmptyString;
}

static wxString getExportTitle(int export_type,
                               const wxString& tablename = wxEmptyString,
                               const wxString& filename = wxEmptyString,
                               const wxString& server = wxEmptyString)
{
    if (!isValidExportType(export_type))
        return wxEmptyString;
            
    if (tablename.IsEmpty() && filename.IsEmpty() && server.IsEmpty())
        return getDefaultExportTitle(export_type);
    
    if (export_type == dbtypeSqlServer ||
        export_type == dbtypeMySql     ||
        export_type == dbtypeOracle    ||
        export_type == dbtypeOdbc      ||
        export_type == dbtypeDb2)
    {
        if (server.IsEmpty())
            return getDefaultExportTitle(export_type);
        
        if (tablename.Length() > 0)
            return wxString::Format(_("Exporting '%s' to '%s'"), tablename.c_str(), server.c_str());
             else
            return wxString::Format(_("Exporting to '%s'"), server.c_str());
    }
     else if (export_type == dbtypeAccess ||
              export_type == dbtypeExcel)
    {
        wxString fn = filename.AfterLast(PATH_SEPARATOR_CHAR);
        if (fn.IsEmpty())
            return getDefaultExportTitle(export_type);

        wxString tn = tablename.AfterLast(wxT('/'));
        if (tn.Length() > 0)
            return wxString::Format(_("Exporting '%s' to '%s'"), tn.c_str(), fn.c_str());
             else
            return wxString::Format(_("Exporting to '%s'"), fn.c_str());
    }
     else if (export_type == dbtypeFixedLengthText ||
              export_type == dbtypeDelimitedText   ||
              export_type == dbtypeXbase)
    {
        wxString fn = filename.AfterLast(PATH_SEPARATOR_CHAR);
        if (fn.IsEmpty())
            return getDefaultExportTitle(export_type);

        wxString tn = tablename.AfterLast(PATH_SEPARATOR_CHAR);
        if (tn.Length() > 0)
            return wxString::Format(_("Exporting '%s' to '%s'"), tn.c_str(), fn.c_str());
             else
            return getDefaultExportTitle(export_type);
    }
    
    return wxEmptyString;
}

void ExportJob::updateJobTitle(ExportJobInfo* info)
{
    wxString tablename, filename, title;
    
    if (!info)
    {
        title = getExportTitle(m_export_type);
        m_job_info->setTitle(towstr(title));
        return;
    }
       
    if (m_export_type == dbtypeSqlServer ||
        m_export_type == dbtypeMySql     ||
        m_export_type == dbtypeOracle    ||
        m_export_type == dbtypeOdbc      ||
        m_export_type == dbtypeDb2)
    {
        tablename = info->input_path.AfterLast(wxT('/'));
        title = getExportTitle(m_export_type, tablename, wxEmptyString, m_host);
        m_job_info->setTitle(towstr(title));
    }
     else if (m_export_type == dbtypeAccess ||
              m_export_type == dbtypeExcel)
    {
        tablename = info->input_path.AfterLast(wxT('/'));
        filename = m_filename.AfterLast(PATH_SEPARATOR_CHAR);
        title = getExportTitle(m_export_type, tablename, filename);
        m_job_info->setTitle(towstr(title));
    }
     else if (m_export_type == dbtypeFixedLengthText ||
              m_export_type == dbtypeDelimitedText   ||
              m_export_type == dbtypeXbase)
    {
        tablename = info->input_path.AfterLast(wxT('/'));
        filename = info->output_path.AfterLast(PATH_SEPARATOR_CHAR);
        title = getExportTitle(m_export_type, tablename, filename);
        m_job_info->setTitle(towstr(title));
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
    wxString job_title = m_job_info->getTitle();
    if (job_title.IsEmpty())
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

            xf_remove(towstr(m_filename));
        
            tango::IDatabasePtr db = db_mgr->createDatabase(towstr(m_filename), L"");
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

    tango::IDatabasePtr src_db_ptr = g_app->getDatabase();
    if (src_db_ptr.isNull())
    {
        // no source database available
        m_job_info->setState(jobStateFailed);
        return 0;
    }



    double max_count;

    m_obj_mutex.lock();
    max_count = 0.0;
    m_obj_mutex.unlock();

    std::vector<ExportJobInfo>::iterator it;
    for (it = m_exports.begin(); it != m_exports.end(); ++it)
    {
        // get total row count
        tango::ISetPtr set = src_db_ptr->openSet(towstr(it->input_path));
        if (set.isNull())
            continue;

        if (set->getSetFlags() & tango::sfFastRowCount)
        {
            m_obj_mutex.lock();
            max_count += (tango::tango_int64_t)set->getRowCount();
            m_obj_mutex.unlock();
        }
    }

    m_job_info->setMaxCount(max_count);

    for (it = m_exports.begin(); it != m_exports.end(); ++it)
    {
        updateJobTitle(&(*it));
        
        tango::ISetPtr src_set;
        tango::IStructurePtr src_struct;
        tango::IIteratorPtr src_iter;
        tango::ISetPtr dest_set;
        tango::IStructurePtr dest_struct;

        std::vector<ExportCopyInfo> copy_info;
        std::vector<ExportCopyInfo>::iterator copyinfo_it;


        src_set = src_db_ptr->openSet(towstr(it->input_path));
        if (src_set.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        src_struct = src_set->getStructure();
        if (src_struct.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        int src_col_count = src_struct->getColumnCount();

        // try to open the set
        dest_set = conn->getDatabasePtr()->openSet(towstr(it->output_path));


        if (it->append && dest_set.isOk())
        {
            // if we are appending to a set and the destination
            // set exists, get the structure from that set

            dest_struct = dest_set->getStructure();


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

                ci.src_name = towx(src_colinfo->getName());
                ci.dest_name = towx(dest_colinfo->getName());
                copy_info.push_back(ci);
            }
        }
         else
        {
            int i;
            ExportCopyInfo ci;


            std::set<wxString> dup_check;

            // we are not appending to a set, so create a new set
            // and fill out it's structure from the FieldTransInfo vector

            dest_struct = conn->getDatabasePtr()->createStructure();

            int conn_type = conn->getType();

            for (i = 0; i < src_col_count; ++i)
            {
                tango::IColumnInfoPtr src_colinfo = src_struct->getColumnInfoByIdx(i);

                ci.src_name = towx(src_colinfo->getName());
                
                // xbase field names can only be 10 characters long
                wxString s;
                if (conn_type == dbtypeXbase)
                    s = ci.src_name.Left(10);
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
                    s.Replace(wxT(" "), wxT("_"), true);
                }
                    
                ci.dest_name = s;

                int counter = 2;
                s.MakeUpper();
                while (dup_check.find(s) != dup_check.end())
                {
                    if (conn_type == dbtypeXbase)
                        s = ci.src_name.Left(10-((counter/10)+1));
                         else
                        s = ci.src_name;
                    s += wxString::Format(wxT("%d"), counter++);
                    ci.dest_name = s;
                    s.MakeUpper();
                }

                dup_check.insert(s);
                copy_info.push_back(ci);


                // create destination column
                tango::IColumnInfoPtr dest_colinfo = dest_struct->createColumn();
                dest_colinfo->setName(towstr(ci.dest_name));
                dest_colinfo->setType(src_colinfo->getType());
                dest_colinfo->setWidth(src_colinfo->getWidth());
                dest_colinfo->setScale(src_colinfo->getScale());
            }

            // if the table already exists, then we are overwriting it, so drop the
            //    existing table before we create the new table
            if (dest_set.isOk())
            {
                dest_set.clear();
                conn->getDatabasePtr()->deleteFile(towstr(it->output_path));
            }

            // even if the set couldn't be opened, the file may still exists
            // (if it was corrupt, etc.), so make sure the file is deleted
            if (!it->append)
            {
                if (conn_type == dbtypeXbase ||
                    conn_type == dbtypeFilesystem ||
                    conn_type == dbtypeDelimitedText ||
                    conn_type == dbtypeFixedLengthText)
                {
                    if (xf_get_file_exist(towstr(it->output_path)))
                        xf_remove(towstr(it->output_path));
                }
            }
            
            
            std::wstring delimiters = towstr(m_delimiters);
            std::wstring text_qualifiers = towstr(m_text_qualifier);
            
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
                    wxString ext;
                    if (it->output_path.Find(wxT('.')) != -1)
                        ext = it->output_path.AfterLast(wxT('.'));                    
                    ext.MakeLower();
                    
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

                    dest_set = conn->getDatabasePtr()->createTable(
                                                       towstr(it->output_path),
                                                       dest_struct,
                                                       &fi);
                }
                break;

                case tango::formatDelimitedText:
                {
                    wxString ext;
                    if (it->output_path.Find(wxT('.')) != -1)
                        ext = it->output_path.AfterLast(wxT('.'));                    
                    ext.MakeLower();
                    
                    if (ext == wxT("icsv"))
                    {
                        tango::FormatInfo fi;
                        fi.table_format = tango::formatDelimitedText;

                        dest_set = conn->getDatabasePtr()->createTable(
                                                           towstr(it->output_path),
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

                        // create a delimited-text set with no fields; create it with
                        // no fields for two reasons because the set creation routine
                        // automatically adds fields with specific delimiters and for
                        // this export, we need control over those delimiters as well
                        // as whether or not the fields even show
                        tango::IStructurePtr empty_struct;
                        empty_struct = g_app->getDatabase()->createStructure();

                        dest_set = conn->getDatabasePtr()->createTable(
                                                           towstr(it->output_path),
                                                           dest_struct,
                                                           &fi);
                    }
                    
                    break;
                }
            }

            if (!dest_set)
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }
        }

        // now that we have created the destination set,
        // insert rows into that set

        // first, update the copy info type; even though we tried to create
        // a set with particular types, the columns may not actually have 
        // those types; in particular, this happens with the Access and Excel 
        // ODBC drivers, that always export to wide character fields; without
        // updating the destination structure, the following insertion routine 
        // tries to insert text into a regular character field, while the handle, 
        // which knows that the output field is a wide character, sets the wide
        // character parameter, leaving the regular character parameter blank,
        //  resulting in no values being inserted into character fields
        dest_struct = dest_set->getStructure();

        // now, do the insert
        src_iter = src_set->createIterator(L"", L"", NULL);

        tango::IRowInserterPtr ri;
        ri = dest_set->getRowInserter();
        ri->startInsert(L"*");

        // get input and output handles
        for (copyinfo_it = copy_info.begin();
             copyinfo_it != copy_info.end();
             ++copyinfo_it)
        {
            tango::IColumnInfoPtr dest_colinfo;
            dest_colinfo = dest_struct->getColumnInfo(towstr(copyinfo_it->dest_name));
            if (dest_colinfo.isNull())
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            copyinfo_it->src_handle = src_iter->getHandle(towstr(copyinfo_it->src_name));
            copyinfo_it->dest_handle = ri->getHandle(towstr(copyinfo_it->dest_name));
            copyinfo_it->dest_type = dest_struct->getColumnInfo(towstr(copyinfo_it->dest_name))->getType();

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

        // store sets
        if (!isCancelling() && dest_set.isOk())
        {
            conn->getDatabasePtr()->storeObject(dest_set,
                                                towstr(it->output_path));
        }
    }

    return 0;
}

void ExportJob::runPostJob()
{
    g_app->getAppController()->refreshDbDoc();
}






