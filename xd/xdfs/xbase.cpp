/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-19
 *
 */


#include <xd/xd.h>
#include "../xdcommon/filestream.h"
#include <kl/file.h>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/math.h>
#include <kl/hex.h>
#include <ctime>
#include "xbase.h"

#include "../xdcommon/util.h"


const int XBASE_BUFFER_ROWCOUNT = 1000;    // number of rows in each buffer


static XbaseField EMPTY_XBASE_FIELD = XbaseField();
static XbaseDate EMPTY_XBASE_DATE = XbaseDate();



void checkXbaseField(XbaseField* f)
{
    size_t field_width = 0;
    size_t field_scale = 0;
    switch (f->type)
    {
        default:
        case 'C':
        {
            // character type
            field_width = (f->width > 254) ? 254 : f->width;
            field_scale = 0;
            break;
        }
        case 'N':
        {
            // numeric type
            field_width = (f->width > 18) ? 18 : f->width;
            field_scale = (f->scale > 12) ? 12 : f->scale;
            break;
        }
        case 'F':
        {
            // float type
            field_width = 20;
            field_scale = (f->scale > 18) ? 18 : f->scale;
            break;
        }
        case 'Y':
        {
            // currency type
            break;
        }
        case 'B':
        {
            // double type
            field_width = 8;
            field_scale = (f->scale > 12) ? 12 : f->scale;
            break;
        }
        case 'D':
        {
            // date type
            field_width = 8;
            field_scale = 0;
            break;
        }
        case 'T':
        {
            // datetime type
            field_width = 8;
            field_scale = 0;
            break;
        }
        case 'L':
        {
            // boolean type
            field_width = 1;
            field_scale = 0;
            break;
        }
        case 'I':
        {
            // integer type
            field_width = 4;
            field_scale = 0;
            break;
        }
    }
    
    f->width = field_width;
    f->scale = field_scale;
    
    // make sure there are no spaces in the field name
    std::string::iterator it;
    for (it = f->name.begin(); it != f->name.end(); ++it)
    {
        if (*it == L' ')
            *it = L'_';
    }
}







XbaseFile::XbaseFile()
{
    m_stream = NULL;
    
    m_buf = NULL;
    m_buf_rows = 0;
    m_buf_firstrow = 0;
    m_buf_maxrows = 0;

    m_currow_ptr = NULL;
    m_currow_num = 0;

    m_header_len = 0;
    m_row_count = 0;
    m_row_width = 0;
    
    m_read_ahead = XBASE_BUFFER_ROWCOUNT;
    m_inserting = false;
}

XbaseFile::~XbaseFile()
{
    close();
}


bool XbaseFile::open(xd::IStream* stream)
{
    m_stream = stream;
    m_stream->ref();

    // read xbase header
    if (!m_stream->seek(0))
        return false;

    unsigned char buf[32];

    unsigned long read_bytes = 0;
    m_stream->read(buf, 32, &read_bytes);
    if (read_bytes != 32)
    {
        m_stream->unref();
        m_stream = NULL;
        return false;
    }

    m_row_count = buf[4] + (buf[5]*256) + (buf[6]*65536) + (buf[7] * 16777216);
    m_header_len = buf[8] + (buf[9]*256);
    m_row_width = buf[10] + (buf[11]*256);

    if (m_row_width == 0)
    {
        m_stream->unref();
        m_stream = NULL;
        return false;
    }

    bool found_terminator = false;
    size_t temp_offset = 32;
    size_t row_offset = 1;              // start with 1 because of delete byte
    size_t col_ordinal = 0;
    while (temp_offset < m_header_len)
    {
        m_stream->read(buf, 32, &read_bytes);

        if (read_bytes != 32)
        {
            // if we just created the file with no records, the last read
            // of the header will only be 1 byte (not 32), which is fine
            // as long as that byte is a terminator character
            if (buf[0] != 0x0d)
            {
                m_stream->unref();
                m_stream = NULL;
                return false;
            }
        }

        if (buf[0] == 0x0d)
        {
            // found terminator, we are done here
            found_terminator = true;
            break;
        }

        XbaseField field;
        field.name = (char*)buf;
        field.type = buf[11];
        field.width = buf[16];
        field.scale = buf[17];
        field.offset = row_offset;
        field.ordinal = col_ordinal++;

        kl::makeUpper(field.name);

        row_offset += int(buf[16]);
        temp_offset += 32;

        m_fields.push_back(field);
    }

    // if we failed to find the column list terminator 0x0d, bail out
    if (!found_terminator)
    {
        return false;
    }

    return true;
}

bool XbaseFile::open(const std::wstring& path)
{
    if (path.substr(0, 12) == L"streamptr://")
    {
        unsigned long l = (unsigned long)kl::hexToUint64(path.substr(12));
        xd::IStream* ptr = (xd::IStream*)l;
        return open(ptr);
    }

    FileStream* f = new FileStream;
    f->ref();
    if (!f->open(path))
    {
        f->unref();
        return false;
    }

    if (!open(f))
    {
        f->unref();
        return false;
    }

    f->unref();
    return true;
}

bool XbaseFile::create(const std::wstring& filename, const std::vector<XbaseField>& fields)
{
    if (isOpen())
        close();

    if (xf_get_file_exist(filename))
        xf_remove(filename);

    FileStream* f = new FileStream;
    f->ref();

    if (!f->create(filename))
    {
        f->unref();
        return false;
    }

    m_stream = f;
    
    

    unsigned char header[32];
    unsigned char* flds;
    unsigned long written = 0;

    // fill out field array
    //size_t field_arr_len = (fields.size() * 32)+264;  // Visual FoxPro
    size_t field_arr_len = (fields.size() * 32)+1;      // dBase

    flds = new unsigned char[field_arr_len];
    memset(flds, 0, field_arr_len);

    unsigned char* entry_ptr;
    size_t field_count;
    size_t offset;
    size_t i;

    i = 0;
    offset = 1;
    field_count = fields.size();
    entry_ptr = flds;

    for (i = 0; i < field_count; ++i)
    {
        XbaseField f = fields[i];
        
        // this function makes sure the field conforms to xbase
        // standards (field names less than 10 characters, etc.)
        checkXbaseField(&f);
        
        // field name
        kl::makeUpper(f.name);
        strncpy((char*)entry_ptr, f.name.c_str(), 10);
        entry_ptr[10] = 0;

        // type
        entry_ptr[11] = f.type;

        // offset
        entry_ptr[12] = (unsigned char)(offset & 0x000000ff);
        entry_ptr[13] = (unsigned char)((offset & 0x0000ff00) >> 8);
        entry_ptr[14] = (unsigned char)((offset & 0x00ff0000) >> 16);
        entry_ptr[15] = (unsigned char)((offset & 0xff000000) >> 24);
        offset += f.width;

        // length
        entry_ptr[16] = f.width;

        // scale
        entry_ptr[17] = f.scale;

        // let column store 'null' values
        //entry_ptr[18] = 0x02;

        entry_ptr += 32;
    }

    entry_ptr[0] = 0x0d;    // terminator


    // fill out header values

    memset(header, 0, 32);
    //header[0] = 0x30;     // Visual FoxPro signature
    header[0] = 0x03;       // dBase IV signature

    time_t t;
    time(&t);
    struct tm curtime;
    localtime_r(&t, &curtime);

    header[1] = curtime.tm_year % 100;         // last modify date (year)
    header[2] = curtime.tm_mon+1;              // last modify date (month)
    header[3] = curtime.tm_mday;               // last modify date (day)

    size_t row_count = 0;
    header[4] = (unsigned char)((row_count & 0x000000ff));
    header[5] = (unsigned char)((row_count & 0x0000ff00) >> 8);
    header[6] = (unsigned char)((row_count & 0x00ff0000) >> 16);
    header[7] = (unsigned char)((row_count & 0xff000000) >> 24);

    int header_len = field_arr_len+32;
    header[8] = (header_len & 0x000000ff);      // header length
    header[9] = (header_len & 0x0000ff00) >> 8;

    header[10] = (offset & 0x000000ff);         // row width
    header[11] = (offset & 0x0000ff00) >> 8;

    header[29] = 0x03;                          // (windows ansi codepage)

    // write out the header info
    m_stream->seek(0);


    m_stream->write(header, 32, &written);

    if (written != 32)
    {
        // something went wrong, so close the file,
        // and delete the file we just tried to create
        m_stream->unref();
        m_stream = NULL;
        xf_remove(filename);

        delete[] flds;
        return false;
    }

    // write out field array info
    written = 0;
    m_stream->write(flds, field_arr_len, &written);

    if (written != field_arr_len)
    {
        // something went wrong, so close the file,
        // and delete the file we just tried to create
        m_stream->unref();
        m_stream = NULL;
        xf_remove(filename);
        
        delete[] flds;
        return false;
    }

    delete[] flds;
    
    // go to the end of the file
    m_stream->seek(-1, xd::seekEnd);

    // is there an EOF marker?
    char ch = 0;
    m_stream->read(&ch, 1, NULL);

    if (ch == 0x1a)
    {
        m_stream->seek(-1, xd::seekEnd);
    }
     else
    {
        m_stream->seek(0, xd::seekEnd);
    }
    
    // write out the file terminator to the file
    unsigned char end_char[1];
    end_char[0] = 0x1a;
    m_stream->write(end_char, 1, NULL);

    return true;
}

bool XbaseFile::isOpen()
{
    return m_stream ? true : false;
}

void XbaseFile::close()
{
    if (m_stream)
    {
        m_stream->unref();
        m_stream = NULL;
    }
    
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }
}

size_t XbaseFile::getHeaderLength()
{
    return m_header_len;
}

size_t XbaseFile::getRowWidth()
{
    return m_row_width;
}

size_t XbaseFile::getRowCount()
{
    return m_row_count;
}

std::vector<XbaseField>& XbaseFile::getFields()
{
    return m_fields;
}

const XbaseField& XbaseFile::getFieldInfo(const std::string& name)
{
    std::vector<XbaseField>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals(name, it->name))
            return (*it);
    }
    
    return EMPTY_XBASE_FIELD;
}

size_t XbaseFile::getFieldIdx(const std::string& name)
{
    std::vector<XbaseField>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals(name, it->name))
            return it->ordinal;
    }
    
    return -1;
}

size_t XbaseFile::getFieldCount()
{
    return m_fields.size();
}

unsigned char* XbaseFile::getRowBuffer()
{
    return m_currow_ptr;
}

unsigned char* XbaseFile::getRaw(size_t col_idx)
{
    return m_currow_ptr+m_fields[col_idx].offset;
}

const std::string& XbaseFile::getString(size_t col_idx)
{
    std::string s;
    s.assign((char*)m_currow_ptr+m_fields[col_idx].offset, m_fields[col_idx].width);
    kl::trim(s);
    m_str_result = s;
    return m_str_result;
}

const XbaseDate& XbaseFile::getDateTime(size_t col_idx)
{
    int year, month, day;
        
    if (m_fields[col_idx].type == 'D')
    {
        char buf[5];

        memcpy(buf, m_currow_ptr+m_fields[col_idx].offset, 4);
        buf[4] = 0;
        year = atoi(buf);

        memcpy(buf, m_currow_ptr+m_fields[col_idx].offset+4, 2);
        buf[2] = 0;
        month = atoi(buf);

        memcpy(buf, m_currow_ptr+m_fields[col_idx].offset+6, 2);
        buf[2] = 0;
        day = atoi(buf);

        // some quick checks
        if (year == 0 ||
            (month < 1 || month > 12) ||
            (day < 1 || day > 31))
        {
            m_date_result = EMPTY_XBASE_DATE;
            return m_date_result;
        }

        m_date_result = XbaseDate((unsigned int)year,
                                  (unsigned int)month,
                                  (unsigned int)day);
        return m_date_result;
    }

    if (m_fields[col_idx].type == 'T')
    {
        unsigned char* buf;
        int julian_day;
        int time_stamp;
        int hour, minute, second, millisecond;

        buf = (unsigned char*)(m_currow_ptr+m_fields[col_idx].offset);
        julian_day = (*buf) + (*(buf+1) << 8) + (*(buf+2) << 16) + (*(buf+3) << 24);
        buf += 4;
        time_stamp = (*buf) + (*(buf+1) << 8) + (*(buf+2) << 16) + (*(buf+3) << 24);

        // set the year, month and day
        julianToDate(julian_day, &year, &month, &day);
        
        // set the timestamp
        setTimeStamp(time_stamp, &hour, &minute, &second, &millisecond);

        m_date_result = XbaseDate((unsigned int)year,
                                  (unsigned int)month,
                                  (unsigned int)day,
                                  (unsigned int)hour,
                                  (unsigned int)minute,
                                  (unsigned int)second,
                                  (unsigned int)millisecond);
        return m_date_result;
    }

    // return an empty XbaseDate
    m_date_result = EMPTY_XBASE_DATE;
    return m_date_result;
}

double XbaseFile::getDouble(size_t col_idx)
{
    if (m_fields[col_idx].type == 'N' || m_fields[col_idx].type == 'F')
    {
        char buf[255];
        memcpy(buf, m_currow_ptr+m_fields[col_idx].offset, m_fields[col_idx].width);
        buf[m_fields[col_idx].width] = 0;
        return kl::nolocale_atof(buf);
    }

    if (m_fields[col_idx].type == 'B')
    {
        // FIXME: this will only work on little-endian (intel) processors
        double d;
        memcpy(&d, m_currow_ptr+m_fields[col_idx].offset, sizeof(double));
        return d;
    }

    if (m_fields[col_idx].type == 'Y')
    {
        long long val;
        long long dec;

        memcpy(&val,
               m_currow_ptr+m_fields[col_idx].offset,
               sizeof(long long));

        dec = val % 10000;
        val -= dec;
        val /= 10000;

        double dbl_whole, dbl_dec;
        dbl_dec = (double)dec;
        dbl_dec /= 10000;

        dbl_whole = (double)val;
        dbl_whole += dbl_dec;

        return kl::dblround(dbl_whole, 4);
    }

    return 0.0;
}

int XbaseFile::getInteger(size_t col_idx)
{
    unsigned char* ptr = m_currow_ptr+m_fields[col_idx].offset;
    unsigned int retval;
    retval = *(ptr) + (*(ptr+1) << 8) + (*(ptr+2) << 16) + (*(ptr+3) << 24);
    return (signed int)retval;
}

bool XbaseFile::getBoolean(size_t col_idx)
{
    char c = *(m_currow_ptr+m_fields[col_idx].offset);
    return (c == 'T' || c == 't' || c == 'Y' || c == 'y' ? true : false);
}

bool XbaseFile::isNull(size_t col_idx)
{
    unsigned char* ptr = m_currow_ptr+m_fields[col_idx].offset;
    size_t col_width = m_fields[col_idx].width;
    size_t i = 0;

    switch (m_fields[col_idx].type)
    {
        case 'C':
        {
            return false;   // never treat a character field in Xbase as null
        }

        case 'N':
        {
            for (i = 0; i < col_width; ++i)
            {
                if (*ptr != ' ')
                {
                    return false;
                }

                ++ptr;
            }

            return true;
        }

        case 'B':
        {
            for (i = 0; i < col_width; ++i)
            {
                if (*ptr != 0)
                {
                    return false;
                }

                ++ptr;
            }

            return true;
        }

        case 'F':
        {
            for (i = 0; i < col_width; ++i)
            {
                if (*ptr != ' ')
                {
                    return false;
                }

                ++ptr;
            }

            return true;
        }

        case 'I':
        {
            return false;   // never treat an integer field in Xbase as null
        }

        case 'D':
        {
            for (i = 0; i < col_width; ++i)
            {
                if (*ptr != ' ')
                {
                    return false;
                }

                ++ptr;
            }

            return true;
        }

        case 'T':
        {
            for (i = 0; i < col_width; ++i)
            {
                if (*ptr != 0)
                {
                    return false;
                }

                ++ptr;
            }

            return true;
        }

        case 'L':
        {
            return (*ptr != 'T' && *ptr != 'F' ? true : false);
        }

    }

    return false;
}

bool XbaseFile::putRowBuffer(const unsigned char* value)
{
    unsigned char* pos = m_currow_ptr;
    size_t str_len = strlen((const char*)value);

    // the raw length is longer than our row width, so trim
    // the raw data to fit our row's width
    if (str_len > m_row_width)
    {
        memcpy((char*)pos, value, m_row_width);
        return true;
    }
    
    // the raw length is shorter than our row width, so write out
    // the raw data and then pad the rest of the row with spaces
    memcpy((char*)pos, value, str_len);
    memset((char*)(pos+str_len), ' ', m_row_width-str_len);
    return true;
}

bool XbaseFile::putRaw(size_t col_idx, const unsigned char* value,
                                       size_t length)
{
    unsigned char* pos = m_currow_ptr+m_fields[col_idx].offset;
    size_t field_len = m_fields[col_idx].width;
    
    // if our raw value is longer than the field length, trim it
    if (length > field_len)
        length = field_len;

    memset((char*)pos, ' ', field_len);
    memcpy((char*)pos, value, length);
    return true;
}

bool XbaseFile::putString(size_t col_idx, const std::string& value)
{
    unsigned char* pos = m_currow_ptr+m_fields[col_idx].offset;

    size_t str_len = value.length();
    size_t field_len = m_fields[col_idx].width;
    
    // if our string value is longer than the field length, trim it
    if (str_len > field_len)
        str_len = field_len;
        
    // pad the field with spaces and then insert the data
    memset((char*)pos, ' ', field_len);
    memcpy((char*)pos, value.c_str(), str_len);
    return true;
}

bool XbaseFile::putDateTime(size_t col_idx, const XbaseDate& value)
{
    unsigned char* pos = m_currow_ptr+m_fields[col_idx].offset;

    // handle empty dates
    if (value.isNull())
    {
        *(pos+0) = ' ';
        *(pos+1) = ' ';
        *(pos+2) = ' ';
        *(pos+3) = ' ';

        *(pos+4) = ' ';
        *(pos+5) = ' ';
        *(pos+6) = ' ';
        *(pos+7) = ' ';
        return true;
    }
    
    if (m_fields[col_idx].type == 'T')
    {
        size_t julian_day = dateToJulian(value.year,
                                         value.month,
                                         value.day);
        size_t time_stamp = getTimeStamp(value.hour,
                                         value.minute,
                                         value.second,
                                         value.millisecond);

        *(pos+0) = (unsigned char)((julian_day & 0x000000ff));
        *(pos+1) = (unsigned char)((julian_day & 0x0000ff00) >> 8);
        *(pos+2) = (unsigned char)((julian_day & 0x00ff0000) >> 16);
        *(pos+3) = (unsigned char)((julian_day & 0xff000000) >> 24);

        *(pos+4) = (unsigned char)((time_stamp & 0x000000ff));
        *(pos+5) = (unsigned char)((time_stamp & 0x0000ff00) >> 8);
        *(pos+6) = (unsigned char)((time_stamp & 0x00ff0000) >> 16);
        *(pos+7) = (unsigned char)((time_stamp & 0xff000000) >> 24);
        return true;
    }

    if (m_fields[col_idx].type == 'D')
    {
        char buf[32];
        sprintf(buf, "%04d%02d%02d", value.year, value.month, value.day);
        memcpy(pos, buf, 8);
        return true;
    }

    return false;
}

bool XbaseFile::putDouble(size_t col_idx, double value)
{
    unsigned char* pos = m_currow_ptr+m_fields[col_idx].offset;

    if (m_fields[col_idx].type == 'N')
    {
        char buf[255];
        sprintf(buf, "%*.*f",
                  m_fields[col_idx].width,
                  m_fields[col_idx].scale,
                  kl::dblround(value, m_fields[col_idx].scale));

        // convert a euro decimal character to a decimal point
        char* p = buf;
        while (*p)
        {
            if (*p == ',')
                *p = '.';
            ++p;
        }

        memcpy(pos, buf, m_fields[col_idx].width);
        return true;
    }

    if (m_fields[col_idx].type == 'F')
    {
        // FIXME: this will only work on little-endian processors right now
        memcpy(pos, &value, sizeof(double));
        return true;
    }

    if (m_fields[col_idx].type == 'B')
    {
        // FIXME: this will only work on little-endian processors right now
        memcpy(pos, &value, sizeof(double));
        return true;
    }

    return false;
}

bool XbaseFile::putInteger(size_t col_idx, int value)
{
    unsigned char* pos = m_currow_ptr+m_fields[col_idx].offset;

    unsigned int v = (unsigned int)value;
    *(pos+0) = (v & 0x000000ff);
    *(pos+1) = (v & 0x0000ff00) >> 8;
    *(pos+2) = (v & 0x00ff0000) >> 16;
    *(pos+3) = (v & 0xff000000) >> 24;
    return true;
}

bool XbaseFile::putBoolean(size_t col_idx, bool value)
{
    unsigned char* pos = m_currow_ptr+m_fields[col_idx].offset;

    *pos = value ? 'T' : 'F';
    return true;
}

bool XbaseFile::putNull(size_t col_idx)
{
    unsigned char* pos = m_currow_ptr+m_fields[col_idx].offset;

    size_t width = m_fields[col_idx].width;
    
    switch (m_fields[col_idx].type)
    {
        case 'C':
        {
            memset(pos, ' ', width);
            return true;
        }

        case 'N':
        {
            memset(pos, ' ', width);
            return true;
        }

        case 'B':
        {
            memset(pos, 0, width);
            return true;
        }

        case 'F':
        {
            memset(pos, ' ', width);
            return true;
        }

        case 'I':
        {
            memset(pos, 0, width);
            return true;
        }

        case 'D':
        {
            memset(pos, ' ', width);
            return true;
        }

        case 'T':
        {
            memset(pos, 0, width);
            return true;
        }

        case 'L':
        {
            *pos = ' ';
            return true;
        }
    }

    return false;
}

bool XbaseFile::writeRow()
{
    // row numbers start at 1, which is why we have to subtract 1
    size_t row_offset = m_row_width*(m_currow_num-1);
    
    // go to the correct location in the file
    m_stream->seek(m_header_len+row_offset, xd::seekSet);
    
    // write out the row data to the file
    unsigned long written = 0;
    m_stream->write(m_buf, m_row_width, &written);
    
    // clear out the buffer for the next set of rows
    memset(m_buf, ' ', m_buf_maxrows*m_row_width);
    
    return (written == m_row_width ? true : false);
}

bool XbaseFile::startInsert()
{
    if (!isOpen())
        return false;
    
    // if we don't have a buffer, create one
    if (!m_buf || m_read_ahead != m_buf_maxrows)
    {
        m_buf_maxrows = m_read_ahead;
        if (m_buf)
        {
            delete[] m_buf;
        }
        m_buf = new unsigned char[m_buf_maxrows*m_row_width];
        memset(m_buf, ' ', m_buf_maxrows*m_row_width);
        m_currow_ptr = m_buf;
        m_currow_num = 0;
    }
        
    m_inserting = true;
    return true;
}

bool XbaseFile::finishInsert()
{
    flush();

    m_inserting = false;
    return true;
}

bool XbaseFile::insertRow()
{
    if (!m_inserting)
        return false;

    m_buf_rows++;
    m_currow_ptr += m_row_width;
    if (m_buf_rows == XBASE_BUFFER_ROWCOUNT)
        return flush();

    return true;
}

bool XbaseFile::flush()
{
    if (m_buf_rows == 0)
        return true;

    unsigned long read_bytes;
    unsigned long written;

    // go to the end of the file
    m_stream->seek(-1, xd::seekEnd);

    // is there an EOF marker?
    char ch = 0;
    m_stream->read(&ch, 1, NULL);

    if (ch == 0x1a)
    {
        m_stream->seek(-1, xd::seekEnd);
    }
     else
    {
        m_stream->seek(0, xd::seekEnd);
    }

    // write out the row data to the file
    m_stream->write(m_buf, m_buf_rows*m_row_width, &written);
    
    // write out the file terminator to the file
    unsigned char end_char[1];
    end_char[0] = 0x1a;
    m_stream->write(end_char, 1, NULL);

    // update record count in header
    unsigned char buf[4];
    m_stream->seek(4);
    m_stream->read(buf, 4, &read_bytes);

    unsigned int row_count = buf[0] +
                             (buf[1]*256) +
                             (buf[2]*65536) +
                             (buf[3] * 16777216);
    row_count += m_buf_rows;
    buf[0] = (row_count & 0x000000ff);
    buf[1] = (row_count & 0x0000ff00) >> 8;
    buf[2] = (row_count & 0x00ff0000) >> 16;
    buf[3] = (row_count & 0xff000000) >> 24;
    m_stream->seek(4);
    m_stream->write(buf, 4, &written);

    // clear out the buffer for the next set of rows
    memset(m_buf, ' ', m_buf_maxrows*m_row_width);

    // start over for the next batch
    m_currow_ptr = m_buf;
    m_buf_rows = 0;
    return true;
}

void XbaseFile::goRow(size_t row)
{
    m_currow_num = row;
    
    if (row == 0)
    {
        // clear out the buffer for the next set of rows
        memset(m_buf, ' ', m_buf_maxrows*m_row_width);
        m_currow_ptr = m_buf;
        m_buf_maxrows = 0;
        return;
    }

    // if the record is in our buffer already,
    // just return the pointer to the buffer

    if (row >= m_buf_firstrow && row < m_buf_firstrow+m_buf_maxrows)
    {
        m_currow_ptr = m_buf+((row-m_buf_firstrow)*m_row_width);
        return;
    }

    // if we haven't create a buffer or the new read ahead count is different
    // than the buffer's current size, we need to resize the buffer

    if (!m_buf || m_read_ahead != m_buf_maxrows)
    {
        m_buf_maxrows = m_read_ahead;
        if (m_buf)
            delete[] m_buf;
        m_buf = new unsigned char[m_buf_maxrows*m_row_width];
        memset(m_buf, ' ', m_buf_maxrows*m_row_width);
    }
    
    // read in buffer
    xf_off_t pos;
    pos = row-1;            // row numbers start at 1
    pos *= m_row_width;
    pos += m_header_len;

    unsigned long read_bytes = 0;
    m_stream->seek(pos);
    m_stream->read(m_buf, m_row_width * m_buf_maxrows, &read_bytes);

    m_buf_firstrow = row;
    m_currow_ptr = m_buf;

    return;
}

void XbaseFile::setReadAhead(bool read_ahead)
{
    m_read_ahead = read_ahead ? XBASE_BUFFER_ROWCOUNT : 1;
}

