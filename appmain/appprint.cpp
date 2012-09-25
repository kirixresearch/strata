/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2008-01-07
 *
 */


#include "appmain.h"
#include "appprint.h"

#ifdef __WXGTK__
#include <wx/dcps.h>
#endif

#ifdef WIN32
    #include "windows.h"
#endif


AppPrinter::AppPrinter(wxPrintDialogData* printdata) : wxTimer()
{
    // default print data
    m_printdata.SetAllPages(true);
    m_printdata.SetCollate(true);
    m_printdata.SetNoCopies(1);

    m_printerror = wxPRINTER_NO_ERROR;
    m_printout = NULL;
    m_dc = NULL;
    
    // page printing parameters; duplicates values
    // in m_printdata member, but factored out for
    // convenience; will be set to appropriate
    // values later
    m_min_page = 0;
    m_max_page = 0;
    m_start_page = 0;
    m_end_page = 0;
    m_number_copies = 0;
    m_collate = true;
    
    // current page/copy being spooled
    m_current_page = 0;
    m_current_copy = 0;
    
    m_printing = false;
    
    // if print data was specified, set it; do this
    // after default initialization so that print data 
    // isn't overwritten by initialization
    if (printdata)
        m_printdata = *printdata;
}

AppPrinter::~AppPrinter()
{
}

void AppPrinter::PrintDialog(wxWindow* parent, wxPrintDialogData* printdata)
{
    m_printerror = wxPRINTER_NO_ERROR;

    wxPrintDialog dialog(parent, &m_printdata);
    if (dialog.ShowModal() != wxID_OK)
    {
        m_printerror = wxPRINTER_CANCELLED;
        return;
    }

    // show the dialog
    m_printdata = dialog.GetPrintDialogData();

    // return the dialog data
    if (printdata)
        *printdata = m_printdata;
}

bool AppPrinter::Print(wxPrintout* printout)
{
    // if we're already printing, we're done
    if (m_printing)
        return false;

    // if we have a already have a printer error, 
    // we're done
    if (m_printerror != wxPRINTER_NO_ERROR)
    {
        Destroy();
        return false;
    }

    // if we don't have a printout, we're done
    if (!printout)
    {
        Destroy();
        return false;
    }

    // set the printout
    m_printout = printout;

    // if we don't have a printer dc already, get one
    if (!m_dc)
    {
        #ifdef WIN32
            m_dc = new wxPrinterDC(m_printdata.GetPrintData());
        #else
            m_dc = new wxPostScriptDC(m_printdata.GetPrintData());
        #endif
    }

    // if we still don't have a printer dc or something is wrong
    // with the DC, we're done
    if (!m_dc || !m_dc->IsOk())
    {
        m_printerror = wxPRINTER_ERROR;
        Destroy();
        return false;
    }

    // get the size of the screen in pixels and millimeters
    wxSize screen_size_pixels = wxGetDisplaySize();
    wxSize screen_size_mm = wxGetDisplaySizeMM();
    
    // get the ppi of the screen, which is the total 
    // pixels divided by total inches (mm/25.4)
    int screen_ppi_x = (int)(screen_size_pixels.GetWidth()*25.4)/(screen_size_mm.GetWidth());
    int screen_ppi_y = (int)(screen_size_pixels.GetHeight()*25.4)/(screen_size_mm.GetHeight());

    // get the ppi of the printer
    #ifdef WIN32
        int printer_ppi_x = ::GetDeviceCaps((HDC)kcl::getHdcFrom(*m_dc), LOGPIXELSX);
        int printer_ppi_y = ::GetDeviceCaps((HDC)kcl::getHdcFrom(*m_dc), LOGPIXELSY);
    #else
        wxPostScriptDC postscript_dc;
        int res = postscript_dc.GetResolution();
        int printer_ppi_x = res;
        int printer_ppi_y = res;
    #endif

    // make sure the printer ppi is returning valid values;
    // if not, we're done
    if (printer_ppi_x <= 0 || printer_ppi_y <= 0)
    {
        m_printerror = wxPRINTER_ERROR;
        Destroy();
        return false;
    }

    // set the printout dc to the printer dc
    printout->SetDC(m_dc);

    // set the printout dimensions from the dc
    printout->SetPPIScreen(screen_ppi_x, screen_ppi_y);
    printout->SetPPIPrinter(printer_ppi_x, printer_ppi_y);

    int page_size_x, page_size_y;
    int page_size_x_mm, page_size_y_mm;
    m_dc->GetSize(&page_size_x, &page_size_y);
    m_dc->GetSize(&page_size_x_mm, &page_size_y_mm);
    
    printout->SetPageSizePixels(page_size_x, page_size_y);
    printout->SetPaperRectPixels(wxRect(0, 0, page_size_x, page_size_y));
    printout->SetPageSizeMM(page_size_x_mm, page_size_y_mm);

    // prepare printing
    printout->OnPreparePrinting();

    // get the print data; note: get the print data after prepare
    // printing in case any of the print data parameters were set
    // in the function
    m_min_page = m_printdata.GetMinPage();
    m_max_page = m_printdata.GetMaxPage();
    m_start_page = m_printdata.GetFromPage();
    m_end_page = m_printdata.GetToPage();
    m_number_copies = m_printdata.GetNoCopies();
    m_collate = m_printdata.GetCollate();

    if (m_start_page < m_min_page)
        m_start_page = m_min_page;
        
    if (m_end_page > m_max_page)
        m_end_page = m_max_page;
        
    if (m_start_page <= 0 || m_end_page <= 0 || m_number_copies <= 0)
    {
        m_printerror = wxPRINTER_ERROR;
        Destroy();
        return false;
    }

    // set the current page and copy to the start of the document;
    // these will be incremented as each page is finished
    m_current_page = m_start_page;
    m_current_copy = 1;

    // start the timer; when the timer is running, pages will be
    // printed at regular intervals; the timer will be stopped
    // when all the pages are finished printing
    Start(100);

    // return true; we're printing
    return true;
}

void AppPrinter::Notify()
{
    if (m_printerror != wxPRINTER_NO_ERROR)
    {
        Destroy();
        return;
    }

    // set the flag that the document is printing
    m_printing = true;

    // if we're on the first page, notify the printout object
    // that we're beginning printing
    if (m_current_page == m_start_page && m_current_copy == 1)
    {
        m_printout->OnBeginPrinting();
        m_printout->OnBeginDocument(m_start_page, m_end_page);
    }

    // finished/cancelled flags
    bool cancelled = false;
    bool finished = false;

    // print the page; if OnPrintPage returns false, cancel
    // the print job
    m_dc->StartPage();
    
    if (!m_printout->OnPrintPage(m_current_page))
        cancelled = true;

    m_dc->EndPage();
    
    // increment the page and copy count
    if (m_collate)
    {
        m_current_page++;
        if (m_current_page > m_end_page)
        {
            m_current_page = m_start_page;
            m_current_copy++;
        }
        
        if (m_current_copy > m_number_copies)
            finished = true;
    }
    else
    {
        m_current_copy++;
        if (m_current_copy > m_number_copies)
        {
            m_current_copy = 1;
            m_current_page++;
        }
        
        if (m_current_page > m_end_page)
            finished = true;
    }

    // if we're finished printing or the print job is cancelled, 
    // notify the printout object that we're ending printing, stop 
    // the timer, and release the dc
    if (finished || cancelled)
    {
        // if the timer is running, stop it
        if (IsRunning())
            Stop();
    
        m_printout->OnEndDocument();
        m_printout->OnEndPrinting();
        Destroy();
    }
}

void AppPrinter::Destroy()
{
/*
    // destroy the printout object
    if (m_printout)
        delete m_printout;

    // destroy the DC
    if (m_dc)
        delete m_dc;

    // destroy this
    delete this;
*/
}


