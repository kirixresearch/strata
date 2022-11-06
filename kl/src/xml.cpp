/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2008-06-28
 *
 */
 

#include <kl/xml.h>
#include <cstdio>
#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>
#include <kl/file.h>
#include <kl/string.h>
#include <kl/system.h>


const wchar_t* KL_XML_HEADER = L"<?xml version=\"1.0\"?>";


// faster iswspace
#define iswspace(ch) ((ch == L' ') || (ch == L'\n') || (ch == L'\r') || (ch == L'\t'))


namespace kl
{


xmlnode empty_xmlnode;
xmlproperty empty_xmlproperty;
std::wstring empty_wstring;


static size_t fromUTF8(wchar_t* dest,
                       size_t dest_size,
                       const char* src,
                       size_t src_size)
{            
    static const unsigned long offsets_from_utf8[6] =
    {
        0x00000000UL,
        0x00003080UL,
        0x000E2080UL,
        0x03C82080UL,
        0xFA082080UL,
        0x82082080UL
    };

    static const char trailing_bytes_for_utf8[256] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
    };

    const char *src_end = src + src_size;
    unsigned long ch;
    int num_bytes;
    size_t i = 0;

    if (dest_size == 0)
        return 0;

    while (i < dest_size - 1)
    {
        if (src_size == -1)
        {
            if (*src == 0)
                break;
            num_bytes = trailing_bytes_for_utf8[(unsigned char)*src];
        }
         else
        {
            if (src >= src_end)
                break;
            num_bytes = trailing_bytes_for_utf8[(unsigned char)*src];
            if (src + num_bytes >= src_end)
                break;
        }
        
        ch = 0;
        
        switch (num_bytes)
        {
            case 3: ch += (unsigned char)*src++; ch <<= 6;
            case 2: ch += (unsigned char)*src++; ch <<= 6;
            case 1: ch += (unsigned char)*src++; ch <<= 6;
            case 0: ch += (unsigned char)*src++;
        }
        
        ch -= offsets_from_utf8[num_bytes];
        
        dest[i++] = (wchar_t)ch;
    }
    
    dest[i] = 0;
    return i;
}




xmlnode::xmlnode()
{
}

xmlnode::~xmlnode()
{
    std::vector<xmlnode*>::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); it++)
        delete *it;

    std::vector<xmlproperty*>::iterator p_it;
    for (p_it = m_properties.begin(); p_it != m_properties.end(); p_it++)
        delete *p_it;
}

bool xmlnode::parse(const wchar_t* xml_text, int parse_flags)
{
    const wchar_t* p;
    const wchar_t* start = xml_text;
    
    while (1)
    {
        start = wcschr(start, L'<');
        if (!start)
            return false;
        
        if (*(start+1) != L'?' && *(start+1) != L'!')
            break;
        start++;
    }
    
    return internalParse(start, &p, parse_flags);
}

bool xmlnode::parse(const char* xml_text, int parse_flags)
{
    bool utf8 = false;
    
    // detect utf-8 encoding
    const char* xml_header = strstr(xml_text, "?xml");
    if (xml_header)
    {
        const char* header_end = strchr(xml_header, '>');
        if (!header_end)
            return false;
            
        std::string header(xml_header, header_end-xml_header);
        size_t i, len = header.length();
        for (i = 0; i < len; ++i)
            header[i] = (char)towlower(header[i]);
        if (header.find("\"utf-8\"") != -1)
        {
            utf8 = true;
        }
    }
    
    if (utf8)
    {
        int len = strlen(xml_text);
        wchar_t* wbuf = new wchar_t[len+5];
        
        fromUTF8(wbuf, len, xml_text, len);
        bool ret = parse(wbuf, parse_flags);
        delete[] wbuf;
        
        return ret;
    }
     else
    {
        std::wstring wstr = kl::towstring(xml_text);
        return parse(wstr.c_str(), parse_flags);
    }
}

bool xmlnode::parse(const std::wstring& xml_text, int parse_flags)
{
    return parse(xml_text.c_str(), parse_flags);
}

bool xmlnode::parse(const std::string& xml_text, int parse_flags)
{
    return parse(xml_text.c_str(), parse_flags);
}




bool xmlnode::load(const std::wstring& filename, int parse_flags, int file_mode)
{
    if (!xf_get_file_exist(filename))
        return false;

    xf_file_t f;

    if (file_mode == filemodeShareRead)
    {
        f = xf_open(filename, xfOpen, xfRead, xfShareRead);
        if (!f)
            return false;
    }
     else if (file_mode == filemodeExclusive)
    {
        f = xf_open(filename, xfOpen, xfRead, xfShareNone);
        if (!f)
            return false;
    }
     else if (file_mode == filemodeExclusiveWait)
    {
        for (int i = 0; i < 70; ++i)
        {
            f = xf_open(filename, xfOpen, xfRead, xfShareNone);
            if (f)
                break;

            kl::millisleep(100);
        }

        if (!f)
            return false;
    }
     else
    {
        return false;
    }

    int file_size = (int)xf_get_file_size(filename);

    unsigned char* buf = new unsigned char[file_size+64];
    if (!buf)
    {
        // out of memory
        return false;
    }
    
    int readb = 0;
    int r = 0;

    readb = xf_read(f, buf, 1, file_size);

    xf_close(f);

    memset(buf+readb, 0, 8);

    if (readb < 4)
        return false;

    bool res = false;


    if (buf[0] == 0xff && buf[1] == 0xfe &&
        (buf[2] != 0x00 || buf[3] != 0x00))
    {
        // little endian unicode

        // this code will only work on little endian machines
        // where sizeof(wchar_t) == 2

        if (sizeof(wchar_t) == 2)
        {
            wchar_t* wc_buf = (wchar_t*)(buf+2);
            res = parse(wc_buf, parse_flags);
        }
         else
        {
            int len = readb/2;
            wchar_t* out = new wchar_t[len+1];
            if (!out)
            {
                // out of memory
                delete[] buf;
                return false;
            }
            
            wchar_t* p = out;
            int off = 0;
            while (1)
            {
                *p = (wchar_t)buf[off++];
                *p |= (((wchar_t)buf[off++]) << 8);
                p++;
                if (off >= readb)
                    break;
            }
            out[len] = 0;
            res = parse(out, parse_flags);
            delete[] out;
        }
    }
     else
    {
        // for now default to regular ASCII
        res = parse((const char*)buf, parse_flags);
    }

    delete[] buf;

    return res;
}

bool xmlnode::save(const std::wstring& filename, int flags, int file_mode)
{
    xf_remove(filename);

    xf_file_t file;
    
    if (file_mode == filemodeExclusive)
    {
        file = xf_open(filename, xfCreate, xfReadWrite, xfShareNone);
        if (!file)
            return false;
    }
     else if (file_mode == filemodeExclusiveWait)
    {
        for (int i = 0; i < 70; ++i)
        {
            file = xf_open(filename, xfCreate, xfReadWrite, xfShareNone);
            if (file)
                break;

            kl::millisleep(100);
        }

        if (!file)
            return false;
    }
     else
    {
        return false;
    }


    std::wstring str;

    if ((flags & 0x0f) == 0)
    {
        #ifdef WIN32
        flags |= formattingSpaces | formattingCrLf;
        #else
        flags |= formattingSpaces | formattingLf;
        #endif
    }
    
    internalGetXML(str, flags);
    

    // write out unicode byte order mark
    unsigned char bom[2];
    bom[0] = 0xff;
    bom[1] = 0xfe;

    if (1 != xf_write(file, bom, 2, 1))
    {
        xf_close(file);
        return false;
    }


    if (sizeof(wchar_t) == 2)
    {
        int res = xf_write(file, (void*)str.c_str(), 1, sizeof(wchar_t) * str.length());
        if ((unsigned int)res != (sizeof(wchar_t) * str.length()))
        {
            xf_close(file);
            return false;
        }
    }
     else
    {
        int len = str.length();
        int i;
        unsigned char* buf = new unsigned char[len * 2];
        unsigned char* p = buf;
        for (i = 0; i < len; ++i)
        {
            *p = (str[i] & 0xff);
            ++p;
            *p = (str[i] >> 8) & 0xff;
            ++p;
        }
        int res = xf_write(file, buf, 1, len * 2);
        delete[] buf;
        if (res != len * 2)
        {
            xf_close(file);
            return false;
        }
    }

    xf_close(file);

    return true;
}

xmlnode& xmlnode::copy(xmlnode& node, bool deep)
{
    // clear the current node
    clear();
    
    // copy the input node to this node
    xmlnode* copy = NULL;
    internalClone(&node, &copy, deep);

    m_nodes = copy->m_nodes;
    m_properties = copy->m_properties;
    m_tag_name = copy->m_tag_name;
    m_contents = copy->m_contents;
    
    return *this;
}

xmlnode& xmlnode::clone(bool deep)
{
    xmlnode* copy = NULL;
    internalClone(this, &copy, deep);
    return *copy;
}

void xmlnode::clear()
{
    std::vector<xmlnode*>::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); it++)
        delete *it;

    std::vector<xmlproperty*>::iterator p_it;
    for (p_it = m_properties.begin(); p_it != m_properties.end(); p_it++)
        delete *p_it;

    m_nodes.clear();
    m_properties.clear();
    m_tag_name = L"";
    m_contents = L"";
}

xmlnode& xmlnode::addChild()
{
    m_nodes.push_back(new xmlnode);
    return *(m_nodes.back());
}

xmlnode& xmlnode::insertChild(size_t idx)
{
    m_nodes.insert(m_nodes.begin() + idx, new xmlnode);
    return *(m_nodes[idx]);
}

xmlnode& xmlnode::addChild(const std::wstring& tag_name)
{
    xmlnode& new_node = addChild();
    new_node.setNodeName(tag_name);
    new_node.setNodeValue(L"");
    return new_node;
}

xmlnode& xmlnode::addChild(const std::wstring& tag_name, const std::wstring& contents)
{
    xmlnode& new_node = addChild();
    new_node.setNodeName(tag_name);
    new_node.setNodeValue(contents);
    return new_node;
}

xmlnode& xmlnode::addChild(const std::wstring& tag_name, int contents)
{
    xmlnode& new_node = addChild();
    new_node.setNodeName(tag_name);
    new_node.setNodeValue(contents);
    return new_node;
}

void xmlnode::setNodeName(const std::wstring& tag_name)
{
    m_tag_name = tag_name;
}

void xmlnode::setNodeValue(const std::wstring& contents)
{
    m_contents = contents;
}

void xmlnode::setNodeValue(int contents)
{
    wchar_t buf[24];
    #ifndef _MSC_VER
    swprintf(buf, 23, L"%d", contents);
    #else
    _snwprintf(buf, 23, L"%d", contents);
    #endif
    buf[23] = 0;
    m_contents = buf;
}

const std::wstring& xmlnode::getNodeName() const
{
    return m_tag_name;
}

const std::wstring& xmlnode::getNodeValue() const
{
    return m_contents;
}

std::wstring xmlnode::getXML(int format_flags) const
{
    std::wstring result;
    internalGetXML(result, format_flags);
    return result;
}

size_t xmlnode::getChildCount() const
{
    return m_nodes.size();
}

bool xmlnode::hasChildNodes() const
{
    return (m_nodes.size() > 0) ? true : false;
}

xmlnode& xmlnode::getChild(const std::wstring& tag_name)
{
    int idx = getChildIdx(tag_name);
    if (idx == -1)
        return empty_xmlnode;
    return getChild(idx);
}

xmlnode& xmlnode::getChild(size_t idx)
{
    return (*m_nodes[idx]);
}

void xmlnode::deleteAllChildren()
{
    std::vector<xmlnode*>::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); it++)
    {
        delete *it;
    }
    
    m_nodes.clear();
}

bool xmlnode::removeChild(size_t idx)
{
    if (idx < 0 || idx >= m_nodes.size())
        return false;

    xmlnode* n = m_nodes[idx];
    m_nodes.erase(m_nodes.begin() + idx);
    delete n;

    return true;
}

bool xmlnode::removeChild(const kl::xmlnode& node)
{
    std::vector<xmlnode*>::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); it++)
    {
        if ((*it) == &node)
        {
            m_nodes.erase(it);
            return true;
        }
    }
    return false;
}

int xmlnode::getChildIdx(const std::wstring& tag_name)
{
    int i = 0;
    std::vector<xmlnode*>::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); it++)
    {
        if ((*it)->m_tag_name == tag_name)
            return i;
        ++i;
    }

    return -1;
}

int xmlnode::lookupChild(const std::wstring& tag_name,
                         const std::wstring& prop_name,
                         const std::wstring& prop_value)
{
    int i = 0;
    int prop_idx;
    std::vector<xmlnode*>::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); it++)
    {
        if ((*it)->m_tag_name != tag_name)
        {
            i++;
            continue;
        }
        
        if (prop_name.length() == 0)
            return i;

        prop_idx = (*it)->getPropertyIdx(prop_name);

        if (prop_idx >= 0)
        {
            xmlproperty& prop = (*it)->getProperty(prop_idx);
            if (prop.value == prop_value)
                return i;
        }

        i++;
    }

    return -1;
}

bool xmlnode::isEmpty() const
{
    return m_tag_name.empty();
}

xmlproperty& xmlnode::appendProperty()
{
    m_properties.push_back(new xmlproperty);
    return *(m_properties.back());
}

xmlproperty& xmlnode::appendProperty(const std::wstring& name,
                                     const std::wstring& value)
{
    xmlproperty* prop = new xmlproperty;
    prop->name = name;
    prop->value = value;
    m_properties.push_back(prop);
    return *(m_properties.back());
}

size_t xmlnode::getPropertyCount() const
{
    return m_properties.size();
}

xmlproperty& xmlnode::getProperty(size_t idx)
{
    if (idx >= m_properties.size())
        return empty_xmlproperty;

    return *(m_properties[idx]);
}

xmlproperty& xmlnode::getProperty(const std::wstring& name)
{
    std::vector<xmlproperty*>::iterator it;
    for (it = m_properties.begin(); it != m_properties.end(); it++)
    {
        if ((*it)->name == name)
            return *(*it);
    }

    return empty_xmlproperty;
}

int xmlnode::getPropertyIdx(const std::wstring& name) const
{
    int idx = 0;

    std::vector<xmlproperty*>::const_iterator it;
    for (it = m_properties.begin(); it != m_properties.end(); it++)
    {
        if ((*it)->name == name)
            return idx;
        idx++;
    }

    return -1;
}

const std::wstring& xmlnode::getPropertyValue(size_t idx) const
{
    if (idx >= m_properties.size())
        return empty_wstring;

    return m_properties[idx]->value;
}

// static
int xmlnode::getLengthAfterTokenization(const wchar_t* src)
{
    int len = 0;
    while (*src)
    {
        if (*src == L'<' || *src == L'>')
            len += 3;
        else if (*src == L'&')
            len += 4;
        else if (*src == L'"')
            len += 5;
        //else if (*src == L'\'')
        //    len += 5;
        len++;
        src++;
    }
    return len;
}

// static
void xmlnode::tokenizeAppend(std::wstring& dest, const std::wstring& src)
{
    const wchar_t* s = src.c_str();
    while (*s)
    {
        switch (*s)
        {
            case '<':
                dest += L"&lt;";
                break;
            case '>':
                dest += L"&gt;";
                break;
            case '&':
                dest += L"&amp;";
                break;
            case '"':
                dest += L"&quot;";
                break;
            //case '\'':
            //    dest += L"&apos;";
            //    break;
            default:
                dest += *s;
                break;
        }

        s++;
    }
}

/*
// static
void xmlnode::detokenize(std::wstring& dest, const wchar_t* src, int src_len)
{
    dest = L"";
    dest.reserve(src_len);

    bool in_cdata = false;
    wchar_t ch;
    int i;

    // detokenize &quot; &amp; &lt; and &gt; entities
    
    for (i = 0; i < src_len; i++)
    {
        ch = *(src+i);

        if (in_cdata)
        {
            if (0 == wcsncmp(src+i, L"]]>", 3))
            {
                in_cdata = false;
                i += 2;
                continue;
            }

            dest += *(src+i);
            continue;
        }
         else
        {
            if (ch == L'<')
            {
                if (0 == wcsncmp(src+i, L"<![CDATA[", 9))
                {
                    in_cdata = true;
                    i += 8;
                    continue;
                }
            }
        }

        

        
        if (ch == L'&')
        {
            if (*(src+i+1) == L'#')
            {
                if (i+6 < src_len)
                {
                    int j;
                    char buf[5];
                    memset(buf, 0, 5);
                    for (j = 0; j < 4; ++j)
                    {
                        buf[j] = (char)*(src+i+2+j);
                        buf[j+1] = 0;
                        if (!iswdigit(buf[j]))
                            break;
                    }
                    dest += (wchar_t)(atoi(buf));
                    i += j+2;
                    continue;
                }
            }
            
            if (memcmp(src+i, L"&lt;", 4 * sizeof(wchar_t)) == 0)
            {
                dest += L'<';
                i += 3;
                continue;
            }
            if (memcmp(src+i, L"&gt;", 4 * sizeof(wchar_t)) == 0)
            {
                dest += L'>';
                i += 3;
                continue;
            }
            if (memcmp(src+i, L"&amp;", 5 * sizeof(wchar_t)) == 0)
            {
                dest += L'&';
                i += 4;
                continue;
            }
            if (memcmp(src+i, L"&quot;", 6 * sizeof(wchar_t)) == 0)
            {
                dest += L'"';
                i += 5;
                continue;
            }
            if (memcmp(src+i, L"&apos;", 6 * sizeof(wchar_t)) == 0)
            {
                dest += L'\'';
                i += 5;
                continue;
            }
        }

        dest += ch;
    }
}
*/

// this is a faster version of detokenize (about 30% faster);
// it seems to have been working well for a while now;  the
// old version is available above for reference purposes

//static
void xmlnode::detokenize(std::wstring& dest, const wchar_t* src, int src_len)
{
    int i;
    int append_start = 0;


    dest = L"";
    dest.reserve(src_len);

    bool in_cdata = false;
    
    // detokenize &quot; &amp; &lt; and &gt; entities
    
    for (i = 0; i < src_len; i++)
    {
        if (in_cdata)
        {
            if (0 == wcsncmp(src+i, L"]]>", 3))
            {
                dest.append(src+append_start, i-append_start);
                append_start = i+3;


                in_cdata = false;
                i += 2;
                continue;
            }
        
            continue;
        }
         else
        {
            if (0 == wcsncmp(src+i, L"<![CDATA[", 9))
            {
                dest.append(src+append_start, i-append_start);
                append_start = i+9;


                in_cdata = true;
                i += 8;
                continue;
            }
        }



        
        if (*(src+i) == L'&')
        {
            if (*(src+i+1) == L'#')
            {
                
                if (i+6 < src_len)
                {
                    int j;
                    char buf[5];
                    memset(buf, 0, 5);
                    for (j = 0; j < 4; ++j)
                    {
                        buf[j] = (char)*(src+i+2+j);
                        buf[j+1] = 0;
                        if (!iswdigit(buf[j]))
                            break;
                    }

                    dest.append(src+append_start, i-append_start);

                    dest += (wchar_t)(atoi(buf));
                    i += j+2;
                    
                    append_start = i+1;
                    continue;
                }
            }
            
            if (memcmp(src+i, L"&lt;", 4 * sizeof(wchar_t)) == 0)
            {
                dest.append(src+append_start, i-append_start);

                dest += L'<';
                i += 3;
                append_start = i+1;
                continue;
            }
            if (memcmp(src+i, L"&gt;", 4 * sizeof(wchar_t)) == 0)
            {
                dest.append(src+append_start, i-append_start);

                dest += L'>';
                i += 3;
                append_start = i+1;
                continue;
            }
            if (memcmp(src+i, L"&amp;", 5 * sizeof(wchar_t)) == 0)
            {
                dest.append(src+append_start, i-append_start);

                dest += L'&';
                i += 4;
                append_start = i+1;
                continue;
            }
            if (memcmp(src+i, L"&quot;", 6 * sizeof(wchar_t)) == 0)
            {
                dest.append(src+append_start, i-append_start);

                dest += L'"';
                i += 5;
                append_start = i+1;
                continue;
            }
            if (memcmp(src+i, L"&apos;", 6 * sizeof(wchar_t)) == 0)
            {
                dest.append(src+append_start, i-append_start);

                dest += L'\'';
                i += 5;
                append_start = i+1;
                continue;
            }

        }
    }


    dest.append(src+append_start, i-append_start);
}


bool xmlnode::parseProperty(const wchar_t* p, const wchar_t** endpos)
{
    while (iswspace(*p))
        p++;

    const wchar_t* name_start = p;
    const wchar_t* name_end = NULL;

    while (1)
    {
        if (!iswalnum(*p) && *p != L'_' && *p != L':' && *p != L'-')
        {
            if (*p == L'=')
            {
                name_end = p-1;
                p++;
                while (iswspace(*p))
                    p++;
                break;
            }
            if (iswspace(*p))
            {
                name_end = p-1;

                // ok...we need to find the equal sign
                while (iswspace(*p))
                    p++;
                if (*p != L'=')
                {
                    // parse error
                    return false;
                }
                p++;
                while (iswspace(*p))
                    p++;

                break;
            }

            // parse error
            return false;
        }
        p++;
    }

    xmlproperty* prop = new xmlproperty;

    prop->name.assign(name_start, name_end-name_start+1);


    // now grab the value
    wchar_t quote_char = 0;
    
    if (*p == L'"' || *p == L'\'')
    {
        quote_char=*p;
        p++;
    }

    const wchar_t* value_start = p;
    const wchar_t* value_end = NULL;

    while (1)
    {
        if (*p == quote_char || (!quote_char && iswspace(*p)))
        {
            value_end = p-1;
            p++;
            break;
        }

        if (*p == L'<' || *p == L'>')
        {
            if (quote_char)
            {
                // missing end quotation -- parse error
                return false;
            }
            
            value_end = p-1;
            p++;
            break;
        }
        p++;
    }

    detokenize(prop->value, value_start, value_end-value_start+1);
    
    //prop->value.assign(value_start, value_end-value_start+1);
    
    *endpos = p;

    m_properties.push_back(prop);

    return true;
}

bool xmlnode::internalParse(const wchar_t* xml_text, const wchar_t** next_start, int parse_flags)
{
    const wchar_t* p;
    int tag_name_len;


    while (1)
    {
        while (iswspace(*xml_text))
            xml_text++;

        // internal parser requires that '<' is the first character
        if (*xml_text != L'<')
            return false;

        p = xml_text+1;
        while (iswspace(*p))
            p++;

        // check for comment
        if (wcsncmp(p, L"!--", 3) != 0)
            break;


        // it is a comment
        while (1)
        {
            p++;

            if (!*p)
            {
                // premature eof
                return false;
            }

            if (wcsncmp(p, L"--", 2) == 0)
            {
                // end of comment, look for close bracket
                while (1)
                {
                    p++;
                    if (!*p)
                    {
                        return false;
                    }
                    if (*p == L'>')
                    {
                        xml_text = p+1;
                        break;
                    }
                }

                break;
            }
        }
    }


    p = xml_text+1;
    
    // get our tag name
    const wchar_t* tag_name;
    while (iswspace(*p))
        p++;
    tag_name = p;
    tag_name_len = 0;

    while (1)
    {
        if (*p == L'>' || *p == L'/')
        {
            break;
        }
        if (iswspace(*p))
        {
            while (iswspace(*p))
                p++;
            break;
        }
        if (!iswalnum(*p) && *p != L'_' && *p != L':')
        {
            // premature tag close or invalid syntax
            return false;
        }

        tag_name_len++;
        p++;
    }
    if (!*p)
    {
        // premature EOF
        return false;
    }

    if (tag_name_len == 0)
        return false;

    m_tag_name.assign(tag_name, tag_name_len);


    // parse properties, if present
    while (1)
    {
        while (iswspace(*p))
            p++;
        if (*p == L'>' || *p == L'/')
            break;

        if (!parseProperty(p, &p))
            return false;
    }

    const wchar_t* tag_content_start;
    const wchar_t* next_tag_start;

    p = wcschr(p, L'>');
    if (!p)
    {
        // expected ">" at the end of the close tag, didn't find one
        return false;
    }
    tag_content_start = p+1;
    
    if (!p)
        return false;

    if (*(p-1) == L'/')
    {
        // found "/>" -- no content, we're done
        return true;
    }


    bool cdata_only = false;


    // find the next tag
    while (1)
    {
        while (1)
        {
            const wchar_t* start = p;
            p = wcschr(start, L'<');
            if (!p)
            {
                next_tag_start = NULL;
                break;
            }


            // if we run into a CDATA, skip it, because we are looking
            // for the next tag, and we don't want to find any angle brackets
            // inside the CDATA section
            if (0 == wcsncmp(p, L"<![CDATA[", 9))
            {
                p = wcsstr(p, L"]]>");
                if (!p)
                {
                    next_tag_start = NULL;
                    break;
                }
                continue;
            }


            if (*(p+1) != '!')
            {
                next_tag_start = p;
                break;
            }
            p++;
        }
        
        if (next_tag_start == NULL)
            return false;

        p++;
        // is it our close tag?
        while (iswspace(*p))
            p++;

        if (*p == L'/')
        {
            if (wcsncmp(m_tag_name.c_str(), p+1, tag_name_len) == 0)
            {
                // it is our closing tag, so, if we have no children,
                // grab our CDATA content and go

                if (m_nodes.size() == 0)
                {   
                    int content_length = (next_tag_start-tag_content_start);

                    if (content_length > 0)
                    {
                        detokenize(m_contents, tag_content_start, content_length);
                    }
                }

                p = wcschr(p, L'>');
                if (p)
                {
                    *next_start = p+1;
                }
                return true;
            }
             else
            {
                // it is not our closing tag.  This is a malformed xml document
                if (!cdata_only)
                    return false;
            }
        }
         else
        {
            if (cdata_only)
            {
                // we are using parseRelaxed mode, and a parsing error
                // had occurred previously -- therefore we want to treat
                // all content as CDATA, so skip until the tag close
                p++;
                continue;
            }
            
            xmlnode& new_child = addChild();
            if (!new_child.internalParse(next_tag_start, &p, parse_flags))
            {
                // a parsing error occurred - if we are using the relaxed parsing mode,
                // clear out the node children and treat the content as CDATA
                if (parse_flags & parseRelaxed)
                {
                    deleteAllChildren();
                    cdata_only = true;
                }
                 else
                {
                    // strict mode, fail out
                    m_nodes.pop_back();
                    return false;
                }
            }
        }
    }

}

void xmlnode::internalPrint(std::wstring& output,
                            int format_flags,
                            int indent,
                            const wchar_t* cr,
                            wchar_t indent_char) const
{
    int child_count = m_nodes.size();

    int cur_len = output.length();
    int cur_cap = output.capacity();

    // reserve output space
    if (cur_len == 0 && ((format_flags & formattingOmitPrologue) == 0))
    {
        output += KL_XML_HEADER;
        output += cr;
    }


    if (cur_len + 10000 > cur_cap)
    {
        output.reserve(cur_cap + 10000);
    }


    if (indent_char)
    {
        output.append(indent, indent_char);
    }

    output += L"<";
    output += m_tag_name;

    std::vector<xmlproperty*>::const_iterator prop_it;
    for (prop_it = m_properties.begin();
         prop_it != m_properties.end(); prop_it++)
    {
        output += L" ";
        output += (*prop_it)->name;
        output += L"=\"";
        tokenizeAppend(output, (*prop_it)->value);
        output += L"\"";
    }

    if (m_contents.empty() && child_count == 0)
    {
        output += L"/>";
        output += cr;
    }
     else
    {
        output += L">";
        
        if (child_count == 0)
        {
            if (!m_contents.empty())
            {
                tokenizeAppend(output, m_contents);
            }
        }
         else
        {
            output += cr;
            
            std::vector<xmlnode*>::const_iterator it;
            for (it = m_nodes.begin(); it != m_nodes.end(); ++it)
            {
                (*it)->internalPrint(output,
                                     format_flags,
                                     indent+1,
                                     cr,
                                     indent_char);
            }

            if (indent_char)
            {
                output.append(indent, indent_char);
            }
        }

        output += L"</";
        output += m_tag_name;
        output += L">";
        output += cr;
    }
}


int xmlnode::getLengthEstimate(int indent) const
{
    int len = 5 + (m_tag_name.length()*2);

    std::vector<xmlproperty*>::const_iterator prop_it;
    for (prop_it = m_properties.begin();
         prop_it != m_properties.end(); prop_it++)
    {
        len += 5 + (*prop_it)->name.length() + (*prop_it)->value.length();
    }

    std::vector<xmlnode*>::const_iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        len += (*it)->getLengthEstimate(indent+1);
        len += indent*2;
    }

    len += (m_contents.length()+1);

    return len;
}

void xmlnode::internalGetXML(std::wstring& result,
                             int format_flags) const
{
    wchar_t cr[3];

    // create CR buffer

    cr[0] = 0;

    if (format_flags & formattingCrLf)
    {
        wcscpy(cr, L"\r\n");
    }
     else if (format_flags & formattingLf)
    {
        wcscpy(cr, L"\n");
    }

    // create indentation buffer
    wchar_t indent_char = 0;

    if (format_flags & formattingTabs)
    {
        indent_char = L'\t';
    }
     else if (format_flags & formattingSpaces)
    {
        indent_char = L' ';
    }

    int len_estimate = getLengthEstimate(0);
    result.reserve(len_estimate);

    // get the xml string
    internalPrint(result, format_flags, 0, cr, indent_char);
}

void xmlnode::internalClone(xmlnode* original, xmlnode** copy, bool deep)
{
    // create a new xml node
    *copy = new xmlnode;

    // copy the tag name and contents
    (*copy)->m_tag_name = original->m_tag_name;
    (*copy)->m_contents = original->m_contents;
    
    // copy the properties
    std::vector<xmlproperty*>::iterator it_prop, it_prop_end;
    it_prop_end = original->m_properties.end();
    
    for (it_prop = original->m_properties.begin(); it_prop != it_prop_end; ++it_prop)
    {
        xmlproperty* p = new xmlproperty;
        p->name = (*it_prop)->name;
        p->value = (*it_prop)->value;
        (*copy)->m_properties.push_back(p);
    }
    
    // if the deep flag is set, copy the children
    if (deep)
    {
        std::vector<xmlnode*>::iterator it_xml, it_xml_end;
        it_xml_end = original->m_nodes.end();
        
        for (it_xml = original->m_nodes.begin(); it_xml != it_xml_end; ++it_xml)
        {
            xmlnode* c = NULL;
            internalClone(*it_xml, &c, deep);
            (*copy)->m_nodes.push_back(c);
        }
    }
}


};

