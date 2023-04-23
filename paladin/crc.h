/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Security Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-27
 *
 */


// NOTE:
//
// The CRC-16 code was devised by Don P. Mitchell of AT&T Bell Laboratories
// and Ned W. Rhodes of Software Systems Group. It has been published in
// "Design and Validation of Computer Protocols", Prentice Hall,
// Englewood Cliffs, NJ, 1991, Chapter 3, ISBN 0-13-539925-4.
//
// Copyright for CRC-16 is held by AT&T.
//
// AT&T gives permission for the free use of the CRC-16 source code.


#ifndef H_PALADIN_CRC_H
#define H_PALADIN_CRC_H


namespace paladin
{


unsigned short crc16(const unsigned char* data, unsigned int size, unsigned short crc = 0);
unsigned long crc32(const unsigned char* data, int size);


};


#endif

