/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2007-01-05
 *
 */


#include <xd/xd.h>
#include "xdcommon.h"
#include "extfileinfo.h"
#include <kl/file.h>
#include <kl/xml.h>


kl::xmlnode empty_xmlnode;


ExtFileEntry::ExtFileEntry(kl::xmlnode& node)
{
    m_node = &node;
}


ExtFileEntry& ExtFileEntry::operator=(const ExtFileEntry& c)
{
    m_node = c.m_node;
    return *this;
}


bool ExtFileEntry::load(const std::wstring& filename)
{
    return m_node->load(filename);
}


bool ExtFileEntry::save(const std::wstring& filename)
{
    return m_node->save(filename);
}


bool ExtFileEntry::isOk()
{
    return (m_node->getNodeName().length()) > 0 ? true : false;
}


ExtFileEntry ExtFileEntry::addChild(const std::wstring& name)
{
    return ExtFileEntry(m_node->addChild(name));
}


ExtFileEntry ExtFileEntry::addChild(const std::wstring& name,
                                    const std::wstring& contents)
{
    return ExtFileEntry(m_node->addChild(name, contents));
}


ExtFileEntry ExtFileEntry::addChild(const std::wstring& name, int contents)
{
    return ExtFileEntry(m_node->addChild(name, contents));
}


ExtFileEntry ExtFileEntry::getChild(const std::wstring& name,
                                    const std::wstring& prop_name,
                                    const std::wstring& prop_value)
{
    if (prop_name.empty() && prop_value.empty())
        return ExtFileEntry(m_node->getChild(name));

    int idx = m_node->lookupChild(name, prop_name, prop_value);
    if (idx == -1)
        return ExtFileEntry(empty_xmlnode);

    return ExtFileEntry(m_node->getChild((size_t)idx));
}


ExtFileEntry ExtFileEntry::getAddChildIfNotExist(
                                    const std::wstring& name,
                                    const std::wstring& prop_name,
                                    const std::wstring& prop_value)
{
    int idx;
    if (prop_name.empty() && prop_value.empty())
        idx = m_node->getChildIdx(name);
     else
        idx = m_node->lookupChild(name, prop_name, prop_value);
        
    if (idx == -1)
        return addChild(name);

    return ExtFileEntry(m_node->getChild((size_t)idx));
}


ExtFileEntry ExtFileEntry::getChild(size_t idx)
{
    return ExtFileEntry(m_node->getChild(idx));
}


bool ExtFileEntry::deleteChild(const std::wstring& name,
                               const std::wstring& prop_name,
                               const std::wstring& prop_value)
{
    int idx;
    if (prop_name.length() == 0 && prop_value.length() == 0)
    {
        idx = m_node->getChildIdx(name);
        return m_node->removeChild(idx);
    }

    idx = m_node->lookupChild(name, prop_name, prop_value);
    return m_node->removeChild(idx);
}


size_t ExtFileEntry::getChildCount()
{
    return m_node->getChildCount();
}


std::wstring ExtFileEntry::getContents()
{
    return m_node->getNodeValue();
}


std::wstring ExtFileEntry::getChildContents(const std::wstring& child_name,
                                const std::wstring& prop_name,
                                const std::wstring& prop_value)
{
    return getChild(child_name, prop_name, prop_value).getContents();
}


void ExtFileEntry::addProperty(const std::wstring& name, const std::wstring& value)
{
    m_node->appendProperty(name, value);
}


std::wstring ExtFileEntry::getPropertyValue(const std::wstring& name)
{
    return m_node->getProperty(name).value;
}


void ExtFileEntry::setTagName(const std::wstring& tag_name)
{
    m_node->setNodeName(tag_name);
}


void ExtFileEntry::setContents(const std::wstring& contents)
{
    m_node->setNodeValue(contents);
}

void ExtFileEntry::setContents(int contents)
{
    m_node->setNodeValue(contents);
}

void ExtFileEntry::setChildContents(const std::wstring& child_name,
                                    const std::wstring& contents)
{
    getChild(child_name).setContents(contents);
}

void ExtFileEntry::setChildContents(const std::wstring& child_name,
                                    int contents)
{
    getChild(child_name).setContents(contents);
}




// -- ExtFileInfo class implementation --

ExtFileInfo::ExtFileInfo()
{
    // create root node and add edf version to it
    m_root = new ExtFileEntry(m_xml_root);
    m_root->setTagName(L"edf");
}

ExtFileInfo::~ExtFileInfo()
{
    if (m_root)
    {
        delete m_root;
        m_root = NULL;
    }
}

    
    

// ExtFileInfo::getConfigFilenameFromPath() is a static function which returns
// the store path for the corresponding edf file for a given filename

std::wstring ExtFileInfo::getConfigFilenameFromPath(
                                const std::wstring& set_definition_path,
                                const std::wstring& filename)
{
    std::wstring network_path = xf_get_network_path(filename);
    std::wstring fileinfo_name;
    
    std::wstring::iterator it;
    for (it = network_path.begin(); it != network_path.end(); ++it)
    {
        if (*it == L'\\' ||
            *it == L'/' ||
            *it == L'*' ||
            *it == L':' ||
            *it == L'"' ||
            *it == L'<' ||
            *it == L'>' ||
            *it == L'|')
        {
            fileinfo_name += L'_';
        }
         else
        {
            fileinfo_name += *it;
        }
    }
    
    std::wstring retval = set_definition_path;
    
    // the last character is not a path separator, so append one
    if (retval.empty() || retval[retval.length()-1] != PATH_SEPARATOR_CHAR)
        retval += PATH_SEPARATOR_CHAR;
    
    retval += fileinfo_name;
    retval += L".edf";
    
    return retval;
}



// ExtFileInfo::getConfigFilenameFromSetId() is a static function which returns
// the store path for the corresponding edf file for a given set id

std::wstring ExtFileInfo::getConfigFilenameFromSetId(
                                const std::wstring& set_definition_path,
                                const std::wstring& set_id)
{
    std::wstring retval = set_definition_path;
    
    // the last character is not a path separator, so append one
    if (retval.empty() || retval[retval.length()-1] != PATH_SEPARATOR_CHAR)
        retval += PATH_SEPARATOR_CHAR;
    
    retval += set_id;
    retval += L".edf";
    
    return retval;
}



bool ExtFileInfo::load(const std::wstring& filename)
{
    return m_root->load(filename);
}

bool ExtFileInfo::save(const std::wstring& filename)
{
    // add the version number if it doesn't exist
    if (m_root->getPropertyValue(L"version").length() == 0)
        m_root->addProperty(L"version", EXTFILEINFO_VERSION);

    return m_root->save(filename);
}

ExtFileEntry ExtFileInfo::getGroup(const std::wstring& name)
{
    ExtFileEntry entry = m_root->getChild(L"group", L"name", name);
    if (entry.isOk())
        return entry;
        
    entry = m_root->addChild(L"group");
    entry.addProperty(L"name", name);
    return entry;
}

bool ExtFileInfo::deleteGroup(const std::wstring& name)
{
    return m_root->deleteChild(L"group", L"name", name);
}



