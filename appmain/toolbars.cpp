/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2004-01-26
 *
 */


#include "appmain.h"
#include "toolbars.h"
#include "panelfind.h"
#include "dbdoc.h"


// this typedef is a workaround for the C2352 problem in VC++ 6.0
typedef kcl::BitmapComboPopup UrlComboPopupBase;

class UrlComboPopup : public kcl::BitmapComboPopup
{
    void Init()
    {
        UrlComboPopupBase::Init();
    }

    bool Create(wxWindow* parent)
    {
        return UrlComboPopupBase::Create(parent);
    }
    
    void SetStringValue(const wxString& s)
    {
#if !wxCHECK_VERSION(2,9,0)
        if (s.StartsWith(wxT("https")))
            m_combo->SetBackgroundColour(wxColour(255,255,190));
             else
            m_combo->SetBackgroundColour(wxColour(*wxWHITE));
#endif

        UrlComboPopupBase::SetStringValue(s);
    }
};



void initIdBitmapMap()
{
    BitmapMgr* mgr = BitmapMgr::getBitmapMgr();
    
    mgr->addIdBitmapMap(ID_Project_New, wxT("gf_blank_document_24"));
    mgr->addIdBitmapMap(ID_Project_OpenFile, wxT("gf_folder_open_24"));
    mgr->addIdBitmapMap(ID_File_Save, wxT("gf_save_24"));
    mgr->addIdBitmapMap(ID_View_ViewSwitcher, wxT("gf_switch_view_24"));
    mgr->addIdBitmapMap(ID_File_Back, wxT("gf_back_arrow_24"));
    mgr->addIdBitmapMap(ID_File_Forward, wxT("gf_forward_arrow_24"));
    mgr->addIdBitmapMap(ID_File_Reload, wxT("gf_reload_24"));
    mgr->addIdBitmapMap(ID_File_Stop, wxT("gf_stop_24"));
    mgr->addIdBitmapMap(ID_Project_Home, wxT("gf_home_24"));
    mgr->addIdBitmapMap(ID_File_Bookmark, wxT("gf_favorities_24"));
    mgr->addIdBitmapMap(ID_Data_QuickFilter, wxT("gf_quick_filter_24"));
    mgr->addIdBitmapMap(ID_Edit_FindNext, wxT("gf_find_next_24"));
    mgr->addIdBitmapMap(ID_Data_Filter, wxT("gf_filter_24"));
    mgr->addIdBitmapMap(ID_Data_CreateDynamicField, wxT("gf_lightning_24"));
    mgr->addIdBitmapMap(ID_Data_Sort, wxT("gf_sort_a_24"));
    mgr->addIdBitmapMap(ID_Data_GroupRecords, wxT("gf_sum_24"));
    mgr->addIdBitmapMap(ID_Project_Relationship, wxT("gf_related_field_24"));
    mgr->addIdBitmapMap(ID_App_ToggleRelationshipSync, wxT("gf_filter_related_16"));
}




// -- Standard Toolbar implementation --

BEGIN_EVENT_TABLE(StandardToolbar, wxAuiToolBar)
    EVT_KCLBITMAPCOMBO_BEGIN_DRAG(ID_Frame_UrlCtrl, StandardToolbar::onUrlBeginDrag)
END_EVENT_TABLE()


StandardToolbar::StandardToolbar(wxWindow* parent,
                                 wxWindowID id,
                                 const wxPoint& pos,
                                 const wxSize& size)
                                 : wxAuiToolBar(parent, id, pos, size,
                                           wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_NO_AUTORESIZE)
{
    SetToolPacking(0);
    
    // find combo box
    FindComboPopup* find_popup = new FindComboPopup;
    m_find = new FindComboControl(this, ID_Frame_FindCtrl, find_popup);
    m_find->setSingleClickSelect(true);
    m_find->setOverlayText(_("Find/Filter..."));
    
    // url combo box
    UrlComboPopup* url_popup = new UrlComboPopup;
    m_url = new kcl::BitmapComboControl(this, ID_Frame_UrlCtrl, url_popup);
    m_url->setDefaultBitmap(GETBMP(gf_blank_document_16));
    m_url->setSingleClickSelect(true);
    //m_url = new wxBitmapComboBox(this, ID_Frame_UrlCtrl);
    
    
    // use 24x24 pixel icons as our default
    setSmallIcons(false);

    SetGripperVisible(false);
    SetOverflowVisible(false);
}

static void AddSizedTool(wxAuiToolBar* toolbar, int id, bool small_icon)
{
    toolbar->AddTool(id, small_icon ? ID2BMP16(id) : ID2BMP(id), wxNullBitmap, false, 0);
}

static void AddSizedToggleTool(wxAuiToolBar* toolbar, int id, bool small_icon)
{
    toolbar->AddTool(id, small_icon ? ID2BMP16(id) : ID2BMP(id), wxNullBitmap, true, 0);
}

bool StandardToolbar::getSmallIcons()
{
    return m_small_icons;
}

void StandardToolbar::setSmallIcons(bool small_icons)
{
    m_small_icons = small_icons;
    int spacer = (small_icons ? 3 : 5);
    
    Clear();

    AddSizedTool(this, ID_File_Back, small_icons);
    AddSpacer(spacer);
    AddSizedTool(this, ID_File_Forward, small_icons);
    AddSpacer(spacer);
    AddSizedTool(this, ID_File_Reload, small_icons);
    AddSpacer(spacer);
    AddSizedTool(this, ID_File_Stop, small_icons);
    AddSpacer(spacer);
    AddSizedTool(this, ID_Project_Home, small_icons);
    AddSpacer(spacer);
    AddControl(m_url, _("Location"));
    AddSpacer(spacer);
    AddSizedTool(this, ID_File_Bookmark, small_icons);
    AddSeparator();
    AddSizedToggleTool(this, ID_Data_QuickFilter, small_icons);
    AddSpacer(spacer);
    AddControl(m_find, _("Find/Filter"));
    AddSpacer(spacer);
    AddSizedTool(this, ID_Edit_FindNext, small_icons);
    AddSeparator();
    AddSizedTool(this, ID_View_ViewSwitcher, small_icons);

    SetToolProportion(ID_Frame_UrlCtrl, 2);
    SetToolProportion(ID_Frame_FindCtrl, 1);

    SetToolDropDown(ID_Project_New, true);
    //SetToolDropDown(ID_File_Bookmark, true);
    SetToolDropDown(ID_View_ViewSwitcher, true);
    
    SetToolLabel(ID_File_Back, _("Back"));
    SetToolLabel(ID_File_Forward, _("Forward"));
    SetToolLabel(ID_File_Reload, _("Refresh"));
    SetToolLabel(ID_File_Stop, _("Stop"));
    SetToolLabel(ID_Project_Home, _("Home"));
    SetToolLabel(ID_File_Bookmark, _("Bookmark"));
    SetToolLabel(ID_Data_QuickFilter, _("Filter"));
    SetToolLabel(ID_Edit_FindNext, _("Find Next"));
    SetToolLabel(ID_View_ViewSwitcher, _("View"));

    SetToolShortHelp(ID_File_Back, _("Go Back"));
    SetToolShortHelp(ID_File_Forward, _("Go Forward"));
    SetToolShortHelp(ID_File_Reload, _("Refresh"));
    SetToolShortHelp(ID_File_Stop, _("Stop"));
    SetToolShortHelp(ID_Project_Home, _("Go Home"));
    SetToolShortHelp(ID_File_Bookmark, _("Bookmark This Location"));
    SetToolShortHelp(ID_Data_QuickFilter, _("Quick Filter"));
    SetToolShortHelp(ID_Edit_FindNext, _("Find Next"));
    SetToolShortHelp(ID_View_ViewSwitcher, _("Switch Document View"));
    
    Realize();
}

void StandardToolbar::onUrlBeginDrag(kcl::BitmapComboEvent& evt)
{
    IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
    if (doc_site.isNull())
        return;
    
    wxString path = m_url->GetValue();
    wxString title = doc_site->getCaption();
    
    DbObjectFsItem* item_raw = new DbObjectFsItem;
    item_raw->setLabel(title);
    item_raw->setPath(path);
    item_raw->setBitmap(m_url->getBitmap(), fsbmpSmall);
    IDbObjectFsItemPtr item = static_cast<IDbObjectFsItem*>(item_raw);
    
    xcm::IVectorImpl<IFsItemPtr>* items = new xcm::IVectorImpl<IFsItemPtr>;
    items->append(item);

    FsDataObject data;
    data.setSourceId(ID_Frame_UrlCtrl);
    data.setFsItems(items);
    wxDropSource dragSource(data, this);
    wxDragResult result = dragSource.DoDragDrop(TRUE);
}




// -- FontComboControl class implementation --

FontComboControl::FontComboControl(wxWindow* parent, wxWindowID id)
{
    wxArrayString font_facenames = g_app->getFontNames();
    
    // add the font facenames to the array we'll use to populate this control
    size_t i, count = font_facenames.GetCount();
    wxString* choices = new wxString[count];
    for (i = 0; i < count; ++i)
        choices[i] = font_facenames.Item(i);
    
    // NOTE: this show/hide code is here because it was taking a VERY long
    //       time to create this control -- this is due to the fact that the
    //       control was updating its visible height every time an item was
    //       appended to it
    #ifdef __WXMSW__
    Show(false);
    Freeze();
    #endif

    wxComboBox::Create(parent,
                       id,
                       wxEmptyString,
                       wxDefaultPosition,
                       wxSize(180,-1),
                       count,
                       choices,
                       wxCB_READONLY);
    
    wxString font_name = getAppPrefsDefaultString(wxT("grid.font.face_name"));
    SetStringSelection(font_name);

    #ifdef __WXMSW__
    Thaw();
    Show(true);
    #endif

    delete[] choices;
}




// -- FontSizeComboControl class implementation --

FontSizeComboControl::FontSizeComboControl(wxWindow* parent,
                                           wxWindowID id)
{
    wxComboBox::Create(parent,
                       id,
                       wxEmptyString,
                       wxDefaultPosition,
                       wxSize(50,-1));

    // append the default font sizes to the combobox
    Append(wxT("8"));
    Append(wxT("9"));
    Append(wxT("10"));
    Append(wxT("11"));
    Append(wxT("12"));
    Append(wxT("14"));
    Append(wxT("16"));
    Append(wxT("18"));
    Append(wxT("20"));
    Append(wxT("22"));
    Append(wxT("24"));
    Append(wxT("26"));
    Append(wxT("36"));
    Append(wxT("48"));
    Append(wxT("72"));
    SetStringSelection(wxT("12"));
}




// -- ZoomComboControl class implementation --

class ZoomComboControl : public wxComboBox
{
public:

    ZoomComboControl(wxWindow* parent, wxWindowID id = wxID_ANY)
    {
        wxComboBox::Create(parent,
                           id,
                           wxEmptyString,
                           wxDefaultPosition,
                           wxDefaultSize,
                           0,
                           NULL);

        // append the default zoom values to the combobox
        Append(wxT("500%"));
        Append(wxT("200%"));
        Append(wxT("175%"));
        Append(wxT("150%"));
        Append(wxT("125%"));
        Append(wxT("100%"));
        Append(wxT("75%"));
        Append(wxT("50%"));
        Append(wxT("25%"));
        SetStringSelection(wxT("100%"));
    }
};




// this art provider is exactly the same as wxAuiDefaultToolbarArt,
// with the exception that it draw a border line at the top to
// separate it from the StandardToolbar (copied from linkbar.cpp)

class FormatToolbarArt : public wxAuiDefaultToolBarArt
{
public:

    void DrawBackground(wxDC& dc,
                        wxWindow* wnd,
                        const wxRect& rect)
    {
        wxColor start_color = kcl::stepColor(kcl::getBaseColor(), 150);
        wxColor end_color = kcl::stepColor(kcl::getBaseColor(), 90);
        dc.GradientFillLinear(rect, start_color, end_color, wxSOUTH);
        dc.SetPen(kcl::getBorderPen());
        dc.DrawLine(rect.x, rect.y, rect.x+rect.width, rect.y);
    }
};




// -- Format Toolbar implementation --

BEGIN_EVENT_TABLE(FormatToolbar, wxAuiToolBar)
    EVT_MENU(ID_Format_AlignTextLeft, FormatToolbar::onHorizontalAlign)
    EVT_MENU(ID_Format_AlignTextCenter, FormatToolbar::onHorizontalAlign)
    EVT_MENU(ID_Format_AlignTextRight, FormatToolbar::onHorizontalAlign)
    EVT_MENU(ID_Format_AlignTop, FormatToolbar::onVerticalAlign)
    EVT_MENU(ID_Format_AlignMiddle, FormatToolbar::onVerticalAlign)
    EVT_MENU(ID_Format_AlignBottom, FormatToolbar::onVerticalAlign)
END_EVENT_TABLE()


FormatToolbar::FormatToolbar(wxWindow* parent,
                             wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size)
                             : wxAuiToolBar(parent, id, pos, size)
{
    SetArtProvider(new FormatToolbarArt);
    SetToolPacking(0);

    // zoom combo box
    m_zoom_combo = new ZoomComboControl(this, ID_View_ZoomCombo);
    
    // font combo box
    m_fontface_combo = new FontComboControl(this, ID_Format_FontFace_Combo);

    // we need to allow an empty string in this combobox because it
    // is read only and when there are multiple fonts selected, we
    // want to show an empty string in this control
    m_fontface_combo->Append(wxEmptyString);

    // font size combo box
    m_fontsize_combo = new FontSizeComboControl(this, ID_Format_FontSize_Combo);
    
    setEmbedded(false);
}

void FormatToolbar::setEmbedded(bool embedded)
{
    Clear();
    
    int spacer;
    
    if (embedded)
    {
        spacer = 2;
        SetMargins(5,2,1,0);
        m_fontface_combo->SetMinSize(wxSize(160,-1));
    }
     else
    {
        spacer = 3;
        SetMargins(5,2,3,2);
        m_fontface_combo->SetMinSize(wxSize(180,-1));
    }
    
    AddTool(ID_Format_Settings, GETBMP(gf_settings_16), wxNullBitmap, false, 0);
    AddSpacer(spacer);
    AddSeparator();
    AddSpacer(spacer);
    AddTool(ID_Edit_Undo, GETBMP(gf_undo_16), wxNullBitmap, false, 0);
    AddSpacer(spacer);
    AddTool(ID_Edit_Redo, GETBMP(gf_redo_16), wxNullBitmap, false, 0);
    AddSpacer(spacer);
    AddSeparator();
    AddSpacer(3);
    AddControl(m_zoom_combo);
    AddSpacer(4);
    AddTool(ID_View_ZoomOut, GETBMP(gf_zoom_out_16), wxNullBitmap, false, 0);
    AddSpacer(spacer);
    AddTool(ID_View_ZoomIn, GETBMP(gf_zoom_in_16), wxNullBitmap, false, 0);
    AddSpacer(spacer);
    AddSeparator();
    AddSpacer(3);
    AddControl(m_fontface_combo);
    AddSpacer(4);
    AddControl(m_fontsize_combo);
    AddSpacer(3);
    AddSeparator();
    AddSpacer(spacer);
    AddTool(ID_Format_ToggleFontWeight, GETBMP(gf_bold_16), wxNullBitmap, true, 0);
    AddSpacer(spacer);
    AddTool(ID_Format_ToggleFontStyle, GETBMP(gf_italic_16), wxNullBitmap, true, 0);
    AddSpacer(spacer);
    AddTool(ID_Format_ToggleFontUnderline, GETBMP(gf_underlined_16), wxNullBitmap, true, 0);
    AddSpacer(spacer);
    AddSeparator();
    AddSpacer(spacer);
    AddTool(ID_Format_AlignTextLeft, GETBMP(gf_text_align_l_16), wxNullBitmap, true, 0);
    AddSpacer(spacer);
    AddTool(ID_Format_AlignTextCenter, GETBMP(gf_text_align_c_16), wxNullBitmap, true, 0);
    AddSpacer(spacer);
    AddTool(ID_Format_AlignTextRight, GETBMP(gf_text_align_r_16), wxNullBitmap, true, 0);
    AddSpacer(spacer);
    AddSeparator();
    AddSpacer(spacer);

    // if (!embedded)
    {
        AddTool(ID_Format_AlignTop, GETBMP(gf_align_top_16), wxNullBitmap, true, 0);
        AddSpacer(spacer);
        AddTool(ID_Format_AlignMiddle, GETBMP(gf_align_middle_16), wxNullBitmap, true, 0);
        AddSpacer(spacer);
        AddTool(ID_Format_AlignBottom, GETBMP(gf_align_bottom_16), wxNullBitmap, true, 0);
        AddSpacer(spacer);
        AddSeparator();
        AddSpacer(spacer);
    }

    AddTool(ID_Format_Border, GETBMP(gf_border_bottom_16), wxNullBitmap, false, 0);
    AddSeparator();    
    AddTool(ID_Format_TextColor, GETBMP(gf_text_color_16), wxNullBitmap, false, 0);
    AddSpacer(spacer);
    AddTool(ID_Format_FillColor, GETBMP(gf_fill_color_16), wxNullBitmap, false, 0);
    AddSpacer(spacer);
    AddSeparator();
    AddSpacer(spacer);
    AddTool(ID_Format_ToggleMergeCells, GETBMP(gf_span_col_16), wxNullBitmap, true, 0);
    AddSpacer(spacer);
    AddTool(ID_Format_ToggleWrapText, GETBMP(gf_text_wrap_16), wxNullBitmap, true, 0);
    
    SetToolShortHelp(ID_Format_Settings, _("Report Settings"));
    SetToolShortHelp(ID_Edit_Undo, _("Undo"));
    SetToolShortHelp(ID_Edit_Redo, _("Redo"));
    SetToolShortHelp(ID_View_ZoomOut, _("Zoom Out"));
    SetToolShortHelp(ID_View_ZoomIn, _("Zoom In"));
    SetToolShortHelp(ID_Format_ToggleFontWeight, _("Bold"));
    SetToolShortHelp(ID_Format_ToggleFontStyle, _("Italic"));
    SetToolShortHelp(ID_Format_ToggleFontUnderline, _("Underline"));
    SetToolShortHelp(ID_Format_AlignTextLeft, _("Align Left"));
    SetToolShortHelp(ID_Format_AlignTextCenter, _("Align Center"));
    SetToolShortHelp(ID_Format_AlignTextRight, _("Align Right"));
    SetToolShortHelp(ID_Format_AlignTop, _("Align Top"));
    SetToolShortHelp(ID_Format_AlignMiddle, _("Align Middle"));
    SetToolShortHelp(ID_Format_AlignBottom, _("Align Bottom"));
    SetToolShortHelp(ID_Format_ToggleWrapText, _("Wrap Text"));
    SetToolShortHelp(ID_Format_ToggleMergeCells, _("Merge Cells"));
    SetToolShortHelp(ID_Format_Border, _("Border Style"));    
    SetToolShortHelp(ID_Format_TextColor, _("Text Color"));
    SetToolShortHelp(ID_Format_FillColor, _("Fill Color"));
    
    SetToolDropDown(ID_Format_Border, true);    
    SetToolDropDown(ID_Format_FillColor, true);
    SetToolDropDown(ID_Format_TextColor, true);

    // initialize the user data for these toolbar items
    m_text_color = wxColor(255,0,0);
    m_fill_color = wxColor(255,255,178);
    
    wxAuiToolBarItem* item;
    item = FindTool(ID_Format_TextColor);
    item->SetUserData((long)&m_text_color);
    item = FindTool(ID_Format_FillColor);
    item->SetUserData((long)&m_fill_color);
    
    SetGripperVisible(false);
    SetOverflowVisible(false);

    Realize();
}

void FormatToolbar::OnCustomRender(wxDC& dc,
                                   const wxAuiToolBarItem& item,
                                   const wxRect& rect)
{
    if (item.GetId() == ID_Format_Border)
    {
        // TODO: fill out
    }

    if (item.GetId() == ID_Format_FillColor ||
        item.GetId() == ID_Format_LineColor ||
        item.GetId() == ID_Format_TextColor)
    {
        // get the fill/line/text color from the item's user data
        wxColor* user_data_color = (wxColor*)(item.GetUserData());
        wxColor c = *user_data_color;
        
        // if the item is disabled, lighten the color
        if (item.GetState() & wxAUI_BUTTON_STATE_DISABLED)
            c = kcl::stepColor(c, 160);

        // draw the color over the item's bitmap
        // (in the color area of the bitmap)
        if (c == wxNullColour)
        {
            dc.SetPen(wxColour(128,128,128));
            dc.SetBrush(wxColour(204,204,204));
        }
         else
        {
            dc.SetPen(c);
            dc.SetBrush(c);
        }
        dc.DrawRectangle(rect.x+5,rect.y+16,15,3);
    }
}

void FormatToolbar::onHorizontalAlign(wxCommandEvent& evt)
{
    int id = evt.GetId();
    ToggleTool(ID_Format_AlignTextLeft,   (id == ID_Format_AlignTextLeft)   ? true : false);
    ToggleTool(ID_Format_AlignTextCenter, (id == ID_Format_AlignTextCenter) ? true : false);
    ToggleTool(ID_Format_AlignTextRight,  (id == ID_Format_AlignTextRight)  ? true : false);
    Refresh();
    
    evt.Skip();
}

void FormatToolbar::onVerticalAlign(wxCommandEvent& evt)
{
    int id = evt.GetId();
    ToggleTool(ID_Format_AlignTop,    (id == ID_Format_AlignTop)    ? true : false);
    ToggleTool(ID_Format_AlignMiddle, (id == ID_Format_AlignMiddle) ? true : false);
    ToggleTool(ID_Format_AlignBottom, (id == ID_Format_AlignBottom) ? true : false);
    Refresh();
    
    evt.Skip();
}

