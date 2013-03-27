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
    wxString input_name;
    int input_type;
    int input_width;
    int input_scale;
    int input_offset;    // used only by fixed-length imports

    wxString output_name;
    int output_type;
    int output_width;
    int output_scale;

    wxString expression; // optional transformation expression
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

    wxString input_path;
    wxString query;
    wxString output_path;
    bool append;

    std::vector<FieldTransInfo> field_info;

    // -- for text-delimited sets only --
    bool specify_text_params;  // whether or not the next three items should be used
    wxString delimiters;
    wxString text_qualifier;
    bool first_row_header;

    // -- for fixed-length text sets only --
    int row_width;
};




xcm_interface IImportJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IImportJob")

public:

    virtual std::vector<ImportJobInfo> getImportSets() = 0;
};

XCM_DECLARE_SMARTPTR(IImportJob)


class ImportJob : public jobs::XdJobBase,
                  public IImportJob
{
    XCM_CLASS_NAME("appmain.ImportJob")
    XCM_BEGIN_INTERFACE_MAP(ImportJob)
        XCM_INTERFACE_ENTRY(IImportJob)
        XCM_INTERFACE_CHAIN(jobs::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ImportJob();
    virtual ~ImportJob();

    void setImportType(int type);
    void setFilename(const wxString& filename);
    void setConnectionInfo(const wxString& host,
                           int port,
                           const wxString& database,
                           const wxString& username,
                           const wxString& password);
    void addImportSet(const ImportJobInfo& info);

    // -- IImportJob interface --
    std::vector<ImportJobInfo> getImportSets();
    
    int runJob();
    void runPostJob();

private:

    bool handleFieldInfo(FieldTransInfo* info);
    void correctFieldParams(FieldTransInfo* info);
    
    void updateJobTitle(const wxString& tablename);
    
    // this is a helper function to make the runJob()
    // function smaller and more readable
    tango::tango_int64_t getTotalRowCount(tango::IDatabasePtr db);

private:

    xcm::mutex m_obj_mutex;

    int m_import_type;

    wxString m_filename;
    
    wxString m_host;
    int m_port;
    wxString m_database;
    wxString m_username;
    wxString m_password;

    std::vector<ImportJobInfo> m_imports;
};




#endif



