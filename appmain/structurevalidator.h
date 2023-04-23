/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-07-19
 *
 */


#ifndef H_APP_STRUCTUREVALIDATOR_H
#define H_APP_STRUCTUREVALIDATOR_H


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
    static int validateExpression(const xd::Structure& structure,
                                  const wxString& expr,
                                  int match_fieldtype = xd::typeUndefined);

    // finds invald expressions in the row check vector, specifies
    // a corresponding error code for each row in the vector and returns 
    // true/false based on if any are found (should be used
    // when you have a list of expressions)    
    static bool findInvalidExpressions(std::vector<RowErrorChecker>& vec,
                                       const xd::Structure& structure);
    
    // returns true/false if any invalid field names exist
    // in the specified structure based on the attributes
    // of the database that is provided
    static bool findInvalidFieldNames(const xd::Structure& structure,
                                      xd::IDatabasePtr db = xcm::null);
    
    // finds invalid/duplicate fields in the row checker vector,
    // specifies a corresponding error code for each row in the vector
    // and returns true/false based on if any are found (should be used
    // when you have a list of fields)
    static bool findDuplicateFieldNames(std::vector<RowErrorChecker>& vec);
    static bool findInvalidFieldNames(std::vector<RowErrorChecker>& vec,
                                      xd::IDatabasePtr db = xcm::null);

    // finds invalid/duplicate objects in the row checker vector,
    // specifies a corresponding error code for each row in the vector
    // and returns true/false based on if any are found (should be used
    // when you have a list of tables, scripts, etc.)
    static bool findDuplicateObjectNames(std::vector<RowErrorChecker>& vec);
    static bool findInvalidObjectNames(std::vector<RowErrorChecker>& vec,
                                       xd::IDatabasePtr db = xcm::null);
                                       
    static bool updateFieldWidthAndScale(int type, int* width, int* scale);
};


class RowErrorChecker
{
public:

    RowErrorChecker()
    {
        row = -1;
        name = wxEmptyString;
        type = xd::typeInvalid;
        expression = wxEmptyString;
        calculated_field = false;        
        errors = StructureValidator::ErrorNone;
    }
    
    RowErrorChecker(int _row, const wxString& _name)
    {
        row = _row;
        name = _name;
        type = xd::typeInvalid;
        expression = wxEmptyString;
        calculated_field = false;
        errors = StructureValidator::ErrorNone;
    }

    RowErrorChecker(int _row, const wxString& _name, int _type, const wxString& _expression, bool _calculated_field)
    {
        row = _row;
        name = _name;
        type = _type;
        expression = _expression;
        calculated_field = _calculated_field;
        errors = StructureValidator::ErrorNone;
    }
    
public:

    int row;
    int errors;
    
    wxString name;
    wxString expression;    
    int type;
    bool calculated_field;
};


#endif  // __APP_STRUCTUREVALIDATOR_H




