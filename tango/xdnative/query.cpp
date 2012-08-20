/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-06
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "database.h"
#include "util.h"
#include "../xdcommon/sqlcommon.h"
#include <kl/portable.h>
#include <kl/url.h>


tango::IDatabasePtr Database::getPassThroughMount(const std::vector<std::wstring>& tables)
{
    // check to see if all the tables are in the same database
    // if they are
    tango::IDatabasePtr db;
    bool pass_through = true;
    
    std::vector<std::wstring>::const_iterator it, it_end;
    it_end = tables.end();
    
    std::wstring cstr, rpath;
    for (it = tables.begin(); it != tables.end(); ++it)
    {
        std::wstring table_name = *it;
        if (table_name.empty())
            continue;

        // look for the mount point            
        if (detectMountPoint(table_name, cstr, rpath))
        {
            tango::IDatabasePtr current_db = lookupOrOpenMountDb(cstr);
            if (db.isNull())
            {
                // if it's the first database we've come to,
                // set the databases
                if (current_db.isOk())
                    db = current_db;
            }
             else
            {
                // if we've already encountered a database, and
                // it's a different database, set the pass through
                // flag to false
                if (db != current_db)
                {
                    pass_through = false;
                    break;
                }
            }
        }
    }
    
    if (!pass_through)
        return xcm::null;
        
    return db;
}

bool Database::execute(const std::wstring& command,
                       unsigned int flags,
                       xcm::IObjectPtr& result,
                       tango::IJob* job)
{
    result.clear();
    m_error.clearError();


    // look for queries directed at a specific mount
    const wchar_t* first_char = command.c_str();
    while (::iswspace(*first_char))
        ++first_char;
    if (*first_char == '[')
    {
        std::wstring mount_name = kl::beforeFirst(first_char+1, ']');
        std::wstring sql = kl::afterFirst(command, ']');
        kl::trim(mount_name);
        tango::IDatabasePtr db = getMountDatabase(mount_name);
        if (db.isNull())
            return false;

        return db->execute(sql, flags, result, job);        
    }




    // if the pass through flag is set to false, simply run the SQL locally
    if (!(flags & tango::sqlPassThrough))
    {
        return doSQL(static_cast<tango::IDatabase*>(this),
                     command, flags, result, m_error, job);
    }



    // user has specified sqlPassThrough -- we'll attempt to pass
    // through the query to the target mount database.  If database objects
    // have been specified which belong to multiple mount databases,
    // we'll use the local engine for cross-database queries
    
    // get all the tables in the SQL
    std::vector<std::wstring> tables = getAllTableNamesFromSql(command);

    // check to see if all the tables are in the same database; if they are,
    // we'll pass the SQL through to that database; otherwise, we'll run
    // it locally
    tango::IDatabasePtr mount_db = getPassThroughMount(tables);

    // we can't pass through the SQL because the tables referenced
    // in the SQL are part of different databases; run the SQL locally
    if (mount_db.isNull())
    {
        return doSQL(static_cast<tango::IDatabase*>(this),
                     command, flags, result, m_error, job);
    }
    
    // find out what the mount databases uses as quote chars
    tango::IAttributesPtr attr = mount_db->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    // discover primary table name
    std::wstring primary_table_name;
    if (tables.size() > 0)
        primary_table_name = tables[0];

    // set the new command and get the primary table name
    std::wstring new_command = command;        

    // replace the full table paths with the tablename in the
    // database; since the tablename name may or may not be quoted,
    // try replace both the quoted and unquoted versions
    std::vector<std::wstring>::iterator it, it_end;
    it_end = tables.end();
    
    std::wstring cstr, rpath;
    for (it = tables.begin(); it != it_end; ++it)
    {
        if (detectMountPoint(*it, cstr, rpath))
        {

            // remove any leading slash -- but if there are any remaining
            // slashes, put it back, because the target engine will process
            // it further
            if (rpath.length() > 0 && rpath[0] == '/')
                rpath.erase(0, 1);
            if (!kl::isFileUrl(rpath) && rpath.find(L"/") != -1)
                rpath = L"/" + rpath;
            
            if (rpath.find(L" ") != -1)
                rpath = quote_openchar + rpath + quote_closechar;


            std::wstring table_name = kl::afterLast(*it,'/');
            std::wstring new_table_name = kl::afterLast(rpath,'/');
            
            kl::replaceStr(new_command, L"[" + *it + L"]", rpath);
            kl::replaceStr(new_command, quote_openchar + *it + quote_closechar, rpath);
            kl::replaceStr(new_command, L" " + *it, L" " + rpath);
            kl::replaceStr(new_command, table_name + L".", new_table_name + L".");
        }
    }

    // if the opening or closing quote character of the database
    // is different from the corresponding internal quote character, 
    // replace it
    if (quote_openchar != L"[" || quote_closechar != L"]")
        requote(new_command, L'[', L']',*quote_openchar.c_str(), *quote_closechar.c_str());

    if (!mount_db->execute(new_command, flags, result, job))
    {
        // proxy the error code and return false
        m_error.setError(mount_db->getErrorCode(), mount_db->getErrorString());
        return false;
    }


    // if last character of table name is a slash, remove it
    if (primary_table_name[primary_table_name.length()-1] == '/')
        primary_table_name.erase(primary_table_name.length()-1, 1);

    // if it's a query, rename the set's path to
    // correspond to it's mount path
    
    tango::IIteratorPtr iter = result;                 
    if (iter.isOk())
    {
        tango::ISetPtr set = iter->getSet();
        if (set.isOk() && set->getObjectPath().length() > 0)
        {
            set->setObjectPath(primary_table_name);
        }
    }
        
    return true;
}
