/*!
 *
 * Copyright (c) 2008-2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Aaron L. Williams
 * Created:  2012-08-09
 *
 */


#include "kl/json.h"
#include "kl/string.h"


namespace kl
{


JsonNode::JsonNode()
{
    m_double = 0.0f;
    m_integer = 0;
    m_boolean = false;
    m_isnull = true;
    m_type = nodetypeNull;
}

JsonNode::~JsonNode()
{
}

JsonNode::JsonNode(const JsonNode& _c)
{
    m_child_nodes = _c.m_child_nodes;
    m_string = _c.m_string;
    m_double = _c.m_double;
    m_integer = _c.m_integer;
    m_boolean = _c.m_boolean;
    m_isnull = _c.m_isnull;
    m_type = _c.m_type;
}

JsonNode& JsonNode::operator=(const JsonNode& _c)
{
    if (this == &_c)
        return *this;

    m_child_nodes = _c.m_child_nodes;
    m_string = _c.m_string;
    m_double = _c.m_double;
    m_integer = _c.m_integer;
    m_boolean = _c.m_boolean;
    m_isnull = _c.m_isnull;
    m_type = _c.m_type;

    return *this;
}

JsonNode& JsonNode::operator=(int i)
{
    setInteger(i);
    return *this;
}

JsonNode& JsonNode::operator=(double d)
{
    setDouble(d);
    return *this;
}

JsonNode& JsonNode::operator=(const std::wstring& str)
{
    setString(str);
    return *this;
}

JsonNode JsonNode::operator[](int i)
{
    char buf[20];
    sprintf(buf, "%d", i);
    return getChild(kl::towstring(buf));
}

JsonNode JsonNode::operator[](const char* str)
{
    return (*this)[kl::towstring(str)];
}

JsonNode JsonNode::operator[](const std::wstring& str)
{
    return getChild(str);
}

bool JsonNode::childExists(const std::wstring& _str)
{
    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_child_nodes.end();

    for (it = m_child_nodes.begin(); it != it_end; ++it)
    {
        if (it->first != _str)
            continue;

        return true;
    }

    return false;
}

JsonNode JsonNode::getChild(const std::wstring& _str)
{
    // reset the node type
    m_type = nodetypeObject;

    // get the child
    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_child_nodes.end();

    for (it = m_child_nodes.begin(); it != it_end; ++it)
    {
        if (it->first != _str)
            continue;
            
        return it->second;
    }

    // if the node doesn't exist, add it
    JsonNode node;
    std::pair<std::wstring,JsonNode> named_node;
    named_node.first = _str;
    named_node.second = node;    
    m_child_nodes.push_back(named_node);

    return named_node.second;
}

std::vector<std::wstring> JsonNode::getChildKeys()
{
    std::vector<std::wstring> result;
    result.reserve(m_child_nodes.size());

    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_child_nodes.end();

    for (it = m_child_nodes.begin(); it != it_end; ++it)
    {
        result.push_back(it->first);
    }

    return result;
}

std::vector<JsonNode> JsonNode::getChildren()
{
    std::vector<JsonNode> result;
    result.reserve(m_child_nodes.size());

    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_child_nodes.end();

    for (it = m_child_nodes.begin(); it != it_end; ++it)
    {
        result.push_back(it->second);
    }

    return result;
}

size_t JsonNode::getChildCount()
{
    return m_child_nodes.size();
}

void JsonNode::setArray()
{
    m_child_nodes.clear();
    m_string.clear();
    m_double = 0.0f;
    m_integer = 0;
    m_boolean = false;
    m_isnull = false;
    m_type = nodetypeArray;
}

JsonNode JsonNode::appendElement()
{
    // if the node type was anything besides an array, clear it out
    if (m_type != nodetypeArray)
    {
        m_child_nodes.clear();
        m_string.clear();
        m_double = 0.0f;
        m_integer = 0;
        m_boolean = false;
        m_isnull = false;
    }

    // reset the node type
    m_type = nodetypeArray;
    return (*this)[getChildCount()];
}

void JsonNode::setString(const std::wstring& str)
{
    m_child_nodes.clear();
    m_string = str;
    m_double = 0.0f;
    m_integer = 0;
    m_boolean = false;
    m_isnull = false;
    m_type = nodetypeString;
}

void JsonNode::setBoolean(bool b)
{
    m_child_nodes.clear();
    m_string.clear();
    m_double = 0.0f;
    m_integer = 0;
    m_boolean = b;
    m_isnull = false;
    m_type = nodetypeBoolean;
}

void JsonNode::setDouble(double num)
{
    m_child_nodes.clear();
    m_string.clear();
    m_double = num;
    m_integer = 0;
    m_boolean = false;
    m_isnull = false;
    m_type = nodetypeDouble;
}

void JsonNode::setInteger(int num)
{
    m_child_nodes.clear();
    m_string.clear();
    m_double = 0.0f;
    m_integer = num;
    m_boolean = false;
    m_isnull = false;
    m_type = nodetypeInteger;
}

void JsonNode::setNull()
{
    m_child_nodes.clear();
    m_string.clear();
    m_double = 0.0f;
    m_integer = 0;
    m_boolean = false;
    m_isnull = true;
    m_type = nodetypeNull;    
}

std::wstring JsonNode::getString()
{
    return m_string;
}

bool JsonNode::getBoolean()
{
    return m_boolean;
}

double JsonNode::getDouble()
{
    return m_double;
}

int JsonNode::getInteger()
{
    return m_integer;
}

bool JsonNode::isNull()
{
    return m_isnull;
}

bool JsonNode::isOk()
{
    return !isNull();
}

JsonNode::operator std::wstring()
{
    return getString();
}

std::wstring JsonNode::toString()
{
    return stringify();
}

bool JsonNode::fromString(const std::wstring& str)
{
    // TODO: fill out
    return false;
}

bool JsonNode::parse(wchar_t* expr, wchar_t** endloc)
{
    // TODO: fill out
    return false;
}

std::wstring JsonNode::stringify()
{
    // TODO: fill out
    std::wstring result;
    return result;
}


};  // namespace kl

