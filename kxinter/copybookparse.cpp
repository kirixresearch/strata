/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-20
 *
 */


#include "kxinter.h"
#include "copybookparse.h"


CopyBookParser::CopyBookParser(DataFormatDefinition* dfd)
{
    m_dfd = dfd;
}


CopyBookParser::~CopyBookParser()
{
}


bool CopyBookParser::init(const char* filename)
{
    FILE* f = fopen(filename, "rt");
    if (!f)
    {
        return false;
    }

    // -- clear out existing record definitions --

    m_offset = 0;

    if (m_dfd->rec_defs.size() == 1)
    {
        if (m_dfd->rec_defs[0].field_defs.size() == 0)
        {
            m_dfd->rec_defs.clear();
        }
    }

    m_dfd->rec_defs.push_back(RecordDefinition());
    m_recdef = &(m_dfd->rec_defs.back());


    char buf[255];
    char* line = new char[50000];
    bool done = false;

    while (!done)
    {
        *line = 0;

        while (1)
        {
            char* result;
            result = fgets(buf, 254, f);

            if (result == NULL)
            {
                done = true;
                break;
            }

            if (buf[6] == '*')
            {
                // -- comment, skip line --
                continue;
            }

            if (strlen(buf) < 3)
            {
                // -- blank line, skip --
                continue;
            }

            int off;


            // -- check for blank lines --
            bool is_blank = true;
            for (off = 7; off < 60; ++off)
            {
                if (!isspace(buf[off]))
                {
                /*
                    if (!isdigit(buf[off]))
                    {
                        // -- level number was not found, skip line --
                        break;
                    }
                */

                    is_blank = false;
                    break;
                }
            }

            if (is_blank)
            {
                continue;
            }

            off = strlen(buf);

            if (off == 0)
                continue;

            if (off > 71)
            {
                off = 71;
            }
             else
            {
                off--;
            }

            char* p = buf+off;
            while (isspace(*p))
                p--;
            *(p+1) = 0;
            memset(buf, ' ', 7);
            strcat(line, buf);

            if (strlen(line) > 500)
            {
                int i = 5;
            }

            if (*p == '.')
            {
                break;
            }
        }

        m_lines.push_back(line);
    }


    delete[] line;

    fclose(f);

    return true;
}


void CopyBookParser::parse()
{
    int line = -1;

    while (1)
    {
        line = parseGroup(line+1, true, 0);

        if (line == -1)
        {
            break;
        }
    }
}


int CopyBookParser::parseGroup(int start_line, bool do_line_parse, int loop)
{
    int group_level = 0;
    int line = start_line;

    while (line < m_lines.size())
    {
        const char* s = m_lines[line].c_str();


        // -- check level --
        int level = atoi(s);

        if (group_level != 0 && level == 1)
        {
            // -- we've encountered another 1-level group,
            //    so stop processing.  (this may not be the
            //    correct behavior) --
            return -1;
        }


        if (group_level == 0)
            group_level = level;

        if (level < group_level)
        {
            // -- return end line --
            return line - 1;
        }

        if (level > group_level)
        {
            line = parseGroup(line, true, loop);
            if (line == -1)
            {
                return -1;
            }
            line++;
            continue;
        }



        // -- is a picture on this line? --
        const char* pic;
        pic = strstr(s, " PIC ");
        if (pic)
        {
            pic += 5;
        }
         else
        {
            pic = strstr(s, " PICTURE ");
            if (pic)
            {
                pic += 9;
            }
        }

        if (strstr(s, " REDEFINES"))
        {
            if (!pic)
            {
                line = parseGroup(line+1, false, 0);
                if (line == -1)
                {
                    return -1;
                }
                line++;
                continue;
            }
             else
            {
                line++;
                continue;
            }            
        }


        const char* occurs = strstr(s, " OCCURS");
        if (occurs)
        {
            occurs += 7;
            while (isspace(*occurs))
                occurs++;

            int times = atoi(occurs);
            int i;
            int subgroup_endline;

            if (pic)
            {
                for (i = 0; i < times; ++i)
                {
                    if (do_line_parse)
                    {
                        parseLine(s, i+1);
                    }
                }

                line++;
                continue;
            }
             else
            {
                for (i = 0; i < times; ++i)
                {
                    subgroup_endline = parseGroup(line+1, true, i+1);
                }

                line = subgroup_endline+1;
                continue;
            }

        }


        if (do_line_parse)
        {
            parseLine(s, loop);
        }

        line++;
    }

    return -1;
}


void CopyBookParser::parseLine(const char* line, int loop)
{
    std::string holder = line;

    char* l = (char*)holder.c_str();

    char* p = l;
    while (*p)
    {
        *p = toupper(*p);
        p++;
    }

    char field_name[255];
    char picture[255];
    int i;

    if (l[6] == '*')
    {
        // -- comment line --
        return;
    }

    if (strstr(l, "SKIP1") ||
        strstr(l, "SKIP2") ||
        strstr(l, "SKIP3"))
    {
        // -- skip line --
        return;
    }


    // -- find picture --
    char* pic;

    pic = strstr(l, " PIC ");

    if (!pic)
    {
        pic = strstr(l, " PICTURE ");
        if (!pic)
        {
            // -- no picture statement --
            return;
        }
    }
    pic++;


    // -- get field name --

    char* field_start;
    char* field_end;
    char* f = l;

    // -- skip past padding --
    while (isspace(*f))
        f++;
    
    // -- skip past group number --
    while (isdigit(*f))
        f++;

    // -- skip past padding --
    while (isspace(*f))
        f++;

    field_start = f;

    if (f-l > 50)
    {
        // -- blank line --
        return;
    }

    while (!isspace(*f))
    {
        f++;
    }
    
    field_end = f-1;

    memcpy(field_name, field_start, field_end-field_start+1);
    field_name[field_end-field_start+1] = 0;


    // -- get picture --
    while (!isspace(*pic))
        pic++;
    while (isspace(*pic))
        pic++;

    i = 0;
    while (1)
    {
        if (*pic == '.')
            break;

        picture[i] = *pic;
        i++;


        pic++;
    }
    picture[i] = 0;


    int type = ddtypeCharacter;
    int format = ddformatEBCDIC;
    int length = 0;
    int dec = 0;
    bool signed_flag = false;

    bool comp = strstr(picture, " COMP") ? true : false;
    bool comp3 = strstr(picture, " COMP-3") ? true : false;

    if (strstr(picture, "COMPUTATIONAL-3"))
    {
        comp3 = true;
    }

    if (comp3)
    {
        comp = false;
    }


    // -- we now have field name and the picture --


    switch (picture[0])
    {
        case 'S':
            signed_flag = true;

        case '9':
            type = ddtypeNumeric;

            if (comp3)
            {
                format = ddformatComp3Numeric;
            }
             else if (comp)
            {
                format = ddformatCompNumeric;
            }

            break;

        case 'X':
        default:
            type = ddtypeCharacter;
            break;
    }

    char* paren = strchr(picture, '(');
    char* dec_marker = strchr(picture, 'V');

    if (dec_marker)
    {
        // this is not quite working for character fields:
        // PIC X(01) VALUE SPACES yielded 4 decimal scale
        
        if (paren > dec_marker)
            paren = NULL;
    }

    if (paren)
    {
        // -- we have parentheses --
        length = atoi(paren+1);
    }
     else
    {
        // -- just count the length --
        i = 0;
        while (picture[i] && !(isspace(picture[i]) || picture[i] == '.' || toupper(picture[i]) == 'V'))
            i++;

        length = i;

        if (toupper(picture[0]) == 'S')
            length--;
    }


    if (dec_marker)
    {
        paren = strchr(dec_marker, '(');
        if (paren)
        {
            dec = atoi(paren+1);
        }
         else
        {
            dec_marker++;
            dec = 0;
            while (*dec_marker && !(isspace(*dec_marker) || *dec_marker == '.'))
            {
                dec_marker++;
                dec++;
            }
        }
    }

    if (comp)
    {
        int new_len;
        if (length >= 0 && length <= 4)
        {
            new_len = 2;
        }
         else if (length > 4 && length <= 8)
        {
            new_len = 4;
        }
         else
        {
            new_len = 8;
        }

        length = new_len;
    }

    if (comp3)
    {
        length = ((length+dec)/2)+1;
    }


    wxString fname = wxString::From8BitData(field_name);
    fname.Replace(wxT("-"), wxT("_"));

    if (loop > 0)
    {
        fname += wxString::Format(wxT("_%d"), loop);
    }



    // -- find simple date types --
    if  (type == ddtypeNumeric &&
         (fname.Find(wxT("_DATE")) != -1 ||
          fname.Find(wxT("DATE_")) != -1 ||
          fname.Find(wxT("_DT")) != -1 ||
          fname.Find(wxT("YYMMDD")) != -1))
    {
        if (comp3)
        {
            type = ddtypeDateCCYYMMDD;
        }
         else
        {
            if (length == 8)
            {
                type = ddtypeDateCCYYMMDD;
            }
             else if (length == 6)
            {
                type = ddtypeDateYYMMDD;
            }
        }
    }


    FieldDefinition fd;
    fd.name = fname;
    fd.offset = m_offset;
    fd.length = length;
    fd.format = format;
    fd.type = type;
    fd.scale = dec;
    fd.signed_flag = signed_flag;
    m_recdef->field_defs.push_back(fd);

    m_offset += length;
}



