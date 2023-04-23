/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Security Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-30
 *
 */


#ifndef H_PALADIN_UTIL_H
#define H_PALADIN_UTIL_H


namespace paladin
{

int dateToJulian(int year, int month, int day);
void int64unpack(unsigned char* bytes, const paladin_int64_t& i);
void int64pack(paladin_int64_t& i, const unsigned char* bytes);
void rot13(char* s);
void rot13(wchar_t* s);
void reverse_string(char* str);
paladin_int64_t getCodeFromString(const char* _code);
void getStringFromCode(const paladin_int64_t& actcode, char* output_string, bool format = true);
void int64crypt(paladin::paladin_int64_t& i, const unsigned char* key, bool encrypt);

};


#endif

