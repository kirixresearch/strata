/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-02-16
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif


#include <stack>
#include <kl/string.h>
#include <kl/math.h>
#include <kl/memory.h>
#include <kl/portable.h>
#include "tango.h"
#include "exindex.h"
#include "exkeypool.h"
#include "../xdcommon/xdcommon.h"


#ifdef WIN32
#include <windows.h>
#endif


#define XDB_MAX_KEYSTORE_SIZE   1000000
#define XDB_MAX_KEYS_PER_NODE   25      // (must be an odd value)


const int iterator_readahead_size = 10000;
const int iterator_readahead_trigger = 1000;


// -- ModInfo class -------------------------------------------

class ModInfoBase
{
public:

    enum
    {
        modinfoReparent,
        modinfoShift,
        modinfoKeyMove,
        modinfoNodeMerge,
        modinfoNodeSplit,
        modinfoRemoveRoot,
        modinfoAddRoot
    };

    ModInfoBase() { }
    virtual ~ModInfoBase() { }
    virtual int getType() = 0;
};

class ReparentInfo : public ModInfoBase
{
public:
    int block;
    int new_parent_block;

    ReparentInfo(int _block, int _new_parent_block)
    {
        block = _block;
        new_parent_block = _new_parent_block;
    }

    int getType() { return ModInfoBase::modinfoReparent; }
};

class ShiftInfo : public ModInfoBase
{
public:
    int block;
    int orig_offset;
    int new_offset;

    ShiftInfo(int _block, int _orig_offset, int _new_offset)
    {
        block = _block;
        orig_offset = _orig_offset;
        new_offset = _new_offset;
    }

    int getType() { return ModInfoBase::modinfoShift; }
};

class KeyMoveInfo : public ModInfoBase
{
public:
    int orig_block;
    int orig_offset;
    std::vector<int> new_path;
    int new_offset;

    KeyMoveInfo()
    {
    }

    int getType() { return ModInfoBase::modinfoKeyMove; }
};

class NodeMergeInfo : public ModInfoBase
{
public:
    int orig_block;
    int new_block;
    int new_offset;

    NodeMergeInfo(int _orig_block, int _new_block, int _new_offset)
    {
        orig_block = _orig_block;
        new_block = _new_block;
        new_offset = _new_offset;
    }

    int getType() { return ModInfoBase::modinfoNodeMerge; }
};

class NodeSplitInfo : public ModInfoBase
{
public:
    int orig_block;
    int new_block;
    int middle_key_offset;

    NodeSplitInfo(int _orig_block, int _new_block, int _middle_key_offset)
    {
        orig_block = _orig_block;
        new_block = _new_block;
        middle_key_offset = _middle_key_offset;
    }

    int getType() { return ModInfoBase::modinfoNodeSplit; }
};

class RemoveRootInfo : public ModInfoBase
{
public:

    int getType() { return ModInfoBase::modinfoRemoveRoot; }
};

class AddRootInfo : public ModInfoBase
{
public:
    int new_root;

    AddRootInfo(int _new_root)
    {
        new_root = _new_root;
    }

    int getType() { return ModInfoBase::modinfoAddRoot; }
};


class ModInfo
{
public:

    std::vector<ModInfoBase*> info;
    
    ModInfo()
    {
    }

    ~ModInfo()
    {
        for (std::vector<ModInfoBase*>::iterator it = info.begin();
             it != info.end(); ++it)
        {
            delete (*it);
        }
    }

    void add(ModInfoBase* m)
    {
        info.push_back(m);
    }
};


// -- ExIndexIterator class -------------------------------------------


ExIndexIterator::ExIndexIterator(ExIndex* index)
{
    for (int i = 0; i < 30; ++i)
        m_levels[i] = NULL;

    m_curlevel = -1;
    m_first_key = NULL;

    m_index = index;
    m_index->ref();

    m_readahead_count = 0;

    index->registerIterator(this);
}

ExIndexIterator::~ExIndexIterator()
{
    if (m_first_key)
    {
        m_first_key->unref();
        m_first_key = NULL;
    }

    _clearCache();
    _clearLevels();

    m_index->unregisterIterator(this);
    m_index->unref();
}


void ExIndexIterator::_clearLevels()
{
    for (int i = 0; i <= m_curlevel; ++i)
    {
        if (m_levels[i])
        {
            m_levels[i]->unref();
            m_levels[i] = NULL;
        }
    }
    
    m_curlevel = -1;
}


void ExIndexIterator::_clearCache()
{
    for (std::vector<BlockEntry*>::iterator it = m_cache.begin();
        it != m_cache.end(); ++it)
    {
        (*it)->unref();
    }

    m_cache.clear();
}

void ExIndexIterator::_loadCache()
{
    _clearCache();

    BlockEntry* curpos_levels[30];
    int curpos_curlevel;
    int curpos_curoffset;
    int i;

    // save current position
    for (i = 0; i <= m_curlevel; ++i)
    {
        curpos_levels[i] = m_levels[i];
        if (curpos_levels[i])
        {
            curpos_levels[i]->ref();
        }
    }
    curpos_curlevel = m_curlevel;
    curpos_curoffset = m_curoffset;

    
    // collect nodes for our cache
    int last_node_idx = -1;
    for (i = 0; i < iterator_readahead_size; ++i)
    {
        _goNext();

        if (isEof())
        {
            break;
        }

        if (m_levels[m_curlevel]->m_block_idx != last_node_idx)
        {
            m_levels[m_curlevel]->ref();
            m_cache.push_back(m_levels[m_curlevel]);
        }
    }


    // restore position
    _clearLevels();

    for (i = 0; i <= curpos_curlevel; ++i)
    {
        // this is already ref'ed from before
        m_levels[i] = curpos_levels[i];
    }
    m_curlevel = curpos_curlevel;
    m_curoffset = curpos_curoffset;
}


BlockEntry* ExIndexIterator::_getRoot()
{
    // get the root block's idx from the info block
    unsigned int root_idx;
    BlockEntry* info = m_index->m_bf.getBlock(0);
    root_idx = buf2int(info->m_buf+20);
    info->unref();

    // load root node (caller must unref return value)
    return m_index->m_bf.getBlock(root_idx);
}

void ExIndexIterator::_descendLeft(BlockEntry* node)
{
    unsigned int child_idx;

    m_curlevel++;
    m_levels[m_curlevel] = node;
    m_levels[m_curlevel]->ref();

    while (1)
    {
        child_idx = m_index->_getEntryBranch(m_levels[m_curlevel], 0);
        if (child_idx == 0)
        {
            break;
        }

        m_curlevel++;
        m_levels[m_curlevel] = m_index->m_bf.getBlock(child_idx);
    }

    m_curoffset = 0;
}

void ExIndexIterator::_descendRight(BlockEntry* node)
{
    unsigned int child_idx;

    m_curlevel++;
    m_levels[m_curlevel] = node;
    m_levels[m_curlevel]->ref();

    while (1)
    {
        child_idx = m_index->_getEntryBranch(m_levels[m_curlevel], m_index->_getEntryCount(m_levels[m_curlevel]));
        if (child_idx == 0)
        {
            break;
        }

        m_curlevel++;
        m_levels[m_curlevel] = m_index->m_bf.getBlock(child_idx);
    }

    m_curoffset = m_index->_getEntryCount(m_levels[m_curlevel])-1;
}


void ExIndexIterator::_descendPct(BlockEntry* block, double pct)
{
    double c, e;
    
    m_curlevel++;
    m_levels[m_curlevel] = block;
    m_levels[m_curlevel]->ref();

    c = (m_index->_getEntryCount(block)+1);
    if (c == 0)
    {
        m_curoffset = 0;
        return;
    }

    if (pct < 0.0)
        pct = 0.0;
    if (pct > 1.0)
        pct = 1.0;

    e = ceil(c*pct) - 1.0;

    int branch = m_index->_getEntryBranch(block, int(e));

    if (branch == 0)
    {
        m_curoffset = int(e);

        if (m_curoffset >= (int)m_index->_getEntryCount(block))
        {
            m_curoffset = m_index->_getEntryCount(block)-1;
        }

        return;
    }

    if (kl::dblcompare(pct, 0.0) == 0)
    {
        pct = 0.0;
    }
     else
    {
        pct = (pct*c)-e;
    }

    BlockEntry* child = m_index->m_bf.getBlock(branch);
    _descendPct(child, pct);
    child->unref();
}






void ExIndexIterator::_goPrev()
{
    int entry_count;
    unsigned int branch;
    
    branch = m_index->_getEntryBranch(m_levels[m_curlevel], m_curoffset);

    if (branch)
    {
        BlockEntry* next_node = m_index->m_bf.getBlock(branch);
        _descendRight(next_node);
        next_node->unref();
        return;
    }
     else
    {
        m_curoffset--;

        while (m_curoffset == -1)
        {
            // go to the parent
            if (m_curlevel == 0)
            {
                // eof condition has been reached
                return;
            }

            unsigned int old_node_idx = m_levels[m_curlevel]->m_block_idx;

            m_levels[m_curlevel]->unref();
            m_levels[m_curlevel] = NULL;
            m_curlevel--;

            entry_count = m_index->_getEntryCount(m_levels[m_curlevel]);
            for (int k = 0; k <= entry_count; k++)
            {
                if (m_index->_getEntryBranch(m_levels[m_curlevel], k) == old_node_idx)
                {
                    m_curoffset = k-1;
                    break;
                }
            }
        }
    }

    return;
}

bool ExIndexIterator::_goNext()
{
    int entry_count;
    unsigned int branch;

    entry_count = m_index->_getEntryCount(m_levels[m_curlevel]);

    m_curoffset++;
    branch = m_index->_getEntryBranch(m_levels[m_curlevel], m_curoffset);

    if (branch)
    {
        bool need_to_cache = m_index->m_bf.isCached(branch);

        BlockEntry* next_node = m_index->m_bf.getBlock(branch);
        _descendLeft(next_node);
        next_node->unref();

        return !need_to_cache;
    }
     else
    {
        while (m_curoffset == entry_count)
        {
            // go to the parent
            if (m_curlevel == 0)
            {
                // eof condition has been reached
                return false;
            }

            unsigned int old_node_idx = m_levels[m_curlevel]->m_block_idx;

            m_levels[m_curlevel]->unref();
            m_levels[m_curlevel] = NULL;
            m_curlevel--;

            entry_count = m_index->_getEntryCount(m_levels[m_curlevel]);
            for (int k = 0; k <= entry_count; k++)
            {
                if (m_index->_getEntryBranch(m_levels[m_curlevel], k) == old_node_idx)
                {
                    m_curoffset = k;
                    break;
                }
            }
        }
    }

    return false;
}


void ExIndexIterator::_adjustPosition(ModInfo& mod_info)
{
    BlockEntry* cur_block = m_levels[m_curlevel];

    std::vector<ModInfoBase*>::iterator it;

    for (it = mod_info.info.begin();
         it != mod_info.info.end(); ++it)
    {
        switch ((*it)->getType())
        {
            case ModInfoBase::modinfoKeyMove:
            {
                KeyMoveInfo* mod = (KeyMoveInfo*)(*it);
                if (cur_block->m_block_idx == mod->orig_block &&
                    m_curoffset == mod->orig_offset)
                {
                    _clearLevels();
                    
                    // load new path
                    int level = 0;
                    std::vector<int>::iterator it;
                    for (it = mod->new_path.begin(); it != mod->new_path.end(); ++it)
                    {
                        m_levels[level] = m_index->m_bf.getBlock(*it);
                        level++;
                    }

                    m_curoffset = mod->new_offset;
                    m_curlevel = level-1;
                }
            }
            break;

            case ModInfoBase::modinfoShift:
            {
                ShiftInfo* mod = (ShiftInfo*)(*it);
                if (mod->block == cur_block->m_block_idx)
                {
                    if (m_curoffset >= mod->orig_offset)
                    {
                        m_curoffset += (mod->new_offset - mod->orig_offset);
                    }
                }
            }
            break;

            case ModInfoBase::modinfoNodeMerge:
            {
                NodeMergeInfo* mod = (NodeMergeInfo*)(*it);
                int i;
                for (i = 0; i < m_curlevel; ++i)
                {
                    if (m_levels[i]->m_block_idx == mod->orig_block)
                    {
                        m_levels[i]->unref();
                        m_levels[i] = m_index->m_bf.getBlock(mod->new_block);
                        break;
                    }
                }

                if (cur_block->m_block_idx == mod->orig_block)
                {
                    m_levels[m_curlevel]->unref();
                    m_levels[i] = m_index->m_bf.getBlock(mod->new_block);
                    m_curoffset += mod->new_offset;
                }
            }
            break;


            case ModInfoBase::modinfoNodeSplit:
            {
                NodeSplitInfo* mod = (NodeSplitInfo*)(*it);

                int i;
                for (i = 0; i < m_curlevel; ++i)
                {
                    if (m_levels[i]->m_block_idx == mod->orig_block)
                    {
                        // children in the node path from this point down may
                        // be a child of the new left node.  The original node
                        // (us) is now the right node in the split
                        
                        BlockEntry* new_left = m_index->m_bf.getBlock(mod->new_block);
                        unsigned int entry_count = m_index->_getEntryCount(new_left);
                        unsigned int j;

                        for (j = 0; j <= entry_count; ++j)
                        {
                            if (m_index->_getEntryBranch(new_left, j) ==
                                    m_levels[i+1]->m_block_idx)
                            {
                                m_levels[i]->unref();
                                m_levels[i] = new_left;
                                return;
                            }
                        }

                        new_left->unref();
                    }
                }

                if (mod->orig_block == cur_block->m_block_idx)
                {
                    if (m_curoffset < ((XDB_MAX_KEYS_PER_NODE-1)/2))
                    {
                        m_levels[i]->unref();
                        m_levels[i] = m_index->m_bf.getBlock(mod->new_block);
                    }
                     else if (m_curoffset == ((XDB_MAX_KEYS_PER_NODE-1)/2))
                    {
                        m_levels[m_curlevel] = NULL;
                        m_curlevel--;
                        m_curoffset = mod->middle_key_offset;
                    }
                     else
                    {
                        m_curoffset -= (((XDB_MAX_KEYS_PER_NODE-1)/2)+1);
                    }
                }

            }
            break;

            case ModInfoBase::modinfoRemoveRoot:
            {
                for (int i = 0; i < m_curlevel; ++i)
                {
                    m_levels[i] = m_levels[i+1];
                    m_levels[i+1] = NULL;
                }
                m_curlevel--;
            }
            break;

            case ModInfoBase::modinfoAddRoot:
            {
                AddRootInfo* mod = (AddRootInfo*)(*it);

                for (int i = m_curlevel; i >= 0; --i)
                {
                    m_levels[i+1] = m_levels[i];
                }
                m_levels[0] = m_index->m_bf.getBlock(mod->new_root);
                m_curlevel++;
            }
            break;
        }
    }

    // check if we are beyond the end of a node
    if (m_curoffset >= (int)m_index->_getEntryCount(m_levels[m_curlevel]))
    {
        m_curoffset--;
        _goNext();
    }
}


IIndex* ExIndexIterator::getIndex()
{
    m_index->ref();
    return static_cast<IIndex*>(m_index);
}

bool ExIndexIterator::setFirstKey()
{
    if (m_first_key)
    {
        m_first_key->unref();
        m_first_key = NULL;
    }

    m_first_key = (ExIndexIterator*)clone();

    return true;
}

void ExIndexIterator::goFirst()
{
    xcm::safe_mutex_locker tree_lock(m_index->m_tree_mutex);

    _clearLevels();

    if (m_first_key)
    {
        for (int i = 0; i <= m_first_key->m_curlevel; ++i)
        {
            m_levels[i] = m_first_key->m_levels[i];
            if (m_levels[i])
            {
                m_levels[i]->ref();
            }
        }
        
        m_curlevel = m_first_key->m_curlevel;
        m_curoffset = m_first_key->m_curoffset;
        return;
    }


    BlockEntry* root = _getRoot();
    _descendLeft(root);
    root->unref();
}

void ExIndexIterator::goLast()
{
    xcm::safe_mutex_locker tree_lock(m_index->m_tree_mutex);

    _clearLevels();

    BlockEntry* root = _getRoot();
    _descendRight(root);
    root->unref();
}

void ExIndexIterator::skip(int delta)
{
    xcm::safe_mutex_locker tree_lock(m_index->m_tree_mutex);

    if (delta == 0)
        return;

    if (delta > 0)
    {
        for (int i = 0; i < delta; i++)
        {
            if (m_readahead_count < iterator_readahead_trigger)
            {
                m_readahead_count++;
            }

            if (_goNext())
            {
                // _goNext returns whether or not we need to reload the cache
                if (m_readahead_count == iterator_readahead_trigger)
                {
                    _loadCache();
                }
            }

            if (isEof())
            {
                _clearCache();
                break;
            }
        }
    }

    if (delta < 0)
    {
        m_readahead_count = 0;

        for (int i = 0; i < -delta; i++)
        {
            _goPrev();

            if (isBof())
            {
                break;
            }
        }
    }
}

void ExIndexIterator::setPos(double pct)
{
    _clearLevels();

    // load root node
    BlockEntry* root = _getRoot();
    _descendPct(root, pct);
    root->unref();
}





double ExIndexIterator::_ascendGetPos(int level, double pct)
{
    int level_pos;
    int level_entry_count = m_index->_getEntryCount(m_levels[level]);

    if (level == m_curlevel)
    {
        level_pos = m_curoffset;
    }
     else
    {
        level_pos = -1;
        int j;
        for (j = 0; j <= level_entry_count; ++j)
        {
            if (m_index->_getEntryBranch(m_levels[level], j) ==
                                               m_levels[level+1]->m_block_idx)
            {
                level_pos = j;
                break;
            }
        }

        if (level_pos == -1)
            return 0.0;
    }

    double d1 = level_pos;
    double d2 = level_entry_count+1;
    double ret;

    ret = (d1/d2);
    ret += (pct/d2);

    if (level == 0)
        return ret;

    return _ascendGetPos(level-1, ret);
}


double ExIndexIterator::getPos()
{
    return _ascendGetPos(m_curlevel, 0.0);
}


bool ExIndexIterator::isEof()
{
    if (m_curlevel < 0)
        return true;
    if (m_curlevel != 0)
        return false;
    int entry_count = m_index->_getEntryCount(m_levels[m_curlevel]);
    return (entry_count == 0 || m_curoffset >= entry_count);
}

bool ExIndexIterator::isBof()
{
    if (m_curlevel < 0)
        return true;
    int entry_count = m_index->_getEntryCount(m_levels[m_curlevel]);
    return (entry_count == 0 || m_curoffset < 0);
}

void* ExIndexIterator::getKey()
{
    return m_index->_getEntryKey(m_levels[m_curlevel], m_curoffset);
}

void* ExIndexIterator::getValue()
{
    return m_index->_getEntryValue(m_levels[m_curlevel], m_curoffset);
}

IIndexIterator* ExIndexIterator::clone()
{
    xcm::safe_mutex_locker tree_lock(m_index->m_tree_mutex);

    ExIndexIterator* new_iter = new ExIndexIterator(m_index);

    for (int i = 0; i <= m_curlevel; ++i)
    {
        new_iter->m_levels[i] = m_levels[i];
        if (new_iter->m_levels[i])
        {
            new_iter->m_levels[i]->ref();
        }
    }

    new_iter->m_curlevel = m_curlevel;
    new_iter->m_curoffset = m_curoffset;
    if (m_first_key)
    {
        new_iter->m_first_key = (ExIndexIterator*)m_first_key->clone();
    }
    new_iter->ref();

    return new_iter;
}



// -- ExIndex class ---------------------------------------------------

ExIndex::ExIndex()
{
    m_root_node = NULL;
    m_allow_dups = false;
    m_bulk_insert = false;
    m_pool = NULL;


    // find a suitable tempfile path default

    #ifdef WIN32
        TCHAR buf[512];
        ::GetTempPath(512, buf);
        m_tempfile_path = (wchar_t*)kl::tstr(buf);

        if (m_tempfile_path.empty() ||
            m_tempfile_path[m_tempfile_path.length()-1] != L'\\')
        {
            m_tempfile_path += L"\\";
        }
    #else
        m_tempfile_path = L"/tmp/";
    #endif


    // due to the nature of the usage of this class,
    // we will increment our own reference count.  When
    // we are done using the class, we should use unref()
    // as opposed to delete

    ref();
}


ExIndex::~ExIndex()
{
    _releaseHoldCache();

    if (m_pool)
    {
        delete m_pool;
        m_pool = NULL;
    }

    if (m_bf.isOpen())
    {
        if (m_root_node)
        {
            m_root_node->unref();
            m_root_node = NULL;
        }

        m_bf.close();
    }
}

void ExIndex::registerIterator(ExIndexIterator* iter)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_iters.push_back(iter);
}

void ExIndex::unregisterIterator(ExIndexIterator* iter)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::vector<ExIndexIterator*>::iterator it;
    for (it = m_iters.begin(); it != m_iters.end(); ++it)
    {
        if (*it == iter)
        {
            m_iters.erase(it);
            return;
        }
    }
}



void ExIndex::_adjustIteratorPositions(ModInfo& mod_info)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::vector<ExIndexIterator*>::iterator it;
    for (it = m_iters.begin(); it != m_iters.end(); ++it)
    {
        (*it)->_adjustPosition(mod_info);
    }
}


bool ExIndex::_lookup(BlockEntry* blockptrs[],
                      unsigned int* blockptr_count,
                      void* key,
                      int keylen,
                      bool not_equal,
                      unsigned int* entry_idx)
{
    if (keylen == -1)
        keylen = m_keylen;

    if (!*blockptr_count)
    {
        blockptrs[(*blockptr_count)++] = m_root_node;
        m_root_node->ref();
    }

    BlockEntry* curnode = blockptrs[*blockptr_count - 1];

    int key_count;
    int i;
    int res;

    key_count = _getEntryCount(curnode);
    for (i = 0; i <= key_count; i++)
    {
        if (i < key_count)
        {
            res = memcmp(_getEntryKey(curnode, i), key, keylen);
            if (res == 0)
            {
                if (not_equal)
                {
                    // this mechanism essentially allows duplicate key storage
                    // in the tree to succeed.  we act like we didn't find the key
                    // so that each new insertation will be put at the end
                    continue;
                }

                int branch_node = _getEntryBranch(curnode, i);
                if (branch_node == 0)
                {
                    *entry_idx = i;
                    return true;
                }
                 else
                {
                    unsigned int save_blockptr_count = *blockptr_count;

                    // load child node and search in that
                    blockptrs[(*blockptr_count)++] = m_bf.getBlock(branch_node);
                    if (!_lookup(blockptrs, blockptr_count, key, keylen, not_equal, entry_idx))
                    {
                        while (*blockptr_count > save_blockptr_count)
                        {
                            (*blockptr_count)--;
                            blockptrs[*blockptr_count]->unref();
                            blockptrs[*blockptr_count] = NULL;
                        }

                        *entry_idx = i;
                    }
                    return true;
                }
            }
        }
        if (i == key_count || res > 0)
        {
            int branch_node = _getEntryBranch(curnode, i);
            if (branch_node == 0)
            {
                // we didn't find it, so return false.
                // this index is used by the insert method
                *entry_idx = i;
                return false;
            }

            // load child node and search in that
            blockptrs[(*blockptr_count)++] = m_bf.getBlock(branch_node);

            return _lookup(blockptrs,
                           blockptr_count,
                           key,
                           keylen,
                           not_equal,
                           entry_idx);
        }
    }

    return false;
}




void ExIndex::_insert(BlockEntry* blockptrs[],
                      unsigned int blockptr_count,
                      const void* key,
                      const void* value,
                      unsigned int branch,
                      int insert_loc,
                      bool* split_indicator,
                      ModInfo* mod_info)
{
    BlockEntry* curnode = blockptrs[blockptr_count-1];

    unsigned int entry_count = _getEntryCount(curnode);

    if (insert_loc == -1)
    {
        // caller will pass -1 if _insert should find
        // appropriate insert location

        for (int i = 0; i < (int)entry_count; i++)
        {
            if (memcmp(_getEntryKey(curnode, i), key, m_keylen) > 0)
            {
                insert_loc = i;
                break;
            }
        }
        
        if (entry_count == 0)
            insert_loc = 0;
        if (insert_loc == -1)
            insert_loc = entry_count;
    }

    // make room for new entry

    // (the +4 in the length param will also
    //  copy the descend right ptr)

    memmove(    _getEntryPtr(curnode, insert_loc+1),
                _getEntryPtr(curnode, insert_loc),
                ((entry_count-insert_loc)*m_entrylen)+4);

    // copy in the new key
    memcpy(_getEntryKey(curnode, insert_loc), key, m_keylen);
    memcpy(_getEntryValue(curnode, insert_loc), value, m_vallen);
    _setEntryBranch(curnode, insert_loc, branch);

    entry_count++;
    _setEntryCount(curnode, entry_count);

    curnode->setDirty(true);

    if (split_indicator)
    {
        *split_indicator = false;
    }



    // record modification info
    if (mod_info)
    {
        mod_info->add(new ShiftInfo(curnode->m_block_idx,
                                    insert_loc,
                                    insert_loc+1));
    }

    // if we have not exceeded the max keys,
    // we are done with the insert

    if (entry_count < XDB_MAX_KEYS_PER_NODE)
    {
        return;
    }


    // there are too many entries in this node, so we need to split
    // it up.  This involves creating a new left node, a new right node,
    // and taking the middle key of curnode and placing it in the parent.
    // This can recurse all the way up to the root node

    if (split_indicator)
    {
        *split_indicator = true;
    }


    BlockEntry* new_root = NULL;
    int parent_idx = 0;

    if (blockptr_count == 1)
    {
        new_root = m_bf.createBlock();
        _setEntryCount(new_root, 0);
        new_root->setDirty(true);
        parent_idx = new_root->m_block_idx;
    }
     else
    {
        parent_idx = blockptrs[blockptr_count-2]->m_block_idx;
    }


    // create new left node
    BlockEntry* left = m_bf.createBlock();
    memcpy(    _getEntryPtr(left, 0),
            _getEntryPtr(curnode, 0),
            (m_entrylen*((XDB_MAX_KEYS_PER_NODE-1)/2))+4);

    _setEntryCount(left, (XDB_MAX_KEYS_PER_NODE-1)/2);
    left->setDirty(true);

    // curnode becomes right node
    memcpy(    _getEntryPtr(curnode, 0),
            _getEntryPtr(curnode, ((XDB_MAX_KEYS_PER_NODE-1)/2)+1),
            (m_entrylen*((XDB_MAX_KEYS_PER_NODE-1)/2))+4);

    _setEntryCount(curnode, (XDB_MAX_KEYS_PER_NODE-1)/2);
    curnode->setDirty(true);

    int insert_pos = 0;

    if (new_root == NULL)
    {
        // insert entry into parent
        BlockEntry* parent = blockptrs[blockptr_count-2];
        int parent_entry_count = _getEntryCount(parent);
        for (insert_pos = 0;
             insert_pos <= parent_entry_count;
             ++insert_pos)
        {
            if (_getEntryBranch(parent, insert_pos) == curnode->m_block_idx)
                break;
        }


        // insert entry into parent
        _insert(blockptrs,
                blockptr_count-1,
                _getEntryKey(curnode, (XDB_MAX_KEYS_PER_NODE-1)/2),
                _getEntryValue(curnode, (XDB_MAX_KEYS_PER_NODE-1)/2),
                left->m_block_idx,
                insert_pos,
                NULL,
                mod_info);
    }
     else
    {
        BlockEntry* ptrs[1];
        ptrs[0] = new_root;

        insert_pos = 0;
        _insert(ptrs,
                1,
                _getEntryKey(curnode, (XDB_MAX_KEYS_PER_NODE-1)/2),
                _getEntryValue(curnode, (XDB_MAX_KEYS_PER_NODE-1)/2),
                left->m_block_idx,
                0,
                NULL,
                mod_info);

        // if new root was created, we must populate the branch right value
        _setEntryBranch(new_root, 1, curnode->m_block_idx);

        new_root->commit();


        // update info block with new root block idx
        BlockEntry* info_block = m_bf.getBlock(0);
        int2buf(info_block->m_buf+20, new_root->m_block_idx);
        info_block->setDirty(true);
        info_block->unref();

        if (m_root_node)
        {
            m_root_node->commit();

            // release old root
            m_root_node->unref();
        }
        m_root_node = new_root;

        if (mod_info)
        {
            mod_info->add(new AddRootInfo(m_root_node->m_block_idx));
        }
    }


    // record modification info

    if (mod_info)
    {
        mod_info->add(new NodeSplitInfo(curnode->m_block_idx,
                                        left->m_block_idx,
                                        insert_pos));
    }

    left->unref();

    // (curnode is unrefed by caller)
}


void ExIndex::_doHoldCache(BlockEntry* curnode, int level)
{
    int i, key_count;
    int branch_node;

    if (level == 0)
        return;

    key_count = _getEntryCount(curnode);
    for (i = 0; i <= key_count; i++)
    {
        branch_node = _getEntryBranch(curnode, i);
        if (branch_node == 0)
            continue;

        BlockEntry* node = m_bf.getBlock(branch_node);
        if (!node)
            continue;
        m_holdcache.push_back(node);
        _doHoldCache(node, level-1);
    }
}




bool ExIndex::_remove(BlockEntry* blockptrs[],
                      unsigned int blockptr_count,
                      int offset,
                      ModInfo* mod_info)
{
    BlockEntry* node = blockptrs[blockptr_count-1];
    int node_branch = _getEntryBranch(node, offset);
    int entry_count = _getEntryCount(node);

    // check if the branch has a child
    if (node_branch == 0)
    {
        // Leaf Node Delete

        int entry_count = _getEntryCount(node);

        // no, no child, so just delete
        memmove(    _getEntryPtr(node, offset),
                    _getEntryPtr(node, offset+1),
                    ((entry_count-offset)*m_entrylen)+4);

        entry_count--;

        _setEntryCount(node, entry_count);
        node->setDirty(true);

        // save mod info
        if (mod_info)
        {
            mod_info->add(new ShiftInfo(node->m_block_idx, offset+1, offset));
        }

        // are we the root?
        if (blockptr_count == 1)
        {
            // we are the root
            if (entry_count == 0)
            {
                // current root is empty, so we need to
                // set a new root
                unsigned int new_root_block = _getEntryBranch(node, 0);
                if (new_root_block != 0)
                {
                    m_root_node->unref();
                    m_root_node = m_bf.getBlock(new_root_block);

                    // update info block with new root block idx
                    BlockEntry* info_block = m_bf.getBlock(0);
                    int2buf(info_block->m_buf+20, new_root_block);
                    info_block->setDirty(true);
                    info_block->unref();

                    if (mod_info)
                    {
                        mod_info->add(new RemoveRootInfo);
                    }
                }
            }

            // if we are the root, we're finished here
            node->commit();
            return true;
        }

        
        // if we still have at least the minimum number of entries
        // in this node, we are finished
        if (entry_count >= (XDB_MAX_KEYS_PER_NODE-1)/2)
        {
            node->commit();
            return true;
        }


        // since the number of entries is now less than the
        // minimum entry count, we must borrow some keys from one of our
        // siblings, and, if that's not possible, we must merge

        BlockEntry* parent_node = blockptrs[blockptr_count-2];

        // find our parent entry
        int parent_entry_num;
        int parent_entry_count;

        parent_entry_num = -1;
        parent_entry_count = _getEntryCount(parent_node);
        for (int k = 0; k <= parent_entry_count; k++)
        {
            if (_getEntryBranch(parent_node, k) == node->m_block_idx)
            {
                parent_entry_num = k;
                break;
            }
        }

        if (parent_entry_num == -1)
        {
            return false;
        }


        BlockEntry* sibling = NULL;

        int left_right = 0;

        if (parent_entry_num == 0)
            left_right = 1; // right

        if (parent_entry_num == parent_entry_count)
            left_right = -1; // left

        if (left_right == 0)
        {
            // choose the sibling with the greater entry count
            BlockEntry* left_sibling = m_bf.getBlock(_getEntryBranch(parent_node, parent_entry_num-1));
            BlockEntry* right_sibling = m_bf.getBlock(_getEntryBranch(parent_node, parent_entry_num+1));

            left_right = 1;
            if (_getEntryCount(left_sibling) > _getEntryCount(right_sibling))
                left_right = -1;

            if (left_right == -1)
            {
                sibling = left_sibling;
                sibling->ref();
            }
            if (left_right == 1)
            {
                sibling = right_sibling;
                sibling->ref();
            }

            left_sibling->unref();
            right_sibling->unref();
        }

        if (sibling == NULL)
        {
            if (left_right < 0)
            {
                sibling = m_bf.getBlock(_getEntryBranch(parent_node, parent_entry_num-1));
            }
            if (left_right > 0)
            {
                sibling = m_bf.getBlock(_getEntryBranch(parent_node, parent_entry_num+1));
            }
        }

        // determine if we will merge or rotate

        bool merge = false;

        if (_getEntryCount(sibling) <= (XDB_MAX_KEYS_PER_NODE-1)/2)
        {
            merge = true;
        }

        if (!merge)
        {
            if (left_right == 1)
            {
                // rotate from the right

                // save right branch
                unsigned int old_right_branch = _getEntryBranch(node, entry_count);

                // copy down parent entry
                memcpy(    _getEntryPtr(node, entry_count),
                        _getEntryPtr(parent_node, parent_entry_num),
                        m_entrylen);
                
                // old right branch becomes left-branch of former parent key
                _setEntryBranch(node, entry_count, old_right_branch);

                // increment entry count
                entry_count++;
                _setEntryCount(node, entry_count);

                // copy sibling's left-most entry into the parent

                unsigned int sibling_entry_branch = _getEntryBranch(sibling, 0);

                memcpy(    _getEntryPtr(parent_node, parent_entry_num),
                        _getEntryPtr(sibling, 0),
                        m_entrylen);

                _setEntryBranch(parent_node, parent_entry_num, node->m_block_idx);
                parent_node->setDirty(true);

                // delete old entry from sibling

                int sibling_entry_count = _getEntryCount(sibling);

                memmove(    _getEntryPtr(sibling, 0),
                            _getEntryPtr(sibling, 1),
                            ((sibling_entry_count-1)*m_entrylen)+4);

                sibling_entry_count--;
                _setEntryCount(sibling, sibling_entry_count);
                sibling->setDirty(true);

                // set our right branch to the sibling entry's old left branch

                _setEntryBranch(node, entry_count, sibling_entry_branch);


                // clean-up

                node->commit();
                sibling->commit();
                sibling->unref();

                // save info modification info for the iterators
                
                if (mod_info)
                {
                    unsigned int i;

                    KeyMoveInfo* km = new KeyMoveInfo;
                    km->orig_block = parent_node->m_block_idx;
                    km->orig_offset = parent_entry_num;
                    km->new_offset = entry_count-1;
                    for (i = 0; i < blockptr_count; ++i)
                        km->new_path.push_back(blockptrs[i]->m_block_idx);
                    mod_info->add(km);

                    km = new KeyMoveInfo;
                    km->orig_block = sibling->m_block_idx;
                    km->orig_offset = 0;
                    km->new_offset = parent_entry_num;
                    for (i = 0; i < blockptr_count-1; ++i)
                        km->new_path.push_back(blockptrs[i]->m_block_idx);
                    mod_info->add(km);

                    mod_info->add(new ShiftInfo(sibling->m_block_idx, 1, 0));

                    if (sibling_entry_branch)
                    {
                        mod_info->add(new ReparentInfo(sibling_entry_branch, node->m_block_idx));
                    }
                }

                return true;
            }
             else
            {
                // rotate from the left

                
                // make space at the left-most entry

                memmove( _getEntryPtr(node, 1),
                         _getEntryPtr(node, 0),
                         (entry_count*m_entrylen)+4);

                // copy down entry from parent
                memcpy(    _getEntryPtr(node, 0),
                        _getEntryPtr(parent_node, parent_entry_num-1),
                        m_entrylen);
                
                int sibling_entry_count = _getEntryCount(sibling);

                // right branch from sibling node becomes entry 0's left branch
                int new_left_branch = _getEntryBranch(sibling, sibling_entry_count);
                _setEntryBranch(node, 0, new_left_branch);

                // increment entry count
                entry_count++;
                _setEntryCount(node, entry_count);

                // copy sibling's right-most entry into the parent

                unsigned int sibling_entry_branch = _getEntryBranch(sibling, sibling_entry_count-1);

                memcpy(    _getEntryPtr(parent_node, parent_entry_num-1),
                        _getEntryPtr(sibling, sibling_entry_count-1),
                        m_entrylen);

                _setEntryBranch(parent_node, parent_entry_num-1, sibling->m_block_idx);
                parent_node->setDirty(true);
                parent_node->commit();

                // delete old entry from sibling

                sibling_entry_count--;
                _setEntryCount(sibling, sibling_entry_count);

                // set sibling entry's right branch

                _setEntryBranch(sibling, sibling_entry_count, sibling_entry_branch);
                sibling->setDirty(true);

                // commit nodes and return

                node->commit();
                sibling->commit();
                sibling->unref();


                // save info modification info for the iterators

                if (mod_info)
                {
                    unsigned int i;

                    KeyMoveInfo* km = new KeyMoveInfo;
                    km->orig_block = parent_node->m_block_idx;
                    km->orig_offset = parent_entry_num-1;
                    km->new_offset = 0;
                    for (i = 0; i < blockptr_count; ++i)
                        km->new_path.push_back(blockptrs[i]->m_block_idx);
                    mod_info->add(km);

                    km = new KeyMoveInfo;
                    km->orig_block = sibling->m_block_idx;
                    km->orig_offset = sibling_entry_count;
                    km->new_offset = parent_entry_num-1;
                    for (i = 0; i < blockptr_count-1; ++i)
                        km->new_path.push_back(blockptrs[i]->m_block_idx);
                    mod_info->add(km);

                    mod_info->add(new ShiftInfo(sibling->m_block_idx, 0, 1));

                    if (sibling_entry_branch)
                    {
                        mod_info->add(new ReparentInfo(sibling_entry_branch, node->m_block_idx));
                    }
                }

                return true;
            }
        }
         else
        {
            // do the merge
            BlockEntry* left_node;
            BlockEntry* right_node;

            if (left_right == 1)
            {
                left_node = node;
                right_node = sibling;
            }
             else
            {
                left_node = sibling;
                right_node = node;
                parent_entry_num--;
            }

            int left_entry_count = _getEntryCount(left_node);
            int right_entry_count= _getEntryCount(right_node);

            unsigned int old_right_branch;

            old_right_branch = _getEntryBranch(left_node, left_entry_count);

            // copy down parent entry
            memcpy(    _getEntryPtr(left_node, left_entry_count),
                    _getEntryPtr(parent_node, parent_entry_num),
                    m_entrylen);
            
            // former parent entry gets our old right branch
            _setEntryBranch(left_node, left_entry_count, old_right_branch);
            left_entry_count++;

            // copy in entries from the right_node
            memcpy( _getEntryPtr(left_node, left_entry_count),
                    _getEntryPtr(right_node, 0),
                    (right_entry_count*m_entrylen)+4);

            // update the entry count

            left_entry_count += right_entry_count;
            _setEntryCount(left_node, left_entry_count);

            left_node->setDirty(true);
            left_node->commit();

            // save info modification info for the iterators

            if (mod_info)
            {
                KeyMoveInfo* km = new KeyMoveInfo;
                km->orig_block = parent_node->m_block_idx;
                km->orig_offset = parent_entry_num;
                km->new_offset = left_entry_count-right_entry_count;
                for (unsigned int i = 0; i <= blockptr_count-2; ++i)
                    km->new_path.push_back(blockptrs[i]->m_block_idx);
                km->new_path.push_back(left_node->m_block_idx);
                mod_info->add(km);

                mod_info->add(new NodeMergeInfo(right_node->m_block_idx,
                                               left_node->m_block_idx,
                                               left_entry_count-right_entry_count));
            }

            // recursively delete old parent entry

            _setEntryBranch(parent_node, parent_entry_num, 0);
            _setEntryBranch(parent_node, parent_entry_num+1, left_node->m_block_idx);

            bool result = _remove(blockptrs, blockptr_count-1, parent_entry_num, mod_info);

            // commit and return

            sibling->unref();

            return result;
        }
    }
     else
    {
        // this is an internal node entry, so we need to find the
        // successor to this key and then delete the leaf node's entry instead

        BlockEntry* r_blockptrs[30];
        unsigned int r_blockptr_count = blockptr_count;
        unsigned int i;

        for (i = 0; i < blockptr_count; ++i)
        {
            r_blockptrs[i] = blockptrs[i];
            r_blockptrs[i]->ref();
        }

        // find successor and fill out new traversal trail
        BlockEntry* child_node;
        int branch = _getEntryBranch(node, offset+1);
        while (branch)
        {
            child_node = m_bf.getBlock(branch);
            r_blockptrs[r_blockptr_count] = child_node;
            r_blockptr_count++;
            branch = _getEntryBranch(child_node, 0);
            if (branch == 0)
                break;
        }
        
        // overwrite this entry with the successor key's entry
        memcpy( _getEntryPtr(node, offset),
                _getEntryPtr(child_node, 0),
                m_entrylen);

        _setEntryBranch(node, offset, node_branch);
        node->setDirty(true);

        // recursively call remove to get rid of the leaf node's entry
        bool result = _remove(r_blockptrs, r_blockptr_count, 0, mod_info);

        for (i = 0; i < r_blockptr_count; ++i)
        {
            r_blockptrs[i]->unref();
        }

        return result;
    }

    return true;
}



void ExIndex::_holdCache(int levels)
{
    std::vector<BlockEntry*> old_entries = m_holdcache;
    std::vector<BlockEntry*>::iterator it;

    m_holdcache.clear();

    // add new levels
    _doHoldCache(m_root_node, levels);

    // release old hold cache
    for (it = old_entries.begin(); it != old_entries.end(); ++it)
    {
        (*it)->unref();
    }
}

void ExIndex::_releaseHoldCache()
{
    for (std::vector<BlockEntry*>::iterator it = m_holdcache.begin();
            it != m_holdcache.end(); ++it)
    {
        (*it)->unref();
    }

    m_holdcache.clear();
}


std::wstring ExIndex::getFilename()
{
    return m_filename;
}


bool ExIndex::create(const std::wstring& filename,
                     int keylen,
                     int vallen,
                     bool allow_dups)
{
    m_keylen = keylen;
    m_vallen = vallen;
    m_entrylen = keylen+vallen+4;
    m_allow_dups = allow_dups;

    int block_size = (m_entrylen*(XDB_MAX_KEYS_PER_NODE-1))+5;

    if (!m_bf.create(filename, block_size))
        return false;

    m_filename = filename;

    m_bf.setBlockAllocSize(block_size+keylen+vallen+4);

    BlockEntry* info_block = m_bf.createBlock();

    // set info
    int2buf(info_block->m_buf, 0xaa505100);            // file signature
    int2buf(info_block->m_buf+4, 1);                   // version
    int2buf(info_block->m_buf+8, m_keylen);            // key length
    int2buf(info_block->m_buf+12, m_vallen);           // value length
    int2buf(info_block->m_buf+16, allow_dups ? 1 : 0); // allow dups
    int2buf(info_block->m_buf+20, 1);                  // root node idx
    info_block->setDirty(true);
    info_block->unref();

    if (m_root_node)
    {
        m_root_node->unref();
    }

    m_root_node = m_bf.createBlock();
    _setEntryCount(m_root_node, 0);
    m_root_node->setDirty(true);
    m_root_node->commit();

    return true;
}



bool ExIndex::close()
{
    if (m_bf.isOpen())
    {
        if (m_root_node)
        {
            m_root_node->unref();
            m_root_node = NULL;
        }

        m_bf.close();
    }

    return true;
}



bool ExIndex::open(const std::wstring& filename)
{
    close();

    if (!m_bf.open(filename))
    {
        m_filename = L"";
        return false;
    }    
    
    m_filename = filename;

    BlockEntry* info_block = m_bf.getBlock(0);

    unsigned int root_idx, signature, version;

    signature = buf2int(info_block->m_buf);
    version = buf2int(info_block->m_buf+4);

    if (signature != 0xaa505100)
    {
        // invalid signature
        m_bf.close();
        return false;
    }

    if (version > 1)
    {
        // version too high
        m_bf.close();
        return false;
    }

    m_keylen = buf2int(info_block->m_buf+8);
    m_vallen = buf2int(info_block->m_buf+12);
    m_entrylen = m_keylen+m_vallen+4;
    m_allow_dups = buf2int(info_block->m_buf+16) ? true : false;
    root_idx = buf2int(info_block->m_buf+20);
    info_block->unref();

    m_bf.setBlockAllocSize(m_bf.getBlockSize()+m_keylen+m_vallen+4);
    m_root_node = m_bf.getBlock(root_idx);

    return true;
}


void ExIndex::flush()
{
    m_bf.flushDirty();
}

bool ExIndex::reopen()
{
    open(m_filename);
    return m_bf.isOpen();
}

bool ExIndex::_sortedInsert(IKeyList* pool,
                            IIndexProgress* progress)
{
    BlockEntry* old_root;
    BlockEntry* blockptrs[30];
    int blockptr_count = 0;
    BlockEntry* insert_node = m_root_node;
    int insert_loc;
    insert_node->ref();
    unsigned char* ptr;
    bool split;
    bool cancelled = false;
    tango::rowpos_t count = 0;

    insert_loc = 0;
    split = false;
    old_root = NULL;

    pool->goFirst();
    while (!pool->isEof())
    {
        ptr = pool->getEntryPtr();

        
        // check if the hierarchy has changed
        if (m_root_node != old_root)
        {
            // save blocks for releasing
            std::vector<BlockEntry*> to_release;
            int i;
            for (i = 0; i < blockptr_count; ++i)
            {
                to_release.push_back(blockptrs[i]);
            }
            
            blockptrs[0] = m_root_node;
            blockptrs[0]->ref();
            blockptr_count = 1;

            int branch;
            while (1)
            {
                // get right branch
                branch = _getEntryBranch(blockptrs[blockptr_count-1],
                                         _getEntryCount(blockptrs[blockptr_count-1]));

                if (branch == 0)
                    break;

                blockptrs[blockptr_count] = m_bf.getBlock(branch);
                ++blockptr_count;
            }
 
            // release old blocks
            std::vector<BlockEntry*>::iterator it;
            for (it = to_release.begin(); it != to_release.end(); ++it)
            {
                (*it)->unref();
            }

            old_root = m_root_node;
        }


        _insert(blockptrs,
                blockptr_count,
                ptr,
                ptr+m_keylen,
                0,
                insert_loc,
                &split,
                NULL);

        insert_loc++;
        if (split)
        {
            insert_loc = _getEntryCount(insert_node);
            split = false;
        }

        if (progress)
        {
            progress->updateProgress(count,
                                     m_bulk_total,
                                     &cancelled);
            if (cancelled)
                break;

            count++;
        }

        pool->goNext();
    }


    // release insert hierarchy
    int i;
    for (i = 0; i < blockptr_count; ++i)
    {
        blockptrs[i]->unref();
    }



    insert_node->unref();
    return true;
}


void ExIndex::startBulkInsert(tango::rowpos_t total_keys)
{
    if (m_pool)
    {
        delete m_pool;
    }

    // decide on the absolute maximum amount of pool space
    long long free_mem = (long long)kl::getFreePhysMemory();
    long long max_pool_bytes = free_mem/5;
    if (max_pool_bytes > 256000000)
        max_pool_bytes = 256000000;
    if (max_pool_bytes < 8000000)
        max_pool_bytes = 8000000;

    // calculate how large the key pool would be if we stored
    // every key/value pair in one pool
    long long total_required_pool_size = (m_keylen + m_vallen);
    total_required_pool_size *= total_keys;

    long long pool_bytes = 64000000;
    if (free_mem >  500000000)
        pool_bytes = 128000000;
    if (free_mem > 2000000000)
        pool_bytes = 256000000;

    if (total_required_pool_size < max_pool_bytes &&
        total_required_pool_size > 0)
    {
        pool_bytes = total_required_pool_size;
    }

    m_pool = new ExKeyPool((int)pool_bytes, m_keylen + m_vallen, m_keylen);


    long block_size = m_bf.getBlockAllocSize();
    if (block_size == 0)
        return;

    m_bulk_insert = true;
    m_bulk_total = total_keys;
    
    m_bulk_filestub = m_tempfile_path;
    m_bulk_filestub += getUniqueString();
    m_bulk_filenum = 1;
}


void ExIndex::cancelBulkInsert()
{
    if (m_pool)
    {
        delete m_pool;
        m_pool = NULL;
    }

    // cancelled, so don't write anything
    m_bf.markAllClean();

    flush();
    m_bf.freePool();
    m_bulk_insert = false;
    m_root_node->commit();

    // delete the temp files
    std::vector<std::wstring>::iterator it;
    for (it = m_pool_files.begin(); it != m_pool_files.end(); ++it)
    {
        xf_remove(*it);
    }
}

void ExIndex::finishBulkInsert(IIndexProgress* progress)
{
    m_pool->sort();
    m_pool->goFirst();

    if (m_pool_files.size() == 0)
    {
        _sortedInsert(m_pool, progress);
    }
     else
    {
        int pool_file_count = m_pool_files.size();
        ExKeyPoolFileReader* pools = new ExKeyPoolFileReader[pool_file_count];
        
        ExKeyPoolCombiner pool_aggregate(m_keylen+m_vallen, m_keylen);

        int i;
        for (i = 0; i < pool_file_count; ++i)
        {
            pools[i].open(m_pool_files[i], m_keylen+m_vallen);
            pool_aggregate.addPool(&pools[i]);
        }

        if (!m_pool->isEof())
        {
            pool_aggregate.addPool(m_pool);
        }

        _sortedInsert(&pool_aggregate, progress);

        delete[] pools;
    }

    if (m_pool)
    {
        delete m_pool;
        m_pool = NULL;
    }

    // delete the temp files
    std::vector<std::wstring>::iterator it;
    for (it = m_pool_files.begin(); it != m_pool_files.end(); ++it)
    {
        xf_remove(*it);
    }

    flush();
    m_bf.freePool();
    m_bulk_insert = false;

    m_root_node->commit();
}


xcm::result ExIndex::insert(const void* key,
                            int keylen,
                            const void* value,
                            int vallen)
{

    if (keylen < 0 || (unsigned int)keylen > m_keylen)
        return idxErrInvalidParameter;
    if (vallen < 0 || (unsigned int)vallen > m_vallen)
        return idxErrInvalidParameter;

    unsigned char* k = (unsigned char*)key;
    unsigned char* v = (unsigned char*)value;

    if (keylen != m_keylen)
    {
        k = new unsigned char[m_keylen];
        memset(k, 0, m_keylen);
        memcpy(k, key, keylen);
    }

    if (vallen != m_vallen)
    {
        v = new unsigned char[m_vallen];
        memset(v, 0, m_vallen);
        memcpy(v, value, vallen);
    }

    if (m_bulk_insert)
    {
        if (m_pool->isFull())
        {
            // we have filled up this key pool, so we need to sort
            // it and write it out
        
            m_pool->sort();
     
            // generate a good temp filename
            std::wstring filename;
        
            filename = m_bulk_filestub;
            filename += L".";
            wchar_t buf[64];
            swprintf(buf, 64, L"%d", m_bulk_filenum);

            filename += buf;
            m_bulk_filenum++;

            if (!m_pool->writeToFile(filename))
            {
                if (keylen != m_keylen)
                    delete[] k;

                if (vallen != m_vallen)
                    delete[] v;

                return idxErrNoStorageSpace;
            }

            m_pool_files.push_back(filename);

            m_pool->reset();
        }

        m_pool->appendData2(k, m_keylen, v, m_vallen);
    }
     else
    {
        XCM_AUTO_LOCK(m_tree_mutex);

        unsigned int entry_idx;
        BlockEntry* blockptrs[30];
        unsigned int blockptrs_cnt = 0;
        bool split = false;
        bool result = _lookup(blockptrs,
                              &blockptrs_cnt,
                              k,
                              m_keylen,
                              m_allow_dups,
                              &entry_idx);

        if (!m_allow_dups && result)
        {
            if (keylen != m_keylen)
                delete[] k;
            if (vallen != m_vallen)
                delete[] v;
        
            return idxErrDuplicateKey;
        }

        ModInfo mod_info;

        _insert(blockptrs,
                blockptrs_cnt,
                key,
                value,
                0,
                entry_idx,
                &split,
                &mod_info);

        _adjustIteratorPositions(mod_info);

        for (unsigned int i = 0; i < blockptrs_cnt; i++)
        {
            blockptrs[i]->unref();
        }
    }

    if (keylen != m_keylen)
        delete[] k;

    if (vallen != m_vallen)
        delete[] v;

    return idxErrSuccess;
}



bool ExIndex::remove(IIndexIterator* _iter)
{
    XCM_AUTO_LOCK(m_tree_mutex);

    if (_iter == NULL)
        return false;

    ExIndexIterator* iter = (ExIndexIterator*)_iter;

    ModInfo mod_info;

    if (!_remove(iter->m_levels,
                 iter->m_curlevel+1,
                 iter->m_curoffset,
                 &mod_info))
    {
        return false;
    }

    // work around code -- there's an infrequent crash when the
    // iterator passed to this function has its position adjusted.
    // See a few lines below for the code that originally elicited
    // the circumstances that cause this crash.
    
    std::vector<ExIndexIterator*>::iterator it;
    for (it = m_iters.begin(); it != m_iters.end(); ++it)
    {
        if (((IIndexIterator*)*it) != _iter)
          (*it)->_adjustPosition(mod_info);
    }


    // the above code replaces this line.  Put this line back when 
    // the real fix is found.   To duplicate.  Create a 547-record file
    // with one field with all '1' values.  Sort it.  Delete all records.
    
    // _adjustIteratorPositions(mod_info);
        
    return true;
}


unsigned int ExIndex::getKeyCount()
{
    IIndexIterator* it = createIterator();
    if (it == NULL)
        return 0;

    unsigned int count = 0;

    it->goFirst();
    while (!it->isEof())
    {
        it->skip(1);
        count++;
    }

    it->unref();
    return count;
}

unsigned int ExIndex::getKeyLength()
{
    return m_keylen;
}

unsigned int ExIndex::getValueLength()
{
    return m_vallen;
}

IIndexIterator* ExIndex::seek(void* key, int keylen, bool soft)
{
    ExIndexIterator* iter = new ExIndexIterator(this);

    unsigned int entry_idx;

    unsigned int blockptrs_cnt = 0;

    bool result = _lookup(iter->m_levels,
                          &blockptrs_cnt,
                          key,
                          keylen,
                          false,
                          &entry_idx);

    if (!result && !soft)
    {
        iter->m_curlevel = blockptrs_cnt-1;
        delete iter;
        return NULL;
    }

    iter->m_curlevel = blockptrs_cnt-1;
    iter->m_curoffset = entry_idx;

    if (!result && soft)
    {
        if (iter->m_curoffset == _getEntryCount(iter->m_levels[iter->m_curlevel]))
        {
            iter->m_curoffset--;
            iter->_goNext();
        }
    }

    iter->ref();
    return iter;
}

IIndexIterator* ExIndex::createIterator()
{
    ExIndexIterator* it = new ExIndexIterator(this);
    it->ref();
    return static_cast<IIndexIterator*>(it);
}



void ExIndex::setTempFilePath(const std::wstring& path)
{
    m_tempfile_path = path;

    #ifdef WIN32
    if (m_tempfile_path.empty() ||
        m_tempfile_path[m_tempfile_path.length()-1] != L'\\')
    {
        m_tempfile_path += L"\\";
    }
    #else
    if (m_tempfile_path.empty() ||
        m_tempfile_path[m_tempfile_path.length()-1] != L'/')
    {
        m_tempfile_path += L"/";
    }
    #endif
}



#ifdef _DEBUG

void doDebugOutput(const char* a)
{
#ifdef WIN32
    OutputDebugStringA(a);
#else
    fprintf(stderr, a);
#endif
}

int ExIndex::_dumpTree(BlockEntry* curnode, int level)
{
    int i, key_count;
    int branch_node;

    char bigbuf[4096];
    char spaces[255];
    char buf[255];

    strcpy(spaces, "                                                ");
    spaces[level] = 0;

    key_count = _getEntryCount(curnode);

    if (key_count < (XDB_MAX_KEYS_PER_NODE-1)/2 && curnode != m_root_node)
    {
        sprintf(bigbuf, "not enough keys: %d (%d keys)\n", curnode->m_block_idx, key_count);
        doDebugOutput(bigbuf);
    }

    int keys=0;

    for (i = 0; i <= key_count; i++)
    {
        if (i < key_count)
        {
            memcpy(buf, _getEntryKey(curnode, i), m_keylen);
            buf[m_keylen] = 0;
        }
         else
        {
            buf[0] = 0;
            if (_getEntryBranch(curnode, i) != 0)
            {
                sprintf(buf, "(descend right -> %d)", _getEntryBranch(curnode, i));
            }
        }

        sprintf(bigbuf, "%03d %s%02d-%02d: %s\n", level, spaces, curnode->m_block_idx, i, buf);
        doDebugOutput(bigbuf);
        
        branch_node = _getEntryBranch(curnode, i);
        if (branch_node == 0)
            continue;

        BlockEntry* node = m_bf.getBlock(branch_node);

        if (!node)
            continue;

        keys += _dumpTree(node, level+1);

        node->unref();
    }

    return keys+key_count;
}

void ExIndex::dumpTree()
{
    char buf[255];
    sprintf(buf, "\nkeys: %d\n", _dumpTree(m_root_node, 0));
    doDebugOutput(buf);
}

#endif




