/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2006-03-20
 *
 */


#ifndef __XDCOMMON_CMNBASESET_H
#define __XDCOMMON_CMNBASESET_H


class CommonBaseSet : public xcm::IObject
{
public:

    CommonBaseSet();
    virtual ~CommonBaseSet();

    // sets the set's config file path (ext file info)
    // this should be called by derived classes
    void setConfigFilePath(const std::wstring& path);
    
    void setObjectPath(const std::wstring& path);
    std::wstring getObjectPath();
    
    virtual std::wstring getSetId() { return L""; }

protected:

    bool modifyStructure(const xd::StructureModify& mod_params, bool* done_flag);

    // calculated field routines
    bool createCalcField(const xd::ColumnInfo& colinfo);
    bool deleteCalcField(const std::wstring& _name);
    bool modifyCalcField(const std::wstring& name, const xd::ColumnInfo& colinfo);
    void appendCalcFields(xd::IStructure* structure);
    void appendCalcFields(xd::Structure& structure);

private:

    bool saveCalcFields();
    
private:

    kl::mutex m_object_mutex;
    std::vector<xd::ColumnInfo> m_calc_fields;
    std::wstring m_obj_path;
    std::wstring m_config_file_path;
};


#endif
