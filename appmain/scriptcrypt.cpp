/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#include "appmain.h"
#include "scripthost.h"
#include "scriptcrypt.h"
#include <kl/crypt.h>


// (CLASS) SymmetricCrypt
// Category: Encryption
// Description: A class that provides symmetric encryption routines.
// Remarks: The SymmetricCrypt class provides basic symmetric encryption and decryption
//     functionality.

SymmetricCrypt::SymmetricCrypt()
{
}

SymmetricCrypt::~SymmetricCrypt()
{
}


void SymmetricCrypt::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) SymmetricCrypt.encryptString
//
// Description: Encrypts a string
//
// Syntax: static function SymmetricCrypt.encryptString(str : String, 
//                                                      enckey : String) : String
//
// Remarks: Encrypts the string specified in the |str| parameter using
//     the value specified in |enckey| as the encryption key.  Both parameters
//     should be strings.  The input string may contain unicode characters.  
//     The resulting string is composed solely of ASCII characters, which makes
//     it convenient for storage and transmission.
//
// Returns: A salted, encrypted version of the string specified
//     by the |str| parameter

void SymmetricCrypt::encryptString(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    std::wstring str;
    std::wstring key;
    std::wstring result;
    
    if (env->getParamCount() >= 1)
        str = env->getParam(0)->getString();
    if (env->getParamCount() >= 2)
        key = env->getParam(1)->getString();
        
    result = L"J"; // encryption version marker
    result += kl::encryptString(str, key);
    
    retval->setString(result);
}


// (METHOD) SymmetricCrypt.decryptString
//
// Description: Decrypts a string
//
// Syntax: static function SymmetricCrypt.encryptString(encstr : String, 
//                                                      enckey : String) : String
//
// Remarks: Decrypts a string which was encrypted with the
//     SymmetricCrypt.encryptString method.  In order to achieve successful decryption,
//     the encryption key must be the same one used during encryption.
//
// Returns: The decrypted string.  If an error was encountered, an empty string
//     is returned.

void SymmetricCrypt::decryptString(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    std::wstring str;
    std::wstring key;
    std::wstring result;
    
    if (env->getParamCount() >= 1)
        str = env->getParam(0)->getString();
    if (env->getParamCount() >= 2)
        key = env->getParam(1)->getString();

    if (str.length() == 0 || str[0] != L'J')
    {
        retval->setString(L"");
        return;
    }
    
    // remove our version marker character
    str.erase(0,1);
    
    if (str.length() == 0)
    {
        retval->setString(L"");
        return;
    }
    
    result = kl::decryptString(str, key);
    retval->setString(result);
}

