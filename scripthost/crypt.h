/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#ifndef __SCRIPTHOST_SCRIPTCRYPT_H
#define __SCRIPTHOST_SCRIPTCRYPT_H

namespace scripthost
{


class SymmetricCrypt : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("SymmetricCrypt", SymmetricCrypt)
        KSCRIPT_METHOD("constructor", SymmetricCrypt::constructor)
        KSCRIPT_STATIC_METHOD("encryptString", SymmetricCrypt::encryptString)
        KSCRIPT_STATIC_METHOD("decryptString", SymmetricCrypt::decryptString)
    END_KSCRIPT_CLASS()

public:

    SymmetricCrypt();
    ~SymmetricCrypt();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

    static void encryptString(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void decryptString(kscript::ExprEnv* env, void* param, kscript::Value* retval);
};


}; // namespace scripthost


#endif
