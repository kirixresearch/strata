/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-03-06
 *
 */


#ifndef __APP_EXPORTPAGES_H
#define __APP_EXPORTPAGES_H


// forward declarations
class ExportInfo;


// page to select the tables the user wants to export

class ExportTableSelectionPage : public kcl::WizardPage,
                                 public xcm::signal_sink
{
public:

    ExportTableSelectionPage(kcl::Wizard* parent, ExportInfo* ei);
    
    void markProblemRow(int row, bool scroll_to);
    void clearProblemRows();
    
    int checkDuplicateTablenames(bool mark_rows = true);
    int checkInvalidTablenames(bool mark_rows = true);
    
    void setExportPathLabel(const wxString& label);
    void refreshGrid();

private:

    void checkOverlayText();

    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    void onGridNeedTooltipText(kcl::GridEvent& evt);
    void onGridEndEdit(kcl::GridEvent& evt);

    void onPageChanged();
    bool onPageChanging(bool forward);
    
    // -- signal events --
    void onDeletedRows(std::vector<int> rows);

private:

    ExportInfo* m_ei;

    wxBoxSizer* m_path_sizer;
    wxBoxSizer* m_main_sizer;
    
    wxStaticText* m_message;
    wxStaticText* m_label_exportpath;
    kcl::RowSelectionGrid* m_grid;

    int m_last_type;

    DECLARE_EVENT_TABLE()
};


// page to specify the delimited text settings

class ExportDelimitedTextSettingsPage : public kcl::WizardPage
{

public:

    ExportDelimitedTextSettingsPage(kcl::Wizard* parent, ExportInfo* ei);
    void loadPageData();
    void savePageData();

private:

    void onPageChanged();
    bool onPageChanging(bool forward);

private:

    ExportInfo* m_ei;

    wxRadioButton* m_comma_radio;
    wxRadioButton* m_tab_radio;
    wxRadioButton* m_semicolon_radio;
    wxRadioButton* m_space_radio;
    wxRadioButton* m_nodelimiters_radio;
    wxRadioButton* m_otherdelimiters_radio;
    wxTextCtrl* m_otherdelimiters_text;

    wxRadioButton* m_doublequote_radio;
    wxRadioButton* m_singlequote_radio;
    wxRadioButton* m_notextqualifier_radio;
    wxRadioButton* m_othertextqualifier_radio;
    wxTextCtrl* m_othertextqualifier_text;

    wxCheckBox* m_firstrowheader_check;

    DECLARE_EVENT_TABLE()
};


#endif  // __APP_EXPORTPAGES_H


