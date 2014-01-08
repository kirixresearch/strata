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


class XdfsBaseSet : public xcm::IObject
{
public:

    XdfsBaseSet();
    virtual ~XdfsBaseSet();

    // sets the set's config file path (ext file info)
    // this should be called by derived classes
    void setConfigFilePath(const std::wstring& path);
    
    void setObjectPath(const std::wstring& path);
    std::wstring getObjectPath();
    
    virtual std::wstring getSetId() { return L""; }

protected:

    bool modifyStructure(xd::IStructure* struct_config,
                                        bool* done_flag);

    // calculated field routines
    bool createCalcField(xd::IColumnInfoPtr colinfo);
    bool deleteCalcField(const std::wstring& _name);
    bool modifyCalcField(const std::wstring& name,
                         xd::IColumnInfoPtr colinfo);
    void appendCalcFields(xd::IStructure* structure);

private:

    bool saveCalcFields();
    
private:

    xcm::mutex m_object_mutex;
    std::vector<xd::IColumnInfoPtr> m_calc_fields;
    std::wstring m_obj_path;
    std::wstring m_config_file_path;
};


#endif
