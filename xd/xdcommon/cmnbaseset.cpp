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
#include "xdcommon.h"
#include "cmnbaseset.h"
#include "extfileinfo.h"
#include <kl/klib.h>


CommonBaseSet::CommonBaseSet()
{
}

CommonBaseSet::~CommonBaseSet()
{

}

void CommonBaseSet::setObjectPath(const std::wstring& path)
{
    m_obj_path = path;
}

std::wstring CommonBaseSet::getObjectPath()
{
    return m_obj_path;
}

void CommonBaseSet::setConfigFilePath(const std::wstring& path)
{
    m_config_file_path = path;
}


bool CommonBaseSet::modifyStructure(const xd::StructureModify& mod_params, bool* done_flag)
{
    KL_AUTO_LOCK(m_object_mutex);

    *done_flag = false;

    std::vector<xd::StructureModify::Action>::const_iterator it;
    int processed_action_count = 0;

    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        if (deleteCalcField(it->column))
            processed_action_count++;
    }

    // handle modify
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionModify)
            continue;

        if (modifyCalcField(it->column, it->params))
            processed_action_count++;
    }

    // handle create
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionCreate)
            continue;

        if (it->params.expression.length() > 0)
        {
            if (createCalcField(it->params))
                processed_action_count++;
        }
    }

    if (processed_action_count == mod_params.actions.size())
    {
        // we have handled all actions, so we're done
        *done_flag = true;
    }
    
    return true;
}


// Calculated Field routines

bool CommonBaseSet::createCalcField(const xd::ColumnInfo& colinfo)
{
    KL_AUTO_LOCK(m_object_mutex);
    
    if (m_config_file_path.empty())
    {
        xd::ColumnInfo c = colinfo;
        c.calculated = true;
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
        field.addChild(L"name", colinfo.name);
        field.addChild(L"type", colinfo.type);
        field.addChild(L"width", colinfo.width);
        field.addChild(L"scale", colinfo.scale);
        field.addChild(L"expression", colinfo.expression);
        
        return fileinfo.save(m_config_file_path);
    }

    return false;
}

bool CommonBaseSet::deleteCalcField(const std::wstring& _name)
{
    KL_AUTO_LOCK(m_object_mutex);

    if (m_config_file_path.empty())
    {
        std::vector<xd::ColumnInfo>::iterator it;
        for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
        {
            if (kl::iequals(it->name, _name))
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
            
            if (kl::iequals(store_name, _name))
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

bool CommonBaseSet::modifyCalcField(const std::wstring& name, const xd::ColumnInfo& colinfo)
{
    KL_AUTO_LOCK(m_object_mutex);
    
    if (!colinfo.calculated)
    {
        // this is a make permanent operation
        return false;
    }

    std::wstring new_name, new_expr;
    int new_type, new_width, new_scale;
    

    if (m_config_file_path.empty())
    {
        // find the calculated field and modify it
        std::vector<xd::ColumnInfo>::iterator it;
        for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
        {
            if (kl::iequals(it->name, name))
            {
                new_name  = colinfo.name;
                new_type  = colinfo.type;
                new_width = colinfo.width;
                new_scale = colinfo.scale;
                new_expr  = colinfo.expression;

                if (new_name.length() > 0)
                    it->name = new_name;

                if (new_type != -1)
                    it->type = new_type;

                if (new_width != -1)
                    it->width = new_width;

                if (new_scale != -1)
                    it->scale = new_scale;

                if (new_expr.length() > 0)
                    it->expression = new_expr;
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
            
            if (kl::iequals(store_name, name))
            {
                new_name  = colinfo.name;
                new_type  = colinfo.type;
                new_width = colinfo.width;
                new_scale = colinfo.scale;
                new_expr  = colinfo.expression;
                
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

void CommonBaseSet::appendCalcFields(xd::IStructure* structure)
{
    KL_AUTO_LOCK(m_object_mutex);

    IStructureInternalPtr intstruct = structure;
        
    std::wstring name, expression;
    int type, width, scale;
    
    std::wstring set_id = getSetId();
    if (set_id.empty())
    {
        std::vector<xd::ColumnInfo>::iterator it;
        for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
        {
            intstruct->addColumn(*it);
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

            xd::ColumnInfo col;

            col.name = name;
            col.type = type;
            col.width = width;
            col.scale = scale;
            col.expression = expression;
            col.calculated = true;

            intstruct->addColumn(col);
        }
    }
}




