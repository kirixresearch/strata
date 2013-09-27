/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2007-12-11
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "xdcommonsql.h"
#include "../xdcommon/util.h"
#include "../xdcommon/errorinfo.h"
#include <map>
#include <kl/string.h>
#include <kl/portable.h>


bool sqlMove(xd::IDatabasePtr db,
             const std::wstring& _command,
             ThreadErrorInfo& error,
             xd::IJob* job)
{
    SqlStatement stmt(_command);

    stmt.addKeyword(L"MOVE");
    stmt.addKeyword(L"RENAME");
    stmt.addKeyword(L"TABLE");
    stmt.addKeyword(L"DIRECTORY");
    stmt.addKeyword(L"TO");

    if (!stmt.getKeywordExists(L"MOVE") && !stmt.getKeywordExists(L"RENAME"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; MOVE statement missing MOVE or RENAME clause");
        return false;
    }
    
    if (!stmt.getKeywordExists(L"TO"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; MOVE statement missing TO clause");    
        return false;
    }
    
    std::wstring src;
    std::wstring dest;
    
    dest = stmt.getKeywordParam(L"TO");
    
    
    if (stmt.getKeywordExists(L"TABLE"))
    {
        src = stmt.getKeywordParam(L"TABLE");
    }
     else if (stmt.getKeywordExists(L"DIRECTORY"))
    {
        src = stmt.getKeywordParam(L"DIRECTORY");
    }
     else if (stmt.getKeywordExists(L"MOVE"))
    {
        src = stmt.getKeywordParam(L"MOVE");
    }
     else if (stmt.getKeywordExists(L"RENAME"))
    {
        src = stmt.getKeywordParam(L"RENAME");
    }
     else
    {
        // can't find source file
        error.setError(xd::errorSyntax, L"Invalid syntax; no source table before TO clause");
        return false;
    }


    dequote(src, '[', ']');
    dequote(dest, '[', ']');
    
    
    if (stmt.getKeywordExists(L"RENAME"))
    {
        if (!db->renameFile(src, dest))
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to rename [%ls] to [%ls]", src.c_str(), dest.c_str());
            error.setError(xd::errorGeneral, buf);        
            return false;
        }
        
        return true;
    }

    if (!db->moveFile(src, dest))
    {
        wchar_t buf[1024]; // some paths might be long
        swprintf(buf, 1024, L"Unable to move [%ls] to [%ls]", src.c_str(), dest.c_str());
        error.setError(xd::errorGeneral, buf);
        return false;
    }
    
    return true;
}


