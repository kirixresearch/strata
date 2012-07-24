/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2006-12-11
 *
 */


#ifndef __APP_EXTENSIONMGR_H
#define __APP_EXTENSIONMGR_H


class ExtensionPkg;

class ExtensionInfo
{
public:

    enum
    {
        stateStopped = 0,
        stateRunning = 1
    };
    
    ExtensionInfo()
    {
        guid = wxEmptyString;
        name = wxEmptyString;
        author = wxEmptyString;
        description = wxEmptyString;
        path = wxEmptyString;
        startup_path = wxEmptyString;
        bitmap_path = wxEmptyString;
        bitmap = wxNullBitmap;
        
        major_version = 1;
        minor_version = 0;
        subminor_version = 0;
        copy_on_install = true;
        
        run_at_startup = true;
        to_uninstall = false;
        state = stateStopped;
    }

    ExtensionInfo(const ExtensionInfo& c)
    {
        guid = c.guid;
        name = c.name;
        author = c.author;
        description = c.description;
        path = c.path;
        startup_path = c.startup_path;
        bitmap_path = c.bitmap_path;
        bitmap = c.bitmap;
        
        major_version = c.major_version;
        minor_version = c.minor_version;
        subminor_version = c.subminor_version;
        copy_on_install = c.copy_on_install;
        
        run_at_startup = c.run_at_startup;
        to_uninstall = c.to_uninstall;
        state = c.state;
    }

    ExtensionInfo& operator=(const ExtensionInfo& c)
    {    
        guid = c.guid;
        name = c.name;
        author = c.author;
        description = c.description;
        path = c.path;
        startup_path = c.startup_path;
        bitmap_path = c.bitmap_path;
        bitmap = c.bitmap;
        
        major_version = c.major_version;
        minor_version = c.minor_version;
        subminor_version = c.subminor_version;
        copy_on_install = c.copy_on_install;

        run_at_startup = c.run_at_startup;
        to_uninstall = c.to_uninstall;
        state = c.state;
        return *this;
    }

    ~ExtensionInfo()
    {
    }
    
    bool isOk() { return guid.IsEmpty() ? false : true; }
    
public:
    
    wxString guid;
    wxString name;
    wxString author;
    wxString description;
    wxString path;          // filename of the extension in the filesystem
    wxString startup_path;  // javascript startup file in the extension package
    wxString bitmap_path;   // path of the bitmap in the extension package
    wxBitmap bitmap;        // bitmap to show in the extension manager panel
    
    long major_version;
    long minor_version;
    long subminor_version;
    bool copy_on_install;   // true if extension should be copied when installed
    
    bool run_at_startup;
    bool to_uninstall;
    int state;
};


class ExtensionMgr
{
public:

    ExtensionMgr();
    ~ExtensionMgr();

    ExtensionInfo getInfo(ExtensionPkg* pkg);
    ExtensionInfo getInfo(const wxString& path);
    
    ExtensionInfo& addExtension(const wxString& filename);
    ExtensionInfo& installExtension(const wxString& filename);
    ExtensionInfo& getExtension(const wxString& guid);
    bool enableExtension(const wxString& guid, bool enable = true);
    bool removeExtension(const wxString& guid);
    
    std::vector<ExtensionInfo>& getExtensions();
    
    bool removeUninstalledExtensions();
    bool startAllExtensions();
    bool stopAllExtensions();
    
    bool startExtension(const wxString& guid);
    bool stopExtension(const wxString& guid);

    // signals
    xcm::signal1<ExtensionInfo&> sigExtensionInstalled;
    xcm::signal1<ExtensionInfo&> sigExtensionStarted;
    
private:

    bool loadEntries();
    bool saveEntry(const ExtensionInfo& info);
    bool removeEntry(const ExtensionInfo& info);
    
private:

    std::vector<ExtensionInfo> m_extensions;
};




#endif

