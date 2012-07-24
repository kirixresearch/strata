/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2011-03-28
 *
 */


#include "appmain.h"

#include "reportengine.h"
#include "reportlayout.h"
#include "scripthost.h"
#include "scriptreport.h"


Report::Report()
{
}

Report::~Report()
{
}

void Report::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_canvas = kcanvas::Canvas::create();
    m_design_component = CompReportDesign::create();

    CompReportDesign* design_component_ptr = getDesignComponentRaw();
    design_component_ptr->setCanvas(m_canvas);

    design_component_ptr->addSection(PROP_REPORT_HEADER, PROP_REPORT_HEADER, 0, true);
    design_component_ptr->addSection(PROP_REPORT_PAGE_HEADER, PROP_REPORT_PAGE_HEADER, 0, true);
    design_component_ptr->addSection(PROP_REPORT_DETAIL, PROP_REPORT_DETAIL, 0, true);
    design_component_ptr->addSection(PROP_REPORT_PAGE_FOOTER, PROP_REPORT_PAGE_FOOTER, 0, true);
    design_component_ptr->addSection(PROP_REPORT_FOOTER, PROP_REPORT_FOOTER, 0, true);

    // set the default column to zero
    design_component_ptr->setColumnCount(0);
    
    // set the active section to the detail
    design_component_ptr->setActiveSectionByName(PROP_REPORT_DETAIL);
}

void Report::load(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    wxString path = env->getParam(0)->getString();
    bool result = getDesignComponentRaw()->load(path);
    retval->setBoolean(result);
}

void Report::save(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    wxString path = env->getParam(0)->getString();
    bool result = getDesignComponentRaw()->save(path);
    retval->setBoolean(result);
}

void Report::saveAsPdf(kscript::ExprEnv* env, kscript::Value* retval)
{
    // make sure we have at least one parameter, which is the output path
    int param_count = env->getParamCount();
    if (param_count < 1)
    {
        retval->setBoolean(false);
        return;
    }

    // get the output path
    wxString output_path = env->getParam(0)->getString();


    // refresh the design layout and get the design parameters
    CompReportDesign* design_component_ptr = getDesignComponentRaw();
    design_component_ptr->setCanvas(m_canvas);
    design_component_ptr->layout();

    int page_width, page_height;
    design_component_ptr->getPageSize(&page_width, &page_height);

    int margin_left, margin_right, margin_top, margin_bottom;
    design_component_ptr->getPageMargins(&margin_left, &margin_right, &margin_top, &margin_bottom);

    double width = ((double)page_width)/kcanvas::CANVAS_MODEL_DPI;
    double height = ((double)page_height)/kcanvas::CANVAS_MODEL_DPI;

    std::vector<ReportSection> sections;
    design_component_ptr->getSections(sections);
    
    wxString data_source = design_component_ptr->getDataSource();
    wxString data_filter = design_component_ptr->getDataFilter();


    // configure the output
    ReportLayoutEngine layout_engine;
    layout_engine.init(sections,
                       data_source,
                       data_filter,
                       page_width,
                       page_height,
                       margin_left,
                       margin_right,
                       margin_top,
                       margin_bottom);


    // set the page range
    ReportPrintInfo info;
    info.setQuality(600);
    info.setMinPage(1);
    info.setMaxPage(1);
    info.setFromPage(1);
    info.setToPage(1);
    info.setAllPages(true);


    // create a new report output pdf; it will destroy 
    // itself when done; block until creation is complete
    ReportOutputPdf* pdf = new ReportOutputPdf(layout_engine, info, output_path, true);
    pdf->create();

    retval->setBoolean(true);
}

void Report::setDataSource(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;

    wxString path = env->getParam(0)->getString();
    getDesignComponentRaw()->setDataSource(path);
}

void Report::setPageSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // two parameters: width and height
    if (env->getParamCount() < 2)
        return;

    int width = env->getParam(0)->getInteger();
    int height = env->getParam(1)->getInteger();
    getDesignComponentRaw()->setPageSize(width, height);
}

void Report::setPageMargins(kscript::ExprEnv* env, kscript::Value* retval)
{
    // four parameters: left, right, top, bottom margin
    if (env->getParamCount() < 4)
        return;

    int left_margin = env->getParam(0)->getInteger();
    int right_margin = env->getParam(1)->getInteger();
    int top_margin = env->getParam(2)->getInteger();
    int bottom_margin = env->getParam(3)->getInteger();

    getDesignComponentRaw()->setPageMargins(left_margin, right_margin, top_margin, bottom_margin);
}

void Report::setRowCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    int count = env->getParam(0)->getInteger();
    bool result = getDesignComponentRaw()->setRowCount(count);
    retval->setBoolean(result);
}

void Report::getRowCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    int count = getDesignComponentRaw()->getRowCount();
    retval->setInteger(count);
}

void Report::setColumnCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    int count = env->getParam(0)->getInteger();
    bool result = getDesignComponentRaw()->setColumnCount(count);
    retval->setBoolean(result);
}

void Report::getColumnCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    int count = getDesignComponentRaw()->getColumnCount();
    retval->setInteger(count);
}

void Report::insertRow(kscript::ExprEnv* env, kscript::Value* retval)
{
    // first parameter is row index; second (optional) is how many to insert
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    int idx = env->getParam(0)->getInteger();
    
    int count = 1;
    if (env->getParamCount() >= 2)
        count = env->getParam(1)->getInteger();

    bool result = getDesignComponentRaw()->insertRow(idx, count);
    retval->setBoolean(result);
}

void Report::removeRow(kscript::ExprEnv* env, kscript::Value* retval)
{
    // first parameter is row index; second (optional) is how many to remove
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    int idx = env->getParam(0)->getInteger();
    
    int count = 1;
    if (env->getParamCount() >= 2)
        count = env->getParam(1)->getInteger();

    bool result = getDesignComponentRaw()->removeRow(idx, count);
    retval->setBoolean(result);
}

void Report::insertColumn(kscript::ExprEnv* env, kscript::Value* retval)
{
    // first parameter is column index; second (optional) is how many to insert
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    int idx = env->getParam(0)->getInteger();

    int count = 1;
    if (env->getParamCount() >= 2)
        count = env->getParam(1)->getInteger();

    bool result = getDesignComponentRaw()->insertColumn(idx, count);
    retval->setBoolean(result);
}

void Report::removeColumn(kscript::ExprEnv* env, kscript::Value* retval)
{
    // first parameter is column index; second (optional) is how many to remove
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    int idx = env->getParam(0)->getInteger();
    
    int count = 1;
    if (env->getParamCount() >= 2)
        count = env->getParam(1)->getInteger();
 
    bool result = getDesignComponentRaw()->removeColumn(idx, count);
    retval->setBoolean(result);
}

void Report::setRowSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // first parameter is row index; second is the size
    if (env->getParamCount() < 2)
    {
        retval->setBoolean(false);
        return;
    }

    int idx = env->getParam(0)->getInteger();
    int size = env->getParam(1)->getInteger();
    bool result = getDesignComponentRaw()->setRowSize(idx, size);
    retval->setBoolean(result);
}

void Report::getRowSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // first parameter is row index; use non-existent column as default
    // and return what object API returns
    int idx = -1;
    if (env->getParamCount() > 0)
        int idx = env->getParam(0)->getInteger();

    int size = getDesignComponentRaw()->getRowSize(idx);
    retval->setInteger(size);
}

void Report::setColumnSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // first parameter is column index; second is the size
    if (env->getParamCount() < 2)
    {
        retval->setBoolean(false);
        return;
    }

    int idx = env->getParam(0)->getInteger();
    int size = env->getParam(1)->getInteger();
    bool result = getDesignComponentRaw()->setColumnSize(idx, size);
    retval->setBoolean(result);
}

void Report::getColumnSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // first parameter is column index; use non-existent column as default
    // and return what object API returns
    int idx = -1;
    if (env->getParamCount() > 0)
        int idx = env->getParam(0)->getInteger();

    int size = getDesignComponentRaw()->getColumnSize(idx);
    retval->setInteger(size);
}

void Report::insertSection(kscript::ExprEnv* env, kscript::Value* retval)
{
    bool insertGroup(const wxString& name, 
                     const wxString& group_field,
                     bool sort_descending = false,
                     bool visible = false);

    // first parameter is the group name, second is the field to group on,
    // third is the sort order ("asc" or "desc" with "asc" as default)
    if (env->getParamCount() < 2)
    {
        retval->setBoolean(false);
        return;
    }

    wxString name = env->getParam(0)->getString();
    wxString group = env->getParam(1)->getString();
    
    bool sort_descending = false;
    if (env->getParamCount() >= 3)
    {
        wxString order = env->getParam(2)->getString();
        if (order.CmpNoCase(wxT("DESC")))
            sort_descending = true;
    }

    bool result = getDesignComponentRaw()->insertGroup(name, group, sort_descending, true);
    retval->setBoolean(result);
}

void Report::setSection(kscript::ExprEnv* env, kscript::Value* retval)
{
    // one parameter: name of the section to set as the active section
    // currently:
    //      report header:  "report.header"
    //      report footer:  "report.footer"
    //      page header:    "report.page.header"
    //      page footer:    "report.page.footer"
    //      report detail:  "report.detail"

    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    wxString name = env->getParam(0)->getString();
    bool result = getDesignComponentRaw()->setActiveSectionByName(name);
    retval->setBoolean(result);
}

void Report::setCellProperty(kscript::ExprEnv* env, kscript::Value* retval)
{
    // four parameters: starting row, starting column, property string and value
    if (env->getParamCount() < 4)
    {
        retval->setBoolean(false);
        return;
    }
    
    int row = env->getParam(0)->getInteger();
    int col = env->getParam(1)->getInteger();
    wxString prop_name = env->getParam(2)->getString();

    kcanvas::CellRange range(row, col);
    kcanvas::PropertyValue prop_value;
    
    // if the property is a font size, get the value as an integer;
    // if the property is an alignment, get the value as a string;
    // if the property isn't either, return false
    if (prop_name == kcanvas::PROP_FONT_SIZE)
    {
        int value = env->getParam(3)->getInteger();
        prop_value.setInteger(value);
    }
     else if (prop_name == kcanvas::PROP_TEXT_HALIGN)
    {
        wxString value = env->getParam(3)->getString();
        prop_value.setString(value);
    }
     else
    {
        retval->setBoolean(false);
        return;        
    }

    getDesignComponentRaw()->setCellProperty(range, prop_name, prop_value);
    retval->setBoolean(true); 
}

void Report::setCellValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    // three parameters: row, column, and value
    if (env->getParamCount() < 3)
    {
        retval->setBoolean(false);
        return;
    }

    int row, col;
    row = env->getParam(0)->getInteger();
    col = env->getParam(1)->getInteger();
    wxString value = env->getParam(2)->getString();

    bool result = getDesignComponentRaw()->setCellValue(row, col, value);
    retval->setBoolean(result);
}

void Report::mergeCells(kscript::ExprEnv* env, kscript::Value* retval)
{
    // four parameters: starting row, starting column, ending row, ending column
    if (env->getParamCount() < 4)
    {
        retval->setBoolean(false);
        return;
    }
    
    int row1 = env->getParam(0)->getInteger();
    int col1 = env->getParam(1)->getInteger();
    int row2 = env->getParam(2)->getInteger();
    int col2 = env->getParam(3)->getInteger();

    kcanvas::CellRange range(row1, col1, row2, col2);
    getDesignComponentRaw()->mergeCells(range);
    retval->setBoolean(true);              
}

void Report::unmergeCells(kscript::ExprEnv* env, kscript::Value* retval)
{
    // four parameters: starting row, starting column, ending row, ending column
    if (env->getParamCount() < 4)
    {
        retval->setBoolean(false);
        return;
    }
    
    int row1 = env->getParam(0)->getInteger();
    int col1 = env->getParam(1)->getInteger();
    int row2 = env->getParam(2)->getInteger();
    int col2 = env->getParam(3)->getInteger();

    kcanvas::CellRange range(row1, col1, row2, col2);
    getDesignComponentRaw()->unmergeCells(range);
    retval->setBoolean(true);              
}

void Report::getTextSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO:
}

CompReportDesign* Report::getDesignComponentRaw()
{
    return static_cast<CompReportDesign*>(m_design_component.p);
}

