/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-28
 *
 */


#include "appmain.h"
#include "editordoc.h"
#include "appcontroller.h"
#include "dlgdatabasefile.h"
#include "dlgpagesetup.h"
#include "toolbars.h"
#include <wx/print.h>
#include <wx/stc/stc.h>
#include <kl/regex.h>
#include <kl/utf8.h>
#include <kl/md5.h>


// NOTE: the default colors for the editor doc, expression builder, and console 
// are the same; TODO: probably should factor these out
#define NORMAL_CHAR_COLOR     wxColour(0x00, 0x00, 0x00)
#define UNICODE_CHAR_COLOR    wxColour(0x00, 0x00, 0x00)
#define KEYWORD_COLOR         wxColour(0x00, 0x00, 0xff)
#define OPERATOR_COLOR        wxColour(0x00, 0x00, 0x00)
#define SINGLE_QUOTED_COLOR   wxColour(0xa0, 0x00, 0x00)
#define DOUBLE_QUOTED_COLOR   wxColour(0xa0, 0x00, 0x00)
#define NUMBER_COLOR          wxColour(0x00, 0x00, 0x00)
#define COMMENT_COLOR         wxColour(0x00, 0x80, 0x00)
#define PREPROCESSOR_COLOR    wxColour(0x80, 0x80, 0x80)


static const char* js_keywords =
    "abstract boolean break byte case catch char class const constructor "
    "continue debugger default do double else enum export extends "
    "false final finally float for function goto if implements import in include "
    "instanceof int interface long native new null package private protected "
    "public return short static strict super switch synchronized this throw throws "
    "transient true try typeof undefined use var void volatile while with";

static const char* aspvb_keywords =
    "and as boolean byref byte bycal call case class const currency debug dim do double "
    "each else elseif empty end endif enum eqv event exit false for function get goto if "
    "imp implements in integer is let like long loop lset me mod new next not nothing "
    "null on option optional or paramarray preserve private public raiseevent redim "
    "rem resume rset select set shared single static stop sub then to true type typeof "
    "until variant wend while with xor";

static const char* html_keywords = "html div link script title pre response";
    
    
    
static wxString stripCRLF(const wxString& _str)
{
    wxString str = _str;
    while (str.Length() > 0 &&(str.Last() == wxT('\r') || str.Last() == wxT('\n')))
        str.RemoveLast();
    return str;
}


// application printer class; used to print pages from
// a wxPrintout-derived class at regular intervals using
// a timer; this allows the application to continue to
// function as well as provide printer feedback using
// the job queue
class EditorDocExternalUpdateTimer : public wxTimer
{
public:

    EditorDocExternalUpdateTimer(EditorDoc* doc)
    {
        // set the document and start the timer
        m_doc = doc;
    }
    
    ~EditorDocExternalUpdateTimer()
    {
    }

public:

    // overridden wxTimer function to call the function
    // to check for external changes
    void Notify()
    {
        // check for external changes
        m_doc->checkForExternalChanges();
        m_doc->m_timer = NULL;

        // delete this object after we're done
        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);
    }

private:

    EditorDoc* m_doc;    
};


class EditorCtrl : public wxStyledTextCtrl
{
public:
    EditorCtrl(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
            : wxStyledTextCtrl(parent, 18543, pos, size, style)
    {
        #ifdef __WXMSW__
        wxFont font(10, wxFONTFAMILY_MODERN,
                    wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL,
                    false,
                    wxT("Courier New"));
        #else
        wxFont font(10, wxFONTFAMILY_MODERN,
                    wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL);
        #endif
              
        StyleSetFont(wxSTC_STYLE_DEFAULT, font);
        StyleClearAll();
        
        // set selection foreground and background color
        SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
        SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
        
        StyleSetForeground(wxSTC_C_DEFAULT, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_COMMENT, COMMENT_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTLINE, COMMENT_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTDOC, COMMENT_COLOR);
        StyleSetForeground(wxSTC_C_NUMBER, NUMBER_COLOR);
        StyleSetForeground(wxSTC_C_WORD, KEYWORD_COLOR);
        StyleSetForeground(wxSTC_C_STRING, DOUBLE_QUOTED_COLOR);
        StyleSetForeground(wxSTC_C_CHARACTER, SINGLE_QUOTED_COLOR);
        StyleSetForeground(wxSTC_C_UUID,  NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_PREPROCESSOR,  PREPROCESSOR_COLOR);
        StyleSetForeground(wxSTC_C_OPERATOR, OPERATOR_COLOR);
        StyleSetForeground(wxSTC_C_IDENTIFIER, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_STRINGEOL, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_VERBATIM, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_REGEX, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTLINEDOC, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_WORD2, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORD, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORDERROR, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_C_GLOBALCLASS, NORMAL_CHAR_COLOR);
        
        StyleSetForeground(wxSTC_HBA_DEFAULT, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_HBA_COMMENTLINE, COMMENT_COLOR);
        StyleSetForeground(wxSTC_HBA_NUMBER, NUMBER_COLOR);
        StyleSetForeground(wxSTC_HBA_WORD, KEYWORD_COLOR);
        StyleSetForeground(wxSTC_HBA_STRING, DOUBLE_QUOTED_COLOR);
        StyleSetForeground(wxSTC_HBA_IDENTIFIER, NORMAL_CHAR_COLOR);
        StyleSetForeground(wxSTC_HBA_STRINGEOL, NORMAL_CHAR_COLOR);
        
        StyleSetForeground(wxSTC_STYLE_BRACELIGHT, NORMAL_CHAR_COLOR);
        StyleSetBold(wxSTC_STYLE_BRACELIGHT, true);

        // set the types of changes we want to use to trigger
        // the document change signal
        SetModEventMask(wxSTC_MOD_INSERTTEXT | wxSTC_MOD_DELETETEXT);

        //MarkerDefine(0, wxSTC_MARK_ARROW);
        MarkerDefineBitmap(0, GETBMPSMALL(gf_exclamation));

        EmptyUndoBuffer();
        
        // set the left and right margins
        SetMargins(2,2);
        
        // show line numbers in the first left margin
        SetMarginType(0, wxSTC_MARGIN_NUMBER);
        SetMarginType(1, wxSTC_MARGIN_SYMBOL);
        
        // make sure margin 1 has the proper width
        SetMarginWidth(1,20);
        
        // allow other lexers to work 'well'
        SetStyleBits(7);
        
        // set the line number margin width
        updateLineNumberMarginWidth();
        
        SetUseTabs(false);
        SetIndent(4);
        
        // horizontal scrollbars weren't being shown
        // on Linux... perhaps this helps???
        SetUseVerticalScrollBar(true);
        SetUseHorizontalScrollBar(true);
        
        m_show_line_numbers = true;
        m_show_syntax_highlighting = true;
        m_lexer = ""; // default lexer
        m_last_pos = -1;
        m_local_modified = false;
        UpdateLexer();
    }
    

    void SetLexerByFilename(const wxString& filename)
    {
        wxString ext = filename.AfterLast(wxT('.'));
        ext.MakeUpper();
        
        m_lexer = ext;
        UpdateLexer();
    }
    
    void UpdateLexer()
    {
        if (m_show_syntax_highlighting)
        {
            /*
            if (m_lexer == wxT("HTML") || m_lexer == wxT("PHTML"))
            {
                SetLexer(wxSTC_LEX_HTML);
                wxString keywords1 = wxString::From8BitData(js_keywords);
                wxString keywords2 = wxString::From8BitData(html_keywords);
                SetKeyWords(2, keywords1 + wxT(" ") + keywords2);
            }
            else */
            if (m_lexer == wxT("ASP"))
            {
                SetLexer(wxSTC_LEX_HTML);
                SetKeyWords(2, wxString::From8BitData(aspvb_keywords));
                
                // set control to use vbscript as the default embedded language
                SetProperty(wxT("asp.default.language"), wxT("2"));
            }
            else
            {
                // default lexer is CPP
                SetLexer(wxSTC_LEX_CPP);
                SetKeyWords(0, wxString::From8BitData(js_keywords));
            }
        }
        else
        {
            // no syntax highlighting
            SetKeyWords(0, wxEmptyString);
            SetLexer(wxSTC_LEX_NULL);
        }
        
        ClearDocumentStyle();
        Colourise(0, GetLength());
    }
    

    void SetValue(const wxString& value)
    {
        SetText(value);
        
        // make sure the line number margin width is updated
        updateLineNumberMarginWidth();
    }

    wxString GetValue()
    {
        return GetText();
    }

    bool IsModified()
    {
        return m_local_modified || GetModify();
    }
    
    void SetModified()
    {
        m_local_modified = true;
    }

    void DiscardEdits()
    {
        m_local_modified = false;
        SetSavePoint();
    }

    bool CanCut()
    {
        return true;
    }

    bool CanCopy()
    {
        return true;
    }

    void SetCursorPosition(int pos)
    {
        GotoPos(pos);
    }

    void RemoveMarkers()
    {
        MarkerDeleteAll(0);
    }

    void SetMarker(int line)
    {
        MarkerAdd(line, 0);
    }


    int FindBrace(int pos)
    {
        wxChar ch;
        wxChar brace_ch = GetCharAt(pos);
        wxChar to_find;
        int direction;
        int text_len = GetTextLength();
        
        switch (brace_ch)
        {
           case wxT('{'):
                direction = 1;
                to_find = wxT('}');
                break;

            case wxT('}'):
                direction = -1;
                to_find = wxT('{');
                break;
                
            case wxT('('):
                direction = 1;
                to_find = wxT(')');
                break;

            case wxT(')'):
                direction = -1;
                to_find = wxT('(');
                break;

            default:
                return -1;
        }

        int level = 0;
        char quote_char = 0;

        while (1)
        {
            ch = GetCharAt(pos);

            if (ch == wxT('"') || ch == wxT('\''))
            {
                if (!quote_char)
                {
                    quote_char = ch;
                }
                else
                {
                    if (quote_char == ch)
                        quote_char = 0;
                }
            }

            if (!quote_char)
            {
                if (ch == brace_ch)
                    level++;
                if (ch == to_find)
                    level--;

                if (ch == to_find && level == 0)
                    return pos;
            }

            pos += direction;

            if (pos < 0)
                break;

            if (pos >= text_len)
                break;
        }

        return -1;
    }
        
    void onIdle(wxIdleEvent& evt)
    {
        // set the line number margin width
        updateLineNumberMarginWidth();
    }
    
    void onSetFocus(wxFocusEvent& evt)
    {
        sigFocus();
        evt.Skip();
    }
    
    void onCharAdded(wxStyledTextEvent& evt)
    {
        evt.Skip();
        
        wxChar key = evt.GetKey();

        if (key == wxT('(') || key == wxT(')') || key == wxT('{') || key == wxT('}'))
        {
            int cur_pos = GetCurrentPos()-1;
            int brace_pos = FindBrace(cur_pos);

            if (brace_pos != wxSTC_INVALID_POSITION)
            {
                BraceHighlight(brace_pos, cur_pos);
                return;
            }
        }

        BraceHighlight(wxSTC_INVALID_POSITION, wxSTC_INVALID_POSITION);
                
        // only process this event where a newline character is added
        if (key != '\r' && key != '\n')
            return;
        
        // set the line number margin width
        updateLineNumberMarginWidth();
        
        // auto indent lines based on the previous line
        
        int cur_line = GetCurrentLine();
        int line_length = LineLength(cur_line);
        
        if (cur_line > 0 && line_length <= 2)
        {
            bool tabs = GetTabIndents();
            int indent_size = GetIndent();
            
            // indent size must be a positive number
            if (indent_size <= 0)
                indent_size = 4;
            
            int last_line_indent = GetLineIndentation(cur_line-1);
            wxString cur_line_indent_str;
            
            // if we're keeping tabs, add tabs to the indent string
            if (tabs)
            {
                while (last_line_indent >= indent_size)
                {
                    cur_line_indent_str += wxT('\t');
                    last_line_indent -= indent_size;
                }
            }
            
            // add spaces to the indent string
            wxString spaces(wxT(' '), last_line_indent);
            cur_line_indent_str += spaces;
            
            // indent the current line
            if (cur_line_indent_str.Length() > 0)
                ReplaceSelection(cur_line_indent_str);
        }
    }

    void onModified(wxStyledTextEvent& evt)
    {
        sigModified();
    }
    
    void onKeyDown(wxKeyEvent& evt)
    {
        // back-tab
        if (evt.GetKeyCode() == '\t' && evt.ShiftDown())
        {
            int sel_start = GetSelectionStart();
            int sel_end = GetSelectionEnd();
            int sel_start_line = LineFromPosition(sel_start);
            int sel_end_line = LineFromPosition(sel_end);
            
            // multiple lines selected; wxStyledTextCtrl handles this case properly
            if (sel_start_line != sel_end_line)
            {
                wxStyledTextCtrl::OnKeyDown(evt);
                checkCursorPos();
                return;
            }
            
            int line = sel_start_line;
            wxString line_text = stripCRLF(GetLine(line));
            wxString sel_text = stripCRLF(GetSelectedText());
            
            // whole single line selected; wxStyledTextCtrl handles this case properly
            if (sel_text.Cmp(line_text) == 0)
            {
                wxStyledTextCtrl::OnKeyDown(evt);
                checkCursorPos();
                return;
            }
            
            wxString trimmed_line_text = line_text;
            trimmed_line_text.Trim().Trim(false);
            
            int line_length = line_text.Length();
            int line_end_pos = GetLineEndPosition(line);
            int line_start_pos = line_end_pos-line_length;
            int text_start_pos = GetLineIndentPosition(line);
            int text_end_pos = text_start_pos+trimmed_line_text.Length();
            
            // at the start of the line, do nothing
            if (sel_start == line_start_pos)
                return;
            
            // selection is "inside" the line text, do nothing
            if (sel_start > text_start_pos && sel_end <= text_end_pos)
                return;
            
            // set this line as the target (text to be replaced)
            SetTargetStart(line_start_pos);
            SetTargetEnd(line_end_pos);
            
            int removed = 0;
            int indent = GetIndent();
            int offset = sel_start-line_start_pos;
            int cur_pos = GetCurrentPos();
            int anchor_pos = GetAnchor();
            wxString replace_text;
            
            // selection starts or ends "outside" the line text
            
            if (line_text.GetChar(offset-1) == wxT('\t'))
            {
                // remove the preceding tab
                removed++;
                offset--;
                replace_text = line_text.Mid(0,offset);
                replace_text += line_text.Mid(offset+removed);
            }
            else if (line_text.GetChar(offset-1) == wxT(' '))
            {
                // remove the preceding spaces (up to the indent size)
                while (offset > 0 && (indent--) > 0 &&
                       line_text.GetChar(offset-1) == wxT(' '))
                {
                    removed++;
                    offset--;
                }
                
                replace_text = line_text.Mid(0,offset);
                replace_text += line_text.Mid(offset+removed);
            }
            
            // replace the line text with the text that has the removed tab
            // or removed spaces and then move the selection area as well
            ReplaceTarget(replace_text);
            SetCurrentPos(cur_pos-removed);
            SetAnchor(anchor_pos-removed);
            checkCursorPos();
            return;
        }
        
        // non back-tab -- pass through to wxStyledTextCtrl
        wxStyledTextCtrl::OnKeyDown(evt);
        checkCursorPos();
    }
    
    void onLeftDown(wxMouseEvent& evt)
    {
        wxStyledTextCtrl::OnMouseLeftDown(evt);
        
        sigCursorMoved();
    }
    
    void checkCursorPos()
    {
        int line = GetCurrentLine();
        int column = GetColumn(GetAnchor());
        int pos = line+column;
        
        if (pos != m_last_pos)
        {
            m_last_pos = pos;
            sigCursorMoved();
        }
    }

    bool FindNextMatch(const wxString& text,
                       bool forward,
                       bool match_case,
                       bool whole)
    {
        int flags = (whole ? wxSTC_FIND_WHOLEWORD : 0) |
                    (match_case ? wxSTC_FIND_MATCHCASE : 0);
        
        // get the line number of the first visible line
        // and the anchor position
        int line_offset = GetFirstVisibleLine();
        int pos = GetAnchor();
        int x1, x2;

        if (forward)
        {
            // clear the selection and reset the search start point to the current 
            // anchor position
            GetSelection(&x1, &x2);
            SetSelection(x2, x2);
            SearchAnchor();

            // see if we find an occurrence of the string from the current
            // position in the document until the end of the doc
            x1 = SearchNext(flags, text);
            x2 = x1 + text.Length();
            
            // if we didn't find an occurrence, wrap to the top of the document
            if (x1 == -1)
            {
                SetSelection(0,0);
                SearchAnchor();
                x1 = SearchNext(flags, text);
                x2 = x1 + text.Length();
            }

            // if we still didn't find anything, restore the anchor position, 
            // and we're done
            if (x1 == -1)
            {
                SetSelection(pos,pos);
                SetAnchor(pos);
                
                // scroll past the current viewport and then scroll back
                // to the line offset we started with
                Freeze();
                ScrollToLine(GetLineCount());
                ScrollToLine(line_offset);
                Thaw();
                return false;
            }

            // if we found something, go to the position and set the selection
            GotoPos(x1);
            SetSelection(x1, x2);
        }
        else
        {
            // clear the selection and reset the search start point to the current
            // anchor position
            GetSelection(&x1, &x2);
            SetSelection(x1, x1);
            SearchAnchor();

            // see if we find an occurrence of the string from the current
            // position in the document until the beginning of the doc
            x1 = SearchPrev(flags, text);
            x2 = x1 + text.Length();
            
            // if we didn't find an occurrence, wrap to the bottom of the document
            if (x1 == -1)
            {
                wxString s = GetValue();
                int len = s.Length();
                
                SetSelection(len,len);
                SearchAnchor();
                x1 = SearchPrev(flags, text);
                x2 = x1 + text.Length();
            }

            // if we still didn't find anything, restore the anchor position,
            // and we're done
            if (x1 == -1)
            {
                SetSelection(pos,pos);
                SetAnchor(pos);
                
                // scroll past the current viewport and then scroll back
                // to the line offset we started with
                Freeze();
                ScrollToLine(GetLineCount());
                ScrollToLine(line_offset);
                Thaw();
                return false;
            }

            // if we found something, go to the position and set the selection
            GotoPos(x1);
            SetSelection(x1, x2);
        }

        return (x1 != -1 ? true : false);
    }
    
    void setTabs(bool tabs, int insert_size)
    {
        SetUseTabs(tabs);
        SetTabIndents(tabs);
        SetIndent(insert_size);
        SetTabWidth(insert_size);
    }
    
    void showLineNumbers(bool show)
    {
        m_show_line_numbers = show;
        
        if (!show)
            SetMarginWidth(0,0);
    }
    
    void showSyntaxHighlighting(bool show)
    {
        m_show_syntax_highlighting = show;
        UpdateLexer();
    }
    
    void showWhitespace(bool show)
    {
        SetViewWhiteSpace(show ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
    }
    
    void updateLineNumberMarginWidth()
    {
        if (!m_show_line_numbers)
            return;
        
        int line_count = GetLineCount();
        int width = FromDIP(100); // default
        
        // always make the width enough to show numbers up to 99
        if (line_count <= 99)
            width = TextWidth(wxSTC_STYLE_LINENUMBER, wxT("99 "));
        else if (line_count > 99 && line_count <= 999)
            width = TextWidth(wxSTC_STYLE_LINENUMBER, wxT("999 "));
        else if (line_count > 999 && line_count <= 9999)
            width = TextWidth(wxSTC_STYLE_LINENUMBER, wxT("9999 "));
        else if (line_count > 9999 && line_count < 99999)
            width = TextWidth(wxSTC_STYLE_LINENUMBER, wxT("99999 "));
        else if (line_count > 99999)
            width = TextWidth(wxSTC_STYLE_LINENUMBER, wxT("999999 "));

        SetMarginWidth(0, width);
    }
    
public: // signals

    xcm::signal0 sigCursorMoved;
    xcm::signal0 sigModified;
    xcm::signal0 sigFocus;

private:

    bool m_show_line_numbers;
    bool m_show_syntax_highlighting;
    bool m_local_modified;
    int m_last_pos;
    wxString m_lexer;
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(EditorCtrl, wxStyledTextCtrl)
    EVT_STC_MODIFIED(18543, EditorCtrl::onModified)
    EVT_STC_CHARADDED(18543, EditorCtrl::onCharAdded)
    EVT_LEFT_DOWN(EditorCtrl::onLeftDown)
    EVT_KEY_DOWN(EditorCtrl::onKeyDown)
    EVT_IDLE(EditorCtrl::onIdle)
    EVT_SET_FOCUS(EditorCtrl::onSetFocus)
END_EVENT_TABLE()




// EditorDocPrintout for EditorDoc printing

class EditorDocPrintout : public wxPrintout
{
public:

    EditorDocPrintout(EditorCtrl* ctrl,
                      wxDC* layout_dc,
                      const wxString& title = wxT("Document"))
    {
        m_editorctrl = ctrl;
        m_layout_dc = layout_dc;    // dc used for laying out document
        m_printer_dc = NULL;
        m_text_length = 0;          // total number of lines in the document
        
        m_page_width = 0.0f;        // page width in inches
        m_page_height = 0.0f;       // page height in inches
        m_margin_left = 0.0f;       // left margin in inches
        m_margin_right = 0.0f;      // right margin in inches
        m_margin_top = 0.0f;        // top margin in inches
        m_margin_bottom = 0.0f;     // bottom margin in inches
    }

    virtual ~EditorDocPrintout()
    {
    }

    bool HasPage(int page_num)
    {
        // if the page number is less than 1, return false
        if (page_num < 1)
            return false;

        // since there's one starting character position per page,
        // if the page number we're looking for is greater than
        // this, we don't have a page; return false
        if ((unsigned)page_num > m_offsets.size())
            return false;

        // return true
        return true;
    }
   
    void OnPreparePrinting()
    {
        // if the page width or height are zero, we're done
        if (m_page_width <= 0.0f || m_page_height <= 0.0f)
            return;
    
        // make sure the offsets are clear
        m_offsets.clear();
    
        // get the printer dc
        m_printer_dc = GetDC();

        // get the number of pixels per inch of the of the printer, and
        // the number of pixels per the page width and height
        int printer_dpi_x, printer_dpi_y;
        int paper_pixels_x, paper_pixels_y;
        GetPPIPrinter(&printer_dpi_x, &printer_dpi_y);
        GetPageSizePixels(&paper_pixels_x, &paper_pixels_y);
        
        // set the page rectangle
        m_page_rect = wxRect(0, 0, paper_pixels_x, paper_pixels_y);

        // set the render rectangle
        int x = (int)(m_margin_left*double(printer_dpi_x));
        int y = (int)(m_margin_top*double(printer_dpi_y));
        int w = (int)(double(paper_pixels_x) - m_margin_right*double(printer_dpi_x) - double(x));
        int h = (int)(double(paper_pixels_y) - m_margin_bottom*double(printer_dpi_y) - double(y));
        m_render_rect = wxRect(x, y, w, h);

        // determine the scaling factor so the text is the correct size
        int scale_x = printer_dpi_x/m_layout_dc->GetPPI().x;
        int scale_y = printer_dpi_y/m_layout_dc->GetPPI().y;
        
        // scale the page rectangle, render rectangle, and printer dc
        // with the scaling factor
        m_page_rect.x = m_page_rect.x/scale_x;
        m_page_rect.y = m_page_rect.y/scale_y;
        m_page_rect.width = m_page_rect.width/scale_x;
        m_page_rect.height = m_page_rect.height/scale_y;

        m_render_rect.x = m_render_rect.x/scale_x;
        m_render_rect.y = m_render_rect.y/scale_y;
        m_render_rect.width = m_render_rect.width/scale_x;
        m_render_rect.height = m_render_rect.height/scale_y;
        
        m_printer_dc->SetUserScale(scale_x, scale_y);

        // if the page rectangle or the render rectangle are empty,
        // we're done
        if (m_page_rect.IsEmpty() || m_render_rect.IsEmpty())
            return;

        // get the total length of text in the document
        m_text_length = m_editorctrl->GetTextLength();

        // iterate through the text and get the position of the
        // first character on each page; stop when we don't have
        // any more charaters left
        int offset = 1;
        do
        {
            m_offsets.push_back(offset);
            offset = m_editorctrl->FormatRange(false, offset, m_text_length,
                                               m_printer_dc, m_printer_dc,
                                               m_render_rect, m_page_rect);
        }
        while (offset < m_text_length);
    }
    
    bool OnPrintPage(int page_num)
    {
        // get the starting character offset and print the page
        int offset = m_offsets[page_num-1];
        m_editorctrl->FormatRange(true, offset, m_text_length, m_printer_dc, m_printer_dc,
                                  m_render_rect, m_page_rect);

        return true;
    }

    void SetPageSize(double width, double height)
    {
        m_page_width = width;
        m_page_height = height;
    }
    
    void SetPageMargins(double left, double right, double top, double bottom)
    {
        m_margin_left = left;
        m_margin_right = right;
        m_margin_top = top;
        m_margin_bottom = bottom;
    }

private:

    std::vector<int> m_offsets;       // starting character position on a page
    EditorCtrl* m_editorctrl;         // editor control
    
    wxDC* m_layout_dc;                // dc used to scale document correctly
    wxDC* m_printer_dc;               // dc used to print document
    
    wxRect m_page_rect;               // the page rectangle in printer pixels
    wxRect m_render_rect;             // the render rectangle in printer pixels
    int m_text_length;                // total number of lines in the document
    
    double m_page_width;              // page width in inches
    double m_page_height;             // page height in inches
    double m_margin_left;             // left margin in inches
    double m_margin_right;            // right margin in inches
    double m_margin_top;              // top margin in inches
    double m_margin_bottom;           // bottom margin in inches
};



// EditorDoc implementation

enum
{
    ID_Text = wxID_HIGHEST + 1,
    ID_RunScript
};

BEGIN_EVENT_TABLE(EditorDoc, wxWindow)
    EVT_MENU(ID_File_Save, EditorDoc::onSave)
    EVT_MENU(ID_File_SaveAs, EditorDoc::onSaveAs)
    EVT_MENU(ID_File_SaveAsExternal, EditorDoc::onSaveAsExternal)
    EVT_MENU(ID_File_Print, EditorDoc::onPrint)
    EVT_MENU(ID_File_PageSetup, EditorDoc::onPageSetup)
    EVT_MENU(ID_Edit_Undo, EditorDoc::onUndo)
    EVT_MENU(ID_Edit_Redo, EditorDoc::onRedo)
    EVT_MENU(ID_Edit_Cut, EditorDoc::onCut)
    EVT_MENU(ID_Edit_Copy, EditorDoc::onCopy)
    EVT_MENU(ID_Edit_Paste, EditorDoc::onPaste)
    EVT_MENU(ID_Edit_SelectAll, EditorDoc::onSelectAll)
    EVT_MENU(ID_Edit_GoTo, EditorDoc::onGoTo)
    EVT_MENU(ID_View_ZoomIn, EditorDoc::onZoomIn)
    EVT_MENU(ID_View_ZoomOut, EditorDoc::onZoomOut)
    EVT_MENU(ID_View_ZoomToActual, EditorDoc::onZoomToActual)
    EVT_MENU(ID_File_Run, EditorDoc::onExecute)
    EVT_SIZE(EditorDoc::onSize)

    // disable data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, EditorDoc::onUpdateUI_DisableAlways)

    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, EditorDoc::onUpdateUI_DisableAlways)

    // disable format items
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, EditorDoc::onUpdateUI_DisableAlways)

    // disable table items
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, EditorDoc::onUpdateUI_DisableAlways)

    // disable the scale-related zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, EditorDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, EditorDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, EditorDoc::onUpdateUI_DisableAlways)  

    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, EditorDoc::onUpdateUI)
END_EVENT_TABLE()



EditorDoc::EditorDoc()
{
    m_temporary = true;
    m_external = false;
    m_text = NULL;
    m_path = wxEmptyString;
    m_error_message = wxEmptyString;
    m_mime_type = wxT("text/plain");
    m_timer = NULL;
    
    #ifdef __WXMSW__
    m_eol_mode = wxSTC_EOL_CRLF;
    #else
    m_eol_mode = wxSTC_EOL_LF;
    #endif
    
    // page dimensions for printing
    m_page_width = 8.5f;
    m_page_height = 11.0f;
    m_page_margin_left = 0.75f;
    m_page_margin_right = 0.75f;
    m_page_margin_top = 0.75f;
    m_page_margin_bottom = 0.75f;
}

EditorDoc::~EditorDoc()
{
    if (m_timer)
    {
        m_timer->Stop();
        delete m_timer;
        m_timer = NULL;
    }
}


bool EditorDoc::initDoc(IFramePtr frame,
                        IDocumentSitePtr doc_site,
                        wxWindow* docsite_wnd,
                        wxWindow* panesite_wnd)
{
    bool result = Create(docsite_wnd,
                         -1,
                         wxPoint(0,0),
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN |
                         wxNO_BORDER);
    if (!result)
    {
        return false;
    }

    m_frame = frame;
    m_doc_site = doc_site;

    SetBackgroundColour(kcl::getBaseColor());
    doc_site->setBitmap(GETBMPSMALL(gf_script));
    
    
    // create editor control
    
    m_text = new EditorCtrl(this,
                            ID_Text,
                            wxDefaultPosition,
                            wxDefaultSize,
                            wxTE_MULTILINE | wxNO_BORDER | wxHSCROLL | wxVSCROLL);
    m_text->SetFont(wxFont(wxNORMAL_FONT->GetPointSize()+2, wxMODERN, wxNORMAL, wxNORMAL));

    
    // when the cursor is moved, the status bar should be updated
    m_text->sigCursorMoved.connect(this, &EditorDoc::updateStatusBar);
    
    // when the document becomes dirty, the title should get an *
    m_text->sigModified.connect(this, &EditorDoc::updateCaption);
    
    // when the document gets focus, check to see if the file has
    // been updated externally
    m_text->sigFocus.connect(this, &EditorDoc::updateContent);

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_text, 1, wxEXPAND);
    SetSizer(main_sizer);
    Layout();

    updateCaption();

    // connect the frame event signal
    m_frame->sigFrameEvent().connect(this, &EditorDoc::onFrameEvent);

    // connect statusbar signals
    g_app->getMainFrame()->getStatusBar()->sigItemLeftDblClick().connect(
                                this, &EditorDoc::onStatusBarItemLeftDblClick);

    // create the statusbar items for this document
    {
        IStatusBarItemPtr item;

        item = addStatusBarItem(wxT("editordoc_line_number"));
        item->setWidth(90);

        item = addStatusBarItem(wxT("editordoc_column_number"));
        item->setWidth(90);
    }

    // make sure the control's preferences match the stored preferences
    refreshControlPreferences();

    return true;
}


// static
bool EditorDoc::newFile(const wxString& path)
{
    return g_app->getDatabase()->createStream(towstr(path), L"text/plain");
}

void EditorDoc::updateCaption()
{
    if (m_temporary)
        m_doc_site->setCaption(_("(Untitled)"));
    else if (m_external)
        m_doc_site->setCaption(m_path.AfterLast(PATH_SEPARATOR_CHAR));
    else
    {
        wxString caption;
        caption.Append(m_path.AfterLast(wxT('/')));
        caption.Append(isModified() ? wxT("*") : wxT(""));
        
        m_doc_site->setCaption(caption);
    }
}

void EditorDoc::updateContent()
{
    // if a timer is already running, don't run another one
    if (m_timer)
    {
        return;
    }
    
    // check to see if the content needs to be updated; this gets called on a
    // focus event in the editor control and creates a timed object to check
    // for updates; the reason a timed object is created is so that the
    // document has a chance to show before the notification appears, which
    // won't happen if the update function is tied directly to the focus event
    EditorDocExternalUpdateTimer* timer = new EditorDocExternalUpdateTimer(this);
    timer->Start(100, true);    // single shot

    m_timer = timer;
}

void EditorDoc::gotoLine()
{
    int line_count = m_text->GetLineCount();
    wxString default_val = wxString::Format(wxT("%d"), m_text->GetCurrentLine()+1);
    
    wxString message = wxString::Format(_("Line number (1 - %d):"), line_count);
    wxTextEntryDialog dlg(this, message, _("Go To Line"), default_val);
    dlg.SetSize(260,143);
    
    if (dlg.ShowModal() == wxID_OK)
    {
        int line = wxAtoi(dlg.GetValue());
        if (line < 1)
            line = 1;
        else if (line > line_count)
            line = line_count;
        
        line -= 1; // lines are 0-based;
        m_text->GotoLine(line);
        updateStatusBar();
    }
}

wxString EditorDoc::getDocumentLocation()
{
    return m_path;
}

wxString EditorDoc::getDocumentTitle()
{
    return wxEmptyString;
}

wxWindow* EditorDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void EditorDoc::setDocumentFocus()
{
    m_text->SetFocus();
}

bool EditorDoc::findNextMatch(const wxString& value,
                              bool forward,
                              bool match_case,
                              bool whole)
{
    bool res = m_text->FindNextMatch(value, forward, match_case, whole);
    m_text->updateLineNumberMarginWidth();
    updateStatusBar();
    return res;
}

bool EditorDoc::findReplaceWith(const wxString& find_val,
                                const wxString& replace_val,
                                bool forward,
                                bool match_case,
                                bool whole)
{
    if (!findIsReplaceAllowed())
        return false;
    
    // no text is selected, we need to find a match first
    wxString selected_text = m_text->GetSelectedText();
    if (selected_text.IsEmpty())
        return findNextMatch(find_val, forward, match_case, whole);
    
    // the selected text does not match the find text (case sensitive),
    // we need to find a match first
    if (match_case && find_val.Cmp(selected_text) != 0)
        return findNextMatch(find_val, forward, match_case, whole);
    
    // the selected text is not a whole word, we need to find a match first
    int start, end;
    m_text->GetSelection(&start, &end);
    if (start > 0)
        start--;
    
    wxChar before_char = m_text->GetCharAt(start);
    wxChar after_char = m_text->GetCharAt(end);
    if (whole && (!iswspace(before_char) || !iswspace(after_char)))
        return findNextMatch(find_val, forward, match_case, whole);
    
    // the selected text does not match the find text (not case sensitive),
    // we need to find a match first
    if (find_val.CmpNoCase(selected_text) != 0)
        return findNextMatch(find_val, forward, match_case, whole);
    
    m_text->ReplaceSelection(replace_val);
    return findNextMatch(find_val, forward, match_case, whole);
}

bool EditorDoc::findReplaceAll(const wxString& find_val,
                               const wxString& replace_val,
                               bool match_case,
                               bool whole)
{
    if (!findIsReplaceAllowed())
        return false;
    
    std::wstring wfind_val = towstr(find_val);
    std::wstring wregex_match_str;
    
    // bracket non-alphanumeric characters
    const wchar_t* p = (const wchar_t*)wfind_val.c_str();
    while (*p)
    {
        if (!iswalnum(*p))
        {
            wregex_match_str += L'[';
            if (*p == L'^' || *p == '[' || *p == ']')
                wregex_match_str += L'\\';
            wregex_match_str += *p;
            wregex_match_str += L']';
        }
        else
        {
            wregex_match_str += *p;
        }

        p++;
    }
    
    // case insensitive
    if (!match_case)
        wregex_match_str.insert(0, L"(?i)");
    
    // whole word only
    if (whole)
    {
        wregex_match_str.insert(0, L"\\b");
        wregex_match_str.append(L"\\b");
    }
    
    // create the regex object
    klregex::wregex regex(wregex_match_str);
    
    // get the text from the control
    std::wstring wtext = towstr(m_text->GetText());
    
    // if we couldn't find the search string, we're done
    bool found = regex.search(wtext);
    if (!found)
        return false;
    
    m_text->Freeze();
    int line_offset = m_text->GetFirstVisibleLine();
    
    // do the regular expression replacement
    std::wstring wreplace_val = towstr(replace_val);
    regex.replace(wtext, wreplace_val);
    
    // set the text in the control
    m_text->SetText(wtext);
    
    // increase/decrease line number margin size
    m_text->updateLineNumberMarginWidth();
    
    // scroll past the current viewport and then scroll back to
    // the line offset we started with so the view doesn't jump at all
    m_text->ScrollToLine(m_text->GetLineCount());
    m_text->ScrollToLine(line_offset);
    m_text->Thaw();

    updateStatusBar();
    return true;
}

bool EditorDoc::findIsReplaceAllowed()
{
    return true;
}

void EditorDoc::onSize(wxSizeEvent& evt)
{
    Layout();
}

void EditorDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void EditorDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

void EditorDoc::onUpdateUI(wxUpdateUIEvent& evt)
{
    switch (evt.GetId())
    {
        case ID_Edit_Undo:
            evt.Enable(m_text->CanUndo() ? true : false);
            break;

        case ID_Edit_Redo:
            evt.Enable(m_text->CanRedo() ? true : false);
            break;

        case ID_Edit_Cut:
            evt.Enable(m_text->CanCut() ? true : false);
            break;

        case ID_Edit_Copy:
            evt.Enable(m_text->CanCopy() ? true : false);
            break;

        case ID_Edit_Paste:
            evt.Enable(m_text->CanPaste() ? true : false);
            break;
        
        case ID_Edit_Find:
        case ID_Edit_Replace:
            evt.Enable(true);
            break;
        
        case ID_Edit_FindPrev:
        case ID_Edit_FindNext:
        {
            wxString find_filter_text = g_app->getAppController()->
                                               getStandardToolbar()->
                                               getFindCombo()->
                                               GetValue();
            evt.Enable((find_filter_text.Length() > 0) ? true : false);
            break;
        }
        
        default:
            evt.Enable(true);
            break;
    }
}

static void clearStatusBarTextItem()
{
    // clear out statusbar text
    IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
    IStatusBarItemPtr item = statusbar->getItem(wxT("app_statusbar_text"));
    if (item.isNull())
        return;

    // doing the following will ensure that the item's dirty flag
    // is set -- if we don't do this, there is a chance that the
    // item will not be refreshed because it is not dirty
    wxString val = item->getValue();
    item->setValue(val + wxT("dirty"));
    item->setValue(wxEmptyString);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}

bool EditorDoc::onSiteClosing(bool force)
{
    if (m_timer)
    {
        m_timer->Stop();
        delete m_timer;
        m_timer = NULL;
    }

    if (force)
    {
        clearStatusBarTextItem();
        return true;
    }
    
    if (m_text->IsModified())
    {
        int result;
        result = appMessageBox(_("Would you like to save the changes made to this document?"),
                                    APPLICATION_NAME,
                                    wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);

        // if we want to save the changes, try to save them and return
        // whatever value the doSave() function returns        
        if (result == wxYES)
        {
            bool retval = doSave();
            if (retval)
                clearStatusBarTextItem();
            return retval;
        }
        
        // if we cancel, do not save and do not close the site            
        if (result == wxCANCEL)
            return false;
    }

    // if we're neither saving nor cancelling, we want to close the
    // site without saving; return true
    clearStatusBarTextItem();
    return true;
}

void EditorDoc::onSiteActivated()
{
    updateStatusBar();
}


void EditorDoc::setCurrentEolMode(const wxString& value)
{
    const wxChar* begin = value.c_str();
    const wxChar* ch = begin+1;

    int mode = -1;
    int m;
    
    while (*ch)
    {
        if (*ch != '\n')
        {
            ch++;
            continue;
        }
        
        if (*(ch-1) == '\r')
            m = wxSTC_EOL_CRLF;
        else
            m = wxSTC_EOL_LF;
        
        if (mode == -1)
        {
            // first char encountered
            mode = m;
        }
        else
        {
            if (mode != m)
            {
                // mixed mode encountered -- set eol mode platform default
                mode = -1;
                break;
            }
        }
        
        ++ch;
    }
    
    if (mode == -1)
    {
        #ifdef __WXMSW__
        m_eol_mode = wxSTC_EOL_CRLF;
        #else
        m_eol_mode = wxSTC_EOL_LF;
        #endif
    }
    else
    {
        m_eol_mode = mode;
    }
    
    m_text->SetEOLMode(m_eol_mode);
}


bool EditorDoc::loadFile(const wxString& _path)
{
    wxString value;
    if (!readFile(_path, value, m_mime_type, m_external))
        return false;

    m_text->SetLexerByFilename(_path);

    m_text->SetValue(value);
    m_text->DiscardEdits();
    m_text->EmptyUndoBuffer();

    // set the file hash (same as getFileHash())
    m_file_hash = kl::md5str(towstr(value));

    // after setting the text, set the flags and update the caption
    m_path = _path;
    m_temporary = false;
    updateCaption();

    // fire this event so that the URL will be updated with the new path
    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));

    setCurrentEolMode(value);
    
    return true;
}

bool EditorDoc::readFile(const wxString _path, 
                         wxString& value, 
                         wxString& mime_type, 
                         bool& external)
{
    std::wstring path = towstr(_path);

    xd::IDatabasePtr db = g_app->getDatabase();
    if (!db)
        return false;

    if (!db->getFileExist(path))
    {
        // file is not in project, try disk filesystem
        xf_file_t f = xf_open(path, xfOpen, xfRead, xfShareReadWrite);
        if (!f)
        {
            return false;
        }
            
        xf_off_t fsize = xf_get_file_size(path);
        unsigned char* buf = new unsigned char[fsize+1];
        if (!buf)
        {
            return false;
        }

        xf_off_t readbytes = xf_read(f, buf, 1, fsize);
        buf[readbytes] = 0;
        
        if (readbytes >= 2 && buf[0] == 0xff && buf[1] == 0xfe)
        {
            // little endian UCS-2
            std::wstring wval;
            kl::ucsle2wstring(wval, buf+2, (readbytes-2)/2);
            value = wval;
        }
        else if (readbytes >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
        {
            // utf-8
            wchar_t* tempbuf = new wchar_t[fsize+1];
            kl::utf8_utf8tow(tempbuf, fsize+1, (char*)buf+3, readbytes);
            value = tempbuf;
            delete[] tempbuf;
        }
        else
        {
            value = (const char*)buf;
        }
        
        delete[] buf;
        xf_close(f);
        
        // set the external flag
        external = true;
    }
    else
    {
        xd::IFileInfoPtr file_info = db->getFileInfo(path);
        if (!file_info)
        {
            return false;
        }

        // note: no nead to handle old node format any longer; node
        // format for scripts was never used in a release per previous
        // comment

        xd::IStreamPtr stream = db->openStream(path);
        if (!stream)
        {
            return false;
        }
            
        wxMemoryBuffer buf;
            
        char* tempbuf = new char[16384];
        unsigned long read = 0;
            
        while (1)
        {
            if (!stream->read(tempbuf, 16384, &read))
                break;
                
            buf.AppendData(tempbuf, read);
                
            if (read != 16384)
                break;
        }
            
        delete[] tempbuf;
            
            
        unsigned char* ptr = (unsigned char*)buf.GetData();
        size_t buf_len = buf.GetDataLen();
        if (buf_len >= 2 && ptr[0] == 0xff && ptr[1] == 0xfe)
        {
            // little endian UCS-2
            std::wstring wval;
            kl::ucsle2wstring(wval, ptr+2, (buf_len-2)/2);
            value = wval;
        }
        else if (buf_len >= 3 && ptr[0] == 0xef && ptr[1] == 0xbb && ptr[2] == 0xbf)
        {
            // utf-8
            wchar_t* tempbuf = new wchar_t[buf_len+1];
            kl::utf8_utf8tow(tempbuf, buf_len+1, (char*)ptr+3, buf_len-3);
            value = tempbuf;
            delete[] tempbuf;
        }
        else
        {
            buf.AppendByte(0);
            value = wxString::From8BitData((char*)buf.GetData());
        }

        // set the mime type and external flag
        mime_type = file_info->getMimeType();
        external = false;
    }

    return true;
}


void EditorDoc::checkForExternalChanges()
{
    // check if the file has been updated externally since
    // the last time it was loaded; if the file has changed
    // as the user if the want to reload the file or not;
    // use an md5 hash to determine if the file has changed

    std::wstring current_file_hash;
    getFileHash(m_path, current_file_hash);

    // if the external file hasn't changes, we're done
    if (m_file_hash == current_file_hash)
        return;
    
    int result;
    result = appMessageBox(_("This file has been changed outside the source editor.\nWould you like to discard changes and reload the file?"),
                                APPLICATION_NAME,
                                wxYES_NO | wxNO | wxICON_QUESTION | wxCENTER);

    // discard the changes and reload the file
    if (result == wxYES)
    {
        loadFile(m_path);
        return;
    }
    
    // update the file hash
    m_file_hash = current_file_hash;
}

bool EditorDoc::getFileHash(const wxString path, std::wstring& hash)
{
    if (!m_external)
    {
        xd::IDatabasePtr db = g_app->getDatabase();
        xcm::class_info* class_info = xcm::get_class_info(db.p);
        wxString class_name = class_info->get_name();
        wxString driver = class_name.BeforeFirst('.');
        if (driver == "xdclient" || driver == "xdpgsql" || driver == "xdsqlite")
        {
            // don't do this operation when running in xdclient or xdpgsql mode
            m_file_hash = L"";
            hash = L"";
            return true;
        }
    }

    wxString value;
    wxString mime_type;
    bool external;

    if (!readFile(path, value, mime_type, external))
        return false;

    hash = kl::md5str(towstr(value));
    return true;
}

bool EditorDoc::saveFile()
{
    // note: previously, if the file wasn't modified, we wouldn't save 
    // it; however, a file can be modified externally while a file is 
    // open in the editor with no local changes; so a file should still
    // be able to be saved to overwrite external changes even if the file 
    // hasn't been changed locally
    // if (!isModified())
    //     return true;

    // get the text
    std::wstring val = towstr(m_text->GetValue());
    bool uses_unicode = false;

    const wchar_t* p = val.c_str();
    while (*p)
    {
        if (*p > 127)
        {
            uses_unicode = true;
            break;
        }    
        ++p;
    }
    
    size_t buf_len = (val.size() * 4) + 3;
    unsigned char* buf = new unsigned char[buf_len];
    
    if (uses_unicode)
    {
        // convert to utf8
        kl::utf8_wtoutf8((char*)buf+3, buf_len-3, val.c_str(), val.length(), &buf_len);
        
        if (buf_len > 0 && (buf+3)[buf_len-1] == 0)
            buf_len--;
            
        // add space for the byte order mark
        buf_len += 3;
        
        // add byte order mark
        buf[0] = 0xef;
        buf[1] = 0xbb;
        buf[2] = 0xbf;
    }
    /* else ( if we want to save in ucs2 le )
    {
        // little endian byte order mark
        buf[0] = 0xff;
        buf[1] = 0xfe;
        kl::wstring2ucsle(buf+2, val, val.length());
        buf_len = (val.length() * 2) + 2;
    }*/
    else
    {
        // just save as 7-bit ascii because we don't use
        // any characters > char code 127
        std::string s = kl::tostring(val);
        buf_len = val.length();
        memcpy(buf, s.c_str(), buf_len);
    }


    if (m_external)
    {
        // file is not in project, try disk filesystem
        xf_file_t f = xf_open(towstr(m_path), xfCreate, xfWrite, xfShareNone);
        if (!f)
        {
            delete[] buf;
            return false;
        }

        xf_write(f, buf, 1, buf_len);
        xf_close(f);
    }
    else
    {
        xd::IDatabasePtr db = g_app->getDatabase();
        if (db.isNull())
        {
            delete[] buf;
            return false;
        }
      
        if (!db->createStream(towstr(m_path), towstr(m_mime_type)))
        {
            delete[] buf;
            return false;
        }

        xd::IStreamPtr stream = db->openStream(towstr(m_path));
        if (stream.isNull())
        {
            delete[] buf;
            return false;
        }

        stream->write(buf, buf_len, NULL);
    }
    
    delete[] buf;
    m_text->DiscardEdits();
    
    // update the file hash
    getFileHash(m_path, m_file_hash);
    
    // note: saveFile() just handles the file save without
    // updating the temporary flag or the caption; this is
    // so temporary files can be saved so they can be run
    // while still preserving the (Untitled) caption; in
    // other instances where saveFile() is called, doSave()
    // is used, which take care of these interface items
    
    return true;
}

bool EditorDoc::doSave()
{
    if (!m_temporary)
    {
        saveFile();
        updateCaption();
        return true;
    }

    if (m_external)
    { 
        wxString filter;
        filter  = _("Script Files");
        filter += wxT(" (*.js)|*.js|");
        filter += _("Text Files");
        filter += wxT(" (*.txt)|*.txt|");
        filter += _("All Files");
        filter += wxT(" (*.*)|*.*|");
        filter.RemoveLast(); // get rid of the last pipe sign

        wxString filename = getFilenameFromPath(m_path, false);
        wxFileDialog dlg(g_app->getMainFrame()->getFrameWindow(),
                         _("Save As"),
                         wxT(""),
                         filename,
                         filter,
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (dlg.ShowModal() == wxID_OK)
        {
            m_path = dlg.GetPath();
            m_external = true;
            m_temporary = false;
            
            saveFile();
            updateCaption();
            g_app->getAppController()->refreshDbDoc();

            // fire this event so that the URL will be updated with the new path
            m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));

            return true;
        }
        
        return false;
    }
    else
    {
        DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSave);
        dlg.setCaption(_("Save As"));

        if (dlg.ShowModal() == wxID_OK)
        {
            // if we're saving the file to a filesystem mount and no
            // extension is specified, then automatically add a 'csv'
            // extension; this is a usability issue since without the
            // extension, the user usually ends up adding this as the 
            // first item of business after saving
            m_path = dlg.getPath();
            m_path = addExtensionIfExternalFsDatabase(towstr(m_path), L".js");

            m_external = false;
            m_temporary = false;
            
            saveFile();
            updateCaption();
            g_app->getAppController()->refreshDbDoc();
            
            // fire this event so that the URL will be updated with the new path
            m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));

            return true;
        }
        else
        {
            return false;
        }
    }
}

bool EditorDoc::isTemporary()
{
    return m_temporary;
}

bool EditorDoc::isModified()
{
    return m_text->IsModified();
}

void EditorDoc::setText(const wxString& text)
{
    m_text->SetValue(text);
}

void EditorDoc::onStatusBarItemLeftDblClick(IStatusBarItemPtr item)
{
    // only pop open the "Go To Line" dialog for the active table
    IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (doc_site.isOk() && doc_site == m_doc_site)
    {
        if (item->getName() == wxT("editordoc_line_number") ||
            item->getName() == wxT("editordoc_column_number"))
        {
            gotoLine();
        }
    }
}

void EditorDoc::onFrameEvent(FrameworkEvent& evt)
{
    // if a file is renamed, update this file with the new file path
    if (evt.name == FRAMEWORK_EVT_TREEPANEL_OFS_FILE_RENAMED)
    {
        if (evt.s_param == m_path)
        {
            m_path = evt.s_param2;
            updateCaption();
            
            IDocumentSitePtr doc_site = m_frame->getActiveChild();
            if (doc_site.isOk() && doc_site == m_doc_site)
            {
                // fire this event so that the URL combobox will be updated
                // with the new path if this is the active child
                m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));
            }
        }
    }
    else if (evt.name == FRAMEWORK_EVT_APPMAIN_FIND_PANEL_QUERY_FIND_VALUE)
    {
        wxString text = m_text->GetSelectedText();
        if (text.Length() > 0 && text.Find(wxT('\n')) == wxNOT_FOUND)
        {
            wxString* val = (wxString*)(evt.l_param);
            *val = text;
        }
    }
    else if (evt.name == FRAMEWORK_EVT_APPMAIN_PREFERENCES_SAVED)
    {
        refreshControlPreferences();
    }
}

void EditorDoc::onSave(wxCommandEvent& evt)
{
    doSave();
}

void EditorDoc::onSaveAs(wxCommandEvent& evt)
{
    bool saved_temp = m_temporary;
    bool saved_ext = m_external;
    m_temporary = true;
    m_external = false;
    
    if (!doSave())
    {
        // we bailed out, restore these values
        m_temporary = saved_temp;
        m_external = saved_ext;
    }
}

void EditorDoc::onSaveAsExternal(wxCommandEvent& evt)
{
    bool saved_temp = m_temporary;
    bool saved_ext = m_external;
    m_temporary = true;
    m_external = true;
    
    if (!doSave())
    {
        // we bailed out, restore these values
        m_temporary = saved_temp;
        m_external = saved_ext;
    }
}

void EditorDoc::onPrint(wxCommandEvent& evt)
{
    // get the paper size and orientation
    double width = m_page_width;
    double height = m_page_height;
    
    wxPaperSize paper_size = wxPAPER_LETTER;
    int orientation = wxPORTRAIT;
    //wxPaperSize paper_size = PageSetupPanel::getPaperSize(&width, &height);
    //int orientation = PageSetupPanel::getPaperOrientation(width, height);

    // create a prinout object and set the page size and margins
    wxClientDC dc(this);
    EditorDocPrintout printout(m_text, &dc);
    printout.SetPageSize(m_page_width, m_page_height);
    printout.SetPageMargins(m_page_margin_left,
                            m_page_margin_right,
                            m_page_margin_top,
                            m_page_margin_bottom);

    // set the print data
    wxPrintData printdata;
    printdata.SetOrientation(orientation);
    printdata.SetQuality(600);
    printdata.SetPaperId(paper_size);
    
    // create a print dialog
    wxPrintDialogData printdialog(printdata);
    printdialog.EnablePageNumbers(false);

    // do the printing
    wxPrinter printer(&printdialog);
    printer.Print(g_app->getMainWindow(), &printout);

    // return focus to the editor doc
    setDocumentFocus();
}

void EditorDoc::onPageSetup(wxCommandEvent& evt)
{
    PageSetupDialog dlg(this);
    dlg.setPageDimensions(m_page_width, m_page_height);
    dlg.setMargins(m_page_margin_left,
                   m_page_margin_right,
                   m_page_margin_top,
                   m_page_margin_bottom);

    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.getPageDimensions(&m_page_width, &m_page_height);
        dlg.getMargins(&m_page_margin_left,
                       &m_page_margin_right,
                       &m_page_margin_top,
                       &m_page_margin_bottom);
    }

    setDocumentFocus();
}

void EditorDoc::onExecute(wxCommandEvent& evt)
{
    // if the file is a temporary file, create a temporary path 
    // for a file in the project
    if (isTemporary() && m_path.Length() == 0)
    {
        static int counter = 0;
        ++counter;
        m_path = wxT("/.temp/");
        m_path += kl::getUniqueString();
        m_external = false;
    }

    // save the file; use saveFile() which saves the
    // file without updating the temporary path flag, 
    // preserving the (Untitled) caption
    saveFile();
    

    IDocumentSiteEnumPtr docsites;
    IDocumentSitePtr site;
    IEditorDocPtr editor_doc;

    // make sure all open scripts (except untitled scripts)
    // are saved before we run this script
    docsites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);

    size_t i, site_count = docsites->size();
    for (i = 0; i < site_count; ++i)
    {
        site = docsites->getItem(i);
        editor_doc = site->getDocument();
        if (editor_doc.isOk() && !editor_doc->isTemporary())
            editor_doc->doSave();
    }

    AppScriptError error;

    m_text->RemoveMarkers();
    m_error_message = wxEmptyString;
    
    if (g_app->getAppController()->executeScript(m_path, NULL, &error).isNull())
    {
        IEditorDocPtr doc_ptr = static_cast<IEditorDoc*>(this);
        
        if (!error.file.IsEmpty())
        {
            int doc_id;
            if (!g_app->getAppController()->openScript(error.file, &doc_id))
                return;
            
            IDocumentSitePtr site = g_app->getMainFrame()->lookupSiteById(doc_id);
            if (site.isNull())
                return;
                
            doc_ptr = site->getDocument();
            if (doc_ptr.isNull())
                return;
        }

        doc_ptr->reportError(error.offset, error.line, error.message);
        return;
    }
    
    updateStatusBar();
}

void EditorDoc::onUndo(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }
    
    if (!m_text->CanUndo())
        return;

    m_text->Undo();
    m_text->updateLineNumberMarginWidth();
    updateStatusBar();
}

void EditorDoc::onRedo(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }
    
    if (!m_text->CanRedo())
        return;

    m_text->Redo();
    m_text->updateLineNumberMarginWidth();
    updateStatusBar();
}

void EditorDoc::onCut(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }

    m_text->Cut();
    m_text->updateLineNumberMarginWidth();
    updateStatusBar();
}

void EditorDoc::onCopy(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }

    m_text->Copy();
}

void EditorDoc::onPaste(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }

    m_text->Paste();
    m_text->updateLineNumberMarginWidth();
    updateStatusBar();
}

void EditorDoc::onSelectAll(wxCommandEvent& evt)
{
    m_text->SelectAll();
    updateStatusBar();
}

void EditorDoc::onGoTo(wxCommandEvent& evt)
{
    gotoLine();
}

void EditorDoc::onZoomIn(wxCommandEvent& evt)
{
    m_text->ZoomIn();
}

void EditorDoc::onZoomOut(wxCommandEvent& evt)
{
    m_text->ZoomOut();
}

void EditorDoc::onZoomToActual(wxCommandEvent& evt)
{
    m_text->SetZoom(0);
}

void EditorDoc::goLine(int line)
{
    m_text->GotoLine(line-1);
}

void EditorDoc::reportError(size_t offset,
                            int line,
                            const wxString& message)
{
    m_text->SetCursorPosition(offset);
    m_text->SetMarker(line);
    
    m_error_message = wxString::Format(_("Compiler error (line %d): %s"),
                                       line,
                                       message.c_str());
    
    wxString s = m_error_message;
    s += wxT("\n");
    
    g_app->getAppController()->showConsolePanel();
    g_app->getAppController()->printConsoleText(s);
                                     
    //updateStatusBar();
}

void EditorDoc::refreshControlPreferences()
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isNull())
        return;
    
    bool show_line_numbers;
    bool show_syntax_highlighting;
    bool show_whitespace;
    bool insert_spaces;
    int  insert_size;
    
    // we really should get these default values from the OptionsPanel
    // (or at least from a common place in the code)
    show_line_numbers = getAppPrefsBoolean(wxT("script.show_line_numbers"));
    show_syntax_highlighting = getAppPrefsBoolean(wxT("script.show_syntax_highlighting"));
    show_whitespace = getAppPrefsBoolean(wxT("script.show_whitespace"));
    insert_spaces = getAppPrefsBoolean(wxT("script.insert_spaces"));
    if (insert_spaces)
        insert_size = getAppPrefsLong(wxT("script.insert_spaces_count"));
    else
        insert_size = getAppPrefsLong(wxT("script.tab_size"));
    
    m_text->showLineNumbers(show_line_numbers);
    m_text->showSyntaxHighlighting(show_syntax_highlighting);
    m_text->showWhitespace(show_whitespace);
    m_text->setTabs(!insert_spaces, insert_size);
}

void EditorDoc::updateStatusBar()
{
    // for the text editor, updateStatusBar() is called in onUpdateUI() since
    // the status lists the cursor position, which can be dependent on a number
    // of events; to reduce flicker in the status bar, we only update it if the
    // values have changed

    if (!m_text)
        return;

    bool update = false;
    wxString old_value;

    int line = m_text->GetCurrentLine() + 1;
    int column = m_text->GetColumn(m_text->GetAnchor()) + 1;

    wxString line_number = wxString::Format(_("Line: %d"), line);
    wxString column_number = wxString::Format(_("Column: %d"), column);

    IStatusBarItemPtr item;
    IStatusBarItemEnumPtr items = getStatusBarItemEnum();
    
    size_t i, count = items->size();
    for (i = 0; i < count; ++i)
    {
        item = items->getItem(i);
        old_value = item->getValue();

        // update the line number
        if (item->getName().CmpNoCase(wxT("editordoc_line_number")) == 0)
        {
            if (line_number != old_value)
            {
                item->setValue(line_number);
                update = true;
            }
        }
        
        // update the column number
        if (item->getName().CmpNoCase(wxT("editordoc_column_number")) == 0)
        {
            if (column_number != old_value)
            {
                item->setValue(column_number);
                update = true;
            }
        }
    }

    // post an event to update the statusbar
    if (update)
    {
        // refresh the statusbar
        g_app->getMainFrame()->getStatusBar()->refresh();
    }
}
