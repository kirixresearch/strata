/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-22
 *
 */


#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "request.h"
#include <kl/portable.h>
#include "../xdcommon/xdcommon.h"
#include "../../kscript/json.h"


const std::string empty_string = "";
const std::wstring empty_wstring = L"";
const int cache_size = 50;


ClientIterator::ClientIterator(ClientDatabase* database, ClientSet* set)
{
    m_current_row = 0;
    m_current_row_ptr = NULL;
    m_cache_row_count = 0;
    m_cache_start = 0;

    m_database = database;
    m_database->ref();

    m_set = set;
    if (m_set)
        m_set->ref();
}

ClientIterator::~ClientIterator()
{
    // clear the data access info
    std::vector<HttpDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
        delete (*it);

    if (m_set)
        m_set->unref();

    if (m_database)
        m_database->unref();
}

bool ClientIterator::init(const std::wstring& handle, const std::wstring& url_query)
{
    m_handle = handle;
    m_url_query = url_query;

    return refreshDataAccessInfo();
}

tango::ISetPtr ClientIterator::getSet()
{
    return m_set;
}

tango::IDatabasePtr ClientIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr ClientIterator::clone()
{
    return xcm::null;
}

unsigned int ClientIterator::getIteratorFlags()
{
    return 0;
}

void ClientIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
}

void ClientIterator::skip(int delta)
{
    int new_row = (int)m_current_row + delta;



    if (m_cache_start != 0 && new_row >= m_cache_start && new_row <= m_cache_start + m_cache_row_count - 1)
    {
        // row is in cache
        m_current_row_ptr = m_current_row_ptr = &(m_cache_rows[new_row - m_cache_start]);
        m_current_row = new_row;
    }
     else
    {
        // the row is not in the cache, so we need to fetch it

        ServerCallParams params;
        params.setParam(L"handle", m_handle);
        params.setParam(L"start", kl::itowstring(new_row));
        params.setParam(L"limit", L"50");
        std::wstring sres = m_database->serverCall(L"/api/fetchrows", &params);
        JsonNode response;
        response.fromString(sres);

        if (!response["success"].getBoolean())
        {
            return;
        }


        JsonNode rows = response["rows"];
        size_t rown, rowcnt = rows.getCount();
        size_t coln, colcnt;

        m_cache_rows.clear();
        m_cache_rows.resize(rowcnt);

        for (rown = 0; rown < rowcnt; ++rown)
        {
            JsonNode row = rows[rown];
            colcnt = row.getCount();

            ClientCacheRow& cache_row = m_cache_rows[rown];
            cache_row.values.resize(colcnt);
            for (coln = 0; coln < colcnt; ++coln)
                cache_row.values[coln] = row[coln].getString();
        }

        m_cache_start = new_row;
        m_cache_row_count = rowcnt;


        m_current_row_ptr = m_current_row_ptr = &(m_cache_rows[new_row - m_cache_start]);
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
    return m_current_row;
}

tango::rowid_t ClientIterator::getRowId()
{
    return m_current_row;
}

bool ClientIterator::bof()
{
    return false;
}

bool ClientIterator::eof()
{
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

void ClientIterator::goRow(const tango::rowid_t& rowid)
{
    int offset = (int)(rowid - m_current_row);
    skip(offset);
}

tango::IStructurePtr ClientIterator::getStructure()
{
    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(L"/api/describetable", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    return m_database->jsonToStructure(response);
}

void ClientIterator::refreshStructure()
{
    refreshDataAccessInfo();
}

bool ClientIterator::modifyStructure(tango::IStructure* struct_config, tango::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<HttpDataAccessInfo*>::iterator it2;

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                HttpDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                if (it->m_params->getName().length() > 0)
                {
                    std::wstring new_name = it->m_params->getName();
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->m_params->getType() != -1)
                {
                    (*it2)->type = it->m_params->getType();
                }

                if (it->m_params->getWidth() != -1)
                {
                    (*it2)->width = it->m_params->getWidth();
                }

                if (it->m_params->getScale() != -1)
                {
                    (*it2)->scale = it->m_params->getScale();
                }

                if (it->m_params->getExpression().length() > 0)
                {
                    if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->m_params->getExpression();
                    (*it2)->expr = parse(it->m_params->getExpression());
                }
            }
        }
    }

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            HttpDataAccessInfo* dai = new HttpDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->type = it->m_params->getType();
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            m_fields.push_back(dai);
        }
    }

    // handle insert
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->m_pos;
        if (insert_idx < 0 || insert_idx >= (int)m_fields.size())
            continue;
        
        if (it->m_params->getExpression().length() > 0)
        {
            HttpDataAccessInfo* dai = new HttpDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->type = it->m_params->getType();
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            m_fields.insert(m_fields.begin()+insert_idx, dai);
        }
    }
    
    return true;
}

tango::objhandle_t ClientIterator::getHandle(const std::wstring& expr)
{
    std::vector<HttpDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        HttpDataAccessInfo* dai = new HttpDataAccessInfo;
        dai->expr = NULL;
        dai->type = tango::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<tango::IIterator*>(this),
                                         expr.substr(4),
                                         false))
        {
            delete dai;
            return 0;
        }
        
        m_exprs.push_back(dai);
        return (tango::objhandle_t)dai;
    }
    
    kscript::ExprParser* parser = parse(expr);
    if (!parser)
        return (tango::objhandle_t)0;

    HttpDataAccessInfo* dai = new HttpDataAccessInfo;
    dai->expr = parser;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

tango::IColumnInfoPtr ClientIterator::getInfo(tango::objhandle_t data_handle)
{
    HttpDataAccessInfo* dai = (HttpDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xcm::null;

    ColumnInfo* colinfo = new ColumnInfo;
    colinfo->setName(dai->name);
    colinfo->setType(dai->type);
    colinfo->setWidth(dai->width);
    colinfo->setScale(dai->scale);
    colinfo->setExpression(dai->expr_text);
    colinfo->setCalculated(dai->isCalculated());

    return static_cast<tango::IColumnInfo*>(colinfo);
}

int ClientIterator::getType(tango::objhandle_t data_handle)
{
    HttpDataAccessInfo* dai = (HttpDataAccessInfo*)data_handle;
    if (dai == NULL)
        return tango::typeInvalid;

    return dai->type;
}

bool ClientIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<HttpDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

const unsigned char* ClientIterator::getRawPtr(tango::objhandle_t data_handle)
{
    return 0;
}

int ClientIterator::getRawWidth(tango::objhandle_t data_handle)
{
    return 0;
}

const std::string& ClientIterator::getString(tango::objhandle_t data_handle)
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

const std::wstring& ClientIterator::getWideString(tango::objhandle_t data_handle)
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

static tango::datetime_t parseDateTime2(const std::wstring& wstr)
{
    char buf[32];
    int parts[6] = { 0,0,0,0,0,0 };
    size_t len = wstr.length();
    if (len > 30)
        return 0;
        
    std::string str = kl::tostring(wstr);
    strcpy(buf, str.c_str());
    
    size_t i = 0;
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
        tango::DateTime dt(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5]);
        return dt.getDateTime();
    }
     else if (partcnt >= 3)
    {
        tango::DateTime dt(parts[0], parts[1], parts[2]);
        return dt.getDateTime();
    }

    return 0;
}

tango::datetime_t ClientIterator::getDateTime(tango::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return 0;

    if (handle->expr)
    {
        handle->expr->eval(&handle->expr_result);
        kscript::ExprDateTime edt = handle->expr_result.getDateTime();

        tango::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        if (handle->type == tango::typeDateTime)
            dt |= edt.time;

        return dt;
    }

    if (!m_current_row_ptr)
        return 0;

    return parseDateTime2(m_current_row_ptr->values[handle->ordinal]);
}

double ClientIterator::getDouble(tango::objhandle_t data_handle)
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

int ClientIterator::getInteger(tango::objhandle_t data_handle)
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

bool ClientIterator::getBoolean(tango::objhandle_t data_handle)
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

bool ClientIterator::isNull(tango::objhandle_t data_handle)
{
    HttpDataAccessInfo* handle = (HttpDataAccessInfo*)data_handle;
    if (handle == NULL)
        return false;

    return false;
}

bool ClientIterator::updateCacheRow(tango::rowid_t rowid,
                                  tango::ColumnUpdateInfo* info,
                                  size_t info_size)
{
    return false;
}

bool ClientIterator::refreshDataAccessInfo()
{
    // clear out any existing data access info
    clearDataAccessInfo();

    tango::IStructurePtr structure = getStructure();
    if (structure.isNull())
        return false;

    int idx;
    int count = structure->getColumnCount();

    for (idx = 0; idx < count; ++idx)
    {
        tango::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);

        HttpDataAccessInfo* dai = new HttpDataAccessInfo;
        dai->name = info->getName();
        dai->type = info->getType();
        dai->width = info->getWidth();
        dai->scale = info->getScale();
        dai->ordinal = m_fields.size();
        dai->calculated = info->getCalculated();
        dai->expr_text = info->getExpression();

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
