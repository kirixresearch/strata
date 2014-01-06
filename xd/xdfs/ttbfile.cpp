/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <algorithm>
#include <ctime>
#include <xcm/xcm.h>
#include <kl/file.h>
#include <kl/math.h>
#include <xd/xd.h>
#include "ttbfile.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
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
// offset   20:   (uint64)  row count
// offset   28:   (uint64)  last structure modify time
// offset   36:   (uint128) GUID for this file; 0 in older file versions
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
//      Version 2, however, stores field names in UNICODE,
//      using the UCS-2 (16-bit) encoding





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
    m_map_file = NULL;
    memset(m_guid, 0, 16);
}

TtbTable::~TtbTable()
{
    close();
}

bool TtbTable::create(const std::wstring& filename, xd::IStructure* structure)
{
    xd::IColumnInfoPtr colinfo;
    int column_count = structure->getColumnCount();
    int i;
    int col_type;
    int col_width;
    int col_scale;
    bool nulls_allowed;
    unsigned char col_flags;

    // check field widths and scales
    for (i = 0; i < column_count; ++i)
    {
        colinfo = structure->getColumnInfoByIdx(i);

        col_type = colinfo->getType();
        col_width = colinfo->getWidth();
        col_scale = colinfo->getScale();

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


    // make sure a map file does NOT exist
    std::wstring map_filename = kl::beforeLast(filename, L'.');
    map_filename += L".map";
    if (xf_get_file_exist(map_filename))
    {
        xf_remove(map_filename);
    }


    int field_arr_len = column_count * ttb_column_descriptor_len;
    unsigned char* header = new unsigned char[ttb_header_len];
    unsigned char* flds = new unsigned char[field_arr_len];

    // create native format file

    memset(header, 0, ttb_header_len);
    memset(flds, 0, field_arr_len);

    // fill out field array

    unsigned char* entry_ptr;
    unsigned int offset;

    offset = 0;
    entry_ptr = flds;


    int cols_written = 0;

    for (i = 0; i < column_count; ++i)
    {
        colinfo = structure->getColumnInfoByIdx(i);
        
        if (colinfo->getCalculated())
            continue;

        cols_written++;

        col_type = colinfo->getType();
        col_width = colinfo->getWidth();
        col_scale = colinfo->getScale();
        nulls_allowed = colinfo->getNullsAllowed();

        switch (col_type)
        {
            case xd::typeDate:     col_width = 4; col_scale = 0; break;
            case xd::typeInteger:  col_width = 4; col_scale = 0; break;
            case xd::typeDouble:   col_width = 8; break;
            case xd::typeBoolean:  col_width = 1; col_scale = 0; break;
            case xd::typeDateTime: col_width = 8; col_scale = 0; break;
            default:                  break;
        };
        
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
        kl::wstring2ucsle(entry_ptr+64, colinfo->getName(), 80);

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
    int2buf(header+4, 2);   // we now write files with
                            // version 2 (UNICODE) field names

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
    XCM_AUTO_LOCK(m_object_mutex);

    // generate map file name
    m_map_filename = kl::beforeLast(filename, L'.');
    m_map_filename += L".map";

    // open table file
    m_filename = filename;
    m_file = xf_open(m_filename, xfOpen, xfReadWrite, xfShareReadWrite);
    if (m_file == NULL)
        return false;

    // open up the map file, if it exists
    if (xf_get_file_exist(m_map_filename))
    {
        m_map_file = new BitmapFile;
        if (!m_map_file->open(m_map_filename))
        {
            delete m_map_file;
            m_map_file = NULL;
        }
    }


    // read native-format file header
    if (!xf_seek(m_file, 0, xfSeekSet))
        return false;

    unsigned char header[ttb_header_len];
    unsigned char* flds;

    if (xf_read(m_file, header, ttb_header_len, 1) == 0)
    {
        xf_close(m_file);
        m_file = NULL;
        return false;
    }

    unsigned int signature, version;
    signature = buf2int(header);
    version = buf2int(header+4);
    if (signature != 0xddaa2299 || version > 2)
    {
        xf_close(m_file);
        m_file = NULL;
        return false;
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


    // check file version
    if (version == 1)
        return false; // no longer supported

    // calculate the actual physical row count from the file size
    xf_seek(m_file, 0, xfSeekEnd);
    xd::rowpos_t file_size = xf_get_file_pos(m_file);
    xd::rowpos_t row_width = m_row_width;
    file_size -= m_data_offset;
    xd::rowpos_t real_phys_row_count = (file_size/m_row_width);

    // compare that to the stats
    if (real_phys_row_count != m_phys_row_count)
    {
        m_phys_row_count = real_phys_row_count;
    }

    if (file_size % m_row_width != 0)
    {
        // the file's size is not a multiple of the row width.
        // This means that somewhere the table is corrupt
    }

    if (!xf_seek(m_file, ttb_header_len, xfSeekSet))
    {
        xf_close(m_file);
        m_file = NULL;
        return false;
    }

    int i, column_count;
    column_count = buf2int(header+16);

    flds = new unsigned char[column_count * ttb_column_descriptor_len];
    
    if (xf_read(m_file,
                flds,
                column_count * ttb_column_descriptor_len,
                1) != 1)
    {
        delete[] flds;
        xf_close(m_file);
        m_file = NULL;
        return false;
    }

    unsigned char* fld = flds;
    std::wstring col_name;
    TtbField fldinfo;
    for (i = 0; i < column_count; ++i)
    {
        ColumnInfo* col = new ColumnInfo;

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
    return true;
}

bool TtbTable::reopen(bool exclusive)
{
    XCM_AUTO_LOCK(m_object_mutex);

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
    XCM_AUTO_LOCK(m_object_mutex);

    if (m_map_file)
    {
        m_map_file->close();
        delete m_map_file;
        m_map_file = NULL;
    }

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
                         bool direction,
                         bool include_deleted)
{
    XCM_AUTO_LOCK(m_object_mutex);

    BitmapFileScroller* bfs = NULL;

    if (m_map_file && !include_deleted)
    {
        bfs = m_map_file->createScroller();
    }

    if (bfs)
    {
        // if we are starting on the first row, we need to
        // find it, as it might be deleted
        if (start_row == 1)
        {
            start_row = 0;
            bfs->findNext(&start_row, false);
            start_row++;
        }
    }

    if (skip != 0)
    {
        start_row = _findNextRowPos(bfs, start_row, skip);
        if (start_row == 0)
        {
            // eof or bof condition
            delete bfs;
            return 0;
        }
    }

    int rowposarr_idx;
    int read_count = 0;

    if (direction)
    {
        rowposarr_idx = 0;

        xd::rowpos_t rowpos = start_row;

        while (1)
        {
            if (rowpos > m_phys_row_count)
            {
                // at the end of the file
                break;
            }

            rowpos_arr[rowposarr_idx] = rowpos;
            read_count++;
            rowposarr_idx++;

            if (rowposarr_idx >= row_count)
            {
                // filled the buffer; done.
                break;
            }

            rowpos = _findNextRowPos(bfs, rowpos, 1);
            if (rowpos == 0)
            {
                // hit eof
                break;
            }
        }
    }
     else
    {
        rowposarr_idx = row_count-1;

        xd::rowpos_t rowpos = start_row;

        while (1)
        {
            if (rowpos == 0)
            {
                // hit bof
                break;
            }

            rowpos_arr[rowposarr_idx] = rowpos;
            read_count++;
            rowposarr_idx--;

            if (rowposarr_idx < 0)
            {
                // filled the buffer; done.
                break;
            }

            rowpos = _findNextRowPos(bfs, rowpos, -1);
        }

        if (rowposarr_idx >= 0)
        {
            // shift rows up
            rowposarr_idx++;

            for (int i = 0; i < read_count; ++i)
            {
                rowpos_arr[i] = rowpos_arr[i + rowposarr_idx];
            }
        }
    }

    // if we haven't read anything, return 0
    if (read_count == 0)
    {
        if (bfs)
        {
            delete bfs;
        }

        return 0;
    }

    // now read in the buffers, in contiguous chunks (if possible)

    int chunk_offset = 0;
    int chunk_len = 1;

    int arr_offset = 1;
    unsigned char* buf_offset = buf;

    while (1)
    {
        if (arr_offset == read_count)
        {
            break;
        }

        if (rowpos_arr[arr_offset] == rowpos_arr[arr_offset-1]+1)
        {
            ++chunk_len;
        }
         else
        {
            // read chunk
            unsigned long long offset = rowpos_arr[chunk_offset]-1;
            offset *= m_row_width;
            offset += m_data_offset;
            xf_seek(m_file, offset, xfSeekSet);
            xf_read(m_file, buf_offset, m_row_width, chunk_len);

            buf_offset += (chunk_len*m_row_width);
            chunk_offset = arr_offset;
            chunk_len = 1;
        }

        ++arr_offset;
    }
    
    // read in last chunk
    if (chunk_len)
    {
        unsigned long long offset = rowpos_arr[chunk_offset]-1;
        offset *= m_row_width;
        offset += m_data_offset;
        xf_seek(m_file, offset, xfSeekSet);
        xf_read(m_file, buf_offset, m_row_width, chunk_len);
    }

    delete bfs;

    return read_count;
}

bool TtbTable::getRow(xd::rowpos_t row, unsigned char* buf)
{
    XCM_AUTO_LOCK(m_object_mutex);

    xf_off_t pos;
    pos = row-1;
    pos *= m_row_width;
    pos += m_data_offset;

    xf_seek(m_file, pos, xfSeekSet);
    return (xf_read(m_file, buf, m_row_width, 1) == 1) ? true : false;
}


xd::rowpos_t TtbTable::_findNextRowPos(BitmapFileScroller* bfs,
                                             xd::rowpos_t offset,
                                             int delta)
{
    if (delta == 0)
        return offset;

    if (!bfs)
    {
        // if there are no deleted rows, thus no map file, we
        // can determine the next rows by arithmatic
        if (delta < 0)
        {
            if ((-delta) > offset)
                return 0;
        }

        if (offset + delta > m_phys_row_count)
            return 0;

        return offset + delta;
    }
     else
    {
        if (delta > 0)
        {
            xd::rowpos_t row = offset;

            // map files have 0-based offsets
            row--;

            while (delta > 0)
            {
                --delta;

                row++;
                if (!bfs->findNext(&row, false))
                {
                    // the end of the bitmap has been reached, but this does
                    // not necessarily mean that the end of the data file has
                    // been reached. There may be more records in the data
                    // file than there are in the bitmap file

                    // restore 1-based offset
                    row++;

                    // add remaining rows
                    row += delta;

                    // if we are beyond the number of records in
                    // the data file, return eof
                    if (row > m_phys_row_count)
                    {
                        return 0;
                    }
                     else
                    {
                        return row;
                    }
                }
            }

            row++;

            if (row > m_phys_row_count)
                return 0;

            return row;
        }
         else
        {
            delta = -delta;

            xd::rowpos_t row = offset;

            // map files have 0-based offsets
            row--;

            while (delta > 0)
            {
                --delta;

                row--;
                if (!bfs->findPrev(&row, false))
                {
                    // we are in a part of the map file that doesn't exist.
                    // This does not necessarily mean that the end/beginning
                    // of the data file has been reached.

                    // restore 1-based offset
                    row++;

                    // subtract remaining rows
                    row -= delta;

                    // if we are beyond the number of records in
                    // the data file, return eof
                    if (row <= 0)
                    {
                        return 0;
                    }
                     else
                    {
                        return row;
                    }
                }
            }
            
            row++;

            return row;
        }

    }

    return 0;
}

int TtbTable::appendRows(unsigned char* buf, int row_count)
{
    XCM_AUTO_LOCK(m_object_mutex);

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
    XCM_AUTO_LOCK(m_object_mutex);

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

    /*
    // send notification
    std::vector<ITableEvents*>::iterator it;
    xd::rowid_t rowid = rowidCreate(m_ordinal, row);
    for (it = m_event_handlers.begin(); it != m_event_handlers.end(); ++it)
    {
        (*it)->onTableRowUpdated(rowid);
    }
    */

    return true;
}


bool TtbTable::writeColumnInfo(int col_idx,
                               const std::wstring& col_name,
                               int type,
                               int width,
                               int scale)
{
    XCM_AUTO_LOCK(m_object_mutex);

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
    XCM_AUTO_LOCK(m_object_mutex);

    unsigned char sig[8];
    unsigned long long i = 0;

    if (!xf_seek(m_file, 28, xfSeekSet))
        return 0;

    if (xf_read(m_file, sig, 8, 1) != 1)
        return 0;

    return (unsigned long long)bufToInt64(sig);
}


bool TtbTable::setStructureModified()
{
    XCM_AUTO_LOCK(m_object_mutex);

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

xd::IStructurePtr TtbTable::getStructure()
{
    Structure* structure = new Structure;

    std::vector<TtbField>::iterator it, it_end = m_fields.end();
    int counter = 0;
    for (it = m_fields.begin(); it != it_end; ++it)
    {
        ColumnInfo* col = new ColumnInfo;
        
        col->setName(it->name);
        col->setType(convertType_ttb2xd(it->ttb_type));
        col->setWidth(it->width);
        col->setScale(it->scale);
        col->setOffset(it->offset);
        col->setCalculated(false);
        col->setColumnOrdinal(counter++);
        col->setTableOrdinal(0);
        col->setNullsAllowed(it->nulls_allowed);

        structure->addColumn(static_cast<xd::IColumnInfo*>(col));
    }

    return static_cast<xd::IStructure*>(structure);
}


bool TtbTable::isRowDeleted(xd::rowpos_t row)
{
    if (m_map_file == NULL)
        return false;
    BitmapFileScroller* scroller = m_map_file->createScroller();
    bool deleted = scroller->getState(row-1);
    delete scroller;
    return deleted;
}


xd::rowpos_t TtbTable::getRowCount(xd::rowpos_t* deleted_row_count)
{
    if (deleted_row_count)
    {
        if (m_map_file)
        {
            *deleted_row_count = m_map_file->getSetBitCount();
        }
         else
        {
            *deleted_row_count = 0;
        }
    }

    // lock the header
    if (!xf_trylock(m_file, 0, ttb_header_len, 10000))
        return m_phys_row_count;

    unsigned char buf[8];
    if (!xf_seek(m_file, 20, xfSeekSet))
    {
        xf_unlock(m_file, 0, ttb_header_len);
        return false;
    }

    if (8 != xf_read(m_file, buf, 1, 8))
    {
        xf_unlock(m_file, 0, ttb_header_len);
        XCM_AUTO_LOCK(m_object_mutex);
        return m_phys_row_count;
    }

    // unlock header

    xf_unlock(m_file, 0, ttb_header_len);

    {
        XCM_AUTO_LOCK(m_object_mutex);
        m_phys_row_count = bufToInt64(buf);
        return m_phys_row_count;
    }
}



xd::rowpos_t TtbTable::recalcPhysRowCount()
{
    XCM_AUTO_LOCK(m_object_mutex);

    // calculate the actual physical row count from the file size
    xf_seek(m_file, 0, xfSeekEnd);
    xd::rowpos_t file_size = xf_get_file_pos(m_file);
    xd::rowpos_t row_width = m_row_width;
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

        /*
        // fire an event
        std::vector<ITableEvents*>::iterator it;
        for (it = m_event_handlers.begin(); it != m_event_handlers.end(); ++it)
        {
            (*it)->onTableRowCountUpdated();
        }
        */
    }

    return real_phys_row_count;
}



std::wstring TtbTable::getFilename()
{
    return m_filename;
}


std::wstring TtbTable::getMapFilename()
{
    return m_map_filename;
}

bool TtbTable::restoreDeleted()
{
    if (!m_map_file)
        return true;
    
    // close the map file
    m_map_file->close();
    delete m_map_file;
    m_map_file = NULL;

    if (!xf_get_file_exist(m_map_filename))
        return true;
       
    if (!xf_remove(m_map_filename))
        return false;

/*
    // fire an event
    std::vector<ITableEvents*>::iterator it;
    for (it = m_event_handlers.begin(); it != m_event_handlers.end(); ++it)
    {
        (*it)->onTableRowCountUpdated();
    }
*/

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





// TtbRowDeleter class implementation

TtbRowDeleter::TtbRowDeleter(TtbTable* table)
{
/*
    m_table = table;
    m_table->ref();

    m_map_scroller = NULL;
    */
}

TtbRowDeleter::~TtbRowDeleter()
{
    //m_table->unref();
}

void TtbRowDeleter::startDelete()
{
    // FIXME: need a mutex here for mutual exclusive access
    // to m_table's m_map_file member

    // make sure that the map file exists
    if (!m_table->m_map_file)
    {
        m_table->m_map_file = new BitmapFile;
        if (!m_table->m_map_file->open(m_table->m_map_filename))
        {
            delete m_table->m_map_file;
            m_table->m_map_file = NULL;
        }
    }
    
    m_map_scroller = m_table->m_map_file->createScroller();
    m_map_scroller->startModify();

    // force map file to include full length of table
    m_map_scroller->setState(m_table->m_phys_row_count+100, false);
}

bool TtbRowDeleter::deleteRow(const xd::rowid_t& rowid)
{
    if (!m_map_scroller)
        return false;

    /*
    if (rowidGetTableOrd(rowid) != m_table->m_ordinal)
        return false;
    */

    // add row to delete map
    //m_map_scroller->setState(rowidGetRowPos(rowid)-1, true);

    /*
    // fire an event
    std::vector<ITableEvents*>::iterator it;
    for (it = m_table->m_event_handlers.begin(); it != m_table->m_event_handlers.end(); ++it)
    {
        (*it)->onTableRowDeleted(rowid);
    }
    */

    return true;
}

void TtbRowDeleter::finishDelete()
{
    m_map_scroller->endModify();
    delete m_map_scroller;
    m_map_scroller = NULL;

    /*
    // fire an event
    std::vector<ITableEvents*>::iterator it;
    for (it = m_table->m_event_handlers.begin(); it != m_table->m_event_handlers.end(); ++it)
    {
        (*it)->onTableRowCountUpdated();
    }
*/
}











// ------------------ FILE FORMAT: xdnative Native Table --------------------
//
// -- file header --
// offset   00:   (uint32) signature 0xa2d022e4;
// offset   04:   (uint32) version (== 01)
// offset   08:   (uint32) data begin offset
// offset   12:   (uint64) set-bit count
// (data begin offset): bitmap



const int bitmap_file_page_size = 512;
const unsigned char bit_values[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };



BitmapFileScroller::BitmapFileScroller()
{
    m_buf = new unsigned char[bitmap_file_page_size];
    m_data_offset = 0;
    m_buf_offset = (unsigned long long)-1;
    m_file = NULL;
    m_bmp_file = NULL;
    m_buf_valid = false;

    m_locked = false;
    m_dirty = false;
    m_modify_set_bit_count = 0;
}


BitmapFileScroller::~BitmapFileScroller()
{
    if (m_buf)
    {
        delete[] m_buf;
    }
}

bool BitmapFileScroller::_flush()
{
    if (m_dirty)
    {
        xf_seek(m_file, m_buf_offset+m_data_offset, xfSeekSet);
        xf_write(m_file, m_buf, bitmap_file_page_size, 1);
        m_dirty = false;
    }

    if (m_locked)
    {
        xf_unlock(m_file,
                  m_buf_offset + m_data_offset,
                  bitmap_file_page_size);
        m_locked = false;
    }

    if (m_modify_set_bit_count != 0)
    {
        unsigned char buf[20];

        if (xf_trylock(m_file, 0, bitmap_file_page_size, 10000))
        {
            xf_seek(m_file, 0, xfSeekSet);
            xf_read(m_file, buf, 20, 1);

            long long count = bufToInt64(buf+12);
            count += m_modify_set_bit_count;
            int64ToBuf(buf+12, count);

            xf_seek(m_file, 0, xfSeekSet);
            xf_write(m_file, buf, 20, 1);

            m_modify_set_bit_count = 0;

            xf_unlock(m_file, 0, bitmap_file_page_size);
        }
    }

    return true;
}

bool BitmapFileScroller::_goBlock(unsigned long long block_number,
                                  bool lock,
                                  bool pad)
{
    // flush last block
    _flush();

    xf_off_t new_offset = block_number * bitmap_file_page_size;

    if (!xf_seek(m_file, new_offset+m_data_offset, xfSeekSet))
        return false;

    if (lock)
    {
        if (!xf_trylock(m_file,
                        new_offset+m_data_offset,
                        bitmap_file_page_size,
                        10000))
        {
            // could not get lock
            return false;
        }
    }

    if (xf_read(m_file, m_buf, bitmap_file_page_size, 1) != 1)
    {
        memset(m_buf, 0, bitmap_file_page_size);

        if (!pad)
        {
            m_buf_valid = false;
            if (lock)
            {
                xf_unlock(m_file, new_offset+m_data_offset, bitmap_file_page_size);
            }
            return false;
        }

        // we are beyond the end, so we must set
        // the intervening bytes to zero

        if (!xf_seek(m_file, 0, xfSeekEnd))
            return false;

        long long cur_block = (xf_get_file_pos(m_file)-m_data_offset) / bitmap_file_page_size;
        cur_block = block_number - cur_block + 1;
        while (cur_block)
        {
            xf_write(m_file, m_buf, bitmap_file_page_size, 1);
            --cur_block;
        }

        // now reseek to position the caller wanted, and try
        // to read the buffer in again
        if (!xf_seek(m_file, new_offset+m_data_offset, xfSeekSet))
            return false;

        if (xf_read(m_file, m_buf, bitmap_file_page_size, 1) != 1)
            return false;

    }

    m_buf_offset = new_offset;
    m_buf_valid = true;

    if (lock)
    {
        m_locked = true;
    }

    return true;
}


bool BitmapFileScroller::getState(unsigned long long offset)
{
    unsigned int byte_offset = (unsigned int)(offset/8);
    unsigned int bit_offset = (unsigned int)(offset % 8);

    if (m_buf_offset == (unsigned long long)-1 ||
        byte_offset < m_buf_offset ||
        byte_offset >= m_buf_offset+bitmap_file_page_size)
    {
        if (!_goBlock(byte_offset/bitmap_file_page_size, false, false))
            return false;
        _flush();
    }

    return (m_buf[byte_offset-m_buf_offset] & bit_values[bit_offset]) ? true : false;
}

bool BitmapFileScroller::findPrev(unsigned long long* offset,
                                  bool state)
{
    unsigned int byte_offset = (unsigned int)((*offset)/8);
    unsigned int bit_offset = (unsigned int)((*offset) % 8);
    unsigned int byte_check = state ?  0x00000000 : 0xffffffff;

    while (1)
    {
        if (m_buf_offset == (unsigned long long)-1 ||
            byte_offset < m_buf_offset ||
            byte_offset >= m_buf_offset+bitmap_file_page_size)
        {
            if (!_goBlock(byte_offset/bitmap_file_page_size, false, false))
                return false;
            _flush();
        }
        
        // looking for a bit that is set (backward direction)
        while (m_buf[byte_offset-m_buf_offset] == byte_check)
        {
            if (byte_offset == m_buf_offset)
            {
                // go to the previous block
                break;
            }

            byte_offset--;
            bit_offset = 7;
        }

        while (1)
        {
            if ((m_buf[byte_offset-m_buf_offset] & bit_values[bit_offset] ? true : false) == state)
            {
                *offset = (byte_offset*8) + bit_offset;
                return true;
            }

            if (bit_offset == 0)
            {
                if (byte_offset == 0)
                {
                    *offset = 0;
                    // bof condition
                    return false;
                }

                byte_offset--;
                bit_offset = 7;
                if (byte_offset < m_buf_offset)
                {
                    // go to the previous block
                    break;
                }
            }
             else
            {
                bit_offset--;
            }
        }
    }

    return false;
}

bool BitmapFileScroller::findNext(unsigned long long* offset,
                                  bool state)
{
    unsigned int byte_offset = (unsigned int)((*offset)/8);
    unsigned int bit_offset = (unsigned int)((*offset) % 8);
    unsigned int byte_check = state ?  0x00000000 : 0xffffffff;

    while (1)
    {
        if (m_buf_offset == (unsigned long long)-1 ||
            byte_offset < m_buf_offset ||
            byte_offset >= m_buf_offset+bitmap_file_page_size)
        {
            if (!_goBlock(byte_offset/bitmap_file_page_size, false, false))
                return false;
            _flush();
        }
        
        // looking for a bit that is set (forward direction)
        while (m_buf[byte_offset-m_buf_offset] == byte_check)
        {
            byte_offset++;
            bit_offset = 0;
            if (byte_offset >= m_buf_offset+bitmap_file_page_size)
            {
                break;
            }
        }

        while (1)
        {
            if (byte_offset >= m_buf_offset+bitmap_file_page_size)
            {
                // go to the next block
                break;
            }

            if ((m_buf[byte_offset-m_buf_offset] & bit_values[bit_offset] ? true : false) == state)
            {
                *offset = (byte_offset*8) + bit_offset;
                return true;
            }

            bit_offset++;
            if (bit_offset == 8)
            {
                byte_offset++;
                bit_offset = 0;
            }
        }
    }

    return false;
}


void BitmapFileScroller::startModify()
{
    m_modify_set_bit_count = 0;
}

void BitmapFileScroller::endModify()
{
    _flush();
}


void BitmapFileScroller::setState(unsigned long long offset,
                                  bool state)
{
    unsigned byte_offset = (unsigned int)(offset/8);
    unsigned bit_offset = (unsigned int)(offset % 8);

    if (m_buf_offset == (unsigned long long)-1 ||
        byte_offset < m_buf_offset ||
        byte_offset >= m_buf_offset+bitmap_file_page_size)
    {
        _goBlock(byte_offset/bitmap_file_page_size, true, true);
    }

    bool cur_state = (m_buf[byte_offset-m_buf_offset] & bit_values[bit_offset]) ? true : false;
    if (cur_state == state)
        return;

    if (state)
    {
        m_modify_set_bit_count++;
        m_buf[byte_offset-m_buf_offset] |= bit_values[bit_offset];
    }
     else
    {
        m_modify_set_bit_count--;
        m_buf[byte_offset-m_buf_offset] &= (~bit_values[bit_offset]);
    }
    m_dirty = true;
}





BitmapFile::BitmapFile()
{
    m_file = NULL;
    m_data_offset = 0;
}


BitmapFile::~BitmapFile()
{
    if (m_file)
    {
        xf_close(m_file);
    }
}


bool BitmapFile::open(const std::wstring& filename)
{
    bool create = false;
    
    if (!xf_get_file_exist(filename))
        create = true;
    
    m_file = xf_open(filename,
                     xfOpenCreateIfNotExist,
                     xfReadWrite,
                     xfShareReadWrite);
    
    if (m_file == NULL)
    {
        return false;
    }

    if (!create)
    {                 
        unsigned char buf[20];
        if (xf_read(m_file, buf, 20, 1) != 1)
        {
            return false;
        }

        unsigned int signature, version;
        signature = buf2int(buf);
        version = buf2int(buf+4);
        m_data_offset = buf2int(buf+8);
    
        if (signature != 0xa2d022e4 || version != 1)
        {
            return false;
        }
    }
     else
    {
        unsigned char buf[bitmap_file_page_size];

        m_data_offset = bitmap_file_page_size;

        memset(buf, 0, bitmap_file_page_size);
        int2buf(buf, 0xa2d022e4);     // signature
        int2buf(buf+4, 1);            // version
        int2buf(buf+8, (unsigned int)m_data_offset); // data begin
        int2buf(buf+12, 0);            // set-bit count (lower 32 bits)
        int2buf(buf+16, 0);            // set-bit count (upper 32 bits)

        if (xf_write(m_file, buf, bitmap_file_page_size, 1) != 1)
        {
            return false;
        }
    }

    return true;
}

bool BitmapFile::close()
{
    if (m_file == NULL)
    {
        return false;
    }

    xf_close(m_file);
    m_file = NULL;

    return true;
}

bool BitmapFile::isOpen()
{
    return (m_file != NULL) ? true : false;
}


unsigned long long BitmapFile::getSetBitCount()
{
    if (!xf_trylock(m_file, 0, bitmap_file_page_size, 10000))
        return 0;

    unsigned char buf[8];
    unsigned long long result;

    memset(buf, 0, 8);
    xf_seek(m_file, 12, xfSeekSet);
    xf_read(m_file, buf, 8, 1);

    result = bufToInt64(buf);

    xf_unlock(m_file, 0, bitmap_file_page_size);

    return result;
}


BitmapFileScroller* BitmapFile::createScroller()
{
    BitmapFileScroller* bfs = new BitmapFileScroller;
    bfs->m_bmp_file = this;
    bfs->m_file = m_file;
    bfs->m_data_offset = m_data_offset;
    return bfs;
}





