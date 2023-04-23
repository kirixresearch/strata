/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-02-16
 *
 */


#ifndef H_XDCOMMON_EXINDEX_H
#define H_XDCOMMON_EXINDEX_H


#include "util.h"
#include "block.h"


class ExIndex;
class ExIndexIterator;
class ExKeyPool;
class IKeyList;
class ModInfo;


// -- indexing interfaces --


enum
{
    idxErrSuccess = 0,
    idxErrInvalidParameter = -1,
    idxErrDuplicateKey = -2,
    idxErrNoStorageSpace = -3,
};


class IIndexProgress
{
public:

    virtual void updateProgress(xd::rowpos_t cur_count,
                                xd::rowpos_t tot_count,
                                bool* cancelled) = 0;
};

xcm_interface IIndex;

xcm_interface IIndexIterator : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IIndexIterator")

public:

    virtual IIndex* getIndex() = 0;
    virtual bool setFirstKey() = 0;
    virtual void goFirst() = 0;
    virtual void goLast() = 0;
    virtual void skip(int delta) = 0;
    virtual void setPos(double pct) = 0;
    virtual double getPos() = 0;
    virtual bool isEof() = 0;
    virtual bool isBof() = 0;
    virtual void* getKey() = 0;
    virtual void* getValue() = 0;
    virtual IIndexIterator* clone() = 0;
};


xcm_interface IIndex : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IIndex")

public:
    
    virtual std::wstring getFilename() = 0;
    virtual IIndexIterator* createIterator() = 0;
    virtual IIndexIterator* seek(void* key, int keylen, bool soft) = 0;
    virtual unsigned int getKeyCount() = 0;
    virtual unsigned int getKeyLength() = 0;
    virtual unsigned int getValueLength() = 0;
    virtual bool remove(IIndexIterator* iter) = 0;
    virtual void startBulkInsert(xd::rowpos_t total_keys) = 0;
    virtual int insert(const void* key, int keylen, const void* value, int vallen) = 0;
    virtual void cancelBulkInsert() = 0;
    virtual void finishBulkInsert(IIndexProgress* progress) = 0;
    virtual void flush() = 0;
    virtual bool close() = 0;
    virtual bool reopen() = 0;
};



class ExIndexIterator : public IIndexIterator
{
friend class ExIndex;

    XCM_CLASS_NAME("xdnative.ExIndexIterator")
    XCM_BEGIN_INTERFACE_MAP(ExIndexIterator)
        XCM_INTERFACE_ENTRY(IIndexIterator)
    XCM_END_INTERFACE_MAP()

public:

    ExIndexIterator(ExIndex* index);
    virtual ~ExIndexIterator();

    IIndex* getIndex();
    bool setFirstKey();
    void goFirst();
    void goLast();
    void skip(int delta);
    void setPos(double pct);
    double getPos();
    bool isEof();
    bool isBof();
    void* getKey();
    void* getValue();
    IIndexIterator* clone();

    void getPosition(BlockEntry** block, int* offset);

private:

    ExIndex* m_index;
    BlockEntry* m_levels[30];
    int m_curlevel;
    int m_curoffset;

private:

    ExIndexIterator* m_first_key;

    std::vector<BlockEntry*> m_cache;
    int m_readahead_count;

    void _clearLevels();
    void _clearCache();
    void _loadCache();
    BlockEntry* _getRoot();
    void _descendLeft(BlockEntry* block);
    void _descendRight(BlockEntry* block);
    void _descendPct(BlockEntry* block, double pct);
    void _goPrev();
    bool _goNext();
    void _adjustPosition(ModInfo& mod_info);
    double _ascendGetPos(int level, double pct);
};


class ExIndex : public IIndex
{
friend class ExIndexIterator;

    XCM_CLASS_NAME("xdnative.ExIndex")
    XCM_BEGIN_INTERFACE_MAP(ExIndex)
        XCM_INTERFACE_ENTRY(IIndex)
    XCM_END_INTERFACE_MAP()

public:
    
    ExIndex();
    virtual ~ExIndex();

    bool create(const std::wstring& filename,
                int keylen,
                int vallen,
                bool allow_dups);

    bool open(const std::wstring& filename);
    bool close();
    void setDeleteOnClose(bool value) { m_delete_on_close = value; }

    // -- IIndex implementation --
    std::wstring getFilename();
    IIndexIterator* createIterator();
    unsigned int getKeyCount();
    unsigned int getKeyLength();
    unsigned int getValueLength();
    void startBulkInsert(xd::rowpos_t total_keys);
    int insert(const void* key, int keylen, const void* value, int vallen);
    void cancelBulkInsert();
    void finishBulkInsert(IIndexProgress* progress);
    void flush();
    bool reopen();
    IIndexIterator* seek(void* key, int keylen, bool soft);
    bool remove(IIndexIterator* iter);
    void setTempFilePath(const std::wstring& path);

#ifdef _DEBUG
    int _dumpTree(BlockEntry* curnode, int level);
    void dumpTree();
#endif


private:

    inline unsigned int _getEntryCount(BlockEntry* b)
    {
        return *(b->m_buf);
    }
    inline void _setEntryCount(BlockEntry* b, unsigned char cnt)
    {
        *(b->m_buf) = cnt;
    }
    inline unsigned char* _getEntryPtr(BlockEntry* b, unsigned int entry)
    {
        return b->m_buf+(m_entrylen*entry)+1;
    }
    inline unsigned int _getEntryBranch(BlockEntry* b, unsigned int entry)
    {
        return buf2int(b->m_buf+(m_entrylen*entry)+1);
    }
    inline void _setEntryBranch(BlockEntry* b, unsigned int entry, unsigned int branch)
    {
        int2buf(b->m_buf+(m_entrylen*entry)+1, branch);
    }
    inline unsigned char* _getEntryKey(BlockEntry* b, unsigned int entry)
    {
        return b->m_buf+(m_entrylen*entry)+5;
    }
    inline unsigned char* _getEntryValue(BlockEntry* b, unsigned int entry)
    {
        return b->m_buf+(m_entrylen*entry)+m_keylen+5;
    }

    bool _lookup(BlockEntry* blockptrs[], unsigned int* blockptr_count, void* key, int keylen, bool not_equal, unsigned int* entry_idx);
    void _insert(BlockEntry* blockptrs[], unsigned int blockptr_count, const void* key, const void* value, unsigned int branch, int insert_loc, bool* split_indicator, ModInfo* mod_info);
    bool _remove(BlockEntry* blockptrs[], unsigned int blockptr_count, int key_offset, ModInfo* mod_info);
    bool _sortedInsert(IKeyList* pool, IIndexProgress* progress);
    void _adjustIteratorPositions(ModInfo& mod_info);

    void registerIterator(ExIndexIterator* iter);
    void unregisterIterator(ExIndexIterator* iter);

public:

    void _doHoldCache(BlockEntry* curnode, int level);
    void _holdCache(int levels);
    void _releaseHoldCache();

private:

    BlockFile m_bf;
    bool m_delete_on_close;

    kl::mutex m_obj_mutex;
    kl::mutex m_tree_mutex;

    unsigned int m_keylen;
    unsigned int m_vallen;
    unsigned int m_entrylen;
    bool m_allow_dups;
    BlockEntry* m_root_node;
    std::vector<BlockEntry*> m_holdcache;
    std::wstring m_tempfile_path;
    std::wstring m_filename;

    bool m_bulk_insert;
    xd::rowpos_t m_bulk_total;
    std::wstring m_bulk_filestub;
    int m_bulk_filenum;

    ExKeyPool* m_pool;
    std::vector<std::wstring> m_pool_files;

    std::vector<ExIndexIterator*> m_iters;
};







#endif

