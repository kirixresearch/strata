/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2013-03-01
 *
 */


#include "tango.h"
#include "database.h"
#include "set.h"
#include "inserter.h"
#include "iterator.h"
#include "pkgfile.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/extfileinfo.h"
#include "../xdcommon/structure.h"
#include <kl/md5.h>


KpgSet::KpgSet(KpgDatabase* database)
{
    m_database = database;
    m_database->ref();

    m_tablename = L"";
}

KpgSet::~KpgSet()
{
    m_database->unref();
}

bool KpgSet::init()
{
    return true;
}



void KpgSet::setObjectPath(const std::wstring& path)
{
    m_path = path;
}

std::wstring KpgSet::getObjectPath()
{
    if (m_path.empty())
        return m_tablename;
        
    return m_path;
}


unsigned int KpgSet::getSetFlags()
{
 //       return tango::sfFastRowCount;
 
    return 0;
}

std::wstring KpgSet::getSetId()
{
    if (m_set_id.length() == 0)
    {
        std::wstring id = L"xdkpg:";
        
        std::wstring server = m_database->getPath();
        kl::makeLower(server);

        id += server;
        id += L":";
        
        std::wstring table_name = m_tablename;
        kl::makeLower(table_name);
            
        id += table_name;
        
        m_set_id = kl::md5str(id);
    }
    
    return m_set_id;
}





tango::IStructurePtr KpgSet::getStructure()
{
    if (m_structure.isOk())
        return m_structure;

    int node_idx = m_info.getChildIdx(L"structure");
    if (node_idx == -1)
        return xcm::null;

    kl::xmlnode& structure_node = m_info.getChild(node_idx);

    m_structure = xdkpgXmlToStructure(structure_node);

    return m_structure;
}

bool KpgSet::modifyStructure(tango::IStructure* struct_config,
                               tango::IJob* job)
{
    return true;
}

tango::IRowInserterPtr KpgSet::getRowInserter()
{
    KpgRowInserter* inserter = new KpgRowInserter(m_database, m_tablename);
    return static_cast<tango::IRowInserter*>(inserter);
}

tango::IRowDeleterPtr KpgSet::getRowDeleter()
{
    return xcm::null;
}

int KpgSet::insert(tango::IIteratorPtr source_iter,
                    const std::wstring& where_condition,
                    int max_rows,
                    tango::IJob* job)
{
    return xdcmnInsert(source_iter, this, where_condition, max_rows, job);
}

tango::IIteratorPtr KpgSet::createIterator(const std::wstring& columns,
                                           const std::wstring& expr,
                                           tango::IJob* job)
{    
    // create an iterator based on our select statement
    KpgIterator* iter = new KpgIterator(m_database, this);
    iter->m_reader = m_database->m_kpg->readStream(m_tablename);
    if (!iter->m_reader->reopen())
    {
        delete iter;
        return xcm::null;
    }

    if (!iter->init())
    {
        delete iter;
        return xcm::null;
    }

    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t KpgSet::getRowCount()
{
    return -1;
}

