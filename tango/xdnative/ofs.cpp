/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-12
 *
 */


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "util.h"
#include "ofs.h"
#include "../xdcommon/xdcommon.h"


/*
// good for checking object lifetime
#include <windows.h>

int countt = 0;

class Munch
{
public:
    ~Munch()
    {
        wchar_t b[255];
        swprintf(b, 255, L"%d", countt);
        ::MessageBox(NULL, b, L"", MB_OK);
    }
};
Munch m;
*/


OfsValue::OfsValue(OfsFile* file, InternalOfsValue* value)
{
    m_file = file;
    m_iv = value;
    
    m_file->ref();
}

OfsValue::~OfsValue()
{
    m_file->unref();
}

void OfsValue::setDirty()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    m_file->setDirty(true);
}

int OfsValue::getType()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return m_iv->m_type;
}

const std::wstring& OfsValue::getName()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return m_iv->m_name;
}

const std::wstring& OfsValue::getString()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return m_iv->m_value;
}

void OfsValue::setString(const std::wstring& value)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (m_iv->m_value != value)
    {
        m_iv->m_value = value;
        setDirty();
    }
}


double OfsValue::getDouble()
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


void OfsValue::setDouble(double value)
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


int OfsValue::getInteger()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return kl::wtoi(m_iv->m_value);
}

void OfsValue::setInteger(int value)
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

bool OfsValue::getBoolean()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (m_iv->m_value.empty())
        return true;

    return (m_iv->m_value != L"0" ? true : false);
}

void OfsValue::setBoolean(bool value)
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


unsigned int OfsValue::getChildCount()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return m_iv->m_children.size();
}

const std::wstring empty_value = L"";

const std::wstring& OfsValue::getChildName(unsigned int idx)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (idx >= m_iv->m_children.size())
        return empty_value;

    return m_iv->m_children[idx]->m_name;
}

INodeValuePtr OfsValue::getChildByIdx(unsigned int idx)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (idx >= m_iv->m_children.size())
        return xcm::null;

    OfsValue* v = new OfsValue(m_file, m_iv->m_children[idx]);
    return static_cast<INodeValue*>(v);
}

INodeValuePtr OfsValue::getChild(const std::wstring& name,
                                       bool create_if_not_exist)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    int idx = getChildIdx(name);
    if (idx != -1)
    {
        OfsValue* v = new OfsValue(m_file, m_iv->m_children[idx]);
        return static_cast<INodeValue*>(v);
    }

    if (!create_if_not_exist)
        return xcm::null;

    return createChild(name);
}

INodeValuePtr OfsValue::createChild(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    if (getChildIdx(name) != -1)
        return xcm::null;

    setDirty();

    InternalOfsValue* new_child = new InternalOfsValue;
    new_child->m_name = name;
    new_child->m_value = L"";

    m_iv->m_children.push_back(new_child);
    m_iv->m_lookup_map[name] = m_iv->m_children.size() - 1;

    OfsValue* v = new OfsValue(m_file, new_child);
    return static_cast<INodeValue*>(v);
}


bool OfsValue::getChildExist(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    return (getChildIdx(name) != -1) ? true : false;
}

bool OfsValue::renameChild(const std::wstring& name,
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

bool OfsValue::deleteChild(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);


    int idx = getChildIdx(name);
    if (idx == -1)
        return false;

    InternalOfsValue* child = m_iv->m_children[idx];
    m_iv->m_children.erase(m_iv->m_children.begin() + idx);
    delete child;
    m_iv->m_lookup_map.clear();
    setDirty();

    return true;
}

bool OfsValue::deleteAllChildren()
{
    XCM_AUTO_LOCK(m_iv->m_object_mutex);

    m_iv->m_lookup_map.clear();
    if (m_iv->m_children.size() == 0)
        return true;

    m_iv->clear();

    setDirty();

    return true;
}


int OfsValue::getChildIdx(const std::wstring& name)
{
    std::map<std::wstring, int>::iterator mit;
    std::vector<InternalOfsValue*>::iterator vit;
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




void OfsValue::ref()
{
    m_refcount_mutex.lock();
    m_refcount_holder.xcm_ref_count++;
    m_file->ref();
    m_refcount_mutex.unlock();
}

void OfsValue::unref()
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

int OfsValue::get_ref_count()
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
        return tango::filetypeTable;
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
        case tango::filetypeTable:     return L"set";
        case tango::filetypeStream:  return L"stream";
    }
    
    return L"generic";
}



static InternalOfsValue* xmlToOfsValue(OfsFile* file, kl::xmlnode& node)
{
    int name_prop_idx = node.getPropertyIdx(L"name");
    if (name_prop_idx == -1)
        return NULL;

    InternalOfsValue* value = new InternalOfsValue;

    value->m_name = node.getProperty(name_prop_idx).value;

    int value_idx = node.getChildIdx(L"value");
    if (value_idx != -1)
    {
        kl::xmlnode& value_node = node.getChild(value_idx);
        value->m_value = value_node.getNodeValue();
    }

    int entries_idx = node.getChildIdx(L"entries");
    if (entries_idx != -1)
    {
        kl::xmlnode& entries_node = node.getChild(entries_idx);
        int entry_count = entries_node.getChildCount();

        for (int i = 0; i < entry_count; ++i)
        {
            kl::xmlnode& entry = entries_node.getChild(i);
            InternalOfsValue* child_value = xmlToOfsValue(file, entry);

            if (!child_value)
            {
                delete value;
                return NULL;
            }

            value->m_children.push_back(child_value);
        }
    }

    return static_cast<InternalOfsValue*>(value);
}


static void ofsValueToXml(kl::xmlnode& node, const InternalOfsValue* value)
{
    kl::xmlnode& base = node.addChild();
    base.setNodeName(L"ofs_node");

    kl::xmlproperty& name_prop = base.appendProperty();
    name_prop.name = L"name";
    name_prop.value = value->m_name;

    kl::xmlnode& value_node = base.addChild();
    value_node.setNodeName(L"value");

    if (value)
    {
        value_node.setNodeValue(value->m_value);
    }


    if (value && value->m_children.size() > 0)
    {
        kl::xmlnode& entries_node = base.addChild();
        entries_node.setNodeName(L"entries");
        
        std::vector<InternalOfsValue*>::const_iterator it;
        
        for (it = value->m_children.begin();
             it != value->m_children.end();
             ++it)
        {
            ofsValueToXml(entries_node, *it);
        }
    }
}


OfsFile::OfsFile(tango::IDatabase* db)
{
    m_root_node = new InternalOfsValue;
    m_root_node->m_name = L"root";
    m_root_node->m_value = L"";

    m_type = tango::filetypeNode;
    m_dirty = false;
    m_database = db;
}

OfsFile::~OfsFile()
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



bool OfsFile::readFile()
{
    XCM_AUTO_LOCK(m_object_mutex);

    kl::xmlnode doc;

    std::wstring phys_filename = m_database->ofsToPhysFilename(m_key_path, false);
    if (!doc.load(phys_filename))
    {
        return false;
    }
    
    // parse file type

    int type_idx = doc.getChildIdx(L"file_type");
    if (type_idx == -1)
    {
        m_type = tango::filetypeNode;
    }
     else
    {
        kl::xmlnode& type_node = doc.getChild(type_idx);
        const std::wstring& type = type_node.getNodeValue();

        m_type = stringTypeToFileType(type);
    }

    // parse file nodes

    int root_idx = doc.getChildIdx(L"ofs_node");
    if (root_idx == -1)
        return false;

    if (m_root_node)
    {
        m_root_node->clear();
        delete m_root_node;
        m_root_node = NULL;
    }

    m_root_node = xmlToOfsValue(this, doc.getChild(root_idx));

    if (!m_root_node)
        return false;

    return true;
}

bool OfsFile::writeFile()
{
    XCM_AUTO_LOCK(m_object_mutex);

    if (!m_dirty)
        return true;

    kl::xmlnode doc;

    doc.setNodeName(L"ofs_file");

    kl::xmlproperty& version_prop = doc.appendProperty();
    version_prop.name = L"version";
    version_prop.value = L"1.0";
    
    kl::xmlnode& filetype_node = doc.addChild();
    filetype_node.setNodeName(L"file_type");
    filetype_node.setNodeValue(fileTypeToStringType(m_type));


    ofsValueToXml(doc, m_root_node);

    m_dirty = false;


    if (m_type == tango::filetypeFolder)
    {
        // if 'this' is a folder, create it if it doesn't exist
        if (m_key_path != L"/")
        {
            std::wstring folder = makePathName(m_ofs_root, m_key_path, L"", L"");
            if (!xf_get_directory_exist(folder))
            {
                if (!xf_mkdir(folder))
                    return false;
            }
        }
    }

    return doc.save(makePathName(m_ofs_root, m_key_path, L"", L"xml"));
}

void OfsFile::setDirty(bool new_value)
{
    XCM_AUTO_LOCK(m_object_mutex);

    m_dirty = new_value;
}

const std::wstring& OfsFile::getPath()
{
    // don't lock m_object_mutex here -- it causes an interlocking
    // deadlock in ofs lookup mechanism for example in the for loop
    // in Database::openLocalNodeFile(); besides, it's not necessary
    // because m_key_path doesn't change during the lifetime of
    // an OfsFile object
    return m_key_path;
}

int OfsFile::getType()
{
    XCM_AUTO_LOCK(m_object_mutex);

    return m_type;
}

void OfsFile::setType(int new_type)
{
    XCM_AUTO_LOCK(m_object_mutex);

    m_type = new_type;

    setDirty(true);
}

INodeValuePtr OfsFile::getRootNode()
{
    // don't lock here; see comment in OfsFile::getPath()
    OfsValue* v = new OfsValue(this, m_root_node);
    return static_cast<INodeValue*>(v);
}


OfsFile* OfsFile::createFile(tango::IDatabase* db,
                             const std::wstring& key_path,
                             int type)
{
    IDatabaseInternalPtr dbi = db;
    std::wstring ofs_root = dbi->getOfsPath();

    std::wstring dir = ofs_root;

    if (dir.empty() || dir[dir.length()-1] != PATH_SEPARATOR_CHAR)
        dir += PATH_SEPARATOR_CHAR;

    std::wstring build_key = L"/";
    wchar_t* buf = wcsdup(key_path.c_str());
    wchar_t* start;
    wchar_t* sl;
    bool end;

    start = buf;
    if (*start == L'/')
        start++;

    while (1)
    {
        sl = wcschr(start, L'/');
        end = (sl ? false : true);
        if (!end)
        {
            *sl = 0;

            build_key += start;
            dir += start;
            start = sl+1;

            if (!xf_get_directory_exist(dir))
            {
                xf_mkdir(dir);
            }

            // write out empty key file too
            std::wstring temps;
            temps = dir;
            temps += L".xml";

            if (!xf_get_file_exist(temps))
            {
                OfsFile* file = new OfsFile(db);
                file->ref();
                file->m_key_path = build_key;
                file->m_ofs_root = ofs_root;
                file->setDirty(true);
                file->setType(tango::filetypeFolder);
                file->writeFile();
                file->unref();
            }

            *sl = L'/';

            build_key += L"/";
            #ifdef WIN32
            dir += L"\\";
            #else
            dir += L"/";
            #endif
        }
         else
        {
            dir += start;
            dir += L".xml";
            break;
        }
    }

    free(buf);


    OfsFile* file = new OfsFile(db);
    file->ref();

    file->m_key_path = key_path;
    file->m_ofs_root = ofs_root;
    file->setType(type);
    file->setDirty(true);
    file->writeFile(); 
    file->m_database->registerNodeFile(file);

    return file;
}



OfsFile* OfsFile::openFile(tango::IDatabase* db,
                           const std::wstring& _key_path)
{
    if (_key_path.empty())
        return NULL;

    IDatabaseInternalPtr dbi = db;
    std::wstring ofs_root = dbi->getOfsPath();

    std::wstring key_path;
    if (*(_key_path.c_str()) != L'/')
    {
        key_path = L"/";
    }
    key_path += _key_path;

    OfsFile* file = new OfsFile(db);
    file->ref();
    file->m_key_path = key_path;
    file->m_ofs_root = ofs_root;

    if (key_path != L"/")
    {
        // attempt to open existing key
        if (!file->readFile())
        {
            delete file;
            return NULL;
        }
    }
     else
    {
        // root directory is a folder
        file->setType(tango::filetypeFolder);
        file->setDirty(false);
    }

    file->m_database->registerNodeFile(file);
    return file;
}




// parsing the whole file just to get the file type is
// a large waste of time.  Since the <file_type> tag is
// very close to the beginning, just look for that without
// parsing the xml

bool OfsFile::getFileType(tango::IDatabase* db,
                              const std::wstring& _key_path,
                              int* type,
                              bool* is_mount)
{
    if (_key_path.empty())
        return false;
        
    if (_key_path == L"/")
        return false;
    
    
    // form the real file name of the file
    IDatabaseInternalPtr dbi = db;
    
    std::wstring key_path;
    if (*(_key_path.c_str()) != L'/')
        key_path = L"/";
    key_path += _key_path;
    
    std::wstring filename = makePathName(dbi->getOfsPath(), key_path, L"", L"xml");
    
    // open the file and read the first portion
    xf_file_t f = xf_open(filename, xfOpen, xfRead, xfShareRead);
    if (!f)
        return false;

    unsigned char* buf = new unsigned char[1048];
    int readb = 0;
    int r = 0;

    readb = xf_read(f, buf, 1, 1024);
    memset(buf+readb, 0, 8);

    xf_close(f);

    if (readb < 4)
        return false;


    // get the chunk into a string
    std::wstring dest;
    
    if (buf[0] == 0xff && buf[1] == 0xfe &&
        (buf[2] != 0x00 || buf[3] != 0x00))
    {
        // little endian unicode
        kl::ucsle2wstring(dest, buf+2, 523);
    }
     else
    {
        dest = kl::towstring((char*)buf);
    }

    delete[] buf;
    
    
    
    // try to find out the file type
    int t = -1;
    bool m = false;
    std::wstring stype;
    
    const wchar_t* p = wcsstr(dest.c_str(), L"<file_type>");
    if (p)
    {
        p += 11;
        const wchar_t* end = wcschr(p, '<');
        if (end)
        {
            stype.assign(p, end-p);
            
            t = stringTypeToFileType(stype);
            
            p = wcsstr(dest.c_str(), L"\"connection_str\"");
            m = p ? true : false;
        }
    }

    // if we couldn't determine the file type
    // via our arts, use classical methods
    if (t == -1)
    {
        OfsFile* file = OfsFile::openFile(db, _key_path);
        if (!file)
            return false;
        
        t = file->getType();
        
        INodeValuePtr root = file->getRootNode();
        if (root.isOk())
        {
            INodeValuePtr cs = root->getChild(L"connection_str", false);
            if (cs.isOk())
            {
                m = true;
            }
             else
            {
                m = false;
            }
            root.clear();
            cs.clear();
        }
        
        file->unref();
    }
    
    
    // assign results
    if (type)
        *type = t;
        
    if (is_mount)
        *is_mount = m;
        
    return true;
}

