/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-08-22
 *
 */


#include "appmain.h"
#include "jobcopy.h"
#include "appcontroller.h"
#include "tabledoc.h"
#include "jobquery.h"


// -- CopyJob implementation --

CopyJob::CopyJob()
{
    m_job_info->setTitle(_("Copy Records"));
}

CopyJob::~CopyJob()
{
}

void CopyJob::addCopyInstruction(tango::IDatabasePtr source_db,
                                 const wxString& query,
                                 tango::IDatabasePtr dest_db,
                                 const wxString& dest_path)
{
    CopyInstruction ci;
    ci.m_mode = CopyInstruction::modeQuery;
    ci.m_source_db = source_db;
    ci.m_query = query;
    ci.m_dest_db = dest_db;
    ci.m_dest_path = dest_path;

    m_instructions.push_back(ci);
}


void CopyJob::addCopyInstruction(tango::IDatabasePtr source_db,
                                 tango::IIteratorPtr source_iter,
                                 const wxString& condition,
                                 const wxString& columns,
                                 tango::IDatabasePtr dest_db,
                                 const wxString& dest_path,
                                 const wxString& cstr)
{
    CopyInstruction ci;
    ci.m_mode = CopyInstruction::modeIterator;
    ci.m_source_db = source_db;
    ci.m_source_iter = source_iter;
    ci.m_columns = columns;
    ci.m_condition = condition;
    ci.m_dest_db = dest_db;
    ci.m_dest_path = dest_path;
    ci.m_dest_cstr = cstr;

    m_instructions.push_back(ci);
}


void CopyJob::addCopyInstruction(tango::IDatabasePtr source_db,
                                 tango::ISetPtr source_set,
                                 const wxString& columns,
                                 const wxString& condition,
                                 const wxString& order,
                                 tango::IDatabasePtr dest_db,
                                 const wxString& dest_path)
{
    CopyInstruction ci;
    ci.m_mode = CopyInstruction::modeFilterSort;
    ci.m_source_db = source_db;
    ci.m_source_set = source_set;
    ci.m_columns = columns;
    ci.m_condition = condition;
    ci.m_order = order;
    ci.m_dest_db = dest_db;
    ci.m_dest_path = dest_path;

    m_instructions.push_back(ci);
}




size_t CopyJob::getInstructionCount()
{
    return m_instructions.size();
}

tango::ISetPtr CopyJob::getResultSet(size_t idx)
{
    return m_instructions[idx].m_result_set;
}



int CopyJob::runJob()
{
    std::vector<CopyInstruction>::iterator it;
    
    for (it = m_instructions.begin(); it != m_instructions.end(); ++it)
    {
        tango::IIteratorPtr source_iter = xcm::null;
        tango::IStructurePtr copy_structure = xcm::null;
        bool is_stream = false;

        if (it->m_mode == CopyInstruction::modeQuery)
        {
            if (!it->m_query.Left(7).CmpNoCase(wxT("SELECT ")))
            {
                xcm::IObjectPtr result;
                
                m_job_info->setProgressString(_("Querying..."));
                if (!it->m_source_db->execute(towstr(it->m_query), tango::sqlPassThrough, result, NULL))
                {
                    m_job_info->setState(cfw::jobStateFailed);
                    m_job_info->setProgressString(_("ERROR: Source table busy."));
                    return 0;
                }
                
                source_iter = result;
                
                if (!source_iter)
                {
                    m_job_info->setState(cfw::jobStateFailed);
                    m_job_info->setProgressString(_("ERROR: Source table busy."));
                    return 0;
                }

                m_job_info->setProgressString(wxEmptyString);
                source_iter->goFirst();
                copy_structure = source_iter->getStructure();
            }
             else
            {
                m_job_info->setProgressString(_("Querying..."));
                tango::ISetPtr set = it->m_source_db->openSet(towstr(it->m_query));
                m_job_info->setProgressString(wxEmptyString);

                if (set)
                {
                    source_iter = set->createIterator(L"", L"", NULL);
                    it->m_source_iter = source_iter;

                    if (!source_iter)
                    {
                        m_job_info->setState(cfw::jobStateFailed);
                        m_job_info->setProgressString(_("ERROR: Source table busy."));
                        return 0;
                    }
                    
                    source_iter->goFirst();
                    copy_structure = source_iter->getStructure();
                }
                 else
                {
                    // set didn't open, maybe we're copying a stream
                    tango::IFileInfoPtr info = it->m_source_db->getFileInfo(towstr(it->m_query));
                    if (info.isOk())
                    {
                        is_stream = (info->getType() == tango::filetypeStream) ? true : false;
                    }
                    
                    
                    if (!is_stream && set.isNull())
                    {
                        m_job_info->setState(cfw::jobStateFailed);
                        m_job_info->setProgressString(_("ERROR: Source table busy."));
                        return 0;
                    }
                }
            }
        }
         else if (it->m_mode == CopyInstruction::modeIterator)
        {
            source_iter = it->m_source_iter;

            if (it->m_condition.length() != 0)
            {
                // check validity of boolean expression
                tango::objhandle_t h = source_iter->getHandle(towstr(it->m_condition));
                if (h == 0)
                {
                    m_job_info->setState(cfw::jobStateFailed);
                    m_job_info->setProgressString(_("ERROR: Invalid condition."));
                    return 0;
                }
                source_iter->releaseHandle(h);
            }
        }
         else if (it->m_mode == CopyInstruction::modeFilterSort)
        {
            wxString sql = writeQueryFromFilterSort(it->m_source_db,
                                                    towx(it->m_source_set->getObjectPath()),
                                                    it->m_condition,
                                                    it->m_order);
                                                    
            m_job_info->setProgressString(_("Querying..."));
            xcm::IObjectPtr result;
            it->m_source_db->execute(towstr(sql), tango::sqlPassThrough, result, NULL);
            source_iter = result;
            m_job_info->setProgressString(wxEmptyString);
            
            if (source_iter.isNull())
            {
                m_job_info->setState(cfw::jobStateFailed);
                m_job_info->setProgressString(_("ERROR: Query failed."));
                return 0;
            }
        }
         else
        {
            // unknown mode
            continue;
        }

        // if we're copying a stream, do it here
        if (is_stream)
        {
            std::wstring mime_type;
            
            tango::IFileInfoPtr src_info = it->m_source_db->getFileInfo(towstr(it->m_query));
            if (src_info)
            {
                mime_type = src_info->getMimeType();
                src_info.clear();
            }
        
        
            tango::IStreamPtr dest = it->m_dest_db->createStream(towstr(it->m_dest_path), mime_type);
            tango::IStreamPtr src = it->m_source_db->openStream(towstr(it->m_query));
    
    
            if (src.isNull())
            {
                m_job_info->setState(cfw::jobStateFailed);
                m_job_info->setProgressString(_("ERROR: Could not create input file"));
                return 0;
            }
                    
            if (dest.isNull())
            {
                m_job_info->setState(cfw::jobStateFailed);
                m_job_info->setProgressString(_("ERROR: Could not create output file"));
                return 0;
            }
            
            
            if (src)
            {
                unsigned char* buf = new unsigned char[32768];
                unsigned long read, written;
                
                while (1)
                {
                    if (!src->read(buf, 32768, &read))
                        break;
                    if (read == 0)
                        break;
                      
                    written = 0;
                    dest->write(buf, read, &written);
                    
                    if (written != read)
                    {
                        m_job_info->setState(cfw::jobStateFailed);
                        m_job_info->setProgressString(_("ERROR: Could not create output file"));
                        break;
                    }
                    
                    if (read != 32768)
                        break;
                }
            }
            
            continue;
        }
        

        
        if (it->m_mode == CopyInstruction::modeIterator ||
            it->m_mode == CopyInstruction::modeFilterSort)
        {
            // determine output structure
            
            if (it->m_columns.IsEmpty())
            {
                // copy all fields
                copy_structure = source_iter->getStructure();
            }
             else
            {
                tango::IStructurePtr iter_structure = source_iter->getStructure();

                copy_structure = g_app->getDatabase()->createStructure();

                wxStringTokenizer t(it->m_columns, wxT(","));
                while (t.HasMoreTokens())
                {
                    wxString col = t.GetNextToken();

                    tango::IColumnInfoPtr colinfo;
                    colinfo = iter_structure->getColumnInfo(towstr(col));
                    if (colinfo.isNull())
                    {
                        getJobInfo()->setState(cfw::jobStateFailed);
                        return 0;
                    }

                    tango::IColumnInfoPtr newcol = copy_structure->createColumn();
                    newcol->setName(colinfo->getName());
                    newcol->setType(colinfo->getType());
                    newcol->setWidth(colinfo->getWidth());
                    newcol->setScale(colinfo->getScale());
                    newcol->setExpression(colinfo->getExpression());
                    newcol->setCalculated(colinfo->getCalculated());
                }
            }
        }




        
        
        // on fields where the width cannot be known, set it to some high value
        
        int i, col_count = copy_structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr colinfo = copy_structure->getColumnInfoByIdx(i);
            if (colinfo->getWidth() < 0)
                colinfo->setWidth(4096);
        }
        


        tango::ISetPtr result_set;
        it->m_result_set = it->m_dest_db->createSet(towstr(it->m_dest_path),
                                                    copy_structure,
                                                    NULL);
        if (it->m_result_set.isNull())
        {
            m_job_info->setState(cfw::jobStateFailed);
            m_job_info->setProgressString(_("ERROR: Could not create output table"));
            return 0;
        }

        // FIXME: job object should be created by the source_db, not g_app's db
        tango::IJobPtr tango_job = g_app->getDatabase()->createJob();
        setTangoJob(tango_job);
        it->m_result_set->insert(source_iter, towstr(it->m_condition), 0, tango_job);
        
        if (tango_job->getCancelled())
        {
            return 0;
        }

        if (tango_job->getStatus() == tango::jobFailed)
        {
            m_job_info->setState(cfw::jobStateFailed);
            m_job_info->setProgressString(_("ERROR: Insufficient disk space"));
            return 0;
        }


        if (it->m_dest_cstr.Length() > 0 && it->m_source_db.isOk())
        {
            std::wstring link_output_path;
            if (link_output_path.length() == 0)
                link_output_path = L"/.temp/" + towstr(makeUniqueString());

            if (!it->m_source_db->setMountPoint(link_output_path, towstr(it->m_dest_cstr), it->m_result_set->getObjectPath()))
                return xcm::null;

            tango::ISetPtr link_set = it->m_source_db->openSet(link_output_path);
            if (link_set.isOk())
            {
                it->m_result_set = link_set;
            }
        }
    }


    for (it = m_instructions.begin(); it != m_instructions.end(); ++it)
    {
        // copy the old table doc model, if any, to the new set
        if (it->m_source_iter)
        {
            tango::ISetPtr source_set = it->m_source_iter->getSet();

            if (source_set)
            {
                TableDocMgr::copyModel(source_set,
                                       it->m_result_set);
            }
        }
    }


    return 0;
}

