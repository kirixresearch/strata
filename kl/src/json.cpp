/*!
 *
 * Copyright (c) 2008-2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Aaron L. Williams
 * Created:  2012-08-09
 *
 */


#include <math.h>
#include "kl/json.h"
#include "kl/string.h"


namespace kl
{


inline bool isWhiteSpaceOrLS(wchar_t ch)
{
    if (ch < 0x80)
        return (ch == ' ' || ch == 0x0d || ch == 0x0a || ch == 0x09 || ch == 0x0b || ch == 0x0c);
         else
        return (ch == 0xa0 || (ch >= 0x2000 && ch <= 0x200b) || ch == 0x202f || ch == 0x205f || ch == 0x3000 || iswspace(ch));
}

inline void skipWhiteSpaceOrLS(wchar_t*& ch)
{
    if (!ch) return;
    while (isWhiteSpaceOrLS(*ch))
        ++ch;
}

bool parseJsonValue(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonObject(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonArray(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonString(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonNumber(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonWord(wchar_t* expr, wchar_t** endloc, JsonNode& node);

bool parseJsonValue(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    // endloc is required
    if (!endloc)
        return NULL;
        
    skipWhiteSpaceOrLS(expr);

    if (*expr == '{')
        return parseJsonObject(expr, endloc, node);
    else if (*expr == '[')
        return parseJsonArray(expr, endloc, node);
    else if (*expr == '\"')
        return parseJsonString(expr, endloc, node);
    else if (*expr == '-' || (*expr >= '0' && *expr <= '9'))
        return parseJsonNumber(expr, endloc, node);
    else   
        return parseJsonWord(expr, endloc, node);
}

bool parseJsonObject(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    if (*expr != '{')
        return false;

    expr++;
    skipWhiteSpaceOrLS(expr);
    node.setObject();

    if (*expr == '}')
    {
        // empty object
        *endloc = expr+1;
        return true;
    }

    while (1)
    {
        bool result = false;
        std::wstring key;
    
        // get the key string
        JsonNode key_node;
        result = parseJsonString(expr, endloc, key_node);
        if (!result)
            return false;

        key = key_node.getString();

        // start where the key string ended
        expr = *endloc;
        skipWhiteSpaceOrLS(expr);

        if (*expr != ':')
            return false;

        // skip past the colon
        expr++;
        skipWhiteSpaceOrLS(expr);
        
        // parse the value portion
        JsonNode value_node;
        result = parseJsonValue(expr, endloc, value_node);
        if (!result)
            return false;

        // set the member; TODO: following is inefficient since getChild()
        // checks for the existence of a member element before setting it
        JsonNode child_node = node.getChild(key);
        child_node = value_node;

        expr = *endloc;
        skipWhiteSpaceOrLS(expr);

        // if we have a comma, continue on
        if (*expr == ',')
        {
            expr++;
            skipWhiteSpaceOrLS(expr);
            continue;
        }

        // if we have a closing brace, we're done
        if (*expr == '}')
        {
            *endloc = expr+1;
            return true;
        }
        
        // unterminated JSON
        node.init();
        *endloc = expr;
        return false;
    }
}

bool parseJsonArray(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    if (*expr != '[')
        return false;

    expr++;
    skipWhiteSpaceOrLS(expr);
    node.setArray();

    if (*expr == ']')
    {
        // empty array
        *endloc = expr+1;
        return true;
    }

    std::wstring key;

    while (1)
    {
        bool result = false;
        std::wstring key;    

        // parse the value
        JsonNode value_node;
        result = parseJsonValue(expr, endloc, value_node);
        if (!result)
            return false;

        // set the member
        JsonNode child_node = node.appendElement();
        child_node = value_node;

        expr = *endloc;
        skipWhiteSpaceOrLS(expr);

        // if we have a comma, continue on
        if (*expr == ',')
        {
            expr++;
            skipWhiteSpaceOrLS(expr);
            continue;
        }

        // if we have a closing bracket, we're done
        if (*expr == ']')
        {
            *endloc = expr+1;
            return true;
        }
        
        // unterminated JSON
        node.init();
        *endloc = expr;
        return false;
    }
}

bool parseJsonString(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    if (*expr != '"')
        return false;

    expr++;

    // note: don't skip white space here since character is 
    // in a quote

    std::wstring value;
    while (1)
    {
        // if we have a closing quote, we're done
        if (*expr == '"')
        {
            node.setString(value);
            *endloc = expr+1;
            return true;
        }

        if (*expr != '\\')
        {
            // non-escaped string; simply add on the character
            value += *expr;
            expr++;
            continue;
        }
         else
        {
            // TODO: escaped string; special handling
            value += *expr;
            expr++;
            continue;
        }

        if (!*expr)
            break;
    }

    // unterminated JSON
    node.init();
    *endloc = expr;
    return false;
}

bool parseJsonNumber(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    std::wstring value;
    
    if (*expr == '-')
    {
        value += *expr;
        expr++;
    }

    bool period = false;
    while (1)
    {
        // TODO: implement better numeric parsing
        if ((*expr <= '0' || *expr >= '9') && *expr != '.')
            break;

        if (*expr == '.')
            period = true;
            
        value += *expr;
        expr++;
    }

    if (!isWhiteSpaceOrLS(*expr))
        return false;


    double dbl_val = nolocale_wtof(value.c_str());
    if (!period)
    {
        // -- see if it will work as an integer --
        if (fabs(dbl_val) < 2147483647.9)
        {
            node.setInteger(wtoi(value.c_str()));
            return true;
        }
    }

    node.setDouble(dbl_val);
    return true;
}

bool parseJsonWord(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    if (!wcsncmp(expr, L"true", 4))
    {
        node.setBoolean(true);
        *endloc = (expr + 4);
        return true;
    }
     else if (!wcsncmp(expr, L"false", 5))
    {
        node.setBoolean(false);
        *endloc = (expr + 5);
        return true;
    }
     else if (!wcsncmp(expr, L"null", 4))
    {
        node.setNull();
        *endloc = (expr + 4);
        return true;
    }

    return false;
}

JsonNode::JsonNode()
{
    init();
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

JsonNode JsonNode::appendElement()
{
    // if the node type was anything besides an array, clear it out
    if (m_type != nodetypeArray)
        init();

    // make sure the type is an array (getChild currently
    // converts node to object type)
    JsonNode child = (*this)[getChildCount()];
    m_type = nodetypeArray;
    
    return child;    
}

void JsonNode::setObject()
{
    m_child_nodes.clear();
    m_string.clear();
    m_double = 0.0f;
    m_integer = 0;
    m_boolean = false;
    m_isnull = false;
    m_type = nodetypeObject;
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
    return parse((wchar_t*)str.c_str());
}

void JsonNode::init()
{
    m_child_nodes.clear();
    m_string.clear();
    m_double = 0.0f;
    m_integer = 0;
    m_boolean = false;
    m_isnull = true;
    m_type = nodetypeNull;
}

bool JsonNode::parse(wchar_t* expr)
{
    wchar_t* endloc = NULL;
    bool success = parseJsonValue(expr, &endloc, *this);

    if (!success)
    {
        // reset the json
        init();
        return false;
    }
    
    return true;
}

std::wstring JsonNode::stringify()
{
    if (m_type == nodetypeNull)
        return L"null";

    if (m_type == nodetypeBoolean)
        return getBoolean() ? L"true" : L"false";

    if (m_type == nodetypeString)
        return L'"' + getString() + L'"';   // TODO: escape string

    if (m_type == nodetypeInteger)  // TODO: implement
        return L"0";
        
    if (m_type == nodetypeDouble)   // TODO: implement
        return L"0";

    if (m_type == nodetypeArray)
    {
        std::wstring result;
        result += L"[";
        
        std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
        it_end = m_child_nodes.end();
        
        bool first = true;
        for (it = m_child_nodes.begin(); it != it_end; ++it)
        {
            if (!first)
                result += L",";
            first = false;
        
            result += it->second.stringify();
        }
        
        result += L"]";
        return result;
    }
    
    if (m_type == nodetypeObject)
    {
        std::wstring result;
        result += L"{";

        std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
        it_end = m_child_nodes.end();

        bool first = true;
        for (it = m_child_nodes.begin(); it != it_end; ++it)
        {
            if (!first)
                result += L",";
            first = false;

            // stringify the key
            result += (L'"' + it->first + L'"');    // TODO: escape the key
            
            // separator
            result += L":";
            
            // stringify the value
            result += it->second.stringify();
        }

        result += L"}";
        return result;
    }
    
    return L"";
}


};  // namespace kl

