/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#include <ctime>
#include <kl/xcm.h>
#include <kl/file.h>
#include <kl/math.h>
#include <kl/string.h>
#include <xd/xd.h>
#include "ttbfile.h"
#include "../xdcommon/util.h"

#ifdef WIN32
#include <windows.h>
#endif


// ------------------ FILE FORMAT: TTB Table --------------------
//
// -- file header --
// offset   00:   (uint32)  signature 0xddaa2299;
// offset   04:   (uint32)  version
// offset   08:   (uint32)  data begin offset
// offset   12:   (uint32)  row width
// offset   16:   (uint32)  column count
// offset   20:   (uint64)  physical row count
// offset   28:   (uint64)  last structure modify time
// offset   36:   (uint128) GUID for this file; 0 in older file versions
// offset   52:   (uint64)  deleted row count
// (null padding)
// offset 1024:   column descriptor list

// -- column descriptor --
// offset   00:   (byte)   type
// offset   01:   (uint32) data offset
// offset   05:   (uint32) width [not size]
// offset   09:   (uint32) decimals
// offset   13:   (uint32) flags
// (null padding)
// offset   64:   (null-terminated string max_len=80) column name
//
// -- column flags --
// bit 0x0001:    (1 = nulls allowed)
//
// -- indicator byte flags --
// bit 0x0001:    (1 = field value null)



//  --- Format Notes ----------------------------------
//
//      If a field is nullable, the byte before the field
//      offset contains a null indicator byte


//  --- Versioning Notes ----------------------------------
//
//      Table version 1 has only ASCII (8-bit) field names.
//      Version 2 and greater, however, store field names in
//      UNICODE, using the UCS-2 (16-bit) encoding

//      Version 3 adds a single byte to the beginning of each
//      row data buffer.  0xff indicates a deleted record
//
//      The deleted record count is stored in the header



char convertType_xd2ttb(int xd_type)
{
    switch (xd_type)
    {
        case xd::typeCharacter:      return 'C';
        case xd::typeWideCharacter:  return 'W';
        case xd::typeNumeric:        return 'N';
        case xd::typeDouble:         return 'B';
        case xd::typeDate:           return 'D';
        case xd::typeDateTime:       return 'T';
        case xd::typeBoolean:        return 'L';
        case xd::typeInteger:        return 'I';
    }

    return ' ';
}

int convertType_ttb2xd(int native_type)
{
    switch (native_type)
    {
        case 'C':   return xd::typeCharacter;
        case 'W':   return xd::typeWideCharacter;
        case 'N':   return xd::typeNumeric;
        case 'B':   return xd::typeDouble;
        case 'F':   return xd::typeNumeric;
        case 'D':   return xd::typeDate;
        case 'T':   return xd::typeDateTime;
        case 'L':   return xd::typeBoolean;
        case 'I':   return xd::typeInteger;
    }

    return xd::typeInvalid;
}

static bool isGuidZero(const unsigned char* guid)
{
    for (int i = 0; i < 16; ++i)
    {
        if (guid[i]) return false;
    }

    return true;
}


TtbTable::TtbTable()
{
    m_file = NULL;
    memset(m_guid, 0, 16);
}

TtbTable::~TtbTable()
{
    close();
}

bool TtbTable::create(const std::wstring& filename, const std::vector<xd::ColumnInfo>& structure)
{
    int column_count = (int)structure.size();
    int i;
    int col_type;
    int col_width;
    int col_scale;
    bool nulls_allowed;
    unsigned char col_flags;

    // check field widths and scales
    for (i = 0; i < column_count; ++i)
    {
        const xd::ColumnInfo& colinfo = structure[i];

        col_type = colinfo.type;
        col_width = colinfo.width;
        col_scale = colinfo.scale;

        if (col_type == xd::typeNumeric)
        {
            if (col_width > xd::max_numeric_width)
            {
                col_width = xd::max_numeric_width;
            }

            if (col_scale > xd::max_numeric_scale)
                return false;

            if (col_scale < 0)
                return false;

            if (col_width < 1)
                return false;
        }

        if (col_type == xd::typeCharacter)
        {
            if (col_width > xd::max_character_width)
                return false;

            if (col_width < 1)
                return false;

            if (col_scale != 0)
                return false;
        }
    }


    xf_file_t f = xf_open(filename, xfCreate, xfWrite, xfShareNone);
    if (f == NULL)
        return false;


    int field_arr_len = column_count * ttb_column_descriptor_len;
    unsigned char* header = new unsigned char[ttb_header_len];
    unsigned char* flds = new unsigned char[field_arr_len];

    // create ttb format file

    memset(header, 0, ttb_header_len);
    memset(flds, 0, field_arr_len);

    // fill out field array

    unsigned char* entry_ptr;
    unsigned int offset;

    offset = 1;       // leave byte for row status flag (delete flag etc)
    entry_ptr = flds;


    int cols_written = 0;

    for (i = 0; i < column_count; ++i)
    {
        const xd::ColumnInfo& colinfo = structure[i];
        
        if (colinfo.calculated)
            continue;

        cols_written++;

        col_type = colinfo.type;
        col_width = colinfo.width;
        col_scale = colinfo.scale;
        nulls_allowed = colinfo.nulls_allowed;

        switch (col_type)
        {
            case xd::typeDate:     col_width = 4; col_scale = 0; break;
            case xd::typeInteger:  col_width = 4; col_scale = 0; break;
            case xd::typeDouble:   col_width = 8; break;
            case xd::typeBoolean:  col_width = 1; col_scale = 0; break;
            case xd::typeDateTime: col_width = 8; col_scale = 0; break;
            default:                  break;
        }
        
        if (nulls_allowed)
        {
            // make space for the null indicator
            offset++;
        }

        // type
        entry_ptr[0] = convertType_xd2ttb(col_type);

        // offset
        int2buf(entry_ptr+1, offset);

        // width
        int2buf(entry_ptr+5, col_width);

        // number scale
        int2buf(entry_ptr+9, col_scale);

        // flags
        col_flags = 0;
        if (nulls_allowed)
            col_flags |= 0x01;

        int2buf(entry_ptr+13, col_flags);

        // field name (80 chars, 160 bytes)
        kl::wstring2ucsle(entry_ptr+64, colinfo.name, 80);

        offset += col_width;

        if (col_type == xd::typeWideCharacter)
        {
            // UCS-2 fields take up twice the space
            offset += col_width;
        }

        entry_ptr += ttb_column_descriptor_len;
    }

    column_count = cols_written;

    // fill out header values

    // signature
    header[0] = 0x99;
    header[1] = 0x22;
    header[2] = 0xaa;
    header[3] = 0xdd;

    // version
    int2buf(header+4, 3);   // we now write files with version 3

    // data begin offset
    int2buf(header+8, ttb_header_len+field_arr_len);

    // row width
    int2buf(header+12, offset);

    // column count
    int2buf(header+16, column_count);

    // row count
    int2buf(header+20, 0); // lower 32 bits
    int2buf(header+24, 0); // upper 32 bits

    // guid
    generateGuid(header+36);


    // write out the header info
    
    if (xf_write(f, header, ttb_header_len, 1) != 1)
    {
        delete[] flds;
        delete[] header;
        xf_close(f);
        return false;
    }

    if (xf_write(f, flds, field_arr_len, 1) != 1)
    {
        delete[] flds;
        delete[] header;
        xf_close(f);
        return false;
    }

    delete[] flds;
    delete[] header;

    xf_close(f);

    return true;
}


bool TtbTable::open(const std::wstring& filename)
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

    unsigned char header[ttb_header_len];

    if (xf_read(m_file, header, ttb_header_len, 1) == 0)
    {
        xf_unlock(m_file, 0, ttb_header_len);
        xf_close(m_file);
        m_file = NULL;
        return false;
    }

    unsigned int signature, version;
    signature = buf2int(header);
    version = buf2int(header+4);
    if (signature != 0xddaa2299)
    {
        xf_close(m_file);
        m_file = NULL;
        return false;
    }

    // check file version
    if (version <= 1 || version > 3)
    {
        xf_close(m_file);
        m_file = NULL;
        return false; // no longer supported
    }

    m_data_offset = buf2int(header+8);
    m_row_width = buf2int(header+12);
    m_phys_row_count = bufToInt64(header+20);

    if (m_row_width == 0)
    {
        xf_close(m_file);
        m_file = NULL;
        return false;
    }

    // get guid
    memcpy(m_guid, header+36, 16);
    if (isGuidZero(m_guid))
    {
        updateHeaderWithGuid();
    }

    return true;
}

bool TtbTable::reopen(bool exclusive)
{
    KL_AUTO_LOCK(m_object_mutex);

    if (m_file)
    {
        xf_close(m_file);
        m_file = NULL;
    }

    m_file = xf_open(m_filename,
                     xfOpen,
                     xfReadWrite,
                     exclusive ? xfShareNone : xfShareReadWrite);

    if (m_file == NULL)
    {
        m_file = xf_open(m_filename,
                         xfOpen,
                         xfReadWrite,
                         xfShareReadWrite);

        return false;
    }

    return true;
}

void TtbTable::close()
{
    KL_AUTO_LOCK(m_object_mutex);

    // close table, if it is open
    if (m_file)
    {
        xf_close(m_file);
        m_file = NULL;
    }
}


void TtbTable::generateGuid(unsigned char* guid /* 16 bytes */)
{
#ifdef WIN32
    GUID gid;
    CoCreateGuid(&gid);
    int size = sizeof(gid);
    if (sizeof(gid) == 16)
    {
        memcpy(guid, &gid, 16);
        return;
    }
#else
    // try to generate a new guid ourselves
    unsigned int i1 = (time(NULL) & 0xffffffff);
    unsigned int i2 = (clock() & 0xffffffff);
    unsigned int i3 = (unsigned int)rand();
    unsigned int i4 = (unsigned int)rand();
    int2buf(guid, i1);
    int2buf(guid+4, i2);
    int2buf(guid+8, i3);
    int2buf(guid+12, i4);
#endif
}

bool TtbTable::getGuid(unsigned char* guid /* 16 bytes */)
{
    memset(guid, 0, 16);
    if (isGuidZero(m_guid))
        return false;
    memcpy(guid, m_guid, 16);
    return true;
}

bool TtbTable::setGuid(const unsigned char* guid /* 16 bytes */)
{
    memcpy(m_guid, guid, 16);

    // lock the header
    if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
        return false;

    xf_seek(m_file, 36, xfSeekSet);

    int res = xf_write(m_file, guid, 1, 16);

    xf_unlock(m_file, 0, ttb_header_len);

    if (res != 16)
        return false;

    return true;
}

void TtbTable::updateHeaderWithGuid()
{
    unsigned char guid[16];

    // lock the header
    if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
        return;

    xf_seek(m_file, 36, xfSeekSet);

    int res = xf_read(m_file, guid, 1, 16);
    if (res != 16)
    {
        xf_unlock(m_file, 0, ttb_header_len);
        return;
    }


    if (isGuidZero(guid))
    {
        generateGuid(guid);

        xf_seek(m_file, 36, xfSeekSet);
        if (16 != xf_write(m_file, guid, 1, 16))
        {
            xf_unlock(m_file, 0, ttb_header_len);
            return;
        }
    }

    memcpy(m_guid, guid, 16);

    // unlock the header
    xf_unlock(m_file, 0, ttb_header_len);
}


int TtbTable::getRows(unsigned char* buf,
                      xd::rowpos_t* rowpos_arr,
                      int skip,
                      xd::rowpos_t start_row,
                      int row_count,
                      bool include_deleted)
{
    int i;
    unsigned long long byte_offset;
    xd::rowpos_t row_offset;
    int rows_read;
    bool reuse_buffer = false;

    if (include_deleted)
    {
        // we want all rows, including ones flagged with delete marks
        row_offset = start_row;
        row_offset += skip;

        byte_offset = row_offset-1;
        byte_offset *= m_row_width;
        byte_offset += m_data_offset;
        xf_seek(m_file, byte_offset, xfSeekSet);
        rows_read = xf_read(m_file, buf, m_row_width, row_count);

        for (i = 0; i < rows_read; ++i)
            rowpos_arr[i] = row_offset+i;

        return rows_read;
    }


    if (skip == 0 || skip == 1)
    {
        // specific optimization for skip 0 or 1 (if there are no deleted rows in the block)

        start_row += skip;
        row_offset = start_row;
        skip = 0;

        byte_offset = row_offset-1;
        byte_offset *= m_row_width;
        byte_offset += m_data_offset;
        xf_seek(m_file, byte_offset, xfSeekSet);
        rows_read = xf_read(m_file, buf, m_row_width, row_count);

        if (rows_read == 0)
        {
            return 0; // can't read any rows, we're done
        }

        // check for deleted records
        bool deleted_found = false;
        for (i = 0; i < rows_read; ++i)
        {
            rowpos_arr[i] = row_offset+i;
            if (buf[m_row_width*i] == 0xff)
            {
                deleted_found = true;
                break;
            }
        }
        if (!deleted_found)
        {
            return rows_read;
        }

        reuse_buffer = true;
    }


    int rows_per_work_buffer = row_count;
    if (rows_per_work_buffer < 100)
        rows_per_work_buffer *= 2;
    if (rows_per_work_buffer < 10)
        rows_per_work_buffer = 10;

    m_workbuf_mutex.lock();

    m_workbuf.alloc(rows_per_work_buffer * m_row_width);
    unsigned char* workbuf = m_workbuf.getData();
    unsigned char* p;

    int filled = 0;
    int fill_iteration_count = 0;

    if (skip >= 0)
    {
        row_offset = start_row;

        while (true)
        {
            if (reuse_buffer)
            {
                // we already had read a chunk in, but it needs further
                // examination for delete flags; copy it via memcpy here so
                // we don't need to read it from the disk again
                memcpy(workbuf, buf, rows_read * m_row_width);
                reuse_buffer = false;
            }
             else
            {
                byte_offset = row_offset-1;
                byte_offset *= m_row_width;
                byte_offset += m_data_offset;
                xf_seek(m_file, byte_offset, xfSeekSet);
                rows_read = xf_read(m_file, workbuf, m_row_width, rows_per_work_buffer);
                if (rows_read == 0)
                {
                    m_workbuf_mutex.unlock();
                    return filled; // can't read any more rows, we're done
                }
            }

            // copy undeleted rows into output buffer
            p = workbuf;
            for (i = 0; i < rows_read; ++i, p += m_row_width)
            {
                if (*p != 0xff)
                {
                    if (skip > 0)
                    {
                        skip--;
                        continue;
                    }

                    memcpy(buf + (filled * m_row_width), p, m_row_width);
                    rowpos_arr[filled] = row_offset + i;
                    filled++;
                    if (filled == row_count) // is request compete?
                        break;
                }
            }

            if (filled == row_count)
            {
                m_workbuf_mutex.unlock();
                return filled; // all done
            }

            row_offset += rows_read;

            if (fill_iteration_count++ == 10)
            {
                // make row buffer bigger after ten iterations
                int new_rows_per_work_buffer = (1000000 / m_row_width);
                if (new_rows_per_work_buffer < 1)
                    new_rows_per_work_buffer = 1;
                if (new_rows_per_work_buffer > rows_per_work_buffer)
                {
                    rows_per_work_buffer = new_rows_per_work_buffer;
                    m_workbuf.alloc(rows_per_work_buffer * m_row_width);
                    workbuf = m_workbuf.getData();
                }
            }
        }
    }
     else
    {
        // backwards scroll

        row_offset = start_row;
        int rows_to_read;


        // the initial read will leave off the row we are skipping from,
        // so subtract one from the skip (which is a negative value, so add one)
        skip++;

        while (row_offset > 1)
        {
            rows_to_read = rows_per_work_buffer;
            if (row_offset-1 < rows_to_read)
                rows_to_read = (int)row_offset-1;
            row_offset -= rows_to_read;

            byte_offset = row_offset-1;
            byte_offset *= m_row_width;
            byte_offset += m_data_offset;
            xf_seek(m_file, byte_offset, xfSeekSet);
            rows_read = xf_read(m_file, workbuf, m_row_width, rows_to_read);
            if (rows_read == 0)
            {
                m_workbuf_mutex.unlock();
                return 0;
            }


            // copy undeleted rows into output buffer
            p = workbuf + ((rows_read-1)*m_row_width);

            for (i = rows_read-1; i >= 0; --i, p -= m_row_width)
            {
                if (*p != 0xff)
                {
                    if (-skip < row_count)
                    {
                        memcpy(buf + ((row_count-filled-1) * m_row_width), p, m_row_width);
                        rowpos_arr[row_count-filled-1] = row_offset + i;
                        filled++;
                        if (filled == row_count) // is request compete?
                            break;
                    }
                    skip++;
                }
            }

            if (filled == row_count)
            {
                m_workbuf_mutex.unlock();
                return filled; // all done
            }
        }


        // ran into bof; shift data up
        memmove(buf, buf + ((row_count-filled) * m_row_width), filled * m_row_width);
        for (i = 0; i < filled; ++i)
            rowpos_arr[i] = rowpos_arr[i+row_count-filled];

        m_workbuf_mutex.unlock();
        return filled; // all done
    }


}

bool TtbTable::getRow(xd::rowpos_t row, unsigned char* buf)
{
    KL_AUTO_LOCK(m_object_mutex);

    xf_off_t pos;
    pos = row-1;
    pos *= m_row_width;
    pos += m_data_offset;

    xf_seek(m_file, pos, xfSeekSet);
    return (xf_read(m_file, buf, m_row_width, 1) == 1) ? true : false;
}




int TtbTable::appendRows(unsigned char* buf, int row_count)
{
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
}


bool TtbTable::writeRow(xd::rowpos_t row, unsigned char* buf)
{
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

    return true;
}


bool TtbTable::writeColumnInfo(int col_idx,
                               const std::wstring& col_name,
                               int type,
                               int width,
                               int scale)
{
    KL_AUTO_LOCK(m_object_mutex);

    if (col_idx < 0 || col_idx >= (int)m_fields.size())
        return false;
    TtbField& field = m_fields[col_idx];


    // lock the header
    if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
        return false;

    // this will update the column name in the header
    unsigned char col_desc[ttb_column_descriptor_len];
    memset(col_desc, 0, ttb_column_descriptor_len);

    xf_seek(m_file, ttb_header_len+(ttb_column_descriptor_len*col_idx), xfSeekSet);
    xf_read(m_file, col_desc, ttb_column_descriptor_len, 1);


    if (col_name.length() > 0)
    {
        // field name (80 chars, 160 bytes)
        field.name = col_name;
        kl::wstring2ucsle(col_desc+64, col_name, 80);
    }

    if (type != -1)
    {
        field.ttb_type = convertType_xd2ttb(type);
        col_desc[0] = field.ttb_type;
    }

    if (width != -1)
    {
        field.width = width;
        int2buf(col_desc+5, width);
    }

    if (scale != -1)
    {
        field.scale = scale;
        int2buf(col_desc+9, scale);
    }

    xf_seek(m_file,
            ttb_header_len+(ttb_column_descriptor_len*col_idx),
            xfSeekSet);

    xf_write(m_file,
             col_desc,
             ttb_column_descriptor_len,
             1);

    setStructureModified();

    // unlock the header
    xf_unlock(m_file, 0, ttb_header_len);

    return true;
}



unsigned long long TtbTable::getStructureModifyTime()
{
    KL_AUTO_LOCK(m_object_mutex);

    unsigned char sig[8];

    if (!xf_seek(m_file, 28, xfSeekSet))
        return 0;

    if (xf_read(m_file, sig, 8, 1) != 1)
        return 0;

    return (unsigned long long)bufToInt64(sig);
}


bool TtbTable::setStructureModified()
{
    KL_AUTO_LOCK(m_object_mutex);

    unsigned char sig[8];
    unsigned long long i = 0;

    i = time(NULL);
    i <<= 32;
    i |= clock();

    int64ToBuf(sig, i);

    if (!xf_seek(m_file, 28, xfSeekSet))
        return false;

    if (xf_write(m_file, sig, 8, 1) != 1)
        return false;

    return true;
}



int TtbTable::getRowWidth()
{
    return m_row_width;
}

xd::Structure TtbTable::getStructure()
{
    if (m_fields.size() == 0)
    {
        unsigned char* flds;

        // lock the header
        if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
            return xd::Structure();

        // get column count
        unsigned char buf[4];
        memset(buf, 0, sizeof(buf));
        if (xf_seek(m_file, 16, xfSeekSet))
            xf_read(m_file, buf, 1, 4);

        int i, column_count;
        column_count = buf2int(buf);
        if (column_count == 0)
        {
            xf_unlock(m_file, 0, ttb_header_len);
            return xd::Structure();
        }

        if (!xf_seek(m_file, ttb_header_len, xfSeekSet))
        {
            xf_unlock(m_file, 0, ttb_header_len);
            return xd::Structure();
        }

        // read in columns data

        flds = new unsigned char[column_count * ttb_column_descriptor_len];
    
        if (xf_read(m_file, flds, column_count * ttb_column_descriptor_len, 1) != 1)
        {
            delete[] flds;
            xf_unlock(m_file, 0, ttb_header_len);
            return xd::Structure();
        }

        xf_unlock(m_file, 0, ttb_header_len);

        unsigned char* fld = flds;
        std::wstring col_name;
        TtbField fldinfo;
        for (i = 0; i < column_count; ++i)
        {
            kl::ucsle2wstring(fldinfo.name, fld+64, 80);
            fldinfo.ttb_type = fld[0];
            fldinfo.width = buf2int(fld+5);
            fldinfo.scale = buf2int(fld+9);
            fldinfo.offset = buf2int(fld+1);
            fldinfo.ordinal = i;
            fldinfo.nulls_allowed = (*(fld+13) & 0x01) ? true : false;

            m_fields.push_back(fldinfo);
            fld += ttb_column_descriptor_len;
        }
    
        delete[] flds;
    }



    xd::Structure s;

    std::vector<TtbField>::iterator it, it_end = m_fields.end();
    int counter = 0;
    for (it = m_fields.begin(); it != it_end; ++it)
    {
        xd::ColumnInfo col;
        
        col.name = it->name;
        col.type = convertType_ttb2xd(it->ttb_type);
        col.width = it->width;
        col.scale = it->scale;
        col.source_offset = it->offset;
        col.calculated = false;
        col.column_ordinal = counter++;
        col.table_ordinal = 0;
        col.nulls_allowed = it->nulls_allowed;

        s.createColumn(col);
    }

    return s;
}



bool TtbTable::deleteRow(xd::rowid_t rowid)
{
    // lock the header
    if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
        return false;

    xf_off_t byte_offset = rowid-1;
    byte_offset *= m_row_width;
    byte_offset += m_data_offset;
    if (!xf_seek(m_file, byte_offset, xfSeekSet))
    {
        // seek error -- unlock the header
        xf_unlock(m_file, 0, ttb_header_len);
        return false;
    }

    unsigned char current_delete_flag = 0;
    if (xf_read(m_file, &current_delete_flag, 1, 1) != 1)
    {
        // read error -- unlock header, return false
        xf_unlock(m_file, 0, ttb_header_len);
        return false;
    }

    if (current_delete_flag == 0xff)
    {
        // delete flag already set, we are done
        xf_unlock(m_file, 0, ttb_header_len);
        return true;
    }

    // set delete flag
    unsigned char deleted = 0xff;
    xf_seek(m_file, byte_offset, xfSeekSet);
    if (xf_write(m_file, &deleted, 1, 1) != 1)
    {
        // write error -- unlock header, return false
        xf_unlock(m_file, 0, ttb_header_len);
        return false;
    }

    // increment deleted record count in header
    unsigned char buf[8];

    xf_seek(m_file, 52, xfSeekSet);
    xf_read(m_file, buf, 8, 1);

    long long delete_count = bufToInt64(buf);
    int64ToBuf(buf, delete_count+1);
    
    xf_seek(m_file, 52, xfSeekSet);
    xf_write(m_file, buf, 8, 1);

    // unlock the header
    xf_unlock(m_file, 0, ttb_header_len);

    return true;
}



xd::rowpos_t TtbTable::getRowCount(xd::rowpos_t* deleted_row_count)
{
    KL_AUTO_LOCK(m_object_mutex);

    // lock the header
    if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
        return m_phys_row_count;

    unsigned char buf[40];
    if (!xf_seek(m_file, 20, xfSeekSet))
    {
        xf_unlock(m_file, 0, ttb_header_len);
        return false;
    }

    if (40 != xf_read(m_file, buf, 1, 40))
    {
        xf_unlock(m_file, 0, ttb_header_len);
        return m_phys_row_count;
    }

    // unlock header
    xf_unlock(m_file, 0, ttb_header_len);

    {
        m_phys_row_count = bufToInt64(buf);

        if (deleted_row_count)
        {
            *deleted_row_count = bufToInt64(buf+32);
        }

        return m_phys_row_count;
    }
}



xd::rowpos_t TtbTable::recalcPhysRowCount()
{
    KL_AUTO_LOCK(m_object_mutex);

    // calculate the actual physical row count from the file size
    xf_seek(m_file, 0, xfSeekEnd);
    xd::rowpos_t file_size = xf_get_file_pos(m_file);
    file_size -= m_data_offset;
    xd::rowpos_t real_phys_row_count = (file_size/m_row_width);

    if (real_phys_row_count != m_phys_row_count)
    {
        // lock the header
        if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
            return real_phys_row_count;

        m_phys_row_count = real_phys_row_count;

        // update record count in header
        unsigned char buf[8];
        int64ToBuf(buf, m_phys_row_count);
        xf_seek(m_file, 20, xfSeekSet);
        xf_write(m_file, buf, 8, 1);

        // unlock the header
        xf_unlock(m_file, 0, ttb_header_len);
    }

    return real_phys_row_count;
}



std::wstring TtbTable::getFilename()
{
    return m_filename;
}

bool TtbTable::restoreDeleted()
{
    return true;
}














static void dbl2decstr(char* dest, double d, int width, int scale)
{
    double intpart;

    // check for negative
    if (d < 0.0)
    {
        *dest = '-';
        dest++;
        width--;
        d = fabs(d);
    }

    // rounding
    d += (0.5/kl::pow10(scale));

    // put everything to the right of the decimal
    d /= kl::pow10(width-scale);

    while (width)
    {
        d *= 10;
        d = modf(d, &intpart);
        if (intpart > 9.1)
            intpart = 0.0;

        *dest = int(intpart) + '0';
        dest++;
        width--;
    }
}




TtbRow::TtbRow()
{
    m_table = NULL;
    m_rowptr = NULL;
}


void TtbRow::clearRow()
{
    memset(m_rowptr, 0, m_table->m_row_width);
}

bool TtbRow::putRawPtr(int column_ordinal,
                       const unsigned char* value,
                       int length)
{
    TtbField* dai = &m_table->m_fields[column_ordinal];

    int write_len = length;
    if (write_len > dai->width)
        write_len = dai->width;

    memset(m_rowptr + dai->offset, 0, dai->width);
    memcpy(m_rowptr + dai->offset, value, write_len);

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    return false;
}


bool TtbRow::putString(int column_ordinal,
                       const std::string& value)
{
    TtbField* dai = &m_table->m_fields[column_ordinal];

    // set data
    if (dai->ttb_type == TtbTable::typeCharacter)
    {
        memset(m_rowptr + dai->offset, 0, dai->width);

        int write_len = value.length();
        if (write_len > dai->width)
            write_len = dai->width;

        memcpy(m_rowptr + dai->offset, value.c_str(), write_len);
    }
     else if (dai->ttb_type == TtbTable::typeWideCharacter)
    {
        kl::string2ucsle(m_rowptr + dai->offset, value, dai->width);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    return true;
}

bool TtbRow::putWideString(int column_ordinal,
                           const std::wstring& value)
{
    TtbField* dai = &m_table->m_fields[column_ordinal];

    // set data
    if (dai->ttb_type == TtbTable::typeWideCharacter)
    {
        kl::wstring2ucsle(m_rowptr + dai->offset, value, dai->width);
    }
     else if (dai->ttb_type == TtbTable::typeCharacter)
    {
        memset(m_rowptr + dai->offset, 0, dai->width);

        std::string ascvalue = kl::tostring(value);

        int write_len = ascvalue.length();
        if (write_len > dai->width)
            write_len = dai->width;

        memcpy(m_rowptr + dai->offset, ascvalue.c_str(), write_len);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    return true;
}

bool TtbRow::putDouble(int column_ordinal,
                       double value)
{
    TtbField* dai = &m_table->m_fields[column_ordinal];

    if (dai->width < 1)
        return false;

    if (dai->ttb_type == TtbTable::typeNumeric)
    {
        dbl2decstr((char*)m_rowptr + dai->offset,
                   value,
                   dai->width,
                   dai->scale);
    }
     else if (dai->ttb_type == TtbTable::typeDouble)
    {
        // FIXME: this will only work on little-endian (intel) processors
        memcpy(m_rowptr + dai->offset, &value, sizeof(double));
    }
     else if (dai->ttb_type == TtbTable::typeInteger)
    {
        int2buf(m_rowptr + dai->offset, (int)value);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    return true;
}

bool TtbRow::putInteger(int column_ordinal,
                        int value)
{
    TtbField* dai = &m_table->m_fields[column_ordinal];


    if (dai->ttb_type == TtbTable::typeInteger)
    {
        // set data
        unsigned char* ptr = m_rowptr+dai->offset;
        unsigned int v = (unsigned int)value;
        *(ptr)   = (v) & 0xff;
        *(ptr+1) = (v >> 8) & 0xff;
        *(ptr+2) = (v >> 16) & 0xff;
        *(ptr+3) = (v >> 24) & 0xff;
    }
     else if (dai->ttb_type == TtbTable::typeNumeric)
    {
        dbl2decstr((char*)m_rowptr + dai->offset,
                   value,
                   dai->width,
                   dai->scale);
    }
     else if (dai->ttb_type == TtbTable::typeDouble)
    {
        // FIXME: this will only work on little-endian (intel) processors
        double d = value;
        memcpy(m_rowptr + dai->offset, &d, sizeof(double));
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    return true;

}

bool TtbRow::putBoolean(int column_ordinal,
                        bool value)
{
    TtbField* dai = &m_table->m_fields[column_ordinal];

    // set data
    *(m_rowptr + dai->offset) = (value ? 'T' : 'F');

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    return true;

}

bool TtbRow::putDateTime(int column_ordinal,
                         xd::datetime_t value)
{
    TtbField* dai = &m_table->m_fields[column_ordinal];

    // set data
    if (dai->ttb_type == TtbTable::typeDate)
    {
        unsigned int julian_day = (unsigned int)(value >> 32);

        int2buf(m_rowptr+dai->offset, julian_day);
    }
     else if (dai->ttb_type == TtbTable::typeDateTime)
    {
        unsigned int julian_day = (unsigned int)(value >> 32);
        unsigned int time_stamp = (unsigned int)(value & 0xffffffff);

        int2buf(m_rowptr+dai->offset, julian_day);
        int2buf(m_rowptr+dai->offset+4, time_stamp);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    return true;

}


bool TtbRow::putNull(int column_ordinal)
{
    TtbField* dai = &m_table->m_fields[column_ordinal];

    if (!dai->nulls_allowed)
        return false;

    *(m_rowptr + dai->offset - 1) |= 0x01;

    return true;
}


