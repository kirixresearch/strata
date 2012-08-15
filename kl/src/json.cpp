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

static std::wstring escape_string(std::wstring& str)
{
    std::wstring result = L"";

    std::wstring::iterator it, it_end;
    it_end = str.end();

    for (it = str.begin(); it != it_end; ++it)
    {
        wchar_t ch = *it;

        switch (ch)
        {
            default:
                {
                    // TODO: check range of values allowed without
                    // unicode escaping
                    if (ch >= 32 && ch <= 126)
                        result += ch;
                    else
                    {
                        wchar_t buf[25];
                        swprintf(buf, 25, L"\\u%04x", (int)ch);
                        result.append(buf);
                    }
                }
                break;

            case L'"':
                result.append(L"\\\"");
                break;

            case L'\\':
                result.append(L"\\\\");
                break;

            case L'\b':
                result.append(L"\\b");
                break;

            case L'\t':
                result.append(L"\\t");
                break;

            case L'\n':
                result.append(L"\\n");
                break;

            case L'\f':
                result.append(L"\\f");
                break;

            case L'\r':
                result.append(L"\\r");
                break;
        };
    }

    return result;
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
        child_node.copyFrom(value_node);

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
        child_node.copyFrom(value_node);

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
            // escaped string; find out what's next
            expr++;
            if (*expr == '"')
                value += L"\"";
            else if (*expr == '\\')
                value += L"\\";
            else if (*expr == 'b')
                value += L"\b";
            else if (*expr == 't')
                value += L"\t";
            else if (*expr == 'n')
                value += L"\n";
            else if (*expr == 'f')
                value += L"\f";
            else if (*expr == 'r')
                value += L"\r";
            else if (*expr == 'u')
            {
                // unicode escape sequence; special handling
                int hex_converted = 0;
                for (int i = 0; i < 4; ++i)
                {
                    *expr++;
                    int hex_value = 0;
                    if (*expr == '0')
                        hex_value = 0;
                    else if (*expr == '1')
                        hex_value = 1;
                    else if (*expr == '2')
                        hex_value = 2;
                    else if (*expr == '3')
                        hex_value = 3;
                    else if (*expr == '4')
                        hex_value = 4;
                    else if (*expr == '5')
                        hex_value = 5;
                    else if (*expr == '6')
                        hex_value = 6;
                    else if (*expr == '7')
                        hex_value = 7;
                    else if (*expr == '8')
                        hex_value = 8;
                    else if (*expr == '9')
                        hex_value = 9;
                    else if (*expr == 'a' || *expr == 'A')
                        hex_value = 10;
                    else if (*expr == 'b' || *expr == 'B')
                        hex_value = 11;
                    else if (*expr == 'c' || *expr == 'C')
                        hex_value = 12;
                    else if (*expr == 'd' || *expr == 'D')
                        hex_value = 13;
                    else if (*expr == 'e' || *expr == 'E')
                        hex_value = 14;
                    else if (*expr == 'f' || *expr == 'F')
                        hex_value = 15;
                    else
                    {
                        // invalid unicode escape sequence
                        node.init();
                        *endloc = expr;
                        return false;
                    }
                    
                    hex_converted = hex_converted*16 + hex_value;
                }
                
                value += (wchar_t)hex_converted;
            }
            else
            {
                // unknown escape sequence
                node.init();
                *endloc = expr;
                return false;
            }
            
            expr++;
            continue;
        }

        if (!*expr)
            break;
    }

    // unterminated JSON or unkown escape sequence
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

    *endloc = expr;

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
    m_value = new JsonValue;
    m_value->ref();
    init();
}

JsonNode::~JsonNode()
{
    m_value->unref();
}

JsonNode::JsonNode(const JsonNode& _c)
{
    m_value = _c.m_value;
    m_value->ref();
}

JsonNode& JsonNode::operator=(const JsonNode& _c)
{
    if (this == &_c)
        return *this;

    m_value = _c.m_value;
    m_value->ref();

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
    it_end = m_value->m_child_nodes.end();

    for (it = m_value->m_child_nodes.begin(); it != it_end; ++it)
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
    m_value->m_type = nodetypeObject;

    // get the child
    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_value->m_child_nodes.end();

    for (it = m_value->m_child_nodes.begin(); it != it_end; ++it)
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
    m_value->m_child_nodes.push_back(named_node);

    return named_node.second;
}

std::vector<std::wstring> JsonNode::getChildKeys()
{
    std::vector<std::wstring> result;
    result.reserve(m_value->m_child_nodes.size());

    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_value->m_child_nodes.end();

    for (it = m_value->m_child_nodes.begin(); it != it_end; ++it)
    {
        result.push_back(it->first);
    }

    return result;
}

std::vector<JsonNode> JsonNode::getChildren()
{
    std::vector<JsonNode> result;
    result.reserve(m_value->m_child_nodes.size());

    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_value->m_child_nodes.end();

    for (it = m_value->m_child_nodes.begin(); it != it_end; ++it)
    {
        result.push_back(it->second);
    }

    return result;
}

size_t JsonNode::getChildCount()
{
    return m_value->m_child_nodes.size();
}

JsonNode JsonNode::appendElement()
{
    // if the node type was anything besides an array, clear it out
    if (m_value->m_type != nodetypeArray)
        init();

    // make sure the type is an array (getChild currently
    // converts node to object type)
    JsonNode child = (*this)[getChildCount()];
    m_value->m_type = nodetypeArray;
    
    return child;    
}

void JsonNode::setObject()
{
    m_value->m_child_nodes.clear();
    m_value->m_string.clear();
    m_value->m_double = 0.0f;
    m_value->m_integer = 0;
    m_value->m_boolean = false;
    m_value->m_isnull = false;
    m_value->m_type = nodetypeObject;
}

void JsonNode::setArray()
{
    m_value->m_child_nodes.clear();
    m_value->m_string.clear();
    m_value->m_double = 0.0f;
    m_value->m_integer = 0;
    m_value->m_boolean = false;
    m_value->m_isnull = false;
    m_value->m_type = nodetypeArray;
}

void JsonNode::setString(const std::wstring& str)
{
    m_value->m_child_nodes.clear();
    m_value->m_string = str;
    m_value->m_double = 0.0f;
    m_value->m_integer = 0;
    m_value->m_boolean = false;
    m_value->m_isnull = false;
    m_value->m_type = nodetypeString;
}

void JsonNode::setBoolean(bool b)
{
    m_value->m_child_nodes.clear();
    m_value->m_string.clear();
    m_value->m_double = 0.0f;
    m_value->m_integer = 0;
    m_value->m_boolean = b;
    m_value->m_isnull = false;
    m_value->m_type = nodetypeBoolean;
}

void JsonNode::setDouble(double num)
{
    m_value->m_child_nodes.clear();
    m_value->m_string.clear();
    m_value->m_double = num;
    m_value->m_integer = 0;
    m_value->m_boolean = false;
    m_value->m_isnull = false;
    m_value->m_type = nodetypeDouble;
}

void JsonNode::setInteger(int num)
{
    m_value->m_child_nodes.clear();
    m_value->m_string.clear();
    m_value->m_double = 0.0f;
    m_value->m_integer = num;
    m_value->m_boolean = false;
    m_value->m_isnull = false;
    m_value->m_type = nodetypeInteger;
}

void JsonNode::setNull()
{
    m_value->m_child_nodes.clear();
    m_value->m_string.clear();
    m_value->m_double = 0.0f;
    m_value->m_integer = 0;
    m_value->m_boolean = false;
    m_value->m_isnull = true;
    m_value->m_type = nodetypeNull;    
}

std::wstring JsonNode::getString()
{
    return m_value->m_string;
}

bool JsonNode::getBoolean()
{
    return m_value->m_boolean;
}

double JsonNode::getDouble()
{
    return m_value->m_double;
}

int JsonNode::getInteger()
{
    return m_value->m_integer;
}

bool JsonNode::isNull()
{
    return m_value->m_isnull;
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

void JsonNode::copyFrom(const JsonNode& node)
{
    m_value->m_child_nodes = node.m_value->m_child_nodes;
    m_value->m_string = node.m_value->m_string;
    m_value->m_double = node.m_value->m_double;
    m_value->m_integer = node.m_value->m_integer;
    m_value->m_boolean = node.m_value->m_boolean;
    m_value->m_isnull = node.m_value->m_isnull;
    m_value->m_type = node.m_value->m_type;
}

void JsonNode::init()
{
    m_value->m_child_nodes.clear();
    m_value->m_string.clear();
    m_value->m_double = 0.0f;
    m_value->m_integer = 0;
    m_value->m_boolean = false;
    m_value->m_isnull = true;
    m_value->m_type = nodetypeNull;
}

bool JsonNode::parse(wchar_t* expr)
{
    wchar_t* endloc = NULL;
    bool success = parseJsonValue(expr, &endloc, *this);
    
    // make sure there's nothing left over
    skipWhiteSpaceOrLS(endloc);
    if (*endloc != NULL)
        success = false;

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
    if (m_value->m_type == nodetypeNull)
        return L"null";

    if (m_value->m_type == nodetypeBoolean)
        return getBoolean() ? L"true" : L"false";

    if (m_value->m_type == nodetypeString)
        return L'"' + escape_string(getString()) + L'"';

    if (m_value->m_type == nodetypeInteger)
    {
        char buf[20];
        sprintf(buf, "%d", getInteger());
        return kl::towstring(buf);
    }
        
    if (m_value->m_type == nodetypeDouble)
    {
        // TODO: determine how to return the number with the same precision as entered    
        char buf[20];
        sprintf(buf, "%f", getDouble());
        return kl::towstring(buf);
    }

    if (m_value->m_type == nodetypeArray)
    {
        std::wstring result;
        result += L"[";
        
        std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
        it_end = m_value->m_child_nodes.end();
        
        bool first = true;
        for (it = m_value->m_child_nodes.begin(); it != it_end; ++it)
        {
            if (!first)
                result += L",";
            first = false;
        
            result += it->second.stringify();
        }
        
        result += L"]";
        return result;
    }
    
    if (m_value->m_type == nodetypeObject)
    {
        std::wstring result;
        result += L"{";

        std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
        it_end = m_value->m_child_nodes.end();

        bool first = true;
        for (it = m_value->m_child_nodes.begin(); it != it_end; ++it)
        {
            if (!first)
                result += L",";
            first = false;

            // stringify the key
            result += (L'"' + escape_string(it->first) + L'"');
            
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

