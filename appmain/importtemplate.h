/*!
*
* Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
*
* Project:  Application Client
* Author:   David Z. Williams
* Created:  2005-04-27
*
*/


#ifndef __APP_IMPORTTEMPLATE_H
#define __APP_IMPORTTEMPLATE_H

#include "connectionwizard.h"

class FixedLengthField
{
public:

    FixedLengthField()
    {
        input_offset = 0;
        input_width = 0;

        output_name = wxT("");
        output_type = xd::typeCharacter;
        output_width = 20;
        output_scale = 0;

        skip = false;
        trim_leading_spaces = false;
        trim_leading_zeros = false;
        decimal_separator = wxT(".");
        negative_sign = wxT("prefix");
        date_order = wxT("YYYYMMDD");
        boolean_format = wxT("T/F");
        custom_expression = wxT("");
        hidden_expression = wxT("");
    }

    FixedLengthField(const FixedLengthField& c)
    {
        input_offset = c.input_offset;
        input_width = c.input_width;

        output_name = c.output_name;
        output_type = c.output_type;
        output_width = c.output_width;
        output_scale = c.output_scale;

        skip = c.skip;
        trim_leading_spaces = c.trim_leading_spaces;
        trim_leading_zeros = c.trim_leading_zeros;
        decimal_separator = c.decimal_separator;
        negative_sign = c.negative_sign;
        date_order = c.date_order;
        boolean_format = c.boolean_format;
        custom_expression = c.custom_expression;
        hidden_expression = c.hidden_expression;
    }

    FixedLengthField& operator=(const FixedLengthField& c)
    {
        input_offset = c.input_offset;
        input_width = c.input_width;

        output_name = c.output_name;
        output_type = c.output_type;
        output_width = c.output_width;
        output_scale = c.output_scale;

        skip = c.skip;
        trim_leading_spaces = c.trim_leading_spaces;
        trim_leading_zeros = c.trim_leading_zeros;
        decimal_separator = c.decimal_separator;
        negative_sign = c.negative_sign;
        date_order = c.date_order;
        boolean_format = c.boolean_format;
        custom_expression = c.custom_expression;
        hidden_expression = c.hidden_expression;

        return *this;
    }

    int input_offset;
    int input_width;

    std::wstring output_name;
    int output_type;
    int output_width;
    int output_scale;

    bool skip;
    bool trim_leading_spaces;
    bool trim_leading_zeros;
    std::wstring decimal_separator;
    std::wstring negative_sign;
    std::wstring date_order;
    std::wstring boolean_format;
    std::wstring custom_expression;
    std::wstring hidden_expression;
};


// functor to sort FixedLengthField objects
class FixedLengthFieldBreakSort
{
public:

    bool operator()(const FixedLengthField& x,
        const FixedLengthField& y) const
    {
        return x.input_offset < y.input_offset ? true : false;
    }
};


class FieldSelection
{
public:

    FieldSelection()
    {
        input_name = wxT("");
        input_type = xd::typeCharacter;
        input_width = 20;
        input_scale = 0;
        input_offset = 0;

        output_name = wxT("");
        output_type = xd::typeCharacter;
        output_width = 20;
        output_scale = 0;

        dynamic = -1;
        dyn_state = false;
        expression = wxT("");
    }

    FieldSelection(const FieldSelection& c)
    {
        input_name = c.input_name;
        input_type = c.input_type;
        input_width = c.input_width;
        input_scale = c.input_scale;
        input_offset = c.input_offset;

        output_name = c.output_name;
        output_type = c.output_type;
        output_width = c.output_width;
        output_scale = c.output_scale;

        dynamic = c.dynamic;
        dyn_state = c.dyn_state;
        expression = c.expression;
    }

    FieldSelection& operator=(const FieldSelection& c)
    {
        input_name = c.input_name;
        input_type = c.input_type;
        input_width = c.input_width;
        input_scale = c.input_scale;
        input_offset = c.input_offset;

        output_name = c.output_name;
        output_type = c.output_type;
        output_width = c.output_width;
        output_scale = c.output_scale;

        dynamic = c.dynamic;
        dyn_state = c.dyn_state;
        expression = c.expression;

        return *this;
    }

public:

    std::wstring input_name;
    int input_type;
    int input_width;
    int input_scale;
    int input_offset;

    std::wstring output_name;
    int output_type;
    int output_width;
    int output_scale;

    int dynamic;
    bool dyn_state;         // for the grid
    std::wstring expression;
};


class ImportTableSelection
{
public:

    enum
    {
        // add values, but don't change existing ones
        typeTable = 0,
        typeQuery = 1
    };

    ImportTableSelection()
    {
        type = typeTable;
        show = false;
        selected = false;
        append = false;
        input_tablename = wxT("");
        output_tablename = wxT("");
        field_mapping_name = wxT("");
        query = wxT("");
        row_width = 0;
    }

    ImportTableSelection(const ImportTableSelection& c)
    {
        type = c.type;
        show = c.show;
        selected = c.selected;
        append = c.append;
        input_tablename = c.input_tablename;
        output_tablename = c.output_tablename;
        field_mapping_name = c.field_mapping_name;
        output_fields = c.output_fields;
        fixed_fields = c.fixed_fields;
        query = c.query;
        row_width = c.row_width;
    }

    ImportTableSelection& operator=(const ImportTableSelection& c)
    {
        type = c.type;
        show = c.show;
        selected = c.selected;
        append = c.append;
        input_tablename = c.input_tablename;
        output_tablename = c.output_tablename;
        field_mapping_name = c.field_mapping_name;
        output_fields = c.output_fields;
        fixed_fields = c.fixed_fields;
        query = c.query;
        row_width = c.row_width;
        return *this;
    }

public:

    int type;
    bool show;
    bool selected;
    bool append;
    std::wstring input_tablename;
    std::wstring output_tablename;
    std::wstring field_mapping_name;
    std::wstring query;
    std::vector<FieldSelection> output_fields;

    // fixed length text settings
    std::vector<FixedLengthField> fixed_fields;
    int row_width;
};


class ImportInfo : public ConnectionInfo
{
public:

    ImportInfo()
    {
        base_path = L"/";
        delimiters = L",";
        text_qualifier = L"\"";
        date_format_str = L"YMDhmsl";
        first_row_header = true;
    }

    ImportInfo(const ImportInfo& c) : ConnectionInfo(c)
    {
        tables = c.tables;
        field_mappings = c.field_mappings;
        base_path = c.base_path;
        delimiters = c.delimiters;
        text_qualifier = c.text_qualifier;
        date_format_str = c.date_format_str;
        first_row_header = c.first_row_header;
    }

    ImportInfo& operator=(const ImportInfo& c)
    {
        tables = c.tables;
        field_mappings = c.field_mappings;
        base_path = c.base_path;
        delimiters = c.delimiters;
        text_qualifier = c.text_qualifier;
        date_format_str = c.date_format_str;
        first_row_header = c.first_row_header;
        return *this;
    }

public:

    std::vector<ImportTableSelection> tables;
    std::vector<ImportTableSelection> field_mappings;
    std::wstring base_path;

    // delimited text settings
    std::wstring delimiters;
    std::wstring text_qualifier;
    std::wstring date_format_str;
    bool first_row_header;
};


class ImportTemplate
{
public:

    ImportTemplate();

    bool load(const std::wstring& path);
    bool save(const std::wstring& path);

    jobs::IJobPtr createJob();
    jobs::IJobPtr execute();

public:

    bool loadJson(const std::wstring& path);
    bool loadJsonFromNode(const std::wstring& path);

public:

    ImportInfo m_ii;
};


// functor to sort ImportTableSelection objects
class ImportTableSelectionLess
{
public:

    bool operator()(const ImportTableSelection& x,
        const ImportTableSelection& y) const
    {
        return wcscasecmp(x.input_tablename.c_str(), y.input_tablename.c_str()) < 0 ? true : false;
    }
};


#endif

