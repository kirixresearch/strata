/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-06
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/portable.h>
#include <kl/string.h>
#include <kl/regex.h>
#include <xd/xd.h>
#include "xdcommonsql.h"
#include "../xdcommon/util.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/keylayout.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/cmndynamicset.h"
#include "../xdcommon/errorinfo.h"
#include "../xdcommon/dbfuncs.h"
#include "../../kscript/kscript.h"
#include "../../kscript/functions.h"


enum
{
    joinNone = 0,
    joinInner,
    joinLeftOuter,
    joinRightOuter,
    joinFullOuter
};


struct SelectField
{
    std::wstring name;
    std::wstring expr;
    bool star;          // true if entry was added
                        // because star was encountered
};

struct OrderByField
{
    std::wstring name;
    bool descending;
};

struct SourceTable
{
    std::wstring path;
    xd::Structure structure;
    std::wstring alias;
    std::wstring join_expr;
    int join_type;
};


struct JoinField
{
    std::wstring name;
    std::wstring alias;
    std::wstring orig_name;
    int type;
    int width;
    int scale;

    SourceTable* source_table;
    xd::IIteratorPtr source_iter;
    xd::objhandle_t source_handle;
    xd::objhandle_t dest_handle;
};


struct JoinInfo
{
    std::wstring left_path;
    std::wstring right_path;
    xd::IIteratorPtr right_iter;
    KeyLayout left_key;
    KeyLayout right_key;
    std::wstring left;
    std::wstring right;

    int join_type;
};


static bool isDelimiterChar(wchar_t ch)
{
    if (!ch) return true;
    if (wcschr(L"+-*/%:;|\\()<>,!='\"\t ", ch))
        return true;
    return false;
}


static void dequoteField(std::wstring& str)
{
    const wchar_t* pstr = str.c_str();
    const wchar_t* period = zl_strchr((wchar_t*)pstr, '.', L"[", L"]");
    
    if (!period)
    {
        dequote(str, L'[', L']');
        return;
    }
    
    int period_pos = -1;
    if (period)
        period_pos = period - pstr;
    
    std::wstring alias = str.substr(0, period_pos);
    std::wstring field = str.substr(period_pos+1);
    
    dequote(alias, L'[', L']');
    dequote(field, L'[', L']');

    str = alias + L"." + field;
}

static void quoteField(std::wstring& str)
{
    int period_pos = str.find('.');
    if (period_pos == -1)
    {
        str = L"[" + str + L"]";
        return;
    }

    std::wstring alias = str.substr(0, period_pos);
    std::wstring field = str.substr(period_pos+1);
    
    str = L"[" + alias + L"].[" + field + L"]";
}


static bool isSamePath(const std::wstring& path1, const std::wstring& path2)
{
    std::wstring s1 = path1, s2 = path2;
    kl::makeLower(s1);
    kl::makeLower(s2);
    if (s1.length() > 0 && s1[0] == '/')
        s1.erase(0,1);
    if (s2.length() > 0 && s2[0] == '/')
        s2.erase(0,1);

    return (s1 == s2) ? true : false;
}

static bool isSameField(const std::wstring& f1, const std::wstring& f2)
{
    if (0 == wcscasecmp(f1.c_str(), f2.c_str()))
        return true;

    std::wstring df1 = f1, df2 = f2;
    
    dequoteField(df1);
    dequoteField(df2);

    return (0 == wcscasecmp(df1.c_str(), df2.c_str()) ? true : false);
}

static bool isFunction(const std::wstring& str, const std::wstring& func)
{
    std::wstring s = str;

    kl::trimLeft(s);

    int len = func.length();

    if (wcsncasecmp(str.c_str(), func.c_str(), len) != 0)
        return false;

    return (iswspace(*(str.c_str()+len)) ||
            *(str.c_str()+len) == L'(') ? true : false;
}

static bool isGroupFunction(const std::wstring& str)
{
    return (isFunction(str, L"FIRST") ||
            isFunction(str, L"LAST") ||
            isFunction(str, L"SUM") ||
            isFunction(str, L"AVG") ||
            isFunction(str, L"MIN") ||
            isFunction(str, L"MAX") ||
            isFunction(str, L"MAXDISTANCE") ||
            isFunction(str, L"COUNT") ||
            isFunction(str, L"STDDEV") ||
            isFunction(str, L"VARIANCE") ||
            isFunction(str, L"GROUPID") ||
            isFunction(str, L"MERGE")) ? true : false;
}

static bool isUniqueFieldName(std::vector<SourceTable>& s,
                              const std::wstring& _field_name)
{
    std::wstring field_name = _field_name;
    dequoteField(field_name);


    bool e = false;

    std::vector<SourceTable>::iterator it;
    for (it = s.begin(); it != s.end(); ++it)
    {
        if (it->structure.getColumnExist(field_name))
        {
            if (e)
                return false;
            e = true;
        }
    }

    return true;
}


static wchar_t* zl_strrchr(wchar_t* str,
                           wchar_t ch,
                           const wchar_t* open_parens,
                           const wchar_t* close_parens)
{
    wchar_t* ret = NULL;
    wchar_t* p = str;
    
    while (1)
    {
        p = zl_strchr(p, ch, open_parens, close_parens);
        if (!p)
            break;

        ret = p;
        p++;
    }
    
    return ret;
}


static xd::ColumnInfo getColumnInfoMulti(std::vector<SourceTable>& s,
                                         const std::wstring& field_name,
                                         SourceTable** tbl = NULL)
{
    if (!isUniqueFieldName(s, field_name))
        return xd::ColumnInfo();

    std::wstring alias_part;
    std::wstring field_part;
    
    const wchar_t* str = field_name.c_str();
    const wchar_t* period = zl_strrchr((wchar_t*)str, '.', L"[", L"]");
    
    int period_pos = -1;
    if (period)
        period_pos = period - str;

    if (period_pos != -1)
    {
        alias_part = field_name.substr(0, period_pos);
        field_part = field_name.substr(period_pos+1);
    }
     else
    {
        field_part = field_name;
        alias_part = L"";
    }

    dequote(alias_part, L'"', L'"');
    dequote(field_part, L'"', L'"');
    dequote(alias_part, L'[', L']');
    dequote(field_part, L'[', L']');


    std::vector<SourceTable>::iterator it;

    // first check if the field name requested has an exact match;
    // This handles the case where a table
    // has field names with a period in them
    if (period_pos != -1)
    {
        for (it = s.begin(); it != s.end(); ++it)
        {
            const xd::ColumnInfo& col_info = it->structure.getColumnInfo(field_name);
            if (col_info.isOk())
            {
                if (tbl)
                    *tbl = &(*it);

                return col_info;
            }
        }
    }

    // now check only those tables where the alias matches
    for (it = s.begin(); it != s.end(); ++it)
    {
        if (!alias_part.empty())
        {
            if (!kl::iequals(alias_part, it->alias))
                continue;
        }
        
        const xd::ColumnInfo& col_info = it->structure.getColumnInfo(field_part);
        if (col_info.isOk())
        {
            if (tbl)
                *tbl = &(*it);

            return col_info;
        }
    }

    return xd::ColumnInfo();
}



static std::wstring getAlias(const std::wstring& path)
{
    std::wstring s = path;
    kl::trim(s);

    // to generate an alias, take the portion after the
    // last slash.  Then, if there's a period, take what
    // is before it.  This is needed for SQL using filenames
    // with extensions
    
    
    //return kl::beforeLast(kl::afterLast(path, L'/'), L'.');
    return kl::afterLast(path, L'/');
}


static void parseTableAndAlias(const std::wstring& input,
                               std::wstring& table,
                               std::wstring& alias)
{
    const wchar_t* input_cstr = input.c_str();
    const wchar_t* temp = zl_stristr((wchar_t*)input_cstr,
                               L"AS",
                               true,
                               false);
    int as_pos = -1;
    if (temp)
        as_pos = (temp - input_cstr);

    if (as_pos != -1)
    {        
        table = input.substr(0, as_pos);
        alias = input.substr(as_pos+2);

        kl::trim(table);
        kl::trim(alias);
    }
     else
    {
        const wchar_t* space = zl_strchr((wchar_t*)input_cstr, ' ', L"[", L"]");
        if (space)
        {
            int space_pos = space-input_cstr;
            table = input.substr(0, space_pos);
            alias = input.substr(space_pos);
        }
         else
        {
            table = input;
            alias = L"";
        }
    }
    
    kl::trim(table);
    kl::trim(alias);
    
    dequote(table, L'"', L'"');
    dequote(alias, L'"', L'"');
    dequote(table, L'[', L']');
    dequote(alias, L'[', L']');


    if (alias.empty())
        alias = getAlias(table);  // generate alias
}

static std::wstring exprReplaceToken(const std::wstring& str,
                                     const std::wstring& search,
                                     const std::wstring& replace,
                                     bool case_sensitive = false)
{
    std::wstring result;


    const wchar_t* str_cstr = str.c_str();
    const wchar_t* s = str_cstr;
    int search_len = search.length();
    int str_len = str.length();
    wchar_t quote_char = 0;
    
    result.reserve(str_len);

    while (*s)
    {
        if (quote_char == *s)
        {
            quote_char = 0;
        }
         else
        {
            if (*s == L'\'' || *s == L'\"')
            {
                quote_char = *s;
            }
        }

        if (quote_char)
        {
            result += *s;
            s++;
            continue;
        }

        // if the remaining chunk of the string is shorter than
        // the search string, no match is found, and we're done
        
        if (str_len - (s-str_cstr) < search_len)
        {
            result += s;
            break;
        }

        // check for delimiter characters at the beginning
        if (s > str_cstr && isDelimiterChar(*s))
        {
            result += *s;
            s++;
            continue;
        }

        // last character must be either a delimiter character
        // or the end of the string
        if (*(s+search_len) && !isDelimiterChar(*(s+search_len)))
        {
            result += *s;
            s++;
            continue;
        }

        if (case_sensitive)
        {
            if (wcsncmp(s, search.c_str(), search_len) != 0)
            {
                result += *s;
                s++;
                continue;
            }
        }    
         else
        {
            if (wcsncasecmp(s, search.c_str(), search_len) != 0)
            {
                result += *s;
                s++;
                continue;
            }
        }

        if (s > str)
        {
            wchar_t ch = *(s-1);
            if (iswalpha(ch) || iswdigit(ch) || ch == L'_')
            {
                result += *s;
                s++;
                continue;
            }
        }


        result += replace;
        s += search_len;
    }

    return result;
}


static std::wstring normalizeFieldNames(std::vector<SourceTable>& source_tables,
                                        const std::wstring& expr)
{
    // when a fieldname is unique, replace the fully qualified fieldname
    // with just the fieldname; e.g:
    //     alias.fieldname => fieldname
    //     alias.[fieldname] => [fieldname]
    //     [alias].fieldname => fieldname
    //     [alias].[fieldname] => [fieldname]

    std::wstring result = expr;
    std::vector<SourceTable>::iterator st_it;
    std::wstring temps;
    
    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        size_t i, col_count = st_it->structure.getColumnCount();
        
        for (i = 0; i < col_count; ++i)
        {
            const std::wstring& colname = st_it->structure.getColumnName(i);
            std::wstring q_colname = L"[" + colname + L"]";
            std::wstring full_name;

            if (isUniqueFieldName(source_tables, colname))
            {       
                // replace alias.fieldname with fieldname
                full_name = st_it->alias;
                full_name += L".";
                full_name += colname;
                result = exprReplaceToken(result, full_name, colname);
                
                // replace [alias].fieldname with fieldname
                full_name = L"[" + st_it->alias + L"]";
                full_name += L".";
                full_name += colname;
                result = exprReplaceToken(result, full_name, colname);

                // replace alias.[fieldname] with [fieldname]
                full_name = st_it->alias;
                full_name += L".";
                full_name += q_colname;
                result = exprReplaceToken(result, full_name, q_colname);

                // replace [alias].[fieldname] with [fieldname]
                full_name = L"[" + st_it->alias + L"]";
                full_name += L".";
                full_name += q_colname;
                result = exprReplaceToken(result, full_name, q_colname);
            }
        }
    }

    return result;
}
                                


static std::wstring renameJoinFields(std::vector<SourceTable>& source_tables,
                                     const std::wstring& expr)
{
    // when a fieldname is unique, replace the fully qualified fieldname
    // with just the fieldname; e.g:
    //     alias.fieldname => fieldname
    //     alias.[fieldname] => [fieldname]
    //     [alias].fieldname => fieldname
    //     [alias].[fieldname] => [fieldname]

    std::wstring result = expr;
    std::vector<SourceTable>::iterator st_it;
    std::wstring temps;
    
    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        size_t i, col_count = st_it->structure.getColumnCount();
        
        for (i = 0; i < col_count; ++i)
        {
            const std::wstring& colname = st_it->structure.getColumnName(i);
            std::wstring q_colname = L"[" + colname + L"]";
            std::wstring replace_with = L"[" + st_it->alias + L"." + colname + L"]";
            std::wstring full_name;
     
            // replace alias.fieldname with [alias.fieldname]
            full_name = st_it->alias;
            full_name += L".";
            full_name += colname;
            result = exprReplaceToken(result, full_name, replace_with);
                
            // replace [alias].fieldname with [alias.fieldname]
            full_name = L"[" + st_it->alias + L"]";
            full_name += L".";
            full_name += colname;
            result = exprReplaceToken(result, full_name, replace_with);

            // replace alias.[fieldname] with [alias.fieldname]
            full_name = st_it->alias;
            full_name += L".";
            full_name += q_colname;
            result = exprReplaceToken(result, full_name, replace_with);

            // replace [alias].[fieldname] with [alias.fieldname]
            full_name = L"[" + st_it->alias + L"]";
            full_name += L".";
            full_name += q_colname;
            result = exprReplaceToken(result, full_name, replace_with);

            if (isUniqueFieldName(source_tables, colname))
            {
                result = exprReplaceToken(result, q_colname, replace_with);
                result = exprReplaceToken(result, colname, replace_with);
            }
        }
    }

    return result;
}


static void normalizeFieldNames(std::vector<SourceTable>& source_tables,
                                std::vector<SelectField>& fields)
{
    std::vector<SourceTable>::iterator st_it;
    
    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        size_t i, col_count = st_it->structure.getColumnCount();

        for (i = 0; i < col_count; ++i)
        {
            std::wstring full_name;
            const std::wstring& colname = st_it->structure.getColumnName(i);
            std::wstring q_colname = L"[" + colname + L"]";

            if (isUniqueFieldName(source_tables, colname))
            {
                std::vector<SelectField>::iterator f_it;
                for (f_it = fields.begin(); f_it != fields.end(); ++f_it)
                {
                    // replace alias.fieldname with fieldname
                    full_name = st_it->alias;
                    full_name += L".";
                    full_name += colname;
                    if (f_it->expr.length() > 0)
                        f_it->expr = exprReplaceToken(f_it->expr, full_name, colname);
                         else
                        f_it->name = exprReplaceToken(f_it->name, full_name, colname);

                    // replace [alias].fieldname with fieldname
                    full_name = L"[" + st_it->alias + L"]";
                    full_name += L".";
                    full_name += colname;
                    if (f_it->expr.length() > 0)
                        f_it->expr = exprReplaceToken(f_it->expr, full_name, colname);
                         else
                        f_it->name = exprReplaceToken(f_it->name, full_name, colname);

                    // replace alias.[fieldname] with [fieldname]
                    full_name = st_it->alias;
                    full_name += L".";
                    full_name += q_colname;
                    if (f_it->expr.length() > 0)
                        f_it->expr = exprReplaceToken(f_it->expr, full_name, q_colname);
                         else
                        f_it->name = exprReplaceToken(f_it->name, full_name, q_colname);

                    // replace [alias].[fieldname] with [fieldname]
                    full_name = L"[" + st_it->alias + L"]";
                    full_name += L".";
                    full_name += q_colname;
                    if (f_it->expr.length() > 0)
                        f_it->expr = exprReplaceToken(f_it->expr, full_name, q_colname);
                         else
                        f_it->name = exprReplaceToken(f_it->name, full_name, q_colname);
                }
            }
        }
    }
}




//insert distinct implementation


struct InsertDistinctField
{
    int type;
    xd::objhandle_t source_handle;
    xd::objhandle_t dest_handle;
};


inline bool getBitState(unsigned char* buf, xd::rowpos_t bit)
{
    return (buf[bit/8] & (1 << (bit%8))) ? true : false;
}

inline void setBitState(unsigned char* buf, xd::rowpos_t bit)
{
    buf[bit/8] |= (1 << (bit%8));
}

class DistinctBulkInsertProgress : public IIndexProgress
{
public:
    xd::IJob* job;
    IJobInternal* ijob;
    std::wstring filename;
    bool cancelled;

    void updateProgress(xd::rowpos_t cur_count,
                        xd::rowpos_t max_count,
                        bool* cancel)
    {
        if (cur_count % 1000 == 0)
        {
            if (ijob)
            {
                ijob->setCurrentCount(cur_count);
                if (job->getCancelled())
                {
                    *cancel = true;
                    cancelled = true;
                }
            }
        }

        if (cur_count % 10000 == 0)
        {
            if (xf_get_free_disk_space(filename) < 50000000)
            {
                if (ijob)
                {
                    ijob->setStatus(xd::jobFailed);
                    *cancel = true;
                    cancelled = true;
                }
            }
        }
    }
};


static bool insertDistinct(xd::IDatabase* db,
                           const std::wstring& target,
                           xd::IIteratorPtr src_iter,
                           xd::IJob* job)
{
    ExIndex* index = new ExIndex;
    std::vector<InsertDistinctField> fields;
    std::vector<InsertDistinctField>::iterator fit;
    
    // put index file and temporary files in the temp path
    xd::IAttributesPtr attr = db->getAttributes();
    if (!attr)
        return false;
    std::wstring temp_dir = attr->getStringAttribute(xd::dbattrTempDirectory);
    if (temp_dir.empty())
    {
        temp_dir = xf_get_temp_path();
    }
    
    index->setTempFilePath(temp_dir);
    
    std::wstring index_filename = temp_dir;
    if (index_filename.empty() || index_filename[index_filename.length() - 1] != PATH_SEPARATOR_CHAR)
        index_filename += PATH_SEPARATOR_CHAR;
        
    index_filename += getUniqueString();
    index_filename += L".tmp";

    // create the index file
    if (!index->create(index_filename,
                       sizeof(unsigned long long),
                       sizeof(xd::rowpos_t),
                       true))
    {
        return false;
    }

    // create row inserter

    xd::IRowInserterPtr sp_output = db->bulkInsert(target);
    xd::IRowInserter* output = sp_output.p;
    xd::IIterator* iter = src_iter.p;
    output->startInsert(L"*");

    // create the key layout and get input and output handles

    KeyLayout key;
    key.setIterator(src_iter);

    xd::Structure s = src_iter->getStructure();

    size_t i, col_count = s.getColumnCount();
    int key_len;

    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& colinfo = s.getColumnInfoByIdx(i);

        key.addKeyPart(colinfo.name);

        InsertDistinctField f;
        f.source_handle = iter->getHandle(colinfo.name);
        f.dest_handle = output->getHandle(colinfo.name);
        f.type = colinfo.type;

        fields.push_back(f);
    }

    key_len = key.getKeyLength();


    IJobInternalPtr sp_ijob = job;
    IJobInternal* ijob = sp_ijob.p;
    xd::rowpos_t cur_row = 1;
    xd::rowpos_t row_count;


    // PHASE 1: populate the index with crc64 hash values

    index->startBulkInsert(src_iter->getRowCount());

    if (job)
    {
        ijob->startPhase();
    }

    unsigned long long crc;
    while (!iter->eof())
    {
        crc64(key.getKey(), key_len, &crc);
        index->insert(&crc,
                      sizeof(unsigned long long),
                      &cur_row,
                      sizeof(xd::rowpos_t));

        iter->skip(1);
        ++cur_row;

        if (job)
        {
            if (cur_row % 1000 == 0)
            {
                ijob->setCurrentCount(cur_row);
                if (job->getCancelled())
                    break;
            }
        }
    }   

    if (job)
    {
        if (job->getCancelled())
        {
            delete index;
            xf_remove(index_filename);
            return false;
        }

        ijob->startPhase();
    }

    DistinctBulkInsertProgress dbip;
    dbip.cancelled = false;
    dbip.filename = index_filename;
    dbip.ijob = ijob;
    dbip.job = job;

    index->finishBulkInsert(&dbip);

    if (dbip.cancelled)
    {
        delete index;
        xf_remove(index_filename);
        return false;
    }

    row_count = cur_row;

    // PHASE 2: create a bitmask of all of non-duplicate rows

    unsigned int bitmask_size = (unsigned int)((row_count/8)+1);
    unsigned char* bitmask = new unsigned char[bitmask_size];
    
    // check if the memory allocation worked
    if (!bitmask)
    {
        delete index;
        xf_remove(index_filename);
        return false;
    }
        
    memset(bitmask, 0, bitmask_size);


    unsigned long long last_crc = 0;

    IIndexIterator* idx_iter = index->createIterator();
    idx_iter->goFirst();
    cur_row = 1;

    if (job)
    {
        ijob->startPhase();
    }

    while (!idx_iter->isEof())
    {
        crc = *(unsigned long long*)idx_iter->getKey();
        if (crc != last_crc)
        {
            setBitState(bitmask, *(xd::rowpos_t*)idx_iter->getValue());
        }

        last_crc = crc;
        idx_iter->skip(1);
        ++cur_row;
        
        if (job)
        {
            if (cur_row % 1000 == 0)
            {
                ijob->setCurrentCount(cur_row);
                if (job->getCancelled())
                    break;
            }
        }
    }

    idx_iter->unref();

    index->close();
    delete index;

    xf_remove(index_filename);


    // PHASE 3: copy out unique rows

    if (job)
    {
        if (job->getCancelled())
        {
            delete[] bitmask;
            return false;
        }

        ijob->startPhase();
    }

    iter->goFirst();
    cur_row = 1;

    while (!iter->eof())
    {
        if (getBitState(bitmask, cur_row))
        {
            for (fit = fields.begin(); fit != fields.end(); ++fit)
            {
                if (iter->isNull(fit->source_handle))
                {
                    output->putNull(fit->dest_handle);
                    continue;
                }

                switch (fit->type)
                {
                    case xd::typeCharacter:
                        output->putString(fit->dest_handle,
                                  iter->getString(fit->source_handle));
                        break;

                    case xd::typeWideCharacter:
                        output->putWideString(fit->dest_handle,
                                  iter->getWideString(fit->source_handle));
                        break;

                    case xd::typeDouble:
                    case xd::typeNumeric:
                        output->putDouble(fit->dest_handle,
                                  iter->getDouble(fit->source_handle));
                        break;

                    case xd::typeInteger:
                        output->putInteger(fit->dest_handle,
                                  iter->getInteger(fit->source_handle));
                        break;

                    case xd::typeDate:
                    case xd::typeDateTime:
                        output->putDateTime(fit->dest_handle,
                                  iter->getDateTime(fit->source_handle));
                        break;

                    case xd::typeBoolean:
                        output->putBoolean(fit->dest_handle,
                                  iter->getBoolean(fit->source_handle));
                        break;
                }
            }

            output->insertRow();
        }

        iter->skip(1);
        cur_row++;
        
        if (job)
        {
            if (cur_row % 1000 == 0)
            {
                ijob->setCurrentCount(cur_row);
                if (job->getCancelled())
                    break;
            }
        }
    }

    if (job && (job->getStatus() != xd::jobCancelled))
    {
        ijob->setCurrentCount(cur_row-1);
    }
    
    output->finishInsert();

    delete[] bitmask;

    return true;
}









// field binding helper functions for the expression parser


static void _bindFieldString(kscript::ExprEnv*,
                             void* param,
                             kscript::Value* retval)
{
    JoinField* jf = (JoinField*)param;

    if (jf->source_iter->eof())
    {
        retval->setString(L"");
        return;
    }

    retval->setString(jf->source_iter->getWideString(jf->source_handle).c_str());
}

static void _bindFieldDouble(kscript::ExprEnv*,
                             void* param,
                             kscript::Value* retval)
{
    JoinField* jf = (JoinField*)param;

    if (jf->source_iter->eof())
    {
        retval->setDouble(0.0);
        return;
    }

    retval->setDouble(jf->source_iter->getDouble(jf->source_handle));
}

static void _bindFieldInteger(kscript::ExprEnv*,
                              void* param,
                              kscript::Value* retval)
{
    JoinField* jf = (JoinField*)param;

    if (jf->source_iter->eof())
    {
        retval->setInteger(0);
        return;
    }

    retval->setInteger(jf->source_iter->getInteger(jf->source_handle));
}

static void _bindFieldDateTime(kscript::ExprEnv*,
                               void* param,
                               kscript::Value* retval)
{
    JoinField* jf = (JoinField*)param;

    if (jf->source_iter->eof())
    {
        retval->setDateTime(0,0);
        return;
    }

    xd::datetime_t dt, d, t;
    dt = jf->source_iter->getDateTime(jf->source_handle);
    d = dt >> 32;
    t = dt & 0xffffffff;
    retval->setDateTime((unsigned int)d, (unsigned int)t);
}

static void _bindFieldBoolean(kscript::ExprEnv*,
                              void* param,
                              kscript::Value* retval)
{
    JoinField* jf = (JoinField*)param;

    if (jf->source_iter->eof())
    {
        retval->setBoolean(false);
        return;
    }

    retval->setBoolean(jf->source_iter->getBoolean(jf->source_handle));
}



struct JoinParseInfo
{
    std::vector<SourceTable>* source_tables;
    SourceTable* left_table;
    SourceTable* right_table;

    std::wstring left;
    std::wstring right;
};


static bool join_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    JoinParseInfo* info = (JoinParseInfo*)(hook_info.hook_param);

    if (hook_info.element_type != kscript::ExprParseHookInfo::typeOperator)
        return false;

    if (hook_info.oper_text != L"=")
        return false;


    xd::ColumnInfo colinfo;
    SourceTable* p1 = NULL;
    SourceTable* p2 = NULL;
    std::wstring left;
    std::wstring right;

    colinfo = getColumnInfoMulti(*(info->source_tables),
                                 hook_info.oper_left.c_str(), &p1);
    if (colinfo.isNull())
    {
        hook_info.res_element = NULL;
        return true;
    }

    colinfo = getColumnInfoMulti(*(info->source_tables),
                                 hook_info.oper_right.c_str(), &p2);
    if (colinfo.isNull())
    {
        hook_info.res_element = NULL;
        return true;
    }

    // left and right arguments must not refer to the same table

    if (p1 == p2)
    {
        hook_info.res_element = NULL;
        return true;
    }

    // one of the arguments must refer to the right table
    
    if (p1 != info->right_table &&
        p2 != info->right_table)
    {
        hook_info.res_element = NULL;
        return true;
    }


    if (p1 == info->right_table)
    {
        right = hook_info.oper_left;
        left = hook_info.oper_right;
    }
     else if (p2 == info->right_table)
    {
        right = hook_info.oper_right;
        left = hook_info.oper_left;
    }


    if (!info->left.empty())
        info->left += L",";
    if (!info->right.empty())
        info->right += L",";

    dequoteField(left);
    dequoteField(right);

    info->left += left;
    info->right += right;


    // return dummy element

    kscript::Value* value = new kscript::Value;
    value->setBoolean(true);
    hook_info.res_element = (kscript::ExprElement*)value;

    return true;
}




static bool join_where_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        if (hook_info.expr_text.length() > 1 &&
            hook_info.expr_text[0] == '[')
        {
            dequoteField(hook_info.expr_text);
            hook_info.res_element = hook_info.parser->createVariableLookup(hook_info.penv, hook_info.expr_text);
            return true;
        }
    }
    
    return false;
}



static kscript::ExprParser* createJoinExprParser(std::vector<JoinField>& all_fields)
{
    kscript::ExprParser* parser;
    std::vector<JoinField>::iterator jf_it;

    parser = createExprParser();

    parser->setParseHook(kscript::ExprParseHookInfo::typeIdentifier,
                         join_where_parse_hook,
                         NULL);

    for (jf_it = all_fields.begin();
         jf_it != all_fields.end();
         ++jf_it)
    {
        std::vector<std::wstring> names;
        std::vector<std::wstring>::iterator n_it;
        
        names.push_back(jf_it->name);
        names.push_back(jf_it->orig_name);

        if (jf_it->alias.length() > 0 &&
            jf_it->alias != jf_it->name &&
            jf_it->alias != jf_it->orig_name)
        {
            names.push_back(jf_it->alias);
        }

        switch (jf_it->type)
        {
            case xd::typeCharacter:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeString,
                                  (void*)_bindFieldString,
                                  &(*jf_it));
                }
                break;

            case xd::typeNumeric:
            case xd::typeDouble:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeDouble,
                                  (void*)_bindFieldDouble,
                                  &(*jf_it));
                }
                break;

            case xd::typeInteger:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeInteger,
                                  (void*)_bindFieldInteger,
                                  &(*jf_it));
                }
                break;

            case xd::typeDate:
            case xd::typeDateTime:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeDateTime,
                                  (void*)_bindFieldDateTime,
                                  &(*jf_it));
                }
                break;

            case xd::typeBoolean:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeBoolean,
                                  (void*)_bindFieldBoolean,
                                  &(*jf_it));
                }
                break;
        }
    }

    return parser;
}



wchar_t* zl_find_field(wchar_t* str,
                       const wchar_t* search_str)
{
    int str_len = wcslen(search_str);
    wchar_t quote_char = 0;
    wchar_t* ptr = NULL;
    wchar_t* ch;

    ch = str;
    while (*ch)
    {
        if (*ch == quote_char)
        {
            quote_char = 0;
            ch++;
            continue;
        }

        if (*ch == L'\'' && !quote_char)
            quote_char = L'\'';
        
        if (*ch == L'"' && !quote_char)
            quote_char = L'\"';

        if (!quote_char)
        {
            if (!wcsncasecmp(ch, search_str, str_len))
            {
                bool valid = true;
                if (ch > str)
                {
                    if (!isDelimiterChar(*(ch-1)))
                        valid = false;
                }
                if (*(ch+str_len))
                {
                    if (!isDelimiterChar(*(ch+str_len)))
                        valid = false;
                }
                if (valid)
                {
                    ptr = ch;
                }
                 else
                {
                    ch++;
                    continue;
                }
            }
        }

        ch++;
    }

    return ptr;
}


static void getReferencedFields(std::vector<SourceTable>& s,
                                const std::wstring& expr,
                                std::vector<std::wstring>& flds)

{
    flds.clear();

    std::wstring full_name;

    std::vector<SourceTable>::iterator it;
    for (it = s.begin(); it != s.end(); ++it)
    {
        size_t i, cnt = it->structure.getColumnCount();
        for (i = 0; i < cnt; ++i)
        {
            std::wstring alias = it->alias;
            const std::wstring& colname = it->structure.getColumnName(i);

            // alias.fieldname
            full_name = alias + L"." + colname;
            if (zl_find_field((wchar_t*)expr.c_str(), full_name.c_str()))
            {
                flds.push_back(L"[" + alias + L"].[" + colname + L"]");
                continue;
            }

            // [alias].fieldname
            full_name = L"[" + alias + L"]." + colname;
            if (zl_find_field((wchar_t*)expr.c_str(), full_name.c_str()))
            {
                flds.push_back(L"[" + alias + L"].[" + colname + L"]");
                continue;
            }

            // alias.[fieldname]
            full_name = alias + L".[" + colname + L"]";
            if (zl_find_field((wchar_t*)expr.c_str(), full_name.c_str()))
            {
                flds.push_back(L"[" + alias + L"].[" + colname + L"]");
                continue;
            }

            // [alias].[fieldname]
            full_name = L"[" + alias + L"].[" + colname + L"]";
            if (zl_find_field((wchar_t*)expr.c_str(), full_name.c_str()))
            {
                flds.push_back(L"[" + alias + L"].[" + colname + L"]");
                continue;
            }

            // [fieldname]
            full_name = L"[" + colname + L"]";
            if (zl_find_field((wchar_t*)expr.c_str(), full_name.c_str()))
            {
                if (isUniqueFieldName(s, colname))
                {
                    flds.push_back(L"[" + alias + L"].[" + colname + L"]");
                    continue;
                }
            }

            // fieldname
            full_name = colname;
            if (zl_find_field((wchar_t*)expr.c_str(), full_name.c_str()))
            {
                if (isUniqueFieldName(s, colname))
                {
                    flds.push_back(L"[" + alias + L"].[" + colname + L"]");
                    continue;
                }
            }
        }
    }
}


static void joinDoInsert(xd::IRowInserter* output,
                         std::vector<JoinField>& fields,
                         kscript::ExprParser* where_expr)
{
    std::vector<JoinField>::iterator fit;

    // check where expression, if any passed
    if (where_expr)
    {
        kscript::Value v;
        if (where_expr->eval(&v))
        {
            if (!v.getBoolean())
                return;
        }
         else
        {
            return;
        }
    }

    for (fit = fields.begin(); fit != fields.end(); ++fit)
    {
        if (fit->source_iter->eof())
        {
            output->putNull(fit->dest_handle);
        }
         else
        {
            switch (fit->type)
            {
                case xd::typeCharacter:
                    output->putString(fit->dest_handle,
                              fit->source_iter->getString(fit->source_handle));
                    break;

                case xd::typeWideCharacter:
                    output->putWideString(fit->dest_handle,
                              fit->source_iter->getWideString(fit->source_handle));
                    break;

                case xd::typeDouble:
                case xd::typeNumeric:
                    output->putDouble(fit->dest_handle,
                              fit->source_iter->getDouble(fit->source_handle));
                    break;

                case xd::typeInteger:
                    output->putInteger(fit->dest_handle,
                              fit->source_iter->getInteger(fit->source_handle));
                    break;

                case xd::typeDate:
                case xd::typeDateTime:
                    output->putDateTime(fit->dest_handle,
                              fit->source_iter->getDateTime(fit->source_handle));
                    break;

                case xd::typeBoolean:
                    output->putBoolean(fit->dest_handle,
                              fit->source_iter->getBoolean(fit->source_handle));
                    break;
            }
        }
    }

    output->insertRow();
}


static void joinIterLoop(xd::IIterator* left,
                         xd::IRowInserter* output,
                         std::vector<JoinInfo>& joins,
                         std::vector<JoinField>& fields,
                         kscript::ExprParser* where_expr,
                         int join_idx,
                         int join_count)
{
    xd::IIterator* right = joins[join_idx].right_iter.p;

    const unsigned char* val = joins[join_idx].left_key.getKey();
    int val_width = joins[join_idx].left_key.getKeyLength();
    
    if (joins[join_idx].left_key.getTruncation() ||
        !right->seek(val, val_width, false))
    {
        // match could not be found in the right set;
        // behavior now depends on the join type

        int join_type = joins[join_idx].join_type;

        if (join_type == joinInner)
        {
            return;
        }
         else
        {
            /*
            // set eof condition
            right->goLast();
            right->skip(1);

            if (join_idx+1 < join_count)
            {
                joinIterLoop(left, output, joins,
                             fields, where_expr,
                             join_idx+1, join_count);
            }
             else
            {
                joinDoInsert(output, fields, where_expr);
            }
            

            if (join_idx+1 < join_count)
            {
                joinIterLoop(left, output, joins, fields, where_expr,
                             join_idx+1, join_count);
            }
            */
            
            // set eof condition
            right->goLast();
            right->skip(1);

            if (join_idx+1 < join_count)
            {
                joinIterLoop(left, output, joins,
                             fields, where_expr,
                             join_idx+1, join_count);
            }
             else
            {
                joinDoInsert(output, fields, where_expr);
            }
        }


        return;
    }


    while (!right->eof())
    {
        const unsigned char* right_val = joins[join_idx].right_key.getKey();

        if (0 != memcmp(val, right_val, val_width))
            break;

        if (join_idx+1 < join_count)
        {
            joinIterLoop(left, output, joins, fields, where_expr,
                         join_idx+1, join_count);
        }
         else
        {
            joinDoInsert(output, fields, where_expr);
        }

        right->skip(1);
    }
}


static bool doJoin(xd::IDatabasePtr db, 
                   std::vector<SourceTable>& source_tables,
                   std::vector<SelectField>& columns,
                   std::vector<std::wstring>& group_by_fields,
                   const std::wstring& having,
                   const std::wstring& where_expr,
                   const std::wstring& output_path,
                   xd::IJob* job)
{
    std::wstring left;
    std::vector<JoinField> jfields;
    std::vector<JoinInfo> joins;
    SourceTable* left_table_info = NULL;

    IJobInternalPtr ijob = job;

    // find left-most set (the one without anything joining to it
    std::vector<SourceTable>::iterator st_it;
    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        if (st_it->join_type == joinNone)
        {
            if (left.length() > 0)
                return false;

            left = st_it->path;
            left_table_info = &(*st_it);
        }
    }

    if (left.length() == 0)
        return false;

    xd::IIteratorPtr left_iter = db->query(left, L"", L"", L"", NULL);
    if (left_iter.isNull())
        return false;

    left_iter->goFirst();



    // parse the join expressions
    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        if (st_it->join_type == joinNone)
            continue;

        JoinParseInfo jparse;
        
        jparse.source_tables = &source_tables;
        jparse.left_table = left_table_info;
        jparse.right_table = &(*st_it);

        kscript::ExprParser* parser =
                            new kscript::ExprParser(kscript::optionLanguageCustom |
                                                    kscript::optionEpsilonNumericCompare);

        parser->addOperator(L"=",
                            true,
                            kscript::oper_equals,
                            70,
                            kscript::ExprOperator::typeNormal,
                            kscript::ExprOperator::directionRightToLeft,
                            false,
                            L"b(nn);b(ss);b(dd);b(bb);b(xx)");

        parser->addOperator(L"and",
                            false,
                            kscript::oper_and,
                            60,
                            kscript::ExprOperator::typeNormal,
                            kscript::ExprOperator::directionRightToLeft,
                            false,
                            L"b(bb)");

        parser->setParseHook(kscript::ExprParseHookInfo::typeOperator,
                             join_parse_hook,
                             (void*)&jparse);

        if (!parser->parse(st_it->join_expr))
        {
            delete parser;
            return false;
        }

        delete parser;


        //  find out who the left table is

        std::vector<std::wstring> left_parts;
        std::vector<std::wstring>::iterator it;
        xd::ColumnInfo colinfo;

        kl::parseDelimitedList(jparse.left, left_parts, ',', true);

        SourceTable* tbl = NULL;
        SourceTable* last_tbl = NULL;
        for (it = left_parts.begin(); it != left_parts.end(); ++it)
        {
            colinfo = getColumnInfoMulti(source_tables, *it, &tbl);
            if (last_tbl != NULL)
            {
                if (tbl != last_tbl)
                    return false;
            }
            last_tbl = tbl;
        }

        if (!tbl)
            return false;

        JoinInfo j;
        j.left_path = tbl->path;
        j.right_path = st_it->path;
        j.left = jparse.left;
        j.right = jparse.right;
        j.join_type = st_it->join_type;

        joins.push_back(j);
    }

    
    // create right-side iterators

    std::vector<JoinInfo>::iterator jit, jit2;

    for (jit = joins.begin(); jit != joins.end(); ++jit)
    {
        xd::IJobPtr iter_job = db->createJob();

        if (job)
        {
            ijob->startPhase(iter_job);
        }



        std::vector<std::wstring> right_parts;
        std::vector<std::wstring>::iterator it;

        std::wstring right_sort;
        kl::parseDelimitedList(jit->right, right_parts, L',', true);
        for (it = right_parts.begin(); it != right_parts.end(); ++it)
        {
            if (!right_sort.empty())
                right_sort += L",";

            // get corresponding left field name (and strip it)
            int period_pos = it->find_last_of(L'.');
            if (period_pos != -1)
            {
                right_sort += it->substr(period_pos+1);
            }
             else
            {
                right_sort += *it;
            }
        }

        xd::rowpos_t old_max_count = 0;
        if (ijob)
        {
            old_max_count = job->getMaxCount();
            //ijob->setMaxCount(jit->right_set->getRowCount());
        }
        

        // create iterator
        jit->right_iter = db->query(jit->right_path, 
                                    L"",
                                    L"",
                                    right_sort,
                                    iter_job);


        if (ijob)
        {
            ijob->setMaxCount(old_max_count);
        }

        if (iter_job->getCancelled())
        {
            if (job)
            {
                job->cancel();
            }

            return false;
        }

        if (jit->right_iter.isNull())
            return false;
        
        // now prepare the right side's key
        jit->right_key.setKeyExpr(jit->right_iter, right_sort);
    }




    // get left-side iterators and handles
    for (jit = joins.begin(); jit != joins.end(); ++jit)
    {
        // construct left key expression
        std::wstring left_key_expr;
        std::vector<std::wstring> left_parts;
        std::vector<std::wstring> right_parts;
        std::vector<std::wstring>::iterator it;

        kl::parseDelimitedList(jit->left, left_parts, L',', true);
        kl::parseDelimitedList(jit->right, right_parts, L',', true);

        if (left_parts.size() != right_parts.size())
            return false;


        // find left iterator
        xd::IIteratorPtr join_left_iter;
        for (jit2 = joins.begin(); jit2 != joins.end(); ++jit2)
        {
            if (&(*jit2) == &(*jit))
                continue;

            if (isSamePath(jit->left_path, jit2->right_path))
            {
                join_left_iter = jit2->right_iter;
                break;
            }
        }

        if (join_left_iter)
        {
            jit->left_key.setIterator(join_left_iter);
        }
         else
        {
            jit->left_key.setIterator(left_iter);
        }


        // construct left key
        int idx = 0;
        for (it = right_parts.begin(); it != right_parts.end(); ++it)
        {
            xd::ColumnInfo colinfo;
  
            SourceTable* tbl = NULL;
            colinfo = getColumnInfoMulti(source_tables, *it, &tbl);
            if (colinfo.isNull())
                return false;
            if (!isSamePath(tbl->path, jit->right_path))
                continue;

            // get corresponding left field name (and strip it)
            std::wstring left;
            int period_pos = left_parts[idx].find(L'.');
            if (period_pos != -1)
            {
                left += left_parts[idx].substr(period_pos+1);
            }
             else
            {
                left += left_parts[idx];
            }


            if (!jit->left_key.addKeyPart(left, colinfo.type, colinfo.width))
            {
                return false;
            }

            ++idx;
        }
    }



    std::set<std::wstring, kl::cmp_nocase> join_fields_duplicate_check;

    // populate join fields vector

    std::vector<SelectField>::iterator sf_it;
    std::vector<JoinField>::iterator jf_it;

    for (sf_it = columns.begin(); sf_it != columns.end(); ++sf_it)
    {
        std::vector<std::wstring> flds;
        std::vector<std::wstring>::iterator fit;

        getReferencedFields(source_tables, sf_it->expr, flds);

        for (fit = flds.begin(); fit != flds.end(); ++fit)
        {
            JoinField jf;
            jf.name = *fit;
            jf.source_handle = 0;
            jf.dest_handle = 0;

            if (jf.name != sf_it->name)
                jf.alias = sf_it->name;

            dequoteField(jf.name);

            if (join_fields_duplicate_check.find(jf.name) == join_fields_duplicate_check.end())
            {
                join_fields_duplicate_check.insert(jf.name);
                jfields.push_back(jf);
            }
        }

    }


    // make sure group fields are in output -- we will need this
    // in the output for the grouping operation, which
    // is executed after the join

    std::vector<std::wstring>::iterator g_it;
    for (g_it = group_by_fields.begin();
         g_it != group_by_fields.end(); ++g_it)
    {
        JoinField jf;
        jf.name = *g_it;
        jf.source_handle = 0;
        jf.dest_handle = 0;

        dequoteField(jf.name);

        if (join_fields_duplicate_check.find(jf.name) == join_fields_duplicate_check.end())
        {
            join_fields_duplicate_check.insert(jf.name);
            jfields.push_back(jf);
        }
    }

    // make sure having fields are in output -- we will need this
    // in the output for the grouping operation, which
    // is executed after the join

    if (having.length() > 0)
    {
        std::vector<std::wstring> flds;
        std::vector<std::wstring>::iterator fit;

        getReferencedFields(source_tables, having, flds);
        for (fit = flds.begin(); fit != flds.end(); ++fit)
        {
            JoinField jf;
            jf.name = *fit;
            jf.source_handle = 0;
            jf.dest_handle = 0;

            dequoteField(jf.name);

            if (join_fields_duplicate_check.find(jf.name) == join_fields_duplicate_check.end())
            {
                join_fields_duplicate_check.insert(jf.name);
                jfields.push_back(jf);
            }
        }
    }



    for (jf_it = jfields.begin(); jf_it != jfields.end(); ++jf_it)
    {
        SourceTable* src_table = NULL;
        xd::ColumnInfo colinfo;

        // get the column info for the join field
        colinfo = getColumnInfoMulti(source_tables, jf_it->name, &src_table);
        if (colinfo.isNull() || !src_table)
        {
            return false;
        }


        jf_it->type = colinfo.type;
        jf_it->width = colinfo.width;
        jf_it->scale = colinfo.scale;

        jf_it->orig_name = colinfo.name;
        jf_it->source_table = src_table;

        // find join field's source iterator

        if (isSamePath(src_table->path, left))
        {
            jf_it->source_iter = left_iter;
        }
         else
        {
            for (jit = joins.begin(); jit != joins.end(); ++jit)
            {
                if (isSamePath(src_table->path, jit->right_path))
                {
                    jf_it->source_iter = jit->right_iter;
                    break;
                }
            }
        }

        if (jf_it->source_iter.isNull())
            return false;

        // get our source handle
        jf_it->source_handle = jf_it->source_iter->getHandle(colinfo.name);
    }




    // create output structure
    xd::FormatDefinition fd;

    for (jf_it = jfields.begin(); jf_it != jfields.end(); ++jf_it)
    {
        xd::ColumnInfo colinfo;

        colinfo.name = jf_it->name;
        colinfo.type = jf_it->type;
        colinfo.width = jf_it->width;
        colinfo.scale = jf_it->scale;

        fd.createColumn(colinfo);
    }

    if (!db->createTable(output_path, fd))
        return false;

    // create output row inserter

    xd::IRowInserterPtr sp_output = db->bulkInsert(output_path);
    if (sp_output.isNull())
    {
        db->deleteFile(output_path);
        return false;
    }

    xd::IRowInserter* output = sp_output.p;

    output->startInsert(L"*");

    // get output handles

    for (jf_it = jfields.begin(); jf_it != jfields.end(); ++jf_it)
    {
        jf_it->dest_handle = output->getHandle(jf_it->name);
        if (jf_it->dest_handle == 0)
        {
            db->deleteFile(output_path);
            return false;
        }
    }

    
    // parse where expression, if any

    std::vector<JoinField> all_fields;


    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        size_t i, col_count = st_it->structure.getColumnCount();

        for (i = 0; i < col_count; ++i)
        {
            const xd::ColumnInfo& colinfo = st_it->structure.getColumnInfoByIdx(i);

            JoinField f;

            f.name = st_it->alias;
            f.name += L".";
            f.name += colinfo.name;

            f.orig_name = colinfo.name;

            // try to find the field's optional output name
            for (sf_it = columns.begin(); sf_it != columns.end(); ++sf_it)
            {
                if (kl::iequals(sf_it->expr, f.name) && !sf_it->name.empty())
                {
                    f.alias = sf_it->name;
                }
            }


            f.type = colinfo.type;
            f.width = colinfo.width;
            f.scale = colinfo.scale;
            f.source_table = &(*st_it);
            f.dest_handle = 0;

            // find source iterator

            if (isSamePath(st_it->path, left))
            {
                f.source_iter = left_iter;
            }
             else
            {
                for (jit = joins.begin(); jit != joins.end(); ++jit)
                {
                    if (isSamePath(st_it->path, jit->right_path))
                    {
                        f.source_iter = jit->right_iter;
                        break;
                    }
                }
            }

            f.source_handle = f.source_iter->getHandle(f.orig_name);
            if (f.source_handle)
            {
                all_fields.push_back(f);
            }
        }
    }



    kscript::ExprParser* parser = NULL;

    if (!where_expr.empty())
    {
        parser = createJoinExprParser(all_fields);

        if (!parser->parse(where_expr))
        {
            delete parser;
            db->deleteFile(output_path);
            return false;
        }
    }



    if (job)
    {
        ijob->startPhase();
    }


    int join_count = joins.size();
    xd::rowpos_t cur_row = 0;
    xd::IIterator* l = left_iter.p;

    while (!l->eof())
    {
        joinIterLoop(l, output, joins, jfields, parser, 0, join_count);

        l->skip(1);

        if (job)
        {
            ++cur_row;
            if (cur_row % 100 == 0)
            {
                ijob->setCurrentCount(cur_row);
                if (job->getCancelled())
                    break;
            }
        }
    }

    delete parser;

    output->finishInsert();

    if (job && job->getCancelled())
    {
        sp_output.clear();
        db->deleteFile(output_path);
        return false;
    }

    return true;
}


// see note below about convertToNativeTables.
bool convertToNativeTables(xd::IDatabasePtr db,
                           std::vector<SourceTable>& source_tables,
                           const std::wstring& mainset,
                           xd::IJob* job)
{
    /*
    std::vector<SourceTable>::iterator st_it;

    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        if (st_it->set.isNull())
            continue;
            
        xcm::class_info* ci = xcm::get_class_info(st_it->set.p);
        std::string class_name = ci->get_name();
        kl::makeLower(class_name);
        if (class_name.find("xdnative.") != -1 ||
            class_name.find("xdfs.") != -1 ||
            class_name.find("xdclient.") != -1 ||
            class_name.find("xdhttp.") != -1)
        {
            // class has good support for the kinds of things
            // we want to do (dynamic filter sets, etc)
            continue;
        }
        
        // need to make an xdnative copy of the set
        xd::Structure structure = st_it->set->getStructure();
        std::wstring output_path = xd::getTemporaryPath();
        if (!db->createTable(output_path, structure, NULL))
        {
            // fail out
            return false;
        }

        
        xd::IIteratorPtr iter = st_it->set->createIterator(L"", L"", L"", NULL);
        if (iter.isNull())
            return false;
        iter->goFirst();
            
        xdcmnInsert(db, iter, output_path,  L"",  0,  job);

        if (mainset == st_it->set)
            mainset = set;
            
        st_it->set = set;
    }
    */
    return true;
}




static wchar_t* findJoin(wchar_t* s)
{
    wchar_t *join, *full, *left, *right, *inner, *outer, *on;
    
    join = zl_stristr(s, L"JOIN", true, true);

    wchar_t* jloc = join;

    full = zl_stristr(s, L"FULL", true, true);
    left = zl_stristr(s, L"LEFT", true, true);
    right = zl_stristr(s, L"RIGHT", true, true);
    inner = zl_stristr(s, L"INNER", true, true);
    outer = zl_stristr(s, L"OUTER", true, true);
    on = zl_stristr(s, L"ON", true, true);

    if (full && full < jloc)
        jloc = full;
    if (left && left < jloc)
        jloc = left;
    if (right && right < jloc)
        jloc = right;
    if (inner && inner < jloc)
        jloc = inner;
    if (outer && outer < jloc)
        jloc = outer;

    return jloc;
}




xd::IIteratorPtr sqlSelect(xd::IDatabasePtr db,
                           const std::wstring& _command,
                           unsigned int flags,
                           ThreadErrorInfo& error,
                           xd::IJob* job)
{
    if (_command.length() == 0)
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; empty SELECT statement");
        return xcm::null;
    }

    wchar_t* command = new wchar_t[_command.length()+1];
    wcscpy(command, _command.c_str());
    DelArrPtr<wchar_t> del_command(command);

    std::vector<SelectField> fields;
    std::vector<SourceTable> source_tables;
    std::vector<OrderByField> order_by_fields;
    std::vector<std::wstring> group_by_fields;
    std::wstring output_path;
    std::wstring where_cond;
    std::wstring having;
    std::wstring set;
    IJobInternalPtr ijob = job;
    bool group_operation = false;
    bool join_operation = false;
    xcm::IObjectPtr filter_set_ref_holder;
    
    // strip off trailing semicolon
    int command_len = wcslen(command);
    if (*(command+command_len-1) == L';')
        *(command+command_len-1) = 0;

    // locate keywords

    wchar_t* p_select = zl_stristr(command, L"SELECT", true, true);
    wchar_t* p_from = zl_stristr(command, L"FROM", true, true);
    wchar_t* p_where = zl_stristr(command, L"WHERE", true, true);
    wchar_t* p_group_by = zl_stristr(command, L"GROUP BY", true, true);
    wchar_t* p_having = zl_stristr(command, L"HAVING", true, true);
    wchar_t* p_order_by = zl_stristr(command, L"ORDER BY", true, true);
    wchar_t* p_into = zl_stristr(command, L"INTO", true, true);
    wchar_t* p_distinct = zl_stristr(command, L"DISTINCT", true,true);
    wchar_t* p_join = zl_stristr(command, L"JOIN", true,true);

    if (p_select)
    {
        *p_select = 0;
        p_select += 6;
    }

    if (p_distinct)
    {
        *p_distinct = 0;
        p_select = p_distinct + 8;
    }

    if (p_from)
    {
        *p_from = 0;
        p_from += 4;
    }

    if (p_where)
    {
        *p_where = 0;
        p_where += 5;
    }

    if (p_group_by)
    {
        *p_group_by = 0;
        p_group_by += 8;
    }

    if (p_having)
    {
        *p_having = 0;
        p_having += 6;
    }

    if (p_order_by)
    {
        *p_order_by = 0;
        p_order_by += 8;
    }

    if (p_into)
    {
        *p_into = 0;
        p_into += 4;

        while (iswspace(*p_into))
            p_into++;

        if (!wcsncasecmp(p_into, L"TABLE", 5) &&
             iswspace(*(p_into+5)))
        {
            p_into += 5;
        }

        output_path = p_into;
        kl::trim(output_path);

        dequote(output_path, L'"', L'"');
        dequote(output_path, L'[', L']');
    }

    // error checks

    if (!p_from)
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; missing FROM clause");
        return xcm::null;
    }

    if (p_where && p_where < p_from)
    {
        // WHERE clause can't be before FROM
        error.setError(xd::errorSyntax, L"Invalid syntax; WHERE clause is before FROM clause");        
        return xcm::null;
    }
    
    if (p_where && p_where < p_join)
    {
        // WHERE clause can't be before FROM
        error.setError(xd::errorSyntax, L"Invalid syntax; WHERE clause is before JOIN clause");        
        return xcm::null;
    }
    
    if (p_into && p_into > p_from)
    {
        // INTO clause can't be after FROM
        error.setError(xd::errorSyntax, L"Invalid syntax; INTO clause is after FROM clause");        
        return xcm::null;
    }
    
    if (p_group_by && (p_group_by < p_where || p_group_by < p_from))
    {
        // GROUP BY clause can't be before FROM or WHERE
        error.setError(xd::errorSyntax, L"Invalid syntax; GROUP BY clause is before FROM clause or WHERE clause");
        return xcm::null;
    }

    if (p_having && (p_having < p_group_by ||
                     p_having < p_where ||
                     p_having < p_from))
    {
        // HAVING clause can't be before FROM or WHERE or GROUP BY
        error.setError(xd::errorSyntax, L"Invalid syntax; HAVING clause is before FROM clause, WHERE clause, or GROUP BY clause");
        return xcm::null;
    }

    if (p_order_by && (p_order_by < p_having ||
                       p_order_by < p_group_by ||
                       p_order_by < p_where ||
                       p_order_by < p_from))
    {
        // ORDER BY clause must be last
        error.setError(xd::errorSyntax, L"Invalid syntax; ORDER BY clause is not at the end of the statement");        
        return xcm::null;
    }



    if (p_where)
    {
        where_cond = p_where;
    }

    if (p_having)
    {
        having = p_having;
    }


    // parse the join statements

    {
        wchar_t* jloc;
        wchar_t* join;
        wchar_t* next_join;
        wchar_t* start = p_from;
        wchar_t tempc;

        jloc = findJoin(start);

        while (jloc)
        {
            join = zl_stristr(jloc, L"JOIN", true, true);
            if (!join)
            {
                error.setError(xd::errorSyntax, L"Invalid syntax; JOIN clause missing JOIN keyword");
                return xcm::null;
            }

            next_join = findJoin(join+1);
            if (next_join)
            {
                tempc = *next_join;
                *next_join = 0;
            }


            wchar_t* full = zl_stristr(jloc, L"FULL", true, true);
            wchar_t* left = zl_stristr(jloc, L"LEFT", true, true);
            wchar_t* right = zl_stristr(jloc, L"RIGHT", true, true);
            wchar_t* inner = zl_stristr(jloc, L"INNER", true, true);
            wchar_t* outer = zl_stristr(jloc, L"OUTER", true, true);

            wchar_t* on = zl_stristr(jloc, L"ON", true, true);
            if (!on)
            {
                error.setError(xd::errorSyntax, L"Invalid syntax; JOIN clause missing ON keyword");            
                return xcm::null;
            }

            *on = 0;
            on += 2;

            std::wstring tbl;
            std::wstring alias;

            std::wstring join_params = join+4;
            kl::trim(join_params);
                
            
            // parse out the table and alias
            parseTableAndAlias(join_params, tbl, alias);


            SourceTable j;
            j.path = tbl;
            j.structure = db->describeTable(tbl);
            j.alias = alias;
            j.join_expr = on;
            j.join_type = joinInner;

            if (j.structure.isNull())
            {
                wchar_t buf[1024]; // some paths might be long
                swprintf(buf, 1024, L"Unable to open table [%ls]", tbl.c_str()); 
                error.setError(xd::errorGeneral, buf);
                return xcm::null;
            }

            if (full)
                j.join_type = joinFullOuter;

            if (left)
                j.join_type = joinLeftOuter;

            if (right)
                j.join_type = joinRightOuter;
            
            if (j.join_type == joinRightOuter || j.join_type == joinFullOuter)
            {
                error.setError(xd::errorSyntax, L"Invalid syntax; FULL and RIGHT OUTER joins are not supported");                 
                return xcm::null;
            }

            if (outer)
            {
                // this keyword is only used for syntactical clarity

                if (!full && !left && !right)
                {
                    error.setError(xd::errorSyntax, L"Invalid syntax; JOIN clause has OUTER keyword but is missing FULL keyword, LEFT keyword, or RIGHT keyword");                 
                    return xcm::null;
                }
            }

            // make sure only one type of outer join is specified
            if ((full ? 1 : 0) + (left ? 1 : 0) + (right ? 1 : 0) > 1)
            {
                error.setError(xd::errorSyntax, L"Invalid syntax; JOIN clause contains invalid combinations of FULL keyword, LEFT keyword, or RIGHT keyword");
                return xcm::null;
            }
        
            source_tables.push_back(j);
            join_operation = true;


            if (next_join)
            {
                *next_join = tempc;
            }

            *jloc = 0;
            jloc = next_join;
        }
    }



    // parse the from table
    {
        SourceTable st;

        std::vector<std::wstring> set_paths;
        kl::parseDelimitedList(p_from, set_paths, L',');

        if (set_paths.size() != 1)
        {
            // the old join syntax is not (yet) supported
            error.setError(xd::errorSyntax, L"Invalid syntax; alternate JOIN syntax not supported");                    
            return xcm::null;
        }
        
        std::wstring table, alias;
        parseTableAndAlias(set_paths[0], table, alias);


        xd::Structure table_structure = db->describeTable(table);
        if (table_structure.isNull())
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to open table [%ls]", table.c_str());
            error.setError(xd::errorGeneral, buf);       
            return xcm::null;
        }
        set = table;

        st.path = table;
        st.join_type = joinNone;
        st.alias = alias;
        st.structure = table_structure;


        source_tables.insert(source_tables.begin(), st);
    }

    // parse the field list
 
    std::vector<std::wstring> field_strs;

    kl::parseDelimitedList(p_select, field_strs, L',', true);

    if (field_strs.size() == 0)
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; missing column or expression in SELECT clause");
        return xcm::null;
    }

    std::vector<std::wstring>::iterator fstr_it;
    int colname_counter = 0;
    for (fstr_it = field_strs.begin(); fstr_it != field_strs.end(); ++fstr_it)
    {
        kl::trim(*fstr_it);

        if (fstr_it->empty())
        {
            error.setError(xd::errorSyntax, L"Invalid syntax; missing column or expression in SELECT clause");
            return xcm::null;
        }

        if (*fstr_it == L"*")
        {
            SelectField f;
            
            std::vector<SourceTable>::iterator st_it;

            for (st_it = source_tables.begin(); st_it != source_tables.end();  ++st_it)
            {
                size_t i, col_count = st_it->structure.getColumnCount();
        
                for (i = 0; i < col_count; ++i)
                {
                    const std::wstring& colname = st_it->structure.getColumnName(i);
                    
                    f.name = colname;

                    if (!isUniqueFieldName(source_tables, f.name))
                    {
                        f.name = st_it->alias;
                        f.name += L".";
                        f.name += colname;
                    }

                    f.expr = st_it->alias;
                    f.expr += L".";
                    f.expr += colname;
                    
                    f.star = true;

                    fields.push_back(f);
                }
            }

            continue;
        }


        if (isGroupFunction(*fstr_it))
        {
            group_operation = true;
        }

        // look for 'AS' keyword
        wchar_t* temp = zl_stristr((wchar_t*)fstr_it->c_str(),
                                   L"AS",
                                   true,
                                   false);
        int as_pos = -1;

        if (temp)
        {
            as_pos = temp - fstr_it->c_str();
        }


        std::wstring colname;
        std::wstring expr;
        SelectField f;
        
        f.star = false;

        if (as_pos != -1)
        {
            f.name = fstr_it->substr(as_pos+2);
            f.expr = fstr_it->substr(0, as_pos);
            
            kl::trim(f.name);
            kl::trim(f.expr);

            if (f.name.length() == 0)
            {
                error.setError(xd::errorSyntax, L"missing AS parameter");
                return xcm::null;
            }
        }
         else
        {
            if (getColumnInfoMulti(source_tables, *fstr_it).isOk())
            {
                f.name = *fstr_it;
                f.expr = *fstr_it;

                // if the output field name is needlessly qualified
                // with a table alias, remove the table alias

                const wchar_t* period = wcschr(f.name.c_str(), L'.');
                if (period && !join_operation)
                {
                    std::wstring fname = period+1;

                    if (isUniqueFieldName(source_tables, fname))
                    {
                        f.name = fname;
                        f.expr = fname;
                    }
                }
            }
             else
            {
                wchar_t buf[255];
                do
                {
                    swprintf(buf, 255, L"EXPR%03d", ++colname_counter);
                } while (getColumnInfoMulti(source_tables, buf).isOk());

                f.name = buf;
                f.expr = *fstr_it;
            }
        }

        dequoteField(f.name);
        
        fields.push_back(f);
    }

    // parse the group fields
    
    if (p_group_by)
    {
        group_operation = true;

        std::vector<std::wstring>::iterator it;

        kl::parseDelimitedList(p_group_by, group_by_fields, L',', true);
    
        for (it = group_by_fields.begin();
             it != group_by_fields.end(); ++it)
        {
            dequoteField(*it);

            xd::ColumnInfo colinfo = getColumnInfoMulti(source_tables, *it);
            
            if (colinfo.isNull())
            {
                wchar_t buf[255];            
                swprintf(buf, 255, L"Invalid syntax; unknown column [%ls] in GROUP BY clause", (*it).c_str());
                error.setError(xd::errorSyntax, buf);
                return xcm::null;
            }
        }
    }

    // parse the order fields

    if (p_order_by)
    {
        std::vector<std::wstring> order_vec;
        std::vector<std::wstring>::iterator it;

        kl::parseDelimitedList(p_order_by, order_vec, L',', true);

        for (it = order_vec.begin(); it != order_vec.end(); ++it)
        {
            xd::ColumnInfo colinfo;
            OrderByField f;

            kl::trim(*it);
            f.descending = false;

            const wchar_t* space;
            const wchar_t* str;
            int dir_pos = -1;
            str = it->c_str();
            space = zl_strchr((wchar_t*)str, ' ', L"[", L"]");
            if (space)
                dir_pos = space-str;
            
            if (dir_pos != -1)
            {
                std::wstring dir_str = it->substr(dir_pos+1);

                kl::trim(dir_str);
                kl::makeUpper(dir_str);

                if (dir_str == L"ASC")
                {
                    f.descending = false;
                    *it = kl::beforeLast(*it, L' ');
                    kl::trim(*it);
                }
                 else if (dir_str == L"DESC")
                {
                    f.descending = true;
                    *it = kl::beforeLast(*it, L' ');
                    kl::trim(*it);
                }
                 else
                {
                    f.descending = false;
                }
            }


            dequoteField(*it);
            f.name = *it;

            if (p_group_by)
            {
                order_by_fields.push_back(f);
                continue;
            }

            colinfo = getColumnInfoMulti(source_tables, *it);
            
            if (colinfo.isOk())
            {
                order_by_fields.push_back(f);
            }
             else
            {
                // not a plain field name;  try to find the original column name
                // (this is unfortunately another place where we require the input
                // 'expression' to be a real field in the source file)
                std::wstring real_field_name = L"";

                std::wstring normalized = normalizeFieldNames(source_tables, f.name);

                std::vector<SelectField>::iterator sf_it;
                for (sf_it = fields.begin(); sf_it != fields.end(); ++sf_it)
                {
                    if (isSameField(sf_it->name, f.name))
                    {
                        real_field_name = sf_it->expr;
                        dequoteField(real_field_name);
                        break;
                    }
                }

                colinfo = getColumnInfoMulti(source_tables, real_field_name);
                if (colinfo.isNull())
                {
                    wchar_t buf[255];            
                    swprintf(buf, 255, L"Invalid syntax; unknown column [%ls] in ORDER BY clause", (*it).c_str());
                    error.setError(xd::errorSyntax, buf);
                    return xcm::null;
                }

                f.name = real_field_name;
                order_by_fields.push_back(f);
            }
        }
    }


    // normalize field names which are needlessly qualified with an alias
    if (!join_operation)
    {
        normalizeFieldNames(source_tables, fields);

        std::vector<OrderByField>::iterator o_it;
        for (o_it = order_by_fields.begin();
             o_it != order_by_fields.end(); ++o_it)
        {
            o_it->name = normalizeFieldNames(source_tables, o_it->name);
        }

        std::vector<std::wstring>::iterator g_it;
        for (g_it = group_by_fields.begin();
             g_it != group_by_fields.end(); ++g_it)
        {
            *g_it = normalizeFieldNames(source_tables, *g_it);
        }

        where_cond = normalizeFieldNames(source_tables, where_cond);
        having = normalizeFieldNames(source_tables, having);
    }



    // convert to native tables if necessary;  note this is an un-noble
    // solution which allows any data source to be queried, regardless
    // of whether it's on a remote server or local server
    
    if (!convertToNativeTables(db, source_tables, set, job))
    {
        error.setError(xd::errorGeneral);
        return xcm::null;
    }

    if (job && job->getCancelled())
    {
        error.setError(xd::errorCancelled, L"Job cancelled");
        return xcm::null;
    }


    // set up job information

    int join_phase_count = 0;

    std::wstring left;
    std::vector<SourceTable>::iterator st_it;
    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        if (st_it->join_type == joinNone)
            left = st_it->path;
             else
            ++join_phase_count;
    }
    
    if (ijob)
    {
        xd::IFileInfoPtr left_finfo = db->getFileInfo(left);
        if (left_finfo.isNull())
        {
            error.setError(xd::errorGeneral);
            return xcm::null;
        }

        if (left_finfo->getFlags() & xd::sfFastRowCount)
            ijob->setMaxCount(left_finfo->getRowCount());
             else
            ijob->setMaxCount(0);
    }

    int phase_count = 0;

    // join
    if (join_operation)
        phase_count += (join_phase_count + 1);

    // where
    if (where_cond.length() > 0 && !join_operation)
        phase_count++;

    if (group_operation)
        phase_count++;

    if (order_by_fields.size() > 0)
        phase_count++;

    // final set creation
    if ((flags & xd::sqlAlwaysCopy) != 0 || p_distinct || p_into)
    {
        if (p_distinct)
            phase_count += 4;
             else
            phase_count++;
    }

    int* phase_pcts = new int[phase_count];

    int tempi;
    int remaining = 100;
    for (tempi = 0; tempi < phase_count; ++tempi)
    {
        if (tempi+1 == phase_count)
        {
            phase_pcts[tempi] = remaining;
        }
         else
        {
            phase_pcts[tempi] = (100/phase_count);
            remaining -= phase_pcts[tempi];
        }
    }

    if (job)
    {
        ijob->setPhases(phase_count, phase_pcts);
    }
    
    delete[] phase_pcts;

    // create the cross-product table (if necessary)
    std::wstring join_output_path;

    if (join_operation)
    {
        join_output_path = xd::getTemporaryPath();

        bool res = doJoin(static_cast<xd::IDatabase*>(db),
                          source_tables,
                          fields,
                          group_by_fields,
                          having,
                          where_cond,
                          join_output_path,
                          job);

        if (!res)
        {
            db->deleteFile(join_output_path);
            error.setError(xd::errorGeneral, L"Unable to process JOIN statement");
            return xcm::null;
        }

        set = join_output_path;

        std::vector<SelectField>::iterator f_it;
        for (f_it = fields.begin(); f_it != fields.end(); ++f_it)
        {
            f_it->expr = renameJoinFields(source_tables, f_it->expr);
        }

        std::wstring order_by_str;
        std::vector<OrderByField>::iterator order_by_it;
        for (order_by_it = order_by_fields.begin();
             order_by_it != order_by_fields.end();
             ++order_by_it)
        {
            order_by_it->name = renameJoinFields(source_tables, order_by_it->name);
        }

        having = renameJoinFields(source_tables, having);
    }


    // filter selected rows (if necessary)

    if (where_cond.length() > 0 && !join_operation)
    {
#if 0
        // create an iterator which we will insert from
        xd::IIteratorPtr source_iter = set->createIterator(L"", L"", L"", NULL);
        
        /*
        // this step is taken care of a few lines below
        
        // make sure the where expression is valid
        xd::objhandle_t h = source_iter->getHandle(where_cond);
        if (h == 0)
        {
            error.setError(xd::errorSyntax, L"Invalid syntax; expression in WHERE clause does not evaluate to a true or false value");            
            return xcm::null;
        }
        source_iter->releaseHandle(h);
        */
        

        CommonDynamicSet* dyn_set = new CommonDynamicSet;
        if (!dyn_set->create(db, set->getObjectPath()))
        {
            error.setError(xd::errorGeneral, L"Unable to process WHERE clause");
            delete dyn_set;
            return xcm::null;
        }
 
        SqlIterator* s_iter = SqlIterator::createSqlIterator(source_iter, where_cond, job);
        if (!s_iter)
        {
            error.setError(xd::errorSyntax, L"Invalid syntax; expression in WHERE clause does not evaluate to a true or false value");            
            return xcm::null;
        }
        
        IJobInternalPtr ijob = job;
        
        if (ijob)
        {
            ijob->startPhase();
        }
        
        dyn_set->startBulkInsert();
        s_iter->goFirst();
        while (!s_iter->isDone())
        {
            if (s_iter->isCancelled())
                break;
            
            dyn_set->insertRow(s_iter->getIterator()->getRowId());
            s_iter->goNext();
        }
        dyn_set->finishBulkInsert();
        
        if (s_iter->isCancelled())
        {
            error.setError(xd::errorCancelled, L"Job cancelled");
            delete dyn_set;
            delete s_iter;
            return xcm::null;
        }

        set = static_cast<xd::IxSet*>(dyn_set);
        delete s_iter;
        
#endif
        
        // create an iterator which we will insert from
        xd::IIteratorPtr source_iter = db->query(set, L"", L"", L"", NULL);
        if (source_iter.isNull())
        {
            error.setError(xd::errorSyntax, L"Could not create iterator");            
            return xcm::null;
        }

        // make sure the where expression is valid
        xd::objhandle_t h = source_iter->getHandle(where_cond);
        if (h == 0)
        {
            error.setError(xd::errorSyntax, L"Invalid syntax; expression in WHERE clause does not evaluate to a true or false value");            
            return xcm::null;
        }
        
        source_iter->releaseHandle(h);
        
        // create the filtered set
        CommonDynamicSet* dyn_set = new CommonDynamicSet;
        if (!dyn_set->create(db, set))
        {
            error.setError(xd::errorGeneral, L"Unable to process WHERE clause");
            delete dyn_set;
            return xcm::null;
        }
        filter_set_ref_holder = (xcm::IObject*)(IXdsqlTable*)dyn_set;
        
        // start job phase
        if (job)
        {
            ijob->startPhase();
        }
        
        // insert all rows meeting expression
        int res = dyn_set->insert(source_iter, where_cond, 0, job);
        if (res == -1)
        {
            error.setError(xd::errorGeneral, L"Unable to process WHERE clause");
            delete dyn_set;
            return xcm::null;
        }
        
        set = dyn_set->getObjectPath();
    }


    if (job && job->getCancelled())
    {
        error.setError(xd::errorCancelled, L"Job cancelled");
        return xcm::null;
    }


    // do grouping operation (if necessary)

    if (group_operation)
    {
        std::wstring group_by_str;
        
        if (p_group_by)
        {
            std::vector<std::wstring>::iterator g_it;
            for (g_it = group_by_fields.begin();
                 g_it != group_by_fields.end(); ++g_it)
            {
                if (!group_by_str.empty())
                    group_by_str += L",";

                group_by_str += L"[" + *g_it + L"]";
            }

            if (group_by_str.length() == 0)
            {
                if (join_output_path.length() > 0)
                    db->deleteFile(join_output_path);

                error.setError(xd::errorSyntax, L"Invalid syntax; missing column or expression in GROUP BY clause");
                return xcm::null;
            }
        }


        std::wstring field_str;
        std::vector<SelectField>::iterator f_it;
        
        for (f_it = fields.begin(); f_it != fields.end(); ++f_it)
        {
            if (!field_str.empty())
                field_str += L",";
            field_str += f_it->name;

            if (!f_it->expr.empty())
            {
                field_str += L"=";
                field_str += f_it->expr;
            }
        }


        if (job)
        {
            ijob->startPhase();
        }


        xd::GroupQueryParams info;
        info.input = set;
        info.output = xd::getTemporaryPath();
        info.columns = field_str;
        info.having = having;
        info.group = group_by_str;

        bool res = db->groupQuery(&info, job);

        if (join_output_path.length() > 0)
            db->deleteFile(join_output_path);

        if (job && job->getCancelled())
        {
            error.setError(xd::errorCancelled, L"Job cancelled");
            return xcm::null;
        }

        if (!res)
        {
            error.setError(xd::errorGeneral, L"Unable to process GROUP BY clause");        
            return xcm::null;
        }

        // set new table
        set = info.output;

        for (f_it = fields.begin(); f_it != fields.end(); ++f_it)
        {
            f_it->expr = L"";
        }
    }
    

    std::wstring field_str;
    std::vector<SelectField>::iterator f_it;
    bool star_added = false;
    
    for (f_it = fields.begin(); f_it != fields.end(); ++f_it)
    {
        if (f_it->star)
        {
            if (!star_added)
            {
                if (!field_str.empty())
                    field_str += L",";

                field_str += L"*";
                star_added = true;
            }

            continue;
        }
        
        if (!field_str.empty())
            field_str += L",";
        
        if (!f_it->expr.empty() && !isSameField(f_it->expr, f_it->name))
        {
            std::wstring f = f_it->expr;
            xd::ColumnInfo colinfo = getColumnInfoMulti(source_tables, f);
            if (colinfo.isOk())
            {
                dequoteField(f);
                field_str += L"[" + f + L"]";
                field_str += L" AS ";
                field_str += L"[" + f_it->name + L"]";
            }
             else
            {
                field_str += f_it->expr;
                field_str += L" AS ";
                field_str += L"[" + f_it->name + L"]";
            }
        }
         else
        {
            std::wstring f = f_it->name;
            dequoteField(f);

            field_str += L"[" + f + L"]";
        }
    }


    std::wstring order_by_str;
    std::vector<OrderByField>::iterator order_by_it;
    for (order_by_it = order_by_fields.begin(); order_by_it != order_by_fields.end(); ++order_by_it)
    {
        if (!order_by_str.empty())
            order_by_str += L",";

        // grouping operation already renamed the field
        // to it's final output name

        order_by_str += L"[" + order_by_it->name + L"]";

        if (order_by_it->descending)
        {
            order_by_str += L" DESC";
        }
    }


    // create iterator

    xd::IJobPtr create_iter_job = db->createJob();

    if (order_by_str.length() > 0)
    {
        if (job)
        {
            ijob->startPhase(create_iter_job);
        }
    }



    xd::IIteratorPtr iter;
    iter = db->query(set, 
                     field_str,
                     L"",
                     order_by_str,
                     create_iter_job);
    
    if (group_operation || join_operation)
    {
        // set this iterator to clean up after destruction
        iter->setIteratorFlags(xd::ifTemporary, xd::ifTemporary);
    }

    if (create_iter_job->getCancelled())
    {
        if (job)
        {
            job->cancel();
        }

        error.setError(xd::errorCancelled, L"Job cancelled");
        return xcm::null;
    }

    if (iter.isNull())
    {
        error.setError(xd::errorGeneral, L"Unable to process SELECT statement");     
        return xcm::null;
    }


    if ((flags & xd::sqlAlwaysCopy) == 0 && !p_distinct && !p_into)
    {
        return iter;
    }



    // create output set
    
    xd::FormatDefinition output_structure;
    xd::Structure iter_structure = iter->getStructure();

    for (f_it = fields.begin(); f_it != fields.end(); ++f_it)
    {
        const xd::ColumnInfo& itercol = iter_structure.getColumnInfo(f_it->name);
        if (itercol.isNull())
            continue;

        xd::ColumnInfo colinfo;
        colinfo.name = itercol.name;
        colinfo.type = itercol.type;
        colinfo.width = itercol.width;
        colinfo.scale = itercol.scale;

        output_structure.createColumn(colinfo);
    }


    bool create_result;
    bool temporary = false;

    // check if the output file will be in a mount
    if (output_path.length() > 0 && db->getMountDatabase(output_path).isOk())
    {
        create_result = db->createTable(output_path, output_structure);
    }
     else
    {
        output_path = xd::getTemporaryPath();
        create_result = db->createTable(output_path, output_structure);
        temporary = true;
    }

    if (!create_result)
    {
        error.setError(xd::errorGeneral, L"Unable to process SELECT statement");
        return xcm::null;
    }

    iter->goFirst();

    if (p_distinct)
    {
        insertDistinct(db, output_path, iter, job);

        if (job && job->getCancelled())
        {
            db->deleteFile(output_path);
            error.setError(xd::errorCancelled, L"Job cancelled");
            return xcm::null;
        }
    }
     else
    {
        if (job)
        {
            ijob->startPhase();
        }

        xdcmnInsert(db, iter, output_path,  L"",  0,  job);

        if (job && job->getCancelled())
        {
            db->deleteFile(output_path);
            error.setError(xd::errorCancelled, L"Job cancelled");
            return xcm::null;
        }
    }


    iter.clear();


    iter = db->query(output_path, L"", L"", L"", NULL);

    if (iter.isOk() && temporary)
    {
        // set this iterator to clean up after destruction
        iter->setIteratorFlags(xd::ifTemporary, xd::ifTemporary);
    }

    return iter;
}


