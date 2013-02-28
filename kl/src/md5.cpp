/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-24
 *
 */


#include <memory.h>         // for memcpy()
#include "kl/file.h"
#include "kl/md5.h"
#include "kl/utf8.h"


namespace kl
{


// This code implements the MD5 message-digest algorithm.
// The algorithm is due to Ron Rivest.    This code was
// written by Colin Plumb in 1993, no copyright is claimed.
// This code is in the public domain; do with it what you wish.


typedef unsigned int uint32;


struct md5context_t
{
    uint32 buf[4];
    uint32 bits[2];
    unsigned char in[64];
};



//#define BIG_ENDIAN


#ifndef BIG_ENDIAN
    #define byteReverse(buf, len)    /* Nothing */
#else
    void byteReverse(unsigned char* buf, unsigned int longs)
    {
        uint32 t;
        do {
        t = (uint32) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
            ((unsigned) buf[1] << 8 | buf[0]);
        *(uint32 *) buf = t;
        buf += 4;
        } while (--longs);
    }
#endif


// The four core functions - F1 is optimized somewhat

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

// This is the central step in the MD5 algorithm.
#define MD5STEP(f, w, x, y, z, data, s) \
    ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )


// The core of the MD5 algorithm, this alters an existing MD5 hash to
// reflect the addition of 16 longwords of new data.  md5_update blocks
// the data and converts bytes into longwords for this routine.

static void md5_transform(uint32 buf[4], uint32 in[16])
{
    register uint32 a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}


// Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
// initialization constants.

static void md5_init(md5context_t* ctx)
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
}



// Update context to reflect the concatenation of another buffer full
// of bytes.

static void md5_update(md5context_t* ctx,
                       const unsigned char* buf,
                       unsigned int len)
{
    uint32 t;

    // Update bitcount

    t = ctx->bits[0];
    if ((ctx->bits[0] = t + ((uint32) len << 3)) < t)
    ctx->bits[1]++;     /* Carry from low to high */
    ctx->bits[1] += len >> 29;

    t = (t >> 3) & 0x3f;    // Bytes already in shsInfo->data

    // Handle any leading odd-sized chunks
    if (t)
    {
        unsigned char* p = (unsigned char*)ctx->in + t;

        t = 64 - t;
        if (len < t)
        {
            memcpy(p, buf, len);
            return;
        }
        
        memcpy(p, buf, t);
        byteReverse(ctx->in, 16);
        md5_transform(ctx->buf, (uint32 *) ctx->in);
        buf += t;
        len -= t;
    }
    
    // Process data in 64-byte chunks

    while (len >= 64)
    {
        memcpy(ctx->in, buf, 64);
        byteReverse(ctx->in, 16);
        md5_transform(ctx->buf, (uint32 *) ctx->in);
        buf += 64;
        len -= 64;
    }

    // Handle any remaining bytes of data.
    memcpy(ctx->in, buf, len);
}


// Final wrapup - pad to 64-byte boundary with the bit pattern 
// 1 0* (64-bit count of bits processed, MSB-first)

static void md5_final(unsigned char digest[16],
                      md5context_t* ctx)
{
    unsigned count;
    unsigned char *p;

    // Compute number of bytes mod 64
    count = (ctx->bits[0] >> 3) & 0x3F;

    // Set the first char of padding to 0x80.  This is safe since there is
    // always at least one byte free
    p = ctx->in + count;
    *p++ = 0x80;

    // Bytes of padding needed to make 64 bytes
    count = 64 - 1 - count;

    // Pad out to 56 mod 64
    if (count < 8)
    {
        // Two lots of padding:  Pad the first block to 64 bytes
        memset(p, 0, count);
        byteReverse(ctx->in, 16);
        md5_transform(ctx->buf, (uint32 *) ctx->in);

        // Now fill the next block with 56 bytes
        memset(ctx->in, 0, 56);
    }
     else
    {
        // Pad block to 56 bytes
        memset(p, 0, count - 8);
    }
    byteReverse(ctx->in, 14);

    // Append length in bits and transform
    ((uint32 *) ctx->in)[14] = ctx->bits[0];
    ((uint32 *) ctx->in)[15] = ctx->bits[1];

    md5_transform(ctx->buf, (uint32 *) ctx->in);
    byteReverse((unsigned char *) ctx->buf, 4);
    memcpy(digest, ctx->buf, 16);
    memset(ctx, 0, sizeof(md5context_t));        // In case it's sensitive
}




// -- public interface --

void md5(const unsigned char* buf,
         size_t len,
         md5result_t* result)
{
    md5context_t ctx;
    
    md5_init(&ctx);
    md5_update(&ctx, buf, len);
    md5_final(result->buf, &ctx);
}

bool md5sum(const std::wstring& path, md5result_t* result)
{
    md5context_t ctx;

    xf_file_t f = xf_open(path, xfOpen, xfRead, xfShareReadWrite);
    if (!f)
        return false;

    md5_init(&ctx);

    unsigned char buf[512];
    int r;

    while (true)
    {
        r = xf_read(f, buf, 1, 512);
        if (!r) break;
        md5_update(&ctx, buf, (unsigned int)r);
    }

    xf_close(f);

    md5_final(result->buf, &ctx);

    return true;
}

std::string md5resultToString(const md5result_t& res)
{
    static const char* hexchars = "0123456789abcdef";
    std::string result;
    result.reserve(32);
        
    for (unsigned int i = 0; i < 16; ++i)
    {
        result += *(hexchars + (res.buf[i] >> 4));
        result += *(hexchars + (res.buf[i] & 0xf));
    }
    
    return result;
}


std::wstring md5w(const unsigned char* buf, size_t len)
{
    static const wchar_t* hexchars = L"0123456789abcdef";
    
    md5result_t mr;
    std::wstring result;
    result.reserve(32);
    
    md5(buf, len, &mr);
    
    for (unsigned int i = 0; i < 16; ++i)
    {
        result += *(hexchars + (mr.buf[i] >> 4));
        result += *(hexchars + (mr.buf[i] & 0xf));
    }
    
    return result;
}

std::string md5(const unsigned char* buf, size_t len)
{
    static const char* hexchars = "0123456789abcdef";
    
    md5result_t mr;
    std::string result;
    result.reserve(32);
    
    md5(buf, len, &mr);
    
    for (unsigned int i = 0; i < 16; ++i)
    {
        result += *(hexchars + (mr.buf[i] >> 4));
        result += *(hexchars + (mr.buf[i] & 0xf));
    }
    
    return result;
}


std::wstring md5str(const std::wstring& str)
{
    int len = str.length();
    char* utf8_str = new char[(len+1)*4];
    utf8_wtoutf8(utf8_str, (len+1)*4, str.c_str(), str.length());
    std::wstring res = md5w((unsigned char*)utf8_str, strlen(utf8_str));
    delete[] utf8_str;
    
    return res;
}

std::string md5str(const std::string& str)
{
    int len = str.length();
    char* utf8_str = new char[(len+1)*4];
    utf8_atoutf8(utf8_str, (len+1)*4, str.c_str(), str.length());
    std::string res = md5((unsigned char*)utf8_str, strlen(utf8_str));
    delete[] utf8_str;
    
    return res;
}


};
