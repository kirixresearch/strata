/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-04-16
 *
 */


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"


const std::string empty_string = "";
const std::wstring empty_wstring = L"";


AccessIterator::AccessIterator()
{
    m_eof = false;
    m_table = NULL;
}

AccessIterator::~AccessIterator()
{
    std::vector<AccessDataAccessInfo*>::iterator it;
    for (it = m_columns.begin();
         it != m_columns.end();
         ++it)
    {
        delete[] (*it)->buf;
        delete (*it);
    }

    if (m_table)
    {
        mdb_free_tabledef(m_table);
    }
}


bool AccessIterator::init(const std::wstring& query)
{
    m_tablename = query;
    m_structure = m_set->getStructure()->clone();
    
    
    std::string asctable = kl::tostring(m_tablename);

    m_table = findTableInCatalog(m_mdb, (gchar*)asctable.c_str());
    if (!m_table)
        return false;

    mdb_read_columns(m_table);
    mdb_rewind_table(m_table);

    // -- bind columns --

    int i;
    int col_count = m_structure->getColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = m_structure->getColumnInfoByIdx(i);

        AccessDataAccessInfo* dai;
        dai = new AccessDataAccessInfo;
        dai->name = colinfo->getName();
        dai->buf = new char[4096];
        dai->colinfo = colinfo->clone();

        std::string ascname = kl::tostring(dai->name);

        mdb_bind_column_by_name(m_table,
                                (gchar*)ascname.c_str(),
                                (gchar*)dai->buf);

        m_columns.push_back(dai);
    }


    return true;
}

// -- IIterator interface implementation --

tango::ISetPtr AccessIterator::getSet()
{
    return m_set;
}

tango::IDatabasePtr AccessIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr AccessIterator::clone()
{
    return xcm::null;
}

unsigned int AccessIterator::getIteratorFlags()
{
    return tango::ifForwardOnly;
}

void AccessIterator::clearFieldData()
{
}

void AccessIterator::skip(int delta)
{
    while (delta > 0)
    {
        if (!mdb_fetch_row(m_table))
        {
            m_eof = true;
            break;
        }

        --delta;
    }
}

void AccessIterator::goFirst()
{
    mdb_rewind_table(m_table);
    m_eof = (mdb_fetch_row(m_table) ? false : true);
}

void AccessIterator::goLast()
{

}

tango::rowid_t AccessIterator::getRowId()
{
    return 0;
}

bool AccessIterator::bof()
{
    return false;
}

bool AccessIterator::eof()
{
    if (m_eof)
    {
        return true;
    }

    return false;
}

bool AccessIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool AccessIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool AccessIterator::setPos(double pct)
{
    return false;
}

void AccessIterator::goRow(const tango::rowid_t& rowpos)
{
}

double AccessIterator::getPos()
{
    return 0.0;
}

tango::IStructurePtr AccessIterator::getStructure()
{
    return m_set->getStructure();
}

void AccessIterator::refreshStructure()
{
}

bool AccessIterator::modifyStructure(tango::IStructure* struct_config,
                                   tango::IJob* job)
{
    return false;
}


tango::ISetPtr AccessIterator::getChildSet(const std::wstring& relation_tag)
{
    return xcm::null;
}

tango::objhandle_t AccessIterator::getHandle(const std::wstring& expr)
{
    std::vector<AccessDataAccessInfo*>::iterator it;
    for (it = m_columns.begin();
         it != m_columns.end();
         ++it)
    {
        if (0 == wcscasecmp((*it)->name.c_str(), expr.c_str()))
        {
            return (tango::objhandle_t)(*it);
        }
    }

    return 0;
}

bool AccessIterator::releaseHandle(tango::objhandle_t data_handle)
{
    if (!data_handle)
        return false;
        
    return true;
}

tango::IColumnInfoPtr AccessIterator::getInfo(tango::objhandle_t data_handle)
{
    AccessDataAccessInfo* dai = (AccessDataAccessInfo*)data_handle;
    if (!dai)
        return xcm::null;

    return dai->colinfo->clone();
}

int AccessIterator::getType(tango::objhandle_t data_handle)
{
    tango::IColumnInfoPtr colinfo = getInfo(data_handle);
    if (colinfo.isNull())
        return tango::typeInvalid;

    return colinfo->getType();
}

int AccessIterator::getRawWidth(tango::objhandle_t data_handle)
{
	return 0;
}

const unsigned char* AccessIterator::getRawPtr(tango::objhandle_t data_handle)
{
	return NULL;
}

const std::string& AccessIterator::getString(tango::objhandle_t data_handle)
{
    AccessDataAccessInfo* dai = (AccessDataAccessInfo*)data_handle;
    if (!dai)
        return empty_string;

    if (m_eof)
        return empty_string;

    dai->result_str = dai->buf;
    return dai->result_str;
}

const std::wstring& AccessIterator::getWideString(tango::objhandle_t data_handle)
{
    AccessDataAccessInfo* dai = (AccessDataAccessInfo*)data_handle;
    if (!dai)
        return empty_wstring;

    if (m_eof)
        return empty_wstring;

    dai->result_wstr = kl::towstring(dai->buf);
    return dai->result_wstr;
}

tango::datetime_t AccessIterator::getDateTime(tango::objhandle_t data_handle)
{
    const std::string& str = getString(data_handle);
    if (m_eof)
        return 0;

    char s[64];
    char* p;
    strncpy(s, str.c_str(), 63);
    s[63] = 0;
    
    p = s;
    while (*p)
    {
        if (*p == '.')
            *p = 0;
        p++;
    }

    tango::datetime_t dt, tm;
    
    dt = dateToJulian(atoi(s),
                      atoi(s+5),
                      atoi(s+8));

    tm = ((atoi(s+11)*3600000) +
          (atoi(s+14)*60000) +
          (atoi(s+17)*1000));

    dt <<= 32;
    dt |= tm;

    return dt;
}

double AccessIterator::getDouble(tango::objhandle_t data_handle)
{
    const std::string& str = getString(data_handle);
    if (m_eof)
        return 0.0;
    return atof(str.c_str());
}

int AccessIterator::getInteger(tango::objhandle_t data_handle)
{
    if (m_eof)
        return 0;
    const std::string& str = getString(data_handle);
    return atoi(str.c_str());
}

bool AccessIterator::getBoolean(tango::objhandle_t data_handle)
{
    if (m_eof)
        return false;
    const std::string& str = getString(data_handle);
    if (str == "0")
        return false;
    return true;
}

bool AccessIterator::isNull(tango::objhandle_t data_handle)
{
    return false;
}



