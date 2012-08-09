/*!
 *
 * Copyright (c) 2008-2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Aaron L. Williams
 * Created:  2012-08-09
 *
 */


#include "kl/json.h"
#include "kl/string.h"


namespace kl
{


JsonNode::JsonNode()
{
}

JsonNode::~JsonNode()
{
}

JsonNode::JsonNode(const JsonNode& _c)
{
    // TODO: fill out
}

JsonNode& JsonNode::operator=(const JsonNode& _c)
{
    // TODO: fill out
    return *this;
}

JsonNode& JsonNode::operator=(const std::wstring& str)
{
    setString(str);
    return *this;
}

JsonNode& JsonNode::operator=(int i)
{
    setInteger(i);
    return *this;
}

JsonNode& JsonNode::operator=(double d)
{
    setDouble(d);
    return *this;
}

JsonNode JsonNode::operator[](int i)
{
    // TODO: implement differently
    char buf[20];
    sprintf(buf, "%d", i);
    return getChild(kl::towstring(buf));
}

JsonNode JsonNode::operator[](const char* str)
{
    return (*this)[kl::towstring(str)];
}

JsonNode JsonNode::operator[](const std::wstring& str)
{
    return getChild(str);
}

JsonNode JsonNode::appendElement()
{
    // TODO: should return appended element
    return (*this)[0];
}

JsonNode JsonNode::getChild(const std::wstring& _str)
{
    // TODO: fill out
    JsonNode result;
    return result;
}

size_t JsonNode::getCount()
{
    // TODO: fill out
    return 0;
}

std::vector<std::wstring> JsonNode::getChildKeys()
{
    // TODO: fill out
    size_t i, count = 0;
    
    std::vector<std::wstring> keys;
    keys.reserve(count);

    for (i = 0; i < count; ++i)
    {
    }

    return keys;
}

void JsonNode::setString(const std::wstring& str)
{
    // TODO: fill out
}

void JsonNode::setBoolean(bool b)
{
    // TODO: fill out
}

void JsonNode::setDouble(double num)
{
    // TODO: fill out
}

void JsonNode::setInteger(int num)
{
    // TODO: fill out
}

std::wstring JsonNode::getString()
{
    // TODO: fill out
    std::wstring result;
    return result;
}

bool JsonNode::getBoolean()
{
    // TODO: fill out
    return false;
}

double JsonNode::getDouble()
{
    // TODO: fill out
    return 0.0f;
}

int JsonNode::getInteger()
{
    // TODO: fill out
    return 0;
}

bool JsonNode::isNull()
{
    // TODO: fill out
    return false;
}

bool JsonNode::isOk()
{
    // TODO: fill out
    return false;
}

JsonNode::operator std::wstring()
{
    // TODO: fill out
    return getString();
}

std::wstring JsonNode::toString()
{
    // TODO: fill out
    std::wstring result;
    return result;
}

bool JsonNode::fromString(const std::wstring& str)
{
    // TODO: fill out
    return false;
}

bool JsonNode::parse(wchar_t* expr, wchar_t** endloc)
{
    // TODO: fill out
    return false;
}

std::wstring JsonNode::stringify()
{
    // TODO: fill out
    std::wstring result;
    return result;
}


};  // namespace kl

