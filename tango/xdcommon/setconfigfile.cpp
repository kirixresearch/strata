/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2006-04-14
 *
 */


#include "xdcommon.h"
#include "util.h"
#include "setconfigfile.h"


// -- SetConfigGroup class definition --

SetConfigGroup::SetConfigGroup(kl::xmlnode& node) : m_node(node)
{

}

SetConfigGroup::~SetConfigGroup()
{

}

std::wstring SetConfigGroup::getValue(const std::wstring& key)
{
    kl::xmlnode& values_node = m_node.getChild(L"values");
    int idx = values_node.lookupChild(L"value", L"name", key);
    
    // -- if we couldn't find the node, return an empty string --
    if (idx == -1)
        return L"";
        
    kl::xmlnode& node = values_node.getChild(idx);
    return node.getNodeValue();
}

void SetConfigGroup::setValue(const std::wstring& key,
                              const std::wstring& value)
{
    // -- create the values node if it doesn't already exist --
    if (m_node.getChildIdx(L"values") == -1)
        m_node.addChild(L"values");

    kl::xmlnode& values_node = m_node.getChild(L"values");
    int idx = values_node.lookupChild(L"value", L"name", key);
    
    if (idx == -1)
    {
        // -- the key doesn't exist, so add it --    
        kl::xmlnode& node = values_node.addChild();
        node.setNodeName(L"value");

        kl::xmlproperty& name_prop = node.appendProperty();
        name_prop.name = L"name";
        name_prop.value = key;
        
        node.setNodeValue(value);
    }
     else
    {
        // -- the key exists, so modify it --
        kl::xmlnode& node = values_node.getChild(idx);
        node.setNodeValue(value);
    }
}

void SetConfigGroup::setValue(const std::wstring& key,
                              int value)
{
    // -- create the values node if it doesn't already exist --
    if (m_node.getChildIdx(L"values") == -1)
        m_node.addChild(L"values");

    kl::xmlnode& values_node = m_node.getChild(L"values");
    int idx = values_node.lookupChild(L"value", L"name", key);
    
    if (idx == -1)
    {
        // -- the key doesn't exist, so add it --    
        kl::xmlnode& node = values_node.addChild();
        node.setNodeName(L"value");

        kl::xmlproperty& name_prop = node.appendProperty();
        name_prop.name = L"name";
        name_prop.value = key;
        
        node.setNodeValue(value);
    }
     else
    {
        // -- the key exists, so modify it --
        kl::xmlnode& node = values_node.getChild(idx);
        node.setNodeValue(value);
    }
}

SetConfigGroup SetConfigGroup::createSubGroup(const std::wstring& key)
{
    // -- create the subgroups node if it doesn't already exist --
    if (m_node.getChildIdx(L"subgroups") == -1)
        m_node.addChild(L"subgroups");

    kl::xmlnode& subgroup_node = m_node.getChild(L"subgroups");

    // -- check if subgroup already exists.  If so, return the subgroup --
    int idx = subgroup_node.lookupChild(L"subgroup", L"name", key);
    if (idx >= 0)
    {
        kl::xmlnode& node = subgroup_node.getChild(idx);
        return SetConfigGroup(node);
    }

    kl::xmlnode& node = subgroup_node.addChild();
    node.setNodeName(L"subgroup");

    kl::xmlproperty& name_prop = node.appendProperty();
    name_prop.name = L"name";
    name_prop.value = key;
    
    return SetConfigGroup(node);
}

bool SetConfigGroup::deleteSubGroup(const std::wstring& key)
{
    if (m_node.getChildIdx(L"subgroups") == -1)
        return false;
        
    kl::xmlnode& subgroup_node = m_node.getChild(L"subgroups");
    
    int idx = subgroup_node.lookupChild(L"subgroup", L"name", key);
    if (idx == -1)
        return false;
        
    subgroup_node.removeChild(idx);
    
    return true;
}

bool SetConfigGroup::renameSubGroup(const std::wstring& key,
                                    const std::wstring& new_name)
{
    if (m_node.getChildIdx(L"subgroups") == -1)
        return false;
        
    kl::xmlnode& subgroup_node = m_node.getChild(L"subgroups");
    
    int idx = subgroup_node.lookupChild(L"subgroup", L"name", key);
    if (idx == -1)
        return false;
    
    kl::xmlnode& child = subgroup_node.getChild(idx);
    child.getProperty(L"name").value = new_name;
    
    return true;
}

SetConfigGroup SetConfigGroup::getSubGroup(const std::wstring& key)
{
    kl::xmlnode& subgroup_node = m_node.getChild(L"subgroups");
    int idx = subgroup_node.lookupChild(L"subgroup", L"name", key);
    kl::xmlnode& node = subgroup_node.getChild(idx);
    return SetConfigGroup(node);
}

SetConfigGroup SetConfigGroup::getSubGroup(size_t idx)
{
    kl::xmlnode& subgroup_node = m_node.getChild(L"subgroups");
    kl::xmlnode& node = subgroup_node.getChild(idx);
    return SetConfigGroup(node);
}

size_t SetConfigGroup::getSubGroupCount()
{
    kl::xmlnode& subgroup_node = m_node.getChild(L"subgroups");
    return (size_t)subgroup_node.getChildCount();
}




// -- SetConfigFile class definition --

SetConfigFile::SetConfigFile()
{
    m_base_dir = L"";
    m_xml_file.setNodeName(L"xtf");
    m_root = new SetConfigGroup(m_xml_file);
}

SetConfigFile::~SetConfigFile()
{
    if (m_root)
    {
        delete m_root;
        m_root = NULL;
    }
}

bool SetConfigFile::load(const std::wstring& filename)
{
    std::wstring path = makePathName(m_base_dir, L"", filename, L"xtf");
    if (!m_xml_file.load(path))
        return false;
    
    // -- we need to make sure our SetConfigGroup has the information
    //    we just loaded associated with it --
    if (m_root)
    {
        delete m_root;
        m_root = new SetConfigGroup(m_xml_file);
    }
    
    return true;
}

bool SetConfigFile::save(const std::wstring& filename)
{
    // -- add xtf version to the config file --
    kl::xmlproperty& version_prop = m_xml_file.getProperty(L"version");
    if (version_prop.isEmpty())
        m_xml_file.appendProperty(L"version", SETCONFIG_VERSION);
        
    // -- update old and/or different xtf versions --
    if (version_prop.value != SETCONFIG_VERSION)
        version_prop.value = SETCONFIG_VERSION;
        
    std::wstring path = makePathName(m_base_dir, L"", filename, L"xtf");
    if (!m_xml_file.save(path))
        return false;
    
    return true;
}

void SetConfigFile::setBaseDirectory(const std::wstring& dir)
{
    m_base_dir = dir;
}

std::wstring SetConfigFile::getValue(const std::wstring& key)
{
    return m_root->getValue(key);
}

void SetConfigFile::setValue(const std::wstring& key,
                             const std::wstring& value)
{
    m_root->setValue(key, value);
}

void SetConfigFile::setValue(const std::wstring& key,
                             int value)
{
    m_root->setValue(key, value);
}

SetConfigGroup SetConfigFile::createSubGroup(const std::wstring& key)
{
    return m_root->createSubGroup(key);
}

bool SetConfigFile::deleteSubGroup(const std::wstring& key)
{
    return m_root->deleteSubGroup(key);
}

bool SetConfigFile::renameSubGroup(const std::wstring& key,
                                   const std::wstring& new_name)
{
    return m_root->renameSubGroup(key, new_name);
}

SetConfigGroup SetConfigFile::getSubGroup(const std::wstring& key)
{
    return m_root->getSubGroup(key);
}

SetConfigGroup SetConfigFile::getSubGroup(size_t idx)
{
    return m_root->getSubGroup(idx);
}

size_t SetConfigFile::getSubGroupCount()
{
    return m_root->getSubGroupCount();
}



