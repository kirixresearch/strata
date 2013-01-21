/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-07-26
 *
 */


#include "kscript.h"
#include "jserror.h"


namespace kscript
{


// note don't document this class until better exception objects are implemented

// (CLASS) Error
// Category: Core
// Description: A class for storing error-related information.
// Remarks: A class that contains error information.

Error::Error()
{
}

// (CONSTRUCTOR) Error.constructor
// Description: Creates a new Error object.
//
// Syntax: Error()
//
// Remarks: Creates a new error object.

void Error::constructor(ExprEnv* env, Value* retval)
{
    getMember(L"name")->setString(L"Error");
}




// (CLASS) TypeError
// Category: Core
// Derives: Error
// Description: A class that represents a type error condition.
// Remarks: A class that represents a type error condition.

TypeError::TypeError()
{
    setClassName(L"Error");
    getMember(L"name")->setString(L"TypeError");
}

// (CONSTRUCTOR) TypeError.constructor
// Description: Creates a new TypeError object.
//
// Syntax: TypeError()
//
// Remarks: Creates a new TypeError object.

void TypeError::constructor(ExprEnv* env, Value* retval)
{
}



// (CLASS) EvalError
// Category: Core
// Derives: Error
// Description:  A class that represents a eval error condition.
// Remarks: A class that represents a eval error condition.

EvalError::EvalError()
{
    setClassName(L"Error");
    getMember(L"name")->setString(L"EvalError");
}

// (CONSTRUCTOR) EvalError.constructor
// Description: Creates a new EvalError object.
//
// Syntax: EvalError()
//
// Remarks: Creates a new EvalError object.

void EvalError::constructor(ExprEnv* env, Value* retval)
{
}



// (CLASS) URIError
// Category: Core
// Derives: Error
// Description:  A class that represents a uri error condition.
// Remarks: A class that represents a uri error condition.

URIError::URIError()
{
    setClassName(L"Error");
    getMember(L"name")->setString(L"URIError");
}

// (CONSTRUCTOR) URIError.constructor
// Description: Creates a new URIError object.
//
// Syntax: URIError()
//
// Remarks: Creates a new URIError object.

void URIError::constructor(ExprEnv* env, Value* retval)
{
}












// (CLASS) SyntaxError
// Category: Core
// Derives: Error
// Description:  A class that represents a syntax error condition.
// Remarks: A class that represents a syntax error condition.

SyntaxError::SyntaxError()
{
    setClassName(L"Error");
    getMember(L"name")->setString(L"SyntaxError");
}

// (CONSTRUCTOR) SyntaxError.constructor
// Description: Creates a new SyntaxError object.
//
// Syntax: SyntaxError()
//
// Remarks: Creates a new SyntaxError object.

void SyntaxError::constructor(ExprEnv* env, Value* retval)
{
}




// (CLASS) RangeError
// Category: Core
// Derives: Error
// Description:  A class that represents a range error condition.
// Remarks: A class that represents a range error condition.

RangeError::RangeError()
{
    setClassName(L"Error");
    getMember(L"name")->setString(L"RangeError");
}

// (CONSTRUCTOR) RangeError.constructor
// Description: Creates a new RangeError object.
//
// Syntax: RangeError()
//
// Remarks: Creates a new RangeError object.

void RangeError::constructor(ExprEnv* env, Value* retval)
{
}



// (CLASS) ReferenceError
// Category: Core
// Derives: Error
// Description:  A class that represents a reference error condition.
// Remarks: A class that represents a reference error condition.

ReferenceError::ReferenceError()
{
    setClassName(L"Error");
    getMember(L"name")->setString(L"ReferenceError");
}

// (CONSTRUCTOR) ReferenceError.constructor
// Description: Creates a new ReferenceError object.
//
// Syntax: ReferenceError()
//
// Remarks: Creates a new ReferenceError object.

void ReferenceError::constructor(ExprEnv* env, Value* retval)
{
}







};

