/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-12-03
 *
 */


#ifndef __APP_JOBIMPORTPKG_H
#define __APP_JOBIMPORTPKG_H


class PkgStreamReader;


class PkgImportInfo
{
public:
    wxString stream_name;
    wxString output_path;
    tango::ISetPtr output_set;
};


class ImportPkgJob : public cfw::JobBase
{
    XCM_CLASS_NAME("appmain.ImportPkgJob")
    XCM_BEGIN_INTERFACE_MAP(ImportPkgJob)
        XCM_INTERFACE_CHAIN(cfw::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    ImportPkgJob();
    virtual ~ImportPkgJob();

    void setPkgFilename(const wxString& filename);
    void addImportObject(const wxString& stream_name,
                              const wxString& output_path);

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

    wxString m_filename;
    std::vector<PkgImportInfo> m_info;
};



#endif
