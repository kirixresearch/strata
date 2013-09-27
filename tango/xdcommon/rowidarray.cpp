/*!
 *
 * Copyright (c) 2010-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2010-11-15
 *
 */


#include <kl/file.h>
#include "tango.h"
#include "exkeypool.h"
#include "rowidarray.h"
#include "util.h"


RowIdArray::RowIdArray(const std::wstring& temp_dir)
{
    // set up key pool to store 64-bit integers
    m_pool = new ExVirtualKeyPool(temp_dir, 800000, 8, 8);
}

RowIdArray::~RowIdArray()
{
    delete m_pool;
}
    
void RowIdArray::append(xd::rowid_t rowid)
{
    unsigned char buf[8];
    rowidToBuf(buf, rowid);
    m_pool->appendData(buf);
}

void RowIdArray::goFirst()
{
    m_pool->goFirst();
}

void RowIdArray::goNext()
{
    m_pool->goNext();
}

bool RowIdArray::isEof()
{
    return m_pool->isEof();
}

xd::rowid_t RowIdArray::getItem()
{
    xd::rowid_t result;
    result = bufToRowid(m_pool->getEntryPtr());
    return result;
}


