/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-12-10
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "appmain.h"
#include "jobimport.h"
#include "appcontroller.h"
#include <set>


ImportJob::ImportJob()
{
    m_job_info->setTitle(wxEmptyString);
    m_import_type = dbtypeUndefined;
    m_port = 0;
}

ImportJob::~ImportJob()
{

}

void ImportJob::setImportType(int type)
{
    m_import_type = type;
}

void ImportJob::setFilename(const wxString& filename)
{
    m_filename = filename;
}

void ImportJob::setConnectionInfo(const wxString& host,
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

void ImportJob::addImportSet(const ImportJobInfo& info)
{
    m_imports.push_back(info);
}

std::vector<ImportJobInfo> ImportJob::getImportSets()
{
    return m_imports;
}

bool ImportJob::handleFieldInfo(FieldTransInfo* info)
{
    // this function handles type, width, and scale info
    // for the output field.  If the output type is not the same
    // as the input type, it will try to convert it, and it will
    // also try to handle possible invalid widths and scales

    wxString current_expression = wxT("");
    if (info->expression.length() > 0)
    {
        current_expression = info->expression;
    }

    int dest_type = info->output_type;

    switch (info->input_type)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
        {
            if (dest_type == tango::typeDate ||
                dest_type == tango::typeDateTime)
            {
                wxString s;

                s.Printf(wxT("date(%s)"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            if (dest_type == tango::typeNumeric ||
                dest_type == tango::typeDouble ||
                dest_type == tango::typeInteger)
            {
                wxString s;

                s.Printf(wxT("val(%s)"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            if (dest_type == tango::typeBoolean)
            {
                wxString s;

                s.Printf(wxT("iif(%s = \"T\", true, false)"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            break;
        }

        case tango::typeDate:
        case tango::typeDateTime:
        {
            if (dest_type == tango::typeCharacter ||
                dest_type == tango::typeWideCharacter)
            {
                wxString s;

                s.Printf(wxT("str(%s)"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            if (dest_type == tango::typeNumeric ||
                dest_type == tango::typeDouble ||
                dest_type == tango::typeInteger ||
                dest_type == tango::typeBoolean)
            {
                // cannot convert to these types
                return false;
            }

            break;
        }

        case tango::typeNumeric:
        {
            if (dest_type == tango::typeCharacter ||
                dest_type == tango::typeCharacter)
            {
                wxString s;

                s.Printf(wxT("str(%s, %d, %d)"),
                         makeProper(info->input_name).c_str(),
                         info->input_width,
                         info->input_scale);

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            if (dest_type == tango::typeDate ||
                dest_type == tango::typeDateTime)
            {
                // cannot convert to these types
                return false;
            }

            if (dest_type == tango::typeBoolean)
            {
                wxString s;

                s.Printf(wxT("iif(%s != 0, true, false)"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            break;
        }

        case tango::typeDouble:
        {
            if (dest_type == tango::typeCharacter ||
                dest_type == tango::typeWideCharacter)
            {
                wxString s;

                s.Printf(wxT("str(%s, %d, %d)"),
                         makeProper(info->input_name).c_str(),
                         info->output_width > 20 ? 20 : info->output_width,
                         info->input_scale);

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;

                break;
            }

            if (dest_type == tango::typeDate ||
                dest_type == tango::typeDateTime)
            {
                // cannot convert to these types
                return false;
            }

            if (dest_type == tango::typeBoolean)
            {
                wxString s;

                s.Printf(wxT("iif(%s != 0, true, false)"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            break;
        }

        case tango::typeInteger:
        {
            if (dest_type == tango::typeCharacter ||
                dest_type == tango::typeWideCharacter)
            {
                wxString s;

                s.Printf(wxT("str(%s, %d, %d)"),
                         makeProper(info->input_name).c_str(),
                         info->output_width > 10 ? 10 : info->output_width,
                         info->input_scale);

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;

                break;
            }

            if (dest_type == tango::typeDate ||
                dest_type == tango::typeDateTime)
            {
                // cannot convert to these types
                return false;
            }

            if (dest_type == tango::typeBoolean)
            {
                wxString s;

                s.Printf(wxT("iif(%s != 0, true, false)"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            break;
        }

        case tango::typeBoolean:
        {
            if (dest_type == tango::typeCharacter ||
                dest_type == tango::typeWideCharacter)
            {
                wxString s;

                s.Printf(wxT("iif(%s, \"T\", \"F\")"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            if (dest_type == tango::typeDate ||
                dest_type == tango::typeDateTime)
            {
                // cannot convert to these types
                return false;
            }

            if (dest_type == tango::typeNumeric ||
                dest_type == tango::typeDouble ||
                dest_type == tango::typeInteger)
            {
                wxString s;

                s.Printf(wxT("iif(%s, 1, 0)"),
                         makeProper(info->input_name).c_str());

                info->expression = current_expression.length() > 0 ?
                                              current_expression : s;
                break;
            }

            break;
        }
    }

    return true;
}


void ImportJob::correctFieldParams(FieldTransInfo* info)
{
    int width = -1;
    int scale = -1;

    switch (info->output_type)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
            scale = 0;
            break;

        case tango::typeNumeric:
            if (width < 1 || width > 18)
                width = 18;
            break;

        case tango::typeDouble:
            width = 8;
            break;

        case tango::typeInteger:
            width = 4;
            scale = 0;
            break;

        case tango::typeDate:
            width = 4;
            scale = 0;
            break;

        case tango::typeDateTime:
            width = 8;
            scale = 0;
            break;

        case tango::typeBoolean:
            width = 1;
            scale = 0;
            break;

        default:
            width = -1;
            break;
    }

    if (width != -1)
        info->output_width = width;

    if (scale != -1)
        info->output_scale = scale;
}


struct ImportCopyInfo
{
    int dest_type;
    tango::objhandle_t src_handle;
    tango::objhandle_t dest_handle;
};


static bool isValidImportType(int import_type)
{
    switch (import_type)
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

static wxString getDefaultImportTitle(int import_type)
{
    switch (import_type)
    {
        case dbtypeAccess:
            return _("Importing Microsoft Access File");
        case dbtypeExcel:
            return _("Importing Microsoft Excel File");
        case dbtypeSqlServer:
            return _("Importing from SQL Server");
        case dbtypeMySql:
            return _("Importing from MySQL");
        case dbtypeOracle:
            return _("Importing from Oracle");
        case dbtypeOdbc:
            return _("Importing from ODBC");
        case dbtypeDb2:
            return _("Importing from DB2");
        case dbtypeXbase:
            return _("Importing Xbase File(s)");
        case dbtypeDelimitedText:
            return _("Importing Text-Delimited File(s)");
        case dbtypeFixedLengthText:
            return _("Importing Fixed-Length File(s)");
    }

    return wxEmptyString;
}

static wxString getImportTitle(int import_type,
                               const wxString& tablename = wxEmptyString,
                               const wxString& filename = wxEmptyString,
                               const wxString& server = wxEmptyString)
{
    if (!isValidImportType(import_type))
        return wxEmptyString;
            
    if (tablename.IsEmpty() && filename.IsEmpty() && server.IsEmpty())
        return getDefaultImportTitle(import_type);
    
    if (import_type == dbtypeSqlServer ||
        import_type == dbtypeMySql     ||
        import_type == dbtypeOracle    ||
        import_type == dbtypeOdbc      ||
        import_type == dbtypeDb2)
    {
        if (server.IsEmpty())
            return getDefaultImportTitle(import_type);
        
        if (tablename.Length() > 0)
            return wxString::Format(_("Importing '%s' from '%s'"), tablename.c_str(), server.c_str());
             else
            return wxString::Format(_("Importing from '%s'"), server.c_str());
    }
     else if (import_type == dbtypeAccess ||
              import_type == dbtypeExcel)
    {
        wxString fn = filename.AfterLast(PATH_SEPARATOR_CHAR);
        if (fn.IsEmpty())
            return getDefaultImportTitle(import_type);

        wxString tn = tablename.AfterLast(wxT('/'));
        if (tn.Length() > 0)
            return wxString::Format(_("Importing '%s' from '%s'"), tn.c_str(), fn.c_str());
             else
            return wxString::Format(_("Importing from '%s'"), fn.c_str());
    }
     else if (import_type == dbtypeFixedLengthText ||
              import_type == dbtypeDelimitedText   ||
              import_type == dbtypeXbase)
    {
        wxString tn = tablename.AfterLast(PATH_SEPARATOR_CHAR);
        if (tn.Length() > 0)
            return wxString::Format(_("Importing '%s'"), tn.c_str());
             else
            return getDefaultImportTitle(import_type);
    }
    
    return wxEmptyString;
}


long long ImportJob::getTotalRowCount(tango::IDatabasePtr db)
{
    std::vector<ImportJobInfo>::iterator it;
    tango::ISetPtr src_set;

    if (m_import_type != dbtypeOracle &&
        m_import_type != dbtypeDelimitedText &&
        m_import_type != dbtypeFixedLengthText)
    {
        long long max_count = 0;
        bool use_max_count = true;

        for (it = m_imports.begin(); it != m_imports.end(); ++it)
        {
            if (!it->query.IsEmpty())
            {
                use_max_count = false;
                break;
            }


            int format = tango::formatNative;
            if (m_import_type == dbtypeDelimitedText)
            {
                format = tango::formatDelimitedText;
            }
             else if (m_import_type == dbtypeFixedLengthText)
            {
                format = tango::formatFixedLengthText;
            }


            src_set = db->openSetEx(towstr(it->input_path), format);
            if (!src_set)
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            // get total row count
            if (src_set->getSetFlags() & tango::sfFastRowCount)
            {
                max_count += src_set->getRowCount();
            }
             else
            {
                use_max_count = false;
            }
        }

        if (use_max_count)
            return max_count;
    }

    return 0;
}

void ImportJob::updateJobTitle(const wxString& tablename)
{
    wxString title;
        
    if (m_import_type == dbtypeSqlServer ||
        m_import_type == dbtypeMySql     ||
        m_import_type == dbtypeOracle    ||
        m_import_type == dbtypeOdbc      ||
        m_import_type == dbtypeDb2)
    {
        title = getImportTitle(m_import_type, tablename, wxEmptyString, m_host);
        m_job_info->setTitle(towstr(title));
    }
     else if (m_import_type == dbtypeAccess ||
              m_import_type == dbtypeExcel)
    {
        title = getImportTitle(m_import_type, tablename, m_filename);
        m_job_info->setTitle(towstr(title));
    }
     else if (m_import_type == dbtypeFixedLengthText ||
              m_import_type == dbtypeDelimitedText   ||
              m_import_type == dbtypeXbase)
    {
        title = getImportTitle(m_import_type, tablename);
        m_job_info->setTitle(towstr(title));
    }              
}

int ImportJob::runJob()
{
    if (!isValidImportType(m_import_type))
    {
        // unknown import type -- fail the job
        m_job_info->setState(jobStateFailed);
        return 0;
    }
    
    // only use default job title if the job info's title is empty
    wxString job_title = m_job_info->getTitle();
    if (job_title.IsEmpty())
    {
        // try to get the name of the first table we're going to import
        if (m_import_type == dbtypeFixedLengthText ||
            m_import_type == dbtypeDelimitedText ||
            m_import_type == dbtypeXbase)
        {
            if (m_imports.size() == 0)
                updateJobTitle(wxEmptyString);
                 else
                updateJobTitle(m_imports.begin()->input_path);
        }
         else
        {
            updateJobTitle(wxEmptyString);
        }
    }
    
    if (m_import_type == dbtypeSqlServer ||
        m_import_type == dbtypeMySql     ||
        m_import_type == dbtypeOracle    ||
        m_import_type == dbtypeOdbc      ||
        m_import_type == dbtypeDb2)
    {
        m_job_info->setProgressString(towstr(_("Opening database connection...")));
    }
     else if (m_import_type == dbtypeFixedLengthText ||
              m_import_type == dbtypeDelimitedText ||
              m_import_type == dbtypeXbase ||
              m_import_type == dbtypeExcel ||
              m_import_type == dbtypeAccess)
    {
        m_job_info->setProgressString(towstr(_("Reading file...")));
    }              
    
    // create a connection to the source database
    IConnectionPtr conn = createUnmanagedConnection();
    conn->setType(m_import_type);
    conn->setHost(m_host);
    conn->setPort(m_port);
    conn->setDatabase(m_database);
    conn->setUsername(m_username);
    conn->setPassword(m_password);
    conn->setPath(m_filename);

    // we couldn't open the connection, bail out
    if (!conn->open())
    {
        if (m_import_type == dbtypeSqlServer ||
            m_import_type == dbtypeMySql     ||
            m_import_type == dbtypeOracle    ||
            m_import_type == dbtypeOdbc      ||
            m_import_type == dbtypeDb2)
        {
            m_job_info->setProgressString(towstr(_("The database connection could not be opened.")));
        }
         else if (m_import_type == dbtypeAccess        ||
                  m_import_type == dbtypeExcel         ||
                  m_import_type == dbtypeXbase         ||
                  m_import_type == dbtypeDelimitedText ||
                  m_import_type == dbtypeFixedLengthText)
        {
            m_job_info->setProgressString(towstr(_("The file could not be opened.")));
        }              

        m_job_info->setState(jobStateFailed);
        return 0;
    }


    tango::IDatabasePtr src_db = conn->getDatabasePtr();
    tango::IDatabasePtr dest_db = g_app->getDatabase();
    tango::ISetPtr src_set;
    tango::IStructurePtr src_struct;
    tango::IIteratorPtr src_iter;
    tango::ISetPtr dest_set;
    tango::IStructurePtr dest_struct;
    tango::rowpos_t row_count = 0;
    long long total_row_count = 0;


    // determine the total row count
    long long max_row_count = getTotalRowCount(src_db);
    
    if (max_row_count == 0)
        m_job_info->setMaxCount(0.0);
         else
        m_job_info->setMaxCount(max_row_count);



    std::vector<ImportJobInfo>::iterator it;
    std::vector<ImportCopyInfo> copy_info;

    // import each set
    for (it = m_imports.begin(); it != m_imports.end(); ++it)
    {
        updateJobTitle(it->input_path);

        // clear the copy info at the beginning of each set's import
        copy_info.clear();


        int format = tango::formatNative;
        if (m_import_type == dbtypeDelimitedText)
        {
            format = tango::formatDelimitedText;
        }
         else if (m_import_type == dbtypeFixedLengthText)
        {
            format = tango::formatFixedLengthText;
        }



        // the input path is filled out and there is no query

        if (!it->input_path.IsEmpty() && it->query.IsEmpty())
        {
            // every time we open a text-delimited set, we need to read some
            // of the file -- let the user know this is happening
            if (m_import_type == dbtypeDelimitedText)
                m_job_info->setProgressString(towstr(_("Reading file...")));

            src_set = src_db->openSetEx(towstr(it->input_path), format);

            // if we can't open the source set, bail out
            if (src_set.isNull())
            {
                m_job_info->setProgressString(wxEmptyString);
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            // set the parameters for a delimited text import
            tango::IDelimitedTextSetPtr td_set = src_set;
            if (td_set.isOk())
            {
                if (it->specify_text_params)
                {
                    // TODO: on sets with definitions, we don't want to overwrite the
                    // definition that's there...  This needs to be investigated
                    // in more detail.  This behavior is the default, and could
                    // possibly be wrong.
                    
                    td_set->setDelimiters(towstr(it->delimiters), false);
                    td_set->setTextQualifier(towstr(it->text_qualifier), false);
                    td_set->setFirstRowColumnNames(it->first_row_header);
                    td_set->setDiscoverFirstRowColumnNames(false);
                }
                
                m_job_info->setProgressString(wxEmptyString);
                m_job_info->setProgressStringFormat(
                                            towstr(_("Determining table structure: $c records processed")),
                                            towstr(_("Determining table structure: $c of $m records processed ($p1%)")));

                tango::IJobPtr tango_job = src_db->createJob();
                setTangoJob(tango_job);
                
                // read through the entire file to determine column sizes
                td_set->determineColumns(-1, tango_job);
                if (tango_job->getCancelled())
                {
                    m_job_info->setProgressString(wxEmptyString);
                    m_job_info->setState(jobStateCancelled);
                }
                m_job_info->setCurrentCount(0);
                if (td_set->isFirstRowColumnNames())
                    m_job_info->setMaxCount((long long)tango_job->getMaxCount()-1);
                     else
                    m_job_info->setMaxCount((long long)tango_job->getMaxCount());
                setTangoJob(xcm::null);
                
                // set the progress string format back to its default
                m_job_info->setProgressString(wxEmptyString);
                m_job_info->setProgressStringFormat(
                                            towstr(_("$c records processed")),
                                            towstr(_("$c of $m records processed ($p1%)")));
            }

            // set the parameters for a fixed-length text import
            tango::IFixedLengthDefinitionPtr flt_set = src_set;
            if (flt_set.isOk())
            {
                flt_set->setRowWidth(it->row_width);
                
                // insert the field info into the set
                std::vector<FieldTransInfo>::iterator field_it;
                for (field_it = it->field_info.begin();
                     field_it != it->field_info.end();
                     ++field_it)
                {
                    // we'll deal with this later... it's part of the
                    // external data tango reorganization
                    /*
                    flt_set->insertColumn(-1,
                                          field_it->input_offset,
                                          field_it->input_width,
                                          towstr(field_it->output_name),
                                          field_it->output_type,
                                          field_it->output_width,
                                          field_it->output_scale,
                                          towstr(field_it->expression));
                    */
                }
                
                
                m_job_info->setProgressString(wxEmptyString);

                tango::IJobPtr tango_job = src_db->createJob();
                setTangoJob(tango_job);

                if (tango_job->getCancelled())
                {
                    m_job_info->setProgressString(wxEmptyString);
                    m_job_info->setState(jobStateCancelled);
                }
                m_job_info->setCurrentCount(0);
                m_job_info->setMaxCount((long long)tango_job->getMaxCount());
                setTangoJob(xcm::null);

                m_job_info->setProgressString(wxEmptyString);

                // we are done with it->field_info, because it was used only
                // to transfer the structure info (this will have to be changed
                // later, if we want to use this structure to communicate an
                // additional field transformation as opposed to only a source
                // structure definition)

                it->field_info.clear();
            }

            // create an iterator on the source set
            src_iter = src_db->createIterator(towstr(it->input_path), L"", L"", NULL);
        }
         else
        {
            if (!it->input_path.IsEmpty())
            {
                wxString table_name = it->input_path;

                if (m_import_type != dbtypeXdnative &&
                    m_import_type != dbtypePackage)
                {
                    table_name.Replace(wxT("/"), wxT("."));
                    while (table_name.Left(1) == wxT("."))
                        table_name.Remove(0,1);
                }

                wxString query = wxT("SELECT ");

                
                if (it->field_info.size() == 0)
                {
                    query += wxT("*");
                }
                 else
                {
                    std::vector<FieldTransInfo>::iterator field_it;
                    for (field_it = it->field_info.begin();
                         field_it != it->field_info.end();
                         ++field_it)
                    {
                        if (field_it != it->field_info.begin())
                        {
                            query += wxT(",");
                        }

                        query += field_it->input_name;
                    }
                }

                query += wxT(" FROM ");
                query += table_name;
                query += wxT(" WHERE ");
                query += it->query;


                xcm::IObjectPtr result;
                src_db->execute(towstr(query), 0, result, NULL);
                src_iter = result;
            }
             else
            {
                xcm::IObjectPtr result;
                src_db->execute(towstr(it->query), 0, result, NULL);
                src_iter = result;
            }
        }


        // if we couldn't create an iterator on our source set, bail out
        if (src_iter.isNull())
        {
            m_job_info->setProgressString(wxEmptyString);
            m_job_info->setState(jobStateFailed);
            return 0;
        }


        // try to open the destination set (to see if it already exists)
        dest_set = dest_db->openSet(towstr(it->output_path));


        // use this opportunity of having the set open to fill out the
        // field map, if one is not already present.  An empty field map
        // indicates that the caller desires all fields to be copied.
        // A field map that is already populated indicates that some
        // kind of transformation is happening 

        if (it->field_info.empty())
        {
            tango::IStructurePtr src_struct = src_iter->getStructure();

            if (!src_struct)
                continue;

            int i;
            int col_count = src_struct->getColumnCount();
            FieldTransInfo fs;
            wxString out_fieldname;

            // if we are appending and can locate the destination set,
            // get the output structure info from that set

            if (it->append && dest_set.isOk())
            {
                tango::IStructurePtr dest_struct = dest_set->getStructure();

                if (!dest_struct)
                    continue;

                for (i = 0; i < col_count; ++i)
                {
                    tango::IColumnInfoPtr src_colinfo;
                    tango::IColumnInfoPtr dest_colinfo;

                    src_colinfo = src_struct->getColumnInfoByIdx(i);
                    dest_colinfo = dest_struct->getColumnInfo(src_colinfo->getName());

                    // only add this field to the TransformField vector
                    // if we can locate the field in the destination set

                    if (dest_colinfo)
                    {
                        fs.input_name = towx(src_colinfo->getName());
                        fs.input_type = src_colinfo->getType();
                        fs.input_width = src_colinfo->getWidth();
                        fs.input_scale = src_colinfo->getScale();
                        fs.input_offset = src_colinfo->getOffset();
                        fs.output_name = towx(dest_colinfo->getName());
                        fs.output_type = dest_colinfo->getType();
                        fs.output_width = dest_colinfo->getWidth();
                        fs.output_scale = dest_colinfo->getScale();

                        correctFieldParams(&fs);
                        it->field_info.push_back(fs);
                    }
                }
            }
             else
            {
                for (i = 0; i < col_count; ++i)
                {
                    tango::IColumnInfoPtr src_colinfo;

                    src_colinfo = src_struct->getColumnInfoByIdx(i);

                    out_fieldname = makeValidFieldName(
                                             towx(src_colinfo->getName()),
                                             dest_db);

                    fs.input_name = towx(src_colinfo->getName());
                    fs.input_type = src_colinfo->getType();
                    fs.input_width = src_colinfo->getWidth();
                    fs.input_scale = src_colinfo->getScale();
                    fs.input_offset = src_colinfo->getOffset();
                    fs.output_name = out_fieldname;
                    fs.output_type = fs.input_type;
                    fs.output_width = fs.input_width;
                    fs.output_scale = fs.input_scale;

                    if (fs.output_scale > tango::max_numeric_scale)
                        fs.output_scale = tango::max_numeric_scale;

                    it->field_info.push_back(fs);
                }
            }
        }


        std::vector<FieldTransInfo>::iterator field_it;
        std::set<wxString>::iterator dup_check_it;
        std::set<wxString> dup_check;


        // add destination database keywords to the dup_check
        // to prevent keywords from ending up in the output field
        
        tango::IAttributesPtr attr = dest_db->getAttributes();
        if (attr.isOk())
        {
            wxStringTokenizer t(towx(attr->getStringAttribute(tango::dbattrKeywords)), wxT(","));
            while (t.HasMoreTokens())
            {
                wxString s = t.GetNextToken();
                s.MakeUpper();
                dup_check.insert(s);
            }
        }


        // now check for duplicate field names

        for (field_it = it->field_info.begin();
             field_it != it->field_info.end();
             ++field_it)
        {
            wxString s = field_it->output_name;
            s.MakeUpper();

            dup_check_it = dup_check.find(s);
            int counter = 2;
            wxString new_name;
            while (dup_check_it != dup_check.end())
            {
                // field has a duplicate name; correct it
                new_name = field_it->output_name;
                new_name += wxString::Format(wxT("%d"), counter++);
                s = new_name;
                s.MakeUpper();
                dup_check_it = dup_check.find(s);
            }

            // change the output fieldname in the vector for
            // later use in the import job
            if (!new_name.IsEmpty())
            {
                field_it->output_name = new_name;
            }

            dup_check.insert(field_it->output_name);
        }


        // clear out our progress string
        m_job_info->setProgressString(wxEmptyString);


        if (it->append && dest_set.isOk())
        {
            // if we are appending to a set and the destination set exists,
            // get the structure from that set

            dest_struct = dest_set->getStructure();
        }
         else
        {
            // create the output structure
            dest_struct = dest_db->createStructure();


            std::vector<FieldTransInfo>::iterator field_it;
            for (field_it = it->field_info.begin();
                 field_it != it->field_info.end();
                 ++field_it)
            {
                tango::IColumnInfoPtr col_info = dest_struct->createColumn();

                col_info->setName(towstr(field_it->output_name));
                col_info->setType(field_it->output_type);
                col_info->setWidth(field_it->output_width);
                col_info->setScale(field_it->output_scale);
            }

            dest_set = dest_db->createTable(towstr(it->output_path), dest_struct, NULL);
        }

        
        // if our destination set doesn't exist, bail out
        if (dest_set.isNull())
        {
            getJobInfo()->setState(jobStateFailed);
            return 0;
        }

        
        // if our source structure doesn't exist, bail out
        src_struct = src_iter->getStructure();
        if (src_struct.isNull())
        {
            getJobInfo()->setState(jobStateFailed);
            return 0;
        }


        // now that we have created the destination set,
        // insert rows into that set

        tango::IRowInserterPtr ri;
        ri = dest_db->bulkInsert(dest_set->getObjectPath());
        ri->startInsert(L"*");

        std::vector<ImportCopyInfo>::iterator copyinfo_it;
        ImportCopyInfo ci;

        // populate our copy info vector so we know what fields to import
        for (field_it = it->field_info.begin();
             field_it != it->field_info.end();
             ++field_it)
        {
            tango::IColumnInfoPtr src_colinfo;
            tango::IColumnInfoPtr dest_colinfo;

            src_colinfo = src_struct->getColumnInfo(towstr(field_it->input_name));
            dest_colinfo = dest_struct->getColumnInfo(towstr(field_it->output_name));

            // if we can't get this column's info, continue
            // looking for other columns
            if (field_it->input_name.Length() > 0 &&
                field_it->expression.length() == 0 &&
                src_colinfo.isNull())
            {
                continue;
            }

            // if we can't get this column's info, continue
            // looking for other columns
            if (dest_colinfo.isNull())
                continue;


            // if the input and output types are irreconcilable,
            // do not add it to our copy info
            if (!handleFieldInfo(&(*field_it)))
                continue;

            tango::objhandle_t src_handle;
            if (field_it->expression.length() > 0)
            {
                src_handle = src_iter->getHandle(towstr(field_it->expression));
            }
             else
            {
                if (src_colinfo.isNull())
                    continue;

                src_handle = src_iter->getHandle(src_colinfo->getName());
            }

            tango::objhandle_t dest_handle;
            dest_handle = ri->getHandle(dest_colinfo->getName());


            if (!src_handle || !dest_handle)
                continue;

            ci.src_handle = src_handle;
            ci.dest_handle = dest_handle;
            ci.dest_type = dest_colinfo->getType();
            copy_info.push_back(ci);
        }


        // insert rows in our destination set based on the
        // iterator from our source set
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
                        ri->putString(
                            copyinfo_it->dest_handle,
                            src_iter->getString(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeWideCharacter:
                    {
                        ri->putWideString(
                            copyinfo_it->dest_handle,
                            src_iter->getWideString(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeNumeric:
                    {
                        ri->putDouble(
                            copyinfo_it->dest_handle,
                            src_iter->getDouble(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeDouble:
                    {
                        ri->putDouble(
                            copyinfo_it->dest_handle,
                            src_iter->getDouble(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeInteger:
                    {
                        ri->putInteger(
                            copyinfo_it->dest_handle,
                            src_iter->getInteger(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeDateTime:
                    {
                        ri->putDateTime(
                            copyinfo_it->dest_handle,
                            src_iter->getDateTime(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeDate:
                    {
                        ri->putDateTime(
                            copyinfo_it->dest_handle,
                            src_iter->getDateTime(copyinfo_it->src_handle));
                        break;
                    }

                    case tango::typeBoolean:
                    {
                        ri->putBoolean(
                            copyinfo_it->dest_handle,
                            src_iter->getBoolean(copyinfo_it->src_handle));
                        break;
                    }
                }
            }

            m_job_info->incrementCurrentCount(1.0);

            if (!ri->insertRow())
            {
                m_job_info->setState(jobStateFailed);
                m_job_info->setProgressString(towstr(_("ERROR: Insufficient disk space")));
                return 0;
            }
          
            src_iter->skip(1);
        }

        ri->finishInsert();

        // free handles
        for (copyinfo_it = copy_info.begin();
             copyinfo_it != copy_info.end();
             ++copyinfo_it)
        {
            src_iter->releaseHandle(copyinfo_it->src_handle);
        }

        // traverse the path given and create all folders
        // in the destination database that do not exist

        std::wstring base_path = kl::beforeFirst(towstr(it->output_path), L'/');
        std::wstring remainder = kl::afterFirst(towstr(it->output_path), L'/');
        std::wstring old_remainder;

        while (1)
        {
            if (base_path.length() > 0)
            {
                bool res = dest_db->createFolder(base_path);
            }

            old_remainder = remainder;

            base_path += L'/';
            base_path += kl::beforeFirst(remainder, L'/');
            remainder = kl::afterFirst(remainder, L'/');

            // if the remainder returns the same result twice,
            // there are no more path separators

            if (!wcscasecmp(remainder.c_str(), old_remainder.c_str()))
            {
                break;
            }
        }

        // close out the tables
        src_set.clear();
        dest_set.clear();
    }

    m_job_info->setProgressString(wxT(""));
    return 0;
}

void ImportJob::runPostJob()
{
}






