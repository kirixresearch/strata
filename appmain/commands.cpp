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
#include "commandmgr.h"

#ifdef BEGIN_COMMANDS
#undef BEGIN_COMMANDS
#undef END_COMMANDS
#undef DEFINE_COMMAND
#undef DEFINE_COMMAND_HIDDEN
#endif
#define BEGIN_COMMANDS(definefunc, startid) void definefunc() {
#define END_COMMANDS() }
#define DEFINE_COMMAND(tag, id) CommandMgr::defineCommand(L##tag, id);
#define DEFINE_COMMAND_HIDDEN(tag, id) CommandMgr::defineCommand(L##tag, id, CommandInfo::Hidden);

#include "commands.h"

