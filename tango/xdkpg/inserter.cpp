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
#include <kl/math.h>
#include <kl/xml.h>
#include "pkgfile.h"
#include "../xdcommon/util.h"

const int PACKAGE_BLOCK_SIZE = 1000000;

KpgRowInserter::KpgRowInserter(KpgDatabase* db, const std::wstring& table, tango::IStructurePtr structure)
{
    m_database = db;
    m_database->ref();
    
    m_structure = structure;
    m_data = NULL;

    tango::IAttributesPtr attr = m_database->getAttributes();

    m_buf_rows = 0;
    m_rows_per_buf = 0;
    m_table = table;
    kl::replaceStr(m_table, L"\"", L"");
}

KpgRowInserter::~KpgRowInserter()
{
    delete[] m_data;

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
    int i, col_count = m_structure->getColumnCount();
    int total_phys_width = 0;

    // calculate the total physical row width
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo;

        colinfo = m_structure->getColumnInfoByIdx(i);

        if (!colinfo->getCalculated())
        {
            KpgInsertFieldData fd;
            fd.m_name = colinfo->getName();
            fd.m_tango_type = colinfo->getType();
            fd.m_width = colinfo->getWidth();
            fd.m_scale = colinfo->getScale();
            fd.m_offset = total_phys_width;
            m_fields.push_back(fd);


            int width = 0;
            
            switch (colinfo->getType())
            {
                case tango::typeCharacter:
                    width = colinfo->getWidth();
                    break;
                case tango::typeWideCharacter:
                    width = colinfo->getWidth() * 2;
                    break;
                case tango::typeNumeric:
                    width = colinfo->getWidth();
                    break;
                case tango::typeDouble:
                    width = sizeof(double);
                    break;
                case tango::typeInteger:
                    width = sizeof(int);
                    break;
                case tango::typeBoolean:
                    width = 1;
                    break;
                case tango::typeDate:
                    width = 4;
                    break;
                case tango::typeDateTime:
                    width = 8;
                    break;
            }

            total_phys_width += width;

            if (colinfo->getNullsAllowed())
                total_phys_width++;
        }
    }

    m_row_width = total_phys_width;
    m_rows_per_buf = PACKAGE_BLOCK_SIZE / m_row_width;
    m_data = new unsigned char[m_rows_per_buf * m_row_width];


    m_writer = m_database->m_kpg->createStream(m_table);




    // write out the table info block
    kl::xmlnode stream_info;

    stream_info.setNodeName(L"pkg_stream_info");
    stream_info.appendProperty(L"type", L"set");
    stream_info.addChild(L"version", 1);

    kl::xmlnode& phys_row_width_node = stream_info.addChild();
    phys_row_width_node.setNodeName(L"phys_row_width");
    phys_row_width_node.setNodeValue(total_phys_width);

    kl::xmlnode& structure_node = stream_info.addChild();
    structure_node.setNodeName(L"structure");
    xdkpgStructureToXml(m_structure, structure_node);

    kl::xmlnode& tabledoc_model_node = stream_info.addChild();
    tabledoc_model_node.setNodeName(L"tabledoc_model");

    /*
    // first load the set's model
    ITableDocModelPtr tabledoc_model;
    tabledoc_model = TableDocMgr::loadModel(set->getSetId());
    if (tabledoc_model.isOk())
    {
        tabledocmodel2xml(tabledoc_model, tabledoc_model_node);
    }
    */

    std::wstring info_block_str = stream_info.getXML(kl::xmlnode::formattingSpaces |
                                                     kl::xmlnode::formattingCrLf);

    m_writer->startWrite();


    // create a little-endian UCS-2 version of the info block xml
    int buf_len = (info_block_str.length()+1)*2;
    unsigned char* info_block = new unsigned char[buf_len];
    kl::wstring2ucsle(info_block, info_block_str, buf_len/2);
    m_writer->writeBlock(info_block, buf_len, true);
    delete[] info_block;


    return true;
}


// reipped from xdnative/util.cpp
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

bool KpgRowInserter::insertRow()
{
    unsigned char* buf = m_data + (m_buf_rows * m_row_width);


    std::vector<KpgInsertFieldData>::iterator it, it_end;
    std::string str;

    it_end = m_fields.end();
    size_t len;
    tango::datetime_t dt;

    memset(buf, 0, m_row_width);

    for (it = m_fields.begin(); it != it_end; ++it)
    {

        switch (it->m_tango_type)
        {
            case tango::typeCharacter:
            {
                str = kl::tostring(it->m_str_val);
                len = str.length();
                if (len > (size_t)it->m_width)
                    len = it->m_width;
                memcpy(buf + it->m_offset, str.c_str(), len);
                break;
            }
            case tango::typeWideCharacter:
                kl::wstring2ucsle(buf + it->m_offset, it->m_str_val, it->m_width);
                break;
            case tango::typeNumeric:
                dbl2decstr((char*)buf + it->m_offset, it->m_dbl_val, it->m_width, it->m_scale);
                break;
            case tango::typeDouble:
                // little-endian only
                memcpy(buf + it->m_offset, &it->m_dbl_val, sizeof(double));
                break;
            case tango::typeInteger:
                // little-endian only
                memcpy(buf + it->m_offset, &it->m_int_val, sizeof(int));
                break;
            case tango::typeBoolean:
                // little-endian only
                *(buf + it->m_offset) = it->m_bool_val ? 'T' : 'F';
                break;
            case tango::typeDate:
                dt = it->m_datetime_val;
                dt >>= 32;
                int2buf(buf + it->m_offset, (unsigned int)dt);
                break;
            case tango::typeDateTime:
                int2buf(buf + it->m_offset, (unsigned int)(it->m_datetime_val >> 32));
                int2buf(buf + it->m_offset + 4, (unsigned int)(it->m_datetime_val & 0xffffffff));
                break;
        }


        // clear out value for next row
        it->m_str_val.clear();
        it->m_dbl_val = 0.0;
        it->m_int_val = 0;
        it->m_datetime_val = 0;
        it->m_bool_val = false;
    }



    m_buf_rows++;

    if (m_buf_rows >= m_rows_per_buf)
    {
        m_writer->writeBlock(m_data,
                             m_buf_rows * m_row_width,
                             true /* compressed */);
        memset(m_data, 0, m_rows_per_buf * m_row_width);
    }

    return true;
}

void KpgRowInserter::finishInsert()
{
    if (!m_writer)
        return;

    if (m_buf_rows > 0)
    {
        m_writer->writeBlock(m_data,
                             m_buf_rows * m_row_width,
                             true /* compressed */);
    }

    m_writer->finishWrite();
    delete m_writer;
    m_writer = NULL;
}

bool KpgRowInserter::flush()
{
    return true;
}
