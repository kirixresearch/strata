/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-06
 *
 */


#include <xd/xd.h>
#include "database.h"
#include "util.h"
#include "../xdcommonsql/xdcommonsql.h"
#include <kl/portable.h>
#include <kl/url.h>


xd::IDatabasePtr XdnativeDatabase::getPassThroughMount(const std::vector<std::wstring>& tables)
{
    // check to see if all the tables are in the same database
    // if they are
    xd::IDatabasePtr db;
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
        if (detectMountPoint(table_name, &cstr, &rpath))
        {
            xd::IDatabasePtr current_db = lookupOrOpenMountDb(cstr);
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

bool XdnativeDatabase::execute(const std::wstring& command,
                       unsigned int flags,
                       xcm::IObjectPtr& result,
                       xd::IJob* job)
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
        xd::IDatabasePtr db = getMountDatabase(mount_name);
        if (db.isNull())
            return false;

        return db->execute(sql, flags, result, job);        
    }




    // if the pass through flag is set to false, simply run the SQL locally
    if (!(flags & xd::sqlPassThrough))
    {
        bool res = doSQL(static_cast<xd::IDatabase*>(this), command, flags, result, m_error, job);
        if (job && m_error.isError())
        {
            IJobInternalPtr ijob = job;
            ijob->setError(m_error.getErrorCode(), m_error.getErrorString());
        }

        return res;
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
    xd::IDatabasePtr mount_db = getPassThroughMount(tables);

    // we can't pass through the SQL because the tables referenced
    // in the SQL are part of different databases; run the SQL locally
    if (mount_db.isNull())
    {
        bool res = doSQL(static_cast<xd::IDatabase*>(this), command, flags, result, m_error, job);
        if (job && m_error.isError())
        {
            IJobInternalPtr ijob = job;
            ijob->setError(m_error.getErrorCode(), m_error.getErrorString());
        }

        return res;
    }
    
    // find out what the mount databases uses as quote chars
    xd::IAttributesPtr attr = mount_db->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

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
        if (detectMountPoint(*it, &cstr, &rpath))
        {
            // remove any leading slash -- but if there are any remaining
            // slashes, put it back, because the target engine will process
            // it further
            if (rpath.length() > 0 && rpath[0] == '/')
                rpath.erase(0, 1);
            if (rpath.find(L"://") == rpath.npos && rpath.find(L"/") != -1)
                rpath = L"/" + rpath;
            
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


    if (flags & xd::sqlAlwaysCopy)
    {
        flags = 0;

        IJobInternalPtr ijob = job;
        if (ijob)
        {
            int phase_pcts[] = { 50,50 };
            ijob->setPhases(2, phase_pcts);
            ijob->startPhase();
        }

        xcm::IObjectPtr mountdb_result;
        if (!mount_db->execute(new_command, flags, mountdb_result, job))
        {
            // proxy the error code and return false
            m_error.setError(mount_db->getErrorCode(), mount_db->getErrorString());
            return false;
        }

        if (ijob)
        {
            ijob->startPhase();
        }

        xd::IIteratorPtr iter = mountdb_result;
        if (iter.isNull())
        {
            m_error.setError(xd::errorGeneral, L"Failed to get iterator from mount database");
            return false;
        }

        xd::CopyParams cp;
        cp.iter_input = iter;
        cp.output = xd::getTemporaryPath();
        if (!copyData(&cp, job))
        {
            return false;
        }

        xd::QueryParams qp;
        qp.from = cp.output;
        result = query(qp);
    }
    else
    {
        if (!mount_db->execute(new_command, flags, result, job))
        {
            // proxy the error code and return false
            m_error.setError(mount_db->getErrorCode(), mount_db->getErrorString());
            return false;
        }
    }


/*
    TODO: implement

    // if last character of table name is a slash, remove it
    if (primary_table_name[primary_table_name.length()-1] == '/')
        primary_table_name.erase(primary_table_name.length()-1, 1);

    // if it's a query, rename the path to correspond to its mount path
    
    xd::IIteratorPtr iter = result;                 
    if (iter.isOk())
    {
        iter->setObjectPath(primary_table_name);
    }
*/
        
    return true;
}
