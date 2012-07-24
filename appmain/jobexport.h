/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2004-01-06
 *
 */


#ifndef __APP_JOBEXPORT_H
#define __APP_JOBEXPORT_H


struct ExportJobInfo
{
public:
    wxString input_path;
    wxString output_path;
    bool append;
};


struct ExportCopyInfo
{
    wxString src_name;
    wxString dest_name;
    int dest_type;
    tango::objhandle_t src_handle;
    tango::objhandle_t dest_handle;
};


class ExportJob : public cfw::JobBase
{
    XCM_CLASS_NAME("appmain.ExportJob")
    XCM_BEGIN_INTERFACE_MAP(ExportJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ExportJob();
    virtual ~ExportJob();
    
    void setExportType(int type);
    void setFixInvalidFieldnames(bool val);
    void setFilename(const wxString& filename,
                     bool overwrite = true);
    void setConnectionInfo(
                     const wxString& host,
                     int port,
                     const wxString& database,
                     const wxString& username,
                     const wxString& password);

    void addExportSet(const ExportJobInfo& info);

    // additional parameters for text-delimited export
    void setDelimiters(const wxString& delimiters);
    void setTextQualifier(const wxString& text_qualifier);
    void setFirstRowHeader(bool first_row_header);
    
    int runJob();
    void runPostJob();

private:

    void updateJobTitle(ExportJobInfo* info);

private:

    xcm::mutex m_obj_mutex;

    int m_export_type;

    wxString m_filename;
    bool m_overwrite_file;
    bool m_fix_invalid_fieldnames;
    
    wxString m_host;
    int m_port;
    wxString m_database;
    wxString m_username;
    wxString m_password;

    // text-delimited parameters
    wxString m_delimiters;
    wxString m_text_qualifier;
    bool m_first_row_header;

    std::vector<ExportJobInfo> m_exports;
};




#endif



