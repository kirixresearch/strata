/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-05
 *
 */


#ifndef __JOBS_UTIL_H
#define __JOBS_UTIL_H


#include <vector>


namespace jobs
{


void vectorToDelimitedString(const std::vector<std::wstring>& vec, std::wstring& str, wchar_t delimiter = ',');

std::wstring getMountRoot(tango::IDatabasePtr db, const std::wstring _path);
tango::IIndexInfoPtr lookupIndex(tango::IIndexInfoEnumPtr idx_enum, const std::wstring& expr, bool exact_column_order);


};  // namespace jobs


#endif
