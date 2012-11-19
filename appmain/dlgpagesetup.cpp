/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams; David Z. Williams
 * Created:  2002-10-08
 *
 */


#include "appmain.h"
#include "dlgpagesetup.h"
#include <wx/paper.h>

enum
{
    ID_Portrait_Radio = wxID_HIGHEST + 1,
    ID_Landscape_Radio,
    ID_PageSize_Choice,
    ID_PageWidth_TextCtrl,
    ID_PageHeight_TextCtrl,
    ID_LeftMargin_TextCtrl,
    ID_RightMargin_TextCtrl,
    ID_TopMargin_TextCtrl,
    ID_BottomMargin_TextCtrl
};


// -- OrientationSizer class implementation --

OrientationPanel::OrientationPanel(wxWindow* parent)
                        : wxPanel(parent,
                                  wxID_ANY,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxNO_FULL_REPAINT_ON_RESIZE |
                                  wxCLIP_CHILDREN |
                                  wxTAB_TRAVERSAL)
{
    // create portrait static bitmap
    wxBitmap portrait = GETBMP(gf_blank_document_32);
    wxStaticBitmap* portrait_bitmap;
    portrait_bitmap = new wxStaticBitmap(this, -1, portrait);
    
    // create landscape static bitmap
    wxBitmap landscape = GETBMP(gf_blank_document_landscape_32);
    wxStaticBitmap* landscape_bitmap;
    landscape_bitmap = new wxStaticBitmap(this, -1, landscape);
    
    // create controls
    m_portrait_radio = new wxRadioButton(this,
                                         ID_Portrait_Radio,
                                         _("Portrait"),
                                         wxDefaultPosition,
                                         wxDefaultSize,
                                         wxRB_GROUP);
    m_landscape_radio = new wxRadioButton(this,
                                         ID_Landscape_Radio,
                                         _("Landscape"));
    m_portrait_radio->SetValue(true);
    
    // create sizer
    wxStaticBox* box = new wxStaticBox(this, -1, _("Orientation"));
    wxStaticBoxSizer* main_sizer = new wxStaticBoxSizer(box, wxHORIZONTAL);
    main_sizer->AddSpacer(10);
    main_sizer->Add(portrait_bitmap, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_portrait_radio, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    main_sizer->AddSpacer(20);
    main_sizer->Add(landscape_bitmap, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_landscape_radio, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    main_sizer->AddSpacer(10);
    SetSizer(main_sizer);
    Layout();
}

void OrientationPanel::setOrientation(int orientation)
{
    if (orientation == wxLANDSCAPE)
        m_landscape_radio->SetValue(true);
         else
        m_portrait_radio->SetValue(true);

    sigOrientationChanged(orientation);
}

int OrientationPanel::getOrientation()
{
    return (m_portrait_radio->GetValue()) ? wxPORTRAIT
                                          : wxLANDSCAPE;
}

void OrientationPanel::onOrientationChanged(wxCommandEvent& evt)
{
    sigOrientationChanged(getOrientation());
}

void OrientationPanel::onPageDimensionsChanged(double width, double height)
{
    if (width > height)
        m_landscape_radio->SetValue(true);
         else
        m_portrait_radio->SetValue(true);
}

BEGIN_EVENT_TABLE(OrientationPanel, wxPanel)
    EVT_RADIOBUTTON(ID_Portrait_Radio, OrientationPanel::onOrientationChanged)
    EVT_RADIOBUTTON(ID_Landscape_Radio, OrientationPanel::onOrientationChanged)
END_EVENT_TABLE()




// utility functions

static double in2mm(double in) { return (in*25.4); }
static double mm2in(double mm) { return (mm/25.4); }


// -- PageSizePanel class implementation --

PageSizePanel::PageSizePanel(wxWindow* parent)
                        : wxPanel(parent,
                                  wxID_ANY,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxNO_FULL_REPAINT_ON_RESIZE |
                                  wxCLIP_CHILDREN |
                                  wxTAB_TRAVERSAL)
{
    // create paper size choice control
    m_pagesize_choice = new wxChoice(this, ID_PageSize_Choice);
    m_custom_name = _("Custom Size");
    
    // populate the choice control from the paper database
    wxPrintPaperDatabase* paper_db = g_app->getPaperDatabase();
    size_t i, count = paper_db->GetCount();
    for (i = 0; i < count; ++i)
    {
        wxPrintPaperType* paper = paper_db->Item(i);
        if (!paper)
            continue;
        
        // map the paper size to the index in the paper size choice control
        m_papersizes.push_back(paper->GetId());
        
        // add the name to the paper size choice control
        m_pagesize_choice->Append(paper->GetName());
        
        // use 'Letter' as our default size
        if (paper->GetId() == wxPAPER_LETTER)
            m_pagesize_choice->SetStringSelection(paper->GetName());
    }
    
    // add 'Custom Size' item to the paper dropdown
    m_pagesize_choice->Append(m_custom_name);
    m_papersizes.push_back(wxPAPER_NONE);
    
    // create page dimensions sizer
    wxStaticText* label_pagewidth = new wxStaticText(this, -1, _("W:"));
    m_pagewidth_textctrl = new wxTextCtrl(this,
                                          ID_PageWidth_TextCtrl,
                                          wxT("8.50"),
                                          wxDefaultPosition,
                                          wxSize(45,-1));

    wxStaticText* label_pageheight = new wxStaticText(this, -1, _("H:"));
    m_pageheight_textctrl = new wxTextCtrl(this,
                                           ID_PageHeight_TextCtrl,
                                           wxT("11.00"),
                                           wxDefaultPosition,
                                           wxSize(45,-1));

    wxBoxSizer* dimensions_sizer = new wxBoxSizer(wxHORIZONTAL);
    dimensions_sizer->Add(label_pagewidth, 0, wxALIGN_CENTER);
    dimensions_sizer->AddSpacer(5);
    dimensions_sizer->Add(m_pagewidth_textctrl, 1, wxALIGN_CENTER);
    dimensions_sizer->AddSpacer(10);
    dimensions_sizer->Add(label_pageheight, 0, wxALIGN_CENTER);
    dimensions_sizer->AddSpacer(5);
    dimensions_sizer->Add(m_pageheight_textctrl, 1, wxALIGN_CENTER);
    
    // create main sizer
    wxStaticBox* box = new wxStaticBox(this, -1, _("Paper Size (in inches)"));
    wxBoxSizer* main_sizer = new wxStaticBoxSizer(box, wxVERTICAL);
    main_sizer->AddSpacer(10);
    main_sizer->Add(m_pagesize_choice, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    main_sizer->AddSpacer(8);
    main_sizer->Add(dimensions_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    main_sizer->AddSpacer(10);
    SetSizer(main_sizer);
    Layout();
    
    // make sure we process the kill focus events for these text controls
    m_pagewidth_textctrl->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(PageSizePanel::onKillFocus), NULL, this);
    m_pageheight_textctrl->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(PageSizePanel::onKillFocus), NULL, this);
    
    // initialize page orientation
    m_orientation = wxPORTRAIT;
}

void PageSizePanel::updatePaperSizeChoice()
{
    double in_w, in_h, mm_w, mm_h;
    getPageDimensions(&in_w, &in_h);
    
    mm_w = kl::dblround(in2mm(in_w), 1);
    mm_h = kl::dblround(in2mm(in_h), 1);
    
    // wxPrintPaperType stores paper sizes in tenths of a millimeter
    mm_w *= 10;
    mm_h *= 10;
    
    // make sure we're aware of our orientation
    if (m_orientation == wxLANDSCAPE)
    {
        double temp = mm_w;
        mm_w = mm_h;
        mm_h = temp;
    }
    
    wxSize s(mm_w, mm_h);
    wxPrintPaperType* paper = g_app->getPaperDatabase()->FindPaperType(s);
    if (!paper)
    {
        m_pagesize_choice->SetStringSelection(m_custom_name);
    }
     else
    {
        int idx = 0;
        
        std::vector<wxPaperSize>::iterator it;
        for (it = m_papersizes.begin(); it != m_papersizes.end(); ++it)
        {
            if (*it == paper->GetId())
            {
                m_pagesize_choice->SetSelection(idx);
                break;
            }
               
            idx++;
        }
    }
}

void PageSizePanel::setPageDimensions(double width, double height)
{
    wxString val = wxString::Format(wxT("%.2f"), width);
    m_pagewidth_textctrl->SetValue(val);

    val = wxString::Format(wxT("%.2f"), height);
    m_pageheight_textctrl->SetValue(val);
    
    // set page orientation
    if (width > height)
        m_orientation = wxLANDSCAPE;
         else
        m_orientation = wxPORTRAIT;

    // update the paper size choice control
    updatePaperSizeChoice();
    
    // fire dimension changed signal
    sigDimensionsChanged(width, height);
}

void PageSizePanel::getPageDimensions(double* width, double* height)
{
    if (!width || !height)
        return;
    
    *width = ::wxAtof(m_pagewidth_textctrl->GetValue());
    *height = ::wxAtof(m_pageheight_textctrl->GetValue());
}

wxPaperSize PageSizePanel::getPaperSize()
{
    int idx = m_pagesize_choice->GetSelection();
    if (idx >= 0 && idx < (int)m_papersizes.size())
        return m_papersizes[idx];
    
    return wxPAPER_NONE;
}

void PageSizePanel::onKillFocus(wxFocusEvent& evt)
{
    double width, height;
    getPageDimensions(&width, &height);
    
    // set page orientation
    if (width > height)
        m_orientation = wxLANDSCAPE;
         else
        m_orientation = wxPORTRAIT;
    
    // update the paper size choice control
    updatePaperSizeChoice();
    
    // fire dimension changed signal
    sigDimensionsChanged(width, height);
    
    evt.Skip();
}

void PageSizePanel::onPaperSizeChanged(wxCommandEvent& evt)
{
    wxPaperSize id = getPaperSize();
    wxPrintPaperType* paper = g_app->getPaperDatabase()->FindPaperType(id);
    
    // 'Custom' was selected from the choice control
    if (!paper || id == wxPAPER_NONE)
        return;
    
    wxSize s = paper->GetSize();
    double in_w = mm2in(s.GetWidth());
    double in_h = mm2in(s.GetHeight());
    
    // wxPrintPaperType stores paper sizes in tenths of a millimeter
    in_w /= 10;
    in_h /= 10;
    
    // respect the page orientation
    if (m_orientation == wxLANDSCAPE)
    {
        double temp = in_w;
        in_w = in_h;
        in_h = temp;
    }
    
    wxString val = wxString::Format(wxT("%.2f"), in_w);
    m_pagewidth_textctrl->SetValue(val);
    
    val = wxString::Format(wxT("%.2f"), in_h);
    m_pageheight_textctrl->SetValue(val);
    
    // fire dimension changed signal
    double width, height;
    getPageDimensions(&width, &height);
    sigDimensionsChanged(width, height);
}

void PageSizePanel::onOrientationChanged(int orientation)
{
    double width, height;
    getPageDimensions(&width, &height);
    
    m_orientation = orientation;
    
    if (orientation == wxLANDSCAPE && (width > height))
        return;

    if (orientation == wxPORTRAIT && (height > width))
        return;
    
    // flip the values to match the page's orientation
    double temp = width;
    width = height;
    height = temp;
    
    wxString val = wxString::Format(wxT("%.2f"), width);
    m_pagewidth_textctrl->SetValue(val);
    
    val = wxString::Format(wxT("%.2f"), height);
    m_pageheight_textctrl->SetValue(val);
}

BEGIN_EVENT_TABLE(PageSizePanel, wxPanel)
    EVT_CHOICE(ID_PageSize_Choice, PageSizePanel::onPaperSizeChanged)
END_EVENT_TABLE()




// -- MarginsPanel class implementation --

MarginsPanel::MarginsPanel(wxWindow* parent)
                        : wxPanel(parent,
                                  wxID_ANY,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxNO_FULL_REPAINT_ON_RESIZE |
                                  wxCLIP_CHILDREN |
                                  wxTAB_TRAVERSAL)
{
    // create left margin sizer
    wxStaticText* label_marginleft = new wxStaticText(this, -1, _("Left:"));
    m_marginleft_textctrl = new wxTextCtrl(this,
                                           ID_LeftMargin_TextCtrl,
                                           wxT("1.00"),
                                           wxDefaultPosition,
                                           wxSize(45,-1));

    wxBoxSizer* marginleft_sizer = new wxBoxSizer(wxHORIZONTAL);
    marginleft_sizer->Add(label_marginleft, 0, wxALIGN_CENTER);
    marginleft_sizer->Add(m_marginleft_textctrl, 1, wxALIGN_CENTER);

    // create right margin sizer
    wxStaticText* label_marginright = new wxStaticText(this, -1, _("Right:"));
    m_marginright_textctrl = new wxTextCtrl(this,
                                            ID_RightMargin_TextCtrl,
                                            wxT("1.00"),
                                            wxDefaultPosition,
                                            wxSize(45,-1));

    wxBoxSizer* marginright_sizer = new wxBoxSizer(wxHORIZONTAL);
    marginright_sizer->Add(label_marginright, 0, wxALIGN_CENTER);
    marginright_sizer->Add(m_marginright_textctrl, 1, wxALIGN_CENTER);

    // create top margin sizer

    wxStaticText* label_margintop = new wxStaticText(this, -1, _("Top:"));
    m_margintop_textctrl = new wxTextCtrl(this,
                                          ID_TopMargin_TextCtrl,
                                          wxT("1.00"),
                                          wxDefaultPosition,
                                          wxSize(45,-1));

    wxBoxSizer* margintop_sizer = new wxBoxSizer(wxHORIZONTAL);
    margintop_sizer->Add(label_margintop, 0, wxALIGN_CENTER);
    margintop_sizer->Add(m_margintop_textctrl, 1, wxALIGN_CENTER);

    // create bottom margin sizer
    wxStaticText* label_marginbottom = new wxStaticText(this, -1, _("Bottom:"));
    m_marginbottom_textctrl = new wxTextCtrl(this,
                                             ID_BottomMargin_TextCtrl,
                                             wxT("1.00"),
                                             wxDefaultPosition,
                                             wxSize(45,-1));

    wxBoxSizer* marginbottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    marginbottom_sizer->Add(label_marginbottom, 0, wxALIGN_CENTER);
    marginbottom_sizer->Add(m_marginbottom_textctrl, 1, wxALIGN_CENTER);

    // make the labels the same width
    wxSize s1 = getMaxTextSize(label_marginleft,
                                    label_marginright);
    
    wxSize s2 = getMaxTextSize(label_margintop,
                                    label_marginbottom);
    
    s1.x += 5;
    s2.x += 5;
    
    marginleft_sizer->SetItemMinSize(label_marginleft, s1);
    marginright_sizer->SetItemMinSize(label_marginright, s1);
    margintop_sizer->SetItemMinSize(label_margintop, s2);
    marginbottom_sizer->SetItemMinSize(label_marginbottom, s2);

    // create the top sizer
    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_sizer->Add(marginleft_sizer, 0, wxEXPAND);
    top_sizer->AddSpacer(10);
    top_sizer->Add(margintop_sizer, 0, wxEXPAND);
    
    // create the bottom sizer
    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    bottom_sizer->Add(marginright_sizer, 0, wxEXPAND);
    bottom_sizer->AddSpacer(10);
    bottom_sizer->Add(marginbottom_sizer, 0, wxEXPAND);
    
    // create margin sizer
    wxStaticBox* box = new wxStaticBox(this, -1, _("Margins (in inches)"));
    wxBoxSizer* main_sizer = new wxStaticBoxSizer(box, wxVERTICAL);
    main_sizer->AddSpacer(10);
    main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    main_sizer->AddSpacer(8);
    main_sizer->Add(bottom_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    main_sizer->AddSpacer(10);
    SetSizer(main_sizer);
    Layout();
}

void MarginsPanel::setMargins(double left,
                              double right,
                              double top,
                              double bottom)
{
    wxString val = wxString::Format(wxT("%.2f"), left);
    m_marginleft_textctrl->SetValue(val);

    val = wxString::Format(wxT("%.2f"), right);
    m_marginright_textctrl->SetValue(val);

    val = wxString::Format(wxT("%.2f"), top);
    m_margintop_textctrl->SetValue(val);

    val = wxString::Format(wxT("%.2f"), bottom);
    m_marginbottom_textctrl->SetValue(val);
}

void MarginsPanel::getMargins(double* left,
                              double* right,
                              double* top,
                              double* bottom)
{
    *left = ::wxAtof(m_marginleft_textctrl->GetValue());
    *right = ::wxAtof(m_marginright_textctrl->GetValue());
    *top = ::wxAtof(m_margintop_textctrl->GetValue());
    *bottom = ::wxAtof(m_marginbottom_textctrl->GetValue());
}




// -- PageSetupDialog class implementation --

PageSetupDialog::PageSetupDialog(wxWindow* parent) :
                            wxDialog(parent, 
                                     -1,
                                     _("Page Setup"),
                                     wxDefaultPosition,
                                     wxSize(420,280),
                                     wxDEFAULT_DIALOG_STYLE |
                                     wxRESIZE_BORDER |
                                     wxCLIP_CHILDREN |
                                     wxNO_FULL_REPAINT_ON_RESIZE)
{
    SetMinSize(wxSize(420, 280));
    SetMaxSize(wxSize(540, 400));

    // create panels
    m_orientation_panel = new OrientationPanel(this);
    m_pagesize_panel = new PageSizePanel(this);
    m_margins_panel = new MarginsPanel(this);

    // connect signals
    m_orientation_panel->sigOrientationChanged.connect(m_pagesize_panel, &PageSizePanel::onOrientationChanged);
    m_pagesize_panel->sigDimensionsChanged.connect(m_orientation_panel, &OrientationPanel::onPageDimensionsChanged);
}

PageSetupDialog::~PageSetupDialog()
{

}

int PageSetupDialog::ShowModal()
{
    // create bottom sizer
    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    bottom_sizer->Add(m_pagesize_panel, 1, wxEXPAND);
    bottom_sizer->AddSpacer(10);
    bottom_sizer->Add(m_margins_panel, 0, wxEXPAND);

    // create a platform standards-compliant OK/Cancel sizer
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_OK));
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_orientation_panel, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(10);
    main_sizer->Add(bottom_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddStretchSpacer();
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(main_sizer);
    Layout();
    
    return wxDialog::ShowModal();
}

void PageSetupDialog::setMargins(double left, double right, double top, double bottom)
{
    m_margins_panel->setMargins(left, right, top, bottom);
}

void PageSetupDialog::getMargins(double* left, double* right, double* top, double* bottom)
{
    m_margins_panel->getMargins(left, right, top, bottom);
}

void PageSetupDialog::setPageDimensions(double width, double height)
{
    m_pagesize_panel->setPageDimensions(width, height);
}

void PageSetupDialog::getPageDimensions(double* width, double* height)
{
    m_pagesize_panel->getPageDimensions(width, height);
}

wxPaperSize PageSetupDialog::getPaperSize()
{
    return m_pagesize_panel->getPaperSize();
}

int PageSetupDialog::getOrientation()
{
    return m_orientation_panel->getOrientation();
}




