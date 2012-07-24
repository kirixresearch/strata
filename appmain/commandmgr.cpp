/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-08-29
 *
 */


#include "appmain.h"


std::map<std::wstring, CommandInfo> g_commands;
std::map<int, std::wstring> g_reverse_commands;


// static
int CommandMgr::defineCommand(const std::wstring& tag, int id, int flags)
{
    CommandInfo info;
    info.tag = tag;
    info.id = id;
    //info.flags = flags;
    g_commands[tag] = info;
    g_reverse_commands[info.id] = tag;
    
    return info.id;
}


// static
int CommandMgr::lookupCommandId(const std::wstring& tag)
{
    std::map<std::wstring, CommandInfo>::iterator it;
    
    it = g_commands.find(tag);
    if (it == g_commands.end())
        return -1;
        
    return it->second.id;
}
