/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-03-12
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "nodefilestream.h"


NodeFileStream::NodeFileStream()
{
}

NodeFileStream::~NodeFileStream()
{
}


bool NodeFileStream::create(const std::wstring& filename)
{
    return false;
}

bool NodeFileStream::open(const std::wstring& filename)
{
    return false;
}

bool NodeFileStream::read(void* buf,
                          unsigned long read_size,
                          unsigned long* read_count)
{
    return false;
}
                  
bool NodeFileStream::write(const void* buf,
                           unsigned long write_size,
                           unsigned long* written_count)
{
    return false;
}

