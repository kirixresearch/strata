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


struct ProjectInfo
{
    std::wstring entry_name;
    std::wstring name;
    std::wstring location;
    std::wstring user_id;
    std::wstring passwd;
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


class ProjectMgr
{

public:

    ProjectMgr();
    ~ProjectMgr();

    bool addProjectEntry(const wxString& name,
        const wxString& location,
        const wxString& user_id,
        const wxString& password,
        bool local);
    bool modifyProjectEntry(int idx, const wxString& name,
        const wxString& location,
        const wxString& user_id,
        const wxString& password);
    bool deleteProjectEntry(int idx);

    void refresh();
    std::vector<ProjectInfo>& getProjectEntries();
    int getIdxFromEntryName(const wxString& entry_name);
    int getIdxFromLocation(const wxString& entry_name);

private:

    std::vector<ProjectInfo> m_projects;
};



#endif

