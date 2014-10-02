/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-20
 *
 */


#ifndef __XDFS_BASESET_H
#define __XDFS_BASESET_H


#include <kl/file.h>
#include <kl/memory.h>


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
    kl::membuf orig_key;
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

    bool modifyStructure(const xd::StructureModify& mod_params, bool* done_flag);
    void refreshIndexEntries();
    bool prepareIndexEntry(XdfsIndexEntry& e);
    IIndex* lookupIndexForOrder(const std::wstring& order);

    // calculated field routines
    bool createCalcField(const xd::ColumnInfo& colinfo);
    bool deleteCalcField(const std::wstring& _name);
    bool modifyCalcField(const std::wstring& name, const xd::ColumnInfo& colinfo);
    void appendCalcFields(xd::Structure& structure);

protected:

    FsDatabase* m_database;

    kl::mutex m_indexes_mutex;
    std::vector<XdfsIndexEntry> m_indexes;
    xf_filetime_t m_indexes_filetime;

private:

    kl::mutex m_object_mutex;
    std::vector<xd::ColumnInfo> m_calc_fields;
    std::wstring m_object_path;
    std::wstring m_object_id;
    std::wstring m_config_file_path;
};


#endif
