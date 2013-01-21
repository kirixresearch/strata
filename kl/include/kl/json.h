/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
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
#include <map>


namespace kl
{

class JsonValue;
class JsonNode;


// helper function
std::wstring escape_string(std::wstring& str);


class JsonValue
{
friend JsonNode;

public:

    JsonValue()
    {
        m_child_nodes.clear();
        m_string.clear();
        m_double = 0.0f;
        m_integer = 0;
        m_boolean = false;
        m_isnull = true;
        m_type = 0;

        m_refcount = 0;
    }
    
    ~JsonValue()
    {
    }

    void ref()
    {
        m_refcount++;
    }

    void unref()
    {
        if(--m_refcount == 0)
        {
            delete this;
            return;
        }
    }

private:

    std::map<std::wstring,JsonNode> m_child_nodes;
    std::vector<std::pair<std::wstring,JsonNode>> m_child_nodes_ordered;

    std::wstring m_string;
    double m_double;
    int m_integer;
    bool m_boolean;
    bool m_isnull;

    int m_type;

private:

    int m_refcount;
};

class JsonNode
{
public:

    JsonNode();
    ~JsonNode();

    JsonNode(const JsonNode& _c);

    JsonNode& operator=(const JsonNode& _c);
    JsonNode& operator=(int i);
    JsonNode& operator=(double d);
    JsonNode& operator=(const std::string& str);
    JsonNode& operator=(const std::wstring& str);
    JsonNode operator[](int i);
    JsonNode operator[](const char* str);
    JsonNode operator[](const std::wstring& str);
    
    bool childExists(const std::wstring& str);
    bool childExists(const std::string& str);
    
    JsonNode getChild(const std::wstring& str);
    JsonNode getChild(const std::string& str);

    std::vector<std::wstring> getChildKeys();
    std::vector<JsonNode> getChildren();
    size_t getChildCount();
    JsonNode appendElement();

    void setObject();
    void setArray();
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

    void copyFrom(const JsonNode& node);
    void init();

private:

    bool parse(wchar_t* expr);
    std::wstring stringify(unsigned int indent_level = 0);

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

    JsonValue* m_value; 
};


};  // namespace kl


#endif

