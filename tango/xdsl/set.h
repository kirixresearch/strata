/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-09-03
 *
 */


#ifndef __XDSL_SET_H
#define __XDSL_SET_H


class SlSet : public tango::ISet,
              public tango::IStorable
{

friend class SlDatabase;
friend class SlRowInserter;

    XCM_CLASS_NAME("xdsl.Set")
    XCM_BEGIN_INTERFACE_MAP(SlSet)
	    XCM_INTERFACE_ENTRY(tango::ISet)
        XCM_INTERFACE_ENTRY(tango::IStorable)
    XCM_END_INTERFACE_MAP()

public:

    SlSet();
    ~SlSet();
    
    bool init();

    // -- tango::ISet interface --

    unsigned int getSetFlags();
    std::wstring getSetId();

    tango::IStructurePtr getStructure();
    bool modifyStructure(tango::IStructure* struct_config,
                         tango::IJob* job);

    tango::IRowInserterPtr getRowInserter();
    tango::IRowDeleterPtr getRowDeleter();

    int insert(tango::IIteratorPtr source_iter,
               tango::ISetPtr constraint,
               int max_rows,
               tango::IJob* job);

    int update(tango::ISetPtr constraint,
               const std::wstring& params,
               tango::IJob* job);

    int remove(tango::ISetPtr constraint,
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
    tango::IRelationPtr SlSet::createRelation(const std::wstring& tag,
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
    tango::IIteratorPtr createBookmark(tango::rowid_t rowid);

    bool rowInSet(tango::rowid_t rowid);
    tango::rowpos_t getRowCount();

    // -- tango::IStorable --

    std::wstring getObjectPath();
    bool isTemporary();
    bool storeObject(const std::wstring& ofs_path);

private:

    xcm::mutex m_object_mutex;
    tango::IStructurePtr m_structure;

    tango::IDatabasePtr m_database;
    SlDatabase* m_dbint;
    
    sqlite3* m_db;

    std::wstring m_tablename;
    tango::tableord_t m_ordinal;
    int m_row_count;
};



class SlRowInserterData
{
public:

    std::wstring name;
    int idx;
    int type;
    int length;
    char* str_data;
    tango::IColumnInfoPtr colinfo;
};


class SlRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdsl.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(SlRowInserter)
	    XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()

public:

    SlRowInserter(SlSet* set);
    ~SlRowInserter();

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

    SlSet* m_set;
    sqlite3_stmt* m_stmt;
    bool m_inserting;

    std::vector<SlRowInserterData> m_fields;
};




#endif



