/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-28
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
#include <kl/portable.h>


bool sqlDrop(xd::IDatabasePtr db,
             const std::wstring& _command,
             ThreadErrorInfo& error,
             xd::IJob* job)
{
    SqlStatement stmt(_command);

    stmt.addKeyword(L"DROP");
    stmt.addKeyword(L"TABLE");
    stmt.addKeyword(L"INDEX");
    stmt.addKeyword(L"DIRECTORY");
    stmt.addKeyword(L"MOUNT");
    stmt.addKeyword(L"IF");
    stmt.addKeyword(L"EXISTS");
    stmt.addKeyword(L"ON");

    if (!stmt.getKeywordExists(L"DROP"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; DROP statement missing DROP clause");
        return false;
    }

    if (!stmt.getKeywordExists(L"TABLE") &&
        !stmt.getKeywordExists(L"MOUNT") &&
        !stmt.getKeywordExists(L"INDEX") && 
        !stmt.getKeywordExists(L"DIRECTORY"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; DROP statement missing TABLE, MOUNT, INDEX, or DIRECTORY clause");    
        return false;
    }

    bool if_exists = stmt.getKeywordExists(L"IF") &&
                     stmt.getKeywordExists(L"EXISTS");

    std::wstring param;
    bool result = false;
    
    if (stmt.getKeywordExists(L"MOUNT"))
    {
        if (if_exists)
            param = stmt.getKeywordParam(L"EXISTS");
             else
            param = stmt.getKeywordParam(L"MOUNT");
            
        dequote(param, '[', ']');

        result = db->deleteFile(param);

        // if the if_exists flag is false, drop item must be
        // deleted, or else there's an error
        if (!if_exists && !result)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to drop mount [%ls]", param.c_str());
            error.setError(xd::errorGeneral, buf);
            return false;
        }
        
        return true;
    }
     else if (stmt.getKeywordExists(L"DIRECTORY"))
    {
        if (if_exists)
            param = stmt.getKeywordParam(L"EXISTS");
             else
            param = stmt.getKeywordParam(L"DIRECTORY");

        dequote(param, '[', ']');

        result = db->deleteFile(param);
        
        // if the if_exists flag is false, drop item must be
        // deleted, or else there's an error
        if (!if_exists && !result)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to drop directory [%ls]", param.c_str());
            error.setError(xd::errorGeneral, buf);
            return false;
        }
        
        return true;
    }
     else if (stmt.getKeywordExists(L"INDEX"))
    {
        if (if_exists)
            param = stmt.getKeywordParam(L"EXISTS");
             else
            param = stmt.getKeywordParam(L"INDEX");
            
        dequote(param, '[', ']');

        std::wstring on = stmt.getKeywordParam(L"ON");
        dequote(on, '[', ']');
        
        result = db->deleteIndex(on, param);
        
        // if the if_exists flag is false, drop item must be
        // deleted, or else there's an error
        if (!if_exists && !result)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to drop index [%ls]", param.c_str());        
            error.setError(xd::errorGeneral, buf);
            return false;
        }
        
        return true;
    }
     else if (stmt.getKeywordExists(L"TABLE"))
    {
        if (if_exists)
            param = stmt.getKeywordParam(L"EXISTS");
             else
            param = stmt.getKeywordParam(L"TABLE");
            
        dequote(param, '[', ']');

        result = db->deleteFile(param);
        
        // if the if_exists flag is false, drop item must be
        // deleted, or else there's an error
        if (!if_exists && !result)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to drop table [%ls]", param.c_str());        
            error.setError(xd::errorGeneral, buf);
            return false;
        }
        
        return true;
    }
     else
    {
        error.setError(xd::errorGeneral, L"Unable to process DROP statement");
        return false;
    }


    return true;
}


