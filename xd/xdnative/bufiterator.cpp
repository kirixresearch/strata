/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-05-05
 *
 */


#include <kl/klib.h>
#include <xd/xd.h>
#include "bufiterator.h"
#include "../xdcommon/exindex.h"


// BufIterator implementation 

BufIterator::BufIterator()
{
    m_first_row = NULL;
    m_row_width = 0;
}

BufIterator::~BufIterator()
{
}


bool BufIterator::init(XdnativeDatabase* database,
                       IXdnativeSet* set)
{
    if (!BaseIterator::init(database, set, L""))
        return false;

    // we don't need to (and don't want to) hold on to
    // the set pointer.  It was only needed to get the
    // physical structure of the buffer
    setSet(xcm::null);

    return true;
}


void BufIterator::setRowBuffer(unsigned char* buf, int row_width)
{
    m_first_row = buf;
    m_rowptr = buf;
    m_row_width = row_width;
}

xd::IIteratorPtr BufIterator::clone()
{
    BufIterator* new_iter = new BufIterator;
    if (!baseClone(new_iter))
    {
        delete new_iter;
        return xcm::null;
    }

    return static_cast<xd::IIterator*>(new_iter);
}

void BufIterator::skip(int delta)
{
    m_rowptr += (delta * m_row_width);
}

void BufIterator::goFirst()
{
    m_rowptr = m_first_row;
}

void BufIterator::goLast()
{
}

xd::rowid_t BufIterator::getRowId()
{
    return 0;
}

bool BufIterator::bof()
{
    return false;
}

bool BufIterator::eof()
{
    return false;
}

bool BufIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool BufIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool BufIterator::setPos(double pct)
{
    return false;
}

double BufIterator::getPos()
{
    return 0.0;
}




