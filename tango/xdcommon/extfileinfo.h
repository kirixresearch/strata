/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2007-01-05
 *
 */


#ifndef __XDCOMMON_EXTFILEINFO_H
#define __XDCOMMON_EXTFILEINFO_H


#include <kl/string.h>
#include <kl/xml.h>


#define EXTFILEINFO_VERSION     L"1.0"


class ExtFileEntry
{

public:
    
    ExtFileEntry(kl::xmlnode& node);
    ExtFileEntry& operator=(const ExtFileEntry& c);

    bool load(const std::wstring& filename);
    bool save(const std::wstring& filename);
    bool isOk();

    ExtFileEntry addChild(const std::wstring& name);
    ExtFileEntry addChild(const std::wstring& name, 
                          const std::wstring& contents);
    ExtFileEntry addChild(const std::wstring& name,
                          int contents);

    size_t getChildCount();
    ExtFileEntry getChild(size_t idx);
    ExtFileEntry getChild(
                     const std::wstring& name,
                     const std::wstring& prop_name = L"",
                     const std::wstring& prop_value = L"");
    ExtFileEntry getAddChildIfNotExist(
                     const std::wstring& name,
                     const std::wstring& prop_name = L"",
                     const std::wstring& prop_value = L"");
    bool deleteChild(const std::wstring& name,
                     const std::wstring& prop_name = L"",
                     const std::wstring& prop_value = L"");

    std::wstring getContents();
    std::wstring getChildContents(
                     const std::wstring& child_name,
                     const std::wstring& prop_name = L"",
                     const std::wstring& prop_value = L"");

    void addProperty(const std::wstring& name,
                     const std::wstring& value);
    std::wstring getPropertyValue(const std::wstring& name);
    void setTagName(const std::wstring& tag_name);
    void setContents(const std::wstring& contents);
    void setContents(int contents);
    void setChildContents(const std::wstring& child_name,
                          const std::wstring& contents);
    void setChildContents(const std::wstring& child_name,
                          int contents);
  
private:

    kl::xmlnode* m_node;
};


    

class ExtFileInfo
{

public:

    ExtFileInfo();
    ~ExtFileInfo();

    static std::wstring getConfigFilenameFromPath(
                              const std::wstring& set_definition_path,
                              const std::wstring& filename);
                              
    static std::wstring getConfigFilenameFromSetId(
                              const std::wstring& set_definition_path,
                              const std::wstring& set_id);

    bool load(const std::wstring& filename);
    bool save(const std::wstring& filename);
    
    ExtFileEntry getGroup(const std::wstring& name);
    bool deleteGroup(const std::wstring& name);
    
private:

    kl::xmlnode m_xml_root;
    ExtFileEntry* m_root;
};




#endif  // __XDCOMMON_EXTFILEINFO_H

