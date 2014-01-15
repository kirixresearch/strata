/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2006-03-20
 *
 */


#ifndef __XDFS_BASESET_H
#define __XDFS_BASESET_H


#include <kl/file.h>

class IIndex;
class KeyLayout;

class XdfsIndexEntry
{
public:

    std::wstring filename;
    std::wstring name;
    std::wstring expr;

    std::vector<bool> active_columns;
    IIndex* index;
    int key_length;
    KeyLayout* key_expr;
    unsigned char* orig_key;
    bool update;
};




class XdfsBaseSet : public xcm::IObject
{
public:

    XdfsBaseSet(FsDatabase* database);
    virtual ~XdfsBaseSet();

    // sets the set's config file path (ext file info)
    // this should be called by derived classes
    void setConfigFilePath(const std::wstring& path);
    
    void setObjectPath(const std::wstring& path);
    std::wstring getObjectPath();
    
    virtual std::wstring getSetId() { return L""; }

protected:

    bool modifyStructure(xd::IStructure* struct_config, bool* done_flag);
    void refreshIndexEntries();
    bool prepareIndexEntry(XdfsIndexEntry& e);
    IIndex* lookupIndexForOrder(const std::wstring& order);

    // calculated field routines
    bool createCalcField(xd::IColumnInfoPtr colinfo);
    bool deleteCalcField(const std::wstring& _name);
    bool modifyCalcField(const std::wstring& name,
                         xd::IColumnInfoPtr colinfo);
    void appendCalcFields(xd::IStructure* structure);

protected:

    FsDatabase* m_database;

private:

    kl::mutex m_object_mutex;
    std::vector<xd::IColumnInfoPtr> m_calc_fields;
    std::wstring m_object_path;
    std::wstring m_object_id;
    std::wstring m_config_file_path;

    std::vector<XdfsIndexEntry> m_indexes;
    xf_filetime_t m_indexes_filetime;
};


#endif
