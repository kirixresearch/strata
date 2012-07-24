/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-03-16
 *
 */


#include "tango.h"
#include "indexinfo.h"


IndexInfo::IndexInfo()
{
}

IndexInfo::~IndexInfo()
{
}


void IndexInfo::setTag(const std::wstring& new_val)
{
    m_tag = new_val;
}


const std::wstring& IndexInfo::getTag()
{
    return m_tag;
}


void IndexInfo::setExpression(const std::wstring& new_val)
{
    m_expression = new_val;
}

const std::wstring& IndexInfo::getExpression()
{
    return m_expression;
}




