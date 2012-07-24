/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Security Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-25
 *
 */


// NOTE:
//
// Code in this file is based off of the public domain 56-bit DES
// encryption code available from: http://www.efgh.com/software/des.htm


#ifndef __PALADIN_CRYPT_H
#define __PALADIN_CRYPT_H


const int DES_KEY_SIZE = 56;
const int DES_DATA_SIZE = 64;
const int DES_SBUFFER_SIZE = 48;
const int DES_ROUNDS =  16;


class Des
{
private:
    unsigned char final_key[DES_ROUNDS][DES_SBUFFER_SIZE];

    void doCrypt(unsigned char[DES_DATA_SIZE], bool encrypt);
    void doKeyInit(const unsigned char[DES_KEY_SIZE]);

public:

    void setKey(const unsigned char* key);
    void crypt(unsigned char* data, int size, bool encrypt);
};


#endif

