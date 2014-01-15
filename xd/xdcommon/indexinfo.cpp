/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-03-16
 *
 */


#include <xd/xd.h>
#include "indexinfo.h"


IndexInfo::IndexInfo()
{
}

IndexInfo::~IndexInfo()
{
}


void IndexInfo::setName(const std::wstring& new_val)
{
    m_name = new_val;
}


const std::wstring& IndexInfo::getName()
{
    return m_name;
}


void IndexInfo::setExpression(const std::wstring& new_val)
{
    m_expression = new_val;
}

const std::wstring& IndexInfo::getExpression()
{
    return m_expression;
}




