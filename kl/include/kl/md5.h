/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-24
 *
 */


#ifndef __KL_MD5_H
#define __KL_MD5_H


#include <string>


namespace kl
{


struct md5result_t
{
    unsigned char buf[16];
};

void md5(const unsigned char* buf, size_t len, md5result_t* result);
std::wstring md5w(const unsigned char* buf, size_t len);
std::string md5(const unsigned char* buf, size_t len);
std::wstring md5str(const std::wstring& str);
std::string md5str(const std::string& str);

std::string md5resultToString(const md5result_t& res);


};



#endif

