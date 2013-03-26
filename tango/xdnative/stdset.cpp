/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-09-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "stdset.h"
#include "util.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include <kl/portable.h>


IterWrapperSet::IterWrapperSet(tango::IDatabase* database) : BaseSet(database)
{
    m_iter = NULL;
}

IterWrapperSet::~IterWrapperSet()
{
    if (m_iter)
    {
        m_iter->unref();
        m_iter = NULL;
    }

    if (m_base_set)
    {
        ISetInternalPtr set_internal = m_base_set;
        set_internal->removeEventHandler(this);
    }
}


std::wstring IterWrapperSet::getObjectPath()
{
    // return an object pointer -- see openSet or lookupSetId in xdnative and xdfs
    // to see how this curiosity plays out
    wchar_t buf[255];
    swprintf(buf, 255, L"/.temp/.ptr/%p", static_cast<tango::ISet*>(this));
    return buf;
}

bool IterWrapperSet::create(tango::ISetPtr base_set,
                            tango::IIteratorPtr iter)
{
    XCM_AUTO_LOCK(m_iterwrapper_mutex);

    tango::IIteratorPtr it = iter->clone();
    m_iter = it.p;
    it.p = NULL;

    m_base_set = base_set;

    ISetInternalPtr set_internal = m_base_set;
    set_internal->addEventHandler(this);

    return true;
}


tango::IRowInserterPtr IterWrapperSet::getRowInserter()
{
    return xcm::null;
}

int IterWrapperSet::insert(tango::IIteratorPtr source_iter,
                           const std::wstring& where_condition,
                           int max_rows,
                           tango::IJob* job)
{
    return 0;
}

tango::IIteratorPtr IterWrapperSet::createIterator(const std::wstring& columns,
                                                   const std::wstring& expr,
                                                   tango::IJob* job)
{
    XCM_AUTO_LOCK(m_iterwrapper_mutex);

    if (expr.empty())
    {       
        m_iter->goFirst();

        tango::IIteratorPtr result = m_iter->clone();
        IIteratorSetAccessPtr iter_int = result;
        if (iter_int.isOk())
        {
            iter_int->setSet(static_cast<tango::ISet*>(this));
        }

        return result;
    }

    xcm::ptr<IDatabaseInternal> dbinternal = m_database;
    std::wstring index_filename = dbinternal->getTempFilename();

    IIndex* idx = createExternalIndex(static_cast<tango::ISet*>(this),
                                      index_filename,
                                      dbinternal->getTempPath(),
                                      expr,
                                      true,
                                      job);
    if (!idx)
    {
        return xcm::null;
    }

    tango::IIteratorPtr iter = createIteratorFromIndex(static_cast<tango::ISet*>(this),
                                                       idx, columns, expr);
    idx->unref();

    return iter;
}

tango::rowpos_t IterWrapperSet::getRowCount()
{
    XCM_AUTO_LOCK(m_iterwrapper_mutex);

    int count = 0;

    m_iter->goFirst();
    while (m_iter->eof())
    {
        count++;
        m_iter->skip(1);
    }

    return count;
}

tango::IStructurePtr IterWrapperSet::getStructure()
{
    return m_base_set->getStructure();
}

bool IterWrapperSet::modifyStructure(tango::IStructure* struct_config,
                                     tango::IJob* job)
{
    return m_base_set->modifyStructure(struct_config, job);
}

bool IterWrapperSet::updateRow(tango::rowid_t rowid,
                               tango::ColumnUpdateInfo* info,
                               size_t info_size)
{
    tango::ISetRowUpdatePtr set_update = m_base_set;
    if (set_update.isNull())
        return false;

    return set_update->updateRow(rowid, info, info_size);
}


void IterWrapperSet::onSetDomainUpdated()
{
}

void IterWrapperSet::onSetStructureUpdated()
{
    // base set structure was updated, let our iterators know too
    fire_onSetStructureUpdated();
}

void IterWrapperSet::onSetRelationshipsUpdated()
{
}

void IterWrapperSet::onSetRowUpdated(tango::rowid_t rowid)
{
}

void IterWrapperSet::onSetRowDeleted(tango::rowid_t rowid)
{
}









EofIterator::EofIterator()
{
}

EofIterator::~EofIterator()
{
}

tango::IIteratorPtr EofIterator::clone()
{
    return xcm::null;
}

void EofIterator::skip(int delta)
{
}

void EofIterator::goFirst()
{
}

void EofIterator::goLast()
{
}

tango::rowid_t EofIterator::getRowId()
{
    return 0;
}

bool EofIterator::bof()
{
    return false;
}

bool EofIterator::eof()
{
    return true;
}

bool EofIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool EofIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool EofIterator::setPos(double pct)
{
    return false;
}

double EofIterator::getPos()
{
    return 0.0;
}




EofSet::EofSet(tango::IDatabase* database) : BaseSet(database)
{
}

EofSet::~EofSet()
{
    if (m_base_set)
    {
        ISetInternalPtr set_internal = m_base_set;
        set_internal->removeEventHandler(this);
    }
}

bool EofSet::create(tango::ISetPtr base_set)
{
    m_base_set = base_set;

    if (m_base_set)
    {
        ISetInternalPtr set_internal = m_base_set;
        set_internal->addEventHandler(this);
    }

    return true;
}


tango::IRowInserterPtr EofSet::getRowInserter()
{
    return xcm::null;
}



int EofSet::insert(tango::IIteratorPtr source_iter,
                   const std::wstring& where_condition,
                   int max_rows,
                   tango::IJob* job)
{
    return 0;
}

tango::IIteratorPtr EofSet::createIterator(const std::wstring& columns,
                                           const std::wstring& expr,
                                           tango::IJob* job)
{
    EofIterator* eof_iter = new EofIterator;
    eof_iter->ref();
    if (!eof_iter->init(m_database, static_cast<tango::ISet*>(this), L""))
    {
        eof_iter->unref();
        return xcm::null;
    }

    return tango::IIteratorPtr(static_cast<tango::IIterator*>(eof_iter), false);
}

tango::rowpos_t EofSet::getRowCount()
{
    return 0;
}

tango::IStructurePtr EofSet::getStructure()
{
    return m_base_set->getStructure();
}

bool EofSet::modifyStructure(tango::IStructure* struct_config, tango::IJob* job)
{
    return m_base_set->modifyStructure(struct_config, job);
}

void EofSet::onSetDomainUpdated()
{
}

void EofSet::onSetStructureUpdated()
{
    fire_onSetStructureUpdated();
}

void EofSet::onSetRelationshipsUpdated()
{
}

void EofSet::onSetRowUpdated(tango::rowid_t rowid)
{
}

void EofSet::onSetRowDeleted(tango::rowid_t rowid)
{
}







