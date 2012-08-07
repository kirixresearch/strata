/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */


#ifndef __XDCLIENT_SET_H
#define __XDCLIENT_SET_H


#include <xcm/xcm.h>
#include "../xdcommon/cmnbaseset.h"


class ClientSet : public CommonBaseSet
{

friend class ClientDatabase;
friend class ClientIterator;
friend class ClientRowInserter;

    XCM_CLASS_NAME("xdclient.Set")
    XCM_BEGIN_INTERFACE_MAP(ClientSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
    XCM_END_INTERFACE_MAP()

public:

    ClientSet(ClientDatabase* database);
    virtual ~ClientSet();
    
    bool init(const std::wstring& path);
    
    // tango::ISet interface
    
    void setObjectPath(const std::wstring& path);
    std::wstring getObjectPath();
    bool isTemporary();
    bool storeObject(const std::wstring& path);
    
    unsigned int getSetFlags();
    std::wstring getSetId();

    tango::IStructurePtr getStructure();
    bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job);

    tango::IRowInserterPtr getRowInserter();
    tango::IRowDeleterPtr getRowDeleter();

    int insert(tango::IIteratorPtr source_iter,
               const std::wstring& where_condition,
               int max_rows,
               tango::IJob* job);

    tango::IIndexInfoEnumPtr getIndexEnum();
    tango::IIndexInfoPtr createIndex(const std::wstring& tag,
                                     const std::wstring& expr,
                                     tango::IJob* job);
                                     
    tango::IIndexInfoPtr lookupIndex(const std::wstring& expr,
                                     bool exact_column_order);
                                     
    bool deleteIndex(const std::wstring& name);
    bool renameIndex(const std::wstring& name,
                     const std::wstring& new_name);

    tango::IRelationEnumPtr getRelationEnum();
    tango::IRelationPtr createRelation(const std::wstring& tag,
                                       const std::wstring& right_set_path,
                                       const std::wstring& left_expr,
                                       const std::wstring& right_expr);
    tango::IRelationPtr getRelation(const std::wstring& tag);
    int getRelationCount();
    bool deleteRelation(const std::wstring& tag);
    bool deleteAllRelations();

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();
    
private:
    
    ClientDatabase* m_database;
    std::wstring m_tablename;
    std::wstring m_path;         // server object path
    std::wstring m_object_path;  // logical object path (e.g. mounts)
    int m_set_flags;
    tango::rowpos_t m_known_row_count;
    tango::IStructurePtr m_structure;
};


class ClientInsertData
{
public:
    std::wstring m_col_name;
    int m_tango_type;
    int m_tango_width;
    int m_tango_scale;

    std::wstring m_text;
};


class ClientRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdclient.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(ClientRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()

public:

    ClientRowInserter(ClientSet* set);
    ~ClientRowInserter();

    tango::objhandle_t getHandle(const std::wstring& column_name);
    tango::IColumnInfoPtr getInfo(tango::objhandle_t column_handle);

    bool putRawPtr(tango::objhandle_t column_handle, const unsigned char* value, int length);
    bool putString(tango::objhandle_t column_handle, const std::string& value);
    bool putWideString(tango::objhandle_t column_handle, const std::wstring& value);
    bool putDouble(tango::objhandle_t column_handle, double value);
    bool putInteger(tango::objhandle_t column_handle, int value);
    bool putBoolean(tango::objhandle_t column_handle, bool value);
    bool putDateTime(tango::objhandle_t column_handle, tango::datetime_t datetime);
    bool putNull(tango::objhandle_t column_handle);

    bool startInsert(const std::wstring& col_list);
    bool insertRow();
    void finishInsert();

    bool flush();

private:

};


#endif

