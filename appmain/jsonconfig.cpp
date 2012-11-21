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
#include <kl/utf8.h>


kl::JsonNode JsonConfig::loadFromFile(const wxString& path)
{
    // TODO: implement
    kl::JsonNode node;
    return node;
}

kl::JsonNode JsonConfig::loadFromDb(tango::IDatabasePtr db, const wxString& path)
{
    kl::JsonNode node;

    std::wstring wval;
    if (!readStreamTextFile(db, towstr(path), wval))
        return node;
    
    node.fromString(wval);
    return node;
}

kl::JsonNode JsonConfig::loadFromString(const wxString& json)
{
    kl::JsonNode node;
    node.fromString(towstr(json));
    return node;
}

bool JsonConfig::saveToString(kl::JsonNode& node, wxString& dest)
{
    wxString s = node.toString();
    if (s.Length() > 0)
    {
        dest = s;
        return true; 
    }
    
    return false;
}

bool JsonConfig::saveToDb(kl::JsonNode& node,
                          tango::IDatabasePtr db,
                          const wxString& path,
                          const wxString& mime_type)
{
    wxString text = towx(node.toString());
    
    // add a trailing \n, because some editors complain
    // when this is missing
    text += wxT("\n");
    
    return writeStreamTextFile(db, towstr(path),
                               towstr(text),
                               towstr(mime_type));
}

bool JsonConfig::saveToFile(kl::JsonNode& node, const wxString& path, const wxString& mime_type)
{
    // TODO: implement
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

