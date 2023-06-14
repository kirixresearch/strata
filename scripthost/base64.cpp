/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2009-10-31
 *
 */


#include "scripthost.h"
#include "base64.h"
#include "memory.h"
#include <kl/base64.h>
#include <kl/utf8.h>
#include <kl/string.h>



namespace scripthost
{


// (CLASS) Base64
// Category: Encryption
// Description: A class that provides base64 routines.
// Remarks: The Base64 class provides basic ability to encode and decode strings using base64.

Base64::Base64()
{
}

Base64::~Base64()
{
}

void Base64::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) Base64.encode
//
// Description: Encodes a string into a base64 string
//
// Syntax: static function Base64.encode(value : String) : String
// Syntax: static function Base64.encode(value : MemoryBuffer) : String
//
// Remarks: Encodes a string into a base64 string.
//
// Param(value): A string or memory buffer to encode
// Returns: The encoded string

void Base64::encode(kscript::ExprEnv* env, void* param, kscript::Value* retval)
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

            char* buf = new char[(m->getBufferSize()+2)*4];
            kl::base64_encodestate state;
            
            kl::base64_init_encodestate(&state);
            int l1 = kl::base64_encode_block((char*)m->getBuffer(), (int)m->getBufferSize(), buf, &state);
            int l2 = kl::base64_encode_blockend(buf+l1, &state);
            buf[l1+l2] = 0;
            retval->setString(std::string(buf));

            retval->setString(buf);
            delete[] buf;
        }
    }
     else
    {
        // string
        std::string str = kl::tostring(env->getParam(0)->getString());
        char* utf8_buf = new char[(str.length()+2)*5];
        size_t utf8_len = 0;
        kl::utf8_wtoutf8(utf8_buf, str.length()*5, env->getParam(0)->getString(), env->getParam(0)->getDataLen(), &utf8_len);
        
        // strip off trailing null if necessary
        if (utf8_len > 0 && utf8_buf[utf8_len-1] == 0)
            utf8_len--;
            
        char* base64_buf = new char[(utf8_len+2)*4];
        
        kl::base64_encodestate state;
        kl::base64_init_encodestate(&state);
        int l1 = kl::base64_encode_block(utf8_buf, (int)utf8_len, base64_buf, &state);
        int l2 = kl::base64_encode_blockend(base64_buf+l1, &state);
        int base64_len = l1+l2;
        
        if (base64_len > 0 && base64_buf[base64_len-1] == '\n')
            base64_len--;
        base64_buf[base64_len] = 0;
        
        retval->setString(base64_buf);

        delete[] utf8_buf;
        delete[] base64_buf;
    }
}

// (METHOD) Base64.decode
//
// Description: Decodes a base64 string into a string
//
// Syntax: static function Base64.decode(str : String) : String
//
// Remarks: Decodes a base64 string into a string.
//
// Param(string): The string to decode
// Returns: The decoded string

void Base64::decode(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
    
    std::string str = kl::tostring(env->getParam(0)->getString());
    char* buf = new char[str.length()+1];
    
    kl::base64_decodestate state;
    kl::base64_init_decodestate(&state);
    int len = kl::base64_decode_block(str.c_str(), (int)str.length(), buf, &state);

    buf[len] = 0;
    
    retval->setString(kl::fromUtf8(buf, len));
    
    delete[] buf;
}



// (METHOD) Base64.decodeToBuffer
//
// Description: Decodes a base64 string into a MemoryBuffer object
//
// Syntax: static function Base64.decodeToBuffer(str : String) : MemoryBuffer
//
// Remarks: Decodes a base64 string into a MemoryBuffer object
//
// Param(string): The string to decode
// Returns: The decoded memory block as a MemoryBuffer object

void Base64::decodeToBuffer(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setNull();
    if (env->getParamCount() < 1)
        return;
    
    MemoryBuffer* mem = MemoryBuffer::createObject(env);
    retval->setObject(mem);
    
    
    std::string str = kl::tostring(env->getParam(0)->getString());
    char* buf = new char[str.length()+1];
    
    kl::base64_decodestate state;
    kl::base64_init_decodestate(&state);
    int len = kl::base64_decode_block(str.c_str(), (int)str.length(), buf, &state);

    buf[len] = 0;
    
    mem->setSizeInternal(len);
    memcpy(mem->getBuffer(), buf, len);
    
    delete[] buf;
}



}; // namespace scripthost
