/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-12-05
 *
 */



#ifndef __XDCOMMON_FORMATDEFINITION_H
#define __XDCOMMON_FORMATDEFINITION_H


std::wstring saveDefinitionToString(const xd::FormatInfo* def);
bool loadDefinitionFromString(const std::wstring& str, xd::FormatInfo* def);

bool saveDefinitionToFile(const std::wstring& path, const xd::FormatInfo* def);
bool loadDefinitionFromFile(const std::wstring& path, xd::FormatInfo* def);


#endif

