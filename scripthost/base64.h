/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2009-10-31
 *
 */


#ifndef H_SCRIPTHOST_SCRIPTBASE64_H
#define H_SCRIPTHOST_SCRIPTBASE64_H

namespace scripthost
{


class Base64 : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Base64", Base64)
        KSCRIPT_METHOD("constructor", Base64::constructor)
        KSCRIPT_STATIC_METHOD("encode", Base64::encode)
        KSCRIPT_STATIC_METHOD("decode", Base64::decode)
        KSCRIPT_STATIC_METHOD("decodeToBuffer", Base64::decodeToBuffer)
    END_KSCRIPT_CLASS()

public:

    Base64();
    ~Base64();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

    static void encode(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void decode(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void decodeToBuffer(kscript::ExprEnv* env, void* param, kscript::Value* retval);
};



} // namespace scripthost

#endif
