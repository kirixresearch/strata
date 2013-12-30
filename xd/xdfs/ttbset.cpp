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


// TtbSet class implementation

TtbSet::TtbSet(FsDatabase* db)
{
    m_database = db;
    m_database->ref();
}

TtbSet::~TtbSet()
{
    if (m_file.isOpen())
        m_file.close();

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


std::wstring TtbSet::getSetId()
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
    return true;
}


// TtbRowInserter class implementation

TtbRowInserter::TtbRowInserter(TtbSet* set)
{
    m_set = set;
    m_set->ref();
    
    m_file = &(m_set->m_file);
    m_inserting = false;
}

TtbRowInserter::~TtbRowInserter()
{
    m_set->unref();
}

xd::objhandle_t TtbRowInserter::getHandle(const std::wstring& column_name)
{
    return (xd::objhandle_t)0;
}

xd::IColumnInfoPtr TtbRowInserter::getInfo(xd::objhandle_t column_handle)
{
    return xcm::null;
}

bool TtbRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                 const unsigned char* value,
                                 int length)
{
    return false;
}

bool TtbRowInserter::putString(xd::objhandle_t column_handle,
                                 const std::string& value)
{
    return false;
}

bool TtbRowInserter::putWideString(xd::objhandle_t column_handle,
                                     const std::wstring& value)
{
    return false;
}

bool TtbRowInserter::putDouble(xd::objhandle_t column_handle,
                                 double value)
{
    return false;
}

bool TtbRowInserter::putInteger(xd::objhandle_t column_handle,
                                  int value)
{
    return false;
}

bool TtbRowInserter::putBoolean(xd::objhandle_t column_handle,
                                  bool value)
{
    return false;
}

bool TtbRowInserter::putDateTime(xd::objhandle_t column_handle,
                                   xd::datetime_t value)
{
    return false;
}

bool TtbRowInserter::putRowBuffer(const unsigned char* value)
{
    return false;
}

bool TtbRowInserter::putNull(xd::objhandle_t column_handle)
{
    return false;
}

bool TtbRowInserter::startInsert(const std::wstring& col_list)
{
    return false;
}

bool TtbRowInserter::insertRow()
{
    return false;
}

void TtbRowInserter::finishInsert()
{
}

bool TtbRowInserter::flush()
{
    return false;
}

