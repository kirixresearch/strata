/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */


#include <kl/md5.h>
#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "request.h"
#include "jsonutil.h"
#include "kl/url.h"


static std::wstring buildOrderParams(const std::wstring& expr)
{

    return kl::url_encodeURIComponent(expr);
/*
    // TODO: do some type of expression translation to
    // a suitable URL syntax

    std::wstring result;

    std::wstring::const_iterator it, it_end;
    it_end = expr.end();
    
    for (it = expr.begin(); it != it_end; ++it)
    {
        result.push_back(*it);
    }

    return result;
*/
}


ClientSet::ClientSet()
{
}

ClientSet::~ClientSet()
{
}

bool ClientSet::init()
{
    return true;
}

void ClientSet::setObjectPath(const std::wstring& path)
{
    m_ofs_path = path;
}

std::wstring ClientSet::getObjectPath()
{
    if (!m_ofs_path.empty())
        return m_ofs_path;

    return m_tablename;
}

bool ClientSet::isTemporary()
{
    return false;
}

bool ClientSet::storeObject(const std::wstring& ofs_path)
{
    return false;
}

unsigned int ClientSet::getSetFlags()
{
    // get the set flags and row count if they exist
    IClientDatabasePtr httpdb = m_database;
    std::wstring path = httpdb->getRequestPath();
    path.append(L"/");
    
    // TODO: use the ofs path instead of the tablename,
    // but strip off the part of the path that represents
    // the mount portion in the ofs
    
    path.append(m_tablename);
    path.append(L"?method=describe");
    
    g_httprequest.setLocation(path);
    g_httprequest.send();
    std::wstring response = g_httprequest.getResponseString();

    kscript::JsonNode node;
    node.fromString(response);

    bool fast_row_count = node["fast_row_count"].getBoolean();
    if (fast_row_count)
        return tango::sfFastRowCount;

    return 0;
}

std::wstring ClientSet::getSetId()
{
    return kl::md5str(m_tablename);
}

tango::IStructurePtr ClientSet::getStructure()
{
    IClientDatabasePtr httpdb = m_database;
    std::wstring path = httpdb->getRequestPath();
    path.append(L"/");
    
    // TODO: use the ofs path instead of the tablename,
    // but strip off the part of the path that represents
    // the mount portion in the ofs
    
    path.append(m_tablename);
    path.append(L"?method=describe");
    
    g_httprequest.setLocation(path);
    g_httprequest.send();
    std::wstring response = g_httprequest.getResponseString();

    kscript::JsonNode node;
    node.fromString(response);
    
    return tango::JsonStructure::fromJsonNode(node);
}

bool ClientSet::modifyStructure(tango::IStructure* struct_config, tango::IJob* job)
{
    bool done_flag = false;
    CommonBaseSet::modifyStructure(struct_config, &done_flag);
    return true;
}

tango::IRowInserterPtr ClientSet::getRowInserter()
{
    return xcm::null;
}

tango::IRowDeleterPtr ClientSet::getRowDeleter()
{
    return xcm::null;
}

int ClientSet::insert(tango::IIteratorPtr source_iter,
                      const std::wstring& where_condition,
                      int max_rows,
                      tango::IJob* job)
{
    return 0;
}

tango::IIndexInfoEnumPtr ClientSet::getIndexEnum()
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* indexes;
    indexes = new xcm::IVectorImpl<tango::IIndexInfoPtr>;
    return indexes;
}

tango::IIndexInfoPtr ClientSet::createIndex(const std::wstring& tag,
                                            const std::wstring& expr,
                                            tango::IJob* job)
{
    return xcm::null;
}
                                 
tango::IIndexInfoPtr ClientSet::lookupIndex(const std::wstring& expr,
                                            bool exact_column_order)
{
    return xcm::null;
}
                                 
bool ClientSet::deleteIndex(const std::wstring& name)
{
    return false;
}

bool ClientSet::renameIndex(const std::wstring& name,
                            const std::wstring& new_name)
{
    return false;
}

tango::IRelationEnumPtr ClientSet::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IRelationPtr ClientSet::createRelation(const std::wstring& tag,
                                              const std::wstring& right_set_path,
                                              const std::wstring& left_expr,
                                              const std::wstring& right_expr)
{
    return xcm::null;
}

tango::IRelationPtr ClientSet::getRelation(const std::wstring& tag)
{
    return xcm::null;
}

int ClientSet::getRelationCount()
{
    return 0;
}

bool ClientSet::deleteRelation(const std::wstring& tag)
{
    return false;
}

bool ClientSet::deleteAllRelations()
{
    return false;
}

tango::IIteratorPtr ClientSet::createIterator(const std::wstring& columns,
                                              const std::wstring& expr,
                                              tango::IJob* job)
{
    // initialize the iterator
    ClientIterator* iter = new ClientIterator();
    iter->m_database = m_database;
    iter->m_set = this;

    // get the data
    IClientDatabasePtr httpdb = m_database;
    std::wstring url_query = httpdb->getRequestPath();
    url_query.append(L"/");
    
    // TODO: use the ofs path instead of the tablename,
    // but strip off the part of the path that represents
    // the mount portion in the ofs
    
    url_query.append(m_tablename);
    url_query.append(L"?method=select");

    // add the order parameters
    std::wstring order_params = buildOrderParams(expr);
    if (order_params.length() > 0)
    {
        url_query.append(L"&order=");
        url_query.append(order_params);
    }

    if (!iter->init(url_query))
    {
        delete iter;
        return xcm::null;
    }
    
    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t ClientSet::getRowCount()
{
    // get the set flags and row count if they exist
    IClientDatabasePtr httpdb = m_database;
    std::wstring path = httpdb->getRequestPath();
    path.append(L"/");

    // TODO: use the ofs path instead of the tablename,
    // but strip off the part of the path that represents
    // the mount portion in the ofs

    path.append(m_tablename);
    path.append(L"?method=describe");

    g_httprequest.setLocation(path);
    g_httprequest.send();
    std::wstring response = g_httprequest.getResponseString();

    kscript::JsonNode node;
    node.fromString(response);

    int row_count = node["row_count"].getInteger();
    return row_count;
}

HttpRowInserter::HttpRowInserter(ClientSet* set)
{
}

HttpRowInserter::~HttpRowInserter()
{
}

tango::objhandle_t HttpRowInserter::getHandle(const std::wstring& column_name)
{
    return 0;
}

tango::IColumnInfoPtr HttpRowInserter::getInfo(tango::objhandle_t column_handle)
{
    return xcm::null;
}

bool HttpRowInserter::putRawPtr(tango::objhandle_t column_handle, const unsigned char* value, int length)
{
    return false;
}

bool HttpRowInserter::putString(tango::objhandle_t column_handle, const std::string& value)
{
    return false;
}

bool HttpRowInserter::putWideString(tango::objhandle_t column_handle, const std::wstring& value)
{
    return false;
}

bool HttpRowInserter::putDouble(tango::objhandle_t column_handle, double value)
{
    return false;
}

bool HttpRowInserter::putInteger(tango::objhandle_t column_handle, int value)
{
    return false;
}

bool HttpRowInserter::putBoolean(tango::objhandle_t column_handle, bool value)
{
    return false;
}

bool HttpRowInserter::putDateTime(tango::objhandle_t column_handle, tango::datetime_t datetime)
{
    return false;
}

bool HttpRowInserter::putNull(tango::objhandle_t column_handle)
{
    return false;
}

bool HttpRowInserter::startInsert(const std::wstring& col_list)
{
    return false;
}

bool HttpRowInserter::insertRow()
{
    return false;
}

void HttpRowInserter::finishInsert()
{
}

bool HttpRowInserter::flush()
{
    return false;
}

