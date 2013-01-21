/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#ifndef __APP_SCRIPTENVIRONMENT_H
#define __APP_SCRIPTENVIRONMENT_H


#include "../kscript/kscript.h"


class Environment : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Environment", Environment)
        KSCRIPT_METHOD("constructor", Environment::constructor)
        KSCRIPT_STATIC_METHOD("getEnvironmentVariable", Environment::getEnvironmentVariable)
        KSCRIPT_STATIC_METHOD("getTempPath", Environment::getTempPath)
        KSCRIPT_STATIC_METHOD("getDocumentsPath", Environment::getDocumentsPath)
        KSCRIPT_STATIC_METHOD("getSystemPath", Environment::getSystemPath)
        KSCRIPT_STATIC_METHOD("getProgramFilesPath", Environment::getProgramFilesPath)
        KSCRIPT_STATIC_METHOD("getDirectorySeparator", Environment::getDirectorySeparator)
    END_KSCRIPT_CLASS()

public:

    Environment();
    ~Environment();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    static void getEnvironmentVariable(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getTempPath(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getDocumentsPath(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getSystemPath(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getProgramFilesPath(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getDirectorySeparator(kscript::ExprEnv* env, void* param, kscript::Value* retval);
};


#endif
