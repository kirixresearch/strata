/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-07-26
 *
 */


#ifndef H_KSCRIPT_JSERROR_H
#define H_KSCRIPT_JSERROR_H


#include "jsobject.h"


namespace kscript
{


class Error : public Object
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Error", Error, Object)
        KSCRIPT_METHOD("constructor",  Error::constructor)
    END_KSCRIPT_CLASS()
    
public:

    Error();
    
    void constructor(ExprEnv* env, Value* retval);
};

class TypeError : public Error
{
    BEGIN_KSCRIPT_DERIVED_CLASS("TypeError", TypeError, Error)
        KSCRIPT_METHOD("constructor",  TypeError::constructor)
    END_KSCRIPT_CLASS()
    
public:

    TypeError();
    void constructor(ExprEnv* env, Value* retval);
};


class SyntaxError : public Error
{
    BEGIN_KSCRIPT_DERIVED_CLASS("SyntaxError", SyntaxError, Error)
        KSCRIPT_METHOD("constructor",  SyntaxError::constructor)
    END_KSCRIPT_CLASS()
    
public:

    SyntaxError();
    void constructor(ExprEnv* env, Value* retval);
};


class EvalError : public Error
{
    BEGIN_KSCRIPT_DERIVED_CLASS("EvalError", EvalError, Error)
        KSCRIPT_METHOD("constructor",  EvalError::constructor)
    END_KSCRIPT_CLASS()
    
public:

    EvalError();
    void constructor(ExprEnv* env, Value* retval);
};


class URIError : public Error
{
    BEGIN_KSCRIPT_DERIVED_CLASS("URIError", URIError, Error)
        KSCRIPT_METHOD("constructor",  URIError::constructor)
    END_KSCRIPT_CLASS()
    
public:

    URIError();
    void constructor(ExprEnv* env, Value* retval);
};



class RangeError : public Error
{
    BEGIN_KSCRIPT_DERIVED_CLASS("RangeError", RangeError, Error)
        KSCRIPT_METHOD("constructor", RangeError::constructor)
    END_KSCRIPT_CLASS()
    
public:

    RangeError();
    void constructor(ExprEnv* env, Value* retval);
};


class ReferenceError : public Error
{
    BEGIN_KSCRIPT_DERIVED_CLASS("ReferenceError", ReferenceError, Error)
        KSCRIPT_METHOD("constructor", ReferenceError::constructor)
    END_KSCRIPT_CLASS()
    
public:

    ReferenceError();
    void constructor(ExprEnv* env, Value* retval);
};



};


#endif

