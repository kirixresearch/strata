/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-12
 *
 */


#ifndef H_XDNATIVE_OFS_H
#define H_XDNATIVE_OFS_H


#include <map>



class Database;
class OfsFile;

class InternalOfsValue
{
public:

    kl::mutex m_object_mutex;
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




class OfsValue : public INodeValue
{
    XCM_CLASS_NAME_CUSTOMREFCOUNT("xdnative.OfsValue")
    XCM_BEGIN_INTERFACE_MAP(OfsValue)
        XCM_INTERFACE_ENTRY(INodeValue)
    XCM_END_INTERFACE_MAP()

public:

    OfsValue(OfsFile* file, InternalOfsValue* iv);
    virtual ~OfsValue();

    bool write() { return true; }

    void setDirty();

    // INodeValue interface
    const std::wstring& getName();
    int getType();
    void setType(int type);

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
    INodeValuePtr getChildByIdx(unsigned int idx);
    INodeValuePtr getChild(const std::wstring& name,
                                 bool create_if_not_exist);
    INodeValuePtr createChild(const std::wstring& name);
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
    
    XCM_CLASS_NAME_CUSTOMREFCOUNT("xdnative.OfsFile")
    XCM_BEGIN_INTERFACE_MAP(OfsFile)
    XCM_END_INTERFACE_MAP()


    virtual void ref()
    {
        m_database->lockObjectRegistryMutex();
        m_refcount_holder.xcm_ref_count++;
        m_database->unlockObjectRegistryMutex();
    }
    
    virtual void unref()
    {
        m_database->lockObjectRegistryMutex();
        if (--m_refcount_holder.xcm_ref_count == 0)
        {
            XdnativeDatabase* db = m_database;
            db->ref();

            db->unregisterNodeFile(this);
            delete this;
            db->unlockObjectRegistryMutex();
            
            db->unref();
            return;
        }
        m_database->unlockObjectRegistryMutex();
    }
    
    virtual int get_ref_count()
    {
        return 10;
    }
    

public:

    static OfsFile* createFile(XdnativeDatabase* db,
                               const std::wstring& path,
                               int type);

    static OfsFile* openFile(XdnativeDatabase* db,
                             const std::wstring& path);
                             
    static bool getFileType(XdnativeDatabase* db,
                            const std::wstring& path,
                            int* type,
                            bool* is_mount);
                            
    INodeValuePtr getRootNode();
    const std::wstring& getPath();
    void setType(int new_type);
    int getType();
    
private:
    
    OfsFile(XdnativeDatabase* database);
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
    kl::mutex m_object_mutex;
    XdnativeDatabase* m_database;
};





#endif

