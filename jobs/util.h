/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-05
 *
 */


#ifndef H_JOBS_UTIL_H
#define H_JOBS_UTIL_H


#include <vector>


namespace jobs
{


void vectorToDelimitedString(const std::vector<std::wstring>& vec, std::wstring& str, wchar_t delimiter = ',');

std::wstring getMountRoot(xd::IDatabasePtr db, const std::wstring _path);
xd::IndexInfo lookupIndex(const xd::IndexInfoEnum& idx_enum, const std::wstring& expr, bool exact_column_order);

kl::JsonNode createJsonNodeSchema(const std::wstring& schema_definition);


};  // namespace jobs


#endif

