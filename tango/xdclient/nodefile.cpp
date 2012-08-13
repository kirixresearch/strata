/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-12
 *
 */

#include "xdclient.h"
#include "nodefile.h"


NodeValue::NodeValue(NodeFile* file, InternalNodeValue* value)
{
    m_file = file;
    m_iv = value;
    
    m_file->ref();
}

NodeValue::~NodeValue()
{
    m_file->unref();
}

void NodeValue::setDirty()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    m_file->setDirty(true);
}

int NodeValue::getType()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return m_iv->m_type;
}

const std::wstring& NodeValue::getName()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return m_iv->m_name;
}

const std::wstring& NodeValue::getString()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return m_iv->m_value;
}

void NodeValue::setString(const std::wstring& value)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (m_iv->m_value != value)
    {
        m_iv->m_value = value;
        setDirty();
    }
}


double NodeValue::getDouble()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return kl::nolocale_wtof(m_iv->m_value);
}


inline void nolocale_ftow(wchar_t* buffer, const double& dbl, int dec_places)
{
    swprintf(buffer, 64, L"%.*f", dec_places, dbl);

    while (*buffer)
    {
        if (*buffer == L',')
            *buffer = L'.';
        buffer++;
    }
}


inline void nolocale_dbltostr(double d, wchar_t* str, int max_buffer)
{
    for (int i = 0; i < 15; ++i)
    {
        swprintf(str, max_buffer, L"%.*f", i, d);

        if (kl::dblcompare(kl::wtof(str), d) == 0)
        {
            wchar_t* p = str;
            while (*p)
            {
                if (*p == L',')
                    *p = L'.';
                p++;
            }
            return;
        }
    }
    
    swprintf(str, max_buffer, L"%.5f", d);
    
    wchar_t* p = str;
    while (*p)
    {
        if (*p == L',')
            *p = L'.';
        p++;
    }
}


void NodeValue::setDouble(double value)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    wchar_t buf[80];
    nolocale_dbltostr(value, buf, 80);

    if (m_iv->m_value != buf)
    {
        m_iv->m_value = buf;
        setDirty();
    }
}


int NodeValue::getInteger()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return kl::wtoi(m_iv->m_value);
}

void NodeValue::setInteger(int value)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    wchar_t buf[64];
    swprintf(buf, 64, L"%d", value);

    if (m_iv->m_value != buf)
    {
        m_iv->m_value = buf;
        setDirty();
    }
}

bool NodeValue::getBoolean()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (m_iv->m_value.empty())
        return true;

    return (m_iv->m_value != L"0" ? true : false);
}

void NodeValue::setBoolean(bool value)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    wchar_t buf[2];
    buf[1] = 0;
    buf[0] = value ? L'1' : L'0';

    if (m_iv->m_value != buf)
    {
        m_iv->m_value = buf;
        setDirty();
    }
}


unsigned int NodeValue::getChildCount()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return m_iv->m_children.size();
}

const std::wstring empty_value = L"";

const std::wstring& NodeValue::getChildName(unsigned int idx)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (idx >= m_iv->m_children.size())
        return empty_value;

    return m_iv->m_children[idx]->m_name;
}

tango::INodeValuePtr NodeValue::getChildByIdx(unsigned int idx)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (idx >= m_iv->m_children.size())
        return xcm::null;

    NodeValue* v = new NodeValue(m_file, m_iv->m_children[idx]);
    return static_cast<tango::INodeValue*>(v);
}

tango::INodeValuePtr NodeValue::getChild(const std::wstring& name,
                                       bool create_if_not_exist)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    int idx = getChildIdx(name);
    if (idx != -1)
    {
        NodeValue* v = new NodeValue(m_file, m_iv->m_children[idx]);
        return static_cast<tango::INodeValue*>(v);
    }

    if (!create_if_not_exist)
        return xcm::null;

    return createChild(name);
}

tango::INodeValuePtr NodeValue::createChild(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (getChildIdx(name) != -1)
        return xcm::null;

    setDirty();

    InternalNodeValue* new_child = new InternalNodeValue;
    new_child->m_name = name;
    new_child->m_value = L"";

    m_iv->m_children.push_back(new_child);
    m_iv->m_lookup_map[name] = m_iv->m_children.size() - 1;

    NodeValue* v = new NodeValue(m_file, new_child);
    return static_cast<tango::INodeValue*>(v);
}


bool NodeValue::getChildExist(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return (getChildIdx(name) != -1) ? true : false;
}

bool NodeValue::renameChild(const std::wstring& name,
                           const std::wstring& new_name)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    int idx = getChildIdx(name);
    if (idx == -1)
        return false;

    m_iv->m_children[idx]->m_name = new_name;
    m_iv->m_lookup_map.erase(name);
    m_iv->m_lookup_map[new_name] = idx;

    setDirty();

    return true;
}

bool NodeValue::deleteChild(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);


    int idx = getChildIdx(name);
    if (idx == -1)
        return false;

    InternalNodeValue* child = m_iv->m_children[idx];
    m_iv->m_children.erase(m_iv->m_children.begin() + idx);
    delete child;
    m_iv->m_lookup_map.clear();
    setDirty();

    return true;
}

bool NodeValue::deleteAllChildren()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    m_iv->m_lookup_map.clear();
    if (m_iv->m_children.size() == 0)
        return true;

    m_iv->clear();

    setDirty();

    return true;
}


int NodeValue::getChildIdx(const std::wstring& name)
{
    std::map<std::wstring, int>::iterator mit;
    std::vector<InternalNodeValue*>::iterator vit;
    int idx;
    
    if (m_iv->m_children.size() != m_iv->m_lookup_map.size())
    {
        // reindex the lookup cache
        m_iv->m_lookup_map.clear();

        // otherwise, do a slow O(n) lookup
        idx = 0;
        for (vit = m_iv->m_children.begin();
             vit != m_iv->m_children.end();
             ++vit)
        {
            m_iv->m_lookup_map[(*vit)->m_name] = idx;
            idx++;
        }
    }


    // attempt to lookup index in the map

    mit = m_iv->m_lookup_map.find(name);
    if (mit != m_iv->m_lookup_map.end())
    {
        return mit->second;
    }

    return -1;
}




void NodeValue::ref()
{
    m_refcount_mutex.lock();
    m_refcount_holder.xcm_ref_count++;
    m_file->ref();
    m_refcount_mutex.unlock();
}

void NodeValue::unref()
{
    m_refcount_mutex.lock();
    if (--m_refcount_holder.xcm_ref_count == 0)
    {
        m_file->unref();
        m_refcount_mutex.unlock();
        delete this;
        return;
    }
    m_file->unref();
    m_refcount_mutex.unlock();
}

int NodeValue::get_ref_count()
{
    m_refcount_mutex.lock();
    int result = m_refcount_holder.xcm_ref_count;
    m_refcount_mutex.unlock();
    return result;
}






static int stringTypeToFileType(const std::wstring& type)
{
    if (type.empty())
        return tango::filetypeNode;
        
    if (0 == wcscasecmp(type.c_str(), L"node") ||
        0 == wcscasecmp(type.c_str(), L"generic"))
    {
        return tango::filetypeNode;
    }
     else if (0 == wcscasecmp(type.c_str(), L"folder"))
    {
        return tango::filetypeFolder;
    }
     else if (0 == wcscasecmp(type.c_str(), L"set"))
    {
        return tango::filetypeSet;
    }
     else if (0 == wcscasecmp(type.c_str(), L"stream"))
    {
        return tango::filetypeStream;
    }
    
    return tango::filetypeNode;
}


static std::wstring fileTypeToStringType(int type)
{
    switch (type)
    {
        default:
        case tango::filetypeNode:    return L"generic";
        case tango::filetypeFolder:  return L"folder";
        case tango::filetypeSet:     return L"set";
        case tango::filetypeStream:  return L"stream";
    }
    
    return L"generic";
}



static InternalNodeValue* jsonToNodeValue(JsonNode& jn)
{
    InternalNodeValue* value = new InternalNodeValue;

    std::vector<std::wstring> keys = jn.getChildKeys();

    if (keys.size() > 0)
    {
        std::vector<std::wstring>::iterator it;

        for (it = keys.begin(); it < keys.end(); ++it)
        {
            JsonNode child = jn[*it];

            InternalNodeValue* child_value = jsonToNodeValue(child);
            child_value->m_name = *it;
            if (!child_value)
            {
                delete value;
                return NULL;
            }

            value->m_children.push_back(child_value);
        }
    }
     else
    {
        value->m_value = jn.getString();
    }

    return static_cast<InternalNodeValue*>(value);
}


static void nodeValueToJson(const InternalNodeValue* value, JsonNode& node)
{
    if (!value)
        return;

    if (value->m_children.size() > 0)
    {
        std::vector<InternalNodeValue*>::const_iterator it;
        
        for (it = value->m_children.begin(); it != value->m_children.end(); ++it)
        {
            JsonNode child = node[(*it)->m_name];
            nodeValueToJson(*it, child);
        }
    }
     else
    {
        node.setString(value->m_value);
    }
}



NodeFile::NodeFile(ClientDatabase* db)
{
    m_root_node = new InternalNodeValue;
    m_root_node->m_name = L"root";
    m_root_node->m_value = L"";

    m_type = tango::filetypeNode;
    m_dirty = false;
    m_database = db;
}

NodeFile::~NodeFile()
{
    if (m_dirty)
    {
        writeFile();
    }

    if (m_root_node)
    {
        m_root_node->clear();
        delete m_root_node;
    }
}




bool NodeFile::readFile()
{
    XCM_AUTO_LOCK(m_object_mutex);

    ServerCallParams params;
    params.setParam(L"path", m_path);
    std::wstring sres = m_database->serverCall(L"/api/readnodefile", &params);

    JsonNode response_node;
    response_node.fromString(sres);

    if (!response_node["success"].getBoolean())
        return false;


    JsonNode data = response_node["data"];


    if (m_root_node)
    {
        m_root_node->clear();
        delete m_root_node;
        m_root_node = NULL;
    }

    m_root_node = jsonToNodeValue(data);

    if (!m_root_node)
        return false;

    m_root_node->m_name = L"root";
    m_root_node->m_value = L"";

    return true;
}

bool NodeFile::writeFile()
{
    XCM_AUTO_LOCK(m_object_mutex);

    if (!m_dirty)
        return true;

    JsonNode json;
    nodeValueToJson(m_root_node, json);

    std::wstring data = json.toString();

    ServerCallParams params;
    params.setParam(L"path", m_path);
    params.setParam(L"data", data);
    std::wstring sres = m_database->serverCall(L"/api/writenodefile", &params);

    JsonNode response_node;
    response_node.fromString(sres);

    return response_node["success"].getBoolean();
}

void NodeFile::setDirty(bool new_value)
{
    XCM_AUTO_LOCK(m_object_mutex);

    m_dirty = new_value;
}

std::wstring NodeFile::getPath()
{
    XCM_AUTO_LOCK(m_object_mutex);

    return m_path;
}

int NodeFile::getType()
{
    XCM_AUTO_LOCK(m_object_mutex);

    return m_type;
}

void NodeFile::setType(int new_type)
{
    XCM_AUTO_LOCK(m_object_mutex);

    m_type = new_type;

    setDirty(true);
}

tango::INodeValuePtr NodeFile::getRootNode()
{
    XCM_AUTO_LOCK(m_object_mutex);

    NodeValue* v = new NodeValue(this, m_root_node);
    return static_cast<tango::INodeValue*>(v);
}


NodeFile* NodeFile::createFile(ClientDatabase* db,
                               const std::wstring& path)
{
    NodeFile* file = new NodeFile(db);
    file->m_path = path;
    return file;
}



NodeFile* NodeFile::openFile(ClientDatabase* db,
                             const std::wstring& path)
{
    NodeFile* file = new NodeFile(db);
    file->m_path = path;
    if (!file->readFile())
    {
        delete file;
        return NULL;
    }

    return file;
}
