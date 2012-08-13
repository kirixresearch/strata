/*!
 *
 * Copyright (c) 2008-2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Aaron L. Williams
 * Created:  2012-08-09
 *
 */


#ifndef __KL_JSON_H
#define __KL_JSON_H


#include <string>
#include <vector>


namespace kl
{

class JsonNode;

class JsonNode
{
public:

    JsonNode();
    ~JsonNode();

    JsonNode(const JsonNode& _c);

    JsonNode& operator=(const JsonNode& _c);
    JsonNode& operator=(int i);
    JsonNode& operator=(double d);
    JsonNode& operator=(const std::wstring& str);    
    JsonNode operator[](int i);
    JsonNode operator[](const char* str);
    JsonNode operator[](const std::wstring& str);
    
    bool childExists(const std::wstring& _str);
    JsonNode getChild(const std::wstring& _str);

    std::vector<std::wstring> getChildKeys();
    std::vector<JsonNode> getChildren();
    size_t getChildCount();

    void setArray();
    JsonNode appendElement();

    void setString(const std::wstring& str);
    void setBoolean(bool b);
    void setDouble(double num);
    void setInteger(int num);
    void setNull();

    std::wstring getString();
    bool getBoolean();
    double getDouble();
    int getInteger();

    bool isNull();
    bool isOk();
    
    operator std::wstring();
    std::wstring toString();

    bool fromString(const std::wstring& str);

private:

    bool parse(wchar_t* expr, wchar_t** endloc);
    std::wstring stringify();

private:

    // JsonNode value types
    enum NodeType
    {
        nodetypeNull = 0,
        nodetypeObject = 1,
        nodetypeArray = 2,
        nodetypeString = 3,
        nodetypeDouble = 4,
        nodetypeInteger = 5,
        nodetypeBoolean = 6
    };

    std::vector<std::pair<std::wstring,JsonNode>> m_child_nodes;
    std::wstring m_string;
    double m_double;
    int m_integer;
    bool m_boolean;
    bool m_isnull;

    int m_type;
};


};  // namespace kl


#endif

