/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2006-12-11
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "extensionmgr.h"
#include "extensionpkg.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <kl/thread.h>


ExtensionInfo empty_info;


// -- ExtensionMgr class implementation --

ExtensionMgr::ExtensionMgr()
{
}

ExtensionMgr::~ExtensionMgr()
{
}



static bool xmlToExtensionInfo(const wxString& xml, ExtensionInfo& info)
{
    // parse meta data
    kl::xmlnode root;
    if (!root.parse((const wxChar*)xml.c_str()))
        return false;
    
    kl::xmlnode& guid = root.getChild(L"guid");
    kl::xmlnode& name = root.getChild(L"name");
    kl::xmlnode& author = root.getChild(L"author");
    kl::xmlnode& bitmap_path = root.getChild(L"bitmap");
    kl::xmlnode& startup_path = root.getChild(L"startup");
    kl::xmlnode& description = root.getChild(L"description");
    kl::xmlnode& major_version = root.getChild(L"major_version");
    kl::xmlnode& minor_version = root.getChild(L"minor_version");
    kl::xmlnode& subminor_version = root.getChild(L"subminor_version");
    kl::xmlnode& copy_on_install = root.getChild(L"copy_on_install");
    
    if (!guid.isEmpty())
    {
        info.guid = towx(guid.getNodeValue());
        info.guid.MakeUpper();
    }
        
    if (!name.isEmpty())
        info.name = towx(name.getNodeValue());
        
    if (!author.isEmpty())
        info.author = towx(author.getNodeValue());
        
    if (!bitmap_path.isEmpty())
        info.bitmap_path = towx(bitmap_path.getNodeValue());
        
    if (!startup_path.isEmpty())
        info.startup_path = towx(startup_path.getNodeValue());
        
    if (!description.isEmpty())
        info.description = towx(description.getNodeValue());
        
    if (!major_version.isEmpty())
        info.major_version = kl::wtoi(major_version.getNodeValue());
        
    if (!minor_version.isEmpty())
        info.minor_version = kl::wtoi(minor_version.getNodeValue());
        
    if (!subminor_version.isEmpty())
        info.subminor_version = kl::wtoi(subminor_version.getNodeValue());
    
    if (!copy_on_install.isEmpty())
        info.copy_on_install = (copy_on_install.getNodeValue() == L"true") ? true:false;
    
    return true;
}


ExtensionInfo ExtensionMgr::getInfo(ExtensionPkg* pkg)
{
    ExtensionInfo info;
    ExtensionInfo empty;
    
    wxInputStream* stream = pkg->getStream(wxT("info.xml"));
    if (!stream)
        return empty;
        
    wxString meta_data;
    
    if (!streamToString(stream, meta_data))
        return empty;
        
    if (!xmlToExtensionInfo(meta_data, info))
        return empty;
        

    // get the extension's bitmap from the extension package
    // based on the bitmap path specified in the info.xml file
    if (info.bitmap_path.Length() > 0)
    {
        wxInputStream* stream = pkg->getStream(info.bitmap_path);
        if (stream)
        {
            wxImage img;
            img.LoadFile(*stream);
            img.Rescale(32,32);
            wxBitmap bmp(img);
            
            if (bmp.IsOk())
            {
                info.bitmap = bmp;
            }
             else
            {
                // assign a default bitmap for the extension
                info.bitmap = GETBMP(kx_extension_32);
            }
        }
         else
        {
            // assign a default bitmap for the extension
            info.bitmap = GETBMP(kx_extension_32);
        }
    }
    
    return info;
}


ExtensionInfo ExtensionMgr::getInfo(const wxString& path)
{
    if (!::wxFileExists(path))
    {
        ExtensionInfo empty;
        return empty;
    }
    
    
    ExtensionPkg pkg;
    bool pkg_opened = pkg.open(path);
    if (pkg_opened)
    {
        // it's a package, just call our other function
        // which gets meta data and icon from a package
        return getInfo(&pkg);
    }
    
    
    // simple text file extension
    
    ExtensionInfo info;
    info.path = path;
    info.name = wxT("");


    
    wxString meta_data;

    // get meta data from text file
    wxFileInputStream fis(path);
    if (!fis.IsOk())
    {
        ExtensionInfo empty;
        return empty;
    }
    
    wxTextInputStream tis(fis);
    
    wxString text, line;
    while (1)
    {
        line = tis.ReadLine();
        text += line;
        if (fis.Eof() && line.empty())
            break;
    }
    
    int idx = text.Find(wxT("<extension_info"));
    if (idx != -1)
    {
        int end_idx = text.Find(wxT("</extension_info>"));
        if (end_idx != -1 && end_idx > idx)
        {
            meta_data = text.Mid((size_t)idx, end_idx - idx + 16);
        }
    }


    xmlToExtensionInfo(meta_data, info);

    return info;
}



ExtensionInfo& ExtensionMgr::installExtension(const wxString& path)
{
    ExtensionInfo info = getInfo(path);
    if (!info.isOk())
        return empty_info;
    
    // make sure we have a guid
    if (info.guid.Length() == 0)
        return empty_info;
    
    // make a directory for the extension
    wxString dir = g_app->getAppDataPath();
    if (dir.Length() == 0 || dir.Last() != PATH_SEPARATOR_CHAR)
        dir += PATH_SEPARATOR_CHAR;
    dir += wxT("Extensions");
    
    // make sure we have an extensions directory
    if (!xf_get_directory_exist(towstr(dir)))
        xf_mkdir(towstr(dir));
    
    dir += PATH_SEPARATOR_CHAR;
    dir += info.guid;
    
    // make sure we have a directory for this specific extension
    if (!xf_get_directory_exist(towstr(dir)))
        xf_mkdir(towstr(dir));
    
    if (!xf_get_directory_exist(towstr(dir)))
        return empty_info;
        
    // copy the extension to the extensions folder
    wxString guid = info.guid;
    guid.MakeLower();

    wxString dest_path = dir;
    dest_path += PATH_SEPARATOR_CHAR;
    dest_path += guid;
    
    if (info.copy_on_install)
    {
        wxString ext = path.AfterLast(wxT('.'));
        ext.MakeUpper();
        if (ext == L"ZIP" || ext == L"KXT")
        {
            wxLogNull log;
            
            dest_path += wxT(".kxt");
            
            // only copy extension packages to the extensions folder
            if (!::wxCopyFile(path, dest_path))
                return empty_info;
            
            ExtensionInfo& added_info = addExtension(dest_path);
            sigExtensionInstalled(added_info);
            return added_info;
        }
    }
    
    ExtensionInfo& added_info = addExtension(path);
    sigExtensionInstalled(added_info);
    return added_info;
}

ExtensionInfo& ExtensionMgr::addExtension(const wxString& path)
{
    ExtensionInfo info = getInfo(path);
    if (info.guid.IsEmpty() && info.path.IsEmpty())
        return empty_info;
    
    if (info.guid.IsEmpty())
    {
        // assign random guid;
        info.guid.Printf(wxT("%08x-%08x"), (int)time(NULL), (int)clock());
    }
    
    info.path = path;
    info.run_at_startup = true;
    
    // find out if an extension with the same guid is already installed
    // if so, then overwrite its entry
    bool found = false;
    std::vector<ExtensionInfo>::iterator it;
    for (it = m_extensions.begin(); it != m_extensions.end(); ++it)
    {
        if (info.guid.CmpNoCase(it->guid) == 0)
        {
            *it = info;
            found = true;
            break;
        }
    }
    
    if (!found)
        m_extensions.push_back(info);
    
    saveEntry(info);
    
    return getExtension(info.guid);
}

bool ExtensionMgr::enableExtension(const wxString& guid, bool enable)
{
    std::vector<ExtensionInfo>::iterator it;
    for (it = m_extensions.begin(); it != m_extensions.end(); ++it)
    {
        if (guid.CmpNoCase(it->guid) == 0)
        {
            it->run_at_startup = enable;
            saveEntry(*it);
            return true;
        }
    }
    
    return false;
}

bool ExtensionMgr::removeExtension(const wxString& guid)
{
    std::vector<ExtensionInfo>::iterator it;
    for (it = m_extensions.begin(); it != m_extensions.end(); ++it)
    {
        if (guid.CmpNoCase(it->guid) == 0)
        {
            removeEntry(*it);
            m_extensions.erase(it);
            return true;
        }
    }
    
    return false;
}

ExtensionInfo& ExtensionMgr::getExtension(const wxString& guid)
{
    std::vector<ExtensionInfo>::iterator it;
    for (it = m_extensions.begin(); it != m_extensions.end(); ++it)
    {
        if (guid.CmpNoCase(it->guid) == 0)
            return (*it);
    }
    
    return empty_info;
}

std::vector<ExtensionInfo>& ExtensionMgr::getExtensions()
{
    return m_extensions;
}

void onScriptJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobStateFinished)
        return;

    wxString script_job_guid = job->getExtraString();
    
    ExtensionMgr* ext_mgr = g_app->getExtensionMgr();
    
    std::vector<ExtensionInfo>::iterator it;
    std::vector<ExtensionInfo>& ext_info = ext_mgr->getExtensions();
    for (it = ext_info.begin(); it != ext_info.end(); ++it)
    {
        if (it->guid.CmpNoCase(script_job_guid) == 0)
        {
            it->state = ExtensionInfo::stateStopped;
            return;
        }
    }
}

bool ExtensionMgr::removeUninstalledExtensions()
{
    bool retval = true;
    
    int i, count = m_extensions.size();
    for (i = count-1; i >= 0; --i)
    {
        ExtensionInfo& info = m_extensions[i];
        if (!info.to_uninstall)
            continue;
        
        bool res = g_app->getExtensionMgr()->removeExtension(info.guid);
        if (!res)
            retval = false;
    }
    
    return retval;
}

bool ExtensionMgr::startAllExtensions()
{
    // make sure our vector is populated
    loadEntries();

    bool retval = true;

    std::vector<ExtensionInfo>::iterator it;
    for (it = m_extensions.begin(); it != m_extensions.end(); ++it)
    {
        // don't include disabled extensions at startup
        if (!it->run_at_startup)
            continue;
        
        // something is wrong with this extension
        if (!it->isOk())
        {
            retval = false;
            continue;
        }

        // the extension is already running -- its old instance
        // must be stopped before it can be instantiated again
        if (it->state == ExtensionInfo::stateRunning)
        {
            retval = false;
            continue;
        }
        
        jobs::IJobPtr job = g_app->getAppController()->executeScript(it->path);
        if (job.isOk())
        {
            jobs::IJobInfoPtr job_info = job->getJobInfo();
            if (job_info.isOk() &&
                job_info->getState() == jobStateRunning)
            {
                job->sigJobFinished().connect(&onScriptJobFinished);
                job->setExtraString(towstr(it->guid));
                it->state = ExtensionInfo::stateRunning;
            }
        }
    }

    return retval;
}

bool ExtensionMgr::stopAllExtensions()
{
    std::vector<jobs::IJobInfoPtr> job_infos_to_watch;
    
    
    IJobQueuePtr job_queue = g_app->getScriptJobQueue();
    if (job_queue.isNull())
        return false;
    
    jobs::IJobInfoEnumPtr job_info_enum = job_queue->getJobInfoEnum(jobStateRunning);
    if (job_info_enum.isNull())
        return false;
        
    std::vector<ExtensionInfo>::iterator it;
    for (it = m_extensions.begin(); it != m_extensions.end(); ++it)
    {
        bool found = false;
        
        jobs::IJobPtr job;
        jobs::IJobInfoPtr job_info;
        size_t i, job_count = job_info_enum->size();
        for (i = 0; i < job_count; ++i)
        {
            // get the job's info
            job_info = job_info_enum->getItem(i);
            if (job_info.isNull())
                continue;
                
            // lookup the job in the job queue
            job = job_queue->lookupJob(job_info->getJobId());
            if (job.isNull())
                continue;
            
            if (it->guid.CmpNoCase(job->getExtraString()) == 0)
            {
                found = true;
                break;
            }
        }
        
        
        
        if (found)
        {
            job_infos_to_watch.push_back(job_info);
            job->cancel();
        }
        
    }
    
    
    
    // wait for jobs to finish, or for 5 seconds, whichever comes first
    wxStopWatch sw;
    sw.Start();
    
    while (1)
    {
        kl::Thread::sleep(100);
        ::wxSafeYield();
        
        
        size_t finish_count = 0;
        
        std::vector<jobs::IJobInfoPtr>::iterator it;
        for (it = job_infos_to_watch.begin();
             it != job_infos_to_watch.end();
             ++it)
        {
            int job_state = (*it)->getState();
            
            if (job_state == jobStateFinished ||
                job_state == jobStateFailed ||
                job_state == jobStateCancelled)
            {
                finish_count++;
            }
        }
             
        
        if (finish_count == job_infos_to_watch.size())
        {
            // all jobs exited normally
            return true;
        }
        
        // after 5 seconds, exit even if all the jobs didn't
        // finish properly
        if (sw.Time() > 5000)
        {
            return true;
        }
    }
    
    return true;
}


bool ExtensionMgr::startExtension(const wxString& guid)
{
    ExtensionInfo& info = getExtension(guid);
    if (!info.isOk())
        return false;
    
    // the extension is already running -- its old instance
    // must be stopped before it can be instantiated again
    if (info.state == ExtensionInfo::stateRunning)
        return false;
    
    jobs::IJobPtr job = g_app->getAppController()->executeScript(info.path);
    if (job.isOk())
    {
        jobs::IJobInfoPtr job_info = job->getJobInfo();
        if (job_info.isOk() && job_info->getState() == jobStateRunning)
        {
            job->sigJobFinished().connect(&onScriptJobFinished);
            job->setExtraString(towstr(info.guid));
            info.state = ExtensionInfo::stateRunning;
            
            // fire a signal that we've started this extension
            sigExtensionStarted(info);
            return true;
        }
    }
    
    return false;
}

bool ExtensionMgr::stopExtension(const wxString& guid)
{
    IJobQueuePtr job_queue = g_app->getScriptJobQueue();
    if (job_queue.isNull())
        return false;
    
    jobs::IJobInfoEnumPtr job_info_enum = job_queue->getJobInfoEnum(jobStateRunning);
    if (job_info_enum.isNull())
        return false;
    
    jobs::IJobPtr job;
    jobs::IJobInfoPtr job_info;
    size_t i, job_count = job_info_enum->size();
    for (i = 0; i < job_count; ++i)
    {
        // get the job's info
        job_info = job_info_enum->getItem(i);
        if (job_info.isNull())
            continue;
            
        // lookup the job in the job queue
        job = job_queue->lookupJob(job_info->getJobId());
        if (job.isNull())
            continue;
        
        if (guid.CmpNoCase(job->getExtraString()) == 0)
        {
            job->cancel();
            return true;
        }
    }

    return false;
}


static bool sortByName(const ExtensionInfo& l, const ExtensionInfo& r)
{
    return l.name.CmpNoCase(r.name) > 0;
}


bool ExtensionMgr::loadEntries()
{
    m_extensions.clear();

    // read all data from the sysconfig
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/Extensions");

    std::vector<std::wstring> guids = config->getGroups();
    std::vector<std::wstring>::iterator it;
    
    std::wstring str;

    for (it = guids.begin(); it != guids.end(); ++it)
    {
        config->setPath(L"/Extensions/" + *it);
        
        ExtensionInfo info;
        info.guid = *it;
        config->read(L"Path",            str, L"");                     info.path = towx(str);
        config->read(L"Name",            str, L"");                     info.name = towx(str);
        config->read(L"Author",          str, L"");                     info.author = towx(str);
        config->read(L"Bitmap",          str, L"");                     info.bitmap_path = towx(str);
        config->read(L"Startup",         str, L"");                     info.startup_path = towx(str);
        config->read(L"Description",     str, L"");                     info.description = towx(str);
        config->read(L"MajorVersion",    &info.major_version, 0);
        config->read(L"MinorVersion",    &info.minor_version, 0);
        config->read(L"SubminorVersion", &info.subminor_version, 0);
        config->read(L"RunAtStartup",    &info.run_at_startup, 0);

        m_extensions.push_back(info);
    }


    // now sort them
    std::sort(m_extensions.begin(), m_extensions.end(), sortByName);

    return true;
}


bool ExtensionMgr::saveEntry(const ExtensionInfo& info)
{
    wxString guid = info.guid;
    guid.MakeLower();
    
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/Extensions");
    config->setPath(towstr(guid));
    
    config->write(wxT("Path"),            towstr(info.path));
    config->write(wxT("Name"),            towstr(info.name));
    config->write(wxT("Author"),          towstr(info.author));
    config->write(wxT("Bitmap"),          towstr(info.bitmap_path));
    config->write(wxT("Startup"),         towstr(info.startup_path));
    config->write(wxT("Description"),     towstr(info.description));
    config->write(wxT("MajorVersion"),    info.major_version);
    config->write(wxT("MinorVersion"),    info.minor_version);
    config->write(wxT("SubminorVersion"), info.subminor_version);
    config->write(wxT("RunAtStartup"),    info.run_at_startup);

    return true;
}

bool ExtensionMgr::removeEntry(const ExtensionInfo& info)
{
    wxString guid = info.guid;
    guid.MakeLower();
    
    IAppConfigPtr config = g_app->getAppConfig();
    return config->deleteGroup(L"/Extensions/" + towstr(guid));
}

