/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2004-01-06
 *
 */


#ifndef __APP_JOBEXPORT_H
#define __APP_JOBEXPORT_H


#include "../jobs/jobbase.h"


struct ExportJobInfo
{
public:
    std::wstring input_path;
    std::wstring output_path;
    bool append;
};


struct ExportCopyInfo
{
    std::wstring src_name;
    std::wstring dest_name;
    int dest_type;
    tango::objhandle_t src_handle;
    tango::objhandle_t dest_handle;
};


class ExportJob : public jobs::JobBase
{
    XCM_CLASS_NAME("appmain.ExportJob")
    XCM_BEGIN_INTERFACE_MAP(ExportJob)
        XCM_INTERFACE_CHAIN(jobs::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ExportJob();
    virtual ~ExportJob();
    
    void setExportType(int type);
    void setFixInvalidFieldnames(bool val);
    void setFilename(const std::wstring& filename,
                     bool overwrite = true);
    void setConnectionInfo(
                     const std::wstring& host,
                     int port,
                     const std::wstring& database,
                     const std::wstring& username,
                     const std::wstring& password);

    void addExportSet(const ExportJobInfo& info);

    // additional parameters for text-delimited export
    void setDelimiters(const std::wstring& delimiters);
    void setTextQualifier(const std::wstring& text_qualifier);
    void setFirstRowHeader(bool first_row_header);
    
    int runJob();
    void runPostJob();

private:

    void updateJobTitle(ExportJobInfo* info);

private:

    xcm::mutex m_obj_mutex;

    int m_export_type;

    std::wstring m_filename;
    bool m_overwrite_file;
    bool m_fix_invalid_fieldnames;
    
    std::wstring m_host;
    int m_port;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;

    // text-delimited parameters
    std::wstring m_delimiters;
    std::wstring m_text_qualifier;
    bool m_first_row_header;

    std::vector<ExportJobInfo> m_exports;
};




#endif



