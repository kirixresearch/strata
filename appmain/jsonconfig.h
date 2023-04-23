/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-05-01
 *
 */


#ifndef H_APP_JSONCONFIG_H
#define H_APP_JSONCONFIG_H


class JsonConfig
{
public:

    // kl::JsonNode implementation
    static kl::JsonNode loadFromDb(xd::IDatabasePtr db, const std::wstring& path);
    static kl::JsonNode loadFromString(const std::wstring& json);
    static kl::JsonNode loadFromFile(const std::wstring& path); 

    static bool saveToString(kl::JsonNode& node, std::wstring& dest);
    
    static bool saveToDb(kl::JsonNode& node,
                         xd::IDatabasePtr db,
                         const std::wstring& path,
                         const std::wstring& mime_type = wxT("text/plain"));

    static bool saveToFile(kl::JsonNode& node, const std::wstring& path);   
};


#endif

