/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-11-19
 *
 */


#include "kxinter.h"
#include "datadef.h"


DataFormatDefinition::DataFormatDefinition()
{
    type = dfdFixed;
    record_length = 0;
    skip_bytes = 0;
    line_delimiter_length = 0;
    memset(line_delimiter, 0, 64);
}

DataFormatDefinition::DataFormatDefinition(const DataFormatDefinition& c)
{
    type = c.type;
    record_length = c.record_length;
    skip_bytes = c.skip_bytes;
    rec_defs = c.rec_defs;
    line_delimiter_length = c.line_delimiter_length;
    memset(line_delimiter, 0, 64);
    memcpy(line_delimiter, c.line_delimiter, line_delimiter_length);
}


static void fieldDefToNode(kl::xmlnode& node, FieldDefinition& fielddef)
{
    std::wstring format;
    std::wstring type;

    switch (fielddef.format)
    {
        case ddformatASCII:         format = L"ascii";   break;
        case ddformatEBCDIC:        format = L"ebcdic";  break;
        case ddformatCompNumeric:   format = L"comp";    break;
        case ddformatComp3Numeric:  format = L"comp3";   break;
    }

    switch (fielddef.type)
    {
        case ddtypeCharacter:       type = L"character";       break;
        case ddtypeDecimal:         type = L"decimal";         break;
        case ddtypeNumeric:         type = L"numeric";         break;
        case ddtypeDateYYMMDD:      type = L"date_yymmdd";     break;
        case ddtypeDateMMDDYY:      type = L"date_mmddyy";     break;
        case ddtypeDateCCYYMMDD:    type = L"date_ccyymmdd";   break;
        case ddtypeDateCCYY_MM_DD:  type = L"date_ccyy_mm_dd"; break;
    }


    node.addChild(L"name", towstr(fielddef.name));
    node.addChild(L"format", format);
    node.addChild(L"type", type);
    node.addChild(L"offset", fielddef.offset);
    node.addChild(L"length", fielddef.length);
    node.addChild(L"scale", fielddef.scale);
    node.addChild(L"signed_flag", fielddef.signed_flag ? 1 : 0);
}

static void recDefToNode(kl::xmlnode& node, RecordDefinition& recdef)
{
    node.addChild(L"name", towstr(recdef.name));
    node.addChild(L"id_offset", recdef.id_offset);
    node.addChild(L"id_value", (int)recdef.id_value);
    node.addChild(L"record_length", recdef.record_length);

    kl::xmlnode& fields_node = node.addChild();
    fields_node.setNodeName(L"fields");

    std::vector<FieldDefinition>::iterator it;
    for (it = recdef.field_defs.begin(); it != recdef.field_defs.end(); ++it)
    {
        kl::xmlnode& field_node = fields_node.addChild();
        field_node.setNodeName(L"field");
        fieldDefToNode(field_node, *it);
    }
}

static void nodeToFieldDef(kl::xmlnode& node, FieldDefinition& fielddef)
{
    std::wstring format_str = node.getChild(L"format").getContents();
    std::wstring type_str = node.getChild(L"type").getContents();

    // -- defaults --
    fielddef.format = ddformatASCII;
    fielddef.type = ddtypeCharacter;
    fielddef.offset = 0;
    fielddef.length = 0;
    fielddef.scale = 0;
    fielddef.signed_flag = false;


    // -- field format --

    if (!wcscasecmp(format_str.c_str(), L"ascii"))
    {
        fielddef.format = ddformatASCII;
    }
     else if (!wcscasecmp(format_str.c_str(), L"ebcdic"))
    {
        fielddef.format = ddformatEBCDIC;
    }
     else if (!wcscasecmp(format_str.c_str(), L"comp"))
    {
        fielddef.format = ddformatCompNumeric;
    }
     else if (!wcscasecmp(format_str.c_str(), L"comp3"))
    {
        fielddef.format = ddformatComp3Numeric;
    }


    // -- field type --

    if (!wcscasecmp(type_str.c_str(), L"character"))
    {
        fielddef.type = ddtypeCharacter;
    }
     else if (!wcscasecmp(type_str.c_str(), L"decimal"))
    {
        fielddef.type = ddtypeDecimal;
    }
     else if (!wcscasecmp(type_str.c_str(), L"numeric"))
    {
        fielddef.type = ddtypeNumeric;
    }
     else if (!wcscasecmp(type_str.c_str(), L"date_yymmdd"))
    {
        fielddef.type = ddtypeDateYYMMDD;
    }
     else if (!wcscasecmp(type_str.c_str(), L"date_mmddyy"))
    {
        fielddef.type = ddtypeDateMMDDYY;
    }
     else if (!wcscasecmp(type_str.c_str(), L"date_ccyymmdd"))
    {
        fielddef.type = ddtypeDateCCYYMMDD;
    }
     else if (!wcscasecmp(type_str.c_str(), L"date_ccyy_mm_dd"))
    {
        fielddef.type = ddtypeDateCCYY_MM_DD;
    }

    fielddef.name = towx(node.getChild(L"name").getContents());
    fielddef.offset = kl::wtoi(node.getChild(L"offset").getContents());
    fielddef.length = kl::wtoi(node.getChild(L"length").getContents());
    fielddef.scale = kl::wtoi(node.getChild(L"scale").getContents());
    fielddef.signed_flag = (node.getChild(L"signed_flag").getContents() == L"0") ? false : true;
}


static void nodeToRecDef(kl::xmlnode& node, RecordDefinition& recdef)
{
    recdef.name = towx(node.getChild(L"name").getContents());
    recdef.id_offset = kl::wtoi(node.getChild(L"id_offset").getContents());
    recdef.id_value = kl::wtoi(node.getChild(L"id_value").getContents());
    recdef.record_length = kl::wtoi(node.getChild(L"record_length").getContents());

    kl::xmlnode& fields_node = node.getChild(L"fields");
    if (fields_node.isEmpty())
        return;

    unsigned int child_count = fields_node.getChildCount();
    unsigned int i;

    for (i = 0; i < child_count; ++i)
    {
        kl::xmlnode& field_node = fields_node.getChild(i);
        FieldDefinition f;
        nodeToFieldDef(field_node, f);
        recdef.field_defs.push_back(f);
    }
}

bool DataFormatDefinition::save(const wxString& filename)
{
    kl::xmlnode root;
    std::wstring type;

    switch (this->type)
    {
        case dfdFixed:     type = L"fixed";       break;
        case dfdVariable:  type = L"variable";    break;
        case dfdDelimited: type = L"delimited";   break;
    }
 
    root.setNodeName(L"data_format_definition");
    root.addChild(L"version", 1);
    root.addChild(L"type", type);
    root.addChild(L"record_length", this->record_length);
    root.addChild(L"skip_bytes", this->skip_bytes);

    std::wstring delimiter;
    int i;
    for (i = 0; i < this->line_delimiter_length; ++i)
    {
        wchar_t buf[255];
        swprintf(buf, 255, L"%02X", this->line_delimiter[i]);
        delimiter += buf;

        if (i+1 < this->line_delimiter_length)
            delimiter += L" ";
    }

    root.addChild(L"line_delimiter", delimiter);


    kl::xmlnode& recdefs_node = root.addChild();
    recdefs_node.setNodeName(L"record_types");

    std::vector<RecordDefinition>::iterator it;
    for (it = this->rec_defs.begin(); it != this->rec_defs.end(); ++it)
    {
        kl::xmlnode& recdef_node = recdefs_node.addChild();
        recdef_node.setNodeName(L"record_type");

        recDefToNode(recdef_node, *it);
    }

    return root.save(towstr(filename));
}





bool DataFormatDefinition::load(const wxString& filename)
{
    kl::xmlnode root;
    
    if (!root.load(towstr(filename)))
    {
        return false;
    }


    // -- defaults --
    this->type = dfdFixed;
    this->record_length = 1000;
    memset(this->line_delimiter, 0, 64);
    this->line_delimiter_length = 0;


    kl::xmlnode& version_node = root.getChild(L"version");
    if (version_node.isEmpty())
    {
        return false;
    }

    if (kl::wtoi(version_node.getContents()) != 1)
    {
        return false;
    }



    kl::xmlnode& type_node = root.getChild(L"type");
    if (!type_node.isEmpty())
    {
        std::wstring type_str = type_node.getContents();
        if (!wcscasecmp(type_str.c_str(), L"fixed"))
        {
            this->type = dfdFixed;
        }
         else if (!wcscasecmp(type_str.c_str(), L"variable"))
        {
            this->type = dfdVariable;
        }
         else if (!wcscasecmp(type_str.c_str(), L"delimited"))
        {
            this->type = dfdDelimited;
        }
    }


    kl::xmlnode& record_length_node = root.getChild(L"record_length");
    if (!record_length_node.isEmpty())
    {
        this->record_length = kl::wtoi(record_length_node.getContents());
    }


    kl::xmlnode& skip_bytes_node = root.getChild(L"skip_bytes");
    if (!skip_bytes_node.isEmpty())
    {
        this->skip_bytes = kl::wtoi(skip_bytes_node.getContents());
    }


    kl::xmlnode& rectypes_node = root.getChild(L"record_types");
    if (rectypes_node.isEmpty())
    {
        return false;
    }

    this->rec_defs.clear();
    
    unsigned int child_count = rectypes_node.getChildCount();
    unsigned int i;
    for (i = 0; i < child_count; ++i)
    {
        kl::xmlnode& rectype_node = rectypes_node.getChild(i);
        RecordDefinition r;
        nodeToRecDef(rectype_node, r);
        this->rec_defs.push_back(r);
    }


    memset(this->line_delimiter, 0, 64);
    this->line_delimiter_length = 0;


    // -- load line delimiter --
    kl::xmlnode& line_delimiter_node = root.getChild(L"line_delimiter");
    if (!line_delimiter_node.isEmpty())
    {
        static const wchar_t* hexchars = L"0123456789ABCDEF";

        unsigned char b;

        std::wstring line_delim = line_delimiter_node.getContents();
        const wchar_t* p = line_delim.c_str();

        while (*p)
        {
            b = 0;

            const wchar_t* v;
            
            v = wcschr(hexchars, towupper(*p));
            if (v)
            {
                b += ((v-hexchars)*16);
            }

            p++;
            
            if (!*p)
            {
                break;
            }

            v = wcschr(hexchars, towupper(*p));
            if (v)
            {
                b += (v-hexchars);
            }

            p++;

            while (iswspace(*p))
                p++;

            this->line_delimiter[this->line_delimiter_length] = b;
            this->line_delimiter_length++;
        }

    }

    return true;
}

