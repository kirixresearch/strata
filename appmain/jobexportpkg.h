/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-12-01
 *
 */


#ifndef __APP_JOBEXPORTPKG_H
#define __APP_JOBEXPORTPKG_H


#include "../jobs/jobbase.h"


class PkgFile;


class PkgStreamInfo
{
public:
    int file_type;         // one of the tango::filetype enums
    wxString stream_name;
    wxString src_path;
    bool compressed;
};


class ExportPkgJob : public jobs::JobBase
{
    XCM_CLASS_NAME("appmain.ExportPkgJob")
    XCM_BEGIN_INTERFACE_MAP(ExportPkgJob)
        XCM_INTERFACE_CHAIN(jobs::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    enum
    {
        modeAppend = 0,
        modeOverwrite = 1
    };

public:

    ExportPkgJob();
    virtual ~ExportPkgJob();

    void setPkgFilename(const wxString&filename,
                        int mode,
                        int version = -1);

    void addExportObject(const wxString& stream_name,
                         const wxString& src_path,
                         bool compressed);

    void setRawExport(bool b) { m_raw = b; }
    
    int runJob();
    void runPostJob();

private:

    bool writeOfsFileStream(tango::IDatabasePtr& db,
                            PkgFile* pkg,
                            PkgStreamInfo* info,
                            bool* cancelled);

    bool writeSetStream(tango::IDatabasePtr& db,
                        PkgFile* pkg,
                        PkgStreamInfo* info,
                        bool* cancelled);
                        
    bool writeStreamStream(tango::IDatabasePtr& db,
                        PkgFile* pkg,
                        PkgStreamInfo* info,
                        bool* cancelled);
    
    bool writeMountStream(tango::IDatabasePtr& db,
                        PkgFile* pkg,
                        PkgStreamInfo* info,
                        bool* cancelled);

private:

    xcm::mutex m_obj_mutex;

    wxString m_filename;
    int m_mode;
    int m_version;
    unsigned char* m_buf;
    bool m_raw;
    std::vector<PkgStreamInfo> m_info;
};








#endif
