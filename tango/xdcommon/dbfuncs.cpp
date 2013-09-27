/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-09-22
 *
 */


#include <kl/klib.h>
#include <ctime>
#include <xd/xd.h>
#include "jobinfo.h"
#include "dbfuncs.h"


struct InsertInfo
{
    bool copy;
    int src_type;
    int src_width;
    xd::objhandle_t src_handle;

    int dest_type;
    int dest_width;
    xd::objhandle_t dest_handle;

    char* buf;
};



int xdcmnInsert(xd::IDatabasePtr dest_db,
                xd::IIteratorPtr sp_source_iter,
                const std::wstring& dest_table,
                const std::wstring& constraint,
                int max_rows,
                xd::IJob* job)
{
    if (sp_source_iter.isNull())
        return 0;

    xd::IIterator* source_iter = sp_source_iter.p;


    // this code assumes for the time being that columns are named the same
    // in the source and destination databases

    xd::IRowInserterPtr sp_insert = dest_db->bulkInsert(dest_table);
    if (sp_insert.isNull())
    {
        return 0;
    }

    xd::IRowInserter* insert = sp_insert.p;

    // get table structure
    xd::IColumnInfoPtr src_colinfo, dest_colinfo;
    xd::IStructurePtr dest_structure = dest_db->describeTable(dest_table);
    xd::IStructurePtr src_structure = source_iter->getStructure();

    if (dest_structure.isNull() || src_structure.isNull())
        return 0;


    int i, col_count = dest_structure->getColumnCount();

    if (!insert->startInsert(L"*"))
    {
        IJobInternalPtr ijob = job;
        if (ijob)
            ijob->setError(xd::errorGeneral, L"cannot initialize inserter object");
        return 0; // error encountered
    }

    InsertInfo* insert_info = new InsertInfo[col_count];

    // allocate column arrays

    int out_idx;
    out_idx = 0;

    for (i = 0; i < col_count; i++)
    {
        dest_colinfo = dest_structure->getColumnInfoByIdx(i);
        if (dest_colinfo.isNull())
            continue;
        if (dest_colinfo->getCalculated())
            continue;

        src_colinfo = src_structure->getColumnInfo(dest_colinfo->getName());
        if (src_colinfo.isNull())
            continue;

        insert_info[out_idx].src_handle = source_iter->getHandle(dest_colinfo->getName());
        if (!insert_info[out_idx].src_handle)
        {
            delete[] insert_info;
            return 0;
        }

        insert_info[out_idx].src_type = src_colinfo->getType();
        insert_info[out_idx].src_width = src_colinfo->getWidth();

        insert_info[out_idx].dest_handle = insert->getHandle(dest_colinfo->getName());
        insert_info[out_idx].dest_type = dest_colinfo->getType();
        insert_info[out_idx].dest_width = dest_colinfo->getWidth();

        if (insert_info[out_idx].src_type == insert_info[out_idx].dest_type &&
            insert_info[out_idx].src_width == insert_info[out_idx].dest_width)
        {
            insert_info[out_idx].copy = true;
        }
         else
        {
            insert_info[out_idx].copy = false;
        }

        insert_info[out_idx].buf = NULL;

        ++out_idx;
    }

    col_count = out_idx;


    // set up job

    IJobInternalPtr ijob;
    xd::rowpos_t cur_count;
    bool max_count_known = false;

    cur_count = 0;

    if (job)
    {
        ijob = job;
        if (!ijob)
        {
            insert->finishInsert();
            return 0;
        }

        if (max_rows > 0)
            ijob->setMaxCount(max_rows);
        ijob->setCurrentCount(0);
        ijob->setStatus(xd::jobRunning);
        ijob->setStartTime(time(NULL));

        if (ijob->getMaxCount() != 0)
            max_count_known = true;
    }



    // if the constraint set is a filter set, we can perform the
    // operation faster by just parsing the filter expression locally
    xd::objhandle_t filter_handle = 0;
    bool result = true;

    if (constraint.length() > 0)
    {
        filter_handle = source_iter->getHandle(constraint);
        if (!filter_handle)
        {
            // error, bad filter expression
            return -1;
        }
    }



    // insertation loop
    while (!source_iter->eof())
    {
        if (filter_handle)
        {
            result = source_iter->getBoolean(filter_handle);
        }


        if (result)
        {
            for (i = 0; i < col_count; i++)
            {
                if (source_iter->isNull(insert_info[i].src_handle))
                {
                    insert->putNull(insert_info[i].dest_handle);
                    continue;
                }            
                        
                switch (insert_info[i].src_type)
                {
                    case xd::typeWideCharacter:
                        insert->putWideString(insert_info[i].dest_handle,
                                          source_iter->getWideString(insert_info[i].src_handle));

                        break;

                    case xd::typeCharacter:
                        insert->putString(insert_info[i].dest_handle,
                                          source_iter->getString(insert_info[i].src_handle));

                        break;

                    case xd::typeDouble:
                    case xd::typeNumeric:
                        insert->putDouble(insert_info[i].dest_handle,
                                          source_iter->getDouble(insert_info[i].src_handle));
                        break;

                    case xd::typeInteger:
                        insert->putInteger(insert_info[i].dest_handle,
                                           source_iter->getInteger(insert_info[i].src_handle));
                        break;

                    case xd::typeDate:
                    case xd::typeDateTime:
                        insert->putDateTime(insert_info[i].dest_handle,
                                            source_iter->getDateTime(insert_info[i].src_handle));
                        break;

                    case xd::typeBoolean:
                        insert->putBoolean(insert_info[i].dest_handle,
                                           source_iter->getBoolean(insert_info[i].src_handle));
                        break;
                }
            }

            if (!insert->insertRow())
            {
                if (job)
                {
                    ijob->setStatus(xd::jobFailed);
                    break;
                }
            }
        }

        source_iter->skip(1);
        cur_count++;

        if (job && (cur_count % 1000) == 0)
        {
            ijob->setCurrentCount(cur_count);

            if (!max_count_known)
            {
                // if the max count isn't known, perhaps we can
                // use IIterator::getPos() to get an approximate percentage
                double pos = source_iter->getPos();
                if (kl::dblcompare(pos, 0.0) != 0)
                    ijob->setPercentage(pos*100);
            }

            if (job->getCancelled())
            {
                break;
            }
        }

        if (cur_count == max_rows)
        {
            break;
        }
    }

    insert->finishInsert();


    if (filter_handle)
    {
        source_iter->releaseHandle(filter_handle);
    }


    for (i = 0; i < col_count; i++)
    {
        if (insert_info[i].src_handle)
        {
            source_iter->releaseHandle(insert_info[i].src_handle);
        }

        if (insert_info[i].buf)
        {
            delete[] insert_info[i].buf;
        }
    }

    delete[] insert_info;


    if (job)
    {
        if (job->getStatus() == xd::jobFailed)
        {
            return cur_count;
        }

        if (!job->getCancelled())
        {
            ijob->setCurrentCount(cur_count);
            ijob->setStatus(xd::jobFinished);
            ijob->setFinishTime(time(NULL));
        }
    }

    return cur_count;
}





bool physStructureEqual(xd::IStructurePtr s1, xd::IStructurePtr s2)
{
    int s1_col_count = s1->getColumnCount();
    int s2_col_count = s2->getColumnCount();
    int i;

    xd::IColumnInfoPtr colinfo;

    std::vector<xd::IColumnInfoPtr> s1_fields;
    std::vector<xd::IColumnInfoPtr> s2_fields;

    // copy physical column infos
    for (i = 0; i < s1_col_count; ++i)
    {
        colinfo = s1->getColumnInfoByIdx(i);

        if (!colinfo->getCalculated())
        {
            s1_fields.push_back(colinfo);
        }
    }

    for (i = 0; i < s2_col_count; ++i)
    {
        colinfo = s2->getColumnInfoByIdx(i);

        if (!colinfo->getCalculated())
        {
            s2_fields.push_back(colinfo);
        }
    }

    if (s1_fields.size() != s2_fields.size())
        return false;

    int count = s1_fields.size();

    for (i = 0; i < count; ++i)
    {
        xd::IColumnInfoPtr col1, col2;

        col1 = s1_fields[i];
        col2 = s2_fields[i];

        if (0 != wcscasecmp(col1->getName().c_str(), col2->getName().c_str()))
            return false;

        if (col1->getType() != col2->getType())
            return false;

        if (col2->getWidth() != col2->getWidth())
            return false;

        if (col2->getScale() != col2->getScale())
            return false;
    }

    return true;
}



