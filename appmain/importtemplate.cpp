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
    m_ii.tables.clear();
    
    kl::JsonNode root = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (!root.isOk())
        return loadOldVersion(path);


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


bool ImportTemplate::loadOldVersion(const wxString& path)
{
    // create an ofs file with the run information
    tango::IDatabasePtr db = g_app->getDatabase();

    tango::INodeValuePtr file = db->openNodeFile(towstr(path));
    if (file.isNull())
        return false;

    tango::INodeValuePtr kpp_template = file->getChild(L"kpp_template", false);
    if (!kpp_template)
        return false;

    tango::INodeValuePtr template_type = kpp_template->getChild(L"type", false);
    if (!template_type)
        return false;

    if (template_type->getString() != L"import")
        return false;

    tango::INodeValuePtr template_version = kpp_template->getChild((L"version"), false);
    if (!template_version)
        return false;

    if (template_version->getInteger() > 1)
        return false;

    tango::INodeValuePtr data_root = kpp_template->getChild(L"data", false);
    if (!data_root)
        return false;


    // type
    tango::INodeValuePtr type_val = data_root->getChild(L"type", false);
    if (!type_val)
        return false;
    m_ii.type = type_val->getInteger();
    m_ii.last_type = type_val->getInteger();

    // description
    tango::INodeValuePtr desc_val = data_root->getChild(L"description", false);
    if (!desc_val)
        return false;
    m_ii.description = towx(desc_val->getString());

    // path
    tango::INodeValuePtr path_val = data_root->getChild(L"path", false);
    if (!path_val)
        return false;
    m_ii.path = towx(path_val->getString());

    // filter
    tango::INodeValuePtr filter_val = data_root->getChild(L"filter", false);
    if (!filter_val)
        return false;
    m_ii.filter = towx(filter_val->getString());

    // server
    tango::INodeValuePtr server_val = data_root->getChild(L"server", false);
    if (!server_val)
        return false;
    m_ii.server = towx(server_val->getString());

    // port
    tango::INodeValuePtr port_val = data_root->getChild(L"port", false);
    if (!port_val)
        return false;
    m_ii.port = port_val->getInteger();
    
    // database
    tango::INodeValuePtr database_val = data_root->getChild(L"database", false);
    if (!database_val)
        return false;
    m_ii.database = towx(database_val->getString());

    // username
    tango::INodeValuePtr username_val = data_root->getChild(L"username", false);
    if (!username_val)
        return false;
    m_ii.username = towx(username_val->getString());

    // password
    tango::INodeValuePtr password_val = data_root->getChild(L"password", false);
    if (!password_val)
        return false;
    m_ii.password = towx(password_val->getString());

    // save password
    tango::INodeValuePtr save_password_val = data_root->getChild(L"save_password", false);
    if (!save_password_val)
        return false;
    m_ii.save_password = save_password_val->getBoolean();

    // base path
    tango::INodeValuePtr base_path_val = data_root->getChild(L"base_path", false);
    if (!base_path_val)
        return false;
    m_ii.base_path = towx(base_path_val->getString());

    // delimiters (for text-delimited imports)
    tango::INodeValuePtr delimiters_val = data_root->getChild(L"delimiters", false);
    if (!delimiters_val)
        return false;
    m_ii.delimiters = towx(delimiters_val->getString());

    // text qualifier (for text-delimited imports)
    tango::INodeValuePtr text_qualifier_val = data_root->getChild(L"text_qualifier", false);
    if (!text_qualifier_val)
        return false;
    m_ii.text_qualifier = towx(text_qualifier_val->getString());

    // first row is header row (for text-delimited imports)
    tango::INodeValuePtr first_row_header_val = data_root->getChild(L"first_row_header", false);
    if (!first_row_header_val)
        return false;
    m_ii.first_row_header = first_row_header_val->getBoolean();

    // date format (for text-delimited imports)
    tango::INodeValuePtr date_format_val = data_root->getChild(L"date_format_str", false);
    if (!date_format_val)
        return false;
    m_ii.date_format_str = towx(date_format_val->getString());

    // tables base
    tango::INodeValuePtr tables_base = data_root->getChild(L"tables", false);
    if (!tables_base)
        return false;

    int table_count = tables_base->getChildCount();

    // try to load the tables that were saved

    int table_counter;
    for (table_counter = 0;
         table_counter < table_count;
         ++table_counter)
    {
        tango::INodeValuePtr table_node = tables_base->getChildByIdx(table_counter);

        tango::INodeValuePtr type = table_node->getChild(L"type", false);

        // show
        tango::INodeValuePtr is_shown = table_node->getChild(L"show", false);
        if (!is_shown)
            return false;

        // selected
        tango::INodeValuePtr selected = table_node->getChild(L"selected", false);
        if (!selected)
            return false;

        // input tablename
        tango::INodeValuePtr input_tn = table_node->getChild(L"input_tablename", false);
        if (!input_tn)
            return false;

        // output tablename
        tango::INodeValuePtr output_tn = table_node->getChild(L"output_tablename", false);
        if (!output_tn)
            return false;

        // append
        tango::INodeValuePtr append = table_node->getChild(L"append", false);
        if (!append)
            return false;

        // field mapping name
        tango::INodeValuePtr td_name = table_node->getChild(L"field_mapping_name", false);
        if (!td_name)
            return false;

        // query
        tango::INodeValuePtr query = table_node->getChild(L"query", false);

        ImportTableSelection ts;

        if (type.isOk())
            ts.type = type->getInteger();
             else
            ts.type = ImportTableSelection::typeTable;

        ts.show = is_shown->getBoolean();
        ts.selected = selected->getBoolean();
        ts.input_tablename = towx(input_tn->getString());
        ts.output_tablename = towx(output_tn->getString());
        ts.append = append->getBoolean();
        ts.field_mapping_name = towx(td_name->getString());

        if (query.isOk())
        {
            ts.query = towx(query->getString());
        }

        // load fixed-length row width (if any)
        tango::INodeValuePtr fixed_length_row_width = table_node->getChild(L"fixed_length_row_width", false);
        if (fixed_length_row_width.isOk())
            ts.row_width = fixed_length_row_width->getInteger();
             else
            ts.row_width = 0;

        // load fixed-length structure (if any)
        tango::INodeValuePtr fixed_length_structure = table_node->getChild(L"fixed_length_structure", false);
        if (fixed_length_structure.isOk())
        {
            // read in fixed-length structure

            int field_counter;
            int field_count = fixed_length_structure->getChildCount();

            for (field_counter = 0; field_counter < field_count; ++field_counter)
            {
                wchar_t buf[255];
                swprintf(buf, 255, L"field%d", field_counter);

                // get field node
                tango::INodeValuePtr field_node = fixed_length_structure->getChild(buf, false);
                if (!field_node)
                    return false;

                // skip
                tango::INodeValuePtr skip = field_node->getChild(L"skip", false);
                if (!skip)
                    return false;

                // input type
                tango::INodeValuePtr input_offset = field_node->getChild(L"input_offset", false);
                if (!input_offset)
                    return false;

                // input width
                tango::INodeValuePtr input_width = field_node->getChild(L"input_width", false);
                if (!input_width)
                    return false;

                // output name
                tango::INodeValuePtr output_name = field_node->getChild(L"output_name", false);
                if (!output_name)
                    return false;

                // output type
                tango::INodeValuePtr output_type = field_node->getChild(L"output_type", false);
                if (!output_type)
                    return false;

                // output width
                tango::INodeValuePtr output_width = field_node->getChild(L"output_width", false);
                if (!output_width)
                    return false;

                // output scale
                tango::INodeValuePtr output_scale = field_node->getChild(L"output_scale", false);
                if (!output_scale)
                    return false;

                // trim leading spaces
                tango::INodeValuePtr trim_leading_spaces = field_node->getChild(L"trim_leading_spaces", false);
                if (!trim_leading_spaces)
                    return false;

                // trim leading zeros
                tango::INodeValuePtr trim_leading_zeros = field_node->getChild(L"trim_leading_zeros", false);
                if (!trim_leading_zeros)
                    return false;

                // decimal separator
                tango::INodeValuePtr decimal_separator = field_node->getChild(L"decimal_separator", false);
                if (!decimal_separator)
                    return false;

                // negative sign
                tango::INodeValuePtr negative_sign = field_node->getChild(L"negative_sign", false);
                if (!negative_sign)
                    return false;

                // date order
                tango::INodeValuePtr date_format = field_node->getChild(L"date_format", false);
                if (!date_format)
                    return false;

                // boolean format
                tango::INodeValuePtr boolean_format = field_node->getChild(L"boolean_format", false);
                if (!boolean_format)
                    return false;

                // custom expression
                tango::INodeValuePtr custom_expression = field_node->getChild(L"expression", false);
                if (!custom_expression)
                    return false;

                // hidden expression
                tango::INodeValuePtr hidden_expression = field_node->getChild(L"hidden_expression", false);
                if (!hidden_expression)
                    return false;


                FixedLengthField fs;
                fs.skip = skip->getBoolean();
                fs.input_offset = input_offset->getInteger();
                fs.input_width = input_width->getInteger();
                fs.output_name = towx(output_name->getString());
                fs.output_type = output_type->getInteger();
                fs.output_width = output_width->getInteger();
                fs.output_scale = output_scale->getInteger();
                fs.trim_leading_spaces = trim_leading_spaces->getBoolean();
                fs.trim_leading_zeros = trim_leading_zeros->getBoolean();
                fs.decimal_separator = towx(decimal_separator->getString());
                fs.negative_sign = towx(negative_sign->getString());
                fs.date_order = towx(date_format->getString());
                fs.boolean_format = towx(boolean_format->getString());
                fs.custom_expression = towx(custom_expression->getString());
                fs.hidden_expression = towx(hidden_expression->getString());

                ts.fixed_fields.push_back(fs);
            }
        }

        m_ii.tables.push_back(ts);
    }

    // field mappings base
    tango::INodeValuePtr field_mappings_base = data_root->getChild(L"field_mappings", false);
    if (!field_mappings_base)
        return false;

    int field_mapping_count = field_mappings_base->getChildCount();

    // try to load the field mappings that were saved

    int field_mapping_counter;
    for (field_mapping_counter = 0;
         field_mapping_counter < field_mapping_count;
         ++field_mapping_counter)
    {
        tango::INodeValuePtr field_mapping_node;
        field_mapping_node = field_mappings_base->getChildByIdx(field_mapping_counter);

        // show
        tango::INodeValuePtr is_shown = field_mapping_node->getChild(L"show", false);
        if (!is_shown)
            return false;

        // selected
        tango::INodeValuePtr selected = field_mapping_node->getChild(L"selected", false);
        if (!selected)
            return false;

        // input tablename
        tango::INodeValuePtr input_tn = field_mapping_node->getChild(L"input_tablename", false);
        if (!input_tn)
            return false;

        // output tablename
        tango::INodeValuePtr output_tn = field_mapping_node->getChild(L"output_tablename", false);
        if (!output_tn)
            return false;

        // append
        tango::INodeValuePtr append = field_mapping_node->getChild(L"append", false);
        if (!append)
            return false;

        // field mapping name
        tango::INodeValuePtr td_name = field_mapping_node->getChild(L"field_mapping_name", false);
        if (!td_name)
            return false;
        
        // pre-filter node
        tango::INodeValuePtr query = field_mapping_node->getChild(L"query", false);


        ImportTableSelection ts;
        ts.show = is_shown->getBoolean();
        ts.selected = selected->getBoolean();
        ts.input_tablename = towx(input_tn->getString());
        ts.output_tablename = towx(output_tn->getString());
        ts.append = append->getBoolean();
        ts.field_mapping_name = towx(td_name->getString());

        if (query.isOk())
        {
            ts.query = towx(query->getString());
        }

        // field count
        tango::INodeValuePtr output_fc = field_mapping_node->getChild(L"output_field_count", false);
        if (!output_fc)
            return false;


        int field_counter;
        int output_field_count = output_fc->getInteger();

        for (field_counter = 0; field_counter < output_field_count; ++field_counter)
        {
            wchar_t buf[255];
            swprintf(buf, 255, L"output_field%d", field_counter);

            // get field node
            tango::INodeValuePtr field_node = field_mapping_node->getChild(buf, false);
            if (!field_node)
                return false;

            // input name
            tango::INodeValuePtr field_in = field_node->getChild(L"input_name", false);
            if (!field_in)
                return false;

            // input type
            tango::INodeValuePtr field_it = field_node->getChild(L"input_type", false);
            if (!field_it)
                return false;

            // input width
            tango::INodeValuePtr field_iw = field_node->getChild(L"input_width", false);
            if (!field_iw)
                return false;

            // input scale
            tango::INodeValuePtr field_is = field_node->getChild(L"input_scale", false);
            if (!field_is)
                return false;

            // output name
            tango::INodeValuePtr field_on = field_node->getChild(L"output_name", false);
            if (!field_on)
                return false;

            // output type
            tango::INodeValuePtr field_ot = field_node->getChild(L"output_type", false);
            if (!field_ot)
                return false;

            // output width
            tango::INodeValuePtr field_ow = field_node->getChild(L"output_width", false);
            if (!field_ow)
                return false;

            // output scale
            tango::INodeValuePtr field_os = field_node->getChild(L"output_scale", false);
            if (!field_os)
                return false;

            // dynamic
            tango::INodeValuePtr field_dyn = field_node->getChild(L"dynamic", false);
            if (!field_dyn)
                return false;

            // dynamic state
            tango::INodeValuePtr field_dyn_state = field_node->getChild(L"dynamic_state", false);
            if (!field_dyn_state)
                return false;

            // expression
            tango::INodeValuePtr field_expression = field_node->getChild(L"expression", false);
            if (!field_expression)
                return false;

            FieldSelection fs;
            fs.input_name = towx(field_in->getString());
            fs.input_type = field_it->getInteger();
            fs.input_width = field_iw->getInteger();
            fs.input_scale = field_is->getInteger();
            fs.output_name = towx(field_on->getString());
            fs.output_type = field_ot->getInteger();
            fs.output_width = field_ow->getInteger();
            fs.output_scale = field_os->getInteger();
            fs.dynamic = field_dyn->getInteger();
            fs.dyn_state = field_dyn_state->getBoolean();
            fs.expression = towx(field_expression->getString());

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

static void onImportJobFinished(IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobStateFinished)
        return;

    g_app->getAppController()->refreshDbDoc();
}


IJobPtr ImportTemplate::execute()
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

        return static_cast<IJob*>(job);
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

    return static_cast<IJob*>(job);
}


