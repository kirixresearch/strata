/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-05-20
 *
 */


#ifndef H_XDORACLE_SET_H
#define H_XDORACLE_SET_H


#include "../xdcommon/cmnbaseset.h"


struct OracleInsertData
{
    // -- metadata --
    std::wstring m_name;

    int m_ind_offset;
    int m_buf_offset;

    int m_oracle_type;
    int m_oracle_width;
    int m_oracle_scale;

    int m_xd_type;
    int m_xd_width;
    int m_xd_scale;

    // -- bind variables --
    OCIBind* m_bind;
    sb2 m_indicator;
};


class OracleSet
{
friend class OracleIterator;
friend class OracleDatabase;
friend class OracleRowInserter;

    XCM_CLASS_NAME("xdoracle.Set")
    XCM_BEGIN_INTERFACE_MAP(OracleSet)
    XCM_END_INTERFACE_MAP()

public:

    OracleSet(OracleDatabase* database);
    ~OracleSet();

    bool init();

    std::wstring getSetId();

    xd::Structure getStructure();

    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       xd::IJob* job);

    xd::rowpos_t getRowCount();


private:

    OracleDatabase* m_database;

    OCIEnv* m_env;
    OCISvcCtx* m_svc;
    OCIStmt* m_stmt;
    OCIError* m_err;

    std::wstring m_tablename;
    kl::mutex m_object_mutex;
};




class OracleRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdoracle.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(OracleRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    OracleRowInserter(OracleSet* set);
    virtual ~OracleRowInserter();

    xd::objhandle_t getHandle(const std::wstring& column_name);

    bool putRawPtr(xd::objhandle_t column_handle,
                   const unsigned char* value, int length);
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

    bool flushRows();

    OracleSet* m_set;

    OCIEnv* m_env;
    OCIError* m_err;
    OCISvcCtx* m_svc;
    OCIStmt* m_stmt;

    bool m_inserting;
    std::vector<OracleInsertData*> m_fields;

    unsigned char* m_buf;
    int m_row_width;
    int m_cur_buf_row;
};




#endif

