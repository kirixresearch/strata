/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2013-05-22
 *
 */


#ifndef __KL_HEX_H
#define __KL_HEX_H


namespace kl
{

unsigned long long hexToUint64(const std::wstring& code);
std::wstring uint64ToHex(unsigned long long num);

};



#endif

