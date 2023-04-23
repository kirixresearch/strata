/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2013-10-29
 *
 */


#ifndef H_KL_CRC_H
#define H_KL_CRC_H



namespace kl
{

unsigned short crc16(const unsigned char* data, unsigned int size, unsigned short crc = 0);
unsigned long crc32(const unsigned char* data, int size);


};



#endif

