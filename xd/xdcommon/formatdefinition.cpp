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




std::wstring saveDefinitionToString(const xd::FormatInfo* def)
{
    kl::JsonNode root;
    
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


    root["data_connection_string"] = def->data_connection_string;
    root["data_file"] = def->data_file;

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

bool loadDefinitionFromString(const std::wstring& str, xd::FormatInfo* def)
{
    kl::JsonNode root;
    if (!root.fromString(str))
        return false;

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
    else     

    if (root.childExists("delimitedtext"))
    {
        kl::JsonNode delimitedtext = root["delimitedtext"];
        def->text_qualifiers = delimitedtext["text_qualifiers"];
        def->delimiters = delimitedtext["delimiters"];
        def->line_delimiters = delimitedtext["line_delimiters"];
        def->first_row_column_names = delimitedtext["header_row"].getBoolean();;
    }

    if (root.childExists("fixedlengthtext"))
    {
        kl::JsonNode fixedlengthtext = root["fixedlengthtext"];
        def->fixed_start_offset = fixedlengthtext["start_offset"].getInteger();
        def->fixed_row_width = fixedlengthtext["row_width"].getInteger();
        def->fixed_line_delimited = fixedlengthtext["line_delimited"].getBoolean();
    }

    return true;
}


bool saveDefinitionToFile(const std::wstring& path, const xd::FormatInfo* def)
{
    std::wstring str = saveDefinitionToString(def);
    if (str.length() == 0)
        return false;
    return xf_put_file_contents(path, str);
}

bool loadDefinitionFromFile(const std::wstring& path, xd::FormatInfo* def)
{
    std::wstring str = xf_get_file_contents(path);;
    return loadDefinitionFromString(str, def);
}

