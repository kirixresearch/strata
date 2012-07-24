/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
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


IIndex* createExternalIndex(tango::ISetPtr set,
                            const std::wstring& index_filename,
                            const std::wstring& tempfile_path,
                            const std::wstring& expr,
                            bool allow_dups,
                            tango::IJob* job);
                            
tango::IIteratorPtr createIteratorFromIndex(tango::ISetPtr set,
                                            IIndex* idx,
                                            const std::wstring& columns,
                                            const std::wstring& order);



// seekRow() seeks a key in an index, and then
// scrolls down to the specific rowid on the value side

IIndexIterator* seekRow(IIndex* idx,
                        const unsigned char* key,
                        int key_len,
                        tango::rowid_t rowid);


class KeyLayout;

class CommonIndexIterator : public tango::IIterator,
                            public IIteratorKeyAccess,
                            public tango::IIteratorRelation,
                            public ISetEvents
{
    XCM_CLASS_NAME("xdcommon.CommonIndexIterator")
    XCM_BEGIN_INTERFACE_MAP(CommonIndexIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
        XCM_INTERFACE_ENTRY(IIteratorKeyAccess)
        XCM_INTERFACE_ENTRY(tango::IIteratorRelation)
        XCM_INTERFACE_ENTRY(ISetEvents)
    XCM_END_INTERFACE_MAP()

public:

    CommonIndexIterator(tango::ISet* set,
                        tango::IIterator* data_iter,
                        IIndexIterator* idx_index,
                        const std::wstring& order,
                        bool value_side = true);

    virtual ~CommonIndexIterator();

    void* getKey();
    int getKeyLength();
    bool setKeyFilter(const void* key, int len);
    void getKeyFilter(const void** key, int* len);
    bool setFirstKey();

    
    tango::IIteratorPtr clone();
    void skip(int delta);
    void goFirst();
    void goLast();
    tango::rowid_t getRowId();
    bool bof();
    bool eof();
    bool seek(const unsigned char* key, int length, bool soft);
    bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft);
    bool setPos(double pct);
    double getPos();
    void goRow(const tango::rowid_t& rowid);
    
    void onSetRowUpdated(tango::rowid_t rowid);
    void onSetRowDeleted(tango::rowid_t rowid);
    void onSetDomainUpdatexd() { }
    void onSetStructureUpdated() { }
    void onSetRelationshipsUpdated() { }
    void onSetDomainUpdated() { }

    // -- tango::IIterator stuff (passed through to data_iter) --
    tango::ISetPtr getSet();
    //tango::IDatabasePtr getDatabase();

    void setIteratorFlags(unsigned int mask, unsigned int value);
    unsigned int getIteratorFlags();

    void refreshStructure();
    tango::IStructurePtr getStructure();
    bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job);

    tango::ISetPtr getChildSet(const std::wstring& relation_tag);
    tango::IIteratorPtr getChildIterator(const std::wstring& relation_tag);

    tango::objhandle_t getHandle(const std::wstring& expr);
    tango::IColumnInfoPtr getInfo(tango::objhandle_t data_handle);
    int getType(tango::objhandle_t data_handle);
    bool releaseHandle(tango::objhandle_t data_handle);

    const unsigned char* getRawPtr(tango::objhandle_t data_handle);
    int getRawWidth(tango::objhandle_t data_handle);
    const std::string& getString(tango::objhandle_t data_handle);
    const std::wstring& getWideString(tango::objhandle_t data_handle);
    tango::datetime_t getDateTime(tango::objhandle_t data_handle);
    double getDouble(tango::objhandle_t data_handle);
    int getInteger(tango::objhandle_t data_handle);
    bool getBoolean(tango::objhandle_t data_handle);
    bool isNull(tango::objhandle_t data_handle);

private:

    void updatePos();
    bool movePrev();
    bool moveNext();

private:

    tango::ISet* m_set;
    tango::IIterator* m_data_iter;
    IIndexIterator* m_idx_iter;
    
    std::wstring m_order;
    KeyLayout* m_layout;
    
    bool m_value_side;
    tango::rowid_t m_rowid;
    unsigned int m_keylen;
    bool m_row_deleted;

    unsigned char* m_key_filter;
    int m_key_filter_len;
};




#endif



