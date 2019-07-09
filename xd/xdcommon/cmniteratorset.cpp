/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-11
 *
 */


#include <xd/xd.h>
#include "util.h"
#include "exindex.h"
#include "keylayout.h"
#include "idxutil.h"
#include "cmniteratorset.h"
#include "cmndynamicset.h"
#include "jobinfo.h"
#include <kl/klib.h>
#include <ctime>





CommonIteratorSet::CommonIteratorSet()
{
}

CommonIteratorSet::~CommonIteratorSet()
{
}

void CommonIteratorSet::setObjectPath(const std::wstring& new_path)
{
    if (m_path.empty())
        return;

    m_path = new_path;
}

std::wstring CommonIteratorSet::getObjectPath()
{
    if (m_path.empty())
    {
        // return an object pointer -- see openSet or lookupSetId in xdnative and xdfs
        // to see how this curiosity plays out
        wchar_t buf[255];
        swprintf(buf, 255, L"/.temp/.ptr/%p", static_cast<IXdsqlTable*>(this));
        return buf;
    }

    return m_path;
}


bool CommonIteratorSet::create(xd::IDatabasePtr database)
{
    m_database = database;

    xd::IAttributesPtr attr = database->getAttributes();
    std::wstring temp_directory = attr->getStringAttribute(xd::dbattrTempDirectory);
    if (!temp_directory.empty())
        m_temp_path = temp_directory;

    return true;
}


xd::Structure CommonIteratorSet::getStructure()
{
    return m_iter->getStructure();
}


/*
bool CommonIteratorSet::create(xd::IDatabasePtr database,
                              const std::wstring& base_path)
{
    IXdsqlDatabasePtr xdb = database;
    if (xdb.isNull())
        return false;

    IXdsqlTablePtr base_table = xdb->openTable(base_path);
    if (base_table.isNull())
        return false;

    return create(database, base_table);
}

bool CommonIteratorSet::create(xd::IDatabasePtr database,
                              IXdsqlTablePtr base_table)
{
    xd::IAttributesPtr attr = database->getAttributes();
    std::wstring temp_directory = attr->getStringAttribute(xd::dbattrTempDirectory);
    if (!temp_directory.empty())
        m_temp_path = temp_directory;
    
 
    m_base_table = base_table;

    return true;
}
*/



xd::IIteratorPtr CommonIteratorSet::createIterator(const std::wstring& columns,
                                                   const std::wstring& expr,
                                                   xd::IJob* job)
{
    if (expr.empty())
    {
        xd::IIteratorPtr iter = m_iter->clone();
        iter->goFirst();
        return iter;
    }
    else
    {
        xd::IIteratorPtr data_iter = m_iter->clone();
        data_iter->goFirst();


        std::wstring index_filename = getUniqueString();
        index_filename += L".idx";

        std::wstring full_index_filename;
        full_index_filename = makePathName(m_temp_path, L"", index_filename);

        IIndex* idx = createExternalIndex(m_database,
                                          getObjectPath(),
                                          full_index_filename,
                                          m_temp_path,
                                          expr,
                                          true,
                                          false, // delete_on_close = false because we handle it ourselves in this class
                                          job);

        IIndexIterator* idx_iter = idx->createIterator();
        if (!idx_iter)
            return xcm::null;
        idx_iter->goFirst();

        CommonIndexIterator* iter = new CommonIndexIterator();
        iter->init(data_iter, idx_iter, L"", true);
        iter->setTable(getObjectPath());

        idx_iter->unref();
        iter->goFirst();

        return static_cast<xd::IIterator*>(iter);
    }
}

