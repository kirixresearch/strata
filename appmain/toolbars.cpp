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
#include <artprovider.h>   // angle brackets to allow overriding


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
        if (s.StartsWith("https"))
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
    
    mgr->addIdBitmapMap(ID_Project_New, "gf_blank_document");
    mgr->addIdBitmapMap(ID_Project_OpenFile, "gf_folder_open");
    mgr->addIdBitmapMap(ID_File_Save, "gf_save");
    mgr->addIdBitmapMap(ID_View_ViewSwitcher, "gf_switch_view");
    mgr->addIdBitmapMap(ID_File_Back, "gf_back_arrow");
    mgr->addIdBitmapMap(ID_File_Forward, "gf_forward_arrow");
    mgr->addIdBitmapMap(ID_File_Reload, "gf_reload");
    mgr->addIdBitmapMap(ID_File_Stop, "gf_stop");
    mgr->addIdBitmapMap(ID_Project_Home, "gf_home");
    mgr->addIdBitmapMap(ID_File_Bookmark, "gf_favorities");
    mgr->addIdBitmapMap(ID_Data_QuickFilter, "gf_quick_filter");
    mgr->addIdBitmapMap(ID_Edit_FindNext, "gf_find_next");
    mgr->addIdBitmapMap(ID_Data_Filter, "gf_filter");
    mgr->addIdBitmapMap(ID_Data_CreateDynamicField, "gf_lightning");
    mgr->addIdBitmapMap(ID_Data_Sort, "gf_sort_a");
    mgr->addIdBitmapMap(ID_Data_GroupRecords, "gf_sum");
    mgr->addIdBitmapMap(ID_Project_Relationship, "gf_related_field");
    mgr->addIdBitmapMap(ID_App_ToggleRelationshipSync, "gf_filter_related");
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
    m_url->setDefaultBitmap(GETBMPSMALL(gf_blank_document));
    m_url->setSingleClickSelect(true);
    //m_url = new wxBitmapComboBox(this, ID_Frame_UrlCtrl);
    
    SetArtProvider(new ToolbarArt);
    SetToolBorderPadding(FromDIP(2));

    // use 24x24 pixel icons as our default
    setSmallIcons(false);

    SetGripperVisible(false);
    SetOverflowVisible(false);
}

static void AddSizedTool(wxAuiToolBar* toolbar, int id, bool small_icon)
{
    wxBitmap bitmap = small_icon ? ID2BMPSMALL(id) : ID2BMPMEDIUM(id);
    toolbar->AddTool(id, bitmap, wxNullBitmap, false, 0);
}

static void AddSizedToggleTool(wxAuiToolBar* toolbar, int id, bool small_icon)
{
    wxBitmap bitmap = small_icon ? ID2BMPSMALL(id) : ID2BMPMEDIUM(id);
    toolbar->AddTool(id, bitmap, wxNullBitmap, true, 0);
}

bool StandardToolbar::getSmallIcons()
{
    return m_small_icons;
}

void StandardToolbar::setSmallIcons(bool small_icons)
{
    m_small_icons = small_icons;
    int spacer = (small_icons ? FromDIP(3) : FromDIP(5));
    
    Clear();

#if APP_NEW_TOOLBARS == 1

    AddSizedTool(this, ID_Project_New, small_icons);
    AddSizedTool(this, ID_Project_OpenFile, small_icons);
    AddSizedTool(this, ID_File_Save, small_icons);

    AddSeparator();

    AddSizedTool(this, ID_Data_Sort, small_icons);
    AddSizedTool(this, ID_Data_Filter, small_icons);
    AddSizedTool(this, ID_Data_CreateDynamicField, small_icons);
    AddSizedTool(this, ID_Data_GroupRecords, small_icons);
    AddSeparator();
    AddSizedTool(this, ID_Project_Relationship, small_icons);
    AddSizedTool(this, ID_App_ToggleRelationshipSync, small_icons);

    AddSeparator();

    AddSizedTool(this, ID_File_Back, small_icons);
    AddSpacer(spacer);
    AddSizedTool(this, ID_File_Forward, small_icons);
    AddSpacer(spacer);
    AddSizedTool(this, ID_File_Reload, small_icons);
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

    SetToolLabel(ID_Project_New, _("New"));
    SetToolLabel(ID_Project_OpenFile, _("Open"));
    SetToolLabel(ID_File_Save, _("Save"));
    SetToolLabel(ID_Data_Sort, _("Sort"));
    SetToolLabel(ID_Data_Filter, _("Filter"));
    SetToolLabel(ID_Data_CreateDynamicField, _("Calculation"));
    SetToolLabel(ID_Data_GroupRecords, _("Group"));
    SetToolLabel(ID_Project_Relationship, _("Relationships"));
    SetToolLabel(ID_App_ToggleRelationshipSync, _("Sync"));

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

#else

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
 #endif

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
    
    wxString font_name = getAppPrefsDefaultString("grid.font.face_name");
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
                       FromDIP(wxSize(50,-1)));

    // append the default font sizes to the combobox
    Append("8");
    Append("9");
    Append("10");
    Append("11");
    Append("12");
    Append("14");
    Append("16");
    Append("18");
    Append("20");
    Append("22");
    Append("24");
    Append("26");
    Append("36");
    Append("48");
    Append("72");
    SetStringSelection("12");
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
        Append("500%");
        Append("200%");
        Append("175%");
        Append("150%");
        Append("125%");
        Append("100%");
        Append("75%");
        Append("50%");
        Append("25%");
        SetStringSelection("100%");
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
    SetToolBorderPadding(FromDIP(2));

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
    item->SetUserData((long)m_text_color.GetRGB());
    item = FindTool(ID_Format_FillColor);
    item->SetUserData((long)m_fill_color.GetRGB());
    
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
        wxColor user_data_color(item.GetUserData());
        wxColor c = user_data_color;
        
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

