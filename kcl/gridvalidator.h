/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2008-09-02
 *
 */


#ifndef __KCL_GRIDVALIDATOR_H
#define __KCL_GRIDVALIDATOR_H


#include <vector>


namespace kcl
{


class GridActionRule
{
public:

    GridActionRule()
    {
        m_check_col = -1;
        m_error_col = -1;
        m_error_text = wxEmptyString;
    }
    
    void setColumn(int col)          { m_check_col = col;   }
    void setErrorColumn(int col)     { m_error_col = col;   }
    void setErrorText(wxString text) { m_error_text = text; }
    
    int getColumn()         { return m_check_col;  }
    int getErrorColumn()    { return m_error_col;  }
    wxString getErrorText() { return m_error_text; }
    
public: // signals

    xcm::signal3<kcl::Grid* /* grid */,
                 int /* col */,
                 std::vector<int>& /* error rows */> sigRuleCheckRequested;

private:

    int m_check_col;                // column to check
    int m_error_col;                // column in which to report the error (bitmap)
    wxString m_error_text;          // error text to display in the tooltip
};


class GridActionValidator : public wxEvtHandler,
                            public xcm::signal_sink
{
public:

    enum RuleType
    {
        RuleNoDuplicates = 0,
        RuleNoSpaces = 1
    };
    
public:

    GridActionValidator(kcl::Grid* grid,
                        const wxBitmap& error_bmp,
                        const wxBitmap& normal_bmp = wxNullBitmap);
    ~GridActionValidator();
    
    GridActionRule* addRule();
    GridActionRule* addRule(int col, RuleType rule_type);
    
    bool validate(bool mark_rows = true);
    
private:
    
    bool validateColumn(int col /* -1 to validate all columns */,
                        bool mark_rows = true);
    
    void markRows(int col, std::vector<int>& rows);
    void getRulesBrokenByCell(int row, int col,
                              std::vector<GridActionRule*>& retval);
    
private:

    void onGridEndEdit(kcl::GridEvent& evt);
    void onGridNeedTooltipText(kcl::GridEvent& evt);
    
    // signal handlers
    void onCheckSpaces(kcl::Grid* grid,
                         int col,
                         std::vector<int>& retval);
    void onCheckDuplicates(kcl::Grid* grid,
                           int col,
                           std::vector<int>& retval);
    
private:
    
    std::vector<GridActionRule*> m_rules;
    kcl::Grid* m_grid;
    wxBitmap m_error_bmp;
    wxBitmap m_normal_bmp;
    
    DECLARE_EVENT_TABLE()
};




};  // namespace kcl


#endif  // __KCL_GRIDVALIDATOR_H


