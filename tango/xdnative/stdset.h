/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-09-28
 *
 */


#ifndef __XDNATIVE_STDSET_H
#define __XDNATIVE_STDSET_H


#include "baseset.h"
#include "baseiterator.h"


class IterWrapperSet :  public BaseSet,
                        public ISetEvents
{
    XCM_CLASS_NAME("xdnative.IterWrapperSet")
    XCM_BEGIN_INTERFACE_MAP(IterWrapperSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
        XCM_INTERFACE_ENTRY(ISetInternal)
        XCM_INTERFACE_ENTRY(ISetEvents)
    XCM_END_INTERFACE_MAP()

public:

    IterWrapperSet(tango::IDatabase* database);
    virtual ~IterWrapperSet();

    bool create(tango::ISetPtr base_set, tango::IIteratorPtr iter);

    std::wstring getObjectPath();

    tango::rowpos_t getRowCount();
    tango::IStructurePtr getStructure();

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    bool updateRow(tango::rowid_t rowid,
                   tango::ColumnUpdateInfo* info,
                   size_t info_size);

    // ISetEvents
    void onSetDomainUpdated();
    void onSetStructureUpdated();
    void onSetRelationshipsUpdated();
    void onSetRowUpdated(tango::rowid_t rowid);
    void onSetRowDeleted(tango::rowid_t rowid);

private:

    xcm::mutex m_iterwrapper_mutex;

    tango::ISetPtr m_base_set;
    tango::IIterator* m_iter;
};






class EofIterator : public BaseIterator
{
    XCM_CLASS_NAME("xdnative.EofIterator")
    XCM_BEGIN_INTERFACE_MAP(EofIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
    XCM_END_INTERFACE_MAP()

public:

    EofIterator();
    virtual ~EofIterator();

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
};


class EofSet :  public BaseSet,
                public ISetEvents
{
    XCM_CLASS_NAME("xdnative.EofSet")
    XCM_BEGIN_INTERFACE_MAP(EofSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
        XCM_INTERFACE_ENTRY(ISetInternal)
        XCM_INTERFACE_ENTRY(ISetEvents)
    XCM_END_INTERFACE_MAP()

public:

    EofSet(tango::IDatabase* database);
    virtual ~EofSet();

    bool create(tango::ISetPtr base_set);

    // ISet
    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);
    
    tango::rowpos_t getRowCount();
    tango::IStructurePtr getStructure();

    // ISetEvents
    void onSetDomainUpdated();
    void onSetStructureUpdated();
    void onSetRelationshipsUpdated();
    void onSetRowUpdated(tango::rowid_t rowid);
    void onSetRowDeleted(tango::rowid_t rowid);

private:

    xcm::mutex m_iterwrapper_mutex;
    tango::ISetPtr m_base_set;
};


#endif
