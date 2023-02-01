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
#include <kl/crypt.h>


inline std::wstring getConnectionStringEncryptionKey()
{
    return std::wstring(L"jgk5]4X4$z(fq#[v8%43nFbgfer5^tnh").substr(7, 8);
}

// utility functions

bool isConnectionString(const std::wstring& str)
{
    return kl::icontains(str, L"xdprovider=");
}

std::wstring getLocationFromConnectionString(const std::wstring& location_or_cstr)
{
    if (isConnectionString(location_or_cstr))
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
    std::wstring loc1 = getLocationFromConnectionString(location1);
    std::wstring loc2 = getLocationFromConnectionString(location2);

#ifdef WIN32
    return kl::iequals(loc1, loc2);
#else
    return (loc1 == loc2 ? true : false);
#endif
}

std::wstring getDefaultConnectionStringForLocation(const std::wstring& location)
{
    if (xf_get_directory_exist(location))
    {
        std::wstring ofs_path = location;
        ofs_path += PATH_SEPARATOR_STR;
        ofs_path += L"ofs";

        if (xf_get_directory_exist(ofs_path))
        {
            return L"Xdprovider=xdnative;Database=" + location + L";User Id=admin;Password=";
        }
        else
        {
            return L"Xdprovider=xdfs;Database=" + location;
        }
    }
    else
    {
        return L"";
    }
}


static std::wstring encryptConnectionString(const std::wstring& dbcstr)
{
    if (dbcstr.length() == 0)
    {
        return L"";
    }

    xd::ConnectionString cstr(dbcstr);
    bool changed = false;

    if (cstr.getValueExist(L"Pwd"))
    {
        std::wstring pw = cstr.getValue(L"Pwd");
        if (pw.length() > 0 && !kl::isEncryptedString(pw))
        {
            pw = kl::encryptString(pw, getConnectionStringEncryptionKey());
            cstr.setValue(L"Pwd", pw);
            changed = true;
        }
    }

    if (cstr.getValueExist(L"Password"))
    {
        std::wstring pw = cstr.getValue(L"Password");
        if (pw.length() > 0 && !kl::isEncryptedString(pw))
        {
            pw = kl::encryptString(pw, getConnectionStringEncryptionKey());
            cstr.setValue(L"Password", pw);
            changed = true;
        }
    }

    if (!changed)
    {
        return dbcstr;
    }
    else
    {
        return cstr.getConnectionString();
    }
}

static std::wstring decryptConnectionString(const std::wstring& dbcstr)
{
    if (dbcstr.length() == 0)
    {
        return L"";
    }

    xd::ConnectionString cstr(dbcstr);
    bool changed = false;

    if (cstr.getValueExist(L"Pwd"))
    {
        std::wstring pw = cstr.getValue(L"Pwd");
        if (kl::isEncryptedString(pw))
        {
            pw = kl::decryptString(pw, getConnectionStringEncryptionKey());
            cstr.setValue(L"Pwd", pw);
            changed = true;
        }
    }

    if (cstr.getValueExist(L"Password"))
    {
        std::wstring pw = cstr.getValue(L"Password");
        if (kl::isEncryptedString(pw))
        {
            pw = kl::decryptString(pw, getConnectionStringEncryptionKey());
            cstr.setValue(L"Password", pw);
            changed = true;
        }
    }

    if (!changed)
    {
        return dbcstr;
    }
    else
    {
        return cstr.getConnectionString();
    }
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

    std::wstring dbname, dblocation, dbuser, dbpasswd, dbcstr;
    bool local;
    ProjectInfo c;

    // first, find out if we need to upgrade the structure; we will
    // check for the existence of a "ConnectionString" parameter
    if (project_keys.size() > 0)
    {
        config->setPath(L"/RecentDatabases/" + project_keys[0]);
        config->read(L"ConnectionString", dbcstr, L"!exist");
        if (dbcstr == L"!exist")
        {
            this->upgrade();
        }
    }


    for (auto& project_key : project_keys)
    {
        config->setPath(L"/RecentDatabases/" + project_key);

        config->read(L"Local",            &local,     true);
        config->read(L"Name",             dbname,     L"");
        config->read(L"Location",         dblocation, L"");
        config->read(L"User",             dbuser,     L"");
        config->read(L"Password",         dbpasswd,   L"");
        config->read(L"ConnectionString", dbcstr,     L"");

        if (dbpasswd.length() > 0 && kl::isEncryptedString(dbpasswd))
        {
            dbpasswd = kl::decryptString(dbpasswd, getConnectionStringEncryptionKey());
        }

        dbcstr = decryptConnectionString(dbcstr);


        c.entry_name = project_key;
        c.local = local;
        c.name = dbname;
        c.location = dblocation;
        c.user_id = dbuser;
        c.passwd = dbpasswd;
        c.connection_string = dbcstr;

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

        m_projects.push_back(c);
    }
}

void ProjectMgr::upgrade()
{
    // read all data from the sysconfig
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");

    std::vector<std::wstring> project_keys = config->getGroups();
    std::vector<std::wstring>::iterator it;

    std::wstring dbname, dblocation, dbuser, dbpasswd, dbcstr;
    bool local;
    bool need_flush = false;

    // first, find out if we need to upgrade the structure; we will
    // check for the existence of a "ConnectionString" parameter
    for (auto key : project_keys)
    {
        config->setPath(L"/RecentDatabases/" + key);

        config->read(L"Local", &local, true);
        config->read(L"Name", dbname, L"");
        config->read(L"Location", dblocation, L"");
        config->read(L"User", dbuser, L"");
        config->read(L"Password", dbpasswd, L"");
        config->read(L"ConnectionString", dbcstr, L"");

        // fix stupid bug we had
        if (dbuser == "admin" && dbpasswd == "admin")
        {
            dbpasswd = L"";
            config->write(L"Password", dbpasswd);
            need_flush = true;
        }

        if (dbpasswd.length() > 0 && !kl::isEncryptedString(dbpasswd))
        {
            dbpasswd = kl::encryptString(dbpasswd, getConnectionStringEncryptionKey());
            config->write(L"Password", dbpasswd);
            need_flush = true;
        }

        // In interim versions after 4.5.4, 'Location' used to store either a path or a
        // connection string (mixed); in 5.x this is separated; location reflects the
        // local path of the database and 'connection string'ConnectionString' is used to
        // store the connection string
        if (isConnectionString(dblocation))
        {
            dbcstr = encryptConnectionString(dblocation);
            dblocation = getLocationFromConnectionString(dbcstr);

            config->write(L"Location", dblocation);
            config->write(L"ConnectionString", dbcstr);

            need_flush = true;
        }

        if (dbcstr.empty() && !dblocation.empty())
        {
            // generate a connection string for an old version entry that
            // only had a location without a connection string
            dbcstr = L"Xdprovider=xdnative;Database=" + dblocation + L";";
            dbcstr += L"User Id=admin;Password=";
            config->write(L"ConnectionString", encryptConnectionString(dbcstr));
            need_flush = true;
        }

        if (dbname.empty() && !dblocation.empty())
        {
            // older versions used to supply the name dynamically from
            // the last portion of the path. This upgrade code makes
            // it permanent

            wxFileName fn(dblocation);
            dbname = fn.GetFullName();
            config->write(L"Name", dbname);
            need_flush = true;
        }
    }

    if (need_flush)
    {
        config->flush();
    }
}

bool ProjectMgr::addProjectEntry(const wxString& name,
                                 const wxString& location,
                                 const wxString& user_id,
                                 const wxString& password,
                                 const wxString& connection_string,
                                 bool local)
{
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");

    wxString new_connection;
    int counter = 0;
    while (1)
    {
        new_connection = wxString::Format("connection%03d", counter++);
        if (!config->exists(towstr(new_connection)))
            break;
    }

    std::wstring cstr = towstr(connection_string);
    cstr = encryptConnectionString(cstr);

    std::wstring encrypted_password = towstr(password);
    if (encrypted_password.length() > 0)
    {
        encrypted_password = kl::encryptString(encrypted_password, getConnectionStringEncryptionKey());
    }

    config->setPath(towstr(new_connection));
    config->write(L"Local", local);
    config->write(L"Name", towstr(name));
    config->write(L"Location", towstr(location));
    config->write(L"User", towstr(user_id));
    config->write(L"Password", encrypted_password);
    config->write(L"ConnectionString", towstr(connection_string));

    config->flush();

    refresh();

    return true;
}

bool ProjectMgr::modifyProjectEntry(int idx,
                                    const wxString& name,
                                    const wxString& location,
                                    const wxString& user_id,
                                    const wxString& password,
                                    const wxString& connection_string)
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
    {
        std::wstring encrypted_password = towstr(password);
        encrypted_password = kl::encryptString(encrypted_password, getConnectionStringEncryptionKey());
        config->write(wxT("Password"), encrypted_password);
    }
    
    if (connection_string.Length() > 0)
    {
        std::wstring cstr = towstr(connection_string);
        cstr = encryptConnectionString(cstr);

        config->write(wxT("ConnectionString"), cstr);
    }

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
