/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-21
 *
 */


#include "kxinter.h"
#include "jobconvert.h"
#include "datadef.h"
#include "datastream.h"
#include "treecontroller.h"


#define CONVERT_BUF_SIZE    65536


int raw2tangoType(int cobol_type)
{
    switch (cobol_type)
    {
        default:
        case ddtypeCharacter:
            return tango::typeCharacter;

        case ddtypeDecimal:
        case ddtypeNumeric:
            return tango::typeNumeric;

        case ddtypeDateYYMMDD:
        case ddtypeDateMMDDYY:
        case ddtypeDateCCYYMMDD:
        case ddtypeDateCCYY_MM_DD:
            return tango::typeDate;
    }
}




// -- ConvertDataJob class implementation --

ConvertDataJob::ConvertDataJob()
{
    m_record_length = 0;
    m_record_type = dfdFixed;
    m_buf = NULL;
    m_buf_size = 0;
    m_skip_bytes = 0;
    m_curpos = NULL;

    memset(m_line_delimiter, 0, 64);
    m_line_delimiter_length = 0;

    m_job_info->setTitle(_("Data Conversion"));

    m_buf = new unsigned char[CONVERT_BUF_SIZE];
}

ConvertDataJob::~ConvertDataJob()
{
    delete[] m_buf;
}


void ConvertDataJob::addInstruction(IDataStream* stream,
                                 const DataFormatDefinition& dfd,
                                 tango::IDatabasePtr destination,
                                 const wxString& table_name)
{
    ConvertDataInstruction i;
    i.m_db = destination;
    i.m_table_name = table_name;
    i.m_dfd = dfd;
    i.m_stream = stream->clone();

    m_instructions.push_back(i);


/*
    m_stream = stream->clone();

    m_dfd = dfd;
    m_db = destination;
    m_table_name = table_name;
    m_record_length = dfd.record_length;
    m_record_type = dfd.type;
    m_skip_bytes = dfd.skip_bytes;

    memcpy(m_line_delimiter, dfd.line_delimiter, 64);
    m_line_delimiter_length = dfd.line_delimiter_length;
*/





/*
    if (m_dfd.type == dfdDelimited)
    {
        cfw::IJobStatEnumPtr stats = m_job_info->getStatInfo();

        int count = stats->size();
        int i;

        for (i = 0; i < count; ++i)
        {
            cfw::IJobStatPtr stat = stats->getItem(i);

            if (stat->getId() == cfw::jobStatCurrentCount)
            {
                stat->setCaption(_("Bytes Processed:"));
            }
            
            if (stat->getId() == cfw::jobStatMaxCount)
            {
                stat->setCaption(_("Total Bytes:"));
            }
        }


        m_skipped_stat = m_job_info->insertStat(0);
        m_processed_stat = m_job_info->insertStat(0);
    }
     else
    {
        m_skipped_stat = m_job_info->insertStat(-1);
    }

    if (m_processed_stat)
    {
        m_processed_stat->setCaption(_("Processed Records:"));
        m_processed_stat->setRightAligned(true);
    }

    if (m_skipped_stat)
    {
        m_skipped_stat->setCaption(_("Skipped Records:"));
        m_skipped_stat->setRightAligned(true);
    }
*/
}




unsigned char* ConvertDataJob::getNextFixedRecord(int* reclen)
{
    *reclen = m_record_length;

    // -- fixed length --

    if (m_curpos+m_record_length >= m_buf+m_buf_size || m_curpos == NULL)
    {
        m_buf_size = m_stream.p->getData(m_buf, (CONVERT_BUF_SIZE/m_record_length)*m_record_length);

        if (m_buf_size == 0)
        {
            return NULL;
        }

        m_curpos = m_buf;
    }
     else
    {
        m_curpos += m_record_length;
    }

    return m_curpos;
}


unsigned char* ConvertDataJob::getNextVariableRecord(int* reclen)
{
    if (m_curpos >= m_buf+m_buf_size || m_curpos == NULL)
    {
        int read_len = m_stream.p->getData(m_buf, 4);
        if (read_len == 0)
            return NULL;

        int block_len = (m_buf[0] << 8) | m_buf[1];
        block_len -= 4;
        read_len = m_stream.p->getData(m_buf, block_len);
        m_buf_size = read_len;
        m_curpos = m_buf;
    }

    *reclen = ((*m_curpos << 8) | *(m_curpos+1)) - 4;
    unsigned char* retval = m_curpos+4;
    m_curpos += (*reclen + 4);
    return retval;
}



unsigned char* ConvertDataJob::getNextDelimitedRecord(int* reclen)
{
    if (m_curpos == NULL || (m_curpos-m_buf) > (CONVERT_BUF_SIZE/2))
    {
        if (m_curpos == NULL)
        {
            m_buf_size = m_stream.p->getData(m_buf, CONVERT_BUF_SIZE);
            m_curpos = m_buf;
        }
         else
        {
            int move_size = CONVERT_BUF_SIZE-(m_curpos-m_buf);
            memmove(m_buf, m_curpos, move_size);
            m_buf_size = move_size;
            int new_read = m_stream.p->getData(m_buf+move_size, CONVERT_BUF_SIZE-move_size);
            m_buf_size += new_read;
            
            //wchar_t buf[255];
            //swprintf(buf, L"offset = %d, move_size = %d, new buf size = %d\n", (int)(m_curpos-m_buf), move_size, m_buf_size);
            //OutputDebugStringW(buf);
                          
            m_curpos = m_buf;
        }
    }


    unsigned char* ptr = m_curpos;

    if (ptr >= m_buf+m_buf_size)
        return NULL;

    int i = 0;
    while (1)
    {
        if (ptr+i+m_line_delimiter_length >= m_buf+m_buf_size)
        {
            *reclen = 0;
            return NULL;
        }

        if (0 == memcmp(ptr+i, m_line_delimiter, m_line_delimiter_length))
        {
            *reclen = i;
            m_curpos = ptr+i+m_line_delimiter_length;
            break;
        }

        i++;
    }

    return ptr;
}

unsigned char* ConvertDataJob::getNextRecord(int* reclen)
{
    switch (m_record_type)
    {
        case dfdFixed:
            return getNextFixedRecord(reclen);

        case dfdVariable:
            return getNextVariableRecord(reclen);

        case dfdDelimited:
            return getNextDelimitedRecord(reclen);

        default:
            *reclen = 0;
            return NULL;
    }

    return NULL;
}



struct OutputFieldInfo
{
    tango::objhandle_t out_handle;
    wxString out_name;
    int out_type;
    int out_width;
    int out_scale;

    int in_format;
    int in_type;
    int in_offset;
    int in_width;
    int in_scale;
    int in_signed_flag;
};


struct OutputTableInfo
{
    std::vector<OutputFieldInfo> fields;
    tango::ISetPtr sp_set;
    tango::IRowInserterPtr sp_inserter;
    wxString out_name;

    // -- record identification --
    int record_length;
    int id_offset;              // offset of id byte, -1 if none
    unsigned char id_value;     // byte value of type
};


bool ConvertDataJob::convertTable(ConvertDataInstruction& cdi)
{
    // -- copy members from the instruction into our current table variables --

    tango::IDatabasePtr db = cdi.m_db;
    m_stream = cdi.m_stream;
    m_dfd = cdi.m_dfd;
    wxString table_name = cdi.m_table_name;

    m_record_length = m_dfd.record_length;
    m_record_type = m_dfd.type;
    m_skip_bytes = m_dfd.skip_bytes;
    memcpy(m_line_delimiter, m_dfd.line_delimiter, 64);
    m_line_delimiter_length = m_dfd.line_delimiter_length;


    // ----------------------------------------------------------------- //

    std::vector<OutputTableInfo> tables;
    std::vector<OutputTableInfo>::iterator table_it;
    std::vector<OutputFieldInfo>::iterator field_it;
    bool record_id_used = false;

    if (m_stream.isNull())
    {
        return false;
    }

    if (m_dfd.rec_defs.size() == 0)
    {
        // -- no record definitions --
        return false;
    }

    // -- first, create output table info structs for each input type --

    std::vector<RecordDefinition>::iterator r_it;

    for (r_it = m_dfd.rec_defs.begin();
         r_it != m_dfd.rec_defs.end(); ++r_it)
    {
        OutputTableInfo table_info;
        
        std::vector<FieldDefinition>::iterator f_it;
        for (f_it = r_it->field_defs.begin();
             f_it != r_it->field_defs.end(); ++f_it)
        {
            if (f_it->name.Left(6).CmpNoCase(wxT("FILLER")) == 0)
                continue;

            OutputFieldInfo field_info;
            field_info.out_name = f_it->name;
            field_info.out_type = raw2tangoType(f_it->type);
            field_info.out_width = f_it->length;
            field_info.out_scale = f_it->scale;

            if (field_info.out_type == tango::typeDate)
            {
                field_info.out_width = 4;
                field_info.out_scale = 0;
            }

            field_info.in_format = f_it->format;
            field_info.in_type = f_it->type;
            field_info.in_offset = f_it->offset;
            field_info.in_width = f_it->length;
            field_info.in_scale = f_it->scale;
            field_info.in_signed_flag = f_it->signed_flag;

            table_info.fields.push_back(field_info);
        }

        table_info.out_name = r_it->name;
        table_info.record_length = r_it->record_length;
        table_info.id_offset = r_it->id_offset;
        table_info.id_value = r_it->id_value;

        if (table_info.id_offset != 0)
        {
            record_id_used = true;
        }

        tables.push_back(table_info);
    }



    // -- next, create output tables, row inserters, and column handles --
    for (table_it = tables.begin(); table_it != tables.end(); ++table_it)
    {
        tango::IStructurePtr structure = db->createStructure();

        for (field_it = table_it->fields.begin();
             field_it != table_it->fields.end(); ++field_it)
        {
            int out_width = field_it->out_width;
            if (field_it->out_type == tango::typeNumeric &&
                (field_it->in_format == ddformatCompNumeric ||
                 field_it->in_format == ddformatComp3Numeric))
            {
                out_width = 18;
            }

            tango::IColumnInfoPtr colinfo = structure->createColumn();
            colinfo->setName(towstr(field_it->out_name));
            colinfo->setType(field_it->out_type);
            colinfo->setWidth(out_width);
            colinfo->setScale(field_it->out_scale);
        }

        table_it->sp_set = db->createSet(L"", structure, NULL);
        if (!table_it->sp_set)
        {
            // -- error --
            return false;
        }

        table_it->sp_inserter = table_it->sp_set->getRowInserter();
        if (!table_it->sp_inserter)
        {
            // -- error --
            return false;
        }

        // -- now retrive insert handles for each column --

        for (field_it = table_it->fields.begin();
             field_it != table_it->fields.end(); ++field_it)
        {
            field_it->out_handle = table_it->sp_inserter->getHandle(towstr(field_it->out_name));
            if (field_it->out_handle == 0)
            {
                // -- error --
                return false;
            }
        }


        table_it->sp_inserter->startInsert(L"*");
    }



    // -- begin conversion --

    if (m_dfd.type != dfdVariable)
    {
        m_stream->setPosition(m_skip_bytes);
    }


    double tempd;
    char* tempbuf = new char[65536];
    int i;

    unsigned char* recbuf;
    int reclen;
    double total_processed = 0.0;
    int skipped = 0;

    int counter = 0;

    double bytes_processed = 0;
    bool count_bytes = false;


    if (m_record_type == dfdDelimited)
    {
        count_bytes = true;
        m_job_info->setMaxCount(m_stream->getStreamSize());
    }
     else
    {
        m_job_info->setMaxCount(m_stream->getStreamSize() / m_record_length);
    }

    while ((recbuf = getNextRecord(&reclen)) != NULL)
    {
        if (isCancelling())
        {
            break;
        }

        bytes_processed += (reclen + m_line_delimiter_length);
        counter++;


        if (counter == 591)
        {
            int i = 5;
        }

        if (count_bytes)
        {
            m_job_info->setCurrentCount(bytes_processed);
        }
         else
        {
            m_job_info->setCurrentCount(counter);
        }

        if (counter % 1000 == 0)
        {
            if (m_processed_stat.isOk())
            {
                m_processed_stat->setValue(cfw::dbl2fstr(counter));
            }

            if (m_skipped_stat.isOk())
            {
                m_skipped_stat->setValue(cfw::dbl2fstr(skipped));
            }
        }

        // -- decide which record it is --
        OutputTableInfo* table_info = NULL;

        if (m_record_type == dfdFixed)
        {
            // -- fixed length --

            if (record_id_used)
            {
                for (table_it = tables.begin(); table_it != tables.end(); ++table_it)
                {
                    if (table_it->id_offset >= m_record_length)
                        continue;

                    if (recbuf[table_it->id_offset] == table_it->id_value)
                    {
                        table_info = &(*table_it);
                        break;
                    }
                }
            }
             else
            {
                table_info = &tables[0];
            }
        }
         else
        {
            // -- variable --
            for (table_it = tables.begin(); table_it != tables.end(); ++table_it)
            {
                if (reclen == table_it->record_length)
                {
                    table_info = &(*table_it);
                    break;
                }
            }
        }

        if (table_info == NULL)
        {
            // -- record skipped --
            skipped++;
            continue;
        }

        
        // -- append record to output table --
        tango::IRowInserter* inserter = table_info->sp_inserter.p;


        for (field_it = table_info->fields.begin();
             field_it != table_info->fields.end(); ++field_it)
        {
            switch (field_it->in_format)
            {
                default:
                case ddformatASCII:
                {
                    int copy_len = field_it->in_width;
                    if (field_it->in_offset + copy_len > reclen)
                    {
                        copy_len = reclen - field_it->in_offset;
                        if (copy_len < 0)
                            copy_len = 0;
                    }

                    if (copy_len > 0)
                    {
                        memcpy(tempbuf, recbuf + field_it->in_offset, copy_len);
                        for (i = 0; i < copy_len; ++i)
                        {
                            if (tempbuf[i] <= 0x10)
                                tempbuf[i] = ' ';
                        }
                    }

                    tempbuf[copy_len] = 0;
                }
                break;

                case ddformatEBCDIC:
                {
                    int copy_len = field_it->in_width;
                    if (field_it->in_offset + copy_len > reclen)
                    {
                        copy_len = reclen - field_it->in_offset;
                        if (copy_len < 0)
                            copy_len = 0;
                    }
                    
                    for (i = 0; i < copy_len; ++i)
                    {
                        tempbuf[i] = cfw::ebcdic2ascii(*(recbuf+field_it->in_offset+i));
                        if (tempbuf[i] <= 0x10)
                            tempbuf[i] = ' ';
                    }

                    tempbuf[copy_len] = 0;
                }
                break;
                
                case ddformatCompNumeric:
                {
                    unsigned char* p = recbuf + field_it->in_offset;

                    tempd = 0.0;

                    if (field_it->in_offset + field_it->in_width > reclen)
                    {
                        break;
                    }

                    switch (field_it->in_width)
                    {
                        case 2:
                        {
                            signed short i;
                            i = *p << 8;
                            i |= *(p+1);
                            tempd = i;
                        }
                        break;

                        case 4:
                        {
                            signed int i;
                            i = *p << 24;
                            i |= *(p+1) << 16;
                            i |= *(p+2) << 8;
                            i |= *(p+3);
                            tempd = i;
                        }
                        break;

                        case 8:
                        {
                            xcm::int64_t i;
                            i = *p << 56;
                            i |= *(p+1) << 48;
                            i |= *(p+2) << 40;
                            i |= *(p+3) << 32;
                            i |= *(p+4) << 24;
                            i |= *(p+5) << 16;
                            i |= *(p+6) << 8;
                            i |= *(p+7);
                            tempd = i;
                        }
                        break;
                    }
                }
                break;

                case ddformatComp3Numeric:
                {
                    tempd = 0.0;

                    if (field_it->in_offset + field_it->in_width > reclen)
                    {
                        break;
                    }

                    int i;
                    int len = field_it->in_width;
                    int digit = (len*2)-2;
                    unsigned char c;
                    for (i = 0; i < len; ++i)
                    {
                        c = *(recbuf + field_it->in_offset + i);
                        tempd += (pow((double)10, digit) * double(c >> 4));
                        digit--;

                        if (i+1 < len)
                        {
                            tempd += (pow((double)10, digit) * double(c & 0xf));
                            digit--;
                        }
                         else
                        {
                            // -- check if negative or positive -- 
                            if ((c & 0xf) == 0x0d ||
                                (c & 0xf) == 0x0b)
                            {
                                tempd *= -1;
                            }        
                        }
                    }

                    if (field_it->in_scale)
                    {
                        tempd /= pow((double)10, field_it->in_scale);
                    }
                }
                break;
            }



            switch (field_it->in_type)
            {
                default:
                case ddtypeCharacter:
                {
                    // -- perform a left trim --
                    char *p = tempbuf;
                    while (isspace(*p))
                    {
                        p++;
                        if (!*p)
                            break;
                    }

                    inserter->putString(field_it->out_handle, p);
                }
                break;

                case ddtypeDecimal:
                {
                    inserter->putDouble(field_it->out_handle, tempd);
                }
                break;

                case ddtypeNumeric:
                {
                    if (field_it->in_format == ddformatASCII ||
                        field_it->in_format == ddformatEBCDIC)
                    {
                        tempd = atof(tempbuf);
                    }

                    inserter->putDouble(field_it->out_handle, tempd);
                }
                break;

                case ddtypeDateCCYY_MM_DD:
                case ddtypeDateCCYYMMDD:
                case ddtypeDateMMDDYY:
                case ddtypeDateYYMMDD:
                {
                    tango::datetime_t dt = 0;
                    int y, m, d;

                    if (field_it->in_format == ddformatCompNumeric ||
                        field_it->in_format == ddformatComp3Numeric)
                    {
                        if (field_it->in_type == ddtypeDateYYMMDD ||
                            field_it->in_type == ddtypeDateCCYYMMDD)
                        {
                            int tempi = (int)tempd;
                            d = tempi % 100;
                            tempi -= d;
                            tempi /= 100;
                            m = tempi % 100;
                            tempi -= m;
                            tempi /= 100;
                            y = tempi;
                        }
                         else if (field_it->in_type == ddtypeDateMMDDYY)
                        {
                            int tempi = (int)tempd;
                            y = tempi % 100;
                            tempi -= y;
                            tempi /= 100;
                            d = tempi % 100;
                            tempi -= d;
                            tempi /= 100;
                            m = tempi;
                        }
                    }
                     else
                    {
                        if (field_it->in_type == ddtypeDateYYMMDD)
                        {
                            char yy[3], mm[3], dd[3];
                            memcpy(yy, tempbuf, 2);
                            yy[2] = 0;
                            memcpy(mm, tempbuf+2, 2);
                            mm[2] = 0;
                            memcpy(dd, tempbuf+4, 2);
                            dd[2] = 0;
                            y = atoi(yy);
                            m = atoi(mm);
                            d = atoi(dd);
                        }
                         else if (field_it->in_type == ddtypeDateMMDDYY)
                        {
                            char yy[3], mm[3], dd[3];
                            memcpy(mm, tempbuf, 2);
                            mm[2] = 0;
                            memcpy(dd, tempbuf+2, 2);
                            dd[2] = 0;
                            memcpy(yy, tempbuf+4, 2);
                            yy[2] = 0;
                            y = atoi(yy);
                            m = atoi(mm);
                            d = atoi(dd);
                        }
                         else if (field_it->in_type == ddtypeDateCCYYMMDD)
                        {
                            char yy[5], mm[3], dd[3];
                            memcpy(yy, tempbuf, 4);
                            yy[4] = 0;
                            memcpy(mm, tempbuf+4, 2);
                            mm[2] = 0;
                            memcpy(dd, tempbuf+6, 2);
                            dd[2] = 0;
                            y = atoi(yy);
                            m = atoi(mm);
                            d = atoi(dd);
                        }
                         else if (field_it->in_type == ddtypeDateCCYY_MM_DD)
                        {
                            char yy[5], mm[3], dd[3];
                            memcpy(yy, tempbuf, 4);
                            yy[4] = 0;
                            memcpy(mm, tempbuf+5, 2);
                            mm[2] = 0;
                            memcpy(dd, tempbuf+8, 2);
                            dd[2] = 0;
                            y = atoi(yy);
                            m = atoi(mm);
                            d = atoi(dd);
                        }
                    }

                    if (y < 100)
                    {
                        if (y <= 60)
                            y += 2000;
                             else
                            y += 1900;
                    }

                    if (y != 0 && m != 0 && y != 0 && m <= 12 && d <= 31)
                    {
                        dt = dateToJulian(y, m, d);
                        dt <<= 32;
                    }

                    inserter->putDateTime(field_it->out_handle, dt);
                }
                break;

            }

        }


        inserter->insertRow();
    }


    delete[] tempbuf;


    if (m_processed_stat.isOk())
    {
        m_processed_stat->setValue(cfw::dbl2fstr(counter));
    }



    for (table_it = tables.begin(); table_it != tables.end(); ++table_it)
    {
        // -- flush inserts --
        table_it->sp_inserter->finishInsert();

        // -- save set objects --
        if (!isCancelling())
        {
            db->storeObject(table_it->sp_set, towstr(table_name));
        }
    }

    return true;
}






int ConvertDataJob::runJob()
{
    std::vector<ConvertDataInstruction>::iterator it;

    for (it = m_instructions.begin(); it != m_instructions.end(); ++it)
    {
        if (!convertTable(*it))
            break;
    }

    return 0;
}







// -- ConvertJob class implementation --

ConvertJob::ConvertJob()
{
    m_job_info->setTitle(_("Data Conversion"));
}

ConvertJob::~ConvertJob()
{

}

void ConvertJob::setInstructions(tango::IDatabasePtr src_db, const wxString& src_table,
                                 tango::IDatabasePtr dest_db, const wxString& dest_table,
                                 cfw::ITreeItemPtr src_item, cfw::ITreeItemPtr dest_item)
{
    m_src_db = src_db;
    m_dest_db = dest_db;
    m_src_table = src_table;
    m_dest_table = dest_table;
    m_src_item = src_item;
    m_dest_item = dest_item;

    m_job_info->deleteStatById(cfw::jobStatMaxCount);
    m_job_info->deleteStatById(cfw::jobStatPercentage);
}





void ConvertJob::createAndSaveSetFromIterator(tango::IDatabasePtr target_db,
                                              const wxString& set_name, 
                                              tango::IIteratorPtr src_iter)
{
    wxBusyCursor bc;

    if (!target_db || !src_iter)
    {
        m_job_info->setState(cfw::jobStateFailed);
        return;
    }

    tango::IStructurePtr src_struct;
    src_struct = src_iter->getStructure();
    if (!src_struct)
    {
        m_job_info->setState(cfw::jobStateFailed);
        return;
    }

    tango::ISetPtr dest_set;
    dest_set = target_db->createSet(towstr(set_name), src_struct, NULL);
    if (!dest_set)
    {
        m_job_info->setState(cfw::jobStateFailed);
        return;
    }


// -- ***** all of this was copied from runJob() - it must not remain in here ***** --


    tango::IStructurePtr dest_struct;
    dest_struct = dest_set->getStructure();
    if (!dest_struct)
    {
        m_job_info->setState(cfw::jobStateFailed);
        return;
    }


    tango::IRowInserterPtr dest_ri;
    dest_ri = dest_set->getRowInserter();
    dest_ri->startInsert(L"*");


    int i;
    std::vector<int> col_types;
    std::vector<tango::objhandle_t> dest_handles;
    std::vector<tango::objhandle_t> src_handles;

    int src_col_count = src_struct->getColumnCount();
    for (i = 0; i < src_col_count; ++i)
    {
        tango::IColumnInfoPtr src_colinfo = src_struct->getColumnInfoByIdx(i);
        src_handles.push_back(src_iter->getHandle(src_colinfo->getName()));

        tango::IColumnInfoPtr dest_colinfo = dest_struct->getColumnInfoByIdx(i);
        col_types.push_back(dest_colinfo->getType());
        dest_handles.push_back(dest_ri->getHandle(dest_colinfo->getName()));
    }


    src_iter->goFirst();
    while (!src_iter->eof())
    {
        if (isCancelling())
        {
            break;
        }

        for (i = 0; i < src_col_count; ++i)
        {
            switch (col_types[i])
            {
                case tango::typeCharacter:
                {
                    dest_ri->putString(dest_handles[i], src_iter->getString(src_handles[i]));
                    break;
                }

                case tango::typeNumeric:
                {
                    dest_ri->putDouble(dest_handles[i], src_iter->getDouble(src_handles[i]));
                    break;
                }

                case tango::typeDouble:
                {
                    dest_ri->putDouble(dest_handles[i], src_iter->getDouble(src_handles[i]));
                    break;
                }

                case tango::typeInteger:
                {
                    dest_ri->putInteger(dest_handles[i], src_iter->getInteger(src_handles[i]));
                    break;
                }

                case tango::typeDateTime:
                {
                    dest_ri->putDateTime(dest_handles[i], src_iter->getDateTime(src_handles[i]));
                    break;
                }

                case tango::typeDate:
                {
                    dest_ri->putDateTime(dest_handles[i], src_iter->getDateTime(src_handles[i]));
                    break;
                }

                case tango::typeBoolean:
                {
                    dest_ri->putBoolean(dest_handles[i], src_iter->getBoolean(src_handles[i]));
                    break;
                }
            }
        }

        dest_ri->insertRow();
        src_iter->skip(1);
        m_job_info->incrementCurrentCount(1.0);
    }

    dest_ri->finishInsert();
}




int ConvertJob::runJob()
{
    char buf[1024];

    cfw::IFileTreeItemPtr file_item = m_src_item;
    if (file_item)
    {
        sprintf(buf, "Converting %s to %s on %s", m_src_table.mbc_str(), m_dest_table.mbc_str(), 
                                                  m_dest_db->getDatabaseName().c_str());
    }
     else
    {
        sprintf(buf, "Converting %s on %s to %s on %s", m_src_table.mbc_str(), m_src_db->getDatabaseName().c_str(),
                                                        m_dest_table.mbc_str(), m_dest_db->getDatabaseName().c_str());
    }

    wxString prog_str = towx(buf);
    m_job_info->setProgressString(prog_str);

    tango::ISetPtr src_set;
    src_set = m_src_db->openSet(towstr(m_src_table));
    if (!src_set)
    {
        m_job_info->setState(cfw::jobStateFailed);
        return 0;
    }


    tango::IStructurePtr src_struct;
    src_struct = src_set->getStructure();
    if (!src_struct)
    {
        m_job_info->setState(cfw::jobStateFailed);
        return 0;
    }


    tango::ISetPtr dest_set;
    dest_set = m_dest_db->createSet(towstr(m_dest_table), src_struct, NULL);
    if (!dest_set)
    {
        m_job_info->setState(cfw::jobStateFailed);
        return 0;
    }


    tango::IStructurePtr dest_struct;
    dest_struct = dest_set->getStructure();
    if (!dest_struct)
    {
        m_job_info->setState(cfw::jobStateFailed);
        return 0;
    }


    tango::IIteratorPtr src_iter;
    src_iter = src_set->createIterator(L"", L"", NULL);


    tango::IRowInserterPtr dest_ri;
    dest_ri = dest_set->getRowInserter();
    dest_ri->startInsert(L"*");


    int i;
    std::vector<int> col_types;
    std::vector<tango::objhandle_t> dest_handles;
    std::vector<tango::objhandle_t> src_handles;

    int src_col_count = src_struct->getColumnCount();
    for (i = 0; i < src_col_count; ++i)
    {
        tango::IColumnInfoPtr src_colinfo = src_struct->getColumnInfoByIdx(i);
        src_handles.push_back(src_iter->getHandle(src_colinfo->getName()));

        tango::IColumnInfoPtr dest_colinfo = dest_struct->getColumnInfoByIdx(i);
        col_types.push_back(dest_colinfo->getType());
        dest_handles.push_back(dest_ri->getHandle(dest_colinfo->getName()));
    }


    src_iter->goFirst();
    while (!src_iter->eof())
    {
        if (isCancelling())
        {
            break;
        }

        for (i = 0; i < src_col_count; ++i)
        {
            switch (col_types[i])
            {
                case tango::typeCharacter:
                {
                    dest_ri->putString(dest_handles[i], src_iter->getString(src_handles[i]));
                    break;
                }

                case tango::typeNumeric:
                {
                    dest_ri->putDouble(dest_handles[i], src_iter->getDouble(src_handles[i]));
                    break;
                }

                case tango::typeDouble:
                {
                    dest_ri->putDouble(dest_handles[i], src_iter->getDouble(src_handles[i]));
                    break;
                }

                case tango::typeInteger:
                {
                    dest_ri->putInteger(dest_handles[i], src_iter->getInteger(src_handles[i]));
                    break;
                }

                case tango::typeDateTime:
                {
                    dest_ri->putDateTime(dest_handles[i], src_iter->getDateTime(src_handles[i]));
                    break;
                }

                case tango::typeDate:
                {
                    dest_ri->putDateTime(dest_handles[i], src_iter->getDateTime(src_handles[i]));
                    break;
                }

                case tango::typeBoolean:
                {
                    dest_ri->putBoolean(dest_handles[i], src_iter->getBoolean(src_handles[i]));
                    break;
                }
            }
        }

        dest_ri->insertRow();
        src_iter->skip(1);
        m_job_info->incrementCurrentCount(1.0);
    }

    dest_ri->finishInsert();

    m_dest_db->storeObject(dest_set, towstr(m_dest_table));
    return 0;
}

void ConvertJob::runPostJob()
{
    if (m_dest_item)
    {
        g_app->getTreeController()->refreshItem(m_dest_item);
    }
     else
    {
        g_app->getTreeController()->refreshAllItems();
    }
}



