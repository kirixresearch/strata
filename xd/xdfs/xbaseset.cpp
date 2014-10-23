/*!
 *
 * Copyright (c) 2014, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2014-05-22
 *
 */


#include "xdfs.h"
#include "database.h"
#include "xbaseset.h"
#include "xbaseiterator.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/formatdefinition.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/util.h"
#include "../xdcommon/extfileinfo.h"
#include "kl/md5.h"


// XbaseSet class implementation

XbaseSet::XbaseSet(FsDatabase* database) : XdfsBaseSet(database)
{
}

XbaseSet::~XbaseSet()
{
    if (m_file.isOpen())
        m_file.close();
}

bool XbaseSet::init(const std::wstring& filename)
{
    if (!m_file.open(filename))
        return false;
    
    m_filename = filename;

    // set the set info filename
    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    
    setConfigFilePath(ExtFileInfo::getConfigFilenameFromPath(definition_path, filename));

    return true;
}


std::wstring XbaseSet::getSetId()
{
    std::wstring set_id;
    
    set_id = L"xdfs:";
    set_id += xf_get_network_path(m_filename);

#ifdef WIN32
    // win32's filenames are case-insensitive, so
    // when generating the set id, make the whole filename
    // lowercase to avoid multiple id's for the same file
    kl::makeLower(set_id);
#endif

    return kl::md5str(set_id);
}

xd::Structure XbaseSet::getStructure()
{
    // create new xd::IStructure
    xd::Structure s;

    // if we can't open the file, return an empty structure
    if (!m_file.isOpen())
        return xd::Structure();

    // get structure from table
    std::vector<XbaseField> fields = m_file.getFields();
    
    // fill out xd structure from xbase structure
    std::vector<XbaseField>::iterator it;
    for (it = fields.begin(); it != fields.end(); ++it)
    {
        xd::ColumnInfo col;

        col.name = kl::towstring(it->name);
        col.type = xbase2xdType(it->type);
        col.width = it->width;
        col.scale = it->scale;
        col.column_ordinal = it->ordinal;
        
        // handle column information for specific types (currency, etc.)
        if (it->type == 'Y')    // xbase currency type
            col.width = 18;
        if (col.type == xd::typeDouble)
            col.width = 8;
        if (col.type == xd::typeNumeric && col.width > xd::max_numeric_width)
            col.width = xd::max_numeric_width;

        s.createColumn(col);
    }

    XdfsBaseSet::appendCalcFields(s);
    return s;
}

bool XbaseSet::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    bool done_flag = false;
    return XdfsBaseSet::modifyStructure(mod_params, &done_flag);
}

xd::IRowInserterPtr XbaseSet::getRowInserter()
{
    XbaseRowInserter* inserter = new XbaseRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}

bool XbaseSet::getFormatDefinition(xd::FormatDefinition* def)
{
    *def = xd::FormatDefinition();
    def->columns = getStructure().columns;
    return true;
}



xd::IIteratorPtr XbaseSet::createIterator(const std::wstring& columns,
                                          const std::wstring& order,
                                          xd::IJob* job)
{
    if (order.empty())
    {
        XbaseIterator* iter = new XbaseIterator(m_database);
        if (!iter->init(this, m_filename))
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

xd::rowpos_t XbaseSet::getRowCount()
{
    return (xd::rowpos_t)m_file.getRowCount();
}

bool XbaseSet::updateRow(xd::rowid_t rowid,
                         xd::ColumnUpdateInfo* info,
                         size_t info_size)
{
    if (!m_file.isOpen())
        return false;
        
    m_file.goRow((unsigned int)rowid);
    
    // copy the current row buffer into memory, so that we don't lose
    // all the data in non-updated columns when we do our replace below

    unsigned char* buf = m_file.getRowBuffer();
    m_file.putRowBuffer(buf);
    
    size_t i;
    for (i = 0; i < info_size; ++i)
    {
        xd::ColumnUpdateInfo* colp = &(info[i]);
        XbaseDataAccessInfo* dai = (XbaseDataAccessInfo*)colp->handle;
        int col_idx = dai->ordinal;
        
        if (colp->null)
        {
            m_file.putNull(col_idx);
            continue;
        }
        
        switch (dai->type)
        {
            case xd::typeCharacter:
            {
                m_file.putString(col_idx, colp->str_val);
                break;
            }
            case xd::typeWideCharacter:
            {
                m_file.putString(col_idx, kl::tostring(colp->wstr_val));
                break;
            }
            case xd::typeNumeric:
            case xd::typeDouble:
            {
                m_file.putDouble(col_idx, colp->dbl_val);
                break;
            }
            case xd::typeInteger:
            {
                m_file.putInteger(col_idx, colp->int_val);
                break;
            }
            case xd::typeDate:
            case xd::typeDateTime:
            {
                xd::DateTime dt = colp->date_val;
                XbaseDate value(dt.getYear(),
                                dt.getMonth(),
                                dt.getDay(),
                                dt.getHour(),
                                dt.getMinute(),
                                dt.getSecond(),
                                dt.getMillisecond());
                m_file.putDateTime(col_idx, value);
                break;
            }
            case xd::typeBoolean:
            {
                m_file.putBoolean(col_idx, colp->bool_val);
                break;
            }
        }
    }

    m_file.writeRow();
    return true;
}


// XbaseRowInserter class implementation

XbaseRowInserter::XbaseRowInserter(XbaseSet* set)
{
    m_set = set;
    m_set->ref();
    
    m_file = &(m_set->m_file);
    m_inserting = false;
}

XbaseRowInserter::~XbaseRowInserter()
{
    // free all insert field data
    std::vector<XbaseField*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete (*it);
    }

    m_set->unref();
}

xd::objhandle_t XbaseRowInserter::getHandle(const std::wstring& column_name)
{
    if (!m_inserting)
        return (xd::objhandle_t)0;


    std::string asc_col_name = kl::tostring(column_name);

    std::vector<XbaseField*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals(asc_col_name, (*it)->name))
            return (xd::objhandle_t)(*it);
    }

    return (xd::objhandle_t)0;
}

bool XbaseRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                 const unsigned char* value,
                                 int length)
{
    XbaseField* f = (XbaseField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putRaw(f->ordinal, value, length);
}

bool XbaseRowInserter::putString(xd::objhandle_t column_handle,
                                 const std::string& value)
{
    XbaseField* f = (XbaseField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putString(f->ordinal, value);
}

bool XbaseRowInserter::putWideString(xd::objhandle_t column_handle,
                                     const std::wstring& value)
{
    XbaseField* f = (XbaseField*)column_handle;
    if (!f)
    {
        return false;
    }

    return putString(column_handle, kl::tostring(value));
}

bool XbaseRowInserter::putDouble(xd::objhandle_t column_handle,
                                 double value)
{
    XbaseField* f = (XbaseField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putDouble(f->ordinal, value);
}

bool XbaseRowInserter::putInteger(xd::objhandle_t column_handle,
                                  int value)
{
    XbaseField* f = (XbaseField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putInteger(f->ordinal, value);
}

bool XbaseRowInserter::putBoolean(xd::objhandle_t column_handle,
                                  bool value)
{
    XbaseField* f = (XbaseField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putBoolean(f->ordinal, value);
}

bool XbaseRowInserter::putDateTime(xd::objhandle_t column_handle,
                                   xd::datetime_t value)
{
    XbaseField* f = (XbaseField*)column_handle;
    if (!f)
    {
        return false;
    }

    if (value == 0)
    {
        // empty date
        return m_file->putDateTime(f->ordinal, XbaseDate());
    }

    xd::DateTime dt(value);
    XbaseDate xbase_date(dt.getYear(),
                         dt.getMonth(),
                         dt.getDay(),
                         dt.getHour(),
                         dt.getMinute(),
                         dt.getSecond(),
                         dt.getMillisecond());
    
    return m_file->putDateTime(f->ordinal, xbase_date);
}

bool XbaseRowInserter::putNull(xd::objhandle_t column_handle)
{
    XbaseField* f = (XbaseField*)column_handle;
    if (!f)
    {
        return false;
    }

    return m_file->putNull(f->ordinal);
}

bool XbaseRowInserter::startInsert(const std::wstring& col_list)
{
    std::vector<XbaseField>::iterator it;
    std::vector<XbaseField> fields = m_file->getFields();
    
    for (it = fields.begin(); it != fields.end(); ++it)
    {
        XbaseField* f = new XbaseField;
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
}

bool XbaseRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    return m_file->insertRow();
}

void XbaseRowInserter::finishInsert()
{
    m_file->finishInsert();
    m_inserting = false;
}

bool XbaseRowInserter::flush()
{
    // all of the "flush" logic happens in XbaseFile,
    // so there is no need to do anything here
    return true;
}

