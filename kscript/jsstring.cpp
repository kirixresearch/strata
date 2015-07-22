/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-01-23
 *
 */


#include "kscript.h"
#include "jsstring.h"
#include "jsregexp.h"
#include "jsarray.h"
#include "util.h"
#include "../kl/include/kl/regex.h"


// MS VC++ 6 compatibility
#ifdef _MSC_VER
#ifndef swprintf
#define swprintf _snwprintf
#endif
#endif


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


namespace kscript
{


static Value* getThisString(ExprEnv* env)
{
    Value* ethis = env->getThis();
    if (!ethis)
        return NULL;
        
    if (ethis->isObject())
    {
        ValueObject* vobj = ethis->getObject();
        if (vobj->isKindOf(String::staticGetClassId()))
            return &((String*)vobj)->val;
    }
    
    return ethis;
}



struct StringMethod
{
    const wchar_t* name;
    Value val;
    
    void set(const wchar_t* _name, ExprBindFunc _func)
    {
        name = _name;
        val.setFunction(_func);
    }
};


const int STRING_METHOD_COUNT = 24;
static StringMethod g_string_methods[STRING_METHOD_COUNT];

static void populateStringMethods()
{
    if (g_string_methods[0].val.isUndefined())
    {
        g_string_methods[0].set(L"valueOf",            String::toString);
        g_string_methods[1].set(L"toString",           String::toString);
        g_string_methods[2].set(L"charAt",             String::charAt);
        g_string_methods[3].set(L"charCodeAt",         String::charCodeAt);
        g_string_methods[4].set(L"concat",             String::concat);
        g_string_methods[5].set(L"indexOf",            String::indexOf);
        g_string_methods[6].set(L"lastIndexOf",        String::lastIndexOf);
        g_string_methods[7].set(L"localeCompare",      String::localeCompare);
        g_string_methods[8].set(L"match",              String::match);
        g_string_methods[9].set(L"replace",            String::replace);
        g_string_methods[10].set(L"search",            String::search);
        g_string_methods[11].set(L"setCharAt",         String::setCharAt);
        g_string_methods[12].set(L"slice",             String::slice);
        g_string_methods[13].set(L"split",             String::split);
        g_string_methods[14].set(L"substring",         String::substring);
        g_string_methods[15].set(L"trim",              String::trim);
        g_string_methods[16].set(L"toLowerCase",       String::toLowerCase);
        g_string_methods[17].set(L"toUpperCase",       String::toUpperCase);
        g_string_methods[18].set(L"toLocaleUpperCase", String::toUpperCase);
        g_string_methods[19].set(L"toLocaleLowerCase", String::toLowerCase);
        g_string_methods[20].set(L"substr",            String::substr);
        g_string_methods[21].set(L"compare",           String::compare);
        g_string_methods[22].set(L"frequency",         String::frequency);
        g_string_methods[23].set(L"fromCharCode",      String::fromCharCode);
        // when you add a method, make sure to increment STRING_METHOD_COUNT above
    }
}





// (CLASS) String
// Category: Core
// Derives: Object
// Description: A class that represents string-typed data.
// Remarks: A class that represents string-typed data.

String::String()
{
    val.setString(L"");
}

// (CONSTRUCTOR) String.constructor
// Description: Creates a new String object.
//
// Syntax: String()
// Syntax: String(value : String)
//
// Remarks: Creates a new String object from |value|. If no value is
//     specified, the initial |value| of the String object is "".
//
// Param(value): The |value| of the new String object.

void String::constructor(ExprEnv* env, Value* retval)
{
    if (env->m_param_count == 1)
    {
        if (env->getParamType(0) == Value::typeString)
        {
            env->getParam(0)->eval(env, &val);
        }
         else
        {
            Value v;
            env->getParam(0)->eval(env, &v);
            v.toString(&val);
        }
    }
     else
    {
        val.setString(L"");
    }
}


void String::staticConstructor(
                        kscript::ExprEnv* env,
                        void* param,
                        kscript::Value* retval)
{
    if (env->m_param_count == 1)
    {
        if (env->getParamType(0) == Value::typeString)
        {
            env->getParam(0)->eval(env, retval);
        }
         else
        {
            Value v;
            env->getParam(0)->eval(env, &v);
            v.toString(retval);
        }
    }
     else
    {
        retval->setString(L"");
    }
}





Value* String::lookupStringMethod(const std::wstring& method)
{
    populateStringMethods();
    
    for (size_t i = 0; i < STRING_METHOD_COUNT; ++i)
    {
        if (method == g_string_methods[i].name)
            return &g_string_methods[i].val;
    }
    
    return NULL;
}

// TODO: document length parameter    
Value* String::getMember(const std::wstring& name)
{
    if (ValueObject::getMemberExists(name))
        return ValueObject::getMember(name);
        
    // look for length
    if (name == L"length")
    {
        Value* v = getValueReturn();
        v->setInteger(val.getDataLen());
        return v;
    }
    
    if (name.length() > 0 && iswdigit(name[0]))
    {
        int offset = wtoi(name.c_str());
        if (offset >= 0 && offset <= val.getDataLen())
        {
            Value* v = getValueReturn();
            v->setString(val.getString()+offset, 1);
            return v;
        }
         else
        {
            Value* v = getValueReturn();
            v->setNull();
            return v;
        }
    }
    
    Value* member = ValueObject::getMember(name);
    
    Value* func = lookupStringMethod(name);
    if (func == NULL)
        return member;
        
    member->setValue(func);
    return member;
}



void String::toString(Value* retval)
{
    retval->setString(&val);
}

void String::toNumber(Value* retval)
{
    parseNumericConstant(val.getString(), retval, NULL);
}

void String::toPrimitive(Value* retval, int preferred_type)
{
    retval->setString(&val);
    //toString(retval);
}


// (METHOD) String.fromCharCode
// Description: Returns a string created by converting all arguments to 
//     characters and concatenating them.
//
// Syntax: static function String.fromCharCode(char_1 : Integer, 
//                                             ..., 
//                                             char_n : Integer) : String
//
// Remarks: Returns a string created by converting all arguments to 
//     characters and concatenating them.
//
// Param(char_1, ..., char_n): A number of integers that are the unicode character 
//     encodings of the characters to which they are to be converted.
//
// Returns: Returns a string created by converting all arguments to 
//     characters and concatenating them.

void String::fromCharCode(ExprEnv* env, void* param, Value* retval)
{
    size_t i, param_count = env->getParamCount();
    
    std::wstring result;
    result.reserve(param_count);
    
    for (i = 0; i < param_count; ++i)
    {
        Value* param = env->getParam(i);

        ks_uint64_t uchar_code = (ks_uint64_t)(ks_int64_t)param->getDouble();
        unsigned int j = (unsigned int)(uchar_code % 65536);
        result += (wchar_t)j;
    }
    
    retval->setString(result);
}

// note: the following function js_String_toString is both String.valueOf() 
// and String.toString()

// (METHOD) String.valueOf
// Description: Returns the primitive string value.
// Syntax: function String.valueOf() : String
// Remarks: Returns the primitive string value associated with the String.
// Returns:  Returns the primitive string value associated with the String.

// (METHOD) String.toString
// Description: Returns the primitive string value.
// Syntax: function String.toString() : String
// Remarks: Returns the primitive string value associated with the String.
// Returns:  Returns the primitive string value associated with the String.

void String::toString(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }
    
    vthis->eval(env, retval);
}

// (METHOD) String.charAt
// Description: Returns the character at the specified position.
//
// Syntax: function String.charAt(index : Integer) : String
//
// Remarks: Returns the character at the specified, zero-based |index|
//     in the string.
//
// Param(index): The character to return at the position given by |index|.
// Returns: Returns the character at the specified position.

void String::charAt(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    if (env->m_param_count < 1)
    {
        retval->setString(L"");
        return;
    }

    double idx = env->m_eval_params[0]->getDouble();
    
    wchar_t* p = vthis->getString();
    int len = vthis->getDataLen();
    
    if ((dblcompare(idx, 0.0) < 0 || dblcompare(idx, len) >= 0) && !isnan(idx)) 
    {
        retval->setString(L"");
        return;
    }

    int i = env->m_eval_params[0]->getInteger();

    wchar_t ret[2];
    ret[0] = *(p+i);
    ret[1] = 0;

    retval->setString(ret, 1);
}

// (METHOD) String.charCodeAt
// Description: Returns the unicode encoding of the character at the 
//     specified position.
//
// Syntax: function String.charCodeAt(index : Integer) : Integer
//
// Remarks: Returns the unicode encoding of the character at the 
//     specified |index|.
//
// Param(index): The zero-based |index| of the character of the string whose 
//     unicode encoding is to be returned.
//
// Returns: Returns the unicode encoding of the character at the 
//     specified |index|.

void String::charCodeAt(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    if (env->m_param_count < 1)
    {
        retval->setNaN();
        return;
    }

    double idx = env->m_eval_params[0]->getDouble();

    wchar_t* p = vthis->getString();
    int len = vthis->getDataLen();
    
    if ((dblcompare(idx, 0.0) < 0 || dblcompare(idx, len) >= 0) && !isnan(idx)) 
    {
        retval->setNaN();
        return;
    }

    int i = env->m_eval_params[0]->getInteger();
    retval->setInteger((int)*(p+i));
}

// (METHOD) String.concat
// Description: Concatenates a number of values to the string.
//
// Syntax: function String.concat(str_1 : String, 
//                                ..., 
//                                str_n : String) : String
//
// Remarks: Concatenates the elements |str_1|, ..., |str_n| with the current
//     string and returns the results as a new string.
//
// Param(str_1, ..., str_n): The elements to concatenate with this string.
//
// Returns: A string resulting from concatenating the current string with the
//     input elements.

void String::concat(ExprEnv* env, void* param, Value* retval)
{
    if (!env->m_val_this)
    {
        retval->setNull();
        return;
    }
    
    retval->setValue(env->m_val_this);
        
    size_t i, param_count = env->getParamCount();
    for (i = 0; i < param_count; ++i)
    {
        Value* v = env->getParam(i);
        if (v->isString())
        {
            retval->appendString(v->getString(), v->getDataLen());
        }
         else
        {
            Value str;
            v->toString(&str);
            retval->appendString(str.getString(), str.getDataLen());
        }
    }
}

// (METHOD) String.indexOf
// Description: Returns the position of the first occurrence of the specified 
//     text in the string.
//
// Syntax: function String.indexOf(text : String) : Integer
// Syntax: function String.indexOf(text : String,
//                                 index : Integer) : Integer
//
// Remarks: Returns the position of the first occurrence of |text| in the
//     string after the starting |index|. If |text| cannot be found, the
//     function returns -1.
//
// Param(text): The |text| to search for in the string.
// Param(index): The |index| at which to start the search. If this argument 
//     is not supplied, 0 is taken as the default value.
//
// Returns: Returns the position of the first occurrence of |text| in the
//     string after the starting |index|. If the |text| cannot be found, the
//     function returns -1.

void String::indexOf(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }
    
    int start_index = 0;
    wchar_t* str = vthis->getString();

    
    if (env->m_param_count == 0)
    {
        retval->setInteger(-1);
        return;
    }
    
    if (env->m_param_count >= 2)
    {
        start_index = env->m_eval_params[1]->getInteger();
        if (start_index < 0)
            start_index = 0;
    }


    kscript::Value tostr;
    wchar_t* search_str;
    int search_str_len;
    if (env->m_eval_params[0]->isString())
    {
        search_str = env->m_eval_params[0]->getString();
        search_str_len = env->m_eval_params[0]->getDataLen();
    }
     else
    {
        env->m_eval_params[0]->toString(&tostr);
        search_str = tostr.getString();
        search_str_len = tostr.getDataLen();
    }
    


    int str_len = vthis->getDataLen();
    if (start_index > str_len)
    {
        retval->setInteger(-1);
        return;
    }

    if (search_str_len == 0)
    {
        retval->setInteger(start_index);
        return;
    }
    
    wchar_t* str2 = wcsstr(str + start_index, search_str);
    
    if (str2)
    {
        retval->setInteger((int)(str2 - str));
    }
     else
    {
        retval->setInteger(-1);
    }
}

// (METHOD) String.lastIndexOf
// Description: Returns the position of the last occurrence of the specified 
//     text in the string.
//
// Syntax: function String.lastIndexOf(text : String) : Integer
// Syntax: function String.lastIndexOf(text : String,
//                                     index : Integer) : Integer
//
// Remarks: Returns the position of the last occurrence of |text| in the
//     string after the starting |index|. If |text| cannot be found, the
//     function returns -1.
//
// Param(text): The |text| to search for in the string.
// Param(index): The |index| at which to start the search. If this argument 
//     is not supplied, 0 is taken as the default value.
//
// Returns: Returns the position of the last occurrence of |text| in the
//     string after the starting |index|. If the |text| cannot be found, the
//     function returns -1.

void String::lastIndexOf(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    int start_index = 99999999;
    wchar_t* str = vthis->getString();
    int len = vthis->getDataLen();

    if (env->m_param_count == 0)
    {
        retval->setInteger(-1);
        return;
    }

    if (env->m_param_count >= 2)
    {
        if (env->m_eval_params[1]->isNumber())
        {
            if (env->m_eval_params[1]->isNaN())
                start_index = 99999999;
                 else
                start_index = env->m_eval_params[1]->getInteger();
        }
         else
        {
            Value v;
            env->m_eval_params[1]->toNumber(&v);
            if (v.isNaN())
                start_index = 99999999;
                 else
                start_index = v.getInteger();
        }
       
        if (start_index < 0)
        {
            retval->setInteger(-1);
            return;
        }
    }

    kscript::Value tostr;
    wchar_t* search_str;
    int search_str_len;
    
    if (env->m_eval_params[0]->isString())
    {
        search_str = env->m_eval_params[0]->getString();
        search_str_len = env->m_eval_params[0]->getDataLen();
    }
     else
    {
        env->m_eval_params[0]->toString(&tostr);
        search_str = tostr.getString();
        search_str_len = tostr.getDataLen();
    }
    
    // if an input search string is specified with length
    // zero, return the length of this string
    if (search_str_len == 0)
    {
        retval->setInteger(len);
        return;
    }

    int i;
    bool found = false;

    // if the current starting index is past the end 
    // of the string, set it to the last character
    // in the string
    if (start_index >= len)
        start_index = len - 1;

    for (i = start_index; i >= 0; --i)
    {
        if (0 == wcsncmp(str+i, search_str, search_str_len))
        {
            found = true;
            break;
        }
    }
    
    if (found)
    {
        retval->setInteger(i);
        return;
    }
     else
    {
        retval->setInteger(-1);
    }
}


// TODO: non-standard function; should we document this?
typedef int (*StrCompareFunc)(const wchar_t*, const wchar_t*);

static void js_String_compare_helper(ExprEnv* env,
                                     void* param,
                                     Value* retval,
                                     StrCompareFunc strcmp_func)
{
    kscript::Value* str1;
    kscript::Value* str2;
    
    if (env->m_param_count == 0)
    {
        retval->setInteger(-1);
        return;
    }
     else if (env->m_param_count == 1)
    {
        if (!env->m_val_this)
        {
            retval->setInteger(-1);
            return;
        }

        str1 = env->m_val_this;
        str2 = env->getParam(0);
    }
     else if (env->m_param_count >= 2)
    {
        str1 = env->getParam(0);
        str2 = env->getParam(1);
    }

    retval->setInteger(strcmp_func(str1->getString(), str2->getString()));
}

// (METHOD) String.localeCompare
// Description: Returns an integer indicating whether the current string is 
//     less than, equal to, or greater than a specified.
//
// Syntax: function String.localeCompare(string : String) : Integer
// Syntax: static function String.localeCompare(string1 : String,
//                                              string2 : String) : Integer
//
// Remarks: Returns an integer indicating whether the current string is 
//     less than, equal to, or greater than a specified |string|.  If the
//     current string is less than the specified |string|, the return result
//     is negative; if the current string is equal to the specified |string|, 
//     the return result is zero; if the current string is greater than the 
//     specified |string|, the return result is positive. If the static function
//     is used, |string1| is compared with |string2|. Note: this function is 
//     equivalent to compare().
//
// Param(string): The |string| to which to compare the current string.
//
// Returns: Returns an integer indicating whether the current string is 
//     less than, equal to, or greater than a specified.

void String::localeCompare(ExprEnv* env, void* param, Value* retval)
{
    js_String_compare_helper(env, param, retval, wcscoll);
}

// (METHOD) String.match
// Description: Searches for the specified pattern in the string.
//
// Syntax: function String.match(regexp : RegExp) : Array(String)
//
// Remarks: Searches the string for the regular expression specified
//     by |regexp| and returns the results of the match in an array. 
//     If no matches are found, the function returns null.
//
// Param(regexp): A regular expression with which to search the 
//     current string.
//
// Returns: Returns an array containing the results of searching the
//     current string with the regular expression |regexp|. Returns
//     null if no matches are found.

void String::match(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }
    
    if (vthis == NULL || env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }

    RegExp* regexp = NULL;
    bool should_delete = false;


    klregex::wmatch results;
    
    if (env->getParamType(0) == Value::typeObject)
    {
        ValueObject* obj = env->getParam(0)->getObject();
        if (obj && obj->isKindOf(L"RegExp"))
        {
            regexp = (RegExp*)obj;
        }     
    }
    
    
    if (!regexp)
    {
        regexp = new RegExp;
        regexp->m_regex.assign(env->getParam(0)->getString());
        should_delete = true;
    }
    
    
    kscript::ExprEnv* nenv = env->getParser()->createEnv();
    nenv->reserveParams(1);
    nenv->m_eval_params[0]->setValue(vthis);
    regexp->exec(nenv, retval);
    nenv->unref();

    if (should_delete)
    {
        delete regexp;
    }
}




static size_t replaceStr(std::wstring& str,
                         const std::wstring& search,
                         const std::wstring& replace,
                         bool global = true)
{
    size_t offset = 0;
    size_t search_len = search.length();
    size_t replace_len = replace.length();
    size_t num_replaces = 0;
    
    while (1)
    {
        size_t pos = str.find(search, offset);
        if (pos == str.npos)
            break;
            
        str.erase(pos, search_len);
        str.insert(pos, replace);
        
        offset += replace_len;
        num_replaces++;
        
        if (search_len == 0 || !global)
            break;
    }
    
    return num_replaces;
}


// (METHOD) String.replace
// Description: Replaces matching text in a string with the text 
//     of another string.
//
// Syntax: function String.replace(param : RegExp,
//                                 text : String) : String
// Syntax: function String.replace(param : String,
//                                 text : String) : String
//
// Remarks: This function returns a new string, where the text
//     in the new string is the text of the original string with
//     the portions of replaced text.  The portions of replaced
//     text are determined by finding matches in the original
//     string with the |param|, where the |param| is either a
//     regular expression or literal text.
//
// Param(param): A String or RegExp that is to be used to search
//     the current string and whose matches are to be replaced
//     with |text|.  If param is a RegExp and the global flag
//     is set, all matching strings are replaced.  If param is
//     is a String, only the first matching string is replaced.
//
// Param(text): The |text| that is to replace the matches of |param|.
//
// Returns: A new string consisting of the text of the original
//     String, with one or more occurrences of |param| replaced 
//     with |text|.

void String::replace(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis || env->getParamCount() < 1)
    {
        if (vthis)
            retval->setValue(vthis);
             else
            retval->setUndefined();
        return;
    }
    
    std::wstring str = vthis->getString();
    std::wstring replace_str = env->getParam(1)->getString();
    
 
    if (env->getParam(0)->isObject() &&
        env->getParam(0)->getObject()->isKindOf(L"RegExp"))
    {
        ValueObject* obj = env->getParam(0)->getObject();
        RegExp* r = (RegExp*)obj;
        r->m_regex.replace(str, replace_str, r->getMember(L"global")->getBoolean());
    }
     else
    {
        Value v;
        env->getParam(0)->toString(&v);

        replaceStr(str, v.getString(), replace_str, false);
    }

    retval->setString(str.c_str(), str.length());
}

// (METHOD) String.search
// Description: Searches for the specified pattern in the string.
//
// Syntax: function String.search(regexp : RegExp) : Integer
//
// Remarks: Returns the position of the first match of |regexp| or -1
//     if no match was found.
//
// Param(regexp): A regular expression object that is to be used to 
//     search the current string.
//
// Returns: Returns the position of the first match of |regexp| or -1
//     if no match was found.

void String::search(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis || env->getParamCount() < 1)
    {
        retval->setInteger(-1);
        return;
    }
    
    klregex::wmatch results;
    
    if (env->getParamType(0) == Value::typeObject)
    {
        ValueObject* obj = env->getParam(0)->getObject();
        if (obj && obj->isKindOf(L"RegExp"))
        {
            RegExp* r = (RegExp*)obj;

            if (!r->m_regex.search(vthis->getString(), results))
            {
                retval->setInteger(-1);
                return;
            } 
            
            retval->setInteger(results.getSubMatch(0).first - results.src_begin);
            return;
        }
    }
    
    Value regex_str_val;
    env->getParam(0)->toString(&regex_str_val);
       
    // NOTE: this is quite inefficient because the regex
    // gets compiled each time
    klregex::wregex regex;
    
    if (!regex.assign(regex_str_val.getString()))
    {
        env->setRuntimeError(rterrorSyntaxError);
        return;
    }
    
    if (!regex.search(vthis->getString(), results))
    {
        retval->setInteger(-1);
        return;
    }
            
    retval->setInteger(results.getSubMatch(0).first - results.src_begin);
}

// (METHOD) String.slice
// Description: Returns a substring of the string.
//
// Syntax: function String.slice(start : Integer,
//                               end : Integer) : String
//
// Remarks: A string containing the characters between |start| and 
//     |end| including the former but excluding the latter. If |start| 
//     or |end| is negative, the result of adding the negative number 
//     to the length of string is used.
//
// Param(start): The position at which the slice is to |start|.
// Param(end): The position before which the slice is to |end|.
//
// Returns: A string containing the characters between |start| and 
//     |end| including the former but excluding the latter. If |start| 
//     or |end| is negative, the result of adding the negative number 
//     to the length of string is used.

void String::slice(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }
    
    const wchar_t* str = vthis->getString();
    int len = (int)wcslen(str);
    int start_off = 0;
    int end_off = 0;
    
    if (env->getParamCount() > 0)
    {
        start_off = env->getParam(0)->getInteger();
    }
    
    if (env->getParamCount() > 1)
    {
        if (env->getParam(1)->isUndefined())
        {
            end_off = len;
        }
         else
        {
            end_off = env->getParam(1)->getInteger();
        }
    }
     else
    {
        end_off = len;
    }
    
    if (start_off < 0)
    {
        start_off = len + start_off;
    }
    if (start_off < 0)
    {
        start_off = 0;
    }
    
    if (end_off < 0)
    {
        end_off = len + end_off;
    }
    
    if (start_off >= len ||
        end_off <= start_off)
    {
        retval->setString(L"");
        return;
    }
    
    if (end_off > len)
    {
        end_off = len;
    }
    
    retval->setString(str + start_off, end_off - start_off);
}

// (METHOD) String.split
// Description: Returns an array obtained by splitting the string at 
//     the instances of the specified character.
//
// Syntax: function String.split(delimiter : String,
//                               limit : Integer) : Array(String)
//
// Remarks: Returns an array containing string split up at the 
//     occurrences of the |delimiter|, where the maximum number
//     of elements that can be returned in the array is given
//     by |limit|.
//
// Param(delimiter): The string at whose occurrences the current
//     string is to be split.
// Param(limit): The maximum number of elements the returned 
//     array can have.
//
// Returns: Returns an array containing string split up at the 
//     occurrences of the |delimiter|.

void String::split(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    const wchar_t* src = vthis->getString();
    const wchar_t* split = NULL;
    int limit = 999999999;

    if (env->m_param_count >= 1)
    {
        split = env->m_eval_params[0]->getString();
    }

    if (env->m_param_count >= 2)
    {
        limit = env->m_eval_params[1]->getInteger();
    }

    retval->setObject(Array::createObject(env));

    if (split == NULL || wcslen(split) == 0 || limit <= 0)
        return;

    int split_str_len = (int)wcslen(split);

    int counter = 0;
    const wchar_t* start = src;
    while (1)
    {
        if (counter == limit)
            break;

        const wchar_t* next = wcsstr(start, split);

        Value* element = retval->getMemberI(counter++);
        if (next)
        {
            element->setString(start, next-start);
        }
         else
        {
            element->setString(start);
            break;
        }
        start = next+split_str_len;
    }
}

// (METHOD) String.substring
// Description: Returns a substring of the string.
//
// Syntax: function String.substring(start : Integer) : String
// Syntax: function String.substring(start : Integer,
//                                   end : Integer) : String
//
// Remarks: Returns a string containing the characters of the current
//     string from the |start| position to the position right before
//     |end|.  If |end| isn't specified, the function returns the string
//     from |start| to the end of the string.
//
// Param(start): The position at which the returned substring is to |start|.
// Param(end): The position at which the returned substring is to |end|.
//
// Returns: Returns a string containing the characters of the current
//     string from the |start| position to the position right before |end|.

void String::substring(ExprEnv* env, void* param, Value* retval)
{
    // substring(start,end); NOTE: this is different from 
    // substr(start,length), which takes a starting position
    // and an offset rather than an end position
    
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    // get the input string length
    wchar_t* src = vthis->getString();
    int src_len = vthis->getDataLen();

    // if there aren't any input parameters, return the string
    int num_params = env->m_param_count;
    if (env->m_param_count < 1)
    {
        retval->setString(src);
        return;
    }
    
    // declare the start and end parameters
    int copy_start;
    int copy_end;
    
    // get the start parameter; if it's NaN or negative, set it to zero;
    // if it's greater than the length of the string, set it to the length
    // of the string
    
    Value copy_start_val;
    env->m_eval_params[0]->toNumber(&copy_start_val);
    
    copy_start = copy_start_val.getInteger();

    if (copy_start < 0 || copy_start_val.isNaN())
        copy_start = 0;

    if (copy_start > src_len)
        copy_start = src_len;
    

    // get the end parameter; if it's undefined, set it equal
    // to the length of the string; otherwise, if it's NaN or negative,
    // set it to zero, or if it's greater than the length of the string,
    // set it to the length of the string

    if (env->m_param_count < 2)
    {
        copy_end = src_len;
    }
    else
    {
        Value copy_end_val;
        env->m_eval_params[1]->toNumber(&copy_end_val);
        
        copy_end = copy_end_val.getInteger();
        
        if (copy_end < 0 || copy_end_val.isNaN())
            copy_end = 0;
            
        if (copy_end > src_len)
            copy_end = src_len;
    }

    // if copy_start is greater than copy_end, swap them
    if (copy_start > copy_end)
    {
        int t = copy_end;
        copy_end = copy_start;
        copy_start = t;
    }

    retval->setString(src + copy_start, copy_end - copy_start);
}


// (METHOD) String.trim
// Description: Returns a string with leading and trailing spaces removed
//
// Syntax: function String.toLowerCase() : String
//
// Remarks: Returns a copy of the string with all leading and trailing
//     spaces removed.  Any white space or line separating character
//     constitues a "space".
//
// Returns: A copy of the string with leading and trailing spaces removed.

void String::trim(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    size_t len = vthis->getDataLen();
    wchar_t* p = vthis->getString();
    
    // trim leading spaces
    while (len && isWhiteSpaceOrLS(*p))
    {
        p++;
        len--;
    }
    
    // trim trailing spaces
    while (len > 0 && isWhiteSpaceOrLS(*(p+len-1)))
        len--;
        
    retval->setString(p, len);
}


// (METHOD) String.toLowerCase
// Description: Returns a copy of the string with all lowercase characters.
//
// Syntax: function String.toLowerCase() : String
//
// Remarks: Returns a copy of the string with all lowercase characters.
//
// Returns: Returns a copy of the string with all lowercase characters.

void String::toLowerCase(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }
    
    retval->setValue(vthis);
    wchar_t* p = retval->getString();
    while (*p)
    {
        *p = jsToLower(*p);
        ++p;
    }
}

// (METHOD) String.toUpperCase
// Description: Returns a copy of the string with all uppercase characters.
//
// Syntax: function String.toUpperCase() : String
//
// Remarks: Returns a copy of the string with all uppercase characters.
//
// Returns: Returns a copy of the string with all uppercase characters.

void String::toUpperCase(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    retval->setValue(vthis);
    wchar_t* p = retval->getString();
    while (*p)
    {
        *p = jsToUpper(*p);
        ++p;
    }
}



// ( METHOD ) String.setCharAt
// Description: Sets a character at a specified offset
//
// Syntax: function String.setCharAt(index : Integer, character : String)
//
// Remarks: Sets the character at the offset specified by |index| to
//     the first character of the |character| parameter.  Please note that
//     this method is not a standard ECMAScript String method.
//
// Returns: Returns a copy of the changed string

void String::setCharAt(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    int idx = -1;
    if (env->getParamCount() > 0)
        idx = env->getParam(0)->getInteger();
    if (idx < 0 || idx >= vthis->getDataLen())
    {
        retval->setValue(vthis);
        return;
    }
    

    if (env->getParamCount() > 1)
    {
        wchar_t* replace_str = env->getParam(1)->getString();
        *(vthis->getString() + idx) = *replace_str;
    }
    


    retval->setValue(vthis);
}




// (METHOD) String.substr
// Description: Returns a substring of the string.
//
// Syntax: function String.substr(start : Integer) : String
// Syntax: function String.substr(start : Integer,
//                                length : Integer) : String
//
// Remarks: Returns a string containing the |length| characters of the current
//     string starting at position |start|. If |length| is not supplied, all 
//     characters after |start| are included in the returned string.
//
// Param(start): The position at which the returned substring is to |start|.
// Param(length): The |length| of the returned substring.
//
// Returns: Returns a string containing the |length| characters of the current
//     string starting at position |start|. If |length| is not supplied, all 
//     characters after |start| are included in the returned string.

void String::substr(ExprEnv* env, void* param, Value* retval)
{
    // substr(start,length); NOTE: this is different from 
    // substring(start,end), which takes a starting and ending
    // position rather than an offset

    Value* vthis = getThisString(env);
    if (!vthis)
    {
        retval->setNull();
        return;
    }

    // get the input string length
    wchar_t* src = vthis->getString();
    int src_len = vthis->getDataLen();

    // if there aren't any input parameters, return the string
    int num_params = env->m_param_count;
    if (env->m_param_count < 1)
    {
        retval->setString(src);
        return;
    }
    
    // declare the start and copy length parameters
    int copy_start;
    int copy_length;
    
    // get the start parameter; if it's NaN, set it to zero
    env->m_eval_params[0]->toNumber(env->m_eval_params[0]);
    copy_start = env->m_eval_params[0]->getInteger();

    // get the copy length parameter; if it's undefined, set it equal
    // to the length of the string; if it's NaN, set it to zero
    if (env->m_param_count < 2)
    {
        copy_length = src_len;
    }
    else
    {
        env->m_eval_params[1]->toNumber(env->m_eval_params[1]);
        copy_length = env->m_eval_params[1]->getInteger();
    }
    

    // if the start is greater or equal to the string length 
    // or the copy length is less than or equal to zero, we're done

    if (copy_start >= src_len || copy_length <= 0)
    {
        retval->setString(L"");
        return;
    }
    
    // if the start is less than zero, wrap around the end of
    // the string until the beginning of the string, then stop wrapping

    if (copy_start < 0)
    {
        copy_start = src_len + copy_start;
        if (copy_start < 0)
            copy_start = 0;
    }


    // if the start plus the copy length is greater than the string 
    // length, set the copy length so that the start plus the 
    // copy length returns the start to the end of the string

    if (copy_start + copy_length > src_len)
        copy_length = src_len - copy_start;
 
    retval->setString(src + copy_start, copy_length);
}

// (METHOD) String.compare
// Description: Returns an integer indicating whether the current string is 
//     less than, equal to, or greater than a specified.
//
// Syntax: function String.compare(string : String) : Integer
// Syntax: static function String.compare(string1 : String,
//                                        string2 : String) : Integer
//
// Remarks: Returns an integer indicating whether the current string is 
//     less than, equal to, or greater than a specified |string|.  If the
//     current string is less than the specified |string|, the return result
//     is negative; if the current string is equal to the specified |string|, 
//     the return result is zero; if the current string is greater than the 
//     specified |string|, the return result is positive.  If the static 
//     function is used, |string1| is compared with |string2|. Note: this 
//     function is equivalent to localeCompare().
//
// Param(string): The |string| to which to compare the current string.
//
// Returns: Returns an integer indicating whether the current string is 
//     less than, equal to, or greater than a specified.

void String::compare(ExprEnv* env, void* param, Value* retval)
{
    js_String_compare_helper(env, param, retval, wcscmp);
}




static std::wstring num2str(Value* num,
                            wchar_t fmt_char,
                            bool left_justify,
                            bool zero_padded,
                            int pad,
                            int scale)
{
    wchar_t fmt[32];
    wchar_t res[255];
    
    if (scale == -1)
        swprintf(fmt, 32, L"%%%s%s%d%c", left_justify ? L"-" : L"", zero_padded ? L"0" : L"", pad, fmt_char);
         else
        swprintf(fmt, 32, L"%%%s%s%d.%d%c", left_justify ? L"-" : L"", zero_padded ? L"0" : L"", pad, scale, fmt_char);
    
    if (fmt_char == 'd' || fmt_char == 'u')
        swprintf(res, 255, fmt, num->getInteger());
         else
        swprintf(res, 255, fmt, num->getDouble());
    
    return res;
}


// (METHOD) String.sprintf
// Description: Returns a string formatted by a specified format string
//
// Syntax: static function String.sprintf(format : String,
//                                        val1 : Any,
//                                        val2 : Any,
//                                        ...) : String
//
// Remarks: String.sprintf() formats a string specified in the |format|
//     parameter by inserting values in the manner indicated by the
//     formatting specifiers.  Please note that method is not a
//     standard ECMAScript String method.
//
// Param(format): The |format| string contains the output string with
//     formatting specifiers.
//
// Returns: Returns a formatted string


// static
void String::sprintf(ExprEnv* env, void* param, Value* retval)
{
    std::wstring result;

    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
        
    
    Value* val_fmt = env->getParam(0);
    wchar_t* fmt = val_fmt->getString();
    
    result.reserve(val_fmt->getDataLen());
    
    int arg, arg_counter = 1;
    int padding = 0, scale = -1;
    bool left_justified;
    bool zero_padded;
    
    
    wchar_t* fmt_start;
    wchar_t* p = fmt;
    
    
    while (*p)
    {
        if (*p == '%')
        {
            arg = -1;
            padding = 0;
            scale = -1;
            left_justified = false;
            zero_padded = false;
            
            ++p;
            if (!*p)
                break;
            if (*p == '%')
            {
                result += '%';
                ++p;
                continue;
            }
            
            fmt_start = p;
            while (*p && !iswalpha(*p))
            {
                if (*p == '$')
                {                
                    arg = wtoi(fmt_start);
                    padding = 0;
                    fmt_start = p+1;
                }
                 else if (*p == '-')
                {
                    left_justified = true;
                }
                 else if (*p == '.')
                {
                    p++;
                    scale = wtoi(p);
                    while (isdigit(*p))
                        p++;
                    continue;
                }
                 else if (iswdigit(*p))
                {
                    if (*p == '0')
                        zero_padded = true;
                    padding = wtoi(p);
                    while (isdigit(*p))
                        p++;
                    continue;
                }
                
                p++;
            }
            
            if (!*p)
                break;
            
            if (arg == -1)
                arg = arg_counter++;
            
            Value* val_arg = env->getParam(arg);
            
            switch (*p)
            {
                case 'c':
                case 's':
                {
                    wchar_t* str = val_arg->getString();
                    int len = (int)wcslen(str);
                    if (*p == 'c' && len > 1)
                        *(str+1) = 0;
                    padding -= len;
                    if (padding > 0 && !left_justified)
                        result.append(padding, L' ');
                    result.append(str, len);
                    if (padding > 0 && left_justified)
                        result.append(padding, L' ');
                    break;
                }
                case 'x':
                case 'X':
                case 'd':
                case 'u':
                case 'f':
                    result.append(num2str(val_arg, *p, left_justified, zero_padded, padding, scale));
                    break;
            }
        }
         else
        {
            result += *p;
        }
        
        ++p;
    }
    
    
    retval->setString(result);
}





// note: this is non-standard, but used in our test scripts;
// leave undocumented
void String::frequency(ExprEnv* env, void* param, Value* retval)
{
    if (!env->m_val_this)
    {
        retval->setNull();
        return;
    }
    
    if (env->getParamCount() < 1)   
        return;
        
    wchar_t* str = env->m_val_this->getString();
    wchar_t* search = env->getParam(0)->getString();
    size_t search_len = wcslen(search);
    size_t count = 0;
    
    if (search_len > 0)
    {
        while (*str)
        {
            if (0 == wcsncmp(str, search, search_len))
            {
                count++;
                str += search_len;
                continue;
            }
            str++;
        }
    }
    
    retval->setInteger(count);
}









            
static std::wstring doEncode(const std::wstring& input, const wchar_t* to_encode)
{
    std::wstring result;
    
    const wchar_t* ch = input.c_str();
    unsigned int c;
    
    wchar_t buf[64];
    
    while ((c = *ch))
    {
        if (c >= 128)
        {
            // TODO: the below works as expected with escape();
            // My understanding is that, for the other functions,
            // we need to utf-8 encode this character per RFC-3986
 
            if (c <= 255)
            {
                swprintf(buf, 64, L"%%%02X", c);
                result += buf;
            }
             else
            {
                swprintf(buf, 64, L"%%u%04X", c);
                result += buf;
            }
            
            ch++;
            continue;
        }
        
        if (c <= 0x1f || wcschr(to_encode, *ch))
        {
            swprintf(buf, 64, L"%%%02X", c);
            result += buf;
        }
         else
        {
            result += *ch;
        }
        
        ++ch;
    }

    return result;
}
      
static std::wstring doDecode(const std::wstring& input)
{
    std::wstring result;
    
    const wchar_t* ch = input.c_str();
    const wchar_t* d;
    static const wchar_t* hexchars = L"0123456789ABCDEF";
    
    while (*ch)
    {
        if (*ch == '%')
        {
            if (towupper(*(ch+1)) == 'U')
            {
                if (*(ch+2) && *(ch+3) && *(ch+4) && *(ch+5))
                {
                    const wchar_t* s = ch+1;
                    const wchar_t* res;
                    unsigned int i, mul = 4096, c = 0;
                    
                    for (i = 1; i <= 4; ++i)
                    {
                        res = wcschr(hexchars, ::towupper(*(s+i)));
                        if (!res)
                            break;
                        c += (unsigned int)((res-hexchars)*mul);
                        mul /= 16;
                    }

                    if (i == 5)
                    {
                        result += (wchar_t)c;
                        ch += 6;
                        continue;
                    }
                }
            }
             else if (*(ch+1) && *(ch+2))
            {
                int digit1, digit2;
                d = wcschr(hexchars, towupper(*(ch+1)));
                if (d)
                {
                    digit1 = (int)(d-hexchars);
                    d = wcschr(hexchars, towupper(*(ch+2)));
                    if (d)
                    {
                        digit2 = (int)(d-hexchars);
                        wchar_t r = (digit1*16) + digit2;
                        result += r;
                        ch += 3;
                        continue;
                    }
                }
            }
        }
        
        result += *ch;
        ++ch;
    }

    return result;
}


// encode-decode functions
            

void js_escape(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setUndefined();
        return;
    }
    
    std::wstring result = doEncode(env->getParam(0)->getString(), L"%!&$#(){}[]<>=:,;?\\|^~`'\" ");
    retval->setString(result);
}

void js_encodeURI(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setUndefined();
        return;
    }
    
    std::wstring result = doEncode(env->getParam(0)->getString(), L"%&{}[]<>\\|^`\" ");
    retval->setString(result);
}

void js_encodeURIComponent(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setUndefined();
        return;
    }

    std::wstring result = doEncode(env->getParam(0)->getString(), L"%@&$#{}[]<>=:/,;?+\\|^`\" ");
    retval->setString(result);
}

void js_unescape(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setUndefined();
        return;
    }
    
    std::wstring result = doDecode(env->getParam(0)->getString());
    retval->setString(result);
}

void js_decodeURI(ExprEnv* env, void* param, Value* retval)
{
    js_unescape(env, param, retval);
}

void js_decodeURIComponent(ExprEnv* env, void* param, Value* retval)
{
    js_unescape(env, param, retval);
}














}


