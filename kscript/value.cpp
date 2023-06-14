/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2002-03-23
 *
 */


#include "kscript.h"
#include "jsstring.h"
#include "jsboolean.h"
#include "jsnumber.h"
#include "jsarray.h"
#include "jsobject.h"
#include "jsfunction.h"
#include "util.h"
#include <cwctype>
#include <cfloat>
#include <cmath>
#include <limits>
#include <cstring>


#ifdef _MSC_VER
#define strcasecmp stricmp
#define wcscasecmp wcsicmp
#define swprintf _snwprintf
#define fpclass _fpclass
#endif


#ifdef __APPLE__
#include "../kl/include/kl/portable.h"
#include "../kl/include/kl/string.h"
#endif


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


namespace kscript
{


ValueObject::ValueObject()
{
    m_value_return = NULL;
    m_prototype = NULL;
    m_highest_index = 0;
    m_in_destructor = false;
    m_parser = NULL;
    m_shadow = NULL;
}

ValueObject::~ValueObject()
{
    // protect against any further destroys
    m_base_ref_count = 2000000000;
    m_in_destructor = true;
    
    std::vector<ValueObjectEvents*>::iterator it;
    for (it = m_valobj_events.begin(); it != m_valobj_events.end(); ++it)
        (*it)->onDestroy(this);
    
    delete m_value_return;
    delete m_prototype;
    deleteAllMembers();
}

void ValueObject::registerEventListener(ValueObjectEvents* evt)
{
    if (m_in_destructor)
        return;
    
    m_valobj_events.push_back(evt);
}

void ValueObject::unregisterEventListener(ValueObjectEvents* evt)
{
    if (m_in_destructor)
        return;
    
    std::vector<ValueObjectEvents*>::iterator it;
    for (it = m_valobj_events.begin(); it != m_valobj_events.end(); ++it)
    {
        if (*it == evt)
        {
            m_valobj_events.erase(it);
            return;
        }
    }
}


Value* ValueObject::lookupMember(const std::wstring& name, bool follow_prototype)
{
    if (name == L"__proto__")
        return getPrototype();
    
    std::map<std::wstring, Value*>::iterator it;
    it = m_nonenum_members_map.find(name);
    if (it != m_nonenum_members_map.end())
        return it->second;
    it = m_members_map.find(name);
    if (it != m_members_map.end())
        return it->second;
        
    if (m_shadow)
    {
        kscript::Value v;
        if (m_shadow->getMember(name, &v))
        {
            kscript::Value* new_member = createMember(name, 0);
            v.eval(NULL, new_member);
            return new_member;
        }
    }
     
    if (follow_prototype && m_prototype && m_prototype->m_obj)
    {
        return m_prototype->m_obj->lookupMember(name, true);
    }

    return NULL;
}

std::wstring ValueObject::lookupMember(Value* value, bool follow_prototype)
{    
    std::vector<ValueMember>::iterator it, it_end;
    
    it_end = m_members.end();
    for (it = m_members.begin(); it < it_end; ++it)
    {
        if (value == it->value)
            return it->name;
    }
    
    it_end = m_nonenum_members.end();
    for (it = m_nonenum_members.begin(); it < it_end; ++it)
    {
        if (value == it->value)
            return it->name;
    }    
    
    if (follow_prototype && m_prototype && m_prototype->m_obj)
    {
        return m_prototype->m_obj->lookupMember(value, true);
    }

    return L"";
}

Value* ValueObject::createMember(const std::wstring& name, unsigned int attr)
{    
    std::vector<ValueMember>* parr;
    std::map<std::wstring, Value*>* pmap;
    
    if (attr & Value::attrDontEnum)
    {
        parr = &m_nonenum_members;
        pmap = &m_nonenum_members_map;
    }
     else
    {
        parr = &m_members;
        pmap = &m_members_map;
    }
    
    
    ValueMember v;
    
    std::map<std::wstring, Value*>::iterator it;
    if (name.length() > 0)
    {
        if (iswdigit(name[0]))
        {
            int index = wtoi(name.c_str());
            if (index >= m_highest_index)
                m_highest_index = index+1;
        }
        
        it = pmap->find(name);
        if (it != pmap->end())
            return it->second;
        
        v.name = name;
        v.value = new Value;
        v.value->setAttributes(attr);
        
        parr->push_back(v);
        (*pmap)[name] = v.value;
    }
     else
    {
        // use next available numeric index
        
        wchar_t buf[32];
        swprintf(buf, 32, L"%d", m_highest_index);
        buf[31] = 0;
        m_highest_index++;

        v.name = buf;
        v.value = new Value;
        v.value->setAttributes(attr);
        
        parr->push_back(v);
        (*pmap)[v.name] = v.value;
    }
    
    return v.value;
}

void ValueObject::setMember(const std::wstring& name, Value* value)
{
    kscript::Value* lm = lookupMember(name, false);
    if (!lm)
    {
        lm = createMember(name, Value::attrNone);
        if (!lm)
            return;
    }
    
    if (lm->getReadOnly())
        return;
    
    value->eval(NULL, lm);
}

void ValueObject::setMemberI(int idx, Value* value)
{
    wchar_t buf[24];
    swprintf(buf, 24, L"%d", idx);
    buf[23] = 0;
    setMember(buf, value);
}

void ValueObject::appendMember(Value* value)
{
    wchar_t buf[24];
    swprintf(buf, 24, L"%d", getHighestIndex());
    buf[23] = 0;
    setMember(buf, value);
}

void ValueObject::getMemberI(int idx, Value* retval)
{
    wchar_t buf[24];
    swprintf(buf, 24, L"%d", idx);
    buf[23] = 0;
    getMember(buf, retval);
}
 
Value* ValueObject::getMemberI(int idx)
{
    wchar_t buf[24];
    swprintf(buf, 24, L"%d", idx);
    buf[23] = 0;
    return getMember(buf);
}

void ValueObject::getMember(const std::wstring& name, Value* retval)
{
    kscript::Value* lm = lookupMember(name);
    if (!lm)
    {
        lm = createMember(name, Value::attrNone);
        if (!lm)
        {
            retval->setNull();
            return;
        }
    }
    
    lm->eval(NULL, retval);
}

Value* ValueObject::getMember(const std::wstring& name)
{
    kscript::Value* lm = lookupMember(name);
    if (!lm)
        lm = createMember(name, Value::attrNone);
    
    return lm;
}

size_t ValueObject::getRawMemberCount()
{
    return m_members.size();
}

std::wstring ValueObject::getRawMemberName(size_t idx)
{
    if (idx >= m_members.size())
        return L"";
        
    return m_members[idx].name;
}

Value* ValueObject::getRawMemberByIdx(size_t idx)
{
    if ((int)idx < 0)
    {
        getValueReturn()->setNull();
        return getValueReturn();
    }
    
    if (idx < m_members.size())
    {
        return m_members[idx].value;
    }
    
    // create a member with the next available highest index
    return createMember(L"", Value::attrNone);
}

Value* ValueObject::getValueReturn()
{
    if (!m_value_return)
        m_value_return = new Value;
    return m_value_return;
}

bool ValueObject::getMemberExists(const std::wstring& name)
{
    Value* lm = lookupMember(name);
    return (lm ? true : false);
}

void ValueObject::deleteAllMembers()
{
    std::vector<ValueMember>::iterator it;
    for (it = m_members.begin(); it != m_members.end(); ++it)
        delete it->value;
    
    for (it = m_nonenum_members.begin(); it != m_nonenum_members.end(); ++it)
        delete it->value;
    
    m_members.clear();
    m_nonenum_members.clear();
    m_members_map.clear();
    m_nonenum_members_map.clear();
}

bool ValueObject::deleteMember(const std::wstring& name)
{
    if (!lookupMember(name))
    {
        // member doesn't exist
        return true;
    }
    
    std::vector<ValueMember>::iterator it;
    for (it = m_nonenum_members.begin(); it != m_nonenum_members.end(); ++it)
    {
        if (name == it->name)
        {
            if (it->value->getDontDelete())
                return false;
            
            delete it->value;
            m_nonenum_members_map.erase(it->name);
            m_nonenum_members.erase(it);
            
            // return now, because, if it's found in m_nonenum_members,
            // it won't be in m_members
            return true;
        }
    }
        
    for (it = m_members.begin(); it != m_members.end(); ++it)
    {
        if (name == it->name)
        {
            if (it->value->getDontDelete())
                return false;
                
            delete it->value;
            m_members_map.erase(it->name);
            m_members.erase(it);
            return true;
        }
    }
    
    return true;
}

bool ValueObject::deleteMemberI(int idx)
{
    wchar_t buf[24];
    swprintf(buf, 24, L"%d", idx);
    buf[23] = 0;
    return deleteMember(buf);
}


void ValueObject::toString(Value* retval)
{
    std::wstring res = L"[object ";
    res += getClassName();
    res += L"]";
    retval->setString(res.c_str());
}

void ValueObject::toNumber(kscript::Value* retval)
{
    // this should follow ECMA-262 8.6.2.6 more closely, but for
    // now we'll just return NaN
    retval->setNaN();
}

void ValueObject::toBoolean(kscript::Value* retval)
{
    // object values evaluate to boolean true (ECMA-262 section 9.2)
    retval->setBoolean(true);
}

void ValueObject::toPrimitive(kscript::Value* retval, int preferred_type)
{
    // this should follow ECMA-262 8.6.2.6 more closely, but for
    // now we'll just call toString
    toString(retval);
}

void ValueObject::setClassName(const std::wstring& class_name)
{
    m_class_name = class_name;
}

const std::wstring& ValueObject::getDefaultClassName() const
{
    static std::wstring empty_string;
    return empty_string;
}

const std::wstring& ValueObject::getClassName() const
{
    if (m_class_name.empty())
        return getDefaultClassName();
    return m_class_name;
}

bool ValueObject::isKindOf(const wchar_t* class_name) const
{
    return false;
}

bool ValueObject::isKindOf(int class_id) const
{
    return false;
}

void ValueObject::staticConstructor(kscript::ExprEnv* env,
                                    void* param,
                                    kscript::Value* retval)
{
    env->setRuntimeError(kscript::rterrorMissingObject);
}

bool ValueObject::customConstructor(int class_id,
                                    kscript::ExprEnv* env,
                                    void* param,
                                    kscript::Value* retval)
{
    return false;
}


Value* ValueObject::getPrototype()
{
    if (!m_prototype)
    {
        m_prototype = new Value;
        m_prototype->setObject(new ValueObject);
    }
    return m_prototype;
}

void ValueObject::setPrototype(kscript::Value* val)
{
    if (!m_prototype)
        m_prototype = new Value;
    m_prototype->setValue(val);
}

bool ValueObject::hasPrototype()
{
    return m_prototype ? true : false;
}


Value::Value()
{
    m_type = typeUndefined;
    m_method = methodNormal;
    m_datalen = 0;
    m_value_return = NULL;
    m_alloc_len = 0;
    m_obj = NULL;
    m_attr = 0;
    m_scale = 0xff;
}

Value::Value(int value)
{
    m_type = typeInteger;
    m_intval = value;
    
    m_method = methodNormal;
    m_datalen = 0;
    m_value_return = NULL;
    m_alloc_len = 0;
    m_obj = NULL;
    m_attr = 0;
    m_scale = 0xff;
}

Value::Value(double value)
{
    m_type = typeDouble;
    m_dblval = value;
    
    m_method = methodNormal;
    m_datalen = 0;
    m_value_return = NULL;
    m_alloc_len = 0;
    m_obj = NULL;
    m_attr = 0;
    m_scale = 0xff;
}

Value::Value(const wchar_t* str, int length)
{
    m_type = typeUndefined;
    m_method = methodNormal;
    m_datalen = 0;
    m_value_return = NULL;
    m_alloc_len = 0;
    m_obj = NULL;
    m_attr = 0;
    m_scale = 0xff;
    
    setString(str, length);
}

Value::Value(const std::wstring& str)
{
    m_type = typeUndefined;
    m_method = methodNormal;
    m_datalen = 0;
    m_value_return = NULL;
    m_alloc_len = 0;
    m_obj = NULL;
    m_attr = 0;
    m_scale = 0xff;
    
    setString(str);
}

Value::~Value()
{
    reset();
    delete m_value_return;
} 

bool Value::isNaN()
{
    if (isNumber())
    {
        return isnan(getDouble()) != 0 ? true : false;
    }
     else if (isString())
    {
        double d = getDouble();
        return isnan(d) != 0 ? true : false;
    }
     else if (isBoolean())
    {
        return false;
    }
     else
    {
        return true;
    }
    
    return true;
}

bool Value::isAnyInfinity()
{
    static const double cmpd1 = std::numeric_limits<double>::infinity();
    static const double cmpd2 = -std::numeric_limits<double>::infinity();
    double d = getDouble();
    return (d == cmpd1 || d == cmpd2) ? true : false;
}


bool Value::isPositiveInfinity()
{
    static const double cmpd = std::numeric_limits<double>::infinity();
    return (getDouble() == cmpd) ? true : false;
}

bool Value::isNegativeInfinity()
{
    static const double cmpd = -std::numeric_limits<double>::infinity();
    return (getDouble() == cmpd) ? true : false;
}

bool Value::isAnyZero()
{
    if (m_type == typeInteger)
    {
        return (getInteger() == 0) ? true : false;
    }
     else
    {
        return (getDouble() == 0.0) ? true : false;
    }
}

int Value::getSign()
{
    if (m_type == typeInteger)
    {
        if (getInteger() < 0)
            return -1;
             else
            return 1;
    }
    
    if (isNegativeZero() || isNegativeInfinity())
        return -1;
    if (isPositiveInfinity())
        return 1;
        
    double d = getDouble();
    return getDoubleSign(&d);
}
    
    
bool Value::isNegativeZero()
{
    double d = getDouble();
    return (d == 0 && getDoubleSign(&d) == -1) ? true : false;
}

bool Value::isPositiveZero()
{
    double d = getDouble();
    return (d == 0 && getDoubleSign(&d) == 1) ? true : false;
}


void Value::clear()
{
    if (m_method == methodNormal)
    {
        if (m_type == typeString || m_type == typeBinary)
        {
            if (m_genericptr)
                free(m_genericptr);
            m_genericptr = NULL;
            m_alloc_len = 0;
        }
         else if (m_type == typeFunction)
        {
            if (m_scope && (uintptr_t)m_scope > 1)
            {
                m_scope->unref();
                m_scope = NULL;
            }
        }
         else if (m_type == typeExprElement)
        {
            delete m_exprptr;
            m_exprptr = NULL;
        }
        
        m_scale = 0xff;
        m_datalen = 0;
    }
}

void Value::releaseObject()
{
    if (m_obj)
    {
        m_obj->baseUnref();
        m_obj = NULL;
    }
}

void Value::releaseScope()
{
    if (isFunction() && m_scope && (uintptr_t)m_scope > 1)
    {
        m_scope->unref();
        m_scope = NULL;
    }
}

void Value::reset()
{
    clear();
    
    if (m_obj)
    {
        m_obj->baseUnref();
        m_obj = NULL;
    }

    if (m_value_return)
        m_value_return->reset();

    m_type = typeUndefined;
}

void Value::setNull()
{
    clear();
    m_type = Value::typeNull;
}

void Value::setNaN()
{
    clear();
    m_type = Value::typeDouble;
    m_dblval = std::numeric_limits<double>::quiet_NaN();
}

void Value::setPositiveInfinity()
{
    clear();
    m_type = Value::typeDouble;
    m_dblval = std::numeric_limits<double>::infinity();
}

void Value::setNegativeInfinity()
{
    clear();
    m_type = Value::typeDouble;
    m_dblval = -std::numeric_limits<double>::infinity();
}

void Value::setArray(ExprEnv* env)
{
    if (env)
    {
        Array* a = Array::createObject(env);
        setObject(a);
    }
     else
    {
        Array* a = new Array;
        setObject(a);
    }
}


void Value::checkObjectExists()
{
    if (!m_obj)
    {
        m_obj = new ValueObject;
        m_obj->baseRef();
    }
}


void Value::setObject()
{
    reset();
    m_type = Value::typeObject;
    Object* obj = new Object;
    m_obj = obj;
    m_obj->baseRef();
}

void Value::setObject(ValueObject* obj)
{
    reset();
    m_type = Value::typeObject;
    m_obj = obj;
    if (m_obj)
        m_obj->baseRef();
}


void Value::setStringLen(unsigned int len)
{
    allocString(len);
    m_datalen = len;
}

void Value::allocString(unsigned int len)
{
    len = (len + 1) * sizeof(wchar_t);
    m_type = Value::typeString;

    if (len <= m_alloc_len)
        return;

    if (m_alloc_len == 0)
    {
        m_genericptr = malloc(len);
        m_alloc_len = len;
        return;
    }

    m_genericptr = realloc(m_genericptr, len);
    m_alloc_len = len;
}

void Value::allocMem(unsigned int len)
{
    if (len <= m_alloc_len)
        return;

    if (m_alloc_len == 0)
    {
        m_genericptr = malloc(len);
        m_alloc_len = len;
        return;
    }

    m_genericptr = realloc(m_genericptr, len);
    m_alloc_len = len;
}


Value* Value::lookupMember(const std::wstring& name, bool follow_prototype)
{
    if (isRef())
        return m_refptr->lookupMember(name, follow_prototype);
    
    if (!m_obj)
        checkObjectExists();
    
    if (isString())
    {
        if (name.length() > 0 && iswdigit(name[0]))
        {
            int offset = wtoi(name.c_str());
            if (offset >= 0 && offset <= getDataLen())
            {
                m_obj->getValueReturn()->setString(getString()+offset, 1);
                return m_obj->m_value_return;
            }
             else
            {
                m_obj->getValueReturn()->setNull();
                return m_obj->m_value_return;
            }
        }
        
        if (name == L"length")
        {
            m_obj->getValueReturn()->setInteger(getDataLen());
            return m_obj->m_value_return;
        }
        
        Value* method = String::lookupStringMethod(name);
        if (method)
        {
            return method;
        }
    }
     else if (isNumber())
    {
        Value* method = Number::lookupNumberMethod(name);
        if (method)
        {
            return method;
        }
    }
     else if (isBoolean())
    {
        Value* method = Boolean::lookupBooleanMethod(name);
        if (method)
        {
            return method;
        }
    }

    return getObject()->lookupMember(name, follow_prototype);
}
    

Value* Value::getMember(const std::wstring& name)
{
    /*
    Value* v = lookupMember(name, true);
    if (v)
        return v;
    
    return createMember(name, Value::attrNone);
    */
    

    if (isRef())
        return m_refptr->getMember(name);
     
    if (!m_obj)
        checkObjectExists();

    if (isString())
    {
        if (name.length() > 0 && iswdigit(name[0]))
        {
            int offset = wtoi(name.c_str());
            if (offset >= 0 && offset <= getDataLen())
            {
                m_obj->getValueReturn()->setString(getString()+offset, 1);
                return m_obj->m_value_return;
            }
             else
            {
                m_obj->getValueReturn()->setNull();
                return m_obj->m_value_return;
            }
        }
        
        if (name == L"length")
        {
            m_obj->getValueReturn()->setInteger(getDataLen());
            return m_obj->m_value_return;
        }
        
        Value* method = String::lookupStringMethod(name);
        if (method)
        {
            return method;
        }
    }
     else if (isNumber())
    {
        Value* method = Number::lookupNumberMethod(name);
        if (method)
        {
            return method;
        }
    }
     else if (isBoolean())
    {
        Value* method = Boolean::lookupBooleanMethod(name);
        if (method)
        {
            return method;
        }
    }

    return getObject()->getMember(name);
}

Value* Value::getMemberI(int idx)
{
    wchar_t buf[24];
    swprintf(buf, 24, L"%d", idx);
    buf[23] = 0;
    return getMember(buf);
}


void Value::appendMember(Value* value)
{
    if (isRef())
    {
        m_refptr->appendMember(value);
        return;
    }

    getObject()->appendMember(value);
}

Value* Value::createMember(const std::wstring& name, unsigned int attr)
{
    if (isRef())
        return m_refptr->createMember(name, attr);

    return getObject()->createMember(name, attr);
}


void Value::setMember(const std::wstring& name, Value* value)
{
    if (isRef())
    {
        m_refptr->setMember(name, value);
        return;
    }

    getObject()->setMember(name, value);
}


void Value::setMemberI(int idx, Value* value)
{
    if (isRef())
    {
        m_refptr->setMemberI(idx, value);
        return;
    }

    getObject()->setMemberI(idx, value);
}


bool Value::getMemberExists(const std::wstring& name)
{
    if (isRef())
        return m_refptr->getMemberExists(name);
        
    if (!m_obj)
        return false;
        
    return getObject()->getMemberExists(name);
}


void Value::toPrimitive(Value* retval, int preferred_type)
{
    if (isRef())
    {
        m_refptr->toPrimitive(retval, preferred_type);
        return;
    }

    if (!isObject())
    {
        // if we are the same object
        // as the return result, do nothing
        if (retval == this)
            return;
        
        eval(NULL, retval);
        return;
    }
     else
    {
        ValueObject* obj = getObject();
        obj->baseRef();
        obj->toPrimitive(retval, preferred_type);
        obj->baseUnref();
    }
}

void Value::toString(Value* retval)
{
    if (isRef())
    {
        m_refptr->toString(retval);
        return;
    }

    if (m_type == Value::typeString)
    {
        if (this == retval)
            return;
        eval(NULL, retval);
        return;
    }
     else if (m_type == Value::typeInteger)
    {
        wchar_t buf[32];
        swprintf(buf, 31, L"%d", getInteger());
        retval->setString(buf);
        return;
    }
     else if (m_type == Value::typeDouble)
    {    
        if (isNaN())
        {
            retval->setString(L"NaN");
        }
         else if (isPositiveInfinity())
        {
            retval->setString(L"Infinity");
        }
         else if (isNegativeInfinity())
        {
            retval->setString(L"-Infinity");
        }
         else
        {
            if (m_scale == 0xff)
            {
                std::wstring dstr = dbltostr(getDouble());
                retval->setString(dstr.c_str(), (int)dstr.length());
            }
             else
            {
                // fixed scale
                wchar_t buf[255];
                swprintf(buf, 255, L"%.*f", (int)m_scale, getDouble());
                retval->setString(buf);
            }
        }
        return;
    }
     else if (m_type == Value::typeBoolean)
    {
        if (getBoolean())
            retval->setString(L"true");
             else
            retval->setString(L"false");
        return;
    }
     else if (m_type == Value::typeObject || m_type == Value::typeFunction)
    {
        ValueObject* obj = getObject();
        obj->baseRef();
        obj->toString(retval);
        obj->baseUnref();
        return;
    }
     else if (m_type == Value::typeUndefined)
    {
        retval->setString(L"undefined", 9);
        return;
    }
     else if (m_type == Value::typeNull)
    {
        retval->setString(L"null", 4);
        return;
    }
    
    retval->setString(L"");
}

void Value::toNumber(Value* retval)
{
    if (isRef())
    {
        m_refptr->toNumber(retval);
        return;
    }

    if (m_type == Value::typeDouble)
    {
        if (this == retval)
            return;
        retval->setDouble(getDouble());
    }
     else if (m_type == Value::typeInteger)
    {
        if (this == retval)
            return;
        retval->setInteger(getInteger());
    }
     else if (m_type == Value::typeBoolean)
    {
        retval->setInteger(getBoolean() ? 1 : 0);
    }
     else if (isString())
    {
        parseNumericConstant(getString(), retval, NULL);
    }
     else if (m_type == Value::typeObject)
    {
        ValueObject* obj = getObject();
        obj->baseRef();
        obj->toNumber(retval);
        obj->baseUnref();
    }
     else if (m_type == typeUndefined)
    {
        retval->setNaN();
    }
     else
    {
        retval->setDouble(0.0);
    }
}

void Value::toBoolean(Value* retval)
{
    if (isRef())
    {
        m_refptr->toBoolean(retval);
        return;
    }

    if (m_type == Value::typeBoolean)
    {
        retval->setBoolean(m_boolval);
    }
     else if (isNull())
    {
        retval->setBoolean(false);
    }
     else if (m_type == Value::typeInteger)
    {
        retval->setBoolean(m_intval != 0 ? true : false);
    }
     else if (m_type == Value::typeDouble)
    {
        if (isNaN())
        {
            retval->setBoolean(false);
        }
         else
        {
            retval->setBoolean(dblcompare(m_dblval, 0.0) != 0 ? true : false);
        }
    }
     else if (isString())
    {
        if (m_datalen == 0)
        {
            retval->setBoolean(false);
        }
         else
        {
            retval->setBoolean(true);
        }
    }
     else if (m_type == Value::typeObject)
    {
        ValueObject* obj = getObject();
        obj->baseRef();
        obj->toBoolean(retval);
        obj->baseUnref();
    }
     else
    {
        retval->setBoolean(false);
    }
}



void Value::setDouble(double num)
{
    clear();
    m_type = typeDouble;
    /*
    if (m_type != typeDouble)
    {
        reset();
        m_type = typeDouble;
    }
    */
    
    switch (m_method)
    {
        case methodNormal:
            m_dblval = num;
            return;
        case methodGetVal:
            return;
    }
}

void Value::setInteger(int num)
{
    clear();
    m_type = typeInteger;
    /*
    if (m_type != typeInteger)
    {
        reset();
        m_type = typeInteger;
    }
    */
    
    switch (m_method)
    {
        case methodNormal:
            m_intval = num;
            return;
        case methodGetVal:
            return;
    }
}

double Value::getDouble()
{
    if (m_type == typeDouble)
    {
        if (m_method == methodNormal)
        {
            return m_dblval;
        }
         else
        {
            Value v;
            eval(NULL, &v);
            return v.m_dblval;
        }
    }
     else if (m_type == typeInteger)
    {
        if (m_method == methodNormal)
        {
            return m_intval;
        }
         else
        {
            Value v;
            eval(NULL, &v);
            return v.m_intval;
        }
    }
     else
    {
        Value v;
        toNumber(&v);
        return v.getDouble();
    }

    return 0.0;
}

int Value::getInteger()
{
    if (m_type == typeDouble)
    {        
        if (m_method == methodNormal)
        {
            return doubleToInt32(m_dblval);
        }
         else
        {
            Value v;
            eval(NULL, &v);
            return doubleToInt32(v.m_dblval);
        }
    }
     else if (m_type == typeInteger)
    {
        if (m_method == methodNormal)
        {
            return m_intval;
        }
         else
        {
            Value v;
            eval(NULL, &v);
            return v.m_intval;
        }
    }
     else if (m_type == typeString)
    {
        Value v;
        parseNumericConstant(getString(), &v, NULL);
        return v.getInteger();
    }
     else
    {
        Value v;
        toNumber(&v);
        return v.getInteger();
    }

    return 0;
}

void Value::setString(const std::string& str)
{
    size_t i, len = str.length();
    
    setStringLen((unsigned int)len);

    const char* src = str.c_str();
    wchar_t* dest = getString();
    
    for (i = 0; i < len; ++i)
    {
        *(dest+i) = (unsigned char)*(src+i);
    }

    *(dest+len) = 0;
}

void Value::setString(const wchar_t* str, int len)
{
    if (m_type != typeString)
        reset();
   
    m_type = typeString;
    m_method = methodNormal;
     
    if (len == -1)
    {
        if (*str == 0)
            m_datalen = 0;
             else
            m_datalen = (unsigned int)wcslen(str);
        allocMem((m_datalen+1)*sizeof(wchar_t));
        memcpy(m_strptr, str, (m_datalen+1)*sizeof(wchar_t));
    }
     else
    {
        m_datalen = len;
        allocMem((len+1)*sizeof(wchar_t));
        memcpy(m_strptr, str, len*sizeof(wchar_t));
        *(m_strptr+len) = 0;
    }
}

void Value::appendString(const wchar_t* str, int len)
{
    if (len == -1)
        len = (int)wcslen(str);

    if (!isString())
    {
        allocString(len);
        setString(str, len);
        return;
    }

    if ((m_datalen + len + 1)*sizeof(wchar_t) >= m_alloc_len)
    {
        int additional_space = m_datalen/10;
        if (additional_space <= len+100)
            additional_space = len+100;

        allocString(m_datalen + additional_space);
        memcpy(m_strptr+m_datalen, str, len*sizeof(wchar_t));
        m_datalen += len;
        *(m_strptr+m_datalen) = 0;
    }
     else
    {
        memcpy(m_strptr+m_datalen, str, len*sizeof(wchar_t));
        m_datalen += len;
        *(m_strptr+m_datalen) = 0;
    }
}


void Value::setBinary(unsigned char* data, int len, int copy_len)
{
    m_type = typeBinary;
    m_method = methodNormal;

    allocMem(len);
    memset(m_binptr, 0, len);
    memcpy(m_binptr, data, copy_len == -1 ? len : copy_len);
    m_datalen = len;
}

void Value::setRef(Value* v)
{
    clear();

    if (v)
    {
        m_type = Value::typeRef;
        m_refptr = v;
    }
     else
    {
        setNull();
    }
}

void Value::setFunction(Function* func, ExprEnv* scope)
{
    reset();

    m_type = Value::typeFunction;
    m_funcptrval = func;
    m_scope = scope;
    if ((uintptr_t)m_scope > 1)
        m_scope->ref();
        
    m_obj = func;
    m_obj->baseRef();
}



void Value::setFunction(ExprBindFunc func_ptr, void* binding_param)
{
    Function* info = new Function;
    info->m_func_ptr = func_ptr;
    info->m_binding_param = binding_param;
    
    setFunction(info);
}


void Value::setExprElement(ExprElement* expr)
{
    reset();

    m_type = Value::typeExprElement;
    m_exprptr = expr;
}


wchar_t* Value::getString()
{
    if (m_type == typeString)
    {
        if (m_method == methodNormal)
        {
            return m_strptr;
        }
         else
        {
            if (!m_value_return)
                m_value_return = new Value;
            
            eval(NULL, m_value_return);
            return m_value_return->getString();
        }
    }
     else
    {
        if (!m_value_return)
            m_value_return = new Value;
        toString(m_value_return);
        if (m_value_return->isString())
            return m_value_return->getString();
    }

    static wchar_t empty_string[1] = { '\0' };
    return empty_string;
}

ExprDateTime Value::getDateTime()
{
    ExprDateTime e;
    getDateTime(&e);
    return e;
}

void Value::getDateTime(ExprDateTime* dest)
{
    if (m_type == typeDateTime)
    {
        switch (m_method)
        {
            case methodNormal:
                memcpy(dest, &m_datetimeval, sizeof(ExprDateTime));
                break;
            case methodGetVal:
            {
                Value v;
                eval(NULL, &v);
                memcpy(dest, &(v.m_datetimeval), sizeof(ExprDateTime));
            }
            break;
        }
    }

    if (m_type == typeString)
    {
        // implicit string-to-date conversion
        str2date(dest, getString());
    }
}


void Value::setDateTime(int year,
                        int month,
                        int day,
                        int hour,
                        int min,
                        int sec,
                        int sec1000)
{
    clear();
    m_method = Value::methodNormal;
    m_type = Value::typeDateTime;

    m_datetimeval.date = dateToJulian(year, month, day);
    m_datetimeval.time = (hour*3600000) + (min*60000) + (sec*1000) + sec1000;
}


void Value::setDateTime(unsigned int date, unsigned int time)
{
    clear();
    m_method = Value::methodNormal;
    m_type = Value::typeDateTime;

    m_datetimeval.date = date;
    m_datetimeval.time = time;
}


void Value::setBoolean(bool b)
{
    clear();
    /*
    if (m_type != typeBoolean)
    {
        reset();
        m_type = typeBoolean;
    }
    */
    
    m_boolval = b;
    m_type = typeBoolean;
    m_method = methodNormal;
}

bool Value::getBoolean()
{
    if (m_type == typeBoolean)
    {
        if (m_method == methodNormal)
        {
            return m_boolval;
        }
         else
        {
            Value v;
            eval(NULL, &v);
            return v.m_boolval;
        }
    }
     else
    {
        Value v;
        toBoolean(&v);
        return v.getBoolean();
    }
    
    return false;
}

void Value::copyFrom(kscript::Value* src_val)
{
    if (src_val->m_method == methodNormal)
    {
        src_val->eval(NULL, this);
    }
     else
    {
        memcpy(this, src_val, sizeof(Value));
        m_value_return = NULL;
        if (m_obj)
        {
            m_obj->baseRef();
        }
    }
}

int Value::eval(ExprEnv* env, Value* retval)
{
    if (m_method == methodNormal)
    {
        if (retval->m_obj)
        {
            retval->m_obj->baseUnref();
            retval->m_obj = NULL;
        }
    
        switch (m_type)
        {                
            case typeString:
                retval->setString(m_strptr, m_datalen);
                break;

            case typeDouble:
                retval->setDouble(m_dblval);
                break;

            case typeInteger:
                retval->setInteger(m_intval);
                break;

            case typeBoolean:
                retval->setBoolean(m_boolval);
                break;

            case typeFunction:
                retval->setFunction(m_funcptrval, m_scope);
                return evalSucceeded;
                
            case typeObject:
                retval->reset();
                retval->m_type = Value::typeObject;
                break;
            
            case typeNull:
                retval->reset();
                retval->m_type = Value::typeNull;
                break;
                
            case typeUndefined:
                retval->reset();
                break;
                
            case typeExprElement:
                return m_exprptr->eval(env, retval);
            
            case typeRef:
                return m_refptr->eval(env, retval);
                
            default:
            {
                retval->reset();
                memcpy(retval, this, sizeof(Value));
                retval->m_value_return = NULL;
                retval->m_attr = 0;
                break;
            }
        }
        
                        
        if (m_obj)
        {
            retval->m_obj = m_obj;
            retval->m_obj->baseRef();
        }
        
        return evalSucceeded;
    }

    if (m_method == methodGetVal)
    {
        m_getvalptr(env, m_getvalparam, retval);
        return evalSucceeded;
    }

    return evalFailed;
}

int Value::getType()
{
    return m_type;
}


Value* Value::getLvalue(ExprEnv* env)
{
    return this;
}




};  // namespace xd

