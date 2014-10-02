/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2004-10-11
 *
 */


#ifndef __APP_FIELDLISTCONTROL_H
#define __APP_FIELDLISTCONTROL_H


class FieldListItem
{
public:

    FieldListItem()
    {
        name = wxEmptyString;
        bitmap = wxNullBitmap;
        type = xd::typeUndefined;
        width = 0;
        scale = 0;
        enabled = true;
    }

    FieldListItem(const FieldListItem& c)
    {
        name = c.name;
        bitmap = c.bitmap;
        type = c.type;
        width = c.width;
        scale = c.scale;
        enabled = c.enabled;
    }

    FieldListItem& operator=(const FieldListItem& c)
    {
        name = c.name;
        bitmap = c.bitmap;
        type = c.type;
        width = c.width;
        scale = c.scale;
        enabled = c.enabled;
        return *this;
    }

public:
    
    wxString name;
    wxBitmap bitmap;
    int type;
    int width;
    int scale;
    bool enabled;
};


class FieldListControl : public wxPanel,
                         public xcm::signal_sink
{

public:

    enum
    {
        SortNone = 0,
        SortAscending = 1,
        SortDescending = 2
    };
    
    enum
    {
        ColNameIdx = 0,
        ColTypeIdx = 1,
        ColWidthIdx = 2,
        ColScaleIdx = 3
    };
    
    enum
    {
        ColName = 0x01,
        ColType = 0x02,
        ColWidth = 0x04,
        ColScale = 0x08,
        ColAll = ColName | ColType | ColWidth | ColScale
    };
    
public:

    FieldListControl(wxWindow* window,
                     wxWindowID id = -1,
                     const wxPoint& position = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxCLIP_CHILDREN |
                                  wxNO_FULL_REPAINT_ON_RESIZE |
                                  kcl::DEFAULT_BORDER);
    ~FieldListControl();
    
    void refresh();
    void showColumns(unsigned int col_mask = ColAll,
                     bool refresh = true,
                     bool proportional = true);
    void setColumnCaption(int col, const wxString& new_value);
    
    void clearSelection();
    void sortList(int sort_state);
    void setDragFormat(const wxString& format);
    void setStructure(const xd::Structure& structure);
    void setItemEnabled(const wxString& item_name, bool enabled = true);
    void addCustomItem(const wxString& text,
                       const wxBitmap& bitmap = wxNullBitmap,
                       int type = xd::typeCharacter,
                       int width = 20,
                       int scale = 0);
    
    std::vector<FieldListItem> getSelectedItems();
    std::vector<FieldListItem> getAllItems();

    // this window does not accept focus, and should
    // not be included in tab traversal lists
    bool AcceptsFocus() const { return false; }
    
public: // signals

    xcm::signal2<int /* row */, const wxString& /* field_name */> sigFieldDblClicked;

private:

    void populate();
    void onGridCellDblClick(kcl::GridEvent& evt);
    void onGridColumnClicked(kcl::GridEvent& evt);
    void onSize(wxSizeEvent& evt);

private:

    int m_sort_state;
    kcl::RowSelectionGrid* m_grid;
    xd::Structure m_structure;
    std::vector<FieldListItem> m_custom_items;
    std::vector<FieldListItem*> m_to_delete;

    DECLARE_EVENT_TABLE()
};


#endif





