/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-05-01
 *
 */


#include "appmain.h"
#include "jsonconfig.h"
#include "apputil.h"
#include "../kscript/json.h"
#include <kl/utf8.h>


wxString JsonNode::toString()
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
    
    wxString result;
    result = towx(retval.getString());
    e.m_eval_params[0] = NULL;
    e.m_eval_params[1] = NULL;
    e.m_eval_params[2] = NULL;
    
    return result;
}

bool JsonNode::fromString(const wxString& str)
{
    m_value.reset();
    
    kscript::ExprParser parser(kscript::optionLanguageECMAScript);
    kscript::Value json_str;
    json_str.setString(towstr(str));
    
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



JsonNode JsonConfig::loadFromFile(const wxString& path)
{
    JsonNode node;
    return node;
}

JsonNode JsonConfig::loadFromDb(tango::IDatabasePtr db, const wxString& path)
{
    JsonNode node;

    std::wstring wval;
    if (!readStreamTextFile(db, towstr(path), wval))
        return node;
    
    node.fromString(wval);
    return node;
}

JsonNode JsonConfig::loadFromString(const wxString& json)
{
    JsonNode node;
    node.fromString(json);
    return node;
}




bool JsonConfig::saveToString(JsonNode& node, wxString& dest)
{
    wxString s = node.toString();
    if (s.Length() > 0)
    {
        dest = s;
        return true; 
    }
    
    return false;
}

bool JsonConfig::saveToDb(JsonNode& node,
                          tango::IDatabasePtr db,
                          const wxString& path,
                          const wxString& mime_type)
{
    wxString text = node.toString();
    
    // add a trailing \n, because some editors complain
    // when this is missing
    text += wxT("\n");
    
    return writeStreamTextFile(db, towstr(path),
                               towstr(text),
                               towstr(mime_type));
}

bool JsonConfig::saveToFile(JsonNode& node, const wxString& path, const wxString& mime_type)
{
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
    
    wxString str;
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

