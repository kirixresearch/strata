/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-07-09
 *
 */


#include "appmain.h"
#include "dlgprojectmgr.h"
#include "appcontroller.h"
#include "paneloptions.h"
#include <wx/dir.h>


// utility functions

static std::wstring getLocationFromString(const std::wstring& location_or_cstr)
{
    if (kl::icontains(location_or_cstr, L"xdprovider="))
    {
        xd::ConnectionString cstr(location_or_cstr);
        std::wstring provider = cstr.getLowerValue(L"xdprovider");

        if (provider == L"xdnative" || provider == L"xdfs")
        {
            return cstr.getLowerValue(L"database");
        }

        return L"";
    }
    else
    {
        return location_or_cstr;
    }
}

static bool isSameLocation(const std::wstring& location1, std::wstring& location2)
{
    std::wstring loc1 = getLocationFromString(location1);
    std::wstring loc2 = getLocationFromString(location2);

#ifdef WIN32
    return kl::iequals(loc1, loc2);
#else
    return (loc1 == loc2 ? true : false);
#endif
}



// ProjectMgr class implementation

ProjectMgr::ProjectMgr()
{
    refresh();
}

ProjectMgr::~ProjectMgr()
{
}

void ProjectMgr::refresh()
{
    m_projects.clear();

    // read all data from the sysconfig
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");

    std::vector<std::wstring> project_keys = config->getGroups();
    std::vector<std::wstring>::iterator it;


    std::wstring dbname, dblocation, dbuser, dbpasswd;
    bool local;
    ProjectInfo c;

    for (it = project_keys.begin(); it != project_keys.end(); ++it)
    {
        config->setPath(L"/RecentDatabases/" + *it);

        config->read(L"Local",    &local,     true);
        config->read(L"Name",     dbname,     L"");
        config->read(L"Location", dblocation, L"");
        config->read(L"User",     dbuser,     L"");
        config->read(L"Password", dbpasswd,   L"");

        c.entry_name = *it;
        c.local = local;
        c.name = dbname;
        c.location = dblocation;
        c.user_id = dbuser;
        c.passwd = dbpasswd;

        if (c.name.length() == 0)
        {
            std::wstring lowercase_loc = c.location;
            kl::makeLower(lowercase_loc);
            
            if (lowercase_loc.find(L"xdprovider") == lowercase_loc.npos)
            {
                c.name = c.location;
                size_t idx = c.name.find_last_of(PATH_SEPARATOR_CHAR);
                if (idx != c.name.npos)
                    c.name = c.name.substr(idx+1);
            }
        }
        
        
        // fix stupid bug we had
        if (c.user_id == "admin" && c.passwd == "admin")
            c.passwd = wxT("");

        m_projects.push_back(c);
    }
}

bool ProjectMgr::addProjectEntry(const wxString& name,
                                 const wxString& location,
                                 const wxString& user_id,
                                 const wxString& password,
                                 bool local)
{
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");

    wxString new_connection;
    int counter = 0;
    while (1)
    {
        new_connection = wxString::Format(wxT("connection%03d"), counter++);
        if (!config->exists(towstr(new_connection)))
            break;
    }

    config->setPath(towstr(new_connection));
    config->write(wxT("Local"), local);
    config->write(wxT("Name"), towstr(name));
    config->write(wxT("Location"), towstr(location));
    config->write(wxT("User"), towstr(user_id));
    config->write(wxT("Password"), towstr(password));

    config->flush();

    refresh();

    return true;
}

bool ProjectMgr::modifyProjectEntry(int idx,
                                    const wxString& name,
                                    const wxString& location,
                                    const wxString& user_id,
                                    const wxString& password)
{
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");
    config->setPath(m_projects[idx].entry_name);

    if (name.Length() > 0)
        config->write(wxT("Name"), towstr(name));

    if (location.Length() > 0)
        config->write(wxT("Location"), towstr(location));

    if (user_id.Length() > 0)
        config->write(wxT("User"), towstr(user_id));

    if (password.Length() > 0)
        config->write(wxT("Password"), towstr(password));
    
    refresh();

    return true;
}

bool ProjectMgr::deleteProjectEntry(int idx)
{
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");
    config->deleteGroup(m_projects[idx].entry_name);

    refresh();

    return true;
}

std::vector<ProjectInfo>& ProjectMgr::getProjectEntries()
{
    return m_projects;
}

int ProjectMgr::getIdxFromEntryName(const wxString& entry_name)
{
    int idx = 0;
    
    std::vector<ProjectInfo>::iterator it;
    for (it = m_projects.begin(); it != m_projects.end(); ++it)
    {
        if (it->entry_name == entry_name)
            return idx;
        idx++;
    }

    return -1;
}

int ProjectMgr::getIdxFromLocation(const wxString& _location)
{
    int idx = 0;
    
    std::wstring location = towstr(_location);

    std::vector<ProjectInfo>::iterator it;
    for (it = m_projects.begin(); it != m_projects.end(); ++it)
    {
        if (isSameLocation(location, it->location))
            return idx;
        idx++;
    }

    return -1;
}
