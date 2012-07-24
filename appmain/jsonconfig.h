/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-05-01
 *
 */


#ifndef __APP_JSONCONFIG_H
#define __APP_JSONCONFIG_H


#include "../kscript/kscript.h"


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
    
    JsonNode& operator=(const wxString& str)
    {
        setString(str);
        return *this;
    }
    
    JsonNode& operator=(int i)
    {
        setInteger(i);
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
        return getChild(wxString::Format(wxT("%d"), i));
    }
    
    JsonNode operator[](const char* str)
    {
        return (*this)[wxString::From8BitData(str)];
    }
    
    JsonNode operator[](const wxString& str)
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
    
    JsonNode getChild(const wxString& _str)
    {
        std::wstring str = towstr(_str);
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

    std::vector<wxString> getChildKeys()
    {
        kscript::ValueObject* vobj = getValuePtr()->getObject();
        size_t i, count = vobj->getRawMemberCount();
        
        std::vector<wxString> keys;
        keys.reserve(count);

        for (i = 0; i < count; ++i)
        {
            keys.push_back(towx(vobj->getRawMemberName(i)));
        }

        return keys;
    }

    bool childExists(const wxString& _str)
    {
        kscript::ValueObject* vobj = getValuePtr()->getObject();
        return vobj->getMemberExists(towstr(_str));
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
    
    void setString(const wxString& str) { getValuePtr()->setString(towstr(str)); }
    void setBoolean(bool b)             { getValuePtr()->setBoolean(b); }
    void setDouble(double num)          { getValuePtr()->setDouble(num); }
    void setInteger(int num)            { getValuePtr()->setInteger(num); }
    void setArray()                     { getValuePtr()->setArray(NULL); }

    wxString getString()                { return towx(getValuePtr()->getString()); }
    bool getBoolean()                   { return getValuePtr()->getBoolean(); }
    double getDouble()                  { return getValuePtr()->getDouble(); }
    int getInteger()                    { return getValuePtr()->getInteger(); }

    bool isNull()                       { return getValuePtr()->isNull(); }
    bool isOk()                         { return !isNull(); }
    
    operator wxString()
    {
        return towx(getValuePtr()->getString());
    }
    
    wxString toString();
    bool fromString(const wxString& str);

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
    static JsonNode loadFromFile(const wxString& path);
    static JsonNode loadFromDb(tango::IDatabasePtr db, const wxString& path);
    static JsonNode loadFromString(const wxString& json);
    
    static bool saveToString(JsonNode& node, wxString& dest);
    
    static bool saveToDb(JsonNode& node,
                         tango::IDatabasePtr db,
                         const wxString& path,
                         const wxString& mime_type = wxT("text/plain"));
                         
    static bool saveToFile(
                         JsonNode& node,
                         const wxString& path,
                         const wxString& mime_type = wxEmptyString);
};



#endif
