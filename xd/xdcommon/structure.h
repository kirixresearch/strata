/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-02-14
 *
 */


#ifndef __XDCOMMON_STRUCTURE_H
#define __XDCOMMON_STRUCTURE_H


#include <map>
#include <kl/string.h>


#ifdef _MSC_VER
#ifndef wcscasecmp
#define wcscasecmp wcsicmp
#endif
#endif


struct StructureAction
{
    enum
    {
        actionCreate = 0,
        actionModify = 1,
        actionDelete = 2,
        actionMove   = 3,
        actionInsert = 4
    };

    int m_action;
    std::wstring m_colname;
    xd::ColumnInfo m_params;
    int m_pos; // for actionMove and actionInsert only;
};


bool calcfieldsModifyStructure(std::vector<StructureAction>& actions,
                               xd::IStructurePtr _mod_struct,
                               std::vector<xd::ColumnInfo>* calc_fields,
                               bool* done_flag);



xcm_interface IStructureInternal : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IStructureInternal")

public:

    virtual std::vector<StructureAction>& getStructureActions() = 0;
    virtual void addColumn(const xd::ColumnInfo& col) = 0;
    virtual bool internalInsertColumn(const xd::ColumnInfo& col, int insert_idx) = 0;
    virtual bool internalMoveColumn(const std::wstring& column_name, int new_idx) = 0;
    virtual bool removeColumn(const std::wstring& name) = 0;
    virtual bool modifyColumn(const std::wstring& column_name,
                              const std::wstring& name,
                              int type,
                              int width,
                              int scale,
                              const std::wstring& expr,
                              int offset,
                              int encoding,
                              int ordinal) = 0;
};

XCM_DECLARE_SMARTPTR(IStructureInternal)



class Structure : public xd::IStructure,
                  public IStructureInternal
{
    XCM_CLASS_NAME("xd.Structure")
    XCM_BEGIN_INTERFACE_MAP(Structure)
        XCM_INTERFACE_ENTRY(xd::IStructure)
        XCM_INTERFACE_ENTRY(IStructureInternal)
    XCM_END_INTERFACE_MAP()

public:

    Structure();
    virtual ~Structure();

    Structure* internalClone();

    // -- IStructureInternal --
    std::vector<StructureAction>& getStructureActions();
    void addColumn(const xd::ColumnInfo& col);
    bool internalInsertColumn(const xd::ColumnInfo& col, int insert_idx);
    bool internalMoveColumn(const std::wstring& column_name, int new_idx);
    bool removeColumn(const std::wstring& column_name);
    bool modifyColumn(const std::wstring& column_name,
                      const std::wstring& name,
                      int type,
                      int width,
                      int scale,
                      const std::wstring& expr,
                      int offset,
                      int encoding,
                      int ordinal);

    // -- IStructure --
    xd::IStructurePtr clone();
    int getColumnCount();
    std::wstring getColumnName(int idx);
    const xd::ColumnInfo& getColumnInfoByIdx(int idx);
    const xd::ColumnInfo& getColumnInfo(const std::wstring& column_name);
    bool getColumnExist(const std::wstring& column_name);

    bool deleteColumn(const std::wstring& column_name);
    bool moveColumn(const std::wstring& column_name, int new_idx);
    xd::IColumnInfoPtr modifyColumn(const std::wstring& column_name);
    void createColumn(const xd::ColumnInfo& col);

    int getExprType(const std::wstring& expression);
    bool isValid();

private:

    std::vector<xd::ColumnInfo> m_cols;
    std::vector<StructureAction> m_actions;
    std::map<std::wstring, int, kl::cmp_nocase> m_map;

    int getColumnIdx(const std::wstring& name);

    xd::ColumnInfo m_ret;
};




#endif


