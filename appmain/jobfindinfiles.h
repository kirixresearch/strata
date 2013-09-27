/*!
 *
 * Copyright (c) 2010-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2010-04-06
 *
 */


#ifndef __APP_JOBFINDINFILES_H
#define __APP_JOBFINDINFILES_H


#include "../jobs/jobbase.h"


class FindInFilesJob : public jobs::JobBase
{
    XCM_CLASS_NAME("appmain.FindInFilesJob")
    XCM_BEGIN_INTERFACE_MAP(FindInFilesJob)
        XCM_INTERFACE_CHAIN(jobs::JobBase)
    XCM_END_INTERFACE_MAP()

public:

    FindInFilesJob();
    virtual ~FindInFilesJob();

    void setInstructions(const wxString& find_str,
                         bool match_case,
                         bool whole_word,
                         const std::vector<wxString>& paths);
    
    int runJob();
    void runPostJob();

private:

    void lookInPath(xd::IDatabasePtr& db, const wxString& path);
    void lookInFile(xd::IDatabasePtr& db, const wxString& path);
    
    std::vector<wxString> m_paths;
    wxString m_find_str;
    bool m_match_case;
    bool m_whole_word;
    
    unsigned char* m_buf;
};



#endif
