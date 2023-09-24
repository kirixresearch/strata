/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#ifndef H_SCRIPTHOST_SCRIPTSYSTEM_H
#define H_SCRIPTHOST_SCRIPTSYSTEM_H

namespace scripthost
{


class System : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("System", System)
        KSCRIPT_METHOD("constructor", System::constructor)
        KSCRIPT_STATIC_METHOD("beep", System::beep)  // TODO: thread context
        KSCRIPT_STATIC_METHOD("time", System::ztime)
        KSCRIPT_STATIC_METHOD("clock", System::clock)
        KSCRIPT_STATIC_METHOD("getClocksPerSecond", System::getClocksPerSecond)
        KSCRIPT_STATIC_METHOD("sleep", System::sleep) // TODO: thread context
        KSCRIPT_STATIC_METHOD("execute", System::execute) // TODO: thread context
    END_KSCRIPT_CLASS()

public:

    System();
    ~System();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

    static void beep(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void ztime(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void clock(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getClocksPerSecond(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void sleep(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void execute(kscript::ExprEnv* env, void* param, kscript::Value* retval);
};

} // namespace scripthost


#endif

