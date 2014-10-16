/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#include <algorithm>
#include <ctime>
#include <kl/xcm.h>
#include <kl/file.h>
#include <kl/math.h>
#include <kl/string.h>
#include <xd/xd.h>
#include "fixedlength.h"
#include "../xdcommon/util.h"

#ifdef WIN32
#include <windows.h>
#endif


const std::string empty_string = "";
const std::wstring empty_wstring = L"";


FixedLengthTable::FixedLengthTable()
{
    m_file = NULL;
}

FixedLengthTable::~FixedLengthTable()
{
    close();
}


bool FixedLengthTable::open(const std::wstring& filename, const xd::FormatDefinition& def)
{
    // open table file
    m_filename = filename;
    m_file = xf_open(m_filename, xfOpen, xfReadWrite, xfShareReadWrite);
    if (m_file == NULL)
    {
        m_file = xf_open(m_filename, xfOpen, xfRead, xfShareReadWrite);
        if (m_file == NULL)
            return false;
    }

    m_def = def;

    return true;
}

bool FixedLengthTable::reopen(bool exclusive)
{
    return true;
}

void FixedLengthTable::close()
{
    KL_AUTO_LOCK(m_object_mutex);

    // close table, if it is open
    if (m_file)
    {
        xf_close(m_file);
        m_file = NULL;
    }
}


int FixedLengthTable::getRows(unsigned char* buf,
                              xd::rowpos_t* rowpos_arr,
                              int skip,
                              xd::rowpos_t start_row,
                              int row_count,
                              bool include_deleted)
{
    KL_AUTO_LOCK(m_object_mutex);

    xd::rowpos_t row_offset;
    unsigned long long byte_offset;
    int i, rows_read;

    row_offset = start_row;
    row_offset += skip;

    byte_offset = row_offset-1;
    byte_offset *= m_def.fixed_row_width;
    byte_offset += m_def.fixed_start_offset;
    xf_seek(m_file, byte_offset, xfSeekSet);
    rows_read = xf_read(m_file, buf, m_def.fixed_row_width, row_count);

    for (i = 0; i < rows_read; ++i)
        rowpos_arr[i] = row_offset+i;

    return rows_read;
}

bool FixedLengthTable::getRow(xd::rowpos_t row, unsigned char* buf)
{
    KL_AUTO_LOCK(m_object_mutex);

    xf_off_t pos;
    pos = row-1;
    pos *= m_def.fixed_row_width;
    pos += m_def.fixed_start_offset;

    xf_seek(m_file, pos, xfSeekSet);
    return (xf_read(m_file, buf, m_def.fixed_row_width, 1) == 1) ? true : false;
}


int FixedLengthTable::appendRows(unsigned char* buf, int row_count)
{
/*
    KL_AUTO_LOCK(m_object_mutex);

    // lock the header
    if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
        return false;

    xf_seek(m_file, 0, xfSeekEnd);

    int res = xf_write(m_file, buf, m_row_width, row_count);

    // unlock the header
    xf_unlock(m_file, 0, ttb_header_len);

    recalcPhysRowCount();

    return res;
*/

    return false;
}


bool FixedLengthTable::writeRow(xd::rowpos_t row, unsigned char* buf)
{
/*
    KL_AUTO_LOCK(m_object_mutex);

    xf_off_t pos;
    pos = row-1;
    pos *= m_row_width;
    pos += m_data_offset;

    // lock the record
    if (!xf_trylock(m_file, pos, m_row_width, 10000))
        return false;

    // write the record
    xf_seek(m_file, pos, xfSeekSet);
    xf_write(m_file, buf, m_row_width, 1);

    // unlock the record
    xf_unlock(m_file, pos, m_row_width);
    */

    return true;
}

int FixedLengthTable::getRowWidth()
{
    // zero-length rows not allowed
    if (m_def.fixed_row_width == 0)
        return 1;

    return m_def.fixed_row_width;
}

xd::Structure FixedLengthTable::getStructure()
{
    xd::Structure structure;

    std::vector<xd::ColumnInfo>::iterator it, it_end = m_def.columns.end();
    int counter = 0;
    for (it = m_def.columns.begin(); it != it_end; ++it)
    {
        xd::ColumnInfo col;
        
        col.name = it->name;
        col.type = it->type;
        col.width = it->width;
        col.scale = it->scale;
        col.source_offset = 0;
        col.calculated = false;
        col.column_ordinal = counter++;
        col.table_ordinal = 0;;
        col.nulls_allowed = it->nulls_allowed;

        structure.createColumn(col);
    }

    return structure;
}





xd::rowpos_t FixedLengthTable::getRowCount()
{
    KL_AUTO_LOCK(m_object_mutex);

    if (m_def.fixed_row_width == 0)
        return 0;

    if (m_def.fixed_line_delimited)
        return 0;

    xf_seek(m_file, 0, xfSeekEnd);
    xd::rowpos_t res = (xd::rowpos_t)xf_get_file_pos(m_file);
    res /= m_def.fixed_row_width;

    return res;
}



std::wstring FixedLengthTable::getFilename()
{
    return m_filename;
}







FixedLengthRow::FixedLengthRow()
{
    m_table = NULL;
    m_rowptr = NULL;
}


void FixedLengthRow::clearRow()
{
}


const unsigned char* FixedLengthRow::getRawPtr(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];
    return m_rowptr + dai->source_offset;
}

int FixedLengthRow::getRawWidth(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];
    return dai->source_width;
}

const std::string& FixedLengthRow::getString(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];
    int i;
    unsigned char* p = m_rowptr + dai->source_offset;

    m_str_result.reserve(dai->source_width);
    m_str_result.clear();


    if (dai->source_encoding == xd::encodingUCS2 || dai->source_encoding == xd::encodingUTF16)
    {
        kl::ucsle2string(m_str_result, m_rowptr + dai->source_offset, dai->source_width);
        kl::trim(m_str_result);
    }
     else if (dai->source_encoding == xd::encodingEBCDIC)
    {
        if (column_ordinal == 7)
        {
            int i = 5;
        }

        for (i = 0; i < dai->source_width; ++i)
            m_str_result += ebcdic2ascii(p[i]);

        kl::trim(m_str_result);
    }
     else if (dai->source_encoding == xd::encodingCOMP)
    {/*
        unsigned char buf[48];
        size_t i;
        if (len > 42)
            len = 42;
        for (i = 0; i < len; ++i)
            buf[i] = (unsigned char)getChar(m_cur_row, off++);
        
        compToString(buf, len, 0, dai->wstr_result);
        */
    }
     else if (dai->source_encoding == xd::encodingCOMP3)
    {
    /*
        unsigned char buf[48];
        size_t i;
        if (len > 42)
            len = 42;
        for (i = 0; i < len; ++i)
            buf[i] = (unsigned char)getChar(m_cur_row, off++);
        
        comp3ToString(buf, len, 0, dai->wstr_result);
        */
    }
     else
    {
        for (i = 0; i < dai->source_width; ++i)
        {
            if (!p[i])
                break;
            m_str_result += p[i];
        }

        kl::trim(m_str_result);
    }
    
    return m_str_result;
}

const std::wstring& FixedLengthRow::getWideString(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];
    int i;
    unsigned char* p = m_rowptr + dai->source_offset;

    if (dai->source_encoding == xd::encodingUCS2 || dai->source_encoding == xd::encodingUTF16)
    {
        kl::ucsle2wstring(m_wstr_result, m_rowptr + dai->source_offset, dai->source_width);
        kl::trim(m_wstr_result);
    }
     else if (dai->source_encoding == xd::encodingEBCDIC)
    {
        for (int i = 0; i < dai->source_width; ++i)
            m_wstr_result += (wchar_t)ebcdic2ascii(*(unsigned char*)(m_rowptr + dai->source_offset + i));

        kl::trim(m_wstr_result);
    }
     else if (dai->source_encoding == xd::encodingCOMP)
    {/*
        unsigned char buf[48];
        size_t i;
        if (len > 42)
            len = 42;
        for (i = 0; i < len; ++i)
            buf[i] = (unsigned char)getChar(m_cur_row, off++);
        
        compToString(buf, len, 0, dai->wstr_result);
        */
    }
     else if (dai->source_encoding == xd::encodingCOMP3)
    {
    /*
        unsigned char buf[48];
        size_t i;
        if (len > 42)
            len = 42;
        for (i = 0; i < len; ++i)
            buf[i] = (unsigned char)getChar(m_cur_row, off++);
        
        comp3ToString(buf, len, 0, dai->wstr_result);
        */
    }
     else
    {
        for (i = 0; i < dai->source_width; ++i)
        {
            if (!p[i])
                break;
            m_wstr_result += (wchar_t)p[i];
        }

        kl::trim(m_str_result);
    }
    
    return m_wstr_result;
}

xd::datetime_t FixedLengthRow::getDateTime(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];

    return 0;
}

double FixedLengthRow::getDouble(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];
    return 0;
}

int FixedLengthRow::getInteger(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];
    return 0;
}

bool FixedLengthRow::getBoolean(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];
    return false;
}

bool FixedLengthRow::isNull(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];
    return false;
}


bool FixedLengthRow::putRawPtr(int column_ordinal, const unsigned char* value, int length)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];

    int write_len = length;
    if (write_len > dai->width)
        write_len = dai->width;

    memset(m_rowptr + dai->source_offset, 0, dai->width);
    memcpy(m_rowptr + dai->source_offset, value, write_len);

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->source_offset - 1) &= 0xfe;
    }

    return false;
}


bool FixedLengthRow::putString(int column_ordinal, const std::string& value)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];

    // set data
    if (dai->type == xd::typeCharacter)
    {
        memset(m_rowptr + dai->source_offset, 0, dai->width);

        int write_len = value.length();
        if (write_len > dai->width)
            write_len = dai->width;

        memcpy(m_rowptr + dai->source_offset, value.c_str(), write_len);
    }
     else if (dai->type == xd::typeWideCharacter)
    {
        kl::string2ucsle(m_rowptr + dai->source_offset, value, dai->width);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->source_offset - 1) &= 0xfe;
    }

    return true;
}

bool FixedLengthRow::putWideString(int column_ordinal, const std::wstring& value)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];

    // set data
    if (dai->type == xd::typeWideCharacter)
    {
        kl::wstring2ucsle(m_rowptr + dai->source_offset, value, dai->width);
    }
     else if (dai->type == xd::typeCharacter)
    {
        memset(m_rowptr + dai->source_offset, 0, dai->width);

        std::string ascvalue = kl::tostring(value);

        int write_len = ascvalue.length();
        if (write_len > dai->width)
            write_len = dai->width;

        memcpy(m_rowptr + dai->source_offset, ascvalue.c_str(), write_len);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->source_offset - 1) &= 0xfe;
    }

    return true;
}

bool FixedLengthRow::putDouble(int column_ordinal, double value)
{
/*
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];

    if (dai->width < 1)
        return false;

    if (dai->type == xd::typeNumeric)
    {
        dbl2decstr((char*)m_rowptr + dai->source_offset,
                   value,
                   dai->width,
                   dai->scale);
    }
     else if (dai->type == xd::typeDouble)
    {
        // FIXME: this will only work on little-endian (intel) processors
        memcpy(m_rowptr + dai->source_offset, &value, sizeof(double));
    }
     else if (dai->type == xd::typeInteger)
    {
        int2buf(m_rowptr + dai->source_offset, (int)value);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->source_offset - 1) &= 0xfe;
    }
    */
    return true;
}

bool FixedLengthRow::putInteger(int column_ordinal,
                                int value)
{
/*
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];


    if (dai->type == xd::typeInteger)
    {
        // set data
        unsigned char* ptr = m_rowptr+dai->source_offset;
        unsigned int v = (unsigned int)value;
        *(ptr)   = (v) & 0xff;
        *(ptr+1) = (v >> 8) & 0xff;
        *(ptr+2) = (v >> 16) & 0xff;
        *(ptr+3) = (v >> 24) & 0xff;
    }
     else if (dai->type == xd::typeNumeric)
    {
        dbl2decstr((char*)m_rowptr + dai->source_offset,
                   value,
                   dai->width,
                   dai->scale);
    }
     else if (dai->type == xd::typeDouble)
    {
        // FIXME: this will only work on little-endian (intel) processors
        double d = value;
        memcpy(m_rowptr + dai->source_offset, &d, sizeof(double));
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->source_offset - 1) &= 0xfe;
    }

    return true;
    */
    return true;
}

bool FixedLengthRow::putBoolean(int column_ordinal,
                                bool value)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];

    // set data
    *(m_rowptr + dai->source_offset) = (value ? 'T' : 'F');

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->source_offset - 1) &= 0xfe;
    }

    return true;

}

bool FixedLengthRow::putDateTime(int column_ordinal,
                         xd::datetime_t value)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];

    // set data
    if (dai->type == xd::typeDate)
    {
        unsigned int julian_day = (unsigned int)(value >> 32);

        int2buf(m_rowptr+dai->source_offset, julian_day);
    }
     else if (dai->type == xd::typeDateTime)
    {
        unsigned int julian_day = (unsigned int)(value >> 32);
        unsigned int time_stamp = (unsigned int)(value & 0xffffffff);

        int2buf(m_rowptr+dai->source_offset, julian_day);
        int2buf(m_rowptr+dai->source_offset+4, time_stamp);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->source_offset - 1) &= 0xfe;
    }

    return true;

}


bool FixedLengthRow::putNull(int column_ordinal)
{
    xd::ColumnInfo* dai = &m_table->m_def.columns[column_ordinal];

    if (!dai->nulls_allowed)
        return false;

    *(m_rowptr + dai->source_offset - 1) |= 0x01;

    return true;
}


