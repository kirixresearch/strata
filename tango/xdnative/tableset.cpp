/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-03-15
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <ctime>
#include <xcm/xcm.h>
#include <kl/klib.h>
#include "tango.h"
#include "tableset.h"
#include "nativetbl.h"
#include "bufiterator.h"
#include "util.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/util.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/keylayout.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/rowidarray.h"
#include "../xdcommon/indexinfo.h"

const int tableiterator_read_ahead_buffer_size = 2097152;


// TableIterator class implementation

TableIterator::TableIterator() : BaseIterator()
{
    m_row_count = 0;
    m_table = NULL;
    m_table_rowwidth = 0;
    m_read_ahead_rowcount = 0;
    m_buf = NULL;
    m_rowpos_buf = NULL;
    m_row_num = 1;
    m_buf_rowcount = 0;
    m_buf_pos = 0;
    m_eof = false;
    m_bof = false;
    m_include_deleted = false;

    setIteratorFlags(tango::ifFastSkip | tango::ifFastRowCount);
}

TableIterator::~TableIterator()
{
    if (m_table)
    {
        m_table->unref();
    }

    delete[] m_buf;
    delete[] m_rowpos_buf;
}

bool TableIterator::init(tango::IDatabase* database,
                         tango::ISet* set,
                         ITable* table,
                         const std::wstring& columns)
{
    if (!BaseIterator::init(database, set, columns))
        return false;

    registerTable(table);

    m_table = table;
    m_table->ref();

    m_table_ord = table->getTableOrdinal();
    m_table_rowwidth = m_table->getRowWidth();

    m_read_ahead_rowcount = tableiterator_read_ahead_buffer_size/m_table_rowwidth;
    if (m_read_ahead_rowcount == 0)
        m_read_ahead_rowcount = 1;
    if (m_read_ahead_rowcount > 10000)
    {
        // some tables with a very skinny row width could have millions of
        // read ahead rows.  On files with a delete map this can lead to
        // performance degredation.  Limit read-ahead rows to 10000
        m_read_ahead_rowcount = 10000;
    }

    m_buf = new unsigned char[m_read_ahead_rowcount * m_table_rowwidth];
    m_rowpos_buf = new tango::rowpos_t[m_read_ahead_rowcount];
    
    return true;
}

tango::IIteratorPtr TableIterator::clone()
{
    TableIterator* new_iter = new TableIterator;
    if (!baseClone(new_iter))
    {
        delete new_iter;
        return xcm::null;
    }

    new_iter->m_table = m_table;
    new_iter->m_table->ref();
    new_iter->m_table_rowwidth = m_table_rowwidth;
    new_iter->registerTable(new_iter->m_table);
    new_iter->m_table_ord = m_table_ord;
    new_iter->m_row_count = m_row_count;
    new_iter->m_eof = m_eof;
    new_iter->m_bof = m_bof;
    new_iter->m_read_ahead_rowcount = m_read_ahead_rowcount;

    new_iter->m_buf = new unsigned char[m_table_rowwidth * m_read_ahead_rowcount];
    new_iter->m_rowpos_buf = new tango::rowpos_t[m_read_ahead_rowcount];
    new_iter->m_buf_rowcount = m_buf_rowcount;
    new_iter->m_buf_pos = m_buf_pos;
    memcpy(new_iter->m_buf, m_buf, m_table_rowwidth * m_buf_rowcount);
    memcpy(new_iter->m_rowpos_buf, m_rowpos_buf, sizeof(tango::rowpos_t) * m_buf_rowcount);

    new_iter->updatePosition();

    return static_cast<tango::IIterator*>(new_iter);
}

void TableIterator::updatePosition()
{
    m_rowptr = (m_buf + (m_buf_pos * m_table_rowwidth));
    setRowId(rowidCreate(m_table_ord, m_rowpos_buf[m_buf_pos]));
}


const unsigned char* TableIterator::getRowBuffer()
{
    return m_rowptr;
}

int TableIterator::getRowBufferWidth()
{
    return m_table_rowwidth;
}




void TableIterator::goFirst()
{
    int read_ahead_rowcount = m_read_ahead_rowcount;
    if (read_ahead_rowcount > 100)
        read_ahead_rowcount = 100;
        
    m_buf_rowcount = m_table->getRows(m_buf,
                                      m_rowpos_buf,
                                      0,
                                      1,
                                      read_ahead_rowcount,
                                      true,
                                      m_include_deleted);

    m_buf_pos = 0;
    m_row_num = 1;
    m_eof = (m_buf_rowcount == 0 ? true : false);
    m_bof = (m_buf_rowcount == 0 ? true : false);

    if (m_eof)
    {
        memset(m_buf, 0, m_table_rowwidth);
    }
     else
    {
        updatePosition();
    }
}

void TableIterator::goLast()
{
}


void TableIterator::skip(int delta)
{
    if (delta == 0)
        return;

    
    if (delta < 0)
    {
        if (m_bof)
            return;

        int new_pos = m_buf_pos+delta;
        m_row_num += delta;

        if (new_pos >= 0 && new_pos < m_buf_rowcount)
        {
            m_buf_pos = new_pos;
        }
         else
        {
            m_buf_rowcount = m_table->getRows(m_buf,
                                              m_rowpos_buf,
                                              delta,
                                              m_rowpos_buf[m_buf_pos],
                                              1,
                                              true,
                                              m_include_deleted);

            m_bof = (m_buf_rowcount == 0);

            if (m_bof)
            {
                m_buf_pos = 0;
                m_row_num = 0;
                m_rowpos_buf[0] = 0;
                return;
            }

            m_eof = false;
            m_buf_pos = 0;
        }

        updatePosition();
    }
     else
    {
        // skip forward
        if (m_eof)
            return;

        int new_pos = m_buf_pos+delta;
        m_row_num += delta;

        if (new_pos >= 0 && new_pos < m_buf_rowcount)
        {
            m_buf_pos = new_pos;
        }
         else
        {
            // ask table to find out the next start row for our buffer
            m_buf_rowcount = m_table->getRows(m_buf,
                                              m_rowpos_buf,
                                              delta,
                                              m_rowpos_buf[m_buf_pos],
                                              m_read_ahead_rowcount,
                                              true,
                                              m_include_deleted);

            m_buf_pos = 0;
            m_eof = (m_buf_rowcount == 0);

            if (m_eof)
            {
                m_rowpos_buf[0] = m_table->getRowCount(NULL)+1;
                m_row_num = m_rowpos_buf[0];
                return;
            }

            m_bof = false;
        }

        updatePosition();
    }
}

bool TableIterator::bof()
{
    return m_bof;
}

bool TableIterator::eof()
{
    return m_eof;
}

bool TableIterator::seek(const unsigned char* key, int length, bool soft)
{
    // keys on table iterators indicate rowid
    if (length != sizeof(tango::rowid_t))
        return false;

    tango::rowid_t* rowid = (tango::rowid_t*)key;

    if (rowidGetTableOrd(*rowid) != m_table_ord)
        return false;

    tango::rowpos_t row = rowidGetRowPos(*rowid);

    m_buf_rowcount = m_table->getRows(m_buf,
                                      m_rowpos_buf,
                                      0,
                                      row,
                                      1,
                                      true,
                                      m_include_deleted);
    m_buf_pos = 0;
    m_eof = (m_buf_rowcount == 0 ? true : false);
    m_bof = (m_buf_rowcount == 0 ? true : false);

    if (m_eof || m_bof)
    {
        memset(m_buf, 0, m_table_rowwidth);
        return false;
    }
     else
    {
        updatePosition();
    }

    return true;
}

bool TableIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool TableIterator::setPos(double pct)
{
    return false;
}

double TableIterator::getPos()
{
    double d = (double)(long long)rowidGetRowPos(m_rowid);
    double row_count = (double)(long long)m_row_count;
    if (kl::dblcompare(row_count, 0.0) == 0)
        return 0.0;

    return (d/row_count);
}




void TableIterator::flushRow()
{
    if (!m_row_dirty)
    {
        return;
    }

    m_table->writeRow(rowidGetRowPos(m_rowid), m_rowptr);

    m_row_dirty = false;
}

tango::rowpos_t TableIterator::getRowNumber()
{
    return m_row_num;
}

void TableIterator::onSetRowUpdated(tango::rowid_t rowid)
{
    if (m_row_dirty)
        return;

    if (rowid == m_rowid)
    {
        if (m_rowptr)
        {
            // if the current row was changed, reread it
            m_table->getRow(rowidGetRowPos(rowid), m_rowptr);
        }
    }
     else
    {
        // some other row was changed, so invalidate our buffer
        m_buf_rowcount = 0;
    }
}

void TableIterator::onSetStructureUpdated()
{
    BaseIterator::onSetStructureUpdated();

    // since the structure was updated, we need to refresh our cache

    m_table_rowwidth = m_table->getRowWidth();

    if (m_rowptr)
    {
        m_table->getRow(rowidGetRowPos(m_rowid), m_rowptr);
    }

    m_buf_rowcount = 0;
}






// TableSet class implementation

TableSet::TableSet(tango::IDatabase* database) : BaseSet(database)
{
    m_ordinal = 0;
    m_ofspath = L"";
    m_row_count = 0;
    m_phys_row_count = 0;
    m_table = NULL;
    m_update_iter = NULL;
    m_update_buf = NULL;
    m_idxrefresh_time = 0;

    setSetFlags(tango::sfFastRowCount);
}


TableSet::~TableSet()
{
    XCM_AUTO_LOCK(m_update_mutex);

    // release table

    if (m_table)
    {
        m_table->removeEventHandler(this);
        m_table->unref();
        m_table = NULL;
    }

    if (m_update_iter)
    {
        m_update_iter->unref();
        m_update_iter = NULL;
    }

    if (m_update_buf)
    {
        delete[] m_update_buf;
        m_update_buf = NULL;
    }

    // release all indexes
    
    std::vector<IndexEntry>::iterator it;
    for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
    {
        if (it->index)
            it->index->unref();

        if (it->orig_key)
            delete[] it->orig_key;

        if (it->key_expr)
            delete it->key_expr;

        // if the index was temporary, try to delete the
        // index file itself
        if (it->tag.length() == 0)
        {
            std::wstring full_index_filename;
            full_index_filename = makePathName(m_dbi->getBasePath(),
                                               L"temp",
                                               it->filename);

            xf_remove(full_index_filename);
        }
    }

    m_indexes.clear();
}



bool TableSet::create(tango::IStructure* struct_config, const std::wstring& path)
{
    IDatabaseInternalPtr dbi = m_database;

    // generate a unique filename for the table
    std::wstring table_filename = dbi->getUniqueFilename();
    table_filename += L".ttb";

    // create the table
    if (!NativeTable::create(table_filename, struct_config))
        return false;

    // allocate an ordinal for us
    m_ordinal = dbi->allocOrdinal();
    if (m_ordinal == 0)
        return false;

    if (!dbi->setOrdinalTable(m_ordinal, table_filename))
        return false;

    // this is a table set, so we must set our id
    // to the corresponding ordinal
    setSetId(getTableSetId(m_ordinal));

    // create the ofs file
    INodeValuePtr file = dbi->createNodeFile(path);
    if (!file)
        return false;
    
    dbi->setFileType(path, tango::filetypeTable);

    INodeValuePtr setid_node = file->getChild(L"set_id", true);
    if (!setid_node)
        return false;
    setid_node->setString(getSetId());
    setid_node.clear();
    file.clear();

    // add the calculated fields
    int col_count = struct_config->getColumnCount();
    tango::IColumnInfoPtr colinfo;
    for (int col = 0; col < col_count; ++col)
    {
        colinfo = struct_config->getColumnInfoByIdx(col);
        if (colinfo->getCalculated())
        {
            createCalcField(colinfo);
        }
    }

    // write out set definition file
    m_ofspath = path;
    save();

    // load up the table
    NativeTable* table = new NativeTable(m_database.p);
    table->ref();
    if (!table->open(table_filename, m_ordinal))
    {
        table->unref();
        return false;
    }

    m_table = table;
    m_table->addEventHandler(this);

    // initialize internal structure
    m_structure = m_table->getStructure();

    updateRowCount();

    m_dbi->registerSet(this);
    return true;
}


bool TableSet::loadTable(const std::wstring& tbl_filename)
{
    NativeTable* table = new NativeTable(m_database.p);
    table->ref();

    if (!table->open(tbl_filename, m_ordinal))
    {
        table->unref();
        return false;
    }

    m_table = table;
    m_table->addEventHandler(this);

    // initialize internal structure
    m_structure = m_table->getStructure();

    updateRowCount();

    return true;
}


bool TableSet::load(INodeValuePtr set_file)
{
    IDatabaseInternalPtr dbi = m_database;

    // verify that this is the corrent set type
    INodeValuePtr settype_node = set_file->getChild(L"set_type", false);
    if (!settype_node)
        return false;
    if (wcscmp(settype_node->getString().c_str(), L"table") != 0)
        return false;

    // load set id
    INodeValuePtr setid_node = set_file->getChild(L"set_id", false);
    if (!setid_node)
        return false;
    setSetId(setid_node->getString());

    // load ordinal
    INodeValuePtr ordinal_node = set_file->getChild(L"ordinal", false);
    if (!ordinal_node)
        return false;

    m_ordinal = ordinal_node->getInteger();

    // load ofs path
    INodeValuePtr ofspath_node = set_file->getChild(L"ofs_path", false);
    if (!ofspath_node)
        return false;

    m_ofspath = ofspath_node->getString();

    // open the table
    std::wstring tbl_filename = dbi->getTableFilename(m_ordinal);
    if (tbl_filename.length() == 0)
        return false;

    if (!loadTable(tbl_filename))
        return false;

    refreshIndexEntries();
    
    m_dbi->registerSet(this);


    return true;
}



bool TableSet::save()
{
    XCM_AUTO_LOCK(m_update_mutex);

    INodeValuePtr set_file = openSetDefinition(true);

    if (!set_file)
    {
        return false;
    }

    // write out values

    INodeValuePtr settype_node = set_file->getChild(L"set_type", true);
    settype_node->setString(L"table");

    INodeValuePtr setid_node = set_file->getChild(L"set_id", true);
    setid_node->setString(getSetId());

    INodeValuePtr ofspath_node = set_file->getChild(L"ofs_path", true);
    ofspath_node->setString(m_ofspath);

    INodeValuePtr ordinal_node = set_file->getChild(L"ordinal", true);
    ordinal_node->setInteger(m_ordinal);

    return true;
}


void TableSet::onOfsPathChanged(const std::wstring& new_path)
{
    m_ofspath = new_path;
}

std::wstring TableSet::getObjectPath()
{
    return m_ofspath;
}

tango::IRowInserterPtr TableSet::getRowInserter()
{
    TableSetRowInserter* inserter = new TableSetRowInserter(this);
    return static_cast<tango::IRowInserter*>(inserter);
}


tango::IRowDeleterPtr TableSet::getRowDeleter()
{
    TableSetRowDeleter* deleter = new TableSetRowDeleter(m_database, this);
    return static_cast<tango::IRowDeleter*>(deleter);
}

bool TableSet::restoreDeleted() // ISetRestoreDeleted
{
    return m_table->restoreDeleted();
}



void TableSet::refreshUpdateBuffer()
{
    XCM_AUTO_LOCK(m_update_mutex);

    if (m_update_buf)
    {
        delete[] m_update_buf;
    }
    
    if (m_update_iter)
    {
        m_update_iter->unref();
    }

    // create the update iterator and its associated buffer
    m_update_buf = new unsigned char[m_table->getRowWidth()];
    m_update_iter = new BufIterator;
    m_update_iter->ref();
    m_update_iter->init(m_database, static_cast<tango::ISet*>(this));
    m_update_iter->setRowBuffer(m_update_buf, m_table->getRowWidth());
}





bool TableSet::prepareIndexEntry(IndexEntry& e)
{
    XCM_AUTO_LOCK(m_update_mutex);

    if (!e.index)
        return false;

    if (!m_update_iter)
        refreshUpdateBuffer(); // indexes key builder use m_update_iter

    delete e.key_expr;  
    delete[] e.orig_key;

    e.key_length = e.index->getKeyLength();
    e.update = false;
    e.orig_key = new unsigned char[e.key_length];
    e.key_expr = new KeyLayout;
    e.key_expr->setKeyExpr(static_cast<tango::IIterator*>(m_update_iter),
                           e.expr);

    tango::IStructurePtr structure = getStructure();
    if (structure.isNull())
        return false;
        
    e.active_columns.resize(structure->getColumnCount(), false);
    std::vector<std::wstring> cols;
    kl::parseDelimitedList(e.expr, cols, L',', true);



    std::vector<std::wstring>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        // get column name (remove possible 'ASC' or 'DESC'
        // from the end of the index piece

        std::wstring piece = *it;
        std::wstring colname;

        kl::trim(piece);

        if (piece.find_last_of(L' ') != -1)
        {
            colname = kl::beforeLast(piece, L' ');
        }
         else
        {
            colname = piece;
        }

        tango::IColumnInfoPtr info = structure->getColumnInfo(colname);

        if (info.isOk())
        {
            if (info->getCalculated())
            {
                // in the case of a calculated field, the index may be
                // dependant on those fields which make up the formula
                
                std::vector<std::wstring> fields_used;
                std::vector<std::wstring>::iterator it;

                fields_used = getFieldsInExpr(info->getExpression(),
                                              structure,
                                              true);

                for (it = fields_used.begin();
                     it != fields_used.end();
                     ++it)
                {
                    info = structure->getColumnInfo(*it);
                    if (!info->getCalculated())
                    {
                        e.active_columns[info->getColumnOrdinal()] = true;
                    }
                }
            }
             else
            {
                e.active_columns[info->getColumnOrdinal()] = true;
            }
        }
    }

    return true;
}




void TableSet::refreshIndexEntries()
{
    XCM_AUTO_LOCK(m_update_mutex);

    IDatabaseInternalPtr dbi;
    dbi = m_database;


    INodeValuePtr set_file = openSetDefinition(true);
    if (!set_file)
    {
        return;
    }


    std::vector<IndexEntry>::iterator idx_it;


    // get rid of all indexes without tags (all temporary indexes)

    std::vector<IIndex*> to_delete;
    std::vector<IIndex*>::iterator temp_it;

    for (idx_it = m_indexes.begin();
         idx_it != m_indexes.end();
         ++idx_it)
    {
        if (idx_it->tag.length() == 0)
        {
            to_delete.push_back(idx_it->index);
        }
    }

    for (temp_it = to_delete.begin();
         temp_it != to_delete.end();
         ++temp_it)
    {
        deleteIndexInternal(*temp_it);
    }



    // load indexes

    std::vector<IndexEntry> old_entries;
    old_entries = m_indexes;

    m_indexes.clear();

    std::vector<std::wstring> bad_index_tags;

    INodeValuePtr indexes_node = set_file->getChild(L"indexes", false);
    if (indexes_node)
    {
        int index_count = indexes_node->getChildCount();
        int i;

        for (i = 0; i < index_count; ++i)
        {
            INodeValuePtr index = indexes_node->getChildByIdx(i);

            std::wstring tag = index->getName();

            INodeValuePtr idx_expr_node = index->getChild(L"expression", false);
            if (idx_expr_node.isNull())
            {
                bad_index_tags.push_back(tag);
                continue;
            }

            INodeValuePtr idx_filename_node = index->getChild(L"filename", false);
            if (idx_filename_node.isNull())
            {
                bad_index_tags.push_back(tag);
                continue;
            }

            INodeValuePtr active_node = index->getChild(L"active", false);
            if (active_node.isOk())
            {
                if (!active_node->getBoolean())
                    continue;
            }

            // find out if we already have the index loaded
            bool found = false;
            for (idx_it = old_entries.begin();
                 idx_it != old_entries.end();
                 ++idx_it)
            {
                if (0 == wcscasecmp(tag.c_str(), idx_it->tag.c_str()))
                {
                    m_indexes.push_back(*idx_it);
                    old_entries.erase(idx_it);
                    found = true;
                    break;
                }
            }


            if (!found)
            {
                IndexEntry i;
                i.tag = tag;
                i.expr = idx_expr_node->getString();
                i.filename = idx_filename_node->getString();
                i.index = NULL;
                i.update = false;
                i.key_length = 0;
                i.orig_key = NULL;
                i.key_expr = NULL;

                // try to open the index file
                std::wstring full_idx_filename;
                full_idx_filename = makePathName(dbi->getBasePath(),
                                                 L"data",
                                                 i.filename);

                ExIndex* idx = new ExIndex;
                if (!idx->open(full_idx_filename))
                {
                    // slate the tag for removal and
                    // delete the bad index file if it exists
                    bad_index_tags.push_back(tag);
                    xf_remove(full_idx_filename);

                    delete idx;
                    continue;
                }

                i.index = idx;
                
                if (prepareIndexEntry(i))
                {
                    m_indexes.push_back(i);
                }
            }
             else
            {
                prepareIndexEntry(m_indexes.back());
            }
        }
    }


    // delete the rejected indexes and remove them
    // from the file index registry

    std::vector<std::wstring>::iterator it;
    for (it = bad_index_tags.begin();
         it != bad_index_tags.end();
         ++it)
    {
        indexes_node->deleteChild(*it);
    }
}

tango::IIndexInfoEnumPtr TableSet::getIndexEnum()
{
    XCM_AUTO_LOCK(m_update_mutex);

    xcm::IVectorImpl<tango::IIndexInfoPtr>* indexes;
    indexes = new xcm::IVectorImpl<tango::IIndexInfoPtr>;

    if (!m_table)
    {
        return indexes;
    }

    unsigned long long mtime = m_table->getStructureModifyTime();
    if (m_idxrefresh_time != mtime)
    {
        m_idxrefresh_time = mtime;
        refreshIndexEntries();
    }


    std::vector<IndexEntry>::iterator it;
    for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
    {
        if (it->tag.length() > 0 && it->index)
        {
            IndexInfo* ii = new IndexInfo;
            ii->setTag(it->tag);
            ii->setExpression(it->expr);

            indexes->append(static_cast<tango::IIndexInfo*>(ii));
        }
    }

    return indexes;
}


tango::IIndexInfoPtr TableSet::createIndex(const std::wstring& tag,
                                           const std::wstring& expr,
                                           tango::IJob* job)
{
    XCM_AUTO_LOCK(m_update_mutex);

    if (tag.length() == 0)
        return xcm::null;

    IDatabaseInternalPtr dbi = m_database;

    std::wstring lower_tag = tag;
    kl::makeLower(lower_tag);

    // find out of the index already exists
    
    INodeValuePtr set_file = openSetDefinition(true);
    if (!set_file)
    {
        if (job)
        {
            IJobInternalPtr ijob = job;
            ijob->setStatus(tango::jobFailed);
        }
        return xcm::null;
    }

    INodeValuePtr indexes_node = set_file->getChild(L"indexes", false);
    if (indexes_node)
    {
        int index_count = indexes_node->getChildCount();
        int i;

        for (i = 0; i < index_count; ++i)
        {
            INodeValuePtr index = indexes_node->getChildByIdx(i);
            std::wstring ntag = index->getName();

            if (0 == wcscasecmp(tag.c_str(), ntag.c_str()))
            {
                // check if the index file exists

                INodeValuePtr filename_node;
                filename_node = index->getChild(L"filename", false);
                if (filename_node.isOk())
                {
                    std::wstring index_filename;
                    index_filename = filename_node->getString();

                    std::wstring full_index_filename;
                    full_index_filename =  makePathName(dbi->getBasePath(),
                                            L"data",
                                            index_filename);
                    if (!xf_get_file_exist(full_index_filename))
                        break;
                }

                // tag already exists and is valid
                if (job)
                {
                    IJobInternalPtr ijob = job;
                    ijob->setStatus(tango::jobFailed);
                }

                return xcm::null;
            }
        }

        // reserve a spot
        indexes_node->createChild(lower_tag);
    }

    indexes_node.clear();
    set_file.clear();


    // create the index

    std::wstring index_filename = getUniqueString();
    index_filename += L".idx";
    
    std::wstring full_index_filename;
    full_index_filename =  makePathName(dbi->getBasePath(),
                                        L"data",
                                        index_filename);


    IIndex* idx = createExternalIndex(m_database,
                                      getObjectPath(),
                                      full_index_filename,
                                      dbi->getTempPath(),
                                      expr,
                                      true,
                                      job);


    if (!idx)
    {
        // delete the space we reserved for this tag
        set_file = openSetDefinition(true);
        if (set_file)
        {
            indexes_node = set_file->getChild(L"indexes", true);
            if (indexes_node)
            {
                indexes_node->deleteChild(lower_tag);
            }
        }


        if (job)
        {
            IJobInternalPtr ijob = job;
            ijob->setStatus(tango::jobFailed);
        }

        return xcm::null;
    }


    IndexEntry e;
    e.tag = tag;
    kl::makeLower(e.tag);
    e.expr = expr;
    e.filename = index_filename;
    e.index = idx;    // (already ref'ed)
    e.update = false;
    e.key_length = 0;
    e.orig_key = NULL;
    e.key_expr = NULL;

    if (prepareIndexEntry(e))
    {
        m_indexes.push_back(e);
    }


    set_file = openSetDefinition(true);
    if (set_file)
    {
        indexes_node = set_file->getChild(L"indexes", true);
        if (indexes_node)
        {
            INodeValuePtr index_node = indexes_node->getChild(e.tag, true);

            INodeValuePtr expr_node = index_node->getChild(L"expression", true);
            expr_node->setString(e.expr);

            INodeValuePtr filename_node = index_node->getChild(L"filename", true);
            filename_node->setString(e.filename);
        }
    }

    m_table->setStructureModified();


    IndexInfo* ii = new IndexInfo;
    ii->setTag(tag);
    ii->setExpression(expr);

    return static_cast<tango::IIndexInfo*>(ii);
}



bool TableSet::deleteIndexInternal(IIndex* idx_to_delete)
{
    XCM_AUTO_LOCK(m_update_mutex);

    bool success = false;

    IDatabaseInternalPtr dbi = m_database;

    std::wstring tag_to_delete;

    std::vector<IndexEntry>::iterator it;
    for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
    {
        if (it->index == idx_to_delete)
        {
            if (it->index)
            {
                it->index->unref();
            }

            delete[] it->orig_key;
            delete it->key_expr;


            std::wstring full_index_filename;
            full_index_filename = makePathName(dbi->getBasePath(),
                                               L"data",
                                               it->filename);

            if (xf_get_file_exist(full_index_filename))
            {
                bool success = xf_remove(full_index_filename);
                if (!success)
                {
                    // add file to trash bin, because it's currently in use
                    dbi->addFileToTrash(it->filename);
                }
            }

            tag_to_delete = it->tag;
            
            success = true;

            m_indexes.erase(it);
            break;
        }
    }


    if (tag_to_delete.length() > 0)
    {
        kl::makeLower(tag_to_delete);

        INodeValuePtr set_file = openSetDefinition(true);
        if (set_file.isOk())
        {
            INodeValuePtr indexes_node;
            indexes_node = set_file->getChild(L"indexes", true);
            if (indexes_node.isOk())
            {
                bool result = indexes_node->deleteChild(tag_to_delete);
                indexes_node.clear();
                set_file.clear();
                m_table->setStructureModified();
                return result;
            }
        }
    }

    return success;
}


bool TableSet::deleteIndex(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_update_mutex);

    std::vector<IndexEntry>::iterator it;
    for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
    {
        if (0 == wcscasecmp(it->tag.c_str(), name.c_str()))
        {
            return deleteIndexInternal(it->index);
        }
    }

    return false;
}

bool TableSet::renameIndex(const std::wstring& name,
                           const std::wstring& new_name)
{
    XCM_AUTO_LOCK(m_update_mutex);


    // delete the space we reserved for this tag
    INodeValuePtr set_file = openSetDefinition(false);
    if (set_file.isNull())
        return false;

    INodeValuePtr indexes_node;
    indexes_node = set_file->getChild(L"indexes", true);
    if (indexes_node.isNull())
        return false;

    std::wstring lower_name = name;
    kl::makeLower(lower_name);

    std::wstring lower_new_name = new_name;
    kl::makeLower(lower_new_name);

    m_table->setStructureModified();

    return indexes_node->renameChild(lower_name, lower_new_name);
}

bool TableSet::deleteAllIndexes()
{
    XCM_AUTO_LOCK(m_update_mutex);

    IDatabaseInternalPtr dbi = m_database;

    std::vector<IndexEntry>::iterator it;
    for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
    {
        if (it->index)
        {
            it->index->unref();
        }

        delete[] it->orig_key;
        delete it->key_expr;

        std::wstring full_index_filename;
        full_index_filename = makePathName(dbi->getBasePath(),
                                           L"data",
                                           it->filename);
        if (xf_get_file_exist(full_index_filename))
        {
            xf_remove(full_index_filename);
        }
    }

    m_indexes.clear();



    INodeValuePtr set_file = openSetDefinition(true);
    if (set_file.isOk())
    {
        INodeValuePtr indexes_node;
        indexes_node = set_file->getChild(L"indexes", true);
        if (indexes_node.isOk())
        {
            indexes_node->deleteAllChildren();
        }
    }
    
    set_file.clear();

    m_table->setStructureModified();

    return true;
}

TableIterator* TableSet::createPhysicalIterator(const std::wstring& columns,
                                                bool include_deleted)
{
    // no sort order, so create a physical order iterator
    TableIterator* it = new TableIterator;

    IDatabaseInternalPtr dbi;
    dbi = m_database;
    ITablePtr tbl = dbi->openTableByOrdinal(m_ordinal);

    if (!it->init(m_database, static_cast<tango::ISet*>(this), tbl, columns))
    {
        delete it;
        return NULL;
    }

    // tell physical iterator how many data records are in file
    it->m_row_count = m_phys_row_count;
    it->m_include_deleted = include_deleted;
    it->goFirst();
    return it;
}



tango::IIteratorPtr TableSet::createIterator(const std::wstring& columns,
                                             const std::wstring& expr,
                                             tango::IJob* job)
{

    if (expr.empty())
    {
        // create an unordered iterator
        return static_cast<tango::IIterator*>(createPhysicalIterator(columns, false));
    }

    IDatabaseInternalPtr dbi = m_database;

    // attempt to find a suitable existing index
    IIndex* idx = NULL;
    {
        XCM_AUTO_LOCK(m_update_mutex);

        std::vector<IndexEntry>::iterator it;
        for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
        {
            if (getOrderExpressionMatch(it->expr, expr))
            {
                idx = it->index;
                idx->ref();
                break;
            }
        }
    }

    if (idx)
    {
        if (job)
        {
            IJobInternalPtr ijob = job;
            ijob->setStartTime(time(NULL));
            ijob->setFinishTime(time(NULL));
            ijob->setCurrentCount(m_row_count);
            ijob->setMaxCount(m_row_count);
            ijob->setStatus(tango::jobFinished);
        }

        tango::IIteratorPtr data_iter = createIterator(columns, L"", NULL);

        tango::IIteratorPtr res;
        res = createIteratorFromIndex(data_iter,
                                      idx,
                                      columns,
                                      expr,
                                      getObjectPath());

        idx->unref();
        return res;
    }
     else
    {
        std::wstring index_filename = getUniqueString();
        index_filename += L".idx";
    
        std::wstring full_index_filename;
        full_index_filename =  makePathName(dbi->getBasePath(),
                                            L"temp",
                                            index_filename);


        idx = createExternalIndex(m_database,
                                  getObjectPath(),
                                  full_index_filename,
                                  dbi->getTempPath(),
                                  expr,
                                  true,
                                  job);
        if (!idx)
        {
            return xcm::null;
        }

        IndexEntry entry;
        entry.expr = expr;
        entry.filename = index_filename;
        entry.tag = L"";
        entry.update = false;
        entry.key_length = 0;
        entry.orig_key = NULL;
        entry.key_expr = NULL;
        entry.index = idx;    // (already ref'ed)

        if (!prepareIndexEntry(entry))
        {
            return xcm::null;
        }

        m_indexes.push_back(entry);

        tango::IIteratorPtr data_iter = createIterator(columns, L"", NULL);
        return createIteratorFromIndex(data_iter,
                                       idx,
                                       columns,
                                       expr,
                                       getObjectPath());
    }
}


unsigned long long TableSet::getStructureModifyTime()
{
    if (!m_table)
        return 0;

    return m_table->getStructureModifyTime();
}


bool TableSet::updateRow(tango::rowid_t rowid,
                         tango::ColumnUpdateInfo* info,
                         size_t info_size)
{
    XCM_AUTO_LOCK(m_update_mutex);

    size_t coli;
    tango::ColumnUpdateInfo* col_it;

    if (rowidGetTableOrd(rowid) != m_ordinal)
        return false;
    tango::rowpos_t row = rowidGetRowPos(rowid);

    if (!m_update_iter)
        refreshUpdateBuffer();

    // read the row
    m_table->getRow(row, m_update_buf);

    // determine which indexes need updating
    
    std::vector<IndexEntry>::iterator idx_it;
    for (idx_it = m_indexes.begin();
         idx_it != m_indexes.end(); ++idx_it)
    {
        idx_it->update = false;

        for (coli = 0; coli < info_size; ++coli)
        {
            DataAccessInfo* dai = (DataAccessInfo*)(info[coli].handle);

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


    // modify the buffer

    for (coli = 0; coli < info_size; ++coli)
    {
        col_it = &info[coli];

        tango::objhandle_t handle = col_it->handle;
        DataAccessInfo* dai = (DataAccessInfo*)handle;
        if (!dai)
            continue;

        if (col_it->null)
        {
            m_update_iter->putNull(handle);
            continue;
        }

        switch (dai->type)
        {
            case tango::typeCharacter:
                m_update_iter->putString(handle, col_it->str_val);
                break;

            case tango::typeWideCharacter:
                m_update_iter->putWideString(handle, col_it->wstr_val);
                break;

            case tango::typeNumeric:
            case tango::typeDouble:
                m_update_iter->putDouble(handle, col_it->dbl_val);
                break;

            case tango::typeInteger:
                m_update_iter->putInteger(handle, col_it->int_val);
                break;

            case tango::typeDate:
            case tango::typeDateTime:
                m_update_iter->putDateTime(handle, col_it->date_val);
                break;

            case tango::typeBoolean:
                m_update_iter->putBoolean(handle, col_it->bool_val);
                break;
        }
    }


    // write the resulting row
    m_table->writeRow(row, m_update_buf);


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
                              sizeof(tango::rowid_t));
    }

    return true;
}


tango::IStructurePtr TableSet::getStructure()
{
    tango::IStructurePtr s = m_table->getStructure();
    appendCalcFields(s);
    return s;
}

void TableSet::onTableRowUpdated(tango::rowid_t rowid)
{
    // ask BaseSet to notify all iterators that a row was updated
    fire_onSetRowUpdated(rowid);
}

void TableSet::onTableRowDeleted(tango::rowid_t rowid)
{
    // ask BaseSet to notify all iterators that a row was updated
    fire_onSetRowDeleted(rowid);
}

void TableSet::onTableRowCountUpdated()
{
    updateRowCount();
}

void TableSet::updateRowCount()
{
    m_phys_row_count = m_table->getRowCount(&m_deleted_row_count);
    m_row_count = m_phys_row_count - m_deleted_row_count;
}

tango::rowpos_t TableSet::getRowCount()
{
    return m_row_count;
}




// TableSetRowInserter class implementation

const int inserter_buf_rows = 500;

TableSetRowInserter::TableSetRowInserter(TableSet* set)
{
    m_set = set;
    m_set->ref();

    m_table = set->m_table;
    m_table->ref();

    m_row_width = m_table->getRowWidth();
    m_tbl_filename = m_table->getFilename();
    m_buf = new unsigned char[inserter_buf_rows * m_row_width];

    m_iter = new BufIterator;
    m_iter->ref();
    m_iter->init(m_set->m_database, static_cast<tango::ISet*>(m_set));
    m_iter->setRowBuffer(m_buf, m_row_width);

    // inserter is not valid until startInsert() is called
    m_valid = false;
}

TableSetRowInserter::~TableSetRowInserter()
{
    delete[] m_buf;

    if (m_iter)
        m_iter->unref();

    if (m_table)
        m_table->unref();

    if (m_set)
        m_set->unref();
}

tango::objhandle_t TableSetRowInserter::getHandle(const std::wstring& column)
{
    return m_iter->getHandle(column);
}

tango::IColumnInfoPtr TableSetRowInserter::getInfo(tango::objhandle_t handle)
{
    return m_iter->getInfo(handle);
}

bool TableSetRowInserter::putRawPtr(tango::objhandle_t column_handle,
                                    const unsigned char* value,
                                    int length)
{
    return false;
}

bool TableSetRowInserter::putString(tango::objhandle_t column_handle,
                                    const std::string& value)
{
    return m_iter->putString(column_handle, value);
}

bool TableSetRowInserter::putWideString(tango::objhandle_t column_handle,
                                        const std::wstring& value)
{
    return m_iter->putWideString(column_handle, value);
}

bool TableSetRowInserter::putDouble(tango::objhandle_t column_handle,
                                    double value)
{
    return m_iter->putDouble(column_handle, value);
}

bool TableSetRowInserter::putInteger(tango::objhandle_t column_handle,
                                     int value)
{
    return m_iter->putInteger(column_handle, value);
}

bool TableSetRowInserter::putBoolean(tango::objhandle_t column_handle,
                                     bool value)
{
    return m_iter->putBoolean(column_handle, value);
}

bool TableSetRowInserter::putDateTime(tango::objhandle_t column_handle,
                                      tango::datetime_t value)
{
    return m_iter->putDateTime(column_handle, value);
}


bool TableSetRowInserter::putNull(tango::objhandle_t column_handle)
{
    return m_iter->putNull(column_handle);
}


bool TableSetRowInserter::startInsert(const std::wstring& col_list)
{
    if (!m_buf)
    {
        m_valid = false;
        return false;
    }

    // this enforces correct API usage
    m_valid = true;

    m_buf_row = 0;
    memset(m_buf, 0, inserter_buf_rows * m_row_width);

    // (col_list is not used in the native file format)
    return true;
}


bool TableSetRowInserter::insertRow()
{
    bool result = true;

    if (!m_valid)
    {
        return false;
    }

    m_iter->skip(1);
    m_buf_row++;

    if (m_buf_row == inserter_buf_rows)
    {
        if (!flush())
        {
            result = false;
        }

        // start over for the next batch
        m_buf_row = 0;
    }

    if (m_buf_row == 0)
    {
        memset(m_buf, 0, inserter_buf_rows * m_row_width);
        m_iter->goFirst();

        // if this is the first row in the buffer, we must
        // make sure that there is enough room on the disk for
        // this block, plus room to spare
        
        if (xf_get_free_disk_space(m_tbl_filename) < 50000000)
        {
            // insert failed
            m_valid = false;
            return false;
        }
    }

    return result;
}

void TableSetRowInserter::finishInsert()
{
    flush();
}

bool TableSetRowInserter::flush()
{
    bool result = true;

    if (!m_valid)
        return false;

    if (m_buf_row == 0)
        return true;

    XCM_AUTO_LOCK(m_set->m_update_mutex);

    tango::rowpos_t row = m_table->getRowCount(NULL) + 1;
    tango::tableord_t table_ord = m_table->getTableOrdinal();

    int rows_added = m_table->appendRows(m_buf, m_buf_row);

    m_buf_row = 0;


    // if indexes exist, update them with the new keys

    if (m_set->m_indexes.size() > 0)
    {    
        int i;
        tango::rowid_t rowid;

        m_set->m_update_iter->setRowBuffer(m_buf, m_row_width);

        for (i = 0; i < rows_added; ++i)
        {
            rowid = rowidCreate(table_ord, row);

            std::vector<IndexEntry>::iterator it;

            for (it = m_set->m_indexes.begin();
                 it != m_set->m_indexes.end(); ++it)
            {
                it->index->insert(it->key_expr->getKey(),
                                  it->key_length,
                                  &rowid,
                                  sizeof(tango::rowid_t));
            }

            m_set->m_update_iter->skip(1);
            row++;
        }

        m_set->m_update_iter->setRowBuffer(m_set->m_update_buf, m_row_width);
    }


    return result;
}




// TableSetRowDeleter class implementation

TableSetRowDeleter::TableSetRowDeleter(tango::IDatabasePtr db, TableSet* set)
{
    m_set = set;
    m_set->ref();
    m_table_row_deleter = set->m_table->getRowDeleter();
    
    IDatabaseInternalPtr dbi = db;
    m_rowid_array = new RowIdArray(dbi->getTempPath());
}

TableSetRowDeleter::~TableSetRowDeleter()
{
    m_set->unref();
    delete m_rowid_array;
}


void TableSetRowDeleter::startDelete()
{
}

bool TableSetRowDeleter::deleteRow(const tango::rowid_t& rowid)
{
    m_rowid_array->append(rowid);
    return true;
}

void TableSetRowDeleter::finishDelete()
{
    if (m_rowid_array)
    {
        m_table_row_deleter->startDelete();

        m_rowid_array->goFirst();
        while (!m_rowid_array->isEof())
        {
            doRowDelete(m_rowid_array->getItem());
            m_rowid_array->goNext();
        }

        m_table_row_deleter->finishDelete();
    }
}

void TableSetRowDeleter::cancelDelete()
{
    delete m_rowid_array;
    m_rowid_array = NULL;
}


bool TableSetRowDeleter::doRowDelete(tango::rowid_t rowid)
{
    if (!m_table_row_deleter->deleteRow(rowid))
        return false;

    // delete the index keys associated with this row
    tango::rowpos_t row = rowidGetRowPos(rowid);

    // read the row
    m_set->m_table->getRow(row, m_set->m_update_buf);

    std::vector<IndexEntry>::iterator it;
    IIndexIterator* iter;

    for (it = m_set->m_indexes.begin();
         it != m_set->m_indexes.end(); ++it)
    {
        iter = seekRow(it->index,
                       it->key_expr->getKey(),
                       it->key_length,
                       rowid);

        if (iter)
        {
            it->index->remove(iter);
            iter->unref();
        }
    }

    return true;
}


