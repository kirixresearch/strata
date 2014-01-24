/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-12-05
 *
 */



#include <xd/xd.h>
#include <kl/json.h>
#include <kl/file.h>
#include "formatdefinition.h"
#include "connectionstr.h"



std::wstring saveDefinitionToString(const xd::FormatDefinition* def)
{
    kl::JsonNode root;
    
    root["object_id"] = def->object_id;
    
    switch (def->object_type)
    {
        default:
        case xd::filetypeTable:            root["object_type"] = "table";         break;
        case xd::filetypeFolder:           root["object_type"] = "folder";        break;
        case xd::filetypeStream:           root["object_type"] = "stream";        break;
    }


    switch (def->format)
    {
        default:
        case xd::formatDefault:            root["format"] = "default";            break;
        case xd::formatXbase:              root["format"] = "xbase";              break;
        case xd::formatDelimitedText:      root["format"] = "delimitedtext";      break;
        case xd::formatFixedLengthText:    root["format"] = "fixedlengthtext";    break;
        case xd::formatText:               root["format"] = "text";               break;
        case xd::formatTypedDelimitedText: root["format"] = "typeddelimitedtext"; break;
    }

    switch (def->encoding)
    {
        default:
        case xd::encodingInvalid:          root["encoding"] = "invalid";          break;
        case xd::encodingUndefined:        root["encoding"] = "undefined";        break;
        case xd::encodingASCII:            root["encoding"] = "ascii";            break;
        case xd::encodingUTF8:             root["encoding"] = "utf8";             break;
        case xd::encodingUTF16:            root["encoding"] = "utf16";            break;
        case xd::encodingUTF32:            root["encoding"] = "utf32";            break;
        case xd::encodingUCS2:             root["encoding"] = "ucs2";             break;
        case xd::encodingUTF16BE:          root["encoding"] = "utf16be";          break;
        case xd::encodingISO8859_1:        root["encoding"] = "iso8859_1";        break;
        case xd::encodingEBCDIC:           root["encoding"] = "ebcdic";           break;
        case xd::encodingCOMP:             root["encoding"] = "comp";             break;
        case xd::encodingCOMP3:            root["encoding"] = "comp3";            break;
    }


    root["data_connection_string"] = xdcommon::encryptConnectionStringPassword(def->data_connection_string);
    root["data_path"] = def->data_path;

    if (def->format == xd::formatDelimitedText)
    {
        kl::JsonNode delimitedtext = root["delimitedtext"];
        delimitedtext["text_qualifiers"] = def->text_qualifiers;
        delimitedtext["delimiters"] = def->delimiters;
        delimitedtext["line_delimiters"] = def->line_delimiters;
        delimitedtext["header_row"].setBoolean(def->first_row_column_names);
    }


    if (def->format == xd::formatFixedLengthText)
    {
        kl::JsonNode fixedlengthtext = root["fixedlengthtext"];
        fixedlengthtext["start_offset"].setInteger(def->fixed_start_offset);
        fixedlengthtext["row_width"].setInteger(def->fixed_row_width);
        fixedlengthtext["line_delimited"].setBoolean(def->fixed_line_delimited);
    }

    return root.toString();
}

bool loadDefinitionFromString(const std::wstring& str, xd::FormatDefinition* def)
{
    kl::JsonNode root;
    if (!root.fromString(str))
        return false;

    // clear out the object
    *def = xd::FormatDefinition();

    // load properties
    def->object_id = root["object_id"];

    std::wstring object_type = root["object_type"];
         if (object_type == L"folder")               def->object_type = xd::filetypeFolder;
    else if (object_type == L"stream")               def->object_type = xd::filetypeStream;
    else if (object_type == L"table")                def->object_type = xd::filetypeTable;
    else                                             def->object_type = xd::filetypeTable;

    std::wstring format_str = root["format"];
         if (format_str == L"default")               def->format = xd::formatDefault;
    else if (format_str == L"xbase")                 def->format = xd::formatXbase;
    else if (format_str == L"delimitedtext")         def->format = xd::formatDelimitedText;
    else if (format_str == L"fixedlengthtext")       def->format = xd::formatFixedLengthText;
    else if (format_str == L"text")                  def->format = xd::formatText;
    else if (format_str == L"typeddelimitedtext")    def->format = xd::formatTypedDelimitedText;
    else                                             def->format = xd::formatDefault;

    std::wstring encoding_str = root["encoding"];
         if (encoding_str == L"invalid")             def->encoding = xd::encodingInvalid;
    else if (encoding_str == L"undefined")           def->encoding = xd::encodingUndefined;
    else if (encoding_str == L"ascii")               def->encoding = xd::encodingASCII;
    else if (encoding_str == L"utf8")                def->encoding = xd::encodingUTF8;
    else if (encoding_str == L"utf16")               def->encoding = xd::encodingUTF16;
    else if (encoding_str == L"utf32")               def->encoding = xd::encodingUTF32;
    else if (encoding_str == L"ucs2")                def->encoding = xd::encodingUCS2;
    else if (encoding_str == L"utf16be")             def->encoding = xd::encodingUTF16BE;
    else if (encoding_str == L"iso8859_1")           def->encoding = xd::encodingISO8859_1;
    else if (encoding_str == L"ebcdic")              def->encoding = xd::encodingEBCDIC;
    else if (encoding_str == L"comp")                def->encoding = xd::encodingCOMP;
    else if (encoding_str == L"comp3")               def->encoding = xd::encodingCOMP3;
    else                                             def->encoding = xd::encodingUndefined;


    def->data_connection_string = xdcommon::decryptConnectionStringPassword(root["data_connection_string"]);
    def->data_path = root["data_path"]; 


    if (root.childExists("delimitedtext"))
    {
        kl::JsonNode delimitedtext = root["delimitedtext"];
        def->text_qualifiers = delimitedtext["text_qualifiers"];
        def->delimiters = delimitedtext["delimiters"];
        def->line_delimiters = delimitedtext["line_delimiters"];
        def->first_row_column_names = delimitedtext["header_row"].getBoolean();
    }

    if (root.childExists("fixedlengthtext"))
    {
        kl::JsonNode fixedlengthtext = root["fixedlengthtext"];
        def->fixed_start_offset = fixedlengthtext["start_offset"].getInteger();
        def->fixed_row_width = fixedlengthtext["row_width"].getInteger();
        def->fixed_line_delimited = fixedlengthtext["line_delimited"].getBoolean();
    }

    
    def->columns.clear();
    if (root.childExists("columns"))
    {
        std::vector<kl::JsonNode> children = root["columns"].getChildren();
        std::vector<kl::JsonNode>::iterator it, it_end = children.end();
        
        for (it = children.begin(); it != it_end; ++it)
        {
            xd::ColumnInfo col;

            col.name = (*it)["name"];
            col.type = xd::stringToDbtype((*it)["type"]);
            col.width = (*it)["width"].getInteger();
            col.scale = (*it)["scale"].getInteger();
            col.source_offset = (*it)["source_offset"].getInteger();
            col.source_width = (*it)["source_width"].getInteger();
            col.source_encoding = xd::stringToDbencoding((*it)["source_encoding"]);

            def->columns.push_back(col);
        }
    }

    return true;
}


bool saveDefinitionToFile(const std::wstring& path, const xd::FormatDefinition* def)
{
    std::wstring str = saveDefinitionToString(def);
    if (str.length() == 0)
        return false;
    return xf_put_file_contents(path, str);
}

bool loadDefinitionFromFile(const std::wstring& path, xd::FormatDefinition* def)
{
    std::wstring str = xf_get_file_contents(path);
    return loadDefinitionFromString(str, def);
}

