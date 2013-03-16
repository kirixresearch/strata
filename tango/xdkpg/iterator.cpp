/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/sqlcommon.h"
#include "../xdcommon/localrowcache.h"
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/md5.h>

const int row_array_size = 1000;

const std::string empty_string = "";
const std::wstring empty_wstring = L"";


KpgIterator::KpgIterator(KpgDatabase* database, KpgSet* set)
{
    m_database = database;
    m_database->ref();

    m_set = set;
    if (m_set)
        m_set->ref();
}

KpgIterator::~KpgIterator()
{
    if (m_set)
        m_set->unref();

    if (m_database)
        m_database->unref();
}

bool KpgIterator::init(const std::wstring& query)
{
    m_structure = m_set->getStructure();

    int i, col_count = m_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = m_structure->getColumnInfoByIdx(i);

        KpgDataAccessInfo* field = new KpgDataAccessInfo;
        field->name = colinfo->getName();
        field->type = colinfo->getType();
        field->width = colinfo->getWidth();
        field->scale = colinfo->getScale();
        field->ordinal = i;

        m_fields.push_back(field);
    }


    refreshStructure();


    return true;
}



tango::ISetPtr KpgIterator::getSet()
{
    return static_cast<tango::ISet*>(m_set);
}

tango::IDatabasePtr KpgIterator::getDatabase()
{
    return static_cast<tango::IDatabase*>(m_database);
}

tango::IIteratorPtr KpgIterator::clone()
{
    return xcm::null;
}


void KpgIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & tango::ifReverseRowCache) != 0) ? true : false;
}
    
    
unsigned int KpgIterator::getIteratorFlags()
{
    return 0;
}

void KpgIterator::skip(int delta)
{
    m_row_pos += delta;

}

void KpgIterator::goFirst()
{
}

void KpgIterator::goLast()
{
}

tango::rowid_t KpgIterator::getRowId()
{
    return m_row_pos;
}

bool KpgIterator::bof()
{
    return false;
}

bool KpgIterator::eof()
{
    return m_eof;
}

bool KpgIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool KpgIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool KpgIterator::setPos(double pct)
{
    return false;
}

void KpgIterator::goRow(const tango::rowid_t& rowid)
{
}

double KpgIterator::getPos()
{
    return (double)(tango::tango_int64_t)m_row_pos;
}

tango::IStructurePtr KpgIterator::getStructure()
{
    return m_set->getStructure();
}

void KpgIterator::refreshStructure()
{
}

bool KpgIterator::modifyStructure(tango::IStructure* struct_config,
                                   tango::IJob* job)
{
    return false;
}



tango::objhandle_t KpgIterator::getHandle(const std::wstring& expr)
{
    std::vector<KpgDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        KpgDataAccessInfo* dai = new KpgDataAccessInfo;
        dai->expr = NULL;
        dai->expr_text = expr;
        dai->type = tango::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<tango::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }
        
        m_exprs.push_back(dai);
        return (tango::objhandle_t)dai;
    }
    
    
    kscript::ExprParser* parser = parse(expr);
    if (!parser)
    {
        return (tango::objhandle_t)0;
    }

    KpgDataAccessInfo* dai = new KpgDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool KpgIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<KpgDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

tango::IColumnInfoPtr KpgIterator::getInfo(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    // try to get the column information from the set structure

    if (m_structure.isNull())
    {
        tango::IStructurePtr s = getStructure();
    }

    if (m_structure.isOk())
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = m_structure->getColumnInfo(dai->name);
        if (colinfo.isOk())
        {
            return colinfo->clone();
        }
    }


    // generate column information from our internal info
    /*
    return pgsqlCreateColInfo(dai->name,
                              dai->type,
                              dai->width,
                              dai->scale,
                              dai->expr_text,
                              -1);
    */
    return xcm::null;
}

int KpgIterator::getType(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return tango::typeInvalid;
    }
    
    return dai->type;
}

int KpgIterator::getRawWidth(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* KpgIterator::getRawPtr(tango::objhandle_t data_handle)
{


    return NULL;
}


const std::string& KpgIterator::getString(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_string;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->str_result = kl::tostring(dai->expr_result.getString());
        return dai->str_result;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return empty_string;
    }

    return dai->str_result;
}

const std::wstring& KpgIterator::getWideString(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->wstr_result = dai->expr_result.getString();
        return dai->wstr_result;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return empty_wstring;
    }

    return dai->wstr_result;
}

tango::datetime_t KpgIterator::getDateTime(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        kscript::ExprDateTime edt = dai->expr_result.getDateTime();

        tango::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        if (dai->type == tango::typeDateTime)
            dt |= edt.time;

        return dt;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }


    return 0;
}

double KpgIterator::getDouble(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0.0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getDouble();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0.0;
    }

    return 0.0;
}

int KpgIterator::getInteger(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getInteger();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    return 0;
}

bool KpgIterator::getBoolean(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getBoolean();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return false;
    }

    return false;
}

bool KpgIterator::isNull(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
        return true;

    if (dai->expr)
        return false;

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return true;
    }

    return true;
}


tango::ISetPtr KpgIterator::getChildSet(tango::IRelationPtr relation)
{
    return xcm::null;

}


tango::IIteratorPtr KpgIterator::getChildIterator(tango::IRelationPtr relation)
{
    return xcm::null;
}



// tango::ICacheRowUpdate::updateCacheRow()

bool KpgIterator::updateCacheRow(tango::rowid_t rowid,
                                 tango::ColumnUpdateInfo* info,
                                 size_t info_size)
{
    return true;
}

