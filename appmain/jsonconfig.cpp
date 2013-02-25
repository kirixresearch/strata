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

