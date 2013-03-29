/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-12-03
 *
 */


#ifndef __APP_JOBIMPORTPKG_H
#define __APP_JOBIMPORTPKG_H


#include "../jobs/jobbase.h"


class PkgStreamReader;


class PkgImportInfo
{
public:
    std::wstring stream_name;
    std::wstring output_path;
};


class ImportPkgJob : public jobs::JobBase
{
    XCM_CLASS_NAME("appmain.ImportPkgJob")
    XCM_BEGIN_INTERFACE_MAP(ImportPkgJob)
        XCM_INTERFACE_CHAIN(jobs::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ImportPkgJob();
    virtual ~ImportPkgJob();

    void setPkgFilename(const std::wstring& filename);
    void addImportObject(const std::wstring& stream_name,
                         const std::wstring& output_path);

    int runJob();
    void runPostJob();

private:

    bool importSet(PkgStreamReader* reader,
                   PkgImportInfo* info,
                   kl::xmlnode& stream_info);
                   
    bool importStream(
                   PkgStreamReader* reader,
                   PkgImportInfo* info,
                   kl::xmlnode& stream_info);

    bool importOfsFile(
                   PkgStreamReader* reader,
                   PkgImportInfo* info,
                   kl::xmlnode& stream_info);
    
    bool importMount(
                   PkgStreamReader* reader,
                   PkgImportInfo* info,
                   kl::xmlnode& stream_info);

private:

    std::wstring m_filename;
    std::vector<PkgImportInfo> m_info;
};



#endif
