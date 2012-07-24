/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifndef __KXINTER_HEXVIEW_H
#define __KXINTER_HEXVIEW_H


#include "dataviewbase.h"


class DataDoc;

class HexView : public DataViewBase
{
    DataDoc* m_doc;
    unsigned char* m_buf;

    int m_line_height;
    int m_char_width;
    int m_xoffset;
    int m_cur_line;
    int m_max_lines;

public:

    HexView(wxWindow* parent,
            DataDoc* doc,
            kcl::RulerControl* top_ruler = NULL,
            wxStatusBar* status_bar = NULL);

    ~HexView();

    void updateScrollbars();
    int getLinesPerPage();
    void setOffset(int offset);

public:

    void onScroll(wxScrollWinEvent& event);
    void onPaint(wxPaintEvent& event);
    void onChar(wxKeyEvent& event);
    void onSize(wxSizeEvent& event);
    void onMouseWheel(wxMouseEvent& event);

    DECLARE_EVENT_TABLE()
};



#endif

