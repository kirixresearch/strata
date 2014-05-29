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
#include <kl/utf8.h>
#include "xlsx.h"
#include "../xdcommon/util.h"
#include "zip.h"
#define XML_STATIC
#include "expat.h"



static XlsxField EMPTY_XLSX_FIELD = XlsxField();
static XlsxDateTime EMPTY_XLSX_DATE = XlsxDateTime();

#define READ_BUFFER_SIZE 16384


// -- XlsxFile class implementation --

XlsxFile::XlsxFile()
{
    m_zip = NULL;
}

XlsxFile::~XlsxFile()
{
    closeFile();
}

bool XlsxFile::openFile(const std::wstring& filename)
{
    //if (!m_file.openFile(filename))
    //    return false;

    m_zip = zip_open(kl::toUtf8(filename), 0, NULL);
    if (!m_zip)
        return false;

    if (!readSharedStrings())
    {
        zip_close(m_zip);
        return false;
    }

    if (!readSheet())
    {
        zip_close(m_zip);
        return false;
    }

    return true;
}





struct SharedStringParseData
{
    SharedStringParseData() { idx = 0; }

    int idx;
    std::wstring chardata;
    std::map<int, std::wstring>* map;
};

static void sharedStringStart(void* user_data, const char* el, const char** attr)
{
}

static void sharedStringEnd(void* user_data, const char* el)
{
    SharedStringParseData* data = (SharedStringParseData*)user_data;

    if (el[0] == 't' && el[1] == 0)
    {
        (*data->map)[data->idx] = data->chardata;
        data->idx++;
    }
}

static void sharedStringCharData(void* user_data, const XML_Char* s, int len)
{
    SharedStringParseData* data = (SharedStringParseData*)user_data;
    std::wstring str;

    if (len >= 0)
        data->chardata = kl::utf8_utf8towstr(s, (size_t)len);
}



bool XlsxFile::readSharedStrings()
{
    // read in shared strings file

    struct zip_file* sf = zip_fopen(m_zip, "xl/sharedStrings.xml", 0);
    if (!sf)
        return false;

    SharedStringParseData data;
    data.map = &m_shared_strings;

    XML_Parser parser = XML_ParserCreate(NULL);

    XML_SetElementHandler(parser, sharedStringStart, sharedStringEnd);
    XML_SetCharacterDataHandler(parser, sharedStringCharData);
    XML_SetUserData(parser, &data);

    bool success = false;
    int bytes_read;
    void* buf;
    while (1)
    {
        buf = XML_GetBuffer(parser, READ_BUFFER_SIZE);
        if (!buf)
            break;

        bytes_read = (int)zip_fread(sf, buf, READ_BUFFER_SIZE);
        if (bytes_read < 0)
            break;

        if (!XML_ParseBuffer(parser, bytes_read, (bytes_read != READ_BUFFER_SIZE)))
            break;

        if (bytes_read != READ_BUFFER_SIZE)
        {
            success = true;
            break;
        }
    }

    XML_ParserFree(parser);
    zip_fclose(sf);

    return success;
}












struct SheetParseData
{
    SheetParseData() { idx = 0; }

    int idx;
    std::wstring chardata;
    std::map<int, std::wstring>* map;
};

static void sheetStart(void* user_data, const char* el, const char** attr)
{
}

static void sheetEnd(void* user_data, const char* el)
{
    SheetParseData* data = (SheetParseData*)user_data;

    if (el[0] == 't' && el[1] == 0)
    {
        (*data->map)[data->idx] = data->chardata;
        data->idx++;
    }
}

static void sheetCharData(void* user_data, const XML_Char* s, int len)
{
    SheetParseData* data = (SheetParseData*)user_data;
    std::wstring str;

    if (len >= 0)
        data->chardata = kl::utf8_utf8towstr(s, (size_t)len);
}



bool XlsxFile::readSheet()
{
    // read in sheet file

    struct zip_file* sf = zip_fopen(m_zip, "xl/worksheets/sheet1.xml", 0);
    if (!sf)
        return false;
    
    SheetParseData data;
    data.map = &m_shared_strings;

    XML_Parser parser = XML_ParserCreate(NULL);

    XML_SetElementHandler(parser, sheetStart, sheetEnd);
    XML_SetCharacterDataHandler(parser, sheetCharData);
    XML_SetUserData(parser, &data);

    bool success = false;
    int bytes_read;
    void* buf;
    while (1)
    {
        buf = XML_GetBuffer(parser, READ_BUFFER_SIZE);
        if (!buf)
            break;

        bytes_read = (int)zip_fread(sf, buf, READ_BUFFER_SIZE);
        if (bytes_read < 0)
            break;

        if (!XML_ParseBuffer(parser, bytes_read, (bytes_read != READ_BUFFER_SIZE)))
            break;

        if (bytes_read != READ_BUFFER_SIZE)
        {
            success = true;
            break;
        }
    }

    XML_ParserFree(parser);
    zip_fclose(sf);

    return success;
}








bool XlsxFile::createFile(const std::wstring& filename,
                          const std::vector<XlsxField>& fields)
{
    return true;
}

bool XlsxFile::isOpen()
{
    return (m_zip ? true : false);
}

void XlsxFile::closeFile()
{
    zip_close(m_zip);
    m_zip = NULL;
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

