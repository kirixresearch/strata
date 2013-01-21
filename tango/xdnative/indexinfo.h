/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-03-16
 *
 */


#ifndef __XDNATIVE_INDEXINFO_H
#define __XDNATIVE_INDEXINFO_H


class IndexInfo : public tango::IIndexInfo
{
    XCM_CLASS_NAME("xdnative.IndexInfo")
    XCM_BEGIN_INTERFACE_MAP(IndexInfo)
        XCM_INTERFACE_ENTRY(tango::IIndexInfo)
    XCM_END_INTERFACE_MAP()

    std::wstring m_tag;
    std::wstring m_expression;

public:

    IndexInfo();
    virtual ~IndexInfo();

    void setTag(const std::wstring& new_val);
    const std::wstring& getTag();

    void setExpression(const std::wstring& new_val);
    const std::wstring& getExpression();
};





#endif

