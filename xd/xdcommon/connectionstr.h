/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-05
 *
 */


#ifndef H_XDCOMMON_CONNECTIONSTR_H
#define H_XDCOMMON_CONNECTIONSTR_H


namespace xdcommon
{

std::wstring urlToConnectionStr(const std::wstring& url);
std::wstring decryptConnectionStringPassword(const std::wstring& connection_str);
std::wstring encryptConnectionStringPassword(const std::wstring& connection_str);


} // namespace xdcommon


#endif

