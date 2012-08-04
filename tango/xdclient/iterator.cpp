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
    m_cache_row_count = 0;
    m_cache_start = 0;
    m_cache_limit = 0;
    m_cache_populated = false;

    m_database = database;
    m_database->ref();

    m_set = set;
    if (m_set)
        m_set->ref();
}

ClientIterator::~ClientIterator()
{
    // clear the cache
    clearRowCache();

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

    if (!rowInCache(new_row))
        populateRowCache(new_row, cache_size);

    // TODO: include some type of bounds check on the
    // new row to make sure it's within the row set

    m_current_row = new_row;
}

void ClientIterator::goFirst()
{
    int new_row = 1;

    if (!rowInCache(new_row))
        populateRowCache(new_row, cache_size);

    m_current_row = new_row;
}

void ClientIterator::goLast()
{
}

double ClientIterator::getPos()
{
    return 0;
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
    // if we haven't yet populated the cache, we don't
    // know what the count is, so return false
    if (!rowCachePopulated())
        return false;

    if (m_current_row > m_cache_row_count)
        return true;

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
    {
        return xcm::null;
    }



    Structure* s = new Structure;

    JsonNode columns = response["columns"];
    size_t i = 0, cnt = columns.getCount();

    for (i = 0; i < cnt; ++i)
    {
        JsonNode column = columns[i];
        std::wstring type = column["type"];
        int ntype;

             if (type == L"undefined")     ntype = tango::typeUndefined;
        else if (type == L"invalid")       ntype = tango::typeInvalid;
        else if (type == L"character")     ntype = tango::typeCharacter;
        else if (type == L"widecharacter") ntype = tango::typeWideCharacter;
        else if (type == L"numeric")       ntype = tango::typeNumeric;
        else if (type == L"double")        ntype = tango::typeDouble;
        else if (type == L"integer")       ntype = tango::typeInteger;
        else if (type == L"date")          ntype = tango::typeDate;
        else if (type == L"datetime")      ntype = tango::typeDateTime;
        else if (type == L"boolean")       ntype = tango::typeBoolean;
        else if (type == L"binary")        ntype = tango::typeBinary;
        else ntype = tango::typeUndefined;

        tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        col->setName(column["name"]);
        col->setType(ntype);
        col->setWidth(column["width"].getInteger());
        col->setScale(column["scale"].getInteger());
        col->setColumnOrdinal(i);
        col->setExpression(column["expression"]);
        col->setCalculated(column["expression"].getString().length() > 0 ? true : false);

        s->addColumn(col);
    }

    return static_cast<tango::IStructure*>(s);
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

    if (m_cache_rows.empty())
        return empty_string;

    int cache_offset = m_current_row - m_cache_start;
    HttpCacheRow* row = m_cache_rows[cache_offset];

    m_result_string = kl::tostring(row->values[handle->ordinal]);
    return m_result_string;
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

    if (m_cache_rows.empty())
        return empty_wstring;

    int cache_offset = m_current_row - m_cache_start;
    HttpCacheRow* row = m_cache_rows[cache_offset];

    m_result_wstring = row->values[handle->ordinal];
    return m_result_wstring;
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

    if (m_cache_rows.empty())
        return 0;

    int cache_offset = m_current_row - m_cache_start;
    HttpCacheRow* row = m_cache_rows[cache_offset];

    int yy, mm, dd, h, m, s;
    if (!parseDateTime(row->values[handle->ordinal], &yy, &mm, &dd, &h, &m, &s))
        return 0;

    // if we don't have an hour, minute, and second, set them to zero
    if (h == -1)
        h = 0;
    if (m == -1)
        m = 0;
    if (s == -1)
        s = 0;

    tango::DateTime dt(yy, mm, dd, h, m, s);
    m_result_date = dt;
    return m_result_date;
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

    if (m_cache_rows.empty())
        return 0.0f;

    int cache_offset = m_current_row - m_cache_start;
    HttpCacheRow* row = m_cache_rows[cache_offset];

    m_result_double = kl::wtof(row->values[handle->ordinal]);
    return m_result_double;
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

    if (m_cache_rows.empty())
        return 0;

    int cache_offset = m_current_row - m_cache_start;
    HttpCacheRow* row = m_cache_rows[cache_offset];

    m_result_integer = kl::wtoi(row->values[handle->ordinal]);
    return m_result_integer;
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

    if (m_cache_rows.empty())
        return false;

    int cache_offset = m_current_row - m_cache_start;
    HttpCacheRow* row = m_cache_rows[cache_offset];

    m_result_boolean = (row->values[handle->ordinal] == L"true") ? true : false;
    return m_result_boolean;
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

    // get the structure from the set
    tango::ISetPtr set = getSet();
    if (set.isNull())
        return false;
        
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

bool ClientIterator::populateRowCache(int start, int limit)
{
    // clear the existing row cache
    clearRowCache();

    std::wstring query = m_url_query;

    char buf[40];
    
    sprintf(buf, "&start=%d", start);
    query.append(kl::towstring(buf));
    
    sprintf(buf, "&limit=%d", limit);
    query.append(kl::towstring(buf));

    g_httprequest.setLocation(query);
    g_httprequest.send();
    std::wstring response = g_httprequest.getResponseString();

    // populate the cache
    kscript::JsonNode node;
    node.fromString(response);

    m_cache_row_count = node["total_count"].getInteger();
    m_cache_start = node["start"].getInteger();
    m_cache_limit = node["limit"].getInteger();
    
    kscript::JsonNode item;
    kscript::JsonNode items = node["items"];

    int idx, count = items.getCount();
    for (idx = 0; idx < count; ++idx)
    {
        item = items[idx];
        std::vector<std::wstring> keys = item.getChildKeys();

        HttpCacheRow* row = new HttpCacheRow;
        row->values.reserve(keys.size());

        std::vector<std::wstring>::iterator it, it_end;
        it_end = keys.end();
        
        for (it = keys.begin(); it != it_end; ++it)
        {
            row->values.push_back(item[*it].getString());
        }

        m_cache_rows.push_back(row);
    }

    // set the cache populated flag
    m_cache_populated = true;
    return true;
}

void ClientIterator::clearRowCache()
{
    std::vector<HttpCacheRow*>::iterator it, it_end;
    it_end = m_cache_rows.end();
    
    for (it = m_cache_rows.begin(); it != it_end; ++it)
    {
        delete *it;
    }

    m_cache_rows.clear();

    m_cache_row_count = 0;
    m_cache_start = 0;
    m_cache_limit = 0;
    m_cache_populated = false;
}

bool ClientIterator::rowInCache(int row)
{
    if (!rowCachePopulated())
        return false;

    if (row < m_cache_start)
        return false;

    if (row > m_cache_start + m_cache_limit - 1)
        return false;

    return true;
}

bool ClientIterator::rowCachePopulated()
{
    return  m_cache_populated;
}