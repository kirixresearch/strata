/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2006-03-20
 *
 */


#include <xd/xd.h>
#include "baseset.h"
#include "../xdcommon/extfileinfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
#include <kl/klib.h>


XdfsBaseSet::XdfsBaseSet()
{
}

XdfsBaseSet::~XdfsBaseSet()
{

}

void XdfsBaseSet::setObjectPath(const std::wstring& path)
{
    m_obj_path = path;
}

std::wstring XdfsBaseSet::getObjectPath()
{
    return m_obj_path;
}

void XdfsBaseSet::setConfigFilePath(const std::wstring& path)
{
    m_config_file_path = path;
}


bool XdfsBaseSet::modifyStructure(xd::IStructure* struct_config,
                                    bool* done_flag)
{
    XCM_AUTO_LOCK(m_object_mutex);

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


// Calculated Field routines

bool XdfsBaseSet::createCalcField(xd::IColumnInfoPtr colinfo)
{
    XCM_AUTO_LOCK(m_object_mutex);
    
    if (m_config_file_path.empty())
    {
        xd::IColumnInfoPtr c = colinfo->clone();
        c->setCalculated(true);
        m_calc_fields.push_back(c);
        return true;
    }
     else
    {
        // load the existing stored information, if it exists
        ExtFileInfo fileinfo;
        fileinfo.load(m_config_file_path);

        // find the "calc_fields" area of the file
        ExtFileEntry entry = fileinfo.getGroup(L"calculated_fields");
        entry = entry.getAddChildIfNotExist(L"fields");

        // save the calculated field info
        ExtFileEntry field = entry.addChild(L"field");
        field.addChild(L"name", colinfo->getName());
        field.addChild(L"type", colinfo->getType());
        field.addChild(L"width", colinfo->getWidth());
        field.addChild(L"scale", colinfo->getScale());
        field.addChild(L"expression", colinfo->getExpression());
        
        return fileinfo.save(m_config_file_path);
    }

    return false;
}

bool XdfsBaseSet::deleteCalcField(const std::wstring& _name)
{
    XCM_AUTO_LOCK(m_object_mutex);

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
        // load the existing stored information, if it exists
        ExtFileInfo fileinfo;
        fileinfo.load(m_config_file_path);

        // find the "calc_fields" area of the file
        ExtFileEntry entry = fileinfo.getGroup(L"calculated_fields");
        entry = entry.getAddChildIfNotExist(L"fields");

        bool field_deleted = false;
        size_t i, count = entry.getChildCount();
        for (i = 0; i < count; ++i)
        {
            ExtFileEntry field = entry.getChild(i);
            std::wstring store_name = field.getChildContents(L"name");
            
            if (!wcscasecmp(store_name.c_str(), _name.c_str()))
            {
                // this is a kludge -- since all of the "calc_fields" childrens'
                // tags are "field", we need to rename the one with a matching
                // field name to something unique so that we can delete it
                field.setTagName(store_name);
                field_deleted = entry.deleteChild(store_name);
                break;
            }
        }

        // if the field was deleted, try to save the changes
        // and return the result
        if (field_deleted)
            return fileinfo.save(m_config_file_path);
     }

    // we couldn't find the field
    return false;
}

bool XdfsBaseSet::modifyCalcField(const std::wstring& name,
                                    xd::IColumnInfoPtr colinfo)
{
    XCM_AUTO_LOCK(m_object_mutex);
    
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
        // load the existing stored information, if it exists
        ExtFileInfo fileinfo;
        fileinfo.load(m_config_file_path);

        // find the "calc_fields" area of the file
        ExtFileEntry entry = fileinfo.getGroup(L"calculated_fields");
        entry = entry.getAddChildIfNotExist(L"fields");

        size_t i, count = entry.getChildCount();
        for (i = 0; i < count; ++i)
        {
            ExtFileEntry field = entry.getChild(i);
            std::wstring store_name = field.getChildContents(L"name");
            
            if (!wcscasecmp(store_name.c_str(), name.c_str()))
            {
                new_name  = colinfo->getName();
                new_type  = colinfo->getType();
                new_width = colinfo->getWidth();
                new_scale = colinfo->getScale();
                new_expr  = colinfo->getExpression();
                
                if (new_name.length() > 0)
                {
                    field.setChildContents(L"name", new_name);
                }
                    
                if (new_type != -1)
                    field.setChildContents(L"type", new_type);

                if (new_width != -1)
                    field.setChildContents(L"width", new_width);

                if (new_scale != -1)
                    field.setChildContents(L"scale", new_scale);

                if (new_expr.length() > 0)
                    field.setChildContents(L"expression", new_expr);
            }
        }

        return fileinfo.save(m_config_file_path);
    }

    return true;
}

void XdfsBaseSet::appendCalcFields(xd::IStructure* structure)
{
    XCM_AUTO_LOCK(m_object_mutex);

    IStructureInternalPtr intstruct = structure;
        
    std::wstring name, expression;
    int type, width, scale;
    
    std::wstring set_id = getSetId();
    if (set_id.empty())
    {
        std::vector<xd::IColumnInfoPtr>::iterator it;
        for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
        {
            intstruct->addColumn((*it)->clone());
        }
    }
     else
    {
        // load the existing stored information, if it exists
        ExtFileInfo fileinfo;
        fileinfo.load(m_config_file_path);

        // find the "calc_fields" area of the file
        ExtFileEntry entry = fileinfo.getGroup(L"calculated_fields");
        entry = entry.getAddChildIfNotExist(L"fields");
        
        size_t i, count = entry.getChildCount();
        for (i = 0; i < count; ++i)
        {
            ExtFileEntry field = entry.getChild(i);
            
            name = field.getChildContents(L"name");
            type = kl::wtoi(field.getChildContents(L"type"));
            width = kl::wtoi(field.getChildContents(L"width"));
            scale = kl::wtoi(field.getChildContents(L"scale"));
            expression = field.getChildContents(L"expression");

            xd::IColumnInfoPtr col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
            col->setName(name);
            col->setType(type);
            col->setWidth(width);
            col->setScale(scale);
            col->setExpression(expression);
            col->setCalculated(true);
            intstruct->addColumn(col);
        }
    }
}




