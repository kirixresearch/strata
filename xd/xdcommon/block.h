/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-24
 *
 */


#ifndef H_XDCOMMON_BLOCK_H
#define H_XDCOMMON_BLOCK_H


#include <vector>
#include <queue>
#include <kl/file.h>


enum
{
    bfInvalidBlock = 0xffffffff,
    bfNewBlock = 0xfffffffe
};


class BlockFile;


class BlockEntry
{
friend class BlockFile;

    unsigned int m_hold_count;
    unsigned int m_deferred_unrefs;

    void flushAndDelete();

public:

    BlockFile* m_block_file;
    unsigned char* m_buf;
    unsigned int m_block_idx;
    bool m_dirty;
    bool m_is_pool;

    BlockEntry();
    ~BlockEntry();
    void commit();
    void setDirty(bool dirty);

    void flushDeferredUnrefs();

    inline void ref()
    {
        m_hold_count++;
    }

    inline void unref()
    {
        if (--m_hold_count == 0)
        {
            flushAndDelete();
        }
    }

    inline void unrefDeferred()
    {
        m_deferred_unrefs++;
    }


};

typedef std::vector<BlockEntry*> BlockList;
typedef std::vector<BlockEntry*>::iterator BlockListIterator;
typedef std::vector<BlockEntry*>::reverse_iterator BlockListRevIterator;


class IntAssoc
{
public:

    std::vector<BlockEntry*> m_arr;
    
    IntAssoc()
    {
        m_arr.resize(10000, NULL);
    }

    virtual ~IntAssoc()
    {
    }

    int getSize() const
    {
        return m_arr.size();
    }

    void add(BlockEntry* e)
    {
        while (e->m_block_idx >= m_arr.size())
            m_arr.resize(m_arr.size()+10000);
        m_arr[e->m_block_idx] = e;
    }

    BlockEntry* get(int block_idx)
    {
        while (block_idx > 0 && (size_t)block_idx >= m_arr.size())
            m_arr.resize(m_arr.size()+10000);
        return m_arr[block_idx];
    }

    void remove(int block_idx)
    {
        while (block_idx > 0 && (size_t)block_idx >= m_arr.size())
            m_arr.resize(m_arr.size()+10000);
        m_arr[block_idx] = NULL;
    }

    void commitAll()
    {
        int dirty = 0;
        int clean = 0;
        int unused = 0;
        for (std::vector<BlockEntry*>::iterator it = m_arr.begin();
                it != m_arr.end(); ++it)
        {
            if (*it)
            {
                if ((*it)->m_dirty)
                {
                    dirty++;
                    (*it)->commit();
                }
                 else
                clean++;
            }
             else
            unused++;
        }
    }

    void flushDeferredUnrefs()
    {
        for (std::vector<BlockEntry*>::iterator it = m_arr.begin();
                it != m_arr.end(); ++it)
        {
            if (*it)
            {
                (*it)->flushDeferredUnrefs();
            }
        }
    }

    void markAllClean()
    {
        for (std::vector<BlockEntry*>::iterator it = m_arr.begin();
                it != m_arr.end(); ++it)
        {
            if (*it)
            {
                (*it)->m_dirty = false;
            }
        }
    }
};


class BlockFile
{
friend class BlockEntry;

public:

    BlockFile();
    ~BlockFile();

    // open/close
    bool create(const std::wstring& filename, unsigned int block_size = 512);
    bool open(const std::wstring& filename);
    void close();
    unsigned int getBlockSize();
    void setBlockAllocSize(unsigned int alloc_size);
    unsigned int getBlockAllocSize();
    bool isOpen();

    // cache control
    void setPoolSize(int new_val);
    void freePool();
    bool getPoolFull();
    int getPoolFree();
    void flushDirty();
    void markAllClean();

    // block access
    int getBlockCount();
    bool isCached(unsigned int block_idx);
    BlockEntry* getBlock(unsigned int block_idx);
    BlockEntry* createBlock();
    BlockEntry* createUnattachedBlock();

protected:

    xf_file_t m_f;
    IntAssoc m_cache;

    unsigned int m_alloc_size;
    unsigned int m_block_size;
    unsigned int m_cur_block;

    //unsigned char* m_pool;
    std::vector<unsigned char*> m_pool;
    std::queue<unsigned char*> m_free_bufs;

    unsigned char* _getPoolPtr();
    void _releasePoolPtr(unsigned char* ptr);

    void _freeBlockMem(BlockEntry* entry);
    void _flushBlock(BlockEntry* entry);
    void _commitBlock(BlockEntry* entry);
    void _depoolAllBufs();

};






#endif

