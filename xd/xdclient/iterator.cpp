/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-22
 *
 */


#include "xdclient.h"
#include "iterator.h"
#include "request.h"
#include <kl/portable.h>
#include "../xdcommon/xdcommon.h"
#include <kl/json.h>

const std::string empty_string = "";
const std::wstring empty_wstring = L"";


ClientIterator::ClientIterator(ClientDatabase* database)
{
    m_current_row = 0;
    m_current_row_ptr = NULL;
    m_cache_row_count = 0;
    m_cache_start = 0;

    m_row_count = (xd::rowpos_t)-1;

    m_database = database;
    m_database->ref();
}

ClientIterator::~ClientIterator()
{
    // let server know that we no longer need this resource

    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(m_url_query, L"close", &params);
    kl::JsonNode response;
    response.fromString(sres);


    // clear the data access info
    std::vector<HttpDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
        delete (*it);

    if (m_database)
        m_database->unref();
}

bool ClientIterator::init(const std::wstring& handle, const std::wstring& url_query)
{
    m_handle = handle;
    m_url_query = url_query;

    return refreshDataAccessInfo();
}

bool ClientIterator::init(const xd::QueryParams& qp)
{
    ServerCallParams params;
    params.setParam(L"columns", qp.columns);
    params.setParam(L"where", qp.where);
    params.setParam(L"order", qp.order);
    params.setParam(L"limit", L"1");

    std::wstring sres = m_database->serverCall(qp.from, L"", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (response.childExists("total_count"))
    {
        m_row_count = response["total_count"].getInteger();
    }

    if (!response["success"].getBoolean())
    {
        return xcm::null;
    }

    return init(response["handle"], qp.from);
}

void ClientIterator::setTable(const std::wstring& tbl)
{
    // TODO: implement
}

std::wstring ClientIterator::getTable()
{
    // TODO: implement
    return L"";
}


void ClientIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
}


unsigned int ClientIterator::getIteratorFlags()
{
    if (m_row_count != (xd::rowpos_t)-1)
        return xd::ifFastRowCount;
    
    return 0;
}

xd::rowpos_t ClientIterator::getRowCount()
{
    if (m_row_count != (xd::rowpos_t)-1)
        return m_row_count;
    return 0;
}

xd::IDatabasePtr ClientIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr ClientIterator::clone()
{
    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(m_url_query, L"clone", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    ClientIterator* iter = new ClientIterator(m_database);
    if (!iter->init(response["handle"], L""))
    {
        delete iter;
        return xcm::null;
    }

    return static_cast<xd::IIterator*>(iter);
}



static wchar_t* js_zl_strchr(wchar_t* str,
                             wchar_t ch,
                             const wchar_t* open_parens,
                             const wchar_t* close_parens)
{
    int paren_level = 0;
    wchar_t quote_char = 0;
    wchar_t* start = str;

    while (*str)
    {
        if (quote_char)
        {
            if (*str == L'\\')
            {
                // escaped character
                str++;
                if (!*str)
                    break;
                str++;
                continue;
            }   
            
            if (*str == quote_char)
            {
                quote_char = 0;
                str++;
                continue;
            }
        }
         else
        {
            if (*str == L'\'')
                quote_char = L'\'';
            else if (*str == L'"')
                quote_char = L'\"';

            if (open_parens && close_parens)
            {
                if (wcschr(open_parens, *str))
                    paren_level++;
                else if (wcschr(close_parens, *str))
                    paren_level--;
            }

            if (paren_level == 0 && *str == ch)
                return str;
        }
        
        str++;
    }

    return NULL;
}

static void dequote(std::wstring& str)
{
    size_t len = str.length();
    if (len > 1)
    {
        if (str[0] == '"' && str[len-1] == '"')
        {
            str.erase(len-1, 1);
            str.erase(0, 1);

            if (str.find('\\') != str.npos)
            {
                kl::replaceStr(str, L"\\\"", L"\"");
                kl::replaceStr(str, L"\\\\", L"\\");
            }
        }
    }
}


void ClientIterator::skip(int delta)
{
    int new_row = (int)m_current_row + delta;


    if (m_cache_start != 0 && new_row >= m_cache_start && new_row <= m_cache_start + m_cache_row_count - 1)
    {
        // row is in cache
        m_current_row_ptr = m_current_row_ptr = &(m_cache_rows[new_row - (int)m_cache_start]);
        m_current_row = new_row;
    }
     else
    {
        int rows_to_fetch;

        // if getting random access calls, fetch a smaller buffer,
        // otherwise fetch a buffer with a larger read-ahead
        // for sequential access

        if (abs(delta) < 50)
            rows_to_fetch = 300;
             else
            rows_to_fetch = 50;

        // the row is not in the cache, so we need to fetch it

        ServerCallParams params;
        params.setParam(L"handle", m_handle);
        params.setParam(L"start", kl::itowstring(new_row));
        params.setParam(L"limit", kl::itowstring(rows_to_fetch));
        std::wstring sres = m_database->serverCall(m_url_query, L"read", &params);


        m_cache_rows.clear();
        m_cache_rows.resize(rows_to_fetch);

        wchar_t* data = (wchar_t*)sres.c_str();
        wchar_t* row_start;
        wchar_t* row_end;

        wchar_t* col_start;
        wchar_t* col_end;
        wchar_t* colon;

        std::wstring colname,colvalue;

        size_t rown = 0;
    
        row_start = wcschr(data, '[');
        if (row_start)
            row_start = wcschr(row_start+1, '{');
        while (row_start)
        {
            row_start++;
            row_end = js_zl_strchr(row_start, '}', NULL, NULL);
            if (!row_end)
                break;

            ClientCacheRow& cache_row = m_cache_rows[rown];
            cache_row.values.clear();

            col_start = row_start;
            col_end = NULL;
            while (col_end != row_end)
            {
                col_end = js_zl_strchr(col_start, ',', NULL, NULL);
                if (col_end == NULL || col_end > row_end)
                    col_end = row_end;

                colon = col_start;
                while (*colon != ':' && colon < col_end)
                    colon++;
                if (colon == col_end)
                    break; // malformed

                colname.assign(col_start, colon);
                kl::trim(colname);
                dequote(colname);

                colvalue.assign(colon+1, col_end);
                kl::trim(colvalue);
                dequote(colvalue);

                cache_row.values.push_back(colvalue);

                col_start = col_end+1;
            }
            
            rown++;
            row_start = wcschr(row_end, '{');
        }



        /*
        m_cache_rows.clear();
        m_cache_rows.resize(300);

        std::vector<std::wstring> colvec;
        std::wstring rowstr;

        wchar_t* data = (wchar_t*)sres.c_str();
        wchar_t* start;
        wchar_t* end;

        size_t coln, colcnt, rown = 0;
    
        start = wcschr(data, '[');
        if (start)
            start = wcschr(start+1, '[');
        while (start)
        {
            start++;
            end = zl_strchr(start, L']', NULL, NULL);
            if (!end)
                break;

            rowstr.assign(start,end);
            colvec.clear();
            kl::parseDelimitedList(rowstr, colvec, ',', true);

            ClientCacheRow& cache_row = m_cache_rows[rown];

            colcnt = colvec.size();
            cache_row.values.resize(colcnt);
            for (coln = 0; coln < colcnt; ++coln)
            {
                kl::trim(colvec[coln]);
                dequote(colvec[coln]);
                cache_row.values[coln] = colvec[coln];
            }
            
            rown++;
            start = wcschr(end, '[');
        }

        */

        /*
        clock_t c1,c2;
        c1 = clock();
        kl::JsonNode response;
        response.fromString(sres);
        c2 = clock();

        if (!response["success"].getBoolean())
        {
            return;
        }


        kl::JsonNode rows = response["rows"];
        size_t rown, rowcnt = rows.getChildCount();
        size_t coln, colcnt;

        m_cache_rows.clear();
        m_cache_rows.resize(rowcnt);

        for (rown = 0; rown < rowcnt; ++rown)
        {
            kl::JsonNode row = rows[rown];
            colcnt = row.getChildCount();

            ClientCacheRow& cache_row = m_cache_rows[rown];
            cache_row.values.resize(colcnt);
            for (coln = 0; coln < colcnt; ++coln)
                cache_row.values[coln] = row[coln].getString();
        }
        */


        m_cache_rows.resize(rown);

        m_cache_start = new_row;
        m_cache_row_count = rown;

        if (new_row - m_cache_start >= m_cache_row_count)
            m_current_row_ptr = NULL;
             else
            m_current_row_ptr = &(m_cache_rows[new_row - (int)m_cache_start]);

        m_current_row = new_row;
    }

    
}

void ClientIterator::goFirst()
{
    m_current_row = 0;
    skip(1);
}

void ClientIterator::goLast()
{
}

double ClientIterator::getPos()
{
    return (double)m_current_row;
}

xd::rowid_t ClientIterator::getRowId()
{
    return m_current_row;
}

bool ClientIterator::bof()
{
    return false;
}

bool ClientIterator::eof()
{
    if (m_current_row == 0)
        return true;
    if (!m_current_row_ptr)
        return true;

   // if (m_current_row > m_cache_row_count)
   //     return true;

    return false;
}

bool ClientIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool ClientIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool ClientIterator::setPos(double pct)
{
    return false;
}

void ClientIterator::goRow(const xd::rowid_t& rowid)
{
    int offset = (int)(rowid - m_current_row);
    skip(offset);
}

xd::Structure ClientIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure;

    std::vector<HttpDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        xd::ColumnInfo col;

        col.name = (*it)->name;
        col.type = (*it)->type;
        col.width = (*it)->width;
        col.scale = (*it)->scale;
        col.column_ordinal = (*it)->ordinal - 1;

        if ((*it)->isCalculated())
        {
            col.expression = (*it)->expr_text;
            col.calculated = true;
        }

        m_structure.createColumn(col);
    }
    
    return m_structure;
}

bool ClientIterator::refreshStructure()
{
    clearCache();

    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(m_url_query, L"refresh", &params);
    kl::JsonNode response;
    response.fromString(sres);

    refreshDataAccessInfo();

    skip(0);  // reload data into cache

    return true;
}

bool ClientIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<HttpDataAccessInfo*>::iterator it2;

    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                HttpDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionModify)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                if (it->params.mask & xd::ColumnInfo::maskName)
                {
                    std::wstring new_name = it->params.name;
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->params.mask & xd::ColumnInfo::maskType)
                {
                    (*it2)->type = it->params.type;
                }

                if (it->params.mask & xd::ColumnInfo::maskWidth)
                {
                    (*it2)->width = it->params.width;
                }

                if (it->params.mask & xd::ColumnInfo::maskScale)
                {
                    (*it2)->scale = it->params.scale;
                }

                if (it->params.mask & xd::ColumnInfo::maskExpression)
                {
                    if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->params.expression;
                    (*it2)->expr = parse(it->params.expression);
                    (*it2)->calculated = true;
                }
            }
        }
    }

    // handle create
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionCreate)
            continue;

        if (it->params.expression.length() > 0)
        {
            HttpDataAccessInfo* dai = new HttpDataAccessInfo;
            dai->name = it->params.name;
            dai->type = it->params.type;
            dai->width = it->params.width;
            dai->scale = it->params.scale;
            dai->ordinal = (int)m_fields.size();
            dai->expr_text = it->params.expression;
            dai->expr = parse(it->params.expression);
            dai->calculated = true;
            m_fields.push_back(dai);
        }
    }

    // handle insert
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->params.column_ordinal;
        if (insert_idx < 0 || insert_idx >= (int)m_fields.size())
            continue;
        
        if (it->params.expression.length() > 0)
        {
            HttpDataAccessInfo* dai = new HttpDataAccessInfo;
            dai->name = it->params.name;
            dai->type = it->params.type;
            dai->width = it->params.width;
            dai->scale = it->params.scale;
            dai->ordinal = (int)m_fields.size();
            dai->expr_text = it->params.expression;
            dai->expr = parse(it->params.expression);
            dai->calculated = true;
            m_fields.insert(m_fields.begin()+insert_idx, dai);
        }
    }
    

    // the next call to getStructure() will refresh m_structure
    m_structure = xd::Structure();

    return true;
}

xd::objhandle_t ClientIterator::getHandle(const std::wstring& expr)
{
    std::vector<HttpDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        HttpDataAccessInfo* dai = new HttpDataAccessInfo;
        dai->expr = NULL;
        dai->type = xd::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<xd::IIterator*>(this),
                                         expr.substr(4),
                                         false))
        {
            delete dai;
            return 0;
        }
        
        m_exprs.push_back(dai);
        return (xd::objhandle_t)dai;
    }
    
    kscript::ExprParser* parser = parse(expr);
    if (!parser)
        return (xd::objhandle_t)0;

    HttpDataAccessInfo* dai = new HttpDataAccessInfo;
    dai->expr = parser;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

xd::ColumnInfo ClientIterator::getInfo(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* dai = (HttpDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::ColumnInfo();

    xd::ColumnInfo colinfo;

    colinfo.name = dai->name;
    colinfo.type = dai->type;
    colinfo.width = dai->width;
    colinfo.scale = dai->scale;
    colinfo.expression = dai->expr_text;
    colinfo.calculated = dai->isCalculated();

    return colinfo;
}

int ClientIterator::getType(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* dai = (HttpDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;

    return dai->type;
}

bool ClientIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<HttpDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

int ClientIterator::getRawWidth(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* dai = (HttpDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* ClientIterator::getRawPtr(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* dai = (HttpDataAccessInfo*)data_handle;
    if (dai == NULL)
        return NULL;
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return NULL;
}


const std::string& ClientIterator::getString(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return empty_string;

    if (handle->expr)
    {
        handle->expr->eval(&handle->expr_result);
        handle->str_result = kl::tostring(handle->expr_result.getString());
        return handle->str_result;
    }

    if (!m_current_row_ptr)
        return empty_string;

    handle->str_result = kl::tostring(m_current_row_ptr->values[handle->ordinal]);
    return handle->str_result;
}

const std::wstring& ClientIterator::getWideString(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return empty_wstring;

    if (handle->expr)
    {
        handle->expr->eval(&handle->expr_result);
        handle->str_result = kl::tostring(handle->expr_result.getString());
        return handle->wstr_result;
    }

    if (!m_current_row_ptr)
        return empty_wstring;

    handle->wstr_result = m_current_row_ptr->values[handle->ordinal];
    return handle->wstr_result;
}

static xd::datetime_t parseDateTime2(const std::wstring& wstr)
{
    char buf[32];
    int parts[6] = { 0,0,0,0,0,0 };
    size_t len = wstr.length();
    if (len > 30)
        return 0;
        
    std::string str = kl::tostring(wstr);
    strcpy(buf, str.c_str());
    
    size_t i;
    char* start = buf;
    size_t partcnt = 0;
    bool last = false;
    for (i = 0; i <= sizeof(buf); ++i)
    {
        if (buf[i] == '/' || buf[i] == '-' || buf[i] == ':' || buf[i] == '.' || buf[i] == ' ' || buf[i] == 0)
        {
            if (buf[i] == 0)
                last = true;
            buf[i] = 0;
            parts[partcnt++] = atoi(start);
            start = buf+i+1;
            if (partcnt == 6 || last)
                break;
        }
    }

    if (partcnt < 3)
    {
        return 0;
    }
     else if (partcnt == 6)
    {
        xd::DateTime dt(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5]);
        return dt.getDateTime();
    }
     else if (partcnt >= 3)
    {
        xd::DateTime dt(parts[0], parts[1], parts[2]);
        return dt.getDateTime();
    }

    return 0;
}

xd::datetime_t ClientIterator::getDateTime(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return 0;

    if (handle->expr)
    {
        handle->expr->eval(&handle->expr_result);
        kscript::ExprDateTime edt = handle->expr_result.getDateTime();

        xd::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        if (handle->type == xd::typeDateTime)
            dt |= edt.time;

        return dt;
    }

    if (!m_current_row_ptr)
        return 0;

    return parseDateTime2(m_current_row_ptr->values[handle->ordinal]);
}

double ClientIterator::getDouble(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return 0.0f;

    if (handle->expr)
    {
        handle->expr->eval(&handle->expr_result);
        return handle->expr_result.getDouble();
    }

    if (!m_current_row_ptr)
        return 0.0f;

    return kl::wtof(m_current_row_ptr->values[handle->ordinal]);
}

int ClientIterator::getInteger(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return 0;

    if (handle->expr)
    {
        handle->expr->eval(&handle->expr_result);
        return handle->expr_result.getInteger();
    }

    if (!m_current_row_ptr)
        return 0;

    return kl::wtoi(m_current_row_ptr->values[handle->ordinal]);
}

bool ClientIterator::getBoolean(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return false;

    if (handle->expr)
    {
        handle->expr->eval(&handle->expr_result);
        return handle->expr_result.getBoolean();
    }

    if (!m_current_row_ptr)
        return false;

    return (m_current_row_ptr->values[handle->ordinal] == L"true" ? true : false);
}

bool ClientIterator::isNull(xd::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return true;

    return false;
}

bool ClientIterator::updateCacheRow(xd::rowid_t rowid,
                                   xd::ColumnUpdateInfo* info,
                                   size_t info_size)
{
    return false;
}

bool ClientIterator::refreshDataAccessInfo()
{
    m_structure = xd::Structure();

    // clear out any existing data access info
    clearDataAccessInfo();

    // get the iterator structure

    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(m_url_query, L"describetable", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;


    // fill out the data access information array

    xd::Structure structure = m_database->jsonToStructure(response);
    size_t i, count = structure.getColumnCount();

    for (i = 0; i < count; ++i)
    {
        const xd::ColumnInfo& info = structure.getColumnInfoByIdx(i);

        HttpDataAccessInfo* dai = new HttpDataAccessInfo;
        dai->name = info.name;
        dai->type = info.type;
        dai->width = info.width;
        dai->scale = info.scale;
        dai->ordinal = (int)m_fields.size();
        dai->calculated = info.calculated;
        dai->expr_text = info.expression;

        m_fields.push_back(dai);
    }

    return true;
}

void ClientIterator::clearDataAccessInfo()
{
    std::vector<HttpDataAccessInfo*>::iterator it, it_end;
    it_end = m_fields.end();
    
    for (it = m_fields.begin(); it != it_end; ++it)
    {
        delete *it;
    }
    
    m_fields.clear();
}

void ClientIterator::clearCache()
{
    m_cache_row_count = 0;
    m_cache_start = 0;
    m_current_row_ptr = NULL;
    m_cache_rows.clear();
}
