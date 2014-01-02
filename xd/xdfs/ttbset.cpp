/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <xd/xd.h>
#include "xdfs.h"
#include "database.h"
#include "ttbset.h"
#include "ttbiterator.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/util.h"
#include "../xdcommon/extfileinfo.h"
#include <kl/md5.h>
#include <kl/math.h>

// TtbSet class implementation

TtbSet::TtbSet(FsDatabase* db)
{
    m_database = db;
    m_database->ref();
}

TtbSet::~TtbSet()
{
    if (m_file.isOpen())
        m_file.close();

    m_database->unref();
}

bool TtbSet::init(const std::wstring& filename)
{
    if (!m_file.open(filename))
        return false;

    // set the set info filename
    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    
    setConfigFilePath(ExtFileInfo::getConfigFilenameFromPath(definition_path, filename));

    return true;
}




xd::IStructurePtr TtbSet::getStructure()
{
    // if we can't open the file, return an empty structure
    if (!m_file.isOpen())
        return xcm::null;

    // get structure from xbase file
    xd::IStructurePtr s = m_file.getStructure();

    CommonBaseSet::appendCalcFields(s);
    return s;
}

bool TtbSet::modifyStructure(xd::IStructure* struct_config,
                               xd::IJob* job)
{
    bool done_flag = false;
    CommonBaseSet::modifyStructure(struct_config, &done_flag);
    return true;
}

xd::IRowInserterPtr TtbSet::getRowInserter()
{
    TtbRowInserter* inserter = new TtbRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}


xd::IIteratorPtr TtbSet::createIterator(const std::wstring& columns,
                                             const std::wstring& order,
                                             xd::IJob* job)
{
    if (order.empty())
    {
        TtbIterator* iter = new TtbIterator;
        if (!iter->init(m_database,
                        this,
                        m_file.getFilename()))
        {
            delete iter;
            return xcm::null;
        }
        
        iter->goFirst();
        return static_cast<xd::IIterator*>(iter);
    }
    
    // find out where the database should put temporary files
    IFsDatabasePtr fsdb = m_database;
    std::wstring temp_directory = fsdb->getTempFileDirectory();
 
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
                              job);
    if (!idx)
    {
        return xcm::null;
    }

    xd::IIteratorPtr data_iter = createIterator(columns, L"", NULL);
    return createIteratorFromIndex(data_iter,
                                   idx,
                                   columns,
                                   order,
                                   getObjectPath());
}

xd::rowpos_t TtbSet::getRowCount()
{
    return (xd::rowpos_t)m_file.getRowCount();
}

bool TtbSet::updateRow(xd::rowid_t rowid,
                       xd::ColumnUpdateInfo* info,
                       size_t info_size)
{
    return true;
}


// TtbRowInserter class implementation


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

const int ttb_inserter_buf_rows = 500;

TtbRowInserter::TtbRowInserter(TtbSet* set)
{
    m_set = set;
    m_set->ref();

    m_file = &(m_set->m_file);

    m_row_width = m_file->getRowWidth();
    m_buf_row = 0;
    m_tbl_filename = m_file->getFilename();
    m_buf = new unsigned char[ttb_inserter_buf_rows * m_row_width];
    m_rowptr = m_buf;

    m_inserting = false;
}

TtbRowInserter::~TtbRowInserter()
{
    delete[] m_buf;

    m_set->unref();
}


bool TtbRowInserter::startInsert(const std::wstring& _col_list)
{
    xd::IStructurePtr structure = m_file->getStructure();
    if (structure.isNull())
        return false;

    std::wstring col_list = _col_list;
    kl::trim(col_list);
    if (col_list == L"" || col_list == L"*")
    {
        col_list = L"";
        int i, col_count = structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            if (i > 0)
                col_list += L",";
            col_list += structure->getColumnName(i);
        }
    }


    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;

    kl::parseDelimitedList(col_list, columns, L',');

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        TtbInsertData* col = new TtbInsertData;
        col->col = structure->getColumnInfo(*it);
        if (col->col.isNull())
        {
            delete col;
            return false;
        }
        col->name = col->col->getName();
        col->type = col->col->getType();
        col->width = col->col->getWidth();
        col->scale = col->col->getScale();
        col->offset = col->col->getOffset();
        col->nulls_allowed = false;

        m_fields.push_back(col);
    }

    memset(m_buf, 0, ttb_inserter_buf_rows * m_row_width);
    m_buf_row = 0;
    m_rowptr = m_buf;
    m_inserting = true;

    return true;
}


xd::objhandle_t TtbRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<TtbInsertData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, column_name))
            return (xd::objhandle_t)(*it);
    }

    return (xd::objhandle_t)0;
}

xd::IColumnInfoPtr TtbRowInserter::getInfo(xd::objhandle_t column_handle)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;
    if (!dai)
        return xcm::null;

    return dai->col;
}

bool TtbRowInserter::putRawPtr(xd::objhandle_t column_handle,
                               const unsigned char* value,
                               int length)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

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

bool TtbRowInserter::putString(xd::objhandle_t column_handle,
                               const std::string& value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    // set data
    if (dai->type == xd::typeCharacter)
    {
        memset(m_rowptr + dai->offset, 0, dai->width);

        int write_len = value.length();
        if (write_len > dai->width)
            write_len = dai->width;

        memcpy(m_rowptr + dai->offset, value.c_str(), write_len);
    }
     else if (dai->type == xd::typeWideCharacter)
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

    return false;
}

bool TtbRowInserter::putWideString(xd::objhandle_t column_handle,
                                   const std::wstring& value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    // set data
    if (dai->type == xd::typeWideCharacter)
    {
        kl::wstring2ucsle(m_rowptr + dai->offset, value, dai->width);
    }
     else if (dai->type == xd::typeCharacter)
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

    return false;
}

bool TtbRowInserter::putDouble(xd::objhandle_t column_handle,
                               double value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    if (dai->width < 1)
        return false;

    if (dai->type == xd::typeNumeric)
    {
        dbl2decstr((char*)m_rowptr + dai->offset,
                   value,
                   dai->width,
                   dai->scale);
    }
     else if (dai->type == xd::typeDouble)
    {
        // FIXME: this will only work on little-endian (intel) processors
        memcpy(m_rowptr + dai->offset, &value, sizeof(double));
    }
     else if (dai->type == xd::typeInteger)
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

bool TtbRowInserter::putInteger(xd::objhandle_t column_handle,
                                int value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    if (dai->type == xd::typeInteger)
    {
        // set data
        unsigned char* ptr = m_rowptr+dai->offset;
        unsigned int v = (unsigned int)value;
        *(ptr)   = (v) & 0xff;
        *(ptr+1) = (v >> 8) & 0xff;
        *(ptr+2) = (v >> 16) & 0xff;
        *(ptr+3) = (v >> 24) & 0xff;
    }
     else if (dai->type == xd::typeNumeric)
    {
        dbl2decstr((char*)m_rowptr + dai->offset,
                   value,
                   dai->width,
                   dai->scale);
    }
     else if (dai->type == xd::typeDouble)
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

bool TtbRowInserter::putBoolean(xd::objhandle_t column_handle,
                                bool value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

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

bool TtbRowInserter::putDateTime(xd::objhandle_t column_handle,
                                 xd::datetime_t value)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;

    // set data
    if (dai->type == xd::typeDate)
    {
        unsigned int julian_day = (unsigned int)(value >> 32);

        int2buf(m_rowptr+dai->offset, julian_day);
    }
     else if (dai->type == xd::typeDateTime)
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

bool TtbRowInserter::putRowBuffer(const unsigned char* value)
{
    return false;
}

bool TtbRowInserter::putNull(xd::objhandle_t column_handle)
{
    TtbInsertData* dai = (TtbInsertData*)column_handle;
    
    if (!dai->nulls_allowed)
        return false;

    *(m_rowptr + dai->offset - 1) |= 0x01;

    return true;
}


bool TtbRowInserter::insertRow()
{
    m_buf_row++;
    m_rowptr += m_row_width;
    if (m_buf_row == ttb_inserter_buf_rows)
    {
        flush();
    }

    return true;
}

void TtbRowInserter::finishInsert()
{
    flush();
}

bool TtbRowInserter::flush()
{
    if (m_buf_row >= 0)
    {
        m_file->appendRows(m_buf, m_buf_row);
        m_buf_row = 0;
        m_rowptr = m_buf;
        memset(m_buf, 0, ttb_inserter_buf_rows * m_row_width);
    }

    return true;
}

