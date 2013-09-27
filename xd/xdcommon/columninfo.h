/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-08-27
 *
 */


#ifndef __XDCOMMON_COLUMNINFO_H
#define __XDCOMMON_COLUMNINFO_H


class ColumnInfo : public xd::IColumnInfo
{
    XCM_CLASS_NAME("tango.ColumnInfo")
    XCM_BEGIN_INTERFACE_MAP(ColumnInfo)
        XCM_INTERFACE_ENTRY(xd::IColumnInfo)
    XCM_END_INTERFACE_MAP()

public:

    ColumnInfo();
    virtual ~ColumnInfo();

    void setName(const std::wstring& new_val);
    std::wstring getName();

    void setType(int new_val);
    int getType();

    void setWidth(int new_val);
    int getWidth();

    void setScale(int new_val);
    int getScale();

    void setNullsAllowed(bool new_val);
    bool getNullsAllowed();

    void setCalculated(bool new_val);
    bool getCalculated();

    void setExpression(const std::wstring& new_val);
    std::wstring getExpression();

    void setOffset(int new_val);
    int getOffset();

    void setEncoding(int new_val);
    int getEncoding();

    void setColumnOrdinal(int new_val);
    int getColumnOrdinal();

    void setTableOrdinal(int new_val);
    int getTableOrdinal();

    xd::IColumnInfoPtr clone();
    void copyTo(xd::IColumnInfoPtr dest);


private:

    wchar_t m_name[81];
    wchar_t* m_expression;
    int m_type;
    int m_width;
    int m_scale;
    int m_offset;
    int m_col_ord;
    int m_table_ord;
    int m_encoding;
    bool m_calculated;
    bool m_nulls_allowed;
};





#endif


