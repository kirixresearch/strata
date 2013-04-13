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


// helper functions
std::wstring escape_string(const std::wstring& str);


class JsonValue
{
friend class JsonNode;

public:

    JsonValue()
    {
        m_child_nodes.clear();
        m_string.clear();
        m_double = 0.0f;
        m_integer = 0;
        m_boolean = false;
        m_type = 0;         // undefined node type

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
    std::vector< std::pair<std::wstring,JsonNode> > m_child_nodes_ordered;

    std::wstring m_string;
    double m_double;
    int m_integer;
    bool m_boolean;

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
    
    bool childExists(const std::wstring& str) const;
    bool childExists(const std::string& str) const;
    
    JsonNode getChild(const std::wstring& str);
    JsonNode getChild(const std::string& str);

    std::vector<std::wstring> getChildKeys() const;
    std::vector<JsonNode> getChildren() const;
    size_t getChildCount() const;
    JsonNode appendElement();

    void setObject();
    void setArray();
    void setString(const std::wstring& str);
    void setBoolean(bool b);
    void setDouble(double num);
    void setInteger(int num);
    void setNull();

    std::wstring getString() const;
    bool getBoolean() const;
    double getDouble() const;
    int getInteger() const;

    bool isObject() const;
    bool isArray() const;
    bool isString() const;
    bool isBoolean() const;
    bool isDouble() const;
    bool isInteger() const;
    bool isNull() const;
    bool isOk() const;
    bool isUndefined() const;
    
    operator std::wstring() const;
    std::wstring toString() const;
    bool fromString(const std::wstring& str);

    void copyFrom(const JsonNode& node);
    void init();

private:

    bool parse(wchar_t* expr);
    std::wstring stringify(unsigned int indent_level = 0) const;

private:

    // JsonNode value types
    enum NodeType
    {
        nodetypeUndefined = 0,
        nodetypeNull = 1,
        nodetypeBoolean = 2,
        nodetypeInteger = 3,
        nodetypeDouble = 4,
        nodetypeString = 5,
        nodetypeArray = 6,
        nodetypeObject = 7
    };

    JsonValue* m_value; 
};

class JsonNodeValidator
{
public:

    JsonNodeValidator();
    ~JsonNodeValidator();

    bool isValid(JsonNode& data, JsonNode& schema);
    std::vector<std::wstring> getMessages();

private:

    bool checkJsonNode(JsonNode& data, JsonNode& schema);
    bool checkTypePrimitive(JsonNode& data, const std::wstring& type);
    bool checkType(JsonNode& data, JsonNode& schema);
    bool checkTypeDisallowed(JsonNode& data, JsonNode& schema);
    bool checkNumberValue(JsonNode& data, JsonNode& schema);
    bool checkEnumValue(JsonNode& data, JsonNode& schema);
    bool checkStringValue(JsonNode& data, JsonNode& schema);
    bool checkArraySize(JsonNode& data, JsonNode& schema);
    bool checkArrayItems(JsonNode& data, JsonNode& schema);
    bool checkObjectKeys(JsonNode& data, JsonNode& schema);
    bool checkObjectValues(JsonNode& data, JsonNode& schema);

    bool isPrimitiveValueEqual(JsonNode& node1, JsonNode& node2);
    bool isJsonNodeValueEqual(JsonNode& node1, JsonNode& node2);

    void flagError(JsonNode& data, const std::wstring& message);
    bool hasErrors();

private:

    std::vector<std::wstring> m_messages;
    bool m_errors_exist;
};


};  // namespace kl


#endif

