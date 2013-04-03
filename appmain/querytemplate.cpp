/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-04-13
 *
 */


#include "appmain.h"
#include "querytemplate.h"
#include "tabledoc.h"
#include "appcontroller.h"
#include "jsonconfig.h"
#include <set>


static void onQueryJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobStateFinished)
        return;


    bool refresh_tree = false;

    int querydoc_site_id = kl::wtoi(job->getExtraValue(L"appmain.siteid"));
    IDocumentSitePtr querydoc_site;

    if (querydoc_site_id != 0)
        querydoc_site = g_app->getMainFrame()->lookupSiteById(querydoc_site_id);
    
    // check if there is an output set
    tango::IIteratorPtr result_iter = job->getResultObject();

    std::wstring output_path = result_iter->getTable();

    // if the result set isn't temporary, the set has been created
    // with an "INTO" statement and should appear on the tree; so, 
    // set the refresh tree flag to true
    if (!isTemporaryTable(output_path))
        refresh_tree = true;

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
            tabledoc->open(output_path, result_iter);
                    
            wxWindow* container = querydoc_site->getContainerWindow();
            g_app->getMainFrame()->createSite(container,
                                                tabledoc,
                                                true);
        }
            else
        {
            // switch to the table view
            tabledoc->getGrid()->Freeze();
            tabledoc->open(output_path, result_iter);
            tabledoc->refreshActiveView();
            tabledoc->getGrid()->Thaw();

            switchToOtherDocument(querydoc_site, "appmain.TableDoc");
        }
    }
        else
    {
        ITableDocPtr doc = TableDocMgr::createTableDoc();
        doc->setTemporaryModel(true);
        doc->open(output_path, result_iter);

        g_app->getMainFrame()->createSite(doc, sitetypeNormal,
                                            -1, -1, -1, -1);
        refresh_tree = true;
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
    return saveJson(path);
}

bool QueryTemplate::load(const wxString& path)
{
    // try to load the path in the new JSON format
    if (loadJson(path))
        return true;

    // if we can't load it in the new format, try
    // to open it with the old format
    return loadJsonFromNode(path);
}

jobs::IJobPtr QueryTemplate::execute(int site_id)
{
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.execute-job");


    // determine the sql execute flags
    if (getDatabaseDisposition() == tango::dbtypeXdnative)
    {
        // local database queries require this because of
        // view bugs in queries that don't return all columns
        job->setExtraValue(L"tango.sqlAlwaysCopy", L"true");
    }
     else
    {
        // for non-local queries, use pass-through to increase performance
        job->setExtraValue(L"tango.sqlPassThrough", L"true");
    }


    // save the site_id; TODO: this is to work with the existing code;
    // any way to get the equivalent functionality without packaging
    // this up and doing it the way it's implemented in the callback?
    job->setExtraValue(L"appmain.siteid", kl::itowstring(site_id));


    // run the job
    kl::JsonNode params;
    params["command"].setString(towstr(getQueryString()));

    job->getJobInfo()->setTitle(towstr(_("Query")));
    job->setParameters(params.toString());
    job->sigJobFinished().connect(onQueryJobFinished);
    g_app->getJobQueue()->addJob(job, jobStateRunning);

    return static_cast<jobs::IJob*>(job);
}

std::vector<wxString> QueryTemplate::getOutputFields()
{
    std::vector<wxString> output_fields;
    output_fields.reserve(m_params.size());
    
    std::vector<QueryBuilderParam>::iterator it, it_end;
    it_end = m_params.end();
    
    for (it = m_params.begin(); it != it_end; ++it)
    {
        output_fields.push_back(it->output_field);
    }
    
    return output_fields;
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
            if (isSamePath(towstr(join_it->right_path), towstr(tbl_it->path)) == 0)
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
            
            wxString output_field = quoteAlias(param_it->output_field);
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
                order_by += quoteAlias(m_params[param_idx].output_field);
            }
             else
            {
                order_by += quoteAlias(m_params[param_idx].input_expr);
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

            fld = quoteTable(tbl.alias);
            fld += wxT(".");
            fld += quoteField(left.GetNextToken());

            left_parts.push_back(fld);
        }

        wxStringTokenizer right(join_it->right_columns, wxT(","));
        while (right.HasMoreTokens())
        {
            wxString fld;

            fld = quoteTable(right_tbl->alias);
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

    type = m_validation_struct->getExprType(towstr(_input));
    
    if (type == tango::typeInvalid || type == tango::typeUndefined)
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = lookupColumnInfo(_input);
        if (colinfo.isOk())
        {
            type = colinfo->getType();
        }
    }

    if (type == tango::typeInvalid || type == tango::typeUndefined)
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
            bool valid = Locale::parseDateTime(expr,
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

    // find field names that are non-unique
    std::set<std::wstring> idx, non_unique_field_names;
    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        int i, col_count = tbl_it->structure->getColumnCount();

        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = tbl_it->structure->getColumnInfoByIdx(i);
            std::wstring fname = colinfo->getName();
            kl::makeLower(fname);

            if (idx.find(fname) == idx.end())
                idx.insert(fname);
                 else
                non_unique_field_names.insert(fname);
        }

    }



    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {   
        int i, col_count = tbl_it->structure->getColumnCount();

        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = tbl_it->structure->getColumnInfoByIdx(i);

            std::wstring alias = tbl_it->alias;
            std::wstring column = colinfo->getName();
            


            tango::IColumnInfoPtr newcol = m_validation_struct->createColumn();
            newcol->setName(alias + L"." + column);
            newcol->setType(colinfo->getType());
            newcol->setWidth(colinfo->getWidth());
            newcol->setScale(colinfo->getScale());
            newcol->setCalculated(colinfo->getCalculated());
            newcol->setExpression(colinfo->getExpression());

            tango::IColumnInfoPtr newcol1 = m_validation_struct->createColumn();
            newcol->copyTo(newcol1);
            newcol1->setName(L"[" + alias + L"]." + column);

            tango::IColumnInfoPtr newcol2 = m_validation_struct->createColumn();
            newcol->copyTo(newcol2);
            newcol1->setName(alias + L".[" + column + L"]");

            tango::IColumnInfoPtr newcol3 = m_validation_struct->createColumn();
            newcol->copyTo(newcol3);
            newcol1->setName(L"[" + alias + L"].[" + column + L"]");

            // if the field name is unique, then represent it also in our validation structure
            std::wstring fname = colinfo->getName();
            kl::makeLower(fname);
            if (non_unique_field_names.find(fname) == non_unique_field_names.end())
            {
                tango::IColumnInfoPtr newcol = m_validation_struct->createColumn();
                newcol->setName(colinfo->getName());
                newcol->setType(colinfo->getType());
                newcol->setWidth(colinfo->getWidth());
                newcol->setScale(colinfo->getScale());
                newcol->setCalculated(colinfo->getCalculated());
                newcol->setExpression(colinfo->getExpression());
            }

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
    kl::JsonNode root;


    // query info
    kl::JsonNode metadata = root["metadata"];
    metadata["type"] = wxT("application/vnd.kx.query");
    metadata["version"] = 1;
    metadata["description"] = wxT("");


    // input info
    root["input"].setArray();
    kl::JsonNode input = root["input"];

    std::vector<QueryBuilderSourceTable>::iterator tbl_it;
    int tbl_counter = 0;
    for (tbl_it = m_source_tables.begin();
         tbl_it != m_source_tables.end(); ++tbl_it)
    {
        kl::JsonNode table = input.appendElement();
        
        // write alias
        table["alias"] = towstr(tbl_it->alias);
        table["path"] = towstr(tbl_it->path);
        table["x"] = tbl_it->x;
        table["y"] = tbl_it->y;
        table["width"] = tbl_it->width;
        table["height"] = tbl_it->height;
        
        
        // if joins are specified, add the joins
        if (tbl_it->joins.size() > 0)
        {
            kl::JsonNode joins = table["joins"];
            joins.setArray();

            std::vector<QueryJoin>::iterator join_iter;
            for (join_iter = tbl_it->joins.begin();
                 join_iter != tbl_it->joins.end(); ++join_iter)
            {
                kl::JsonNode join = joins.appendElement();
                
                kl::JsonNode join_type = join["type"];
                switch (join_iter->join_type)
                {
                    default:
                    case QueryJoinNone:       join_type.setString(L"none"); break;
                    case QueryJoinInner:      join_type.setString(L"inner"); break;
                    case QueryJoinLeftOuter:  join_type.setString(L"left_outer"); break;
                    case QueryJoinRightOuter: join_type.setString(L"right_outer"); break;
                    case QueryJoinFullOuter:  join_type.setString(L"full_outer"); break;
                }

                kl::JsonNode join_table = join["table"];
                join_table.setString(towstr(join_iter->right_path));

                kl::JsonNode left_columns = join["left_columns"];
                left_columns.setString(towstr(join_iter->left_columns));
                
                kl::JsonNode right_columns = join["right_columns"];
                right_columns.setString(towstr(join_iter->right_columns));
            }
        }
    }


    // output info
    kl::JsonNode output = root["output"];

    output["table"].setString(towstr(m_output_path));
    output["distinct"].setBoolean(m_distinct);

    output["fields"].setArray();
    kl::JsonNode fields = output["fields"];

    std::vector<QueryBuilderParam>::iterator param_iter;
    int param_counter = 0;
    for (param_iter = m_params.begin();
         param_iter != m_params.end();
         ++param_iter)
    {
        kl::JsonNode field = fields.appendElement();
        
        field["output"].setBoolean(param_iter->output);
        field["input"].setString(towstr(param_iter->input_expr));
        field["name"].setString(towstr(param_iter->output_field));

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
            kl::JsonNode conditions = field["conditions"];
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
                kl::JsonNode condition = conditions.appendElement();
                condition.setString(towstr(*cond_it));
            }
        }
    }

    return JsonConfig::saveToDb(root, g_app->getDatabase(), path, wxT("application/vnd.kx.query"));
}

bool QueryTemplate::loadJson(const wxString& path)
{
    // try to load the JSON string
    kl::JsonNode root = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (!root.isOk())
        return false;


    // make sure we have the appropriate mime type and version
    kl::JsonNode metadata = root["metadata"];
    if (!metadata.isOk())
        return false;
    kl::JsonNode type = metadata["type"];
    if (!type.isOk())
        return false;
    kl::JsonNode version = metadata["version"];
    if (!version.isOk())
        return false;
    if (type.getString() != wxT("application/vnd.kx.query"))
        return false;
    if (version.getInteger() != 1)
        return false;


    m_source_tables.clear();
    m_params.clear();

    // input info
    kl::JsonNode input = root["input"];
    if (input.isOk())
    {
        size_t i, input_count = input.getChildCount();
        for (i =  0; i < input_count; ++i)
        {
            QueryBuilderSourceTable query_table;
            query_table.x = 0;
            query_table.y = 0;
            query_table.width = 160;
            query_table.height = 180;

            kl::JsonNode table = input[i];

            if (table.childExists("alias"))
                query_table.alias = table["alias"].getString();
            if (table.childExists("path"))
                query_table.path = table["path"].getString();
            if (table.childExists("x"))
                query_table.x = table["x"].getInteger();
            if (table.childExists("y"))
                query_table.y = table["y"].getInteger();
            if (table.childExists("width"))
                query_table.width = table["width"].getInteger();
            if (table.childExists("height"))
                query_table.height = table["height"].getInteger();

            // attempt to open the set and get it's structure
            tango::IFileInfoPtr finfo = g_app->getDatabase()->getFileInfo(towstr(query_table.path));
            if (finfo.isNull())
                continue;
            tango::IStructurePtr structure = g_app->getDatabase()->describeTable(towstr(query_table.path));
            if (structure.isNull())
                continue;

            query_table.structure = structure;

            // if joins are specified, load them
            kl::JsonNode joins = table["joins"];
            if (joins.isOk())
            {
                size_t j, join_count = joins.getChildCount();
                for (j = 0; j < join_count; ++j)
                {
                    QueryJoin query_join;
                    kl::JsonNode join = joins[j];

                    wxString type;                    
                    if (join.childExists("type"))
                        type = join["type"].getString();

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

                    if (join.childExists("table"))
                        query_join.right_path = join["table"].getString();
                    if (join.childExists("left_columns"))
                        query_join.left_columns = join["left_columns"].getString();
                    if (join.childExists("right_columns"))
                        query_join.right_columns = join["right_columns"].getString();

                    // set the join info
                    query_table.joins.push_back(query_join);
                }
            }
            
            // set the table info
            m_source_tables.push_back(query_table);
        }
    }


    // output info
    kl::JsonNode output = root["output"];
    if (output.isOk())
    {
        if (output.childExists("table"))
            m_output_path = output["table"].getString();
        if (output.childExists("distinct"))
            m_distinct = output["distinct"].getBoolean();
        
        kl::JsonNode fields = output["fields"];
        if (fields.isOk())
        {
            size_t f, field_count = fields.getChildCount();
            
            for (f = 0; f < field_count; ++f)
            {
                QueryBuilderParam query_param;

                kl::JsonNode field = fields[f];
                if (!field.isOk())
                    continue;

                if (field.childExists("output"))
                    query_param.output = field["output"].getBoolean();
                if (field.childExists("input"))
                    query_param.input_expr = field["input"].getString();
                if (field.childExists("name"))
                    query_param.output_field = field["name"].getString();

                wxString group_function;
                if (field.childExists("group_function"))
                    group_function = field["group_function"].getString();

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

                if (field.childExists("order"))
                    query_param.sort_order = field["order"].getInteger();

                kl::JsonNode conditions = field["conditions"];
                if (conditions.isOk())
                {
                    size_t c, condition_count = conditions.getChildCount();
                    for (c = 0; c < condition_count; ++c)
                    {
                        kl::JsonNode condition = conditions[c];
                        query_param.conditions.push_back(condition.getString());
                    }
                }

                m_params.push_back(query_param);
            }
        }
    }

    return true;
}

bool QueryTemplate::loadJsonFromNode(const wxString& path)
{
    kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (!node.isOk())
        return false;

    m_source_tables.clear();
    m_params.clear();


    kl::JsonNode root_node = node["root"];
    if (!root_node.isOk())
        return false;

    kl::JsonNode kpp_template_node = root_node["kpp_template"];
    if (!kpp_template_node.isOk())
        return false;

    kl::JsonNode template_type_node = kpp_template_node["type"];
    if (!template_type_node.isOk())
        return false;

    if (template_type_node.getString() != L"query")
        return false;

    kl::JsonNode data_node = kpp_template_node["data"];
    if (!data_node.isOk())
        return false;

    kl::JsonNode output_path_node = data_node["output_path"];
    if (!output_path_node.isOk())
        return false;
    m_output_path = towx(output_path_node.getString());

    kl::JsonNode distinct_node = data_node["distinct"];
    if (!distinct_node.isOk())
        return false;
    m_distinct = (distinct_node.getInteger() != 0 ? true : false);

    kl::JsonNode source_tables_node = data_node["source_tables"];
    if (!source_tables_node.isOk())
        return false;

    std::vector<kl::JsonNode> source_tables_node_children = source_tables_node.getChildren();
    std::vector<kl::JsonNode>::iterator it_table, it_table_end;
    it_table_end = source_tables_node_children.end();

    for (it_table = source_tables_node_children.begin(); it_table != it_table_end; ++it_table)
    {
        QueryBuilderSourceTable tbl;

        kl::JsonNode table_node = *it_table;

        kl::JsonNode alias_node = table_node["alias"];
        if (!alias_node.isOk())
            return false;
        tbl.alias = towx(alias_node.getString());

        kl::JsonNode path_node = table_node["path"];
        if (!path_node.isOk())
            return false;
        tbl.path = towx(path_node.getString());

        kl::JsonNode x_node = table_node["x"];
        if (!x_node.isOk())
            return false;
        tbl.x = x_node.getInteger();

        kl::JsonNode y_node = table_node["y"];
        if (!y_node.isOk())
            return false;
        tbl.y = y_node.getInteger();

        kl::JsonNode width_node = table_node["width"];
        if (!width_node.isOk())
            return false;
        tbl.width = width_node.getInteger();

        kl::JsonNode height_node = table_node["height"];
        if (!height_node.isOk())
            return false;
        tbl.height = height_node.getInteger();

        // attempt to open the set and get it's structure
        {
            tango::IStructurePtr structure = g_app->getDatabase()->describeTable(towstr(tbl.path));
            if (structure)
                tbl.structure = structure;
                 else
                continue;
        }

        kl::JsonNode joins_node = table_node["joins"];
        if (!joins_node.isOk())
            return false;

        std::vector<kl::JsonNode> joins_node_children = joins_node.getChildren();
        std::vector<kl::JsonNode>::iterator it_join, it_join_end;
        it_join_end = joins_node_children.end();

        for (it_join = joins_node_children.begin(); it_join != it_join_end; ++it_join)
        {
            QueryJoin join;
            
            kl::JsonNode join_node = *it_join;

            kl::JsonNode join_type_node = join_node["type"];
            if (!join_type_node.isOk())
                return false;
            wxString join_type = towx(join_type_node.getString());

            // join type
            if (!join_type.CmpNoCase(wxT("inner")))
                join.join_type = QueryJoinInner;
             else if (!join_type.CmpNoCase(wxT("left_outer")))
                join.join_type = QueryJoinLeftOuter;
             else if (!join_type.CmpNoCase(wxT("right_outer")))
                join.join_type = QueryJoinRightOuter;
             else if (!join_type.CmpNoCase(wxT("full_outer")))
                join.join_type = QueryJoinFullOuter;
             else
                join.join_type = QueryJoinNone;

            // right path
            kl::JsonNode right_path_node = join_node["right_path"];
            if (!right_path_node.isOk())
                return false;
            join.right_path = towx(right_path_node.getString());

            // right columns
            kl::JsonNode right_columns_node = join_node["right_columns"];
            if (!right_columns_node.isOk())
                return false;
            join.right_columns = towx(right_columns_node.getString());

            // left columns
            kl::JsonNode left_columns_node = join_node["left_columns"];
            if (!left_columns_node.isOk())
                return false;
            join.left_columns = towx(left_columns_node.getString());

            tbl.joins.push_back(join);
        }

        m_source_tables.push_back(tbl);
    }

    kl::JsonNode parameters_node = data_node["parameters"];
    if (!parameters_node.isOk())
        return false;

    std::vector<kl::JsonNode> parameters_node_children = parameters_node.getChildren();
    std::vector<kl::JsonNode>::iterator it, it_end;
    it_end = parameters_node_children.end();

    for (it = parameters_node_children.begin(); it != it_end; ++it)
    {
        QueryBuilderParam p;

        kl::JsonNode param_node = *it;

        kl::JsonNode output_node = param_node["output"];
        if (!output_node.isOk())
            return false;
        p.output = (output_node.getInteger() != 0 ? true : false);

        kl::JsonNode input_expr_node = param_node["input_expr"];
        if (!input_expr_node.isOk())
            return false;
        p.input_expr = towx(input_expr_node.getString());

        kl::JsonNode output_field_node = param_node["output_field"];
        if (!output_field_node.isOk())
            return false;
        p.output_field = towx(output_field_node.getString());

        kl::JsonNode group_func_node = param_node["group_func"];
        if (!group_func_node.isOk())
            return false;

        wxString group_func = towx(group_func_node.getString());

        if (!group_func.CmpNoCase(wxT("group_by")))
            p.group_func = QueryGroupFunction_GroupBy;
         else if (!group_func.CmpNoCase(wxT("first")))
            p.group_func = QueryGroupFunction_GroupBy;
         else if (!group_func.CmpNoCase(wxT("last")))
            p.group_func = QueryGroupFunction_GroupBy;
         else if (!group_func.CmpNoCase(wxT("min")))
            p.group_func = QueryGroupFunction_Min;
         else if (!group_func.CmpNoCase(wxT("max")))
            p.group_func = QueryGroupFunction_Max;
         else if (!group_func.CmpNoCase(wxT("sum")))
            p.group_func = QueryGroupFunction_Sum;
         else if (!group_func.CmpNoCase(wxT("avg")))
            p.group_func = QueryGroupFunction_Avg;
         else if (!group_func.CmpNoCase(wxT("count")))
            p.group_func = QueryGroupFunction_Count;
         else if (!group_func.CmpNoCase(wxT("stddev")))
            p.group_func = QueryGroupFunction_Stddev;
         else if (!group_func.CmpNoCase(wxT("variance")))
            p.group_func = QueryGroupFunction_Variance;
         else if (!group_func.CmpNoCase(wxT("group_id")))
            p.group_func = QueryGroupFunction_GroupID;
         else
            p.group_func = QueryGroupFunction_None;

         kl::JsonNode sort_order_node = param_node["sort_order"];
         if (!sort_order_node.isOk())
            return false;
         p.sort_order = sort_order_node.getInteger();

        wchar_t buf[255];
        int cond = 0;
        while (1)
        {
            swprintf(buf, 255, L"condition_%d", cond++);
            kl::JsonNode condition_node = param_node[buf];
            if (!condition_node.isOk())
                break;
            p.conditions.push_back(towx(condition_node.getString()));
        }

        m_params.push_back(p);
    }

    return true;
}

wxString QueryTemplate::quoteTable(const wxString& _str)
{
    wxString str = _str;

    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return str;

    str = towx(tango::quoteIdentifier(g_app->getDatabase(), towstr(str)));
    return str;
}

wxString QueryTemplate::quoteAlias(const wxString& _str)
{
    // note: quote alias is used for output fieldnames

    wxString str = _str;
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return str;

    if (str.Freq('.') == 0)
    {
        str = towx(tango::quoteIdentifier(g_app->getDatabase(), towstr(str)));
    }
     else
    {
        wxString alias = str.BeforeLast('.');
        wxString field = str.AfterLast('.');

        alias = towx(tango::quoteIdentifier(db, towstr(alias)));
        field = towx(tango::quoteIdentifier(db, towstr(field)));
        str = alias + wxT(".") + field;
    }

    return str;
}

wxString QueryTemplate::quoteField(const wxString& _str)
{
    wxString str = _str;
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return str;

    // create a unique list of column names with both a qualified and 
    // unqualified variant; we'll use these to determine whether or 
    // not an input is an expression
    std::map<wxString,int> fieldlist;
    
    std::vector<QueryBuilderSourceTable>::iterator it, it_end;
    it_end = m_source_tables.end();
    for (it = m_source_tables.begin(); it != it_end; ++it)
    {
        tango::IStructurePtr structure = it->structure;
        if (structure.isNull())
            continue;
        
        int col_count = structure->getColumnCount();
        for (int idx = 0; idx < col_count; ++idx)
        {
            wxString table_name = it->alias;
            wxString col_name = towx(structure->getColumnName(idx));
            
            table_name.MakeUpper();
            col_name.MakeUpper();

            fieldlist[table_name + wxT(".") + col_name] = 1;
            fieldlist[col_name] = 1;
        }
    }   

    // if the string isn't in the list of fields, don't try to quote it
    // because it's either an expression, or something we don't know what
    // to do with
    wxString s = _str.Upper();
    std::map<wxString,int>::iterator it_fields = fieldlist.find(s);
    if (it_fields == fieldlist.end())
        return str;


    if (str.Freq('.') == 0)
    {
        str = towx(tango::quoteIdentifier(g_app->getDatabase(), towstr(str)));
    }
     else
    {
        wxString alias = str.BeforeLast('.');
        wxString field = str.AfterLast('.');

        alias = towx(tango::quoteIdentifier(db, towstr(alias)));
        field = towx(tango::quoteIdentifier(db, towstr(field)));
        str = alias + wxT(".") + field;
    }

    return str;
}

wxString QueryTemplate::formatTableName(const wxString& _table_path)
{
    // this function formats a table name to remove the leading slash
    // if the table is in the root.  Thus "/mytbl" becomes "mytbl",
    // and "/myfolder/mytbl" remains the same

    wxString table_path = quoteTable(_table_path);
    if (table_path.Freq('/') == 1 && table_path.GetChar(0) == '/')
    {
        wxString result = table_path;
        result.Remove(0,1);
        return result;
    }
    
    return table_path;
}

wxString QueryTemplate::makeQueryGroupFunction(const wxString& _input_expr, int group_func)
{
    wxString expr;
    wxString input_expr = quoteField(_input_expr);

    switch (group_func)
    {
        default:
        case QueryGroupFunction_GroupBy:
        case QueryGroupFunction_First:
        case QueryGroupFunction_None:
            expr = input_expr;
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
