/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-09-12
 *
 */


#include "tango.h"
#include "block.h"


/*

// asynchronous test function
inline int xf_write_async(xf_file_t fileh,
                    void* buffer,
                    const xf_off_t& position,
                    unsigned int write_size,
                    unsigned int write_count)
{
    OVERLAPPED o;
    o.Offset = (position & 0xffffffff);
    o.OffsetHigh = (position >> 32);
    o.Internal = 0;
    o.InternalHigh = 0;
    o.hEvent = 0;

    WriteFileEx(fileh, buffer, write_size*write_count, &o, NULL);
    return write_size*write_count;
}
*/


//int g_alloced = 0;
//int g_dealloced = 0;
//int g_exists = 0;

BlockEntry::BlockEntry()
{
    //g_alloced++;
    //g_exists++;

    m_block_file = NULL;
    m_buf = NULL;
    m_hold_count = 0;
    m_block_idx = 0;
    m_dirty = false;
    m_deferred_unrefs = 0;
    m_is_pool = false;
}

BlockEntry::~BlockEntry()
{
    //g_dealloced++;
    //g_exists--;

    if (m_buf && m_block_file)
    {
        m_block_file->_freeBlockMem(this);
    }
}


void BlockEntry::flushAndDelete()
{
    if (m_block_file)
    {
        m_block_file->_flushBlock(this);
    }
}


void BlockEntry::commit()
{
    if (m_block_file)
    {
        m_block_file->_commitBlock(this);
    }
}

void BlockEntry::setDirty(bool dirty)
{
    m_dirty = dirty;
}

void BlockEntry::flushDeferredUnrefs()
{
    m_hold_count -= m_deferred_unrefs;
    m_deferred_unrefs = 0;
    if (m_hold_count <= 0)
    {
        m_hold_count = 0;
        if (m_block_file)
        {
            m_block_file->_flushBlock(this);
        }
    }
}


// BlockFile implementation

BlockFile::BlockFile()
{
    m_cur_block = 0;
    m_block_size = 0;
    m_alloc_size = 0;
    m_f = NULL;
}

BlockFile::~BlockFile()
{
    if (m_pool.size() > 0)
    {
        freePool();
    }
}

bool BlockFile::create(const std::wstring& filename, unsigned int block_size)
{
    m_f = xf_open(filename, xfCreate, xfReadWrite, xfShareReadWrite);
    if (!m_f)
        return false;

    unsigned char buf[4];

    // write out version id (== 1)
    buf[0] = 0x01;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;

    if (xf_write(m_f, buf, 4, 1) != 1)
    {
        xf_close(m_f);
        return false;
    }

    // write out row count to dbf header
    buf[0] = (block_size & 0x000000ff);
    buf[1] = ((block_size & 0x0000ff00) >> 8);
    buf[2] = ((block_size & 0x00ff0000) >> 16);
    buf[3] = ((block_size & 0xff000000) >> 24);

    m_block_size = block_size;
    m_alloc_size = block_size;

    if (xf_write(m_f, buf, 4, 1) != 1)
    {
        xf_close(m_f);
        return false;
    }

    return true;
}


bool BlockFile::open(const std::wstring& filename)
{
    m_f = xf_open(filename, xfOpen, xfReadWrite, xfShareReadWrite);
    if (!m_f)
        return false;

    unsigned char buf[4];
    if (xf_read(m_f, buf, 4, 1) != 1)
    {
        xf_close(m_f);
        m_f = NULL;
        return false;
    }

    unsigned int version;
    version =        (((unsigned int)buf[0])) |
                    (((unsigned int)buf[1]) << 8) |
                    (((unsigned int)buf[2]) << 16) |
                    (((unsigned int)buf[3]) << 24);

    if (version != 1)
        return false;

    if (xf_read(m_f, buf, 4, 1) != 1)
    {
        xf_close(m_f);
        return false;
    }

    m_block_size =    (((unsigned int)buf[0])) |
                    (((unsigned int)buf[1]) << 8) |
                    (((unsigned int)buf[2]) << 16) |
                    (((unsigned int)buf[3]) << 24);

    m_alloc_size = m_block_size;

    return true;
}

void BlockFile::close()
{
    if (m_pool.size() > 0)
    {
        freePool();
    }
    
    if (m_f)
    {
        xf_close(m_f);
        m_f = NULL;
    }
}

bool BlockFile::isOpen()
{
    return (m_f != NULL);
}


void BlockFile::setPoolSize(int new_val)
{
    /*
    _depoolAllBufs();

    if (m_pool)
    {
        free(m_pool);
        m_pool = NULL;
    }

    while (!m_free_bufs.empty())
        m_free_bufs.pop();

    if (new_val == 0)
    {
        return;
    }

    m_pool = (unsigned char*)malloc(m_alloc_size*new_val);

    for (int i = 0; i < new_val; ++i)
    {
        m_free_bufs.push(m_pool + (i * m_alloc_size));
    }
    */
}


void BlockFile::freePool()
{
    _depoolAllBufs();

    std::vector<unsigned char*>::iterator it;
    for (it = m_pool.begin(); it != m_pool.end(); ++it)
    {
        free(*it);
    }

    m_pool.clear();

    while (!m_free_bufs.empty())
        m_free_bufs.pop();
}


void BlockFile::_freeBlockMem(BlockEntry* e)
{
    if (!e->m_buf)
    {
        return;
    }

    if (e->m_is_pool)
    {
        m_free_bufs.push(e->m_buf);
        e->m_is_pool = false;
    }
     else
    {
        free(e->m_buf);
    }

    e->m_buf = NULL;
}


unsigned char* BlockFile::_getPoolPtr()
{
    if (m_free_bufs.empty())
    {
        // we need to allocate another pool chunk
        if (m_alloc_size == 0)
            return NULL;

        unsigned char* pool;
        int pool_blocks = 1000000 / m_alloc_size;

        pool = (unsigned char*)malloc(m_alloc_size*pool_blocks);

        for (int i = 0; i < pool_blocks; ++i)
        {
            m_free_bufs.push(pool + (i * m_alloc_size));
        }

        m_pool.push_back(pool);
    }

    unsigned char* result = m_free_bufs.front();
    m_free_bufs.pop();
    return result;
}

void BlockFile::_releasePoolPtr(unsigned char* ptr)
{
    m_free_bufs.push(ptr);
}

void BlockFile::_depoolAllBufs()
{
    BlockEntry* e;
    int cache_size = m_cache.getSize();
    int i;
    for (i = 0; i < cache_size; ++i)
    {
        e = m_cache.m_arr[i];
        if (!e)
        {
            continue;
        }

        if (e->m_is_pool)
        {
            unsigned char* new_buf = (unsigned char*)malloc(m_alloc_size);
            memcpy(new_buf, e->m_buf, m_alloc_size);
            _releasePoolPtr(e->m_buf);
            e->m_buf = new_buf;
            e->m_is_pool = false;
        }
    }
}


bool BlockFile::getPoolFull()
{
    return (m_free_bufs.size() < 1000) && (m_pool.size() > 20);
}


int BlockFile::getPoolFree()
{
    return m_free_bufs.size();
}

void BlockFile::flushDirty()
{
    m_cache.commitAll();
    m_cache.flushDeferredUnrefs();
}

void BlockFile::markAllClean()
{
    m_cache.markAllClean();
}

unsigned int BlockFile::getBlockSize()
{
    return m_block_size;
}

void BlockFile::setBlockAllocSize(unsigned int alloc_size)
{
    freePool();
    m_alloc_size = alloc_size;
}

unsigned int BlockFile::getBlockAllocSize()
{
    return m_alloc_size;
}

void BlockFile::_flushBlock(BlockEntry* entry)
{
    if (entry->m_dirty)
    {
        _commitBlock(entry);
    }

    m_cache.remove(entry->m_block_idx);

    delete entry;
}

void BlockFile::_commitBlock(BlockEntry* entry)
{
    if (!entry->m_dirty)
        return;

    entry->m_dirty = false;

    if (entry->m_block_idx == bfNewBlock)
    {
        // append the new block
        xf_seek(m_f, 0, xfSeekEnd);
        xf_off_t pos = xf_get_file_pos(m_f);
        entry->m_block_idx = ((pos-8)/m_block_size);
        xf_write(m_f, entry->m_buf, m_block_size, 1);

        m_cache.add(entry);
    }
     else
    {
        xf_off_t pos;
        pos = m_block_size;
        pos *= entry->m_block_idx;
        pos += 8;

        xf_seek(m_f, pos, xfSeekSet);
        xf_write(m_f, entry->m_buf, m_block_size, 1);
    }
}



int BlockFile::getBlockCount()
{
    xf_seek(m_f, 0, xfSeekEnd);
    xf_off_t pos = xf_get_file_pos(m_f);
    return ((pos-8)/m_block_size);
}


bool BlockFile::isCached(unsigned int block_idx)
{
    return m_cache.get(block_idx) ? true : false;
}

BlockEntry* BlockFile::getBlock(unsigned int block_idx)
{
    BlockEntry* entry;

    entry = m_cache.get(block_idx);
    if (entry)
    {
        entry->ref();
        return entry;
    }

    entry = new BlockEntry;
    entry->m_buf = _getPoolPtr();
    entry->m_block_file = this;
    entry->m_hold_count = 1;
    entry->m_dirty = false;
    entry->m_block_idx = block_idx;
    entry->m_is_pool = (entry->m_buf ? true : false);

    if (!entry->m_buf)
    {
        entry->m_buf = (unsigned char*)malloc(m_alloc_size);
    }


    xf_off_t pos;
    pos = m_block_size;
    pos *= block_idx;
    pos += 8;

    xf_seek(m_f, pos, xfSeekSet);
    xf_read(m_f, entry->m_buf, m_block_size, 1);

    m_cache.add(entry);

    return entry;
}


BlockEntry* BlockFile::createBlock()
{
    BlockEntry* entry = new BlockEntry;
    entry->m_buf = _getPoolPtr();
    entry->m_block_file = this;
    entry->m_hold_count = 1;
    entry->m_dirty = false;
    entry->m_is_pool = (entry->m_buf ? true : false);

    if (!entry->m_buf)
    {
        entry->m_buf = (unsigned char*)malloc(m_alloc_size);
    }

    memset(entry->m_buf, 0, m_alloc_size);


    // append the new block
    xf_seek(m_f, 0, xfSeekEnd);
    xf_off_t pos = xf_get_file_pos(m_f);
    entry->m_block_idx = ((pos-8)/m_block_size);
    xf_write(m_f, entry->m_buf, m_block_size, 1);

    m_cache.add(entry);

    return entry;
}



BlockEntry* BlockFile::createUnattachedBlock()
{
    BlockEntry* entry = new BlockEntry;
    entry->m_buf = _getPoolPtr();
    entry->m_block_file = this;
    entry->m_hold_count = 1;
    entry->m_dirty = false;
    entry->m_is_pool = (entry->m_buf ? true : false);

    if (!entry->m_buf)
    {
        entry->m_buf = (unsigned char*)malloc(m_alloc_size);
    }

    memset(entry->m_buf, 0, m_alloc_size);

    entry->m_block_idx = bfNewBlock;
    return entry;
}



