/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#include "scripthost.h"
#include "environment.h"
#include <kl/string.h>
#include <kl/file.h>


#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#endif



namespace scripthost
{



// (CLASS) Environment
// Category: System
// Description: A class that provides information about the computer and 
//     execution environment.
// Remarks: The Environment class allows the application to access information
//     about the computer and the application's execution environment.  Some
//     examples of this are environment variables and standard paths.

Environment::Environment()
{ 
}

Environment::~Environment()
{
}

void Environment::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{

}



// (METHOD) Environment.getEnvironmentVariable
//
// Description: Returns the value of a system environment variable
//
// Syntax: static function Environment.getEnvironmentVariable(var_name : String) : String
//
// Remarks: Calling getEnvironmentVariable() returns the value stored
//     in the system environment variable specified by the |var_name| parameter.
//     If the environment variable does not exist, an empty string is returned
//
// Param(var_name): The name of the environment variable
// Returns: The value of the environment variable.  If the requested environment
//     variable does not exist, an empty string is returned

void Environment::getEnvironmentVariable(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setString(L"");
        return;
    }



#ifdef WIN32
    const wchar_t* envvar = env->getParam(0)->getString();

    DWORD size = ::GetEnvironmentVariableW(envvar, NULL, 0);
    if (size == 0)
    {
        retval->setString(L"");
        return;
    }

    wchar_t* buf = new wchar_t[size+2];
    buf[0] = 0;
    ::GetEnvironmentVariableW(envvar, buf, size+1);
    retval->setString(buf);
    delete[] buf;
#else

    std::string envvar = kl::tostring(env->getParam(0)->getString());
    char* val = getenv(envvar.c_str());
    if (val)
        retval->setString(kl::towstring(val));
         else
        retval->setString(L"");

#endif

}

// (METHOD) Environment.getTempPath
//
// Description: Returns the system's temporary path
//
// Syntax: static function Environment.getTempPath() : String
//
// Remarks: Returns the system's temporary path.

void Environment::getTempPath(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setString(xf_get_temp_path());
}

// (METHOD) Environment.getDocumentsPath
//
// Description: Returns the user's document path
//
// Syntax: static function Environment.getDocumentsPath() : String
//
// Remarks: Returns the user's document path.  On Windows systems, this
//     is the usually the user's My Documents directory

void Environment::getDocumentsPath(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
#ifdef WIN32
    wchar_t path[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
#else
    retval->setString(getenv("HOME"));
#endif
}

// (METHOD) Environment.getSystemPath
//
// Description: Returns the computer's system path
//
// Syntax: static function Environment.getSystemPath() : String
//
// Remarks: Returns the computer's system path.  On Windows, this yields
//     either the 'System' or the 'System32' path, depending on the computer's
//     configuration.  On Linux systems, this is /usr/bin

void Environment::getSystemPath(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
#ifdef WIN32
    TCHAR buf[255];
    ::GetSystemDirectory(buf, 255);
    retval->setString(kl::towstring(buf));
#else
    retval->setString(L"/usr/bin");
#endif
}

// (METHOD) Environment.getProgramFilesPath
//
// Description: Returns the computer's program files path
//
// Syntax: static function Environment.getProgramFilesPath() : String
//
// Remarks: Returns the computer's program files path.  On Windows, this yields
//     either the 'Program Files' path.  On Linux systems, the /opt path is returned

void Environment::getProgramFilesPath(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
#ifdef WIN32
    wchar_t buf[255];
    if (0 == ::GetEnvironmentVariableW(L"ProgramFiles", buf, 254))
    {
        retval->setString(L"C:\\Program Files");
        return;
    }

    retval->setString(buf);
#else
    retval->setString(L"/opt");
#endif
}


// (METHOD) Environment.getDirectorySeparator
//
// Description: Returns the computer's directory path separator
//
// Syntax: static function Environment.getDirectorySeparator() : String
//
// Remarks: Returns the computer's standard directory separator.  On Windows
//     systems, this is a backslash ("\"), on Linux systems it is a forward slash ("/")


void Environment::getDirectorySeparator(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
#ifdef WIN32
    retval->setString(L"\\");
#else
    retval->setString(L"/");
#endif
}




}; // namespace scripthost
