/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2006-03-20
 *
 */


#ifndef H_XDCOMMON_CMNBASESET_H
#define H_XDCOMMON_CMNBASESET_H


class CommonBaseSet
{
public:

    CommonBaseSet();
    virtual ~CommonBaseSet();

    // sets the set's config file path (ext file info)
    // this should be called by derived classes
    void setConfigFilePath(const std::wstring& path);
    
    void setObjectPath(const std::wstring& path);
    std::wstring getObjectPath();
    
    bool modifyStructure(const xd::StructureModify& mod_params, bool* done_flag);

    // calculated field routines
    bool createCalcField(const xd::ColumnInfo& colinfo);
    bool deleteCalcField(const std::wstring& _name);
    bool modifyCalcField(const std::wstring& name, const xd::ColumnInfo& colinfo);
    void appendCalcFields(xd::Structure& structure);

private:

    kl::mutex m_object_mutex;
    std::vector<xd::ColumnInfo> m_calc_fields;
    std::wstring m_obj_path;
    std::wstring m_config_file_path;
};


#endif
