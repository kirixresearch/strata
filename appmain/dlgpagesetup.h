/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams; David Z. Williams
 * Created:  2002-10-08
 *
 */


#ifndef H_APP_DLGPAGESETUP_H
#define H_APP_DLGPAGESETUP_H


class OrientationPanel : public wxPanel,
                         public xcm::signal_sink
{
public:

    OrientationPanel(wxWindow* parent);
    
    void setOrientation(int orientation);
    int getOrientation();
    
public: // signals
    
    xcm::signal1<int /*orientation*/> sigOrientationChanged;
    
public: // signal handlers
    
    void onPageDimensionsChanged(double width, double height);
    
private:
    
    void onOrientationChanged(wxCommandEvent& evt);
    
private:

    wxRadioButton* m_portrait_radio;
    wxRadioButton* m_landscape_radio;
    
    DECLARE_EVENT_TABLE()
};




class wxPrintPaperDatabase;
class PageSizePanel : public wxPanel,
                      public xcm::signal_sink
{
public:

    PageSizePanel(wxWindow* parent);
    
    void setPageDimensions(double width, double height);
    void getPageDimensions(double* width, double* height);

    wxPaperSize getPaperSize();
    
public: // signals
    
    xcm::signal2<double /*width*/, double /*height*/> sigDimensionsChanged;
    
public: // signal handlers
    
    void onOrientationChanged(int orientation);
    
private:

    void updatePaperSizeChoice();
    
private:

    void onKillFocus(wxFocusEvent& evt);
    void onPaperSizeChanged(wxCommandEvent& evt);
    
private:

    wxChoice*   m_pagesize_choice;
    wxTextCtrl* m_pagewidth_textctrl;
    wxTextCtrl* m_pageheight_textctrl;
    
    // this vector's index corresponds with the choice control's index
    std::vector<wxPaperSize> m_papersizes;
    
    wxString m_custom_name;
    int m_orientation;
    
    DECLARE_EVENT_TABLE()
};




class MarginsPanel : public wxPanel
{
public:

    MarginsPanel(wxWindow* parent);
    
    void setMargins(double left,
                    double right,
                    double top,
                    double bottom);
    void getMargins(double* left,
                    double* right,
                    double* top,
                    double* bottom);
    
private:

    wxTextCtrl* m_marginleft_textctrl;
    wxTextCtrl* m_marginright_textctrl;
    wxTextCtrl* m_margintop_textctrl;
    wxTextCtrl* m_marginbottom_textctrl;
};




class PageSetupDialog : public wxDialog
{
public:

    PageSetupDialog(wxWindow* parent);
    ~PageSetupDialog();
    
    // wxDialog override
    int ShowModal();
    
    void setMargins(double left, double right, double top, double bottom);
    void getMargins(double* left, double* right, double* top, double* bottom);

    void setPageDimensions(double width, double height);
    void getPageDimensions(double* width, double* height);
    
    wxPaperSize getPaperSize();
    int getOrientation();
    void setOrientation(int orientation /* wxPORTRAIT, wxLANDSCAPE */);
    
private:
    
    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    
private:

    OrientationPanel* m_orientation_panel;
    PageSizePanel* m_pagesize_panel;
    MarginsPanel* m_margins_panel;
};

#endif

