/*!
 *
 * Copyright (c) 2014, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2014-01-16
 *
 */


#ifndef H_XDCOMMON_RELATIONINFO_H
#define H_XDCOMMON_RELATIONINFO_H


class RelationInfo : public xd::IRelation
{
    XCM_CLASS_NAME("xd.RelationInfo")
    XCM_BEGIN_INTERFACE_MAP(RelationInfo)
        XCM_INTERFACE_ENTRY(xd::IRelation)
    XCM_END_INTERFACE_MAP()

public:

    RelationInfo()
    {
    }

    virtual ~RelationInfo()
    {
    }

    void setRelationId(const std::wstring& new_val)
    {
        m_relation_id = new_val;
    }

    const std::wstring& getRelationId()
    {
        return m_relation_id;
    }

    void setTag(const std::wstring& new_val)
    {
        m_tag = new_val;
    }

    const std::wstring& getTag()
    {
        return m_tag;
    }

    void setLeftTable(const std::wstring& new_value)
    {
        m_left_path = new_value;
    }
    
    std::wstring getLeftTable()
    {
        return m_left_path;
    }

    void setLeftExpression(const std::wstring& new_value)
    {
        m_left_expression = new_value;
    }

    std::wstring getLeftExpression()
    {
        return m_left_expression;
    }

    void setRightTable(const std::wstring& new_value)
    {
        m_right_path = new_value;
    }

    std::wstring getRightTable()
    {
        return m_right_path;
    }

    void setRightExpression(const std::wstring& new_value)
    {
        m_right_expression = new_value;
    }

    std::wstring getRightExpression()
    {
        return m_right_expression;
    }

private:

    std::wstring m_relation_id;
    std::wstring m_tag;

    std::wstring m_left_path;
    std::wstring m_right_path;
    std::wstring m_left_expression;
    std::wstring m_right_expression;
};



#endif

