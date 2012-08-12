/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-12
 *
 */


#ifndef __XDCLIENT_NODEFILE_H
#define __XDCLIENT_NODEFILE_H


#include <map>



class ClientDatabase;
class NodeFile;


class InternalNodeValue
{
public:

    xcm::mutex m_object_mutex;
    std::wstring m_name;
    std::wstring m_value;
    int m_type;
    std::vector<InternalNodeValue*> m_children;
    std::map<std::wstring, int> m_lookup_map;

    void clear()
    {
        std::vector<InternalNodeValue*>::iterator it;
        for (it = m_children.begin(); it != m_children.end(); ++it)
        {
            (*it)->clear();
            delete (*it);
        }

        m_children.clear();
        m_lookup_map.clear();
    }

};




class NodeValue : public tango::INodeValue
{
    XCM_CLASS_NAME_CUSTOMREFCOUNT("xdnative.NodeValue")
    XCM_BEGIN_INTERFACE_MAP(NodeValue)
        XCM_INTERFACE_ENTRY(tango::INodeValue)
    XCM_END_INTERFACE_MAP()

public:

    NodeValue(NodeFile* file, InternalNodeValue* iv);
    virtual ~NodeValue();

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

    NodeFile* m_file;
    InternalNodeValue* m_iv;
};


class NodeFile : public xcm::IObject
{
    friend class NodeValue;
    
    XCM_CLASS_NAME("xdnative.NodeFile")
    XCM_BEGIN_INTERFACE_MAP(NodeFile)
    XCM_END_INTERFACE_MAP()
    
public:

    static NodeFile* createFile(ClientDatabase* db,
                               const std::wstring& path,
                               int type);

    static NodeFile* openFile(ClientDatabase* db,
                              const std::wstring& path);
                                       
    tango::INodeValuePtr getRootNode();
    std::wstring getPath();
    void setType(int new_type);
    int getType();
    
private:
    
    NodeFile(ClientDatabase* database);
    ~NodeFile();

    void setDirty(bool dirty);

    bool readFile();
    bool writeFile();

private:

    xcm::mutex m_object_mutex;
    ClientDatabase* m_database;
    std::wstring m_key_path;     // path of this registry node
    std::wstring m_ofs_root;
    InternalNodeValue* m_root_node;
    bool m_dirty;
    int m_type;
};





#endif

