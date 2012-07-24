/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-23
 *
 */


#ifndef __XDCOMMON_CMNBASEITERATOR_H
#define __XDCOMMON_CMNBASEITERATOR_H


// -- forwards --
class CmnBaseIteratorBindInfo;
namespace kscript
{
    class ExprParser;
}


class CommonBaseIterator : public tango::IIterator
{
public:

    CommonBaseIterator();
    ~CommonBaseIterator();

    virtual tango::IStructurePtr getParserStructure();
    kscript::ExprParser* parse(const std::wstring& expr);
    
    void setIteratorFlags(unsigned int mask, unsigned int value)
    {
    }

private:

    std::vector<CmnBaseIteratorBindInfo*> m_bindings;
};


#endif


