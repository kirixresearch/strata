/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-12
 *
 */


#ifndef __KXINTER_DATAVIEWBASE_H
#define __KXINTER_DATAVIEWBASE_H


struct ColumnSelection
{
    int offset;         // horizontal offset (bytes)
    int length;         // column length (bytes);
};


struct DataSelection
{
    int offset;         // data offset (bytes)
    int length;         // data length (bytes)
};



class DataViewBase : public wxWindow
{
public:

    DataViewBase(wxWindow* parent, wxWindowID id, const wxPoint& position, const wxSize& size, unsigned int style) :
                wxWindow(parent, id, position, size, style)
    {
    }
};


#endif

