/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-05-12
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <cmath>
#include "xdfs.h"
#include "database.h"
#include "rawtext.h"
#include "fixedlengthtextset.h"
#include "fixedlengthtextiterator.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/util.h"
#include "../xdcommon/extfileinfo.h"
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/math.h>
#include <kl/md5.h>


const int GUESS_LINE_DELIMITERS_NEEDED = 20;
const int GUESS_BUF_SIZE = 400000;
const int BUF_ROW_COUNT = 10000;


// FixedLengthTextSet class implementation

FixedLengthTextSet::FixedLengthTextSet(FsDatabase* database)  : XdfsBaseSet(database)
{
    m_path = L"";
    m_row_count = 0;

    m_file_size = 0;
    m_temporary = false;
}

FixedLengthTextSet::~FixedLengthTextSet()
{
}

bool FixedLengthTextSet::init(const std::wstring& filename, const xd::FormatDefinition& def)
{
    if (!xf_get_file_exist(filename))
        return false;

    // set our member variables
    m_path = filename;
    m_def = def;
    m_def.format = xd::formatFixedLengthText;
    
    if (m_def.fixed_row_width == 0)
        m_def.fixed_row_width = 80;  // must have some kind of default row width

    if (m_def.columns.size() == 0)
    {
        // must have some kind of column
        xd::ColumnInfo col;
        col.name = L"Field1";
        col.type = xd::typeCharacter;
        col.source_offset = 0;
        col.source_width = m_def.fixed_row_width;
        col.width = m_def.fixed_row_width;

        m_def.columns.push_back(col);
    }

    return m_file.open(filename, def);
}


xd::IRowInserterPtr FixedLengthTextSet::getRowInserter()
{
    FixedLengthTextRowInserter* inserter = new FixedLengthTextRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}

bool FixedLengthTextSet::getFormatDefinition(xd::FormatDefinition* def)
{
    *def = m_def;
    return true;
}

xd::IIteratorPtr FixedLengthTextSet::createIterator(
                                        const std::wstring& columns,
                                        const std::wstring& order,
                                        xd::IJob* job)
{
    if (order.empty())
    {
        FixedLengthTextIterator* iter = new FixedLengthTextIterator(m_database);
        if (!iter->init(this, m_path, columns))
        {
            delete iter;
            return xcm::null;
        }
            
        return static_cast<xd::IIterator*>(iter);
    }
    
    // find out where the database should put temporary files
    std::wstring temp_directory = m_database->getTempFileDirectory();

    // create a unique index file name with .idx extension
    std::wstring index_filename = getUniqueString();
    index_filename += L".idx";

    // generate a full path name from the temp path and unique idx filename
    std::wstring full_index_filename;
    full_index_filename = makePathName(temp_directory,
                                       L"",
                                       index_filename);

    IIndex* idx;
    idx = createExternalIndex(m_database,
                              getObjectPath(),
                              full_index_filename,
                              temp_directory,
                              order,
                              true,
                              true,
                              job);
    if (!idx)
        return xcm::null;

    xd::IIteratorPtr data_iter = createIterator(columns, L"", NULL);
    if (data_iter.isNull())
    {
        idx->unref();
        return xcm::null;
    }


    xd::IIteratorPtr result_iter = createIteratorFromIndex(data_iter,
                                                           idx,
                                                           columns,
                                                           order,
                                                           getObjectPath());
    idx->unref();
    return result_iter;
}


xd::rowpos_t FixedLengthTextSet::getRowCount()
{
    return m_file.getRowCount();
}



inline xd::Structure createDefaultStructure(const xd::Structure& source)
{
    xd::Structure s;

    int i, col_count = source.getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo&  sourcecol = source.getColumnInfoByIdx(i);
        
        xd::ColumnInfo col;
        col.name = sourcecol.name;
        col.type = sourcecol.type;
        col.width = sourcecol.width;
        col.scale = sourcecol.scale;
        col.expression = sourcecol.name;
        col.column_ordinal = i;

        s.createColumn(col);
    }
    
    return s;
}


xd::Structure FixedLengthTextSet::getStructure()
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
        col.table_ordinal = 0;
        col.nulls_allowed = it->nulls_allowed;

        structure.createColumn(col);
    }

    XdfsBaseSet::appendCalcFields(structure);
    return structure;
}





// FixedLengthTextRowInserter class implementation

FixedLengthTextRowInserter::FixedLengthTextRowInserter(FixedLengthTextSet* set)
{
    m_set = set;
    m_set->ref();

    m_inserting = false;

    m_file = NULL;
    m_buf = NULL;                    // buffer for rows
    m_buf_ptr = NULL;
    m_row_width = 0;
    m_crlf = true;
}

FixedLengthTextRowInserter::~FixedLengthTextRowInserter()
{
    std::vector<FixedLengthTextInsertData*>::iterator it;
    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        delete (*it);
    }
    
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }

    if (m_file)
    {
        xf_close(m_file);
        m_file = NULL;
    }
    
    m_set->unref();
}

xd::objhandle_t FixedLengthTextRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<FixedLengthTextInsertData*>::iterator it;

    std::string asc_colname = kl::tostring(column_name);

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!strcasecmp((*it)->m_colname.c_str(), asc_colname.c_str()))
            return (xd::objhandle_t)(*it);
    }

    return 0;
}

bool FixedLengthTextRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                         const unsigned char* value,
                                         int length)
{
    return true;
}

bool FixedLengthTextRowInserter::putString(xd::objhandle_t column_handle,
                                         const std::string& value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_str_val = value;
    return true;
}

bool FixedLengthTextRowInserter::putWideString(xd::objhandle_t column_handle,
                                             const std::wstring& value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    return putString(column_handle, kl::tostring(value));
}

bool FixedLengthTextRowInserter::putDouble(xd::objhandle_t column_handle, double value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    char buf[255];
    sprintf(buf, "%.*f", f->m_scale, kl::dblround(value, f->m_scale));

    // convert a euro decimal character to a decimal point
    char* p = buf;
    while (*p)
    {
        if (*p == ',')
            *p = '.';
        ++p;
    }

    f->m_str_val = buf;
    return true;
}

bool FixedLengthTextRowInserter::putInteger(xd::objhandle_t column_handle,
                                          int value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    char buf[64];
    sprintf(buf, "%d", value);
    f->m_str_val = buf;
    return true;
}

bool FixedLengthTextRowInserter::putBoolean(xd::objhandle_t column_handle,
                                          bool value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    value ? f->m_str_val = "T" : f->m_str_val = "F";
    return true;
}

bool FixedLengthTextRowInserter::putDateTime(xd::objhandle_t column_handle,
                                           xd::datetime_t value)
{
    FixedLengthTextInsertData* f = (FixedLengthTextInsertData*)column_handle;
    if (!f)
    {
        return false;
    }

    if (value == 0)
    {
        f->m_str_val = "";
        return true;
    }

    xd::DateTime dt(value);

    int y = dt.getYear();
    int m = dt.getMonth();
    int d = dt.getDay();
    int hh = dt.getHour();
    int mm = dt.getMinute();
    int ss = dt.getSecond();

    char buf[64];

    if (hh == 0 && mm == 0 && ss == 0)
    {
        snprintf(buf, 64, "%04d/%02d/%02d", y, m, d);
    }
     else
    {
        snprintf(buf, 64, "%04d/%02d/%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
    }

    f->m_str_val = buf;
    return true;
}

bool FixedLengthTextRowInserter::putNull(xd::objhandle_t column_handle)
{
    return true;
}

bool FixedLengthTextRowInserter::startInsert(const std::wstring& col_list)
{
    return false;

/*
    if (!m_file)
    {
        m_file = xf_open(m_set->m_path, xfOpen, xfReadWrite, xfShareNone);

        if (m_file == NULL)
            return false;
    }

    // get the source structure since we're inserting records
    xd::Structure s = m_set->getSourceStructure();

    int i;
    int col_count = s->getColumnCount();
    m_row_width = 0;
    
    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& colinfo = s->getColumnInfoByIdx(i);

        FixedLengthTextInsertData* field = new FixedLengthTextInsertData;
        field->m_colname = kl::tostring(colinfo.name);
        field->m_type = colinfo.type;
        field->m_width = colinfo.width;
        field->m_scale = colinfo.scale;

        switch (field->m_type)
        {
            default:
                field->m_align_right = false;
                break;

            // right-align numerics                
            case xd::typeNumeric:
            case xd::typeDouble:
            case xd::typeInteger:
                field->m_align_right = true;
                break;
        }

        m_row_width += field->m_width;
        m_insert_data.push_back(field);
    }
    
    // add two bytes to the row width for the CR/LF
    if (m_crlf)
        m_row_width += 2;
        
    // create the buffer for writing to the file
    m_buf = new char[BUF_ROW_COUNT*m_row_width];

    // set the buffer pointer to the beginning of the buffer
    m_buf_ptr = m_buf;
    
    m_inserting = true;
    return true;
    */
}

bool FixedLengthTextRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    // if this row extends past our buffer size, flush the buffer
    if (m_buf_ptr+m_row_width >= m_buf+(BUF_ROW_COUNT*m_row_width))
        flush();

    int diff;
    
    std::vector<FixedLengthTextInsertData*>::iterator it;
    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        // (field width - actual data width)
        diff = (*it)->m_width - (*it)->m_str_val.length();
        
        // add the string
        char* p = (char*)(*it)->m_str_val.c_str();

        // for left-padded fields
        if ((*it)->m_align_right)
        {
            while (diff-- > 0)
            {
                *m_buf_ptr = L' ';
                m_buf_ptr++;
            }
        }

        // insert data
        while (*p)
        {
            *m_buf_ptr = *p;

            m_buf_ptr++;
            p++;
        }
        
        // for right-padded fields
        if (!(*it)->m_align_right)
        {
            while (diff-- > 0)
            {
                *m_buf_ptr = L' ';
                m_buf_ptr++;
            }
        }
    }

    if (m_crlf)
    {
        *(m_buf_ptr) = 0x0d;
        m_buf_ptr++;
        *(m_buf_ptr) = 0x0a;
        m_buf_ptr++;
    }
    
    return true;
}

void FixedLengthTextRowInserter::finishInsert()
{
    // flush any remaining data in the buffer before closing the file
    flush();
    
    // delete the buffer
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }

    // close the file
    if (m_file)
    {
        xf_close(m_file);
        m_file = NULL;
    }
    
    m_inserting = false;
}

bool FixedLengthTextRowInserter::flush()
{
    // make sure that the last two characters of the file are a CF/LF
    if (m_crlf && xf_seek(m_file, -2, xfSeekEnd))
    {
        char temp_buf[2];
        if (xf_read(m_file, temp_buf, 1, 2) == 2)
        {
            if (temp_buf[0] != 0x0d ||
                temp_buf[1] != 0x0a)
            {
                temp_buf[0] = 0x0d;
                temp_buf[1] = 0x0a;
                xf_write(m_file, temp_buf, 1, 2);
            }
        }
         else
        {
            xf_seek(m_file, 0, xfSeekEnd);
        }
    }
     else
    {
        xf_seek(m_file, 0, xfSeekEnd);
    }

    xf_write(m_file, m_buf, m_buf_ptr-m_buf, 1);
    m_buf_ptr = m_buf;
    
    return true;
}




