/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2006-04-14
 *
 */


#ifndef __XDCOMMON_SETCONFIGFILE_H
#define __XDCOMMON_SETCONFIGFILE_H


#include <kl/string.h>
#include <kl/xml.h>


#define SETCONFIG_VERSION L"1.0"


class SetConfigGroup
{
public:

    SetConfigGroup(kl::xmlnode& node);
    ~SetConfigGroup();
    
    std::wstring getValue(const std::wstring& key);
    void setValue(const std::wstring& key, const std::wstring& value);
    void setValue(const std::wstring& key, int value);

    SetConfigGroup createSubGroup(const std::wstring& key);
    bool deleteSubGroup(const std::wstring& key);
    bool renameSubGroup(const std::wstring& key, const std::wstring& new_name);
    SetConfigGroup getSubGroup(const std::wstring& key);
    SetConfigGroup getSubGroup(size_t idx);
    size_t getSubGroupCount();

private:

    kl::xmlnode& m_node;
};


class SetConfigFile
{

public:

    SetConfigFile();
    ~SetConfigFile();
    
    bool load(const std::wstring& filename);
    bool save(const std::wstring& filename);
    void setBaseDirectory(const std::wstring& dir);

    // -- these functions pass through to m_root --
    std::wstring getValue(const std::wstring& key);
    void setValue(const std::wstring& key, const std::wstring& value);
    void setValue(const std::wstring& key, int value);

    SetConfigGroup createSubGroup(const std::wstring& key);
    bool deleteSubGroup(const std::wstring& key);
    bool renameSubGroup(const std::wstring& key, const std::wstring& new_name);
    SetConfigGroup getSubGroup(const std::wstring& key);
    SetConfigGroup getSubGroup(size_t idx);
    size_t getSubGroupCount();

public:

    SetConfigGroup* m_root;
    kl::xmlnode m_xml_file;
    std::wstring m_base_dir;
};


#endif



