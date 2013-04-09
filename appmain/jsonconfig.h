/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-05-01
 *
 */


#ifndef __APP_JSONCONFIG_H
#define __APP_JSONCONFIG_H


class JsonConfig
{
public:

    // kl::JsonNode implementation
    static kl::JsonNode loadFromDb(tango::IDatabasePtr db, const wxString& path);
    static kl::JsonNode loadFromString(const wxString& json);
    static bool loadFromFile(kl::JsonNode& node, const std::wstring& path); 

    static bool saveToString(kl::JsonNode& node, wxString& dest);
    
    static bool saveToDb(kl::JsonNode& node,
                         tango::IDatabasePtr db,
                         const wxString& path,
                         const wxString& mime_type = wxT("text/plain"));

    static bool saveToFile(kl::JsonNode& node, const std::wstring& path);   
};


#endif

