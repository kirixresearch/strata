/*!
 *
 * Copyright (c) 2014, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2014-05-20
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <ctime>
#include "xlsx.h"
#include "../xdcommon/util.h"


static XlsxField EMPTY_XLSX_FIELD = XlsxField();
static XlsxDateTime EMPTY_XLSX_DATE = XlsxDateTime();


// -- XlsxFile class implementation --

XlsxFile::XlsxFile()
{

}

XlsxFile::~XlsxFile()
{
    closeFile();
}

bool XlsxFile::openFile(const std::wstring& filename)
{
    return true;
}

bool XlsxFile::createFile(const std::wstring& filename,
                           const std::vector<XlsxField>& fields)
{
    return true;
}

bool XlsxFile::isOpen()
{
    return m_file ? true : false;
}

void XlsxFile::closeFile()
{

}

const std::wstring& XlsxFile::getFilename()
{
    return m_filename;
}

size_t XlsxFile::getRowCount()
{
    return m_row_count;
}

std::vector<XlsxField>& XlsxFile::getFields()
{
    return m_fields;
}

const XlsxField& XlsxFile::getFieldInfo(const std::string& name)
{
    std::vector<XlsxField>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (strcasecmp(name.c_str(), it->name.c_str()) == 0)
        {
            return (*it);
        }
    }
    
    return EMPTY_XLSX_FIELD;
}

size_t XlsxFile::getFieldIdx(const std::string& name)
{
    std::vector<XlsxField>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (strcasecmp(name.c_str(), it->name.c_str()) == 0)
        {
            return it->ordinal;
        }
    }
    
    return -1;
}

size_t XlsxFile::getFieldCount()
{
    return m_fields.size();
}

const std::string& XlsxFile::getString(size_t col_idx)
{
    return m_str_result;
}

const XlsxDateTime& XlsxFile::getDateTime(size_t col_idx)
{
    m_date_result = EMPTY_XLSX_DATE;
    return m_date_result;
}

double XlsxFile::getDouble(size_t col_idx)
{
    return 0.0;
}

int XlsxFile::getInteger(size_t col_idx)
{
    return 0;
}

bool XlsxFile::getBoolean(size_t col_idx)
{
    return false;
}

bool XlsxFile::isNull(size_t col_idx)
{
    return false;
}

bool XlsxFile::putRowBuffer(const unsigned char* value)
{
    return true;
}

bool XlsxFile::putRaw(size_t col_idx, const unsigned char* value, size_t length)
{
    return true;
}

bool XlsxFile::putString(size_t col_idx, const std::string& value)
{
    return true;
}

bool XlsxFile::putDateTime(size_t col_idx, const XlsxDateTime& value)
{
    return false;
}

bool XlsxFile::putDouble(size_t col_idx, double value)
{
    return false;
}

bool XlsxFile::putInteger(size_t col_idx, int value)
{
    return true;
}

bool XlsxFile::putBoolean(size_t col_idx, bool value)
{
    return true;
}

bool XlsxFile::putNull(size_t col_idx)
{
    return false;
}

bool XlsxFile::writeRow()
{
    return true;
}

bool XlsxFile::startInsert()
{
    return true;
}

bool XlsxFile::finishInsert()
{
    return true;
}

bool XlsxFile::insertRow()
{
    return false;
}

bool XlsxFile::flush()
{
    return true;
}

void XlsxFile::goRow(size_t row)
{

}

