/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#ifndef __SCRIPTHOST_SCRIPTHASH_H
#define __SCRIPTHOST_SCRIPTHASH_H


namespace scripthost
{


class Hash : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Hash", Hash)
        KSCRIPT_METHOD("constructor", Hash::constructor)
        KSCRIPT_STATIC_METHOD("crc32", Hash::crc32)
        KSCRIPT_STATIC_METHOD("md5", Hash::md5)
        KSCRIPT_STATIC_METHOD("md5sum", Hash::md5sum)
    END_KSCRIPT_CLASS()

public:

    Hash();
    ~Hash();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

    static void crc32(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void md5(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void md5sum(kscript::ExprEnv* env, void* param, kscript::Value* retval);
};


}; // namespace scripthost


#endif
