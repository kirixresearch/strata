/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-10
 *
 */


#ifndef __KCANVAS_COMPONENTTEXTBOX_H
#define __KCANVAS_COMPONENTTEXTBOX_H


namespace kcanvas
{


// Canvas TextBox Component Classes

class TextProperties;

class CompTextBox : public Component,
                    public ICompTextBox,
                    public IEdit
{
    XCM_CLASS_NAME("kcanvas.CompTextBox")
    XCM_BEGIN_INTERFACE_MAP(CompTextBox)
        XCM_INTERFACE_ENTRY(ICompTextBox)
        XCM_INTERFACE_ENTRY(IEdit)
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompTextBox();
    virtual ~CompTextBox();

    static IComponentPtr create();
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    void addProperty(const wxString& prop_name, const PropertyValue& value);
    void addProperties(const Properties& properties);
    void removeProperty(const wxString& prop_name);
    bool setProperty(const wxString& prop_name, const PropertyValue& value);
    bool setProperties(const Properties& properties);
    void extends(wxRect& rect);
    void render(const wxRect& rect = wxRect());

    // ICompTextBox
    void setText(const wxString& text);
    const wxString& getText();

    void setInsertionPointByPos(int x, int y);
    void getInsertionPointByPos(int* x, int* y);

    void setSelectionByPos(int x1, int y1, int x2, int y2);
    void getSelectionByPos(int* x1, int* y1, int* x2, int* y2);

    // IEdit interface
    void beginEdit();
    void endEdit(bool accept);
    bool isEditing() const;

    bool canCut() const;
    bool canCopy() const;
    bool canPaste() const;

    void cut();
    void copy();
    void paste();

    void selectAll();
    void selectNone();

    void clear(bool text = true);

private:

    // event handlers
    void onKey(IEventPtr evt);
    void onMouse(IEventPtr evt);

    // mouse event handler helpers
    bool startAction(IEventPtr evt, wxString action);
    bool endAction(IEventPtr evt, wxString action);
    bool isAction(IEventPtr evt, wxString action);
    void resetAction();

private:

    void posToXY(int pos, int* x, int* y);
    int xyToPos(int* x, int* y);

    int findEndOfLine(int* x, int* y);
    int findBegOfLine(int* x, int* y);
    int findNextWord(int pos);
    int findPrevWord(int pos);
    
    void moveBeginning(int keystate);
    void moveEnd(int keystate);

    void setCursorPos(int pos);
    int getCursorPos() const;

    void moveCursor(int row_diff, int col_diff, int keystate);
    void insertCharAtCursor(int keycode, int keystate);
    void deleteCharBeforeCursor(int keystate);
    void deleteCharAfterCursor(int keystate);
    bool deleteSelectedChars(wxString& text, int keystate);
    bool validDisplayChar(int keycode, int keystate);

    void setSelection(int x1, int x2);
    void clearSelection();

    bool hasSelection() const;
    void getSelection(int* x1, int* x2, bool ordered = false) const;
    int getSelectionStart() const;
    int getSelectionEnd() const;

    bool isCacheEmpty() const;
    void populateCache();
    void clearCache();

private:

    enum
    {
        KEYSTATE_NONE = 0x0000,
        KEYSTATE_CTRL_DOWN = 0x0001,
        KEYSTATE_SHIFT_DOWN = 0x0002,
        KEYSTATE_ALT_DOWN = 0x0004,
        KEYSTATE_INSERT_DOWN = 0x0008
    };

    // edit variables
    wxString m_text_before_edit;
    bool m_editing;

    // cached variables
    std::vector<TextProperties> m_text_lines;
    PropertyValue m_cache_value;

    Font m_cache_font;
    wxString m_cache_halign;
    wxString m_cache_valign;
    int m_cache_line_size;
    int m_cache_width;
    int m_cache_height;
    int m_cache_cursor_x;
    int m_cache_cursor_y;
    int m_cache_cursor_x_last;

    // cursor position
    int m_cursor_pos;

    // selection point location
    int m_selection_x1;        // the beginning position of the selection
    int m_selection_x2;        // the ending position of the selection

    // mouse variables
    wxString m_mouse_action;   // name of current mouse action
    
    int m_mouse_x;             // current x position of the mouse
    int m_mouse_y;             // current y position of the mouse
    int m_mouse_x_start;       // start x position of the mouse
    int m_mouse_y_start;       // start y position of the mouse
};


}; // namespace kcanvas


#endif

