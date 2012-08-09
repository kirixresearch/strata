/*!
 *
 * Copyright (c) 2008-2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Aaron L. Williams
 * Created:  2012-08-09
 *
 */


#ifndef __KL_JSON_H
#define __KL_JSON_H


#include <string>
#include <vector>


namespace kl
{


class JsonNode
{
public:

    JsonNode();
    ~JsonNode();

    JsonNode(const JsonNode& _c);

    JsonNode& operator=(const JsonNode& _c);
    JsonNode& operator=(const std::wstring& str);
    JsonNode& operator=(int i);
    JsonNode& operator=(double d);
    JsonNode operator[](int i);
    JsonNode operator[](const char* str);
    JsonNode operator[](const std::wstring& str);

    JsonNode appendElement();
    JsonNode getChild(const std::wstring& _str);

    size_t getCount();
    std::vector<std::wstring> getChildKeys();

    void setString(const std::wstring& str);
    void setBoolean(bool b);
    void setDouble(double num);
    void setInteger(int num);

    std::wstring getString();
    bool getBoolean();
    double getDouble();
    int getInteger();

    bool isNull();
    bool isOk();
    
    operator std::wstring();
    std::wstring toString();

    bool fromString(const std::wstring& str);

private:

    bool parse(wchar_t* expr, wchar_t** endloc);
    std::wstring stringify();

};


};  // namespace kl


#endif

