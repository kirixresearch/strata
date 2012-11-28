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
#include "kl/math.h"


namespace kl
{

// numeric test cases
/*

Not Allowed:

[+1,01,-01,1.,-.1,1e,1e+,1e-,e1,e-1,e+1,-e1,.1e1,1.e1,e,e.1,1.1.1,-1-1,1.23e,1.23e-,1.23e+,1e01]


Allowed:

int:
[0,9,123,-0,-9,-123]

int frac:
[0.0,0.9,9.0,9.9,123.0,123.9,-0.0,-0.9,-9.0,-9.9,-123.0,-123.9,1.01,1.23,-1.01,-1.23]

int exp:
[0e0,9e1,123e23,-0e456,-9e789,-123e1,0e+0,9e+1,123e+23,-0e+456,-9e+789,-123e+1,0e-0,9e -1,123e-23,-0e-456,-9e-789,-123e-1,2E2,2E+3,2E-4]

int frac exp:
[0.0e0,1.0e1,0.1e1,-0.0e-0,-0.0e+0,-0.0E+1,1.23e5]

*/


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

std::wstring escape_string(std::wstring& str)
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

bool parseFail(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonValue(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonObject(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonArray(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonString(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonNumber(wchar_t* expr, wchar_t** endloc, JsonNode& node);
bool parseJsonWord(wchar_t* expr, wchar_t** endloc, JsonNode& node);

bool parseFail(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    node.init();
    *endloc = expr;
    return false;
}

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
        return parseFail(expr, endloc, node);

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
            return parseFail(expr, endloc, node);

        key = key_node.getString();

        // start where the key string ended
        expr = *endloc;
        skipWhiteSpaceOrLS(expr);

        if (*expr != ':')
            return parseFail(expr, endloc, node);

        // skip past the colon
        expr++;
        skipWhiteSpaceOrLS(expr);
        
        // parse the value portion
        JsonNode value_node;
        result = parseJsonValue(expr, endloc, value_node);
        if (!result)
            return parseFail(expr, endloc, node);

        // set the member
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
        return parseFail(expr, endloc, node);
    }
}

bool parseJsonArray(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    if (*expr != '[')
        return parseFail(expr, endloc, node);

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
            return parseFail(expr, endloc, node);

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
        return parseFail(expr, endloc, node);
    }
}

bool parseJsonString(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    if (*expr != '"')
        return parseFail(expr, endloc, node);

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
            else if (*expr == '/')  // "\/" is allowed even though "/" is valid
                value += L"/";                
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
                        return parseFail(expr, endloc, node);
                    }
                    
                    hex_converted = hex_converted*16 + hex_value;
                }
                
                value += (wchar_t)hex_converted;
            }
            else
            {
                // unknown escape sequence
                return parseFail(expr, endloc, node);
            }
            
            expr++;
            continue;
        }

        if (!*expr)
            break;
    }

    // unterminated JSON or unkown escape sequence
    return parseFail(expr, endloc, node);
}

bool parseJsonNumber(wchar_t* expr, wchar_t** endloc, JsonNode& node)
{
    std::wstring value;

    bool decimal_part = false;
    bool exponent_part = false;

    // check for a starting negative sign
    if (*expr == '-')
    {
        value += *expr;
        expr++;
        
        // if we have a negative sign, the next character has to
        // be a digit
        if (*expr < '0' || *expr > '9')
            return parseFail(expr, endloc, node);
    }

    // check for numeric digits
    while (1)
    {
        // digits
        if (*expr >= '0' && *expr <= '9')
        {
            value += *expr;
            expr++;
            continue;
        }
        
        // period
        if (*expr == '.')
        {
            // if we've encountered more than one period, 
            // the number's invalid
            if (decimal_part)
                return parseFail(expr, endloc, node);

            decimal_part = true;        
            value += *expr;
            expr++;
            
            // the period must be followed by at least one digit
            if (*expr < '0' || *expr > '9')
                return parseFail(expr, endloc, node);

            continue;
        }

        // exponent
        if (*expr == 'e' || *expr == 'E')
        {
            // if we've encountered more than one exponent,
            // the number's invalid
            if (exponent_part)
                return parseFail(expr, endloc, node);

            exponent_part = true;
            value += *expr;
            expr++;
            
            // the exponent part may be followed by a plus or minus
            if (*expr == '-' || *expr == '+')
            {
                value += *expr;
                expr++;
            }
            
            // the exponent must be followed by at least one digit
            if (*expr < '0' || *expr > '9')
                return parseFail(expr, endloc, node);

            continue;
        }

        break;
    }
    

    *endloc = expr;

    // if we don't have a decimal part or an exponent part,
    // see if the number will work as an integer
    double dbl_val = nolocale_wtof(value.c_str());    
    if (!decimal_part && !exponent_part)
    {
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

    return parseFail(expr, endloc, node);
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

JsonNode& JsonNode::operator=(bool b)
{
    setBoolean(b);
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

JsonNode& JsonNode::operator=(const std::string& str)
{
    setString(kl::towstring(str));
    return *this;
}

JsonNode& JsonNode::operator=(const std::wstring& str)
{
    setString(str);
    return *this;
}

JsonNode JsonNode::operator[](int i)
{
    wchar_t buf[30];
    swprintf(buf, 30, L"%d", i);
    return getChild(buf);
}

JsonNode JsonNode::operator[](const char* str)
{
    return (*this)[kl::towstring(str)];
}

JsonNode JsonNode::operator[](const std::wstring& str)
{
    return getChild(str);
}

bool JsonNode::childExists(const std::string& str)
{
    return childExists(kl::towstring(str));
}

bool JsonNode::childExists(const std::wstring& str)
{
    std::map<std::wstring,JsonNode>::iterator it = m_value->m_child_nodes.find(str);
    if (it != m_value->m_child_nodes.end())
        return true;

    return false;
}

JsonNode JsonNode::getChild(const std::wstring& _str)
{
    // reset the node type
    m_value->m_type = nodetypeObject;

    // try to find the child in the mapped container
    std::map<std::wstring,JsonNode>::iterator it = m_value->m_child_nodes.find(_str);
    if (it != m_value->m_child_nodes.end())
        return it->second;

    // if we can't find the child, create a new one and store it in both
    // containers for ready access
    JsonNode new_child;
    std::pair<std::wstring,JsonNode> m_child_keyvalue(_str, new_child);

    m_value->m_child_nodes[_str] = new_child;
    m_value->m_child_nodes_ordered.push_back(m_child_keyvalue);

    return new_child;
}

JsonNode JsonNode::getChild(const std::string& str)
{
    return getChild(kl::towstring(str));
}


std::vector<std::wstring> JsonNode::getChildKeys()
{
    // return an ordered list of keys
    std::vector<std::wstring> child_keys;
    child_keys.reserve(m_value->m_child_nodes_ordered.size());

    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_value->m_child_nodes_ordered.end();
    
    for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
    {
        child_keys.push_back(it->first);
    }

    return child_keys;
}

std::vector<JsonNode> JsonNode::getChildren()
{
    // return an ordered list of children
    std::vector<JsonNode> child_nodes;
    child_nodes.reserve(m_value->m_child_nodes_ordered.size());

    std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
    it_end = m_value->m_child_nodes_ordered.end();
    
    for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
    {
        child_nodes.push_back(it->second);
    }

    return child_nodes;
}

size_t JsonNode::getChildCount()
{
    return m_value->m_child_nodes_ordered.size();
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
    m_value->m_child_nodes_ordered.clear();
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
    m_value->m_child_nodes_ordered.clear();    
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
    m_value->m_child_nodes_ordered.clear();    
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
    m_value->m_child_nodes_ordered.clear();    
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
    m_value->m_child_nodes_ordered.clear();
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
    m_value->m_child_nodes_ordered.clear();    
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
    m_value->m_child_nodes_ordered.clear();    
    m_value->m_string.clear();
    m_value->m_double = 0.0f;
    m_value->m_integer = 0;
    m_value->m_boolean = false;
    m_value->m_isnull = true;
    m_value->m_type = nodetypeNull;    
}

std::wstring JsonNode::getString()
{
    // TODO: add implicit type conversions

    return m_value->m_string;
}

bool JsonNode::getBoolean()
{
    switch (m_value->m_type)
    {
        case nodetypeBoolean:
            return m_value->m_boolean;
        case nodetypeInteger:
            return (m_value->m_integer != 0 ? true : false);
        case nodetypeDouble:
            return (m_value->m_double != 0 ? true : false);
        case nodetypeString:
            return (m_value->m_string.size() > 0 ? true : false); // ECMAScript behavior
        case nodetypeNull:
            return false;
        case nodetypeObject:
        case nodetypeArray:
            return true;
    }

    return false;
}

double JsonNode::getDouble()
{
    // TODO: add implicit type conversions

    if (m_value->m_type == nodetypeInteger)
        return m_value->m_integer;
         else
        return m_value->m_double;
}

int JsonNode::getInteger()
{
    // TODO: add implicit type conversions

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
    m_value->m_child_nodes_ordered = node.m_value->m_child_nodes_ordered;    
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
    m_value->m_child_nodes_ordered.clear();
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

static std::wstring addspaces(unsigned int indent_level)
{
    std::wstring spaces = L"";
    int spaces_per_indent_level = 4;

    for (int i = 0; i < indent_level*spaces_per_indent_level; ++i)
        spaces += L" ";

    return spaces;
}

std::wstring JsonNode::stringify(unsigned int indent_level)
{
    if (m_value->m_type == nodetypeNull)
        return L"null";

    if (m_value->m_type == nodetypeBoolean)
        return getBoolean() ? L"true" : L"false";

    if (m_value->m_type == nodetypeString)
        return L'"' + escape_string(getString()) + L'"';

    if (m_value->m_type == nodetypeInteger)
    {
        wchar_t buf[30];
        swprintf(buf, 30, L"%d", getInteger());
        return buf;
    }
        
    if (m_value->m_type == nodetypeDouble)
    {
        return kl::dbltostr(getDouble());
    }

    // following are for formatting
    std::wstring newline = L"\n";

    if (m_value->m_type == nodetypeArray)
    {
        std::wstring result;
        result += L"[" + newline + addspaces(indent_level+1);

        std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
        it_end = m_value->m_child_nodes_ordered.end();

        bool first = true;
        for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
        {
            if (!first)
                result += L"," + newline + addspaces(indent_level+1);
            first = false;

            result += it->second.stringify(indent_level+1);
        }
        
        result += newline + addspaces(indent_level) + L"]";
        return result;
    }
    
    if (m_value->m_type == nodetypeObject)
    {
        std::wstring result;
        result += L"{" + newline + addspaces(indent_level+1);

        std::vector<std::pair<std::wstring,JsonNode>>::iterator it, it_end;
        it_end = m_value->m_child_nodes_ordered.end();

        bool first = true;
        for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
        {
            if (!first)
                result += L"," + newline + addspaces(indent_level+1);
            first = false;

            // stringify the key
            result += (L'"' + escape_string(it->first) + L'"');
            
            // separator
            result += L":";

            // if the value is an array or an object, add in a return
            if (it->second.m_value->m_type == nodetypeArray || 
                it->second.m_value->m_type == nodetypeObject)
            {
                result += newline + addspaces(indent_level+1);       
            }

            // stringify the value
            result += it->second.stringify(indent_level+1);
        }

        result += newline + addspaces(indent_level) + L"}";
        return result;
    }
    
    return L"";
}


};  // namespace kl

