/*!
 *
 * Copyright (c) 2010-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2010-04-06
 *
 */


#include "appmain.h"
#include "jobfindinfiles.h"
#include "appcontroller.h"


const int MAX_STREAM_SIZE = 2000000;

FindInFilesJob::FindInFilesJob()
{
    m_job_info->setTitle(towstr(_("Find In Files")));
    m_buf = new unsigned char[MAX_STREAM_SIZE+25];
    m_whole_word = false;
    m_match_case = false;
}

FindInFilesJob::~FindInFilesJob()
{
    delete[] m_buf;
}

void FindInFilesJob::setInstructions(
                        const wxString& find_str,
                        bool match_case,
                        bool whole_word,
                        const std::vector<wxString>& paths)
{
    m_find_str = find_str;
    m_match_case = match_case;
    m_whole_word = whole_word;
    m_paths = paths;
}

void FindInFilesJob::lookInPath(xd::IDatabasePtr& db, const wxString& path)
{
    xd::IFileInfoPtr f = db->getFileInfo(towstr(path));

    if (f.isNull())
        return;

    if (f->getType() != xd::filetypeFolder)
    {
        lookInFile(db, path);
        return;
    }
    
    xd::IFileInfoEnumPtr folder_info = db->getFolderInfo(towstr(path));
    if (folder_info.isNull())
        return;
        
    wxString full_path;

    size_t i, count = folder_info->size();
    for (i = 0; i < count; ++i)
    {
        if (isCancelling())
            return;
            
        xd::IFileInfoPtr info = folder_info->getItem(i);
        if (info.isNull())
            continue;
        
        wxString filename = info->getName();
        
        // skip hidden files
        if (filename.Length() > 0 && filename.GetChar(0) == '.')
            continue;
        
        full_path = path;
        if (full_path.Length() == 0 || full_path.Last() != wxT('/'))
            full_path += wxT("/");
        full_path += filename;
        
        if (info->getType() == xd::filetypeFolder)
            lookInPath(db, full_path);
             else
            lookInFile(db, full_path);
    }
}

static unsigned char* moveNextOffset(unsigned char* offset,
                                     unsigned char* buf,
                                     unsigned long buf_size)
{
    unsigned char* np = (unsigned char*)memchr(offset, '\n', buf_size - (offset-buf));
    if (!np)
        return buf+buf_size;
         else
        return np+1;
}

void FindInFilesJob::lookInFile(xd::IDatabasePtr& db, const wxString& path)
{
    xd::IStreamPtr stream = db->openStream(towstr(path));
    if (stream.isNull())
        return;
    
    wxString test_string, upper_test_string;
    wxString upper_find_str = m_find_str;
    upper_find_str.MakeUpper();
    
    unsigned long read_count = 0;
    int line_no = 1;
    if (stream->read(m_buf, MAX_STREAM_SIZE, &read_count))
    {
        unsigned char* offset = m_buf;
        
        while (offset < m_buf+read_count)
        {
            if (isCancelling())
                return;

            unsigned char* next = moveNextOffset(offset, m_buf, read_count);
            unsigned char* end_of_string = next;
            while (end_of_string-1 >= offset && (*(end_of_string-1) == '\r' || *(end_of_string-1) == '\n'))
                end_of_string--;
                
            unsigned char tempc = *end_of_string;
            *end_of_string = 0;

            // convert to ascii
            unsigned char* p;
            for (p = offset; p < end_of_string; ++p)
            {
                if (*p > 127) *p = ' ';
            }

            test_string = (const char*)offset;
            *end_of_string = tempc;
            
            bool found = false;
            
            
            if (m_match_case)
            {
                if (test_string.Find(m_find_str) != -1)
                    found = true;
            }
             else
            {
                upper_test_string = test_string;
                upper_test_string.MakeUpper();
                
                if (upper_test_string.Find(upper_find_str) != -1)
                    found = true;
            }
            
            
            if (found)
            {
                wxString s;
                s.Printf(wxT("%s(%d):%s\n"), path, line_no, test_string);
                g_app->getAppController()->printConsoleText(s);
            }
            
            
            line_no++;
            offset = next;
        }
    }

}

int FindInFilesJob::runJob()
{
    xd::IDatabasePtr db = g_app->getDatabase();
    
    std::vector<wxString>::iterator it;
    for (it = m_paths.begin(); it != m_paths.end(); ++it)
    {
        if (isCancelling())
            return 0;

        lookInPath(db, *it);
    }
    
    return 0;
}

void FindInFilesJob::runPostJob()
{
}
