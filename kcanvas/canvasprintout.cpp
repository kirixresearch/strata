/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2007-01-31
 *
 */


#include "kcanvas_int.h"
#include "canvas.h"
#include "canvasdc.h"
#include "canvasprintout.h"


namespace kcanvas
{

// wxPrintout implementation for canvas
CanvasPrintout::CanvasPrintout(const wxString& title) : wxPrintout(title)
{
    m_printer_dc = NULL;    // printer dc
    m_printer_dpi_x = 0;    // number of pixels per horizontal inch of the printer
    m_printer_dpi_y = 0;    // number of pixels per vertical inch of the printer
    m_paper_pixels_x = 0;   // number of pixels per the width of the page
    m_paper_pixels_y = 0;   // number of pixels per the height of the page
    
    m_min_page = 0;         // minimum page number for the document
    m_max_page = 0;         // maximum page number for the document
    m_start_page = 0;       // start page for printing
    m_end_page = 0;         // end page for printing
    m_number_copies = 0;    // number of copies to print
    
    m_cancelled = false;    // true if the print job is cancelled    
}

CanvasPrintout::~CanvasPrintout()
{
}

ICanvasPtr CanvasPrintout::create(const wxString& title)
{
    return static_cast<ICanvas*>(new CanvasPrintout(title));
}

ICanvasPtr CanvasPrintout::clone()
{
    CanvasPrintout* c = new CanvasPrintout;
    c->copy(this);

    return static_cast<ICanvas*>(c);
}

void CanvasPrintout::copy(ICanvasPtr canvas)
{
    // copy the base canvas properties
    Canvas::copy(canvas);

    // TODO: copy additional pdf info
}

void CanvasPrintout::GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo)
{
    *minPage = m_min_page;
    *maxPage = m_max_page;
    *pageFrom = m_start_page;
    *pageTo = m_end_page;
}

void CanvasPrintout::SetPageInfo(int minPage, int maxPage, int pageFrom, int pageTo)
{
    m_min_page = minPage;
    m_max_page = maxPage;
    m_start_page = pageFrom;
    m_end_page = pageTo;
}

bool CanvasPrintout::HasPage(int page_num)
{    
    // if the page is contained between the minimum and maximum
    // number of pages, return true; otherwise return false
    if (page_num >= m_min_page && page_num <= m_max_page)
        return true;
        
    return false;
}

int CanvasPrintout::GetNumberCopies()
{
    return m_number_copies;
}

void CanvasPrintout::SetNumberCopies(int number_copies)
{
    m_number_copies = number_copies;
}

void CanvasPrintout::OnPreparePrinting()
{
    // called by printing framework before other functions
    // so we have a chance to perform calculations
    
    // get information about the printer resolution and page size
    GetPPIPrinter(&m_printer_dpi_x, &m_printer_dpi_y);
    GetPageSizePixels(&m_paper_pixels_x, &m_paper_pixels_y);
}

void CanvasPrintout::OnBeginPrinting()
{
    // get the printer dc
    m_printer_dc = GetDC();
    if (m_printer_dc == NULL)
        return;

    // dispatch a begin print notify event; start page is the
    // index of the page to start printing; end page is the
    // index of the page to end printing; pages is the total
    // number of pages to print, taking into account multiple
    // copies
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_PRINT_BEGIN, this);
    notify_evt->addProperty(EVENT_PROP_PAGE_START, m_start_page);
    notify_evt->addProperty(EVENT_PROP_PAGE_END, m_end_page);
    notify_evt->addProperty(EVENT_PROP_PAGES, (m_end_page-m_start_page+1)*m_number_copies);
    dispatchEvent(notify_evt);

    // if the event is vetoed, cancel the job
    if (!notify_evt->isAllowed())
    {
        m_cancelled = true;
        return;
    }

    // set the canvas dc
    setDC(m_printer_dc);
}

void CanvasPrintout::OnEndPrinting()
{
    // dispatch an end print notify event
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_PRINT_END, this);
    notify_evt->addProperty(EVENT_PROP_PAGE_START, m_start_page);
    notify_evt->addProperty(EVENT_PROP_PAGE_END, m_end_page);
    notify_evt->addProperty(EVENT_PROP_PAGES, (m_end_page-m_start_page+1)*m_number_copies);
    dispatchEvent(notify_evt);
}

bool CanvasPrintout::OnPrintPage(int page_num)
{
    // if the print job has been cancelled, return false
    if (m_cancelled)
        return false;

    // if we still don't have a printer dc, we're done
    if (m_printer_dc == NULL)
        return false;

    // if the page number to print is outside the range of
    // pages, we're done
    if (!HasPage(page_num))
        return false;

    // dispatch a print page notify event
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_PRINT_PAGE, this);
    notify_evt->addProperty(EVENT_PROP_PAGE_START, m_start_page);
    notify_evt->addProperty(EVENT_PROP_PAGE_END, m_end_page);
    notify_evt->addProperty(EVENT_PROP_PAGES, (m_end_page-m_start_page+1)*m_number_copies);
    notify_evt->addProperty(EVENT_PROP_PAGE, page_num);
    dispatchEvent(notify_evt);

    // if the event was vetoed, return false, we're
    // if the event is vetoed, cancel the job
    if (!notify_evt->isAllowed())
    {
        m_cancelled = true;
        return false;
    }

    // set the scale and zoom for the canvas
    setScale(m_printer_dpi_x, m_printer_dpi_y);
    setZoom(100);

    // TODO: if the paper size is different from the model size, 
    // we might want to adjust the zoom so that the page fits;
    // this isn't a requirement, since the paper size matches 
    // the model size in the current use case; however, we might 
    // want to add this later

    // get the page rectangle, which corresponds to the canvas
    wxRect page(0, 0, 0, 0);
    getSize(&page.width, &page.height);

    // calculate the printer margins and adjust the view to account
    // for these margins; note: printer margins are different than page
    // margins; printer margins are the small, non-printable spaces around 
    // the edge of a page, not the larger margins defined by the document;
    // since the printing starts at these margins, if we don't account 
    // for them, the printout is slightly offset from the screen
    int x_margin, y_margin;
    x_margin = (page.GetWidth() - dtom_x(m_paper_pixels_x))/2;
    y_margin = (page.GetHeight() - dtom_y(m_paper_pixels_y))/2;

    // save the old view origin and size
    int view_x, view_y, view_width, view_height;
    getViewOrigin(&view_x, &view_y);
    getViewSize(&view_width, &view_height);

    // set the new view origin and size; note: adjusting the view 
    // origin in the following way accounts for the printer margins
    // we calculated above
    setViewOrigin(x_margin, y_margin);
    setViewSize(page.width, page.height);

    // render the page
    render(page);

    // restore the old view origin and size
    setViewOrigin(view_x, view_y);
    setViewSize(view_width, view_height);

    return true;
}


}; // namespace kcanvas

