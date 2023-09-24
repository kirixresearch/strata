/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2007-01-31
 *
 */


#ifndef H_KCANVAS_CANVASPRINTOUT_H
#define H_KCANVAS_CANVASPRINTOUT_H


#include <wx/print.h>

namespace kcanvas
{


// Canvas Printout Classes

// canvas printout class definition
class CanvasPrintout : public CanvasDC,
                       public wxPrintout
{
    XCM_CLASS_NAME_NOREFCOUNT("kcanvas.CanvasPrintout")
    XCM_BEGIN_INTERFACE_MAP(CanvasPrintout)
        XCM_INTERFACE_CHAIN(CanvasDC)
    XCM_END_INTERFACE_MAP()

public:

    CanvasPrintout(const wxString& title = wxT("Document"));
    virtual ~CanvasPrintout();

    static ICanvasPtr create(const wxString& title = wxT("Document"));

public:

    // ICanvas
    
    ICanvasPtr clone();
    void copy(ICanvasPtr canvas);
    
public:    

    // note: wxPrintout derived, except SetPageInfo(), 
    // GetNumberCopies(), SetNumberCopies()
    
    void GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo);
    void SetPageInfo(int minPage, int maxPage, int pageFrom, int pageTo);
    bool HasPage(int page_num);

    int GetNumberCopies();
    void SetNumberCopies(int number_copies);
    
    void OnPreparePrinting();
    void OnBeginPrinting();
    void OnEndPrinting();
    bool OnPrintPage(int page_num);
    
private:

    wxDC* m_printer_dc;        // printer dc
    wxDC* m_original_dc;       // canvas dc at start of printing

    int m_printer_dpi_x;       // number of pixels per horizontal inch of the printer
    int m_printer_dpi_y;       // number of pixels per vertical inch of the printer
    int m_paper_pixels_x;      // number of pixels per the width of the page
    int m_paper_pixels_y;      // number of pixels per the height of the page

    int m_min_page;            // minimum page number for the document
    int m_max_page;            // maximum page number for the document
    int m_start_page;          // start page for printing
    int m_end_page;            // end page for printing
    int m_number_copies;       // number of copies to print
    
    bool m_cancelled;           // true if the print job is cancelled
};


} // namespace kcanvas


#endif

