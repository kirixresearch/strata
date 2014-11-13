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
#include "appcontroller.h"
#include "jsonconfig.h"
#include "tabledoc.h"
#include "importtemplate.h"
#include <kl/crypt.h>


// password for scrambling password in templates and package files
#define PASSWORD_KEY L"83401732"


#ifdef FindWindow
#undef FindWindow
#endif


// tableselection grid column indexes

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
        case xd::dbtypeSqlServer:       return "mssql";
        case xd::dbtypeMySql:           return "mysql";
        case xd::dbtypeOracle:          return "oracle";
        case xd::dbtypePostgres:        return "postgres";
        case xd::dbtypeOdbc:            return "odbc";
        case xd::dbtypeDb2:             return "db2";
        case xd::dbtypeKpg:             return "package";
        case xd::dbtypeAccess:          return "msaccess";
        case xd::dbtypeExcel:           return "msexcel";
        case xd::dbtypeFilesystem:      return "files";
        //case xd::dbtypeXbase:           return "xbase";
        //case xd::dbtypeDelimitedText:   return "delimited_text";
        //case xd::dbtypeFixedLengthText: return "fixed_length_text";
        case xd::dbtypeSqlite:          return "sqlite";
    }
    
    return wxT("");
}

static int stringToServerType(const wxString& str)
{
         if (str == "mssql")             return xd::dbtypeSqlServer;
    else if (str == "mysql")             return xd::dbtypeMySql;
    else if (str == "oracle")            return xd::dbtypeOracle;
    else if (str == "postgres")          return xd::dbtypePostgres;
    else if (str == "odbc")              return xd::dbtypeOdbc;
    else if (str == "db2")               return xd::dbtypeDb2;
    else if (str == "package")           return xd::dbtypeKpg;
    else if (str == "msaccess")          return xd::dbtypeAccess;
    else if (str == "msexcel")           return xd::dbtypeExcel;
    else if (str == "xbase")             return xd::dbtypeFilesystem;
    else if (str == "delimited_text")    return xd::dbtypeFilesystem;
    else if (str == "fixed_length_text") return xd::dbtypeFilesystem;
    else if (str == "files")             return xd::dbtypeFilesystem;
    else if (str == "sqlite")            return xd::dbtypeSqlite;
    else return xd::dbtypeUndefined;
}





// ImportTemplate class implementation

ImportTemplate::ImportTemplate()
{

}

bool ImportTemplate::load(const std::wstring& path)
{
    // try to load the path in the new JSON format
    if (loadJson(path))
        return true;

    // if we can't load it in the new format, try
    // to open it with the old format
    return loadJsonFromNode(path);
}

bool ImportTemplate::loadJson(const std::wstring& path)
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
    

    kl::JsonNode binary_copy = root["binary_copy"];
    if (binary_copy.isOk())
    {
        m_ii.binary_copy = binary_copy.getBoolean();
    }
     else
    {
        m_ii.binary_copy = false;
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

bool ImportTemplate::loadJsonFromNode(const std::wstring& path)
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
    m_ii.description = description_node.getString();

    // path
    kl::JsonNode path_node = data_node["path"];
    if (!path_node.isOk())
        return false;
    m_ii.path = path_node.getString();

    // filter
    kl::JsonNode filter_node = data_node["filter"];
    if (!filter_node.isOk())
        return false;
    m_ii.filter = filter_node.getString();

    // server
    kl::JsonNode server_node = data_node["server"];
    if (!server_node.isOk())
        return false;
    m_ii.server = server_node.getString();

    // port
    kl::JsonNode port_node = data_node["port"];
    if (!port_node.isOk())
        return false;
    m_ii.port = port_node.getInteger();

    // database
    kl::JsonNode database_node = data_node["database"];
    if (!database_node.isOk())
        return false;
    m_ii.database = database_node.getString();

    // username
    kl::JsonNode username_node = data_node["username"];
    if (!username_node.isOk())
        return false;
    m_ii.username = username_node.getString();

    // password
    kl::JsonNode password_node = data_node["password"];
    if (!password_node.isOk())
        return false;
    m_ii.password = password_node.getString();

    // save password flag
    kl::JsonNode save_password_node = data_node["save_password"];
    if (!save_password_node.isOk())
        return false;
    m_ii.save_password = (save_password_node.getInteger() != 0 ? true : false);

    // base path
    kl::JsonNode base_path_node = data_node["base_path"];
    if (!base_path_node.isOk())
        return false;
    m_ii.base_path = base_path_node.getString();

    // delimiters (for text_delimited imports);
    kl::JsonNode delimiters_node = data_node["delimiters"];
    if (!delimiters_node.isOk())
        return false;
    m_ii.delimiters = delimiters_node.getString();

    // text qualifier (for text-delimited imports);
    kl::JsonNode text_qualifier_node = data_node["text_qualifier"];
    if (!text_qualifier_node.isOk())
        return false;
    m_ii.text_qualifier = text_qualifier_node.getString();

    // first row is header row (for text-delimited imports)
    kl::JsonNode first_row_header_node = data_node["first_row_header"];
    if (!first_row_header_node.isOk())
        return false;
    m_ii.first_row_header = (first_row_header_node.getInteger() != 0 ? true : false);

    // date format (for text-delimited imports)
    kl::JsonNode date_format_str_node = data_node["date_format_str"];
    if (!date_format_str_node.isOk())
        return false;
    m_ii.date_format_str = date_format_str_node.getString();

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
        ts.field_mapping_name = field_mapping_name_node.getString();

        kl::JsonNode query_node = table_node["query"];
        if (query_node.isOk())
            ts.query = query_node.getString();

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
                fs.output_name = output_name_node.getString();

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
                fs.decimal_separator = decimal_separator_node.getString();

                kl::JsonNode negative_sign_node = field_node["negative_sign"];
                if (!negative_sign_node.isOk())
                    return false;
                fs.negative_sign = negative_sign_node.getString();

                kl::JsonNode date_format_node = field_node["date_format"];
                if (!date_format_node.isOk())
                    return false;
                fs.date_order = date_format_node.getString();

                kl::JsonNode boolean_format_node = field_node["boolean_format"];
                if (!boolean_format_node.isOk())
                    return false;
                fs.boolean_format = boolean_format_node.getString();

                kl::JsonNode expression_node = field_node["expression"];
                if (!expression_node.isOk())
                    return false;
                fs.custom_expression = expression_node.getString();

                kl::JsonNode hidden_expression_node = field_node["hidden_expression"];
                if (!hidden_expression_node.isOk())
                    return false;
                fs.hidden_expression = hidden_expression_node.getString();


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
        ts.input_tablename = input_tablename_node.getString();

        kl::JsonNode output_tablename_node = field_mapping_node["output_tablename"];
        if (!output_tablename_node.isOk())
            return false;
        ts.output_tablename = output_tablename_node.getString();

        kl::JsonNode append_node = field_mapping_node["append"];
        if (!append_node.isOk())
            return false;
        ts.append = (append_node.getInteger() != 0 ? true : false);

        kl::JsonNode field_mapping_name_node = field_mapping_node["field_mapping_name"];
        if (!field_mapping_name_node.isOk())
            return false;
        ts.field_mapping_name = field_mapping_name_node.getString();

        kl::JsonNode query_node = field_mapping_node["query"];
        if (query_node.isOk())
            ts.query = query_node.getString();


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
            fs.input_name = input_name_node.getString();

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
            fs.output_name = output_name_node.getString();

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
            fs.expression = expression_node.getString();


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
        case xd::dbtypeSqlServer:
        case xd::dbtypeMySql:
        case xd::dbtypeOracle:
        case xd::dbtypePostgres:
        case xd::dbtypeOdbc:
        case xd::dbtypeDb2:
            return true;
    }
    
    return false;
}

bool ImportTemplate::save(const std::wstring& path)
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
        connection_info["server"] = m_ii.server;
        connection_info["port"] = m_ii.port;
        connection_info["database"] = m_ii.database;
        connection_info["username"] = m_ii.username;
        connection_info["password"] = kl::encryptString(m_ii.password, PASSWORD_KEY);
    }
    else
    {
        if (m_ii.path.length() > 0)
        {
            root["path"] = m_ii.path;
        }
    }

    if (m_ii.base_path.length() > 0)
    {
        root["target_path"] = m_ii.base_path;
    }

    if (m_ii.delimiters.length() > 0)
    {
        kl::JsonNode delimited_text = root["delimited_text"];
        delimited_text["delimiter"] = m_ii.delimiters;
        delimited_text["text_qualifier"] = m_ii.text_qualifier;
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
            objects_node["input"] = table_it->input_tablename;
            objects_node["output"] = table_it->output_tablename;
            if (table_it->append)
                objects_node["append"].setBoolean(true);
        }
         else if (table_it->type == ImportTableSelection::typeQuery)
        {
            objects_node["query"] = table_it->query;
            objects_node["output"] = table_it->output_tablename;
            if (table_it->append)
                objects_node["append"].setBoolean(true);
        }
    }
    


    return JsonConfig::saveToDb(root, g_app->getDatabase(), path, L"application/vnd.kx.import");
}



static void readKpgMetadata(jobs::IJobPtr job)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    IConnectionPtr conn = createUnmanagedConnection();
    conn->setType(xd::dbtypeKpg);
    conn->setPath(job->getExtraValue(L"kpg"));
    conn->open();

    xd::IDatabasePtr kpg = conn->getDatabasePtr();
    if (kpg.isNull())
        return;

    kl::JsonNode params;
    params.fromString(job->getParameters());

    size_t i;
    for (i = 0; i < params["objects"].getChildCount(); ++i)
    {
        kl::JsonNode object = params["objects"][i];
        
        std::wstring source_path = object["source_path"];
        std::wstring destination_path = object["destination_path"];

        if (source_path.empty() || destination_path.empty())
            continue;

        // get object id of the table we imported
        xd::IFileInfoPtr finfo = db->getFileInfo(destination_path);
        if (finfo.isNull())
            continue;
        std::wstring object_id = finfo->getObjectId();
        

        // read resource from kpg
        source_path = L".resource/" + source_path;

        std::wstring json;
        if (!readStreamTextFile(kpg, source_path, json))
            continue;


        // load the json into the model
        ITableDocModelPtr model = TableDocMgr::loadModel(object_id);
        if (model.isNull())
            continue;
        model->fromJson(json);
    }
}

static void onImportJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;

    if (job->getExtraValue(L"kpg").length() > 0)
        readKpgMetadata(job);

    g_app->getAppController()->refreshDbDoc();
}

jobs::IJobPtr ImportTemplate::createJob()
{
    // concatenate the base path and the table name

    std::vector<ImportTableSelection>::iterator it;
    for (it = m_ii.tables.begin(); it != m_ii.tables.end(); ++it)
    {
        if (!it->selected)
            continue;

        std::wstring new_output_path = m_ii.base_path;

        // handle empty base path
        if (new_output_path.empty() || new_output_path[new_output_path.length() - 1] != '/')
            new_output_path += L"/";

        if (it->output_tablename.substr(0, 1) == L"/")
            new_output_path += it->output_tablename.substr(1);
             else
            new_output_path += it->output_tablename;

        it->output_tablename = new_output_path;
    }


    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.load-job");
    job->getJobInfo()->setTitle(towstr(_("Importing Data")));

    if (m_ii.type == xd::dbtypeKpg)
    {
        job->setExtraValue(L"kpg", m_ii.path);
    }

    // determine source connection string

    IConnectionPtr conn = createUnmanagedConnection();
    conn->setType(m_ii.type);
    conn->setDescription(m_ii.description);
    conn->setHost(m_ii.server);
    conn->setPort(m_ii.port);
    conn->setDatabase(m_ii.database);
    conn->setUsername(m_ii.username);
    conn->setPassword(m_ii.password);

    conn->setPath(m_ii.path);

    std::wstring source_connection = conn->getConnectionString();
    std::wstring destination_connection = g_app->getDatabaseConnectionString();

    // configure the job parameters
    kl::JsonNode params;

    params["objects"].setArray();
    kl::JsonNode objects = params["objects"];


    for (it = m_ii.tables.begin(); it != m_ii.tables.end(); ++it)
    {
        if (it->selected)
        {
            kl::JsonNode object = objects.appendElement();

            object["source_connection"] = source_connection;
            object["destination_connection"] = destination_connection;

            object["source_path"] = it->input_tablename;
            object["destination_path"] = it->output_tablename;

            object["overwrite"].setBoolean(true);

            if (m_ii.binary_copy)
            {
                object["source_format"].setObject();
                kl::JsonNode source_format = object["source_format"];
            
                source_format["object_type"] = L"stream";
            }
             else if (m_ii.delimiters.length() > 0)
            {
                object["source_format"].setObject();
                kl::JsonNode source_format = object["source_format"];
            
                source_format["object_type"] = L"table";
                source_format["format"] = L"delimited_text";
                source_format["delimiter"] = m_ii.delimiters;
                source_format["text_qualifier"] = m_ii.text_qualifier;
                source_format["header_row"].setBoolean(m_ii.first_row_header);
            }
        }
    }

    job->setParameters(params.toString());
    job->sigJobFinished().connect(&onImportJobFinished);

    return job;
}


jobs::IJobPtr ImportTemplate::execute()
{
    jobs::IJobPtr job = createJob();

    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);

    return job;
}

