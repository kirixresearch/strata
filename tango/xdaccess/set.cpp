/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-09-03
 *
 */


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/dbfuncs.h"


AccessSet::AccessSet()
{
}

AccessSet::~AccessSet()
{
}

bool AccessSet::init()
{
    MdbTableDef* table;

    std::string ascname = kl::tostring(m_tablename);

    table = findTableInCatalog(m_mdb, (gchar*)ascname.c_str());
    if (!table)
        return false;

    m_row_count = table->num_rows;

    mdb_free_tabledef(table);

    return true;
}

// -- tango::ISet interface implementation --

unsigned int AccessSet::getSetFlags()
{
    return tango::sfFastRowCount;
}

std::wstring AccessSet::getSetId()
{
    return L"";
}


struct FixColumn
{
    tango::objhandle_t handle;
    tango::IColumnInfoPtr colinfo;
    int max_dec;
};

void AccessSet::fixStructure(tango::IStructurePtr& s)
{
    // -- because Access can store arbitrary numeric
    //    scales (decimal digits to the right of the
    //    decimal point), we must find out the correct
    //    scale by performing a query to look at the
    //    data itself --

    tango::IIteratorPtr iter = createIterator(L"", L"", NULL);
    

    std::vector<FixColumn> cols;
    std::vector<FixColumn>::iterator it;
    
    int col_count = s->getColumnCount();
    
    // -- bind the columns to their return values --
    int i;
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        int type = colinfo->getType();
                
        if (type == tango::typeNumeric ||
            type == tango::typeDouble)
        {
            FixColumn col;
            col.handle = iter->getHandle(colinfo->getName());
            col.colinfo = colinfo;
            col.max_dec = colinfo->getScale();
            cols.push_back(col);
        }
    }

    iter->goFirst();
    
    i = 0;
    while (i < 1000)
    {
        if (iter->eof())
            break;
        
        for (it = cols.begin(); it != cols.end(); ++it)
        {
            if (iter->isNull(it->handle))
                continue;
                
            const std::string& value = iter->getString(it->handle);
            const char* ptr = value.c_str();
            const char* p = strchr(ptr, '.');
            
            if (p)
            {
                int offset = p - ptr;
                int len = value.length();
                int decimal_places = len-(offset+1);
                if (decimal_places > it->max_dec)
                    it->max_dec = decimal_places;
            }
        }
        
        iter->skip(1);
        ++i;
    }

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        it->colinfo->setScale(it->max_dec);
    }
}


tango::IStructurePtr AccessSet::getStructure()
{
    if (m_structure.isOk())
        return m_structure->clone();
    
    MdbCatalogEntry *entry;
    
    bool found = false;
    
    unsigned int i;
    
    for (i = 0; i < m_mdb->num_catalog; i++)
    {
        entry = (MdbCatalogEntry*)g_ptr_array_index (m_mdb->catalog, i);
        
        if (entry->object_type == MDB_TABLE)
        {
            std::wstring object_name = kl::towstring(entry->object_name);
            
            if (0 == wcscasecmp(m_tablename.c_str(), object_name.c_str()))
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
        return xcm::null;

    // -- create new tango::IStructure --
    Structure* s = new Structure;
    
            
    tango::IColumnInfoPtr colinfo;
    
    MdbTableDef *table;
    MdbHandle *mdb = entry->mdb;
    MdbColumn *col;


    table = mdb_read_table(entry);

    /* get the columns */
    mdb_read_columns(table);

    /* loop over the columns, dumping the names and types */
    for (i = 0; i < table->num_cols; i++)
    {
        col = (MdbColumn*)g_ptr_array_index(table->columns, i);
        
        colinfo = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        colinfo->setName(kl::towstring(col->name));
        //colinfo->setType(tango::typeWideCharacter);
        //colinfo->setWidth(100);
        colinfo->setType(access2tangoType(col->col_type));
        colinfo->setWidth(col->col_size);
        colinfo->setScale(col->col_scale);
        
        s->addColumn(colinfo);
    }

    
    m_structure = static_cast<tango::IStructure*>(s);
    
    fixStructure(m_structure);
    
    return m_structure->clone();
}

bool AccessSet::modifyStructure(tango::IStructure* struct_config,
                              tango::IJob* job)
{
    XCM_AUTO_LOCK(m_object_mutex);
    return true;
}

tango::IRowInserterPtr AccessSet::getRowInserter()
{
    AccessRowInserter* inserter = new AccessRowInserter(this);
    return static_cast<tango::IRowInserter*>(inserter);
}

tango::IRowDeleterPtr AccessSet::getRowDeleter()
{
    return xcm::null;
}

int AccessSet::insert(tango::IIteratorPtr source_iter,
                      const std::wstring& where_condition,
                      int max_rows,
                      tango::IJob* job)
{
    return xdcmnInsert(source_iter, this, where_condition, max_rows, job);
}

int AccessSet::update(tango::ISetPtr constraint,
                      const std::wstring& params,
                      tango::IJob* job)
{
    return 0;
}

int AccessSet::remove(tango::ISetPtr constraint, tango::IJob* job)
{
    return 0;
}

tango::IIndexInfoEnumPtr AccessSet::getIndexEnum()
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* indexes;
    indexes = new xcm::IVectorImpl<tango::IIndexInfoPtr>;
    return indexes;
}

tango::IIndexInfoPtr AccessSet::createIndex(const std::wstring& tag,
                                            const std::wstring& expr,
                                            tango::IJob* job)
{
    return xcm::null;
}

tango::IIndexInfoPtr AccessSet::lookupIndex(const std::wstring& expr,
                                            bool exact_column_order)
{
    return xcm::null;
}

bool AccessSet::deleteIndex(const std::wstring& name)
{
    return false;
}


bool AccessSet::renameIndex(const std::wstring& name,
                            const std::wstring& new_name)
{
    return false;
}


tango::IRelationEnumPtr AccessSet::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IRelationPtr AccessSet::createRelation(const std::wstring& tag,
                                              const std::wstring& right_set_path,
                                              const std::wstring& left_expr,
                                              const std::wstring& right_expr)
{
    return xcm::null;
}

tango::IRelationPtr AccessSet::getRelation(const std::wstring& rel_tag)
{
    return xcm::null;
}

int AccessSet::getRelationCount()
{
    return xcm::null;
}

bool AccessSet::deleteRelation(const std::wstring& tag)
{
    return false;
}

bool AccessSet::deleteAllRelations()
{
    return false;
}

tango::IIteratorPtr AccessSet::createIterator(const std::wstring& columns,
                                              const std::wstring& expr,
                                              tango::IJob* job)
{
    // -- create an iterator based on our select statement --
    AccessIterator* iter = new AccessIterator;
    iter->m_set = static_cast<tango::ISet*>(this);
    iter->m_mdb = m_mdb;
    iter->m_path = m_path;
    iter->m_database = m_database;
    
    std::wstring query;

    query = m_tablename;

    // -- initialize Access connection for this set --
    if (!iter->init(query))
    {
        return xcm::null;
    }

    return static_cast<tango::IIterator*>(iter);
}

tango::IIteratorPtr AccessSet::createBookmark(tango::rowid_t rowid)
{
    return xcm::null;
}

bool AccessSet::rowInSet(tango::rowid_t rowid)
{
    return false;
}

tango::rowpos_t AccessSet::getRowCount()
{
    return m_row_count;
}




// -- AccessRowInserter class implementation --

AccessRowInserter::AccessRowInserter(AccessSet* set)
{
    m_inserting = false;
}

AccessRowInserter::~AccessRowInserter()
{
}

tango::objhandle_t AccessRowInserter::getHandle(const std::wstring& column_name)
{
    return 0;
}

tango::IColumnInfoPtr AccessRowInserter::getInfo(tango::objhandle_t column_handle)
{
    return xcm::null;
}

bool AccessRowInserter::putRawPtr(tango::objhandle_t column_handle,
                                  const unsigned char* value,
                                  int length)
{
    return false;
}

bool AccessRowInserter::putString(tango::objhandle_t column_handle,
                                  const std::string& value)
{
	return true;
}

bool AccessRowInserter::putWideString(tango::objhandle_t column_handle,
                                      const std::wstring& value)
{
	return true;
}

bool AccessRowInserter::putDouble(tango::objhandle_t column_handle,
                                  double value)
{
	return true;
}

bool AccessRowInserter::putInteger(tango::objhandle_t column_handle,
                                   int value)
{
	return true;
}

bool AccessRowInserter::putBoolean(tango::objhandle_t column_handle,
                                   bool value)
{
	return true;
}

bool AccessRowInserter::putDateTime(tango::objhandle_t column_handle,
                                    tango::datetime_t datetime)
{
	return true;
}

bool AccessRowInserter::putNull(tango::objhandle_t column_handle)
{
    return false;
}

bool AccessRowInserter::startInsert(const std::wstring& col_list)
{
    return true;
}

bool AccessRowInserter::insertRow()
{
    return true;
}

void AccessRowInserter::finishInsert()
{
    m_inserting = false;
}

bool AccessRowInserter::flush()
{
    return false;
}






