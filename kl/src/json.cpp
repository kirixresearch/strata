/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
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
#include "kl/regex.h"


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



////////////////////////////////////////////////////////////////////////////////
//
// JsonNode implementation
//
////////////////////////////////////////////////////////////////////////////////


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

std::wstring escape_string(const std::wstring& str)
{
    std::wstring result = L"";

    std::wstring::const_iterator it, it_end;
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

    // release old value
    m_value->unref();

    // assign new value
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

bool JsonNode::childExists(const std::string& str) const
{
    return childExists(kl::towstring(str));
}

bool JsonNode::childExists(const std::wstring& str) const
{
    std::map<std::wstring,JsonNode>::const_iterator it = m_value->m_child_nodes.find(str);
    if (it != m_value->m_child_nodes.end())
        return true;

    return false;
}

JsonNode JsonNode::getChild(const std::wstring& _str)
{
    // try to find the child in the mapped container
    std::map<std::wstring,JsonNode>::iterator it = m_value->m_child_nodes.find(_str);
    if (it != m_value->m_child_nodes.end())
        return it->second;

    // reset the node type to object, as we'll be adding an element
    m_value->m_type = nodetypeObject;

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


std::vector<std::wstring> JsonNode::getChildKeys() const
{
    // return an ordered list of keys
    std::vector<std::wstring> child_keys;
    child_keys.reserve(m_value->m_child_nodes_ordered.size());

    std::vector<std::pair<std::wstring,JsonNode> >::const_iterator it, it_end;
    it_end = m_value->m_child_nodes_ordered.end();
    
    for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
    {
        child_keys.push_back(it->first);
    }

    return child_keys;
}

std::vector<JsonNode> JsonNode::getChildren() const
{
    // return an ordered list of children
    std::vector<JsonNode> child_nodes;
    child_nodes.reserve(m_value->m_child_nodes_ordered.size());

    std::vector<std::pair<std::wstring,JsonNode> >::const_iterator it, it_end;
    it_end = m_value->m_child_nodes_ordered.end();
    
    for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
    {
        child_nodes.push_back(it->second);
    }

    return child_nodes;
}

size_t JsonNode::getChildCount() const
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

bool JsonNode::deleteChild(size_t idx)
{
    wchar_t buf[30];
    swprintf(buf, 30, L"%d", idx);

    std::map<std::wstring,JsonNode>::iterator mit = m_value->m_child_nodes.find(buf);
    if (mit == m_value->m_child_nodes.end())
        return false;

    m_value->m_child_nodes.erase(mit);



    std::vector<std::pair<std::wstring,JsonNode> >::iterator it, it_end;
    it_end = m_value->m_child_nodes_ordered.end();
    
    for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
    {
        if (it->first == buf)
        {
            m_value->m_child_nodes_ordered.erase(it);
            break;
        }
    }


    return true;
}


void JsonNode::setObject()
{
    m_value->m_child_nodes.clear();
    m_value->m_child_nodes_ordered.clear();
    m_value->m_string.clear();
    m_value->m_double = 0.0f;
    m_value->m_integer = 0;
    m_value->m_boolean = false;
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
    m_value->m_type = nodetypeNull;    
}

std::wstring JsonNode::getString() const
{
    switch (m_value->m_type)
    {
        default:
            return L"";
        case nodetypeUndefined:
            return L"";
        case nodetypeNull:
            return L"null";
        case nodetypeBoolean:
            return m_value->m_boolean ? L"true" : L"false";
        case nodetypeInteger:
            {
                wchar_t buf[30];
                swprintf(buf, 30, L"%d", m_value->m_integer);
                return buf;
            }
        case nodetypeDouble:
            return kl::dbltostr(getDouble());
        case nodetypeString:
            return m_value->m_string;
        case nodetypeArray:
        case nodetypeObject:
            return L"";
    }
}

bool JsonNode::getBoolean() const
{
    switch (m_value->m_type)
    {
        default:
            return false;
        case nodetypeUndefined:
        case nodetypeNull:
            return false;
        case nodetypeBoolean:
            return m_value->m_boolean;
        case nodetypeInteger:
            return (m_value->m_integer != 0 ? true : false);
        case nodetypeDouble:
            return (m_value->m_double != 0 ? true : false);
        case nodetypeString:
            return (m_value->m_string.size() > 0 ? true : false); // ECMAScript behavior
        case nodetypeObject:
        case nodetypeArray:
            return true;
    }

    return false;
}

double JsonNode::getDouble() const
{
    // TODO: should consider using NaN for undefined, 
    // array, and object (null should still convert to 0)

    switch (m_value->m_type)
    {
        default:
            return 0;
        case nodetypeUndefined:
        case nodetypeNull:
            return 0;
        case nodetypeBoolean:
            return m_value->m_boolean ? 1 : 0;
        case nodetypeInteger:
            return m_value->m_integer;
        case nodetypeDouble:
            return m_value->m_double;
        case nodetypeString:
            return wtof(m_value->m_string);
        case nodetypeArray:
        case nodetypeObject:
            return 0;
    }
}

int JsonNode::getInteger() const
{
    switch (m_value->m_type)
    {
        default:
            return 0;
        case nodetypeUndefined:
        case nodetypeNull:
            return 0;
        case nodetypeBoolean:
            return m_value->m_boolean ? 1 : 0;
        case nodetypeInteger:
            return m_value->m_integer;
        case nodetypeDouble:
            return m_value->m_double;
        case nodetypeString:
            return wtoi(m_value->m_string);
        case nodetypeArray:
        case nodetypeObject:
            return 0;
    }
}

bool JsonNode::isObject() const
{
    return (m_value->m_type == nodetypeObject);
}

bool JsonNode::isArray() const
{
    return (m_value->m_type == nodetypeArray);
}

bool JsonNode::isString() const
{
    return (m_value->m_type == nodetypeString);
}

bool JsonNode::isBoolean() const
{
    return (m_value->m_type == nodetypeBoolean);
}

bool JsonNode::isDouble() const
{
    return (m_value->m_type == nodetypeDouble);
}

bool JsonNode::isInteger() const
{
    return (m_value->m_type == nodetypeInteger);
}

bool JsonNode::isNull() const
{
    return (m_value->m_type == nodetypeNull);
}

bool JsonNode::isOk() const
{
    return !isUndefined();
}

bool JsonNode::isUndefined() const
{
    return (m_value->m_type == nodetypeUndefined);
}

JsonNode::operator std::wstring() const
{
    return getString();
}

std::wstring JsonNode::toString() const
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
    m_value->m_type = nodetypeUndefined;
}

bool JsonNode::parse(wchar_t* expr)
{
    wchar_t* endloc = NULL;
    bool success = parseJsonValue(expr, &endloc, *this);
    
    // make sure there's nothing left over
    skipWhiteSpaceOrLS(endloc);
    if (*endloc)
        success = false;

    if (!success)
    {
        // reset the json
        init();
        return false;
    }
    
    return true;
}

inline std::wstring addspaces(unsigned int indent_level)
{
    std::wstring spaces;
    spaces.append(indent_level*4, L' ');
    return spaces;
}

std::wstring JsonNode::stringify(unsigned int indent_level) const
{
    if (m_value->m_type == nodetypeUndefined)
        return getString();

    if (m_value->m_type == nodetypeNull)
        return getString();

    if (m_value->m_type == nodetypeBoolean)
        return getString();

    if (m_value->m_type == nodetypeInteger)
        return getString();
        
    if (m_value->m_type == nodetypeDouble)
        return getString();

    // make sure to escape the string
    if (m_value->m_type == nodetypeString)
        return L'"' + escape_string(getString()) + L'"';

    // following are for formatting
    std::wstring newline = L"\n";

    if (m_value->m_type == nodetypeArray)
    {
        std::wstring result;
        result += L"[" + newline + addspaces(indent_level+1);

        std::vector<std::pair<std::wstring,JsonNode> >::iterator it, it_end;
        it_end = m_value->m_child_nodes_ordered.end();

        bool first = true;
        for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
        {
            // if a node is undefined, don't include it to avoid 
            // creating output like:  ["a",,,"b"]
            if (it->second.isUndefined())
                continue;

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

        std::vector<std::pair<std::wstring,JsonNode> >::iterator it, it_end;
        it_end = m_value->m_child_nodes_ordered.end();

        bool first = true;
        for (it = m_value->m_child_nodes_ordered.begin(); it != it_end; ++it)
        {
            // if a node is undefined, don't include it to avoid 
            // creating output like:  {"a": }
            if (it->second.isUndefined())
                continue;

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




////////////////////////////////////////////////////////////////////////////////
//
// JsonNodeValiator implementation
//
////////////////////////////////////////////////////////////////////////////////


JsonNodeValidator::JsonNodeValidator()
{
    m_errors_exist = false;
}

JsonNodeValidator::~JsonNodeValidator()
{
}

bool JsonNodeValidator::isValid(JsonNode& data, JsonNode& schema)
{
    // TODO: would be nice to make the input const JsonNode& schema;
    // however schema isn't const because implementation depends on 
    // getChild() which creates a child if it doesn't exist

    // reset the error flag and the messages
    m_errors_exist = false;
    m_messages.clear();

    // check the node and return the results
    if (!checkJsonNode(data, schema))
        return false;

    return true;
}

bool JsonNodeValidator::checkJsonNode(JsonNode& data, JsonNode& schema)
{
    // make sure schema is an object
    if (!schema.isObject())
    {
        // flag error and return, since there's nothing more we can do
        flagError(data, L"Error: schema is not an object type");
        return false;
    }

    // validate the object type
    if (!checkType(data, schema))
        flagError(data, L"Error: node type is not one of allowed types near '%s'");

    // validate the object type based on disallowed types
    if (!checkTypeDisallowed(data, schema))
        flagError(data, L"Error: node type is disallowed near '%s'");

    // validate any numeric value
    if (!checkNumberValue(data, schema))
        flagError(data, L"Error: number value invalid near '%s'");

    // validate any string value
    if (!checkStringValue(data, schema))
        flagError(data, L"Error: string value invalid near '%s'");

    // validate against an enumeration of values
    if (!checkEnumValue(data, schema))
        flagError(data, L"Error: value is not an enumerated value near '%s'");

    // validate any array size
    if (!checkArraySize(data, schema))
        flagError(data, L"Error: array size out of bounds near '%s'");

    // validate array items
    if (!checkArrayItems(data, schema))
        flagError(data, L"Error: array item invalid near '%s'");

    // validate object keys
    if (!checkObjectKeys(data, schema))
        flagError(data, L"Error: object key invalid near '%s'");

    // validate object properties
    if (!checkObjectValues(data, schema))
        return false;

    // TODO: implement:

    // extends
    // $ref
    // patternProperties
    // dependencies
    // enum
    // format
    // additionalItems
    // uniqueItems

    if (hasErrors())
        return false;

    return true;
}

bool JsonNodeValidator::checkTypePrimitive(JsonNode& data, const std::wstring& type)
{
    if (type == L"any")
        return true;
    if (type == L"null" && !data.isNull())
        return false;
    if (type == L"boolean" && !data.isBoolean())
        return false;
    if (type == L"integer" && !data.isInteger())
        return false;
    if (type == L"number" && !(data.isInteger() || data.isDouble()))
        return false;
    if (type == L"string" && !data.isString())
        return false;
    if (type == L"object" && !data.isObject())
        return false;
    if (type == L"array" && !data.isArray())
        return false;

    // something not in this list, so any value is allowed
    return true;
}

bool JsonNodeValidator::checkType(JsonNode& data, JsonNode& schema)
{
    // if the type doesn't exist in the schema, any type is valid
    if (!schema.childExists(L"type"))
        return true;

    // type has to be either a string or an array
    if (schema[L"type"].isString())
        return checkTypePrimitive(data, schema[L"type"].getString());

    if (schema[L"type"].isArray())
    {
        std::vector<JsonNode> children = schema[L"type"].getChildren();
        std::vector<JsonNode>::iterator it, it_end;
        it_end = children.end();

        bool match = false;
        for (it = children.begin(); it != it_end; ++it)
        {
            if (checkTypePrimitive(data, it->getString()))
                return true;

            // TODO: need to handle case where type in array is a schema
        }

        // type doesn't match any of the children
        return false;
    }

    // type is specified, but it's not a string or an array; 
    // handle as if type isn't defined
    return true;
}

bool JsonNodeValidator::checkTypeDisallowed(JsonNode& data, JsonNode& schema)
{
    // TODO: fill out

    // disallow

    return true;
}

bool JsonNodeValidator::checkNumberValue(JsonNode& data, JsonNode& schema)
{
    // if the data type isn't a number, nothing to validate
    if (!data.isDouble() && !data.isInteger())
        return true;

    // validate the minimum number
    if (schema.childExists(L"minimum"))
    {
        bool exclusive = false;
        if (schema.childExists(L"exclusiveMinimum"))
            exclusive = schema[L"exclusiveMinimum"].getBoolean();

        JsonNode constraint_node = schema[L"minimum"];

        // validate double value
        if (constraint_node.isDouble())
        {
            double minvalue = constraint_node.getDouble();

            if (exclusive)
            {
                if (data.isDouble() && data.getDouble() <= minvalue)
                    return false;
                if (data.isInteger() && data.getInteger() <= minvalue)
                    return false;
            }
             else
            {
                if (data.isDouble() && data.getDouble() < minvalue)
                    return false;
                if (data.isInteger() && data.getInteger() < minvalue)
                    return false;
            }
        }

        // validate integer value
        if (constraint_node.isInteger())
        {
            int minvalue = constraint_node.getInteger();

            if (exclusive)
            {
                if (data.isDouble() && data.getDouble() <= minvalue)
                    return false;
                if (data.isInteger() && data.getInteger() <= minvalue)
                    return false;
            }
             else
            {
                if (data.isDouble() && data.getDouble() < minvalue)
                    return false;
                if (data.isInteger() && data.getInteger() < minvalue)
                    return false;
            }
        }

        // if minimum is something else, don't do anything
    }

    // validate the maximum number
    if (schema.childExists(L"maximum"))
    {
        bool exclusive = false;
        if (schema.childExists(L"exclusiveMaximum"))
            exclusive = schema[L"exclusiveMaximum"].getBoolean();

        JsonNode constraint_node = schema[L"maximum"];

        // validate double value
        if (constraint_node.isDouble())
        {
            double maxvalue = constraint_node.getDouble();

            if (exclusive)
            {
                if (data.isDouble() && data.getDouble() >= maxvalue)
                    return false;
                if (data.isInteger() && data.getInteger() >= maxvalue)
                    return false;
            }
             else
            {
                if (data.isDouble() && data.getDouble() > maxvalue)
                    return false;
                if (data.isInteger() && data.getInteger() > maxvalue)
                    return false;
            }
        }

        // validate integer value
        if (constraint_node.isInteger())
        {
            int maxvalue = constraint_node.getInteger();

            if (exclusive)
            {
                if (data.isDouble() && data.getDouble() >= maxvalue)
                    return false;
                if (data.isInteger() && data.getInteger() >= maxvalue)
                    return false;
            }
             else
            {
                if (data.isDouble() && data.getDouble() > maxvalue)
                    return false;
                if (data.isInteger() && data.getInteger() > maxvalue)
                    return false;
            }
        }

        // if maximum is something else, don't do anything
    }

    if (schema.childExists(L"divisibleBy"))
    {
        JsonNode constraint_node = schema[L"divisibleBy"];

        if (constraint_node.isDouble())
        {
            double divisibleby = constraint_node.getDouble();
            if (divisibleby != 0 && data.isDouble() && (fmod(data.getDouble(),divisibleby) != 0))
                return false;
            if (divisibleby != 0 && data.isInteger() && (fmod(data.getInteger(),divisibleby) != 0))
                return false;
        }

        if (constraint_node.isInteger())
        {
            int divisibleby = constraint_node.getInteger();
            if (divisibleby != 0 && data.isDouble() && (fmod(data.getDouble(),(double)divisibleby) != 0))
                return false;
            if (divisibleby != 0 && data.isInteger() && (data.getInteger() % divisibleby != 0))
                return false;
        }
    }

    return true;
}

bool JsonNodeValidator::checkEnumValue(JsonNode& data, JsonNode& schema)
{
    // if the enum value doesn't exist, nothing to validate
    if (!schema.childExists("enum"))
        return true;

    // the enumeration has to be an array; if it isn't, the
    // data passes the test
    JsonNode enum_node = schema[L"enum"];
    if (!enum_node.isArray())
        return true;

    // an enumeration exists; compare the values in the enumeration
    // against the object
    std::vector<JsonNode> enum_children = enum_node.getChildren();
    std::vector<JsonNode>::iterator it, it_end;
    it_end = enum_children.end();

    for (it = enum_children.begin(); it != it_end; ++it)
    {
        if (isJsonNodeValueEqual(data, *it))
            return true;
    }

    // none of the objects match
    return false;
}

bool JsonNodeValidator::checkStringValue(JsonNode& data, JsonNode& schema)
{
    // if the data type isn't a string, nothing to validate
    if (!data.isString())
        return true;

    // validate the minimum string length
    if (schema.childExists(L"minLength"))
    {
        int minlen = schema[L"minLength"].getInteger();
        int strlen = data.getString().length();

        if (strlen < minlen)
            return false;
    }

    // validate the maximum string length
    if (schema.childExists(L"maxLength"))
    {
        int maxlen = schema[L"maxLength"].getInteger();
        int strlen = data.getString().length();

        if (strlen > maxlen)
            return false;
    }

    // validate the string pattern
    if (schema.childExists(L"pattern"))
    {
        JsonNode constraint_node = schema[L"pattern"];

        std::wstring regexstr;
        if (constraint_node.isString())
            regexstr = constraint_node.getString();

        klregex::wregex regex;
        regex.assign(regexstr);

        std::wstring t = data.getString();
        const wchar_t* datastr = t.c_str();

        // note: use search rather than match so that an embedded string 
        // match counts (i.e., a regex of "b" would match a string of "abc"), 
        // which is the required functionality for the pattern parameter
        if (!regex.search(datastr, datastr + wcslen(datastr)))
            return false;
    }

    return true;
}

bool JsonNodeValidator::checkArraySize(JsonNode& data, JsonNode& schema)
{
    // if the data type isn't an array, nothing to validate
    if (!data.isArray())
        return true;

    if (schema.childExists(L"minItems"))
    {
        int minitem_count = schema[L"minItems"].getInteger();
        int child_count = data.getChildCount();

        if (child_count < minitem_count)
            return false;
    }

    if (schema.childExists(L"maxItems"))
    {
        int maxitem_count = schema[L"maxItems"].getInteger();
        int child_count = data.getChildCount();

        if (child_count > maxitem_count)
            return false;
    }

    return true;
}

bool JsonNodeValidator::checkArrayItems(JsonNode& data, JsonNode& schema)
{
    // if the data type isn't an array, nothing to validate
    if (!data.isArray())
        return true;

    JsonNode schema_items = schema[L"items"];

    // if the items node is an object, then it's a schema;
    // compare it against each of the items in the data array  
    if (schema_items.isObject())
    {
        // validate each of the array items against the schema
        // defined by the items
        std::vector<JsonNode> child_nodes = data.getChildren();
        std::vector<JsonNode>::iterator it, it_end;
        it_end = child_nodes.end();

        for (it = child_nodes.begin(); it != child_nodes.end(); ++it)
        {
            if (!checkJsonNode(*it, schema_items))
                return false;
        }
    }

    // if the schema items node is an array, compare the elements of
    // the schema items array to the items in the data array by
    // cycling through the elements of the schema items array as
    // we cycle through the data items
    if (schema_items.isArray())
    {
        // TODO: implement
    }

    return true;
}

bool JsonNodeValidator::checkObjectKeys(JsonNode& data, JsonNode& schema)
{
    // if the data type isn't an object, nothing to validate
    if (!data.isObject())
        return true;

    // get the list of required keys; TODO: verify behavior;
    // this behavior matches examples rather than the draft3
    // standard
    JsonNode schema_requiredkeys_node = schema[L"required"];
    if (schema_requiredkeys_node.isArray())
    {
        std::vector<JsonNode> required_keys = schema_requiredkeys_node.getChildren();
        std::vector<JsonNode>::iterator it, it_end;
        it_end = required_keys.end();

        for (it = required_keys.begin(); it != it_end; ++it)
        {
            // validate the key
            if (!data.childExists(it->getString()))
                return false;
        }
    }

    return true;
}

bool JsonNodeValidator::checkObjectValues(JsonNode& data, JsonNode& schema)
{
    // if the data type isn't an object, nothing to validate
    if (!data.isObject())
        return true;

    // get the property tests from the schema
    JsonNode schema_properties = schema[L"properties"];
    JsonNode schema_additionalproperties = schema[L"additionalProperties"];

    // if neither are defined, we're done
    if (schema_properties.isUndefined() && schema_additionalproperties.isUndefined())
        return true;

    // get the data object children
    std::vector<std::wstring> child_keys = data.getChildKeys();
    std::vector<std::wstring>::iterator it, it_end;
    it_end = child_keys.end();

    // iterate through the children and test them against the
    // properties and the additional properties
    for (it = child_keys.begin(); it != it_end; ++it)
    {
        // test against the schema properties
        if (schema_properties.isObject() && schema_properties.childExists(*it))
        {
            JsonNode data_child_node = data.getChild(*it);
            JsonNode schema_property_child_node = schema_properties.getChild(*it);
            
            // we have schema defined for the child node, so validate against it
            if (!checkJsonNode(data_child_node, schema_property_child_node))
                return false;

            // valid; no need to compare against the additional properties
            continue;
        }

        // we didn't find the child in the schema; test against the
        // additional properties
        if (schema_additionalproperties.isObject())
        {
            JsonNode data_child_node = data.getChild(*it);
            if (!checkJsonNode(data_child_node, schema_additionalproperties))
                return false;
        }
    }

    // all tests pass
    return true;
}

bool JsonNodeValidator::isPrimitiveValueEqual(JsonNode& node1, JsonNode& node2)
{
    // if either node is an array or object, it's not a primitive
    if (node1.isArray() || node2.isArray())
        return false;
    if (node1.isObject() || node2.isObject())
        return false;

    // note: for combinations of null and undefined, we'll follow
    // the same conventions as ECMAScript
    if (node1.isUndefined() && node2.isUndefined())
        return true;
    if (node1.isNull() && node2.isNull())
        return true;
    if (node1.isUndefined() && node2.isNull())
        return true;
    if (node1.isNull() && node2.isUndefined())
        return true;

    // we're comparing two types, at least one of which is either a 
    // boolean, number or string; if the other is null or undefined, 
    // they're not equal
    if (node1.isUndefined() || node2.isUndefined())
        return false;
    if (node1.isNull() || node2.isNull())
        return false;

    // if we have two strings, return the results of comparing
    // them directly
    if (node1.isString() && node2.isString())
        return (node1.getString() == node2.getString());

    // we're comparing two primitive types of mixed boolean, number,
    // or string types; if they are numerically equivalent, the 
    // values match
    if (node1.getDouble() == node2.getDouble())
        return true;

    // values don't match
    return false;
}

bool JsonNodeValidator::isJsonNodeValueEqual(JsonNode& node1, JsonNode& node2)
{
    // nodes are equal if they are both primitive types (not array 
    // and not object) and they have the same equivalent value
    if (isPrimitiveValueEqual(node1, node2))
        return true;

    // nodes are equal if they are both arrays and have the 
    // same number of elements with equivalent values
    if (node1.isArray() && node2.isArray())
    {
        int node1_childcount = node1.getChildCount();
        int node2_childcount = node2.getChildCount();

        // different number elements
        if (node1_childcount != node2_childcount)
            return false;

        // handle "no element" case
        if (node1_childcount == 0 && node2_childcount)
            return true;

        std::vector<JsonNode> node1_children = node1.getChildren();
        std::vector<JsonNode> node2_children = node2.getChildren();

        std::vector<JsonNode>::iterator it, it_end;
        it_end = node1_children.end();

        int idx = 0;
        for (it = node1_children.begin(); it != it_end; ++it)
        {
            JsonNode node1_child = *it;
            JsonNode node2_child = node2_children[idx];

            // if child elements in corresponding positions aren't equal,
            // the arrays aren't equal
            if (!isJsonNodeValueEqual(node1_child, node2_child))
                return false;
        }

        // same number of array elements and values in the same position
        return true;
    }

    // nodes are equal if they are both objects and have the 
    // same number of elements with equivalent values
    if (node1.isObject() && node2.isObject())
    {
        int node1_childcount = node1.getChildCount();
        int node2_childcount = node2.getChildCount();

        // different number elements
        if (node1_childcount != node2_childcount)
            return false;

        // handle "no element" case
        if (node1_childcount == 0 && node2_childcount)
            return true;

        // same number of non-zero elements; key's and values must match, but
        // don't have to have the same order
        std::vector<std::wstring> node1_childkeys = node1.getChildKeys();
        std::vector<std::wstring>::iterator it, it_end;
        it_end = node1_childkeys.end();

        for (it = node1_childkeys.begin(); it != it_end; ++it)
        {
            // if we can't find the node1 key in node2, objects aren't the same
            if (!node2.childExists(*it))
                return false;

            JsonNode node1_child = node1.getChild(*it);
            JsonNode node2_child = node2.getChild(*it);

            // node exists in both; values must now be the same
            if (!isJsonNodeValueEqual(node1_child, node2_child))
                return false;
        }

        // same object keys and child values
        return true;
    }

    // some other case; shouldn't happen
    return false;
}

void JsonNodeValidator::flagError(JsonNode& data, const std::wstring& message)
{
    m_errors_exist = true;    

    if (message.length() == 0)
        return;

    std::wstring error_message = message;
    std::wstring error_context = data.toString();
    error_context = error_context.substr(0,50);
    kl::replaceStr(error_message, L"%s", error_context, true);

    m_messages.push_back(error_message);
}

bool JsonNodeValidator::hasErrors()
{
    return m_errors_exist;
}

std::vector<std::wstring> JsonNodeValidator::getMessages()
{
    return m_messages;
}


};  // namespace kl

