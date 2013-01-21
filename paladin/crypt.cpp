/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
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


#include <memory.h>
#include "crypt.h"


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
        unsigned int dword[DES_DATA_SIZE/8];
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
            pbuffer.dword[0] = * (unsigned int *) (MODIFIED_SBOX_1 + index(0));
            pbuffer.dword[1] = * (unsigned int *) (MODIFIED_SBOX_2 + index(6));
            pbuffer.dword[2] = * (unsigned int *) (MODIFIED_SBOX_3 + index(12));
            pbuffer.dword[3] = * (unsigned int *) (MODIFIED_SBOX_4 + index(18));
            pbuffer.dword[4] = * (unsigned int *) (MODIFIED_SBOX_5 + index(24));
            pbuffer.dword[5] = * (unsigned int *) (MODIFIED_SBOX_6 + index(30));
            pbuffer.dword[6] = * (unsigned int *) (MODIFIED_SBOX_7 + index(36));
            pbuffer.dword[7] = * (unsigned int *) (MODIFIED_SBOX_8 + index(42));
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
        // fail. input data size must be a multiple of 8
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




