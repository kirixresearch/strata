/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-10
 *
 */


#include "kcanvas_int.h"
#include "componenttextbox.h"
#include "util.h"


namespace kcanvas
{


const wxString ACTION_NONE = wxT("textbox.action.none");
const wxString ACTION_SELECT = wxT("textbox.action.select");

static bool isWhiteSpace(wxChar c)
{
    if (c == (wxChar)WXK_SPACE ||
        c == (wxChar)WXK_TAB || 
        c == (wxChar)WXK_RETURN ||
        c == (wxChar)0x0a)
    {
        return true;
    }
        
    return false;
}

static bool isLineTerminator(wxChar c)
{
    if (c == (wxChar)0x0a ||
        c == (wxChar)0x0d)
    {
        return true;
    }
    
    return false;
}


class TextProperties
{
public:

    TextProperties()
    {
        m_starting_idx = 0;
        m_ending_idx = 0;
        
        m_xpos = 0;
        m_ypos = 0;
        m_width = 0;
    }

public:

    // note: this class stores information about a slice 
    // of text within the main text string

    wxString m_string;  // text string

    int m_starting_idx; // starting index of the text
    int m_ending_idx;   // ending index of the text

    int m_xpos;         // starting x position of the text line in canvas units
    int m_ypos;         // starting y position of the text line in canvas units
    int m_width;        // width of the line, including trailing whitespace, in canvas units
};


class LineTokenizer
{
public:
    
    LineTokenizer(ICanvasPtr canvas, const wxString& string, int width, bool wrap)
    {
        m_canvas = canvas;
        m_string = string;
        m_length = string.Length();
        m_container_width = width;
        m_wrap = wrap;

        m_current_idx = 0;
        m_line_beginning = 0;
        m_line_ending = 0;

        m_canvas->getPartialTextExtents(m_string, m_widths);
        m_word = false;
    }
    
    ~LineTokenizer()
    {
    }
    
    wxString getNextLine()
    {
        // set the beginning of the line to the offset we last
        // ended with
        m_line_beginning = m_current_idx;

        // iterate through the characters of the string, looking
        // for the end of the line
        while(hasMoreLines())
        {
            // get the character
            wxChar c = m_string.GetChar(m_current_idx);

            // if we're on a paragraph marker, we've found the end
            // of this line; set the line ending to the current
            // character, and increment the current character index 
            // so we step over the return character
            int next_idx;
            if (isParagraphBoundary(c, &next_idx))
            {
                m_line_ending = m_current_idx;
                m_current_idx += next_idx;
                break;
            }

            // if wrap is true, return the line when it exceeds
            // the width of the container
            if (m_wrap)
            {
                // if we encounter the start of a word, keep track of 
                // where it is; later on, if this word causes the line 
                // to exceed the width of the container, we'll use this 
                // position as the end of the line
                int flag;
                if (isWordBoundary(c, &flag) && flag == 1)
                    m_line_ending = m_current_idx;

                // if we exceed the width of the container and we're on 
                // a visible character, we're at the end of the line
                if (exceedsWidth(m_current_idx) && !isWhiteSpace(c))
                {
                    // if the line ending has moved because we've encountered the 
                    // start of a word, then it's already set to what we want;
                    // however, if the line ending hasn't moved, we're in a solid
                    // block of characters, so use the current index as the end of 
                    // the line, unless the current index is the beginning of the
                    // line, in which case we have a single character and we have
                    // to advance at least one
                    if (m_line_ending == m_line_beginning)
                    {
                        if (m_current_idx == m_line_beginning)
                            m_current_idx++;
                    
                        m_line_ending = m_current_idx;
                    }

                    // reset the current index to the end of the line, so
                    // we'll start in the correct place next time we get
                    // a line
                    m_current_idx = m_line_ending;
                    break;
                }
            }
 
            // if we haven't encountered a paragraph break or exceeded
            // the width of the container, increment the counter
            m_current_idx++;

            // if we have no more lines, then this is the last line, and
            // we have to explicity set the line ending so the last line
            // is returned
            if (!hasMoreLines())
                m_line_ending = m_current_idx;
        }

        // return the line
        return m_string.Mid(m_line_beginning, m_line_ending - m_line_beginning);
    }

    bool hasMoreLines() const
    {
        return (m_current_idx < m_length);
    }

    int getLineBeginning() const
    {
        return m_line_beginning;
    }
 
    int getLineEnding() const
    {
        return m_line_ending;
    }
 
    int getLineWidth() const
    {
        // if the line ending position is the same as the
        // line beginning position, return 0
        if (m_line_ending <= m_line_beginning)
            return 0;
            
        // if the line beginning position is zero, return the
        // width of the string at the line ending position
        if (m_line_beginning == 0)
            return m_widths[m_line_ending - 1];
            
        // if the beginning line position isn't zero, return the 
        // width of the string before the line ending position
        // minus the width of the string before the line beginning
        // position
        return m_widths[m_line_ending - 1] - m_widths[m_line_beginning - 1];
    }

private:

    bool isParagraphBoundary(wxChar c, int* n)
    {
        // note: c is the input character; n is the number of
        // characters used to specify the paragraph boundary,
        // which will be 0 if there is no paragraph boundary,
        // 1 if the paragraph boundary is a single carriage
        // return or linefeed, and 2 if the paragraph boundary
        // is a return/linefeed combination
    
        // return/linefeed paragraph boundary
        if (c == (wxChar)0x0d)
        {
            // see if the next character is a line feed;
            // if so, include it in the paragraph boundary
            if (m_current_idx < m_length - 1 && 
                m_string.GetChar(m_current_idx + 1) == (wxChar)0x0a)
            {
                *n = 2;
                return true;
            }
        }

        // single character line terminators
        if (isLineTerminator(c))
        {
            m_word = false;
            *n = 1;
            return true;
        }

        // not a paragraph boundary
        *n = 0;
        return false;
    }
    
    bool isWordBoundary(wxChar c, int* flag)
    {
        // a word boundary with a flag of 1 has an index
        // corresponding to the first character in a word;
        // a word boundary with a flag of -1 has an index
        // corresponding to the first white space following
        // a word

        if (!m_word && !isWhiteSpace(c))
        {
            m_word = true;
            *flag = 1;
            return true;
        }    
    
        if (m_word && isWhiteSpace(c))
        {
            m_word = false;
            *flag = -1;
            return true;
        }

        *flag = 0;
        return false;
    }
    
    bool exceedsWidth(int idx)
    {
        int width;
        if (m_line_beginning <= 0)
        {
            width = m_widths[idx];
        }
        else
        {
            width = m_widths[idx] - m_widths[m_line_beginning-1];
        }
     
        return (width > m_container_width);
    }

private:

    ICanvasPtr m_canvas;
    wxString m_string;
    int m_length;
    int m_container_width;
    bool m_wrap;

    int m_current_idx;
    int m_line_beginning;
    int m_line_ending;

    wxArrayInt m_widths;
    bool m_word;
};


CompTextBox::CompTextBox()
{
    // properties
    initProperties(m_properties);

    // add event handlers
    addEventHandler(EVENT_KEY, &CompTextBox::onKey);
    addEventHandler(EVENT_MOUSE_LEFT_DCLICK, &CompTextBox::onMouse);
    addEventHandler(EVENT_MOUSE_LEFT_DOWN, &CompTextBox::onMouse);
    addEventHandler(EVENT_MOUSE_LEFT_UP, &CompTextBox::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_DCLICK, &CompTextBox::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_DOWN, &CompTextBox::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_UP, &CompTextBox::onMouse);
    addEventHandler(EVENT_MOUSE_MOTION, &CompTextBox::onMouse);
    addEventHandler(EVENT_MOUSE_OVER, &CompTextBox::onMouse);

    // editing variable
    m_editing = false;
    
    // cache initialization
    m_cache_halign = wxT("");
    m_cache_valign = wxT("");
    m_cache_line_size = -1;
    m_cache_width = -1;
    m_cache_height = -1;
    m_cache_cursor_x = -1;
    m_cache_cursor_y = -1;
    m_cache_cursor_x_last = -1;

    // cursor initialization
    m_cursor_pos = 0;
    
    // selection variable initialization
    m_selection_x1 = -1;    // the beginning position of the selection
    m_selection_x2 = -1;    // the ending position of the selection
    
    // mouse variable initialization
    m_mouse_action = ACTION_NONE;   // name of current mouse action    
    m_mouse_x = 0;                  // current x position of the mouse
    m_mouse_y = 0;                  // current y position of the mouse
    m_mouse_x_start = 0;            // start x position of the mouse
    m_mouse_y_start = 0;            // start y position of the mouse
}

CompTextBox::~CompTextBox()
{
}

IComponentPtr CompTextBox::create()
{
    return static_cast<IComponent*>(new CompTextBox);
}

void CompTextBox::initProperties(Properties& properties)
{
    Font default_font;
    PropertyValue font_facename;
    PropertyValue font_size;
    PropertyValue font_style;
    PropertyValue font_weight;
    PropertyValue font_underscore;

    setPropertiesFromFont(default_font,
                          font_facename,
                          font_size,
                          font_style,
                          font_weight,
                          font_underscore);

    // add properties
    Component::initProperties(properties);    
    properties.add(PROP_COMP_TYPE, COMP_TYPE_TEXTBOX);
    properties.add(PROP_FONT_FACENAME, font_facename);
    properties.add(PROP_FONT_SIZE, font_size);
    properties.add(PROP_FONT_STYLE, font_style);
    properties.add(PROP_FONT_WEIGHT, font_weight);
    properties.add(PROP_FONT_UNDERSCORE, font_underscore);
    properties.add(PROP_TEXT_COLOR, Color(COLOR_BLACK));
    properties.add(PROP_TEXT_HALIGN, ALIGNMENT_LEFT);
    properties.add(PROP_TEXT_VALIGN, ALIGNMENT_MIDDLE);
    properties.add(PROP_TEXT_SPACING, TEXTBOX_LINE_SPACING);
    properties.add(PROP_TEXT_WRAP, true);
    properties.add(PROP_CONTENT_MIMETYPE, wxString(wxT("")));    
    properties.add(PROP_CONTENT_ENCODING, wxString(wxT("")));    
    properties.add(PROP_CONTENT_VALUE, wxString(wxT("")));
}

IComponentPtr CompTextBox::clone()
{
    CompTextBox* c = new CompTextBox;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompTextBox::copy(IComponentPtr component)
{
    Component::copy(component);
    clearCache();
}

void CompTextBox::addProperty(const wxString& prop_name, const PropertyValue& value)
{
    // add the property and clear the cache
    Component::addProperty(prop_name, value);
    clearCache();
}

void CompTextBox::addProperties(const Properties& properties)
{
    // add the properties and clear the cache
    Component::addProperties(properties);
    clearCache();
}

void CompTextBox::removeProperty(const wxString& prop_name)
{
    // remove the property and clear the cache
    Component::removeProperty(prop_name);
    clearCache();
}

bool CompTextBox::setProperty(const wxString& prop_name, const PropertyValue& value)
{
    // try to set the component property; if the property is 
    // unchanged, we're done; return false
    if (!Component::setProperty(prop_name, value))
        return false;
        
    // if the property has changed, clear the cache; return true
    clearCache();
    return true;
}

bool CompTextBox::setProperties(const Properties& properties)
{
    // try to set the component properties; if the properties are 
    // unchanged, we're done; return false
    if (!Component::setProperties(properties))
        return false;
    
    // if the properties have changed, clear the cache; return true
    clearCache();
    return true;
}

void CompTextBox::extends(wxRect& rect)
{
    // note: this function returns the size of the text content
    
    // if we don't have any text, we're done
    if (getText().Length() == 0)
    {
        rect.x = 0;
        rect.y = 0;
        rect.width = 0;
        rect.height = 0;
        return;
    }

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // calculate the extent of the text
    int x1 = INT_MAX;
    int y1 = INT_MAX;
    int x2 = INT_MIN;
    int y2 = INT_MIN;

    std::vector<TextProperties>::iterator it, it_end;
    it_end = m_text_lines.end();
    
    for (it = m_text_lines.begin(); it != it_end; ++it)
    {
        if (it->m_xpos < x1)
            x1 = it->m_xpos;

        if (it->m_xpos + it->m_width > x2)
            x2 = it->m_xpos + it->m_width;

        if (it->m_ypos < y1)
            y1 = it->m_ypos;

        if (it->m_ypos + m_cache_line_size > y2)
            y2 = it->m_ypos + m_cache_line_size;
    }

    // set the extent rectangle
    rect.x = x1;
    rect.y = y1;
    rect.width = x2 - x1;
    rect.height = y2 - y1;
    
    // if the wrap property is set, then by definition, the text
    // is limited to the width of the component, even if a small
    // portion of the whitespace at the end of each line causes
    // the actual text width to exceed the bounds of the text
    // component; so in this case, clamp the width of the textbox 
    // extent to the width of the component
    PropertyValue value;
    getProperty(PROP_TEXT_WRAP, value);
    bool wrap = value.getBoolean();

    if (wrap)
    {
        x1 = 0;
        rect.width = getWidth();
    }
}

void drawText(ICanvasPtr canvas,
              TextProperties* tp,
              const Color& text_color,
              const Color& selection_color,
              int selection_start = -1,
              int selection_end = -1)
{
    // note: this function draws the string in the text
    // properties to the canvas with the specified text
    // color; if a selection is specified (i.e., if 
    // selection_start or selection_end is greater than 
    // zero, then this function draws the non-selected
    // text with the text_color and the selected text with
    // the selection_color; canvas is the canvas to which
    // to draw the text, tp contains the text to info
    // about the index and position of the text, text_color
    // is the normal color to draw the text, selection_color
    // is the color to draw the text when it is selected,
    // and selection_start/selection_end are the well
    // ordered (selection_start < selection_end) range
    // of selection indexes

    // if we don't have a canvas, we're done
    if (canvas.isNull())
        return;

    // if we don't have a selection; draw the text with 
    // the normal color
    if (selection_start <= 0 && selection_end <= 0)
    {
        canvas->setTextForeground(text_color);
        canvas->drawText(tp->m_string, tp->m_xpos, tp->m_ypos);
        return;
    }

    // if we have a selection, but the selection doesn't
    // intersect the text, draw the text with the 
    // normal color
    if (selection_start > tp->m_ending_idx ||
        selection_end < tp->m_starting_idx)
    {
        canvas->setTextForeground(text_color);
        canvas->drawText(tp->m_string, tp->m_xpos, tp->m_ypos);
        return;
    }
    
    // if the selection completely contains the text,
    // draw the text with the selection color
    if (selection_start <= tp->m_starting_idx &&
        selection_end >= tp->m_ending_idx)
    {
        canvas->setTextForeground(selection_color);
        canvas->drawText(tp->m_string, tp->m_xpos, tp->m_ypos);
        return;
    }
    
    // if the selection divides the text into two parts,
    // some initial unselected text, and then selected
    // text to the end of the string, draw two strings, 
    // the first with the normal color and the second with 
    // the selected color
    if (selection_start > tp->m_starting_idx &&
        selection_end >= tp->m_ending_idx)
    {
        // calculate the starting position of the second
        // part of the string
        int part2 = selection_start - tp->m_starting_idx;
    
        // draw the first part of the string    
        canvas->setTextForeground(text_color);
        canvas->drawText(tp->m_string.Left(part2),
                         tp->m_xpos, 
                         tp->m_ypos);
        
        // draw the second part of the string; calculate the second
        // part's position by subtracting the width of the second
        // part from the overall width of the string; this ensures
        // the second part is positioned correctly; if we simply
        // start at the end of the width of the first string, the
        // second string won't be positioned correctly because it
        // won't take into account kerning

        int height = 0;
        int width1 = 0;
        int width2 = 0;
        int width = 0;

        // get the extent of the first part of the string
        canvas->getTextExtent(tp->m_string.Left(part2), &width1, &height);
        
        // get the extent of the second part of the string
        canvas->getTextExtent(tp->m_string.Mid(part2), &width2, &height);
        
        // get the text extent of the string
        canvas->getTextExtent(tp->m_string, &width, &height);
        
        // get the kerning difference
        int kerning = width1 + width2 - width;

        // draw the second part
        canvas->setTextForeground(selection_color);
        canvas->drawText(tp->m_string.Mid(part2), 
                         tp->m_xpos + width1 - kerning, 
                         tp->m_ypos);

        return;
    }

    // if the selection divides the text into two parts,
    // some initial selected text, and then unselected
    // text to the end of the string, draw two strings, 
    // the first with the selected color and the second 
    // with the normal color
    if (selection_start <= tp->m_starting_idx &&
        selection_end < tp->m_ending_idx)
    {
        // calculate the starting position of the second
        // part of the string
        int part2 = selection_end - tp->m_starting_idx;
    
        // draw the first part of the string    
        canvas->setTextForeground(selection_color);
        canvas->drawText(tp->m_string.Left(part2),
                         tp->m_xpos, 
                         tp->m_ypos);
        
        // draw the second part of the string; calculate the second
        // part's position by subtracting the width of the second
        // part from the overall width of the string; this ensures
        // the second part is positioned correctly; if we simply
        // start at the end of the width of the first string, the
        // second string won't be positioned correctly because it
        // won't take into account kerning

        int height = 0;
        int width1 = 0;
        int width2 = 0;
        int width = 0;

        // get the extent of the first part of the string
        canvas->getTextExtent(tp->m_string.Left(part2), &width1, &height);
        
        // get the extent of the second part of the string
        canvas->getTextExtent(tp->m_string.Mid(part2), &width2, &height);
        
        // get the text extent of the string
        canvas->getTextExtent(tp->m_string, &width, &height);
        
        // get the kerning difference
        int kerning = width1 + width2 - width;

        // draw the second part
        canvas->setTextForeground(text_color);
        canvas->drawText(tp->m_string.Mid(part2), 
                         tp->m_xpos + width1 - kerning, 
                         tp->m_ypos);

        return;
    }

    // if the selection divides the text into three parts,
    // some initial unselected text, some selected text,
    // and them some unselected text, draw three strings,
    // the first with the normal color, the second with
    // the selected color, and the third with the normal
    // color
    if (selection_start > tp->m_starting_idx &&
        selection_end < tp->m_ending_idx)
    {
        int height = 0;
        int width1 = 0;
        int width2 = 0;
        int width3 = 0;
        int width = 0;

        int part2 = selection_start - tp->m_starting_idx;
        int part3 = selection_end - tp->m_starting_idx;

        // get the width of the first part
        canvas->getTextExtent(tp->m_string.Left(part2), &width1, &height);
        
        // get the width of the second part
        canvas->getTextExtent(tp->m_string.Mid(part2, part3 - part2), &width2, &height);
        
        // get the width of the third part
        canvas->getTextExtent(tp->m_string.Mid(part3), &width3, &height);
    
        // adjust for the kerning between the first and second part
        int kerning12 = 0;
        canvas->getTextExtent(tp->m_string.Left(part3), &width, &height);
        kerning12 = width1 + width2 - width;
        
        // adjust for the kerning between the first and second part
        int kerning23 = 0;
        canvas->getTextExtent(tp->m_string.Mid(part2), &width, &height);
        kerning23 = width2 + width3 - width;
        
        // draw the first part of the string
        canvas->setTextForeground(text_color);
        canvas->drawText(tp->m_string.Left(part2), tp->m_xpos, tp->m_ypos);
        
        // draw the second part of the string
        canvas->setTextForeground(selection_color);
        canvas->drawText(tp->m_string.Mid(part2, part3), tp->m_xpos + width1 - kerning12, tp->m_ypos);

        // draw the third part of the string
        canvas->setTextForeground(text_color);
        canvas->drawText(tp->m_string.Mid(part3), tp->m_xpos + width1 + width2 - kerning12 - kerning23, tp->m_ypos);
    
        return;
    }    
}

void CompTextBox::render(const wxRect& rect)
{
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // get the width and height
    int width, height;
    getSize(&width, &height);

    // draw background rectangle
    canvas->setPen(m_properties);
    canvas->setBrush(m_properties);
    canvas->drawRectangle(0, 0, width, height);

    // draw borders; TODO: for now set the pen manually; should be set
    // from the properties
    Pen pen;
    pen.setCap(wxCAP_BUTT);
    pen.setJoin(wxJOIN_MITER);
    pen.setWidth(1);
    pen.setColor(COLOR_BLACK);
    canvas->setPen(pen);

    // small adjustment so that border lines are the same length
    // as the background rectangle size
    int border_adjust_x = canvas->dtom_x(1);
    int border_adjust_y = canvas->dtom_y(1);

    PropertyValue border_value;
    if (getProperty(PROP_BORDER_TOP_STYLE, border_value) && border_value.getString() == BORDER_STYLE_SOLID)
        canvas->drawLine(0, 0, width + border_adjust_x, 0);
    if (getProperty(PROP_BORDER_BOTTOM_STYLE, border_value) && border_value.getString() == BORDER_STYLE_SOLID)
        canvas->drawLine(0, height, width + border_adjust_x, height);
    if (getProperty(PROP_BORDER_LEFT_STYLE, border_value) && border_value.getString() == BORDER_STYLE_SOLID)
        canvas->drawLine(0, 0, 0, height + border_adjust_y);
    if (getProperty(PROP_BORDER_RIGHT_STYLE, border_value) && border_value.getString() == BORDER_STYLE_SOLID)
        canvas->drawLine(width, 0, width, height + border_adjust_y);

    // if we're editing and a selection exists, then
    // render the selection
    if (isEditing() && hasSelection())
    {
        // get the starting and ending positions of the selection,
        // as well as the line size
        int x1, y1, x2, y2, line_size;
        getSelectionByPos(&x1, &y1, &x2, &y2);
        line_size = m_cache_line_size;

        // set the selection background color
        Color selection_backround_color;
        PropertyValue prop_selection_backround_color;
        if (canvas->getProperty(PROP_COLOR_HIGHLIGHT, prop_selection_backround_color))
            selection_backround_color = prop_selection_backround_color.getColor();        
        
        canvas->setPen(selection_backround_color);
        canvas->setBrush(selection_backround_color);
        
        // if the starting and ending y values are the same, draw
        // a single selection rectangle; otherwise, draw three
        // rectangles: one for the top line, one for the middle
        // section if it exists, and one for the bottom line
        if (y1 == y2)
        {
            canvas->drawRectangle(x1, y1, x2 - x1, line_size);
        }
        else
        {
            // draw the top section of the selection
            canvas->drawRectangle(x1, y1, width - x1, line_size);
            
            // draw the middle section of the selection
            canvas->drawRectangle(0, y1 + line_size, width, y2 - y1 - line_size);
            
            // draw the bottom section of the selection
            canvas->drawRectangle(0, y2, x2, line_size);
        }
    }

    // if the size of the text string isn't zero,
    // render the text
    if (getText().Length() != 0)
    { 
        // get the text color
        PropertyValue value;
        getProperty(PROP_TEXT_COLOR, value);
        Color text_color = value.getColor();

        // if the text foreground color is null, we're done
        if (text_color == COLOR_NULL)
            return;

        // if the cache is empty, populate it
        if (isCacheEmpty())
            populateCache();

        // if we have a selection, get the selection range
        int selection_start = -1;
        int selection_end = -1;

        if (hasSelection())
            getSelection(&selection_start, &selection_end, true);

        // set the font properties
        canvas->setFont(m_cache_font);

        // go through the lines and draw them
        std::vector<TextProperties>::iterator it, it_end;
        it_end = m_text_lines.end();

        // set the text highlight color
        Color selection_foreground_color;
        PropertyValue prop_selection_foreground_color;
        if (canvas->getProperty(PROP_COLOR_HIGHLIGHTTEXT, prop_selection_foreground_color))
            selection_foreground_color = prop_selection_foreground_color.getColor();  

        // draw the text        
        for (it = m_text_lines.begin(); it != it_end; ++it)
        {
            drawText(canvas,
                     &(*it),
                     text_color,
                     selection_foreground_color,
                     selection_start,
                     selection_end);
        }
    }

    // if we're editing, set the caret size
    if (isEditing())
    {
        int x, y, draw_x, draw_y;
        getInsertionPointByPos(&x, &y);
        canvas->getDrawOrigin(&draw_x, &draw_y);
        
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_CANVAS_CARET, this);
        notify_evt->addProperty(EVENT_PROP_CARET_X, x + draw_x);
        notify_evt->addProperty(EVENT_PROP_CARET_Y, y + draw_y);
        notify_evt->addProperty(EVENT_PROP_CARET_H, m_cache_line_size);
        dispatchEvent(notify_evt);
    }
}

void CompTextBox::setText(const wxString& text)
{
    // reset the selection area
    clearSelection();

    // set the text
    setProperty(PROP_CONTENT_VALUE, text);
}

const wxString& CompTextBox::getText()
{
    getProperty(PROP_CONTENT_VALUE, m_cache_value);
    return m_cache_value.getString();
}

void CompTextBox::setInsertionPointByPos(int x, int y)
{
    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // set the cursor position from the x and y position 
    setCursorPos(xyToPos(&x, &y));
    m_cache_cursor_x_last = x;
    m_cache_cursor_x = x;
    m_cache_cursor_y = y;
}

void CompTextBox::getInsertionPointByPos(int* x, int* y)
{
    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // if the cursor location isn't set, then set it
    if (m_cache_cursor_x == -1 || m_cache_cursor_y == -1)
        posToXY(getCursorPos(), &m_cache_cursor_x, &m_cache_cursor_y);

    *x = m_cache_cursor_x;
    *y = m_cache_cursor_y;
}

void CompTextBox::setSelectionByPos(int x1, int y1, int x2, int y2)
{
    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // find the starting and ending positions of the selection from the 
    // x and y input positions and set the selection       
    int p1 = xyToPos(&x1, &y1);
    int p2 = xyToPos(&x2, &y2);
    
    // set the selection
    setSelection(p1, p2);
}

void CompTextBox::getSelectionByPos(int* x1, int* y1, int* x2, int* y2)
{
    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // get the selection
    int selection_x1, selection_x2;
    getSelection(&selection_x1, &selection_x2, true);

    // return the x and y input positions of the selection 
    // starting and ending positions
    posToXY(selection_x1, x1, y1);
    posToXY(selection_x2, x2, y2);
}

void CompTextBox::beginEdit()
{
    // get the text before the edit so we can choose to not 
    // accept the change; set the editing state to true
    m_text_before_edit = getText();
    m_editing = true;
}

void CompTextBox::endEdit(bool accept)
{
    // if the accepted flag is true, set the text
    if (!accept)
        setText(m_text_before_edit);

    // clear any selections
    clearSelection();

    // set the editing state to false
    m_text_before_edit = wxT("");
    m_editing = false;
}

bool CompTextBox::isEditing() const
{
    return m_editing;
}

bool CompTextBox::canCut() const
{
    return hasSelection();
}

bool CompTextBox::canCopy() const
{
    return hasSelection();
}

bool CompTextBox::canPaste() const
{
    bool canpaste = false;
    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))
            canpaste = true;
        
        wxTheClipboard->Close();
    }

    return canpaste;
}

void CompTextBox::cut()
{
    // copy the text to the clipboard
    copy();

    // delete the selected characters
    wxString text = getText();
    deleteSelectedChars(text, KEYSTATE_NONE);
    
    // set the text
    setText(text);
}

void CompTextBox::copy()
{
    // if there's no selection, we're done
    if (!hasSelection())
        return;

    // get the selection
    int x1, x2;
    getSelection(&x1, &x2, true);

    // get the text of the selection
    wxString text;
    text = getText();
    text = text.Mid(x1, x2 - x1);

    // copy the cut string to the clipboard
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
}

void CompTextBox::paste()
{
    // get the paste text from the clipboard
    wxString paste_text;

    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))
        {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            paste_text = data.GetText();
            paste_text.Replace(wxT("\r\n"), wxT("\n"));
        }
        
        wxTheClipboard->Close();
    }

    // get the textbox's text
    wxString text;
    text = getText();

    // if there are selected characters, then delete them
    deleteSelectedChars(text, KEYSTATE_NONE);

    // add the paste text after the cursor and move the
    // cursor forward
    wxString t1 = text.Mid(0, getCursorPos()).Append(paste_text);
    wxString t2 = text.Mid(getCursorPos());
    setText(t1 + t2);

    // increment the cursor position
    setCursorPos(getCursorPos() + paste_text.Length());
}

void CompTextBox::selectAll()
{
    setSelection(0, getText().Length());
}

void CompTextBox::selectNone()
{
    clearSelection();
}

void CompTextBox::clear(bool text)
{
    // note: this function clears the contents of the text
    // box; if text is true, reset the text property, otherwise
    // reset all the properties
    if (text)
    {
        setProperty(PROP_CONTENT_VALUE, wxString(wxT("")));
        return;
    }

    initProperties(m_properties);
}

void CompTextBox::onKey(IEventPtr evt)
{
    // if we're not in the at-target phase,
    // we're done
    if (evt->getPhase() != EVENT_AT_TARGET)
        return;

    IKeyEventPtr key_evt = evt;
    if (key_evt.isNull())
        return;

    // get the key state
    int keystate = 0;
    keystate |= (key_evt->isCtrlDown() ? KEYSTATE_CTRL_DOWN : 0);
    keystate |= (key_evt->isShiftDown() ? KEYSTATE_SHIFT_DOWN : 0);
    keystate |= (key_evt->isAltDown() ? KEYSTATE_ALT_DOWN : 0);
    keystate |= (::wxGetKeyState(WXK_INSERT) ? KEYSTATE_INSERT_DOWN : 0);

    // get the key code and process the key event
    int key_code = key_evt->getKeyCode();
    switch (key_code)
    {
        case WXK_LEFT:
        case WXK_NUMPAD_LEFT:
            moveCursor(0, -1, keystate);
            break;

        case WXK_RIGHT:
        case WXK_NUMPAD_RIGHT:
            moveCursor(0, 1, keystate);
            break;
            
        case WXK_UP:
        case WXK_NUMPAD_UP:
            moveCursor(-1, 0, keystate);
            break;

        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            moveCursor(1, 0, keystate);
            break;

        case WXK_HOME:
        case WXK_NUMPAD_HOME:
            moveBeginning(keystate);
            break;
            
        case WXK_END:
        case WXK_NUMPAD_END:
            moveEnd(keystate);
            break;

        case WXK_PAGEUP:
        case WXK_NUMPAD_PAGEUP:
            break;

        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEDOWN:
            break;

        case WXK_BACK:
            deleteCharBeforeCursor(keystate);
            break;

        case WXK_DELETE:
            deleteCharAfterCursor(keystate);
            break;
        
        default:
            insertCharAtCursor(key_code, keystate);
            break;
    }

    invalidate();
}

void CompTextBox::onMouse(IEventPtr evt)
{
    // if we're not in the at-target phase,
    // we're done
    if (evt->getPhase() != EVENT_AT_TARGET)
        return;

    IMouseEventPtr mouse_evt = evt;
    if (mouse_evt.isNull())
        return;

    if (m_mouse_action == ACTION_NONE)
    {
    }

    // set the mouse state variables
    m_mouse_x = mouse_evt->getX();
    m_mouse_y = mouse_evt->getY();

    // get the event name
    wxString name = evt->getName();

    // if the event is a mouse-over event, dispatch
    // a cursor event
    if (name == EVENT_MOUSE_OVER)
    {
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_CANVAS_CURSOR, this);
        notify_evt->addProperty(EVENT_PROP_CURSOR, CURSOR_TEXTEDIT);
        dispatchEvent(notify_evt);
    }

    // if the event is a double-click event, set the
    // insertion point, and we're done
    if (name == EVENT_MOUSE_LEFT_DCLICK)
    {
        // if the shift key isn't down, clear the selection
        if (!mouse_evt->isShiftDown())
            clearSelection();

        // set a new cursor position
        setInsertionPointByPos(mouse_evt->getX(), mouse_evt->getY());

        // find the text position of the double-click
        int x = m_mouse_x;
        int y = m_mouse_y;
        int pos = xyToPos(&x, &y);

        // find the word corresponding to the text position of the 
        // double-click
        int x1 = findPrevWord(pos);
        int x2 = findNextWord(pos);

        // if the shift key is down and we have a selection,
        // adjust x1 and x2 to include the previous selection
        if (mouse_evt->isShiftDown() && hasSelection())
        {
            int selection_x1, selection_x2;
            getSelection(&selection_x1, &selection_x2, true);

            x1 = wxMin(x1, selection_x1);
            x2 = wxMax(x2, selection_x2);
        }

        // set the selection area and set the new cursor position to 
        // the end of the selection area        
        posToXY(x2, &x, &y);
        setSelection(x1, x2);
        setInsertionPointByPos(x, y);
        
        // invalidate
        invalidate();
        
        // return; we're done
        return;
    }

    // start the selection event
    if (startAction(mouse_evt, ACTION_SELECT))
    {
        // if the shift key isn't down, clear the selection
        if (!mouse_evt->isShiftDown())
            clearSelection();

        // if we don't have a selection, set an initial selection 
        // starting point from the initial cursor position
        int cursor_pos = getCursorPos();
        if (!hasSelection())
            setSelection(cursor_pos, cursor_pos);

        // set the new cursor position
        setInsertionPointByPos(mouse_evt->getX(), mouse_evt->getY());

        // if the shift key is down, set a selection from the intial 
        // selection start to the new cursor position; otherwise, reset
        // the selection start to the new cursor position
        cursor_pos = getCursorPos();
        if (mouse_evt->isShiftDown())
            setSelection(getSelectionStart(), cursor_pos);
        else
            setSelection(cursor_pos, cursor_pos);

        // invalidate
        invalidate();
    }
    
    if (endAction(mouse_evt, ACTION_SELECT))
    {
        // invalidate
        invalidate();
    }
    
    if (isAction(mouse_evt, ACTION_SELECT))
    {
        // set the new cursor position
        setInsertionPointByPos(mouse_evt->getX(), mouse_evt->getY());

        // set a selection
        setSelection(getSelectionStart(), getCursorPos());

        // invalidate
        invalidate();
    }
}

bool CompTextBox::startAction(IEventPtr evt, wxString action)
{
    // if the event is a left-mouse button down and we
    // don't already have an action, set the action and 
    // return true
    if (evt->getName() == EVENT_MOUSE_LEFT_DOWN &&
        m_mouse_action == ACTION_NONE)
    {
        // set the mouse action
        m_mouse_action = action;

        // set the starting mouse position
        m_mouse_x_start = m_mouse_x;
        m_mouse_y_start = m_mouse_y;

        return true;
    }
    
    // otherwise, return false
    return false;
}

bool CompTextBox::endAction(IEventPtr evt, wxString action)
{
    // if the event is a left-mouse button up and we
    // already have an action that matches the input 
    // action, reset the action and return true
    if (evt->getName() == EVENT_MOUSE_LEFT_UP &&
        m_mouse_action == action)
    {
        m_mouse_action = ACTION_NONE;
        return true;
    }
    
    // otherwise, return false
    return false;
}

bool CompTextBox::isAction(IEventPtr evt, wxString action)
{
    // if the event is a mouse move event and we
    // already have an action that matches the input 
    // action, return true
    if (evt->getName() == EVENT_MOUSE_MOTION &&
        m_mouse_action == action)
    {
        return true;
    }

    // otherwise, return false
    return false;
}

void CompTextBox::resetAction()
{
    // reset the mouse action state; this is used to bail out
    // of an action if some condition in the start action isn't 
    // true, so other handlers have a chance to process the event
    m_mouse_action = ACTION_NONE;
}

void CompTextBox::posToXY(int pos, int* x, int* y)
{
    // if we don't have a canvas, set the position to (0,0),
    // and we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
    {
        *x = 0;
        *y = 0;
        return;
    }

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // make sure the position is at least zero
    if (pos < 0)
        pos = 0;

    // find the line with the position by iterating through the
    // lines in reverse order and finding the first line where
    // the offset is less than the position; NOTE: we'll always
    // have at least one line because we inserted a blank line
    // when we populated the cache
    std::vector<TextProperties>::reverse_iterator itr, itr_end;
    itr_end = m_text_lines.rend();
    
    for (itr = m_text_lines.rbegin(); itr != m_text_lines.rend(); ++itr)
    {
        if (itr->m_starting_idx <= pos)
            break;
    }
    
    // if the string is empty, set the x and y positions to the 
    // positions of the line, and we're done
    if (itr->m_string.IsEmpty())
    {
        *x = itr->m_xpos;
        *y = itr->m_ypos;
        return;
    }

    // get the x and y location of the position relative to the
    // line containing the position; make sure we set the font
    // before we get the extent in case somebody else has
    // changed it
    wxString strpart = itr->m_string.Mid(0, pos - itr->m_starting_idx);
    canvas->setFont(m_cache_font);
    canvas->getTextExtent(strpart, x, y);

    // add the x location of the position in the string to the
    // x and y location of the offset and set the return values
    *x = *x + itr->m_xpos;
    *y = itr->m_ypos;
}

int CompTextBox::xyToPos(int* x, int* y)
{
    // note: this function takes an x and y location and
    // returns the position in the string that's closest
    // to that location, adjusting the x and y input to
    // reflect the actual location of the position

    // if we don't have a canvas, return 0, and we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return 0;

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // make sure the y position isn't less than the y position of 
    // the first line; note: we'll always have at least one line 
    // because we inserted a blank line when we populated the cache
    int y_min = m_text_lines.begin()->m_ypos;
    if (*y < y_min)
        *y = y_min;

    // find the line with the x and y position by iterating
    // through the lines in reverse order and finding the first
    // line where the y position of the line is less than the
    // input y position we're looking for
    std::vector<TextProperties>::reverse_iterator itr, itr_end;
    itr_end = m_text_lines.rend();
    
    for (itr = m_text_lines.rbegin(); itr != m_text_lines.rend(); ++itr)
    {
        if (itr->m_ypos <= *y)
            break;
    }

    // if the x position is in the middle of the line, find the 
    // character position where it's located; make sure we
    // set the font in case somebody else has changed it
    wxArrayInt widths;
    canvas->setFont(m_cache_font);
    canvas->getPartialTextExtents(itr->m_string, widths);
    
    wxArrayInt::iterator it, it_end;
    it_end = widths.end();

    int x_input = *x;
    int x_new = 0;
    int x_old = 0;
    int x_avg = 0;
    
    // set the x and y inputs to the end of the line in
    // case we don't have any widths
    *x = itr->m_xpos + itr->m_width;
    *y = itr->m_ypos;
    
    // iterate through the widths
    for (it = widths.begin(); it != it_end; ++it)
    {
        // use the the center of each character to determine the
        // position, so we measure the position relative to the 
        // average of the extent of the word up to the current 
        // character and the extent of the word up to the last 
        // character
        x_new = *it;
        x_avg = (x_new + x_old)/2;

        if (x_input < itr->m_xpos + x_avg)
        {
            // adjust the inputs to correspond to the position
            *x = itr->m_xpos + x_old;
            *y = itr->m_ypos;
            break;
        }

        x_old = *it;
    }

    int char_offset = it - widths.begin();
    int line_offset = itr->m_starting_idx;
    
    // return the position
    return line_offset + char_offset;
}

int CompTextBox::findEndOfLine(int* x, int* y)
{
    // set the x value of the location to a large positive value 
    // and return the position and location of the new position
    *x = INT_MAX;
    return xyToPos(x, y);
}

int CompTextBox::findBegOfLine(int* x, int* y)
{
    // set the x value of the location to a large negative value 
    // and return the position and location of the new position
    *x = -INT_MAX;
    return xyToPos(x, y);
}

int CompTextBox::findNextWord(int pos)
{
    // get the text
    wxString text = getText();

    // iterate through the characters
    wxChar c;
    bool after_last_word = false;
    int length = text.Length();
    while (pos < length)
    {
        // get the character
        c = text[pos];
        
        // if the character is a white space, we're in the 
        // space after the word
        if (isWhiteSpace(c) && !after_last_word)
            after_last_word = true;

        // if we've been in the space after a word and the
        // the current character isn't a space, we've found
        // the start of the next word, so break
        if (after_last_word && !isWhiteSpace(c))
            break;

        // increment the position            
        ++pos;
    }
    
    // return the position
    return pos;
}

int CompTextBox::findPrevWord(int pos)
{
    // get the text
    wxString text = getText();

    // iterate through the characters
    wxChar c;
    bool in_word = false;
    while (pos > 0)
    {
        // get the character before the current position
        c = text[pos-1];
        
        // if the character is not a white space, and we haven't
        // been in a word, then we'll be in a word
        if (!isWhiteSpace(c) && !in_word)
            in_word = true;

        // if we're in a word and the character is a space, we've 
        // found the start of the word, so break
        if (in_word && isWhiteSpace(c))
            break;

        // decrement the position
        --pos;
    }
    
    // return the position
    return pos;
}

void CompTextBox::moveBeginning(int keystate)
{
    // note: this function moves the cursor to the beginning of 
    // the line of text if the control key isn't down and the 
    // beginning of the text if the control key is down

    // if the shift key isn't down, clear the selection
    if (!(keystate & KEYSTATE_SHIFT_DOWN))
        clearSelection();

    // calculate the selection start in case we need to set
    // a selection after moving the cursor
    int selection_start = getCursorPos();
    if (hasSelection())
        selection_start = getSelectionStart();

    // set an initial new cursor position for the beginning of
    // the text
    int new_cursor_pos = 0;

    // if the control key isn't down, reset the offset from the 
    // beginning of the text position to the beginning of the 
    // line position
    if (!(keystate & KEYSTATE_CTRL_DOWN))
    {
        // find the beginning of the line position
        int x, y;
        getInsertionPointByPos(&x, &y);
        new_cursor_pos = findBegOfLine(&x, &y);
    }

    // set the cursor position
    setCursorPos(new_cursor_pos);

    // if the shift key is down, set a selection
    if (keystate & KEYSTATE_SHIFT_DOWN)
        setSelection(selection_start, getCursorPos());
}

void CompTextBox::moveEnd(int keystate)
{
    // note: this function moves the cursor to the end of the
    // line of text if the control key isn't down and the end
    // of the text if the control key is down

    // if the shift key isn't down, clear the selection
    if (!(keystate & KEYSTATE_SHIFT_DOWN))
        clearSelection();

    // calculate the selection start in case we need to set
    // a selection after moving the cursor
    int selection_start = getCursorPos();
    if (hasSelection())
        selection_start = getSelectionStart();

    // if the control key is down, set the new cursor position
    // to the end of the text; otherwise, set the new cursor
    // position to the end of the line            
    if (keystate & KEYSTATE_CTRL_DOWN)
    {
        // get the text
        wxString text = getText();
        setCursorPos(text.Length());
    }
    else
    {
        // find the end of the line position
        int x, y, cursor_pos;
        getInsertionPointByPos(&x, &y);
        cursor_pos = findEndOfLine(&x, &y);

        // set the new cursor position
        setCursorPos(cursor_pos);

        // if the control key isn't down, we're moving to the end of 
        // the line, so we need to set the insertion point explicitly;
        // the reason for this is that the setCursorPos() function set 
        // the cursor by position; however, because it is position based, 
        // it places the cursor at the beginning of the next line, which 
        // is normally correct; however, for the end of line command, 
        // it's better to show the cursor at the end of the same line, 
        // and we need to use setInsertionPointByPos() for this
        if (!(keystate & KEYSTATE_CTRL_DOWN))
            setInsertionPointByPos(x, y);
    }
    
    // if the shift key is down, set a selection
    if (keystate & KEYSTATE_SHIFT_DOWN)
        setSelection(selection_start, getCursorPos());
}

void CompTextBox::setCursorPos(int pos)
{
    // clear the cursor position cache
    m_cache_cursor_x_last = -1;
    m_cache_cursor_x = -1;
    m_cache_cursor_y = -1;

    // get the text
    wxString text = getText();
    int length = text.Length();
    
    // if the position is less than zero, set the position to
    // zero and we're done
    if (pos < 0)
    {
        m_cursor_pos = 0;
        return;
    }
    
    // if the position is greater than the length, set the
    // positioin to the length, and we're done
    if (pos > length)
    {
        pos = length;
        return;
    }
    
    // if the position is greater than zero, see if we're 
    // in the middle of a return/linefeed combination, and if 
    // so, adjust the position so the cursor is immediately 
    // after the linefeed
    if (pos > 0)
    {
        if (text.GetChar(pos-1) == 0x0d &&
            text.GetChar(pos) == 0x0a)
        {
            pos++;
        }
    }

    m_cursor_pos = pos;
}

int CompTextBox::getCursorPos() const
{
    return m_cursor_pos;
}

void CompTextBox::moveCursor(int row_diff, int col_diff, int keystate)
{
    // if the shift key isn't down, clear the selection if it exists
    if (!(keystate & KEYSTATE_SHIFT_DOWN))
        clearSelection();
        
    // save the cursor position
    int cursor_pos_start = getCursorPos();

    // increment the row
    if (row_diff != 0)
    {
        // find the x and y location of the cursor
        int x, y;
        getInsertionPointByPos(&x, &y);
        
        // if we a previous cursor position, use it as the x position;
        // this is so we can return back to the same position again
        // if the row changed and changed back
        if (m_cache_cursor_x_last != -1)
            x = m_cache_cursor_x_last;

        // increment the y position and set the new insertion point
        y += row_diff*m_cache_line_size;
        setInsertionPointByPos(x, y);

        // save the x location; it will be reset if the cursor column 
        // position is changed or the insertion point is set directly     
        m_cache_cursor_x_last = x;
    }

    // increment the column
    if (col_diff != 0)
    {
        // clear the cursor position cache
        m_cache_cursor_x_last = -1;
        m_cache_cursor_x = -1;
        m_cache_cursor_y = -1;

        // initially, use the col_diff as the offset as the number of columns
        // to move the cursor; however, if the control key is down, set the
        // offset so that the cursor will move forward or backward by one
        // word, based on the sign of col_diff
        int offset = col_diff;

        if (keystate & KEYSTATE_CTRL_DOWN && col_diff > 0)
            offset = findNextWord(cursor_pos_start) - cursor_pos_start;

        if (keystate & KEYSTATE_CTRL_DOWN && col_diff < 0)
            offset = findPrevWord(cursor_pos_start) - cursor_pos_start;

        // save the current cursor position and set the location
        // of the new cursor position
        setCursorPos(cursor_pos_start + offset);
        
        // if we're moving the cursor back by one and the cursor position hasn't 
        // changed, it might be because we're trying to set it in between a 
        // return/linefeed combination which will move the cursor to the end of
        // the linefeed; in this case, move the cursor back by two; if it still
        // doesn't move, it's because we're at the beginning of the text
        if (col_diff == -1 && cursor_pos_start == getCursorPos())
            setCursorPos(cursor_pos_start - 2);
    }
    
    // if the control key is down, set a selection
    if (keystate & KEYSTATE_SHIFT_DOWN)
    {
        // if the selection isn't set, set a new selection
        // from the old cursor position to the new position;
        // otherwise, set a selection from the start of the
        // old selection to the new cursor position;
        int start = cursor_pos_start;
        if (hasSelection())
            start = getSelectionStart();
        
        setSelection(start, getCursorPos());
    }
}

void CompTextBox::insertCharAtCursor(int keycode, int keystate)
{
    if (!validDisplayChar(keycode, keystate))
        return;

    // get the text
    wxString text = getText();

    // if there are selected characters, then delete them
    deleteSelectedChars(text, keystate);

    // add the character after the cursor
    int cursor_start = getCursorPos();
    wxString t1 = text.Mid(0, cursor_start).Append((wxChar)keycode);
    wxString t2 = text.Mid(cursor_start);
    setText(t1 + t2);

    // set the new cursor position
    setCursorPos(t1.Length());
}

void CompTextBox::deleteCharBeforeCursor(int keystate)
{
    // get the text
    wxString text = getText();

    // if we delete selected characters, set the text and we're done
    if (deleteSelectedChars(text, keystate))
    {
        setText(text);
        return;
    }

    // if the cursor position is outside the range where
    // there's a character before the cursor to delete, 
    // we're done
    int cursor_start = getCursorPos();
    if (cursor_start < 1 || cursor_start > (int)text.Length())
        return;

    // move the cursor back by one and set the text; the reason
    // we move the cursor, rather than just delete the character 
    // is because return/linefeed combos take up two characters, 
    // and moveCursor() accounts for this
    moveCursor(0, -1, KEYSTATE_NONE);
    setText(text.Mid(0, getCursorPos()) + text.Mid(cursor_start));
}

void CompTextBox::deleteCharAfterCursor(int keystate)
{
    // get the text
    wxString text = getText();

    // if we delete selected characters, set the text and we're done
    if (deleteSelectedChars(text, keystate))
    {
        setText(text);
        return;
    }

    // if the cursor position is outside the range where
    // we there's a character after the cursor to delete, 
    // we're done
    int cursor_start = getCursorPos();
    if (cursor_start < 0 || cursor_start >= (int)text.Length())
        return;

    // move the cursor forward by one and set the text; the reason
    // we move the cursor, rather than just delete the character is 
    // because return/linefeed combos take up two characters, and 
    // moveCursor() accounts for this
    moveCursor(0, 1, KEYSTATE_NONE);
    setText(text.Mid(0, cursor_start) + text.Mid(getCursorPos()));
    setCursorPos(cursor_start);
}

bool CompTextBox::deleteSelectedChars(wxString& text, int keystate)
{
    // if we don't have a selection, we're done
    if (!hasSelection())
        return false;
    
    // set the cursor position to the start of the selection
    int x1, x2;
    getSelection(&x1, &x2, true);

    // cut out the selection and set the new string
    wxString t1 = text.Mid(0, x1);
    wxString t2 = text.Mid(x2);
    text = t1 + t2;

    // set the new cursor position, clear the selection
    // and we're done
    setCursorPos(x1);
    clearSelection();
    return true;
}

bool CompTextBox::validDisplayChar(int keycode, int keystate)
{
    // if the keycode is a line terminator, the character is valid;
    // return true
    if (isLineTerminator(keycode))
        return true;

    // if the control key is down, the character isn't valid
    if (keystate & KEYSTATE_CTRL_DOWN)
        return false;

    switch (keycode)
    {
        // if it's not in the list of keycodes, it's valid
        default:
            return true;

        // if it's in the list of keycodes, it's valid if it's
        // one of the following
        case WXK_SPACE:
        case WXK_RETURN:
            return true;
    
        // if it's any of the other keycodes, it's invalid
        case WXK_BACK:
        case WXK_TAB:
        case WXK_ESCAPE:
        case WXK_DELETE:
        case WXK_START:
        case WXK_LBUTTON:
        case WXK_RBUTTON:
        case WXK_CANCEL:
        case WXK_MBUTTON:
        case WXK_CLEAR:
        case WXK_SHIFT:
        case WXK_ALT:
        case WXK_CONTROL:
        case WXK_MENU:
        case WXK_PAUSE:
        case WXK_CAPITAL:
        case WXK_END:
        case WXK_HOME:
        case WXK_LEFT:
        case WXK_UP:
        case WXK_RIGHT:
        case WXK_DOWN:
        case WXK_SELECT:
        case WXK_PRINT:
        case WXK_EXECUTE:
        case WXK_SNAPSHOT:
        case WXK_INSERT:
        case WXK_HELP:
        case WXK_NUMPAD0:
        case WXK_NUMPAD1:
        case WXK_NUMPAD2:
        case WXK_NUMPAD3:
        case WXK_NUMPAD4:
        case WXK_NUMPAD5:
        case WXK_NUMPAD6:
        case WXK_NUMPAD7:
        case WXK_NUMPAD8:
        case WXK_NUMPAD9:
        case WXK_MULTIPLY:
        case WXK_ADD:
        case WXK_SEPARATOR:
        case WXK_SUBTRACT:
        case WXK_DECIMAL:
        case WXK_DIVIDE:        
        case WXK_F1:
        case WXK_F2:
        case WXK_F3:
        case WXK_F4:
        case WXK_F5:
        case WXK_F6:
        case WXK_F7:
        case WXK_F8:
        case WXK_F9:
        case WXK_F10:
        case WXK_F11:
        case WXK_F12:
        case WXK_F13:
        case WXK_F14:
        case WXK_F15:
        case WXK_F16:
        case WXK_F17:
        case WXK_F18:
        case WXK_F19:
        case WXK_F20:
        case WXK_F21:
        case WXK_F22:
        case WXK_F23:
        case WXK_F24:
        case WXK_NUMLOCK:
        case WXK_SCROLL:
        case WXK_PAGEUP:
        case WXK_PAGEDOWN:
        case WXK_NUMPAD_SPACE:        
        case WXK_NUMPAD_TAB:
        case WXK_NUMPAD_ENTER:
        case WXK_NUMPAD_F1:
        case WXK_NUMPAD_F2:
        case WXK_NUMPAD_F3:
        case WXK_NUMPAD_F4:
        case WXK_NUMPAD_HOME:
        case WXK_NUMPAD_LEFT:
        case WXK_NUMPAD_UP:
        case WXK_NUMPAD_RIGHT:
        case WXK_NUMPAD_DOWN:
        case WXK_NUMPAD_PAGEUP:
        case WXK_NUMPAD_PAGEDOWN:
        case WXK_NUMPAD_END:
        case WXK_NUMPAD_BEGIN:
        case WXK_NUMPAD_INSERT:
        case WXK_NUMPAD_DELETE:
        case WXK_NUMPAD_EQUAL:
        case WXK_NUMPAD_MULTIPLY:
        case WXK_NUMPAD_ADD:
        case WXK_NUMPAD_SEPARATOR:
        case WXK_NUMPAD_SUBTRACT:
        case WXK_NUMPAD_DECIMAL:
        case WXK_NUMPAD_DIVIDE:        
        case WXK_WINDOWS_LEFT:
        case WXK_WINDOWS_RIGHT:
        case WXK_WINDOWS_MENU:
#ifndef __WXMAC__
#if !wxCHECK_VERSION(2,9,3)
        case WXK_COMMAND:
#endif
#endif
        case WXK_SPECIAL1:
        case WXK_SPECIAL2:
        case WXK_SPECIAL3:
        case WXK_SPECIAL4:
        case WXK_SPECIAL5:
        case WXK_SPECIAL6:
        case WXK_SPECIAL7:
        case WXK_SPECIAL8:
        case WXK_SPECIAL9:
        case WXK_SPECIAL10:
        case WXK_SPECIAL11:
        case WXK_SPECIAL12:
        case WXK_SPECIAL13:
        case WXK_SPECIAL14:
        case WXK_SPECIAL15:
        case WXK_SPECIAL16:
        case WXK_SPECIAL17:
        case WXK_SPECIAL18:
        case WXK_SPECIAL19:
        case WXK_SPECIAL20:
            return false;
    }
}

void CompTextBox::setSelection(int x1, int x2)
{
    // if either of the inputs are less than zero clear the 
    // selection, and we're done
    if (x1 < 0 || x2 < 0)
    {
        clearSelection();
        return;
    }
    
    // set the selection
    m_selection_x1 = x1;
    m_selection_x2 = x2;
}

void CompTextBox::clearSelection()
{
    // reset the selection
    m_selection_x1 = -1;
    m_selection_x2 = -1;
}

bool CompTextBox::hasSelection() const
{
    // if the selection endpoints are different and not
    // equal to -1, we have a selection; return true
    if (m_selection_x1 != m_selection_x2 &&
        m_selection_x1 >= 0 && m_selection_x2 >= 0)
    {
        return true;
    }
    
    // otherwise, return false;
    return false;
}

void CompTextBox::getSelection(int* x1, int* x2, bool ordered) const
{
    // note: this function returns the selection area; if ordered is
    // false, then x1 is the start of the selection and x2 is the end 
    // of the selection in the order in which the selection was created;
    // if ordererd is true, then x1 and x2 are sorted so that x1 is the
    // smaller position of the selection and x2 is the higher position
    // is true, then x1
    *x1 = m_selection_x1;
    *x2 = m_selection_x2;
    
    if (m_selection_x1 > m_selection_x2 && ordered)
    {
        int t = *x1;
        *x1 = *x2;
        *x2 = t;
    }
}

int CompTextBox::getSelectionStart() const
{
    return m_selection_x1;
}

int CompTextBox::getSelectionEnd() const
{
    return m_selection_x2;
}

bool CompTextBox::isCacheEmpty() const
{
    if (m_cache_line_size == -1)
        return true;

    return false;
}

void CompTextBox::populateCache()
{
    // property value
    PropertyValue value;

    // clear the cache
    clearCache();

    // if we don't have a canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // set the font
    PropertyValue font_facename;
    PropertyValue font_size;
    PropertyValue font_style;
    PropertyValue font_weight;
    PropertyValue font_underscore;

    getProperty(PROP_FONT_FACENAME, font_facename);
    getProperty(PROP_FONT_SIZE, font_size);
    getProperty(PROP_FONT_STYLE, font_style);
    getProperty(PROP_FONT_WEIGHT, font_weight);
    getProperty(PROP_FONT_UNDERSCORE, font_underscore);

    setFontFromProperties(font_facename,
                          font_size,
                          font_style,
                          font_weight,
                          font_underscore,
                          m_cache_font);

    canvas->setFont(m_cache_font);

    // set the line spacing
    getProperty(PROP_TEXT_SPACING, value);
    m_cache_line_size = wxMax(0, ((value.getInteger())*canvas->getCharHeight())/100);

    // get the width of this component; subtract a small amount of 
    // space from the width so the text draws clearly
    int hpad = TEXTBOX_HORZ_PIXEL_BUFF;
    int m_cache_width = wxMax(0, getWidth() - 2*canvas->dtom_x(hpad));

    // get the height of this component; subtract a small amount of 
    // space from the height so the text draws clearly
    int vpad = TEXTBOX_VERT_PIXEL_BUFF;
    int m_cache_height = wxMax(0, getHeight() - 2*canvas->dtom_y(vpad));

    // set the horizontal alignment
    getProperty(PROP_TEXT_HALIGN, value);
    m_cache_halign = value.getString();
    
    // set the vertical alignment
    getProperty(PROP_TEXT_VALIGN, value);
    m_cache_valign = value.getString();

    // get the wrap property
    getProperty(PROP_TEXT_WRAP, value);
    bool wrap = value.getBoolean();

    // TODO: clean up line feed characters that may be on 
    // the end of the line; if flag is set, replace any 
    // meta-tags with appropriate values
    
    // break up the string into paragraph chunks
    wxString text = getText();
    TextProperties text_part;
 
    LineTokenizer p(canvas, text, m_cache_width, wrap);
    while (p.hasMoreLines())
    {
        text_part.m_string = p.getNextLine();
        text_part.m_starting_idx = p.getLineBeginning();
        text_part.m_ending_idx = p.getLineEnding();
        text_part.m_width = p.getLineWidth();

        m_text_lines.push_back(text_part);
        continue;
    }

    // special case: if the string is empty or the last character is
    // a line terminator, add on a blank line; this is not technically 
    // a separate line the tokenizer should return, since it's the end
    // of the preceding line; however it's necessary for placing the 
    // cursor properly on a new blank line as well as necessary for 
    // correct vertical alignment
    if (text.IsEmpty() || isLineTerminator(text.Last()))
    {
        text_part.m_string = wxT("");
        text_part.m_starting_idx = text.Length();
        text_part.m_ending_idx = text.Length();
        text_part.m_width = 0;
        m_text_lines.push_back(text_part);
    }

    // iterate through the lines and set the x and y position of 
    // each of the lines, accounting for horizontal and vertical 
    // alignment
    int h_offset = canvas->dtom_x(hpad);
    int v_offset = canvas->dtom_y(vpad);
    int text_height = m_cache_line_size*m_text_lines.size();

    // if a middle or bottom vertical alignment is specified, adjust
    // the vertical position accordingly
    if (m_cache_valign == ALIGNMENT_MIDDLE)
        v_offset += (m_cache_height - text_height)/2;

    if (m_cache_valign == ALIGNMENT_BOTTOM)
        v_offset += (m_cache_height - text_height);

    std::vector<TextProperties>::iterator it, it_end;
    it_end = m_text_lines.end();
    
    for (it = m_text_lines.begin(); it != it_end; ++it)
    {
        // default position
        it->m_xpos = h_offset;
        it->m_ypos = v_offset + m_cache_line_size*(it - m_text_lines.begin());

        // if a center or right horizontal alignment is specified, 
        // adjust the horizontal position accordingly
        if (m_cache_halign == ALIGNMENT_CENTER)
            it->m_xpos = (m_cache_width - it->m_width)/2 + h_offset;

        if (m_cache_halign == ALIGNMENT_RIGHT)
            it->m_xpos = (m_cache_width - it->m_width) + h_offset;
    }
    
    // if the cursor is outside the range of the string, set it so 
    // it's at the beginning or end of the string
    if (m_cursor_pos < 0)
        m_cursor_pos = 0;
        
    if (m_cursor_pos > 0 && m_cursor_pos > (int)text.Length())
        m_cursor_pos = text.Length();
}

void CompTextBox::clearCache()
{
    m_text_lines.clear();
    m_cache_halign = wxT("");
    m_cache_valign = wxT("");
    m_cache_line_size = -1;
    m_cache_width = -1;
    m_cache_height = -1;
    m_cache_cursor_x = -1;
    m_cache_cursor_y = -1;
    m_cache_cursor_x_last = -1;
}



/*
// TODO: integrate following old code
static void caseInsensitiveReplace(wxString& str,
                                   const wxString& search,
                                   const wxString& replace)
{
    wxString strpart;
    wxString upper_str = str;
    wxString upper_search = search;

    upper_str.MakeUpper();
    upper_search.MakeUpper();

    int idx;
    idx = upper_str.Find(upper_search);
    
    if (idx == -1)
        return;

    strpart = str.Mid(idx + search.Length());
    caseInsensitiveReplace(strpart, search, replace);

    str = str.Left(idx) + replace + strpart;
}
*/
/*
// old tag code
wxString var;

var = wxString::Format(wxT("%d"), m_stats.page);
caseInsensitiveReplace(str_part, wxT("<page>"), var);

var = wxString::Format(wxT("%d"), m_stats.page_count);
caseInsensitiveReplace(str_part, wxT("<pages>"), var);

var = cfw::Locale::formatDate(m_stats.date_year,
                              m_stats.date_month,
                              m_stats.date_day);

caseInsensitiveReplace(str_part, wxT("<curdate>"), var);

caseInsensitiveReplace(str_part, wxT("<data>"), m_stats.data_source);
*/




}; // namespace kcanvas

