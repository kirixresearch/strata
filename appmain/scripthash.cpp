/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#include "appmain.h"
#include "scripthost.h"
#include "scripthash.h"
#include "scriptmemory.h"
#include <kl/md5.h>


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
