/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
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
    
    if (errorcode == ErrorNoFields)
    {
        *block = true;
        return cfw::appMessageBox(_("There are no fields in the table structure.  Add at least one field to the table to continue."),
                                  _("Invalid Table Structure"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    if (errorcode == ErrorInvalidStructure)
    {
        // NOTE: This error message should never be shown to the user
        *block = true;
        return cfw::appMessageBox(_("The source table structure is invalid or corrupt.  This file cannot be opened in the Table View."),
                                  _("Invalid Table Structure"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    // this error code can use the bitmask
    if (errorcode & ErrorDuplicateFieldNames)
    {
        *block = true;
        return cfw::appMessageBox(_("Two or more fields have the same name.  Check to make sure all field names are unique to continue."),
                                  _("Invalid Table Structure"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }
    
    // this error code can use the bitmask
    if (errorcode & ErrorDuplicateTableNames)
    {
        *block = true;
        return cfw::appMessageBox(_("Two or more tables have the same name.  Check to make sure all table names are unique to continue."),
                                  _("Duplicate Table Names"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }
    
    // this error code can use the bitmask
    if (errorcode & ErrorInvalidFieldNames)
    {
        *block = true;
        return cfw::appMessageBox(_("One or more field names are invalid.  Check to make sure all field names are valid to continue."),
                                  _("Invalid Table Structure"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    // this error code can use the bitmask
    if (errorcode & ErrorInvalidTableNames)
    {
        *block = true;
        return cfw::appMessageBox(_("One or more table names are invalid.  Check to make sure all table names are valid to continue."),
                                  _("Invalid Table Names"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    if (errorcode == ErrorInvalidExpressions)
    {
        *block = true;
        return cfw::appMessageBox(_("One or more formulas has invalid syntax.  Check to make sure all formulas have valid syntax to continue."),
                                  _("Invalid Formula(s)"),
                                  wxOK | wxICON_EXCLAMATION | wxCENTER,
                                  g_app->getMainWindow());
    }

    if (errorcode == ErrorExpressionTypeMismatch)
    {
        *block = false;
        int retcode = cfw::appMessageBox(_("One or more of the formulas has a return type that does not match the field type.  Would you like to continue?"),
                                         _("Invalid Formula(s)"),
                                         wxYES_NO | wxICON_QUESTION | wxCENTER,
                                         g_app->getMainWindow());
        if (retcode == wxNO)
            *block = true;
        
        return retcode;
    }

    *block = false;
    return -1;
}

int StructureValidator::validateExpression(tango::IStructurePtr structure,
                                           const wxString& expr,
                                           int match_fieldtype)
{
    if (structure.isNull())
        return -1;
    
    int expr_type = structure->getExprType(towstr(expr));
    if (expr_type == tango::typeInvalid ||
        expr_type == tango::typeUndefined)
    {
        return ExpressionInvalid;
    }
     else
    {
        if (match_fieldtype != tango::typeUndefined &&
            !tango::isTypeCompatible(match_fieldtype, expr_type))
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
                it->errors  |= StructureValidator::ErrorDuplicateFieldNames;
                it2->errors |= StructureValidator::ErrorDuplicateFieldNames;
                found = true;
                break;
            }
            
            idx2++;
        }
        
        idx1++;
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

bool StructureValidator::findInvalidFieldNames(tango::IStructurePtr structure,
                                               tango::IDatabasePtr db)
{
    bool found = false;
    size_t i, count = structure->getColumnCount();
    for (i = 0; i < count; ++i)
    {
        tango::IColumnInfoPtr col = structure->getColumnInfoByIdx(i);
        if (!isValidFieldName(towx(col->getName()), db))
        {
            found = true;
            break;
        }
    }
    
    return found;
}

bool StructureValidator::findInvalidFieldNames(std::vector<RowErrorChecker>& vec,
                                               tango::IDatabasePtr db)
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

bool StructureValidator::findInvalidObjectNames(std::vector<RowErrorChecker>& vec,
                                                tango::IDatabasePtr db)
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
