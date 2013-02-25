/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-07-18
 *
 */


#ifndef __APP_EXPRBUILDER_H
#define __APP_EXPRBUILDER_H


#include <wx/stc/stc.h>
#include <wx/hyperlink.h>


class ExprEditorControl : public wxStyledTextCtrl,
                          public xcm::signal_sink
{
public:

    ExprEditorControl(wxWindow* parent,
                      wxWindowID id = -1,
                      const wxPoint& position = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = kcl::DEFAULT_BORDER);

    void setStructure(tango::IStructurePtr structure);

    wxString GetValue();
    void SetValue(const wxString& value);

    void SetInsertionPointEnd();
    int  GetInsertionPoint();
    void SetInsertionPoint(int pos);

private:

    int FindBrace(int pos);
    
    void onKeyDown(wxKeyEvent& evt);
    void onCharAdded(wxStyledTextEvent& evt);
    void onChanged(wxStyledTextEvent& evt);

private:

    wxString m_text;
};


class FieldListControl;


class ExprBuilderPanel : public wxPanel,
                         public xcm::signal_sink
{

public:

    ExprBuilderPanel();
    ExprBuilderPanel(wxWindow* parent);

    bool create(wxWindow* parent);
    
    void setOKEnabled(bool enable);
    void setOKVisible(bool visible);
    void setCancelVisible(bool visible);
    void setOKText(const wxString& ok_text);
    void setCancelText(const wxString& cancel_text);
    void setTypeOnly(int type);
    void setEmptyOk(bool empty_ok);
    void setValidationEnabled(bool enable) { m_validation = enable; }

    void setIterator(tango::IIteratorPtr iter);
    wxString getExpression();
    int getExpressionType();
    void setExpression(const wxString& expr);
    void setEnabled(bool enabled);
    bool validate();
    
    // signal event handlers
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    void onColumnActivated(int row, const wxString& text);
    
    // event handlers
    void onGridBeginDrag(kcl::GridEvent& evt);
    void onOperButtonClicked(wxCommandEvent& evt);
    void onFunctionActivated(kcl::GridEvent& evt);
    void onFunctionListCursorMove(kcl::GridEvent& evt);
    void onFunctionListNeedTooltipText(kcl::GridEvent& evt);
    void onLearnMoreHyperlinkClicked(wxHyperlinkEvent& evt);
    void onExprChanged(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onOK(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);

public: // signals

    xcm::signal1<ExprBuilderPanel*> sigCancelPressed;
    xcm::signal1<ExprBuilderPanel*> sigOkPressed;
    xcm::signal1<ExprBuilderPanel*> sigExpressionChanged;

protected:

    void construct();
    void insertText(const wxString& text);

protected:
    
    tango::IIteratorPtr m_iter;
    tango::IStructurePtr m_structure;
    wxString m_ok_text;
    wxString m_cancel_text;
    int m_type_only;
    int m_expr_type;
    bool m_mask_signals;
    bool m_empty_ok;
    bool m_validation;

    wxBoxSizer* m_main_sizer;
    wxBoxSizer* m_helper_sizer;
    
    ExprEditorControl* m_expr_text;
    kcl::RowSelectionGrid* m_func_list;
    FieldListControl* m_col_list;
    kcl::ValidControl* m_valid_control;
    
    wxStaticText* m_func_syntax_label;
    wxHyperlinkCtrl* m_learn_more_hyperlink;
    
    wxButton* m_ok_button;
    wxButton* m_cancel_button;

    DECLARE_EVENT_TABLE()
};



class KeyBuilderPanel : public wxPanel,
                        public xcm::signal_sink
{
public:

    KeyBuilderPanel();
    KeyBuilderPanel(wxWindow* parent);

    bool create(wxWindow* parent);

    void setOKVisible(bool visible);
    void setCancelVisible(bool visible);
    void setOKText(const wxString& ok_text);
    void setCancelText(const wxString& cancel_text);
    void setEmptyOk(bool empty_ok);
    void setEnabled(bool enabled);
    void setNoOrders();
    void setOverlayText(const wxString& expr);

    void setIterator(tango::IIteratorPtr iter);
    tango::IIteratorPtr getIterator();

    void setExpression(const wxString& expr);
    wxString getExpression();

    bool validate();

private:

    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onColumnActivated(int row, const wxString& text);
    
    // -- grid events --
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    void onGridDeletedRows(std::vector<int> rows);

public: // signals

    xcm::signal1<KeyBuilderPanel*> sigCancelPressed;
    xcm::signal1<KeyBuilderPanel*> sigOkPressed;
    xcm::signal1<KeyBuilderPanel*> sigExpressionChanged;

protected:

    void construct();
    void parseExpr();
    void buildExpr();

    //void addField(const wxString& field);

    void insertText(const wxString& text);
    void updateGridOverlay();
    void onGridModelChange(kcl::GridEvent& evt);

protected:

    tango::IIteratorPtr m_iter;
    tango::IStructurePtr m_structure;
    wxString m_ok_text;
    wxString m_cancel_text;
    wxString m_expr;
    wxString m_overlay_text;

    kcl::RowSelectionGrid* m_grid;
    FieldListControl* m_col_list;
    kcl::ValidControl* m_valid_control;
    wxButton* m_ok_button;
    wxButton* m_cancel_button;

    bool m_mask_signals;
    bool m_with_orders;
    bool m_empty_ok;
    bool m_length_column_visible;

    DECLARE_EVENT_TABLE()
};




class DlgExprBuilder : public wxDialog,
                       public xcm::signal_sink
{

public:

    DlgExprBuilder(wxWindow* parent,
                   const wxString& title = _("Formula Builder"));

    void onOKPressed(ExprBuilderPanel*);
    void onCancelPressed(ExprBuilderPanel*);
    void onCancel(wxCommandEvent& evt);
    
    void setIterator(tango::IIteratorPtr iter);
    wxString getExpression();
    void setExpression(const wxString& expr);
    void setTypeOnly(int type);
    void setEmptyOk(bool empty_ok);

    void setOKVisible(bool visible);
    void setCancelVisible(bool visible);
    void setOKText(const wxString& ok_text);
    void setCancelText(const wxString& cancel_text);
    
public: // signals

    xcm::signal1<wxString> sigExprEditFinished;
    xcm::signal0 sigExprEditCancelled;
    
protected:

    ExprBuilderPanel* m_panel;
    wxBoxSizer* m_main_sizer;

    DECLARE_EVENT_TABLE()
};



class DlgKeyBuilder : public wxDialog,
                      public xcm::signal_sink
{
public:

    DlgKeyBuilder(wxWindow* parent,
                  const wxString& title = _("Key Builder"));

    void setIterator(tango::IIteratorPtr iter);
    wxString getExpression();
    void setExpression(const wxString& expr);
    void setEmptyOk(bool empty_ok);
    void setNoOrders();
    void setOverlayText(const wxString& expr);

    void setOKVisible(bool visible);
    void setCancelVisible(bool visible);
    void setOKText(const wxString& ok_text);
    void setCancelText(const wxString& cancel_text);

protected:

    KeyBuilderPanel* m_panel;
    wxBoxSizer* m_main_sizer;

    void onOKPressed(KeyBuilderPanel*);
    void onCancelPressed(KeyBuilderPanel*);
    void onCancel(wxCommandEvent& evt);

public: // signals

    xcm::signal1<wxString> sigExprEditFinished;
    xcm::signal0 sigExprEditCancelled;

    DECLARE_EVENT_TABLE()
};





#endif

