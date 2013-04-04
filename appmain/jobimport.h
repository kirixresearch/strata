/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-12-10
 *
 */


#ifndef __APP_JOBIMPORT_H
#define __APP_JOBIMPORT_H


#include "../jobs/jobbase.h"
#include "../jobs/xdjobbase.h"


struct FieldTransInfo
{
    std::wstring input_name;
    int input_type;
    int input_width;
    int input_scale;
    int input_offset;    // used only by fixed-length imports

    std::wstring output_name;
    int output_type;
    int output_width;
    int output_scale;

    std::wstring expression; // optional transformation expression
};


struct ImportJobInfo
{
public:

    ImportJobInfo()
    {
        append = false;
        first_row_header = true;
        specify_text_params = true;
        row_width = 0;
    }
    
    ImportJobInfo(const ImportJobInfo& c)
    {
        input_path = c.input_path;
        query = c.query;
        output_path = c.output_path;
        append = c.append;
        field_info = c.field_info;
        delimiters = c.delimiters;
        text_qualifier = c.text_qualifier;
        first_row_header = c.first_row_header;
        row_width = c.row_width;
        specify_text_params = c.specify_text_params;
    }
    
    ImportJobInfo& operator=(const ImportJobInfo& c)
    {
        input_path = c.input_path;
        query = c.query;
        output_path = c.output_path;
        append = c.append;
        field_info = c.field_info;
        delimiters = c.delimiters;
        text_qualifier = c.text_qualifier;
        first_row_header = c.first_row_header;
        row_width = c.row_width;
        specify_text_params = c.specify_text_params;
        return *this;
    }

public:

    std::wstring input_path;
    std::wstring query;
    std::wstring output_path;
    bool append;

    std::vector<FieldTransInfo> field_info;

    // for text-delimited sets only
    bool specify_text_params;  // whether or not the next three items should be used
    std::wstring delimiters;
    std::wstring text_qualifier;
    bool first_row_header;

    // for fixed-length text sets only
    int row_width;
};


class ImportJob : public jobs::XdJobBase
{
    XCM_CLASS_NAME("appmain.ImportJob")
    XCM_BEGIN_INTERFACE_MAP(ImportJob)
        XCM_INTERFACE_CHAIN(jobs::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ImportJob();
    virtual ~ImportJob();

    void setImportType(int type);
    void setFilename(const std::wstring& filename);
    void setConnectionInfo(const std::wstring& host,
                           int port,
                           const std::wstring& database,
                           const std::wstring& username,
                           const std::wstring& password);
    void addImportSet(const ImportJobInfo& info);

    std::vector<ImportJobInfo> getImportSets();
    
    int runJob();
    void runPostJob();

private:

    bool handleFieldInfo(FieldTransInfo* info);
    void correctFieldParams(FieldTransInfo* info);
    
    void updateJobTitle(const std::wstring& tablename);
    
    // this is a helper function to make the runJob()
    // function smaller and more readable
    long long getTotalRowCount(tango::IDatabasePtr db);

private:

    xcm::mutex m_obj_mutex;

    int m_import_type;

    std::wstring m_filename;
    
    std::wstring m_host;
    int m_port;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;

    std::vector<ImportJobInfo> m_imports;
};




#endif



