/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <xd/xd.h>
#include "xdfs.h"
#include "database.h"
#include "ttbset.h"
#include "ttbiterator.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/util.h"
#include "../xdcommon/extfileinfo.h"
#include <kl/md5.h>
#include <kl/math.h>

// TtbSet class implementation

TtbSet::TtbSet(FsDatabase* db)
{
    m_database = db;
    m_database->ref();

    m_update_buf = NULL;
    m_update_row.setTable(&m_file);
}

TtbSet::~TtbSet()
{
    if (m_file.isOpen())
        m_file.close();

    delete[] m_update_buf;

    m_database->unref();
}

bool TtbSet::init(const std::wstring& filename)
{
    if (!m_file.open(filename))
        return false;

    // set the set info filename
    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    
    setConfigFilePath(ExtFileInfo::getConfigFilenameFromPath(definition_path, filename));

    return true;
}




xd::IStructurePtr TtbSet::getStructure()
{
    // if we can't open the file, return an empty structure
    if (!m_file.isOpen())
        return xcm::null;

    // get structure from xbase file
    xd::IStructurePtr s = m_file.getStructure();

    CommonBaseSet::appendCalcFields(s);
    return s;
}

bool TtbSet::modifyStructure(xd::IStructure* struct_config,
                               xd::IJob* job)
{
    bool done_flag = false;
    CommonBaseSet::modifyStructure(struct_config, &done_flag);
    return true;
}

xd::IRowInserterPtr TtbSet::getRowInserter()
{
    TtbRowInserter* inserter = new TtbRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}


xd::IIteratorPtr TtbSet::createIterator(const std::wstring& columns,
                                             const std::wstring& order,
                                             xd::IJob* job)
{
    if (order.empty())
    {
        TtbIterator* iter = new TtbIterator;
        if (!iter->init(m_database,
                        this,
                        m_file.getFilename()))
        {
            delete iter;
            return xcm::null;
        }
        
        iter->goFirst();
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

xd::rowpos_t TtbSet::getRowCount()
{
    return (xd::rowpos_t)m_file.getRowCount();
}

bool TtbSet::updateRow(xd::rowid_t rowid,
                       xd::ColumnUpdateInfo* info,
                       size_t info_size)
{
    size_t coli;
    xd::ColumnUpdateInfo* col_it;

    xd::rowpos_t row = rowidGetRowPos(rowid);


    if (!m_update_buf)
    {
        m_update_buf = new unsigned char[m_file.getRowWidth()];
        m_update_row.setRowPtr(m_update_buf);
    }


    // read the row
    m_file.getRow(row, m_update_buf);

    /*
    // determine which indexes need updating
    
    std::vector<IndexEntry>::iterator idx_it;
    for (idx_it = m_indexes.begin();
         idx_it != m_indexes.end(); ++idx_it)
    {
        idx_it->update = false;

        for (coli = 0; coli < info_size; ++coli)
        {
            TtbDataAccessInfo* dai = (TtbDataAccessInfo*)(info[coli].handle);

            if (idx_it->active_columns[dai->ordinal])
            {
                idx_it->update = true;
                break;
            }
        }

        if (idx_it->update)
        {
            memcpy(idx_it->orig_key,
                   idx_it->key_expr->getKey(),
                   idx_it->key_length);
        }
    }
    */


    // modify the buffer

    for (coli = 0; coli < info_size; ++coli)
    {
        col_it = &info[coli];

        xd::objhandle_t handle = col_it->handle;
        TtbDataAccessInfo* dai = (TtbDataAccessInfo*)handle;
        if (!dai)
            continue;

        if (col_it->null)
        {
            m_update_row.putNull(dai->ordinal);
            continue;
        }

        switch (dai->type)
        {
            case xd::typeCharacter:
                m_update_row.putString(dai->ordinal, col_it->str_val);
                break;

            case xd::typeWideCharacter:
                m_update_row.putWideString(dai->ordinal, col_it->wstr_val);
                break;

            case xd::typeNumeric:
            case xd::typeDouble:
                m_update_row.putDouble(dai->ordinal, col_it->dbl_val);
                break;

            case xd::typeInteger:
                m_update_row.putInteger(dai->ordinal, col_it->int_val);
                break;

            case xd::typeDate:
            case xd::typeDateTime:
                m_update_row.putDateTime(dai->ordinal, col_it->date_val);
                break;

            case xd::typeBoolean:
                m_update_row.putBoolean(dai->ordinal, col_it->bool_val);
                break;
        }
    }


    // write the resulting row
    m_file.writeRow(row, m_update_buf);

/*
    // update the indexes
    for (idx_it = m_indexes.begin();
         idx_it != m_indexes.end(); ++idx_it)
    {
        if (!idx_it->update)
            continue;

        // if present key is the same as the last key, don't do anything
        const unsigned char* new_key = idx_it->key_expr->getKey();
        
        if (0 == memcmp(idx_it->orig_key, new_key, idx_it->key_length))
            continue;

        
        IIndexIterator* index_iter;

        index_iter = seekRow(idx_it->index,
                             idx_it->orig_key,
                             idx_it->key_length,
                             rowid);

        if (index_iter)
        {
            // remove old key
            idx_it->index->remove(index_iter);
            index_iter->unref();
        }


        idx_it->index->insert(new_key,
                              idx_it->key_length,
                              &rowid,
                              sizeof(xd::rowid_t));
    }

*/

    return true;
}


// TtbRowInserter class implementation


const int ttb_inserter_buf_rows = 500;

TtbRowInserter::TtbRowInserter(TtbSet* set)
{
    m_set = set;
    m_set->ref();

    m_file = &(m_set->m_file);
    m_row.setTable(m_file);

    m_row_width = m_file->getRowWidth();
    m_buf_row = 0;
    m_tbl_filename = m_file->getFilename();
    m_buf = new unsigned char[ttb_inserter_buf_rows * m_row_width];
    m_row.setRowPtr(m_buf);

    m_inserting = false;
}

TtbRowInserter::~TtbRowInserter()
{
    delete[] m_buf;

    m_set->unref();
}


bool TtbRowInserter::startInsert(const std::wstring& _col_list)
{
    xd::IStructurePtr structure = m_file->getStructure();
    if (structure.isNull())
        return false;

    std::wstring col_list = _col_list;
    kl::trim(col_list);
    if (col_list == L"" || col_list == L"*")
    {
        col_list = L"";
        int i, col_count = structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            if (i > 0)
                col_list += L",";
            col_list += structure->getColumnName(i);
        }
    }


    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;

    kl::parseDelimitedList(col_list, columns, L',');

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        TtbInsertData* col = new TtbInsertData;
        col->col = structure->getColumnInfo(*it);
        if (col->col.isNull())
        {
            delete col;
            return false;
        }
        col->name = col->col->getName();
        col->ordinal = col->col->getColumnOrdinal();
        col->type = col->col->getType();
        col->width = col->col->getWidth();
        col->scale = col->col->getScale();
        col->offset = col->col->getOffset();
        col->nulls_allowed = false;

        m_fields.push_back(col);
    }

    memset(m_buf, 0, ttb_inserter_buf_rows * m_row_width);
    m_buf_row = 0;
    m_row.setRowPtr(m_buf);
    m_inserting = true;

    return true;
}


xd::objhandle_t TtbRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<TtbInsertData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, column_name))
            return (xd::objhandle_t)(*it);
    }

    return (xd::objhandle_t)0;
}

xd::IColumnInfoPtr TtbRowInserter::getInfo(xd::objhandle_t column_handle)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;
    if (!dai)
        return xcm::null;

    return dai->col;
}

bool TtbRowInserter::putRawPtr(xd::objhandle_t column_handle,
                               const unsigned char* value,
                               int length)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    return m_row.putRawPtr(dai->ordinal, value, length);
}

bool TtbRowInserter::putString(xd::objhandle_t column_handle,
                               const std::string& value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    return m_row.putString(dai->ordinal, value);

}

bool TtbRowInserter::putWideString(xd::objhandle_t column_handle,
                                   const std::wstring& value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    return m_row.putWideString(dai->ordinal, value);
}

bool TtbRowInserter::putDouble(xd::objhandle_t column_handle,
                               double value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    return m_row.putDouble(dai->ordinal, value);
}

bool TtbRowInserter::putInteger(xd::objhandle_t column_handle,
                                int value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    return m_row.putDouble(dai->ordinal, value);
}

bool TtbRowInserter::putBoolean(xd::objhandle_t column_handle,
                                bool value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    return m_row.putBoolean(dai->ordinal, value);
}

bool TtbRowInserter::putDateTime(xd::objhandle_t column_handle,
                                 xd::datetime_t value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    return m_row.putDateTime(dai->ordinal, value);
}

bool TtbRowInserter::putRowBuffer(const unsigned char* value)
{
    return false;
}

bool TtbRowInserter::putNull(xd::objhandle_t column_handle)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;
    
    if (!dai->nulls_allowed)
        return false;

    return m_row.putNull(dai->ordinal);
}


bool TtbRowInserter::insertRow()
{
    m_buf_row++;
    m_row.setRowPtr(m_buf + (m_buf_row * m_row_width));
    if (m_buf_row == ttb_inserter_buf_rows)
    {
        flush();
    }

    return true;
}

void TtbRowInserter::finishInsert()
{
    flush();
}

bool TtbRowInserter::flush()
{
    if (m_buf_row >= 0)
    {
        m_file->appendRows(m_buf, m_buf_row);
        m_buf_row = 0;
        m_row.setRowPtr(m_buf);
        memset(m_buf, 0, ttb_inserter_buf_rows * m_row_width);
    }

    return true;
}

