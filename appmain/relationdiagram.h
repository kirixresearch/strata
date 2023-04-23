/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2008-02-06
 *
 */


#ifndef H_APP_RELATIONDIAGRAM_H
#define H_APP_RELATIONDIAGRAM_H


// -- forward declarations --
class RelationBox;
class RelationDiagram;
class RelationLineDataObject;


// -- function declarations --
void showAddTableDialog(RelationDiagram* diagram);




// -- RelationLine class declaration --

class RelationLine
{
public:

    wxString left_path;
    wxString left_expr;
    wxString right_path;
    wxString right_expr;

    wxBitmap left_bitmap;
    wxBitmap right_bitmap;

    wxString tag;
    int user;
    bool selected;
    bool valid;

    int x1, y1, x2, y2;     // for internal use
};


// -- RelationInfo class declaration --

class RelationInfo
{
public:

    wxString left_path;
    wxString right_path;
    wxString tag;

    std::vector<RelationLine> lines;
};




// -- RelationBox class declaration --

class RelationBox : public wxControl,
                    public xcm::signal_sink
{
public:

    enum ActionId
    {
        ActionNone = 0,
        ActionClick,
        ActionLine,
        ActionMove,
        ActionClose,
        ActionSEResize,
        ActionSWResize,
        ActionNWResize,
        ActionNEResize,
        ActionSResize,
        ActionEResize,
        ActionWResize,
        ActionNResize,
    };

    enum ButtonState
    {
        ButtonStateNormal = 1,
        ButtonStateHover = 2,
        ButtonStatePressed = 3,
        ButtonStateDisabled = 4
    };

    enum SortOrder
    {
        SortAscending = 1,
        SortDescending = 2,
        SortNone = 3
    };

    enum
    {
        ID_FieldList = 8000,
        ID_CursorTimer
    };

public:

    RelationBox(RelationDiagram* parent,
                wxWindowID id = -1,
                const wxString& title = wxEmptyString,
                const wxPoint& position = wxDefaultPosition,
                const wxSize& size = wxDefaultSize);
    ~RelationBox();
    
    void setCaption(const wxString& caption);
    
    void setSetPath(const wxString& set_path);
    wxString getSetPath();
    
    void setStructure(const xd::Structure& s);
    const xd::Structure& getStructure();
    
    bool getItemExists(const wxString& expr);
    int getItemY(const wxString& expr);
    
    void fireSizedMovedSignal();
    void clearSelection();
    void repopulate();
    void redraw();
    void raise();
    
private:

    void doSizing();
    void recalcCaption();
    void cycleSortOrder();
    void populate();
    
    bool isLinked(const wxString& expr);
    void diagramRefresh();

    // event handlers
    void onCreateLink(wxDragResult& drag_result, RelationLineDataObject* data);
    void onBeginDrag(kcl::GridEvent& event);
    void onLeftClick(kcl::GridEvent& event);
    void onLeftDblClick(kcl::GridEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onPaint(wxPaintEvent& event);
    void onMouse(wxMouseEvent& event);
    void onCursorTimer(wxTimerEvent& event);
    void onVScroll(kcl::GridEvent& event);
    void onSize(wxSizeEvent& event);
    
    // we want the relationship diagram to have the focus,
    // not the little boxes inside
    bool AcceptsFocusFromKeyboard() const { return false; }
    bool AcceptsFocus() const { return false; }
    
private:

    xd::Structure m_structure;

    wxString m_set_path;
    wxString m_caption;
    wxString m_paint_caption;
    wxBitmap m_closebutton_bitmap;
    wxColour m_base_color;
    wxColour m_caption_textcolor;
    wxPen m_border_pen;
    wxFont m_caption_font;
    wxTimer m_cursor_timer;

    RelationDiagram* m_diagram;
    kcl::Grid* m_grid;
    wxRect m_closebutton_rect;
    wxRect m_sortbutton_rect;
    
    int m_box_caption_height;
    int m_box_border_size;
    int m_box_min_width;
    int m_box_min_height;

    int m_action;
    int m_action_offx;
    int m_action_offy;
    
    int m_sort_order;

    DECLARE_EVENT_TABLE()
};




// -- RelationDiagram class declaration --

class RelationDiagram : public wxScrolledWindow,
                        public xcm::signal_sink
{
friend class RelationBox;

public:

    enum
    {
        MenuID_Cancel = 0,
        MenuID_DeleteLine = 8001
    };
    
    RelationDiagram(wxWindow* parent,
                    wxWindowID id = -1,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    bool x_scroll = false,
                    bool y_scroll = false);

    ~RelationDiagram();

    bool load();
    bool save();
    
    void setOverlayText(const wxString& overlay_text);
    
    bool addBox(const wxString& path,
                const wxString& caption,
                const wxColor& color = wxColor(0,0,128),
                int x = 0,
                int y = 0,
                int width = 0,
                int height = 0);

    RelationLine* addLine(const wxString& left_path,
                          const wxString& left_expr,
                          const wxString& right_path,
                          const wxString& right_expr);

    void getBoxPaths(std::vector<wxString>& paths);
    RelationBox* getBox(unsigned int idx);
    RelationBox* getBoxByName(const wxString& path);
    int getBoxCount();

    void getRelationInfo(const wxString& left_path,
                         std::vector<RelationInfo>& info);
    void getLines(const wxString& left_path,
                  const wxString& right_path,
                  std::vector<RelationLine*>& lines);
                  
    void getOrigRelationInfo(const wxString& left_path,
                             std::vector<RelationInfo>& info);
    void getOrigLines(const wxString& left_path,
                      const wxString& right_path,
                      std::vector<RelationLine*>& lines);

    void deleteLines(const wxString& path);
    void deleteAllLines();

    void refresh();
    void refreshBoxes();
    void redrawBoxes();

    void clearBoxSelection();
    void clearLineSelection();
    void clear();

    void resetModified();

    void onSetRenamed(const wxString& old_path,
                      const wxString& new_path);
    void onSetStructureChanged(const wxString& set_path);

public: // signals

    xcm::signal1<wxString> sigSetRemoved;
    xcm::signal2<wxString, bool*> sigSetAdded;
    xcm::signal2<RelationLine*, bool*> sigLineAdded;
    xcm::signal1<RelationLine*> sigLineDeleted;
    xcm::signal1<RelationLine*> sigLineActivated;
    xcm::signal2<RelationLine*, int*> sigLineRightClicked;
    xcm::signal2<wxString, wxRect> sigBoxSizedMoved;
    xcm::signal2<wxString, wxString> sigFieldActivated;
    xcm::signal0 sigDiagramUpdated;

private:

    void deleteLine(RelationLine* line);

    void repaint(wxDC* dc = NULL, bool render = true, wxRect* rect = NULL);

    RelationLine* hitTest(int x, int y);

    void drawConnectingLine(wxDC* dc,
                            int x1, int y1, int x2, int y2,
                            int direction,
                            bool bold = false,
                            bool valid = true,
                            const wxBitmap& from_bmp = wxNullBitmap,
                            const wxBitmap& to_bmp = wxNullBitmap,
                            wxPoint* out_pt1 = NULL,
                            wxPoint* out_pt2 = NULL);

    void _getRelationInfo(std::vector<RelationLine>& lines,
                          const wxString& left_path,
                          std::vector<RelationInfo>& info);

    void onKeyDown(wxKeyEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);
    void onPaint(wxPaintEvent& evt);
    void onMouse(wxMouseEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onScroll(wxScrollWinEvent& evt);
    void onTreeDataDropped(wxDragResult& drag_result, FsDataObject* data);
    void onDragOver(wxDragResult& drag_result);
    void onDragLeave();

    void onBoxClosing(RelationBox* box, bool* allow);

private:

    std::vector<RelationLine> m_orig_lines;
    std::vector<RelationLine> m_lines;
    std::vector<RelationBox*> m_boxes;

    wxPoint m_last_drag_pt;
    wxPen m_thick_black_pen;
    wxPen m_thin_black_pen;
    wxPen m_thick_red_pen;
    wxPen m_thin_red_pen;
    wxBitmap m_bmp;
    wxFont m_overlay_font;
    wxString m_overlay_text;
    wxMemoryDC m_memdc;

    RelationLine* m_selected_line;
    RelationBox* m_action_box;
    int m_action;
    int m_action_line;
    int m_action_x;
    int m_action_y;
    int m_action_offx;
    int m_action_offy;
    int m_rband_startx;
    int m_rband_starty;

    bool m_changed;
    bool m_first_time;

    DECLARE_EVENT_TABLE()
};


#endif  // __APP_RELATIONDIAGRAM_H




