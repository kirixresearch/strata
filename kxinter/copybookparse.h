/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-20
 *
 */


#ifndef __KXINTER_COPYBOOKPARSE_H
#define __KXINTER_COPYBOOKPARSE_H


#include "datadef.h"


class CopyBookParser
{

public:

    CopyBookParser(DataFormatDefinition* dfd);
    ~CopyBookParser();

    bool init(const char* filename);
    void parse();

private:
    int parseGroup(int start_line, bool do_line_parse, int loop);
    void parseLine(const char* line, int loop);

private:
    DataFormatDefinition* m_dfd;
    RecordDefinition* m_recdef;
    int m_offset;
    std::vector<std::string> m_lines;
};




#endif


