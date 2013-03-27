/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-04-13
 *
 */


#ifndef __APP_QUERYTEMPLATE_H
#define __APP_QUERYTEMPLATE_H


enum
{
    QueryJoinNone = 0,
    QueryJoinInner,
    QueryJoinLeftOuter,
    QueryJoinRightOuter,
    QueryJoinFullOuter
};


enum
{
    QueryGroupFunction_None = 0,
    QueryGroupFunction_GroupBy = 1,
    QueryGroupFunction_First = 2,
    QueryGroupFunction_Last = 3,
    QueryGroupFunction_Min = 4,
    QueryGroupFunction_Max = 5,
    QueryGroupFunction_Sum = 6,
    QueryGroupFunction_Avg = 7,
    QueryGroupFunction_Count = 8,
    QueryGroupFunction_Stddev = 9,
    QueryGroupFunction_Variance = 10,
    QueryGroupFunction_GroupID = 11
};


struct QueryBuilderParam
{
    bool output;
    wxString input_expr;
    wxString output_field;
    int group_func;
    int sort_order;
    std::vector<wxString> conditions;
};


struct QueryJoin
{
    int join_type;
    wxString right_path;
    wxString left_columns;
    wxString right_columns;
};


struct QueryBuilderSourceTable
{
    wxString alias;
    wxString path;
    std::vector<QueryJoin> joins;
    tango::IStructurePtr structure;
    int x, y, width, height;
};


class QueryTemplate
{
public:

    QueryTemplate();
    virtual ~QueryTemplate();

    bool save(const wxString& path, bool refresh_tree_if_necessary = true);
    bool load(const wxString& path);
    jobs::IJobPtr execute(int target_site_id = 0);
    
    std::vector<wxString> getOutputFields();
    wxString getQueryString();
    int getDatabaseDisposition() { return m_disposition; }
    
public:

    QueryBuilderSourceTable* lookupTableByPath(const wxString& path);
    QueryBuilderSourceTable* lookupTableByAlias(const wxString& alias);
    tango::IColumnInfoPtr lookupColumnInfo(const wxString& input);
    wxString buildJoinString(QueryBuilderSourceTable& tbl);
    wxString stripAllAliases(const wxString& input);
    wxString completeFilter(const wxString& _expr,
                            const wxString& input,
                            int group_func = 0);
    void updateValidationStructure();

private:

    // functions for saving/loading the query template
    bool saveJson(const wxString& path);
    bool loadJson(const wxString& path);
    bool loadJsonFromNode(const wxString& path);

private:

    wxString quoteTable(const wxString& _str);
    wxString quoteAlias(const wxString& _str);    
    wxString quoteField(const wxString& _str);
    wxString formatTableName(const wxString& _table_path);
    wxString makeQueryGroupFunction(const wxString& _input_expr, int group_func);

public:

    std::vector<QueryBuilderSourceTable> m_source_tables;
    std::vector<QueryBuilderParam> m_params;
    wxString m_output_path;
    bool m_distinct;
    tango::IStructurePtr m_validation_struct;
    int m_disposition;
};


#endif

