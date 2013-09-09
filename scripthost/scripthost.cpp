/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-29
 *
 */


#include "scripthost.h"
#include "db.h"
#include "memory.h"
#include "file.h"
#include "filetransfer.h"
#include "http.h"
#include "xml.h"
#include "environment.h"
#include "system.h"
#include "native.h"
#include "base64.h"
#include "crypt.h"
#include "hash.h"
#include "process.h"
#include <kl/utf8.h>
#include <kl/thread.h>
#include <kl/file.h>
#include <kl/memory.h>
#include <kl/string.h>



namespace scripthost
{


// -- ScriptHostBase class implementation --

// you can derive your class from kscript::ValueObject if you want;
// deriving from this intermediate class provides a way of getting
// the current Application* object as well as the current ScriptHost* object;
// In order for this to work, you have to call initComponent() from
// your object's MyObject::constructor method

ScriptHostBase::ScriptHostBase()
{
    m_script_host = NULL;
}

ScriptHost* ScriptHostBase::getScriptHost()
{
    // note if this is returning NULL, you probably forgot to call
    // FormComponent::initComponent() from your component's constructor

    return m_script_host;
}


void ScriptHostBase::initComponent(kscript::ExprEnv* env)
{
/*
    if (m_app.getApp())
        return;
        
    kscript::ExprParser* parser = env->getParser();
    m_script_host = (ScriptHost*)parser->getExtraLong();
    
    kscript::Value* val = parser->getBindVariable(L"Application");
    Application* app = (Application*)val->getObject();
    
    m_app.setApp(app);
    */
}



void func_exit(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    env->setRuntimeError(kscript::rterrorExit);
}



// -- ScriptHost implementation --

ScriptHost::ScriptHost()
{
    m_error_code = 0;
    m_error_line = 0;
    m_error_offset = 0;
    m_error_string = L"";

    m_expr = new kscript::ExprParser(kscript::optionLanguageECMAScript);
    m_expr->setExtraLong((long)this);
    
    m_retval = new kscript::Value;


    
    // -- non-gui --
    Directory::compiletimeBind(m_expr);
    DriveInfo::compiletimeBind(m_expr);
    Environment::compiletimeBind(m_expr);
    File::compiletimeBind(m_expr);
    FileAccess::compiletimeBind(m_expr);
    FileMode::compiletimeBind(m_expr);
    FileShare::compiletimeBind(m_expr);
    FileTransfer::compiletimeBind(m_expr);
    zFileInfo::compiletimeBind(m_expr);
    FileStream::compiletimeBind(m_expr);
    Base64::compiletimeBind(m_expr);
    Hash::compiletimeBind(m_expr);
    Log::compiletimeBind(m_expr);
    MemoryBuffer::compiletimeBind(m_expr);
    NativeCall::compiletimeBind(m_expr);
    NativeType::compiletimeBind(m_expr);
    NativeModule::compiletimeBind(m_expr);
    SeekOrigin::compiletimeBind(m_expr);
    SymmetricCrypt::compiletimeBind(m_expr);
    System::compiletimeBind(m_expr);
    TextReader::compiletimeBind(m_expr);
    TextWriter::compiletimeBind(m_expr);
    XmlNode::compiletimeBind(m_expr);
    HttpRequest::compiletimeBind(m_expr);

    // -- database --
    DbBulkInsert::compiletimeBind(m_expr);
    DbColumn::compiletimeBind(m_expr);
    DbDatabaseType::compiletimeBind(m_expr);
    DbEncoding::compiletimeBind(m_expr);
    DbError::compiletimeBind(m_expr);
    DbException::compiletimeBind(m_expr);
    DbObjectInfo::compiletimeBind(m_expr);
    DbType::compiletimeBind(m_expr);
    DbInsertMode::compiletimeBind(m_expr);
    DbResult::compiletimeBind(m_expr);
    DbConnection::compiletimeBind(m_expr);  
}


ScriptHost::~ScriptHost()
{
    delete m_retval;
    delete m_expr;
}

void ScriptHost::setParams(ScriptHostParams* params)
{
    if (!params)
        return;
    
    m_print_function.setValue(params->print_function);
    
    std::map<std::wstring,kscript::Value*>::iterator it;
    for (it = params->global_vars.begin(); it != params->global_vars.end(); ++it)
    {
        m_expr->addValue(it->first, *it->second);
    }
}

void ScriptHost::addValue(const std::wstring& var_name, kscript::Value& val)
{
    m_expr->addValue(var_name, val);
}


void ScriptHost::addFunction(const std::wstring& func_name,
                             BindFunc func_ptr,
                             const std::wstring& formats)
{
    m_expr->addFunction(func_name,
                        true,
                        (kscript::ExprBindFunc)func_ptr,
                        false,
                        formats,
                        NULL);
}

void ScriptHost::addIncludePath(const ScriptHostInclude& inc)
{
    m_include_paths.push_back(inc);
}

const std::vector<ScriptHostInclude>& ScriptHost::getIncludePaths()
{
    return m_include_paths;
}


void ScriptHost::setStartupPath(const std::wstring& startup_path)
{
    m_startup_path = startup_path;
}

std::wstring ScriptHost::getStartupPath() const
{
    return m_startup_path;
}


static bool tryIncludeFile(const std::wstring& filename, std::wstring& res_string)
{
    // file is not in project, try disk filesystem

    if (!xf_get_file_exist(filename))
        return false;

    // file is not in project, try disk filesystem
    xf_file_t f = xf_open(filename, xfOpen, xfRead, xfShareReadWrite);
    if (!f)
        return false;
        
    xf_off_t long_fsize = xf_get_file_size(filename);
    if (long_fsize >= 10000000)
        return false;

    unsigned int fsize = (unsigned int)long_fsize;

    unsigned char* buf = new unsigned char[fsize+1];
    if (!buf)
        return false;
    xf_off_t readbytes = xf_read(f, buf, 1, fsize);
    buf[readbytes] = 0;
    
    if (readbytes >= 2 && buf[0] == 0xff && buf[1] == 0xfe)
    {
        // little endian UCS-2
        std::wstring wval;
        kl::ucsle2wstring(wval, buf+2, ((unsigned int)readbytes-2)/2);
        res_string = wval;
    }
     else if (readbytes >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
    {
        // utf-8
        wchar_t* tempbuf = new wchar_t[fsize+1];
        kl::utf8_utf8tow(tempbuf, fsize+1, (char*)buf+3, (size_t)readbytes);
        res_string = tempbuf;
        delete[] tempbuf;
    }
     else
    {
        res_string = kl::towstring((const char*)buf);
    }
    
    delete[] buf;
    xf_close(f);
        
    return true;
}



static std::wstring chopOffPathPart(const std::wstring& _path)
{
    size_t len = _path.length();
    if (len == 0)
        return _path;
    
    std::wstring path = _path;
    
    if (path[len-1] == '\\' || path[len-1] == '/')
        path.erase(len-1, 1);
    
    size_t slash = path.find_last_of('/');
    size_t back_slash = path.find_last_of('\\');
    
    if (slash == path.npos)
        slash = back_slash;
    if (slash == path.npos)
        return L"";
    
    if (back_slash != path.npos && back_slash > slash)
        slash = back_slash;
        
    return path.substr(0, slash);
}

bool ScriptHost::getFullIncludePath(const std::wstring& include_path, ScriptHostInclude* result)
{
    if (include_path.substr(0, 1) == L"/" ||
        include_path.substr(0, 1) == L"\\" ||
        (include_path.length() > 2 && include_path[1] == L':'))
    {
        // absolute path name
        return false;
    }
    
    std::vector<ScriptHostInclude> include_paths = m_include_paths;
    std::vector<ScriptHostInclude>::iterator it;
    
    if (!m_include_contexts.empty())
        include_paths.insert(include_paths.begin(), m_include_contexts.front());
    
    for (it = include_paths.begin(); it != include_paths.end(); ++it)
    {
        std::wstring result_path = it->path;
        std::wstring path = include_path;
        wchar_t slash = '/';
        
        #ifdef WIN32
        if (it->type != ScriptHostInclude::includeProjectFile)
            slash = '\\';
        #endif
        
        
        while (path.length())
        {
            std::wstring part, part1, part2;
            part1 = kl::beforeFirst(path, '\\');
            part2 = kl::beforeFirst(path, '/');
            if (part1.length() < part2.length())
                part = part1;
                 else
                part = part2;
            path.erase(0, part.length());
            if (path.length() > 0)
            {
                // remove slash
                path.erase(0, 1);
            }
            
            
            if (part.substr(0,2) == L"..")
            {
                result_path = chopOffPathPart(result_path);
            }
             else
            {
                size_t len = result_path.length();
                if (len > 0 && (result_path[len-1] == L'\\' || result_path[len-1] == L'/'))
                    result_path.erase(len-1, 1);
                
                result_path += slash;
                result_path += part;
            }
        }
    }
    
    
    return false;
}


bool ScriptHost::script_host_parse_hook(kscript::ExprParseHookInfo& info)
{
    ScriptHost* host = (ScriptHost*)info.hook_param;

    if (info.element_type == kscript::ExprParseHookInfo::typeInclude)
    {
        if (host->m_include_contexts.size() > 255)
        {
            // most likely some circular include;
            return false;
        }
        
        ScriptHostInclude inc;
        if (!host->getFullIncludePath(info.expr_text, &inc))
            return false;
            
        if (inc.type == ScriptHostInclude::includeFile)
        {
            std::wstring code;
            
            if (tryIncludeFile(inc.path, code))
            {
                info.expr_text = inc.path;
                info.res_string = code;
                
                inc.path = chopOffPathPart(inc.path);
                host->m_include_contexts.push(inc);
                return true;
            }
        }
        
        return false;
    }
     else if (info.element_type == kscript::ExprParseHookInfo::typeIncludePop)
    {
        host->m_include_contexts.pop();
        return true;
    }
    
    return false;
}


bool ScriptHost::compile(const wchar_t* text)
{
    m_error_code = 0;
    m_error_line = 0;
    m_error_offset = 0;
    m_error_string = L"";
    m_error_file = L"";

    m_expr->setParseHook(kscript::ExprParseHookInfo::typeInclude |
                         kscript::ExprParseHookInfo::typeIncludePop,
                         script_host_parse_hook,
                         (void*)this);

    bool result = m_expr->parse(text);

    if (!result)
    {
        kscript::ExprErrorInfo error = m_expr->getErrorInfo();
        
        m_error_code = error.getCode();
        m_error_line = error.getLine();
        m_error_offset = error.getOffset();
        m_error_string = error.getText();
        m_error_file = error.getFileName();
        

        switch (m_error_code)
        {
            case kscript::errorNone:
                m_error_string = L"No error";
                break;
                
            case kscript::errorMissingOpenBrace:
                m_error_string = L"Missing open brace";
                break;

            case kscript::errorMissingCloseBrace:
                m_error_string = L"Missing close brace";
                break;
                
            case kscript::errorMissingOpenBracket:
                m_error_string = L"Missing open bracket";
                break;

            case kscript::errorMissingCloseBracket:
                m_error_string = L"Missing close bracket";
                break;
                
            case kscript::errorMissingOpenParenthesis:
                m_error_string = L"Missing open parenthesis '('";
                break;

            case kscript::errorMissingCloseParenthesis:
                m_error_string = L"Missing close parenthesis ')'";
                break;
                
            case kscript::errorMissingColon:
                m_error_string = L"Missing colon ':'";
                break;
            
            case kscript::errorMissingSemicolon:
                m_error_string = L"Missing semicolon ';'";
                break;
                
            case kscript::errorMissingQuote:
                m_error_string = L"Missing quotation mark";
                break;
                
            case kscript::errorMissingOperator:
                m_error_string = L"Missing operator";
                break;
                
            case kscript::errorMissingCatchFinally:
                m_error_string = L"Missing 'catch' or 'finally' statement";
                break;

            case kscript::errorMissingBaseClass:
                m_error_string = L"Missing base class";
                break;
                
            case kscript::errorMalformedStatement:
                m_error_string = L"Malformed statement";
                break;
                
            case kscript::errorInvalidIncludeFile:
                m_error_string = L"Missing or invalid include file";
                break;

            case kscript::errorUndeclaredIdentifier:
                m_error_string = kl::stdswprintf(L"Undeclared identifier: '%ls'", m_error_string.c_str());
                break;
                
            case kscript::errorInvalidIdentifier:
                m_error_string = kl::stdswprintf(L"Invalid identifier or reserved word: '%ls'", m_error_string.c_str());
                break;
            
            case kscript::errorVarRedefinition:
                m_error_string = kl::stdswprintf(L"Variable redefinition: '%ls'", m_error_string.c_str());
                break;

            case kscript::errorInvalidParameterTypes:
                m_error_string = kl::stdswprintf(L"Invalid parameter type(s) for function '%ls'", m_error_string.c_str());
                break;

            case kscript::errorUnterminatedString:
                m_error_string = L"Unterminated string";
                break;

            case kscript::errorMalformedNumericConstant:
                m_error_string = L"Malformed numeric constant";
                break;
            
            case kscript::errorNotInClassScope:
                m_error_string = L"This construction is only allowed in class scope";
                break;

            case kscript::errorUnknownEscapeSequence:
                m_error_string = kl::stdswprintf(L"Unknown escape sequence '%ls'", m_error_string.c_str());
                break;
                
            case kscript::errorUnexpectedToken:
                m_error_string = L"Unexpected token";
                break;
                
            case kscript::errorSyntaxError:
                m_error_string = L"Syntax error";
                break;
            
            case kscript::errorUnknown:
            default:
                m_error_string = L"Unknown Error";
        }
    }

    return result;
}

bool ScriptHost::compile(const std::wstring& text)
{
    return compile((const wchar_t*)text.c_str());
}

bool ScriptHost::run()
{
    m_error_code = kscript::rterrorNone;
    m_error_line = 0;
    m_error_offset = 0;
    m_error_string = L"";
    m_error_file = L"";
    
    bool res = m_expr->eval(m_retval);

    if (!res)
    {
        kscript::ExprErrorInfo error_info = m_expr->getErrorInfo();
        
        m_error_code = error_info.getCode();
        m_error_line = 0;

        switch (m_error_code)
        {
            case kscript::rterrorNone:
                m_error_string = L"";
                break;

            case kscript::rterrorThrownException:
                m_error_string = L"Uncaught exception.";
                break;
            
            case kscript::rterrorSyntaxError:
                m_error_string = L"Syntax error.";
                break;
            
            case kscript::rterrorTypeError:
                m_error_string = L"Type error.";
                break;
                
            case kscript::rterrorRangeError:
                m_error_string = L"Range error.";
                break;
           
            case kscript::rterrorReferenceError:
                if (error_info.getText().length() > 0)
                {
                    std::wstring s = kl::stdswprintf(L"Reference error: while evaluating '%ls'.",
                                                     error_info.getText().c_str());
                    m_error_string = s;
                }
                 else
                {
                    m_error_string = L"Reference error.";
                }
                
                break;
                     
            case kscript::rterrorNoEntryPoint:
                m_error_string = L"No entry point.";
                break;
                
            case kscript::rterrorMissingObject:
                m_error_string = L"Object expected, but is missing.";
                break;
                
            case kscript::rterrorObjectTypeMismatch:
                m_error_string = L"Object type mismatch (possibly missing 'new' operator)";
                break;

            case kscript::rterrorTermNotFunction:
                m_error_string = kl::stdswprintf(L"Term '%ls' does not evaluate to a function.",
                                                  m_expr->getErrorInfo().getText().c_str());
                break;
            
            case kscript::rterrorExit:
                m_error_code = kscript::rterrorNone;
                return true;

            case kscript::rterrorUnknown:
            default:
                m_error_string = L"Unknown Error";
        }
    }

    return res;
}

void ScriptHost::cancel()
{
    m_expr->cancel();
}

std::wstring ScriptHost::getErrorString()
{
    return m_error_string;
}

std::wstring ScriptHost::getErrorFile()
{
    return m_error_file;
}

int ScriptHost::getErrorLine()
{
    return m_error_line;
}

size_t ScriptHost::getErrorOffset()
{
    return m_error_offset;
}

int ScriptHost::getErrorCode()
{
    return m_error_code;
}

kscript::Value* ScriptHost::getRetval()
{
    return m_retval;
}



}; // namespace scripthost
