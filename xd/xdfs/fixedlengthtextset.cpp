/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-05-12
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <cmath>
#include <xd/xd.h>
#include "xdfs.h"
#include "database.h"
#include "rawtext.h"
#include "fixedlengthtextset.h"
#include "fixedlengthtextiterator.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/util.h"
#include "../xdcommon/extfileinfo.h"
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/math.h>
#include <kl/md5.h>


const int GUESS_LINE_DELIMITERS_NEEDED = 20;
const int GUESS_BUF_SIZE = 400000;
const int BUF_ROW_COUNT = 10000;


// -- FixedLengthDefinition --

FixedLengthDefinition::FixedLengthDefinition()
{
    m_source_structure = static_cast<xd::IStructure*>(new Structure);
    m_dest_structure = static_cast<xd::IStructure*>(new Structure);
    
    m_line_delimiters = L"\x0a";
    m_skip_chars = 0;
    m_row_width = 0;
    m_file_type = FixedLengthDefinition::LineDelimited;
}



// -- FixedLengthTextSet class implementation --

FixedLengthTextSet::FixedLengthTextSet(FsDatabase* database)  : XdfsBaseSet(database)
{
    m_definition = new FixedLengthDefinition;
    m_definition->ref();
    
    m_path = L"";
    m_row_count = 0;

    m_file_size = 0;

}

FixedLengthTextSet::~FixedLengthTextSet()
{
    // release definition object
    m_definition->unref();
}

bool FixedLengthTextSet::init(const std::wstring& filename)
{
    if (!xf_get_file_exist(filename))
        return false;

    // set our member variables
    m_path = filename;
    
    // figure out the config file name
    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    m_configfile_path = ExtFileInfo::getConfigFilenameFromPath(definition_path, filename);

    // set the set info filename
    setConfigFilePath(m_configfile_path);

    // try to load the config file for this set
    if (!loadConfiguration())
    {
        // get some default structure information about the file
        RawTextFile rtf;
        if (!rtf.openFile(filename))
            return false;

        m_definition->m_file_type = (rtf.getFileType() == RawTextFile::FixedWidth) ?
                                      FixedLengthDefinition::FixedWidth :
                                      FixedLengthDefinition::LineDelimited;
        m_definition->m_row_width = (size_t)rtf.getRowWidth();
        
        rtf.closeFile();
        
        if (m_definition->m_source_structure->getColumnCount() == 0)
        {
            // create a default column in the source structure
            xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
            col->setOffset(0);
            col->setType(xd::typeCharacter);
            col->setWidth(m_definition->m_row_width);
            col->setName(L"Field1");
            
            IStructureInternalPtr struct_int = m_definition->m_source_structure;
            struct_int->addColumn(col);
        }
    }
    
    return true;
}

void FixedLengthTextSet::setCreateStructure(xd::IStructurePtr structure)
{
    // this function is used when creating a new from a structure, 
    // such as when exporting to fixed-length; code adapted from 
    // DelimitedTextSet::setCreateStructure()
    
    m_definition->m_source_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr src_struct_int = m_definition->m_source_structure;
    
    m_definition->m_dest_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr dest_struct_int = m_definition->m_dest_structure;

    int row_width = 0;
    int i, col_count = structure->getColumnCount();
    
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr orig_col = structure->getColumnInfoByIdx(i);
        if (orig_col->getCalculated())
            continue;
    
        // the new width is used to ensure there is enough space in
        // the source structure to accomodate a string representation
        // of a number (negative and decimal) and a date (slashes and
        // colons)
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

        // source structure; for the source structure use the original 
        // column information (except the width), even though the 
        // underlying structure is all text; this ensures that when 
        // writing to the underlying structure, we can format a string
        // with the correct number of decimal places as well as right-
        // justify numbers
        xd::IColumnInfoPtr src_col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        src_col->setName(orig_col->getName());
        src_col->setType(orig_col->getType());
        src_col->setWidth(new_width);
        src_col->setScale(orig_col->getScale());

        src_col->setOffset(row_width);
        src_col->setEncoding(orig_col->getEncoding());
        src_col->setColumnOrdinal(i);
        src_struct_int->addColumn(src_col);

        // destination structure
        // TODO: automatically add type and formulas to restore 
        // original types so that when file is reopened, the definition 
        // is automatically set
        xd::IColumnInfoPtr dest_col = orig_col->clone();
        dest_col->setName(orig_col->getName());
        dest_col->setType(orig_col->getType());
        dest_col->setWidth(orig_col->getWidth());
        dest_col->setScale(orig_col->getScale());

        std::wstring expr = src_col->getName();
        if (orig_col->getType() == xd::typeDate ||
            orig_col->getType() == xd::typeDateTime)
        {
            expr = L"((DATE(TRIM(" + expr + L"))))";
        }

        dest_col->setExpression(expr);
        dest_col->setEncoding(src_col->getEncoding());
        dest_col->setColumnOrdinal(i);
        dest_struct_int->addColumn(dest_col);

        // keep track of the row width
        row_width += new_width;
    }

    // add the calculated fields
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr orig_col = structure->getColumnInfoByIdx(i);
        if (!orig_col->getCalculated())
            continue;

        createCalcField(orig_col->clone());
    }
    
    // set additional fixed-length parameters
    setBeginningSkipCharacterCount(0);
    setRowWidth(row_width + 2);         // +2 for line terminator
    setLineDelimited(false);
}

/*
std::wstring FixedLengthTextSet::getSetId()
{
    std::wstring set_id;
    
    set_id = L"xdfs:";
    set_id += xf_get_network_path(m_path);
    
#ifdef WIN32
    // win32's filenames are case-insensitive, so
    // when generating the set id, make the whole filename
    // lowercase to avoid multiple id's for the same file
    kl::makeLower(set_id);
#endif

    return kl::md5str(set_id);
}
*/

xd::IRowInserterPtr FixedLengthTextSet::getRowInserter()
{
    FixedLengthTextRowInserter* inserter = new FixedLengthTextRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}

xd::IIteratorPtr FixedLengthTextSet::createIterator(
                                        const std::wstring& columns,
                                        const std::wstring& order,
                                        xd::IJob* job)
{
    if (order.empty())
    {
        FixedLengthTextIterator* iter = new FixedLengthTextIterator;        
        if (!iter->init(m_database, this, columns))
        {
            delete iter;
            return xcm::null;
        }
            
        return static_cast<xd::IIterator*>(iter);
    }
    
    // find out where the database should put temporary files
    std::wstring temp_directory = m_database->getTempFileDirectory();

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
                              true,
                              job);
    if (!idx)
        return xcm::null;

    xd::IIteratorPtr data_iter = createIterator(columns, L"", NULL);
    if (data_iter.isNull())
    {
        idx->unref();
        return xcm::null;
    }


    xd::IIteratorPtr result_iter = createIteratorFromIndex(data_iter,
                                                           idx,
                                                           columns,
                                                           order,
                                                           getObjectPath());
    idx->unref();
    return result_iter;
}


xd::rowpos_t FixedLengthTextSet::getRowCount()
{
/*
    if (m_definition->m_file_type == FixedLengthDefinition::FixedWidth)
    {
        if (m_row_count == 0)
        {
            if (m_file_size == 0)
                m_file_size = xf_get_file_size(m_path);

            m_row_count = calcRowCount(m_file_size, m_definition->m_skip_chars, m_row_width);
        }

        return m_row_count;
    }
*/

    return 0;
}


// -- xd::IFixedLengthDefinition interface --

bool FixedLengthTextSet::loadConfiguration()
{
    // try to load the external text definition
    ExtFileInfo fileinfo;
    if (!fileinfo.load(m_configfile_path))
        return false;

    // find out if the we're dealing with the right type of text definition
    ExtFileEntry base = fileinfo.getGroup(L"file_info");
    std::wstring file_type = base.getChildContents(L"type");
    if (file_type != L"text/fixed" &&
        file_type != L"fixed_length")
    {
        return false;
    }

    // get the main file settings
    ExtFileEntry entry = base.getChild(L"settings");
    m_definition->m_skip_chars = kl::wtoi(entry.getChildContents(L"skip_beginning_characters"));
    m_definition->m_row_width = kl::wtoi(entry.getChildContents(L"row_width"));
    int line_delimited = kl::wtoi(entry.getChildContents(L"line_delimited"));
    m_definition->m_file_type = (line_delimited == 1) ? FixedLengthDefinition::LineDelimited :
                                          FixedLengthDefinition::FixedWidth;

    // clear out any existing source structure in the set 
    m_definition->m_source_structure.clear();
    m_definition->m_source_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr src_struct_int = m_definition->m_source_structure;
    
    int offset, width, type, scale, encoding;
    std::wstring name, expression;
    
    // now, get the source structure from the file
    entry = base.getChild(L"fields");
    int i, count = entry.getChildCount();
    for (i = 0; i < count; ++i)
    {
        ExtFileEntry field = entry.getChild(i);
        
        offset = kl::wtoi(field.getChildContents(L"offset"));
        width = kl::wtoi(field.getChildContents(L"width"));
        name = field.getChildContents(L"name");
        encoding = xd::encodingUndefined;
        
        ExtFileEntry encoding_entry = field.getChild(L"encoding");
        if (encoding_entry.isOk())
        {
            // older versions of "edf" files don't have the encoding parameter
            encoding = kl::wtoi(encoding_entry.getContents());
        }
        
        xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
        col->setOffset(offset);
        col->setWidth(width);
        col->setName(name);
        col->setType(xd::typeCharacter);
        col->setColumnOrdinal(i);
        col->setEncoding(encoding);
        src_struct_int->addColumn(col);
    }
    
    // clear out any existing destination structure in the set
    m_definition->m_dest_structure.clear();
    m_definition->m_dest_structure = static_cast<xd::IStructure*>(new Structure);
    IStructureInternalPtr dest_struct_int = m_definition->m_dest_structure;

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

bool FixedLengthTextSet::saveConfiguration()
{
    // if an external text definition exists, start from there
    ExtFileInfo fileinfo;
    fileinfo.load(m_configfile_path);
    
    // delete the existing "file_info" group and create a new one
    fileinfo.deleteGroup(L"file_info");
    ExtFileEntry base = fileinfo.getGroup(L"file_info");
    
    ExtFileEntry entry = base.addChild(L"type", L"text/fixed");
    entry = base.addChild(L"settings");
    entry.addChild(L"skip_beginning_characters", m_definition->m_skip_chars);
    entry.addChild(L"row_width", m_definition->m_row_width);
    entry.addChild(L"line_delimited",
                   (m_definition->m_file_type == FixedLengthDefinition::LineDelimited) ? 1 : 0);
    
    entry = base.addChild(L"fields");

    // save the source structure to the file
    xd::IStructurePtr s = getSourceStructure();
    int i, col_count = s->getColumnCount();
    
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);

        ExtFileEntry field = entry.addChild(L"field");
        field.addChild(L"name", colinfo->getName());
        field.addChild(L"offset", colinfo->getOffset());
        field.addChild(L"width", colinfo->getWidth());
        field.addChild(L"encoding", colinfo->getEncoding());
    }
    
    // delete the existing "set_info" group and create a new one
    fileinfo.deleteGroup(L"set_info");
    base = fileinfo.getGroup(L"set_info");
    
    entry = base.addChild(L"fields");

    // now, save the set structure to the file
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

bool FixedLengthTextSet::deleteConfiguration()
{
    return xf_remove(m_configfile_path);
}

inline xd::IStructurePtr createDefaultStructure(xd::IStructurePtr source)
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

xd::IStructurePtr FixedLengthTextSet::getSourceStructure()
{
    return m_definition->m_source_structure->clone();
}

xd::IStructurePtr FixedLengthTextSet::getDestinationStructure()
{
    if (m_definition->m_dest_structure->getColumnCount() == 0)
        m_definition->m_dest_structure = createDefaultStructure(m_definition->m_source_structure);
    
    return m_definition->m_dest_structure->clone();
}

xd::IStructurePtr FixedLengthTextSet::getStructure()
{
    xd::IStructurePtr s = getDestinationStructure();
    int i, col_count = s->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        colinfo->setExpression(L"");
        colinfo->setCalculated(false);
    }

    XdfsBaseSet::appendCalcFields(s);
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

bool FixedLengthTextSet::modifySourceStructure(xd::IStructure* struct_config,
                                               xd::IJob* job)
{
    // get the structure actions from the
    // structure configuration that was passed
    IStructureInternalPtr struct_int = struct_config;
    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    int processed_action_count = 0;

    // make sure we actually modify the stored source structure
    struct_int = m_definition->m_source_structure;

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
        col->setEncoding(it->m_params->getEncoding());
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
        col->setEncoding(it->m_params->getEncoding());
        struct_int->internalInsertColumn(col, insert_idx);
    }
    
    // ensure that the each column has the proper column ordinal
    resetColumnOrdinals(m_definition->m_source_structure);
    return true;
}

bool FixedLengthTextSet::modifyDestinationStructure(xd::IStructure* struct_config,
                                                    xd::IJob* job)
{
    // get the structure actions from the
    // structure configuration that was passed
    IStructureInternalPtr struct_int = struct_config;
    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    int processed_action_count = 0;

    // make sure we actually modify the stored structure
    struct_int = m_definition->m_dest_structure;

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
        colinfo = m_definition->m_dest_structure->getColumnInfo(it->m_colname);
        
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
    resetColumnOrdinals(m_definition->m_dest_structure);
    return true;
}

bool FixedLengthTextSet::modifyStructure(xd::IStructure* struct_config,
                                         xd::IJob* job)
{
    bool done_flag = false;
    XdfsBaseSet::modifyStructure(struct_config, &done_flag);
    return true;
}


void FixedLengthTextSet::setBeginningSkipCharacterCount(size_t new_val)
{
    m_definition->m_skip_chars = new_val;
}

size_t FixedLengthTextSet::getBeginningSkipCharacterCount()
{
    return m_definition->m_skip_chars;
}

void FixedLengthTextSet::setRowWidth(size_t new_val)
{
    // NOTE: this action does not remove fields from the destination
    //       structure, even if those fields refer back to a missing
    //       source structure field -- this is because the destination
    //       structure is not really anything more than a list of
    //       fields that the user wants in the destination table

    m_definition->m_row_width = new_val;
    
    // the structure which we'll populate with the structure actions
    xd::IStructurePtr mod_struct = getSourceStructure();
    
    xd::IColumnInfoPtr colinfo;
    int i, col_count = m_definition->m_source_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        colinfo = m_definition->m_source_structure->getColumnInfoByIdx(i);
        if (colinfo.isNull())
            continue;
            
        if (colinfo->getOffset() >= (int)m_definition->m_row_width)
        {
            mod_struct->deleteColumn(colinfo->getName());
        }
    }
    
    modifySourceStructure(mod_struct, NULL);
}

size_t FixedLengthTextSet::getRowWidth()
{
    return m_definition->m_row_width;
}

void FixedLengthTextSet::setLineDelimited(bool new_val)
{
    (new_val == true) ? m_definition->m_file_type = FixedLengthDefinition::LineDelimited :
                        m_definition->m_file_type = FixedLengthDefinition::FixedWidth;
}

bool FixedLengthTextSet::isLineDelimited()
{
    return (m_definition->m_file_type == FixedLengthDefinition::LineDelimited) ? true : false;
}



// -- FixedLengthTextRowInserter class implementation --

FixedLengthTextRowInserter::FixedLengthTextRowInserter(FixedLengthTextSet* set)
{
    m_set = set;
    m_set->ref();

    m_inserting = false;

    m_file = NULL;
    m_buf = NULL;                    // buffer for rows
    m_buf_ptr = NULL;
    m_row_width = 0;
    m_crlf = true;
}

FixedLengthTextRowInserter::~FixedLengthTextRowInserter()
{
    std::vector<FixedLengthTextInsertData*>::iterator it;
    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        delete (*it);
    }
    
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }

    if (m_file)
    {
        xf_close(m_file);
        m_file = NULL;
    }
    
    m_set->unref();
}

xd::objhandle_t FixedLengthTextRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<FixedLengthTextInsertData*>::iterator it;

    std::string asc_colname = kl::tostring(column_name);

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!strcasecmp((*it)->m_colname.c_str(), asc_colname.c_str()))
            return (xd::objhandle_t)(*it);
    }

    return 0;
}

xd::IColumnInfoPtr FixedLengthTextRowInserter::getInfo(xd::objhandle_t column_handle)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return xcm::null;
    }

    xd::IStructurePtr structure = m_set->getStructure();
    xd::IColumnInfoPtr col = structure->getColumnInfo(kl::towstring(f->m_colname));
    return col;
}

bool FixedLengthTextRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                         const unsigned char* value,
                                         int length)
{
    return true;
}

bool FixedLengthTextRowInserter::putString(xd::objhandle_t column_handle,
                                         const std::string& value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_str_val = value;
    return true;
}

bool FixedLengthTextRowInserter::putWideString(xd::objhandle_t column_handle,
                                             const std::wstring& value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    return putString(column_handle, kl::tostring(value));
}

bool FixedLengthTextRowInserter::putDouble(xd::objhandle_t column_handle,
                                         double value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    char buf[255];
    sprintf(buf, "%.*f", f->m_scale, kl::dblround(value, f->m_scale));

    // -- convert a euro decimal character to a decimal point --
    char* p = buf;
    while (*p)
    {
        if (*p == ',')
            *p = '.';
        ++p;
    }

    f->m_str_val = buf;
    return true;
}

bool FixedLengthTextRowInserter::putInteger(xd::objhandle_t column_handle,
                                          int value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    char buf[64];
    sprintf(buf, "%d", value);
    f->m_str_val = buf;
    return true;
}

bool FixedLengthTextRowInserter::putBoolean(xd::objhandle_t column_handle,
                                          bool value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    value ? f->m_str_val = "T" : f->m_str_val = "F";
    return true;
}

bool FixedLengthTextRowInserter::putDateTime(xd::objhandle_t column_handle,
                                           xd::datetime_t value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    if (value == 0)
    {
        f->m_str_val = "";
        return true;
    }

    xd::DateTime dt(value);

    int y = dt.getYear();
    int m = dt.getMonth();
    int d = dt.getDay();
    int hh = dt.getHour();
    int mm = dt.getMinute();
    int ss = dt.getSecond();

    char buf[64];

    if (hh == 0 && mm == 0 && ss == 0)
    {
        snprintf(buf, 64, "%04d/%02d/%02d", y, m, d);
    }
     else
    {
        snprintf(buf, 64, "%04d/%02d/%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
    }

    f->m_str_val = buf;
    return true;
}

bool FixedLengthTextRowInserter::putNull(xd::objhandle_t column_handle)
{
    return true;
}

bool FixedLengthTextRowInserter::startInsert(const std::wstring& col_list)
{
    if (!m_file)
    {
        m_file = xf_open(m_set->m_path, xfOpen, xfReadWrite, xfShareNone);

        if (m_file == NULL)
            return false;
    }

    // get the source structure since we're inserting records
    xd::IStructurePtr s = m_set->getSourceStructure();

    int i;
    int col_count = s->getColumnCount();
    m_row_width = 0;
    
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);

        FixedLengthTextInsertData* field = new FixedLengthTextInsertData;
        field->m_colname = kl::tostring(colinfo->getName());
        field->m_type = colinfo->getType();
        field->m_width = colinfo->getWidth();
        field->m_scale = colinfo->getScale();

        switch (field->m_type)
        {
            default:
                field->m_align_right = false;
                break;

            // right-align numerics                
            case xd::typeNumeric:
            case xd::typeDouble:
            case xd::typeInteger:
                field->m_align_right = true;
                break;
        }

        m_row_width += field->m_width;
        m_insert_data.push_back(field);
    }
    
    // add two bytes to the row width for the CR/LF
    if (m_crlf)
        m_row_width += 2;
        
    // create the buffer for writing to the file
    m_buf = new char[BUF_ROW_COUNT*m_row_width];

    // set the buffer pointer to the beginning of the buffer
    m_buf_ptr = m_buf;
    
    m_inserting = true;
    return true;
}

bool FixedLengthTextRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    // -- if this row extends past our buffer size, flush the buffer --
    if (m_buf_ptr+m_row_width >= m_buf+(BUF_ROW_COUNT*m_row_width))
        flush();

    int diff;
    
    std::vector<FixedLengthTextInsertData*>::iterator it;
    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        // -- (field width - actual data width) --
        diff = (*it)->m_width - (*it)->m_str_val.length();
        
        // -- add the string --
        char* p = (char*)(*it)->m_str_val.c_str();

        // -- for left-padded fields --
        if ((*it)->m_align_right)
        {
            while (diff-- > 0)
            {
                *m_buf_ptr = L' ';
                m_buf_ptr++;
            }
        }

        // -- insert data --
        while (*p)
        {
            *m_buf_ptr = *p;

            m_buf_ptr++;
            p++;
        }
        
        // -- for right-padded fields --
        if (!(*it)->m_align_right)
        {
            while (diff-- > 0)
            {
                *m_buf_ptr = L' ';
                m_buf_ptr++;
            }
        }
    }

    if (m_crlf)
    {
        *(m_buf_ptr) = 0x0d;
        m_buf_ptr++;
        *(m_buf_ptr) = 0x0a;
        m_buf_ptr++;
    }
    
    return true;
}

void FixedLengthTextRowInserter::finishInsert()
{
    // -- flush any remaining data in the buffer before closing the file --
    flush();
    
    // -- delete the buffer --
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }

    // -- close the file --
    if (m_file)
    {
        xf_close(m_file);
        m_file = NULL;
    }
    
    m_inserting = false;
}

bool FixedLengthTextRowInserter::flush()
{
    // -- make sure that the last two characters of the file are a CF/LF --
    if (m_crlf && xf_seek(m_file, -2, xfSeekEnd))
    {
        char temp_buf[2];
        if (xf_read(m_file, temp_buf, 1, 2) == 2)
        {
            if (temp_buf[0] != 0x0d ||
                temp_buf[1] != 0x0a)
            {
                temp_buf[0] = 0x0d;
                temp_buf[1] = 0x0a;
                xf_write(m_file, temp_buf, 1, 2);
            }
        }
         else
        {
            xf_seek(m_file, 0, xfSeekEnd);
        }
    }
     else
    {
        xf_seek(m_file, 0, xfSeekEnd);
    }

    xf_write(m_file, m_buf, m_buf_ptr-m_buf, 1);
    m_buf_ptr = m_buf;
    
    return true;
}




