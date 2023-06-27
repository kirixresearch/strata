/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-05-15
 *
 */


#include "kscript.h"
#include "jsregexp.h"
#include "jsarray.h"


namespace kscript
{


RegExp::RegExp()
{
}

RegExp::~RegExp()
{
}
    

bool RegExp::assign(const std::wstring& regex_src, const std::wstring& flags)
{
    bool global = false;
    bool ignoreCase = false;
    bool multiline = false;
    
    const wchar_t* fp = flags.c_str();
    while (*fp)
    {
        switch (*fp)
        {
            case L'g': global = true; break;
            case L'i': ignoreCase = true; break;
            case L'm': multiline = true; break;
            default:
                return false;
        }
        fp++;
    }
    
    getMember(L"global")->setBoolean(global);
    getMember(L"ignoreCase")->setBoolean(ignoreCase);
    getMember(L"multiline")->setBoolean(multiline);
    getMember(L"source")->setString(regex_src);
    getMember(L"lastIndex")->setInteger(0);

    std::wstring regex;
    if (ignoreCase)
        regex += L"(?i)";
    if (multiline)
        regex += L"(?m)";
    regex += regex_src;
    
    return m_regex.assign(regex);
}


// (CLASS) RegExp
// Category: Core
// Derives: Object
// Description: A class that represents a regular expression.
// Remarks: A class that represents a regular expression.

// (CONSTRUCTOR) RegExp.constructor
// Description: Creates a new RegExp object.
//
// Syntax: RegExp(pattern : String)
// Syntax: RegExp(pattern : String, flags : String)
//
// Remarks: Creates a regular expression object from the specified
//     |pattern| and |flags|.
//
// Param(pattern): A string that contains a regular expression.
// Param(flags): A String that contains of the avalable flags "g", "i" and "m".
//     "g" means that the search is global, "i" that the search is case-insensitive 
//     and "m" switches to multiline mode.

void RegExp::constructor(ExprEnv* env, Value* retval)
{
    std::wstring regex;
    std::wstring flags;
    
    
    if (env->getParamCount() >= 1 && env->getParam(0)->isObject())
    {
        ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(RegExp::staticGetClassId()))
        {
            if (!env->getParam(1)->isNull())
            {
                env->setRuntimeError(rterrorTypeError);
                return;
            }
            
            // 'copy constructor' for regexs
            // e.g. new RegExp(/(?:)/) or similar
            RegExp* r = (RegExp*)obj;
            
            if (r->getMember(L"global")->getBoolean())
                flags += L"g";
            if (r->getMember(L"ignoreCase")->getBoolean())
                flags += L"i";
            if (r->getMember(L"multiline")->getBoolean())
                flags += L"m";
            regex = r->getMember(L"source")->getString();
        }
    }
    
    if (regex.length() == 0)
    {
        if (env->getParamCount() >= 1)
            regex = env->getParam(0)->getString();
        
        if (env->getParamCount() >= 2)
            flags = env->getParam(1)->getString();
    }
        

    if (!assign(regex, flags))
    {
        env->setRuntimeError(rterrorSyntaxError);
    }
}


void RegExp::staticConstructor(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() > 0)
    {
        if (env->getParam(0)->isObject())
        {
            // check if it's a RegExp object, pass it through if it is
            // (ECMA-262 15.10.3.1)
            kscript::ValueObject* vobj = env->getParam(0)->getObject();
            if (vobj->isKindOf(L"RegExp"))
            {
                retval->setValue(env->getParam(0));
                return;
            }     
        }
    }
    
    RegExp* retobj = RegExp::createObject(env);
    retobj->constructor(env, retval);
    retval->setObject(retobj);
}




// (METHOD) RegExp.exec
// Description: Searches a string for a pattern.
//
// Syntax: function RegExp.exec(text : String) : Array(String)
//
// Remarks: Searches the |text| for the string that the regular
//     expression represents and returns the results in an Array.
//     Returns null if no text match was found.
//
// Param(text): The |text| to search with the regular expression.
//
// Returns: Returns an Array containing the results of searching |text|
//     for the regular expression.  Returns null if no text match was 
//     found.

void RegExp::exec(ExprEnv* env, Value* retval)
{
    if (m_regex.empty() || env->getParamType(0) != Value::typeString)
    {
        retval->setNull();
        return;
    }
    
    
    int i = 0; // search start position
    
    kscript::Value* global = getMember(L"global");
    kscript::Value* lastIndex = getMember(L"lastIndex");
    
    if (global->getBoolean())
    {
        i = lastIndex->getInteger();
    }
    
    
    const wchar_t* str = env->m_eval_params[0]->getString();
    int len = env->m_eval_params[0]->getDataLen();
    
    if (i < 0 || i > len)
    {
        lastIndex->setInteger(0);
        retval->setNull();
        return;
    }
    
    
    klregex::wmatch results;
    if (!m_regex.search(str+i, results))
    {
        lastIndex->setInteger(0);
        retval->setNull();
        return;
    }
    
    klregex::wsubmatch submatch = results.getSubMatch(0);
    lastIndex->setInteger((int)(submatch.second - str));
    
    
    retval->setObject(Array::createObject(env));

    size_t si, submatch_count = results.size();
    for (si = 0; si < submatch_count; ++si)
    {
        if (results[si].name.length() > 0)
            retval->getMember(results[si].name)->setString(results[si].str().c_str());
             else
            retval->getMemberI((int)si)->setString(results[si].str().c_str());
    }
}


// (METHOD) RegExp.test
// Description: Searches a string for a pattern.
//
// Syntax: function RegExp.test(text : String) : Boolean
//
// Remarks: Calling the test() method determines if the regular expression
//     matches the string specified in the |text| parameter.  If so, the
//     call returns |true|.  It is identical to "RegExp.exec(text) != null"
//
// Param(text): The |text| to search with the regular expression.
//
// Returns: Returns |true| if the pattern matches, |false| otherwise.

void RegExp::test(ExprEnv* env, Value* retval)
{
    if (m_regex.empty() || env->getParamType(0) != Value::typeString)
    {
        retval->setNull();
        return;
    }
    
    Value exec_retval;
    exec(env, &exec_retval);
    retval->setBoolean(exec_retval.isNull() ? false : true);
}


// (METHOD) RegExp.toString
// Description: Returns a string that represents the regular expression
//
// Syntax: function RegExp.toString() : String
//
// Remarks: Returns a string that represents the regular expression.
//
// Returns: Returns a string containing the regular expression.

void RegExp::toString(ExprEnv* env, Value* retval)
{
    toString(retval);
}

void RegExp::toString(Value* val)
{
    const wchar_t* src_expr = getMember(L"source")->getString();
    
    std::wstring str;
    str = L"/";
    while (*src_expr)
    {
        if (*src_expr == L'/')
            str += '\\';
        str += *src_expr;
        src_expr++;
    }
    str += L"/";
    
    if (getMember(L"global")->getBoolean())
        str += L"g";
    if (getMember(L"ignoreCase")->getBoolean())
        str += L"i";
    if (getMember(L"multiline")->getBoolean())
        str += L"m";
        
    val->setString(str);
}



};
