/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-08-29
 *
 */


#ifndef __APP_COMMANDMGR_H
#define __APP_COMMANDMGR_H


class CommandInfo
{
public:
    enum
    {
        Default = 0x0000,
        Hidden =  0x0001
    };
    
public:

    int id;
    std::wstring category;
    std::wstring tag;
};

class CommandMgr
{
public:

    static int lookupCommandId(const std::wstring& tag);
    static int defineCommand(const std::wstring& tag,
                             int id = -1,
                             int flags = CommandInfo::Default);
};


#endif

