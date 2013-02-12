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

unsigned int toDbType(const std::wstring& str)
{
    if (str == L"character")
        return tango::typeCharacter;
    else if (str == L"widecharacter")
        return tango::typeWideCharacter;
    else if (str == L"binary")
        return tango::typeBinary;
    else if (str == L"numeric")
        return tango::typeNumeric;
    else if (str == L"double")
        return tango::typeDouble;
    else if (str == L"integer")
        return tango::typeInteger;
    else if (str == L"date")
        return tango::typeDate;
    else if (str == L"datetime")
        return tango::typeDateTime;
    else if (str == L"boolean")
        return tango::typeBoolean;
    else
        return tango::typeUndefined;
}

std::wstring fromDbType(unsigned int type)
{
    switch (type)
    {
        default:
        case tango::typeUndefined:      return L"undefined";
        case tango::typeCharacter:      return L"character";
        case tango::typeWideCharacter:  return L"widecharacter";
        case tango::typeBinary:         return L"binary";
        case tango::typeNumeric:        return L"numeric";
        case tango::typeDouble:         return L"double";
        case tango::typeInteger:        return L"integer";
        case tango::typeDate:           return L"date";
        case tango::typeDateTime:       return L"datetime";
        case tango::typeBoolean:        return L"boolean";
    }
}


}; // namespace jobs

