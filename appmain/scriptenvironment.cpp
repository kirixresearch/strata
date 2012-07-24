/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#include "appmain.h"
#include "scriptenvironment.h"
#include <wx/stdpaths.h>


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
    
    wxString val;
    ::wxGetEnv(env->getParam(0)->getString(), &val);
    
    retval->setString(towstr(val));
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
    wxString res = wxStandardPaths::Get().GetTempDir();
    retval->setString(towstr(res));
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
    wxString res = wxStandardPaths::Get().GetDocumentsDir();
    retval->setString(towstr(res));
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
    wxString res;
    ::wxGetEnv(wxT("ProgramFiles"), &res);
    retval->setString(towstr(res));
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


