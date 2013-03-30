/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-08-31
 *
 */


#include "appmain.h"
#include "tabledoc.h"
#include "tabledoc_private.h"
#include "appcontroller.h"
#include "scripthost.h"



void TableDoc::setFilter(const wxString& filter)
{
    if (filter.IsEmpty())
        removeFilter();


    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.query-job");

    // configure the job parameters
    kl::JsonNode params;
    params = createSortFilterJobParams(m_browse_set->getObjectPath(), towstr(filter), towstr(m_sort_order));


    // set the job parameters and start the job
    wxString title = wxString::Format(_("Filtering '%s'"),
                                      getCaption().c_str());

    job->getJobInfo()->setTitle(towstr(title));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(this, &TableDoc::onFilterJobFinished);
    g_app->getJobQueue()->addJob(job, jobStateRunning);


    // if the job is a quick filter job, track the ID
    if (m_quick_filter_jobid == quickFilterPending)
        m_quick_filter_jobid = job->getJobInfo()->getJobId();
}

void TableDoc::setQuickFilter(const wxString& val)
{
    bool match_case, whole_cell;
    getAppPrefsFindMatchCase(&match_case);
    getAppPrefsFindMatchWholeWord(&whole_cell);

    wxString filter_expr = getFindExprFromValue(val, match_case, whole_cell);
    setFilter(filter_expr);
}

wxString TableDoc::getFilter()
{
    return m_filter;
}

void TableDoc::removeFilter()
{
    if (m_grid->isEditing())
    {
        m_grid->endEdit(true);
    }

    m_filter = wxT("");
    m_sort_order = wxT("");

    setBrowseSet(m_set, xcm::null);

    updateStatusBar();
    g_app->getAppController()->updateQuickFilterToolBarItem();

    setCaption(wxEmptyString, wxEmptyString);
}

void TableDoc::setSortOrder(const wxString& expr)
{
    if (m_grid->isEditing())
        m_grid->endEdit(true);


    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.query-job");


    // if the database can't handle createIterator() with a different
    // sort order on an existing set (e.g. must requery), do that here.
    wxString db_driver = getDbDriver();
    if (db_driver != wxT("xdnative") && db_driver != wxT("xdfs"))
    {
        // configure the job parameters
        kl::JsonNode params;
        params = createSortFilterJobParams(towstr(m_path), 
                                           towstr(getFilter()), 
                                           towstr(expr));

        // set the job parameters and start the job
        wxString title = wxString::Format(_("Sorting '%s'"),
                                          getCaption().c_str());

        job->getJobInfo()->setTitle(towstr(title));
        job->setParameters(params.toString());

        job->sigJobFinished().connect(this, &TableDoc::onSortJobFinished);
        g_app->getJobQueue()->addJob(job, jobStateRunning);

        return;
    }

    if (expr.IsEmpty())
    {
        // set default order
        m_sort_order = wxT("");
        tango::IIteratorPtr iter;
        iter = g_app->getDatabase()->createIterator(m_browse_set->getObjectPath(), L"", L"", NULL);
        setIterator(iter);
        return;
    }


    // configure the job parameters
    kl::JsonNode params;
    params = createSortFilterJobParams(m_browse_set->getObjectPath(), 
                                       L"",
                                       towstr(expr));

    // set the job parameters and start the job
    wxString title = wxString::Format(_("Sorting '%s'"),
                                        getCaption().c_str());

    job->getJobInfo()->setTitle(towstr(title));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(this, &TableDoc::onSortJobFinished);
    g_app->getJobQueue()->addJob(job, jobStateRunning);
}

wxString TableDoc::getSortOrder()
{
    return m_sort_order;
}

void TableDoc::setGroupBreak(const wxString& _expr)
{
    wxString expr = _expr;
    m_group_break = expr;
    if (expr.Length() > 0)
        expr.Prepend(wxT("KEY:"));

    ITangoGridModelPtr model = m_grid->getModel();
    if (model.isNull())
        return;

    model->setGroupBreakExpr(expr);
    m_grid->refresh(kcl::Grid::refreshAll);
}

wxString TableDoc::getGroupBreak()
{
    return m_group_break;
}

void TableDoc::createNewMark(const wxString& expr)
{
    // create mark
    ITableDocMarkPtr mark = m_model->createMarkObject();
    ITableDocMarkEnumPtr markvec = m_model->getMarkEnum();
    int mark_count = markvec->size();
    mark->setBackgroundColor(m_model->getNextMarkColor());
    mark->setDescription(towstr(wxString::Format(_("Mark #%d"), mark_count+1)));
    mark->setExpression(towstr(expr));

    m_model->writeObject(mark);
    m_grid->refresh(kcl::Grid::refreshAll);

    // let other windows know that a mark was created
    // and make sure the marks panel is shown
    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_MARK_CREATED));
}




// -- TableDoc scripting elements --

bool TableDoc::getScriptMember(const std::wstring& member,
                               kscript::Value* retval)
{
    kscript::ExprBindFunc func = NULL;
    

         if (member == L"open")           func = &TableDoc::scriptfuncOpen;
    else if (member == L"setFilter")      func = &TableDoc::scriptfuncSetFilter;
    else if (member == L"setQuickFilter") func = &TableDoc::scriptfuncSetQuickFilter;
    else if (member == L"getFilter")      func = &TableDoc::scriptfuncGetFilter;
    else if (member == L"setSortOrder")   func = &TableDoc::scriptfuncSetSortOrder;
    else if (member == L"getSortOrder")   func = &TableDoc::scriptfuncGetSortOrder;
    else if (member == L"setGroupBreak")  func = &TableDoc::scriptfuncSetGroupBreak;
    else if (member == L"getGroupBreak")  func = &TableDoc::scriptfuncGetGroupBreak;


    if (func)
    {
        gui_marshal.getRerouteInfo(func, this, retval);
        return true;
    }
    
    return false;
}

// static
void TableDoc::scriptfuncOpen(kscript::ExprEnv* env,
                              void* param,
                              kscript::Value* retval)
{
    TableDoc* pThis = (TableDoc*)param;
    
    tango::ISetPtr set = g_app->getDatabase()->openSet(env->getParam(0)->getString());
    if (set)
    {
        pThis->open(set, xcm::null);
        retval->setBoolean(true);
    }
     else
    {
        retval->setBoolean(false);
    }    
}

// static
void TableDoc::scriptfuncSetFilter(kscript::ExprEnv* env,
                                   void* param,
                                   kscript::Value* retval)
{
    TableDoc* pThis = (TableDoc*)param;
    wxString val;
    if (env->getParamCount() > 0)
        val = env->getParam(0)->getString();
    pThis->setFilter(val);
}

// static
void TableDoc::scriptfuncSetQuickFilter(kscript::ExprEnv* env,
                                   void* param,
                                   kscript::Value* retval)
{
    TableDoc* pThis = (TableDoc*)param;
    wxString val;
    if (env->getParamCount() > 0)
        val = env->getParam(0)->getString();
    pThis->setQuickFilter(val);
}

// static
void TableDoc::scriptfuncGetFilter(kscript::ExprEnv* env,
                                   void* param,
                                   kscript::Value* retval)
{
    TableDoc* pThis = (TableDoc*)param;
    retval->setString(towstr(pThis->getFilter()));
}

// static
void TableDoc::scriptfuncSetSortOrder(kscript::ExprEnv* env,
                                      void* param,
                                      kscript::Value* retval)
{
    TableDoc* pThis = (TableDoc*)param;
    wxString val;
    if (env->getParamCount() > 0)
        val = env->getParam(0)->getString();
    pThis->setSortOrder(val);
}

// static
void TableDoc::scriptfuncGetSortOrder(kscript::ExprEnv* env,
                                      void* param,
                                      kscript::Value* retval)
{
    TableDoc* pThis = (TableDoc*)param;
    retval->setString(towstr(pThis->getSortOrder()));
}

// static
void TableDoc::scriptfuncSetGroupBreak(kscript::ExprEnv* env,
                                       void* param,
                                       kscript::Value* retval)
{
    TableDoc* pThis = (TableDoc*)param;
    wxString val;
    if (env->getParamCount() > 0)
        val = env->getParam(0)->getString();
    pThis->setGroupBreak(val);
}

// static
void TableDoc::scriptfuncGetGroupBreak(kscript::ExprEnv* env,
                                       void* param,
                                       kscript::Value* retval)
{
    TableDoc* pThis = (TableDoc*)param;
    retval->setString(towstr(pThis->getGroupBreak()));
}

