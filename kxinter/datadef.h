/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-21
 *
 */


#ifndef __KXINTER_DATADEF_H
#define __KXINTER_DATADEF_H


// -- data formats and types --

enum
{
    ddformatASCII = 0,
    ddformatEBCDIC = 1,
    ddformatCompNumeric = 2,
    ddformatComp3Numeric = 3
};


enum
{
    ddtypeCharacter = 0,
    ddtypeDecimal = 1,
    ddtypeNumeric = 2,
    ddtypeDateYYMMDD = 3,
    ddtypeDateMMDDYY = 4,
    ddtypeDateCCYYMMDD = 5,
    ddtypeDateCCYY_MM_DD = 6
};


enum
{
    dfdFixed = 0,
    dfdVariable = 1,
    dfdDelimited = 2
};

struct FieldDefinition
{
    wxString name;
    int format;
    int type;
    int offset;
    int length;
    int scale;
    bool signed_flag;

    FieldDefinition()
    {
        name = wxT("");
        format = 0;
        type = 0;
        offset = 0;
        length = 0;
        scale = 0;
        signed_flag = false;
    }

    FieldDefinition(const FieldDefinition& c)
    {
        name = c.name;
        format = c.format;
        type = c.type;
        offset = c.offset;
        length = c.length;
        scale = c.scale;
        signed_flag = c.signed_flag;
    }
};


struct RecordDefinition
{
    wxString name;
    int id_offset;              // offset of id byte, -1 if none
    unsigned char id_value;     // byte value of type
    int record_length;          // record length

    std::vector<FieldDefinition> field_defs;

    RecordDefinition()
    {
        name = wxT("");
        id_offset = 0;
        id_value = 0;
        record_length = 0;
    }

    RecordDefinition(const RecordDefinition& c)
    {
       name = c.name;
       id_offset = c.id_offset;
       id_value = c.id_value;
       record_length = c.record_length;
       field_defs = c.field_defs;
    }
};


class DataFormatDefinition
{
public:

    DataFormatDefinition();
    DataFormatDefinition(const DataFormatDefinition& c);

    bool save(const wxString& filename);
    bool load(const wxString& filename);

public:

    int type;                   // either dfdFixed, dfdVariable, dfdDelimited
    int record_length;          // record length (used only if fixed length)
    int skip_bytes;             // byte to skip from the beginning
    char line_delimiter[64];
    int line_delimiter_length;
    std::vector<RecordDefinition> rec_defs;
};



#endif

