/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-28
 *
 */


#ifndef __XDCLIENT_JSONUTIL_H
#define __XDCLIENT_JSONUTIL_H


#include "../../kscript/kscript.h"
#include "../../kscript/json.h"
#include "../xdcommon/xdcommon.h"


namespace tango
{


class JsonFileType
{
public:

    JsonFileType::JsonFileType()
    {
    }

    JsonFileType::~JsonFileType()
    {
    }
        
    static std::wstring JsonFileType::toString(int type)
    {
        wchar_t* result = L"";
        switch (type)
        {
            case tango::filetypeFolder:
                result = L"folder";
                break;
                
            case tango::filetypeNode:
                result = L"node";
                break;
                
            case tango::filetypeSet:
                result = L"set";
                break;
                
            case tango::filetypeStream:
                result = L"stream";
                break;
        }
        
        return kl::towstring(result);
    }

    static int JsonFileType::fromString(const std::wstring& type)
    {
        if (type == L"folder")
            return tango::filetypeFolder;
            
        if (type == L"set")
            return tango::filetypeSet;        
            
        if (type == L"node")
            return tango::filetypeNode;
            
        if (type == L"stream")
            return tango::filetypeStream;

        // TODO: what do we do for an invalid file type?        
        return -1;
    }

private:

};


class JsonColumnType
{
public:

    JsonColumnType::JsonColumnType()
    {
    }

    JsonColumnType::~JsonColumnType()
    {
    }

    static std::wstring JsonColumnType::toString(int type)
    {
        wchar_t* result = L"";
        switch (type)
        {
            default:
            case tango::typeUndefined:
                result = L"undefined";
                break;
        
            case tango::typeInvalid:
                result = L"invalid";
                break;
        
            case tango::typeCharacter:
                result = L"character";
                break;
                
            case tango::typeWideCharacter:
                result = L"widecharacter";
                break;
                
            case tango::typeNumeric:
                result = L"numeric";
                break;

            case tango::typeDouble:
                result = L"double";
                break;
                
            case tango::typeInteger:
                result = L"integer";
                break;

            case tango::typeDate:
                result = L"date";
                break;
                
            case tango::typeDateTime:
                result = L"datetime";
                break;

            case tango::typeBoolean:
                result = L"boolean";
                break;
                
            case tango::typeBinary:
                result = L"binary";
                break;
        }
        
        return kl::towstring(result);
    }

    static int JsonColumnType::fromString(const std::wstring& type)
    {
        if (type == L"undefined")
            return tango::typeUndefined;
            
        if (type == L"invalid")
            return tango::typeInvalid;
            
        if (type == L"character")
            return tango::typeCharacter;
            
        if (type == L"widecharacter")
            return tango::typeWideCharacter;

        if (type == L"numeric")
            return tango::typeNumeric;
            
        if (type == L"double")
            return tango::typeDouble;
            
        if (type == L"integer")
            return tango::typeInteger;
            
        if (type == L"date")
            return tango::typeDate;
            
        if (type == L"datetime")
            return tango::typeDateTime;
            
        if (type == L"boolean")
            return tango::typeBoolean;
            
        if (type == L"binary")
            return tango::typeBinary;
            
        return tango::typeInvalid;
    }

private:

};


class JsonStructure
{
public:

    JsonStructure::JsonStructure()
    {
    }

    JsonStructure::~JsonStructure()
    {
    }

    static tango::IStructurePtr JsonStructure::fromJsonNode(kscript::JsonNode& node)
    {
        if (node.isNull())
            return xcm::null;

        Structure* s = new Structure;

        size_t idx;
        size_t count = node["total_count"].getInteger();
        kscript::JsonNode items = node["items"];

        for (idx = 0; idx < count; ++idx)
        {
            kscript::JsonNode item = items[idx];
        
            tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
            col->setName(item["name"]);
            col->setType(JsonColumnType::fromString(item["type"]));
            col->setWidth(item["width"].getInteger());
            col->setScale(item["scale"].getInteger());
            col->setExpression(L"");
            col->setCalculated(false);
            s->addColumn(col);
        }

        return static_cast<tango::IStructure*>(s);
    }

    static kscript::JsonNode JsonStructure::toJsonNode(tango::ISetPtr set)
    {
        kscript::JsonNode root;

        if (set.isNull())
            return root;

        tango::IStructurePtr structure = set->getStructure();
        if (structure.isNull())
            return root;

        // set the total number of items
        int idx;
        int count = structure->getColumnCount();
        root["total_count"].setInteger(count);

        // set the number of rows
        root["row_count"].setInteger((int)set->getRowCount());

        // set the fast row count flag
        bool fast_row_count = false;
        if (set->getSetFlags() & tango::sfFastRowCount)
            fast_row_count = true;

        root["fast_row_count"].setBoolean(fast_row_count);

        // set the items
        kscript::JsonNode items = root["items"];
        for (idx = 0; idx < count; ++idx)
        {
            kscript::JsonNode item = items.appendElement();
            
            tango::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);
            item["name"] = info->getName();
            item["type"] = JsonColumnType::toString(info->getType());
            item["width"].setInteger(info->getWidth());
            item["scale"].setInteger(info->getScale());        
        }

        return root;
    }
    
private:

};


};  // namespace


#endif

