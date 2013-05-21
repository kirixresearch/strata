/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-11
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
#include "cmndynamicset.h"
#include "jobinfo.h"
#include <kl/klib.h>
#include <ctime>





class CommonDynamicIterator : public CommonIndexIterator
{

public:

    CommonDynamicIterator(CommonDynamicSet* set) : CommonIndexIterator()
    {
        m_set = set;
        m_set->ref();
    }

    ~CommonDynamicIterator()
    {
        m_set->unref();
    }

    unsigned int getIteratorFlags()
    {
        return tango::ifFastRowCount;
    }

    tango::rowpos_t getRowCount()
    {
        return m_set->m_row_count;
    }

private:

    CommonDynamicSet* m_set;
};


CommonDynamicSet::CommonDynamicSet()
{
    m_index = NULL;
    m_row_count = 0;
    m_temporary = true;
    m_temp_path = xf_get_temp_path();
}

CommonDynamicSet::~CommonDynamicSet()
{
    if (m_index)
    {
        m_index->unref();
    }

    if (m_temporary)
    {
        xf_remove(m_filename);
    }


    // release all indexes
    std::vector<CommonDynamicSetIndexEntry>::iterator it;
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

        // try to delete the index file itself:
        // right now all indexes on CommonDynamicSets are
        // considered 'temporary'

        std::wstring full_index_filename;
        full_index_filename = makePathName(m_temp_path,
                                           L"",
                                           it->filename);

        xf_remove(full_index_filename);
    }
    

}

void CommonDynamicSet::setObjectPath(const std::wstring& new_path)
{
    if (m_ofspath.empty())
        return;
    
    m_ofspath = new_path;
}

std::wstring CommonDynamicSet::getObjectPath()
{
    if (m_ofspath.empty())
    {
        // return an object pointer -- see openSet or lookupSetId in xdnative and xdfs
        // to see how this curiosity plays out
        wchar_t buf[255];
        swprintf(buf, 255, L"/.temp/.ptr/%p", static_cast<IXdsqlTable*>(this));
        return buf;
    }
    
    return m_ofspath;
}


std::wstring CommonDynamicSet::getSetId()
{
    XCM_AUTO_LOCK(m_object_mutex);

    if (m_set_id.length() == 0)
    {
        m_set_id = getUniqueString();
    }

    return m_set_id;
}

tango::IStructurePtr CommonDynamicSet::getStructure()
{
    return m_base_table->getStructure();
}


bool CommonDynamicSet::create(tango::IDatabasePtr database,
                              const std::wstring& base_path)
{
    IXdsqlDatabasePtr xdb = database;
    if (xdb.isNull())
        return false;

    IXdsqlTablePtr base_table = xdb->openTable(base_path);
    if (base_table.isNull())
        return false;

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
    

    // filename specified, so create a disk index
    ExIndex* di = new ExIndex;
    di->setTempFilePath(m_temp_path);
    if (!di->create(filename, sizeof(tango::rowid_t), 0, false))
    {
        delete di;
        return false;
    }

    m_index = static_cast<IIndex*>(di);
    m_filename = filename;
    m_base_path = base_path;
    m_base_table = base_table;
    m_row_count = 0;

    m_base_iter = database->query(base_path, L"", L"", L"", NULL);
    m_database = database;
    
    return true;
}

void CommonDynamicSet::startBulkInsert()
{
    m_index->startBulkInsert(100000);
}

void CommonDynamicSet::finishBulkInsert()
{
    m_index->finishBulkInsert(NULL);
    m_index->reopen();
}

bool CommonDynamicSet::insertRow(const tango::rowid_t& rowid)
{
    tango::rowid_t fixed_rowid;
    invert_rowid_endianness((unsigned char*)&fixed_rowid,
                            (unsigned char*)&rowid);
    m_index->insert((void*)&fixed_rowid, sizeof(tango::rowid_t), NULL, 0);
    
    m_row_count++;
    
    return true;
}

bool CommonDynamicSet::deleteRow(const tango::rowid_t& rowid)
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


    m_base_iter->goRow(rowid);

    std::vector<CommonDynamicSetIndexEntry>::iterator it;
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


IXdsqlRowDeleterPtr CommonDynamicSet::getRowDeleter()
{
    CommonDynamicSetRowDeleter* deleter = new CommonDynamicSetRowDeleter(this);
    return static_cast<IXdsqlRowDeleter*>(deleter);
}


int CommonDynamicSet::insert(tango::IIteratorPtr source_iter,
                             const std::wstring& condition,
                             int max_rows,
                             tango::IJob* job)
{
    IJobInternalPtr sp_ijob = job;
    IJobInternal* ijob = sp_ijob.p;

    if (job)
    {
        ijob->setMaxCount(max_rows);
        ijob->setCurrentCount(0);
        ijob->setStatus(tango::jobRunning);
        ijob->setStartTime(time(NULL));
    }

    m_index->startBulkInsert(max_rows);

    // if the constraint set is a filter set, we can perform the
    // operation faster by just parsing the filter expression locally
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


tango::IIteratorPtr CommonDynamicSet::createIterator(const std::wstring& columns,
                                                     const std::wstring& expr,
                                                     tango::IJob* job)
{
    tango::IIteratorPtr data_iter = m_database->query(m_base_path, columns, L"", L"", NULL);
    if (data_iter.isNull())
        return xcm::null;
    data_iter->goFirst();

    if (expr.empty())
    {
        // create an index iterator
        IIndexIterator* idx_iter = m_index->createIterator();
        if (!idx_iter)
            return xcm::null;
        idx_iter->goFirst();

        CommonDynamicIterator* iter = new CommonDynamicIterator(this);
        iter->init(data_iter, idx_iter, L"", false);
        iter->setTable(getObjectPath());
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

    IIndex* idx = createExternalIndex(m_database,
                                      getObjectPath(),
                                      full_index_filename,
                                      m_temp_path,
                                      expr,
                                      true,
                                      job);
    if (!idx)
    {
        return xcm::null;
    }

    CommonDynamicSetIndexEntry e;
    e.expr = expr;
    e.filename = index_filename;
    e.index = idx;      // (already ref'ed)
    e.key_expr = new KeyLayout;
    if (!e.key_expr->setKeyExpr(m_base_iter, expr, false))
    {
        delete e.key_expr;
        return xcm::null;
    }
    e.key_length = e.key_expr->getKeyLength();
    m_indexes.push_back(e);



    IIndexIterator* idx_iter = idx->createIterator();
    if (!idx_iter)
        return xcm::null;
    idx_iter->goFirst();

    CommonDynamicIterator* iter = new CommonDynamicIterator(this);
    iter->init(data_iter, idx_iter, L"", true);
    iter->setTable(getObjectPath());

    idx_iter->unref();
    iter->goFirst();

    return static_cast<tango::IIterator*>(iter);
}


tango::rowpos_t CommonDynamicSet::getRowCount()
{
    return m_row_count;
}



/*
bool CommonDynamicSet::modifyStructure(tango::IStructure* struct_config,
                                       tango::IJob* job)
{

    // release all indexes
    std::vector<CommonDynamicSetIndexEntry>::iterator it;
    for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
    {
        if (it->index)
            it->index->unref();

        if (it->key_expr)
            delete it->key_expr;
    }
    m_indexes.clear();

    bool result = m_base_set->modifyStructure(struct_config, job);
    //fire_onSetStructureUpdated();
    
    m_base_iter.clear();
    m_base_iter = m_base_set->createIterator(L"", L"", NULL);

    return result;
}
*/






bool CommonDynamicSet::updateRow(tango::rowid_t rowid,
                           tango::ColumnUpdateInfo* info,
                           size_t info_size)
{
    return m_base_table->updateRow(rowid, info, info_size);
}


CommonDynamicSetRowDeleter::CommonDynamicSetRowDeleter(CommonDynamicSet* set)
{
    m_set = set;
    m_set->ref();

    m_index = set->m_index;
    m_index->ref();
    
    m_rows_to_delete.reserve(1000);
    
}

CommonDynamicSetRowDeleter::~CommonDynamicSetRowDeleter()
{
    m_index->unref();
    m_set->unref();
}

void CommonDynamicSetRowDeleter::startDelete()
{
}

bool CommonDynamicSetRowDeleter::deleteRow(const tango::rowid_t& rowid)
{
    m_rows_to_delete.push_back(rowid);
    return true;
}

void CommonDynamicSetRowDeleter::finishDelete()
{
    std::vector<tango::rowid_t>::iterator it, it_end = m_rows_to_delete.end();
    for (it = m_rows_to_delete.begin(); it != it_end; ++it)
        m_set->deleteRow(*it);
}

void CommonDynamicSetRowDeleter::cancelDelete()
{
    m_rows_to_delete.clear();
}

