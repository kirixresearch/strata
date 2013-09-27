/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-20
 *
 */


#ifndef __XDNATIVE_TABLESETMODIFY_H
#define __XDNATIVE_TABLESETMODIFY_H


#include "../xdcommon/structure.h"


struct StructureField
{
    StructureField* oldfld;

    std::wstring name;
    int type;
    int width;
    int scale;
    int pos;
    int offset;

    StructureField()
    {
        oldfld = NULL;

        name = L"";
        type = -1;
        width = -1;
        scale = -1;
        pos = -1;
        offset = -1;
    }
};


class NativeTableModify
{

public:

    NativeTableModify(xd::IDatabasePtr database);

    bool modify(const char* filename,
                std::vector<StructureAction>& actions,
                xd::IJob* job);

private:

    std::vector<StructureField> m_old_fields;
    std::vector<StructureField> m_new_fields;
    xd::IDatabasePtr m_database;
};




#endif

