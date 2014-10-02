/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-27
 *
 */


#ifndef __APP_TRANSFORMATIONDOC_H
#define __APP_TRANSFORMATIONDOC_H


#include "panelcolumnlist.h"


class TransformField
{
public:

    TransformField()
    {
        input_name = wxEmptyString;
        input_type = xd::typeCharacter;
        input_width = 20;
        input_scale = 0;
        input_offset = 0;

        output_name = wxEmptyString;
        output_type = xd::typeCharacter;
        output_width = 20;
        output_scale = 0;
        output_expression = wxEmptyString;

        calculated = false;
        original = true;
    }

    TransformField(const TransformField& c)
    {
        input_name = c.input_name;
        input_type = c.input_type;
        input_width = c.input_width;
        input_scale = c.input_scale;
        input_offset = c.input_offset;

        output_name = c.output_name;
        output_type = c.output_type;
        output_width = c.output_width;
        output_scale = c.output_scale;
        output_expression = c.output_expression;

        calculated = c.calculated;
        original = c.original;
    }

    TransformField& operator=(const TransformField& c)
    {
        input_name = c.input_name;
        input_type = c.input_type;
        input_width = c.input_width;
        input_scale = c.input_scale;
        input_offset = c.input_offset;

        output_name = c.output_name;
        output_type = c.output_type;
        output_width = c.output_width;
        output_scale = c.output_scale;
        output_expression = c.output_expression;

        calculated = c.calculated;
        original = c.original;

        return *this;
    }

public:

    wxString input_name;
    int input_type;
    int input_width;
    int input_scale;
    int input_offset;

    wxString output_name;
    int output_type;
    int output_width;
    int output_scale;
    wxString output_expression;

    bool calculated;
    bool original;  // flag to determine if field is in original structure
};




xcm_interface ITransformationDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ITransformationDoc")

public:

    virtual void initFromDefinition(const xd::FormatDefinition& def) = 0;
    virtual void close() = 0;
    
    virtual void getTransformation(std::vector<TransformField>& result) = 0;

    virtual void setInputStructure(const std::vector<TransformField>& input_fields) = 0;
    virtual void setInputStructure(xd::IStructurePtr structure) = 0;
};

XCM_DECLARE_SMARTPTR(ITransformationDoc)




class TransformationDoc : public wxWindow,
                          public IDocument,
                          public ITransformationDoc,
                          public IColumnListTarget,
                          public StatusBarProviderBase,
                          public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.TransformationDoc")
    XCM_BEGIN_INTERFACE_MAP(TransformationDoc)
        XCM_INTERFACE_ENTRY(IDocument)
        XCM_INTERFACE_ENTRY(ITransformationDoc)
        XCM_INTERFACE_ENTRY(IColumnListTarget)
        XCM_INTERFACE_CHAIN(StatusBarProviderBase)
    XCM_END_INTERFACE_MAP()

public:

    enum ErrorCheckFlags
    {
        CheckDefault         = 0x00,   // will not mark rows
        CheckMarkRows        = 0x01,   // marks rows with errors
        CheckEmptyFieldnames = 0x02    // flag empty fieldnames as invalid
    };
    
public:
    
    TransformationDoc();
    virtual ~TransformationDoc();
    
    // ITransformationDoc
    void initFromDefinition(const xd::FormatDefinition& def);
    void close();

    void getTransformation(std::vector<TransformField>& result);

    void setInputStructure(const std::vector<TransformField>& input_fields);
    void setInputStructure(xd::IStructurePtr structure);

private:

    // IDocument
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow() { return static_cast<wxWindow*>(this); }
    wxString getDocumentLocation();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    void onSiteActivated();

    // IColumnListTarget
    void getColumnListItems(std::vector<ColumnListItem>& items);
    void onColumnListDblClicked(const std::vector<wxString>& items);

    // TransformationDoc methods
    TransformField getInputFieldByName(const wxString& input_name);
    
    void insertRow(int row, bool dynamic = false);
    void insertSelectedRows(bool dynamic = false);
    void insertRowFromColumnInfo(int row, const xd::ColumnInfo& colinfo);

    void updateNumberColumn();
    void updateExpressionIcon(int row, int validation_res);
    void updateRowWidthAndScale(int row);
    void updateRowCellProps(int row);
    void updateStatusBar();
    void checkOverlayText();

    // error checking functions
    void clearProblemRows();
    void markProblemRow(int row, bool scroll_to);
    int checkDuplicateFieldnames(int check_flags);
    int checkInvalidFieldnames(int check_flags);
    int validateExpression(const xd::Structure& structure, const wxString& expr, int type = xd::typeUndefined);
    int validateStructure();
    bool doErrorCheck();
    
    // under-the-hood expression functions
    wxString getFieldExpression(int row);
    wxString createDestinationExpression(int row);
    bool getInfoFromDestinationExpression(const wxString& expression,
                                          int xd_type,
                                          wxString* source_name,
                                          int* format_comboidx);

    void populateSourceFieldDropDown();
    xd::IStructurePtr createStructureFromGrid();
    xd::IStructurePtr getSourceStructure();
    
    bool doSave();

private:

    // signal events
    void onInsertingRows(std::vector<int> rows);
    void onDeletedRows(std::vector<int> rows);

    // frame event handlers
    void onFrameEvent(FrameworkEvent& evt);

    // event handlers
    void onSize(wxSizeEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);
    void onSelectAll(wxCommandEvent& evt);
    void onCopy(wxCommandEvent& evt);
    void onSave(wxCommandEvent& evt);
    void onDeleteField(wxCommandEvent& evt);
    void onInsertField(wxCommandEvent& evt);
    void onToggleView(wxCommandEvent& evt);
    void onGridNeedTooltipText(kcl::GridEvent& evt);
    void onGridBeginEdit(kcl::GridEvent& evt);
    void onGridEndEdit(kcl::GridEvent& evt);
    void onGridEditChange(kcl::GridEvent& evt);
    void onGridCellRightClick(kcl::GridEvent& evt);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI(wxUpdateUIEvent& evt);

private:

    IFramePtr m_frame;            // ptr to the application frame
    IDocumentSitePtr m_doc_site;  // ptr to our document site
    
    kcl::RowSelectionGrid* m_grid;
    
    std::vector<TransformField> m_source_fields;
    bool m_dirty;
    int m_last_selected_fieldtype;

    // choices for the format/formula combobox dropdown
    std::vector<wxString> m_char_format_choices;
    std::vector<wxString> m_date_format_choices;
    std::vector<wxString> m_bool_format_choices;
    std::vector<wxString> m_numeric_format_choices;

    DECLARE_EVENT_TABLE()
};


#endif  // __APP_TRANSFORMATIONDOC_H
