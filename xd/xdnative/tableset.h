/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-03-15
 *
 */


#ifndef H_XDNATIVE_TABLESET_H
#define H_XDNATIVE_TABLESET_H



#include "baseset.h"
#include "baseiterator.h"
#include "../xdcommon/xd_private.h"


xcm_interface ITable;
class BufIterator;
class KeyLayout;


class IndexEntry
{
public:

    std::wstring filename;
    std::wstring name;
    std::wstring expr;

    std::vector<bool> active_columns;
    IIndex* index;
    int key_length;
    KeyLayout* key_expr;
    unsigned char* orig_key;
    bool update;
};



class TableSet;
class TableIterator : public BaseIterator
{
    friend class TableSet;

    XCM_CLASS_NAME("xdnative.TableIterator")
    XCM_BEGIN_INTERFACE_MAP(TableIterator)
        XCM_INTERFACE_CHAIN(BaseIterator)
    XCM_END_INTERFACE_MAP()

public:

    TableIterator();
    virtual ~TableIterator();

    bool init(XdnativeDatabase* database,
              TableSet* set,
              ITable* table,
              const std::wstring& columns);

    std::wstring getTable();

    void setIteratorFlags(unsigned int mask, unsigned int value);

    const unsigned char* getRowBuffer();
    int getRowBufferWidth();

    xd::IIteratorPtr clone();
    void skip(int delta);
    void goFirst();
    void goLast();
    bool bof();
    bool eof();
    bool seek(const unsigned char* key, int length, bool soft);
    bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft);
    bool setPos(double pct);
    double getPos();

    void flushRow();
    xd::rowpos_t getRowNumber();

    void onSetRowUpdated(xd::rowid_t rowid);
    void onSetStructureUpdated();

private:

    void updatePosition();

private:

    xd::tableord_t m_table_ord;
    xd::rowpos_t m_row_count;
    TableSet* m_table_set;
    ITable* m_table;
    int m_table_rowwidth;
    int m_read_ahead_rowcount;
    unsigned char* m_buf;
    xd::rowpos_t* m_rowpos_buf;
    int m_buf_rowcount;
    int m_buf_pos;
    xd::rowpos_t m_row_num;    // sequential row number for ROWNUM()
    bool m_eof;
    bool m_bof;
    bool m_include_deleted;
};


class NativeTable;
class TableSet : public BaseSet,
                 public ITableEvents
{
friend class TableSetRowInserter;
friend class TableSetRowDeleter;
friend class TableIterator;

    XCM_CLASS_NAME_CUSTOMREFCOUNT("xdnative.TableSet")
    XCM_BEGIN_INTERFACE_MAP(TableSet)
        XCM_INTERFACE_ENTRY(IXdnativeSet)
        XCM_INTERFACE_ENTRY(ITableEvents)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()


    virtual void ref()
    {
        m_database->lockObjectRegistryMutex();
        m_refcount_holder.xcm_ref_count++;
        m_database->unlockObjectRegistryMutex();
    }
    
    virtual void unref()
    {
        m_database->lockObjectRegistryMutex();
        if (--m_refcount_holder.xcm_ref_count == 0)
        {
            XdnativeDatabase* db = m_database;
            db->ref();

            db->unregisterSet(this);
            delete this;
            db->unlockObjectRegistryMutex();

            db->unref();
            return;
        }
        m_database->unlockObjectRegistryMutex();
    }
    
    virtual int get_ref_count()
    {
        return 10;
    }
    

public:

    TableSet(XdnativeDatabase* database);
    virtual ~TableSet();

    bool create(const xd::FormatDefinition& format_definition, const std::wstring& path);
    bool load(INodeValuePtr file);
    bool save();

    void updateRowCount();

    xd::Structure getStructure();
    bool modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job);

    xd::IndexInfoEnum getIndexEnum();

    xd::IndexInfo createIndex(const std::wstring& tag,
                              const std::wstring& expr,
                              xd::IJob* job);
    bool deleteIndex(const std::wstring& name);
    bool renameIndex(const std::wstring& name,
                     const std::wstring& new_name);

    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& order,
                                       xd::IJob* job);

    xd::IRowInserterPtr getRowInserter();
    IXdsqlRowDeleterPtr getRowDeleter();
    bool restoreDeleted();
    
    xd::rowpos_t getRowCount();

    bool updateRow(xd::rowid_t rowid,
                   xd::ColumnUpdateInfo* info,
                   size_t info_size);

    unsigned long long getStructureModifyTime();

    // ITableEvents
    void onTableRowUpdated(xd::rowid_t rowid);
    void onTableRowDeleted(xd::rowid_t rowid);
    void onTableRowCountUpdated();

private:

    bool loadTable(const std::wstring& tbl_filename);
    bool prepareIndexEntry(IndexEntry& e);
    void refreshIndexEntries();
    void refreshUpdateBuffer();
    bool deleteAllIndexes();
    void onOfsPathChanged(const std::wstring& new_path);
    TableIterator* createPhysicalIterator(const std::wstring& columns,
                                          bool include_deleted);
    bool deleteIndexInternal(IIndex* idx);

private:

    xd::tableord_t m_ordinal;
    std::wstring m_ofspath;
    xd::rowpos_t m_row_count;
    xd::rowpos_t m_deleted_row_count;
    xd::rowpos_t m_phys_row_count;
    unsigned long long m_idxrefresh_time;
    NativeTable* m_table;
    xd::Structure m_structure;

    kl::mutex m_update_mutex;
    std::vector<IndexEntry> m_indexes;
    unsigned char* m_update_buf;
    BufIterator* m_update_iter;
};



class RowIdArray;
class NativeRowDeleter;
class TableSetRowDeleter : public IXdsqlRowDeleter
{
    XCM_CLASS_NAME("xdnative.TableSetRowDeleter")
    XCM_BEGIN_INTERFACE_MAP(TableSetRowDeleter)
        XCM_INTERFACE_ENTRY(IXdsqlRowDeleter)
    XCM_END_INTERFACE_MAP()

public:

    TableSetRowDeleter(XdnativeDatabase* db, TableSet* set);
    virtual ~TableSetRowDeleter();

    void startDelete();
    bool deleteRow(const xd::rowid_t& rowid);
    void finishDelete();
    void cancelDelete();

private:

    bool doRowDelete(xd::rowid_t rowid);

    NativeRowDeleter* m_table_row_deleter;
    TableSet* m_set;
    RowIdArray* m_rowid_array;
};



class TableSetRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdnative.TableSetRowInserter")
    XCM_BEGIN_INTERFACE_MAP(TableSetRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    TableSetRowInserter(XdnativeDatabase* db, TableSet* set);
    virtual ~TableSetRowInserter();

    xd::objhandle_t getHandle(const std::wstring& column_name);

    bool putRawPtr(xd::objhandle_t column_handle,
                   const unsigned char* value,
                   int length);

    bool putString(xd::objhandle_t column_handle,
                   const std::string& value);

    bool putWideString(xd::objhandle_t column_handle,
                   const std::wstring& value);

    bool putDouble(xd::objhandle_t column_handle,
                   double value);

    bool putInteger(xd::objhandle_t column_handle,
                   int value);

    bool putBoolean(xd::objhandle_t column_handle,
                   bool value);

    bool putDateTime(xd::objhandle_t column_handle,
                   xd::datetime_t datetime);

    bool putNull(xd::objhandle_t column_handle);

    bool startInsert(const std::wstring& col_list);
    bool insertRow();
    void finishInsert();

    bool flush();

private:

    XdnativeDatabase* m_database;
    TableSet* m_set;
    ITable* m_table;
    BufIterator* m_iter;
    unsigned char* m_buf;
    int m_buf_row;
    int m_row_width;
    bool m_valid;
    std::wstring m_tbl_filename;
};


#endif

