/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#ifndef H_XDNATIVE_NATIVETBL_H
#define H_XDNATIVE_NATIVETBL_H

#include <kl/file.h>
#include "xdnative_private.h"
#include "database.h"


const int native_header_len = 1024;
const int native_column_descriptor_len = 256;

// in general these constants should be the same as
// xd::max_numeric_width and xd::max_numeric_scale

const int native_max_numeric_width = 20;
const int native_max_numeric_scale = 12;


class BitmapFile;
class BitmapFileScroller;


char convertType_xd2native(int xd_type);
int convertType_native2xd(int native_type);


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
    static bool create(const std::wstring& filename, const xd::FormatDefinition& structure);

public:
    
    NativeTable(XdnativeDatabase* database);
    virtual ~NativeTable();

    // ITable
    bool addEventHandler(ITableEvents* handler);
    bool removeEventHandler(ITableEvents* handler);
    
    bool open(const std::wstring& filename,
              xd::tableord_t ordinal);

    bool reopen(bool exclusive);

    void close();

    std::wstring getFilename();
    std::wstring getMapFilename();

    int getRowWidth();
    xd::tableord_t getTableOrdinal();
    xd::rowpos_t getRowCount(xd::rowpos_t* deleted_row_count);
    xd::Structure getStructure();

    xd::rowpos_t findNextRowPos(xd::rowpos_t offset, int delta);
    bool getRow(xd::rowpos_t row, unsigned char* buf);
    int getRows(unsigned char* buf,
                xd::rowpos_t* rowpos_arr,
                int skip,
                xd::rowpos_t start_row,
                int row_count,
                bool direction,
                bool include_deleted);
    int appendRows(unsigned char* buf, int row_count);

    bool isRowDeleted(xd::rowpos_t row);
    bool writeRow(xd::rowpos_t row, unsigned char* buf);
    bool writeColumnInfo(int col,
                         const std::wstring& col_name,
                         int type,
                         int width,
                         int scale);

    xd::rowpos_t setDeletedRowCount(int deleted_row_count);
    void recalcPhysRowCount();

    bool restoreDeleted();

    unsigned long long getStructureModifyTime();
    bool setStructureModified();

private:

    xd::rowpos_t _findNextRowPos(BitmapFileScroller* _bfs,
                                    xd::rowpos_t offset,
                                    int delta);

    bool upgradeVersion1(unsigned char* header);

private:
    kl::mutex m_object_mutex;

    XdnativeDatabase* m_database;

    xd::Structure m_structure;
    xf_file_t m_file;
    BitmapFile* m_map_file;

    xd::tableord_t m_ordinal;
    std::wstring m_filename;
    std::wstring m_map_filename;

    std::vector<ITableEvents*> m_event_handlers;

    xd::rowpos_t m_phys_row_count;     // number of rows in this table
    unsigned int m_row_width;             // row width
    unsigned int m_data_offset;           // offset where the data rows begin
    long long m_modified_time; // time that an update has occurred
};




class NativeRowDeleter
{

public:

    NativeRowDeleter(NativeTable* table);
    virtual ~NativeRowDeleter();

    void startDelete();
    bool deleteRow(const xd::rowid_t& rowid);
    void finishDelete();
    void cancelDelete() { }

public:

    NativeTable* m_table;
    BitmapFileScroller* m_map_scroller;
    bool m_started;
    int m_deleted_count;
};




#endif
