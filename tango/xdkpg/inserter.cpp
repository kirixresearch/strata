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
#include "inserter.h"
#include <kl/file.h>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include "pkgfile.h"

KpgRowInserter::KpgRowInserter(KpgDatabase* db, const std::wstring& table)
{
    m_database = db;
    m_database->ref();
    
    tango::IAttributesPtr attr = m_database->getAttributes();
    m_quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    m_quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    m_buf_rows = 0;
    m_insert_rows = 0;
    m_table = table;
    kl::replaceStr(m_table, L"\"", L"");

    m_utf8data = NULL;
    m_utf8data_len = 0;
}

KpgRowInserter::~KpgRowInserter()
{
    if (m_utf8data)
        free(m_utf8data);

    m_database->unref();
}

tango::objhandle_t KpgRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<KpgInsertFieldData>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp(it->m_name.c_str(), column_name.c_str()))
            return &(*it);
    }

    return 0;
}

tango::IColumnInfoPtr KpgRowInserter::getInfo(tango::objhandle_t column_handle)
{
    return xcm::null;
}

bool KpgRowInserter::putRawPtr(tango::objhandle_t column_handle,
                               const unsigned char* value,
                               int length)
{
    return false;
}

bool KpgRowInserter::putString(tango::objhandle_t column_handle,
                                 const std::string& value)
{
    KpgInsertFieldData* f = (KpgInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_str_val = kl::towstring(value);

    return true;
}

bool KpgRowInserter::putWideString(tango::objhandle_t column_handle,
                                   const std::wstring& value)
{
    KpgInsertFieldData* f = (KpgInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_str_val = value;

    return true;
}

bool KpgRowInserter::putDouble(tango::objhandle_t column_handle,
                               double value)
{
    KpgInsertFieldData* f = (KpgInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_dbl_val = value;

    return true;
}

bool KpgRowInserter::putInteger(tango::objhandle_t column_handle,
                                int value)
{
    KpgInsertFieldData* f = (KpgInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_int_val = value;

    return true;
}

bool KpgRowInserter::putBoolean(tango::objhandle_t column_handle,
                                bool value)
{
    KpgInsertFieldData* f = (KpgInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_bool_val = value;

    return true;
}

bool KpgRowInserter::putDateTime(tango::objhandle_t column_handle,
                                 tango::datetime_t value)
{
    KpgInsertFieldData* f = (KpgInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_datetime_val = value;

    return true;
}

bool KpgRowInserter::putNull(tango::objhandle_t column_handle)
{
    KpgInsertFieldData* f = (KpgInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    return false;
}

bool KpgRowInserter::startInsert(const std::wstring& col_list)
{
    m_writer = m_database->m_kpg->createStream(m_table);



    return true;
}

bool KpgRowInserter::insertRow()
{

    return true;
}

void KpgRowInserter::finishInsert()
{
    if (!m_writer)
        return;

    m_writer->finishWrite();
    delete m_writer;
    m_writer = NULL;
}

bool KpgRowInserter::flush()
{
    return true;
}
