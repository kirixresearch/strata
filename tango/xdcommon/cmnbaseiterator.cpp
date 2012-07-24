/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-23
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "xdcommon.h"
#include "cmnbaseiterator.h"
#include "util.h"
#include "../../kscript/kscript.h"


class CmnBaseIteratorBindInfo
{
public:
    tango::IIterator* iter;
    tango::objhandle_t handle;
};


CommonBaseIterator::CommonBaseIterator()
{
}

CommonBaseIterator::~CommonBaseIterator()
{
    // -- clear out all of our cruft --
    std::vector<CmnBaseIteratorBindInfo*>::iterator it;
    for (it = m_bindings.begin();
         it != m_bindings.end(); ++it)
    {
        delete (*it);
    }
}

static void _bindFieldString(kscript::ExprEnv*,
                             void* param,
                             kscript::Value* retval)
{
    CmnBaseIteratorBindInfo* info = (CmnBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    retval->setString(info->iter->getWideString(info->handle));
}


static void _bindFieldDouble(kscript::ExprEnv*,
                             void* param,
                             kscript::Value* retval)
{
    CmnBaseIteratorBindInfo* info = (CmnBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    retval->setDouble(info->iter->getDouble(info->handle));
}

static void _bindFieldInteger(kscript::ExprEnv*,
                              void* param,
                              kscript::Value* retval)
{
    CmnBaseIteratorBindInfo* info = (CmnBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    retval->setInteger(info->iter->getInteger(info->handle));
}

static void _bindFieldDateTime(kscript::ExprEnv*,
                               void* param,
                               kscript::Value* retval)
{
    CmnBaseIteratorBindInfo* info = (CmnBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    tango::datetime_t dt, d, t;
    dt = info->iter->getDateTime(info->handle);
    d = dt >> 32;
    t = dt & 0xffffffff;
    retval->setDateTime((unsigned int)d, (unsigned int)t);
}

static void _bindFieldBoolean(kscript::ExprEnv*,
                              void* param,
                              kscript::Value* retval)
{
    CmnBaseIteratorBindInfo* info = (CmnBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    retval->setBoolean(info->iter->getBoolean(info->handle));
}

tango::IStructurePtr CommonBaseIterator::getParserStructure()
{
    // default is to just use the normal iterator structure
    // as source fields for the expression.  This can, however,
    // be overridden by the derived class.  Just as long as the field
    // handles can be gotten from getHandle with the names
    // from this structure
    
    return getStructure();
}


struct CommonBaseIteratorParseHookInfo
{
    CommonBaseIterator* iter;
    tango::IStructurePtr structure;
};


static bool script_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        tango::IStructurePtr& structure = ((CommonBaseIteratorParseHookInfo*)hook_info.hook_param)->structure;
        CommonBaseIterator* iter = ((CommonBaseIteratorParseHookInfo*)hook_info.hook_param)->iter;
        
        if (hook_info.expr_text.length() > 1 &&
            hook_info.expr_text[0] == '[' &&
            hook_info.expr_text[hook_info.expr_text.length()-1] == ']')
        {
            // remove brackets from beginning and end e.g. [Field Name] => Field Name
            hook_info.expr_text = hook_info.expr_text.substr(0, hook_info.expr_text.length()-1);
            hook_info.expr_text.erase(0, 1);
        }


        tango::IColumnInfoPtr colinfo = structure->getColumnInfo(hook_info.expr_text);
        if (colinfo.isNull())
            return false;
        
        tango::objhandle_t handle = iter->getHandle(hook_info.expr_text);
        
        if (handle == 0)
            return false;
       
        CmnBaseIteratorBindInfo* bind_info = new CmnBaseIteratorBindInfo;
        bind_info->iter = iter;
        bind_info->handle = handle;
        
        
        kscript::Value* val = new kscript::Value;
        switch (colinfo->getType())
        {
            case tango::typeCharacter:
            case tango::typeWideCharacter:
                val->setGetVal(kscript::Value::typeString,
                               _bindFieldString,
                               (void*)bind_info);
                break;

            case tango::typeNumeric:
            case tango::typeDouble:
                val->setGetVal(kscript::Value::typeDouble,
                               _bindFieldDouble,
                               (void*)bind_info);
                break;

            case tango::typeInteger:
                val->setGetVal(kscript::Value::typeInteger,
                               _bindFieldInteger,
                               (void*)bind_info);
                break;

            case tango::typeDate:
            case tango::typeDateTime:
                val->setGetVal(kscript::Value::typeDateTime,
                               _bindFieldDateTime,
                               (void*)bind_info);
                break;

            case tango::typeBoolean:
                val->setGetVal(kscript::Value::typeBoolean,
                               _bindFieldBoolean,
                               (void*)bind_info);
                break;
            
            default:
                delete bind_info;
                delete val;
                return false;
        }
        
        hook_info.res_element = val;
        return true;
    }
    
    return false;
}

kscript::ExprParser* CommonBaseIterator::parse(const std::wstring& expr)
{
    if (expr.length() == 0)
        return NULL;
        
    kscript::ExprParser* parser = createExprParser();


    CommonBaseIteratorParseHookInfo info;
    info.iter = this;
    info.structure = getParserStructure();
    
    parser->setParseHook(kscript::ExprParseHookInfo::typeIdentifier,
                         script_parse_hook,
                         (void*)&info);
    
    if (!parser->parse(expr))
    {
        delete parser;
        return NULL;
    }

    return parser;
}
