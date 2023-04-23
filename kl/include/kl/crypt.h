/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2007-02-28
 *
 */


#ifndef H_KL_CRYPT_H
#define H_KL_CRYPT_H


#include <string>


namespace kl
{


std::wstring encryptString(const std::wstring& source_string, const std::wstring& key);
std::wstring decryptString(const std::wstring& encrypted_string, const std::wstring& key);
bool isEncryptedString(const std::wstring& encrypted_string);

// please note that, in encryptBuffer, the output buffer
// size must be 16 bytes greater in size than the input_buffer
// the total size of the encrypted data is placed in the value
// pointed by |output_buffer_size|
bool encryptBuffer(const unsigned char* input_buf,
                   size_t size,
                   unsigned char* output_buf,
                   size_t* output_buffer_size,
                   const std::wstring& key);

// please note that, in decryptBuffer, the output buffer
// size must be at least as big as the input_buffer
// the total size of the decrypted data is placed in the value
// pointed by |output_buffer_size|
bool decryptBuffer(const unsigned char* input_buf,
                   size_t size,
                   unsigned char* output_buf,
                   size_t* output_buffer_size,
                   const std::wstring& key);
          
};



#endif

