/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#ifndef __XDNATIVE_NATIVETBL_H
#define __XDNATIVE_NATIVETBL_H


#include "xdnative_private.h"


const int native_header_len = 1024;
const int native_column_descriptor_len = 256;

// in general these constants should be the same as
// tango::max_numeric_width and tango::max_numeric_scale

const int native_max_numeric_width = 20;
const int native_max_numeric_scale = 12;


class BitmapFile;
class BitmapFileScroller;


char convertType_tango2native(int tango_type);
int convertType_native2tango(int native_type);


class NativeTable : public ITable
{
    friend class NativeRowDeleter;
    friend class NativeRowInserter;

    XCM_CLASS_NAME_CUSTOMREFCOUNT("xdnative.NativeTable")
    XCM_BEGIN_INTERFACE_MAP(NativeTable)
        XCM_INTERFACE_ENTRY(ITable)
    XCM_END_INTERFACE_MAP()


    virtual void ref()
    {
        m_database_internal->lockObjectRegistryMutex();
        m_refcount_holder.xcm_ref_count++;
        m_database_internal->unlockObjectRegistryMutex();
    }
    
    virtual void unref()
    {
        m_database_internal->lockObjectRegistryMutex();
        if (--m_refcount_holder.xcm_ref_count == 0)
        {
            IDatabaseInternalPtr dbi = m_database_internal;
            delete this;
            dbi->unlockObjectRegistryMutex();
            return;
        }
        m_database_internal->unlockObjectRegistryMutex();
    }
    
    virtual int get_ref_count()
    {
        return 10;
    }
    
    
public:
    static bool create(const std::wstring& filename, tango::IStructure* structure);

public:
    
    NativeTable(tango::IDatabase* database);
    virtual ~NativeTable();

    // -- ITable --
    bool addEventHandler(ITableEvents* handler);
    bool removeEventHandler(ITableEvents* handler);
    
    bool open(const std::wstring& filename,
              tango::tableord_t ordinal);

    bool reopen(bool exclusive);

    void close();

    std::wstring getFilename();
    std::wstring getMapFilename();

    int getRowWidth();
    tango::tableord_t getTableOrdinal();
    tango::rowpos_t getRowCount(tango::rowpos_t* deleted_row_count);
    tango::IStructurePtr getStructure();

    tango::rowpos_t findNextRowPos(tango::rowpos_t offset, int delta);
    bool getRow(tango::rowpos_t row, unsigned char* buf);
    int getRows(unsigned char* buf,
                tango::rowpos_t* rowpos_arr,
                int skip,
                tango::rowpos_t start_row,
                int row_count,
                bool direction,
                bool include_deleted);
    int appendRows(unsigned char* buf, int row_count);

    bool isRowDeleted(tango::rowpos_t row);
    bool writeRow(tango::rowpos_t row, unsigned char* buf);
    bool writeColumnInfo(int col,
                         const std::wstring& col_name,
                         int type,
                         int width,
                         int scale);

    tango::rowpos_t setDeletedRowCount(int deleted_row_count);
    void recalcPhysRowCount();

    tango::IRowDeleterPtr getRowDeleter();
    bool restoreDeleted();

    unsigned long long getStructureModifyTime();
    bool setStructureModified();

private:

    tango::rowpos_t _findNextRowPos(BitmapFileScroller* _bfs,
                                    tango::rowpos_t offset,
                                    int delta);

    bool upgradeVersion1(unsigned char* header);

private:
    xcm::mutex m_object_mutex;

    IDatabaseInternalPtr m_database_internal;

    tango::IStructurePtr m_structure;
    xf_file_t m_file;
    BitmapFile* m_map_file;

    tango::tableord_t m_ordinal;
    std::wstring m_filename;
    std::wstring m_map_filename;

    std::vector<ITableEvents*> m_event_handlers;

    tango::rowpos_t m_phys_row_count;     // number of rows in this table
    unsigned int m_row_width;             // row width
    unsigned int m_data_offset;           // offset where the data rows begin
    long long m_modified_time; // time that an update has occurred
};




class NativeRowDeleter : public tango::IRowDeleter
{
    XCM_CLASS_NAME("xdnative.NativeRowDeleter")
    XCM_BEGIN_INTERFACE_MAP(NativeRowDeleter)
        XCM_INTERFACE_ENTRY(tango::IRowDeleter)
    XCM_END_INTERFACE_MAP()

public:

    NativeRowDeleter(tango::IDatabase* database, NativeTable* table);
    virtual ~NativeRowDeleter();

    void startDelete();
    bool deleteRow(const tango::rowid_t& rowid);
    void finishDelete();
    void cancelDelete() { }

public:

    tango::IDatabasePtr m_database;
    NativeTable* m_table;
    BitmapFileScroller* m_map_scroller;
    bool m_started;
    int m_deleted_count;
};




#endif
