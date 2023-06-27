/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifndef H_XDDUCKDB_UTIL_H
#define H_XDDUCKDB_UTIL_H

#include "duckdbinc.h"


#define XDDUCKDB_PATH_SEPARATOR L"___"
#define XDDUCKDB_PATH_SEPARATOR_LEN 3
#define XDDUCKDB_PATH_SEPARATOR_LIKE L"\\_\\_\\_"


xd::Structure parseCreateStatement(const std::wstring& create);
std::wstring xdGetTablenameFromPath(const std::wstring& path, bool quote = false);
std::wstring xdSanitizePath(const std::wstring& path);


#endif

