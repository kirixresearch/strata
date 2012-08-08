/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-26
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "util.h"
#include "sqlcommon.h"
#include "jobinfo.h"
#include "../xdcommon/util.h"
#include "keylayout.h"
#include "tango_private.h"
#include <kl/klib.h>
#include <kl/regex.h>


SqlStatement::SqlStatement(const std::wstring& stmt)
{
    m_stmt = new wchar_t[stmt.length()+1];
    wcscpy(m_stmt, stmt.c_str());
}

SqlStatement::~SqlStatement()
{
    delete[] m_stmt;
}

void SqlStatement::addKeyword(const std::wstring& keyword)
{
    std::vector<wchar_t*>& arr = m_keywords[keyword];
    
    wchar_t* pstmt = m_stmt;
    wchar_t* res;
    while (1)
    {
        res = zl_stristr(pstmt, keyword.c_str(), true, true);
        if (!res)
            break;
        arr.push_back(res);
        m_stops.insert(res);
        pstmt = res+1;
    }
}

bool SqlStatement::getKeywordExists(const std::wstring& keyword)
{
    std::map<std::wstring, std::vector<wchar_t*> >::iterator it;
    it = m_keywords.find(keyword);
    if (it == m_keywords.end())
        return false;

    return (it->second.size() > 0 ? true : false);
}

std::wstring SqlStatement::getKeywordParam(const std::wstring& keyword)
{
    std::map<std::wstring, std::vector<wchar_t*> >::iterator it;
    it = m_keywords.find(keyword);
    if (it == m_keywords.end())
        return L"";
        
    if (it->second.size() == 0)
        return L"";

    wchar_t* start = it->second[0] + it->first.length();
    wchar_t* end = m_stmt + wcslen(m_stmt);

    std::set<wchar_t*>::iterator stop_it;
    stop_it = m_stops.upper_bound(start);
    if (stop_it != m_stops.end())
        end = *stop_it;

    wchar_t temps = *end;
    *end = 0;
    std::wstring resstring = start;
    *end = temps;
    
    kl::trim(resstring);
    return resstring;
}

std::vector<std::wstring> SqlStatement::getKeywordParams(const std::wstring& keyword)
{
    std::map<std::wstring, std::vector<wchar_t*> >::iterator it;
    it = m_keywords.find(keyword);
    if (it == m_keywords.end())
        return std::vector<std::wstring>();
        
    if (it->second.size() == 0)
        return std::vector<std::wstring>();

    wchar_t* default_end = m_stmt + wcslen(m_stmt);
    
    std::vector<std::wstring> res;

    std::vector<wchar_t*>::iterator kit;
    for (kit = it->second.begin(); kit != it->second.end(); ++kit)
    {
        wchar_t* start = *kit + it->first.length();
        wchar_t* end = default_end;

        std::set<wchar_t*>::iterator stop_it;
        stop_it = m_stops.upper_bound(start);
        if (stop_it != m_stops.end())
            end = *stop_it;

        wchar_t temps = *end;
        *end = 0;
        std::wstring resstring = start;
        *end = temps;
        
        kl::trim(resstring);
        res.push_back(resstring);
    }

    return res;
}

size_t SqlStatement::getKeywordPosition(const std::wstring& keyword)
{
    std::map<std::wstring, std::vector<wchar_t*> >::iterator it;
    it = m_keywords.find(keyword);
    if (it == m_keywords.end() || it->second.size() == 0)
        return (size_t)-1;
        
    return it->second[0] - m_stmt;
}



// TODO: peekToken and popToken should factor
// out common code

std::wstring peekToken(const std::wstring& str)
{
    const wchar_t* start = str.c_str();
    const wchar_t* p = start;
    std::wstring ret;
    int chars = 0;

    while (iswspace(*p))
    {
        p++;
        chars++;
    }

    if (*p == '[')
    {
        // identifier quotation
        const wchar_t* close = wcschr(p, ']');
        if (close)
        {
            ret.assign(p, close-p+1);
            return ret;
        }
    }
    
    while (*p)
    {
        if (0 != wcschr(L" \t\n\r!@#$%^&*-=|/+,()[]{}:'\"", *p))
        {
            if (ret.empty())
            {
                ret = *p;
                return ret;
            }
             else
            {
                break;
            }
        }

        ret += *p;
        ++p;
        ++chars;
    }

    return ret;
}


// WARNING: until peekToken and popToken
// are factored, if you make a change to popToken,
// make sure to make the change to the function above

std::wstring popToken(std::wstring& str)
{
    const wchar_t* start = str.c_str();
    const wchar_t* p = start;
    std::wstring ret;
    int chars = 0;

    while (iswspace(*p))
    {
        p++;
        chars++;
    }

    if (*p == '[')
    {
        // identifier quotation
        const wchar_t* close = wcschr(p, ']');
        if (close)
        {
            ret.assign(p, close-p+1);
            str.erase(0, close-start+1);
            return ret;
        }
    }
    
    while (*p)
    {
        if (0 != wcschr(L" \t\n\r!@#$%^&*-=|/+,()[]{}:'\"", *p))
        {
            if (ret.empty())
            {
                ret = *p;
                str.erase(0,chars+1);
                return ret;
            }
             else
            {
                break;
            }
        }

        ret += *p;
        ++p;
        ++chars;
    }

    str.erase(0, chars);
    return ret;
}



std::wstring getTableNameFromSql(const std::wstring& _sql)
{
    // get all the tables in the SQL statement
    std::wstring result;
    std::vector<std::wstring> tables = getAllTableNamesFromSql(_sql);
    
    // if any tables exist in the SQL statement, the first table
    // in the list is the one we're interested in
    if (tables.size() > 0)
        result = tables[0];

    return result;
}

std::vector<std::wstring> getAllTableNamesFromSql(const std::wstring& sql)
{
    // note: this function extracts all references to tables in
    // SQL statement, dequotes them, and returns them in a vector;
    // in instances where multiple fields are contained in a SQL
    // statement, the tables are returned in order of precedence,
    // with the "more important" tables appearing earlier; so the
    // first member of the vector is the table that should be used
    // when setting the internal tablename member of a set; the
    // remaining tables are included to determine, in conjunction
    // with detectMountPoint(), if the tables are all members of
    // the same database or spread across different databases
    //
    // behavior examples:
    // 1. if the SQL statement is 
    //     "SELECT * FROM <table1>
    // the function will return one table: (table1)
    // 2. if the SQL statement is 
    //     "SELECT * FROM <table1> INTO <table2>
    // the function will return two tables: (table1, table2)

    // "FROM" syntax examples:
    // SELECT * INTO <new_table> ... FROM <old_table> ...;
    // SELECT * FROM <table1> INNER/RIGHT/LEFT/OUTER JOIN <table2> ON <table1>.<field>=<table2>.<field> ...;
    // SELECT * FROM <table> ...;
    // DELETE FROM <table> WHERE ...;
    // CREATE VIEW <view> AS SELECT * FROM <table> WHERE ...;

    // "INTO" syntax examples:
    // INSERT INTO <table> VALUES ...
    // INSERT INTO <table> (column1, column2, column3) VALUES ...;

    // "UPDATE" syntax example:
    // UPDATE <table> SET ... WHERE ...;

    // "TABLE" syntax examples:
    // ALTER TABLE <table> ...;
    // CREATE TABLE <table> (column1, ...);
    // TRUNCATE TABLE <table> ...;
    // DROP TABLE <table>

    // "JOIN" syntax example:
    // SELECT * FROM <table1> INNER/RIGHT/LEFT/OUTER JOIN <table2> ON <table1>.<field>=<table2>.<field> ...;

    // "ON" syntax examples:
    // SELECT * FROM <table1> INNER/RIGHT/LEFT/OUTER JOIN <table2> ON <table1>.<field>=<table2>.<field> ...;    
    // CREATE INDEX <index> ON <table> (column) ...;
    // CREATE UNIQUE INDEX <index> ON <table> (column) ...;


    wchar_t left_quote, right_quote;

    SqlStatement stmt(sql);

    // add the keywords that are followed by tablenames
    stmt.addKeyword(L"FROM");
    stmt.addKeyword(L"INTO");
    stmt.addKeyword(L"UPDATE");
    stmt.addKeyword(L"TABLE");
    stmt.addKeyword(L"JOIN");
    stmt.addKeyword(L"ON");

    std::vector<std::wstring> parts;
    std::vector<std::wstring> tables;

    // note: the following order of keywords is important to
    // give precedence to some tablenames over other tablenames

    if (stmt.getKeywordExists(L"FROM"))
        parts.push_back(stmt.getKeywordParam(L"FROM"));
    
    if (stmt.getKeywordExists(L"INTO"))
        parts.push_back(stmt.getKeywordParam(L"INTO"));

    if (stmt.getKeywordExists(L"UPDATE"))
        parts.push_back(stmt.getKeywordParam(L"UPDATE"));

    if (stmt.getKeywordExists(L"TABLE"))
        parts.push_back(stmt.getKeywordParam(L"TABLE"));
    
    if (stmt.getKeywordExists(L"JOIN"))
    {
        std::vector<std::wstring> join_tables = stmt.getKeywordParams(L"JOIN");
        std::vector<std::wstring>::iterator it;
        for (it = join_tables.begin(); it != join_tables.end(); ++it)
            parts.push_back(*it);
    }
    
    // iterate through the tables
    std::vector<std::wstring>::iterator it, it_end;
    it_end = parts.end();
    
    for (it = parts.begin(); it != it_end; ++it)
    {
        // the getKeywordParam function returns the tablename 
        // all the way to the following keyword, so we need to 
        // remove the part of the SQL statement that immediately
        // follows the table; we can determine this in one of
        // two ways: 1) if the table is quoted, everything inside
        // the quotes; 2) if the table isn't quoted, everything up
        // to the first space, period, or parenthesis (see examples
        // above)

        if (it->length() == 0)
            continue;

        const wchar_t* expr = it->c_str();
        const wchar_t* ch = expr;
        int quote_level = 0;

        
        if (*ch == '[')
        {
            left_quote = '[';
            right_quote = ']';
        }
         else if (*ch == '`')
        {
            left_quote = '`';
            right_quote = '`';
        }
         else if (*ch == '"')
        {
            left_quote = '"';
            right_quote = '"';
        }
         else
        {
            left_quote = 0;
            right_quote = 0;
        }
        
        
        while (*ch)
        {
            if (left_quote == right_quote)
            {
                if (*ch == left_quote)
                    quote_level = (quote_level == 0) ? 1 : 0;
            }
             else
            {
                if (*ch == left_quote)
                    ++quote_level;
                if (*ch == right_quote)
                    --quote_level;
            }
            
            if (quote_level == 0)
            {
                if (*ch == L'(' ||  *ch == L';' || ::iswspace(*ch))
                    break;
            }
            
            ++ch;
        }
        
        std::wstring piece = std::wstring(expr, ch-expr);
        kl::trim(piece);
        if (left_quote)
            dequote(piece, left_quote, right_quote);
        tables.push_back(piece);
        
        /*
        // see if the table is quoted; if it is, extract the quoted
        // portion
        if ((*it)[0] == lquote)
        {
            // look for the closing quote
            ch = zl_strchr((wchar_t*)expr, rquote, L"", L"");
            
            // if we don't have a closing quote, move on
            if (!ch)
                continue;
            
            *it = it->substr(0, (int)(ch - expr) + 1);

            // remove the quotes
            dequote(*it, lquote, rquote);
            
            // TODO: handle case where table and fieldname are
            // quoted together: [table.field]
            
            // add the table to the output list
            tables.push_back(*it);
        }
         else
        {
            // if the table isn't quoted, find the first space, period
            // parenthesis, or semi-colon, and cut it off and everything 
            // afterward
            ch = (wchar_t*)expr;
            while (1)
            {
                if (!*ch)
                {
                    // we're at the end
                    tables.push_back(*it);
                    break;
                }

                if (*ch == L'.' || 
                    *ch == L'(' || 
                    *ch == L';' || 
                    ::iswspace(*ch))
                {
                    // add the table to the output list
                    *it = it->substr(0, (int)(ch - expr));
                    tables.push_back(*it);
                    break;
                }

                ++ch;
            }
        }
        */
    }

    return tables;
}

static void removeComments(std::wstring& expr)
{
    size_t i = 0, j = 0, len = expr.length();
    wchar_t quote_char = 0;
    
    while (i < len)
    {
        if (quote_char && expr[i] == quote_char)
        {
            quote_char = 0;
        }
         else
        {
            if (expr[i] == '\'' || expr[i] == '"')
            {
                quote_char = expr[i];
            }
        }
        
        
        // look for -- comment
        if (quote_char == 0 && i+1 < len && expr[i] == '-' && expr[i+1] == '-')
        {
            for (j = i; j < len; ++j)
            {
                if (expr[j] == '\n' || expr[j] == '\r')
                {
                    i = j;
                    break;
                }
                
                expr[j] = ' ';
            }
        }
        
        ++i;
    }
}


void splitSQL(const std::wstring& _sql,
           std::vector<std::wstring>& commands)
{
    std::wstring sql = _sql;
    removeComments(sql);
    kl::parseDelimitedList(sql, commands, L';', true);
}
           

bool doSQL(tango::IDatabasePtr db,
           const std::wstring& _sql,
           unsigned int flags,
           xcm::IObjectPtr& result_obj,
           ThreadErrorInfo& error,
           tango::IJob* job)
{
    error.clearError();
    
    std::wstring sql = _sql;
    removeComments(sql);

    std::vector<std::wstring> commands;
    kl::parseDelimitedList(sql, commands, L';', true);

    std::vector<std::wstring>::iterator it;
    
    for (it = commands.begin(); it != commands.end(); ++it)
    {
        std::wstring command = *it;
        kl::trim(command);

        if (command.length() == 0)
        {
            // if we're on the first command, it's an empty command,
            // so return false
            if (it == commands.begin())
            {
                if (!error.isError())
                    error.setError(tango::errorGeneral);
                return false;
            }
            
            // otherwise assume a trailing semicolon, which is ok
            return true;
        }
            
        const wchar_t* command_cstr = command.c_str();

        bool result = false;

        if (0 == wcsncasecmp(command_cstr, L"INSERT", 6) &&
            iswspace(command[6]))
        {
            result = sqlInsert(db, command, error, job);
        }
         else if (0 == wcsncasecmp(command_cstr, L"CREATE", 6) &&
                  iswspace(command[6]))
        {
            result = sqlCreate(db, command, result_obj, error, job);
        }
         else if (0 == wcsncasecmp(command_cstr, L"SELECT", 6) &&
                  iswspace(command[6]))
        {
            tango::IIteratorPtr iter = sqlSelect(db, command, flags, error, job);
            result = iter.isOk();
            result_obj = iter;
        }
         else if (0 == wcsncasecmp(command_cstr, L"UPDATE", 6) &&
                  iswspace(command[6]))
        {
            result = sqlUpdate(db, command, error, job);
        }
         else if (0 == wcsncasecmp(command_cstr, L"DELETE", 6) &&
                  iswspace(command[6]))
        {
            result = sqlDelete(db, command, error, job);
        }
         else if (0 == wcsncasecmp(command_cstr, L"RENAME", 6) &&
                  iswspace(command[6]))
        {
            // move handles both MOVE and RENAME
            result = sqlMove(db, command, error, job);
        }
         else if (0 == wcsncasecmp(command_cstr, L"MOVE", 4) &&
                  iswspace(command[4]))
        {
            result = sqlMove(db, command, error, job);
        }
         else if (0 == wcsncasecmp(command_cstr, L"DROP", 4) &&
                  iswspace(command[4]))
        {
            result = sqlDrop(db, command, error, job);
        }
         else if (0 == wcsncasecmp(command_cstr, L"ALTER", 5) &&
                  iswspace(command[5]))
        {
            result = sqlAlter(db, command, error, job);
        }
         else if (0 == wcsncasecmp(command_cstr, L"OPEN", 4) &&
                  iswspace(command[4]))
        {
            result = sqlOpen(db, command, result_obj, error, job);
        }
                
        if (!result)
        {
            result_obj.clear();
            
            if (!error.isError())
            {
                // there was an error, but it was not marked in the error
                // object, so set a generic error
                error.setError(tango::errorGeneral);
            }
            
            return false;
        }
    }
    
    return true;
}










// -- iteration classes --



class SqlSimpleIterator : public SqlIterator
{
public:

    SqlSimpleIterator()
    {
        m_cond = 0;
        m_curpos = 0;
        m_cancelled = false;
    }
    
    ~SqlSimpleIterator()
    {
        if (m_cond && m_iter)
        {
            m_iter->releaseHandle(m_cond);
        }
    }
     
    bool init(tango::IIteratorPtr iter,
              const std::wstring& condition,
              tango::IJob* job)
    {
        m_sp_iter = iter;
        m_iter = m_sp_iter.p;
        m_cond = 0;
        m_job = job;
       
        if (condition.length() > 0)
        {
            m_cond = m_iter->getHandle(condition);
            if (!m_cond)
                return false;
        }
        
        return true;
    }
    
    tango::IIterator* getIterator()
    {
        return m_iter;
    }
    
    void goFirst()
    {
        m_curpos = 0;
        updateJobInfo();
        
        m_iter->goFirst();
        findNextRow();
    }
    
    void goNext()
    {
        m_curpos++;
        updateJobInfo();
        
        m_iter->skip(1);
        findNextRow();
    }
    
    void findNextRow()
    {
        if (!m_cond)
            return;
        
        while (!m_iter->eof())
        {
            if (m_cancelled)
                return;
            if (m_iter->getBoolean(m_cond))
                return;
                
            m_iter->skip(1);
            
            m_curpos++;
            updateJobInfo();
        }
    }
    
    bool isDone()
    {
        return m_iter->eof();
    }
    
    bool isCancelled()
    {
        return m_cancelled;
    }
    
    inline void updateJobInfo()
    {
        if ((m_curpos % 1000) == 0 && m_job.p)
        {
            m_job->setCurrentCount(m_curpos);
            if (m_job->getCancelled())
                m_cancelled = true;
        }
    }
    
public:

    tango::IIteratorPtr m_sp_iter;
    tango::IIterator* m_iter;
    IJobInternalPtr m_job;
    
    tango::objhandle_t m_cond;
    tango::rowpos_t m_curpos;
    bool m_cancelled;
};


class SqlSingleRowIterator : public SqlIterator
{
public:

    SqlSingleRowIterator()
    {
        m_done = false;
        m_rowid = 0;
    }
    
    ~SqlSingleRowIterator()
    {
    }
     
    bool init(tango::IIteratorPtr iter,
              tango::rowid_t rowid,
              tango::IJob* job)
    {
        m_sp_iter = iter;
        m_iter = m_sp_iter.p;
        m_rowid = rowid;
        m_done = false;
        
        return true;
    }
    
    tango::IIterator* getIterator()
    {
        return m_iter;
    }
    
    void goFirst()
    {
        m_iter->goRow(m_rowid);
        m_done = false;
    }
    
    void goNext()
    {
        m_done = true;
    }
    
    bool isDone()
    {
        if (m_done)
            return true;
        
        return m_iter->eof();
    }
    
    bool isCancelled()
    {
        // this iterator only updates one row;
        // there is no cancelling because it
        // happens too fast anyway
        return false;
    }
    
public:

    tango::IIteratorPtr m_sp_iter;
    tango::IIterator* m_iter;
    tango::rowid_t m_rowid;
    bool m_done;
};



class SqlSeekIterator : public SqlIterator
{
public:

    SqlSeekIterator()
    {
        m_curpos = 0;
        m_cancelled = false;
    }
    
    ~SqlSeekIterator()
    {
    }
     
    bool init(tango::ISetPtr set,
              const std::wstring& key_field,
              const std::wstring& key_value,
              tango::IJob* job)
    {        
        m_sp_iter = set->createIterator(L"", key_field, NULL);
        if (!m_sp_iter)
            return false;
        m_iter = m_sp_iter.p;
        
        KeyLayout kl;
        kl.setKeyExpr(m_sp_iter, key_field, false);
        const wchar_t* values[1];
        values[0] = key_value.c_str();
        const unsigned char* key = kl.getKeyFromValues(values, 1);
        size_t key_len = (size_t)kl.getKeyLength();
        if (kl.getTruncation())
            return false;

        IIteratorKeyAccessPtr key_access = m_sp_iter;
        if (!key_access)
            return false;
        
        m_iter->seek(key, key_len, true);

        key_access->setKeyFilter(key, key_len);
        
        m_job = job;
        return true;
    }

    tango::IIterator* getIterator()
    {
        return m_iter;
    }
    
    void goFirst()
    {
        m_curpos = 0;
        updateJobInfo();
    }
    
    void goNext()
    {
        m_curpos++;
        updateJobInfo();
        
        m_iter->skip(1);
    }

    bool isDone()
    {
        return m_iter->eof();
    }
    
    bool isCancelled()
    {
        return m_cancelled;
    }
    
    inline void updateJobInfo()
    {
        if ((m_curpos % 1000) == 0 && m_job.p)
        {
            m_job->setCurrentCount(m_curpos);
            if (m_job->getCancelled())
                m_cancelled = true;
        }
    }
    
public:

    tango::IIteratorPtr m_sp_iter;
    tango::IIterator* m_iter;
    IJobInternalPtr m_job;
    
    tango::rowpos_t m_curpos;
    bool m_cancelled;
};



SqlIterator* SqlIterator::createSqlIterator(tango::IIteratorPtr iter,
                                            const std::wstring& condition,
                                            tango::IJob* job)
{
    // test for a condition that looks like (rowid = '0123456789abcdef')
    klregex::wregex rowid_regex(L"(?i)^\\s*rowid\\s*[=]\\s*['\"]([0-9a-f]+)['\"]\\s*$");
    klregex::wmatch rowid_matchres;    
    if (rowid_regex.match(condition.c_str(), rowid_matchres))
    {
        tango::rowid_t rowid = hex2uint64(rowid_matchres[1].str().c_str());
        
        SqlSingleRowIterator* ret_iter = new SqlSingleRowIterator();
        
        if (!ret_iter->init(iter, rowid, job))
        {
            delete ret_iter;
            return NULL;
        }
        
        return static_cast<SqlIterator*>(ret_iter);
    }
    
    /*
    // test for a condition that looks like (field_name = '0123456789abcdef')
    klregex::wregex fldvalue_regex(L"(?i)^\\s*([[]?[a-z][a-z0-9_ ]*[]]?)\\s*[=]\\s*['\"]?([^'\"]*)['\"]?\\s*$");
    klregex::wmatch fldvalue_matchres;    
    if (fldvalue_regex.match(condition.c_str(), fldvalue_matchres))
    {
        std::wstring field = fldvalue_matchres[1].str();
        std::wstring value = fldvalue_matchres[2].str();
        
        kl::trim(field);
        dequote(field, '[', ']');

        tango::ISetPtr set = iter->getSet();
        if (set)
        {
            tango::IIndexInfoPtr index = set->lookupIndex(field, true);
            if (index)
            {
                SqlSeekIterator* ret_iter = new SqlSeekIterator();
                
                if (!ret_iter->init(set, field, value, job))
                {
                    delete ret_iter;
                    return NULL;
                }
                
                return static_cast<SqlIterator*>(ret_iter);
            }
        }
    }
    */
    
    // any other condition gets a full table scan
    SqlSimpleIterator* ret_iter = new SqlSimpleIterator();
    
    if (!ret_iter->init(iter, condition, job))
    {
        delete ret_iter;
        return NULL;
    }
    
    return static_cast<SqlIterator*>(ret_iter);
}

