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


ClientSet::ClientSet(ClientDatabase* database)
{
    m_database = database;
    m_database->ref();
    m_set_flags = 0;
    m_known_row_count = (tango::rowpos_t)-1;
}

ClientSet::~ClientSet()
{
    m_database->unref();
}

bool ClientSet::init(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = m_database->serverCall(L"/api/fileinfo", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    JsonNode file_info = response["file_info"];
    if (file_info.isNull() || file_info["type"].getString() != L"table")
        return false;

    JsonNode fast_row_count = file_info["row_count"];
    if (fast_row_count.isOk())
    {
        if (fast_row_count.getBoolean())
            m_set_flags |= tango::sfFastRowCount;
    }

    m_path = path;
    m_tablename = getTablenameFromOfsPath(path);

    return true;
}

void ClientSet::setObjectPath(const std::wstring& path)
{
    m_path = path;
}

std::wstring ClientSet::getObjectPath()
{
    if (!m_path.empty())
        return m_path;

    return m_tablename;
}

bool ClientSet::isTemporary()
{
    return false;
}

bool ClientSet::storeObject(const std::wstring& path)
{
    return false;
}

unsigned int ClientSet::getSetFlags()
{
    return m_set_flags;
}

std::wstring ClientSet::getSetId()
{
    return kl::md5str(m_tablename);
}

tango::IStructurePtr ClientSet::getStructure()
{
    if (m_structure.isOk())
        return m_structure;

    ServerCallParams params;
    params.setParam(L"path", m_path);
    std::wstring sres = m_database->serverCall(L"/api/describetable", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    m_structure = m_database->jsonToStructure(response);

    return m_structure->clone();
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
    ServerCallParams params;
    params.setParam(L"mode", L"createiterator");
    params.setParam(L"path", m_path);
    params.setParam(L"columns", columns);
    params.setParam(L"expr", expr);

    std::wstring sres = m_database->serverCall(L"/api/query", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
    {
        return xcm::null;
    }





    // initialize the iterator
    ClientIterator* iter = new ClientIterator(m_database, this);
    if (!iter->init(response["handle"], L""))
    {
        delete iter;
        return xcm::null;
    }
    
    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t ClientSet::getRowCount()
{
    if (m_known_row_count != (tango::rowpos_t)-1)
    {
        return m_known_row_count;
    }


    ServerCallParams params;
    params.setParam(L"path", m_path);
    std::wstring sres = m_database->serverCall(L"/api/fileinfo", &params);
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return 0;
    
    JsonNode file_info = response["file_info"];
    if (file_info.isNull())
        return 0;

    JsonNode row_count = file_info["row_count"];
    if (row_count.isNull())
        return 0;

    return row_count.getDouble();
}




ClientRowInserter::ClientRowInserter(ClientSet* set)
{
}

ClientRowInserter::~ClientRowInserter()
{
}

tango::objhandle_t ClientRowInserter::getHandle(const std::wstring& column_name)
{
    return 0;
}

tango::IColumnInfoPtr ClientRowInserter::getInfo(tango::objhandle_t column_handle)
{
    return xcm::null;
}

bool ClientRowInserter::putRawPtr(tango::objhandle_t column_handle, const unsigned char* value, int length)
{
    return false;
}

bool ClientRowInserter::putString(tango::objhandle_t column_handle, const std::string& value)
{
    return false;
}

bool ClientRowInserter::putWideString(tango::objhandle_t column_handle, const std::wstring& value)
{
    return false;
}

bool ClientRowInserter::putDouble(tango::objhandle_t column_handle, double value)
{
    return false;
}

bool ClientRowInserter::putInteger(tango::objhandle_t column_handle, int value)
{
    return false;
}

bool ClientRowInserter::putBoolean(tango::objhandle_t column_handle, bool value)
{
    return false;
}

bool ClientRowInserter::putDateTime(tango::objhandle_t column_handle, tango::datetime_t datetime)
{
    return false;
}

bool ClientRowInserter::putNull(tango::objhandle_t column_handle)
{
    return false;
}

bool ClientRowInserter::startInsert(const std::wstring& col_list)
{
    return false;
}

bool ClientRowInserter::insertRow()
{
    return false;
}

void ClientRowInserter::finishInsert()
{
}

bool ClientRowInserter::flush()
{
    return false;
}

