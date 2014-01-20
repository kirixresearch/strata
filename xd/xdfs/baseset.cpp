/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2006-03-20
 *
 */


#include "xdfs.h"
#include <xd/util.h>
#include <kl/json.h>
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/extfileinfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/keylayout.h"
#include "database.h"
#include "baseset.h"





XdfsBaseSet::XdfsBaseSet(FsDatabase* database)
{
    m_database = database;
    m_database->ref();
}

XdfsBaseSet::~XdfsBaseSet()
{
    m_indexes_mutex.lock();
    std::vector<XdfsIndexEntry>::iterator it;
    for (it = m_indexes.begin(); it != m_indexes.end(); ++it)
    {
        if (it->index)
            it->index->unref();

        if (it->orig_key)
            delete[] it->orig_key;

        if (it->key_expr)
            delete it->key_expr;
    }
    m_indexes_mutex.unlock();

    m_database->unref();
}

void XdfsBaseSet::setObjectPath(const std::wstring& path)
{
    m_object_path = path;
}

std::wstring XdfsBaseSet::getObjectPath()
{
    return m_object_path;
}

void XdfsBaseSet::setConfigFilePath(const std::wstring& path)
{
    m_config_file_path = path;
}


bool XdfsBaseSet::modifyStructure(xd::IStructure* struct_config,
                                  bool* done_flag)
{
    KL_AUTO_LOCK(m_object_mutex);

    *done_flag = false;

    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    int processed_action_count = 0;

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        if (deleteCalcField(it->m_colname))
            processed_action_count++;
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        if (modifyCalcField(it->m_colname, it->m_params))
            processed_action_count++;
    }

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            if (createCalcField(it->m_params))
                processed_action_count++;
        }
    }

    if (processed_action_count == actions.size())
    {
        // we have handled all actions, so we're done
        *done_flag = true;
    }
    
    return true;
}




void XdfsBaseSet::refreshIndexEntries()
{
    // get object id from path
    if (m_object_id.empty())
    {
        xd::IFileInfoPtr info = m_database->getFileInfo(m_object_path);
        if (info.isNull())
            return;

        m_object_id = info->getObjectId();
    }


    std::wstring index_registry_file = m_database->m_ctrl_path + xf_path_separator_wchar + L"indexes" + xf_path_separator_wchar + m_object_id + L".info";

    if (!xf_get_file_exist(index_registry_file))
        return;

    // check to see if index registry file is newer than last time we checked
    xf_filetime_t ft = xf_get_file_modify_time(index_registry_file);
    if (ft <= m_indexes_filetime)
        return;
    m_indexes_filetime = ft;




    std::vector<XdfsIndexEntry> entries;
    std::vector<XdfsIndexEntry>::iterator it, it_end, it2;

    kl::exclusive_file file(index_registry_file);
    if (!file.isOk())
        return;

    std::wstring json = file.getContents();

    kl::JsonNode root;
    if (json.empty() || !root.fromString(json))
        return;

    kl::JsonNode indexes = root["indexes"];
    std::vector<kl::JsonNode> index_nodes = indexes.getChildren();

    std::wstring name, expr, filename;

    std::vector<kl::JsonNode>::iterator jit, jit_end = index_nodes.end();
    for (jit = index_nodes.begin(); jit != jit_end; ++jit)
    {
        XdfsIndexEntry e;
        e.name = (*jit)["name"];
        e.expr = (*jit)["expression"];
        e.filename = ((m_database->m_ctrl_path + xf_path_separator_wchar) + L"indexes" + xf_path_separator_wchar) + (*jit)["filename"].getString();
        e.index = NULL;
        e.update = false;
        e.key_length = 0;
        e.orig_key = NULL;
        e.key_expr = NULL;

        entries.push_back(e);
    }



    // merge index entries we just read with m_indexes

    it_end = entries.end();
    for (it = entries.begin(); it != it_end; ++it)
    {
        m_indexes_mutex.lock();
        bool found = false;
        for (it2 = m_indexes.begin(); it2 != m_indexes.end(); ++it2)
        {
            if (kl::iequals(it->name, it2->name))
            {
                found = true;
                break;
            }
        }
        m_indexes_mutex.unlock();

        if (found)
            continue;

        XdfsIndexEntry& entry = *it;


        ExIndex* idx = new ExIndex;
        idx->ref();
        if (!idx->open(entry.filename))
        {
            idx->unref();
            continue;
        }

        entry.index = idx;
                
        if (prepareIndexEntry(entry))
        {
            m_indexes_mutex.lock();
            m_indexes.push_back(entry);
            m_indexes_mutex.unlock();
        }
         else
        {
            entry.index->unref();
            entry.index = NULL;
        }
    }

}





bool XdfsBaseSet::prepareIndexEntry(XdfsIndexEntry& e)
{
    if (!e.index)
        return false;

    delete e.key_expr;  
    delete[] e.orig_key;

    e.key_length = e.index->getKeyLength();
    e.update = false;
    e.orig_key = new unsigned char[e.key_length];
    //e.key_expr = new KeyLayout;
    //e.key_expr->setKeyExpr(static_cast<xd::IIterator*>(m_update_iter), e.expr);

/*
    xd::IStructurePtr structure = m_database->describeTable(m_object_path);
    if (structure.isNull())
        return false;
        
    e.active_columns.resize(structure->getColumnCount(), false);
    std::vector<std::wstring> cols;
    kl::parseDelimitedList(e.expr, cols, L',', true);



    std::vector<std::wstring>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        // get column name (remove possible 'ASC' or 'DESC'
        // from the end of the index piece

        std::wstring piece = *it;
        std::wstring colname;

        kl::trim(piece);

        if (piece.find_last_of(L' ') != -1)
        {
            colname = kl::beforeLast(piece, L' ');
        }
         else
        {
            colname = piece;
        }

        xd::IColumnInfoPtr info = structure->getColumnInfo(colname);

        if (info.isOk())
        {
            if (info->getCalculated())
            {
                // in the case of a calculated field, the index may be
                // dependant on those fields which make up the formula
                
                std::vector<std::wstring> fields_used;
                std::vector<std::wstring>::iterator it;

                fields_used = getFieldsInExpr(info->getExpression(), structure, true);

                for (it = fields_used.begin();
                     it != fields_used.end();
                     ++it)
                {
                    info = structure->getColumnInfo(*it);
                    if (!info->getCalculated())
                    {
                        e.active_columns[info->getColumnOrdinal()] = true;
                    }
                }
            }
             else
            {
                e.active_columns[info->getColumnOrdinal()] = true;
            }
        }
    }
*/

    return true;
}


IIndex* XdfsBaseSet::lookupIndexForOrder(const std::wstring& order)
{
    refreshIndexEntries();

    IIndex* idx = NULL;

    m_indexes_mutex.lock();
    std::vector<XdfsIndexEntry>::iterator it, it_end = m_indexes.end();
    for (it = m_indexes.begin(); it != it_end; ++it)
    {
        if (getIndexExpressionMatch(it->expr, order))
        {
            idx = it->index;
            idx->ref();
            break;
        }
    }
    m_indexes_mutex.unlock();

    return idx;
}





// calculated Field routines

bool XdfsBaseSet::createCalcField(xd::IColumnInfoPtr colinfo)
{
    KL_AUTO_LOCK(m_object_mutex);
    
    if (m_config_file_path.empty())
    {
        xd::IColumnInfoPtr c = colinfo->clone();
        c->setCalculated(true);
        m_calc_fields.push_back(c);
        return true;
    }
     else
    {
        kl::JsonNode root;

        std::wstring json = xf_get_file_contents(m_config_file_path);
        if (!json.empty())
            root.fromString(json);

        kl::JsonNode calculated_fields = root.getChild("calculated_fields");
        if (!calculated_fields.isArray())
            calculated_fields.setArray();

        kl::JsonNode field = calculated_fields.appendElement();
        field["name"] = colinfo->getName();
        field["type"] = xd::dbtypeToString(colinfo->getType());
        field["width"] = colinfo->getWidth();
        field["scale"] = colinfo->getScale();
        field["expression"] = colinfo->getExpression();

        return xf_put_file_contents(m_config_file_path, root.toString());
    }

    return false;
}

bool XdfsBaseSet::deleteCalcField(const std::wstring& _name)
{
    KL_AUTO_LOCK(m_object_mutex);

    if (m_config_file_path.empty())
    {
        std::vector<xd::IColumnInfoPtr>::iterator it;
        for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
        {
            if (!wcscasecmp((*it)->getName().c_str(), _name.c_str()))
            {
                m_calc_fields.erase(it);
                return true;
            }
        }
     }
      else
     {
        kl::JsonNode root;

        std::wstring json = xf_get_file_contents(m_config_file_path);
        if (!json.empty())
            root.fromString(json);

        kl::JsonNode calculated_fields = root.getChild("calculated_fields");
        if (!calculated_fields.isArray())
            calculated_fields.setArray();

        bool field_deleted = false;

        size_t i, cnt = calculated_fields.getChildCount();
        for (i = 0; i < cnt; ++i)
        {
            kl::JsonNode field = calculated_fields[i];
            if (kl::iequals(field["name"], _name))
            {
                calculated_fields.deleteChild(i);
                field_deleted = true;
                break;
            }
        }

        if (field_deleted)
        {
            xf_put_file_contents(m_config_file_path, root.toString());
            return true;
        }

        return false;
     }

    // we couldn't find the field
    return false;
}

bool XdfsBaseSet::modifyCalcField(const std::wstring& name,
                                  xd::IColumnInfoPtr colinfo)
{
    KL_AUTO_LOCK(m_object_mutex);
    
    if (!colinfo->getCalculated())
    {
        // this is a make permanent operation
        return false;
    }

    std::wstring new_name, new_expr;
    int new_type, new_width, new_scale;
    

    if (m_config_file_path.empty())
    {
        // find the calculated field and modify it
        std::vector<xd::IColumnInfoPtr>::iterator it;
        for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
        {
            if (!wcscasecmp((*it)->getName().c_str(), name.c_str()))
            {
                new_name  = colinfo->getName();
                new_type  = colinfo->getType();
                new_width = colinfo->getWidth();
                new_scale = colinfo->getScale();
                new_expr  = colinfo->getExpression();

                if (new_name.length() > 0)
                    (*it)->setName(new_name);

                if (new_type != -1)
                    (*it)->setType(new_type);

                if (new_width != -1)
                    (*it)->setWidth(new_width);

                if (new_scale != -1)
                    (*it)->setScale(new_scale);

                if (new_expr.length() > 0)
                    (*it)->setExpression(new_expr);
            }
        }
    }
     else
    {

        kl::JsonNode root;

        std::wstring json = xf_get_file_contents(m_config_file_path);
        if (!json.empty())
            root.fromString(json);

        kl::JsonNode calculated_fields = root.getChild("calculated_fields");
        if (!calculated_fields.isArray())
            calculated_fields.setArray();

        bool field_modified = false;

        size_t i, cnt = calculated_fields.getChildCount();
        for (i = 0; i < cnt; ++i)
        {
            kl::JsonNode field = calculated_fields[i];
            if (kl::iequals(field["name"], name))
            {
                new_name  = colinfo->getName();
                new_type  = colinfo->getType();
                new_width = colinfo->getWidth();
                new_scale = colinfo->getScale();
                new_expr  = colinfo->getExpression();
                
                if (new_name.length() > 0)
                {
                    field["name"] = new_name;
                }
                    
                if (new_type != -1)
                    field["type"] = xd::dbtypeToString(new_type);

                if (new_width != -1)
                    field["width"] = new_width;

                if (new_scale != -1)
                    field["scale"] = new_scale;

                if (new_expr.length() > 0)
                    field["expression"] = new_expr;

                field_modified = true;
                break;
            }
        }

        if (field_modified)
        {
            xf_put_file_contents(m_config_file_path, root.toString());
            return true;
        }

        return true;
    }

    return true;
}

void XdfsBaseSet::appendCalcFields(xd::IStructure* structure)
{
    KL_AUTO_LOCK(m_object_mutex);

    IStructureInternalPtr intstruct = structure;
    
    if (m_config_file_path.empty())
    {
        std::vector<xd::IColumnInfoPtr>::iterator it;
        for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
        {
            intstruct->addColumn((*it)->clone());
        }
    }
     else
    {
        kl::JsonNode root;

        std::wstring json = xf_get_file_contents(m_config_file_path);
        if (!json.empty())
            root.fromString(json);

        kl::JsonNode calculated_fields = root.getChild("calculated_fields");

        size_t i, cnt = calculated_fields.getChildCount();

        for (i = 0; i < cnt; ++i)
        {
            kl::JsonNode field = calculated_fields[i];

            xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
            col->setName(field["name"]);
            col->setType(xd::stringToDbtype(field["type"]));
            col->setWidth(field["width"].getInteger());
            col->setScale(field["scale"].getInteger());
            col->setExpression(field["expression"]);
            col->setCalculated(!col->getExpression().empty());
            intstruct->addColumn(col);
        }
    }
}




