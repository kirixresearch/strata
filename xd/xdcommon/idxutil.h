/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-27
 *
 */


#ifndef __XDCOMMON_IDXUTIL_H
#define __XDCOMMON_IDXUTIL_H


#include "tango_private.h"
#include "cmnbaseiterator.h"


IIndex* createExternalIndex(xd::IDatabasePtr db,
                            const std::wstring& table_path,
                            const std::wstring& index_filename,
                            const std::wstring& tempfile_path,
                            const std::wstring& expr,
                            bool allow_dups,
                            xd::IJob* job);
                            
xd::IIteratorPtr createIteratorFromIndex(xd::IIteratorPtr data_iter,
                                            IIndex* idx,
                                            const std::wstring& columns,
                                            const std::wstring& order,
                                            const std::wstring& table = L"");



// seekRow() seeks a key in an index, and then
// scrolls down to the specific rowid on the value side

IIndexIterator* seekRow(IIndex* idx,
                        const unsigned char* key,
                        int key_len,
                        xd::rowid_t rowid);


class KeyLayout;

class CommonIndexIterator : public xd::IIterator,
                            public xd::IIteratorRelation,
                            public IIteratorKeyAccess
{
    XCM_CLASS_NAME("xdcommon.CommonIndexIterator")
    XCM_BEGIN_INTERFACE_MAP(CommonIndexIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_ENTRY(xd::IIteratorRelation)
        XCM_INTERFACE_ENTRY(IIteratorKeyAccess)
    XCM_END_INTERFACE_MAP()

public:

    CommonIndexIterator();
    virtual ~CommonIndexIterator();

    bool init(xd::IIterator* data_iter,
              IIndexIterator* idx_index,
              const std::wstring& order,
              bool value_side = true);

    void setTable(const std::wstring& path) { m_table = path; }

    void* getKey();
    int getKeyLength();
    bool setKeyFilter(const void* key, int len);
    void getKeyFilter(const void** key, int* len);
    bool setFirstKey();

    
    xd::IIteratorPtr clone();
    void skip(int delta);
    void goFirst();
    void goLast();
    xd::rowid_t getRowId();
    bool bof();
    bool eof();
    bool seek(const unsigned char* key, int length, bool soft);
    bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft);
    bool setPos(double pct);
    double getPos();
    void goRow(const xd::rowid_t& rowid);
    
    std::wstring getTable();
    xd::rowpos_t getRowCount();

    void setIteratorFlags(unsigned int mask, unsigned int value);
    unsigned int getIteratorFlags();

    void refreshStructure();
    xd::IStructurePtr getStructure();
    bool modifyStructure(xd::IStructure* struct_config, xd::IJob* job);

    xd::IIteratorPtr getChildIterator(xd::IRelationPtr relation);
    xd::IIteratorPtr getFilteredChildIterator(xd::IRelationPtr relation);

    xd::objhandle_t getHandle(const std::wstring& expr);
    xd::IColumnInfoPtr getInfo(xd::objhandle_t data_handle);
    int getType(xd::objhandle_t data_handle);
    bool releaseHandle(xd::objhandle_t data_handle);

    const unsigned char* getRawPtr(xd::objhandle_t data_handle);
    int getRawWidth(xd::objhandle_t data_handle);
    const std::string& getString(xd::objhandle_t data_handle);
    const std::wstring& getWideString(xd::objhandle_t data_handle);
    xd::datetime_t getDateTime(xd::objhandle_t data_handle);
    double getDouble(xd::objhandle_t data_handle);
    int getInteger(xd::objhandle_t data_handle);
    bool getBoolean(xd::objhandle_t data_handle);
    bool isNull(xd::objhandle_t data_handle);

public:

    void onRowUpdated(xd::rowid_t rowid);
    void onRowDeleted(xd::rowid_t rowid);

protected:

    void updatePos();
    bool movePrev();
    bool moveNext();

protected:

    xcm::IObjectPtr m_refobj;
    std::wstring m_table;
    xd::IIterator* m_data_iter;
    IIndexIterator* m_idx_iter;
    
    std::wstring m_order;
    KeyLayout* m_layout;
    
    bool m_value_side;
    xd::rowid_t m_rowid;
    unsigned int m_keylen;
    bool m_row_deleted;

    unsigned char* m_key_filter;
    int m_key_filter_len;
};




#endif



