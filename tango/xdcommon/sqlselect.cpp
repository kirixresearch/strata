/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
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
#include "tango.h"
#include "util.h"
#include "exindex.h"
#include "keylayout.h"
#include "jobinfo.h"
#include "sqlcommon.h"
#include "cmndynamicset.h"
#include "errorinfo.h"
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
    tango::ISetPtr set;
    tango::IStructurePtr structure;
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
    tango::IIteratorPtr source_iter;
    tango::objhandle_t source_handle;
    tango::objhandle_t dest_handle;
};


struct JoinInfo
{
    tango::ISetPtr left_set;
    tango::ISetPtr right_set;
    tango::IStructurePtr right_structure;
    tango::IIteratorPtr right_iter;
    KeyLayout left_key;
    KeyLayout right_key;
    std::wstring left;
    std::wstring right;

    int join_type;
};


static bool isDelimiterChar(wchar_t ch)
{
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
        if (it->structure->getColumnExist(field_name))
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

static tango::IColumnInfoPtr getColumnInfoMulti(std::vector<SourceTable>& s,
                                                const std::wstring& field_name,
                                                SourceTable** tbl = NULL)
{
    if (!isUniqueFieldName(s, field_name))
        return xcm::null;

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
    for (it = s.begin(); it != s.end(); ++it)
    {
        if (!alias_part.empty())
        {
            if (wcscasecmp(alias_part.c_str(), it->alias.c_str()) != 0)
                continue;
        }
        
        tango::IColumnInfoPtr col_info = it->structure->getColumnInfo(field_part);
        if (col_info)
        {
            if (tbl)
                *tbl = &(*it);

            return col_info;
        }
    }

    return xcm::null;
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
        alias = input.substr(as_pos+3);
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
                result += ch;
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
        int col_count = st_it->structure->getColumnCount();
        int i;
        
        for (i = 0; i < col_count; ++i)
        {
            std::wstring full_name;
            std::wstring colname = st_it->structure->getColumnName(i);
            std::wstring q_colname = L"[" + colname + L"]";
            tango::IColumnInfoPtr colinfo;

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
                                

static void normalizeFieldNames(std::vector<SourceTable>& source_tables,
                                std::vector<SelectField>& fields)
{
    std::vector<SourceTable>::iterator st_it;
    
    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        int col_count = st_it->structure->getColumnCount();
        int i;

        for (i = 0; i < col_count; ++i)
        {
            std::wstring full_name;
            std::wstring colname = st_it->structure->getColumnName(i);
            std::wstring q_colname = L"[" + colname + L"]";
            tango::IColumnInfoPtr colinfo;

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
    tango::objhandle_t source_handle;
    tango::objhandle_t dest_handle;
};


inline bool getBitState(unsigned char* buf, tango::rowpos_t bit)
{
    return (buf[bit/8] & (1 << (bit%8))) ? true : false;
}

inline void setBitState(unsigned char* buf, tango::rowpos_t bit)
{
    buf[bit/8] |= (1 << (bit%8));
}

class DistinctBulkInsertProgress : public IIndexProgress
{
public:
    tango::IJob* job;
    IJobInternal* ijob;
    std::wstring filename;
    bool cancelled;

    void updateProgress(tango::rowpos_t cur_count,
                        tango::rowpos_t max_count,
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
                    ijob->setStatus(tango::jobFailed);
                    *cancel = true;
                    cancelled = true;
                }
            }
        }
    }
};


static bool insertDistinct(tango::IDatabase* db,
                           tango::ISetPtr target,
                           tango::IIteratorPtr src_iter,
                           tango::IJob* job)
{
    ExIndex* index = new ExIndex;
    std::vector<InsertDistinctField> fields;
    std::vector<InsertDistinctField>::iterator fit;
    
    // put index file and temporary files in the temp path
    tango::IAttributesPtr attr = db->getAttributes();
    if (!attr)
        return false;
    std::wstring temp_dir = attr->getStringAttribute(tango::dbattrTempDirectory);
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
                       sizeof(tango::tango_uint64_t),
                       sizeof(tango::rowpos_t),
                       true))
    {
        return false;
    }

    // create row inserter

    tango::IRowInserterPtr sp_output = target->getRowInserter();
    tango::IRowInserter* output = sp_output.p;
    tango::IIterator* iter = src_iter.p;
    output->startInsert(L"*");

    // create the key layout and get input and output handles

    KeyLayout key;
    key.setIterator(src_iter);

    tango::IStructurePtr s = src_iter->getStructure();

    int col_count = s->getColumnCount();
    int i, key_len;

    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr info = s->getColumnInfoByIdx(i);
        std::wstring col_name = info->getName();

        key.addKeyPart(col_name);

        InsertDistinctField f;
        f.source_handle = iter->getHandle(col_name);
        f.dest_handle = output->getHandle(col_name);
        f.type = info->getType();

        fields.push_back(f);
    }

    key_len = key.getKeyLength();


    IJobInternalPtr sp_ijob = job;
    IJobInternal* ijob = sp_ijob.p;
    tango::rowpos_t cur_row = 1;
    tango::rowpos_t row_count;


    // PHASE 1: populate the index with crc64 hash values

    tango::ISetPtr src_set = src_iter->getSet();
    index->startBulkInsert(src_set->getRowCount());
    src_set.clear();

    if (job)
    {
        ijob->startPhase();
    }

    tango::tango_uint64_t crc;
    while (!iter->eof())
    {
        crc64(key.getKey(), key_len, &crc);
        index->insert(&crc,
                      sizeof(tango::tango_uint64_t),
                      &cur_row,
                      sizeof(tango::rowpos_t));

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


    tango::tango_uint64_t last_crc = 0;

    IIndexIterator* idx_iter = index->createIterator();
    idx_iter->goFirst();
    cur_row = 1;

    if (job)
    {
        ijob->startPhase();
    }

    while (!idx_iter->isEof())
    {
        crc = *(tango::tango_uint64_t*)idx_iter->getKey();
        if (crc != last_crc)
        {
            setBitState(bitmask, *(tango::rowpos_t*)idx_iter->getValue());
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
                    case tango::typeCharacter:
                        output->putString(fit->dest_handle,
                                  iter->getString(fit->source_handle));
                        break;

                    case tango::typeWideCharacter:
                        output->putWideString(fit->dest_handle,
                                  iter->getWideString(fit->source_handle));
                        break;

                    case tango::typeDouble:
                    case tango::typeNumeric:
                        output->putDouble(fit->dest_handle,
                                  iter->getDouble(fit->source_handle));
                        break;

                    case tango::typeInteger:
                        output->putInteger(fit->dest_handle,
                                  iter->getInteger(fit->source_handle));
                        break;

                    case tango::typeDate:
                    case tango::typeDateTime:
                        output->putDateTime(fit->dest_handle,
                                  iter->getDateTime(fit->source_handle));
                        break;

                    case tango::typeBoolean:
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

    if (job && (job->getStatus() != tango::jobCancelled))
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

    tango::datetime_t dt, d, t;
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


    tango::IColumnInfoPtr colinfo;
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

static kscript::ExprParser* createJoinExprParser(
                                      std::vector<JoinField>& all_fields)

{
    kscript::ExprParser* parser;
    std::vector<JoinField>::iterator jf_it;

    parser = createExprParser();

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
            case tango::typeCharacter:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeString,
                                  (void*)_bindFieldString,
                                  &(*jf_it));
                }
                break;

            case tango::typeNumeric:
            case tango::typeDouble:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeDouble,
                                  (void*)_bindFieldDouble,
                                  &(*jf_it));
                }
                break;

            case tango::typeInteger:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeInteger,
                                  (void*)_bindFieldInteger,
                                  &(*jf_it));
                }
                break;

            case tango::typeDate:
            case tango::typeDateTime:
                for (n_it = names.begin(); n_it != names.end(); ++n_it)
                {
                    parser->addVarBinding(*n_it,
                                  false,
                                  kscript::Value::typeDateTime,
                                  (void*)_bindFieldDateTime,
                                  &(*jf_it));
                }
                break;

            case tango::typeBoolean:
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

static void joinDoInsert(tango::IRowInserter* output,
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
                case tango::typeCharacter:
                    output->putString(fit->dest_handle,
                              fit->source_iter->getString(fit->source_handle));
                    break;

                case tango::typeWideCharacter:
                    output->putWideString(fit->dest_handle,
                              fit->source_iter->getWideString(fit->source_handle));
                    break;

                case tango::typeDouble:
                case tango::typeNumeric:
                    output->putDouble(fit->dest_handle,
                              fit->source_iter->getDouble(fit->source_handle));
                    break;

                case tango::typeInteger:
                    output->putInteger(fit->dest_handle,
                              fit->source_iter->getInteger(fit->source_handle));
                    break;

                case tango::typeDate:
                case tango::typeDateTime:
                    output->putDateTime(fit->dest_handle,
                              fit->source_iter->getDateTime(fit->source_handle));
                    break;

                case tango::typeBoolean:
                    output->putBoolean(fit->dest_handle,
                              fit->source_iter->getBoolean(fit->source_handle));
                    break;
            }
        }
    }

    output->insertRow();
}


static void joinIterLoop(tango::IIterator* left,
                         tango::IRowInserter* output,
                         std::vector<JoinInfo>& joins,
                         std::vector<JoinField>& fields,
                         kscript::ExprParser* where_expr,
                         int join_idx,
                         int join_count)
{
    tango::IIterator* right = joins[join_idx].right_iter.p;

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


static tango::ISetPtr doJoin(tango::IDatabasePtr db, 
                             std::vector<SourceTable>& source_tables,
                             std::vector<SelectField>& columns,
                             const std::wstring& where,
                             tango::IJob* job)
{
    tango::ISetPtr left;
    tango::ISetPtr output_set;
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
            if (left.isOk())
                return xcm::null;

            left = st_it->set;
            left_table_info = &(*st_it);
        }
    }

    if (left.isNull())
        return xcm::null;

    tango::IIteratorPtr left_iter = left->createIterator(L"", L"", NULL);
    if (left_iter.isNull())
        return xcm::null;

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
            return xcm::null;

        delete parser;


        //  find out who the left table is

        std::vector<std::wstring> left_parts;
        std::vector<std::wstring>::iterator it;
        tango::IColumnInfoPtr colinfo;

        kl::parseDelimitedList(jparse.left, left_parts, ',', true);

        SourceTable* tbl = NULL;
        SourceTable* last_tbl = NULL;
        for (it = left_parts.begin(); it != left_parts.end(); ++it)
        {
            colinfo = getColumnInfoMulti(source_tables, *it, &tbl);
            if (last_tbl != NULL)
            {
                if (tbl != last_tbl)
                {
                    return xcm::null;
                }
            }
            last_tbl = tbl;
        }

        if (!tbl)
            return xcm::null;

        JoinInfo j;
        j.left_set = tbl->set;
        j.right_set = st_it->set;
        j.right_structure = st_it->structure;
        j.left = jparse.left;
        j.right = jparse.right;
        j.join_type = st_it->join_type;

        joins.push_back(j);
    }

    
    // create right-side iterators

    std::vector<JoinInfo>::iterator jit, jit2;

    for (jit = joins.begin(); jit != joins.end(); ++jit)
    {
        tango::IJobPtr iter_job = db->createJob();

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

        tango::rowpos_t old_max_count = 0;
        if (ijob)
        {
            old_max_count = job->getMaxCount();
            ijob->setMaxCount(jit->right_set->getRowCount());
        }
        

        // create iterator
        jit->right_iter = jit->right_set->createIterator(L"",
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
            return xcm::null;
        }

        if (jit->right_iter.isNull())
            return xcm::null;
            
            
            
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
            return xcm::null;


        // find left iterator
        tango::IIteratorPtr join_left_iter;
        for (jit2 = joins.begin(); jit2 != joins.end(); ++jit2)
        {
            if (&(*jit2) == &(*jit))
                continue;

            if (jit->left_set == jit2->right_set)
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
            tango::IColumnInfoPtr colinfo;
  
            SourceTable* tbl = NULL;
            colinfo = getColumnInfoMulti(source_tables, *it, &tbl);
            if (colinfo.isNull())
                return xcm::null;
            if (tbl->set != jit->right_set)
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


            if (!jit->left_key.addKeyPart(left,
                                     colinfo->getType(),
                                     colinfo->getWidth()))
            {
                return xcm::null;
            }

            ++idx;
        }
    }


    // populate join fields vector

    std::vector<SelectField>::iterator sf_it;
    for (sf_it = columns.begin(); sf_it != columns.end(); ++sf_it)
    {
        // this stuff is not really done; what needs to happen is the join
        // expressions need to be parsed to find out which fields are
        // referenced in the join expressions; all reference fields should
        // then be included in the output join structure, and dynamic fields
        // should be created for constructions like "T1.F1 + T2.F1 AS Dynfield"

        JoinField jf;
        jf.name = sf_it->expr;
        //if (jf.name.empty())
        //    jf.name = sf_it->name;
        jf.source_handle = 0;
        jf.dest_handle = 0;

        if (jf.name != sf_it->name)
        {
            jf.alias = sf_it->name;
        }


        SourceTable* src_table = NULL;
        tango::IColumnInfoPtr colinfo;

        // this is a temporary measure
        if (isGroupFunction(sf_it->name))
            continue;

        if (isGroupFunction(sf_it->expr))
        {
            jf.name = sf_it->expr;
            jf.name = kl::afterFirst(jf.name, L'(');
            jf.name = kl::beforeLast(jf.name, L')');
            kl::trim(jf.name);


            std::wstring func_name = kl::beforeFirst(sf_it->expr, L'(');
            kl::trim(func_name);
            kl::makeUpper(func_name);
            if (func_name == L"COUNT" ||
                func_name == L"GROUPID")
            {
                // we don't need any additional output for this
                continue;
            }
        }

        dequoteField(jf.name);

        // get the column info for the join field
        colinfo = getColumnInfoMulti(source_tables, jf.name, &src_table);
        if (colinfo.isNull() || !src_table)
        {
            return xcm::null;
        }


        jf.type = colinfo->getType();
        jf.width = colinfo->getWidth();
        jf.scale = colinfo->getScale();

        jf.orig_name = colinfo->getName();
        jf.source_table = src_table;

        // find join field's source iterator

        if (src_table->set == left)
        {
            jf.source_iter = left_iter;
        }
         else
        {
            for (jit = joins.begin(); jit != joins.end(); ++jit)
            {
                if (src_table->set == jit->right_set)
                {
                    jf.source_iter = jit->right_iter;
                    break;
                }
            }
        }

        if (jf.source_iter.isNull())
            return xcm::null;

        // get our source handle
        jf.source_handle = jf.source_iter->getHandle(colinfo->getName());

        jfields.push_back(jf);
    }

    // create output structure
    tango::IStructurePtr output_structure = db->createStructure();

    std::vector<JoinField>::iterator jf_it;
    for (jf_it = jfields.begin(); jf_it != jfields.end(); ++jf_it)
    {
        tango::IColumnInfoPtr colinfo = output_structure->createColumn();
        colinfo->setName(jf_it->name);
        colinfo->setType(jf_it->type);
        colinfo->setWidth(jf_it->width);
        colinfo->setScale(jf_it->scale);
    }

    output_set = db->createSet(L"", output_structure, NULL);

    if (output_set.isNull())
        return xcm::null;

    // create output row inserter

    tango::IRowInserterPtr sp_output = output_set->getRowInserter();
    tango::IRowInserter* output = sp_output.p;

    output->startInsert(L"*");

    // get output handles

    for (jf_it = jfields.begin(); jf_it != jfields.end(); ++jf_it)
    {
        jf_it->dest_handle = output->getHandle(jf_it->name);
        if (jf_it->dest_handle == 0)
            return xcm::null;
    }

    
    // parse where expression, if any

    std::vector<JoinField> all_fields;


    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        int col_count = st_it->structure->getColumnCount();
        int i;

        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = st_it->structure->getColumnInfoByIdx(i);

            JoinField f;

            f.name = st_it->alias;
            f.name += L".";
            f.name += colinfo->getName();

            f.orig_name = colinfo->getName();

            // try to find the field's optional output name
            for (sf_it = columns.begin(); sf_it != columns.end(); ++sf_it)
            {
                if (0 == wcscasecmp(sf_it->expr.c_str(), f.name.c_str()) &&
                    !sf_it->name.empty())
                {
                    f.alias = sf_it->name;
                }
            }


            f.type = colinfo->getType();
            f.width = colinfo->getWidth();
            f.scale = colinfo->getScale();
            f.source_table = &(*st_it);
            f.dest_handle = 0;

            // find source iterator

            if (st_it->set == left)
            {
                f.source_iter = left_iter;
            }
             else
            {
                for (jit = joins.begin(); jit != joins.end(); ++jit)
                {
                    if (st_it->set == jit->right_set)
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

    if (!where.empty())
    {
        parser = createJoinExprParser(all_fields);

        if (!parser->parse(where))
        {
            delete parser;
            return xcm::null;
        }
    }



    if (job)
    {
        ijob->startPhase();
    }


    int join_count = joins.size();
    tango::rowpos_t cur_row = 0;
    tango::IIterator* l = left_iter.p;

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
        return xcm::null;

    return output_set;
}


// see note below about convertToNativeTables.
bool convertToNativeTables(tango::IDatabasePtr db,
                           std::vector<SourceTable>& source_tables,
                           tango::ISetPtr& mainset,
                           tango::IJob* job)
{
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
        tango::IStructurePtr structure = st_it->set->getStructure();
        tango::ISetPtr set = db->createSet(L"", structure, NULL);
        if (set.isNull())
        {
            // fail out
            return false;
        }

        
        tango::IIteratorPtr iter = st_it->set->createIterator(L"", L"", NULL);
        if (iter.isNull())
            return false;
        iter->goFirst();
            
        set->insert(iter, L"", 0, job);
        
        if (mainset == st_it->set)
            mainset = set;
            
        st_it->set = set;
    }
    
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




tango::IIteratorPtr sqlSelect(tango::IDatabasePtr db,
                              const std::wstring& _command,
                              unsigned int flags,
                              ThreadErrorInfo& error,
                              tango::IJob* job)
{
    if (_command.length() == 0)
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; empty SELECT statement");
        return xcm::null;
    }

    wchar_t* command = new wchar_t[_command.length()+1];
    wcscpy(command, _command.c_str());
    DelArrPtr<wchar_t> del_command(command);

    std::vector<tango::ISetPtr> sets;
    std::vector<SelectField> fields;
    std::vector<SourceTable> source_tables;
    std::vector<OrderByField> order_by_fields;
    std::vector<std::wstring> group_by_fields;
    std::wstring output_path;
    std::wstring where_cond;
    std::wstring having;
    tango::ISetPtr set;
    IJobInternalPtr ijob = job;
    bool group_operation = false;
    bool join_operation = false;

    
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
    }

    // error checks

    if (!p_from)
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; missing FROM clause");
        return xcm::null;
    }

    if (p_where && p_where < p_from)
    {
        // WHERE clause can't be before FROM
        error.setError(tango::errorSyntax, L"Invalid syntax; WHERE clause is before FROM clause");        
        return xcm::null;
    }
    
    if (p_into && p_into > p_from)
    {
        // INTO clause can't be after FROM
        error.setError(tango::errorSyntax, L"Invalid syntax; INTO clause is after FROM clause");        
        return xcm::null;
    }
    
    if (p_group_by && (p_group_by < p_where || p_group_by < p_from))
    {
        // GROUP BY clause can't be before FROM or WHERE
        error.setError(tango::errorSyntax, L"Invalid syntax; GROUP BY clause is before FROM clause or WHERE clause");
        return xcm::null;
    }

    if (p_having && (p_having < p_group_by ||
                     p_having < p_where ||
                     p_having < p_from))
    {
        // HAVING clause can't be before FROM or WHERE or GROUP BY
        error.setError(tango::errorSyntax, L"Invalid syntax; HAVING clause is before FROM clause, WHERE clause, or GROUP BY clause");
        return xcm::null;
    }

    if (p_order_by && (p_order_by < p_having ||
                       p_order_by < p_group_by ||
                       p_order_by < p_where ||
                       p_order_by < p_from))
    {
        // ORDER BY clause must be last
        error.setError(tango::errorSyntax, L"Invalid syntax; ORDER BY clause is not at the end of the statement");        
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
                error.setError(tango::errorSyntax, L"Invalid syntax; JOIN clause missing JOIN keyword");
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
                error.setError(tango::errorSyntax, L"Invalid syntax; JOIN clause missing ON keyword");            
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
            j.set = db->openSet(tbl);
        
            if (j.set.isNull())
            {
                wchar_t buf[1024]; // some paths might be long
                swprintf(buf, 1024, L"Unable to open table [%ls]", tbl.c_str()); 
                error.setError(tango::errorGeneral, buf);
                return xcm::null;
            }

            j.structure = j.set->getStructure();
            j.alias = alias;
            j.join_expr = on;
            j.join_type = joinInner;

            if (full)
                j.join_type = joinFullOuter;

            if (left)
                j.join_type = joinLeftOuter;

            if (right)
                j.join_type = joinRightOuter;

            if (outer)
            {
                // this keyword is only used for syntactical clarity

                if (!full && !left && !right)
                {
                    error.setError(tango::errorSyntax, L"Invalid syntax; JOIN clause has OUTER keyword but is missing FULL keyword, LEFT keyword, or RIGHT keyword");                 
                    return xcm::null;
                }
            }

            // make sure only one type of outer join is specified
            if ((full ? 1 : 0) + (left ? 1 : 0) + (right ? 1 : 0) > 1)
            {
                error.setError(tango::errorSyntax, L"Invalid syntax; JOIN clause contains invalid combinations of FULL keyword, LEFT keyword, or RIGHT keyword");
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
            error.setError(tango::errorSyntax, L"Invalid syntax; alternate JOIN syntax not supported");                    
            return xcm::null;
        }
        
        std::wstring table, alias;
        parseTableAndAlias(set_paths[0], table, alias);

        set = db->openSet(table);
        if (set.isNull())
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to open table [%ls]", table.c_str());
            error.setError(tango::errorGeneral, buf);       
            return xcm::null;
        }

        st.join_type = joinNone;
        st.alias = alias;
        st.set = set;
        st.structure = st.set->getStructure();

        source_tables.insert(source_tables.begin(), st);
    }

    // parse the field list
    tango::IStructurePtr set_structure = set->getStructure();

    std::vector<std::wstring> field_strs;

    kl::parseDelimitedList(p_select, field_strs, L',', true);

    if (field_strs.size() == 0)
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; missing column or expression in SELECT clause");
        return xcm::null;
    }

    std::vector<std::wstring>::iterator fstr_it;
    int colname_counter = 0;
    for (fstr_it = field_strs.begin();
         fstr_it != field_strs.end();
         ++fstr_it)
    {
        kl::trim(*fstr_it);

        if (fstr_it->empty())
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; missing column or expression in SELECT clause");
            return xcm::null;
        }

        if (*fstr_it == L"*")
        {
            SelectField f;
            
            std::vector<SourceTable>::iterator st_it;
            
            for (st_it = source_tables.begin();
                 st_it != source_tables.end();
                 ++st_it)
            {
                int col_count = st_it->structure->getColumnCount();
                int i;
        
                for (i = 0; i < col_count; ++i)
                {
                    std::wstring colname = st_it->structure->getColumnName(i);
                    tango::IColumnInfoPtr colinfo;

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
            f.name = fstr_it->substr(as_pos+3);
            f.expr = fstr_it->substr(0, as_pos);
            
            kl::trim(f.name);
            kl::trim(f.expr);
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
                if (period)
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
        std::vector<std::wstring>::iterator it;

        kl::parseDelimitedList(p_group_by, group_by_fields, L',', true);
    
        for (it = group_by_fields.begin();
             it != group_by_fields.end(); ++it)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = getColumnInfoMulti(source_tables, *it);
            
            if (colinfo.isNull())
            {
                wchar_t buf[255];            
                swprintf(buf, 255, L"Invalid syntax; unknown column [%ls] in GROUP BY clause", (*it).c_str());
                error.setError(tango::errorSyntax, buf);
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
            OrderByField f;
            tango::IColumnInfoPtr colinfo;
            
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
                    error.setError(tango::errorSyntax, buf);
                    return xcm::null;
                }

                f.name = real_field_name;
                order_by_fields.push_back(f);
            }
        }
    }


    // normalize field names which are needlessly qualified with an alias
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
        error.setError(tango::errorGeneral);
        return xcm::null;
    }

    if (job && job->getCancelled())
    {
        error.setError(tango::errorCancelled, L"Job cancelled");
        return xcm::null;
    }


    // set up job information

    int join_phase_count = 0;

    tango::ISetPtr left;
    std::vector<SourceTable>::iterator st_it;
    for (st_it = source_tables.begin();
         st_it != source_tables.end();
         ++st_it)
    {
        if (st_it->join_type == joinNone)
            left = st_it->set;
             else
            ++join_phase_count;
    }
    
    if (ijob)
    {
        if (left->getSetFlags() & tango::sfFastRowCount)
            ijob->setMaxCount(left->getRowCount());
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

    if (p_group_by || group_operation)
        phase_count++;

    if (order_by_fields.size() > 0)
        phase_count++;

    // final set creation
    if ((flags & tango::sqlAlwaysCopy) != 0 || p_distinct || p_into)
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

    if (join_operation)
    {
        set = doJoin(static_cast<tango::IDatabase*>(db),
                     source_tables,
                     fields,
                     where_cond,
                     job);

        if (set.isNull())
        {
            error.setError(tango::errorGeneral, L"Unable to process JOIN statement");
            return xcm::null;
        }                     
    }

    if (set.isNull())
    {
        // general purpose check
        error.setError(tango::errorGeneral, L"Unable to process SQL statement");
        return xcm::null;
    }

    // filter selected rows (if necessary)

    if (where_cond.length() > 0 && !join_operation)
    {
#if 0
        // create an iterator which we will insert from
        tango::IIteratorPtr source_iter = set->createIterator(L"", L"", NULL);
        
        /*
        // this step is taken care of a few lines below
        
        // make sure the where expression is valid
        tango::objhandle_t h = source_iter->getHandle(where_cond);
        if (h == 0)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; expression in WHERE clause does not evaluate to a true or false value");            
            return xcm::null;
        }
        source_iter->releaseHandle(h);
        */
        

        CommonDynamicSet* dyn_set = new CommonDynamicSet;
        if (!dyn_set->create(db, set))
        {
            error.setError(tango::errorGeneral, L"Unable to process WHERE clause");
            delete dyn_set;
            return xcm::null;
        }
 
        SqlIterator* s_iter = SqlIterator::createSqlIterator(source_iter, where_cond, job);
        if (!s_iter)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; expression in WHERE clause does not evaluate to a true or false value");            
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
            error.setError(tango::errorCancelled, L"Job cancelled");
            delete dyn_set;
            delete s_iter;
            return xcm::null;
        }

        set = static_cast<tango::ISet*>(dyn_set);
        delete s_iter;
        
#endif
        
        // create an iterator which we will insert from
        tango::IIteratorPtr source_iter = set->createIterator(L"", L"", NULL);
        
        // make sure the where expression is valid
        tango::objhandle_t h = source_iter->getHandle(where_cond);
        if (h == 0)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; expression in WHERE clause does not evaluate to a true or false value");            
            return xcm::null;
        }
        
        source_iter->releaseHandle(h);
        
        // create the filtered set
        CommonDynamicSet* dyn_set = new CommonDynamicSet;
        if (!dyn_set->create(db, set))
        {
            error.setError(tango::errorGeneral, L"Unable to process WHERE clause");
            delete dyn_set;
            return xcm::null;
        }
        
        // start job phase
        if (job)
        {
            ijob->startPhase();
        }
        
        // insert all rows meeting expression
        int res = dyn_set->insert(source_iter, where_cond, 0, job);
        if (res == -1)
        {
            error.setError(tango::errorGeneral, L"Unable to process WHERE clause");
            delete dyn_set;
            return xcm::null;
        }
        
        set = static_cast<tango::ISet*>(dyn_set);
    }


    if (job && job->getCancelled())
    {
        error.setError(tango::errorCancelled, L"Job cancelled");
        return xcm::null;
    }


    // do grouping operation (if necessary)

    if (p_group_by || group_operation)
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

                group_by_str += *g_it;
            }

            if (group_by_str.length() == 0)
            {
                error.setError(tango::errorSyntax, L"Invalid syntax; missing column or expression in GROUP BY clause");
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

        tango::ISetPtr result_set;
        result_set = db->runGroupQuery(set,
                                       group_by_str,
                                       field_str,
                                       L"",
                                       having,
                                       job);

        if (job && job->getCancelled())
        {
            error.setError(tango::errorCancelled, L"Job cancelled");
            return xcm::null;
        }

        if (result_set.isNull())
        {
            error.setError(tango::errorGeneral, L"Unable to process GROUP BY clause");        
            return xcm::null;
        }

        set = result_set;

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
            tango::IColumnInfoPtr colinfo = getColumnInfoMulti(source_tables, f);
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
    for (order_by_it = order_by_fields.begin();
         order_by_it != order_by_fields.end();
         ++order_by_it)
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

    tango::IJobPtr create_iter_job = db->createJob();

    if (order_by_str.length() > 0)
    {
        if (job)
        {
            ijob->startPhase(create_iter_job);
        }
    }



    tango::IIteratorPtr iter;
    iter = set->createIterator(field_str,
                               order_by_str,
                               create_iter_job);
    
    if (create_iter_job->getCancelled())
    {
        if (job)
        {
            job->cancel();
        }

        error.setError(tango::errorCancelled, L"Job cancelled");
        return xcm::null;
    }

    if (iter.isNull())
    {
        error.setError(tango::errorGeneral, L"Unable to process SELECT statement");     
        return xcm::null;
    }


    if ((flags & tango::sqlAlwaysCopy) == 0 && !p_distinct && !p_into)
    {
        return iter;
    }

    // create output set
    
    tango::IStructurePtr output_structure = db->createStructure();
    tango::IStructurePtr iter_structure = iter->getStructure();

    int iter_column_count = iter_structure->getColumnCount();

    for (f_it = fields.begin(); f_it != fields.end(); ++f_it)
    {
        tango::IColumnInfoPtr itercol;
        tango::IColumnInfoPtr colinfo;

        itercol = iter_structure->getColumnInfo(f_it->name);

        if (itercol.isNull())
            continue;

        colinfo = output_structure->createColumn();

        colinfo->setName(itercol->getName());
        colinfo->setType(itercol->getType());
        colinfo->setWidth(itercol->getWidth());
        colinfo->setScale(itercol->getScale());
    }


    tango::ISetPtr output_set;
    bool store_object;

    // check if the output file will be in a mount
    if (output_path.length() > 0 && db->getMountDatabase(output_path).isOk())
    {
        output_set = db->createSet(output_path, output_structure, NULL);
        store_object = false;
    }
     else
    {
        output_set = db->createSet(L"", output_structure, NULL);
        store_object = true;
    }

    if (output_set.isNull())
    {
        error.setError(tango::errorGeneral, L"Unable to process SELECT statement");
        return xcm::null;
    }

    iter->goFirst();

    if (p_distinct)
    {
        insertDistinct(db, output_set, iter, job);

        if (job && job->getCancelled())
        {
            if (!store_object)
                db->deleteFile(output_path);
            error.setError(tango::errorCancelled, L"Job cancelled");
            return xcm::null;
        }
    }
     else
    {
        if (job)
        {
            ijob->startPhase();
        }

        output_set->insert(iter, L"", 0, job);

        if (job && job->getCancelled())
        {
            if (!store_object)
                db->deleteFile(output_path);
            error.setError(tango::errorCancelled, L"Job cancelled");
            return xcm::null;
        }
    }

    if (output_path.length() > 0 && store_object)
    {
        db->storeObject(output_set, output_path);
    }



    /*
    // change all periods to underscores
    tango::IStructurePtr os = output_set->getStructure();
    tango::IStructurePtr ms = output_set->getStructure();
    int col_count = os->getColumnCount();
    int i;
    bool modifying = false;
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = os->getColumnInfoByIdx(i);
        std::wstring name = colinfo->getName();

        bool changed = false;

        int i;
        while (1)
        {
            i = name.find(L'.');
            if (i == -1)
                break;
            name[i] = L'_';
            changed = true;
        }

        if (!changed)
            continue;

        modifying = true;
        tango::IColumnInfoPtr mc = ms->modifyColumn(colinfo->getName());
        mc->setName(name);
    }
    
    if (modifying)
    {
        output_set->modifyStructure(ms, NULL);
    }
    */

    // create iterator, done.

    return output_set->createIterator(L"", L"", NULL);
}


bool sqlOpen(
       tango::IDatabasePtr db,
       const std::wstring& command,
       xcm::IObjectPtr& result_obj,
       ThreadErrorInfo& error,
       tango::IJob* job)
{
    result_obj.clear();
    
    SqlStatement stmt(command);

    stmt.addKeyword(L"OPEN");
    stmt.addKeyword(L"NODEFILE");

    if (!stmt.getKeywordExists(L"OPEN"))
        return false;

    if (!stmt.getKeywordExists(L"NODEFILE"))
        return false;
        
    std::wstring file = stmt.getKeywordParam(L"NODEFILE");
    dequote(file, '[', ']');
    
    result_obj = db->openNodeFile(file);
    return result_obj.isOk();
}

