/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2002-06-28
 *
 */


#ifndef H_APP_PANELGROUP_H
#define H_APP_PANELGROUP_H


class FieldListControl;


class GroupPanel :  public wxPanel,
                    public IDocument,
                    public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.GroupPanel")
    XCM_BEGIN_INTERFACE_MAP(GroupPanel)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    GroupPanel();
    ~GroupPanel();

    void setParameters(const wxString& path, const wxString& where_condition);

    // IDocument
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // panel methods
    void markProblemRow(int row,
                        bool scroll_to = false,
                        bool refresh = true);

    void addGroupField(const wxString& input);

private:
    
    bool validateGroupQuery();
    void checkOverlayText();
    void insertOutputField(int row,
                           const wxString& input,
                           int function = -1,
                           bool refresh = true);

private:

    void onExecute(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onAdvancedQueryText(wxCommandEvent& evt);

    // grid events
    void onGridEndEdit(kcl::GridEvent& evt);
    void onGridEditChange(kcl::GridEvent& evt);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);

    // signal events
    void onFieldDblClicked(int row, const wxString& field);
    void onDeletedRows(std::vector<int> rows);

private:

    IDocumentSitePtr m_doc_site;
    xd::Structure m_structure;
    
    wxString m_path;
    wxString m_where_condition;

    FieldListControl* m_tablecols;
    kcl::RowSelectionGrid* m_grid;
    wxButton* m_ok_button;
    
    // advanced options elements
    wxTextCtrl* m_adv_group_query;
    kcl::ValidControl* m_adv_group_query_valid;
    wxCheckBox* m_adv_checkbox;

    DECLARE_EVENT_TABLE()
};






#endif

