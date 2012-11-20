/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-07-18
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "exprbuilder.h"
#include "fieldlistcontrol.h"


// NOTE: the default colors for the editor doc, expression builder,
// and console are the same; TODO: probably should factor these out
#define NORMAL_CHAR_COLOR     wxColour(0x00, 0x00, 0x00)
#define UNICODE_CHAR_COLOR    wxColour(0x00, 0x00, 0x00)
#define KEYWORD_COLOR         wxColour(0x00, 0x00, 0xff)
#define OPERATOR_COLOR        wxColour(0x00, 0x00, 0x00)
#define SINGLE_QUOTED_COLOR   wxColour(0xa0, 0x00, 0x00)
#define DOUBLE_QUOTED_COLOR   wxColour(0xa0, 0x00, 0x00)
#define NUMBER_COLOR          wxColour(0x00, 0x00, 0x00)
#define COMMENT_COLOR         wxColour(0x00, 0x80, 0x00)
#define PREPROCESSOR_COLOR    wxColour(0x80, 0x80, 0x80)


struct FuncInfo
{
    wxString name;
    wxString syntax;
    wxString helptext;
    wxString helpurl;
    
    FuncInfo(const wxString& _name,
             const wxString& _syntax,
             const wxString& _helptext)
    {
        name = _name;
        syntax = _syntax;
        helptext = _helptext;
        helpurl = wxString::Format(APP_WEBLOCATION_FUNCTIONHELP, name.c_str());
    }

    FuncInfo(const FuncInfo& c)
    {
        name = c.name;
        syntax = c.syntax;
        helptext = c.helptext;
        helpurl = c.helpurl;
    }
};


std::vector<FuncInfo> func_info_list;


static void populateFuncInfoList()
{
    func_info_list.push_back(FuncInfo(wxT("abs"),       _("ABS(number)"),                                    _("Returns the absolute value of a number")));
    func_info_list.push_back(FuncInfo(wxT("acos"),      _("ACOS(number)"),                                   _("Returns the arc cosine of a number in units of radians")));
    func_info_list.push_back(FuncInfo(wxT("asin"),      _("ASIN(number)"),                                   _("Returns the arc sin of a number in units of radians")));
    func_info_list.push_back(FuncInfo(wxT("atan"),      _("ATAN(number)"),                                   _("Returns the arc tangent of a number in units of radians")));
    func_info_list.push_back(FuncInfo(wxT("avg"),       _("AVG(number1 [, number2, ...])"),                  _("Returns the average number of an input list")));
    func_info_list.push_back(FuncInfo(wxT("ceiling"),   _("CEILING(number [, precision])"),                  _("Returns the smallest number, to the specified precision, that is greater than or equal to the input number")));
    func_info_list.push_back(FuncInfo(wxT("chr"),       _("CHR(number)"),                                    _("Returns the ASCII character corresponding to the input number")));
    func_info_list.push_back(FuncInfo(wxT("contains"),  _("CONTAINS(string, search string)"),                _("Returns true if the search string is contained in the string, false otherwise")));
    func_info_list.push_back(FuncInfo(wxT("cos"),       _("COS(number)"),                                    _("Returns the cosine of a number, where the input number is in units of radians")));
    func_info_list.push_back(FuncInfo(wxT("curdate"),   _("CURDATE()"),                                      _("Returns the current system date and time")));
    func_info_list.push_back(FuncInfo(wxT("date"),      _("DATE(string)"),                                   _("Returns a date from the specified input string")));
    func_info_list.push_back(FuncInfo(wxT("day"),       _("DAY(date)"),                                      _("Returns the day corresponding to a date")));
    func_info_list.push_back(FuncInfo(wxT("dayname"),   _("DAYNAME(date)"),                                  _("Returns the day of the week corresponding to a date")));
    func_info_list.push_back(FuncInfo(wxT("dayofweek"), _("DAYOFWEEK(date)"),                                _("Returns the day of the week corresponding to a date as a number between 1 and 7, where Sunday is the first day of the week")));
    func_info_list.push_back(FuncInfo(wxT("empty"),     _("EMPTY(value)"),                                   _("Returns true if an input string or date is empty, false otherwise")));
    func_info_list.push_back(FuncInfo(wxT("exp"),       _("EXP(number)"),                                    _("Returns the constant e raised to the power of a number")));
    func_info_list.push_back(FuncInfo(wxT("floor"),     _("FLOOR(number [, precision])"),                    _("Returns the largest number, to the specified precision, that is less than or equal to the input number")));
    func_info_list.push_back(FuncInfo(wxT("hour"),      _("HOUR(date)"),                                     _("Returns the hour corresponding to a date")));
    func_info_list.push_back(FuncInfo(wxT("iif"),       _("IIF(boolean test, true value, false value)"),     _("If the boolean test is true, the function returns the true value, otherwise it returns the false value")));
    func_info_list.push_back(FuncInfo(wxT("instr"),     _("INSTR(string, search string [, occurrence])"),    _("Returns the nth occurrence of the search string in the string, starting from the left side of the string")));
    func_info_list.push_back(FuncInfo(wxT("isnull"),    _("ISNULL(value)"),                                  _("Returns true if an input string or date is null, false otherwise")));
    func_info_list.push_back(FuncInfo(wxT("left"),      _("LEFT(string, length)"),                           _("Returns the leftmost characters of a string to the specified length")));
    func_info_list.push_back(FuncInfo(wxT("length"),    _("LENGTH(string)"),                                 _("Returns the number of characters in a string")));
    func_info_list.push_back(FuncInfo(wxT("lfind"),     _("LFIND(string, search string [, occurrence])"),    _("Returns the nth occurrence of the search string in the string, starting from the left side of the string")));
    func_info_list.push_back(FuncInfo(wxT("ln"),        _("LN(number)"),                                     _("Returns the natural logarithm of a number")));
    func_info_list.push_back(FuncInfo(wxT("log"),       _("LOG(number [, base])"),                           _("Returns the logarithm of a number in the specified base")));
    func_info_list.push_back(FuncInfo(wxT("lower"),     _("LOWER(string)"),                                  _("Converts all characters in a string to lowercase")));
    func_info_list.push_back(FuncInfo(wxT("lpad"),      _("LPAD(string, length [, list])"),                  _("Adds the characters in the list to the left side of a string until the string is the specified length")));
    func_info_list.push_back(FuncInfo(wxT("ltrim"),     _("LTRIM(string [, list])"),                         _("Removes each character in the list from the left side of a string")));
    func_info_list.push_back(FuncInfo(wxT("max"),       _("MAX(value1 [, value2, ...])"),                    _("Returns the maximum value from an input list")));
    func_info_list.push_back(FuncInfo(wxT("min"),       _("MIN(value1 [, value2, ...])"),                    _("Returns the minimum value from an input list")));
    func_info_list.push_back(FuncInfo(wxT("minute"),    _("MINUTE(date)"),                                   _("Returns the minute corresponding to a date")));
    func_info_list.push_back(FuncInfo(wxT("mod"),       _("MOD(numerator, denominator)"),                    _("Returns the remainder of the numerator divided by the denominator")));
    func_info_list.push_back(FuncInfo(wxT("month"),     _("MONTH(date)"),                                    _("Returns the month corresponding to a date")));
    func_info_list.push_back(FuncInfo(wxT("monthname"), _("MONTHNAME(date)"),                                _("Returns the name of the month containing a date")));
    func_info_list.push_back(FuncInfo(wxT("pi"),        _("PI()"),                                           _("Returns an approximation of the constant PI")));
    func_info_list.push_back(FuncInfo(wxT("power"),     _("POWER(base, exponent)"),                          _("Returns the base raised to the power of the exponent")));
    func_info_list.push_back(FuncInfo(wxT("proper"),    _("PROPER(string)"),                                 _("Converts any character in a string that follows a non-letter character to uppercase")));
    func_info_list.push_back(FuncInfo(wxT("rand"),      _("RAND()"),                                         _("Returns a random number between zero and one")));
    func_info_list.push_back(FuncInfo(wxT("recid"),     _("RECID()"),                                        _("Returns a string that uniquely identifies a record in a project")));
    func_info_list.push_back(FuncInfo(wxT("recno"),     _("RECNO()"),                                        _("Returns a number indicating the unsorted, absolute position of a record in a set")));
    func_info_list.push_back(FuncInfo(wxT("regexp"),    _("REGEXP(string, match string)"),                   _("Returns a portion of the string that matches the regular expression given by the match string")));
    func_info_list.push_back(FuncInfo(wxT("repeat"),    _("REPEAT(string, length)"),                         _("Repeats the characters in the string until the specified length is reached")));
    func_info_list.push_back(FuncInfo(wxT("replace"),   _("REPLACE(string, search string, replace string)"), _("Replaces each occurrence of the search string in the input string with the corresponding replace string")));
    func_info_list.push_back(FuncInfo(wxT("reverse"),   _("REVERSE(string)"),                                _("Reverses the order of characters in a string")));
    func_info_list.push_back(FuncInfo(wxT("rfind"),     _("RFIND(string, search string [, occurrence])"),    _("Returns the nth occurrence of the search string in the string, starting from the right side of the string")));
    func_info_list.push_back(FuncInfo(wxT("right"),     _("RIGHT(string, length)"),                          _("Returns the rightmost characters of a string to the specified length")));
    func_info_list.push_back(FuncInfo(wxT("round"),     _("ROUND(number [, precision])"),                    _("Returns a number that is rounded to the specified precision")));
    func_info_list.push_back(FuncInfo(wxT("rpad"),      _("RPAD(string, length [, list])"),                  _("Adds the characters in the list to the right side of a string until the string is the specified length")));
    func_info_list.push_back(FuncInfo(wxT("rtrim"),     _("RTRIM(string [, list])"),                         _("Removes each character in the list from the right side of a string")));
    func_info_list.push_back(FuncInfo(wxT("second"),    _("SECOND(date)"),                                   _("Returns the second corresponding to a date")));
    func_info_list.push_back(FuncInfo(wxT("sign"),      _("SIGN(number)"),                                   _("Returns the sign of a number")));
    func_info_list.push_back(FuncInfo(wxT("sin"),       _("SIN(number)"),                                    _("Returns the sin of a number, where the input number is in units of radians")));
    func_info_list.push_back(FuncInfo(wxT("soundex"),   _("SOUNDEX(string)"),                                _("Returns a phonetic representation of a string")));
    func_info_list.push_back(FuncInfo(wxT("stddev"),    _("STDDEV(number1 [, number2, ...])"),               _("Returns the standard deviation of an input list")));
    func_info_list.push_back(FuncInfo(wxT("str"),       _("STR(number [, precision])"),                      _("Rounds a number to a specified precision and returns it as a string")));
    func_info_list.push_back(FuncInfo(wxT("strpart"),   _("STRPART(string, section [, delimiter])"),         _("Returns the nth section of a string that is divided into sections by the delimiter string")));
    func_info_list.push_back(FuncInfo(wxT("substr"),    _("SUBSTR(string, start [, length])"),               _("Returns a portion of an input string, from the specified start position in the string to the specified length")));
    func_info_list.push_back(FuncInfo(wxT("sum"),       _("SUM(number1 [, number2, ...])"),                  _("Returns the sum of an input list")));
    func_info_list.push_back(FuncInfo(wxT("tan"),       _("TAN(number)"),                                    _("Returns the tangent of a number, where the input number is in units of radians")));
    func_info_list.push_back(FuncInfo(wxT("translate"), _("TRANSLATE(string, search, replace)"),             _("Replaces each character in a string that matches a character in the search list with a character in the replace list")));
    func_info_list.push_back(FuncInfo(wxT("trim"),      _("TRIM(string [, list])"),                          _("Removes each character in the list from the left and right sides of a string")));
    func_info_list.push_back(FuncInfo(wxT("trunc"),     _("TRUNC(number [, precision])"),                    _("Returns a number that is truncated the specified precision")));
    func_info_list.push_back(FuncInfo(wxT("upper"),     _("UPPER(string)"),                                  _("Converts all characters in a string to uppercase")));
    func_info_list.push_back(FuncInfo(wxT("val"),       _("VAL(string)"),                                    _("Returns a number from a string by interpreting it according to a format string")));
    func_info_list.push_back(FuncInfo(wxT("variance"),  _("VARIANCE(number1 [, number2, ...])"),             _("Returns the variance of an input list")));    
    func_info_list.push_back(FuncInfo(wxT("week"),      _("WEEK(date)"),                                     _("Returns the week of the year containing the given date as a number between 1 and 53")));
    func_info_list.push_back(FuncInfo(wxT("year"),      _("YEAR(date)"),                                     _("Returns the year corresponding to a date")));
};





enum
{
    ID_ExprBuilderFunctionList = 10000,
    ID_ExprBuilderFieldListCtrl,
    ID_KeyBuilderFieldListCtrl,
    ID_LearnMoreHyperlinkCtrl
};




/*
BEGIN_EVENT_TABLE(ExprEditorControl, wxStyledTextCtrl)
    EVT_STC_CHARADDED(18543, ExprEditorControl::onCharAdded)
    EVT_STC_CHANGE(18543, ExprEditorControl::onChanged)
END_EVENT_TABLE()
*/


ExprEditorControl::ExprEditorControl(wxWindow *parent,
                                     wxWindowID id,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style)
                    : wxStyledTextCtrl(parent,
                                       id,
                                       pos,
                                       size == wxDefaultSize ? wxSize(80,205) : size,
                                       style)
{
    wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
    StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    StyleClearAll();

    // set selection foreground and background color
    SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));

    StyleSetForeground(wxSTC_C_DEFAULT,  NORMAL_CHAR_COLOR);
    StyleSetForeground(wxSTC_C_COMMENT,  COMMENT_COLOR);
    StyleSetForeground(wxSTC_C_COMMENTLINE,  COMMENT_COLOR);
    StyleSetForeground(wxSTC_C_COMMENTDOC,  COMMENT_COLOR);
    StyleSetForeground(wxSTC_C_NUMBER,  NUMBER_COLOR);
    StyleSetForeground(wxSTC_C_WORD,  KEYWORD_COLOR);
    StyleSetForeground(wxSTC_C_STRING,  DOUBLE_QUOTED_COLOR);
    StyleSetForeground(wxSTC_C_CHARACTER,  SINGLE_QUOTED_COLOR);
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
    
    StyleSetForeground(wxSTC_STYLE_BRACELIGHT, NORMAL_CHAR_COLOR);
    StyleSetBold(wxSTC_STYLE_BRACELIGHT, true);
   
    MarkerDefine(0, wxSTC_MARK_ARROW);

    EmptyUndoBuffer();

    SetLexer(wxSTC_LEX_CPPNOCASE);

    wxString keywords;
    if (func_info_list.size() == 0)
        populateFuncInfoList();
    int funcinfo_count = func_info_list.size();

    for (int i = 0; i < funcinfo_count; ++i)
    {
        keywords += func_info_list[i].name;
        keywords += wxT(" ");
    }
    keywords += wxT("count sum avg min max first last stddev variance ");
    keywords += wxT("and or not like");

    SetKeyWords(0, keywords);

    SetMargins(2,2);
    
    SetUseTabs(false);
    SetUseHorizontalScrollBar(false);
    SetMarginWidth(1,0);
    SetIndent(4);
    CallTipSetBackground(wxColour(255,255,225));
    CallTipSetForeground(*wxBLACK);
    SetWrapMode(wxSTC_WRAP_WORD);

    Connect(id, wxEVT_KEY_DOWN, (wxObjectEventFunction)(wxEventFunction)(wxCharEventFunction)&ExprEditorControl::onKeyDown);
    Connect(id, wxEVT_STC_CHARADDED, (wxObjectEventFunction)(wxEventFunction)(wxStyledTextEventFunction)&ExprEditorControl::onCharAdded);
    Connect(id, wxEVT_STC_CHANGE, (wxObjectEventFunction)(wxEventFunction)(wxStyledTextEventFunction)&ExprEditorControl::onChanged);
}

void ExprEditorControl::SetValue(const wxString& value)
{
    SetText(value);
}

wxString ExprEditorControl::GetValue()
{
    return GetText();
}

void ExprEditorControl::setStructure(tango::IStructurePtr structure)
{

}


int ExprEditorControl::FindBrace(int pos)
{
    wxChar ch;
    wxChar brace_ch = GetCharAt(pos);
    wxChar to_find;
    int direction;
    int text_len = GetTextLength();
    
    switch (brace_ch)
    {
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

void ExprEditorControl::onKeyDown(wxKeyEvent& evt)
{
    wxChar key = evt.GetKeyCode();
    
    if (key == wxT('\t'))
    {
        // this code is specially tailord to work with
        // the expression editor dialog below.  If you want
        // to use ExprEditorControl elsewhere, and the code
        // below is interfering, just remove it.  The code
        // is designed to perform manual tab traversal in
        // the dialog, while still allowing the editor to
        // receive arrow key events;
        
        #ifdef __WXMSW__
        wxWindow* ok = GetParent()->FindItem(wxID_OK);
        wxWindow* cancel = GetParent()->FindItem(wxID_CANCEL);
        
        if (ok->IsEnabled())
            ok->SetFocus();
             else
            cancel->SetFocus();
        #endif
            
        return;
    }
    
    evt.Skip();
}

void ExprEditorControl::onCharAdded(wxStyledTextEvent& evt)
{
    evt.Skip();
    
    wxChar key = evt.GetKey();

    if (key == wxT('('))
    {
        int cur_pos = GetCurrentPos();

        wxString calltip_text;
        wxString func_name;
        wxString text = GetText().Left(cur_pos-1);
        int len = text.Length();
        if (len == 0)
            return;

        const wxChar* begin = text.c_str();
        const wxChar* ch = begin + len - 1;

        while (ch >= begin)
        {
            if (!::wxIsalpha(*ch) && *ch != wxT('_'))
                break;

            func_name.Prepend(*ch);
            ch--;
        }

        if (func_name.Length() == 0)
            return;
        
        func_name.MakeUpper();

        int funcinfo_count = func_info_list.size();
        for (int i = 0; i < funcinfo_count; ++i)
        {
            wxString s = func_info_list[i].name;
            s.MakeUpper();
            
            if (s == func_name)
            {
                calltip_text = func_info_list[i].syntax;
                calltip_text.Trim();
                break;
            }
        }

        if (calltip_text.Length() == 0)
            return;

        CallTipShow(cur_pos, calltip_text);
    }
     else if (key == wxT(')'))
    {
        int cur_pos = GetCurrentPos();
        int brace_pos = FindBrace(cur_pos-1);

        if (brace_pos == -1)
        {
            //BraceBadLight(cur_pos);
            return;
        }

        if (CallTipActive())
        {
            if (brace_pos == CallTipPosAtStart()-1)
            {
                CallTipCancel();
            }
        }

        BraceHighlight(brace_pos, cur_pos);
    }
     else
    {
        BraceHighlight(wxSTC_INVALID_POSITION, wxSTC_INVALID_POSITION);
    }
}

void ExprEditorControl::onChanged(wxStyledTextEvent& evt)
{
    // for some reason we are receiving this event twice;
    // this code will prevent that

    wxString value = GetValue();
    if (value != m_text)
    {
        m_text = value;

        wxCommandEvent e(wxEVT_COMMAND_TEXT_UPDATED, GetId());
        InitCommandEvent(e);
        e.SetString(GetValue());

        GetEventHandler()->ProcessEvent(e);

        // make sure the scrollbar becomes enabled as soon as there are
        // more lines in the control than are visible on screen
        if (GetLineCount() > LinesOnScreen())
            SetScrollbar(wxVERTICAL, GetCurrentLine(), LinesOnScreen(), GetLineCount());
    }
}

void ExprEditorControl::SetInsertionPointEnd()
{
    GotoPos(GetTextLength());
}

int ExprEditorControl::GetInsertionPoint()
{
    return GetCurrentPos();
}

void ExprEditorControl::SetInsertionPoint(int pos)
{
    GotoPos(pos);
}







// NOTE TO SELF:  20 Jul 2002...wee hours of the morning....I'm sitting in
// Stansted airport in greater London, it's 2:58 am in the morning and I'm
// waiting for a flight back to Germany and then back home two days from now.
// Fortunately I have enough battery to program all night.  However, for future
// reference, don't do this again...it stinks.  Four more hours to go....
//
// 4:22 am: another discovery...Red Bull helps greatly in this circumstance.



enum
{
    ID_OperPlus = 10000,
    ID_OperMinus,
    ID_OperMult,
    ID_OperDiv,
    ID_OperLParen,
    ID_OperRParen,
    ID_OperEq,
    ID_OperNeq,
    ID_OperLt,
    ID_OperGt,
    ID_OperLte,
    ID_OperGte,
    ID_OperAnd,
    ID_OperOr,
    ID_OperNot,
    ID_Expr,
    ID_Advanced,
    ID_AddKey,
    ID_RemoveKey
};


BEGIN_EVENT_TABLE(ExprBuilderPanel, wxPanel)
    EVT_TEXT(ID_Expr, ExprBuilderPanel::onExprChanged)
    EVT_COMMAND_RANGE(ID_OperPlus, ID_OperNot, wxEVT_COMMAND_BUTTON_CLICKED, ExprBuilderPanel::onOperButtonClicked)
    EVT_BUTTON(wxID_OK, ExprBuilderPanel::onOK)
    EVT_BUTTON(wxID_CANCEL, ExprBuilderPanel::onCancel)
    EVT_KCLGRID_CELL_LEFT_DCLICK(ExprBuilderPanel::onFunctionActivated)
    EVT_KCLGRID_CURSOR_MOVE(ExprBuilderPanel::onFunctionListCursorMove)
    EVT_KCLGRID_NEED_TOOLTIP_TEXT(ExprBuilderPanel::onFunctionListNeedTooltipText)
    EVT_HYPERLINK(ID_LearnMoreHyperlinkCtrl, ExprBuilderPanel::onLearnMoreHyperlinkClicked)
    EVT_SIZE(ExprBuilderPanel::onSize)
END_EVENT_TABLE()


ExprBuilderPanel::ExprBuilderPanel()
{
    m_mask_signals = false;
    m_type_only = tango::typeUndefined;
    m_empty_ok = false;
    m_expr_type = tango::typeInvalid;
    m_validation = true;

    m_ok_button = NULL;
    m_cancel_button = NULL;
    m_ok_text = _("OK");
    m_cancel_text = _("Cancel");
}


ExprBuilderPanel::ExprBuilderPanel(wxWindow* parent) :
                           wxPanel(parent,
                                   -1,
                                   wxDefaultPosition,
                                   wxDefaultSize,
                                   wxNO_FULL_REPAINT_ON_RESIZE |
                                   wxCLIP_CHILDREN |
                                   wxTAB_TRAVERSAL)
{
    m_mask_signals = false;
    m_type_only = tango::typeUndefined;
    m_empty_ok = false;
    m_expr_type = tango::typeInvalid;
    m_validation = true;

    m_ok_button = NULL;
    m_cancel_button = NULL;
    m_ok_text = _("OK");
    m_cancel_text = _("Cancel");
    
    construct();
}

bool ExprBuilderPanel::create(wxWindow* parent)
{
    if (!Create(parent,
                -1,
                wxDefaultPosition,
                wxDefaultSize,
                wxNO_FULL_REPAINT_ON_RESIZE |
                wxCLIP_CHILDREN |
                wxTAB_TRAVERSAL))
    {
        return false;
    }

    construct();
    return true;
}


void ExprBuilderPanel::construct()
{

    m_expr_text = new ExprEditorControl(this,
                                        ID_Expr,
                                        wxDefaultPosition,
                                        wxSize(100,40),
                                        kcl::DEFAULT_BORDER | wxVSCROLL);

    // make the m_expr_text text control a drop target for these grids
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(NULL);
    drop_target->setGridDataObjectFormats(wxT("exprbuilder_funclist"),
                                          wxT("exprbuilder_fieldlist"));
    drop_target->sigDropped.connect(this, &ExprBuilderPanel::onGridDataDropped);
    m_expr_text->SetDropTarget(drop_target);
    
    m_expr_text->SetFont(wxFont(wxNORMAL_FONT->GetPointSize()+2,
                         wxMODERN,
                         wxNORMAL,
                         wxNORMAL));

    
    // these should be created right after the ExprEditorControl above,
    // so that the tab order of the dialog is correct
    m_ok_button = new wxButton(this, wxID_OK);
    m_cancel_button = new wxButton(this, wxID_CANCEL);
    m_ok_button->SetDefault();


    wxBitmapButton* plus_button = new wxBitmapButton(this,
                                       ID_OperPlus,
                                       GETBMP(gf_ebbc9_16));

    wxBitmapButton* minus_button = new wxBitmapButton(this,
                                       ID_OperMinus,
                                       GETBMP(gf_ebbc10_16));

    wxBitmapButton* mult_button = new wxBitmapButton(this,
                                       ID_OperMult,
                                       GETBMP(gf_ebbc8_16));

    wxBitmapButton* div_button = new wxBitmapButton(this,
                                       ID_OperDiv,
                                       GETBMP(gf_ebbc7_16));

    wxBitmapButton* lparen_button = new wxBitmapButton(this,
                                       ID_OperLParen,
                                       GETBMP(gf_ebbc12_16));

    wxBitmapButton* rparen_button = new wxBitmapButton(this,
                                       ID_OperRParen,
                                       GETBMP(gf_ebbc11_16));

    wxBitmapButton* eq_button = new wxBitmapButton(this,
                                       ID_OperEq,
                                       GETBMP(gf_ebbc3_16));

    wxBitmapButton* neq_button = new wxBitmapButton(this,
                                       ID_OperNeq,
                                       GETBMP(gf_ebbc4_16));

    wxBitmapButton* lt_button = new wxBitmapButton(this,
                                       ID_OperLt,
                                       GETBMP(gf_ebbc2_16));

    wxBitmapButton* gt_button = new wxBitmapButton(this,
                                       ID_OperGt,
                                       GETBMP(gf_ebbc1_16));

    wxBitmapButton* lte_button = new wxBitmapButton(this,
                                       ID_OperLte,
                                       GETBMP(gf_ebbc6_16));

    wxBitmapButton* gte_button = new wxBitmapButton(this,
                                       ID_OperGte,
                                       GETBMP(gf_ebbc5_16));

    wxBitmapButton* and_button = new wxBitmapButton(this,
                                       ID_OperAnd,
                                       GETBMP(gf_and),
                                       wxDefaultPosition,
                                       wxSize(40,24));

    wxBitmapButton* or_button = new wxBitmapButton(this,
                                       ID_OperOr,
                                       GETBMP(gf_or),
                                       wxDefaultPosition,
                                       wxSize(40,24));

    wxBitmapButton* not_button = new wxBitmapButton(this, 
                                       ID_OperNot,
                                       GETBMP(gf_not),
                                       wxDefaultPosition,
                                       wxSize(40,24));

    int spacer = 2;
    
    wxBoxSizer* oper_sizer1 = new wxBoxSizer(wxHORIZONTAL);
    oper_sizer1->Add(plus_button, 1);
    oper_sizer1->AddSpacer(spacer);
    oper_sizer1->Add(minus_button, 1);
    oper_sizer1->AddSpacer(spacer);
    oper_sizer1->Add(mult_button, 1);
    oper_sizer1->AddSpacer(spacer);
    oper_sizer1->Add(div_button, 1);
    oper_sizer1->AddSpacer(spacer);
    oper_sizer1->Add(and_button, 0);
    
    wxBoxSizer* oper_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    oper_sizer2->Add(lparen_button, 1);
    oper_sizer2->AddSpacer(spacer);
    oper_sizer2->Add(rparen_button, 1);
    oper_sizer2->AddSpacer(spacer);
    oper_sizer2->Add(eq_button, 1);
    oper_sizer2->AddSpacer(spacer);
    oper_sizer2->Add(neq_button, 1);
    oper_sizer2->AddSpacer(spacer);
    oper_sizer2->Add(or_button, 0);
    
    wxBoxSizer* oper_sizer3 = new wxBoxSizer(wxHORIZONTAL);
    oper_sizer3->Add(lt_button, 1);
    oper_sizer3->AddSpacer(spacer);
    oper_sizer3->Add(gt_button, 1);
    oper_sizer3->AddSpacer(spacer);
    oper_sizer3->Add(lte_button, 1);
    oper_sizer3->AddSpacer(spacer);
    oper_sizer3->Add(gte_button, 1);
    oper_sizer3->AddSpacer(spacer);
    oper_sizer3->Add(not_button, 0);

    m_valid_control = new kcl::ValidControl(this, -1);
    m_valid_control->setValidBitmap(GETBMP(gf_checkmark_16));
    m_valid_control->setInvalidBitmap(GETBMP(gf_x_16));

    wxBoxSizer* button_sizer = new wxBoxSizer(wxVERTICAL);
    button_sizer->Add(m_valid_control, 0, wxEXPAND);
    button_sizer->AddSpacer(spacer*2);
    button_sizer->Add(oper_sizer1, 0, wxEXPAND);
    button_sizer->AddSpacer(spacer);
    button_sizer->Add(oper_sizer2, 0, wxEXPAND);
    button_sizer->AddSpacer(spacer);
    button_sizer->Add(oper_sizer3, 0, wxEXPAND);


    if (!m_validation)
    {
        // hide the sizer, and add a little static space at the top
        // and the bottom to compensate
        button_sizer->Prepend(1, 10);
        button_sizer->Show(m_valid_control, false);
        button_sizer->Add(1, 10);
    }
    

    //button_sizer->SetItemMinSize(m_valid_control, okcancel_sizer->CalcMin());

    // create function list
    m_func_list = new kcl::RowSelectionGrid(this,
                                            ID_ExprBuilderFunctionList,
                                            wxDefaultPosition,
                                            wxSize(145,30),
                                            kcl::DEFAULT_BORDER,
                                            true, false);
    m_func_list->setOptionState(kcl::Grid::optColumnResize |
                                kcl::Grid::optEdit,
                                false);
    m_func_list->setAcceptsFocus(false);
    m_func_list->setDragFormat(wxT("exprbuilder_funclist"));
    m_func_list->setGreenBarInterval(0);

    m_func_list->createModelColumn(-1,
                                   _("Function Name"),
                                   kcl::Grid::typeCharacter,
                                   256,
                                   0);
    m_func_list->createDefaultView();

    m_func_list->setRowHeight(m_func_list->getRowHeight()-2);
    m_func_list->setColumnProportionalSize(0, 1);

    int funcinfo_count = func_info_list.size();
    int i;
    for (i = 0; i < funcinfo_count; ++i)
    {
        wxString s = func_info_list[i].name;
        s.MakeUpper();
        m_func_list->insertRow(-1);
        m_func_list->setCellString(i, 0, s);
        m_func_list->setCellBitmap(i, 0, GETBMP(gf_function_16));
    }
    
    // create column list
    m_col_list = new FieldListControl(this, ID_ExprBuilderFieldListCtrl, wxDefaultPosition, wxSize(140,30));
    m_col_list->setDragFormat(wxT("exprbuilder_fieldlist"));
    m_col_list->sigFieldDblClicked.connect(this, &ExprBuilderPanel::onColumnActivated);
    
    wxCoord text_height;
    {
        wxClientDC dc(this);
        wxCoord x;
        dc.GetTextExtent(wxT("XXXXHHHH"), &x, &text_height);
    }
    
    // create bottom sizer
    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    bottom_sizer->Add(m_func_list, 0, wxEXPAND);
    bottom_sizer->Add(m_col_list, 1, wxEXPAND | wxLEFT, 8);
    bottom_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT, 8);
    bottom_sizer->SetItemMinSize(m_func_list, 150, 1);

    // create helper sizer
    
    m_func_syntax_label = new wxStaticText(this, -1, wxEmptyString);

    m_learn_more_hyperlink = new wxHyperlinkCtrl(this,
                                                 ID_LearnMoreHyperlinkCtrl,
                                                 _("More..."),
                                                 wxT("about:blank"));
    m_learn_more_hyperlink->SetVisitedColour(m_learn_more_hyperlink->GetNormalColour());
    m_learn_more_hyperlink->SetHoverColour(m_learn_more_hyperlink->GetNormalColour());

    m_helper_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_helper_sizer->Add(m_func_syntax_label, 0, wxALIGN_CENTER);
    m_helper_sizer->AddSpacer(10);
    m_helper_sizer->Add(m_learn_more_hyperlink, 0, wxALIGN_CENTER);
    m_helper_sizer->AddStretchSpacer();

    // create a platform standards-compliant OK/Cancel sizer
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(m_ok_button);
    ok_cancel_sizer->AddButton(m_cancel_button);
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->Prepend(m_helper_sizer, 1, wxEXPAND | wxLEFT, 8);
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    // create main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(8);
    m_main_sizer->Add(m_expr_text, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    m_main_sizer->AddSpacer(8);
    m_main_sizer->Add(bottom_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    m_main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(m_main_sizer);

    // refresh the row selection grid
    m_func_list->moveCursor(1,0);
    m_func_list->moveCursor(0,0);
    m_func_list->setRowSelected(0, true);
    m_func_list->refresh(kcl::Grid::refreshAll);

    m_expr_text->SetFocus();

    validate();
}

void ExprBuilderPanel::setOKEnabled(bool enable)
{
    m_ok_button->Enable(enable);
}

void ExprBuilderPanel::setOKVisible(bool visible)
{
    m_ok_button->Show(visible);
}

void ExprBuilderPanel::setCancelVisible(bool visible)
{
    m_cancel_button->Show(visible);
}

void ExprBuilderPanel::setOKText(const wxString& ok_text)
{
    m_ok_text = ok_text;
    if (m_ok_button)
        m_ok_button->SetLabel(m_ok_text);
}

void ExprBuilderPanel::setCancelText(const wxString& cancel_text)
{
    m_cancel_text = cancel_text;
    if (m_cancel_button)
        m_cancel_button->SetLabel(m_cancel_text);
}

void ExprBuilderPanel::setEnabled(bool enabled)
{
    m_expr_text->Enable(enabled);
}


// type only causes the expression builder to accept only expression types
// that are equal to that which is passed in the "type" parameter.  If
// tango::typeUndefined is passed, all types are allowed again

void ExprBuilderPanel::setTypeOnly(int type)
{
    m_type_only = type;

    if (m_ok_button)
    {
        // if the window already exists, recheck validation
        validate();
    }
}

void ExprBuilderPanel::setEmptyOk(bool empty_ok)
{
    m_empty_ok = empty_ok;

    if (m_ok_button)
    {
        // if the window already exists, recheck validation
        validate();
    }
}

void ExprBuilderPanel::setIterator(tango::IIteratorPtr iter)
{
    m_iter = iter;
    m_structure = m_iter->getStructure();

    m_col_list->setStructure(m_structure);
}

wxString ExprBuilderPanel::getExpression()
{
    return m_expr_text->GetValue();
}

int ExprBuilderPanel::getExpressionType()
{
    return m_expr_type;
}

void ExprBuilderPanel::setExpression(const wxString& new_expression)
{
    m_mask_signals = true;
    m_expr_text->SetValue(new_expression);
    m_expr_text->SetInsertionPointEnd();
    m_mask_signals = false;

    validate();
}

void ExprBuilderPanel::insertText(const wxString& new_text)
{
    if (!m_expr_text->IsEnabled())
    {
        return;
    }

    wxString text = m_expr_text->GetValue();
    int insert_loc = m_expr_text->GetInsertionPoint();

    wxString final = text.Left(insert_loc);
    final += new_text;
    final += text.Mid(insert_loc);

    m_expr_text->SetValue(final);
    m_expr_text->SetInsertionPoint(insert_loc + new_text.Length());

    validate();

    sigExpressionChanged(this);

    m_expr_text->SetFocus();
}


bool ExprBuilderPanel::validate()
{
    // first, check if validation is turned off.  If validation is
    // off, always return true.  Validation is sometimes turned off
    // for external databases.
    if (!m_validation)
        return true;
        
    wxString text = m_expr_text->GetValue();
    bool valid = false;
    int type = tango::typeInvalid;
    
    if (m_iter)
    {
        tango::objhandle_t handle = m_iter->getHandle(towstr(text));
        if (handle)
        {
            type = m_iter->getType(handle);
            m_iter->releaseHandle(handle);
        }
    }

    wxString valid_text;
    valid_text = getDbColumnTypeText(type);

    if (m_type_only != tango::typeUndefined &&
        !tango::isTypeCompatible(type, m_type_only))
    {
        valid_text = _("Invalid");
    }

    if (m_empty_ok && text.IsEmpty())
    {
        valid_text = _("Valid");
    }

    m_valid_control->setValidLabel(valid_text);
    m_valid_control->setInvalidLabel(valid_text);

    if (type == tango::typeInvalid)
    {
        valid = false;
    }
     else
    {
        valid = true;

        if (m_type_only != tango::typeUndefined)
        {
            if (!tango::isTypeCompatible(type, m_type_only))
                valid = false;
        }
    }

    if (m_empty_ok && text.IsEmpty())
    {
        valid = true;
    }

    m_valid_control->setValid(valid);
    
    m_ok_button->Enable(valid);

    m_expr_type = type;

    return valid;
}

void ExprBuilderPanel::onExprChanged(wxCommandEvent& evt)
{
    validate();

    if (!m_mask_signals)
    {
        sigExpressionChanged(this);
    }
}

void ExprBuilderPanel::onColumnActivated(int row, const wxString& text)
{
    wxString quoted_text = towx(tango::quoteIdentifier(g_app->getDatabase(), towstr(text)));
    insertText(quoted_text);
}

void ExprBuilderPanel::onFunctionActivated(kcl::GridEvent& evt)
{
    if (evt.GetEventObject() != m_func_list)
    {
        evt.Skip();
        return;
    }

    wxString str = m_func_list->getCellString(evt.GetRow(), 0);
    str += wxT("(");
    insertText(str);
}

void ExprBuilderPanel::onFunctionListCursorMove(kcl::GridEvent& evt)
{
    if (evt.GetEventObject() != m_func_list)
    {
        evt.Skip();
        return;
    }
    
    int row = m_func_list->getCursorRow();
    if (row < 0 || row >= (int)func_info_list.size())
    {
        m_func_syntax_label->Show(false);
        m_learn_more_hyperlink->Show(false);
    }
     else
    {
        m_func_syntax_label->Show(true);
        m_learn_more_hyperlink->Show(true);
        m_func_syntax_label->SetLabel(func_info_list[row].syntax);
        m_learn_more_hyperlink->SetURL(func_info_list[row].helpurl);
    }
    
    Layout();
}

void ExprBuilderPanel::onFunctionListNeedTooltipText(kcl::GridEvent& evt)
{
    if (evt.GetEventObject() != m_func_list)
    {
        evt.Skip();
        return;
    }
    
    // don't know why, but for some reason this event's row number
    // is sometimes -1, which causes a crash
    int row = evt.GetRow();
    if (row < 0 || row >= (int)func_info_list.size())
        return;
    
    wxString tip = func_info_list[row].syntax;
    tip += wxT(" - ");
    tip += func_info_list[row].helptext;
    evt.SetString(tip);
}

void ExprBuilderPanel::onLearnMoreHyperlinkClicked(wxHyperlinkEvent& evt)
{
    wxString url = evt.GetURL();
    g_app->getAppController()->openWeb(url);
}

void ExprBuilderPanel::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    kcl::GridDraggedCells cells = drop_target->getDraggedCells();
    kcl::GridDraggedCells::iterator it;

    if (drop_target->getSourceGridId() == ID_ExprBuilderFunctionList)
    {
        for (it = cells.begin(); it != cells.end(); ++it)
        {
            if ((*it)->m_col == 0)
            {
                wxString str = (*it)->m_strvalue;
                str += wxT("(");
                insertText(str);
            }
        }
    }
     else if (drop_target->getSourceGridId() == ID_ExprBuilderFieldListCtrl)
    {
        for (it = cells.begin(); it != cells.end(); ++it)
        {
            if ((*it)->m_col == 0)
                insertText((*it)->m_strvalue);
        }
    }
}

void ExprBuilderPanel::onOperButtonClicked(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
        case ID_OperPlus:    insertText(wxT(" + "));    break;
        case ID_OperMinus:   insertText(wxT(" - "));    break;
        case ID_OperMult:    insertText(wxT(" * "));    break;
        case ID_OperDiv:     insertText(wxT(" / "));    break;
        case ID_OperLParen:  insertText(wxT("("));      break;
        case ID_OperRParen:  insertText(wxT(")"));      break;
        case ID_OperEq:      insertText(wxT(" = "));    break;
        case ID_OperNeq:     insertText(wxT(" != "));   break;
        case ID_OperLt:      insertText(wxT(" < "));    break;
        case ID_OperGt:      insertText(wxT(" > "));    break;
        case ID_OperLte:     insertText(wxT(" <= "));   break;
        case ID_OperGte:     insertText(wxT(" >= "));   break;
        case ID_OperAnd:     insertText(wxT(" AND "));  break;
        case ID_OperOr:      insertText(wxT(" OR "));   break;
        case ID_OperNot:     insertText(wxT(" NOT "));  break;
    }
}

void ExprBuilderPanel::onOK(wxCommandEvent& evt)
{
    sigOkPressed(this);
}

void ExprBuilderPanel::onCancel(wxCommandEvent& evt)
{
    sigCancelPressed(this);
}

void ExprBuilderPanel::onSize(wxSizeEvent& evt)
{
    Layout();
}








BEGIN_EVENT_TABLE(KeyBuilderPanel, wxPanel)
    EVT_BUTTON(wxID_OK, KeyBuilderPanel::onOK)
    EVT_BUTTON(wxID_CANCEL, KeyBuilderPanel::onCancel)
    EVT_KCLGRID_MODEL_CHANGE(KeyBuilderPanel::onGridModelChange)
END_EVENT_TABLE()



KeyBuilderPanel::KeyBuilderPanel()
{
    m_mask_signals = false;
    m_empty_ok = false;
    m_with_orders = true;
    m_grid = NULL;

    m_ok_button = NULL;
    m_cancel_button = NULL;
    m_ok_text = _("OK");
    m_cancel_text = _("Cancel");
}

KeyBuilderPanel::KeyBuilderPanel(wxWindow* parent)
                              : wxPanel(parent,
                                        -1,
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxNO_FULL_REPAINT_ON_RESIZE |
                                        wxCLIP_CHILDREN |
                                        wxTAB_TRAVERSAL)
{
    m_mask_signals = false;
    m_empty_ok = false;
    m_with_orders = true;

    m_ok_button = NULL;
    m_cancel_button = NULL;
    m_ok_text = _("OK");
    m_cancel_text = _("Cancel");

    construct();
}

void KeyBuilderPanel::setNoOrders()
{
    if (!m_with_orders)
        return;

    m_with_orders = false;

    if (m_grid)
    {
        m_grid->deleteAllRows();
        kcl::IModelPtr model;
        model = m_grid->getModel();
        model->deleteColumn(1);
        m_grid->createDefaultView();

        m_grid->setColumnProportionalSize(0, 3);
        m_grid->setColumnProportionalSize(1, 1);

        parseExpr();
    }
}

void KeyBuilderPanel::updateGridOverlay()
{
    if (!m_grid)
        return;
    
    // this code probably should be in it's own function, but it's convenient
    // to have here since it's related to how many rows are in the grid
    m_ok_button->Enable(m_grid->getRowCount() > 0 ? true : false);
    
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(m_overlay_text);
         else
        m_grid->setOverlayText(wxEmptyString);
}


void KeyBuilderPanel::setOverlayText(const wxString& str)
{
    m_overlay_text = str;
    updateGridOverlay();
}

bool KeyBuilderPanel::create(wxWindow* parent)
{
    if (!Create(parent,
                -1,
                wxDefaultPosition,
                wxDefaultSize,
                wxNO_FULL_REPAINT_ON_RESIZE |
                wxCLIP_CHILDREN |
                wxTAB_TRAVERSAL))
    {
        return false;
    }

    construct();
    return true;
}

void KeyBuilderPanel::construct()
{
    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);


    // create column list

    m_col_list = new FieldListControl(this,
                                      ID_KeyBuilderFieldListCtrl,
                                      wxDefaultPosition,
                                      wxSize(150,100));
    m_col_list->setDragFormat(wxT("keybuilder_fields"));
    m_col_list->sigFieldDblClicked.connect(this, &KeyBuilderPanel::onColumnActivated);


    // create sort order grid

    m_grid = new kcl::RowSelectionGrid(this);
    m_grid->setOptionState(kcl::Grid::optEditBoxBorder |
                           kcl::Grid::optColumnResize,
                           false);
    m_grid->setDragFormat(wxT("keybuilder_sortlist"));
    m_grid->setAllowDeletes(true);

    m_grid->createModelColumn(-1, _("Field Name"), kcl::Grid::typeCharacter, 1024, 0);

    // the list of fields should not be editable
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskEditable;
    cellprops.editable = false;
    m_grid->setModelColumnProperties(0, &cellprops);

    if (m_with_orders)
    {
        m_grid->createModelColumn(-1, _("Sort Direction"), kcl::Grid::typeCharacter, 1024, 0);

        kcl::CellProperties props;
        props.mask = kcl::CellProperties::cpmaskCtrlType | kcl::CellProperties::cpmaskCbChoices;
        props.ctrltype = kcl::Grid::ctrltypeDropList;
        props.cbchoices.push_back(_("Ascending"));
        props.cbchoices.push_back(_("Descending"));

        m_grid->setModelColumnProperties(1, &props);
    }
                                     
    m_grid->createDefaultView();
    m_grid->setColumnProportionalSize(0, 1);

    if (m_with_orders)
        m_grid->setColumnSize(1, 120);

    // create the drop target for the grid and connect the signal
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_grid);
    drop_target->setGridDataObjectFormats(wxT("keybuilder_fields"), wxT("keybuilder_sortlist"));
    drop_target->sigDropped.connect(this, &KeyBuilderPanel::onGridDataDropped);
    m_grid->SetDropTarget(drop_target);

    // connect other signals
    m_grid->sigDeletedRows.connect(this, &KeyBuilderPanel::onGridDeletedRows);
    

    // create horizontal sizer
    
    wxBoxSizer* horz_sizer = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer->Add(m_col_list, 0, wxEXPAND);
    horz_sizer->AddSpacer(8);
    horz_sizer->Add(m_grid, 1, wxEXPAND);


    // create a platform standards-compliant OK/Cancel sizer
    
    m_ok_button = new wxButton(this, wxID_OK, m_ok_text);
    m_cancel_button = new wxButton(this, wxID_CANCEL);
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(m_ok_button);
    ok_cancel_sizer->AddButton(m_cancel_button);
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);


    // create main sizer
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(horz_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);

    updateGridOverlay();

    // refresh the row selection grid
    m_grid->refresh(kcl::Grid::refreshAll);

    SetSizer(main_sizer);
    Layout();
}

void KeyBuilderPanel::onGridModelChange(kcl::GridEvent& evt)
{
    buildExpr();
}

void KeyBuilderPanel::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    int drop_row = drop_target->getDropRow();
    wxWindowID source_id = drop_target->getSourceGridId();
    
    if (source_id == ID_KeyBuilderFieldListCtrl)
    {
        kcl::GridDraggedCells cells = drop_target->getDraggedCells();
        kcl::GridDraggedCells::iterator it;
        for (it = cells.begin(); it != cells.end(); ++it)
        {
            if ((*it)->m_col != 0)
                continue;
                
            m_grid->insertRow(drop_row);
            m_grid->setCellString(drop_row, 0, (*it)->m_strvalue);
            
            if (m_with_orders)
            {
                m_grid->setCellComboSel(drop_row, 1, 0);
            }
            
            drop_row++;
        }
    }
     else
    {
        drop_target->doRowDrag(false);
    }

    updateGridOverlay();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void KeyBuilderPanel::onGridDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    updateGridOverlay();
}

void KeyBuilderPanel::buildExpr()
{
    int key_colidx = 0;
    int direction_colidx = 1;

    wxString s;

    int row_count = m_grid->getRowCount();
    int row;

    for (row = 0; row < row_count; ++row)
    {
        if (!s.IsEmpty())
            s += wxT(", ");

        s += m_grid->getCellString(row, key_colidx);

        if (m_with_orders)
        {
            if (m_grid->getCellComboSel(row, direction_colidx) == 1)
            {
                s += wxT(" DESC");
            }
        } 
    }

    m_expr = s;
}


void KeyBuilderPanel::parseExpr()
{
    int key_colidx = 0;
    int direction_colidx = 1;

    m_grid->deleteAllRows();

    std::vector<wxString> pieces;

    // parse pieces

    const wxChar* start = m_expr.c_str();
    const wxChar* end;
    bool done = false;

    while (wxIsspace(*start))
        start++;

    while (1)
    {
        end = zl_strchr((wxChar*)start, wxT(','));

        if (!end)
        {
            end = start + ::wxStrlen(start);
            done = true;
        }
        
        wxString piece(start, end-start);
        piece.Trim(true);

        if (piece.length() == 0 && done)
            break;

        pieces.push_back(piece);

        start = end+1;
        while (wxIsspace(*start))
            start++;

        if (!*start || done)
        {
            break;
        }
    }

    // populate grid

    int row = 0;
    
    int direction;

    

    std::vector<wxString>::iterator it;

    for (it = pieces.begin(); it != pieces.end(); ++it)
    {
        m_grid->insertRow(row);

        wxString piece;
        piece = *it;
        piece.MakeUpper();

        start = piece.c_str();
        direction = 0;

        wxString dir_piece = piece.AfterLast(wxT(' '));
        
        if (dir_piece.Length() < piece.Length())
        {
            if (!wxStrncmp(dir_piece, wxT("ASC"), 3))
            {
                direction = 0;
                piece = piece.BeforeLast(wxT(' '));
            }
             else if (!wxStrncmp(dir_piece, wxT("DESC"), 4))
            {
                direction = 1;
                piece = piece.BeforeLast(wxT(' '));
            }
        }

        m_grid->setCellString(row, key_colidx, makeProper(piece));

        if (m_with_orders)
        {
            m_grid->setCellComboSel(row, direction_colidx, direction);
        }

        row++;
    }

    updateGridOverlay();

    m_grid->moveCursor(0,0);
    m_grid->refresh(kcl::Grid::refreshAll);
}

void KeyBuilderPanel::setOKVisible(bool visible)
{
    m_ok_button->Show(visible);
}

void KeyBuilderPanel::setCancelVisible(bool visible)
{
    m_cancel_button->Show(visible);
}

void KeyBuilderPanel::setOKText(const wxString& ok_text)
{
    m_ok_text = ok_text;
    if (m_ok_button)
    {
        m_ok_button->SetLabel(m_ok_text);
    }
}

void KeyBuilderPanel::setCancelText(const wxString& cancel_text)
{
    m_cancel_text = cancel_text;
    if (m_cancel_button)
    {
        m_cancel_button->SetLabel(m_cancel_text);
    }
}

void KeyBuilderPanel::setEmptyOk(bool empty_ok)
{
    m_empty_ok = empty_ok;

    validate();
}

void KeyBuilderPanel::setIterator(tango::IIteratorPtr iter)
{
    m_iter = iter;
    m_structure = m_iter->getStructure();

    m_col_list->setStructure(m_structure);
}

tango::IIteratorPtr KeyBuilderPanel::getIterator()
{
    return m_iter;
}

wxString KeyBuilderPanel::getExpression()
{
    return m_expr;
}

void KeyBuilderPanel::setExpression(const wxString& expr)
{
    m_expr = expr;
    parseExpr();
}

void KeyBuilderPanel::setEnabled(bool enabled)
{

}

bool KeyBuilderPanel::validate()
{
    return false;
}


void KeyBuilderPanel::onOK(wxCommandEvent& evt)
{
    buildExpr();
    sigOkPressed(this);
}

void KeyBuilderPanel::onCancel(wxCommandEvent& evt)
{
    sigCancelPressed(this);
}

void KeyBuilderPanel::onColumnActivated(int row, const wxString& text)
{
    if (m_structure.isNull())
        return;

    int insert_row = m_grid->getRowCount();
    m_grid->insertRow(-1);
    m_grid->setCellString(insert_row, 0, text);
    
    if (m_with_orders)
    {
        m_grid->setCellComboSel(insert_row, 1, 0);
    }
    
    updateGridOverlay();
    m_grid->refresh(kcl::Grid::refreshAll);
}






// -- DlgExprBuilder class implementation --

BEGIN_EVENT_TABLE(DlgExprBuilder, wxDialog)
    EVT_BUTTON(wxID_CANCEL, DlgExprBuilder::onCancel)
END_EVENT_TABLE()

DlgExprBuilder::DlgExprBuilder(wxWindow* parent, const wxString& title)
      : wxDialog(parent,
                 -1,
                 title,
                 wxDefaultPosition,
                 wxDefaultSize,
                  wxDEFAULT_DIALOG_STYLE |
                  wxRESIZE_BORDER |
                  wxNO_FULL_REPAINT_ON_RESIZE |
                  wxCLIP_CHILDREN)
{

    m_panel = new ExprBuilderPanel(this);
    m_panel->sigOkPressed.connect(this, &DlgExprBuilder::onOKPressed);
    m_panel->sigCancelPressed.connect(this, &DlgExprBuilder::onCancelPressed);

    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->Add(m_panel, 1, wxEXPAND);

    SetSizer(m_main_sizer);

    SetSize(560, 300);
    CentreOnScreen();

    Layout();
}

void DlgExprBuilder::setIterator(tango::IIteratorPtr iter)
{
    m_panel->setIterator(iter);
}

wxString DlgExprBuilder::getExpression()
{
    return m_panel->getExpression();
}

void DlgExprBuilder::setExpression(const wxString& expr)
{
    m_panel->setExpression(expr);
}

void DlgExprBuilder::setTypeOnly(int type)
{
    m_panel->setTypeOnly(type);
}

void DlgExprBuilder::setEmptyOk(bool empty_ok)
{
    m_panel->setEmptyOk(empty_ok);
}

void DlgExprBuilder::setOKVisible(bool visible)
{
    m_panel->setOKVisible(visible);
}

void DlgExprBuilder::setCancelVisible(bool visible)
{
    m_panel->setCancelVisible(visible);
}

void DlgExprBuilder::setOKText(const wxString& ok_text)
{
    m_panel->setOKText(ok_text);
}

void DlgExprBuilder::setCancelText(const wxString& cancel_text)
{
    m_panel->setCancelText(cancel_text);
}

void DlgExprBuilder::onOKPressed(ExprBuilderPanel*)
{
    sigExprEditFinished(m_panel->getExpression());
    if (IsModal())
    {
        EndModal(wxID_OK);
    }
     else
    {
        Destroy();
    }
}

void DlgExprBuilder::onCancelPressed(ExprBuilderPanel*)
{
    sigExprEditCancelled();
    if (IsModal())
    {
        EndModal(wxID_CANCEL);
    }
     else
    {
        Destroy();
    }
}

void DlgExprBuilder::onCancel(wxCommandEvent& evt)
{
    sigExprEditCancelled();
    if (IsModal())
    {
        EndModal(wxID_CANCEL);
    }
     else
    {
        Destroy();
    }
}






// -- DlgKeyBuilder class implementation --

BEGIN_EVENT_TABLE(DlgKeyBuilder, wxDialog)
    EVT_BUTTON(wxID_CANCEL, DlgKeyBuilder::onCancel)
END_EVENT_TABLE()

DlgKeyBuilder::DlgKeyBuilder(wxWindow* parent, const wxString& title)
      : wxDialog(parent, -1, title,
                 wxDefaultPosition, wxSize(560,300),
                 wxDEFAULT_DIALOG_STYLE |
                 wxRESIZE_BORDER |
                 wxNO_FULL_REPAINT_ON_RESIZE |
                 wxCLIP_CHILDREN)
{

    m_panel = new KeyBuilderPanel(this);
    m_panel->sigOkPressed.connect(this, &DlgKeyBuilder::onOKPressed);
    m_panel->sigCancelPressed.connect(this, &DlgKeyBuilder::onCancelPressed);

    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->Add(m_panel, 1, wxEXPAND);

    CentreOnScreen();

    SetSizer(m_main_sizer);
}

void DlgKeyBuilder::setIterator(tango::IIteratorPtr iter)
{
    m_panel->setIterator(iter);
}

wxString DlgKeyBuilder::getExpression()
{
    return m_panel->getExpression();
}

void DlgKeyBuilder::setExpression(const wxString& expr)
{
    m_panel->setExpression(expr);
}

void DlgKeyBuilder::setEmptyOk(bool empty_ok)
{
    m_panel->setEmptyOk(empty_ok);
}

void DlgKeyBuilder::setNoOrders()
{
    DlgKeyBuilder::m_panel->setNoOrders();
}

void DlgKeyBuilder::setOverlayText(const wxString& expr)
{
    m_panel->setOverlayText(expr);
}

void DlgKeyBuilder::setOKVisible(bool visible)
{
    m_panel->setOKVisible(visible);
}

void DlgKeyBuilder::setCancelVisible(bool visible)
{
    m_panel->setCancelVisible(visible);
}

void DlgKeyBuilder::setOKText(const wxString& ok_text)
{
    m_panel->setOKText(ok_text);
}

void DlgKeyBuilder::setCancelText(const wxString& cancel_text)
{
    m_panel->setCancelText(cancel_text);
}


void DlgKeyBuilder::onOKPressed(KeyBuilderPanel*)
{
    sigExprEditFinished(m_panel->getExpression());
    if (IsModal())
    {
        EndModal(wxID_OK);
    }
     else
    {
        Destroy();
    }
}

void DlgKeyBuilder::onCancelPressed(KeyBuilderPanel*)
{
    sigExprEditCancelled();
    if (IsModal())
    {
        EndModal(wxID_CANCEL);
    }
     else
    {
        Destroy();
    }
}

void DlgKeyBuilder::onCancel(wxCommandEvent& evt)
{
    sigExprEditCancelled();
    if (IsModal())
    {
        EndModal(wxID_CANCEL);
    }
     else
    {
        Destroy();
    }
}

