/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-02-14
 *
 */


#ifndef __APP_PANELCOLPROPS_H
#define __APP_PANELCOLPROPS_H


#include "tabledoc.h"


class ExprBuilderPanel;

class ColPropsPanel :  public wxPanel,
                       public IDocument,
                       public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ColPropsPanel")
    XCM_BEGIN_INTERFACE_MAP(ColPropsPanel)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    ColPropsPanel();
    ~ColPropsPanel();

    void setModifyField(const wxString& new_val);
    wxString getModifyField();

public: // signals

    xcm::signal1<ColPropsPanel*> sigCancelPressed;
    xcm::signal1<ColPropsPanel*> sigOkPressed;

private:

    //  IDocument 
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    bool onSiteClosing(bool force);

    //  panel methods 

    void setName(const wxString& name);
    void setType(int type);
    void setWidth(int width);
    void setScale(int scale);
    void setExpression(const wxString& expr);

    void changeColumnCaption(const wxString& new_caption);

    void refreshDynamicFields();
    void updateSpinBoxes();
    void revertChanges();

    void populate();

    // frame events
    void onFrameEvent(FrameworkEvent& evt);
    void onSiteClose(IDocumentSitePtr site);

    void onOkPressed(ExprBuilderPanel*);
    void onCancelPressed(ExprBuilderPanel*);

    void onNameChanged(wxCommandEvent& evt);
    void onTypeChanged(wxCommandEvent& evt);
    void onWidthChanged(wxCommandEvent& evt);
    void onScaleChanged(wxCommandEvent& evt);
    void onWidthSpinChanged(wxSpinEvent& evt);
    void onScaleSpinChanged(wxSpinEvent& evt);
    void onExpressionChanged(ExprBuilderPanel*);

    void onCancel(wxCommandEvent& evt);

    void closeSite(bool ok = true);

private:

    // container context info
    IDocumentSitePtr m_doc_site;         // our document's site
    tango::ISetPtr m_set;                // tabledoc's set
    tango::IIteratorPtr m_iter;          // tabledoc's iterator
    IDocumentSitePtr m_tabledoc_site;    // tabledoc's site
    ITableDocPtr m_tabledoc;             // tabledoc ptr
    bool m_ok_pressed;

    wxString m_edit_column;
    wxString m_modify_field;

    wxTextCtrl* m_colname_text;
    wxComboBox* m_coltype_combo;
    wxSpinCtrl* m_colwidth_text;
    wxSpinCtrl* m_colscale_text;
    ExprBuilderPanel* m_expr_panel;

    wxString m_last_name;
    int m_last_type;
    int m_last_width;
    int m_last_scale;
    wxString m_last_expr;

    wxString m_orig_name;
    int m_orig_type;
    int m_orig_width;
    int m_orig_scale;
    wxString m_orig_expr;
    bool m_orig_existed;

    int m_saved_character_width;
    int m_saved_numeric_width;
    int m_saved_numeric_scale;

    DECLARE_EVENT_TABLE()
};




#endif



