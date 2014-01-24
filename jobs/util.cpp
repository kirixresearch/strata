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
#include "kl/portable.h"
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

std::wstring getMountRoot(xd::IDatabasePtr db, const std::wstring _path)
{
    std::wstring path = _path;
    size_t old_len = 0;

    while (1)
    {
        if (path.length() <= 1)
            return L"";
            
        std::wstring cstr, rpath;
        if (db->getMountPoint(path, cstr, rpath))
            return path;

        old_len = path.length();
        path = kl::beforeLast(path, '/');
        if (path.length() == old_len)
            return L"";
    }
}

xd::IIndexInfoPtr lookupIndex(xd::IIndexInfoEnumPtr idx_enum, const std::wstring& expr, bool exact_column_order)
{
    if (idx_enum.isNull())
        return xcm::null;

    std::vector<std::wstring> expr_cols;
    size_t i, idx_count = idx_enum->size();
    xd::IIndexInfoPtr result;

    kl::parseDelimitedList(expr, expr_cols, L',', true);
    
    for (i = 0; i < idx_count; ++i)
    {
        std::vector<std::wstring> idx_cols;
        xd::IIndexInfoPtr idx = idx_enum->getItem(i);

        kl::parseDelimitedList(idx->getExpression(), idx_cols, L',', true);

        if (idx_cols.size() != expr_cols.size())
            continue;

        if (exact_column_order)
        {
            int col_count = idx_cols.size();
            int j;
            bool match = true;

            for (j = 0; j < col_count; ++j)
            {
                if (0 != wcscasecmp(idx_cols[j].c_str(),
                                    expr_cols[j].c_str()))
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                return idx;
            }
        }
         else
        {
            int col_count = idx_cols.size();
            int j, k;
            bool match = true;

            for (j = 0; j < col_count; ++j)
            {
                // -- try to find it in the idx columns --
                
                bool found = false;

                for (k = 0; k < col_count; ++k)
                {
                    if (0 == wcscasecmp(idx_cols[j].c_str(),
                                        expr_cols[k].c_str()))
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    match = false;
                }

            }

            if (match)
            {
                return idx;
            }

        }
    }

    return result;
}

kl::JsonNode createJsonNodeSchema(const std::wstring& schema_definition)
{
    // note: to store the schema definition in a somewhat readable form,
    // all double quotes are represented with single quotes (since it's
    // more readable than escaping all the double quotes in the string);
    // in future compilers, it should be possible to store string literals, 
    // so hopefully this won't be necessary

    // convert single quotes to double quotes
    std::wstring schema_definition_copy = schema_definition;
    kl::replaceStr(schema_definition_copy, L"'", L"\"", true);

    kl::JsonNode schema;
    schema.fromString(schema_definition_copy);

    return schema;
}


}; // namespace jobs

