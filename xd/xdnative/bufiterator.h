/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-05-05
 *
 */


#ifndef H_XDNATIVE_STDITER_H
#define H_XDNATIVE_STDITER_H


#include "baseiterator.h"


class BufIterator : public BaseIterator
{
    XCM_CLASS_NAME("xdnative.BufIterator")
    XCM_BEGIN_INTERFACE_MAP(BufIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_CHAIN(BaseIterator)
    XCM_END_INTERFACE_MAP()

public:

    BufIterator();
    virtual ~BufIterator();

    bool init(XdnativeDatabase* database,
              IXdnativeSet* set);

    void setRowBuffer(unsigned char* buf, int row_width);

    void skip(int delta);
    void goFirst();

private:

    xd::IIteratorPtr clone();
    void goLast();
    xd::rowid_t getRowId();
    bool bof();
    bool eof();
    bool seek(const unsigned char* key, int length, bool soft);
    bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft);
    bool setPos(double pct);
    double getPos();

private:

    unsigned char* m_first_row;
    int m_row_width;
};



#endif
