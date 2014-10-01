/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-27
 *
 */


#include <xd/xd.h>
#include "xdcommon.h"
#include "exindex.h"
#include "idxutil.h"
#include "keylayout.h"
#include "util.h"
#include <kl/math.h>
#include <kl/portable.h>
#include <ctime>


class BulkInsertProgress : public IIndexProgress
{
public:
    void updateProgress(xd::rowpos_t cur_count,
                        xd::rowpos_t max_count,
                        bool* cancel)
    {
        if (cur_count == 0)
        {
            if (ijob)
            {
                ijob->startPhase();
            }
        }

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

public:
    xd::IJob* job;
    IJobInternal* ijob;
    std::wstring filename;
    bool cancelled;

};


IIndex* createExternalIndex(xd::IDatabasePtr db,
                            const std::wstring& table_path,
                            const std::wstring& index_filename,
                            const std::wstring& tempfile_path,
                            const std::wstring& expr,
                            bool allow_dups,
                            bool delete_on_close,
                            xd::IJob* job)
{
    if (table_path.empty())
        return false;

    xd::IFileInfoPtr finfo = db->getFileInfo(table_path);
    if (finfo.isNull())
        return false;

    // job information
    IJobInternalPtr ijob;
    xd::rowpos_t cur_count;
    xd::rowpos_t max_count;

    cur_count = 0;
    max_count = 0;

    if (job)
    {
        if (finfo->getFlags() & xd::sfFastRowCount)
        {
            max_count = finfo->getRowCount();
        }

        ijob = job;
        if (!ijob)
        {
            return NULL;
        }

        ijob->setMaxCount(max_count);
        ijob->setCurrentCount(0);
        ijob->setStatus(xd::jobRunning);
        ijob->setStartTime(time(NULL));
        int phase_pcts[] = { 70, 30 };
        ijob->setPhases(2, phase_pcts);
        ijob->startPhase();
    }


    // create an unordered iterator
    xd::IIteratorPtr sp_iter = db->query(table_path, L"", L"", L"", NULL);
    if (!sp_iter)
        return NULL;

    xd::IIterator* iter = sp_iter.p;
    iter->ref();
    sp_iter = xcm::null;


    KeyLayout kl;
    if (!kl.setKeyExpr(iter, expr))
    {
        iter->unref();
        return NULL;
    }

    int key_length = kl.getKeyLength();


    iter->goFirst();


    // create the index
    ExIndex* index = new ExIndex;
    index->setTempFilePath(tempfile_path);
    
    if (delete_on_close)
        index->setDeleteOnClose(true);

    if (!index->create(index_filename,
                       key_length,
                       sizeof(xd::rowid_t),
                       true))
    {
        delete index;
        iter->unref();
        return NULL;
    }


    // add keys to the index
    xd::rowid_t rowid;
    index->startBulkInsert(max_count);
    while (!iter->eof())
    {
        rowid = iter->getRowId();


        if (index->insert(kl.getKey(),
                          key_length,
                          &rowid,
                          sizeof(xd::rowid_t)) != idxErrSuccess)
        {
            ijob->setStatus(xd::jobFailed);
            index->cancelBulkInsert();
            index->unref();

            iter->unref();

            return NULL;
        }

        iter->skip(1);
        cur_count++;

        if (job)
        {
            if (cur_count % 100 == 0)
            {
                ijob->setCurrentCount(cur_count);
                if (job->getCancelled())
                {
                    index->cancelBulkInsert();
                    index->unref();

                    iter->unref();

                    xf_remove(index_filename);

                    return NULL;
                }
            }
        }
    }

    BulkInsertProgress bip;
    bip.job = job;
    bip.ijob = ijob;
    bip.filename = index_filename;
    bip.cancelled = false;

    index->finishBulkInsert(&bip);


    iter->unref();


    if (bip.cancelled)
    {
        index->unref();
        xf_remove(index_filename);
        return NULL;
    }

    // mark job as finished
    if (job)
    {
        ijob->setCurrentCount(cur_count);
        ijob->setFinishTime(time(NULL));
        ijob->setStatus(xd::jobFinished);
    }

    return index;
}




// seekRow() seeks a key in an index, and then
// scrolls down to the specific rowid on the value side

IIndexIterator* seekRow(IIndex* idx,
                        const unsigned char* key,
                        int key_len,
                        xd::rowid_t rowid)
{
    // attempt to seek

    IIndexIterator* iter = idx->seek((unsigned char*)key, key_len, false);
    
    if (!iter)
        return NULL;

    xd::rowid_t idx_rowid;

    bool found = false;
    while (1)
    {
        memcpy(&idx_rowid, iter->getValue(), sizeof(xd::rowid_t));

        if (idx_rowid == rowid)
        {
            found = true;
            break;
        }

        iter->skip(1);
        if (iter->isEof())
            break;
        if (0 != memcmp(key, iter->getKey(), key_len))
            break;
    }

    if (!found)
    {
        iter->unref();
        return NULL;
    }

    return iter;
}


xd::IIteratorPtr createIteratorFromIndex(xd::IIteratorPtr data_iter,
                                            IIndex* idx,
                                            const std::wstring& columns,
                                            const std::wstring& order,
                                            const std::wstring& table)
{   
    IIndexIterator* idx_iter = idx->createIterator();
    if (!idx_iter)
        return xcm::null;
    idx_iter->goFirst();

    CommonIndexIterator* iter;
    iter = new CommonIndexIterator();
    iter->init(data_iter, idx_iter, order, true);
    iter->setTable(table);

    idx_iter->unref();
    iter->goFirst();
    return iter;
}




// CommonIndexIterator Implementation

CommonIndexIterator::CommonIndexIterator()
{
    m_data_iter = NULL;
    m_idx_iter = NULL;
    m_value_side = false;
    m_row_deleted = false;
    m_keylen = 0;
    m_key_filter = NULL;
    m_key_filter_len = 0;

    m_order = L"";
    m_layout = NULL;
}

CommonIndexIterator::~CommonIndexIterator()
{
    delete m_layout;
    
    if (m_data_iter)
        m_data_iter->unref();
    
    if (m_idx_iter)
        m_idx_iter->unref();

    delete[] m_key_filter;
}

bool CommonIndexIterator::init(xd::IIterator* data_iter,
                               IIndexIterator* idx_iter,
                               const std::wstring& order,
                               bool value_side)
{
    // store the data iterator pointer
    m_data_iter = data_iter;
    m_data_iter->ref();
    
    // store the index iterator pointer
    m_idx_iter = idx_iter;
    m_idx_iter->ref();

    m_value_side = value_side;

    m_row_deleted = false;

    // get key length
    IIndex* idx = idx_iter->getIndex();
    m_keylen = idx->getKeyLength();
    idx->unref();

    m_key_filter = new unsigned char[m_keylen];
    m_key_filter_len = 0;
    
    // store the key columns
    m_order = order;
    m_layout = NULL;

    return true;
}



xd::IIteratorPtr CommonIndexIterator::clone()
{
    IIndexIterator* index_iter = m_idx_iter->clone();
    xd::IIteratorPtr data_iter = m_data_iter->clone();
    
    CommonIndexIterator* new_iter = new CommonIndexIterator;
    new_iter->init(data_iter.p, index_iter, m_order, m_value_side);
    index_iter->unref();
    new_iter->setTable(m_table);
    memcpy(new_iter->m_key_filter, m_key_filter, m_keylen);
    new_iter->m_key_filter_len = m_key_filter_len;

    new_iter->updatePos();

    return static_cast<xd::IIterator*>(new_iter);
}

void* CommonIndexIterator::getKey()
{
    return m_idx_iter->getKey();
}

int CommonIndexIterator::getKeyLength()
{
    return m_keylen;
}

bool CommonIndexIterator::setKeyFilter(const void* key, int len)
{
    if (key == NULL || len == 0)
    {
        m_key_filter_len = 0;
        return true;
    }

    if ((unsigned int)len > m_keylen)
        len = m_keylen;

    m_key_filter_len = len;
    memcpy(m_key_filter, key, len);

    return true;
}

void CommonIndexIterator::getKeyFilter(const void** key, int* len)
{
    *key = m_key_filter;
    *len = m_key_filter_len;
}

bool CommonIndexIterator::setFirstKey()
{
    return m_idx_iter->setFirstKey();
}


void CommonIndexIterator::updatePos()
{
    if (m_idx_iter->isEof() || m_idx_iter->isBof())
    {
        m_rowid = 0;
        return;
    }
    
    if (m_value_side)
    {
        memcpy(&m_rowid, m_idx_iter->getValue(), sizeof(xd::rowid_t));
    }
     else
    {
        invert_rowid_endianness((unsigned char*)&m_rowid,
                                (const unsigned char*)m_idx_iter->getKey());
    }

    m_data_iter->goRow(m_rowid);
}


void CommonIndexIterator::skip(int delta)
{
    if (m_row_deleted && delta > 0)
    {
        delta--;
        m_row_deleted = false;
    }
     
    if (delta != 0)
    {       
        m_idx_iter->skip(delta);
    }

    updatePos();
}

void CommonIndexIterator::goFirst()
{
    m_idx_iter->goFirst();
    m_row_deleted = false;
    updatePos();
}

void CommonIndexIterator::goLast()
{
    m_idx_iter->goLast();
    m_row_deleted = false;
    updatePos();
}

xd::rowid_t CommonIndexIterator::getRowId()
{
    return m_rowid;
}

bool CommonIndexIterator::bof()
{
    return m_idx_iter->isBof();
}

bool CommonIndexIterator::eof()
{
    if (m_idx_iter->isEof())
        return true;

    if (!m_key_filter)
        return false;

    return (memcmp(m_idx_iter->getKey(),
                   m_key_filter,
                   m_key_filter_len) == 0) ? false : true;
}


bool CommonIndexIterator::seek(const unsigned char* key,
                               int length,
                               bool soft)
{
    IIndex* idx = m_idx_iter->getIndex();
    if (!idx)
        return false;

    if (!m_value_side)
    {
        // we are seeking for a record number.
        // 'key' contains a 64-bit record id

        xd::rowid_t fixed_rowid;
        invert_rowid_endianness((unsigned char*)&fixed_rowid,
                                (unsigned char*)key);

        IIndexIterator* idx_iter;
        idx_iter = idx->seek(&fixed_rowid, sizeof(xd::rowid_t), false);

        if (idx_iter != NULL)
        {
            m_idx_iter->unref();
            m_idx_iter = idx_iter;

            if (!eof())
            {
                updatePos();
            }

            return true;
        }
    
        return false;
    }


    // we are seeking for a specific key

    bool result = false;

    IIndexIterator* seek_iter;
    seek_iter = idx->seek((void*)key,
                          std::max((unsigned int)length, idx->getKeyLength()),
                          soft);
    
    if (seek_iter)
    {
        m_row_deleted = false;

        m_idx_iter->unref();
        m_idx_iter = seek_iter;

        result = true;

        if (soft)
        {
            int cmp_len = idx->getKeyLength();
            if (length < cmp_len)
                cmp_len = length;

            if (memcmp(seek_iter->getKey(), key, cmp_len) != 0)
            {
                result = false;
            }
        }
    }

    idx->unref();

    if (!eof())
    {
        updatePos();
    }

    return result;
}

bool CommonIndexIterator::seekValues(const wchar_t* values[], size_t values_size, bool soft)
{
    if (!m_layout)
    {
        m_layout = new KeyLayout;
        m_layout->setKeyExpr(m_data_iter, m_order, false);
    }

    const unsigned char* key = m_layout->getKeyFromValues(values, values_size);
    if (m_layout->getTruncation())
        return false;
    
    return seek(key, m_layout->getKeyLength(), soft);
}

bool CommonIndexIterator::setPos(double pct)
{
    if (kl::dblcompare(pct, 0.001) <= 0)
    {
        m_idx_iter->goFirst();
        return true;
    }

    if (kl::dblcompare(pct, 0.999) >= 0)
    {
        m_idx_iter->goLast();
        return true;
    }

    m_idx_iter->setPos(pct);

    m_row_deleted = false;

    updatePos();

    return true;
}

double CommonIndexIterator::getPos()
{
    return m_idx_iter->getPos();
}

void CommonIndexIterator::goRow(const xd::rowid_t& rowid)
{
    seek((unsigned char*)&rowid, sizeof(xd::rowid_t), false);
    m_data_iter->goRow(rowid);
}

void CommonIndexIterator::onRowUpdated(xd::rowid_t rowid)
{
    if (rowid == m_rowid)
    {
        // refetch row
        m_data_iter->goRow(rowid);
    }
}

void CommonIndexIterator::onRowDeleted(xd::rowid_t rowid)
{
    if (rowid == m_rowid)
    {
        m_row_deleted = true;
    }
}

std::wstring CommonIndexIterator::getTable()
{
    if (m_table.length() > 0)
        return m_table;

    return m_data_iter->getTable();
}

xd::rowpos_t CommonIndexIterator::getRowCount()
{
    return m_data_iter->getRowCount();
}

void CommonIndexIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    if (mask & xd::ifTemporary)
    {
        m_data_iter->setIteratorFlags(xd::ifTemporary, value);
    }
}

unsigned int CommonIndexIterator::getIteratorFlags()
{
    unsigned int flags = m_data_iter->getIteratorFlags();

    if (m_key_filter_len > 0)
        flags &= ~(xd::ifFastRowCount);

    return flags;
}

bool CommonIndexIterator::refreshStructure()
{
    return m_data_iter->refreshStructure();
}

xd::IStructurePtr CommonIndexIterator::getStructure()
{
    return m_data_iter->getStructure();
}

bool CommonIndexIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    return m_data_iter->modifyStructure(mod_params, job);
}

xd::IIteratorPtr CommonIndexIterator::getChildIterator(xd::IRelationPtr relation)
{
    xd::IIteratorRelationPtr iter = m_data_iter;
    if (!iter)
        return xcm::null;
    return iter->getChildIterator(relation);
}

xd::IIteratorPtr CommonIndexIterator::getFilteredChildIterator(xd::IRelationPtr relation)
{
    xd::IIteratorRelationPtr iter = m_data_iter;
    if (!iter)
        return xcm::null;
    return iter->getFilteredChildIterator(relation);
}

xd::objhandle_t CommonIndexIterator::getHandle(const std::wstring& expr)
{
    return m_data_iter->getHandle(expr);
}

xd::ColumnInfo CommonIndexIterator::getInfo(xd::objhandle_t data_handle)
{
    return m_data_iter->getInfo(data_handle);
}

int CommonIndexIterator::getType(xd::objhandle_t data_handle)
{
    return m_data_iter->getType(data_handle);
}

bool CommonIndexIterator::releaseHandle(xd::objhandle_t data_handle)
{
    return m_data_iter->releaseHandle(data_handle);
}


const unsigned char* CommonIndexIterator::getRawPtr(xd::objhandle_t data_handle)
{
    return m_data_iter->getRawPtr(data_handle);
}

int CommonIndexIterator::getRawWidth(xd::objhandle_t data_handle)
{
    return m_data_iter->getRawWidth(data_handle);
}

const std::string& CommonIndexIterator::getString(xd::objhandle_t data_handle)
{
    return m_data_iter->getString(data_handle);
}

const std::wstring& CommonIndexIterator::getWideString(xd::objhandle_t data_handle)
{
    return m_data_iter->getWideString(data_handle);
}

xd::datetime_t CommonIndexIterator::getDateTime(xd::objhandle_t data_handle)
{
    return m_data_iter->getDateTime(data_handle);
}

double CommonIndexIterator::getDouble(xd::objhandle_t data_handle)
{
    return m_data_iter->getDouble(data_handle);
}

int CommonIndexIterator::getInteger(xd::objhandle_t data_handle)
{
    return m_data_iter->getInteger(data_handle);
}

bool CommonIndexIterator::getBoolean(xd::objhandle_t data_handle)
{
    return m_data_iter->getBoolean(data_handle);
}

bool CommonIndexIterator::isNull(xd::objhandle_t data_handle)
{
    return m_data_iter->isNull(data_handle);
}


