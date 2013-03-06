/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Aaron L. Williams
 * Created:  2008-12-22
 *
 */


#include "kscript.h"
#include "json.h"
#include "jsarray.h"
#include "jsdate.h"
#include "jsstring.h"
#include "jsnumber.h"
#include "jsboolean.h"
#include "util.h"

#include "../kl/include/kl/json.h"


#ifdef _MSC_VER
#define swprintf _snwprintf
#endif


namespace kscript
{


static std::wstring escape_string(std::wstring& str)
{
    std::wstring result = L"";

    std::wstring::iterator it, it_end;
    it_end = str.end();

    for (it = str.begin(); it != it_end; ++it)
    {
        wchar_t ch = *it;

        switch (ch)
        {
            default:
                {
                    // TODO: check range of values allowed without
                    // unicode escaping
                    if (ch >= 32 && ch <= 126)
                        result += ch;
                    else
                    {
                        wchar_t buf[25];
                        swprintf(buf, 25, L"\\u%04x", (int)ch);
                        result.append(buf);
                    }
                }
                break;

            case L'"':
                result.append(L"\\\"");
                break;

            case L'\\':
                result.append(L"\\\\");
                break;

            case L'\b':
                result.append(L"\\b");
                break;

            case L'\t':
                result.append(L"\\t");
                break;

            case L'\n':
                result.append(L"\\n");
                break;

            case L'\f':
                result.append(L"\\f");
                break;

            case L'\r':
                result.append(L"\\r");
                break;
        };
    }

    return result;
}

static void add_token(kscript::ExprEnv* env, 
                      kscript::Value* func,
                      JsonInfo& info)
{
    // if a formatting function isn't specified, simply add
    // the token and set the last token
    if (func == NULL)
    {
        // add the token
        info.result_str.append(info.append_str);
        info.last_token = info.append_str;
    }
     else if (func->isFunction())
    {
        // set the left token to the last token
        std::wstring ltoken = info.last_token;

        // if the token being added is a brace, comma, etc, set
        // the right token
        std::wstring rtoken = L"";
        if (info.append_str == L"{" ||
            info.append_str == L"}" ||
            info.append_str == L"[" ||
            info.append_str == L"]" ||
            info.append_str == L":" ||
            info.append_str == L",")
        {
            rtoken = info.append_str;
        }

        // set the function parameters
        Value* params[3];
        params[0] = new Value(ltoken);
        params[1] = new Value(rtoken);
        params[2] = new Value(info.level);
        
        // invoke the function
        Value retval;
        env->getParser()->invoke(env, func, NULL, &retval, params, 3);

        delete params[0];
        delete params[1];
        delete params[2];

        // return the format result
        info.result_str.append(retval.getString());

        // add the token
        info.result_str.append(info.append_str);
        info.last_token = rtoken;
    }
     else
    {
        // set the left token to the last token
        std::wstring ltoken = info.last_token;

        std::wstring spaces;
        std::wstring total_spaces;
        if (func->isNumber())
            spaces.assign(func->getInteger(), L' ');
             else
            spaces = func->getString();
        
        for (int i = 0; i < info.level; ++i)
            total_spaces += spaces;

        if (info.append_str == L"{" || info.append_str == L"[")
        {
            if (info.result_str.length() > 0)
                info.result_str += L"\n";
            info.result_str += total_spaces;
            info.result_str += info.append_str;
            info.result_str += L"\n";
        }
         else if (info.append_str == L"}" || info.append_str == L"]")
        {
            info.result_str += L"\n";
            info.result_str += total_spaces;
            info.result_str += info.append_str;
        }
         else if (info.append_str == L",")
        {
            info.result_str += info.append_str;
            info.result_str += L"\n";
        }
         else if (info.append_str == L":")
        {
            info.result_str += info.append_str;
        }
         else
        {
            if (ltoken != L":")
                info.result_str += total_spaces;
            info.result_str += info.append_str;
        }
        
        info.last_token = info.append_str;
    }
}


// (CLASS) JSON
// Category: Core
// Description: A class for parsing JSON and serializing objects into JSON.
// Remarks: The JSON object provides a convenient way to parse JSON and
//     serialize objects into JSON.

Json::Json()
{
}

Json::~Json()
{
}

void Json::constructor(ExprEnv* env, Value* retval)
{
}


// (METHOD) JSON.stringify
// Description: Encodes an object into a JSON string.
//
// Syntax: function JSON.stringify(object : Object,
//                                 replacer : Function,
//                                 space : Number, String) : String
//
// Remarks: Encodes the |object| into a JSON string.  If a |replacer|
//     function is specified, the replacer function's return value
//     will be used for the values in the return string.  If a |space|
//     parameter is specified, the return string will include
//     formatting spaces.
//
// Param(object): The object to encode.
// Param(replacer): An optional function to use when generating the
//     return string.
// Param(space): An optional number or string space parameter.
//
// Returns: Returns a string representing the encoded object.

void Json::stringify(ExprEnv* env, void*, Value* retval)
{
    // get the value to encode
    Value* value = env->getParam(0);

    // get an optional format function
    Value* replacer = NULL;
    if (env->getParamCount() > 1)
    {
        Value* r = env->getParam(1);
        if (r->isFunction())
            replacer = r;
    }

    // get an optional space parameter
    Value* spacer = NULL;
    if (env->getParamCount() > 2)
        spacer = env->getParam(2);

    // object for accumulating results for json string
    JsonInfo info;

    // encode the value and return the result
    encode(env, value, replacer, spacer, info);
    retval->setString(info.result_str);
}

// (METHOD) JSON.parse
// Description: Parses a JSON string into an object
//
// Syntax: function Json.parse(string : String) : Object
//
// Remarks: Parses a JSON |string| into an object.
//
// Param(text): The string to decode.
//
// Returns: Returns an object from the parsed string.

void Json::parse(ExprEnv* env, void*, Value* retval)
{
    retval->setNull();
    if (env->getParamCount() < 1)
        return;

    wchar_t* endloc = NULL;
    Value* value;
    value = parseJson(env, env->getParam(0)->getString(), &endloc);

    if (!value)
    {
        // the JSON didn't parse, so throw an exception
        return;
    }
    
    retval->setValue(value);
    delete value;

/*
    // TODO: need to implement strict JSON parser with
    // reviver function

    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }

    ExprElement* e;
    wchar_t* endloc = NULL;

    e = env->getParser()->parseElement(
                            env->getParserContext(),
                            env->getParam(0)->getString(),
                            &endloc);
    if (!e)
    {
        // parse error
        retval->setNull();
        return;
    }

    e->eval(env, retval);

    delete e;
*/
}

void Json::validate(ExprEnv* env, void*, Value* retval)
{
    // NOTE: private test function for JSON schema validator

    retval->setBoolean(false);
    if (env->getParamCount() < 2)
        return;

    // get the json string to validate
    Value* value_data = env->getParam(0);
    std::wstring data_str = value_data->getString();

    Value* value_schema = env->getParam(1);
    std::wstring schema_str = value_schema->getString();

    kl::JsonNode data_node;
    data_node.fromString(data_str);

    kl::JsonNode schema_node;
    schema_node.fromString(schema_str);

    bool result = data_node.isValid(schema_node);
    retval->setBoolean(result);
}

void Json::encode(ExprEnv* env, 
                  Value* value, 
                  Value* replacer,
                  Value* spacer,
                  JsonInfo& info)
{
    // if a replacer function is specified, call the
    // replacer function and encode the return value
    Value replacer_retval;
    if (replacer)
    {
        // set the function parameters
        Value* params[2];
        params[0] = &info.key;
        params[1] = value;
        
        // invoke the function
        env->getParser()->invoke(env, replacer, NULL, &replacer_retval, params, 2);

        // replace the value with the new value
        value = &replacer_retval;
    }

    // if the value is null, return null
    if (!value || value->isNull())
    {
        info.append_str = L"null";
        add_token(env, spacer, info);
        return;
    }

    ValueObject* obj = value->getObject();

    // if the object is a date, convert it to a string
    if (obj->isKindOf(Date::staticGetClassId()))
    {
        // get the date values
        Date* obj_date = static_cast<Date*>(obj);

        int yy, mm, dd, h, m, s, ms;
        obj_date->getDateTime(&yy, &mm, &dd, &h, &m, &s, &ms);

        // convert the date into a string of the form "YYYY-MM-DDTHH:MM:SS"
        wchar_t buf[255];
        swprintf(buf, 255, L"%02d-%02d-%02dT%02d:%02d:%02d",
                  yy, mm + 1, dd, h, m, s);

        info.append_str = L"";
        info.append_str.append(L"\"");
        info.append_str.append(buf);
        info.append_str.append(L"\"");

        // add the string to the result
        add_token(env, spacer, info);
        return;
    }

    if (obj->isKindOf(String::staticGetClassId()) ||
        value->isString())
    {
        // build the result string
        std::wstring str = value->getString();
        info.append_str = L"";
        info.append_str.append(L"\"");
        info.append_str.append(escape_string(str));
        info.append_str.append(L"\"");

        // add the string to the result
        add_token(env, spacer, info);
        return;
    }

    if (obj->isKindOf(Number::staticGetClassId()) ||
        value->isNumber())
    {
        // if the value isn't finite, return null
        if (value->isAnyInfinity() || value->isNaN())
        {
            info.append_str = L"null";
            add_token(env, spacer, info);
            return;
        }

        // add the string to the result
        info.append_str = value->getString();
        add_token(env, spacer, info);
        return;
    }

    if (obj->isKindOf(Boolean::staticGetClassId()) ||
        value->isBoolean())
    {
        // add the string to the result
        info.append_str = (value->getBoolean() ? L"true" : L"false");
        add_token(env, spacer, info);
        return;
    }

    // if the object is an array, iterate through the
    // members and convert it to an array string
    if (obj->isKindOf(Array::staticGetClassId()))
    {
        std::wstring lbracket = L"[";
        std::wstring rbracket = L"]";
        std::wstring comma = L",";
        std::wstring rparam = L"";

        // add the opening bracket as a separate token
        info.append_str = lbracket;
        add_token(env, spacer, info);
        info.level++;

        // append the array values
        size_t member_count = obj->getRawMemberCount();
        for (size_t idx = 0; idx < member_count; ++idx)
        {
            // add the comma as a separate token
            if (idx > 0)
            {
                info.append_str = comma;
                add_token(env, spacer, info);
            }

            // get the member
            Value* new_value = obj->getRawMemberByIdx(idx);

            // append the value
            info.key.setInteger(idx);
            encode(env, new_value, replacer, spacer, info);
        }

        // add the closing bracket as a separate token
        info.level--;
        info.append_str = rbracket;
        add_token(env, spacer, info);
        return;
    }

    // any other type of object, iterate through the
    // members and convert it to an object string

    std::wstring lbrace = L"{";
    std::wstring rbrace = L"}";
    std::wstring comma = L",";
    std::wstring colon = L":";

    // add the opening brace as a separate token
    info.append_str = lbrace;
    add_token(env, spacer, info);
    info.level++;

    // append the values
    size_t member_count = obj->getRawMemberCount();
    for (size_t idx = 0; idx < member_count; ++idx)
    {
        // add the comma as a separate token
        if (idx > 0)
        {
            info.append_str = comma;
            add_token(env, spacer, info);
        }

        // get the member name
        std::wstring member_name = obj->getRawMemberName(idx);

        info.append_str = L"";
        info.append_str.append(L"\"");
        info.append_str.append(escape_string(member_name));
        info.append_str.append(L"\"");

        // add the member name
        add_token(env, spacer, info);

        // add the colon as a separate token
        info.append_str = colon;
        add_token(env, spacer, info);

        // get the member
        Value* new_value = obj->getRawMemberByIdx(idx);

        // add the value
        info.key.setString(member_name);
        encode(env, new_value, replacer, spacer, info);
    }

    // add the closing brace as a separate token
    info.level--;
    info.append_str = rbrace;
    add_token(env, spacer, info);
    return;
}

Value* Json::parseJson(ExprEnv* env,
                       wchar_t* expr,
                       wchar_t** endloc)
{
    // endloc is required
    if (!endloc)
        return NULL;
 
    // clear leading white space    
    skipWhiteSpaceOrLS(expr);
    
    // parse the json value
    Value* value = parseJsonValue(env, expr, endloc);
    expr = *endloc;

    if (!expr)
        return value;

    // clear trailing whitespace
    if (*expr)
        skipWhiteSpaceOrLS(expr);
    
    // if the end location isn't at the end of
    // the string, there's some other trailing character,
    // which is invalid, so set the return value to
    // null
    if (*expr)
    {
        if (value)
            delete value;

        value = NULL;
    }

    return value;
}

Value* Json::parseJsonValue(ExprEnv* env,
                            wchar_t* expr,
                            wchar_t** endloc)
{
    // endloc is required
    if (!endloc)
        return NULL;
        
    skipWhiteSpaceOrLS(expr);

    if (*expr == '{')
        return parseJsonObject(env, expr, endloc);
    else if (*expr == '[')
        return parseJsonArray(env, expr, endloc);
    else if (*expr == '\"')
        return parseJsonString(env, expr, endloc);
    else if (*expr == '-' || (*expr >= '0' && *expr <= '9'))
        return parseJsonNumber(env, expr, endloc);
    else   
        return parseJsonWord(env, expr, endloc);
}

Value* Json::parseJsonObject(ExprEnv* env,
                             wchar_t* expr,
                             wchar_t** endloc)
{
    if (*expr != '{')
        return NULL;

    expr++;
    skipWhiteSpaceOrLS(expr);

    Value* retval = new Value;
    retval->setObject(Object::createObject(env));

    if (*expr == '}')
    {
        // empty object;
        *endloc = expr + 1;
        return retval;
    }

    std::wstring key;

    while (1)
    {
        // get the key string
        Value* key_val = parseJsonString(env, expr, endloc);
        if (!key_val)
            return NULL;

        key = key_val->getString();
        delete key_val;

        // start where the key string ended
        expr = *endloc;
        skipWhiteSpaceOrLS(expr);

        if (*expr != ':')
        {
            // missing colon
            delete retval;
            return NULL;
        }

        // skip past the colon
        expr++;
        skipWhiteSpaceOrLS(expr);
        
        // parse the value portion
        Value* value = parseJsonValue(env, expr, endloc);
        if (!value)
        {
            // value portion didn't parse
            delete retval;
            return NULL;
        }

        // set the member
        retval->setMember(key, value);
        delete value;

        expr = *endloc;
        skipWhiteSpaceOrLS(expr);

        // if we have a comma, continue on
        if (*expr == ',')
        {
            expr++;
            skipWhiteSpaceOrLS(expr);
            continue;
        }

        // if we have a closing brace, we're done
        if (*expr == '}')
        {
            *endloc = expr+1;
            return retval;
        }
        
        // unterminated JSON
        *endloc = expr;
        delete retval;
        return NULL;
    }
}


Value* Json::parseJsonArray(ExprEnv* env,
                            wchar_t* expr,
                            wchar_t** endloc)
{
    if (*expr != '[')
        return NULL;

    expr++;
    skipWhiteSpaceOrLS(expr);

    Value* retval = new Value;
    retval->setObject(Array::createObject(env));

    if (*expr == ']')
    {
        // empty object;
        *endloc = expr + 1;
        return retval;
    }

    std::wstring key;

    int idx = 0;
    while (1)
    {
        // parse the value
        Value* value = parseJsonValue(env, expr, endloc);
        if (!value)
        {
            // value portion didn't parse
            delete retval;
            return NULL;
        }

        // set the member
        retval->setMemberI(idx, value);
        delete value;

        expr = *endloc;
        skipWhiteSpaceOrLS(expr);

        // if we have a comma, continue on
        if (*expr == ',')
        {
            idx++;
            expr++;
            skipWhiteSpaceOrLS(expr);
            continue;
        }

        // if we have a closing bracket, we're done
        if (*expr == ']')
        {
            *endloc = expr+1;
            return retval;
        }
        
        // unterminated JSON
        *endloc = expr;
        delete retval;
        return NULL;
    }
}

Value* Json::parseJsonString(ExprEnv* env,
                             wchar_t* expr,
                             wchar_t** endloc)
{
    return (Value*)env->m_parser->parseStringLiteral(NULL, expr, endloc);
}

Value* Json::parseJsonNumber(ExprEnv* env,
                             wchar_t* expr,
                             wchar_t** endloc)
{
    Value* v = new Value;
    
    const wchar_t* offending = NULL;
    bool result = parseNumericConstant(expr, v, &offending, true);
    if (result)
    {
        *endloc = expr+wcslen(expr);
    }
     else
    {
        *endloc = (wchar_t*)offending;
        
        {
            SetCharOnExitScope sc((wchar_t*)offending, *offending, 0);
            result = parseNumericConstant(expr, v, &offending, true);
            if (!result)
            {
                delete v;
                return NULL;
            }
        }
        
        return v;
    }
/*
    const wchar_t* offending = NULL;
    bool result = parseNumericConstant(expr, v, &offending, true);
    if (result)
    {
        *endloc = expr+wcslen(expr);
    }
     else
    {
        *endloc = offending;
        
        {
            SetCharOnExitScope((wchar_t*)offending, *offending, 0);
            result = parseNumericConstant(expr, v, offending, true);
            if (!result)
            {
                
            }
        }
        
        return NULL;
    }
*/
    return v;
}

Value* Json::parseJsonWord(ExprEnv* env,
                           wchar_t* expr,
                           wchar_t** endloc)
{
    Value* v = NULL;
    
    if (!wcsncmp(expr, L"true", 4))
    {
        v = new Value;
        v->setBoolean(true);
        *endloc = (expr + 4);
    }
     else if (!wcsncmp(expr, L"false", 5))
    {
        v = new Value;
        v->setBoolean(false);
        *endloc = (expr + 5);
    }
     else if (!wcsncmp(expr, L"null", 4))
    {
        v = new Value;
        v->setNull();
        *endloc = (expr + 4);
    }    
    
    return v;
}


};  // namespace kscript

