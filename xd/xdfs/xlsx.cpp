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
#include <kl/json.h>
#include "xlsx.h"
#include "../xdcommon/util.h"
#include "zip.h"
#include "sqlite3.h"
#define XML_STATIC
#include "expat.h"





static int getColumnIdxFromCoord(const char* col)
{
    const char* p = col;
    char c;

    while (*p)
    {
        if (!*p || isdigit(*p))
            break;
        c = toupper(*p);
        if (c < 'A' || c > 'Z')
            return -1;    
        ++p;
    }

    if (p == col)
        return -1;

    int res = 0;
    int multiplier = 1;
    --p;
    while (p >= col)
    {
        c = toupper(*p);
        res = ((int)(c - 'A')) * multiplier;

        --p;
        multiplier *= 26;
    }

    return res;
}











class XlsxStore
{
public:

    XlsxStore()
    {
        db = NULL;
        col_count = 0;
        row_count = 0;
    }

    ~XlsxStore()
    {
        if (db)
            sqlite3_close(db);
    }

    bool checkInit()
    {
        if (db)
            return true;
        if (0 != sqlite3_open(NULL, &db))
            return false;
        char* errmsg = NULL;
        if (0 != sqlite3_exec(db, "CREATE TABLE store (rownum, data TEXT)", NULL, NULL, &errmsg))
            return false;
        return (db ? true : false);
    }

    bool addRow(int rownum, const XlsxStoreRow& row)
    {
        if (!checkInit())
            return false;

        kl::JsonNode node;
        node.setArray();

        std::map<int, XlsxStoreCol>::const_iterator it;
        for (it = row.values.begin(); it != row.values.end(); ++it)
        {
            kl::JsonNode element = node.appendElement();
            element.setObject();

            element["column"] = it->first;
            element["type"] = it->second.type;
            element["value"] = it->second.value;
        }


        std::wstring str = node.toString();
        kl::replaceStr(str, L"'", L"\\'");

        std::string sql = "INSERT INTO store (rownum, data) VALUES (" + kl::itostring(rownum) + ", '" + kl::tostring(str) + "')";

        char* errmsg = NULL;
        if (0 != sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg))
            return false;


        int row_column_count = 0;
        if (row.values.size() > 0)
        {
            row_column_count = row.values.rbegin()->first + 1;
            col_count = std::max(col_count, row_column_count);
        }

        row_count = std::max(row_count, rownum);

        return true;
    }

    bool getRow(int rownum, XlsxStoreRow& row)
    {
        if (!checkInit())
            return false;

        std::string query = "SELECT data FROM store WHERE rownum=";
        query += kl::itostring(rownum);

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, 0);
        if (rc != SQLITE_OK)
            return false;

        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);
            return false;
        }


        
        std::wstring json = kl::towstring((char*)sqlite3_column_text(stmt, 0));

        sqlite3_finalize(stmt);

        kl::JsonNode node;
        if (!node.fromString(json))
            return false;

        int i, cnt = node.getChildCount();
        for (i = 0; i < cnt; ++i)
        {
            kl::JsonNode element = node[i];
            if (element.childExists("column") && element.childExists("type") && element.childExists("value"))
            {
                XlsxStoreCol col;
                int colidx = element["column"].getInteger();
                col.type = kl::tostring(element["type"].getString());
                col.value = element["value"].getString();
                row.values[colidx] = col;
            }
        }


        return true;
    }

public:

    sqlite3* db;
    int col_count;
    int row_count;
};











static XlsxField EMPTY_XLSX_FIELD = XlsxField();
static XlsxDateTime EMPTY_XLSX_DATE = XlsxDateTime();

#define READ_BUFFER_SIZE 16384


// -- XlsxFile class implementation --

XlsxFile::XlsxFile()
{
    m_zip = NULL;
    m_store = new XlsxStore;
    m_col_count = 0;
    m_row_count = 0;
}

XlsxFile::~XlsxFile()
{
    closeFile();
    delete m_store;
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

    goRow(1);

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
    SheetParseData() { store = NULL; }

    XlsxStore* store;
    std::string curtag;
    int currow;
    int curcol;

    XlsxStoreCol storecol;
    XlsxStoreRow row;
};

static void sheetStart(void* user_data, const char* el, const char** attr)
{
    SheetParseData* data = (SheetParseData*)user_data;
    data->curtag = el;

    if (0 == strcmp(el, "row"))
    {
        data->row.values.clear();
        data->currow = -1;

        while (*attr)
        {
            if (0 == strcmp(*attr, "r"))
                data->currow = atoi(*(attr+1));
            attr += 2;
        }
    }
     else if (0 == strcmp(el, "c"))
    {
        data->curcol = -1;
        data->storecol = XlsxStoreCol();
        while (*attr)
        {
            if (0 == strcmp(*attr, "r"))
                data->curcol = getColumnIdxFromCoord(*(attr+1));
            else if (0 == strcmp(*attr, "t"))
                data->storecol.type = *(attr+1);
            attr += 2;
        }
    }

}

static void sheetEnd(void* user_data, const char* el)
{
    SheetParseData* data = (SheetParseData*)user_data;

    if (0 == strcmp(el, "row"))
    {
        if (data->currow != -1)
        {
            // append new row
            data->store->addRow(data->currow, data->row);
        }

        data->row.values.clear();
    }
}

static void sheetCharData(void* user_data, const XML_Char* s, int len)
{
    SheetParseData* data = (SheetParseData*)user_data;

    if (data->curtag == "v")
    {
        
        if (data->curcol != -1)
        {
            data->storecol.value = kl::utf8_utf8towstr(s, (size_t)len);
            data->row.values[data->curcol] = data->storecol;
        }

    }
}



bool XlsxFile::readSheet()
{
    // read in sheet file

    struct zip_file* sf = zip_fopen(m_zip, "xl/worksheets/sheet1.xml", 0);
    if (!sf)
        return false;
    
    SheetParseData data;
    data.store = m_store;

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

    m_col_count = m_store->col_count;
    m_row_count = m_store->row_count;

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

/*
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
*/

size_t XlsxFile::getColumnCount()
{
    return (size_t)m_col_count;
}

void XlsxFile::goRow(size_t row)
{
    m_currow = XlsxStoreRow();
    m_store->getRow((int)row, m_currow);
}


const std::wstring& XlsxFile::getString(size_t col_idx)
{
    std::map<int, XlsxStoreCol>::iterator it;
    it = m_currow.values.find((int)col_idx);
    if (it == m_currow.values.end())
    {
        m_str_result.clear();
        return m_str_result;
    }

    if (it->second.type == "s")
    {
        int shared_string_offset = kl::wtoi(it->second.value);
        std::map<int, std::wstring>::iterator ssit = m_shared_strings.find(shared_string_offset);
        if (ssit != m_shared_strings.end())
            m_str_result = ssit->second;
             else
            m_str_result = L"";
    }
     else
    {
        m_str_result = it->second.value;
    }

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

