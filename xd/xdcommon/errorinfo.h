/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2004-09-02
 *
 */


#ifndef __XDCOMMON_ERRORINFO_H
#define __XDCOMMON_ERRORINFO_H


struct ErrInfo
{
    xcm::threadid_t thread_id;
    int error_code;
    std::wstring error_string;
};


class ThreadErrorInfo
{
public:

    void setError(int error_code);
    void setError(int error_code, const std::wstring& value);
    void clearError();
    
    int getErrorCode();
    bool isError();
    const std::wstring& getErrorString();

private:

    xcm::mutex m_mutex;

    std::vector<ErrInfo> m_info;
};




#endif

