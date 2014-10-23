/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-28
 *
 */


#include "xdfs.h"
#include "database.h"
#include "xlsxset.h"
#include "xlsxiterator.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/formatdefinition.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/util.h"
#include "../xdcommon/extfileinfo.h"
#include "kl/md5.h"


// XlsxSet class implementation

XlsxSet::XlsxSet(FsDatabase* database) : XdfsBaseSet(database)
{
}

XlsxSet::~XlsxSet()
{
    if (m_file.isOpen())
        m_file.close();
}

bool XlsxSet::init(const std::wstring& filename)
{
    if (!m_file.open(filename))
        return false;

    // set the set info filename
    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    
    setConfigFilePath(ExtFileInfo::getConfigFilenameFromPath(definition_path, filename));

    return true;
}


std::wstring XlsxSet::getSetId()
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


static std::wstring getSpreadsheetColumnName(int idx)
{
    if (idx == 0)
        return L"A";

    std::wstring res;
    int n;

    while (idx)
    {
        n = idx % 26;
        res.insert(res.begin(), ('A' + n));
        idx -= n;
        idx /= 26;
    }

    return res;
}

xd::Structure XlsxSet::getStructure()
{
    xd::Structure s;

    // if we can't open the file, return an empty structure
    if (!m_file.isOpen())
        return xd::Structure();

    // get structure from table
    //std::vector<XlsxField> fields = m_file.getFields();
    
    // fill out xd structure from xbase structure
    //std::vector<XlsxField>::iterator it;
    //for (it = fields.begin(); it != fields.end(); ++it)

    size_t i, col_count = m_file.getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        xd::ColumnInfo col;

        col.name = getSpreadsheetColumnName((int)i);
        col.type = xd::typeWideCharacter;
        col.width = 255;
        col.scale = 0;
        col.column_ordinal = i;

        s.createColumn(col);
    }

    XdfsBaseSet::appendCalcFields(s);
    return s;
}

bool XlsxSet::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    bool done_flag = false;
    return XdfsBaseSet::modifyStructure(mod_params, &done_flag);
}

xd::IRowInserterPtr XlsxSet::getRowInserter()
{
    XlsxRowInserter* inserter = new XlsxRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}

bool XlsxSet::getFormatDefinition(xd::FormatDefinition* def)
{
    *def = xd::FormatDefinition();
    def->columns = getStructure().columns;
    return true;
}



xd::IIteratorPtr XlsxSet::createIterator(const std::wstring& columns,
                                         const std::wstring& order,
                                         xd::IJob* job)
{
    if (order.empty())
    {
        XlsxIterator* iter = new XlsxIterator(m_database);
        if (!iter->init(this))
        {
            delete iter;
            return xcm::null;
        }
        
        iter->goFirst();
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

xd::rowpos_t XlsxSet::getRowCount()
{
    return (xd::rowpos_t)m_file.getRowCount();
}

bool XlsxSet::updateRow(xd::rowid_t rowid,
                         xd::ColumnUpdateInfo* info,
                         size_t info_size)
{
    return false;
}


// XlsxRowInserter class implementation

XlsxRowInserter::XlsxRowInserter(XlsxSet* set)
{
    m_set = set;
    m_set->ref();
    
    m_file = &(m_set->m_file);
    m_inserting = false;
}

XlsxRowInserter::~XlsxRowInserter()
{
    // free all insert field data
    std::vector<XlsxField*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete (*it);
    }

    m_set->unref();
}

xd::objhandle_t XlsxRowInserter::getHandle(const std::wstring& column_name)
{
    if (!m_inserting)
        return (xd::objhandle_t)0;


    std::string asc_col_name = kl::tostring(column_name);

    std::vector<XlsxField*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals(asc_col_name, (*it)->name))
        {
            return (xd::objhandle_t)(*it);
        }
    }

    return (xd::objhandle_t)0;
}

bool XlsxRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                 const unsigned char* value,
                                 int length)
{
    XlsxField* f = (XlsxField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putRaw(f->ordinal, value, length);
}

bool XlsxRowInserter::putString(xd::objhandle_t column_handle,
                                 const std::string& value)
{
    XlsxField* f = (XlsxField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putString(f->ordinal, value);
}

bool XlsxRowInserter::putWideString(xd::objhandle_t column_handle,
                                     const std::wstring& value)
{
    XlsxField* f = (XlsxField*)column_handle;
    if (!f)
    {
        return false;
    }

    return putString(column_handle, kl::tostring(value));
}

bool XlsxRowInserter::putDouble(xd::objhandle_t column_handle,
                                 double value)
{
    XlsxField* f = (XlsxField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putDouble(f->ordinal, value);
}

bool XlsxRowInserter::putInteger(xd::objhandle_t column_handle,
                                  int value)
{
    XlsxField* f = (XlsxField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putInteger(f->ordinal, value);
}

bool XlsxRowInserter::putBoolean(xd::objhandle_t column_handle,
                                  bool value)
{
    XlsxField* f = (XlsxField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putBoolean(f->ordinal, value);
}

bool XlsxRowInserter::putDateTime(xd::objhandle_t column_handle,
                                   xd::datetime_t value)
{
    XlsxField* f = (XlsxField*)column_handle;
    if (!f)
    {
        return false;
    }

    if (value == 0)
    {
        // empty date
        return m_file->putDateTime(f->ordinal, XlsxDateTime());
    }

    xd::DateTime dt(value);
    XlsxDateTime xbase_date(dt.getYear(),
                         dt.getMonth(),
                         dt.getDay(),
                         dt.getHour(),
                         dt.getMinute(),
                         dt.getSecond(),
                         dt.getMillisecond());
    
    return m_file->putDateTime(f->ordinal, xbase_date);
}

bool XlsxRowInserter::putNull(xd::objhandle_t column_handle)
{
    XlsxField* f = (XlsxField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putNull(f->ordinal);
}

bool XlsxRowInserter::startInsert(const std::wstring& col_list)
{
    return false;

/*
    std::vector<XlsxField>::iterator it;
    std::vector<XlsxField> fields = m_file->getFields();
    
    for (it = fields.begin(); it != fields.end(); ++it)
    {
        XlsxField* f = new XlsxField;
        f->name = it->name;
        f->type = it->type;
        f->width = it->width;
        f->scale = it->scale;
        f->offset = it->offset;
        f->ordinal = it->ordinal;
        m_fields.push_back(f);
    }
    
    m_inserting = true;
    return m_file->startInsert();
*/
}

bool XlsxRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    return m_file->insertRow();
}

void XlsxRowInserter::finishInsert()
{
    m_file->finishInsert();
    m_inserting = false;
}

bool XlsxRowInserter::flush()
{
    // all of the "flush" logic happens in XbaseFile,
    // so there is no need to do anything here
    return true;
}

