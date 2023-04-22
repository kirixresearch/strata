/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-20
 *
 */


#include <ctime>
#include <kl/klib.h>
#include <xd/xd.h>
#include "xdnative_private.h"
#include "util.h"
#include "nativetbl.h"
#include "tablesetmodify.h"
#include "tableset.h"
#include "../xdcommon/xdcommon.h"


struct ModifyField
{
    xd::objhandle_t src_handle;
    xd::objhandle_t dest_handle;

    bool calculated;

    std::wstring dest_name;
    int dest_type;
    int dest_width;
    int dest_scale;
    std::wstring dest_expr;

    std::wstring src_name;
    int src_type;
    int src_width;
    int src_scale;
    bool src_calc;
    std::wstring src_expr;


    ModifyField()
    {
        src_handle = 0;
        dest_handle = 0;
        calculated = false;

        dest_name = L"";
        dest_type = 0;
        dest_width = 0;
        dest_scale = 0;
        dest_expr = L"";

        src_name = L"";
        src_type = 0;
        src_width = 0;
        src_scale = 0;
        src_expr = L"";
        src_calc = false;
    }

    ModifyField(const ModifyField& c)
    {
        src_handle = c.src_handle;
        dest_handle = c.dest_handle;
        calculated = c.calculated;

        dest_name = c.dest_name;
        dest_type = c.dest_type;
        dest_width = c.dest_width;
        dest_scale = c.dest_scale;
        dest_expr = c.dest_expr;

        src_name = c.src_name;
        src_type = c.src_type;
        src_width = c.src_width;
        src_scale = c.src_scale;
        src_expr = c.src_expr;
        src_calc = c.src_calc;
    }
    
    ModifyField& operator=(const ModifyField& c)
    {
        src_handle = c.src_handle;
        dest_handle = c.dest_handle;
        calculated = c.calculated;

        dest_name = c.dest_name;
        dest_type = c.dest_type;
        dest_width = c.dest_width;
        dest_scale = c.dest_scale;
        dest_expr = c.dest_expr;

        src_name = c.src_name;
        src_type = c.src_type;
        src_width = c.src_width;
        src_scale = c.src_scale;
        src_expr = c.src_expr;
        src_calc = c.src_calc;
        
        return *this;
    }
};




inline void doCopy(xd::IRowInserter* dest,
                   xd::IIterator* src,
                   const ModifyField& ci)
{
    switch (ci.src_type)
    {
        case xd::typeWideCharacter:
        {
            if (ci.dest_type == xd::typeWideCharacter)
            {
                dest->putWideString(ci.dest_handle, src->getWideString(ci.src_handle));
                break;
            }
        }
        // fall through
        
        case xd::typeCharacter:
        {
            switch (ci.dest_type)
            {
                case xd::typeCharacter:
                case xd::typeWideCharacter:
                {
                    dest->putString(ci.dest_handle, src->getString(ci.src_handle));
                }
                break;

                case xd::typeDouble:
                case xd::typeNumeric:
                {
                    dest->putDouble(ci.dest_handle, kl::nolocale_atof(src->getString(ci.src_handle)));
                }
                break;

                case xd::typeInteger:
                {
                    dest->putInteger(ci.dest_handle, atoi(src->getString(ci.src_handle).c_str()));
                }
                break;

                case xd::typeDate:
                case xd::typeDateTime:
                {
                    // note: similar to way numbers are converted to dates
                    // if this logic is changed, change logic for how numbers
                    // are converted
                
                    int y, m, d, hh, mm, ss;
                    if (parseDateTime(src->getWideString(ci.src_handle), &y, &m, &d, &hh, &mm, &ss))
                    {
                        if (hh == -1 || mm == -1)
                        {
                            hh = 0;
                            mm = 0;
                            ss = 0;
                        }
                        
                        xd::DateTime dt(y, m, d, hh, mm, ss);
                        dest->putDateTime(ci.dest_handle, dt);
                    }
                }
                break;

                case xd::typeBoolean:
                {
                    // get the value and convert it to upper for a case-insensitive comparison
                    std::string value = src->getString(ci.src_handle);
                    kl::makeUpper(value);
                    
                    // T, TRUE and related case variants go to true; others go to false
                    if (value == "T" || value == "TRUE")
                        dest->putBoolean(ci.dest_handle, true);
                          else
                        dest->putBoolean(ci.dest_handle, false);
                }
                break;
            }
        }

        break;

        case xd::typeInteger:
        case xd::typeDouble:
        case xd::typeNumeric:
        {
            switch (ci.dest_type)
            {
                case xd::typeNumeric:
                case xd::typeInteger:
                case xd::typeDouble:
                {
                    if (ci.src_type == xd::typeDouble)
                    {
                        // this is a small workaround for when the user changes
                        // both type and numeric scale at the same time.  Using
                        // getRawPtr will avoid getDouble()'s rounding

                        dest->putDouble(ci.dest_handle,
                                   *((double*)src->getRawPtr(ci.src_handle)));
                    }
                     else
                    {
                        dest->putDouble(ci.dest_handle,
                                        src->getDouble(ci.src_handle));
                    }
                }
                break;

                case xd::typeCharacter:
                {
                    char buf[64];
                    
                    if (ci.src_type == xd::typeInteger)
                    {
                        snprintf(buf, 64, "%d", src->getInteger(ci.src_handle));
                        buf[63] = 0;
                    }
                     else
                    {
                        snprintf(buf, 64, "%.*f",
                                     ci.src_scale,
                                     src->getDouble(ci.src_handle));
                        buf[63] = 0;
                    }

                    dest->putString(ci.dest_handle, buf);
                }
                break;

                case xd::typeWideCharacter:
                {
                    wchar_t buf[64];
                    
                    if (ci.src_type == xd::typeInteger)
                    {
                        swprintf(buf, 64, L"%d", src->getInteger(ci.src_handle));
                    }
                     else
                    {
                        swprintf(buf, 64, L"%.*f",
                                          ci.src_scale,
                                          src->getDouble(ci.src_handle));
                    }

                    dest->putWideString(ci.dest_handle, buf);
                }
                break;

                case xd::typeDateTime:
                case xd::typeDate:
                {
                    // note: similar to way strings are converted to dates
                    // if this logic is changed, change logic for how strings
                    // are converted; TODO: somewhat inefficient

                    // convert the number to a string
                    wchar_t buf[64];
                    
                    if (ci.src_type == xd::typeInteger)
                    {
                        swprintf(buf, 64, L"%d", src->getInteger(ci.src_handle));
                    }
                     else
                    {
                        swprintf(buf, 64, L"%.*f",
                                          ci.src_scale,
                                          src->getDouble(ci.src_handle));
                    }

                    // convert the string to a date
                    std::wstring str(buf);
                    int y, m, d, hh, mm, ss;
                    if (parseDateTime(str, &y, &m, &d, &hh, &mm, &ss))
                    {
                        if (hh == -1 || mm == -1)
                        {
                            hh = 0;
                            mm = 0;
                            ss = 0;
                        }

                        xd::DateTime dt(y, m, d, hh, mm, ss);
                        dest->putDateTime(ci.dest_handle, dt);
                    }
                }
                break;

                case xd::typeBoolean:
                    
                    if (!kl::dblcompare(src->getDouble(ci.src_handle), 0.0))
                    {
                        dest->putBoolean(ci.dest_handle, false);
                    }
                     else
                    {
                        dest->putBoolean(ci.dest_handle, true);
                    }

                    break;
            }
        }
        break;


        case xd::typeDateTime:
        {
            switch (ci.dest_type)
            {
                case xd::typeCharacter:
                {
                    char buf[64];
                    int y, m, d, hh, mm, ss;

                    xd::DateTime dt = src->getDateTime(ci.src_handle);
                    y = dt.getYear();
                    m = dt.getMonth();
                    d = dt.getDay();
                    hh = dt.getHour();
                    mm = dt.getMinute();
                    ss = dt.getSecond();

                    if (!dt.isNull())
                    {
                        sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
                        dest->putString(ci.dest_handle, buf);
                    }
                }
                break;

                case xd::typeWideCharacter:
                {
                    wchar_t buf[64];
                    int y, m, d, hh, mm, ss;

                    xd::DateTime dt = src->getDateTime(ci.src_handle);
                    y = dt.getYear();
                    m = dt.getMonth();
                    d = dt.getDay();
                    hh = dt.getHour();
                    mm = dt.getMinute();
                    ss = dt.getSecond();

                    if (!dt.isNull())
                    {
                        swprintf(buf, 64, L"%04d-%02d-%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
                        dest->putWideString(ci.dest_handle, buf);
                    }
                }
                break;

                case xd::typeDate:
                case xd::typeDateTime:
                    dest->putDateTime(ci.dest_handle, src->getDateTime(ci.src_handle));
                    break;
            }
        }
        break;
        
        case xd::typeDate:
        {
            switch (ci.dest_type)
            {
                case xd::typeCharacter:
                {
                    char buf[64];
                    int y, m, d;

                    xd::DateTime dt = src->getDateTime(ci.src_handle);
                    y = dt.getYear();
                    m = dt.getMonth();
                    d = dt.getDay();

                    if (!dt.isNull())
                    {
                        sprintf(buf, "%04d-%02d-%02d", y, m, d);
                        dest->putString(ci.dest_handle, buf);
                    }
                }
                break;

                case xd::typeWideCharacter:
                {
                    wchar_t buf[64];
                    int y, m, d;

                    xd::DateTime dt = src->getDateTime(ci.src_handle);
                    y = dt.getYear();
                    m = dt.getMonth();
                    d = dt.getDay();

                    if (!dt.isNull())
                    {
                        swprintf(buf, 64, L"%04d-%02d-%02d", y, m, d);
                        dest->putWideString(ci.dest_handle, buf);
                    }
                }
                break;

                case xd::typeDate:
                case xd::typeDateTime:
                    dest->putDateTime(ci.dest_handle, src->getDateTime(ci.src_handle));
                    break;
            }
        }
        break;

        case xd::typeBoolean:
        {
            switch (ci.dest_type)
            {
                case xd::typeCharacter:
                    if (src->getBoolean(ci.src_handle))
                    {
                        dest->putString(ci.dest_handle, "T");
                    }
                     else
                    {
                        dest->putString(ci.dest_handle, "F");
                    }
                    break;

                case xd::typeWideCharacter:
                    if (src->getBoolean(ci.src_handle))
                    {
                        dest->putWideString(ci.dest_handle, L"T");
                    }
                     else
                    {
                        dest->putWideString(ci.dest_handle, L"F");
                    }
                    break;

                case xd::typeDouble:
                case xd::typeNumeric:
                case xd::typeInteger:
                    if (src->getBoolean(ci.src_handle))
                    {
                        dest->putDouble(ci.dest_handle, 1.0);
                    }
                     else
                    {
                        dest->putDouble(ci.dest_handle, 0.0);
                    }
                    break;
                
                case xd::typeBoolean:
                    dest->putBoolean(ci.dest_handle, src->getBoolean(ci.src_handle));
                    break;
                     
            }
        }

        break;
    }

}




class JobInfoManager
{
private:
    IJobInternalPtr ijob;

public:

    JobInfoManager(xd::IJob* job)
    {
        ijob = job;

        if (ijob)
        {
            ijob->setStartTime(time(NULL));
            ijob->setStatus(xd::jobRunning);
            ijob->setCanCancel(true);
            ijob->setMaxCount(0);
            ijob->setCurrentCount(0);
        }
    }

    ~JobInfoManager()
    {
        if (ijob)
        {
            xd::IJobPtr job = ijob;

            // set the job to finished
            ijob->setFinishTime(time(NULL));

            if (job->getStatus() == xd::jobRunning)
            {
                ijob->setStatus(xd::jobFinished);

                int max_count = (int)job->getMaxCount();
                if (max_count > 0)
                {
                    ijob->setCurrentCount(max_count);
                }
            }

        }
    }
};




bool TableSet::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    IJobInternalPtr ijob = job;

    std::vector<xd::StructureModify::Action>::const_iterator it_sa;
    xd::Structure src_structure = getStructure();


    // start the job
    JobInfoManager jobinfo_mgr(job);



    // drop all indexes that are based on the columns
    // being modified or dropped

    for (it_sa = mod_params.actions.cbegin(); it_sa != mod_params.actions.cend(); ++it_sa)
    {
        switch (it_sa->action)
        {
            case xd::StructureModify::Action::actionModify:
            case xd::StructureModify::Action::actionDelete:
            {
                KL_AUTO_LOCK(m_update_mutex);

                std::vector<IndexEntry>::iterator idx_it;
                std::vector<IIndex*> to_delete;
                std::vector<IIndex*>::iterator temp_it;

                for (idx_it = m_indexes.begin();
                     idx_it != m_indexes.end();
                     ++idx_it)
                {
                    if (findFieldInExpr(it_sa->column,
                                        idx_it->expr,
                                        src_structure,
                                        true))
                    {
                        to_delete.push_back(idx_it->index);
                    }
                }

                for (temp_it = to_delete.begin();  temp_it != to_delete.end(); ++temp_it)
                {
                    deleteIndexInternal(*temp_it);
                }
            }
            break;
        }
    }




    // modify the base set, which will take care of dynamic
    // field modification; after that, if we are done, then leave

    bool done = false;

    if (!BaseSet::baseSetModifyStructure(mod_params, &done))
        return false;

    if (done)
    {
        refreshUpdateBuffer();
        refreshIndexEntries();
        m_table->setStructureModified();
        fire_onSetStructureUpdated();
        return true;
    }


    // get row count information and set job info
    xd::rowpos_t phys_row_count;
    xd::rowpos_t deleted_row_count;
    phys_row_count = m_table->getRowCount(&deleted_row_count);

    if (job)
    {
        ijob->setMaxCount(phys_row_count - deleted_row_count);
    }

    // attempt to lock the set
    if (!m_database->lockSet(getSetId()))
    {
        fire_onSetStructureUpdated();

        if (ijob.isOk())
        {
            ijob->setStatus(xd::jobFailed);
        }

        return false;
    }

    // attempt to reopen the set exclusively
    if (!m_table->reopen(true))
    {
        // unlock set
        m_database->unlockSet(getSetId());

        if (ijob.isOk())
        {
            ijob->setStatus(xd::jobFailed);
        }

        return false;
    }


    std::vector<ModifyField> modfields;
    std::vector<ModifyField>::iterator it_mf;
    ModifyField mf;

    size_t i, col_count = src_structure.getColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& col_info = src_structure.getColumnInfoByIdx(i);

        mf.calculated = col_info.calculated;

        mf.dest_name = col_info.name;
        mf.dest_type = col_info.type;
        mf.dest_width = col_info.width;
        mf.dest_scale = col_info.scale;

        mf.src_name = mf.dest_name;
        mf.src_type = mf.dest_type;
        mf.src_width = mf.dest_width;
        mf.src_scale = mf.dest_scale;

        modfields.push_back(mf);
    }


    bool write_all = false;
    std::vector<std::wstring> makeperm_fields;

    for (it_sa = mod_params.actions.cbegin(); it_sa != mod_params.actions.cend(); ++it_sa)
    {
        // we've already handled calculated fields upstream in
        // BaseSet::modifyStructure, so don't perform non-delete
        // work here or we'll end up with a "shadow" real field 
        // corresponding to the duplicate
        if (it_sa->action != xd::StructureModify::Action::actionDelete)
        {
            if (it_sa->params.calculated)
                continue;
        }

        switch (it_sa->action)
        {
            case xd::StructureModify::Action::actionCreate:
            {
                ModifyField mf;

                mf.dest_name = it_sa->params.name;
                mf.dest_type = it_sa->params.type;
                mf.dest_width = it_sa->params.width;
                mf.dest_scale = it_sa->params.scale;

                int pos = it_sa->params.column_ordinal;
                if (pos < 0)
                    pos = modfields.size();
                    
                if (mf.dest_type == xd::typeNumeric &&
                    mf.dest_width > native_max_numeric_width)
                {
                    mf.dest_width = native_max_numeric_width;
                }

                if ((size_t)pos < modfields.size())
                    modfields.insert(modfields.begin() + pos, mf);
                     else
                    modfields.push_back(mf);

                write_all = true;
            }
            break;

            case xd::StructureModify::Action::actionModify:
            {
                for (it_mf = modfields.begin(); it_mf != modfields.end(); ++it_mf)
                {
                    if (kl::iequals(it_mf->src_name, it_sa->column))
                    {
                        if (it_sa->params.mask & xd::ColumnInfo::maskName)
                        {
                            it_mf->dest_name = it_sa->params.name;
                        }

                        if (it_sa->params.mask & xd::ColumnInfo::maskType)
                        {
                            it_mf->dest_type = it_sa->params.type;
                            write_all = true;
                        }

                        if (it_sa->params.mask & xd::ColumnInfo::maskWidth)
                        {
                            it_mf->dest_width = it_sa->params.width;
                            write_all = true;

                            if ((it_mf->dest_type == -1 && it_mf->src_type == xd::typeNumeric) ||
                                (it_mf->dest_type == xd::typeNumeric))
                            {
                                if (it_mf->dest_width > native_max_numeric_width)
                                {
                                    it_mf->dest_width = native_max_numeric_width;
                                }
                            }
                        }

                        if (it_sa->params.mask & xd::ColumnInfo::maskScale)
                        {
                            it_mf->dest_scale = it_sa->params.scale;

                            if (it_mf->src_type != xd::typeDouble)
                            {
                                // if we are changing the scale of a double,
                                // we don't need to write the whole file out
                                write_all = true;
                            }
                        }

                        if ((it_sa->params.mask & xd::ColumnInfo::maskCalculated) && !it_sa->params.calculated && it_mf->calculated)
                        {
                            // "make permanent" operation
                            it_mf->calculated = false;
                            write_all = true;
                            makeperm_fields.push_back(it_sa->column);
                        }
                        
                        if (it_sa->params.mask & xd::ColumnInfo::maskColumnOrdinal)
                        {
                            int desired_pos = it_sa->params.column_ordinal;
                            if (desired_pos < 0)
                                desired_pos = 0;
                        
                            if ((size_t)desired_pos < modfields.size())
                            {
                                if (&(*it_mf) != &modfields[desired_pos])
                                {
                                    // swap positions
                                    ModifyField temp = modfields[desired_pos];
                                    modfields[desired_pos] = *it_mf;
                                    *it_mf = temp;
                                }
                            }
                             else
                            {
                                ModifyField f = *it_mf;
                                modfields.erase(it_mf);
                                modfields.push_back(f);
                            }
                            
                            write_all = true;
                        }

                        break;
                    }
                }
            }
            break;

            case xd::StructureModify::Action::actionDelete:
            {
                for (it_mf = modfields.begin(); it_mf != modfields.end(); ++it_mf)
                {
                    if (kl::iequals(it_mf->src_name, it_sa->column))
                    {
                        write_all = true;
                        modfields.erase(it_mf);
                        break;
                    }
                }
            }
            break;

            case xd::StructureModify::Action::actionMove:
            {
                // right now this is unimplemented
                write_all = true;
            }
            break;
        }
    }



    if (!write_all)
    {
        for (it_sa = mod_params.actions.cbegin(); it_sa != mod_params.actions.cend(); ++it_sa)
        {
            if (it_sa->action == xd::StructureModify::Action::actionModify)
            {
                // unchanged parameters are -1 for integers and empty strings
                // for character values, and thus will not be changed in the
                // writeRowInfo() call

                int idx = -1;

                for (i = 0; i < col_count; ++i)
                {
                    if (kl::iequals(src_structure.getColumnName(i), it_sa->column))
                    {
                        idx = (int)i;
                        break;
                    }
                }

                if (idx == -1)
                {
                    // column not found
                    continue;
                }

                const xd::ColumnInfo& col = src_structure.getColumnInfoByIdx(idx);
                if (col.calculated)
                {
                    // skip calculated fields (they have already been processed)
                    continue;
                }

                m_table->writeColumnInfo(idx,
                                         (it_sa->params.mask & xd::ColumnInfo::maskName) ? it_sa->params.name : L"",
                                         (it_sa->params.mask & xd::ColumnInfo::maskType) ? it_sa->params.type : -1,
                                         (it_sa->params.mask & xd::ColumnInfo::maskWidth) ? it_sa->params.width : -1,
                                         (it_sa->params.mask & xd::ColumnInfo::maskScale) ? it_sa->params.scale : -1);
            }
        }


        refreshUpdateBuffer();
        deleteAllIndexes();
        refreshIndexEntries();

        // unlock set
        m_database->unlockSet(getSetId());
        fire_onSetStructureUpdated();

        // done
        return true;
    }





    // delete all calculated fields from the physical copy info

    int delete_count;
    while (1)
    {
        delete_count = 0;

        for (it_mf = modfields.begin(); it_mf != modfields.end(); ++it_mf)
        {
            if (it_mf->calculated)
            {
                modfields.erase(it_mf);
                delete_count++;
                break;
            }
        }

        if (delete_count == 0)
        {
            break;
        }
    }



    // create output table
    xd::FormatDefinition output_structure;

    for (it_mf = modfields.begin(); it_mf != modfields.end(); ++it_mf)
    {
        xd::ColumnInfo col_info;

        col_info.name = it_mf->dest_name;
        col_info.type = it_mf->dest_type;
        col_info.width = it_mf->dest_width;
        col_info.scale = it_mf->dest_scale;

        output_structure.createColumn(col_info);
    }


    // generate a unique filename for the table
    std::wstring table_filename = m_database->getUniqueFilename();
    table_filename += L".ttb";

    // create the table
    if (!NativeTable::create(table_filename, output_structure))
    {
        // unlock set
        m_database->unlockSet(getSetId());

        if (ijob.isOk())
        {
            ijob->setStatus(xd::jobFailed);
        }

        return false;
    }


    // load up the table
    TableSet* tbl_set = new TableSet(m_database);
    tbl_set->ref();

    if (!tbl_set->loadTable(table_filename))
    {
        tbl_set->unref();

        // unlock set
        m_database->unlockSet(getSetId());

        if (ijob.isOk())
        {
            ijob->setStatus(xd::jobFailed);
        }

        return false;
    }



    // create a row inserter for the output set
    xd::IRowInserterPtr sp_output_inserter = tbl_set->getRowInserter();
    if (sp_output_inserter.isNull())
    {
        // unlock set
        m_database->unlockSet(getSetId());

        if (ijob.isOk())
        {
            ijob->setStatus(xd::jobFailed);
        }

        return false;
    }

    xd::IRowInserter* output_inserter = sp_output_inserter.p;


    // create an unordered iterator for the input set
    // (including deleted records)

    xd::IIteratorPtr sp_iter = createPhysicalIterator(L"", true);
    if (sp_iter.isNull())
    {
        // unlock set
        m_database->unlockSet(getSetId());

        if (ijob.isOk())
        {
            ijob->setStatus(xd::jobFailed);
        }

        return false;
    }
    xd::IIterator* iter = sp_iter.p;


    // get input and output handles
    
    for (it_mf = modfields.begin(); it_mf != modfields.end(); ++it_mf)
    {
        it_mf->src_handle = 0;
        it_mf->dest_handle = 0;

        if (it_mf->src_name.length() > 0)
        {
            it_mf->src_handle = iter->getHandle(it_mf->src_name);
            if (it_mf->src_handle == 0)
            {
                // unlock set
                m_database->unlockSet(getSetId());

                if (ijob.isOk())
                {
                    ijob->setStatus(xd::jobFailed);
                }

                return false;
            }
        }

        it_mf->dest_handle = output_inserter->getHandle(it_mf->dest_name);
        if (it_mf->dest_handle == 0)
        {
            // unlock set
            m_database->unlockSet(getSetId());

            if (ijob.isOk())
            {
                ijob->setStatus(xd::jobFailed);
            }

            return false;
        }
    }


    xd::rowpos_t rows_processed = 0;
    bool cancelled = false;


    std::vector<ModifyField>::iterator mf_begin = modfields.begin();
    std::vector<ModifyField>::iterator mf_end = modfields.end();

    
    iter->goFirst();

    bool failed = false;

    output_inserter->startInsert(L"*");

    while (!iter->eof())
    {

        for (it_mf = mf_begin; it_mf != mf_end; ++it_mf)
        {
            if (it_mf->src_handle)
            {
                doCopy(output_inserter, iter, *it_mf);
            }
        }

        ++rows_processed;
        if (rows_processed % 1000 == 0)
        {
            if (job)
            {
                if (deleted_row_count == 0)
                {
                    ijob->setCurrentCount(rows_processed);
                }
                 else
                {
                    double r = (double)(long long)rows_processed;
                    double p = (double)(long long)phys_row_count;
                    double t = (double)(long long)(phys_row_count - deleted_row_count);

                    ijob->setCurrentCount((xd::rowpos_t)(((r*t)/p)));
                }

                if (job->getCancelled())
                {
                    cancelled = true;
                    break;
                }
            }
        }

        if (!output_inserter->insertRow())
        {
            if (ijob)
            {
                ijob->setStatus(xd::jobFailed);
            }

            failed = true;
            break;
        }

        iter->skip(1);
    }

    
    output_inserter->finishInsert();


    if (cancelled || failed)
    {
        tbl_set->unref();
        xf_remove(table_filename);

        // unlock set
        m_database->unlockSet(getSetId());

        if (ijob.isOk() && !cancelled)
        {
            ijob->setStatus(xd::jobFailed);
        }

        return false;
    }


    // remove all calculated fields which were made permanent
    std::vector<std::wstring>::iterator makeperm_it;
    for (makeperm_it = makeperm_fields.begin(); makeperm_it != makeperm_fields.end(); ++makeperm_it)
    {
        deleteCalcField(*makeperm_it);
    }


    sp_iter = xcm::null;
    sp_output_inserter = xcm::null;

    // close output table
    tbl_set->unref();


    // re-assign ordinal's table
    if (!m_database->setOrdinalTable(m_ordinal, table_filename))
    {
        // unlock set
        m_database->unlockSet(getSetId());

        if (ijob.isOk())
        {
            ijob->setStatus(xd::jobFailed);
        }

        return false;
    }


    // attempt to get rid of the old table
    std::wstring old_filename = m_table->getFilename();
    std::wstring old_mapfilename = m_table->getMapFilename();
    m_table->close();


    // rename map filename
    std::wstring new_mapfilename = kl::beforeLast(table_filename, L'.');
    new_mapfilename += L".map";
    if (xf_get_file_exist(new_mapfilename))
    {
        xf_remove(new_mapfilename);
    }
    xf_move(old_mapfilename, new_mapfilename);


    // assign the new output table to this set
    m_table->open(table_filename, m_ordinal);
    xf_remove(old_filename);

    m_structure = m_table->getStructure();

    // for now, all indexes are deleted with modify structure
    refreshUpdateBuffer();
    deleteAllIndexes();
    refreshIndexEntries();

    save();

    m_table->setStructureModified();

    // unlock set
    m_database->unlockSet(getSetId());
    fire_onSetStructureUpdated();

    return true;
}






