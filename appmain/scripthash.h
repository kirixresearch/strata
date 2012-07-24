/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#ifndef __APP_SCRIPTHASH_H
#define __APP_SCRIPTHASH_H


class Hash : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Hash", Hash)
        KSCRIPT_METHOD("constructor", Hash::constructor)
        KSCRIPT_STATIC_METHOD("md5", Hash::md5)
    END_KSCRIPT_CLASS()

public:

    Hash();
    ~Hash();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

    static void md5(kscript::ExprEnv* env, void* param, kscript::Value* retval);
};


#endif
