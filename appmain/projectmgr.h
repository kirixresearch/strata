/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2023-01-17
 *
 */


#ifndef __APP_PROJECTMGR_H
#define __APP_PROJECTMGR_H


class ProjectInfo
{
public:
    ProjectInfo()
    {
        local = true;
    }

    ProjectInfo(const ProjectInfo& c)
    {
        entry_name = c.entry_name;
        name = c.name;
        location = c.location;
        user_id = c.user_id;
        passwd = c.passwd;
        connection_string = c.connection_string;
        local = c.local;
    }

    ProjectInfo& operator=(const ProjectInfo& c)
    {
        entry_name = c.entry_name;
        name = c.name;
        location = c.location;
        user_id = c.user_id;
        passwd = c.passwd;
        connection_string = c.connection_string;
        local = c.local;
        return *this;
    }

    bool isOk() const
    {
        return !entry_name.empty();
    }
    
public:
    std::wstring entry_name;
    std::wstring name;
    std::wstring location;
    std::wstring user_id;
    std::wstring passwd;
    std::wstring connection_string;
    bool local;
};


class ProjectInfoLess
{
public:

    bool operator()(const ProjectInfo& x,
        const ProjectInfo& y) const
    {
        return wcscasecmp(x.name.c_str(), y.name.c_str()) < 0 ? true : false;
    }
};

bool isConnectionString(const std::wstring& str);
std::wstring getLocationFromConnectionString(const std::wstring& location_or_cstr);
std::wstring getDefaultConnectionStringForLocation(const std::wstring& location);

class ProjectMgr
{

public:

    ProjectMgr();
    ~ProjectMgr();

    bool addProjectEntry(const wxString& name,
        const wxString& location,
        const wxString& user_id,
        const wxString& password,
        const wxString& connection_string,
        bool local);

    bool modifyProjectEntry(int idx,
        const wxString& name,
        const wxString& location,
        const wxString& user_id,
        const wxString& password,
        const wxString& connection_string
    );

    bool deleteProjectEntry(int idx);

    void refresh();
    std::vector<ProjectInfo>& getProjectEntries();
    int getIdxFromEntryName(const wxString& entry_name);
    int getIdxFromLocation(const wxString& entry_name);

private:

    void upgrade();

private:

    std::vector<ProjectInfo> m_projects;
};



#endif

