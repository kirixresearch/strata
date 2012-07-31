/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2009-05-01
 *
 */


#ifndef __APP_JSONCONFIG_H
#define __APP_JSONCONFIG_H


#include "../kscript/kscript.h"
#include <kl/string.h>

class JsonNode
{
    friend class JsonConfig;
    
public:

    JsonNode() { }
    
    JsonNode(const JsonNode& _c)
    {
        JsonNode& c = (JsonNode&)_c;
        copyFrom(&c.m_value);
    }
    
    JsonNode(const kscript::Value& _v)
    {
        kscript::Value& v = (kscript::Value&)_v;
        copyFrom(&v);
    }
    
    JsonNode& operator=(const JsonNode& _c)
    {
        JsonNode& c = (JsonNode&)_c;
        copyFrom(&c.m_value);
        return *this;
    }
    
    JsonNode& operator=(const std::wstring& str)
    {
        setString(str);
        return *this;
    }
    
    JsonNode& operator=(const std::string& str)
    {
        setString(str);
        return *this;
    }

    JsonNode& operator=(int i)
    {
        setInteger(i);
        return *this;
    }
    
    JsonNode& operator=(bool b)
    {
        setBoolean(b);
        return *this;
    }
    
    JsonNode& operator=(double d)
    {
        setDouble(d);
        return *this;
    }
    
    JsonNode operator[](int i)
    {
        kscript::Value* value = getValuePtr();
        if (!value->isObject())
            value->setArray(NULL);
        wchar_t buf[80];
        swprintf(buf, 80, L"%d", i);
        return getChild(buf);
    }
    
    JsonNode operator[](const char* str)
    {
        return (*this)[kl::towstring(str)];
    }
    
    JsonNode operator[](const std::wstring& str)
    {
        return getChild(str);
    }
    
    JsonNode appendElement()
    {
        kscript::Value* value = getValuePtr();
        if (!value->isObject())
            value->setArray(NULL);
        return (*this)[(int)value->getObject()->getHighestIndex()];
    }
    
    JsonNode getChild(const std::wstring& _str)
    {
        std::wstring str = _str;
        kscript::Value* value = getValuePtr();
        if (!value->isObject())
            value->setObject();
        JsonNode result;
        kscript::Value* val = value->getMember(str);
        //val->eval(NULL, &result.m_value);
        result.m_value.setRef(val);
        return result;
    }
    
    size_t getCount() { return (size_t)getValuePtr()->getObject()->getHighestIndex(); }

    std::vector<std::wstring> getChildKeys()
    {
        kscript::ValueObject* vobj = getValuePtr()->getObject();
        size_t i, count = vobj->getRawMemberCount();
        
        std::vector<std::wstring> keys;
        keys.reserve(count);

        for (i = 0; i < count; ++i)
        {
            keys.push_back(vobj->getRawMemberName(i));
        }

        return keys;
    }

    bool childExists(const std::wstring& str)
    {
        kscript::ValueObject* vobj = getValuePtr()->getObject();
        return vobj->getMemberExists(str);
    }

    kscript::Value* getValuePtr()
    {
        if (m_value.isRef())
        {
            kscript::Value* v = &m_value;
            while (v->isRef())
                v = v->getRef();
            return v;
        }
        return &m_value;
    }
    
    void setString(const std::string& str)  { getValuePtr()->setString(str); }
    void setString(const std::wstring& str) { getValuePtr()->setString(str); }
    void setBoolean(bool b)             { getValuePtr()->setBoolean(b); }
    void setDouble(double num)          { getValuePtr()->setDouble(num); }
    void setInteger(int num)            { getValuePtr()->setInteger(num); }
    void setArray()                     { getValuePtr()->setArray(NULL); }

    std::wstring getString()            { return getValuePtr()->getString(); }
    bool getBoolean()                   { return getValuePtr()->getBoolean(); }
    double getDouble()                  { return getValuePtr()->getDouble(); }
    int getInteger()                    { return getValuePtr()->getInteger(); }

    bool isNull()                       { return getValuePtr()->isNull(); }
    bool isOk()                         { return !isNull(); }
    
    operator std::wstring()
    {
        return getValuePtr()->getString();
    }
    
    std::wstring toString();
    bool fromString(const std::wstring& str);

private:

    void copyFrom(kscript::Value* src_val)
    {
        if (src_val->isRef())
        {
            m_value.reset();
            m_value.setRef(src_val->getRef());
            return;
        }
         else
        {
            src_val->eval(NULL, &m_value);
        }
    }
        
private:

    kscript::Value m_value;
};


class JsonConfig
{
public:

    static JsonNode loadFromString(const std::wstring& json);
    static bool saveToString(JsonNode& node, std::wstring& dest);
};


#endif
