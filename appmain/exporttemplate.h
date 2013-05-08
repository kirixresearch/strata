/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2013-05-08
 *
 */


#ifndef __APP_EXPORTTEMPLATE_H
#define __APP_EXPORTTEMPLATE_H



class ExportTableSelection
{
public:

    ExportTableSelection()
    {
        append = false;
        input_tablename = wxT("");
        output_tablename = wxT("");
    }

    ExportTableSelection(const ExportTableSelection& c)
    {
        append = c.append;
        input_tablename = c.input_tablename;
        output_tablename = c.output_tablename;
    }

    ExportTableSelection& operator=(const ExportTableSelection& c)
    {
        append = c.append;
        input_tablename = c.input_tablename;
        output_tablename = c.output_tablename;
        return *this;
    }
    
public:

    bool append;
    wxString input_tablename;
    wxString output_tablename;
};


class ExportInfo : public ConnectionInfo
{
public:

    ExportInfo()
    {
        base_path = wxT("");
        delimiters = wxT(",");
        text_qualifier = wxT("\"");
        date_format_str = wxT("YMDhmsl");
        first_row_header = false;
        overwrite_file = true;
        fix_invalid_fieldnames = false;
    }

    ExportInfo(const ExportInfo& c) : ConnectionInfo(c)
    {
        tables = c.tables;
        base_path = c.base_path;
        delimiters = c.delimiters;
        text_qualifier = c.text_qualifier;
        date_format_str = c.date_format_str;
        first_row_header = c.first_row_header;
        overwrite_file = c.overwrite_file;
        fix_invalid_fieldnames = c.fix_invalid_fieldnames;
    }

    ExportInfo& operator=(const ExportInfo& c)
    {
        tables = c.tables;
        base_path = c.base_path;
        delimiters = c.delimiters;
        text_qualifier = c.text_qualifier;
        date_format_str = c.date_format_str;
        first_row_header = c.first_row_header;
        overwrite_file = c.overwrite_file;
        fix_invalid_fieldnames = c.fix_invalid_fieldnames;
        return *this;
    }
    
public:

    std::vector<ExportTableSelection> tables;
    wxString base_path;
    wxString delimiters;
    wxString text_qualifier;
    wxString date_format_str;
    bool first_row_header;
    bool overwrite_file;
    bool fix_invalid_fieldnames;
};



class ExportTemplate
{
public:

    ExportTemplate();

    bool load(const wxString& path);
    bool save(const wxString& path);
    jobs::IJobPtr execute();
    
public:

    ExportInfo m_ei;
};




#endif  // __APP_EXPORTPAGES_H


