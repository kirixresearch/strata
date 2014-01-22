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
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/rowidarray.h"
#include "../xdcommon/keylayout.h"
#include <kl/md5.h>
#include <kl/math.h>

// TtbSet class implementation

TtbSet::TtbSet(FsDatabase* database) : XdfsBaseSet(database)
{
    m_update_buf = NULL;
    m_update_row.setTable(&m_file);
}

TtbSet::~TtbSet()
{
    if (m_file.isOpen())
        m_file.close();

    delete[] m_update_buf;
}

bool TtbSet::init(const std::wstring& filename)
{
    if (!m_file.open(filename))
        return false;

    // set the set info filename
    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    
    std::wstring config_file_path = kl::beforeLast(filename, '.');
    config_file_path += L".xdi";

    setConfigFilePath(config_file_path);

    return true;
}




xd::IStructurePtr TtbSet::getStructure()
{
    // if we can't open the file, return an empty structure
    if (!m_file.isOpen())
        return xcm::null;

    // get structure from ttb file
    xd::IStructurePtr s = m_file.getStructure();

    XdfsBaseSet::appendCalcFields(s);
    return s;
}

xd::IRowInserterPtr TtbSet::getRowInserter()
{
    TtbRowInserter* inserter = new TtbRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}

IXdsqlRowDeleterPtr TtbSet::getRowDeleter()
{
    TtbSetRowDeleter* deleter = new TtbSetRowDeleter(m_database, this);
    return static_cast<IXdsqlRowDeleter*>(deleter);
}


xd::IIteratorPtr TtbSet::createIterator(const std::wstring& columns,
                                        const std::wstring& order,
                                        xd::IJob* job)
{
    if (order.empty())
    {
        TtbIterator* iter = new TtbIterator(m_database);
        if (!iter->init(this, m_file.getFilename(), columns))
        {
            delete iter;
            return xcm::null;
        }
        
        iter->goFirst();
        return static_cast<xd::IIterator*>(iter);
    }
    
    IIndex* idx;

    idx = lookupIndexForOrder(order);
    if (idx)
    {
        if (job)
        {
            time_t t = time(NULL);
            IJobInternalPtr ijob = job;
            ijob->setStartTime(t);
            ijob->setFinishTime(t);
            ijob->setCurrentCount(0);
            ijob->setMaxCount(0);
            ijob->setStatus(xd::jobFinished);
        }

        xd::IIteratorPtr data_iter = createIterator(columns, L"", NULL);

        xd::IIteratorPtr res = createIteratorFromIndex(data_iter,
                                      idx,
                                      columns,
                                      order,
                                      getObjectPath());
        idx->unref();
        return res;
    }

    // find out where the database should put temporary files
    std::wstring temp_directory = m_database->getTempFileDirectory();
 
    // create a unique index file name with .idx extension
    std::wstring index_filename = getUniqueString() + L".idx";

    // generate a full path name from the temp path and unique idx filename
    std::wstring full_index_filename = makePathName(temp_directory, L"", index_filename);


    idx = createExternalIndex(m_database,
                              getObjectPath(),
                              full_index_filename,
                              temp_directory,
                              order,
                              true,
                              true,
                              job);
    if (!idx)
    {
        return xcm::null;
    }

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



void TtbSet::refreshUpdateBuffer()
{
    KL_AUTO_LOCK(m_indexes_mutex);

    refreshIndexEntries();

    if (!m_update_buf)
    {
        m_update_buf = new unsigned char[m_file.getRowWidth()];
        m_update_row.setRowPtr(m_update_buf);
        TtbIterator* iter = new TtbIterator(m_database);
        iter->initFromBuffer(this, &m_file, m_update_buf, L"");
        m_update_iter = static_cast<xd::IIterator*>(iter);
    }


    std::vector<XdfsIndexEntry>::iterator idx_it, idx_it_end = m_indexes.end();
    for (idx_it = m_indexes.begin(); idx_it != idx_it_end; ++idx_it)
    {
        if (!idx_it->key_expr)
        {
            idx_it->key_expr = new KeyLayout;
            if (idx_it->key_expr->setKeyExpr(static_cast<xd::IIterator*>(m_update_iter), idx_it->expr))
            {
                idx_it->orig_key.setDataSize(idx_it->key_length);
                memcpy(idx_it->orig_key.getData(), idx_it->key_expr->getKey(), idx_it->key_length);
            }
             else
            {
                // key expression could not be parsed
                delete idx_it->key_expr;
                idx_it->key_expr = NULL;
                idx_it->update = false;
            }
        }
    }
}


bool TtbSet::updateRow(xd::rowid_t rowid,
                       xd::ColumnUpdateInfo* info,
                       size_t info_size)
{
    KL_AUTO_LOCK(m_indexes_mutex);
    
    size_t coli;
    xd::ColumnUpdateInfo* col_it;

    // read the row
    refreshUpdateBuffer();
    xd::rowpos_t row = rowidGetRowPos(rowid);
    m_file.getRow(row, m_update_buf);

    // determine which indexes need updating
    
    std::vector<XdfsIndexEntry>::iterator idx_it, idx_it_end = m_indexes.end();
    for (idx_it = m_indexes.begin(); idx_it != idx_it_end; ++idx_it)
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
    }



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


    // update the indexes
    for (idx_it = m_indexes.begin(); idx_it != idx_it_end; ++idx_it)
    {
        if (!idx_it->update || !idx_it->key_expr)
            continue;

        // if present key is the same as the last key, don't do anything
        const unsigned char* new_key = idx_it->key_expr->getKey();
        
        if (0 == memcmp(idx_it->orig_key.getData(), new_key, idx_it->key_length))
            continue;

        
        IIndexIterator* index_iter;

        index_iter = seekRow(idx_it->index,
                             idx_it->orig_key.getData(),
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



    return true;
}




bool TtbSet::deleteRow(xd::rowid_t rowid)
{
    if (!m_file.deleteRow(rowid))
        return false;

    KL_AUTO_LOCK(m_indexes_mutex);
    
    refreshIndexEntries();

    if (m_indexes.size() == 0)
        return true;

    // refresh update buffer if necessary and read the row
    refreshUpdateBuffer();
    xd::rowpos_t row = rowidGetRowPos(rowid);
    m_file.getRow(row, m_update_buf);


    std::vector<XdfsIndexEntry>::iterator idx_it;
    IIndexIterator* iter;

    for (idx_it = m_indexes.begin(); idx_it != m_indexes.end(); ++idx_it)
    {
        if (!idx_it->key_expr)
            continue;

        iter = seekRow(idx_it->index,
                       idx_it->key_expr->getKey(),
                       idx_it->key_length,
                       rowid);

        if (iter)
        {
            idx_it->index->remove(iter);
            iter->unref();
        }
    }


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










// TtbSetRowDeleter class implementation

TtbSetRowDeleter::TtbSetRowDeleter(FsDatabase* db, TtbSet* set)
{
    m_set = set;
    m_set->ref();

    m_rowid_array = new RowIdArray(db->getTempFileDirectory());
}

TtbSetRowDeleter::~TtbSetRowDeleter()
{
    delete m_rowid_array;

    m_set->unref();
}


void TtbSetRowDeleter::startDelete()
{
}

bool TtbSetRowDeleter::deleteRow(const xd::rowid_t& rowid)
{
    m_rowid_array->append(rowid);
    return true;
}

void TtbSetRowDeleter::finishDelete()
{
    if (m_rowid_array)
    {
        m_rowid_array->goFirst();
        while (!m_rowid_array->isEof())
        {
            m_set->deleteRow(m_rowid_array->getItem());
            m_rowid_array->goNext();
        }
    }
}

void TtbSetRowDeleter::cancelDelete()
{
    delete m_rowid_array;
    m_rowid_array = NULL;
}

