/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-04-13
 *
 */


#include "appmain.h"
#include "querytemplate.h"
#include "jobquery.h"
#include "tabledoc.h"
#include "appcontroller.h"
#include "jsonconfig.h"


static bool needsQuoting(const wxString& str)
{
    if (str.Freq(' ') > 0 || str.Freq('.') > 0 ||
        str.Freq('/') > 0 || str.Freq('\\') > 0 ||
        str.Freq(':') > 0)
    {
        return true;
    }
    
    return false;
}

static wxString quoteTable(const wxString& _str)
{
    if (!needsQuoting(_str))
        return _str;
        
    wxString str = _str;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return str;
    
    tango::IAttributesPtr attr = db->getAttributes();
    if (attr.isNull())
        return str;
        
    str.Prepend(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar)));
    str.Append(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar)));
    
    return str;
}

static wxString quoteAlias(const wxString& _str)
{
    if (!needsQuoting(_str))
        return _str;
        
    wxString str = _str;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return str;
    
    tango::IAttributesPtr attr = db->getAttributes();
    if (attr.isNull())
        return str;
 
    str.Prepend(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar)));
    str.Append(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar)));

    return str;
}

static wxString quoteField(const wxString& _str)
{
    wxString str = _str;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return str;
    
    tango::IAttributesPtr attr = db->getAttributes();
    if (attr.isNull())
        return str;
   
    if (::wxStrpbrk(str, wxT("+-*/()[]<>")))
    {
        // looks like a calculated formula, we don't want to touch that
        return str;
    }
   
    if (str.Freq('.') > 0)
    {
        wxString alias = str.BeforeLast('.');
        wxString field = str.AfterLast('.');
        
        if (needsQuoting(alias))
        {
            alias.Prepend(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar)));
            alias.Append(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar)));
        }
        
        if (needsQuoting(field))
        {
            field.Prepend(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar)));
            field.Append(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar)));
        }
        
        str = alias + wxT(".") + field;
    }
     else
    {
        if (!needsQuoting(str))
            return str;
        
        str.Prepend(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar)));
        str.Append(towx(attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar)));
    }
    
    return str;
}

// this function formats a table name to remove the leading slash
// if the table is in the root.  Thus "/mytbl" becomes "mytbl",
// and "/myfolder/mytbl" remains the same
static wxString formatTableName(const wxString& _table_path)
{
    wxString table_path = quoteTable(_table_path);
    
    if (table_path.Freq('/') == 1 && table_path.GetChar(0) == '/')
    {
        wxString result = table_path;
        result.Remove(0,1);
        return result;
    }
    
    return table_path;
}

static wxString makeQueryGroupFunction(const wxString& _input_expr, int group_func)
{
    wxString expr;

    
    
    // TODO: currently, the SQL parser can't handle quoted field
    // names within aggregate functions; for example, the following
    // will fail:
    //     select sum([field with space]) from t;
    // however, the parser is flexible enough to correctly process the
    // result when the quoting is absent; so for now, don't quote
    // fields within aggregate functions; when aliases are supported,
    // uncomment the following:

    wxString input_expr = _input_expr;
    //wxString input_expr = quoteField(_input_expr);
    
    
    switch (group_func)
    {
        default:
        case QueryGroupFunction_GroupBy:
        case QueryGroupFunction_First:
        case QueryGroupFunction_None:
            expr = quoteField(input_expr);  // TODO: remove quoting when above quoting is reinstated
            break;

        case QueryGroupFunction_Last:
            expr.Printf(wxT("LAST(%s)"), input_expr.c_str());
            break;

        case QueryGroupFunction_Min:
            expr.Printf(wxT("MIN(%s)"), input_expr.c_str());
            break;

        case QueryGroupFunction_Max:
            expr.Printf(wxT("MAX(%s)"), input_expr.c_str());
            break;

        case QueryGroupFunction_Sum:
            expr.Printf(wxT("SUM(%s)"), input_expr.c_str());
            break;

        case QueryGroupFunction_Avg:
            expr.Printf(wxT("AVG(%s)"), input_expr.c_str());
            break;

        case QueryGroupFunction_Count:
            expr.Printf(wxT("COUNT(*)"));
            break;

        case QueryGroupFunction_Stddev:
            expr.Printf(wxT("STDDEV(%s)"), input_expr.c_str());
            break;

        case QueryGroupFunction_Variance:
            expr.Printf(wxT("VARIANCE(%s)"), input_expr.c_str());
            break;

        case QueryGroupFunction_GroupID:
            expr.Printf(wxT("GROUPID()"));
            break;
    }

    return expr;
}

static void onQueryJobFinished(cfw::IJobPtr job)
{
    if (job->getJobInfo()->getState() != cfw::jobStateFinished)
        return;

    IQueryJobPtr query_job = job;
    bool refresh_tree = false;
    int querydoc_site_id = job->getExtraLong();
    cfw::IDocumentSitePtr querydoc_site;
    if (querydoc_site_id != 0)
        querydoc_site = g_app->getMainFrame()->lookupSiteById(querydoc_site_id);
    
    // check if there is an output set
    if (query_job.isOk())
    {
        tango::IIteratorPtr result_iter = query_job->getResultIterator();
        tango::ISetPtr result_set = query_job->getResultSet();

        // if the result set isn't temporary, the set has been created
        // with an "INTO" statement and should appear on the tree; so, 
        // set the refresh tree flag to true
        if (!result_set->isTemporary())
            refresh_tree = true;

        if (result_set.isOk())
        {
            if (querydoc_site.isOk())
            {
                // first try to find an existing tabledoc 
                ITableDocPtr tabledoc;
                tabledoc = lookupOtherDocument(querydoc_site, "appmain.TableDoc");
                if (tabledoc.isNull())
                {
                    // none exists yet, create one  
                    tabledoc = TableDocMgr::createTableDoc();
                    tabledoc->setTemporaryModel(true);
                    tabledoc->open(result_set, result_iter);
                    
                    wxWindow* container = querydoc_site->getContainerWindow();
                    g_app->getMainFrame()->createSite(container,
                                                      tabledoc,
                                                      true);
                }
                 else
                {
                    // switch to the table view
                    tabledoc->getGrid()->Freeze();
                    tabledoc->open(result_set, result_iter);
                    tabledoc->refreshActiveView();
                    tabledoc->getGrid()->Thaw();

                    switchToOtherDocument(querydoc_site, "appmain.TableDoc");
                }
            }
             else
            {
                ITableDocPtr doc = TableDocMgr::createTableDoc();
                doc->setTemporaryModel(true);
                doc->open(result_set, result_iter);

                g_app->getMainFrame()->createSite(doc, cfw::sitetypeNormal,
                                                  -1, -1, -1, -1);
                refresh_tree = true;
            }
        }
    }

    if (refresh_tree)
        g_app->getAppController()->refreshDbDoc();
}



QueryTemplate::QueryTemplate()
{
    m_distinct = false;
    m_disposition = tango::dbtypeXdnative;
}

QueryTemplate::~QueryTemplate()
{
}

bool QueryTemplate::save(const wxString& path, bool refresh_tree_if_necessary)
{
    // now create an ofs file with the run information
    tango::IDatabasePtr db = g_app->getDatabase();
    bool should_refresh_tree = true;
    
    if (db->getFileExist(towstr(path)))
    {
        // try to load a potentially existing template; the goal is to
        // see if it already exists so we can avoid an unnecessary tree
        // refresh, which can cause ugly flicker
        tango::INodeValuePtr file = db->openNodeFile(towstr(path));
        if (file.isOk())
        {
            tango::INodeValuePtr template_node = file->getChild(L"kpp_template", false);
            if (template_node.isOk())
            {
                tango::INodeValuePtr type_node = template_node->getChild(L"type", false);
                if (type_node.isOk() && type_node->getString() == L"query")
                {
                    should_refresh_tree = false;
                }
            }
            file.clear();
        }
        
        db->deleteFile(towstr(path));
    }


    // TODO: for now allow JSON to be saved by directly
    // specifying it in the extension; this is to allow
    // testing of the JSON format while still saving
    // in the XML format as the primary format

    // if the path ends in .json, save the file
    // in the json format, otherwise, use XML
    wxString ext = path.AfterLast(wxT('.'));
    if (ext.Length() < path.Length() && ext.CmpNoCase(wxT("json")) == 0)
        saveJson(path);
         else
        saveXml(path);
    

    // refresh tree
    if (should_refresh_tree && refresh_tree_if_necessary)
    {
        g_app->getAppController()->refreshDbDoc();
    }

    return true;
}

bool QueryTemplate::load(const wxString& path)
{
    // try to load the path in the new JSON format
    if (loadJson(path))
        return true;

    // if we can't load it in the new format, try
    // to open it with the old format
    return loadXml(path);
}

cfw::IJobPtr QueryTemplate::execute(int site_id)
{
    // create and run the query job
    wxString sql = getQueryString();
    int flags = 0;
    
    if (getDatabaseDisposition() == tango::dbtypeXdnative)
    {
        // local database queries require this because of
        // view bugs in queries that don't return all columns
        flags = tango::sqlAlwaysCopy;
    }
     else
    {
        // for non-local queries, use pass-through to increase performance
        flags = tango::sqlPassThrough;
    }
    
    QueryJob* job = new QueryJob;
    job->setExtraLong(site_id);
    job->sigJobFinished().connect(&onQueryJobFinished);
    job->getJobInfo()->setTitle(_("Query"));
    job->setQuery(sql, flags);

    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
    return static_cast<cfw::IJob*>(job);
}

wxString QueryTemplate::getQueryString()
{
    wxString columns;
    wxString from;
    wxString join;
    wxString where;
    wxString group_by;
    wxString having;
    wxString order_by;

    std::vector<wxString> top_tables;
    std::vector<QueryJoin> all_joins;

    std::vector<QueryBuilderSourceTable>::iterator tbl_it;
    std::vector<QueryBuilderParam>::iterator param_it;
    std::vector<QueryJoin>::iterator join_it;
    std::vector<wxString>::iterator s_it;

    updateValidationStructure();


    // calculate the 'database disposition'.  If the various source tables
    // are from the same database or mount, then we'll use that database's
    // SQL semantics.  If the source tables are from different databases, we'll
    // use tango's native sql engine, which pulls the source tables down
    // first, and then issues the query locally.
    int disposition = -1;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isOk())
    {
        for (tbl_it = m_source_tables.begin();
             tbl_it != m_source_tables.end(); ++tbl_it)
        {
            int d = tango::dbtypeXdnative;
            
            tango::IDatabasePtr mnt_db = db->getMountDatabase(towstr(tbl_it->path));
            if (mnt_db)
            {
                d = mnt_db->getDatabaseType();
            }

            if (disposition == -1)
                disposition = d;
            if (disposition != d)
            {
                disposition = tango::dbtypeXdnative;
                break;
            }
        }
    }
    
    if (disposition == -1)
        disposition = tango::dbtypeXdnative;
    
    m_disposition = disposition;
    

    // collect all joins
    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        for (join_it = tbl_it->joins.begin();
             join_it != tbl_it->joins.end(); ++join_it)
        {
            all_joins.push_back(*join_it);
        }
    }


    // find top tables (those tables which are not
    // 'right' tables in joins
    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        bool found = false;

        for (join_it = all_joins.begin();
             join_it != all_joins.end(); ++join_it)
        {
            if (join_it->right_path.CmpNoCase(tbl_it->path) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            top_tables.push_back(tbl_it->path);
        }
    }

    // if we have no top tables, bail out
    if (top_tables.size() == 0)
        return wxT("");


    // build column list
    for (param_it = m_params.begin();
         param_it != m_params.end(); ++param_it)
    {
        if (!param_it->output)
            continue;

        if (!columns.IsEmpty())
            columns += wxT(", ");

        columns += makeQueryGroupFunction(param_it->input_expr, param_it->group_func);

        if (param_it->output_field.Length() > 0)
        {
            columns += wxT(" AS ");
            
            wxString output_field = quoteField(param_it->output_field);
            columns += output_field;
        }
    }


    // build FROM clause
    for (s_it = top_tables.begin(); s_it != top_tables.end(); ++s_it)
    {
        QueryBuilderSourceTable* tbl;
        
        tbl = lookupTableByPath(*s_it);

        if (tbl)
        {
            if (s_it != top_tables.begin())
            {
                from += wxT(", ");
            }

            from += formatTableName(*s_it);
        }

        wxString join_str = buildJoinString(*tbl);
        if (!join_str.IsEmpty())
        {
            from += wxT(" ");
            from += join_str;
        }
    }

    // build WHERE clause
    size_t i;

    for (i = 0; i < 255; ++i)
    {
        wxString condition;

        for (param_it = m_params.begin();
             param_it != m_params.end(); ++param_it)
        {
            wxString piece = wxT("");

            if (i >= 0 && i < param_it->conditions.size())
            {
                piece = param_it->conditions[i];
            }

            if (param_it->group_func == QueryGroupFunction_None &&
                !piece.IsEmpty())
            {
                if (!condition.IsEmpty())
                    condition += wxT(" AND ");

                condition += wxT("(");
                condition += completeFilter(piece, param_it->input_expr);
                condition += wxT(")");
            }
        }

        if (!condition.IsEmpty())
        {
            if (!where.IsEmpty())
                where += wxT(" OR ");

            where += wxT("(");
            where += condition;
            where += wxT(")");
        }
    }


    // build HAVING clause
    for (i = 0; i < 255; ++i)
    {
        wxString condition;

        for (param_it = m_params.begin();
             param_it != m_params.end();
             ++param_it)
        {
            wxString piece = wxT("");

            if (i >= 0 && i < param_it->conditions.size())
            {
                piece = param_it->conditions[i];
            }

            if (param_it->group_func != QueryGroupFunction_None &&
                !piece.IsEmpty())
            {
                if (!condition.IsEmpty())
                    condition += wxT(" AND ");

                condition += wxT("(");
                condition += completeFilter(piece,
                                            param_it->input_expr,
                                            param_it->group_func);
                condition += wxT(")");
            }
        }

        if (!condition.IsEmpty())
        {
            if (!having.IsEmpty())
                having += wxT(" OR ");

            having += wxT("(");
            having += condition;
            having += wxT(")");
        }
    }


    // build GROUP BY clause
    for (param_it = m_params.begin();
         param_it != m_params.end(); ++param_it)
    {
        if (param_it->group_func == QueryGroupFunction_GroupBy)
        {
            if (!group_by.IsEmpty())
            {
                group_by += wxT(", ");
            }

            group_by += quoteField(param_it->input_expr);
        }
    }


    // build ORDER BY clause
    int order;
    for (order = 1; order <= 50; ++order)
    {
        bool found = false;
        
        int param_idx = 0;
        int sort_order = 0;
        int p = 0;

        for (param_it = m_params.begin();
             param_it != m_params.end(); ++param_it)
        {
            if (abs(param_it->sort_order) == order)
            {
                if (found)
                {
                    // two identical sort specifiers found:
                    // this is not allowed

                    return wxEmptyString;
                }

                param_idx = p;
                sort_order = param_it->sort_order;

                found = true;
            }

            ++p;
        }

        if (found)
        {
            if (!order_by.IsEmpty())
                order_by += wxT(", ");

            if (m_params[param_idx].output_field.length() > 0)
            {
                order_by += quoteField(m_params[param_idx].output_field);
            }
             else
            {
                order_by += quoteField(m_params[param_idx].input_expr);
            }

            if (sort_order < 0)
            {
                order_by += wxT(" DESC");
            }
        }
    }


    wxString sql;

    sql = wxT("SELECT ");

    if (m_distinct)
    {
        sql += wxT("DISTINCT ");
    }

    sql += columns;

    if (m_output_path.Length() > 0)
    {
        sql += wxT(" INTO ");
        sql += m_output_path;
    }


    sql += wxT(" FROM ");
    sql += from;

    if (!where.IsEmpty())
    {
        sql += wxT(" WHERE ");
        sql += where;
    }

    if (!group_by.IsEmpty())
    {
        sql += wxT(" GROUP BY ");
        sql += group_by;
    }

    if (!having.IsEmpty())
    {
        sql += wxT(" HAVING ");
        sql += having;
    }

    if (!order_by.IsEmpty())
    {
        sql += wxT(" ORDER BY ");
        sql += order_by;
    }

    return sql;
}

QueryBuilderSourceTable* QueryTemplate::lookupTableByPath(const wxString& path)
{
    std::vector<QueryBuilderSourceTable>::iterator tbl_it;

    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        if (tbl_it->path.CmpNoCase(path) == 0)
            return &(*tbl_it);
    }

    return NULL;
}

QueryBuilderSourceTable* QueryTemplate::lookupTableByAlias(const wxString& alias)
{
    std::vector<QueryBuilderSourceTable>::iterator tbl_it;

    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        if (tbl_it->alias.CmpNoCase(alias) == 0)
            return &(*tbl_it);
    }

    return NULL;
}

tango::IColumnInfoPtr QueryTemplate::lookupColumnInfo(const wxString& input)
{
    tango::IColumnInfoPtr colinfo;
    QueryBuilderSourceTable* tbl;
    
    if (input.IsEmpty())
        return xcm::null;

    wxString alias = input.BeforeFirst(wxT('.'));
    wxString fname = input.AfterFirst(wxT('.'));

    tbl = lookupTableByAlias(alias);
    if (!tbl)
        return xcm::null;

    return tbl->structure->getColumnInfo(towstr(fname));
}

wxString QueryTemplate::stripAllAliases(const wxString& input)
{
    std::vector<QueryBuilderSourceTable>::iterator tbl_it;

    std::wstring result = towstr(input);
    
    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        std::wstring alias_dot = towstr(tbl_it->alias);
        alias_dot += L".";
        
        kl::replaceStrNoCase(result, alias_dot, L"");
    }

    return towx(result);
}

wxString QueryTemplate::buildJoinString(QueryBuilderSourceTable& tbl)
{
    wxString s;

    std::vector<QueryJoin>::iterator join_it;

    for (join_it = tbl.joins.begin();
         join_it != tbl.joins.end(); ++join_it)
    {
        if (!s.IsEmpty())
            s += wxT(" ");

        switch (join_it->join_type)
        {
            case QueryJoinNone:
                return wxT("");

            case QueryJoinInner:
                s += wxT("INNER JOIN");
                break;

            case QueryJoinLeftOuter:
                s += wxT("LEFT OUTER JOIN");
                break;

            case QueryJoinRightOuter:
                s += wxT("RIGHT OUTER JOIN");
                break;

            case QueryJoinFullOuter:
                s += wxT("FULL OUTER JOIN");
                break;
        }
        
        s += wxT(" ");
        s += formatTableName(join_it->right_path);
        s += wxT(" ON ");

        
        QueryBuilderSourceTable* right_tbl;
        right_tbl = lookupTableByPath(join_it->right_path);
        if (!right_tbl)
            return wxEmptyString;

        std::vector<wxString> left_parts;
        std::vector<wxString> right_parts;

        wxStringTokenizer left(join_it->left_columns, wxT(","));
        while (left.HasMoreTokens())
        {
            wxString fld;

            fld = quoteAlias(tbl.alias);
            fld += wxT(".");
            fld += quoteField(left.GetNextToken());

            left_parts.push_back(fld);
        }

        wxStringTokenizer right(join_it->right_columns, wxT(","));
        while (right.HasMoreTokens())
        {
            wxString fld;

            fld = quoteAlias(right_tbl->alias);
            fld += wxT(".");
            fld += quoteField(right.GetNextToken());

            right_parts.push_back(fld);
        }

        if (left_parts.size() != right_parts.size())
        {
            return wxT("");
        }


        size_t i;
        for (i = 0; i < left_parts.size(); ++i)
        {
            if (i > 0)
            {
                s += wxT(" AND ");
            }

            s += left_parts[i];
            s += wxT("=");
            s += right_parts[i];
        }


        QueryBuilderSourceTable* tbl = lookupTableByPath(join_it->right_path);
        if (tbl)
        {
            s += wxT(" ");
            s += buildJoinString(*tbl);
        }
    }

    return s;
}

wxString QueryTemplate::completeFilter(const wxString& _expr,
                                       const wxString& _input,
                                       int group_func)
{
    int type;

    tango::IColumnInfoPtr colinfo;
    colinfo = lookupColumnInfo(_input);
    if (colinfo.isOk())
    {
        type = colinfo->getType();
    }
     else
    {
        switch (group_func)
        {
            case QueryGroupFunction_Count:
                type = tango::typeNumeric;
                break;

            case QueryGroupFunction_GroupID:
                type = tango::typeNumeric;
                break;

            default:
            {
                // if there are multiple tables, we can't reliably determine
                // the type of the output expression without doing more work here;
                // for now, if there's only one table input, strip all the aliases
                // off and parse the expression to determine its type
                if (m_source_tables.size() != 1)
                    return wxEmptyString;
                
                type = m_source_tables[0].structure->getExprType(towstr(stripAllAliases(_input)));
                if (type == tango::typeInvalid || type == tango::typeUndefined)
                    return wxEmptyString;
            }
        }
    }


    wxString input = makeQueryGroupFunction(_input, group_func);

    wxString output;
    output.Alloc(_expr.Length() + input.Length() * 3);

    wxString expr = _expr;
    expr.Trim(false);
    expr.Trim();

    const wxChar* c = expr.c_str();
    
    output += input;

    if (*c == wxT(':'))
    {
        // a colon at the beginning will cause the query builder
        // to do no expression processing and just pass through
        // whatever follows the colon
        wxString result = c+1;
        return result;
    }

    if (wxStrchr(wxT("<>!="), *c))
    {
        // operator found
        while (wxStrchr(wxT("<>!="), *c))
        {
            output += *c;
            ++c;
        }
        
        while (wxIsspace(*c))
            ++c;

        wxString temps = c;
        expr = temps;
    }
     else
    {
        output += wxT("=");
    }


    // if when we append the right part of the expression it validates to
    // a boolean expression, the whole expression is valid and requires no
    // further pre-processing; otherwise, we need to massage the values by
    // adding quotes and/or functions
    wxString test;
    test = output;
    test += expr;

    if (m_validation_struct->getExprType(towstr(test)) == tango::typeBoolean)
    {
        return test;
    }


    switch (type)
    {
        case tango::typeWideCharacter:
        case tango::typeCharacter:
        {
            output += wxT("'");

            // double quote print
            const wxChar* c = expr.c_str();
            while (*c)
            {
                if (*c == wxT('\''))
                    output += wxT("''");
                     else
                    output += *c;

                ++c;
            }

            output += wxT("'");
        }
        break;

        case tango::typeNumeric:
        case tango::typeDouble:
        {
            expr.Replace(wxT(","), wxT("."));

            // check that all of the characters are digits
            const wxChar* c = expr.c_str();

            while (*c)
            {
                if (!wxIsspace(*c) &&
                    !wxIsdigit(*c) &&
                    *c != wxT('.') &&
                    *c != wxT('-'))
                {
                    return wxEmptyString;
                }

                ++c;
            }

            output += expr;
        }
        break;

        case tango::typeInteger:
        {
            // check that all of the characters are digits
            const wxChar* c = expr.c_str();

            while (*c)
            {
                if (!wxIsspace(*c) && !wxIsdigit(*c))
                    return wxEmptyString;
                ++c;
            }

            output += expr;
        }
        break;

        case tango::typeDate:
        case tango::typeDateTime:
        {
            int y, m, d, hh, mm, ss;
            bool valid = cfw::Locale::parseDateTime(expr,
                                                    &y, &m, &d,
                                                    &hh, &mm, &ss);

            if (!valid)
                return wxEmptyString;

            if (hh == -1 || mm == -1)
            {
                switch (m_disposition)
                {
                    default:
                    case tango::dbtypeXdnative:
                        output += wxString::Format(wxT("DATE(%d,%d,%d)"),
                                                   y, m, d);
                        break;
                    case tango::dbtypeOdbc:
                    case tango::dbtypeSqlServer:
                    case tango::dbtypeMySql:
                        output += wxString::Format(wxT("{d '%04d-%02d-%02d'}"),
                                                   y, m, d);
                        break;
                    case tango::dbtypeOracle:
                        output += wxString::Format(wxT("TO_DATE('%04d-%02d-%02d','YYYY-MM-DD')"),
                                                   y, m, d);
                        break;
                }
            }
             else
            {
                switch (m_disposition)
                {
                    default:
                    case tango::dbtypeXdnative:
                        output += wxString::Format(wxT("DATE(%d,%d,%d,%d,%d,%d)"),
                                                   y, m, d, hh, mm, ss);
                        break;
                    case tango::dbtypeOdbc:
                    case tango::dbtypeSqlServer:
                    case tango::dbtypeMySql:
                        output += wxString::Format(wxT("{ts '%04d-%02d-%02d %02d:%02d:%02d'}"),
                                                   y, m, d, hh, mm, ss);
                        break;
                    case tango::dbtypeOracle:
                        output += wxString::Format(wxT("TO_DATE('%04d-%02d-%02d %02d:%02d:%02d', 'YYYY-MM-DD HH24:MI:SS')"),
                                                   y, m, d, hh, mm, ss);
                        break;
                }
            }
        }
        break;

        case tango::typeBoolean:
            if (0 != expr.CmpNoCase(wxT("TRUE")) &&
                0 != expr.CmpNoCase(wxT("FALSE")))
            {
                // boolean conditions must equate to either true or false
                return wxEmptyString;
            }
        break;

        default:
            output += expr;
            break;
    }


    output.Trim();
    output.Trim(TRUE);

    return output;
}

void QueryTemplate::updateValidationStructure()
{
    m_validation_struct = g_app->getDatabase()->createStructure();

    std::vector<QueryBuilderSourceTable>::iterator tbl_it;

    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {   
        int col_count = tbl_it->structure->getColumnCount();
        int i;

        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = tbl_it->structure->getColumnInfoByIdx(i);

            wxString name;
            name = tbl_it->alias;
            name += wxT(".");
            name += towx(colinfo->getName());

            tango::IColumnInfoPtr newcol = m_validation_struct->createColumn();
            newcol->setName(towstr(name));
            newcol->setType(colinfo->getType());
            newcol->setWidth(colinfo->getWidth());
            newcol->setScale(colinfo->getScale());
            newcol->setCalculated(colinfo->getCalculated());
            newcol->setExpression(colinfo->getExpression());
        }
    }


    std::vector<QueryBuilderParam>::iterator param_iter;

    for (param_iter = m_params.begin();
         param_iter != m_params.end();
         ++param_iter)
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = lookupColumnInfo(param_iter->input_expr);

        if (colinfo.isOk() && param_iter->output_field.length() > 0)
        {
            tango::IColumnInfoPtr newcol = m_validation_struct->createColumn();
            newcol->setName(towstr(param_iter->output_field));
            newcol->setType(colinfo->getType());
            newcol->setWidth(colinfo->getWidth());
            newcol->setScale(colinfo->getScale());
            newcol->setCalculated(colinfo->getCalculated());
            newcol->setExpression(colinfo->getExpression());
        }
    }
}

bool QueryTemplate::saveJson(const wxString& path)
{
    JsonNode root;


    // query info
    JsonNode metadata = root["metadata"];
    metadata["type"] = wxT("application/vnd.kx.query");
    metadata["version"] = 1;
    metadata["description"] = wxT("");


    // input info
    root["input"].setArray();
    JsonNode input = root["input"];

    std::vector<QueryBuilderSourceTable>::iterator tbl_it;
    int tbl_counter = 0;
    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        JsonNode table = input.appendElement();
        
        // write alias
        table["alias"] = tbl_it->alias;
        table["path"] = tbl_it->path;
        table["x"] = tbl_it->x;
        table["y"] = tbl_it->y;
        table["width"] = tbl_it->width;
        table["height"] = tbl_it->height;
        
        
        // if joins are specified, add the joins
        if (tbl_it->joins.size() > 0)
        {
            JsonNode joins = table["joins"];
            joins.setArray();

            std::vector<QueryJoin>::iterator join_iter;
            for (join_iter = tbl_it->joins.begin();
                 join_iter != tbl_it->joins.end(); ++join_iter)
            {
                JsonNode join = joins.appendElement();
                
                JsonNode join_type = join["type"];
                switch (join_iter->join_type)
                {
                    default:
                    case QueryJoinNone:       join_type.setString(L"none"); break;
                    case QueryJoinInner:      join_type.setString(L"inner"); break;
                    case QueryJoinLeftOuter:  join_type.setString(L"left_outer"); break;
                    case QueryJoinRightOuter: join_type.setString(L"right_outer"); break;
                    case QueryJoinFullOuter:  join_type.setString(L"full_outer"); break;
                }

                JsonNode join_table = join["table"];
                join_table.setString(join_iter->right_path);

                JsonNode left_columns = join["left_columns"];
                left_columns.setString(join_iter->left_columns);
                
                JsonNode right_columns = join["right_columns"];
                right_columns.setString(join_iter->right_columns);
            }
        }
    }


    // output info
    JsonNode output = root["output"];

    output["table"].setString(m_output_path);
    output["distinct"].setBoolean(m_distinct);

    output["fields"].setArray();
    JsonNode fields = output["fields"];

    std::vector<QueryBuilderParam>::iterator param_iter;
    int param_counter = 0;
    for (param_iter = m_params.begin();
         param_iter != m_params.end();
         ++param_iter)
    {
        JsonNode field = fields.appendElement();
        
        field["output"].setBoolean(param_iter->output);
        field["input"].setString(param_iter->input_expr);
        field["name"].setString(param_iter->output_field);        

        switch (param_iter->group_func)
        {
            default:
            case QueryGroupFunction_None:     field["group_function"].setString(L"none"); break;
            case QueryGroupFunction_GroupBy:  field["group_function"].setString(L"group_by"); break;
            case QueryGroupFunction_First:    field["group_function"].setString(L"first"); break;
            case QueryGroupFunction_Last:     field["group_function"].setString(L"last"); break;
            case QueryGroupFunction_Min:      field["group_function"].setString(L"min"); break;
            case QueryGroupFunction_Max:      field["group_function"].setString(L"max"); break;
            case QueryGroupFunction_Sum:      field["group_function"].setString(L"sum"); break;
            case QueryGroupFunction_Avg:      field["group_function"].setString(L"avg"); break;
            case QueryGroupFunction_Count:    field["group_function"].setString(L"count"); break;
            case QueryGroupFunction_Stddev:   field["group_function"].setString(L"stddev"); break;
            case QueryGroupFunction_Variance: field["group_function"].setString(L"variance"); break;
            case QueryGroupFunction_GroupID: field["group_function"].setString(L"group_id"); break;
        }
        
        field["order"].setInteger(param_iter->sort_order);

        // if conditions are specified, add the conditions
        if (param_iter->conditions.size() > 0)
        {
            JsonNode conditions = field["conditions"];
            conditions.setArray();

            std::vector<wxString>::iterator cond_it;
            for (cond_it = param_iter->conditions.begin();
                 cond_it != param_iter->conditions.end();
                 ++cond_it)
            {
                // note: need to save all conditions, even if the
                // condition length is zero since the condition
                // positions map to the "criteria" or one of the
                // "or" cells in the display
                JsonNode condition = conditions.appendElement();
                condition.setString(*cond_it);
            }
        }
    }

    return JsonConfig::saveToDb(root, g_app->getDatabase(), path, wxT("application/vnd.kx.query"));
}

bool QueryTemplate::loadJson(const wxString& path)
{
    // try to load the JSON string
    JsonNode root = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (!root.isOk())
        return false;

    // TODO: check version info

    m_source_tables.clear();
    m_params.clear();

    // input info
    JsonNode input = root["input"];
    if (input.isOk())
    {
        QueryBuilderSourceTable query_table;

        size_t i, input_count = input.getCount();
        for (i =  0; i < input_count; ++i)
        {
            JsonNode table = input[i];
            
            // TODO: check for existence of following elements
            // instead of assuming them

            query_table.alias = table["alias"];
            query_table.path = table["path"];
            query_table.x = table["x"].getInteger();
            query_table.y = table["y"].getInteger();
            query_table.width = table["width"].getInteger();
            query_table.height = table["height"].getInteger();
            
            // attempt to open the set and get it's structure
            tango::ISetPtr set = g_app->getDatabase()->openSet(towstr(query_table.path));
            if (set.isNull())
                continue;
                 else
                query_table.structure = set->getStructure();

            // if joins are specified, load them                
            JsonNode joins = table["joins"];
            if (joins.isOk())
            {
                size_t j, join_count = joins.getCount();
                for (j = 0; j < join_count; ++j)
                {
                    QueryJoin query_join;
                    JsonNode join = joins[j];                

                    wxString type = join["type"];
                    query_join.join_type = QueryJoinNone;    // default
                    
                    if (type == wxT("none"))
                        query_join.join_type = QueryJoinNone;
                    if (type == wxT("inner"))
                        query_join.join_type = QueryJoinInner;
                    if (type == wxT("left_outer"))
                        query_join.join_type = QueryJoinLeftOuter;
                    if (type == wxT("right_outer"))
                        query_join.join_type = QueryJoinRightOuter;
                    if (type == wxT("full_outer"))
                        query_join.join_type = QueryJoinFullOuter;

                    // TODO: check for existence of following elements
                    // instead of assuming them

                    query_join.right_path = join["table"];
                    query_join.left_columns = join["left_columns"];
                    query_join.right_columns = join["right_columns"];

                    // set the join info
                    query_table.joins.push_back(query_join);
                }
            }
            
            // set the table info
            m_source_tables.push_back(query_table);
        }
    }


    // output info
    JsonNode output = root["output"];
    if (output.isOk())
    {
        // TODO: check for existence of following
        m_output_path = output["table"].getString();
        m_distinct = output["distinct"].getBoolean();
        
        JsonNode fields = output["fields"];
        size_t f, field_count = fields.getCount();
        
        for (f = 0; f < field_count; ++f)
        {
            QueryBuilderParam query_param;
            JsonNode field = fields[f];

            query_param.output = field["output"].getBoolean();
            query_param.input_expr = field["input"].getString();
            query_param.output_field = field["name"].getString();

            wxString group_function = field["group_function"].getString();
            query_param.group_func = QueryGroupFunction_None;   // default

            if (group_function == wxT("none"))
                query_param.group_func = QueryGroupFunction_None;
            if (group_function == wxT("group_by"))
                query_param.group_func = QueryGroupFunction_GroupBy;
            if (group_function == wxT("first"))
                query_param.group_func = QueryGroupFunction_First;
            if (group_function == wxT("last"))
                query_param.group_func = QueryGroupFunction_Last;
            if (group_function == wxT("min"))
                query_param.group_func = QueryGroupFunction_Min;
            if (group_function == wxT("max"))
                query_param.group_func = QueryGroupFunction_Max;
            if (group_function == wxT("sum"))
                query_param.group_func = QueryGroupFunction_Sum;
            if (group_function == wxT("avg"))
                query_param.group_func = QueryGroupFunction_Avg;
            if (group_function == wxT("count"))
                query_param.group_func = QueryGroupFunction_Count;
            if (group_function == wxT("stddev"))
                query_param.group_func = QueryGroupFunction_Stddev;
            if (group_function == wxT("variance"))
                query_param.group_func = QueryGroupFunction_Variance;
            if (group_function == wxT("group_id"))
                query_param.group_func = QueryGroupFunction_GroupID;

            query_param.sort_order = field["order"].getInteger();
            
            JsonNode conditions = field["conditions"];
            if (conditions.isOk())
            {
                size_t c, condition_count = conditions.getCount();
                for (c = 0; c < condition_count; ++c)
                {
                    JsonNode condition = conditions[c];
                    query_param.conditions.push_back(condition.getString());
                }
            }

            m_params.push_back(query_param);
        }
    }

    return true;
    
/*

    // criteria collection
    tango::INodeValuePtr parameters = data_root->getChild(L"parameters", false);
    if (!parameters)
        return false;

    int param_count = parameters->getChildCount();

    int param_counter;
    for (param_counter = 0; param_counter < param_count; ++param_counter)
    {
        QueryBuilderParam p;

        tango::INodeValuePtr param = parameters->getChildByIdx(param_counter);

        tango::INodeValuePtr output_node = param->getChild(L"output", false);
        if (!output_node)
            return false;
        p.output = output_node->getBoolean();

        tango::INodeValuePtr input_expr = param->getChild(L"input_expr", false);
        if (!input_expr)
            return false;
        p.input_expr = towx(input_expr->getString());

        tango::INodeValuePtr output_field = param->getChild(L"output_field", false);
        if (!output_field)
            return false;
        p.output_field = towx(output_field->getString());

        tango::INodeValuePtr group_func_node = param->getChild(L"group_func", false);
        if (!group_func_node)
            return false;
        
        wxString group_func = towx(group_func_node->getString());

        if (!group_func.CmpNoCase(wxT("group_by")))
        {
            p.group_func = QueryGroupFunction_GroupBy;
        }
         else if (!group_func.CmpNoCase(wxT("first")))
        {
            p.group_func = QueryGroupFunction_GroupBy;
            //p.group_func = QueryGroupFunction_First;
        }
         else if (!group_func.CmpNoCase(wxT("last")))
        {
            p.group_func = QueryGroupFunction_GroupBy;
            //p.group_func = QueryGroupFunction_Last;
        }
         else if (!group_func.CmpNoCase(wxT("min")))
        {
            p.group_func = QueryGroupFunction_Min;
        }
         else if (!group_func.CmpNoCase(wxT("max")))
        {
            p.group_func = QueryGroupFunction_Max;
        }
         else if (!group_func.CmpNoCase(wxT("sum")))
        {
            p.group_func = QueryGroupFunction_Sum;
        }
         else if (!group_func.CmpNoCase(wxT("avg")))
        {
            p.group_func = QueryGroupFunction_Avg;
        }
         else if (!group_func.CmpNoCase(wxT("count")))
        {
            p.group_func = QueryGroupFunction_Count;
        }
         else if (!group_func.CmpNoCase(wxT("stddev")))
        {
            p.group_func = QueryGroupFunction_Stddev;
        }
         else if (!group_func.CmpNoCase(wxT("variance")))
        {
            p.group_func = QueryGroupFunction_Variance;
        }
         else if (!group_func.CmpNoCase(wxT("group_id")))
        {
            p.group_func = QueryGroupFunction_GroupID;
        }
         else
        {
            p.group_func = QueryGroupFunction_None;
        }


        tango::INodeValuePtr sort_order = param->getChild(L"sort_order", false);
        if (!sort_order)
            return false;
        p.sort_order = sort_order->getInteger();

        
        wchar_t buf[255];
        int cond = 0;
        while (1)
        {
            swprintf(buf, 255, L"condition_%d", cond++);
            tango::INodeValuePtr condition = param->getChild(buf, false);
            if (!condition)
                break;
            p.conditions.push_back(towx(condition->getString()));
        }


        m_params.push_back(p);
    }

    return true;
*/
}

bool QueryTemplate::saveXml(const wxString& path)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    tango::INodeValuePtr file = db->createNodeFile(towstr(path));
    if (file.isNull())
        return false;

    // set up template root
    tango::INodeValuePtr kpp_template = file->createChild(L"kpp_template");
    
    tango::INodeValuePtr template_type = kpp_template->createChild(L"type");
    template_type->setString(L"query");

    tango::INodeValuePtr template_version = kpp_template->createChild(L"version");
    template_version->setString(L"1");

    tango::INodeValuePtr data_root = kpp_template->createChild(L"data");

    // write output path
    tango::INodeValuePtr output_path_node = data_root->createChild(L"output_path");
    output_path_node->setString(towstr(m_output_path));

    // write distinct flag
    tango::INodeValuePtr distinct_node = data_root->createChild(L"distinct");
    distinct_node->setBoolean(m_distinct);

    // write out source tables
    tango::INodeValuePtr tables_base = data_root->createChild(L"source_tables");
    
    std::vector<QueryBuilderSourceTable>::iterator tbl_it;
    int tbl_counter = 0;
    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        wxString temps;
        temps.Printf(wxT("source_table_%d"), tbl_counter++);

        tango::INodeValuePtr table = tables_base->createChild(towstr(temps));

        // write alias
        tango::INodeValuePtr alias_node = table->createChild(L"alias");
        alias_node->setString(towstr(tbl_it->alias));

        // write path
        tango::INodeValuePtr path_node = table->createChild(L"path");
        path_node->setString(towstr(tbl_it->path));

        // write coordinates
        tango::INodeValuePtr x_node = table->createChild(L"x");
        x_node->setInteger(tbl_it->x);

        tango::INodeValuePtr y_node = table->createChild(L"y");
        y_node->setInteger(tbl_it->y);

        tango::INodeValuePtr width_node = table->createChild(L"width");
        width_node->setInteger(tbl_it->width);

        tango::INodeValuePtr height_node = table->createChild(L"height");
        height_node->setInteger(tbl_it->height);

        // join collection
        tango::INodeValuePtr joins = table->createChild(L"joins");

        std::vector<QueryJoin>::iterator join_iter;
        int join_counter = 0;
        for (join_iter = tbl_it->joins.begin();
             join_iter != tbl_it->joins.end(); ++join_iter)
        {
            wxString temps;
            temps.Printf(wxT("join_%d"), join_counter++);

            tango::INodeValuePtr join = joins->createChild(towstr(temps));

            // join type
            tango::INodeValuePtr join_type = join->createChild(L"type");
            switch (join_iter->join_type)
            {
                default:
                case QueryJoinNone:       join_type->setString(L"none"); break;
                case QueryJoinInner:      join_type->setString(L"inner"); break;
                case QueryJoinLeftOuter:  join_type->setString(L"left_outer"); break;
                case QueryJoinRightOuter: join_type->setString(L"right_outer"); break;
                case QueryJoinFullOuter:  join_type->setString(L"full_outer"); break;
            }

            // write right_path
            tango::INodeValuePtr right_path_node = join->createChild(L"right_path");
            right_path_node->setString(towstr(join_iter->right_path));

            // write left columns
            tango::INodeValuePtr left_columns_node = join->createChild(L"left_columns");
            left_columns_node->setString(towstr(join_iter->left_columns));

            // write right columns
            tango::INodeValuePtr right_columns_node = join->createChild(L"right_columns");
            right_columns_node->setString(towstr(join_iter->right_columns));
        }
    }

    // parameters collection
    tango::INodeValuePtr parameters = data_root->createChild(L"parameters");

    std::vector<QueryBuilderParam>::iterator param_iter;
    int param_counter = 0;
    for (param_iter = m_params.begin();
         param_iter != m_params.end();
         ++param_iter)
    {
        wxString temps;
        temps.Printf(wxT("parameter_%d"), param_counter++);

        tango::INodeValuePtr param = parameters->createChild(towstr(temps));

        tango::INodeValuePtr output_node = param->createChild(L"output");
        output_node->setBoolean(param_iter->output);

        tango::INodeValuePtr input_expr = param->createChild(L"input_expr");
        input_expr->setString(towstr(param_iter->input_expr));

        tango::INodeValuePtr output_field = param->createChild(L"output_field");
        output_field->setString(towstr(param_iter->output_field));

        tango::INodeValuePtr group_func = param->createChild(L"group_func");
        switch (param_iter->group_func)
        {
            default:
            case QueryGroupFunction_None:     group_func->setString(L"none"); break;
            case QueryGroupFunction_GroupBy:  group_func->setString(L"group_by"); break;
            case QueryGroupFunction_First:    group_func->setString(L"first"); break;
            case QueryGroupFunction_Last:     group_func->setString(L"last"); break;
            case QueryGroupFunction_Min:      group_func->setString(L"min"); break;
            case QueryGroupFunction_Max:      group_func->setString(L"max"); break;
            case QueryGroupFunction_Sum:      group_func->setString(L"sum"); break;
            case QueryGroupFunction_Avg:      group_func->setString(L"avg"); break;
            case QueryGroupFunction_Count:    group_func->setString(L"count"); break;
            case QueryGroupFunction_Stddev:   group_func->setString(L"stddev"); break;
            case QueryGroupFunction_Variance: group_func->setString(L"variance"); break;
            case QueryGroupFunction_GroupID: group_func->setString(L"group_id"); break;
        }

        tango::INodeValuePtr sort_order = param->createChild(L"sort_order");
        sort_order->setInteger(param_iter->sort_order);

        std::vector<wxString>::iterator cond_it;
        int condition_counter = 0;

        for (cond_it = param_iter->conditions.begin();
             cond_it != param_iter->conditions.end();
             ++cond_it)
        {
            wchar_t buf[255];
            swprintf(buf, 255, L"condition_%d", condition_counter++);

            tango::INodeValuePtr condition;
            condition = param->createChild(buf);
            condition->setString(towstr(*cond_it));
        }
    }
    
    return true;
}

bool QueryTemplate::loadXml(const wxString& path)
{
    m_source_tables.clear();
    m_params.clear();

    // create an ofs file with the run information
    tango::IDatabasePtr db = g_app->getDatabase();

    tango::INodeValuePtr file = db->openNodeFile(towstr(path));
    if (file.isNull())
        return false;

    tango::INodeValuePtr kpp_template = file->getChild(L"kpp_template", false);
    if (!kpp_template)
        return false;

    tango::INodeValuePtr template_type = kpp_template->getChild(L"type", false);
    if (!template_type)
        return false;

    if (template_type->getString() != L"query")
        return false;

    tango::INodeValuePtr data_root = kpp_template->getChild(L"data", false);
    if (!data_root)
        return false;

    // read output path
    tango::INodeValuePtr output_path_node = data_root->getChild(L"output_path", false);
    if (!output_path_node)
        return false;
    m_output_path = towx(output_path_node->getString());

    // read distinct flag
    tango::INodeValuePtr distinct_node = data_root->getChild(L"distinct", false);
    if (!distinct_node)
        return false;
    m_distinct = distinct_node->getBoolean();

    // source tables base
    tango::INodeValuePtr tables_base = data_root->getChild(L"source_tables", false);
    if (!tables_base)
        return false;
    int tables_child_count = tables_base->getChildCount();

    // try to load the input set
    int tbl_counter;
    for (tbl_counter = 0; tbl_counter < tables_child_count; ++tbl_counter)
    {
        QueryBuilderSourceTable tbl;

        tango::INodeValuePtr table = tables_base->getChildByIdx(tbl_counter);

        // read alias
        tango::INodeValuePtr alias_node = table->getChild(L"alias", false);
        if (!alias_node)
            return false;
        tbl.alias = towx(alias_node->getString());

        // read path
        tango::INodeValuePtr path_node = table->getChild(L"path", false);
        if (!path_node)
            return false;
        tbl.path = towx(path_node->getString());

        // read coordinates
        tango::INodeValuePtr x_node = table->getChild(L"x", false);
        if (!x_node)
            return false;
        tbl.x = x_node->getInteger();

        tango::INodeValuePtr y_node = table->getChild(L"y", false);
        if (!y_node)
            return false;
        tbl.y = y_node->getInteger();

        tango::INodeValuePtr width_node = table->getChild(L"width", false);
        if (!width_node)
            return false;
        tbl.width = width_node->getInteger();

        tango::INodeValuePtr height_node = table->getChild(L"height", false);
        if (!height_node)
            return false;
        tbl.height = height_node->getInteger();

        // attempt to open the set and get it's structure
        {
            tango::ISetPtr set = db->openSet(towstr(tbl.path));
            if (set)
            {
                tbl.structure = set->getStructure();
            }
             else
            {
                continue;
            }
        }


        // join collection
        tango::INodeValuePtr joins = table->getChild(L"joins", false);
        if (!joins)
            return false;

        int join_count = joins->getChildCount();

        int join_counter;
        for (join_counter = 0; join_counter < join_count; ++join_counter)
        {
            QueryJoin join;

            tango::INodeValuePtr join_node = joins->getChildByIdx(join_counter);

            tango::INodeValuePtr join_type_node = join_node->getChild(L"type", false);
            if (!join_type_node)
                return false;

            // read join type
            wxString join_type = towx(join_type_node->getString());

            if (!join_type.CmpNoCase(wxT("inner")))
            {
                join.join_type = QueryJoinInner;
            }
             else if (!join_type.CmpNoCase(wxT("left_outer")))
            {
                join.join_type = QueryJoinLeftOuter;
            }
             else if (!join_type.CmpNoCase(wxT("right_outer")))
            {
                join.join_type = QueryJoinRightOuter;
            }
             else if (!join_type.CmpNoCase(wxT("full_outer")))
            {
                join.join_type = QueryJoinFullOuter;
            }
             else
            {
                join.join_type = QueryJoinNone;
            }

            // read right_path
            tango::INodeValuePtr right_path_node = join_node->getChild(L"right_path", false);
            if (!right_path_node)
                return false;
            join.right_path = towx(right_path_node->getString());

            // read left columns
            tango::INodeValuePtr left_columns_node = join_node->getChild(L"left_columns", false);
            if (!left_columns_node)
                return false;
            join.left_columns = towx(left_columns_node->getString());

            // read right columns
            tango::INodeValuePtr right_columns_node = join_node->getChild(L"right_columns", false);
            if (!right_columns_node)
                return false;
            join.right_columns = towx(right_columns_node->getString());

            tbl.joins.push_back(join);
        }

        m_source_tables.push_back(tbl);
    }


    // criteria collection
    tango::INodeValuePtr parameters = data_root->getChild(L"parameters", false);
    if (!parameters)
        return false;

    int param_count = parameters->getChildCount();

    int param_counter;
    for (param_counter = 0; param_counter < param_count; ++param_counter)
    {
        QueryBuilderParam p;

        tango::INodeValuePtr param = parameters->getChildByIdx(param_counter);

        tango::INodeValuePtr output_node = param->getChild(L"output", false);
        if (!output_node)
            return false;
        p.output = output_node->getBoolean();

        tango::INodeValuePtr input_expr = param->getChild(L"input_expr", false);
        if (!input_expr)
            return false;
        p.input_expr = towx(input_expr->getString());

        tango::INodeValuePtr output_field = param->getChild(L"output_field", false);
        if (!output_field)
            return false;
        p.output_field = towx(output_field->getString());

        tango::INodeValuePtr group_func_node = param->getChild(L"group_func", false);
        if (!group_func_node)
            return false;
        
        wxString group_func = towx(group_func_node->getString());

        if (!group_func.CmpNoCase(wxT("group_by")))
        {
            p.group_func = QueryGroupFunction_GroupBy;
        }
         else if (!group_func.CmpNoCase(wxT("first")))
        {
            p.group_func = QueryGroupFunction_GroupBy;
            //p.group_func = QueryGroupFunction_First;
        }
         else if (!group_func.CmpNoCase(wxT("last")))
        {
            p.group_func = QueryGroupFunction_GroupBy;
            //p.group_func = QueryGroupFunction_Last;
        }
         else if (!group_func.CmpNoCase(wxT("min")))
        {
            p.group_func = QueryGroupFunction_Min;
        }
         else if (!group_func.CmpNoCase(wxT("max")))
        {
            p.group_func = QueryGroupFunction_Max;
        }
         else if (!group_func.CmpNoCase(wxT("sum")))
        {
            p.group_func = QueryGroupFunction_Sum;
        }
         else if (!group_func.CmpNoCase(wxT("avg")))
        {
            p.group_func = QueryGroupFunction_Avg;
        }
         else if (!group_func.CmpNoCase(wxT("count")))
        {
            p.group_func = QueryGroupFunction_Count;
        }
         else if (!group_func.CmpNoCase(wxT("stddev")))
        {
            p.group_func = QueryGroupFunction_Stddev;
        }
         else if (!group_func.CmpNoCase(wxT("variance")))
        {
            p.group_func = QueryGroupFunction_Variance;
        }
         else if (!group_func.CmpNoCase(wxT("group_id")))
        {
            p.group_func = QueryGroupFunction_GroupID;
        }
         else
        {
            p.group_func = QueryGroupFunction_None;
        }


        tango::INodeValuePtr sort_order = param->getChild(L"sort_order", false);
        if (!sort_order)
            return false;
        p.sort_order = sort_order->getInteger();

        
        wchar_t buf[255];
        int cond = 0;
        while (1)
        {
            swprintf(buf, 255, L"condition_%d", cond++);
            tango::INodeValuePtr condition = param->getChild(buf, false);
            if (!condition)
                break;
            p.conditions.push_back(towx(condition->getString()));
        }


        m_params.push_back(p);
    }

    return true;
}

