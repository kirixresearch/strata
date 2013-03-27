/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-04-27
 *
 */


#include "appmain.h"
#include "dlgdatabasefile.h"
#include "importwizard.h"
#include "appcontroller.h"
#include "pkgfile.h"
#include "jobimport.h"
#include "jobimportpkg.h"
#include "jsonconfig.h"
#include <kl/crypt.h>


// password for scrambling password in templates and package files
#define PASSWORD_KEY L"83401732"


#ifdef FindWindow
#undef FindWindow
#endif


// -- tableselection grid column indexes --

enum
{
    ONOFF_IDX = 0,
    SOURCE_TABLENAME_IDX = 1,
    DEST_TABLENAME_IDX = 2,
    APPEND_IDX = 3,
    FIELDMAPPING_IDX = 4
};


const int COMBO_DEFAULT_IDX  = 0;






// helper functions



static wxString serverTypeToString(int type)
{
    switch (type)
    {
        case dbtypeSqlServer:       return wxT("mssql");
        case dbtypeMySql:           return wxT("mysql");
        case dbtypeOracle:          return wxT("oracle");
        case dbtypePostgres:        return wxT("postgres");
        case dbtypeOdbc:            return wxT("odbc");
        case dbtypeDb2:             return wxT("db2");
        case dbtypePackage:         return wxT("package");
        case dbtypeAccess:          return wxT("msaccess");
        case dbtypeExcel:           return wxT("msexcel");
        case dbtypeXbase:           return wxT("xbase");
        case dbtypeDelimitedText:   return wxT("delimited_text");
        case dbtypeFixedLengthText: return wxT("fixed_length_text");
        case dbtypeSqlite:          return wxT("sqlite");
    }
    
    return wxT("");
}

static int stringToServerType(const wxString& str)
{
         if (str == wxT("mssql"))             return dbtypeSqlServer;
    else if (str == wxT("mysql"))             return dbtypeMySql;
    else if (str == wxT("oracle"))            return dbtypeOracle;
    else if (str == wxT("postgres"))          return dbtypePostgres;
    else if (str == wxT("odbc"))              return dbtypeOdbc;
    else if (str == wxT("db2"))               return dbtypeDb2;
    else if (str == wxT("package"))           return dbtypePackage;
    else if (str == wxT("msaccess"))          return dbtypeAccess;
    else if (str == wxT("msexcel"))           return dbtypeExcel;
    else if (str == wxT("xbase"))             return dbtypeXbase;
    else if (str == wxT("delimited_text"))    return dbtypeDelimitedText;
    else if (str == wxT("fixed_length_text")) return dbtypeFixedLengthText;
    else if (str == wxT("sqlite"))            return dbtypeSqlite;
    else return dbtypeUndefined;
}





// -- ImportTemplate class implementation --

ImportTemplate::ImportTemplate()
{

}

bool ImportTemplate::load(const wxString& path)
{
    // try to load the path in the new JSON format
    if (loadJson(path))
        return true;

    // if we can't load it in the new format, try
    // to open it with the old format
    return loadJsonFromNode(path);
}

bool ImportTemplate::loadJson(const wxString& path)
{
    m_ii.tables.clear();
    
    kl::JsonNode root = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (!root.isOk())
        return false;


    // make sure we have the appropriate mime type and version
    kl::JsonNode metadata = root["metadata"];
    if (!metadata.isOk())
        return false;
    kl::JsonNode type = metadata["type"];
    if (!type.isOk())
        return false;
    kl::JsonNode version = metadata["version"];
    if (!version.isOk())
        return false;
    if (type.getString() != wxT("application/vnd.kx.import"))
        return false;
    if (version.getInteger() != 1)
        return false;


    if (root.childExists(L"database_type"))
        m_ii.type = stringToServerType(root["database_type"].getString());
    else if (root.childExists(L"import_type"))
        m_ii.type = stringToServerType(root["import_type"].getString()); // deprecated
            

    kl::JsonNode connection_string = root["connection_string"];
    if (connection_string.isOk())
    {
    }
     else
    {
        kl::JsonNode connection_info = root["connection_info"];
        if (connection_info.isOk())
        {
            if (connection_info.childExists("server"))
                m_ii.server = connection_info["server"].getString();
            if (connection_info.childExists("port"))
                m_ii.port = connection_info["port"].getInteger();
            if (connection_info.childExists("database"))
                m_ii.database = connection_info["database"].getString();
            if (connection_info.childExists("username"))
                m_ii.username = connection_info["username"].getString();

            if (connection_info.childExists("password"))
            {
                wxString pw = connection_info["password"].getString();
                if (kl::isEncryptedString(towstr(pw)))
                    m_ii.password = kl::decryptString(towstr(pw), PASSWORD_KEY);
                     else
                    m_ii.password = pw;
            }
        }
    }
    
    
    kl::JsonNode path_node = root["path"];
    if (path_node.isOk())
        m_ii.path = path_node.getString();
         else
        m_ii.path = wxT("");
    
    kl::JsonNode target_path_node = root["target_path"];
    if (target_path_node.isOk())
        m_ii.base_path = target_path_node.getString();
         else
        m_ii.base_path = wxT("");
        
    kl::JsonNode delimited_text = root["delimited_text"];
    if (delimited_text.isOk())
    {
        if (delimited_text.childExists("delimiter"))
            m_ii.delimiters = delimited_text["delimiter"].getString();
        if (delimited_text.childExists("text_qualifier"))
            m_ii.text_qualifier = delimited_text["text_qualifier"].getString();
        if (delimited_text.childExists("first_row_header"))
            m_ii.first_row_header = delimited_text["first_row_header"].getBoolean();
    }
    
    
    kl::JsonNode objects = root["objects"];
    
    if (objects.isUndefined())
        objects = root["imports"];  // backward compatibility -- can be removed later
    
    if (objects.isOk())
    {
        size_t i, count = objects.getChildCount();
        for (i =  0; i < count; ++i)
        {
            kl::JsonNode object = objects[i];
            
            if (object["query"].isOk())
            {
                // query-style entry
                
                kl::JsonNode query_node = object["query"];
                kl::JsonNode output_node = object["output"];
                kl::JsonNode append_node = object["append"];
                
                if (query_node.isOk() && output_node.isOk())
                {
                    // table-style entry
                    ImportTableSelection its;
                    its.type = ImportTableSelection::typeTable;
                    its.selected = true;
                    its.query = query_node.getString();
                    its.output_tablename = output_node.getString();
                    its.append = false;
                    if (append_node.isOk())
                        its.append = append_node.getBoolean();
                    m_ii.tables.push_back(its);
                }
            }
             else
            {
                kl::JsonNode input_node = object["input"];
                kl::JsonNode output_node = object["output"];
                kl::JsonNode append_node = object["append"];
                
                if (input_node.isOk() && output_node.isOk())
                {
                    // table-style entry
                    ImportTableSelection its;
                    its.type = ImportTableSelection::typeTable;
                    its.selected = true;
                    its.input_tablename = input_node.getString();
                    its.output_tablename = output_node.getString();
                    its.append = false;
                    if (append_node.isOk())
                        its.append = append_node.getBoolean();
                    m_ii.tables.push_back(its);
                }
            }
        }
    }

    return true;
}

bool ImportTemplate::loadJsonFromNode(const wxString& path)
{
    kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (!node.isOk())
        return false;

    kl::JsonNode root_node = node["root"];
    if (!root_node.isOk())
        return false;

    kl::JsonNode kpp_template_node = root_node["kpp_template"];
    if (!kpp_template_node.isOk())
        return false;

    kl::JsonNode type_node = kpp_template_node["type"];
    if (!type_node.isOk() || type_node.getString() != L"import")
        return false;

    kl::JsonNode version_node = kpp_template_node["version"];
    if (!version_node.isOk() || version_node.getInteger() > 1)
        return false;

    kl::JsonNode data_node = kpp_template_node["data"];
    if (!data_node.isOk())
        return false;


    // import type
    kl::JsonNode import_type_node = data_node["type"];
    if (!import_type_node.isOk())
        return false;
    m_ii.type = import_type_node.getInteger();
    m_ii.last_type = import_type_node.getInteger();

    // description
    kl::JsonNode description_node = data_node["description"];
    if (!description_node.isOk())
        return false;
    m_ii.description = towx(description_node.getString());

    // path
    kl::JsonNode path_node = data_node["path"];
    if (!path_node.isOk())
        return false;
    m_ii.path = towx(path_node.getString());

    // filter
    kl::JsonNode filter_node = data_node["filter"];
    if (!filter_node.isOk())
        return false;
    m_ii.filter = towx(filter_node.getString());

    // server
    kl::JsonNode server_node = data_node["server"];
    if (!server_node.isOk())
        return false;
    m_ii.server = towx(server_node.getString());

    // port
    kl::JsonNode port_node = data_node["port"];
    if (!port_node.isOk())
        return false;
    m_ii.port = port_node.getInteger();

    // database
    kl::JsonNode database_node = data_node["database"];
    if (!database_node.isOk())
        return false;
    m_ii.database = towx(database_node.getString());

    // username
    kl::JsonNode username_node = data_node["username"];
    if (!username_node.isOk())
        return false;
    m_ii.username = towx(username_node.getString());

    // password
    kl::JsonNode password_node = data_node["password"];
    if (!password_node.isOk())
        return false;
    m_ii.password = towx(password_node.getString());

    // save password flag
    kl::JsonNode save_password_node = data_node["save_password"];
    if (!save_password_node.isOk())
        return false;
    m_ii.save_password = (save_password_node.getInteger() != 0 ? true : false);

    // base path
    kl::JsonNode base_path_node = data_node["base_path"];
    if (!base_path_node.isOk())
        return false;
    m_ii.base_path = towx(base_path_node.getString());

    // delimiters (for text_delimited imports);
    kl::JsonNode delimiters_node = data_node["delimiters"];
    if (!delimiters_node.isOk())
        return false;
    m_ii.delimiters = towx(delimiters_node.getString());

    // text qualifier (for text-delimited imports);
    kl::JsonNode text_qualifier_node = data_node["text_qualifier"];
    if (!text_qualifier_node.isOk())
        return false;
    m_ii.text_qualifier = towx(text_qualifier_node.getString());

    // first row is header row (for text-delimited imports)
    kl::JsonNode first_row_header_node = data_node["first_row_header"];
    if (!first_row_header_node.isOk())
        return false;
    m_ii.first_row_header = (first_row_header_node.getInteger() != 0 ? true : false);

    // date format (for text-delimited imports)
    kl::JsonNode date_format_str_node = data_node["date_format_str"];
    if (!date_format_str_node.isOk())
        return false;
    m_ii.date_format_str = towx(date_format_str_node.getString());

    // tables base
    kl::JsonNode tables_base_node = data_node["tables"];
    if (!tables_base_node.isOk())
        return false;

    std::vector<kl::JsonNode> tables_base_children_node = tables_base_node.getChildren();
    std::vector<kl::JsonNode>::iterator it_tables, it_tables_end;
    it_tables_end = tables_base_children_node.end();

    for (it_tables = tables_base_children_node.begin(); it_tables != it_tables_end; ++it_tables)
    {
        ImportTableSelection ts;


        kl::JsonNode table_node = *it_tables;
        if (!table_node.isOk())
            return false;

        kl::JsonNode type_node = table_node["type"];
        if (type_node.isOk())
            ts.type = type_node.getInteger();
             else
            ts.type = ImportTableSelection::typeTable;

        kl::JsonNode show_node = table_node["show"];
        if (!show_node.isOk())
            return false;
        ts.show = (show_node.getInteger() != 0 ? true : false);

        kl::JsonNode selected_node = table_node["selected"];
        if (!selected_node.isOk())
            return false;
        ts.selected = (selected_node.getInteger() != 0 ? true : false);

        kl::JsonNode input_tablename_node = table_node["input_tablename"];
        if (!input_tablename_node.isOk())
            return false;
        ts.input_tablename = input_tablename_node.getString();

        kl::JsonNode output_tablename_node = table_node["output_tablename"];
        if (!output_tablename_node.isOk())
            return false;
        ts.output_tablename = output_tablename_node.getString();

        kl::JsonNode append_node = table_node["append"];
        if (!append_node.isOk())
            return false;
        ts.append = (append_node.getInteger() != 0 ? true : false);

        kl::JsonNode field_mapping_name_node = table_node["field_mapping_name"];
        if (!field_mapping_name_node.isOk())    
            return false;
        ts.field_mapping_name = towx(field_mapping_name_node.getString());

        kl::JsonNode query_node = table_node["query"];
        if (query_node.isOk())
            ts.query = towx(query_node.getString());

        // load fixed-length row width (if any)
        kl::JsonNode fixed_length_row_width_node = table_node["fixed_length_row_width"];
        if (fixed_length_row_width_node.isOk())
            ts.row_width = fixed_length_row_width_node.getInteger();
             else
            ts.row_width = 0;
    
        // load fixed-length structure (if any)
        kl::JsonNode fixed_length_structure_node = table_node["fixed_length_structure"];
        if (fixed_length_structure_node.isOk())
        {
            int field_counter;
            int field_count = fixed_length_structure_node.getChildCount();

            for (field_counter = 0; field_counter < field_count; ++field_counter)
            {
                FixedLengthField fs;


                wchar_t buf[255];
                swprintf(buf, 255, L"field%d", field_counter);

                kl::JsonNode field_node = fixed_length_structure_node[buf];
                if (!field_node.isOk())
                    return false;

                kl::JsonNode skip_node = field_node["skip"];
                if (!skip_node.isOk())
                    return false;
                fs.skip = (skip_node.getInteger() != 0 ? true : false);

                kl::JsonNode input_offset_node = field_node["input_offset"];
                if (!input_offset_node.isOk())
                    return false;
                fs.input_offset = input_offset_node.getInteger();

                kl::JsonNode input_width_node = field_node["input_width"];
                if (!input_width_node.isOk())
                    return false;
                fs.input_width = input_width_node.getInteger();

                kl::JsonNode output_name_node = field_node["output_name"];
                if (!output_name_node.isOk())
                    return false;
                fs.output_name = towx(output_name_node.getString());

                kl::JsonNode output_type_node = field_node["output_type"];
                if (!output_type_node.isOk())
                    return false;
                fs.output_type = output_type_node.getInteger();

                kl::JsonNode output_width_node = field_node["output_width"];
                if (!output_width_node.isOk())
                    return false;
                fs.output_width = output_width_node.getInteger();

                kl::JsonNode output_scale_node = field_node["output_scale"];
                if (!output_scale_node.isOk())
                    return false;
                fs.output_scale = output_scale_node.getInteger();

                kl::JsonNode trim_leading_spaces_node = field_node["trim_leading_spaces"];
                if (!trim_leading_spaces_node.isOk())
                    return false;
                fs.trim_leading_spaces = (trim_leading_spaces_node.getInteger() != 0 ? true : false);

                kl::JsonNode trim_leading_zeros_node = field_node["trim_leading_zeros"];
                if (!trim_leading_zeros_node.isOk())
                    return false;
                fs.trim_leading_zeros = (trim_leading_zeros_node.getInteger() != 0 ? true : false);

                kl::JsonNode decimal_separator_node = field_node["decimal_separator"];
                if (!decimal_separator_node.isOk())
                    return false;
                fs.decimal_separator = towx(decimal_separator_node.getString());

                kl::JsonNode negative_sign_node = field_node["negative_sign"];
                if (!negative_sign_node.isOk())
                    return false;
                fs.negative_sign = towx(negative_sign_node.getString());

                kl::JsonNode date_format_node = field_node["date_format"];
                if (!date_format_node.isOk())
                    return false;
                fs.date_order = towx(date_format_node.getString());

                kl::JsonNode boolean_format_node = field_node["boolean_format"];
                if (!boolean_format_node.isOk())
                    return false;
                fs.boolean_format = towx(boolean_format_node.getString());

                kl::JsonNode expression_node = field_node["expression"];
                if (!expression_node.isOk())
                    return false;
                fs.custom_expression = towx(expression_node.getString());

                kl::JsonNode hidden_expression_node = field_node["hidden_expression"];
                if (!hidden_expression_node.isOk())
                    return false;
                fs.hidden_expression = towx(hidden_expression_node.getString());


                ts.fixed_fields.push_back(fs);
            }
        }

        m_ii.tables.push_back(ts);
    }


    // field mappings base
    kl::JsonNode field_mappings_base_node = data_node["field_mappings"];
    if (!field_mappings_base_node.isOk())
        return false;

    std::vector<kl::JsonNode> field_mappings_base_children_node = field_mappings_base_node.getChildren();
    std::vector<kl::JsonNode>::iterator it_fieldmap, it_fieldmap_end;
    it_fieldmap_end = field_mappings_base_children_node.end();

    for (it_fieldmap = field_mappings_base_children_node.begin(); it_fieldmap != it_fieldmap_end; ++it_fieldmap)
    {
        ImportTableSelection ts;


        kl::JsonNode field_mapping_node = *it_fieldmap;

        kl::JsonNode show_node = field_mapping_node["show"];
        if (!show_node.isOk())
            return false;
        ts.show = (show_node.getInteger() != 0 ? true : false);

        kl::JsonNode selected_node = field_mapping_node["selected"];
        if (!selected_node.isOk())
            return false;
        ts.selected = (selected_node.getInteger() != 0 ? true : false);

        kl::JsonNode input_tablename_node = field_mapping_node["input_tablename"];
        if (!input_tablename_node.isOk())
            return false;
        ts.input_tablename = towx(input_tablename_node.getString());

        kl::JsonNode output_tablename_node = field_mapping_node["output_tablename"];
        if (!output_tablename_node.isOk())
            return false;
        ts.output_tablename = towx(output_tablename_node.getString());

        kl::JsonNode append_node = field_mapping_node["append"];
        if (!append_node.isOk())
            return false;
        ts.append = (append_node.getInteger() != 0 ? true : false);

        kl::JsonNode field_mapping_name_node = field_mapping_node["field_mapping_name"];
        if (!field_mapping_name_node.isOk())
            return false;
        ts.field_mapping_name = towx(field_mapping_name_node.getString());

        kl::JsonNode query_node = field_mapping_node["query"];
        if (query_node.isOk())
            ts.query = towx(query_node.getString());


        // output fields
        kl::JsonNode output_field_count_node = field_mapping_node["output_field_count"];
        if (!output_field_count_node.isOk())
            return false;

        int field_counter;
        int output_field_count = output_field_count_node.getInteger();

        for (field_counter = 0; field_counter < output_field_count; ++field_counter)
        {
            FieldSelection fs;


            wchar_t buf[255];
            swprintf(buf, 255, L"output_field%d", field_counter);

            kl::JsonNode output_field_node = field_mapping_node[buf];
            if (!output_field_node.isOk())
                return false;

            kl::JsonNode input_name_node = output_field_node["input_name"];
            if (!input_name_node.isOk())
                return false;
            fs.input_name = towx(input_name_node.getString());

            kl::JsonNode input_type_node = output_field_node["input_type"];
            if (!input_type_node.isOk())
                return false;
            fs.input_type = input_type_node.getInteger();

            kl::JsonNode input_width_node = output_field_node["input_width"];
            if (!input_width_node.isOk())
                return false;
            fs.input_width = input_width_node.getInteger();

            kl::JsonNode input_scale_node = output_field_node["input_scale"];
            if (!input_scale_node.isOk())
                return false;
            fs.input_scale = input_scale_node.getInteger();

            kl::JsonNode output_name_node = output_field_node["output_name"];
            if (!output_name_node.isOk())
                return false;
            fs.output_name = towx(output_name_node.getString());

            kl::JsonNode output_type_node = output_field_node["output_type"];
            if (!output_type_node.isOk())
                return false;
            fs.output_type = output_type_node.getInteger();

            kl::JsonNode output_width_node = output_field_node["output_width"];
            if (!output_width_node.isOk())
                return false;
            fs.output_width = output_width_node.getInteger();

            kl::JsonNode output_scale_node = output_field_node["output_scale"];
            if (!output_scale_node.isOk())
                return false;
            fs.output_scale = output_scale_node.getInteger();

            kl::JsonNode dynamic_node = output_field_node["dynamic"];
            if (!dynamic_node.isOk())
                return false;
            fs.dynamic = dynamic_node.getInteger();

            kl::JsonNode dynamic_state_node = output_field_node["dynamic_state"];
            if (!dynamic_state_node.isOk())
                return false;
            fs.dyn_state = (dynamic_state_node.getInteger() != 0 ? true : false);

            kl::JsonNode expression_node = output_field_node["expression"];
            if (!expression_node.isOk())
                return false;
            fs.expression = towx(expression_node.getString());


            ts.output_fields.push_back(fs);
        }

        m_ii.field_mappings.push_back(ts);
    }

    return true;
}

bool usesConnectionPage(int type)
{
    switch (type)
    {
        case dbtypeSqlServer:
        case dbtypeMySql:
        case dbtypeOracle:
        case dbtypePostgres:
        case dbtypeOdbc:
        case dbtypeDb2:
            return true;
    }
    
    return false;
}

bool ImportTemplate::save(const wxString& path)
{
    kl::JsonNode root;
    
    kl::JsonNode metadata = root["metadata"];
    metadata["type"] = wxT("application/vnd.kx.import");
    metadata["version"] = 1;
    metadata["description"] = wxT("");

    std::wstring dbtype = towstr(serverTypeToString(m_ii.type));
    root["database_type"] = dbtype;

    if (usesConnectionPage(m_ii.type))
    {
        kl::JsonNode connection_info = root["connection_info"];
        connection_info["server"] = towstr(m_ii.server);
        connection_info["port"] = m_ii.port;
        connection_info["database"] = towstr(m_ii.database);
        connection_info["username"] = towstr(m_ii.username);
        connection_info["password"] = kl::encryptString(towstr(m_ii.password), PASSWORD_KEY);
    }
    else
    {
        if (m_ii.path.Length() > 0)
        {
            root["path"] = towstr(m_ii.path);
        }
    }

    if (m_ii.base_path.Length() > 0)
    {
        root["target_path"] = towstr(m_ii.base_path);
    }

    if (m_ii.type == dbtypeDelimitedText)
    {
        kl::JsonNode delimited_text = root["delimited_text"];
        delimited_text["delimiter"] = towstr(m_ii.delimiters);
        delimited_text["text_qualifier"] = towstr(m_ii.text_qualifier);
        delimited_text["first_row_header"].setBoolean(m_ii.first_row_header);
    }
    
    root["objects"].setArray();
    kl::JsonNode objects = root["objects"];


    int table_counter = 0;
    std::vector<ImportTableSelection>::iterator table_it;
    for (table_it = m_ii.tables.begin();
         table_it != m_ii.tables.end();
         ++table_it)
    {
        // if the table was not selected for anything, don't write
        // it out to the save file
        if (!table_it->selected)
            continue;

        kl::JsonNode objects_node = objects.appendElement();


        if (table_it->type == ImportTableSelection::typeTable)
        {
            objects_node["input"] = towstr(table_it->input_tablename);
            objects_node["output"] = towstr(table_it->output_tablename);
            if (table_it->append)
                objects_node["append"].setBoolean(true);
        }
         else if (table_it->type == ImportTableSelection::typeQuery)
        {
            objects_node["query"] = towstr(table_it->query);
            objects_node["output"] = towstr(table_it->output_tablename);
            if (table_it->append)
                objects_node["append"].setBoolean(true);
        }
    }
    


    return JsonConfig::saveToDb(root, g_app->getDatabase(), path, wxT("application/vnd.kx.import"));
}

static void onImportJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobStateFinished)
        return;

    g_app->getAppController()->refreshDbDoc();
}


jobs::IJobPtr ImportTemplate::execute()
{
    // -- concatenate the base path and the table name --

    std::vector<ImportTableSelection>::iterator it;
    for (it = m_ii.tables.begin(); it != m_ii.tables.end(); ++it)
    {
        if (!it->selected)
            continue;

        wxString new_output_path = m_ii.base_path;

        // handle empty base path
        if (new_output_path.IsEmpty())
            new_output_path += wxT('/');

        // handle no slash between base path and tablename
        if (new_output_path.Last() != wxT('/') &&
            !it->output_tablename.StartsWith(wxT("/")))
        {
            new_output_path += wxT("/");
        }

        // handle double slash between base path and tablename
        if (new_output_path.Last() == wxT('/') &&
            it->output_tablename.StartsWith(wxT("/")))
        {
            new_output_path.RemoveLast();
        }

        new_output_path += it->output_tablename;

        it->output_tablename = new_output_path;
    }


    // -- check for package file import --

    if (m_ii.type == dbtypePackage)
    {
        ImportPkgJob* job = new ImportPkgJob;
        job->setPkgFilename(m_ii.path);

        std::vector<ImportTableSelection>::iterator it;
        for (it = m_ii.tables.begin(); it != m_ii.tables.end(); ++it)
        {
            if (!it->selected)
                continue;

            job->addImportObject(it->input_tablename,
                                 it->output_tablename);
        }

        g_app->getJobQueue()->addJob(job, jobStateRunning);

        return static_cast<jobs::IJob*>(job);
    }

    // we have to do this here, since the path selection page is now used
    // for these types of imports (if we have a path of c:\myfile.txt,
    // when we try to create an unmanaged connection, it will fail
    // because that path is not a folder, it is a file
    if (m_ii.type == dbtypeXbase ||
        m_ii.type == dbtypeDelimitedText)
        m_ii.path = wxEmptyString;
        
    ImportJob* job = new ImportJob;
    job->sigJobFinished().connect(&onImportJobFinished);
    job->setImportType(m_ii.type);
    job->setFilename(m_ii.path);
    job->setConnectionInfo(m_ii.server,
                           m_ii.port,
                           m_ii.database,
                           m_ii.username,
                           m_ii.password);

    // set the job title
    if (m_ii.type == dbtypePackage ||
        m_ii.type == dbtypeExcel ||
        m_ii.type == dbtypeAccess)
    {
        wxString title = m_ii.path.AfterLast(PATH_SEPARATOR_CHAR);
        wxString job_title = wxString::Format(_("Importing from '%s'"),
                                              title.c_str());
        
        job->getJobInfo()->setTitle(towstr(job_title));
    }
     else if (m_ii.type == dbtypeSqlServer ||
              m_ii.type == dbtypeMySql     ||
              m_ii.type == dbtypeOracle    ||
              m_ii.type == dbtypeOdbc      ||
              m_ii.type == dbtypeDb2)
    {
        wxString db_name;
        switch (m_ii.type)
        {
            case dbtypeSqlServer:   db_name = _("SQL Server");  break;
            case dbtypeMySql:       db_name = _("MySQL");       break;
            case dbtypeOracle:      db_name = _("Oracle");      break;
            case dbtypeOdbc:        db_name = _("ODBC");        break;
            case dbtypeDb2:         db_name = _("DB2");         break;
        }
        
        wxString job_title = wxString::Format(_("Importing from %s on '%s'"),
                                              db_name.c_str(), m_ii.server.c_str());
        
        job->getJobInfo()->setTitle(towstr(job_title));
    }

    for (it = m_ii.tables.begin(); it != m_ii.tables.end(); ++it)
    {
        ImportJobInfo job_import_info;

        if (!it->selected)
            continue;

        if (it->type == ImportTableSelection::typeTable)
        {
            job_import_info.delimiters = m_ii.delimiters;
            job_import_info.text_qualifier = m_ii.text_qualifier;
            job_import_info.first_row_header = m_ii.first_row_header;

            job_import_info.input_path = it->input_tablename;
            job_import_info.output_path = it->output_tablename;
            job_import_info.append = it->append;
            job_import_info.row_width = it->row_width;
            job_import_info.field_info.clear();


            if (m_ii.type == dbtypeFixedLengthText)
            {
                // -- fill out the field info for the fixed-length text set --
                FieldTransInfo fi;

                std::vector<FixedLengthField>::iterator field_it;
                for (field_it = it->fixed_fields.begin();
                     field_it != it->fixed_fields.end();
                     ++field_it)
                {
                    if (field_it->skip)
                        continue;

                    fi.input_offset = field_it->input_offset;
                    fi.input_width = field_it->input_width;

                    fi.output_name = field_it->output_name;
                    fi.output_type = field_it->output_type;
                    fi.output_width = field_it->output_width;
                    fi.output_scale = field_it->output_scale;

                    if (field_it->custom_expression.Length() > 0)
                    {
                        fi.expression = field_it->custom_expression;
                    }
                     else
                    {
                        fi.expression = field_it->hidden_expression;
                    }

                    job_import_info.field_info.push_back(fi);
                }
            }
             else if (it->field_mapping_name.Length() > 0)
            {
                // -- try to find an appropriate field mapping --
                ImportTableSelection field_mapping;

                bool found = false;

                std::vector<ImportTableSelection>::iterator fm_it;

                for (fm_it = m_ii.field_mappings.begin();
                     fm_it != m_ii.field_mappings.end();
                     ++fm_it)
                {
                    if (0 == fm_it->field_mapping_name.CmpNoCase(
                                                      it->field_mapping_name))
                    {
                        field_mapping = *fm_it;
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    // -- problem: specified field mapping could not
                    //    be found in the field mapping array --
                    return xcm::null;
                }


                job_import_info.query = field_mapping.query;


                FieldTransInfo fi;

                std::vector<FieldSelection>::iterator field_it;
                for (field_it = field_mapping.output_fields.begin();
                     field_it != field_mapping.output_fields.end();
                     ++field_it)
                {
                    fi.input_name = field_it->input_name;
                    fi.input_type = field_it->input_type;
                    fi.input_width = field_it->input_width;
                    fi.input_scale = field_it->input_scale;
                    fi.input_offset = field_it->input_offset;

                    fi.output_name = field_it->output_name;
                    fi.output_type = field_it->output_type;
                    fi.output_width = field_it->output_width;
                    fi.output_scale = field_it->output_scale;

                    fi.expression = field_it->expression;

                    job_import_info.field_info.push_back(fi);
                }
            }
        }
         else if (it->type == ImportTableSelection::typeQuery)
        {
            job_import_info.query = it->query;
            job_import_info.output_path = it->output_tablename;
            job_import_info.append = it->append;
            job_import_info.field_info.clear();
        }

        job->addImportSet(job_import_info);
    }

    g_app->getJobQueue()->addJob(job, jobStateRunning);

    return static_cast<jobs::IJob*>(job);
}


