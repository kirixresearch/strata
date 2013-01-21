/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-07-23
 *
 */


#ifndef __APP_EXTENSIONPKG_H
#define __APP_EXTENSIONPKG_H


class wxMemoryInputStream;
class wxFileInputStream;
class wxZipInputStream;
class wxZipEntry;


bool streamToString(wxInputStream* stream, wxString& res_string);


class ExtensionStream
{
public:
    wxString m_name;
    wxMemoryBuffer m_buf;
};

class ExtensionPkg
{
public:

    ExtensionPkg();
    ~ExtensionPkg();

    bool open(const wxString& filename);
    wxInputStream* getStream(const wxString& stream_name);
    bool getStreamExists(const wxString& stream_name);
    
public:

    xcm::mutex m_obj_mutex;
    std::vector<ExtensionStream*> m_streams;
    std::vector<wxMemoryInputStream*> m_to_delete;
    
    wxZipInputStream* m_zip;
    wxInputStream* m_file;
    std::vector<wxZipEntry*> m_zip_entries;
};




#endif

