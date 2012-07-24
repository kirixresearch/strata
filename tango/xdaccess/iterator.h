/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-09-03
 *
 */


#ifndef __XDACCESS_ITERATOR_H
#define __XDACCESS_ITERATOR_H


extern "C" {
#include "mdbtools.h"
};


#include "../xdcommon/cmnbaseiterator.h"


struct AccessDataAccessInfo
{
    std::wstring name;
    char* buf;
    int datalen;

    std::string result_str;
    std::wstring result_wstr;

    tango::IColumnInfoPtr colinfo;
};

class AccessIterator : public CommonBaseIterator
{
friend class AccessDatabase;
friend class AccessSet;

    XCM_CLASS_NAME("xdaccess.Iterator")
    XCM_BEGIN_INTERFACE_MAP(AccessIterator)
	    XCM_INTERFACE_ENTRY(tango::IIterator)
    XCM_END_INTERFACE_MAP()

private:
    void clearFieldData();

public:

    AccessIterator();
    ~AccessIterator();
    bool init(const std::wstring& query);

    // -- tango::IIterator interface implementation --

    tango::ISetPtr getSet();
    tango::IDatabasePtr getDatabase();
    tango::IIteratorPtr clone();

    unsigned int getIteratorFlags();

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
    void goRow(const tango::rowid_t& row);

    tango::IStructurePtr getStructure();
    void refreshStructure();
    bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job);

    tango::ISetPtr getChildSet(const std::wstring& relation_tag);

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

    std::vector<AccessDataAccessInfo*> m_columns;

    tango::IDatabasePtr m_database;
    tango::ISetPtr m_set;
    tango::IStructurePtr m_structure;
    std::wstring m_path;
    std::wstring m_tablename;
    MdbHandle* m_mdb;
    MdbTableDef* m_table;

    bool m_eof;
};




#endif









