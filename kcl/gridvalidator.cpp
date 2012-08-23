/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2008-09-02
 *
 */


#include "grid.h"
#include "gridvalidator.h"


namespace kcl
{


BEGIN_EVENT_TABLE(GridActionValidator, wxEvtHandler)
    EVT_KCLGRID_END_EDIT(GridActionValidator::onGridEndEdit)
    EVT_KCLGRID_NEED_TOOLTIP_TEXT(GridActionValidator::onGridNeedTooltipText)
END_EVENT_TABLE()


GridActionValidator::GridActionValidator(kcl::Grid* grid,
                                         const wxBitmap& error_bmp,
                                         const wxBitmap& normal_bmp)
{
    m_grid = grid;
    m_grid->PushEventHandler(this);
    
    m_error_bmp = error_bmp;
    m_normal_bmp = normal_bmp;
}

GridActionValidator::~GridActionValidator()
{
    std::vector<GridActionRule*>::iterator it;
    for (it = m_rules.begin(); it != m_rules.end(); ++it)
        delete (*it);
}

GridActionRule* GridActionValidator::addRule()
{
    GridActionRule* rule = new GridActionRule;
    m_rules.push_back(rule);
    return rule;
}

GridActionRule* GridActionValidator::addRule(int col, RuleType rule_type)
{
    if (rule_type == GridActionValidator::RuleNoDuplicates)
    {
        GridActionRule* rule = addRule();
        rule->setColumn(col);
        rule->setErrorColumn(col);
        rule->setErrorText(_("This item has the same name as another item in this list"));
        rule->sigRuleCheckRequested.connect(this, &GridActionValidator::onCheckDuplicates);
        return rule;
    }
     else if (rule_type == GridActionValidator::RuleNoSpaces)
    {
        kcl::GridActionRule* rule = addRule();
        rule->setColumn(col);
        rule->setErrorColumn(col);
        rule->setErrorText(_("Item names cannot contain spaces"));
        rule->sigRuleCheckRequested.connect(this, &GridActionValidator::onCheckSpaces);
        return rule;
    }
    
    return addRule();
}

bool GridActionValidator::validate(bool mark_rows)
{
    return validateColumn(-1, mark_rows);
}

bool GridActionValidator::validateColumn(int col, bool mark_rows)
{
    std::vector<int> error_rows;
    int col_count = m_grid->getColumnCount();
    
    if (mark_rows)
    {
        // clear marked rows
        
        std::vector<GridActionRule*>::iterator it;
        for (it = m_rules.begin(); it != m_rules.end(); ++it)
        {
            GridActionRule* rule = (*it);
            int error_col = rule->getErrorColumn();
            
            // error column out of bounds
            if (error_col < 0 || error_col >= col_count)
                continue;
            
            // if col == -1, clear all rule error columns, if not,
            // only clear rule error columns to which the rule applies
            if (col != rule->getColumn() && col != -1)
                continue;
            
            // clear marks from the rule's error column
            int row, row_count = m_grid->getRowCount();
            for (row = 0; row < row_count; ++row)
                m_grid->setCellBitmap(row, error_col, m_normal_bmp);
        }
    }
    
    std::vector<GridActionRule*>::iterator it;
    for (it = m_rules.begin(); it != m_rules.end(); ++it)
    {
        // find (and mark) rows that break any rules
        
        GridActionRule* rule = (*it);
        int error_col = rule->getErrorColumn();
        
        // if col == -1, check all columns, if not,
        // only check columns to which the rule applies
        if (col != rule->getColumn() && col != -1)
            continue;
        
        // get the rows that have an error in them
        std::vector<int> rule_errors;
        rule->sigRuleCheckRequested(m_grid, rule->getColumn(), rule_errors);
        
        // no errors for this rule; continue
        if (rule_errors.size() == 0)
            continue;
        
        // add the rule's errors to the error rows vector
        std::vector<int>::iterator it2;
        for (it2 = rule_errors.begin(); it2 != rule_errors.end(); ++it2)
            error_rows.push_back(*it2);
        
        // error column out of bounds
        if (error_col < 0 || error_col >= col_count)
            continue;
        
        // mark the rows that have errors in them
        if (mark_rows)
            markRows(error_col, rule_errors);
    }
    
    return (error_rows.size() > 0) ? false : true;
}

void GridActionValidator::markRows(int col, std::vector<int>& rows)
{
    // out of bounds
    int col_count = m_grid->getColumnCount();
    if (col < 0 || col >= col_count)
        return;
    
    // mark the problem rows
    std::vector<int>::iterator row_it;
    for (row_it = rows.begin(); row_it != rows.end(); ++row_it)
        m_grid->setCellBitmap(*row_it, col, m_error_bmp);
}

void GridActionValidator::getRulesBrokenByCell(
                                    int row, int col,
                                    std::vector<GridActionRule*>& retval)
{
    retval.clear();
    
    int row_count = m_grid->getRowCount();
    int col_count = m_grid->getColumnCount();
    
    // out of bounds
    if (row < 0 || col < 0 || row >= row_count || col >= col_count)
        return;
    
    std::vector<GridActionRule*>::iterator it;
    for (it = m_rules.begin(); it != m_rules.end(); ++it)
    {
        GridActionRule* rule = (*it);
        
        // this rule doesn't apply to this column; continue
        if (col != rule->getColumn())
            continue;
        
        // get the rows that have an error in them
        std::vector<int> error_rows;
        rule->sigRuleCheckRequested(m_grid, col, error_rows);
        
        if (error_rows.size() == 0)
            continue;
        
        // if the row exists in the row error vector, add the rule to the return
        // vector of rules that have been broken by this cell
        std::vector<int>::iterator it2;
        for (it2 = error_rows.begin(); it2 != error_rows.end(); ++it2)
        {
            if (row == *it2)
            {
                retval.push_back(rule);
                break;
            }
        }
    }
}

void GridActionValidator::onCheckSpaces(kcl::Grid* grid,
                                        int col,
                                        std::vector<int>& retval)
{
    retval.clear();
    
    int row, row_count = grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        wxString s = grid->getCellString(row, col);
        
        // if the cell contains an invalid character,
        // add it to the return vector
        if (wxStrchr(s, wxT(' ')))
            retval.push_back(row);
    }
}

void GridActionValidator::onCheckDuplicates(kcl::Grid* grid,
                                            int col,
                                            std::vector<int>& retval)
{
    retval.clear();
    
    std::vector<int>::iterator it;
    std::vector<int> check_rows;
    
    // store the check rows
    int row1, row2, row_count = grid->getRowCount();
    for (row1 = 0; row1 < row_count; ++row1)
        check_rows.push_back(row1);
    
    bool row1_added;
    wxString s1, s2;
    
    for (row1 = 0; row1 < row_count; ++row1)
    {
        s1 = grid->getCellString(row1, col);
        row1_added = false;
        
        int i, count = (int)check_rows.size();
        for (i = count-1; i >= 0; --i)
        {
            // assign the iterator to a variable (for readability)
            row2 = check_rows[i];
            
            // don't check the row against itself
            // or rows that we've previously checked
            if (row2 <= row1)
                continue;
            
            s2 = grid->getCellString(row2, col);
            if (s1.CmpNoCase(s2) == 0)
            {
                // add row1 to the return vector
                if (!row1_added)
                    retval.push_back(row1);
                
                // add row2 to the return vector
                retval.push_back(row2);
                
                // remove the row2 entry from the rows we need to check
                it = check_rows.erase(check_rows.begin()+i);
                
                // make sure we don't add the row1 value
                // to the return vector more than once
                row1_added = true;
            }
        }
    }
}

void GridActionValidator::onGridEndEdit(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    // we need to set the cell value here because getCellString() is called
    // in the rule check request below and the value is not normally changed
    // until after the end edit event is processed (which means the value
    // that would be checked would be the old value in the cell)
    m_grid->setCellString(row, col, evt.GetString());
    
    // validate (and mark) the rows that have an error in them
    validateColumn(col);
    
    evt.Skip();
}

void GridActionValidator::onGridNeedTooltipText(kcl::GridEvent& evt)
{
    int evt_col = evt.GetColumn();
    int evt_row = evt.GetRow();
    
    // don't show a tooltip tooltip for column headers
    if (evt_row < 0)
    {
        evt.Skip();
        return;
    }
    
    // check to see if any rules are broken by the cell we're hovering over
    std::vector<GridActionRule*> broken_rules;
    getRulesBrokenByCell(evt_row, evt_col, broken_rules);
    
    // no rules broken, we're done
    if (broken_rules.size() == 0)
    {
        evt.Skip();
        return;
    }
    
    wxString tooltip;
    
    std::vector<GridActionRule*>::iterator it;
    for (it = broken_rules.begin(); it != broken_rules.end(); ++it)
    {
        GridActionRule* rule = (*it);
        
        // no error text, go to the next error
        if (rule->getErrorText().IsEmpty())
            continue;
        
        // for multiple infractions, show a bulleted list
        // of errors in the tooltip
        if (broken_rules.size() > 1)
            tooltip += wxT("- ");
        
        // add the error text
        tooltip += rule->getErrorText();
        
        // add line feed
        if (broken_rules.size() > 1 && it != broken_rules.end()-1)
            tooltip += wxT("\n");
    }
    
    // show the tooltip
    if (tooltip.Length() > 0)
        evt.SetString(tooltip);
}


};  // namespace kcl


