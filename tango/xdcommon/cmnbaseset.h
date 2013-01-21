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


class CommonBaseSet : public tango::ISet
{
public:

    CommonBaseSet();
    virtual ~CommonBaseSet();

    // sets the set's config file path (ext file info)
    // this should be called by derived classes
    void setConfigFilePath(const std::wstring& path);
    
    // tango::ISet::setObjectPath() and getObjectPath();
    void setObjectPath(const std::wstring& path);
    std::wstring getObjectPath();
    
    bool isTemporary() { return false; }

protected:

    bool modifyStructure(tango::IStructure* struct_config,
                                        bool* done_flag);

    // calculated field routines
    bool createCalcField(tango::IColumnInfoPtr colinfo);
    bool deleteCalcField(const std::wstring& _name);
    bool modifyCalcField(const std::wstring& name,
                         tango::IColumnInfoPtr colinfo);
    void appendCalcFields(tango::IStructure* structure);

private:

    bool saveCalcFields();
    
private:

    xcm::mutex m_object_mutex;
    std::vector<tango::IColumnInfoPtr> m_calc_fields;
    std::wstring m_obj_path;
    std::wstring m_config_file_path;
};


#endif


