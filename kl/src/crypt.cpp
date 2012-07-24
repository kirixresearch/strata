/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-25
 *
 */


// NOTE: this code is in paladin, too


#include <ctime>
#include <memory.h>
#include "kl/crypt.h"
#include "kl/string.h"
#include "kl/portable.h"
#include "kl/regex.h"


namespace kl
{


const int DES_KEY_SIZE = 56;
const int DES_DATA_SIZE = 64;
const int DES_SBUFFER_SIZE = 48;
const int DES_ROUNDS =  16;


class Des
{
public:

    void setKey(const unsigned char* key);
    void crypt(unsigned char* data, int size, bool encrypt);
    
private:
    unsigned char final_key[DES_ROUNDS][DES_SBUFFER_SIZE];

    void doCrypt(unsigned char[DES_DATA_SIZE], bool encrypt);
    void doKeyInit(const unsigned char[DES_KEY_SIZE]);

};



const int KEY_SHIFT[DES_ROUNDS] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};

const unsigned char COMPRESSION_PERMUTATION[DES_SBUFFER_SIZE] =
{
    14,17,11,24, 1, 5, 3,28,15, 6,21,10,
    23,19,12, 4,26, 8,16, 7,27,20,13, 2,
    41,52,31,37,47,55,30,40,51,45,33,48,
    44,49,39,56,34,53,46,42,50,36,29,32
};

const unsigned char EXPANSION_PERMUTATION[DES_SBUFFER_SIZE] =
{
    32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9,
    8, 9,10,11,12,13,12,13,14,15,16,17,
    16,17,18,19,20,21,20,21,22,23,24,25,
    24,25,26,27,28,29,28,29,30,31,32, 1
};


// The following are the above S-boxes, packed one bit per byte:

const unsigned char MODIFIED_SBOX_1[4*64] =
{
    1,1,1,0,0,1,0,0,1,1,0,1,0,0,0,1,0,0,1,0,1,1,1,1,1,0,1,1,1,0,0,0,
    0,0,1,1,1,0,1,0,0,1,1,0,1,1,0,0,0,1,0,1,1,0,0,1,0,0,0,0,0,1,1,1,
    0,0,0,0,1,1,1,1,0,1,1,1,0,1,0,0,1,1,1,0,0,0,1,0,1,1,0,1,0,0,0,1,
    1,0,1,0,0,1,1,0,1,1,0,0,1,0,1,1,1,0,0,1,0,1,0,1,0,0,1,1,1,0,0,0,
    0,1,0,0,0,0,0,1,1,1,1,0,1,0,0,0,1,1,0,1,0,1,1,0,0,0,1,0,1,0,1,1,
    1,1,1,1,1,1,0,0,1,0,0,1,0,1,1,1,0,0,1,1,1,0,1,0,0,1,0,1,0,0,0,0,
    1,1,1,1,1,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,0,1,0,0,0,1,0,1,1,1,
    0,1,0,1,1,0,1,1,0,0,1,1,1,1,1,0,1,0,1,0,0,0,0,0,0,1,1,0,1,1,0,1
};

const unsigned char MODIFIED_SBOX_2[4*64] =
{
    1,1,1,1,0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,0,0,1,0,1,1,1,0,0,1,0,1,1,0,1,1,1,0,0,0,0,0,0,0,1,0,1,1,0,1,0,
    0,0,1,1,1,1,0,1,0,1,0,0,0,1,1,1,1,1,1,1,0,0,1,0,1,0,0,0,1,1,1,0,
    1,1,0,0,0,0,0,0,0,0,0,1,1,0,1,0,0,1,1,0,1,0,0,1,1,0,1,1,0,1,0,1,
    0,0,0,0,1,1,1,0,0,1,1,1,1,0,1,1,1,0,1,0,0,1,0,0,1,1,0,1,0,0,0,1,
    0,1,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1,0,0,1,0,0,1,1,0,0,1,0,1,1,1,1,
    1,1,0,1,1,0,0,0,1,0,1,0,0,0,0,1,0,0,1,1,1,1,1,1,0,1,0,0,0,0,1,0,
    1,0,1,1,0,1,1,0,0,1,1,1,1,1,0,0,0,0,0,0,0,1,0,1,1,1,1,0,1,0,0,1
};

const unsigned char MODIFIED_SBOX_3[4*64] =
{
    1,0,1,0,0,0,0,0,1,0,0,1,1,1,1,0,0,1,1,0,0,0,1,1,1,1,1,1,0,1,0,1,
    0,0,0,1,1,1,0,1,1,1,0,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,0,0,
    1,1,0,1,0,1,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,1,0,0,0,1,1,0,1,0,1,0,
    0,0,1,0,1,0,0,0,0,1,0,1,1,1,1,0,1,1,0,0,1,0,1,1,1,1,1,1,0,0,0,1,
    1,1,0,1,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,1,1,1,0,0,1,1,0,0,0,0,
    1,0,1,1,0,0,0,1,0,0,1,0,1,1,0,0,0,1,0,1,1,0,1,0,1,1,1,0,0,1,1,1,
    0,0,0,1,1,0,1,0,1,1,0,1,0,0,0,0,0,1,1,0,1,0,0,1,1,0,0,0,0,1,1,1,
    0,1,0,0,1,1,1,1,1,1,1,0,0,0,1,1,1,0,1,1,0,1,0,1,0,0,1,0,1,1,0,0
};

const unsigned char MODIFIED_SBOX_4[4*64] =
{
    0,1,1,1,1,1,0,1,1,1,1,0,0,0,1,1,0,0,0,0,0,1,1,0,1,0,0,1,1,0,1,0,
    0,0,0,1,0,0,1,0,1,0,0,0,0,1,0,1,1,0,1,1,1,1,0,0,0,1,0,0,1,1,1,1,
    1,1,0,1,1,0,0,0,1,0,1,1,0,1,0,1,0,1,1,0,1,1,1,1,0,0,0,0,0,0,1,1,
    0,1,0,0,0,1,1,1,0,0,1,0,1,1,0,0,0,0,0,1,1,0,1,0,1,1,1,0,1,0,0,1,
    1,0,1,0,0,1,1,0,1,0,0,1,0,0,0,0,1,1,0,0,1,0,1,1,0,1,1,1,1,1,0,1,
    1,1,1,1,0,0,0,1,0,0,1,1,1,1,1,0,0,1,0,1,0,0,1,0,1,0,0,0,0,1,0,0,
    0,0,1,1,1,1,1,1,0,0,0,0,0,1,1,0,1,0,1,0,0,0,0,1,1,1,0,1,1,0,0,0,
    1,0,0,1,0,1,0,0,0,1,0,1,1,0,1,1,1,1,0,0,0,1,1,1,0,0,1,0,1,1,1,0
};

const unsigned char MODIFIED_SBOX_5[4*64] =
{
    0,0,1,0,1,1,0,0,0,1,0,0,0,0,0,1,0,1,1,1,1,0,1,0,1,0,1,1,0,1,1,0,
    1,0,0,0,0,1,0,1,0,0,1,1,1,1,1,1,1,1,0,1,0,0,0,0,1,1,1,0,1,0,0,1,
    1,1,1,0,1,0,1,1,0,0,1,0,1,1,0,0,0,1,0,0,0,1,1,1,1,1,0,1,0,0,0,1,
    0,1,0,1,0,0,0,0,1,1,1,1,1,0,1,0,0,0,1,1,1,0,0,1,1,0,0,0,0,1,1,0,
    0,1,0,0,0,0,1,0,0,0,0,1,1,0,1,1,1,0,1,0,1,1,0,1,0,1,1,1,1,0,0,0,
    1,1,1,1,1,0,0,1,1,1,0,0,0,1,0,1,0,1,1,0,0,0,1,1,0,0,0,0,1,1,1,0,
    1,0,1,1,1,0,0,0,1,1,0,0,0,1,1,1,0,0,0,1,1,1,1,0,0,0,1,0,1,1,0,1,
    0,1,1,0,1,1,1,1,0,0,0,0,1,0,0,1,1,0,1,0,0,1,0,0,0,1,0,1,0,0,1,1
};

const unsigned char MODIFIED_SBOX_6[4*64] =
{
    1,1,0,0,0,0,0,1,1,0,1,0,1,1,1,1,1,0,0,1,0,0,1,0,0,1,1,0,1,0,0,0,
    0,0,0,0,1,1,0,1,0,0,1,1,0,1,0,0,1,1,1,0,0,1,1,1,0,1,0,1,1,0,1,1,
    1,0,1,0,1,1,1,1,0,1,0,0,0,0,1,0,0,1,1,1,1,1,0,0,1,0,0,1,0,1,0,1,
    0,1,1,0,0,0,0,1,1,1,0,1,1,1,1,0,0,0,0,0,1,0,1,1,0,0,1,1,1,0,0,0,
    1,0,0,1,1,1,1,0,1,1,1,1,0,1,0,1,0,0,1,0,1,0,0,0,1,1,0,0,0,0,1,1,
    0,1,1,1,0,0,0,0,0,1,0,0,1,0,1,0,0,0,0,1,1,1,0,1,1,0,1,1,0,1,1,0,
    0,1,0,0,0,0,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,1,0,1,1,1,1,1,1,0,1,0,
    1,0,1,1,1,1,1,0,0,0,0,1,0,1,1,1,0,1,1,0,0,0,0,0,1,0,0,0,1,1,0,1
};

const unsigned char MODIFIED_SBOX_7[4*64] =
{
    0,1,0,0,1,0,1,1,0,0,1,0,1,1,1,0,1,1,1,1,0,0,0,0,1,0,0,0,1,1,0,1,
    0,0,1,1,1,1,0,0,1,0,0,1,0,1,1,1,0,1,0,1,1,0,1,0,0,1,1,0,0,0,0,1,
    1,1,0,1,0,0,0,0,1,0,1,1,0,1,1,1,0,1,0,0,1,0,0,1,0,0,0,1,1,0,1,0,
    1,1,1,0,0,0,1,1,0,1,0,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,0,0,0,1,1,0,
    0,0,0,1,0,1,0,0,1,0,1,1,1,1,0,1,1,1,0,0,0,0,1,1,0,1,1,1,1,1,1,0,
    1,0,1,0,1,1,1,1,0,1,1,0,1,0,0,0,0,0,0,0,0,1,0,1,1,0,0,1,0,0,1,0,
    0,1,1,0,1,0,1,1,1,1,0,1,1,0,0,0,0,0,0,1,0,1,0,0,1,0,1,0,0,1,1,1,
    1,0,0,1,0,1,0,1,0,0,0,0,1,1,1,1,1,1,1,0,0,0,1,0,0,0,1,1,1,1,0,0
};

const unsigned char MODIFIED_SBOX_8[4*64] =
{
    1,1,0,1,0,0,1,0,1,0,0,0,0,1,0,0,0,1,1,0,1,1,1,1,1,0,1,1,0,0,0,1,
    1,0,1,0,1,0,0,1,0,0,1,1,1,1,1,0,0,1,0,1,0,0,0,0,1,1,0,0,0,1,1,1,
    0,0,0,1,1,1,1,1,1,1,0,1,1,0,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,1,0,0,
    1,1,0,0,0,1,0,1,0,1,1,0,1,0,1,1,0,0,0,0,1,1,1,0,1,0,0,1,0,0,1,0,
    0,1,1,1,1,0,1,1,0,1,0,0,0,0,0,1,1,0,0,1,1,1,0,0,1,1,1,0,0,0,1,0,
    0,0,0,0,0,1,1,0,1,0,1,0,1,1,0,1,1,1,1,1,0,0,1,1,0,1,0,1,1,0,0,0,
    0,0,1,0,0,0,0,1,1,1,1,0,0,1,1,1,0,1,0,0,1,0,1,0,1,0,0,0,1,1,0,1,
    1,1,1,1,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,1,0,1,0,1,0,1,1,0,1,0,1,1
};

const unsigned char PBOX_PERMUTATION[32] =
{
    16, 7,20,21,29,12,28,17, 1,15,23,26, 5,18,31,10,
    2, 8,24,14,32,27, 3, 9,19,13,30, 6,22,11, 4,25
};








static void shift_half_key_left(unsigned char *k)
{
    unsigned char x = *k;
    unsigned char *kmax = k + (DES_KEY_SIZE/2-1);
    do k[0] = k[1]; while (++k < kmax);
    *k = x;
}



void Des::doKeyInit(const unsigned char key[DES_KEY_SIZE])
{
    unsigned char shifted_key[DES_KEY_SIZE];

    memcpy(shifted_key, key, DES_KEY_SIZE);

    for (int round = 0; round < DES_ROUNDS; round++)
    {
        shift_half_key_left(shifted_key);
        shift_half_key_left(shifted_key+DES_KEY_SIZE/2);

        if (KEY_SHIFT[round] == 2)
        {
            shift_half_key_left(shifted_key);
            shift_half_key_left(shifted_key+DES_KEY_SIZE/2);
        }

        for (int i = 0; i < DES_SBUFFER_SIZE; i++)
        {
            final_key[round][i] =
            shifted_key[COMPRESSION_PERMUTATION[i]-1];
        }
    }
}



#define index(n) (sbuffer[n]<<(5+2) | sbuffer[n+5]<<(4+2) | \
        sbuffer[n+1]<<(3+2) | sbuffer[n+2]<<(2+2) | sbuffer[n+3]<<(1+2) | \
        sbuffer[n+4]<<(0+2))

void Des::doCrypt(unsigned char data[DES_DATA_SIZE],
                          bool encrypt)
{
    unsigned char sbuffer[DES_SBUFFER_SIZE];
    union pbuffer_tag
    {
        unsigned char byte[DES_DATA_SIZE/2];
        unsigned long dword[DES_DATA_SIZE/8];
    } pbuffer;

    for (int round = 0; round < DES_ROUNDS; round++)
    {
        /* XOR compressed key with expanded right half of data */
        {
            int i;
            for (i = 0; i < DES_SBUFFER_SIZE; i++)
            {
                sbuffer[i] = data[DES_DATA_SIZE/2 + EXPANSION_PERMUTATION[i]-1] ^
                    final_key[encrypt ? round : 15-round][i];
            }
        }

        /* S-Box substitutions */
        {
            pbuffer.dword[0] = * (unsigned long *) (MODIFIED_SBOX_1 + index(0));
            pbuffer.dword[1] = * (unsigned long *) (MODIFIED_SBOX_2 + index(6));
            pbuffer.dword[2] = * (unsigned long *) (MODIFIED_SBOX_3 + index(12));
            pbuffer.dword[3] = * (unsigned long *) (MODIFIED_SBOX_4 + index(18));
            pbuffer.dword[4] = * (unsigned long *) (MODIFIED_SBOX_5 + index(24));
            pbuffer.dword[5] = * (unsigned long *) (MODIFIED_SBOX_6 + index(30));
            pbuffer.dword[6] = * (unsigned long *) (MODIFIED_SBOX_7 + index(36));
            pbuffer.dword[7] = * (unsigned long *) (MODIFIED_SBOX_8 + index(42));
        }

        /* XOR and swap */
        if (round < 15)
        {
            int i;
            for (i = 0; i < DES_DATA_SIZE/2; i++)
            {
                unsigned char x = data[DES_DATA_SIZE/2 + i];
                data[DES_DATA_SIZE/2 + i] = data[i] ^ pbuffer.byte[PBOX_PERMUTATION[i]-1];
                data[i] = x;
            }
        }
         else
        {
            int i;
            for (i = 0; i < DES_DATA_SIZE/2; i++)
                data[i] = data[i] ^ pbuffer.byte[PBOX_PERMUTATION[i]-1];
        }
    }
}


void Des::setKey(const unsigned char* data)
{
    unsigned char buffer[DES_KEY_SIZE];
    int i;

    for (i = 0; i < DES_KEY_SIZE; i += 8)
    {
        buffer[i] = *data >> 7 & 1;
        buffer[i+1] = *data >> 6 & 1;
        buffer[i+2] = *data >> 5 & 1;
        buffer[i+3] = *data >> 4 & 1;
        buffer[i+4] = *data >> 3 & 1;
        buffer[i+5] = *data >> 2 & 1;
        buffer[i+6] = *data >> 1 & 1;
        buffer[i+7] = *data & 1;

        data++;
    }

    doKeyInit(buffer);
}


void Des::crypt(unsigned char* data, int size, bool encrypt)
{
    unsigned char buffer[64];
    
    if (size % 8 != 0)
    {
        // -- fail. input data size must
        //    be a multiple of 8 --
        return;
    }

    int i;

    while (size > 0)
    {
        unsigned char* pos = data;

        memset(buffer, 0, 64);
        for (i = 0; i < DES_DATA_SIZE; i += 8)
        {
            buffer[i] = *data >> 7 & 1;
            buffer[i+1] = *data >> 6 & 1;
            buffer[i+2] = *data >> 5 & 1;
            buffer[i+3] = *data >> 4 & 1;
            buffer[i+4] = *data >> 3 & 1;
            buffer[i+5] = *data >> 2 & 1;
            buffer[i+6] = *data >> 1 & 1;
            buffer[i+7] = *data & 1;

            data++;
        }

        size -= 8;

        doCrypt(buffer, encrypt);

        for (i = 0; i < DES_DATA_SIZE; i += 8)
        {
            *pos =  buffer[i] << 7 |
                    buffer[i+1] << 6 |
                    buffer[i+2] << 5 |
                    buffer[i+3] << 4 |
                    buffer[i+4] << 3 |
                    buffer[i+5] << 2 |
                    buffer[i+6] << 1 |
                    buffer[i+7];

            pos++;
        }
    }
}







void keystringTo24ByteKey(const std::wstring& key_string, unsigned char* out_key)
{
    memset(out_key, 0, 24);
    
    size_t len = key_string.length();
    
    if (len > 0)
    {
        for (size_t i = 0; i < 24; ++i)
        {
            out_key[i] = key_string[i % len];
        }
    }
}





bool isEncryptedString(const std::wstring& encrypted_string)
{
    int len = encrypted_string.length();
    
    // check if the string begins with an X or a Z (it must).  This
    // let's us skip over using the slower regex for many strings
    if (len == 0 || (encrypted_string[0] != 'X' && encrypted_string[0] != 'Z'))
        return false;
    
    // length must be odd -- because the encrypted strings are
    // stored as the signature ('X') plus an arbitrary number of
    // two-digit hex numbers
    if (len % 2 == 0)
        return false;
        
    static const klregex::wregex fmt1(L"(X|ZXP)[0123456789ABCDEF]+");
    return fmt1.match(encrypted_string.c_str()) ? true : false;
}

static std::wstring getCodePortion(const std::wstring& encrypted_string)
{
    if (encrypted_string.substr(0,1) == L"X")
        return encrypted_string.substr(1);
    if (encrypted_string.substr(0,3) == L"ZXP")
        return encrypted_string.substr(3);
    return L"";
}

std::wstring encryptString(const std::wstring& source_string, const std::wstring& key_string)
{
    unsigned char key[24];
    keystringTo24ByteKey(key_string, key);
    
    size_t i, str_len = source_string.length();

    size_t buf_size = (str_len * 2) + 1;
    if ((buf_size % 8) != 0)
        buf_size += (8 - (buf_size % 8));
    
    unsigned char* buf = new unsigned char[buf_size];
    memset(buf, 0, buf_size);
    kl::wstring2ucsle(buf+1, source_string, str_len);
    
    
    // first byte stores our xor salt byte
    unsigned char xor_salt_byte = ((unsigned int)time(NULL) + (unsigned int)clock() + rand()) % 255;
    buf[0] = xor_salt_byte;
    for (i = 1; i < buf_size; ++i)
        buf[i] ^= xor_salt_byte;
    
    
    Des d1, d2, d3;
    
    d1.setKey(key);
    d2.setKey(key+8);
    d3.setKey(key+16);
    
    d1.crypt(buf, buf_size, true);
    d2.crypt(buf, buf_size, true);
    d3.crypt(buf, buf_size, true);
    
    std::wstring retval;
    retval = L"X"; // version marker
    
    wchar_t ch[10];
    for (i = 0; i < buf_size; ++i)
    {
        swprintf(ch, 9, L"%02X", buf[i]);
        retval += ch;
    }
    
    delete[] buf;
    
    return retval;
}




const char* hex_digits = "0123456789ABCDEF";
static unsigned char hex2char(const wchar_t* s)
{
    int b1 = 0, b2 = 0;
    
    if (*s && *(s+1))
    {
        b1 = (strchr(hex_digits, toupper(s[0]))-hex_digits)*16;
        b2 = strchr(hex_digits, towupper(s[1]))-hex_digits;
    }
     else if (*s)
    {
        b1 = strchr(hex_digits, toupper(s[0]))-hex_digits;
    }
    
    return (unsigned char)(b1+b2);
}


std::wstring decryptString(const std::wstring& _encrypted_string, const std::wstring& key_string)
{
    unsigned char key[24];
    keystringTo24ByteKey(key_string, key);

    if (!isEncryptedString(_encrypted_string))
        return L"";
    
    // chop off version marker
    std::wstring encrypted_string = getCodePortion(_encrypted_string);
        
    size_t buf_size = encrypted_string.length() / 2;
    unsigned char* buf = new unsigned char[buf_size];
    memset(buf, 0, buf_size);

    // un-hexify the string
    size_t i;
    const wchar_t* p = encrypted_string.c_str();
    
    for (i = 0; i < buf_size; ++i)
    {
        if (!(*p) || !*(p+1))
            break;
            
        buf[i] = hex2char(p);
        
        p += 2;
    }
    
    // decrypt the buffer
    Des d1, d2, d3;
    
    d1.setKey(key);
    d2.setKey(key+8);
    d3.setKey(key+16);
    
    d3.crypt(buf, buf_size, false);
    d2.crypt(buf, buf_size, false);
    d1.crypt(buf, buf_size, false);
   
   
    // fetch the xor salt byte
    unsigned char xor_salt_byte = buf[0];
    for (i = 1; i < buf_size; ++i)
        buf[i] ^= xor_salt_byte;
   
   
    std::wstring retval;
    ucsle2wstring(retval, buf+1, (buf_size-1)/2);
    
    delete[] buf;
    
    return retval;
}



// please note that, in encryptBuffer, the output buffer
// size must be 16 bytes greater in size than the input_buffer
// the total size of the encrypted data is placed in the value
// pointed by |output_buffer_size|
bool encryptBuffer(const unsigned char* input_buf,
                   size_t input_size,
                   unsigned char* output_buf,
                   size_t* output_size,
                   const std::wstring& key_string)
{
    if (*output_size < input_size+16)
        return false;

    unsigned char key[24];
    keystringTo24ByteKey(key_string, key);


    unsigned char xor_salt_byte = rand() % 255;

    memset(output_buf, 0, *output_size);
    output_buf[0] = xor_salt_byte;
    output_buf[1] = input_size & 0xff;
    output_buf[2] = (input_size >> 8) & 0xff;
    output_buf[3] = (input_size >> 16) & 0xff;
    output_buf[4] = (input_size >> 24) & 0xff;
    memcpy(output_buf+5, input_buf, input_size);
    
    int enc_size = input_size + 5;
    while ((enc_size % 8) != 0)
        ++enc_size;
    
    for (size_t i = 1; i < enc_size; ++i)
        output_buf[i] ^= xor_salt_byte;

    Des d1, d2, d3;
    
    d1.setKey(key);
    d2.setKey(key+8);
    d3.setKey(key+16);
    
    d1.crypt(output_buf, enc_size, true);
    d2.crypt(output_buf, enc_size, true);
    d3.crypt(output_buf, enc_size, true);
            
    *output_size = enc_size;
    
    return true;
}

// please note that, in decryptBuffer, the output buffer
// size must be at least as big as the input_buffer
// the total size of the decrypted data is placed in the value
// pointed by |output_buffer_size|
bool decryptBuffer(const unsigned char* input_buf,
                   size_t input_size,
                   unsigned char* output_buf,
                   size_t* output_size,
                   const std::wstring& key_string)
{
    if ((input_size % 8) != 0)
        return false;
    
    if (*output_size < input_size)
        return false;
        
    unsigned char key[24];
    keystringTo24ByteKey(key_string, key);

    Des d1, d2, d3;
    
    d1.setKey(key);
    d2.setKey(key+8);
    d3.setKey(key+16);
    
    memcpy(output_buf, input_buf, input_size);
    
    d3.crypt(output_buf, input_size, false);
    d2.crypt(output_buf, input_size, false);
    d1.crypt(output_buf, input_size, false);

    // fetch the xor salt byte
    unsigned char xor_salt_byte = output_buf[0];
    for (size_t i = 1; i < input_size; ++i)
        output_buf[i] ^= xor_salt_byte;

    size_t data_size = 0;
    size_t tempi;
    tempi = output_buf[4];
    tempi <<= 24;
    data_size |= tempi;
    tempi = output_buf[3];
    tempi <<= 16;
    data_size |= tempi;
    tempi = output_buf[2];
    tempi <<= 8;
    data_size |= tempi;
    tempi = output_buf[1];
    data_size |= tempi;

    memmove(output_buf, output_buf+5, data_size);
    *output_size = data_size;
    
    return true;
}





}; // namespace kl
