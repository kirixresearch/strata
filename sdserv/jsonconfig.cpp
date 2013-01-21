/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2009-05-01
 *
 */


#include "sdserv.h"
#include "jsonconfig.h"
#include "../kscript/json.h"
#include <kl/utf8.h>


std::wstring JsonNode::toString()
{
    kscript::Value* v = getValuePtr();
    if (v->isString())
        return v->getString();
    
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

bool JsonNode::fromString(const std::wstring& str)
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




JsonNode JsonConfig::loadFromString(const std::wstring& json)
{
    JsonNode node;
    node.fromString(json);
    return node;
}



bool JsonConfig::saveToString(JsonNode& node, std::wstring& dest)
{
    std::wstring s = node.toString();
    if (s.length() > 0)
    {
        dest = s;
        return true; 
    }
    
    return false;
}



/*
void jsonTest()
{
    JsonNode node;
    node["input"] = wxT("c:\\hello");
    node["names"]["hat"][0] = 123;
    node["names"]["hat"][1] = 456;
    node["names"]["hat"][2] = 789;
    
    std::wstring str;
    JsonConfig::saveToString(node, str);
    printf("%ls\n", (const wchar_t*)str.c_str());
    
    
    JsonNode node2;
    node2 = JsonConfig::loadFromString(str);
    if (node2.isOk())
    {
        printf("%ls\n", (const wchar_t*)node["input"].getString().c_str());
        
        JsonNode arr = node["names"]["hat"];
        size_t i = 0, count = arr.getCount();
        for (i = 0; i < count; ++i)
        {
            printf("%d\n", arr[i].getInteger());
        }
    }
}
*/

