/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifndef __KXINTER_TEXTVIEW_H
#define __KXINTER_TEXTVIEW_H


#include "dataviewbase.h"


class DataDoc;

class TextView : public DataViewBase
{

public:

    TextView(wxWindow* parent,
             DataDoc* doc,
             kcl::RulerControl* top_ruler = NULL,
             wxStatusBar* status_bar = NULL);

    ~TextView();

    void updateScrollbars();
    int getLinesPerPage();
    void setColumnSelection(int offset, int length, int reclen);
    void getColumnSelection(int* offset, int* length, int* reclen);
    void clearColumnSelection();
    void refresh();
    void setOffset(int cur_line);

private:

    int positionToColumn(int x);
    int rowToPosition(int row);
    int positionToRow(int y);
    int columnToPosition(int col);
    void updateStatusBar(int field = -1);

private:

    void onPaint(wxPaintEvent& event);
    void onChar(wxKeyEvent& event);
    void onSize(wxSizeEvent& event);
    void onMouse(wxMouseEvent& event);
    void onScroll(wxScrollWinEvent& event);

private:

    enum
    {
        actionNone = 0,
        actionSelectBytes,
        actionSelectColumn
    };

    DataDoc* m_doc;
    unsigned char* m_buf;
    kcl::RulerControl* m_top_ruler;
    wxStatusBar* m_status_bar;

    int m_line_height;
    int m_char_width;
    int m_char_height;
    int m_offsettext_width;
    int m_offsettext_height;
    int m_cur_line;
    int m_xoffset;
    int m_max_lines;

    int m_colselect_offset;
    int m_colselect_length;
    int m_colselect_reclen;

    int m_mouse_action;
    int m_mouse_start;
    int m_mouse_end;

    DECLARE_EVENT_TABLE()
};



#endif

