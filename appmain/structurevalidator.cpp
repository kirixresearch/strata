/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-07-19
 *
 */


#include "appmain.h"
#include "structurevalidator.h"


int StructureValidator::showErrorMessage(int errorcode, bool* block)
{
    if (errorcode == ErrorNone)
    {
        *block = false;
        return -1;
    }
    
    if (errorcode & ErrorNoFields)
    {
        *block = true;
        return appMessageBox(_("There are no fields in the table structure.  Add at least one field to the table to continue."),
                                  _("Invalid Table Structure"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    if (errorcode & ErrorInvalidStructure)
    {
        // NOTE: This error message should never be shown to the user
        *block = true;
        return appMessageBox(_("The source table structure is invalid or corrupt.  This file cannot be opened in the Table View."),
                                  _("Invalid Table Structure"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    // this error code can use the bitmask
    if (errorcode & ErrorDuplicateFieldNames)
    {
        *block = true;
        return appMessageBox(_("Two or more fields have the same name.  Check to make sure all field names are unique to continue."),
                                  _("Invalid Table Structure"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }
    
    // this error code can use the bitmask
    if (errorcode & ErrorDuplicateTableNames)
    {
        *block = true;
        return appMessageBox(_("Two or more tables have the same name.  Check to make sure all table names are unique to continue."),
                                  _("Duplicate Table Names"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }
    
    // this error code can use the bitmask
    if (errorcode & ErrorInvalidFieldNames)
    {
        *block = true;
        return appMessageBox(_("One or more field names are invalid.  Check to make sure all field names are valid to continue."),
                                  _("Invalid Table Structure"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    // this error code can use the bitmask
    if (errorcode & ErrorInvalidTableNames)
    {
        *block = true;
        return appMessageBox(_("One or more table names are invalid.  Check to make sure all table names are valid to continue."),
                                  _("Invalid Table Names"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }
/*
    // NOTE: these messages end up getting in the way more than helping; sometimes,
    // it might be useful to create a structure with some placeholder calculated
    // fields or just get to the table view without validating every calculated field;
    //  also, invalid calculated fields can happen in table view anyway by deleting 
    // a calculated field that is used in other calculated fields, rendering those
    // other fields invalid; in short, don't stop the user from going to the table
    // view, but keep this code here for reference

    if (errorcode & ErrorInvalidExpressions)
    {
        *block = true;
        return appMessageBox(_("One or more formulas has invalid syntax.  Check to make sure all formulas have valid syntax to continue."),
                                  _("Invalid Formula(s)"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    if (errorcode & ErrorExpressionTypeMismatch)
    {
        *block = false;
        int retcode = appMessageBox(_("One or more of the formulas has a return type that does not match the field type.  Would you like to continue?"),
                                         _("Invalid Formula(s)"),
                                         wxYES_NO | wxICON_QUESTION | wxCENTER,
                                         g_app->getMainWindow());
        if (retcode == wxNO)
            *block = true;
        
        return retcode;
    }
*/
    *block = false;
    return -1;
}

int StructureValidator::validateExpression(const xd::Structure& structure,
                                           const wxString& expr,
                                           int match_fieldtype)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return ExpressionInvalid;

    if (structure.isNull())
        return ExpressionInvalid;

    // right now, empty expressions are regarded as invalid
    if (expr.Length() == 0)
        return ExpressionInvalid;
    
    int expr_type = db->validateExpression(towstr(expr), structure).type;

    if (expr_type == xd::typeInvalid ||
        expr_type == xd::typeUndefined)
    {
        return ExpressionInvalid;
    }
     else
    {
        if (match_fieldtype != xd::typeUndefined &&
            !xd::isTypeCompatible(match_fieldtype, expr_type))
        {
            // the expression return type and specified type differ
            return ExpressionTypeMismatch;
        }
         else
        {
            return ExpressionValid;
        }
    }
    
    return -1;
}

bool StructureValidator::findInvalidExpressions(std::vector<RowErrorChecker>& vec,
                                                const xd::Structure& structure)
{
    bool found = false;
    std::vector<RowErrorChecker>::iterator it;
    for (it = vec.begin(); it != vec.end(); ++it)
    {
        if (!it->calculated_field)
            continue;

        int result = validateExpression(structure, it->expression, it->type);
        if (result == ExpressionInvalid)
        {
            it->errors |= StructureValidator::ErrorInvalidExpressions;
            found = true;
        }
        if (result == ExpressionTypeMismatch)
        {
            it->errors |= StructureValidator::ErrorExpressionTypeMismatch;
            found = true;
        }        
    }
    
    return found;
}

bool StructureValidator::findDuplicateFieldNames(std::vector<RowErrorChecker>& vec)
{
    int idx1 = 0, idx2 = 0;
    std::vector<RowErrorChecker>::iterator it, it2;
    wxString s1, s2;
    bool found = false;
    
    for (it = vec.begin(); it != vec.end(); ++it)
    {
        s1 = it->name;
        s1.Trim();
        s1.Trim(true);
        idx1++;

        // don't compare empty strings
        if (s1.IsEmpty())
            continue;
        
        idx2 = 0;
        for (it2 = vec.begin(); it2 != vec.end(); ++it2)
        {            
            s2 = it2->name;
            s2.Trim();
            s2.Trim(true);
            idx2++;
            
            // don't compare empty strings
            if (s2.IsEmpty())
                continue;

            // don't compare the same string
            if (idx1 == idx2)
                continue;
            
            if (s1.CmpNoCase(s2) == 0)
            {
                it->errors  |= StructureValidator::ErrorDuplicateFieldNames;
                it2->errors |= StructureValidator::ErrorDuplicateFieldNames;
                found = true;
                break;
            }
        }
    }
    
    return found;
}

bool StructureValidator::findDuplicateObjectNames(std::vector<RowErrorChecker>& vec)
{
    int idx1 = 0, idx2 = 0;
    std::vector<RowErrorChecker>::iterator it, it2;
    wxString s1, s2;
    bool found = false;
    
    for (it = vec.begin(); it != vec.end(); ++it)
    {
        s1 = it->name;
        s1.Trim();
        s1.Trim(true);
        
        // don't compare empty strings
        if (s1.IsEmpty())
        {
            idx1++;
            continue;
        }
        
        idx2 = 0;
        for (it2 = vec.begin(); it2 != vec.end(); ++it2)
        {
            if (idx1 == idx2)
            {
                idx2++;
                continue;
            }
            
            s2 = it2->name;
            s2.Trim();
            s2.Trim(true);
            
            // don't compare empty strings
            if (s2.IsEmpty())
            {
                idx1++;
                continue;
            }
            
            if (s1.CmpNoCase(s2) == 0)
            {
                it->errors  |= StructureValidator::ErrorDuplicateTableNames;
                it2->errors |= StructureValidator::ErrorDuplicateTableNames;
                found = true;
                break;
            }
            
            idx2++;
        }
        
        idx1++;
    }
    
    return found;
}

bool StructureValidator::findInvalidFieldNames(const xd::Structure& structure, xd::IDatabasePtr db)
{
    bool found = false;
    size_t i, count = structure.getColumnCount();
    for (i = 0; i < count; ++i)
    {
        if (!isValidFieldName(structure.getColumnName(i), db))
        {
            found = true;
            break;
        }
    }
    
    return found;
}

bool StructureValidator::findInvalidFieldNames(std::vector<RowErrorChecker>& vec, xd::IDatabasePtr db)
{
    bool found = false;
    std::vector<RowErrorChecker>::iterator it;
    for (it = vec.begin(); it != vec.end(); ++it)
    {
        if (!isValidFieldName(it->name, db))
        {
            it->errors |= StructureValidator::ErrorInvalidFieldNames;
            found = true;
        }
    }
    
    return found;
}

bool StructureValidator::findInvalidObjectNames(std::vector<RowErrorChecker>& vec, xd::IDatabasePtr db)
{
    bool found = false;
    std::vector<RowErrorChecker>::iterator it;
    for (it = vec.begin(); it != vec.end(); ++it)
    {
        if (!isValidObjectName(it->name, db))
        {
            it->errors |= StructureValidator::ErrorInvalidTableNames;
            found = true;
        }
    }
    
    return found;
}

bool StructureValidator::updateFieldWidthAndScale(int type, int* width, int* scale)
{
    int old_width = *width;
    int old_scale = *scale;

    if (type == xd::typeCharacter || type == xd::typeWideCharacter)
    {
        if (*width < xd::min_character_width)
            *width = xd::min_character_width;
        if (*width > xd::max_character_width)
            *width = xd::max_character_width;
        *scale = 0;
    }

    if (type == xd::typeNumeric)
    {
        if (*width < xd::min_numeric_width)
            *width = xd::min_numeric_width;
        if (*width > xd::max_numeric_width)
            *width = xd::max_numeric_width;
        if (*width <= *scale)
            *scale = *width - 1;
        if (*scale > xd::max_numeric_scale)
            *scale = xd::max_numeric_scale;
        if (*scale < xd::min_numeric_scale)
            *scale = xd::min_numeric_scale;
    }

    if (type == xd::typeDouble)
    {
        *width = 8;
        if (*scale > xd::max_numeric_scale)
            *scale = xd::max_numeric_scale;
        if (*scale < xd::min_numeric_scale)
            *scale = xd::min_numeric_scale;
    }

    if (type == xd::typeInteger)
    {
        *width = 4;
        *scale = 0;
    }

    if (type == xd::typeDate)
    {
        *width = 4;
        *scale = 0;
    }

    if (type == xd::typeDateTime)
    {
        *width = 8;
        *scale = 0;
    }

    if (type == xd::typeBoolean)
    {
        *width = 1;
        *scale = 0;
    }
    
    // if nothing's been updated
    if (old_width == *width && old_scale == *scale)
        return false;

    // return true for "updated"      
    return true;
}