/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-29
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <ctime>
#include <xd/xd.h>
#include "xdfs.h"
#include "database.h"
#include "delimitedtext.h"
#include "delimitedtextset.h"
#include "delimitedtextiterator.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/extfileinfo.h"
#include "kl/md5.h"
#include "kl/regex.h"
#include "kl/string.h"

const int ROWS_TO_DETERMINE_STRUCTURE = 10000;


// DelimitedTextSet class implementation

DelimitedTextSet::DelimitedTextSet(FsDatabase* database)
{
    m_database = database;
    m_database->ref();
    
    m_source_structure = static_cast<xd::IStructure*>(new Structure);
    m_dest_structure = static_cast<xd::IStructure*>(new Structure);

    m_delimiters = L",";
    m_line_delimiters = L"\x0d\x0a";  // CR/LF
    m_text_qualifier = L"\"";
    m_first_row_column_names = false;
    m_discover_first_row_column_names = true;

}

DelimitedTextSet::~DelimitedTextSet()
{
    if (m_file.isOpen())
        m_file.closeFile();

    m_database->unref();
}

bool DelimitedTextSet::init(const std::wstring& filename)
{
    if (!m_file.openFile(filename))
        return false;

    // figure out the config file name
    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    m_configfile_path = ExtFileInfo::getConfigFilenameFromPath(definition_path, filename);
    
    // set the set info filename
    setConfigFilePath(m_configfile_path);

    // try to load the config file for this set
    
    if (loadConfigurationFromDataFile())
    {
        // structure config from data file succeeded
    }
     else if (loadConfigurationFromConfigFile())
    {
        // structure config from config file succeeded
    }
     else
    {
        // there is no saved configuration about this file, so
        // we have to run logic that tries to determine the best
        // way to open this file
        
        // look for an extension -- if no extension, assume csv
        std::wstring ext;
        int ext_pos = filename.find_last_of(L'.');
        if (ext_pos >= 0)
            ext = filename.substr(ext_pos);
             else
            ext = L".csv";
        kl::makeLower(ext);
        
        
        if (ext == L".tsv")
        {
            // these settings follows the "tsv" standard.  Note, if the file
            // has a .tsv extension, we simply assume without any further investigation
            // that the file is tab delimited.
            m_delimiters = L"\t";
            m_text_qualifier = L"";
            m_first_row_column_names = false;
        }
         else
        {
            // these settings follows the "csv" standard
            m_delimiters = L",";
            m_text_qualifier = L"\"";
            m_first_row_column_names = false;
            
            // however, many csv files also use other delimiters, like semicolons

            FsSetFormatInfo info;
            if (m_database->getSetFormat(filename,
                                         &info,
                                         FsSetFormatInfo::maskFormat |
                                         FsSetFormatInfo::maskDelimiters))
            {
                m_delimiters = info.delimiters;
            }
        }

        
        // make sure the text-delimited file class is updated
        m_file.setDelimiters(m_delimiters);
        m_file.setLineDelimiters(m_line_delimiters);
        m_file.setTextQualifiers(m_text_qualifier);


        int rows_to_check = ROWS_TO_DETERMINE_STRUCTURE;
        
        // if the file is small (<= 2MB), just read in the whole
        // file to make sure we have a good structure
        if (xf_get_file_size(filename) < 2000000)
            rows_to_check = -1;

        // originally, if we couldn't define the structure of the file, we
        // would bail out; now simply try to determine the structure, but
        // don't bail out if we can't determine it; the reason for this is
        // that sometimes we need to create a set and then define the
        // structure afterwards, such as when exporting to a text delimited
        // file; in this case, trying to determine the rows ahead of time
        // will cause the export to fail since the file we're exporting to
        // doesn't have rows until after the structure is set and the
        // row written out
        determineColumns(rows_to_check, NULL);
    }
    
    return true;
}


std::wstring DelimitedTextSet::getSetId()
{
    std::wstring set_id;
    
    set_id = L"xdfs:";
    set_id += xf_get_network_path(m_file.getFilename());
    
#ifdef WIN32
    // win32's filenames are case-insensitive, so
    // when generating the set id, make the whole filename
    // lowercase to avoid multiple id's for the same file
    kl::makeLower(set_id);
#endif
    
    return kl::md5str(set_id);
}

xd::IRowInserterPtr DelimitedTextSet::getRowInserter()
{
    DelimitedTextRowInserter* inserter = new DelimitedTextRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}

xd::IIteratorPtr DelimitedTextSet::createSourceIterator(xd::IJob* job)
{
    DelimitedTextIterator* iter = new DelimitedTextIterator;
    iter->setUseSourceIterator(true);
    if (!iter->init(m_database,
                    this,
                    m_file.getFilename()))
    {
        delete iter;
        return xcm::null;
    }
        
    return static_cast<xd::IIterator*>(iter);
}

xd::IIteratorPtr DelimitedTextSet::createIterator(const std::wstring& columns,
                                                     const std::wstring& order,
                                                     xd::IJob* job)
{
    if (order.empty())
    {
        DelimitedTextIterator* iter = new DelimitedTextIterator;
        if (!iter->init(m_database,
                        this,
                        m_file.getFilename()))
        {
            delete iter;
            return xcm::null;
        }
            
        return static_cast<xd::IIterator*>(iter);
    }
    
    // find out where the database should put temporary files
    IFsDatabasePtr fsdb = m_database;
    std::wstring temp_directory = fsdb->getTempFileDirectory();

    // create a unique index file name with .idx extension
    std::wstring index_filename = getUniqueString();
    index_filename += L".idx";

    // generate a full path name from the temp path and unique idx filename
    std::wstring full_index_filename;
    full_index_filename = makePathName(temp_directory,
                                       L"",
                                       index_filename);

    IIndex* idx;
    idx = createExternalIndex(m_database,
                              getObjectPath(),
                              full_index_filename,
                              temp_directory,
                              order,
                              true,
                              job);
    if (!idx)
    {
        return xcm::null;
    }

    xd::IIteratorPtr data_iter = createIterator(columns, L"", NULL);
    return createIteratorFromIndex(data_iter,
                                   idx,
                                   columns,
                                   order,
                                   getObjectPath());
}

xd::rowpos_t DelimitedTextSet::getRowCount()
{
    return 0;
}




static xd::IStructurePtr createDefaultDestinationStructure(xd::IStructurePtr source)
{
    xd::IStructurePtr s = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr struct_int = s;
    
    int i, col_count = source->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr sourcecol = source->getColumnInfoByIdx(i);
        
        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setName(sourcecol->getName());
        col->setType(sourcecol->getType());
        col->setWidth(sourcecol->getWidth());
        col->setScale(sourcecol->getScale());
        col->setExpression(sourcecol->getName());
        col->setColumnOrdinal(i);
        struct_int->addColumn(col);
    }
    
    return s;
}


// -- xd::IDelimitedTextSet interface --

bool DelimitedTextSet::loadConfigurationFromDataFile()
{
    m_file.rewind();
    if (m_file.eof())
        return false;
    
    std::vector<xd::IColumnInfoPtr> fields;
    
    size_t i, row_cell_count = m_file.getRowCellCount();
    for (i = 0; i < row_cell_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        fields.push_back(colinfo);
        
        std::wstring str = m_file.getString(i);
        kl::trim(str);
        size_t open = str.find_last_of('(');
        size_t close = str.find_last_of(')');
        
        if (open == str.npos || close == str.npos || open > close || close != str.length()-1)
            return false;
        
        std::wstring name = str.substr(0, open);
        std::wstring params_str = str.substr(open+1, close-open-1);
        
        kl::trim(name);
        kl::trim(params_str);
        kl::replaceStr(params_str, L"\t", L" ");
        
        std::vector<std::wstring> params;
        kl::parseDelimitedList(params_str, params, L' ');
        
        // remove all empty elements from the parameters array; 
        // this takes care of multiple whitespaces in the parameters string
        std::vector<std::wstring> temps  = params;
        params.clear();
        for (std::vector<std::wstring>::iterator it = temps.begin(); it != temps.end(); ++it)
        {
            if (it->length() > 0)
                params.push_back(*it);
        }
        
        if (params.size() == 0)
            return false; // no parameters specified
        
        colinfo->setName(name);
        
        if (params[0] == L"C")
        {
            if (params.size() < 2)
                return false;
            
            int width = kl::wtoi(params[1]);
            if (width < 1)
                return false;
            
            colinfo->setType(xd::typeCharacter);
            colinfo->setWidth(width);
        }
         else if (params[0] == L"N")
        {
            if (params.size() < 2)
                return false;
                
            int width = kl::wtoi(params[1]);
            if (width < 1)
                return false;
            
            colinfo->setType(xd::typeNumeric);
            colinfo->setWidth(width);
            
            if (params.size() >= 3)
            {
                int scale = kl::wtoi(params[2]);
                if (scale < 0)
                    return false;
                colinfo->setScale(scale);
            }
        }
         else if (params[0] == L"D")
        {
            colinfo->setType(xd::typeDate);
            colinfo->setWidth(4);
        }
         else if (params[0] == L"T")
        {
            colinfo->setType(xd::typeDateTime);
            colinfo->setWidth(8);
        }
         else if (params[0] == L"B")
        {
            colinfo->setType(xd::typeBoolean);
            colinfo->setWidth(1);
        }
    }
    

    // update the types and widths in the source structure
    m_source_structure.clear();
    m_source_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr src_struct_int = m_source_structure;

    for (i = 0; i < fields.size(); ++i)
    {        
        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setName(fields[i]->getName());
        col->setType(xd::typeCharacter);
        col->setScale(0);
        col->setExpression(L"");
        col->setColumnOrdinal(i);

        switch (fields[i]->getType())
        {
            default:
            case xd::typeCharacter:
            case xd::typeWideCharacter:
                col->setWidth(fields[i]->getWidth());
                break;
            case xd::typeDate:
                col->setWidth(50);
                break;
            case xd::typeNumeric:
                col->setWidth(50);
                break;
        }
        
        src_struct_int->addColumn(col);
    }
    modifySourceStructure(m_source_structure, NULL);

    populateColumnNameMatchVector();

    // make sure m_dest_structure is filled out
    m_dest_structure = createDefaultDestinationStructure(m_source_structure);
    
    // update the destination structure with the proper types we detected
    for (i = 0; i < fields.size(); ++i)
    {
        xd::IColumnInfoPtr col = m_dest_structure->getColumnInfoByIdx(i);
        col->setType(fields[i]->getType());
        col->setWidth(fields[i]->getWidth());
        col->setScale(fields[i]->getScale());
    }
    
    m_delimiters = L",";
    m_text_qualifier = L"\"";
    m_first_row_column_names = true;

    return true;
}



bool DelimitedTextSet::loadConfigurationFromConfigFile()
{
    // try to load the external text definition
    ExtFileInfo fileinfo;
    if (!fileinfo.load(m_configfile_path))
        return false;

    // find out if the we're dealing with the right type of text definition
    ExtFileEntry base = fileinfo.getGroup(L"file_info");
    std::wstring file_type = base.getChildContents(L"type");
    if (file_type != L"text/delimited" &&
        file_type != L"text/csv" &&
        file_type != L"text_delimited")
    {
        return false;
    }
    
    // -- the load succeeded, get the external file structure info --
    
    // get the main file settings
    ExtFileEntry entry = base.getChild(L"settings");
    m_delimiters = entry.getChildContents(L"field_delimiters");
    m_text_qualifier = entry.getChildContents(L"text_qualifier");
    int frfn = kl::wtoi(entry.getChildContents(L"first_row_field_names"));
    m_first_row_column_names = (frfn != 0) ? true : false;

    // make sure the text-delimited file class is updated
    m_file.setDelimiters(m_delimiters);
    m_file.setLineDelimiters(m_line_delimiters);
    m_file.setTextQualifiers(m_text_qualifier);

    // clear out any existing source structure in the set 
    m_colname_matches.clear();
    m_source_structure.clear();
    m_source_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr src_struct_int = m_source_structure;
    
    std::wstring name, expression, type_string;
    int type, width, scale;
    
    // now, get the source structure from the file
    entry = base.getChild(L"fields");
    int i, count = entry.getChildCount();
    for (i = 0; i < count; ++i)
    {
        ExtFileEntry field = entry.getChild(i);
        
        name = field.getChildContents(L"name");
        type_string = field.getChildContents(L"type");
        width = kl::wtoi(field.getChildContents(L"width"));

        int ftype = xd::typeCharacter;
        if (type_string.length() > 0)
        {
            if (type_string == L"wide_character")
                ftype = xd::typeWideCharacter;
        }

        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setName(name);
        col->setType(ftype);
        col->setWidth(width);
        col->setScale(0);
        col->setColumnOrdinal(i);
        src_struct_int->addColumn(col);
        
        // if the first row field names is not specified, that means the
        // definition's column names were the user-specified column names
        if (!m_first_row_column_names)
        {
            ColumnNameMatch col_match;
            col_match.user_name = name;
            m_colname_matches.push_back(col_match);
        }
    }

    // populate the column names matching vector
    // for first row column name switching
    populateColumnNameMatchVector();

    // clear out any existing destination structure in the set
    m_dest_structure.clear();
    m_dest_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr dest_struct_int = m_dest_structure;

    // now, get the destination structure from the file
    base = fileinfo.getGroup(L"set_info");
    entry = base.getChild(L"fields");
    count = entry.getChildCount();
    for (i = 0; i < count; ++i)
    {
        ExtFileEntry field = entry.getChild(i);
        
        name = field.getChildContents(L"name");
        type = kl::wtoi(field.getChildContents(L"type"));
        width = kl::wtoi(field.getChildContents(L"width"));
        scale = kl::wtoi(field.getChildContents(L"scale"));
        expression = field.getChildContents(L"expression");

        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setName(name);
        col->setType(type);
        col->setWidth(width);
        col->setScale(scale);
        col->setExpression(expression);
        col->setColumnOrdinal(i);
        dest_struct_int->addColumn(col);
    }

    return true;
}

bool DelimitedTextSet::saveConfiguration()
{
    std::wstring filename = m_file.getFilename();

    // if an external text definition exists, start from there
    ExtFileInfo fileinfo;
    fileinfo.load(m_configfile_path);
    
    // delete the existing "file_info" group and create a new one
    fileinfo.deleteGroup(L"file_info");
    ExtFileEntry base = fileinfo.getGroup(L"file_info");
    
    ExtFileEntry entry = base.addChild(L"type", L"text/delimited");
    entry = base.addChild(L"settings");
    entry.addChild(L"field_delimiters", m_delimiters);
    entry.addChild(L"text_qualifier", m_text_qualifier);
    entry.addChild(L"first_row_field_names", m_first_row_column_names ? 1 : 0);

    entry = base.addChild(L"fields");

    // save the source structure to the file
    xd::IStructurePtr s = getSourceStructure();
    int i,col_count = s->getColumnCount();
    
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        
        std::wstring type_string = L"character";
        if (colinfo->getType() == xd::typeWideCharacter)
            type_string = L"wide_character";
        
        ExtFileEntry field = entry.addChild(L"field");
        field.addChild(L"name", colinfo->getName());
        field.addChild(L"type", type_string);
        field.addChild(L"width", colinfo->getWidth());
    }
    
    // delete the existing "set_info" group and create a new one
    fileinfo.deleteGroup(L"set_info");
    base = fileinfo.getGroup(L"set_info");
    
    entry = base.addChild(L"fields");

    // now, save the destination structure to the file
    s = getDestinationStructure();
    col_count = s->getColumnCount();
    
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        
        ExtFileEntry field = entry.addChild(L"field");
        field.addChild(L"name", colinfo->getName());
        field.addChild(L"type", colinfo->getType());
        field.addChild(L"width", colinfo->getWidth());
        field.addChild(L"scale", colinfo->getScale());
        field.addChild(L"expression", colinfo->getExpression());
    }
    
    return fileinfo.save(m_configfile_path);
}

bool DelimitedTextSet::deleteConfiguration()
{
    return xf_remove(m_configfile_path);
}



void DelimitedTextSet::setCreateStructure(xd::IStructurePtr structure)
{
    // this function is used when creating a set.  When a set is created,
    // only the field headers are present in the file.  The newly created
    // file is closed and then reopened by FsDatabase::createTable().  Because
    // there is no data in the file, the file's structure cannot be reliably
    // determined.  This function allows FsDatabase::createTable() explicity
    // set up the structure of set.
    
    m_source_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr src_struct_int = m_source_structure;
    
    m_dest_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr dest_struct_int = m_dest_structure;

    int i, col_count = structure->getColumnCount();
    
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr orig_col = structure->getColumnInfoByIdx(i);
        if (orig_col->getCalculated())
            continue;
    
        int new_width = orig_col->getWidth();
        
        switch (orig_col->getType())
        {
            case xd::typeDate:
                new_width = 10;
                break;
            case xd::typeDateTime:
                new_width = 20;
                break;
            case xd::typeInteger:
                new_width = 12; // size of 2^32 + sign + 1 to round to 12
                break;
            case xd::typeDouble:
                new_width = 20;
                break;
            case xd::typeNumeric:
                new_width += 2; // dec place and sign
                break;
        }
    
    
        xd::IColumnInfoPtr src_col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        src_col->setName(orig_col->getName());
        src_col->setType(xd::typeCharacter);
        src_col->setWidth(new_width);
        src_col->setScale(0);
        src_col->setColumnOrdinal(i);
        src_struct_int->addColumn(src_col);
        
        
        
        xd::IColumnInfoPtr dest_col = orig_col->clone();
        dest_col->setExpression(src_col->getName());
        dest_col->setColumnOrdinal(i);
        dest_struct_int->addColumn(dest_col);
    }
    
    // add the calculated fields
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr orig_col = structure->getColumnInfoByIdx(i);
        if (!orig_col->getCalculated())
            continue;

        createCalcField(orig_col->clone());
    }
}





xd::IStructurePtr DelimitedTextSet::getSourceStructure()
{
    xd::IStructurePtr s = m_source_structure->clone();
    return s;
}

xd::IStructurePtr DelimitedTextSet::getDestinationStructure()
{
    if (m_dest_structure->getColumnCount() == 0)
    {
        m_dest_structure = createDefaultDestinationStructure(m_source_structure);
    }
    
    xd::IStructurePtr s = m_dest_structure->clone();
    return s;
}

xd::IStructurePtr DelimitedTextSet::getStructure()
{
    xd::IStructurePtr s = getDestinationStructure();
    int i, col_count = s->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        colinfo->setExpression(L"");
    }

    CommonBaseSet::appendCalcFields(s);
    return s;
}

inline void resetColumnOrdinals(xd::IStructurePtr s)
{
    int i, col_count = s->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        
        if (colinfo->getColumnOrdinal() != i)
            colinfo->setColumnOrdinal(i);
    }
}

bool DelimitedTextSet::renameSourceColumn(const std::wstring& source_col,
                                          const std::wstring& new_val)
{
    xd::IStructurePtr dest_struct = getDestinationStructure();
    xd::IStructurePtr src_struct = getSourceStructure();

    xd::IColumnInfoPtr colinfo;
    xd::IColumnInfoPtr modinfo;

    bool source_retval, dest_retval;
    
    // rename the source field
    modinfo = src_struct->modifyColumn(source_col);
    modinfo->setName(new_val);
    source_retval = modifySourceStructure(src_struct, xcm::null);
    
    // update the column name matching vector
    std::vector<ColumnNameMatch>::iterator it;
    for (it = m_colname_matches.begin(); it != m_colname_matches.end(); ++it)
    {
        if (m_first_row_column_names && it->file_name == source_col)
            it->file_name = new_val;
            
        if (!m_first_row_column_names && it->user_name == source_col)
            it->user_name = new_val;
    }
    
    // now, lookup and modify any columns in the destination
    // structure that have a corresponding expression
    int i, dest_colcount = m_dest_structure->getColumnCount();
    for (i = 0; i < dest_colcount; ++i)
    {
        colinfo = dest_struct->getColumnInfoByIdx(i);
        
        if (colinfo->getExpression() == source_col)
        {
            modinfo = dest_struct->modifyColumn(colinfo->getName());
            modinfo->setExpression(new_val);
            
            if (colinfo->getName() == source_col)
                modinfo->setName(new_val);
        }
    }
    
    dest_retval = modifyDestinationStructure(dest_struct, xcm::null);
    
    return (source_retval && dest_retval);
}

bool DelimitedTextSet::modifySourceStructure(xd::IStructure* struct_config,
                                             xd::IJob* job)
{
    // get the structure actions from the
    // structure configuration that was passed
    IStructureInternalPtr struct_int = struct_config;
    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    int processed_action_count = 0;

    // make sure we actually modify the stored source structure
    struct_int = m_source_structure;

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        struct_int->removeColumn(it->m_colname);
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        struct_int->modifyColumn(it->m_colname,
                                 it->m_params->getName(),
                                 it->m_params->getType(),
                                 it->m_params->getWidth(),
                                 it->m_params->getScale(),
                                 it->m_params->getExpression(),
                                 it->m_params->getOffset(),
                                 it->m_params->getEncoding(),
                                 -1 /* no column moving */);
    }

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setName(it->m_params->getName());
        col->setType(it->m_params->getType());
        col->setWidth(it->m_params->getWidth());
        col->setScale(it->m_params->getScale());
        col->setExpression(it->m_params->getExpression());
        col->setOffset(it->m_params->getOffset());
        struct_int->addColumn(col);
    }

    // handle insert
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        int insert_idx = it->m_pos;
        
        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setName(it->m_params->getName());
        col->setType(it->m_params->getType());
        col->setWidth(it->m_params->getWidth());
        col->setScale(it->m_params->getScale());
        col->setExpression(it->m_params->getExpression());
        col->setOffset(it->m_params->getOffset());
        struct_int->internalInsertColumn(col, insert_idx);
    }
    
    // ensure that the each column has the proper column ordinal
    resetColumnOrdinals(m_source_structure);
    return true;
}

bool DelimitedTextSet::modifyDestinationStructure(xd::IStructure* struct_config,
                                                  xd::IJob* job)
{
    // get the structure actions from the
    // structure configuration that was passed
    IStructureInternalPtr struct_int = struct_config;
    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    int processed_action_count = 0;

    // make sure we actually modify the stored structure
    struct_int = m_dest_structure;

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        struct_int->removeColumn(it->m_colname);
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        xd::IColumnInfoPtr colinfo;
        colinfo = m_dest_structure->getColumnInfo(it->m_colname);
        
        struct_int->modifyColumn(it->m_colname,
                                 it->m_params->getName(),
                                 it->m_params->getType(),
                                 it->m_params->getWidth(),
                                 it->m_params->getScale(),
                                 it->m_params->getExpression(),
                                 it->m_params->getOffset(),
                                 it->m_params->getEncoding(),
                                 -1 /* no column moving */);
        
        // NOTE: it'd be nice to change this at some point,
        //       but not this close to release (02/01/2007)
        //       and on such a base-level function
        
        // we have to do this because the modColumn() function which is
        // buried in the internal structure class assigns the column to be
        // calculated if there is any expression present (and since we use
        // the expression to reference the source structure, this includes us)
        if (colinfo)
            colinfo->setCalculated(false);
    }

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setName(it->m_params->getName());
        col->setType(it->m_params->getType());
        col->setWidth(it->m_params->getWidth());
        col->setScale(it->m_params->getScale());
        col->setExpression(it->m_params->getExpression());
        col->setOffset(it->m_params->getOffset());
        struct_int->addColumn(col);
    }

    // handle insert
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        int insert_idx = it->m_pos;
        
        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setName(it->m_params->getName());
        col->setType(it->m_params->getType());
        col->setWidth(it->m_params->getWidth());
        col->setScale(it->m_params->getScale());
        col->setExpression(it->m_params->getExpression());
        col->setOffset(it->m_params->getOffset());
        struct_int->internalInsertColumn(col, insert_idx);
    }
    
    // ensure that the each column has the proper column ordinal
    resetColumnOrdinals(m_dest_structure);
    return true;
}

bool DelimitedTextSet::modifyStructure(xd::IStructure* struct_config,
                                       xd::IJob* job)
{
    bool done_flag = false;
    CommonBaseSet::modifyStructure(struct_config, &done_flag);
    return true;
}

void DelimitedTextSet::setDelimiters(const std::wstring& new_val,
                                     bool refresh_structure)
{
    if (new_val != m_delimiters)
    {
        m_delimiters = new_val;

        // pass through file metadata to the DelimitedTextFile class;
        // this sets the delimiter on a file in cases, such as data
        // export to a delimited file, when the refresh structure
        // shouldn't be called because the exported file isn't yet
        // created
        m_file.setDelimiters(m_delimiters);
        
        if (refresh_structure)
            determineColumns(ROWS_TO_DETERMINE_STRUCTURE, NULL);
    }
}

std::wstring DelimitedTextSet::getDelimiters()
{
    return m_delimiters;
}

void DelimitedTextSet::setLineDelimiters(const std::wstring& new_val,
                                         bool refresh_structure)
{
    if (new_val != m_line_delimiters)
    {
        m_line_delimiters = new_val;

        // pass through file metadata to the DelimitedTextFile class;
        // this sets the delimiter on a file in cases, such as data
        // export to a delimited file, when the refresh structure
        // shouldn't be called because the exported file isn't yet
        // created
        m_file.setLineDelimiters(m_line_delimiters);
        
        if (refresh_structure)
            determineColumns(ROWS_TO_DETERMINE_STRUCTURE, NULL);
    }
}

std::wstring DelimitedTextSet::getLineDelimiters()
{
    return m_line_delimiters;
}

void DelimitedTextSet::setTextQualifier(const std::wstring& new_val,
                                        bool refresh_structure)
{
    if (new_val != m_text_qualifier)
    {
        m_text_qualifier = new_val;

        // pass through file metadata to the DelimitedTextFile class;
        // this sets the qualifier on a file in cases, such as data
        // export to a delimited file, when the refresh structure
        // shouldn't be called because the exported file isn't yet
        // created
        m_file.setTextQualifiers(m_text_qualifier);

        if (refresh_structure)
            determineColumns(ROWS_TO_DETERMINE_STRUCTURE, NULL);
    }
}

std::wstring DelimitedTextSet::getTextQualifier()
{
    return m_text_qualifier;
}

void DelimitedTextSet::setDiscoverFirstRowColumnNames(bool new_val)
{
    m_discover_first_row_column_names = new_val;
}

void DelimitedTextSet::setFirstRowColumnNames(bool new_val)
{
    m_first_row_column_names = new_val;
    updateColumnNames();
}

bool DelimitedTextSet::isFirstRowColumnNames()
{
    return m_first_row_column_names;
}

static std::wstring makeValidFieldName(const std::wstring& name,
                                       const std::wstring& keyword_list,
                                       const std::wstring& invalid_col_chars)
{
    static std::vector<std::wstring> keywords;
    if (keywords.size() == 0)
    {
        std::vector<std::wstring> vec;
        kl::parseDelimitedList(keyword_list, vec, L',');

        size_t i, count = vec.size();
        for (i = 0; i < count; ++i)
            keywords.push_back(vec[i]);
    }
    
    const wchar_t* bad_col_chars = invalid_col_chars.c_str();
    wchar_t* buf = wcsdup(name.c_str());
    wchar_t* ch = buf;
    while (*ch)
    {
        if (wcschr(bad_col_chars, *ch))
            *ch = L'_';
        ++ch;
    }

    std::wstring result = buf;
    free(buf);
    
    // make sure the field name is not a keyword
    size_t i, count = keywords.size();
    for (i = 0; i < count; ++i)
    {
        if (0 == wcscasecmp(result.c_str(), keywords[i].c_str()))
        {
            result += L"_";
            break;
        }
    }
    
    
    return result;
}


// determineFirstRowHeader() tries to determine if the first
// row contains field names. determineColumns*() should be called
// after this function to find out field widths

bool DelimitedTextSet::determineFirstRowHeader()
{
    // go to the beginning of the file
    m_file.rewind();
            
    size_t i, cell_count = m_file.getRowCellCount();
    for (i = 0; i < cell_count; ++i)
    {
        std::wstring text = m_file.getString(i);
        kl::trim(text);
        
        // if a cell is empty, the first row is
        // probably not a list of field names
        if (text.empty())
            return false;
        
        // if a cell starts with a numeric digit, the first
        // row is probably not a list of field names
        if (wcschr(L"0123456789!@#$%^&*()-+=:;<>,./?'\"`\\", text[0]))
            return false;
    }
    
    return true;
}


static bool isDelimitedStringNumeric(const std::wstring& str, int* num_decimals)
{
    int i, len = (int)str.length();
    if (len == 0)
        return false;
    
    int num_dec = 0;
    bool found_dec = false;
    
    int sign_count = 0;
    int dollarsign_count = 0;
    int period_count = 0;
    
    wchar_t ch;
    for (i = len-1; i >= 0; --i)
    {
        ch = str[i];
        if (ch == '.')
            found_dec = true;
        if (iswdigit((unsigned char)ch))
        {
            if (!found_dec)
                num_dec++;
            continue;
        }
        
        if (ch == '-' || ch == '+')
        {
            if (++sign_count > 1)
                return false;
        }
         else if (ch == '$')
        {
            if (++dollarsign_count > 1)
                return false;
        }
         else if (ch == '.')
        {
            if (++period_count > 1)
                return false;
        }
         else if (ch == ',' || ch == '%')
        {
            // these chars are allowed in numbers
        }
         else
        {
            return false;
        }
    }
    
    if (!found_dec)
        num_dec = 0;
        
    if (num_decimals)
        *num_decimals = num_dec;
    
    return true;
}





static bool extractMatchResults(klregex::wmatch& matchres,
                                int* yy,
                                int* mm,
                                int* dd)
{
    const klregex::wsubmatch& year_match = matchres[L"year"];
    const klregex::wsubmatch& month_match = matchres[L"month"];
    const klregex::wsubmatch& day_match = matchres[L"day"];

    if (!year_match.isValid())
        return false;
    if (!month_match.isValid())
        return false;
    if (!day_match.isValid())
        return false;
    
    static const wchar_t* months[] = { L"JAN", L"FEB", L"MAR",
                                       L"APR", L"MAY", L"JUN",
                                       L"JUL", L"AUG", L"SEP",
                                       L"OCT", L"NOV", L"DEC" };
    if (yy)
    {
        *yy = kl::wtoi(year_match.str().c_str());
        if (*yy < 100)
        {
            if (*yy < 70)
                *yy += 2000;
                 else
                *yy += 1900;
        }
    }

    if (mm)
    {
        std::wstring month = month_match.str();
        if (month.length() == 0)
            return false;
            
        if (iswdigit(month[0]))
        {
            *mm = kl::wtoi(month.c_str());
        }
         else
        {            
            int j;
            bool found = false;
            for (j = 0; j < 12; ++j)
            {
                if (0 == wcsncasecmp(month.c_str(), months[j], 3))
                {
                    *mm = j+1;
                    found = true;
                    break;
                }
            }
            
            if (!found)
            {
                // unknown month name
                return false;
            }
        }
    }
    
    if (dd)
    {
        *dd = kl::wtoi(day_match.str().c_str());
    }

    return true;
}


bool parseDelimitedStringDate(const std::wstring& str,
                              int* year,
                              int* month,
                              int* day)
{
    if (str.length() == 0)
        return false;

    if (year)
    {
        *year = 0;
        *month = 0;
        *day = 0;
    }
    
    // CCYY-MM-DD this format is so commonly a date format that
    // we don't need to do further validation.  It does however check to make
    // sure the first digit is between 0 and 3 (should be enough for years 0 - 3999)
    static const klregex::wregex fmt1(L"(?<year>[0-3]\\d{3})[-/. ](?<month>\\d{1,2})[-/. ](?<day>\\d{1,2})");
    
    // this format checks for CCYYMMDD, but does numerical range checking on the month and day portions
    static const klregex::wregex fmt2(L"(?<year>[[0-9]{4})(?<month>0[1-9]|1[0-2])(?<day>0[1-9]|[12][0-9]|3[01])");

    // this format checks for MM/DD/CCYY
    static const klregex::wregex fmt3(L"(?<month>0?[1-9]|1[0-2])[-/. ](?<day>0?[1-9]|[12][0-9]|3[01])[-/. ](?<year>[0-9]{2,4})");

    // this format checks for DD/MM/CCYY
    static const klregex::wregex fmt4(L"(?<day>0?[1-9]|[12][0-9]|3[01])[-/. ](?<month>0?[1-9]|1[0-2])[-/. ](?<year>[0-9]{2,4})");

    // this format checks for DD-MMM-CCYY
    static const klregex::wregex fmt5(L"(?<day>0?[1-9]|[12][0-9]|3[01])[-/., ]+(?<month>\\w+)[-/., ]+(?<year>[0-9]{2,4})");
    
    // this format checks for MMM DD CCYY
    static const klregex::wregex fmt6(L"(?i)(?<month>jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)\\w*[-/., ]+(?<day>0?[1-9]|[12][0-9]|3[01])[-/., ]+(?<year>[0-9]{2,4})");

    klregex::wmatch matchres;
    
    const wchar_t* start = str.c_str();
    const wchar_t* end = start + str.length();
    
    if (fmt1.match(start, end, matchres) || 
        fmt2.match(start, end, matchres))
    {
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt3.match(start, end, matchres))
    {
        // using MDY order
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt4.match(start, end, matchres))
    {
        // using YMD order
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt5.match(start, end, matchres) ||
        fmt6.match(start, end, matchres)) 
    {
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
  
    return false;
}



struct DetermineColumnInfo
{
    int type;
    int max_width;
    int max_scale;
    
    DetermineColumnInfo()
    {
        type = xd::typeDate;
        max_width = 1;
        max_scale = 0;
    }
    
    DetermineColumnInfo(const DetermineColumnInfo& c)
    {
        type = c.type;
        max_width = c.max_width;
        max_scale = c.max_scale;
    }
    
    DetermineColumnInfo& operator=(const DetermineColumnInfo& c)
    {
        type = c.type;
        max_width = c.max_width;
        max_scale = c.max_scale;
        return *this;
    }
};


bool DelimitedTextSet::determineColumns(int check_rows, xd::IJob* job)
{
    // if field information is stored in the header row, no need to sense structure
    if (loadConfigurationFromDataFile())
        return true;
    
    IJobInternalPtr ijob = job;
    if (job)
    {
        ijob->setStartTime(time(NULL));
        ijob->setStatus(xd::jobRunning);
        ijob->setCanCancel(true);
        ijob->setCurrentCount(0);
        ijob->setMaxCount((check_rows != -1) ? check_rows : 0);
    }
    
    // clear out any existing structure
    m_source_structure.clear();
    m_dest_structure.clear();
    m_source_structure = static_cast<xd::IStructure*>(new Structure);
    m_dest_structure = static_cast<xd::IStructure*>(new Structure);

    // pass through file metadata to the DelimitedTextFile class
    m_file.setDelimiters(m_delimiters);
    m_file.setLineDelimiters(m_line_delimiters);
    m_file.setTextQualifiers(m_text_qualifier);


    // determine if the first row is a header, set m_first_row_column_names
    if (m_discover_first_row_column_names)
        m_first_row_column_names = determineFirstRowHeader();
    
    // get the database keywords and invalid column characters
    // for use in the makeValidFieldName() function below
    
    xd::IAttributesPtr attr = m_database->getAttributes();
    if (attr.isNull())
        return false;
    
    std::wstring keyword_list;
    keyword_list = attr->getStringAttribute(xd::dbattrKeywords);
    
    std::wstring invalid_col_chars;
    invalid_col_chars = attr->getStringAttribute(xd::dbattrColumnInvalidChars);
    
    bool cancelled = false;
    int i, j, width, col_count = 0;
    int row_cell_count, rows_read = 0;
    std::vector<DetermineColumnInfo> col_stats;
    std::wstring colname;
    IStructureInternalPtr src_struct_int = m_source_structure;
    
    // go to the beginning of the file
    m_file.rewind();

    // scan through the file to gather column information
    while (!m_file.eof())
    {
        row_cell_count = m_file.getRowCellCount();

        // the current row has more cells than any of the preceeding rows
        while (col_count < row_cell_count)
        {
            wchar_t temps[81];
            swprintf(temps, 81, L"Field%d", col_count+1);

            colname = L"";
            
            // get the first row's cell values for possible column names
            if (rows_read == 0)
            {
                colname = m_file.getString(col_count);
                if (colname.length() > 80)
                    colname = colname.substr(0, 80);
            }
            
            // make sure we don't have any empty column names
            if (colname.empty())
                colname = temps;
            
            // set the column's name based on the m_first_row_column_names flag
            if (m_first_row_column_names)
                colname = makeValidFieldName(colname, keyword_list, invalid_col_chars);
                 else
                colname = temps;
            
            xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
            col->setName(colname);
            col->setType(m_file.isUnicode() ? xd::typeWideCharacter : xd::typeCharacter);
            col->setWidth(1);
            col->setScale(0);
            col->setColumnOrdinal(col_count);
            src_struct_int->addColumn(col);
            
            col_stats.push_back(DetermineColumnInfo());
            col_count++;
        }
        
        for (j = 0; j < row_cell_count; ++j)
        {
            const std::wstring& str = m_file.getString(j);
            width = str.length();

            // note: extra blank lines in a delimited file can cause the field
            // type of the first row to be changed to character (e.g. if the 
            // first field is comprised of dates, extra blank lines at the end
            // will cause this field to be interpreted as a character field);
            // ignore blank lines in determining the structure
            if (row_cell_count <= 1 && width == 0)
                continue;

            // if the width of this cell is greater than the column width
            // in the structure, update the column width accordingly
            if (width > col_stats[j].max_width)
                col_stats[j].max_width = width;
            
            // if the first row contains column names, don't use it during
            // statistical gathering of column widths and types
            if (m_first_row_column_names == false || rows_read > 0)
            {
                int scale = 0;
                
                if (col_stats[j].type == xd::typeDate)
                {
                    // check if the field value is a date
                    if (!parseDelimitedStringDate(str))
                    {
                        // if not, then fall back to a numeric
                        col_stats[j].type = xd::typeNumeric;
                    }
                }
                
                if (col_stats[j].type == xd::typeNumeric)
                {
                    if (isDelimitedStringNumeric(str, &scale))
                    {
                        // see if our max_scale needs to be increased
                        if (scale > col_stats[j].max_scale)
                            col_stats[j].max_scale = scale;
                    }
                     else
                    {
                        // field is no longer numeric 
                        col_stats[j].type = (m_file.isUnicode() ? xd::typeWideCharacter : xd::typeCharacter);
                        col_stats[j].max_scale = 0;
                    }
                }
            }
        }

        // stop iterating through the rows if we've read through
        // the required number of rows to determing the structure
        rows_read++;
        if ((check_rows != -1) && (rows_read >= check_rows))
            break;

        if (job && rows_read % 1000 == 0)
        {
            ijob->setCurrentCount(rows_read);

            if (job->getCancelled())
            {
                cancelled = true;
                break;
            }
        }

        m_file.skip(1);
    }

    // if we still don't have any columns, we're in trouble, bail out
    if (col_count < 1)
        return false;
    
    if (check_rows != -1 || cancelled)
    {
        col_count = (int)col_stats.size();
        
        // expand all field widths by 20% just for good measure
        for (i = 0; i < col_count; ++i)
        {
            col_stats[i].max_width = (int)(ceil((double)(col_stats[i].max_width) * 1.2));
            if (col_stats[i].max_width <= 0)
                col_stats[i].max_width = 1;
        }
    }
     else
    {
        // make sure there are no zero-sized columns
        for (i = 0; i < col_count; ++i)
        {
            if (col_stats[i].max_width <= 0)
                col_stats[i].max_width = 1;
        }
    }

    // check numeric-typed fields for width/scale conformance
    for (i = 0; i < col_count; ++i)
    {
        if (col_stats[i].type == xd::typeNumeric)
        {
            if (col_stats[i].max_width > xd::max_numeric_width)
            {
                col_stats[i].type = xd::typeDouble;
            }
        }
        
        if (col_stats[i].type == xd::typeNumeric ||
            col_stats[i].type == xd::typeDouble)
        {
            if (col_stats[i].max_scale > xd::max_numeric_scale)
            {
                col_stats[i].max_scale = xd::max_numeric_scale;
            }
        }
    }


    if (job && !cancelled)
    {
        ijob->setCurrentCount(rows_read);
        ijob->setMaxCount(rows_read);
        ijob->setFinishTime(time(NULL));
        ijob->setStatus(xd::jobFinished);
    }

    // update the types and widths in the source structure
    col_count = m_source_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr col = m_source_structure->getColumnInfoByIdx(i);
        col->setWidth(col_stats[i].max_width);
    }
    modifySourceStructure(m_source_structure, NULL);
    
    // populate the column names vector for first row column name switching
    populateColumnNameMatchVector();
    
    // make sure m_dest_structure is filled out
    m_dest_structure = createDefaultDestinationStructure(m_source_structure);
    
    // update the destination structure with the proper types we detected
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr col = m_dest_structure->getColumnInfoByIdx(i);
        col->setType(col_stats[i].type);
        if (col_stats[i].type == xd::typeNumeric)
        {
            col->setScale(col_stats[i].max_scale);
        }
    }
    
    // once again, go back to the beginning of the file
    m_file.rewind();
    return true;
}

void DelimitedTextSet::populateColumnNameMatchVector()
{
    size_t i, col_count = (size_t)m_source_structure->getColumnCount();
    
    // populate the user-specified column names vector
    while (m_colname_matches.size() < col_count)
    {
        wchar_t temps[81];
        swprintf(temps, 81, L"Field%d", m_colname_matches.size()+1);
        
        ColumnNameMatch col_match;
        col_match.user_name = temps;
        m_colname_matches.push_back(col_match);
    }
    
    // get the database keywords and invalid column characters
    // for use in the makeValidFieldName() function below
    
    xd::IAttributesPtr attr = m_database->getAttributes();
    if (attr.isNull())
        return;
    
    std::wstring keyword_list;
    keyword_list = attr->getStringAttribute(xd::dbattrKeywords);
    
    std::wstring invalid_col_chars;
    invalid_col_chars = attr->getStringAttribute(xd::dbattrColumnInvalidChars);
    
    // populate the first row column names vector
    m_file.rewind();
    int cell_count = m_file.getRowCellCount();
    std::wstring colname;
    
    for (i = 0; i < col_count; ++i)
    {
        wchar_t temps[81];
        swprintf(temps, 81, L"Field%d", i+1);

        // use the first row's cells unless the index is past the cell count,
        // then use default column names instead of empty strings
        if (i < (size_t)cell_count)
        {
            colname = m_file.getString(i);
            if (colname.length() > 80)
                colname = colname.substr(0, 80);
        }
         else
        {
            colname = temps;
        }
        
        // we can't allow empty column names
        if (colname.empty())
            colname = temps;
        
        colname = makeValidFieldName(colname, keyword_list, invalid_col_chars);
        
        if (i < m_colname_matches.size())
        {
            m_colname_matches[i].file_name = colname;
        }
         else
        {
            ColumnNameMatch col_match;
            col_match.file_name = colname;
            m_colname_matches.push_back(col_match);
        }
    }
    
    // go to the beginning of the file
    m_file.rewind();
    
    // make sure the user_name value is filled out for every item
    i = 0;
    std::vector<ColumnNameMatch>::iterator it;
    for (it = m_colname_matches.begin(); it != m_colname_matches.end(); ++it)
    {
        wchar_t temps[81];
        swprintf(temps, 81, L"Field%d", i+1);

        if (it->user_name.length() == 0)
            it->user_name = temps;
        
        i++;
    }
    
    // get rid of any extraneous column name information
    m_colname_matches.resize(col_count);
}

void DelimitedTextSet::updateColumnNames()
{
    xd::IStructurePtr dest_struct = getDestinationStructure();
    xd::IStructurePtr src_struct = getSourceStructure();
    
    int dest_colcount = dest_struct->getColumnCount();
    int src_colcount = src_struct->getColumnCount();
    
    xd::IColumnInfoPtr dest_colinfo, src_colinfo;
    std::wstring name, new_name;
    
    std::vector<std::wstring> old_names;
    int i;

    // if we don't have a column match vector filled out (such
    // as when we create a csv without a header row, this function
    // shouldn't be run (as far as I can tell)

    if (m_colname_matches.size() != src_colcount)
        return;

    // switch all of the column names to their unique match name
    for (i = 0; i < src_colcount; ++i)
    {
        if (m_first_row_column_names)
            name = m_colname_matches[i].user_name;
             else
            name = m_colname_matches[i].file_name;

        new_name = m_colname_matches[i].match_name;

        // modify the source column name
        src_colinfo = src_struct->modifyColumn(name);
        if (src_colinfo.isOk())
            src_colinfo->setName(new_name);
        
        // save the old fieldnames for later (for reference
        // when changing the destination structure later on)
        old_names.push_back(name);
    }
    
    modifySourceStructure(src_struct, xcm::null);
    
    // we have to "re-get" the source structure here
    // to make sure we have the "updated" structure
    src_struct = getSourceStructure();

    // switch all of the column names to their new name
    for (i = 0; i < src_colcount; ++i)
    {
        name = m_colname_matches[i].match_name;
        
        if (m_first_row_column_names)
            new_name = m_colname_matches[i].file_name;
             else
            new_name = m_colname_matches[i].user_name;

        // modify the source column name
        src_colinfo = src_struct->modifyColumn(name);
        if (src_colinfo.isOk())
            src_colinfo->setName(new_name);
        
        // the fieldname we're changing from
        std::wstring old_name = old_names[i];
        
        // lookup any corresponding columns in the destination
        // structure and modify them as well
        for (int j = 0; j < dest_colcount; ++j)
        {
            dest_colinfo = dest_struct->getColumnInfoByIdx(i);
            if (dest_colinfo.isNull())
                continue;
            
            // the destination expression is the same as the
            // source column name we're modifying, so continue
            if (dest_colinfo->getExpression() == old_name)
            {
                // modify the destination expression and name (if applicable)
                std::wstring dest_colname = dest_colinfo->getName();
                dest_colinfo = dest_struct->modifyColumn(dest_colname);
                dest_colinfo->setExpression(new_name);
                if (dest_colname == old_name)
                    dest_colinfo->setName(new_name);
            }
        }
    }
    
    modifySourceStructure(src_struct, xcm::null);
    modifyDestinationStructure(dest_struct, xcm::null);
}




// -- DelimitedTextRowInserter class implementation --

DelimitedTextRowInserter::DelimitedTextRowInserter(DelimitedTextSet* set)
{
    m_set = set;
    m_set->ref();

    m_file = &(m_set->m_file);
    m_inserting = false;
}

DelimitedTextRowInserter::~DelimitedTextRowInserter()
{
    // free all insert field data
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    m_set->unref();
}

xd::objhandle_t DelimitedTextRowInserter::getHandle(const std::wstring& column_name)
{
    if (!m_inserting)
        return (xd::objhandle_t)0;
    
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), column_name.c_str()))
            return (xd::objhandle_t)(*it);
    }

    return (xd::objhandle_t)0;
}

xd::IColumnInfoPtr DelimitedTextRowInserter::getInfo(xd::objhandle_t column_handle)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return xcm::null;

    // create new xd::IColumnInfoPtr
    xd::IColumnInfoPtr col_info = static_cast<xd::IColumnInfo*>(new ColumnInfo);
    col_info->setName(f->name);
    col_info->setType(f->type);
    col_info->setWidth(f->width);
    col_info->setScale(f->scale);
    col_info->setColumnOrdinal(f->ordinal);
    return col_info;
}

bool DelimitedTextRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                         const unsigned char* value,
                                         int length)
{
    return false;
}

bool DelimitedTextRowInserter::putString(xd::objhandle_t column_handle,
                                         const std::string& value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putString(f->ordinal, kl::towstring(value));
}

bool DelimitedTextRowInserter::putWideString(xd::objhandle_t column_handle,
                                             const std::wstring& value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putString(f->ordinal, value);
}

bool DelimitedTextRowInserter::putDouble(xd::objhandle_t column_handle,
                                         double value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putDouble(f->ordinal, value);
}

bool DelimitedTextRowInserter::putInteger(xd::objhandle_t column_handle,
                                          int value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putInteger(f->ordinal, value);
}

bool DelimitedTextRowInserter::putBoolean(xd::objhandle_t column_handle,
                                          bool value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putBoolean(f->ordinal, value);
}

bool DelimitedTextRowInserter::putDateTime(xd::objhandle_t column_handle,
                                           xd::datetime_t value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    if (value == 0)
    {
        // empty date
        return m_row.putDateTime(f->ordinal, L"");
    }
    
    xd::DateTime dt(value);

    int y = dt.getYear();
    int m = dt.getMonth();
    int d = dt.getDay();
    int hh = dt.getHour();
    int mm = dt.getMinute();
    int ss = dt.getSecond();

    wchar_t buf[64];

    if (hh == 0 && mm == 0 && ss == 0)
    {
        swprintf(buf, 64, L"%04d/%02d/%02d", y, m, d);
    }
     else
    {
        swprintf(buf, 64, L"%04d/%02d/%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
    }

    return m_row.putDateTime(f->ordinal, buf);
}

bool DelimitedTextRowInserter::putRowBuffer(const unsigned char* value)
{
    return false;
}

bool DelimitedTextRowInserter::putNull(xd::objhandle_t column_handle)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putNull(f->ordinal);
}

bool DelimitedTextRowInserter::startInsert(const std::wstring& col_list)
{
    xd::IStructurePtr s = m_set->getStructure();
    
    int i, col_count = s->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        
        DelimitedTextDataAccessInfo* f = new DelimitedTextDataAccessInfo;
        f->name = colinfo->getName();
        f->type = colinfo->getType();
        f->width = colinfo->getWidth();
        f->scale = colinfo->getScale();
        f->ordinal = colinfo->getColumnOrdinal();
        f->nulls_allowed = colinfo->getNullsAllowed();
        f->expr_text = colinfo->getExpression();
        m_fields.push_back(f);
    }

    m_inserting = true;
    return m_file->startInsert();
}

bool DelimitedTextRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    bool res = m_file->insertRow(m_row);
    
    m_row.clear();
    
    return res;
}

void DelimitedTextRowInserter::finishInsert()
{
    m_file->finishInsert();
    m_inserting = false;
}

bool DelimitedTextRowInserter::flush()
{
    m_file->flush();
    return true;
}
