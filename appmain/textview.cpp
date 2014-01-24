/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-05-01
 *
 */


#include "appmain.h"
#include "textview.h"
#include "appcontroller.h"

const int TEXTVIEW_BUF_SIZE = 400000;  // buffer size used in TextViewModel

const int CAPTION_HEIGHT = 20;
const int MIN_CAPTION_TEXTCTRL_WIDTH = 100;
const int RULER_HEIGHT = 20;
const int RULER_LINE_HEIGHT_MAJOR = 6;
const int RULER_LINE_HEIGHT_MINOR = 8;
const int RULER_LINE_HEIGHT_TICK = 4;

const int ScrollTimerId = 54323;


// menu id's for popup column menu

enum
{
    ID_PopupMenu_RenameColumn = 31990,
    ID_PopupMenu_EncodingDefault,
    ID_PopupMenu_EncodingASCII,
    ID_PopupMenu_EncodingUCS2,
    ID_PopupMenu_EncodingEBCDIC,
    ID_PopupMenu_EncodingCOMP,
    ID_PopupMenu_EncodingCOMP3
};


// utility functions for TextView

static int offsetToPixels(int offset_idx, int scroll_offset_idx, int char_width)
{
    return (offset_idx-scroll_offset_idx)*char_width;
}

static int pixelsToOffset(int cursor_x, int scroll_xoff, int char_width)
{
    int x_off_mod = scroll_xoff % char_width;
    return (cursor_x+scroll_xoff+(char_width/2)-x_off_mod-1)/char_width;
}



static unsigned char ascii2ebcdic(unsigned char c)
{
    static unsigned char a2e[256] =
    {
          0,  1,  2,  3, 55, 45, 46, 47, 22,  5, 37, 11, 12, 13, 14, 15,
         16, 17, 18, 19, 60, 61, 50, 38, 24, 25, 63, 39, 28, 29, 30, 31,
         64, 79,127,123, 91,108, 80,125, 77, 93, 92, 78,107, 96, 75, 97,
        240,241,242,243,244,245,246,247,248,249,122, 94, 76,126,110,111,
        124,193,194,195,196,197,198,199,200,201,209,210,211,212,213,214,
        215,216,217,226,227,228,229,230,231,232,233, 74,224, 90, 95,109,
        121,129,130,131,132,133,134,135,136,137,145,146,147,148,149,150,
        151,152,153,162,163,164,165,166,167,168,169,192,106,208,161,  7,
         32, 33, 34, 35, 36, 21,  6, 23, 40, 41, 42, 43, 44,  9, 10, 27,
         48, 49, 26, 51, 52, 53, 54,  8, 56, 57, 58, 59,  4, 20, 62,225,
         65, 66, 67, 68, 69, 70, 71, 72, 73, 81, 82, 83, 84, 85, 86, 87,
         88, 89, 98, 99,100,101,102,103,104,105,112,113,114,115,116,117,
        118,119,120,128,138,139,140,141,142,143,144,154,155,156,157,158,
        159,160,170,171,172,173,174,175,176,177,178,179,180,181,182,183,
        184,185,186,187,188,189,190,191,202,203,204,205,206,207,218,219,
        220,221,222,223,234,235,236,237,238,239,250,251,252,253,254,255
    };

    return a2e[c];
}


static unsigned char ebcdic2ascii(unsigned char c)
{
    static unsigned char e2a[256] =
    {
          0,  1,  2,  3,156,  9,134,127,151,141,142, 11, 12, 13, 14, 15,
         16, 17, 18, 19,157,133,  8,135, 24, 25,146,143, 28, 29, 30, 31,
        128,129,130,131,132, 10, 23, 27,136,137,138,139,140,  5,  6,  7,
        144,145, 22,147,148,149,150,  4,152,153,154,155, 20, 21,158, 26,
         32,160,161,162,163,164,165,166,167,168, 91, 46, 60, 40, 43, 33,
         38,169,170,171,172,173,174,175,176,177, 93, 36, 42, 41, 59, 94,
         45, 47,178,179,180,181,182,183,184,185,124, 44, 37, 95, 62, 63,
        186,187,188,189,190,191,192,193,194, 96, 58, 35, 64, 39, 61, 34,
        195, 97, 98, 99,100,101,102,103,104,105,196,197,198,199,200,201,
        202,106,107,108,109,110,111,112,113,114,203,204,205,206,207,208,
        209,126,115,116,117,118,119,120,121,122,210,211,212,213,214,215,
        216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,
        123, 65, 66, 67, 68, 69, 70, 71, 72, 73,232,233,234,235,236,237,
        125, 74, 75, 76, 77, 78, 79, 80, 81, 82,238,239,240,241,242,243,
         92,159, 83, 84, 85, 86, 87, 88, 89, 90,244,245,246,247,248,249,
         48, 49, 50, 51, 52, 53, 54, 55, 56, 57,250,251,252,253,254,255
    };

    return e2a[c];
}

    

TextViewModel::TextViewModel()
{
    m_file = 0;
    m_file_size = 0;
    m_file_name = wxT("");
    m_row_count = 0;
    m_cur_row = 0;
    m_cur_row_offset = 0;

    m_char_encoding = xd::encodingASCII;
    m_line_delimiters = wxT("\x0a");
    m_row_width = 0;
    m_skip_chars = 0;

    m_file_type = lineFixed;

    m_chunk_size = 0;
    m_chunk_offset = 0;

    // initialize the buffer 
    m_buf = new unsigned char[TEXTVIEW_BUF_SIZE];
    memset(m_buf, 0, TEXTVIEW_BUF_SIZE);
}


TextViewModel::~TextViewModel()
{
    closeFile();

    delete[] m_buf;
}


bool TextViewModel::isOpen()
{
    if (!m_file)
        return false;

    return true;
}


bool TextViewModel::openFile(const wxString& filename)
{
    // we can only open one file at a time
    if (isOpen())
        return false;

    std::wstring wfilename = towstr(filename);

    m_file_size = xf_get_file_size(wfilename);
    m_file = xf_open(wfilename, xfOpen, xfRead, xfShareReadWrite);

    // if opening the file failed, bail out
    if (!m_file)
        return false;

    m_file_name = filename;

    // try to guess the file type
    m_file_type = guessFileType();

    // try to guess the row width
    m_row_width = guessRowWidth();

    return true;
}


void TextViewModel::closeFile()
{
    if (m_file)
    {
        xf_close(m_file);
        m_file = 0;
    }
}


int TextViewModel::getFileType()
{
    return m_file_type;
}


int TextViewModel::guessFileType()
{
    int i, delim_count = 0;

    for (i = 0; i < 400000; ++i)
    {
        if (isEof(i))
            break;

        if (m_line_delimiters.Find(getCharAtOffset(i)) != -1)
            delim_count++;
    }

    if (delim_count > 20)
        return lineDelimited;

    return lineFixed;
}


xf_off_t TextViewModel::guessRowWidth()
{
    if (m_file_type == lineDelimited)
        return guessDelimitedRowWidth();
     else if (m_file_type == lineFixed)
        return guessFixedRowWidth();

    return 80;
}

xf_off_t TextViewModel::guessDelimitedRowWidth()
{
    int i;
    int max_row_read = 200;

    xf_off_t row_width = 0;
    xf_off_t last_row_offset = 0;
    xf_off_t max_row_width = 1;

    for (i = 0; i < max_row_read; ++i)
    {
        getDelimitedChar(i, 0);

        row_width = m_cur_row_offset - last_row_offset;

        if (row_width > max_row_width)
            max_row_width = row_width;

        last_row_offset = m_cur_row_offset;
    }

    return max_row_width;
}

xf_off_t TextViewModel::guessFixedRowWidth()
{
    int i;
    int max_char_read = 20000;

    for (i = 0; i < max_char_read; ++i)
    {
        //  *ix text file format
        if (getCharAtOffset(i) == 0x0a)
            return i+1;

        // mac text file format
        if (getCharAtOffset(i) == 0x0d)
        {
            if (i+1 < max_char_read)
            {
                // windows format
                if (getCharAtOffset(i+1) == 0x0a)
                    return i+2;
            }

            return i+1;
        }
    }

    return 80;
}


wxChar TextViewModel::getCharAtOffset(xf_off_t offset)
{
    xf_off_t off = offset;
    off += m_skip_chars;

    if (off >= m_file_size)
        return wxT('\0');

    // if the character is in memory, return it
    if (off >= m_chunk_offset && off < m_chunk_offset+m_chunk_size)
        return (m_char_encoding == xd::encodingEBCDIC) ? ebcdic2ascii(m_buf[off-m_chunk_offset])
                                                       : m_buf[off-m_chunk_offset];

    // character was not in the buffer, so we need to read in a new chunk

    memset(m_buf, 0, TEXTVIEW_BUF_SIZE);

    m_chunk_offset = (off / TEXTVIEW_BUF_SIZE) * TEXTVIEW_BUF_SIZE;

    if (!xf_seek(m_file, m_chunk_offset, xfSeekSet))
        return wxT('\0');

    m_chunk_size = xf_read(m_file, m_buf, 1, TEXTVIEW_BUF_SIZE);

    // return the character (if not beyond EOF)
    if (off >= m_chunk_offset && off < m_chunk_offset+m_chunk_size)
        return (m_char_encoding == xd::encodingEBCDIC) ? ebcdic2ascii(m_buf[off-m_chunk_offset])
                                                       : m_buf[off-m_chunk_offset];
    
    return wxT('\0');
}


bool TextViewModel::isEof(xf_off_t offset)
{
    return (offset+m_skip_chars >= m_file_size) ? true : false;
}


xf_off_t TextViewModel::getCurrentRowLength()
{
    xf_off_t row_off = m_cur_row_offset;
    xf_off_t row_len = 0;

    while (1)
    {
        if (isEof(row_off))
            break;
        if (m_line_delimiters.Find(getCharAtOffset(row_off++)) != -1)
            break;

        ++row_len;
    }

    return row_len;
}


wxChar TextViewModel::getDelimitedChar(xf_off_t row, xf_off_t col)
{
    // if we want a character from the current row,
    // just get it from the buffer (and check to make
    // sure we are in the bounds of the line)
    if (m_cur_row == row)
    {
        if (col >= m_cur_row_length)
            return wxT('\0');
        return getCharAtOffset(m_cur_row_offset + col);
    }

    // this next chunk of code will scroll to the correct line.
    // if the desired line is ahead, we need to scroll forward,
    // otherwise we need to scroll backward

    xf_off_t off = m_cur_row_offset;
    int diff = row-m_cur_row;

    if (diff > 0)
    {
        // scroll forward
        int cnt = 0;
        while (diff > 0)
        {
            ++off;

            if (isEof(off))
                return wxT('\0');

            if (++cnt >= 16384)
            {
                // a line length of 16K or more gets increasingly improbable;
                // return zero -- otherwise we risk a program freeze
                return wxT('\0');
            }
            
            if (m_line_delimiters.Find(getCharAtOffset(off)) != -1)
                --diff;
            
            ++cnt;
        }

        ++off;
    }
     else
    {
        // scroll backward (need to go "one more" delimiter
        // to get to the beginning of the line)
        
        int cnt = 0;
        while (diff <= 0)
        {
            --off;

            if (off < 0)
            {
                ++diff;
                break;
            }
            
            if (++cnt >= 16384)
            {
                // a line length of 16K or more gets increasingly improbable;
                // return zero -- otherwise we risk a program freeze
                return wxT('\0');
            }

            if (m_line_delimiters.Find(getCharAtOffset(off)) != -1)
                ++diff;
            
            ++cnt;
        }

        ++off;
    }

    m_cur_row = row;
    m_cur_row_offset = off;


    // discover row length
    xf_off_t row_off = off;
    m_cur_row_length = getCurrentRowLength();

    // return the desired character from the line
    if (col >= m_cur_row_length)
        return wxT('\0');

    return getCharAtOffset(m_cur_row_offset + col);
}


wxChar TextViewModel::getFixedChar(xf_off_t row, xf_off_t col)
{
    m_cur_row = row;

    return getCharAtOffset((row*m_row_width) + col);
}


wxChar TextViewModel::getChar(xf_off_t row, xf_off_t col)
{
    if (!m_file || m_row_width == 0)
        return wxT('\0');

    if (m_file_type == lineDelimited)
        return getDelimitedChar(row, col);
     else if (m_file_type == lineFixed)
        return getFixedChar(row, col);

    return wxT('\0');
}


wxString TextViewModel::getFilename()
{
    return m_file_name;
}

xf_off_t TextViewModel::getFileSize()
{
    return m_file_size;
}

xf_off_t TextViewModel::getColumnCount()
{
    return m_row_width;
}

xf_off_t TextViewModel::getRowCount()
{
    // we can only accurately calculate the
    // row count on true fixed-length files
    if (m_file_type == lineFixed)
        return m_row_count;

    return -1;
}

wxString TextViewModel::getLineDelimiters()
{
    return m_line_delimiters;
}

xf_off_t TextViewModel::getRowWidth()
{
    return m_row_width;
}

xf_off_t TextViewModel::getSkipChars()
{
    return m_skip_chars;
}

void TextViewModel::setLineDelimiters(const wxString& line_delimiters)
{
    m_line_delimiters = line_delimiters;
}

void TextViewModel::setRowWidth(xf_off_t row_width)
{
    m_row_width = row_width;
    m_row_count = calcRowCount();
}

void TextViewModel::setCharEncoding(int char_encoding)
{
    m_char_encoding = char_encoding;
}

void TextViewModel::setSkipChars(xf_off_t skip_chars)
{
    m_skip_chars = skip_chars;
    
    if (m_file_type == lineDelimited)
    {
        // reset the view
        m_cur_row = 0;
        m_cur_row_offset = 0;
        m_cur_row_length = getCurrentRowLength();
    }

    m_row_count = calcRowCount();
}

void TextViewModel::setFileType(int file_type)
{
    m_cur_row = 0;
    m_cur_row_offset = 0;
    
    if (m_file_type == lineDelimited)
    {
        // reset the view
        m_cur_row_length = getCurrentRowLength();
    }

    m_file_type = file_type;
}

xf_off_t TextViewModel::calcRowCount()
{
    double fs;
    fs = m_file_size;
    fs -= m_skip_chars;
    return (xf_off_t)ceil(fs/m_row_width);
}


    
    
// CaptionTextCtrl class implementation

class CaptionTextCtrl : public wxTextCtrl
{

public:

    CaptionTextCtrl(TextView* textview,
                    wxWindow* parent,
                    wxWindowID id,
                    const wxString& value,
                    const wxPoint& pos,
                    const wxSize& size,
                    long style)
                : wxTextCtrl(parent, id, value, pos, size, style)
    {
        m_textview = textview;
        m_last_key = -1;
        m_focus_lost = false;
        m_user_action = true;
        m_deleting = false;
    }

    ~CaptionTextCtrl()
    {
        // attempt to tell grid to write out the value
        m_deleting = true;
        wxString value = GetValue();
        m_textview->onEditDestroy(value,
                                  m_last_key,
                                  m_focus_lost,
                                  m_user_action);
    }

    void destroyNow(bool accept)
    {
        if (!accept)
            m_last_key = WXK_ESCAPE;

        m_user_action = false;
        Destroy();
    }
    
private:

    void onKeyDown(wxKeyEvent& evt)
    {
        m_last_key = evt.GetKeyCode();
        if (m_last_key == WXK_RETURN       ||
            m_last_key == WXK_NUMPAD_ENTER ||
            m_last_key == WXK_TAB          ||
            m_last_key == WXK_ESCAPE)
        {
            if (m_last_key == WXK_NUMPAD_ENTER)
                m_last_key = WXK_RETURN;
                
            if (m_last_key == WXK_TAB && evt.ShiftDown())
                m_last_key = WXK_LEFT;
                
            if (m_last_key == WXK_TAB)
                m_last_key = WXK_RIGHT;
                
            Destroy();
            return;
        }
        
        evt.Skip();
    }

    void onChar(wxKeyEvent& evt)
    {
        wxTextCtrl::OnChar(evt);

        // fire an event to ourselves
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, 50000);
        ::wxPostEvent(this, e);
    }

    void onTextChanged(wxCommandEvent& evt)
    {
        //m_textview->onEditChanged(GetValue());
    }

    void onKillFocus(wxFocusEvent& evt)
    {
        wxWindow* focus_wnd = wxWindow::FindFocus();
        
        if (focus_wnd != NULL &&
            focus_wnd != (wxWindow*)m_textview &&
            !m_deleting)
        {
            m_last_key = -1;
            m_focus_lost = true;
            m_user_action = false;

            Destroy();
        }
    }
    
private:

    TextView* m_textview;
    int m_last_key;
    bool m_focus_lost;
    bool m_user_action;
    bool m_deleting;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(CaptionTextCtrl, wxTextCtrl)
    EVT_KEY_DOWN(CaptionTextCtrl::onKeyDown)
    EVT_CHAR(CaptionTextCtrl::onChar)
    EVT_MENU(50000, CaptionTextCtrl::onTextChanged)
    EVT_KILL_FOCUS(CaptionTextCtrl::onKillFocus)
END_EVENT_TABLE()
    
    
// TextView class implementation

BEGIN_EVENT_TABLE(TextView, wxWindow)
    EVT_PAINT(TextView::onPaint)
    EVT_SCROLLWIN(TextView::onScroll)
    EVT_MOTION(TextView::onMouseMove)
    EVT_LEAVE_WINDOW(TextView::onMouseLeave)
    EVT_LEFT_DCLICK(TextView::onMouseLeftDblClick)
    EVT_LEFT_DOWN(TextView::onMouseLeftDown)
    EVT_LEFT_UP(TextView::onMouseLeftUp)
    EVT_RIGHT_DOWN(TextView::onMouseRightDown)
    EVT_MOUSE_CAPTURE_LOST(TextView::onMouseCaptureLost)
    EVT_MOUSE_CAPTURE_CHANGED(TextView::onMouseCaptureChanged)
    EVT_TIMER(ScrollTimerId, TextView::onScrollTimer)
    EVT_SIZE(TextView::onSize)
END_EVENT_TABLE()


TextView::TextView(wxWindow* parent,
                   wxWindowID id,
                   const wxPoint& position,
                   const wxSize& size,
                   long style) 
                   : wxWindow(parent, id, position, size, style)
{
    GetClientSize(&m_cliwidth, &m_cliheight);

    m_dirty = false;

    m_default_char_encoding = xd::encodingASCII;

    m_cursor_x = -1;
    m_cursor_y = -1;
    m_cursor_down_x = -1;
    m_cursor_down_y = -1;
    m_cursor_offset = 0;
    m_cursor_row = -1;
    m_edited_col_idx = -1;
    m_was_editing = false;
    m_dragging = false;
    m_offset_dragged = false;
    m_dragged = m_cols.end();

    m_control = NULL;
    m_offset_deleting = false;

    m_pixel_xoff = 0;
    m_pixel_yoff = 0;
    m_saved_pixel_yoff = 0;
    
    m_char_offset = 0;
    m_row_offset = 0;
    m_saved_row_offset = 0;

    m_char_width = 0;
    m_char_height = 0;

    m_bmp_alloc_width = 0;
    m_bmp_alloc_height = 0;

    // create the textview model for file access
    m_text_model = new TextViewModel;

    // allocate the bitmap for bit blitting
    allocBitmap(100, 100);

    m_font = wxFont(10, wxMODERN, wxNORMAL, wxNORMAL);
    m_base_color = kcl::getBaseColor();
    m_border_color = kcl::getBorderColor();

    // get the grid's default invalid area color
    m_invalidarea_color = getAppPrefsColor(wxT("grid.invalid_area_color"));

    // get the character width so we can draw the ruler tick marks
    GetTextExtent(wxT("X"),
                  &m_char_width,
                  &m_char_height,
                  NULL,
                  NULL,
                  &m_font);

    // add a little spacing around the characters
    m_char_width += 2;
    m_char_height += 3;

    // initialize a default total heigh and total width
    m_total_width = m_char_width*200;
    m_total_height = m_char_height*200;
    m_saved_total_height = -1;

    // initialize our visible row and column count
    m_vis_rows = m_cliheight/m_char_height;
    m_vis_chars = m_cliwidth/m_char_width;

    m_scroll_timer.SetOwner(this, ScrollTimerId);
}

TextView::~TextView()
{
    if (m_text_model->isOpen())
    {
        m_text_model->closeFile();
    }

    delete m_text_model;
}

bool TextView::openFile(const wxString& filename)
{
    bool retval = m_text_model->openFile(filename);

    if (retval)
    {
        setSkipChars(getModelSkipChars());
        setRowWidth(getModelRowWidth());
        
        // remove any existing columns from the TextView
        deleteAllColumns();
        
        // add a default column to the TextView
        addColumn(0);
        
        // refresh the view
        refresh();
    }
       
    return retval;
}

void TextView::closeFile()
{
    if (m_text_model->isOpen())
    {
        m_text_model->closeFile();

        m_pixel_xoff = 0;
        m_pixel_yoff = 0;
        m_row_offset = 0;
        m_char_offset = 0;
        m_total_width = 0;
        m_total_height = 0;
    }
}

void TextView::getColumns(std::vector<TextViewColumn>& cols)
{
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        cols.push_back(*it);
    }
}

size_t TextView::getColumnCount()
{
    return m_cols.size();
}

void TextView::refresh()
{
    recalcScrollbars();
    render();
    repaint();
}

void TextView::setCharEncoding(int char_encoding)
{
    m_default_char_encoding = char_encoding;
    
    m_text_model->setCharEncoding(char_encoding);
    refresh();
}

void TextView::setSkipChars(xf_off_t skip_chars)
{
    m_text_model->setSkipChars(skip_chars);

    // the refresh() here is to counteract a line drawing problem where
    // the first line of the line-delimited file does not initially get
    // drawn if the first character in the line is a line delimiter...
    // this refresh() should eventually be removed
    refresh();
}

void TextView::setRowWidth(xf_off_t row_width)
{
    m_text_model->setRowWidth(row_width);

    // now delete any breaks greater than the row width

    std::vector<TextViewColumn> temp;
    
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        if (it->offset < row_width)
            temp.push_back(*it);
    }

    // adjust the width of the last column
    if (temp.size() > 0)
    {
        it = temp.begin()+temp.size()-1;
        it->width = row_width-it->offset;
    }
    
    if (temp.size() != m_cols.size())
        setDirty(true);

    m_cols = temp;
    m_dragged = m_cols.end();

    int old_pixel_xoff = m_pixel_xoff;
    updateScrollbars();
    scrollHorzToPosition(old_pixel_xoff);
}

void TextView::setFileType(int file_type)
{
    // save the y-offset, current row and total height
    int curtype = m_text_model->getFileType();
    if (curtype == TextViewModel::lineDelimited)
    {
        m_saved_pixel_yoff = m_pixel_yoff;
        m_saved_row_offset = m_row_offset;
        m_saved_total_height = m_total_height;
    }
    
    m_text_model->setFileType(file_type);
    refresh();
}

xf_off_t TextView::getModelSkipChars()
{
    return m_text_model->getSkipChars();
}

xf_off_t TextView::getModelRowWidth()
{
    return m_text_model->getRowWidth();
}

xf_off_t TextView::getModelRowCount()
{
    return m_text_model->getRowCount();
}

xf_off_t TextView::getModelFileSize()
{
    return m_text_model->getFileSize();
}
    
int TextView::getModelFileType()
{
    return m_text_model->getFileType();
}
    
int TextView::getCursorColumn()
{
    int col_left_pixel;
    int col_right_pixel;
    
    int idx = 0;
    int offset = 0;
    
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        offset = it->offset;
        col_left_pixel = offsetToPixels(offset, m_char_offset, m_char_width);
        offset += it->width;
        col_right_pixel = offsetToPixels(offset, m_char_offset, m_char_width);
        
        if (m_cursor_x >= col_left_pixel &&
            m_cursor_x < col_right_pixel)
        {
            return idx;
        }
        
        idx++;
    }
    
    return -1;
}

int TextView::getCursorOffset()
{
    return m_cursor_offset;
}

int TextView::getCursorRow()
{
    return m_cursor_row;
}

wxString TextView::getDefaultColumnName(int col_idx)
{
    if (col_idx == -1)
        col_idx = m_cols.size()+1;
        
    wxString name = wxString::Format(_("Field%d"), col_idx);
    
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        if (it->name.CmpNoCase(name) == 0)
            return getDefaultColumnName(col_idx+1);
    }
    
    return name;
}

TextViewColumn* TextView::getBisectedColumn(int offset)
{
    int idx = 0;
    int left_bound, right_bound;
    
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        left_bound = it->offset;
        right_bound = (it->offset)+(it->width);
        
        if ((offset > left_bound) && (offset < right_bound))
            return &(m_cols[idx]);
        
        idx++;
    }
    
    return NULL;
}

TextViewColumn* TextView::getColumnFromOffset(int offset)
{
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        if (offset == it->offset)
            return &(*it);
    }
    
    return NULL;
}

size_t TextView::getColumnIdxFromOffset(int offset)
{
    size_t idx = 0;
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        if (offset == it->offset)
            return idx;
        
        idx++;
    }
    
    return -1;
}

void TextView::addColumn(int offset, int encoding, const wxString& name)
{
    if (encoding == -1)
        encoding = m_default_char_encoding;
        
    // we have deleted all the columns, so just add the column
    if (m_cols.size() == 0)
    {
        TextViewColumn col;
        
        // we need to add a default first column because the specified
        // offset was not 0 and there are no fields yet
        if (offset != 0)
        {
            col.offset = 0;
            col.width = offset;
            col.name = getDefaultColumnName();
            m_cols.push_back(col);
            sigColumnAdded(col);
        }
        
        // create the new column
        col.offset = offset;
        col.width = (int)getModelRowWidth();
        col.encoding = encoding;
        col.name = name.Length() == 0 ? getDefaultColumnName() : name;
        m_cols.push_back(col);

        // make sure the vector is sorted
        std::sort(m_cols.begin(), m_cols.end(), TextViewSort());
        
        // make sure we know we're not dragging anything
        m_dragged = m_cols.end();
        
        setDirty(true);
        sigColumnAdded(col);
        return;
    }

    // look for the offset we're trying to add
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        // if this column offset already exists, bail out
        if (offset == it->offset)
            return;
    }

    // we're trying to add a column offset at the end of the row, bail out
    if (offset >= (int)getModelRowWidth())
        return;
        
    // find out which column we're bisecting
    TextViewColumn* bisected = getBisectedColumn(offset);
    
    // we don't have a bisected column... something went way wrong, bail out
    if (bisected == NULL)
        return;
    
    // save the bisected column's width for use below
    int saved_bisected_offset = bisected->offset;
    int saved_bisected_width = bisected->width;
    int new_bisected_width = offset-(bisected->offset);
    
    // change the bisected column's width (this will also fire a signal)
    int bisected_idx = getColumnIdxFromOffset(bisected->offset);
    modifyColumn(bisected_idx, -1, new_bisected_width);
    
    // create the new column
    TextViewColumn col;
    col.offset = offset;
    col.name = name.IsEmpty() ? getDefaultColumnName() : name;
    col.width = saved_bisected_offset+saved_bisected_width-offset;
    col.encoding = encoding;
    m_cols.push_back(col);

    // make sure the vector is sorted
    std::sort(m_cols.begin(), m_cols.end(), TextViewSort());
    
    // make sure we know we're not dragging anything
    m_dragged = m_cols.end();
    
    setDirty(true);
    sigColumnAdded(col);
}

bool TextView::modifyColumn(int col_idx,
                            int new_offset,
                            int new_width,
                            int new_encoding,
                            const wxString& new_name)
{
    if (col_idx < 0 || (size_t)col_idx >= m_cols.size())
        return false;
    
    TextViewColumn new_settings;
    new_settings.offset = new_offset;
    new_settings.width = new_width;
    new_settings.encoding = new_encoding;
    new_settings.name = new_name;
    
    // fire a signal that the column was modified
    sigColumnModified(m_cols[col_idx], new_settings);
    
    if (new_offset != -1)
        m_cols[col_idx].offset = new_offset;
    if (new_width != -1)
        m_cols[col_idx].width = new_width;
    if (new_encoding != -1)
        m_cols[col_idx].encoding = new_encoding;
    if (!new_name.IsEmpty())
        m_cols[col_idx].name = new_name;
    
    if (new_offset != -1 || new_width != -1 || new_encoding != -1 || !new_name.IsEmpty())
        setDirty(true);
    
    return true;
}

bool TextView::moveColumn(int col_idx, int new_offset)
{
    // don't allow the user to drag the column offset past the row width
    if (new_offset >= getModelRowWidth())
        return false;

    // it is impossible for the user to drag the first columns offset, so if
    // the colidx == 0, something went wrong, bail out
    if (col_idx == 0)
        return false;

    std::vector<TextViewColumn>::iterator it = m_cols.begin()+col_idx;
    std::vector<TextViewColumn>::iterator left_it = it-1;
    int left_bound = left_it->offset;
    int right_bound = (it->offset)+(it->width);

    // the user tried to drag the offset past the left or right
    // boundary offsets, so set the new offset to be the "stop"
    // offset (either left+1 or right-1)
    if (new_offset <= left_bound)
        new_offset = left_bound+1;
    if (new_offset >= right_bound)
        new_offset = right_bound-1;

    // the dragged column offset was moved within
    // the boundaries of the surrounding offsets
    int new_width = right_bound-new_offset;
    int coltoleft_newwidth = new_offset-left_bound;
    int coltoleft_idx = getColumnIdxFromOffset(left_it->offset);
    modifyColumn(col_idx, new_offset, new_width);
    modifyColumn(coltoleft_idx, -1, coltoleft_newwidth);
    return true;

// THIS CODE DOESN'T APPLY ANYMORE SINCE THE USER CAN'T DRAG AN OFFSET
// OUTSIDE OF IT'S SURROUNDING OFFSETS        
/*
    // the offset has been dropped over an existing offset, in which case
    // we'll delete the offset we dragged and the column associated with
    // the drop offset will assume the dragged column's name
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        if ((it->offset == new_offset) && (it != m_dragged))
        {
            int drop_idx = getColumnIdxFromOffset(new_offset);
            wxString new_name = m_dragged->name;
            
            modifyColumn(drop_idx, -1, -1, new_name);
            deleteColumn(col_idx);

            // make sure we know we're not dragging anything anymore
            m_dragged = m_cols.end();

            std::sort(m_cols.begin(), m_cols.end(), TextViewSort());
            setDirty(true);
            return true;
        }
    }

    it = m_cols.begin()+col_idx;
    std::vector<TextViewColumn>::iterator left_it = it-1;
    
    TextViewColumn* bisected_col = getBisectedColumn(new_offset);
    int bisected_idx = getColumnIdxFromOffset(bisected_col->offset);
    int coltoleft_idx = getColumnIdxFromOffset(left_it->offset);
    
    int left_bound = left_it->offset;
    int right_bound = (it->offset)+(it->width);

    if (new_offset < left_bound || new_offset >= right_bound)
    {
        // the dragged offset was moved outside
        // the boundaries of its surrounding offsets
        
        int bisected_newwidth = new_offset-(bisected_col->offset);
        int new_width = (bisected_col->offset)+(bisected_col->width)-new_offset;
        int coltoleft_newwidth = right_bound-left_bound;
        
        modifyColumn(bisected_idx, -1, bisected_newwidth);
        modifyColumn(col_idx, new_offset, new_width);
        modifyColumn(coltoleft_idx, -1, coltoleft_newwidth);
    }
     else
    {
        // the dragged column offset was moved within
        // the boundaries of the surrounding offsets
        
        int new_width = right_bound-new_offset;
        int coltoleft_newwidth = new_offset-left_bound;
        
        modifyColumn(col_idx, new_offset, new_width);
        modifyColumn(coltoleft_idx, -1, coltoleft_newwidth);
    }
    
    // make sure we know we're not dragging anything anymore
    m_dragged = m_cols.end();

    std::sort(m_cols.begin(), m_cols.end(), TextViewSort());
    setDirty(true);
    return true;
*/
}

bool TextView::deleteColumn(int col_idx)
{
    if (col_idx < 0 || col_idx >= (int)m_cols.size())
        return false;

    // update the width of the column to the
    // left of the column we're about to delete
    if (col_idx > 0)
    {
        std::vector<TextViewColumn>::iterator left_it;
        left_it = m_cols.begin()+col_idx-1;
        
        int coltoleft_idx = getColumnIdxFromOffset(left_it->offset);
        int coltoleft_newwidth = (left_it->width)+(m_cols[col_idx].width);
        modifyColumn(coltoleft_idx, -1, coltoleft_newwidth);
    }
    
    // we need to fire this signal before we actually delete the column
    sigColumnDeleted(m_cols[col_idx]);

    // remove the column from the vector
    m_cols.erase(m_cols.begin()+col_idx);
    m_offset_deleting = true;

    // make sure we know we're not dragging anything
    m_dragged = m_cols.end();

    std::sort(m_cols.begin(), m_cols.end(), TextViewSort());
    setDirty(true);
    return true;
}

void TextView::deleteAllColumns()
{
    // no signals are fired here because, in the case where we delete
    // a large number of columns, we don't want to fire a signal
    // every time one of the columns is deleted
    m_cols.clear();
    m_dragged = m_cols.end();
    setDirty(true);
}

bool TextView::isDirty()
{
    return m_dirty;
}

void TextView::setDirty(bool dirty)
{
    m_dirty = dirty;
}

void TextView::allocBitmap(int width, int height)
{
    if (width < 16)
        width = 16;

    if (height < 16)
        height = 16;

    if (width <= m_bmp_alloc_width && height <= m_bmp_alloc_height)
        return;

    m_bmp.Create(width, height, -1);
    m_memdc.SelectObject(m_bmp);

    m_bmp_alloc_width = width;
    m_bmp_alloc_height = height;
}

void TextView::drawRuler(wxMemoryDC* memdc,
                         int major_line,
                         int minor_line,
                         bool show_all_ticks)
{
    int x = -1;
    int y = m_cliheight-RULER_HEIGHT;
    
    // draw tick line header background
    memdc->SetPen(m_border_color);
    memdc->SetBrush(*wxTRANSPARENT_BRUSH);
    memdc->GradientFillLinear(wxRect(x, y, m_cliwidth+2, RULER_HEIGHT),
                              m_base_color,
                              kcl::stepColor(m_base_color, 170),
                              wxSOUTH);
    memdc->DrawRectangle(x, y, m_cliwidth+2, RULER_HEIGHT+2);

    memdc->SetPen(*wxBLACK);
    
    wxFont ruler_font = wxFont(*wxNORMAL_FONT);
    ruler_font.SetPointSize(8);
    m_memdc.SetFont(ruler_font);

    int col_idx, col_x;
    for (col_idx = m_char_offset+1;
         col_idx < m_char_offset+m_vis_chars+2;
         ++col_idx)
    {
        col_x = offsetToPixels(col_idx, m_char_offset, m_char_width);

        if (col_idx % major_line == 0)
        {
            memdc->DrawLine(col_x, y,
                            col_x, y+RULER_LINE_HEIGHT_MAJOR);

            int w, h;
            wxString s = wxString::Format(wxT("%d"), col_idx);
            GetTextExtent(s, &w, &h, NULL, NULL, &ruler_font);

            m_memdc.DrawText(s,
                             col_x-(w/2)+1,
                             y+RULER_LINE_HEIGHT_MAJOR);
        }

        if (col_idx % minor_line == 0)
        {
            memdc->DrawLine(col_x, y, col_x, y+RULER_LINE_HEIGHT_MINOR);
        }

        memdc->DrawLine(col_x, y, col_x, y+RULER_LINE_HEIGHT_TICK);
    }

    m_memdc.SetFont(m_font);
}

void TextView::drawColumns(wxMemoryDC* memdc)
{
    int col_x, lower_bound, upper_bound;

    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        // don't draw the first column's offset line
        if (it == m_cols.begin())
            continue;
            
        col_x = offsetToPixels(it->offset, m_char_offset, m_char_width);
        lower_bound = col_x-(m_char_width/2);
        upper_bound = col_x+(m_char_width/2);

        // the column break is outside the client area, don't draw it
        if (col_x < 0 || col_x >= m_cliwidth)
            continue;

        // if we're dragging a break, don't draw it
        if (m_offset_dragged && it == m_dragged)
            continue;

        // draw the line
        memdc->SetPen(*wxMEDIUM_GREY_PEN);
        memdc->SetBrush(*wxWHITE_BRUSH);
        memdc->DrawLine(col_x, CAPTION_HEIGHT-1, col_x, m_cliheight+2);

        // if we aren't dragging and we're hovering over
        // the column handle, draw a bold column handle
        if (m_cursor_x > lower_bound && m_cursor_x <= upper_bound)
        {
            if (!m_dragging && !isEditing())
            {
                memdc->SetPen(*wxBLACK);
                memdc->DrawLine(col_x+1,
                                CAPTION_HEIGHT-1,
                                col_x+1,
                                m_cliheight+2);
            }
        }
    }
}

void TextView::drawCursorLine(wxMemoryDC* memdc)
{
    // if we're editing a column caption, don't draw the cursor line
    if (isEditing())
        return;

    unsigned char stipple_data[2];
    stipple_data[0] = 0xaa;
    stipple_data[1] = 0x55;
    wxBitmap stipple_bitmap((char*)stipple_data, 8, 2, 1);

    wxPen* pen = new wxPen(*wxBLACK, 1, wxSTIPPLE);

#ifdef __WXMSW__
    // this only works on wxWidgets in MSW
    pen->SetStipple(stipple_bitmap);
#endif

    memdc->SetPen(*pen);

    int col_x, col_y, col_height, col_idx, lower_bound, upper_bound;
                
    for (col_idx = m_char_offset+1;
         col_idx < m_char_offset+m_vis_chars+2;
         ++col_idx)
    {
        col_x = offsetToPixels(col_idx, m_char_offset, m_char_width);
        col_y = CAPTION_HEIGHT-2;
        col_height = m_cliheight+2;
        
        lower_bound = col_x-(m_char_width/2);
        upper_bound = col_x+(m_char_width/2);

        if (m_cursor_x > lower_bound && m_cursor_x <= upper_bound)
        {
            if (m_dragging && m_dragged != m_cols.end())
            {
                // we can't drag the first columns offset, bail out
                if (m_dragged == m_cols.begin())
                    return;

                int left_offset_bound = (m_dragged-1)->offset;
                int right_offset_bound = (m_dragged->offset)+(m_dragged->width);
                int cursor_offset = getCursorOffset();
                
                // only draw the dragged line within the bounds
                // of the surrounding column offsets
                if (cursor_offset > left_offset_bound &&
                    cursor_offset < right_offset_bound)
                {
                    memdc->DrawLine(col_x, col_y, col_x, col_height);
                    memdc->DrawLine(col_x+1, col_y, col_x+1, col_height);
                }
                
                // make the left offset bound a "stop" for the dragged line
                if (cursor_offset <= left_offset_bound)
                {
                    col_x = offsetToPixels(left_offset_bound+1,
                                           m_char_offset,
                                           m_char_width);
                    memdc->DrawLine(col_x, col_y, col_x, col_height);
                    memdc->DrawLine(col_x+1, col_y, col_x+1, col_height);
                }
                
                // make the right offset bound a "stop" for the dragged line
                if (cursor_offset >= right_offset_bound)
                {
                    col_x = offsetToPixels(right_offset_bound-1,
                                           m_char_offset,
                                           m_char_width);
                    memdc->DrawLine(col_x, col_y, col_x, col_height);
                    memdc->DrawLine(col_x+1, col_y, col_x+1, col_height);
                }
            }
             else
            {
                // we're not dragging an offset, so draw a normal stipple line
                memdc->DrawLine(col_x, col_y, col_x, col_height);
            }

            break;
        }
    }

    delete pen;
}

void TextView::drawColumnCaptions(wxMemoryDC* memdc)
{
    // draw header background
    memdc->SetPen(m_border_color);
    memdc->SetBrush(*wxTRANSPARENT_BRUSH);
    memdc->SetFont(*wxNORMAL_FONT);
    memdc->GradientFillLinear(wxRect(-1,0,m_cliwidth+2, CAPTION_HEIGHT),
                              m_base_color,
                              kcl::stepColor(m_base_color, 170),
                              wxNORTH);
    memdc->DrawRectangle(-1, -1, m_cliwidth+2, CAPTION_HEIGHT+1);

    int col_left, col_right;
    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        col_left = offsetToPixels(it->offset,
                                  m_char_offset,
                                  m_char_width);
        col_right = offsetToPixels(it->offset+it->width,
                                   m_char_offset,
                                   m_char_width);

        // the column caption is outside the client area, don't draw it
        if (col_right < 0)
            continue;

        // the column caption is outside the client area, don't draw it
        if (col_left >= m_cliwidth)
            continue;
 
        memdc->SetTextForeground(*wxBLACK);

        // if we're dragging a break, draw the header slightly lighter
        if (m_offset_dragged && (it == m_dragged) && !isEditing())
        {
            wxColor lighter = kcl::stepColor(*wxBLACK, 170);
            memdc->SetTextForeground(lighter);
        }

        memdc->SetClippingRegion(col_left, 0, col_right-col_left-2, CAPTION_HEIGHT);
        memdc->DrawText(it->name, col_left+4, 3);
        memdc->DestroyClippingRegion();
        
        // draw the vertical caption border line
        memdc->DrawLine(col_right, 0, col_right, CAPTION_HEIGHT);
    }
    
    // set the text foreground color and font back to their stored values
    memdc->SetTextForeground(*wxBLACK);
    memdc->SetFont(m_font);
}

void TextView::drawRowLines(wxMemoryDC* memdc)
{
    memdc->SetPen(wxPen(wxColor(232,232,232)));

    int row;
    int row_y;
    for (row = (m_row_offset+2); row <= (m_row_offset+m_vis_rows); ++row)
    {
        row_y = (row-m_row_offset)*m_char_height+m_char_height/4;
        memdc->DrawLine(0, row_y, m_cliwidth+2, row_y);
    }

}

void TextView::drawData(wxMemoryDC* memdc)
{
    // if we haven't opened a file yet, no need to draw anything
    if (!m_text_model->isOpen())
        return;

    m_memdc.SetFont(m_font);

    wxChar ch;
    int col_x, row, col;
    
    int filetype = getModelFileType(); // fixed-length or line-delimited
    int rowwidth = getModelRowWidth();
    
    for (row = m_row_offset; row < m_row_offset+m_vis_rows; ++row)
    {
        // don't draw past the end of the file
        if (row*m_char_height >= m_total_height)
            break;

        for (col = m_char_offset; col < m_char_offset+m_vis_chars; ++col)
        {
            // don't draw too many characters in a given row
            if ((col+1)*m_char_width >= m_total_width)
                continue;

            // if we're in fixed-length mode and the character we want
            // to draw is past the row width, don't draw it
            if (filetype == TextViewModel::lineFixed && (col+1) > rowwidth)
                continue;
                
            ch = m_text_model->getChar(row, col);

            col_x = offsetToPixels(col, m_char_offset, m_char_width);

            if (ch != 0 &&
                ch != 0x0a &&
                ch != 0x0d)
            {
                m_memdc.DrawText(ch,
                                 col_x+1,
                                 CAPTION_HEIGHT+2+(row-m_row_offset)*m_char_height);
            }
        }
    }
}

void TextView::drawInvalidArea(wxMemoryDC* memdc)
{
    int invalidarea_x = offsetToPixels(getModelRowWidth(),
                                       m_char_offset,
                                       m_char_width);
    
    m_memdc.SetPen(m_invalidarea_color);
    m_memdc.SetBrush(m_invalidarea_color);
    m_memdc.DrawRectangle(invalidarea_x,
                          CAPTION_HEIGHT,
                          m_cliwidth-invalidarea_x+2,
                          m_cliheight-CAPTION_HEIGHT+2);
}

void TextView::render()
{
    allocBitmap(m_cliwidth + 100, m_cliheight);

    // draw white background
    m_memdc.SetPen(*wxWHITE_PEN);
    m_memdc.SetBrush(*wxWHITE_BRUSH);
    m_memdc.SetTextForeground(*wxBLACK);
    m_memdc.DrawRectangle(-1, -1, m_cliwidth+2, m_cliheight+2);

    //drawRowLines(&m_memdc);

    if (mouseInWindow() && !m_offset_deleting &&
        m_cursor_y > CAPTION_HEIGHT+2)
        drawCursorLine(&m_memdc);

    drawData(&m_memdc);
    drawColumns(&m_memdc);
    drawInvalidArea(&m_memdc);
    drawColumnCaptions(&m_memdc);
    drawRuler(&m_memdc, 10, 5);

    updateScrollbars();
}

void TextView::repaint(wxDC* dc)
{
    int x, y, w, h;

    x = 0;
    y = 0;
    w = m_cliwidth;
    h = m_cliheight;

    if (!m_bmp.Ok())
    {
        if (dc)
        {
            // if there is no bitmap
            dc->SetBrush(*wxWHITE_BRUSH);
            dc->SetPen(*wxWHITE_PEN);
            dc->DrawRectangle(x,y,w,h);
        }
         else
        {
            wxClientDC cdc(this);
            cdc.SetBrush(*wxWHITE_BRUSH);
            cdc.SetPen(*wxWHITE_PEN);
            cdc.DrawRectangle(x,y,w,h);
        }
        return;
    }

    if (dc)
    {
        dc->Blit(x, y, w, h, &m_memdc, x, y);
    }
     else
    {
        wxClientDC cdc(this);
        cdc.Blit(x, y, w, h, &m_memdc, x, y);
    }
}

void TextView::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    repaint(&dc);
}

void TextView::recalcScrollbars()
{
    // calculated the total width -- make sure that the last column's
    // whole caption text control can be shown on the screen
    m_total_width = m_char_width*(getModelRowWidth()+1);
    m_total_width += MIN_CAPTION_TEXTCTRL_WIDTH;
    
    // calculate the total height
    if (getModelFileType() == TextViewModel::lineFixed)
    {
        m_total_height = m_char_height*(getModelRowCount()+1);
        m_total_height += CAPTION_HEIGHT;
        m_total_height += RULER_HEIGHT;
    }
     else
    {
        // we're not switching file types (from fixed-length
        // to line-delimited), so grow the scrollbar
        if (m_saved_pixel_yoff == -1 &&
            m_saved_row_offset == -1 &&
            m_saved_total_height == -1)
        {
            int new_total_height = m_total_height;
            
            // find the offset of the last character on screen
            int last_vis_row = m_row_offset+m_vis_rows+1;
            xf_off_t offset = last_vis_row*getModelRowWidth();
            
            // set the new total height
            if (!m_text_model->isEof(offset))
                new_total_height = (last_vis_row+1000)*m_char_height;

            // grow the scrollbar
            if (new_total_height > m_total_height)
                m_total_height = new_total_height;
        }
         else
        {
            m_pixel_yoff = m_saved_pixel_yoff;
            m_row_offset = m_saved_row_offset;
            m_total_height = m_saved_total_height;
            
            m_saved_pixel_yoff = -1;
            m_saved_row_offset = -1;
            m_saved_total_height = -1;
        }
    }

    if (m_pixel_xoff+m_cliwidth > m_total_width)
        m_pixel_xoff = m_total_width-m_cliwidth;

    if (m_pixel_xoff < 0)
        m_pixel_xoff = 0;
        
    if (m_pixel_yoff+m_cliheight > m_total_height)
        m_pixel_yoff = m_total_height-m_cliheight;

    if (m_pixel_yoff < 0)
        m_pixel_yoff = 0;

    // update m_row_offset (m_pixel_yoff may have been changed above)
    m_row_offset = m_pixel_yoff/m_char_height;
}

void TextView::updateScrollbars(int scrollbar)
{
    recalcScrollbars();

    if (scrollbar == wxHORIZONTAL || scrollbar == -1)
    {
        int scroll_width = m_total_width;

        if (m_pixel_xoff+m_cliwidth > scroll_width)
            scroll_width = m_pixel_xoff+m_cliwidth;

        SetScrollbar(wxHORIZONTAL,
                     m_pixel_xoff,
                     m_cliwidth,
                     scroll_width, true);
    }

    if (scrollbar == wxVERTICAL || scrollbar == -1)
    {
        int scroll_height = m_total_height;

        if (m_pixel_yoff+m_cliheight > scroll_height)
            scroll_height = m_pixel_yoff+m_cliheight;

        SetScrollbar(wxVERTICAL,
                     m_pixel_yoff,
                     m_cliheight,
                     scroll_height, true);
    }
}

void TextView::scrollHorzToPosition(int pos)
{
    scrollHorz(pos-m_pixel_xoff);
}

void TextView::scrollHorz(int pixels)
{
    if (pixels == 0)
        return;

    if (m_offset_deleting)
        return;

    int old_char_offset = m_char_offset;
    int new_xoffset = m_pixel_xoff + pixels;

    if (pixels < 0)
    {
        // scrolling left

        if (new_xoffset < 0)
        {
            // scrolled past beginning of the text
            pixels += -(new_xoffset);
        }

        m_cursor_x -= pixels;
        m_pixel_xoff += pixels;
        m_char_offset = m_pixel_xoff/m_char_width;
    }
     else
    {
        // scrolling right

        int max_offset = m_total_width - m_cliwidth;
        if (new_xoffset > max_offset)
        {
            // scrolled past end of the text
            pixels -= (new_xoffset-max_offset);
        }

        m_cursor_x -= pixels;
        m_pixel_xoff += pixels;
        m_char_offset = m_pixel_xoff/m_char_width;
    }

    updateScrollbars(wxHORIZONTAL);
    render();

    // if a text control exists, move it
    if (m_control)
    {
        int move_pixels = m_char_offset-old_char_offset;
        move_pixels *= m_char_width;
        
        int x, y;
        m_control->GetPosition(&x, &y);
        m_control->Move(x-move_pixels,y);
    }
}

void TextView::scrollVert(int pixels)
{
    if (pixels == 0)
        return;

    if (m_offset_deleting)
        return;

    // translate from pixels to rows
    pixels *= m_char_height;

    int new_yoff = m_pixel_yoff+pixels;

    if (pixels < 0)
    {
        // scrolling up

        if (new_yoff < 0)
        {
            // scrolled past beginning of the text
            pixels += -(new_yoff);
        }

        m_cursor_y -= pixels;
        m_pixel_yoff += pixels;
        m_row_offset = m_pixel_yoff/m_char_height;
    }
     else
    {
        // scrolling down

        int max_offset = m_total_height - m_cliheight;
        if (new_yoff > max_offset)
        {
            // scrolled past end of the text
            pixels -= (new_yoff-max_offset);
        }

        m_cursor_y -= pixels;
        m_pixel_yoff += pixels;
        m_row_offset = m_pixel_yoff/m_char_height;
    }

    updateScrollbars(wxVERTICAL);
    render();
}

void TextView::beginEdit(int col_idx)
{
    if (col_idx < 0 || col_idx >= (int)m_cols.size())
        return;
    
    m_edited_col_idx = col_idx;
    
    int offset = m_cols[col_idx].offset;
    int width = m_cols[col_idx].width;
    int x = offsetToPixels(offset, m_char_offset, m_char_width);
    int w = offsetToPixels(offset+width, m_char_offset, m_char_width);
    w -= x;
    if (w < MIN_CAPTION_TEXTCTRL_WIDTH)
        w = MIN_CAPTION_TEXTCTRL_WIDTH;
    
    // ensure the text control is on the screen
    if (x < 0 || x+MIN_CAPTION_TEXTCTRL_WIDTH >= m_cliwidth)
    {
        // initialize the scroll amount to the width of the column
        int scroll_width = w;
        
        // if the column if offscreen to the right, scroll at least
        // enough to see part of the column
        if (x > m_cliwidth)
            scroll_width = (x-m_cliwidth)+MIN_CAPTION_TEXTCTRL_WIDTH;
            
        if (scroll_width > m_cliwidth)
            scroll_width = m_cliwidth-40;

        // if the column is off-screen to the right, scroll at least
        // enough to see the beginning of the column
        if (x < 0)
            scroll_width = x;
        
        scrollHorz(scroll_width);
            
        // recalculate the x-offset and width of the column
        x = offsetToPixels(offset, m_char_offset, m_char_width);
        w = offsetToPixels(offset+width, m_char_offset, m_char_width);
        w -= x;
        if (w < MIN_CAPTION_TEXTCTRL_WIDTH)
            w = MIN_CAPTION_TEXTCTRL_WIDTH;
    }

    // if we're on the last column, extend the caption text control
    // all the way to the end of the client area
    if (col_idx == m_cols.size()-1)
        w = m_cliwidth-x-1;
            
    CaptionTextCtrl* text = new CaptionTextCtrl(this,
                                                this,
                                                wxID_ANY,
                                                m_cols[col_idx].name,
                                                wxPoint(x,0),
                                                wxSize(w+1,CAPTION_HEIGHT),
                                                wxTE_PROCESS_ENTER);
    text->SetMaxLength(80);
    text->SetFocus();
    text->SetSelection(-1,-1);
    m_control = (wxControl*)text;
}

void TextView::endEdit(bool accept)
{
    if (m_control)
    {
        CaptionTextCtrl* ctrl = static_cast<CaptionTextCtrl*>(m_control);
        ctrl->destroyNow(accept);
        m_control = NULL;
        return;
    }
}

bool TextView::isEditing()
{
    return m_control ? true : false;
}

void TextView::onScroll(wxScrollWinEvent& evt)
{
    int newpos, oldpos;

    if (evt.GetOrientation() == wxHORIZONTAL)
    {
        oldpos = m_pixel_xoff;
        newpos = m_pixel_xoff;

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            newpos -= m_char_width*3;
            if (newpos < 0)
                newpos = 0;
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            newpos = std::min(newpos+(m_char_width*3), m_total_width - m_cliwidth);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            newpos -= (m_cliwidth*2)/3;
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            newpos += (m_cliwidth*2)/3;
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
        {
            newpos = evt.GetPosition();
        }

        if (newpos != m_pixel_xoff)
        {
            scrollHorz(newpos - oldpos);
            repaint();
        }
    }

    if (evt.GetOrientation() == wxVERTICAL)
    {
        oldpos = m_pixel_yoff;
        newpos = m_pixel_yoff;

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            newpos -= m_char_height*1;
            if (newpos < 0)
                newpos = 0;
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            newpos = std::min(newpos+(m_char_height*1), m_total_height - m_cliheight);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            newpos -= (m_vis_rows-3)*m_char_height;
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            newpos += (m_vis_rows-3)*m_char_height;
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
        {
            newpos = evt.GetPosition();
        }

        if (newpos != m_pixel_yoff)
        {
            scrollVert((newpos-oldpos)/m_char_height);
            repaint();
        }
    }
}

void TextView::onEditDestroy(const wxString& value,
                             int last_keycode,
                             bool focus_lost,
                             bool user_action)
{
    m_control = NULL;

    if (m_edited_col_idx >= 0 && m_edited_col_idx < (int)m_cols.size())
    {
        if (last_keycode != WXK_ESCAPE)
            modifyColumn(m_edited_col_idx, -1, -1, -1, value);
        
        if (last_keycode == WXK_LEFT &&
            m_edited_col_idx > 0)
        {
            m_edited_col_idx--;
            beginEdit(m_edited_col_idx);
        }
        
        if (last_keycode == WXK_RIGHT &&
            m_edited_col_idx < (int)m_cols.size()-1)
        {
            m_edited_col_idx++;
            beginEdit(m_edited_col_idx);
        }
        
        refresh();
    }
}

bool TextView::mouseInWindow()
{
    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);

    if (pt_mouse.y < 0 || pt_mouse.y > m_cliheight)
        return false;

    if (pt_mouse.x < 0 || pt_mouse.x > m_cliwidth)
        return false;

    return true;
}

void TextView::onMouseMove(wxMouseEvent& evt)
{
    m_cursor_x = evt.GetX();
    m_cursor_y = evt.GetY();

    int old_cursor_offset = m_cursor_offset;
    int old_cursor_row = m_cursor_row;
    
    m_cursor_offset = pixelsToOffset(m_cursor_x, m_pixel_xoff, m_char_width);
    m_cursor_row = m_row_offset + (m_cursor_y/m_char_height);
    if (!m_offset_deleting)
    {
        if (m_dragged != m_cols.end())
        {
            if (m_cursor_offset != m_dragged->offset)
                m_offset_dragged = true;
        }
    }
    
    // fire a signal with the new cursor offset and row position
    if (old_cursor_offset != m_cursor_offset ||
        old_cursor_row != m_cursor_row)
    {
        sigCursorPositionChanged(m_cursor_offset, m_cursor_row);
    }
    
    
    wxCursor cursor = wxNullCursor;

    // determine cursor
    if (!isEditing() && (m_cursor_y > CAPTION_HEIGHT))
    {
        // if we're dragging an offset, set the cursor to a left/right arrow
        if (m_offset_dragged)
        {
            cursor = wxCursor(wxCURSOR_SIZEWE);
        }
         else
        {
            std::vector<TextViewColumn>::iterator it;
            for (it = m_cols.begin(); it != m_cols.end(); ++it)
            {
                if (m_cursor_offset == it->offset)
                {
                    cursor = wxCursor(wxCURSOR_SIZEWE);
                    break;
                }
            }
        }
    }
    
    SetCursor(cursor);
    
    render();
    repaint();
}

void TextView::onMouseLeave(wxMouseEvent& evt)
{
    render();
    repaint();
}

void TextView::onMouseLeftDblClick(wxMouseEvent& evt)
{
    int mouse_y = evt.GetY();
    
    if (mouse_y < CAPTION_HEIGHT + 2)
    {
        // this just allows a little padding so that the user
        // doesn't accidentally remove a break when they want
        // to actually rename a column
        return;
    }
    
    if (!mouseInWindow() || m_offset_dragged)
        return;

    int col_idx = getColumnIdxFromOffset(m_cursor_offset);
    deleteColumn(col_idx);
}

void TextView::onMouseLeftDown(wxMouseEvent& evt)
{
    if (isEditing())
    {
        m_was_editing = true;
        endEdit(true);
    }
       
    m_cursor_down_x = evt.GetX();
    m_cursor_down_y = evt.GetY();
    m_dragging = true;

    int offset = pixelsToOffset(m_cursor_down_x, m_pixel_xoff, m_char_width);

    std::vector<TextViewColumn>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        // the user clicked on a break handle
        if (it->offset == offset)
        {
            // in the case that we are going to drag a column,
            // save the break info for future use
            m_dragged = it;
            break;
        } 
    }

    if (!m_scroll_timer.IsRunning())
    {
        m_scroll_timer.Start(30);
    }

    CaptureMouse();

    render();
    repaint();
}

void TextView::onMouseLeftUp(wxMouseEvent& evt)
{
    m_cursor_down_x = -1;
    m_cursor_down_y = -1;
    m_dragging = false;

    int mouse_y = evt.GetY();
    if (mouse_y < CAPTION_HEIGHT)
    {
        // release the mouse
        if (GetCapture() == this)
            ReleaseMouse();
            
        // stop the scroll timer if it was started
        if (m_scroll_timer.IsRunning())
        {
            m_scroll_timer.Stop();
        }

        // reset these variables to their defaults
        m_offset_deleting = false;
        m_offset_dragged = false;
        m_dragged = m_cols.end();

        m_was_editing = false;
        beginEdit(getCursorColumn());
        return;
    }

    if (mouseInWindow() &&
        !isEditing()    &&
        !m_was_editing  &&
        !m_offset_deleting)
    {
        if (m_offset_dragged)
        {
            int col_idx = getColumnIdxFromOffset(m_dragged->offset);
            moveColumn(col_idx, m_cursor_offset);
        }
         else
        {
            if (m_cursor_y > CAPTION_HEIGHT+2)
            {
                addColumn(m_cursor_offset);
            }
        }
    }

    // release the mouse
    if (GetCapture() == this)
        ReleaseMouse();
        
    // stop the scroll timer if it was started
    if (m_scroll_timer.IsRunning())
    {
        m_scroll_timer.Stop();
    }

    // reset these variables to their defaults
    m_offset_deleting = false;
    m_offset_dragged = false;
    m_dragged = m_cols.end();

    m_was_editing = false;
    render();
    repaint();
}

void TextView::onMouseRightDown(wxMouseEvent& evt)
{
    m_cursor_x = evt.GetX();
    m_cursor_y = evt.GetY();
    
    if (m_cursor_y < CAPTION_HEIGHT)
    {
        int col = getCursorColumn();
        if (col >= 0 && (size_t)col < m_cols.size())
        {
            wxMenu menuPopup;
            menuPopup.Append(ID_PopupMenu_RenameColumn, _("Rename Column"));
            menuPopup.AppendSeparator();
            menuPopup.AppendRadioItem(ID_PopupMenu_EncodingDefault, _("Default Encoding"));
            menuPopup.AppendRadioItem(ID_PopupMenu_EncodingASCII, _("ASCII"));
            menuPopup.AppendRadioItem(ID_PopupMenu_EncodingUCS2, _("Unicode UCS-2"));
            menuPopup.AppendRadioItem(ID_PopupMenu_EncodingEBCDIC, _("EBCDIC"));
            menuPopup.AppendRadioItem(ID_PopupMenu_EncodingCOMP, _("COMP Numeric"));
            menuPopup.AppendRadioItem(ID_PopupMenu_EncodingCOMP3, _("COMP-3 Numeric"));
    
            switch (m_cols[col].encoding)
            {
                case xd::encodingUndefined: menuPopup.Check(ID_PopupMenu_EncodingDefault, true); break;
                case xd::encodingASCII:     menuPopup.Check(ID_PopupMenu_EncodingASCII, true); break;
                case xd::encodingEBCDIC:    menuPopup.Check(ID_PopupMenu_EncodingEBCDIC, true); break;
                case xd::encodingUTF8:      break;
                case xd::encodingUTF16:     break;
                case xd::encodingUTF32:     break;
                case xd::encodingUCS2:      menuPopup.Check(ID_PopupMenu_EncodingUCS2, true); break;
                case xd::encodingCOMP:      menuPopup.Check(ID_PopupMenu_EncodingCOMP, true); break;
                case xd::encodingCOMP3:     menuPopup.Check(ID_PopupMenu_EncodingCOMP3, true); break;
            };
            
            wxPoint pt_mouse = ::wxGetMousePosition();
            pt_mouse = ScreenToClient(pt_mouse);
            CommandCapture* cc = new CommandCapture;
            PushEventHandler(cc);
            PopupMenu(&menuPopup, pt_mouse);
            int command = cc->getLastCommandId();
            PopEventHandler(true);

            if (command > 0)
            {
                int new_encoding = -1;
                
                switch (command)
                {
                    case ID_PopupMenu_RenameColumn:   break;
                    case ID_PopupMenu_EncodingDefault: new_encoding = xd::encodingUndefined; break;
                    case ID_PopupMenu_EncodingASCII:   new_encoding = xd::encodingASCII; break;
                    case ID_PopupMenu_EncodingUCS2:    new_encoding = xd::encodingUCS2; break;
                    case ID_PopupMenu_EncodingEBCDIC:  new_encoding = xd::encodingEBCDIC; break;
                    case ID_PopupMenu_EncodingCOMP:    new_encoding = xd::encodingCOMP; break;
                    case ID_PopupMenu_EncodingCOMP3:   new_encoding = xd::encodingCOMP3; break;
                };
                
                if (new_encoding != -1)
                {
                    modifyColumn(col, -1, -1, new_encoding, wxEmptyString);
                }
            }
        }
    }
}


void TextView::onMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
    m_offset_deleting = false;
    m_offset_dragged = false;
    m_dragged = m_cols.end();

    if (m_scroll_timer.IsRunning())
    {
        m_scroll_timer.Stop();
    }
}

void TextView::onMouseCaptureChanged(wxMouseCaptureChangedEvent& evt)
{
    m_offset_deleting = false;
    m_offset_dragged = false;
    m_dragged = m_cols.end();

    if (m_scroll_timer.IsRunning())
    {
        m_scroll_timer.Stop();
    }
}

void TextView::onScrollTimer(wxTimerEvent& evt)
{
    if (!mouseInWindow())
    {
        wxPoint pt_mouse = ::wxGetMousePosition();
        pt_mouse = ScreenToClient(pt_mouse);

        if (pt_mouse.x < 0)
        {
            scrollHorz(-m_char_width*3);
            updateScrollbars(wxHORIZONTAL);
        }

        if (pt_mouse.x > m_cliwidth)
        {
            scrollHorz(m_char_width*3);
            updateScrollbars(wxHORIZONTAL);
        }
        
        if (pt_mouse.y < 0)
        {
            scrollVert(-3);
            updateScrollbars(wxVERTICAL);
        }

        if (pt_mouse.y > m_cliheight)
        {
            scrollVert(3);
            updateScrollbars(wxVERTICAL);
        }

        repaint();
    }
}

void TextView::onSize(wxSizeEvent& evt)
{
    if (isEditing())
        endEdit(true);
        
    GetClientSize(&m_cliwidth, &m_cliheight);

    int bitmap_height = m_bmp.GetHeight();
    int bitmap_width = m_bmp.GetWidth();

    if (m_cliheight > bitmap_height ||
        m_cliwidth > bitmap_width)
    {
        m_bmp.SetHeight(m_cliheight + 25);
        m_bmp.SetWidth(m_cliwidth + 25);
        m_memdc.SelectObject(m_bmp);
    }

    // update our visible row and column count
    m_vis_rows = m_cliheight/m_char_height;
    m_vis_chars = m_cliwidth/m_char_width;

    render();
    repaint();
}
