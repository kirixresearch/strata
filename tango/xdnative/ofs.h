/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-12
 *
 */


#ifndef __XDNATIVE_OFS_H
#define __XDNATIVE_OFS_H


#include <map>


// -- forward declarations --

class Database;
class OfsFile;


class InternalOfsValue
{
public:

    xcm::mutex m_object_mutex;
    std::wstring m_name;
    std::wstring m_value;
    int m_type;
    std::vector<InternalOfsValue*> m_children;
    std::map<std::wstring, int> m_lookup_map;

    void clear()
    {
        std::vector<InternalOfsValue*>::iterator it;
        for (it = m_children.begin(); it != m_children.end(); ++it)
        {
            (*it)->clear();
            delete (*it);
        }

        m_children.clear();
        m_lookup_map.clear();
    }

};




class OfsValue : public tango::INodeValue
{
    XCM_CLASS_NAME_CUSTOMREFCOUNT("xdnative.OfsValue")
    XCM_BEGIN_INTERFACE_MAP(OfsValue)
        XCM_INTERFACE_ENTRY(tango::INodeValue)
    XCM_END_INTERFACE_MAP()

public:

    OfsValue(OfsFile* file, InternalOfsValue* iv);
    virtual ~OfsValue();

    void setDirty();

    // -- INodeValue interface --
    const std::wstring& getName();
    int getType();

    const std::wstring& getString();
    void setString(const std::wstring& value);
    double getDouble();
    void setDouble(double value);
    int getInteger();
    void setInteger(int value);
    bool getBoolean();
    void setBoolean(bool value);

    unsigned int getChildCount();
    const std::wstring& getChildName(unsigned int idx);
    tango::INodeValuePtr getChildByIdx(unsigned int idx);
    tango::INodeValuePtr getChild(const std::wstring& name,
                                 bool create_if_not_exist);
    tango::INodeValuePtr createChild(const std::wstring& name);
    bool getChildExist(const std::wstring& name);
    bool deleteChild(const std::wstring& name);
    bool renameChild(const std::wstring& name,
                     const std::wstring& new_name);
    bool deleteAllChildren();

public:

    void ref();
    void unref();
    int get_ref_count();

    int getChildIdx(const std::wstring& name);

public:

    OfsFile* m_file;
    InternalOfsValue* m_iv;
};


class OfsFile : public xcm::IObject
{
    friend class OfsValue;
    
    XCM_CLASS_NAME("xdnative.OfsFile")
    XCM_BEGIN_INTERFACE_MAP(OfsFile)
    XCM_END_INTERFACE_MAP()
    
public:

    static OfsFile* createFile(tango::IDatabase* db,
                               const std::wstring& path,
                               int type);

    static OfsFile* openFile(tango::IDatabase* db,
                             const std::wstring& path);
                             
    static bool getFileType(tango::IDatabase* db,
                            const std::wstring& path,
                            int* type,
                            bool* is_mount);
                            
    tango::INodeValuePtr getRootNode();
    std::wstring getPath();
    void setType(int new_type);
    int getType();
    
private:
    
    OfsFile(tango::IDatabase* database);
    ~OfsFile();

    void setDirty(bool dirty);

    bool readFile();
    bool writeFile();

private:

    std::wstring m_key_path;     // path of this registry node
    std::wstring m_ofs_root;
    InternalOfsValue* m_root_node;
    bool m_dirty;
    int m_type;
    xcm::mutex m_object_mutex;
    IDatabaseInternalPtr m_database;
};





#endif

