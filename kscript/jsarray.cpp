/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-06-12
 *
 */


#include "kscript.h"
#include "jsarray.h"
#include "util.h"
#include <algorithm>
#include <wctype.h>


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


#ifdef _MSC_VER
#define isnan _isnan
#define strcasecmp stricmp
#define wcscasecmp wcsicmp
#define wcsncasecmp wcsnicmp
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef swprintf
#define swprintf _snwprintf
#endif
#endif


namespace kscript
{


static ValueObject* getThisObject(ExprEnv* env)
{
    Value* ethis = env->getThis();
    if (ethis && ethis->isObject())
        return ethis->getObject();
    
    env->setRuntimeError(rterrorTypeError);
    return NULL;
}


// (CLASS) Array
// Category: Core
// Derives: Object
// Description: A class for storing lists of objects or data.
// Remarks: An array class for storing lists of objects or data.

Array::Array()
{
}

Array::~Array()
{
}

// (CONSTRUCTOR) Array.constructor
// Description: Creates a new Array object.
//
// Syntax: Array()
// Syntax: Array(length : Integer)
// Syntax: Array(elem_0 : Object,
//               ... , 
//               elem_n : Object)
//
// Remarks: Creates a new Array object.  If |length| is specified, the new
//     Array object will have |length| elements.  If |elem_0|, ... , |elem_n| are
//     specified, the new Array will contain these objects.
//
// Param(length): The index of the highest element of array plus one. Only the number 
//     of elements in the Array if every index smaller than the highest index 
//     has an element.
// Param(elem0, ..., elemn): The first n elements of the new Array. n has to greater than 1.

void Array::constructor(ExprEnv* env, Value* retval)
{
    if (env->m_param_count == 0)
    {
        return;
    }
     else if (env->m_param_count == 1 &&
              env->m_eval_params[0]->isNumber())
    {
        // |didx| will check for weird array sizes like 15.01 and throw
        // an exception if one is encountered
        double didx = env->m_eval_params[0]->getDouble();
        int idx = env->m_eval_params[0]->getInteger();
        if (idx < 0 || dblcompare(didx, (double)idx) != 0)
        {
            env->setRuntimeError(rterrorRangeError);
            return;
        }

        if (idx > 0)
        {
            // -- auto resize result --
            getMemberI(idx-1);
            return;
        }
    }
     else
    {
        for (size_t i = 0; i < env->m_param_count; ++i)
        {
            env->m_eval_params[i]->eval(env, getMemberI(i));
        }
    }
}

void Array::staticConstructor(ExprEnv* env, void* param, Value* retval)
{
    retval->setObject(Array::createObject(env));
        
    if (env->m_param_count == 1 &&
        env->m_eval_params[0]->isNumber())
    {
        // |didx| will check for weird array sizes like 15.01 and throw
        // an exception if one is encountered
        double didx = env->m_eval_params[0]->getDouble();
        int idx = env->m_eval_params[0]->getInteger();
        if (idx < 0 || dblcompare(didx, (double)idx) != 0)
        {
            env->setRuntimeError(rterrorRangeError);
            return;
        }

        if (idx > 0)
        {
            // -- auto resize result --
            retval->getMemberI(idx-1);
            return;
        }
    }
     else
    {
        for (size_t i = 0; i < env->m_param_count; ++i)
        {
            env->m_eval_params[i]->eval(env, retval->getMemberI(i));
        }
    }
}

Value* Array::getMember(const std::wstring& name)
{
    if (name == L"length")
    {
        Value* v = getValueReturn();
        v->setInteger(getHighestIndex());
        return v;
    }
    
    return ValueObject::getMember(name);
}


// (METHOD) Array.toString
// Description: Returns a string representation of an Array.
//
// Syntax: function Array.toString() : String
//
// Remarks: Converts each element of the array to a string, concatenates
//     the elements using the "," as a separator, and returns a single
//     concatenated string of the elements.
//
// Returns: Returns a string that represents the array.

void Array::toString(ExprEnv* env, Value* retval)
{
    toString(retval);
}

// (METHOD) Array.toLocaleString
// Description: Returns a local specific string representation of an Array.
//
// Syntax: function Array.toLocaleString() : String
//
// Remarks: Calls the function toLocaleString() on every element of the array
//     and returns the results concatenated with a locale-specific separator.
//
// Returns: Returns a localized string that represents the array.

void Array::toLocaleString(ExprEnv* env, Value* retval)
{
    toString(env, retval);
}

// (METHOD) Array.concat
// Description: Concatenates a number of elements to an Array.
//
// Syntax: function Array.concat(elem_n+1 : Object,
//                               ..., 
//                               elem_n+m : Object) : Array(Object)
//
// Remarks: Returns a new array, without modifying the original array, formed by 
//     concatenating the input elements with the elements of the array.
//
// Param(elem_n+1, ..., elem_n+m): The elements to concatenate to the array.  If
//     a specified element in the parameter list is an array, the contents of the
//     specified array are copied into the result.
//
// Returns: Returns a new Array, created by concatenating the input elements to
//     the array.

void Array::concat(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;
        
    Array* arr = Array::createObject(env);
    
    // copy elements from |this| first
    size_t i, count = vthis->getRawMemberCount();
    for (i = 0; i < count; ++i)
    {
        const ValueMember& val = vthis->getRawValueMemberByIdx(i);
        arr->setMember(val.name, val.value);
    }

    // copy any elements passed in the parameters;
    size_t param_count = env->getParamCount();
    for (i = 0; i < param_count; ++i)
    {
        // first, examine if the parameter contains an object with members
        Value* param = env->getParam(i);
        if (param->isObject())
        {
            ValueObject* obj = param->getObject();
            size_t mi, mcount = obj->getRawMemberCount();
            for (mi = 0; mi < mcount; ++mi)
            {
                const ValueMember& val = obj->getRawValueMemberByIdx(mi);
                arr->setMember(val.name, val.value);
            }
        }
         else
        {
            arr->setMember(L"", param);
        }
    }
        
    retval->setObject(arr);
}


// (METHOD) Array.join
// Description: Returns the elements of an Array as a string.
//
// Syntax: function Array.join() : String
// Syntax: function Array.join(separator : String) : String
//
// Remarks: Joins the elements of the array into a string, where the
//     individual elements of the array are separated by |separator| if
//     it is specified, or by "," if the |separator| is not specified.
//
// Param(separator): A string that will be used to separate the elements 
//     of the Array in the returned String.
// Returns: Returns a string formed by converting all elements of array to 
//     strings and concatenating them with the |separator| added between elements. 
//     If a |separator| is not supplied, a "," is used.

void Array::join(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;

    std::wstring delim = L",";
    if (env->getParamCount() > 0)
        delim = env->getParam(0)->getString();
        
    makeStringRetval(vthis, delim, retval);
}



// (METHOD) Array.pop
// Description: Deletes and returns the last element of an Array.
//
// Syntax: function Array.pop() : Object
//
// Remarks: Deletes and returns the last element of the array. If the
//     array is empty, the function returns undefined and leaves the
//     array unchanged.
//
// Returns: Returns the last element of the array if the array is not empty,
//     otherwise returns undefined.

void Array::pop(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;

    if (vthis->getHighestIndex() <= 0)
    {
        retval->setUndefined();
        return;
    }
    
    int new_highest = vthis->getHighestIndex() - 1;
    vthis->getMemberI(new_highest, retval);
    vthis->deleteMemberI(new_highest);
    vthis->setHighestIndex(new_highest);
}

// (METHOD) Array.push
// Description: Appends a number of elements to the end of an Array.
//
// Syntax: function Array.push(elem_n+1 : Object,
//                             ..., 
//                             elem_n+m : Object) : Integer
//
// Remarks: Appends a number of elements to the end of an Array.
//
// Param(elem_n+1, ..., elem_n+m): The objects to append to the array.
//
// Returns: The new length of the array. 

void Array::push(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;

    for (size_t i = 0; i < env->m_param_count; ++i)
        vthis->appendMember(env->m_eval_params[i]);
    
    retval->setInteger(vthis->getHighestIndex());
}


// (METHOD) Array.reverse
// Description: Reverses the order of elements in an Array.
//
// Syntax: function Array.reverse()
//
// Remarks: Reverses the order of the elements of the array.

void Array::reverse(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;
    
    retval->setObject(vthis);
    
    int topidx = vthis->getHighestIndex();
    if (topidx == 0)
        return;
    topidx--;
        
    std::vector<ValueMember>::iterator it;
    wchar_t buf[64];
    
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
    {
        if (it->name.length() == 0 || wcschr(L"1234567890", it->name[0]))
        {
            int num = wtoi(it->name.c_str());
            num = abs(topidx-num);
            swprintf(buf, 64, L"%d", num);
            it->name = buf;
        }
    }
    
    vthis->m_members_map.clear();
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
        vthis->m_members_map[it->name] = it->value;
}



// (METHOD) Array.shift
// Description: Deletes and returns the first element of an Array.
//
// Syntax: function Array.shift() : Object
//
// Remarks: Deletes and returns the first element of the array and moves
//     all the other elements down one place. If the array is empty, the
//     function returns undefined and does not modify the array.
//
// Returns: Returns the first element of the array.

void Array::shift(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;

    retval->setObject(Array::createObject(env));
    
    // put the first element in our retval
    vthis->getMemberI(0, retval);
    vthis->deleteMemberI(0);
    
    // shift all the others down
    wchar_t buf[64];
    std::vector<ValueMember>::iterator it;
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
    {
        if (it->name.length() > 0 && iswdigit(it->name[0]))
        {
            int num = wtoi(it->name.c_str());
            swprintf(buf, 64, L"%d", --num);
            it->name = buf;
        }
    }
    
    vthis->m_highest_index--;

    
    // re-do the map lookup
    
    vthis->m_members_map.clear();
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
        vthis->m_members_map[it->name] = it->value;
}



// (METHOD) Array.slice
// Description: Returns a selection of elements of an Array in a new Array.
//
// Syntax: function Array.slice(start : Integer) : Array(Object)
// Syntax: function Array.slice(start : Integer, 
//                              end : Integer) : Array(Object)
//
// Remarks: Returns a new array containing all elements of array with an index 
//     greater or equal than |start| and less than |end| in order. If |start| or 
//     |end| is negative, the result of adding this negative number to the 
//     length of the array is used.  If |end| is not specified, all elements from the 
//     |start| to the |end| of the array are used. Note: slice() does not modify array.
//
// Param(start): The index of the element that will be the first element of 
//     the returned array.
// Param(end): The index of the first element that will not be included 
//     in the returned array.
//
// Returns: A new array containing all elements of the array from |start| to |end|, 
//     including the |start| element, but excluding the |end| element.

void Array::slice(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;

    retval->setObject(Array::createObject(env));
    Array* arr = (Array*)retval->getObject();
    
    int i, start, end;
    start = 0;
    end = vthis->m_members.size();
    
    if (env->getParamCount() < 1)
        return;
    
    start = env->getParam(0)->getInteger();
    
  
    if (env->getParamCount() >= 2)
        end = env->getParam(1)->getInteger();
        
    if (start < 0)
        start += vthis->m_members.size();
    if (start < 0)
        start = 0;
    if (end < 0)
        end += vthis->m_members.size();
    if (end < 0)
        end = 0;
        
        
    std::wstring name;
    wchar_t buf[32];
    
    size_t new_idx = 0;
       
    for (i = start; i < end; ++i)
    {
        swprintf(buf, 32, L"%d", i);
        name = buf;
        
        if (vthis->getMemberExists(name))
        {
            Value* val = vthis->getMember(name);
            
            swprintf(buf, 32, L"%d", new_idx);
            arr->setMember(buf, val);
        }
        
        new_idx++;
    }
}





class ArraySortLess
{
public:

    ArraySortLess(ValueObject* _obj, ExprEnv* _env, Value* _sortfcn)
    {
        obj = _obj;
        env = _env;
        sortfcn = _sortfcn;
    }
    
    bool operator()(const ValueMember& x,
                    const ValueMember& y) const                
    {
        if (sortfcn == NULL)
        {
            return (wcscmp(x.value->getString(), y.value->getString()) < 0) ? true : false;
        }
         else
        {
            Value retval;
            Value* params[2];
            params[0] = x.value;
            params[1] = y.value;
            env->getParser()->invoke(env, sortfcn, NULL, &retval, params, 2);
            
            return (retval.getInteger() < 0) ? true : false;
        }
        
        return false;
    }
     
public:

    ValueObject* obj;
    ExprEnv* env;
    Value* sortfcn;
};


// (METHOD) Array.sort
// Description: Sorts the elements of an Array.
//
// Syntax: function Array.sort() : Array(Object)
// Syntax: function Array.sort(compare_function : Function) : Array(Object)
//
// Remarks: Sorts the array and returns it using the |compare_function| function. If 
//     |compare_function| is not specified, the elements are converted to Strings and 
//     sorted according to the character encoding. Undefined elements will appear at 
//     the end of the array.
//
// Param(compare_function): A comparison function to be used for sorting. This function should 
//      take two arguments and return 1) a value less than zero if the first argument 
//      should appear before the second argument in the returned Array, 2) zero if it 
//      doesn't matter in which way the arguments appear in the returned Array, and 
//      3) a positive number otherwise.
//
// Returns: Returns the original array with the elements sorted.

void Array::sort(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;

    Value* sortfcn = NULL;
    if (env->getParamCount() >= 1)
        sortfcn = env->getParam(0);
     
     
    std::vector<ValueMember> non_numeric;
    std::vector<ValueMember> numeric;
    
    
    std::vector<ValueMember>::iterator it;
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
    {
        if (it->name.length() == 0 || wcschr(L"1234567890", it->name[0]))
        {
            numeric.push_back(*it);
        }
         else
        {
            non_numeric.push_back(*it);
        }
    }
     
        
    std::sort(numeric.begin(), numeric.end(), ArraySortLess(vthis, env, sortfcn));
    
    vthis->m_members = non_numeric;
    vthis->m_members.insert(vthis->m_members.end(), numeric.begin(), numeric.end());
    
    // because the array has been sorted, all the indexes are now off
    // so reset them with the following for loop

    reassignMemberIndexes(vthis);
    
    retval->setObject(vthis);
}

// (METHOD) Array.splice
// Description: Returns and replaces a selection of elements of an Array 
//     in a new Array with other elements.
//
// Syntax: function Array.splice(start : Integer) : Array(Object)
// Syntax: function Array.splice(start : Integer,
//                               length : Integer) : Array(Object)
// Syntax: function Array.splice(start : Integer,
//                               length : Integer, 
//                               newElem_1 : Object, 
//                               ..., 
//                               newElem_k : Object) : Array(Object)
//
// Param(start): The index of the first element that is to be replaced.
// Param(length): The number of elements that will be deleted from the array.
// Param(newElem_1, ..., newElem_k): Values that will be inserted into the array.
//
// Remarks: Deletes and returns the |length| elements of the array after |start|, 
//     including the element at |start|. If |length| is not supplied, all elements 
//     after |start| will be deleted. If |newElem_1|, ..., |newElem_k| are supplied, 
//     they will be inserted at |start| after the deletion.
//
// Returns: A new array containing the elements deleted from the array.

void Array::splice(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;

    retval->setObject(Array::createObject(env));
    
    if (env->getParamCount() < 1)
        return;
        
    Array* arr = (Array*)retval->getObject();
    
    int i, start, end;
    start = 0;
    end = vthis->getHighestIndex();
    
    start = env->getParam(0)->getInteger();
     
    if (env->getParamCount() >= 2)
    {
        end = start + env->getParam(1)->getInteger();
        if (end <= start)
            end = start;
    }
        
    wchar_t buf[32];
    std::wstring orig_name, new_name;
    
    // cut out the old values into a new array
    for (i = start; i < end; ++i)
    {
        swprintf(buf, 32, L"%d", i);
        orig_name = buf;
        
        swprintf(buf, 32, L"%d", i-start);
        new_name = buf;
        
        if (vthis->getMemberExists(orig_name))
        {
            arr->setMember(new_name, vthis->getMember(orig_name));
            vthis->deleteMember(orig_name);
        }
    }
    
    
    int total_to_insert = (int)env->m_param_count - 2;
    if (total_to_insert < 0)
        total_to_insert = 0;


    // shift all values after that in the original array
    std::vector<ValueMember>::iterator it;
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
    {
        if (it->name.length() > 0 && iswdigit(it->name[0]))
        {
            int num = wtoi(it->name.c_str());
            
            if (num >= end)
            {
                num -= (end-start);
                num += total_to_insert;
                
                swprintf(buf, 32, L"%d", num);
                it->name = buf;
            }
        }
    }
    
    vthis->m_highest_index -= (end-start);
    vthis->m_highest_index += total_to_insert;
    
    // re-do the map lookup
    
    vthis->m_members_map.clear();
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
    {
        vthis->m_members_map[it->name] = it->value;
    }


    // insert new values
    for (i = 0; i < total_to_insert; ++i)
    {
        swprintf(buf, 32, L"%d", start+i);
        orig_name = buf;
        
        vthis->setMember(orig_name, env->m_eval_params[i+2]);
    }

}



// (METHOD) Array.unshift
// Description: Prepends a number of elements to the beginning of an Array.
//
// Syntax: function Array.unshift(newElem_1 : Object, 
//                                ..., 
//                                newElem_k : Object) : Integer
//
// Remarks: Inserts |newElem_1|, ..., |newElem_k| at the beginning of array, shifts 
//     other elements forward accordingly, and returns the new length of the array.
//
// Param(newElem_1, ..., newElem_k): The values to insert at the beginning of the array.
//
// Returns: Returns the new length of the array.

void Array::unshift(ExprEnv* env, void*, Value* retval)
{
    ValueObject* vthis = getThisObject(env);
    if (!vthis)
        return;

    int i, param_count = (int)env->m_param_count;

    // shift numbers right
    std::vector<ValueMember>::iterator it;
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
    {
        if (it->name.length() == 0 || wcschr(L"1234567890", it->name[0]))
        {
            int num = wtoi(it->name.c_str());
            num += param_count;
            wchar_t buf[32];
            swprintf(buf, 32, L"%d", num);
            it->name = buf;
        }
    }
    
    vthis->m_highest_index += param_count;
    
    // reset lookup map
    vthis->m_members_map.clear();
    for (it = vthis->m_members.begin(); it != vthis->m_members.end(); ++it)
        vthis->m_members_map[it->name] = it->value;

    for (i = 0; i < param_count; ++i)
    {
        Value* v = env->m_eval_params[i];
        
        if (v->isObject())
        {
            Value vstr;
            v->toString(&vstr);
            vthis->setMemberI(i, &vstr);
        }
         else
        {
            vthis->setMemberI(i, v);
        }
    }

    retval->setInteger(vthis->getHighestIndex());
}






void Array::toString(Value* retval)
{
    makeStringRetval(this, L",", retval);
}







void Array::reassignMemberIndexes(ValueObject* obj)
{
    // makes sure that, if the order of the array was corrupted,
    // that the member names for the index values are reassigned
    
    int idx = 0;
    wchar_t buf[64];
    
    std::vector<ValueMember> non_numeric;
    std::vector<ValueMember> numeric;
    
    
    std::vector<ValueMember>::iterator it;
    for (it = obj->m_members.begin(); it != obj->m_members.end(); ++it)
    {
        if (it->name.length() == 0 || wcschr(L"1234567890", it->name[0]))
        {
            swprintf(buf, 64, L"%d", idx++);
            it->name = buf;
            numeric.push_back(*it);
        }
         else
        {
            non_numeric.push_back(*it);
        }
        
    }
    
    
    for (it = numeric.begin(); it != numeric.end(); ++it)
    {
        non_numeric.push_back(*it);
    }

    obj->m_members = non_numeric;
    obj->m_members_map.clear();
    
    
    for (it = obj->m_members.begin(); it != obj->m_members.end(); ++it)
    {
        obj->m_members_map[it->name] = it->value;
    }
}




// the 'forbidden' parameter prevents circular recursion

void Array::makeStringRetvalInternal(ValueObject* vthis,
                                     const std::wstring& delim,
                                     Value* retval,
                                     std::set<ValueObject*>& forbidden)
{
    std::wstring res;
    
    wchar_t buf[64];
    std::wstring name;
    
    Value* member;
    size_t i, member_count = vthis->getHighestIndex();
    
    // reserve some member (10 is an arbitrary number)
    res.reserve(member_count * 10);
    
    for (i = 0; i < member_count; ++i)
    {
        swprintf(buf, 64, L"%d", i);
        name = buf;
        
        if (vthis->getMemberExists(name))
        {
            member = vthis->getMember(name);
            
            if (member->isString())
            {
                res += member->getString();
            }
             else
            {
                if (member->isObject() && member->getObject()->isKindOf(Array::staticGetClassId()))
                {
                    Array* arr = (Array*)member->getObject();
                    if (forbidden.find(arr) == forbidden.end())
                    {
                        Value v;
                        forbidden.insert(arr);
                        arr->makeStringRetvalInternal(member->getObject(), delim, &v, forbidden);
                        res += v.getString();
                        forbidden.erase(arr);
                    }
                }
                 else if (!member->isNull())
                {
                    Value v;
                    vthis->getMember(name)->toString(&v);
                    res += v.getString();
                }
            }
        }
        
        if (i+1 < member_count)
            res += delim;
    }

    retval->setString(res.c_str(), res.length());
}


void Array::makeStringRetval(ValueObject* vthis, const std::wstring& delim, Value* retval)
{
    std::set<ValueObject*> forbidden;
    forbidden.insert(vthis);
    makeStringRetvalInternal(vthis, delim, retval, forbidden);
}


};

