/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2008-03-13
 *
 */


#include "appmain.h"
#include "appprint.h"
#include "reportlayout.h"

#include "../kcanvas/util.h"

#include <wx/paper.h>


// table header size
const int TABLE_COLUMN_HEADER_SIZE      = (int)(kcanvas::CANVAS_MODEL_DPI*0.1875);
const int TABLE_ROW_HEADER_SIZE         = (int)(kcanvas::CANVAS_MODEL_DPI*0.125);


static double in2mm(double in) { return (in*25.4); }
static double mm2in(double mm) { return (mm/25.4); }


CompReportLayout::CompReportLayout()
{
    // signal handler for when the layout engine is ready
    m_layout_engine.sigLayoutUpdated().connect(this, &CompReportLayout::onReportLayoutEngineUpdated);

    // event handlers
    addEventHandler(kcanvas::EVENT_LAYOUT, &CompReportLayout::onLayout);
    addEventHandler(kcanvas::EVENT_RENDER, &CompReportLayout::onPreRender, true); // capturing phase
    addEventHandler(kcanvas::EVENT_RENDER, &CompReportLayout::onRender);
    addEventHandler(kcanvas::EVENT_RENDER, &CompReportLayout::onRenderHeader, true); // capturing phase
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN, &CompReportLayout::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZE_COLUMN, &CompReportLayout::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_SELECT_CELL, &CompReportLayout::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_SELECT_CELL, &CompReportLayout::onTableEvent);

    // flag for rendering the row headers the first time
    m_first_row_header_render = false;
}

CompReportLayout::~CompReportLayout()
{
}

kcanvas::IComponentPtr CompReportLayout::create()
{
    // create a new report layout component
    return static_cast<kcanvas::IComponent*>(new CompReportLayout);
}

kcanvas::IComponentPtr CompReportLayout::clone()
{
    CompReportLayout* comp = new CompReportLayout;
    comp->copy(this);

    return static_cast<kcanvas::IComponentPtr>(comp);
}

// fix for VC6 C2352 compiler bug
typedef kcanvas::Component kcanvasComponent;
void CompReportLayout::copy(kcanvas::IComponentPtr component)
{
    // copy the base component properties
    kcanvasComponent::copy(component);

    if (component.isNull())
        return;

    CompReportLayout* c;
    c = static_cast<CompReportLayout*>(component.p);

    m_layout_engine = c->m_layout_engine;
}

void CompReportLayout::setReportLayoutEngine(const ReportLayoutEngine& engine)
{
    m_layout_engine = engine;
}

ReportLayoutEngine& CompReportLayout::getReportLayoutEngineRef()
{
    return m_layout_engine;
}

int CompReportLayout::getPageCount()
{
    return m_cache_pages.size();
}

    // function for page location
bool CompReportLayout::getPagePosByIdx(int idx, int* x, int* y)
{
    // note: this function returns the page position and size
    // based on a 1-based page index (i.e., page 1 = 1, not zero);
    // returns true if the page idx exists, and false otherwise

    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return false;

    // if the pages are outside the bounds, we're done
    if (idx < 1 || idx > getPageCount())
        return false;

    wxRect page_rect = m_cache_pages[idx-1].m_page_rect;
    *x = page_rect.x;
    *y = page_rect.y;

    return true;
}

bool CompReportLayout::getPageIdxByPos(int x, int y, int* idx)
{
    // note: this function returns the 1-based page index closest 
    // to the (x,y) position; returns true if the an index is
    // returned and false otherwise

    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return false;

    if (getPageCount() <= 0)
        return false;

    // default to first page
    *idx = 1;

    // iterate through the pages and find the one closest
    // to the page the position
    std::vector<CompReportLayoutPage>::iterator it, it_end;
    it_end = m_cache_pages.end();

    int distance = INT_MAX;    
    for (it = m_cache_pages.begin(); it != it_end; ++it)
    {
        // calculate the page index and position
        int page_x, page_y;
        int current_idx = it - m_cache_pages.begin() + 1;   // convert to a one-based index
        getPagePosByIdx(current_idx, &page_x, &page_y);

        // calculate a simple distance metric between the input
        // point and the page origin (no need to do a precise
        // sqrt-of-the-sum-of-the-squares calculation)
        int new_distance = ::abs(x - page_x) + ::abs(y - page_y);
        
        // if the new distance from the point in question is greater
        // than the distance to the last page, we've gone too far;
        // the last page we were on is the one closest to the
        // point in question
        if (new_distance > distance)
            break;

        distance = new_distance;
        *idx = current_idx;
    }

    return true;
}

void CompReportLayout::layout()
{
    // if the canvas is null, use the default canvas to which
    // the component belongs to layout the component
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // clear all the child pages from this component
    removeAll();


    // ready the layout; if the layout is ready, the pages
    // will be available; otherwise, it will trigger a
    // job that will eventually cause this function to be
    // called again so the layout will be properly refreshed
    m_layout_engine.execute();


    // calculate a default page seperator size; the default
    // measurement is in pixels so that the page spacing
    // doesn't change when zooming the canvas
    int page_spacing = canvas->dtom_x(PROP_REPORT_PAGE_SPACING);

    // get the page rectangels
    std::vector<wxRect> page_sizes;
    m_layout_engine.getPageSizes(page_sizes);

    std::vector<wxRect>::iterator it, it_end;
    it_end = page_sizes.end();
    
    // set the container origin to accomodate table headers
    int container_offset_x = TABLE_ROW_HEADER_SIZE;
    int container_offset_y = TABLE_COLUMN_HEADER_SIZE;    
    setOrigin(container_offset_x, container_offset_y);
    
    // set the container width to the widest page
    int max_page_width = 0;
    for (it = page_sizes.begin(); it != it_end; ++it)
    {
        if (it->width > max_page_width)
            max_page_width = it->width;
    }
    setWidth(2*page_spacing + max_page_width + container_offset_x);

    // note: container height set at the end when we have
    // the rest of the page layout info


    // if the width of the layout container is less than the width of the
    // view, shift the layout container so that it's centered in the view    

    int container_width = getWidth();
    int view_width = canvas->getViewWidth();
    if (container_width < view_width)
    {
        container_offset_x = (view_width - container_width)/2 + container_offset_x;
        setOrigin(container_offset_x, container_offset_y);
    }

    // set the relative view origin
    wxRect view_rect;
    canvas->getViewOrigin(&view_rect.x, &view_rect.y);
    canvas->getViewSize(&view_rect.width, &view_rect.height);

    int rel_view_origin_x = -container_offset_x + view_rect.x;
    int rel_view_origin_y = -container_offset_y + view_rect.y;


    // calculate the pages that will fit in the view by looking
    // through the page info
    CompReportLayoutPage first_page;
    m_cache_pages.clear();    

    int y_offset = page_spacing;
    for (it = page_sizes.begin(); it != it_end; ++it)
    {
        CompReportLayoutPage page_info;

        // calculate the page index and position
        int page_x = page_spacing;
        int page_y = y_offset;
        y_offset += it->height + page_spacing; // update the y_offset for next time around
        
        // set the page rectangle        
        page_info.m_page_rect.x = page_x + container_offset_x;
        page_info.m_page_rect.y = page_y;
        page_info.m_page_rect.width = it->width;
        page_info.m_page_rect.height = it->height;

        // if the page rectangle doesn't intersect the view, reset
        // any references to previously created pages so that we
        // don't hog memory with hundreds of saved pages and their
        // related tables; if the page intersects the view, create
        // a new page
        if (!view_rect.Intersects(page_info.m_page_rect))
        {
            page_info.m_page = xcm::null;
            page_info.m_row_header = xcm::null;
        }
        else
        {
            // add the page
            int idx = it - page_sizes.begin();
            kcanvas::IComponentPtr page;            
            page = m_layout_engine.getPageByIdx(idx);
            page_info.m_page = page;

            page->setOrigin(page_x, page_y);
            add(page);
            
            // set the first page in view so we can add an
            // associated column header
            if (first_page.m_page.isNull())
                first_page = page_info;
                
            // add the table row header
            kcanvas::IComponentPtr row_header;
            row_header = updateTableRowHeader(page_info, rel_view_origin_x, 0);
            add(row_header);
        }
        
        // save the page in the cache
        m_cache_pages.push_back(page_info);
    }


    // set the container height
    setHeight(y_offset + page_spacing + container_offset_y);


    // add the table column header
    kcanvas::IComponentPtr column_header;
    column_header = updateTableColumnHeader(first_page, 
                                            -container_offset_x, 
                                            rel_view_origin_y);
    add(column_header);
}

bool CompReportLayout::refresh()
{
    m_layout_engine.reset();
    layout();
    return true;
}

void CompReportLayout::clear()
{
    m_layout_engine.reset();
}

void CompReportLayout::onLayout(kcanvas::IEventPtr evt)
{
    // do nothing; call layout() explicitly before the
    // canvas layout event start to initialize the
    // component
}

void CompReportLayout::onPreRender(kcanvas::IEventPtr evt)
{
    // if the table has the focus, turn on the cursor;
    // otherwise, turn it off
    kcanvas::ICompTablePtr table = evt->getTarget();
    if (!table.isNull() && !isColumnHeader(table) && !isRowHeader(table))
    {
        if (getCanvas()->hasFocus(table))
            table->setPreference(kcanvas::CompTable::prefCursor, true);
        else
            table->setPreference(kcanvas::CompTable::prefCursor, false);
    }
}

void CompReportLayout::onRender(kcanvas::IEventPtr evt)
{
    // this component is rendered before it's children; use
    // this as a chance to reset the row header render flag
    if (evt->getPhase() == kcanvas::EVENT_AT_TARGET)
        m_first_row_header_render = true;
}

void CompReportLayout::onRenderHeader(kcanvas::IEventPtr evt)
{
    // pre-render for drawing table header backgrounds

    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    kcanvas::IComponentPtr comp = evt->getTarget();
    if (comp.isNull())
        return;

    // render the side
    kcanvas::Color base_color = kcanvas::tokccolor(kcl::getBaseColor());
    canvas->setPen(base_color);
    canvas->setBrush(base_color);

    kcanvas::Color start_color = base_color;
    kcanvas::Color end_color = kcanvas::stepColor(kcanvas::COLOR_WHITE, start_color, 70);

    // calculate the view rectangle in view coordinates
    wxRect row_header_background(0, 0,
                                 TABLE_ROW_HEADER_SIZE, canvas->getViewHeight());

    wxRect column_header_background(0, 0, 
                                    canvas->getViewWidth(), TABLE_COLUMN_HEADER_SIZE);

    // set the draw origin to view coordinates
    int old_draw_origin_x, old_draw_origin_y;
    canvas->getDrawOrigin(&old_draw_origin_x, &old_draw_origin_y);
    
    int view_x, view_y;
    canvas->getViewOrigin(&view_x, &view_y);
    canvas->setDrawOrigin(view_x, view_y);

    // m_first_render_row_header is so that we only draw it once; we don't want
    // multiple header backgrounds to draw over previously rendererd headers
    if (isRowHeader(comp) && m_first_row_header_render)
    {
        canvas->drawGradientFill(row_header_background, start_color, end_color, wxWEST);
        m_first_row_header_render = false;
    }

    if (isColumnHeader(comp))
        canvas->drawGradientFill(column_header_background, start_color, end_color, wxNORTH);

    // restore the draw origin
    canvas->setDrawOrigin(old_draw_origin_x, old_draw_origin_y);
}

void CompReportLayout::onTableEvent(kcanvas::IEventPtr evt)
{
    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // if we don't have a notify event, we're done
    kcanvas::INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    // if the event target isn't a component, we're done
    kcanvas::IComponentPtr comp = evt->getTarget();
    if (comp.isNull())
        return;

    if (!isRowHeader(comp) && !isColumnHeader(comp))
        return;

    wxString name = evt->getName();

    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN)
    {
        // save the action header table
        m_action_table = comp;
    }

    if (name == kcanvas::EVENT_TABLE_RESIZE_COLUMN)
    {
        // reset the action header table
        m_action_table = xcm::null;
    }

    if (name == kcanvas::EVENT_TABLE_PRE_SELECT_CELL)
    {
        // save the action table
        m_action_table = comp;
    }

    if (name == kcanvas::EVENT_TABLE_SELECT_CELL)
    {
        // reset the action table
        m_action_table = xcm::null;
    }
}

void CompReportLayout::onReportLayoutEngineUpdated()
{
    // fire an event indicating that the report has been updated
    kcanvas::INotifyEventPtr notify_evt;
    notify_evt = kcanvas::NotifyEvent::create(EVENT_REPORT_UPDATED, this);
    dispatchEvent(notify_evt);
}

kcanvas::IComponentPtr CompReportLayout::createTableColumnHeader()
{
    kcanvas::ICompTablePtr table;
    table = kcanvas::CompTableHeader::create(kcanvas::CompTableHeader::Column);
    
    if (table.isNull())
        return xcm::null;

    // add a name property so we can identify the header
    kcanvas::IComponentPtr comp = table;
    comp->addProperty(NAME_TABLE_COLUMN_HEADER, wxT(""));

    // set the default column header parameters
    table->setColumnCount(25);
    table->setRowCount(1);
    table->setRowSize(0, TABLE_COLUMN_HEADER_SIZE);

    // save the defaults
    table->tag(TAG_TABLE_HEADER_DEFAULT);

    return table;
}

kcanvas::IComponentPtr CompReportLayout::createTableRowHeader()
{
    kcanvas::ICompTablePtr table;
    table = kcanvas::CompTableHeader::create(kcanvas::CompTableHeader::Row);
    
    if (table.isNull())
        return xcm::null;

    // add a name property so we can identify the header
    kcanvas::IComponentPtr comp = table;
    comp->addProperty(NAME_TABLE_ROW_HEADER, wxT(""));

    // set the default row header parameters
    table->setColumnCount(1);
    table->setRowCount(25);
    table->setColumnSize(0, TABLE_ROW_HEADER_SIZE);

    // save the defaults
    table->tag(TAG_TABLE_HEADER_DEFAULT);

    return table;
}

kcanvas::IComponentPtr CompReportLayout::updateTableRowHeader(CompReportLayoutPage& page_info,
                                                              int x_offset,
                                                              int y_offset)
{
    // get the table associated with the page
    kcanvas::IComponentPtr comp_page_table;
    kcanvas::ICompTablePtr page_table;
    comp_page_table = getPageTable(page_info.m_page);
    page_table = comp_page_table;

    // if we don't have a table, we're done
    if (page_table.isNull())
        return xcm::null;

    kcanvas::IComponentPtr row_header = page_info.m_row_header;
    if (row_header.isNull())
    {
        row_header = createTableRowHeader();
        page_info.m_row_header = row_header;
    }

    // copy the row sizes from the page table
    kcanvas::ICompTablePtr table_row_header = row_header;
    if (table_row_header.isNull())
        return xcm::null;

    // restore state and retag
    table_row_header->restore(TAG_TABLE_HEADER_DEFAULT);
    table_row_header->tag(TAG_TABLE_HEADER_DEFAULT);

    // set the row count and row sizes
    std::vector<kcanvas::CellProperties> row_sizes;
    page_table->getCellProperty(kcanvas::CellRange(-1,-1), kcanvas::PROP_SIZE_H, row_sizes);

    table_row_header->setRowCount(page_table->getRowCount());
    table_row_header->addCellProperties(row_sizes, false); // don't replace since we're using tags

    // get the page table position
    int page_table_x, page_table_y;
    comp_page_table->getOrigin(&page_table_x, &page_table_y);

    // set the table size and position
    wxRect rect;
    row_header->extends(rect);

    row_header->setOrigin(x_offset, page_info.m_page_rect.y + page_table_y);
    row_header->setSize(rect.width, rect.height);
    return row_header;
}

kcanvas::IComponentPtr CompReportLayout::updateTableColumnHeader(CompReportLayoutPage& page_info,
                                                                 int x_offset,
                                                                 int y_offset)
{
    // get the table associated with the page
    kcanvas::IComponentPtr comp_page_table;
    kcanvas::ICompTablePtr page_table;
    comp_page_table = getPageTable(page_info.m_page);
    page_table = comp_page_table;

    // if we don't have a table, we're done
    if (page_table.isNull())
        return xcm::null;

    kcanvas::IComponentPtr column_header = page_info.m_column_header;
    if (column_header.isNull())
    {
        // if we have an action table that's a column header, use it;
        // otherwise, create one        
        column_header = m_action_table;
        if (!isColumnHeader(m_action_table))
            column_header = createTableColumnHeader();

        page_info.m_column_header = column_header;
    }

    // copy the column sizes from the page table; first save
    // the state, then copy
    kcanvas::ICompTablePtr table_column_header = column_header;
    if (table_column_header.isNull())
        return xcm::null;

    table_column_header->restore(TAG_TABLE_HEADER_DEFAULT);
    table_column_header->tag(TAG_TABLE_HEADER_DEFAULT);

    // set the column count and column sizes
    std::vector<kcanvas::CellProperties> column_sizes;
    page_table->getCellProperty(kcanvas::CellRange(-1,-1), kcanvas::PROP_SIZE_W, column_sizes);

    table_column_header->setColumnCount(page_table->getColumnCount());
    table_column_header->addCellProperties(column_sizes, false); // don't replace since we're using tags

    // copy the selections from the page table
    std::vector<kcanvas::CellRange> selections;
    page_table->getCellSelections(selections);
    
    std::vector<kcanvas::CellRange>::iterator it, it_end;
    it_end = selections.end();

    for (it = selections.begin(); it != it_end; ++it)
    {
        kcanvas::CellRange column(-1, it->col1(), -1, it->col2());
        table_column_header->selectCells(column);
    }

    // set the table size and position
    int page_table_x, page_table_y;
    comp_page_table->getOrigin(&page_table_x, &page_table_y);

    wxRect rect;
    column_header->extends(rect);

    column_header->setOrigin(x_offset + page_info.m_page_rect.x + page_table_x, 
                             y_offset);
    column_header->setSize(rect.width, rect.height);
    return column_header;
}

kcanvas::ICompTablePtr CompReportLayout::getPageTable(kcanvas::ICompPagePtr page)
{
    // note: returns the first table component of a page

    // if we don't have a valid component, we're done
    kcanvas::IComponentPtr comp = page;
    if (comp.isNull())
        return xcm::null;

    // iterate through the children        
    std::vector<kcanvas::IComponentPtr> children;
    comp->getComponents(children);

    std::vector<kcanvas::IComponentPtr>::iterator it, it_end;
    it_end = children.end();

    for (it = children.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = *it;
        if (!table.isNull())
            return table;
    }

    // we don't have a child table
    return xcm::null;
}


ReportOutputPdf::ReportOutputPdf(const ReportLayoutEngine& engine,
                     const ReportPrintInfo& print_info,
                     const wxString& location,
                     bool block) :
                 ReportOutputBase(engine, print_info, location, block)
{
    init();
}

ReportOutputPdf::ReportOutputPdf(const std::vector<ReportLayoutEngine>& engines,
                     const ReportPrintInfo& print_info,
                     const wxString& location,
                     bool block) :
                 ReportOutputBase(engines, print_info, location, block)
{
    init();
}

ReportOutputPdf::~ReportOutputPdf()
{
}

void ReportOutputPdf::create()
{
    // prepare the layout; the rest is handled in the engine
    // updated handler; note; use a "for loop" rather than
    // a vector iterator since with an iterator, the end of the 
    // last execute job will destroy the vector, causing a crash
    int count = m_layout_engines.size();
    for (int idx = 0; idx < count; ++idx)
    {
        m_layout_engines[idx]->execute(m_block);
    }
}

void ReportOutputPdf::Notify()
{
    doCreatePage();
}

void ReportOutputPdf::onReportLayoutEngineUpdated()
{
    // wait until all the engines are read; because of the signal
    // handlers, the engines will call this function when they are
    // finished, so when all are finished, we'll be ready to move on
    std::vector<ReportLayoutEngine*>::iterator it, it_end;
    it_end = m_layout_engines.end();
    
    for (it = m_layout_engines.begin(); it != it_end; ++it)
    {    
        if (!(*it)->isReady())
            return;
    }

    // get the total page count
    int total_page_count = 0;
    for (it = m_layout_engines.begin(); it != it_end; ++it)
    {
        total_page_count += (*it)->getPageCount();
    }

    if (m_print_info.getAllPages())
    {
        m_print_info.setMinPage(1);
        m_print_info.setMaxPage(total_page_count);
        m_print_info.setFromPage(1);
        m_print_info.setToPage(total_page_count);
    }

    // reset the current output page
    m_pdf_output_page_idx = 0;

    // if we're not in blocking mode, start the timer, and
    // it will take care of the rest
    if (!m_block)
    {
        Start(100);
        return;
    }

    // we're in blocking mode; create all the pages
    while (doCreatePage())
    {
    }
}

void ReportOutputPdf::init()
{
    // initialize the pdf canvas
    kcanvas::ICanvas* pdf = kcanvas::CanvasPDF::create();
    m_pdf_ptr = static_cast<kcanvas::CanvasPDF*>(pdf);
    m_pdf_ptr->setZoom(100);

    // connect the signal handlers
    std::vector<ReportLayoutEngine*>::iterator it, it_end;
    it_end = m_layout_engines.end();
    
    for (it = m_layout_engines.begin(); it != it_end; ++it)
    {
        (*it)->sigLayoutUpdated().connect(this, &ReportOutputPdf::onReportLayoutEngineUpdated);
    }

    // current output page
    m_pdf_output_page_idx = 0;
}

void ReportOutputPdf::destroyObject()
{
    // destroy the pdf canvas manually, since it isn't 
    // reference counted
    if (m_pdf_ptr)
        m_pdf_ptr->destroy();
        
    // delete the current object
    delete this;
}

bool ReportOutputPdf::doCreatePage()
{
    // note: returns true if there are additional pages
    // to save; false otherwise

    // finished/cancelled flags
    bool cancelled = false;
    bool finished = false;
    int pdf_output_start_page = m_print_info.getFromPage();
    int pdf_output_end_page = m_print_info.getToPage();
    int pdf_page_count = pdf_output_end_page - pdf_output_start_page + 1;

    // notify event
    kcanvas::INotifyEventPtr notify_evt;

    // if we're on the first page
    if (m_pdf_output_page_idx == 0)
    {
        // set the page index to the start page
        m_pdf_output_page_idx = pdf_output_start_page;

        // notify handlers that the output process is starting
        kcanvas::Properties props;
        props.add(kcanvas::EVENT_PROP_PAGE_START, pdf_output_start_page);
        props.add(kcanvas::EVENT_PROP_PAGE_END, pdf_output_end_page);
        props.add(kcanvas::EVENT_PROP_PAGES, pdf_page_count);
        sigOutputBegin().fire(this, props);
        
        // if the process is cancelled, set the flag
        if (isCancelled())
            cancelled = true;       
    }

    // get the appropriate layout engine, and find out the page
    // idx we're creating within that particular engine
    ReportLayoutEngine* layout_engine;
    int output_page_idx = m_pdf_output_page_idx;

    std::vector<ReportLayoutEngine*>::iterator it, it_end;
    it_end = m_layout_engines.end();

    for (it = m_layout_engines.begin(); it != it_end; ++it)
    {
        layout_engine = *it;
        int count = (*it)->getPageCount();

        if (output_page_idx - count <= 0)
            break;

        output_page_idx -= count;
    }

    // create the page and add it to the pdf canvas;
    // note: getPageByIdx() is zero based, so subtract 1
    // get the appropriate layout engine, and find out the page
    // idx we're creating within that particular engine
    wxRect page_rect;
    kcanvas::IComponentPtr comp = layout_engine->getPageByIdx(output_page_idx - 1);
    comp->getOrigin(&page_rect.x, &page_rect.y);
    comp->getSize(&page_rect.width, &page_rect.height);

    m_pdf_ptr->removeAll();
    m_pdf_ptr->add(comp);
    m_pdf_ptr->setSize(page_rect.width, page_rect.height);
    m_pdf_ptr->setViewOrigin(page_rect.x, page_rect.y);
    m_pdf_ptr->setViewSize(page_rect.width, page_rect.height);

    // create the pdf page
    m_pdf_ptr->addPage(page_rect.width, page_rect.height);
    m_pdf_ptr->layout();
    m_pdf_ptr->render(page_rect);
    
    // notify handlers that the output process is outputting a page
    kcanvas::Properties props;
    props.add(kcanvas::EVENT_PROP_PAGE_START, pdf_output_start_page);
    props.add(kcanvas::EVENT_PROP_PAGE_END, pdf_output_end_page);
    props.add(kcanvas::EVENT_PROP_PAGES, pdf_page_count);
    props.add(kcanvas::EVENT_PROP_PAGE, m_pdf_output_page_idx);
    sigOutputPage().fire(this, props);

    // if the process is cancelled, set the flag
    if (isCancelled())
        cancelled = true;       

    // increment the page; if the output page is past the last page,
    // set the finished flag to true
    m_pdf_output_page_idx++;
    if (m_pdf_output_page_idx > pdf_output_end_page)
        finished = true;

    // if we're finished output the pdf or the job is cancelled
    // notify, stop the timer
    if (finished || cancelled)
    {
        if (IsRunning())
            Stop();

        // if the job isn't cancelled, save the PDF
        if (!cancelled)
            m_pdf_ptr->saveAs(m_location);

        // notify handlers that the output process is ending
        kcanvas::Properties props;
        props.add(kcanvas::EVENT_PROP_PAGE_START, pdf_output_start_page);
        props.add(kcanvas::EVENT_PROP_PAGE_END, pdf_output_end_page);
        props.add(kcanvas::EVENT_PROP_PAGES, pdf_page_count);
        sigOutputEnd().fire(this, props);

        // we're done; this will delete this object
        destroyObject();
        return false;
    }

    return true;
}


ReportOutputPrinter::ReportOutputPrinter(const ReportLayoutEngine& engine,
                         const ReportPrintInfo& print_info,
                         const wxString& location,
                         bool block) :
                     ReportOutputBase(engine, print_info, location, block)
{
    init();
}

ReportOutputPrinter::ReportOutputPrinter(const std::vector<ReportLayoutEngine>& engines,
                         const ReportPrintInfo& print_info,
                         const wxString& location,
                         bool block) :
                     ReportOutputBase(engines, print_info, location, block)
{
    init();
}

ReportOutputPrinter::~ReportOutputPrinter()
{
}

void ReportOutputPrinter::create()
{
    // prepare the layout; the rest is handled in the engine
    // updated handler; note; use a "for loop" rather than
    // a vector iterator since with an iterator, the end of the 
    // last execute job will destroy the vector, causing a crash
    int count = m_layout_engines.size();
    for (int idx = 0; idx < count; ++idx)
    {
        m_layout_engines[idx]->execute(m_block);
    }
}

void ReportOutputPrinter::onPrint(kcanvas::IEventPtr evt)
{
    wxString name = evt->getName();
    kcanvas::INotifyEventPtr print_evt = evt;

    if (name == kcanvas::EVENT_PRINT_PAGE)
    {
        // notify handlers that the output process is outputting a page
        kcanvas::PropertyValue start_page, end_page, page_count, prop_page_idx;
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGE_START, start_page);
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGE_END, end_page);
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGES, page_count);
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGE, prop_page_idx);

        kcanvas::Properties props;
        props.add(kcanvas::EVENT_PROP_PAGE_START, start_page.getInteger());
        props.add(kcanvas::EVENT_PROP_PAGE_END, end_page.getInteger());
        props.add(kcanvas::EVENT_PROP_PAGES, page_count.getInteger());
        props.add(kcanvas::EVENT_PROP_PAGE, prop_page_idx.getInteger());
        sigOutputPage().fire(this, props);

        // if the process is cancelled, veto the event which
        // will stop the printing process
        if (isCancelled())
            print_evt->veto();

        // create the page being printed
        m_printout_output_page_idx = prop_page_idx.getInteger();
        doCreatePage();
    }

    if (name == kcanvas::EVENT_PRINT_BEGIN)
    {
        // notify handlers that the output process is starting
        kcanvas::PropertyValue start_page, end_page, page_count;
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGE_START, start_page);
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGE_END, end_page);
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGES, page_count);

        kcanvas::Properties props;
        props.add(kcanvas::EVENT_PROP_PAGE_START, start_page.getInteger());
        props.add(kcanvas::EVENT_PROP_PAGE_END, end_page.getInteger());
        props.add(kcanvas::EVENT_PROP_PAGES, page_count.getInteger());
        sigOutputBegin().fire(this, props);

        // if the process is cancelled, veto the event which
        // will stop the printing process
        if (isCancelled())
            print_evt->veto();
    }

    if (name == kcanvas::EVENT_PRINT_END)
    {
       // notify handlers that the output process is ending
        kcanvas::PropertyValue start_page, end_page, page_count;
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGE_START, start_page);
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGE_END, end_page);
        print_evt->getProperty(kcanvas::EVENT_PROP_PAGES, page_count);

        kcanvas::Properties props;
        props.add(kcanvas::EVENT_PROP_PAGE_START, start_page.getInteger());
        props.add(kcanvas::EVENT_PROP_PAGE_END, end_page.getInteger());
        props.add(kcanvas::EVENT_PROP_PAGES, page_count.getInteger());
        sigOutputEnd().fire(this, props);

        // printing is done; destroy this object
        destroyObject();
    }
}

void ReportOutputPrinter::onReportLayoutEngineUpdated()
{
    // wait until all the engines are read; because of the signal
    // handlers, the engines will call this function when they are
    // finished, so when all are finished, we'll be ready to move on
    std::vector<ReportLayoutEngine*>::iterator it, it_end;
    it_end = m_layout_engines.end();    
    
    for (it = m_layout_engines.begin(); it != it_end; ++it)
    {    
        if (!(*it)->isReady())
            return;
    }

    // get the total page count
    int total_page_count = 0;
    for (it = m_layout_engines.begin(); it != it_end; ++it)
    {
        total_page_count += (*it)->getPageCount();
    }

    if (m_print_info.getAllPages())
    {
        m_print_info.setMinPage(1);
        m_print_info.setMaxPage(total_page_count);
        m_print_info.setFromPage(1);
        m_print_info.setToPage(total_page_count);
    }

    // reset the current output page
    m_printout_output_page_idx = 0;

    // set some printout info
    m_printout_ptr->SetPageInfo(m_print_info.getMinPage(),
                                m_print_info.getMaxPage(),
                                m_print_info.getFromPage(),
                                m_print_info.getToPage());

    m_printout_ptr->SetNumberCopies(m_print_info.getNumberCopies());

    // create a printer object; will be destroyed when printing 
    // is done or there's a printing problem
    wxPrintDialogData data;
    data.SetAllPages(m_print_info.getAllPages());
    data.SetMinPage(m_print_info.getMinPage());
    data.SetMaxPage(m_print_info.getMaxPage());
    data.SetFromPage(m_print_info.getFromPage());
    data.SetToPage(m_print_info.getToPage());
    data.SetNoCopies(m_print_info.getNumberCopies());

    // set the printout size; TODO: currently, this is set here as well as 
    // right before the page is printed; ideally, we would like each page to 
    // be printed according to it's own dimensions, but currently, the printout 
    // canvas sets the page dimensions at the beginning of the printout, so 
    // they need to be set here for proper orientation; when the printout canvas
    // can accept different page sizes for each page, simply remove this

    int page_width, page_height;
    m_print_info.getPageSize(&page_width, &page_height);
    page_width = in2mm((double)page_width/kcanvas::CANVAS_MODEL_DPI)*10;
    page_height = in2mm((double)page_height/kcanvas::CANVAS_MODEL_DPI)*10;
    wxSize psize(page_width, page_height);

    int orientation = wxPORTRAIT;
    if (psize.x >= psize.y)
    {
        int y = psize.y;
        psize.y = psize.x;
        psize.x = y;
        orientation = wxLANDSCAPE;
    }

    wxPrintPaperDatabase* paper_db = g_app->getPaperDatabase();
    wxPrintPaperType* paper_type = paper_db->FindPaperType(psize);
    
    wxPaperSize paper_size;    
    if (!paper_type)
        paper_size = wxPAPER_NONE;
         else
        paper_size = paper_type->GetId();

    data.GetPrintData().SetOrientation(orientation);
    data.GetPrintData().SetPaperId(paper_size);    

    // object will destroy itself when done
    AppPrinter* printer;
    printer = new AppPrinter(&data);

    // print the document; the printout canvas will call onPrint() 
    // when a particular page is about to be printed; actual creation 
    // and layout of a page happens there; note: this is a little
    // different than the equivalent functionality for the PDF since
    // the CanvasPrintout derives from wxPrintout and uses events
    // to signal that a page needs to be updated
    printer->Print(m_printout_ptr);
}

void ReportOutputPrinter::init()
{
    // initialize the printout canvas
    kcanvas::ICanvas* printout = kcanvas::CanvasPrintout::create();
    m_printout_ptr = static_cast<kcanvas::CanvasPrintout*>(printout);
    m_printout_ptr->setZoom(100);

    // connect to the canvas printout events
    kcanvas::EventHandler* handler = new kcanvas::EventHandler;
    handler->sigEvent().connect(this, &ReportOutputPrinter::onPrint);

    kcanvas::IEventTargetPtr target = printout;    
    target->addEventHandler(kcanvas::EVENT_PRINT_BEGIN, handler);
    target->addEventHandler(kcanvas::EVENT_PRINT_END, handler);
    target->addEventHandler(kcanvas::EVENT_PRINT_PAGE, handler);

    // connect the signal handlers
    std::vector<ReportLayoutEngine*>::iterator it, it_end;
    it_end = m_layout_engines.end();
    
    for (it = m_layout_engines.begin(); it != it_end; ++it)
    {
        (*it)->sigLayoutUpdated().connect(this, &ReportOutputPrinter::onReportLayoutEngineUpdated);
    }

    // current output page
    m_printout_output_page_idx = 0;
}

void ReportOutputPrinter::destroyObject()
{
    // destroy the pdf canvas manually, since it isn't 
    // reference counted
    if (m_printout_ptr)
        m_printout_ptr->destroy();

    // delete the current object
    delete this;
}

bool ReportOutputPrinter::doCreatePage()
{
    // get the appropriate layout engine, and find out the page
    // idx we're creating within that particular engine
    ReportLayoutEngine* layout_engine = m_layout_engines[0];
    int output_page_idx = m_printout_output_page_idx;

    std::vector<ReportLayoutEngine*>::iterator it, it_end;
    it_end = m_layout_engines.end();

    for (it = m_layout_engines.begin(); it != it_end; ++it)
    {
        int count = (*it)->getPageCount();
        if (output_page_idx - count <= 0)
            break;

        layout_engine = *it;
        output_page_idx -= count;
    }

    // create the page and add it to the printout canvas;
    // note: getPageByIdx() is zero based, so subtract 1
    wxRect page_rect;
    kcanvas::IComponentPtr comp = layout_engine->getPageByIdx(output_page_idx - 1);
    comp->getOrigin(&page_rect.x, &page_rect.y);
    comp->getSize(&page_rect.width, &page_rect.height);

    m_printout_ptr->removeAll();
    m_printout_ptr->add(comp);
    m_printout_ptr->setSize(page_rect.width, page_rect.height);
    m_printout_ptr->setViewOrigin(page_rect.x, page_rect.y);
    m_printout_ptr->setViewSize(page_rect.width, page_rect.height);

    // layout the printout page; rendering will be handled by
    // the framework
    m_printout_ptr->layout();

    return true;
}

