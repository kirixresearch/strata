/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-12-05
 *
 */


#ifndef H_XDCOMMON_FORMATDEFINITION_H
#define H_XDCOMMON_FORMATDEFINITION_H


std::wstring saveDefinitionToString(const xd::FormatDefinition& def);
bool loadDefinitionFromString(const std::wstring& str, xd::FormatDefinition* def);

bool saveDefinitionToFile(const std::wstring& path, const xd::FormatDefinition& def);
bool loadDefinitionFromFile(const std::wstring& path, xd::FormatDefinition* def);


#endif

