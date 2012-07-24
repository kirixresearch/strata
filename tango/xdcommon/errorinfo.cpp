/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2004-09-02
 *
 */


#include "tango.h"
#include "errorinfo.h"


static std::wstring empty_wstring = L"";

void ThreadErrorInfo::clearError()
{
    XCM_AUTO_LOCK(m_mutex);

    xcm::threadid_t thread_id = xcm::get_current_thread_id();
    std::vector<ErrInfo>::iterator it;

    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        if (it->thread_id == thread_id)
        {
            m_info.erase(it);
            return;
        }
    }

}

void ThreadErrorInfo::setError(int error_code)
{
    // in the future this will set default text for the
    // given error code
    setError(error_code, L"");
}

void ThreadErrorInfo::setError(int error_code, const std::wstring& error_string)
{
    XCM_AUTO_LOCK(m_mutex);

    xcm::threadid_t thread_id = xcm::get_current_thread_id();
    std::vector<ErrInfo>::iterator it;

    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        if (it->thread_id == thread_id)
        {
            it->error_code = error_code;
            it->error_string = error_string;
            return;
        }
    }

    ErrInfo i;
    i.error_code = error_code;
    i.error_string = error_string;
    i.thread_id = thread_id;
    m_info.push_back(i);
}

int ThreadErrorInfo::getErrorCode()
{
    XCM_AUTO_LOCK(m_mutex);

    xcm::threadid_t thread_id = xcm::get_current_thread_id();
    std::vector<ErrInfo>::iterator it;
    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        if (it->thread_id == thread_id)
            return it->error_code;
    }

    return tango::errorNone;
}

bool ThreadErrorInfo::isError()
{
    int code = getErrorCode();
    if (code == tango::errorNone)
        return false;
    return true;
}

const std::wstring& ThreadErrorInfo::getErrorString()
{
    XCM_AUTO_LOCK(m_mutex);

    xcm::threadid_t thread_id = xcm::get_current_thread_id();
    std::vector<ErrInfo>::iterator it;
    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        if (it->thread_id == thread_id)
            return it->error_string;
    }

    return empty_wstring;
}

