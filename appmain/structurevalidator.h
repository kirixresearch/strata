/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-07-19
 *
 */


#ifndef __APP_STRUCTUREVALIDATOR_H
#define __APP_STRUCTUREVALIDATOR_H


class RowErrorChecker;
class StructureValidator
{
public:

    enum
    {
        ExpressionInvalid = 0,
        ExpressionValid,
        ExpressionTypeMismatch
    };

    // NOTE: most of these error codes won't use the bitmask, however,
    //       we do need it for some of the error codes below
    enum
    {
        ErrorNone                   = 0x00,
        ErrorNoFields               = 0x01,
        ErrorExpressionTypeMismatch = 0x02,
        ErrorInvalidExpressions     = 0x04,
        ErrorDuplicateFieldNames    = 0x08, // uses bitmask
        ErrorDuplicateTableNames    = 0x10, // uses bitmask
        ErrorInvalidFieldNames      = 0x20, // uses bitmask
        ErrorInvalidTableNames      = 0x40, // uses bitmask
        ErrorInvalidStructure       = 0x80
    };
    
    // this function shows the proper messagebox for the specified error code,
    // lets the caller know if this error should block the application from
    // continuing, and returns the result of the messagebox
    static int showErrorMessage(int errorcode, bool* block);
    
    // this function validates an expression based on the input structure
    static int validateExpression(tango::IStructurePtr structure,
                                  const wxString& expr,
                                  int match_fieldtype = tango::typeUndefined);
    
    // returns true/false if any invalid field names exist
    // in the specified structure based on the attributes
    // of the database that is provided
    static bool findInvalidFieldNames(tango::IStructurePtr structure,
                                      tango::IDatabasePtr db = xcm::null);
    
    // finds invalid/duplicate fields in the row checker vector,
    // specifies a corresponding error code for each row in the vector
    // and returns true/false based on if any are found (should be used
    // when you have a list of fields)
    static bool findDuplicateFieldNames(std::vector<RowErrorChecker>& vec);
    static bool findInvalidFieldNames(std::vector<RowErrorChecker>& vec,
                                      tango::IDatabasePtr db = xcm::null);
    
    // finds invalid/duplicate objects in the row checker vector,
    // specifies a corresponding error code for each row in the vector
    // and returns true/false based on if any are found (should be used
    // when you have a list of tables, scripts, etc.)
    static bool findDuplicateObjectNames(std::vector<RowErrorChecker>& vec);
    static bool findInvalidObjectNames(std::vector<RowErrorChecker>& vec,
                                       tango::IDatabasePtr db = xcm::null);
};


class RowErrorChecker
{
public:

    RowErrorChecker()
    {
        row = -1;
        name = wxEmptyString;
        errors = StructureValidator::ErrorNone;
    }
    
    RowErrorChecker(int _row, const wxString& _name)
    {
        row = _row;
        name = _name;
        errors = StructureValidator::ErrorNone;
    }
    
public:

    int row;
    int errors;
    wxString name;
};


#endif  // __APP_STRUCTUREVALIDATOR_H




