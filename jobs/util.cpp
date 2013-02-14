/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams
 * Created:  2013-02-05
 *
 */


#include "jobspch.h"
#include "util.h"


namespace jobs
{


void vectorToDelimitedString(const std::vector<std::wstring>& vec, std::wstring& str, wchar_t delimiter)
{
    str.clear();

    std::vector<std::wstring>::const_iterator it, it_end;
    it_end = vec.end();

    bool first = true;
    for (it = vec.begin(); it != it_end; ++it)
    {
        if (!first)
            str += delimiter;

        first = false;
        str += *it;
    }
}


}; // namespace jobs

