/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-05-01
 *
 */


#include "appmain.h"
#include "jsonconfig.h"
#include <kl/utf8.h>


kl::JsonNode JsonConfig::loadFromDb(tango::IDatabasePtr db, const std::wstring& path)
{
    kl::JsonNode node;

    std::wstring wval;
    if (!readStreamTextFile(db, path, wval))
        return node;
    
    node.fromString(wval);
    return node;
}

kl::JsonNode loadFromFile(kl::JsonNode& node, const std::wstring& path)
{

    bool success = false;
    std::wstring wval = xf_get_file_contents(path, &success);
    if (!success)
        return node;

    node.fromString(wval);
    return node;
}


kl::JsonNode JsonConfig::loadFromString(const std::wstring& json)
{
    kl::JsonNode node;
    node.fromString(json);
    return node;
}

bool JsonConfig::saveToString(kl::JsonNode& node, std::wstring& dest)
{
    std::wstring s = node.toString();
    if (s.length() > 0)
    {
        dest = s;
        return true; 
    }
    
    return false;
}

bool JsonConfig::saveToDb(kl::JsonNode& node,
                          tango::IDatabasePtr db,
                          const std::wstring& path,
                          const std::wstring& mime_type)
{
    std::wstring text = node.toString();
    
    // add a trailing \n, because some editors complain
    // when this is missing
    text += L"\n";
    
    return writeStreamTextFile(db, path, text, mime_type);
}


bool JsonConfig::saveToFile(kl::JsonNode& node, const std::wstring& path)
{
    std::wstring text = node.toString();
    
    // add a trailing \n, because some editors complain
    // when this is missing
    text += L"\n";
    
    return xf_put_file_contents(path, text);
}
