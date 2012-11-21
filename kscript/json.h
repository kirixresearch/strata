/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Aaron L. Williams
 * Created:  2008-12-22
 *
 */


#ifndef __KSCRIPT_JSON_H
#define __KSCRIPT_JSON_H


#include <cstdio>
#include "../kl/include/kl/string.h"


namespace kscript
{


// helper class for storing information when stringifying objects
class JsonInfo
{
public:

    JsonInfo()
    {
        key.setString(L"");
        result_str = L"";
        append_str = L"";
        last_token = L"";
        level = 0;
    }

    ~JsonInfo()
    {
    }

public:

    Value key;
    std::wstring result_str;
    std::wstring append_str;
    std::wstring last_token;
    int level;
};


// json object class
class Json : public ValueObject
{
    BEGIN_KSCRIPT_CLASS("JSON", Json)
        KSCRIPT_METHOD("constructor", Json::constructor)
        KSCRIPT_STATIC_METHOD("stringify", Json::stringify)
        KSCRIPT_STATIC_METHOD("parse", Json::parse)
    END_KSCRIPT_CLASS()

public:

    Json();
    ~Json();

    void constructor(ExprEnv* env, Value* retval);
    static void stringify(ExprEnv* env, void*, Value* retval);
    static void parse(ExprEnv* env, void*, Value* retval);

private:

    static void encode(ExprEnv* env, 
                       Value* value,
                       Value* replacer,
                       Value* spacer,
                       JsonInfo& info);

    static Value* parseJson(ExprEnv* env,
                            wchar_t* expr,
                            wchar_t** endloc = NULL);
    
    static Value* parseJsonValue(ExprEnv* env, wchar_t* expr, wchar_t** endloc = NULL);
    static Value* parseJsonObject(ExprEnv* env, wchar_t* expr, wchar_t** endloc = NULL);
    static Value* parseJsonArray(ExprEnv* env, wchar_t* expr, wchar_t** endloc = NULL);
    static Value* parseJsonString(ExprEnv* env, wchar_t* expr, wchar_t** endloc = NULL);
    static Value* parseJsonNumber(ExprEnv* env, wchar_t* expr, wchar_t** endloc = NULL);
    static Value* parseJsonWord(ExprEnv* env, wchar_t* expr, wchar_t** endloc = NULL);
};


// useful class for interfacing with JSON script object
class JsonNode
{
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
            
        char buf[20];
        sprintf(buf, "%d", i);
        return getChild(kl::towstring(buf));
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
    
    bool getChildExists(const std::wstring& _str)
    {
        kscript::Value* value = getValuePtr();
        if (value->lookupMember(_str, false) != NULL)
            return true;
             else
            return false;
    }

    std::wstring getChildName(size_t child_idx)
    {
        kscript::ValueObject* vobj = getValuePtr()->getObject();
        return vobj->getRawMemberName(child_idx);
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
    
    size_t getChildCount() { return (size_t)getValuePtr()->getObject()->getHighestIndex(); }

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
    
    void setString(const std::wstring& str) { getValuePtr()->setString(str); }
    void setBoolean(bool b)                 { getValuePtr()->setBoolean(b); }
    void setDouble(double num)              { getValuePtr()->setDouble(num); }
    void setInteger(int num)                { getValuePtr()->setInteger(num); }
    void setArray()                         { getValuePtr()->setArray(NULL); }

    std::wstring getString()                { return getValuePtr()->getString(); }
    bool getBoolean()                       { return getValuePtr()->getBoolean(); }
    double getDouble()                      { return getValuePtr()->getDouble(); }
    int getInteger()                        { return getValuePtr()->getInteger(); }

    bool isNull()                           { return getValuePtr()->isNull(); }
    bool isOk()                             { return !isNull(); }
    
    operator std::wstring()
    {
        return getValuePtr()->getString();
    }
    
    std::wstring toString()
    {
        kscript::Value p1;
        kscript::Value p2;
        p1.setNull();
        p2.setString(L"    ");
        
        kscript::Json j;
        kscript::ExprEnv e;
        e.reserveParams(3);
        e.m_eval_params[0] = getValuePtr();
        e.m_eval_params[1] = &p1;
        e.m_eval_params[2] = &p2;
        e.m_param_count = 3;
        
        kscript::Value retval;
        j.stringify(&e, NULL, &retval);
        
        std::wstring result;
        result = retval.getString();
        e.m_eval_params[0] = NULL;
        e.m_eval_params[1] = NULL;
        e.m_eval_params[2] = NULL;
        
        return result;
    }

    bool fromString(const std::wstring& str)
    {
        m_value.reset();
        
        kscript::ExprParser parser(kscript::optionLanguageECMAScript);
        kscript::Value json_str;
        json_str.setString(str);
        
        kscript::Json j;
        kscript::ExprEnv e;
        e.reserveParams(1);
        e.m_parser = &parser;
        e.m_eval_params[0] = &json_str;
        e.m_param_count = 1;
        
        m_value.reset();
        j.parse(&e, NULL, &m_value);
        e.m_eval_params[0] = NULL;
        
        return m_value.isObject();
    }

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


};  // namespace kscript


#endif

