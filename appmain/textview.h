/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-05-01
 *
 */


#ifndef __APP_TEXTVIEW_H
#define __APP_TEXTVIEW_H


#include "appmain.h"


class TextViewModel
{
public:

    enum
    {
        lineFixed = 100,
        lineDelimited = 101
    };

public:

    TextViewModel();
    virtual ~TextViewModel();

    bool isOpen();
    bool openFile(const wxString& filename);
    void closeFile();

    wxString getFilename();
    xf_off_t getFileSize();
    int getFileType();

    wxChar getChar(xf_off_t row, xf_off_t col);
    bool isEof(xf_off_t offset);

    xf_off_t getColumnCount();
    xf_off_t getRowCount();

    wxString getLineDelimiters();
    xf_off_t getCurrentRowLength();     // returns the current row's length based on line-delimiter
    xf_off_t getRowWidth();             // returns the user-specified row width
    xf_off_t getSkipChars();
    
    void setCharEncoding(int char_encoding);
    void setLineDelimiters(const wxString& line_delimiters);
    void setRowWidth(xf_off_t row_width);
    void setSkipChars(xf_off_t skip_chars);
    void setFileType(int file_type);

private:

    wxChar getCharAtOffset(xf_off_t offset);
    wxChar getDelimitedChar(xf_off_t row, xf_off_t col);
    wxChar getFixedChar(xf_off_t row, xf_off_t col);

    int guessFileType();
    xf_off_t guessRowWidth();
    xf_off_t guessDelimitedRowWidth();
    xf_off_t guessFixedRowWidth();

    xf_off_t calcRowCount();

private:

    wxString m_file_name;       // file name
    xf_file_t m_file;           // file handle
    xf_off_t m_file_size;       // size of the file

    xf_off_t m_row_count;       // number of rows in the file
    xf_off_t m_cur_row;         // the current row that we're on
    xf_off_t m_cur_row_offset;  // offset where the current row begins
    xf_off_t m_cur_row_length;  // length of the current row

    xf_off_t m_chunk_size;      // size of data inside m_buf
    xf_off_t m_chunk_offset;    // offset of m_buf in the file

    int m_char_encoding;        // character encoding of the file
    wxString m_line_delimiters; // character array containing one or more delimiters
    xf_off_t m_row_width;       // user-specified width of each row
    xf_off_t m_skip_chars;      // number of chars to skip at the beginning of the file

    int m_file_type;            // fixed or line-delimited (see above)

    unsigned char* m_buf;       // buffer which holds the data
};




class TextViewColumn
{
public:

    TextViewColumn()
    {
        name = wxEmptyString;
        offset = 0;
        width = 0;
        encoding = tango::encodingASCII;
    }
    
    TextViewColumn(const TextViewColumn& c)
    {
        name = c.name;
        offset = c.offset;
        width = c.width;
        encoding = c.encoding;
    }
    
    TextViewColumn& operator=(const TextViewColumn& c)
    {
        name = c.name;
        offset = c.offset;
        width = c.width;
        encoding = c.encoding;
        return *this;
    }
    
public:

    wxString name;
    int offset;
    int width;
    int encoding;
};
    
    
class CaptionTextCtrl;
class TextView : public wxWindow,
                 public xcm::signal_sink
{
friend class CaptionTextCtrl;
friend class TextDoc;

public:

    TextView(wxWindow* parent,
             wxWindowID id,
             const wxPoint& position = wxDefaultPosition,
             const wxSize& = wxDefaultSize,
             long style = 0);

    ~TextView();

    bool openFile(const wxString& filename);
    void closeFile();
    
    void addColumn(int offset,
                   int encoding = -1,
                   const wxString& name = wxEmptyString);
                   
    void getColumns(std::vector<TextViewColumn>& cols);
    void deleteAllColumns();
    size_t getColumnIdxFromOffset(int offset);
    size_t getColumnCount();
    void refresh();

    int getCursorOffset();
    int getCursorColumn();
    int getCursorRow();

    void setDirty(bool dirty);
    bool isDirty();

    // -- TextViewModel pass through functions --
    void setCharEncoding(int char_encoding);
    void setSkipChars(xf_off_t skip_chars);
    void setRowWidth(xf_off_t row_width);
    void setFileType(int file_type);

    xf_off_t getModelSkipChars();
    xf_off_t getModelRowWidth();
    xf_off_t getModelRowCount();
    xf_off_t getModelFileSize();
    int getModelFileType();

public: // signals

    xcm::signal1<TextViewColumn /*column*/> sigColumnAdded;
    xcm::signal1<TextViewColumn /*column*/> sigColumnDeleted;
    xcm::signal2<TextViewColumn /*column*/, TextViewColumn /*new_settings*/> sigColumnModified;
    xcm::signal2<int /*new_cursor_offset*/, int /*new_cursor_row*/> sigCursorPositionChanged;
        
private:
    
    wxString getDefaultColumnName(int col_idx = -1);
    TextViewColumn* getBisectedColumn(int offset);
    TextViewColumn* getColumnFromOffset(int offset);

    bool modifyColumn(int col_idx,
                      int new_offset = -1,
                      int new_width = -1,
                      int new_encoding = -1,
                      const wxString& new_name = wxEmptyString);
    bool moveColumn(int col_idx, int new_offset);
    bool deleteColumn(int col_idx);

    void beginEdit(int col_idx);
    void endEdit(bool accept);
    bool isEditing();
    
    void scrollHorzToPosition(int pixel);
    void scrollHorz(int pixels);
    void scrollVert(int pixels);

    bool mouseInWindow();
    
    // -- drawing functions --
    void allocBitmap(int width, int height);
    void drawColumns(wxMemoryDC* memdc);
    void drawColumnCaptions(wxMemoryDC* memdc);
    void drawCursorLine(wxMemoryDC* memdc);
    void drawRowLines(wxMemoryDC* memdc);
    void drawData(wxMemoryDC* memdc);
    void drawInvalidArea(wxMemoryDC* memdc);
    void drawRuler(wxMemoryDC* memdc,
                   int major_line = 10,
                   int minor_line = 5,
                   bool show_all_ticks = true);

    void render();
    void repaint(wxDC* dc = NULL);
    void updateScrollbars(int scrollbar = -1);
    void recalcScrollbars();
        
    // event handlers
    void onPaint(wxPaintEvent& evt);
    void onScroll(wxScrollWinEvent& evt);
    void onMouseMove(wxMouseEvent& evt);
    void onMouseLeave(wxMouseEvent& evt);
    void onMouseLeftDblClick(wxMouseEvent& evt);
    void onMouseLeftDown(wxMouseEvent& evt);
    void onMouseLeftUp(wxMouseEvent& evt);
    void onMouseRightDown(wxMouseEvent& evt);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& evt);        // MSW only
    void onMouseCaptureChanged(wxMouseCaptureChangedEvent& evt);  // MSW only
    void onSize(wxSizeEvent& evt);
    void onScrollTimer(wxTimerEvent& evt);
    void onEditDestroy(const wxString& value,
                       int last_keycode,
                       bool focus_lost,
                       bool user_action);

private:

    TextViewModel* m_text_model;
    int m_default_char_encoding;

    wxTimer m_scroll_timer;     // for offset dragging outside client window
    wxControl* m_control;       // pointer to caption text control
    int m_edited_col_idx;       // index of the column caption we're editing
    bool m_was_editing;         // stored editing flag for onMouseLeftDown/Up()
    bool m_dragging;            // set to true if mouse is being dragging
    bool m_offset_deleting;     // set to true if offset is being deleted
    bool m_offset_dragged;      // set to true if offset is being dragged
    bool m_dirty;               // set to true if user made changes

    int m_cliwidth;             // client width
    int m_cliheight;            // client height

    int m_total_width;          // width of all characters
    int m_total_height;         // height of all characters
    int m_saved_total_height;   // saved total height (for line-delimited only)
    
    int m_pixel_xoff;           // horizontal pixel scrollbar position
    int m_pixel_yoff;           // vertical pixel scrollbar position
    int m_saved_pixel_yoff;     // saved pixel y-off (for line-delimited only)

    int m_char_width;           // character width
    int m_char_height;          // character height

    int m_vis_chars;            // number of visible chars (left to right)
    int m_vis_rows;             // number of visible rows

    int m_char_offset;          // number of chars we've scrolled horizontally
    int m_row_offset;           // number of rows we've scrolled vertically
    int m_saved_row_offset;     // saved row offset (for line-delimited only)

    int m_cursor_row;           // m_row_offset + client window cursor offset
    int m_cursor_offset;        // m_char_offset + client window cursor offset
    
    int m_cursor_x;             // client window cursor x-position
    int m_cursor_y;             // client window cursor y-position
    int m_cursor_down_x;        // client window cursor left down x-position
    int m_cursor_down_y;        // client window cursor left down y-position
    
    // -- column info --
    std::vector<TextViewColumn> m_cols;
    std::vector<TextViewColumn>::iterator m_dragged;

    // -- gui parameters --
    wxMemoryDC m_memdc;
    wxBitmap m_bmp;
    wxFont m_font;
    wxColour m_base_color;
    wxColour m_border_color;
    wxColour m_invalidarea_color;
    int m_bmp_alloc_width;
    int m_bmp_alloc_height;

    DECLARE_EVENT_TABLE()
};


// -- functor for sorting TextViewColumns --

class TextViewSort
{
public:

     bool operator()(const TextViewColumn& x,
                     const TextViewColumn& y) const                
     {
        return x.offset < y.offset ? true : false;
     }
};


#endif  // __APP_TEXTVIEW_H
