/*!
 *
 * Copyright (c) 2010-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2010-11-19
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "util.h"
#include "exindex.h"
#include "keylayout.h"
#include "idxutil.h"
#include "cmnfilterset.h"
#include "jobinfo.h"
#include <kl/klib.h>
#include <ctime>


// -- CommonFilterSet Implementation --

CommonFilterSet::CommonFilterSet()
{
    m_index = NULL;
    m_row_count = 0;
    m_temporary = true;
    m_temp_path = xf_get_temp_path();
}

CommonFilterSet::~CommonFilterSet()
{
    if (m_index)
    {
        m_index->unref();
    }

    if (m_temporary)
    {
        xf_remove(m_filename);
    }


    // -- release all indexes --
    std::vector<CommonFilterSetIndexEntry>::iterator it;
    for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
    {
        if (it->index)
        {
            it->index->unref();
        }

        if (it->key_expr)
        {
            delete it->key_expr;
        }

        // -- try to delete the index file itself:
        //    right now all indexes on CommonFilterSets are
        //    considered 'temporary' --

        std::wstring full_index_filename;
        full_index_filename = makePathName(m_temp_path,
                                           L"",
                                           it->filename);

        xf_remove(full_index_filename);
    }
    

}


// -- IStorable --

void CommonFilterSet::onOfsPathChanged(const std::wstring& new_path)
{
    m_ofspath = new_path;
}

void CommonFilterSet::setObjectPath(const std::wstring& new_path)
{
    if (m_ofspath.empty())
        return;
    
    m_ofspath = new_path;
}

std::wstring CommonFilterSet::getObjectPath()
{
    if (m_ofspath.empty())
    {
        // return an object pointer -- see openSet or lookupSetId in xdnative and xdfs
        // to see how this curiosity plays out
        wchar_t buf[255];
        swprintf(buf, 255, L"/.temp/.ptr/%p", static_cast<tango::ISet*>(this));
        return buf;
    }
    
    return m_ofspath;
}

bool CommonFilterSet::isTemporary()
{
    if (m_ofspath.empty())
        return true;
        
    std::wstring ofs_path = m_ofspath;
    kl::makeLower(ofs_path);
    return (wcsstr(ofs_path.c_str(), L".temp") != NULL ? true : false);
}

bool CommonFilterSet::storeObject(const std::wstring& _ofs_path)
{
    if (_ofs_path.empty() || iswspace(*(_ofs_path.c_str())))
        return false;
  
    // -- prepend a slash if it is missing --
    std::wstring ofs_path;
    if (*(_ofs_path.c_str()) != L'/')
        ofs_path = L"/";
    ofs_path += _ofs_path;

    if (!m_database->moveFile(m_ofspath, ofs_path))
        return false;

    m_temporary = false;

    return true;
}


std::wstring CommonFilterSet::getSetId()
{
    XCM_AUTO_LOCK(m_object_mutex);

    if (m_set_id.length() == 0)
    {
        m_set_id = getUniqueString();
    }

    return m_set_id;
}

bool CommonFilterSet::create(tango::IDatabasePtr database,
                              tango::ISetPtr base_set)
{
    //std::wstring filename = dbi->getUniqueFilename();
    //filename += L".dyn";
    
    tango::IAttributesPtr attr = database->getAttributes();
    std::wstring temp_directory = attr->getStringAttribute(tango::dbattrTempDirectory);
    if (!temp_directory.empty())
        m_temp_path = temp_directory;
    
    
    std::wstring filename = makePathName(m_temp_path,
                                         L"",
                                         getUniqueString(),
                                         L".dyn");
    

    // -- filename specified, so create a disk index --
    ExIndex* di = new ExIndex;
    di->setTempFilePath(m_temp_path);
    if (!di->create(filename, sizeof(tango::rowid_t), 0, false))
    {
        delete di;
        return false;
    }

    m_index = static_cast<IIndex*>(di);
    m_filename = filename;
    m_base_set = base_set;
    m_base_set_update = base_set;
    m_row_count = 0;


    m_base_set_iter = base_set->createIterator(L"", L"", NULL);

    m_database = database;
    
    return true;
}


unsigned int CommonFilterSet::getSetFlags()
{
    return tango::sfFastRowCount;
}


tango::ISetPtr CommonFilterSet::getBaseSet()
{
    return m_base_set;
}


bool CommonFilterSet::insertRow(const tango::rowid_t& rowid)
{
    tango::rowid_t fixed_rowid;
    invert_rowid_endianness((unsigned char*)&fixed_rowid,
                            (unsigned char*)&rowid);
    m_index->insert((void*)&fixed_rowid, sizeof(tango::rowid_t), NULL, 0);
    return true;
}

bool CommonFilterSet::deleteRow(const tango::rowid_t& rowid)
{
    tango::rowid_t fixed_rowid;
    invert_rowid_endianness((unsigned char*)&fixed_rowid,
                            (unsigned char*)&rowid);

    IIndexIterator* idx_iter = m_index->seek(&fixed_rowid,
                                             sizeof(tango::rowid_t),
                                             false);

    if (!idx_iter)
    {
        return false;
    }


    m_index->remove(idx_iter);
    idx_iter->unref();
    m_row_count--;
    //fire_onSetRowDeleted(rowid);


    m_base_set_iter->goRow(rowid);

    std::vector<CommonFilterSetIndexEntry>::iterator it;
    IIndexIterator* iter;

    for (it = m_indexes.begin();
         it != m_indexes.end(); ++it)
    {
        iter = seekRow(it->index,
                       it->key_expr->getKey(),
                       it->key_length,
                       rowid);

        if (iter)
        {
            it->index->remove(iter);
            iter->unref();
        }
    }


    return true;
}

tango::IRowInserterPtr CommonFilterSet::getRowInserter()
{
    return xcm::null;
}

tango::IRowDeleterPtr CommonFilterSet::getRowDeleter()
{
    CommonFilterSetRowDeleter* deleter = new CommonFilterSetRowDeleter(this);
    return static_cast<tango::IRowDeleter*>(deleter);
}


int CommonFilterSet::insert(tango::IIteratorPtr source_iter,
                             const std::wstring& condition,
                             int max_rows,
                             tango::IJob* job)
{
    IJobInternalPtr sp_ijob = job;
    IJobInternal* ijob = sp_ijob.p;

    tango::rowpos_t max_count = 0;


    tango::ISetPtr source_set = source_iter->getSet();
    if (source_set)
    {
        if (source_set->getSetFlags() & tango::sfFastRowCount)
        {
            max_count = source_set->getRowCount();
        }
    }
    source_set = xcm::null;


    if (job)
    {
        ijob->setMaxCount(max_count);
        ijob->setCurrentCount(0);
        ijob->setStatus(tango::jobRunning);
        ijob->setStartTime(time(NULL));
    }

    m_index->startBulkInsert(max_count);

    // -- if the constraint set is a filter set, we can perform the
    //    operation faster by just parsing the filter expression locally --
    tango::objhandle_t filter_handle = 0;
    if (condition.length() > 0)
    {
        filter_handle = source_iter->getHandle(condition);
        if (!filter_handle)
            return -1;
    }

    int counter = 0;
    int inserted = 0;
    tango::rowid_t rowid;
    tango::rowid_t fixed_rowid;
    bool cancelled = false;
    bool result = true;

    while (!source_iter->eof())
    {
        if (filter_handle)
        {
            result = source_iter->getBoolean(filter_handle);
        }


        if (result)
        {
            rowid = source_iter->getRowId();

            invert_rowid_endianness((unsigned char*)&fixed_rowid,
                                    (unsigned char*)&rowid);

            m_index->insert((void*)&fixed_rowid,
                            sizeof(tango::rowid_t),
                            NULL,
                            0);

            inserted++;
            m_row_count++;
        }

        source_iter->skip(1);

        counter++;
        if (counter % 100 == 0)
        {
            if (ijob)
            {
                ijob->setCurrentCount(counter);
                if (job->getCancelled())
                {
                    cancelled = true;
                    m_index->cancelBulkInsert();
                    break;
                }
            }
        }

    }

    if (!cancelled)
    {
        m_index->finishBulkInsert(NULL);
        m_index->reopen();
    }

    if (filter_handle)
    {
        source_iter->releaseHandle(filter_handle);
    }

    if (job)
    {
        if (!job->getCancelled())
        {
            ijob->setCurrentCount(counter);
            ijob->setStatus(tango::jobFinished);
            ijob->setFinishTime(time(NULL));
        }
    }

    return inserted;
}


tango::IIteratorPtr CommonFilterSet::createIterator(const std::wstring& columns,
                                                     const std::wstring& expr,
                                                     tango::IJob* job)
{
    if (expr.empty())
    {
        // create an index iterator
        IIndexIterator* idx_iter = m_index->createIterator();
        if (!idx_iter)
            return xcm::null;
        idx_iter->goFirst();

        // create a physical-order iterator 
        tango::IIteratorPtr data_iter = m_base_set->createIterator(columns, L"", NULL);
        if (data_iter.isNull())
            return xcm::null;
        data_iter->goFirst();

        CommonIndexIterator* iter = new CommonIndexIterator(static_cast<tango::ISet*>(this),
                                                            data_iter,
                                                            idx_iter,
                                                            L"",
                                                            false);
        idx_iter->unref();
        iter->goFirst();
        return static_cast<tango::IIterator*>(iter);
    }




    std::wstring index_filename = getUniqueString();
    index_filename += L".idx";

    std::wstring full_index_filename;
    full_index_filename = makePathName(m_temp_path,
                                       L"",
                                       index_filename);

    IIndex* idx = createExternalIndex(static_cast<tango::ISet*>(this),
                                      full_index_filename,
                                      m_temp_path,
                                      expr,
                                      true,
                                      job);
    if (!idx)
    {
        return xcm::null;
    }

    CommonFilterSetIndexEntry e;
    e.expr = expr;
    e.filename = index_filename;
    e.index = idx;      // (already ref'ed)
    e.key_expr = new KeyLayout;
    if (!e.key_expr->setKeyExpr(m_base_set_iter, expr, false))
    {
        delete e.key_expr;
        return xcm::null;
    }
    e.key_length = e.key_expr->getKeyLength();
    m_indexes.push_back(e);

    return createIteratorFromIndex(static_cast<tango::ISet*>(this),
                                   idx,
                                   columns,
                                   expr);
}


tango::rowpos_t CommonFilterSet::getRowCount()
{
    return m_row_count;
}





tango::IStructurePtr CommonFilterSet::getStructure()
{
    return m_base_set->getStructure();
}


bool CommonFilterSet::modifyStructure(tango::IStructure* struct_config,
                                 tango::IJob* job)
{
    bool result = m_base_set->modifyStructure(struct_config, job);
    //fire_onSetStructureUpdated();
    return result;
}


tango::IRelationPtr CommonFilterSet::createRelation(
                                    const std::wstring& tag,
                                    const std::wstring& right_set_path,
                                    const std::wstring& left_expr,
                                    const std::wstring& right_expr)
{
    return m_base_set->createRelation(tag,
                                      right_set_path,
                                      left_expr,
                                      right_expr);
}

tango::IRelationPtr CommonFilterSet::getRelation(const std::wstring& rel_tag)
{
    return m_base_set->getRelation(rel_tag);
}

int CommonFilterSet::getRelationCount()
{
    return m_base_set->getRelationCount();
}

tango::IRelationEnumPtr CommonFilterSet::getRelationEnum()
{
    return m_base_set->getRelationEnum();
}

bool CommonFilterSet::deleteRelation(const std::wstring& tag)
{
    return m_base_set->deleteRelation(tag);
}

bool CommonFilterSet::deleteAllRelations()
{
    return m_base_set->deleteAllRelations();
}


// -- Indexing routines --

tango::IIndexInfoEnumPtr CommonFilterSet::getIndexEnum()
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* indexes;
    indexes = new xcm::IVectorImpl<tango::IIndexInfoPtr>;
    return indexes;
}

tango::IIndexInfoPtr CommonFilterSet::createIndex(const std::wstring& name,
                                          const std::wstring& expr,
                                          tango::IJob* job)
{
    return xcm::null;
}

tango::IIndexInfoPtr CommonFilterSet::lookupIndex(const std::wstring& expr,
                                          bool exact_column_order)
{
    return xcm::null;
}

bool CommonFilterSet::deleteIndex(const std::wstring& name)
{
    return false;
}

bool CommonFilterSet::renameIndex(const std::wstring& name,
                                   const std::wstring& new_name)
{
    return false;
}





void CommonFilterSet::onSetDomainUpdated()
{
}

void CommonFilterSet::onSetStructureUpdated()
{
/*
    // -- forward event from base set --
    fire_onSetStructureUpdated();

    // -- because the structure of the base set has changed,
    //    we need to renew our bookmark, which is used for
    //    index updating among other things --

    BookmarkSet* bs = new BookmarkSet(m_database);
    bs->ref();
    bs->setBaseSet(m_base_set);
    m_base_set_iter = bs->createIterator(L"", L"", NULL);
    bs->unref();
*/
}

void CommonFilterSet::onSetRelationshipsUpdated()
{
}

void CommonFilterSet::onSetRowUpdated(tango::rowid_t rowid)
{
/*
    // -- forward event from base set --
    fire_onSetRowUpdated(rowid);
*/
}

void CommonFilterSet::onSetRowDeleted(tango::rowid_t rowid)
{
}


bool CommonFilterSet::updateRow(tango::rowid_t rowid,
                           tango::ColumnUpdateInfo* info,
                           size_t info_size)
{
    if (m_base_set_update.isNull())
    {
        return false;
    }

    return m_base_set_update->updateRow(rowid, info, info_size);
}


CommonFilterSetRowDeleter::CommonFilterSetRowDeleter(CommonFilterSet* set)
{
    m_set = set;
    m_set->ref();

    m_index = set->m_index;
    m_index->ref();
    
    m_rows_to_delete.reserve(1000);
    
}

CommonFilterSetRowDeleter::~CommonFilterSetRowDeleter()
{
    m_index->unref();
    m_set->unref();
}

void CommonFilterSetRowDeleter::startDelete()
{
}

bool CommonFilterSetRowDeleter::deleteRow(const tango::rowid_t& rowid)
{
    m_rows_to_delete.push_back(rowid);
    return true;
}

void CommonFilterSetRowDeleter::finishDelete()
{
    std::vector<tango::rowid_t>::iterator it, it_end = m_rows_to_delete.end();
    for (it = m_rows_to_delete.begin(); it != it_end; ++it)
        m_set->deleteRow(*it);
}

void CommonFilterSetRowDeleter::cancelDelete()
{
    m_rows_to_delete.clear();
}

