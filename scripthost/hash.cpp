/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#include "scripthost.h"
#include "hash.h"
#include "memory.h"
#include <kl/md5.h>
#include <kl/crc.h>
#include <kl/utf8.h>


namespace scripthost
{



// (CLASS) Hash
// Category: Encryption
// Description: A class that provides hashing routines.
// Remarks: The Hash class provides basic hashing functionality.

Hash::Hash()
{
}

Hash::~Hash()
{
}

void Hash::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) Hash.crc32
//
// Description: Calculates a crc32 value
//
// Syntax: static function Hash.crc32(value : String) : Number
// Syntax: static function Hash.crc32(value : MemoryBuffer) : Number
//
// Remarks: Calculates the crc32 value of the parameter passed to the
//     function and returns it as a number
//
// Param(value): A string or memory buffer to hash
// Returns: A string containing the md5 hash value of the parameter.  If
//     an error is encountered, null is returned

void Hash::crc32(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
        
    if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"MemoryBuffer"))
        {
            MemoryBuffer* m = (MemoryBuffer*)obj;
            unsigned int crcval = kl::crc32(m->getBuffer(), (int)m->getBufferSize());
            retval->setInteger((int)crcval);
        }
    }
     else
    {
        // crc of string
        std::wstring str = env->getParam(0)->getString();
        size_t len = str.length();

        char* utf8_str = new char[(len+1)*4];
        kl::utf8_wtoutf8(utf8_str, (len+1)*4, str.c_str(), len);
        unsigned int crcval = kl::crc32((unsigned char*)utf8_str, strlen(utf8_str));
        delete[] utf8_str;

        retval->setInteger((int)crcval);
    }
}



// (METHOD) Hash.md5
//
// Description: Calculates an md5 hash value
//
// Syntax: static function Hash.md5(value : String) : String
// Syntax: static function Hash.md5(value : MemoryBuffer) : String
//
// Remarks: Calculates the md5 hash value of the parameter passed to the
//     function and returns it as a string.
//
// Param(value): A string or memory buffer to hash
// Returns: A string containing the md5 hash value of the parameter.  If
//     an error is encountered, null is returned

void Hash::md5(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
        
    if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"MemoryBuffer"))
        {
            MemoryBuffer* m = (MemoryBuffer*)obj;
            kl::md5result_t md5res;
            kl::md5(m->getBuffer(), m->getBufferSize(), &md5res);
            retval->setString(kl::md5resultToString(md5res));
        }
    }
     else
    {
        // md5 string
        std::wstring str = env->getParam(0)->getString();
        retval->setString(kl::md5str(str));
    }
}





// (METHOD) Hash.md5sum
//
// Description: Calculates an md5sum of a file
//
// Syntax: static function Hash.md5sum(path : String) : String
//
// Remarks: Calculates the md5sum of the file specified in the |path| parameter
//
// Param(value): A string or memory buffer to hash
// Returns: A string containing the md5sum of the file specified in the |path|
//     parameter.  If an error is encountered, null is returned

void Hash::md5sum(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
        
    kl::md5result_t res;
    if (!kl::md5sum(env->getParam(0)->getString(), &res))
        return;

    retval->setString(kl::md5resultToString(res));
}



}; // namespace scripthost
