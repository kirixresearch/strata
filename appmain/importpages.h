/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-03-01
 *
 */


#ifndef __APP_IMPORTPAGES_H
#define __APP_IMPORTPAGES_H


// forward declarations
class ImportInfo;
class ImportTableSelection;

// -- page to select the tables the user wants to import --

class ImportTableSelectionPage : public kcl::WizardPage,
                                 public xcm::signal_sink
{
public:

    ImportTableSelectionPage(kcl::Wizard* parent, ImportInfo* ii);
    void markProblemRow(int row, bool scroll_to);
    void clearProblemRows();
    int checkDuplicateTablenames(bool mark_rows = true);
    int checkInvalidTablenames(bool mark_rows = true);

    void loadPageData();
    void savePageData();

private:

    void checkOverlayText();
    ImportTableSelection* lookupTemplateTable(const wxString& tbl);

    void onPageChanged();
    bool onPageChanging(bool forward);

    // -- event handlers --
    void onGridNeedTooltipText(kcl::GridEvent& evt);
    void onGridEndEdit(kcl::GridEvent& evt);
    void onBrowse(wxCommandEvent& evt);
    void onBasePathChanged(wxCommandEvent& evt);
    void onSelectAllTables(wxCommandEvent& evt);
    void onSelectNoTables(wxCommandEvent& evt);

private:

    ImportInfo* m_ii;

    wxStaticText* m_message;
    wxTextCtrl* m_basepath_textctrl;
    kcl::RowSelectionGrid* m_grid;

    int m_last_type;             // these two variables are used for tracking
    wxString m_last_path;        // user changes during wizard use

    DECLARE_EVENT_TABLE()
};


// -- page to specify the delimited text settings --

class ImportDelimitedTextSettingsPage : public kcl::WizardPage
{
public:

    ImportDelimitedTextSettingsPage(kcl::Wizard* parent, ImportInfo* ii);
    void loadPageData();
    void savePageData();

private:

    void onPageChanged();
    bool onPageChanging(bool forward);

private:

    ImportInfo* m_ii;

    wxRadioButton* m_comma_radio;
    wxRadioButton* m_tab_radio;
    wxRadioButton* m_semicolon_radio;
    wxRadioButton* m_pipe_radio;
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


#endif  // __APP_IMPORTPAGES_H


