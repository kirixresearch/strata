/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2004-01-26
 *
 */


#ifndef H_APP_TOOLBARS_H
#define H_APP_TOOLBARS_H


const int ID_FirstToolbarId       = 5841;
const int ID_FirstAppCommandId    = 20000;
const int ID_FirstScriptCommandId = 25000;
const int ID_FirstScriptControlId = 27000;
const int ID_FirstLinkBarId       = 29000;


// these structures help populate the view switcher dropdown list

struct ViewSwitcherListItem
{
    int id;
    wxString label;
    bool checked;
};

struct ViewSwitcherList : public wxObject
{
    void addItem(int id, const wxString& label, bool checked = false)
    {
        ViewSwitcherListItem item;
        item.id = id;
        item.label = label;
        item.checked = checked;
        items.push_back(item);
    }
    
    void getItemInfo(size_t idx, int* id, wxString* label, bool* checked)
    {
        if (idx < 0 || idx >= items.size())
            return;

        *id = items[idx].id;
        *label = items[idx].label;
        *checked = items[idx].checked;
    }
    
    size_t getCount() { return items.size(); }
    
    std::vector<ViewSwitcherListItem> items;
};




void initIdBitmapMap();




class FindComboControl;
class StandardToolbar : public wxAuiToolBar
{
public:
    
    StandardToolbar(wxWindow* parent,
                    wxWindowID id = ID_Toolbar_Standard,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize);

    void setSmallIcons(bool small_icons = true);
    bool getSmallIcons();
    
    FindComboControl* getFindCombo() const { return m_find; }
    
private:

    void onUrlBeginDrag(kcl::BitmapComboEvent& evt);

public:

    kcl::BitmapComboControl* m_url;
    FindComboControl* m_find;
    bool m_small_icons;
    
    DECLARE_EVENT_TABLE()
};




// -- FontComboControl class implementations --

class FontComboControl : public wxComboBox
{
public:

    FontComboControl(wxWindow* parent,
                     wxWindowID id = wxID_ANY);
};


// -- FontSizeComboControl class implementations --

class FontSizeComboControl : public wxComboBox
{
public:

    FontSizeComboControl(wxWindow* parent,
                         wxWindowID id = wxID_ANY);
};




class FormatToolbar : public wxAuiToolBar,
                      public xcm::signal_sink
{
public:

    FormatToolbar(wxWindow* parent,
                  wxWindowID id = ID_Toolbar_Format,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize);

    void setEmbedded(bool embedded = true);

private:
    
    void OnCustomRender(wxDC& dc,
                        const wxAuiToolBarItem& item,
                        const wxRect& rect);

private:
    
    void onHorizontalAlign(wxCommandEvent& evt);
    void onVerticalAlign(wxCommandEvent& evt);

public:

    FontComboControl* m_fontface_combo;
    FontSizeComboControl* m_fontsize_combo;
    wxComboBox* m_zoom_combo;
    
    wxColor m_text_color;
    wxColor m_fill_color;
    
    DECLARE_EVENT_TABLE()
};


#endif

