/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2008-03-13
 *
 */
 

#ifndef __APP_REPORTLAYOUT_H
#define __APP_REPORTLAYOUT_H


#include "reportmodel.h"
#include "reportengine.h"
#include "reportdesign.h"

#include "../kcanvas/kcanvas.h"
#include "../kcanvas/canvas.h"
#include "../kcanvas/canvasdc.h"
#include "../kcanvas/canvaspdf.h"
#include "../kcanvas/canvasprintout.h"

#include <wx/timer.h>


// report spacing
const int PROP_REPORT_PAGE_SPACING  = 12;    // pixels


// forward declarations
class CompReportLayoutPage;
class CompReportLayout;


class CompReportLayoutPage
{
friend class CompReportLayout;

public:

    CompReportLayoutPage()
    {
    }
    
    virtual ~CompReportLayoutPage()
    {
    }

private:

    kcanvas::IComponentPtr m_page;
    kcanvas::IComponentPtr m_column_header;
    kcanvas::IComponentPtr m_row_header;
    wxRect m_page_rect;
};


class CompReportLayout : public kcanvas::Component
{
    XCM_CLASS_NAME("appmain.CompReportLayout")
    XCM_BEGIN_INTERFACE_MAP(CompReportLayout)
        XCM_INTERFACE_CHAIN(kcanvas::Component)
    XCM_END_INTERFACE_MAP()

public:

    CompReportLayout();
    virtual ~CompReportLayout();

    static kcanvas::IComponentPtr create();

    // IComponent
    kcanvas::IComponentPtr clone();
    void copy(kcanvas::IComponentPtr component);

public:

    // function for setting the layout engine
    void setReportLayoutEngine(const ReportLayoutEngine& engine);
    ReportLayoutEngine& getReportLayoutEngineRef();

    // function for page count and location
    int getPageCount();
    bool getPagePosByIdx(int idx, int* x, int* y);
    bool getPageIdxByPos(int x, int y, int* idx);

    // functions for laying out the component and clearing it
    void layout();
    bool refresh();
    void clear();

private:

    // event handler functions
    void onLayout(kcanvas::IEventPtr evt);
    void onPreRender(kcanvas::IEventPtr evt);
    void onRender(kcanvas::IEventPtr evt);
    void onRenderHeader(kcanvas::IEventPtr evt);
    void onTableEvent(kcanvas::IEventPtr evt);

private:

    // signal sinks
    void onReportLayoutEngineUpdated();

private:

    // functions for creating internal stock components
    kcanvas::IComponentPtr createTableColumnHeader();
    kcanvas::IComponentPtr createTableRowHeader();

    // functions for updating the table row/column headers
    kcanvas::IComponentPtr updateTableRowHeader(CompReportLayoutPage& page_info, int x_offset, int y_offset);
    kcanvas::IComponentPtr updateTableColumnHeader(CompReportLayoutPage& page_info, int x_offset, int y_offset);

    // utility function for getting the table associated
    // with a page
    kcanvas::ICompTablePtr getPageTable(kcanvas::ICompPagePtr page);

private:

    // cache of page info
    std::vector<CompReportLayoutPage> m_cache_pages;

    // layout engine
    ReportLayoutEngine m_layout_engine;

    // action table
    kcanvas::IComponentPtr m_action_table;

    // flag for rendering the row headers the first time
    bool m_first_row_header_render;
};


// helper classes for outputting results to a PDF or a printer
class ReportOutputBase : public xcm::signal_sink
{
public:

    // use signals instead of canvas events to make report output
    // more accessible for uses outside of canvas environment
    XCM_IMPLEMENT_SIGNAL2(sigOutputBegin, ReportOutputBase*, kcanvas::Properties&)
    XCM_IMPLEMENT_SIGNAL2(sigOutputEnd, ReportOutputBase*, kcanvas::Properties&)
    XCM_IMPLEMENT_SIGNAL2(sigOutputPage, ReportOutputBase*, kcanvas::Properties&)

public:

    ReportOutputBase(const ReportLayoutEngine& engine,
                     const ReportPrintInfo& print_info,
                     const wxString& location,
                     bool block = false)
    {
        ReportLayoutEngine* engine_ptr = new ReportLayoutEngine;
        *engine_ptr = engine;
        m_layout_engines.push_back(engine_ptr);

        m_print_info = print_info;
        m_location = location;
        m_block = block;
        m_cancelled = false;
    }

    ReportOutputBase(const std::vector<ReportLayoutEngine>& engines,
                     const ReportPrintInfo& print_info,
                     const wxString& location,
                     bool block = false)
    {
        std::vector<ReportLayoutEngine>::const_iterator it, it_end;
        it_end = engines.end();
        
        for (it = engines.begin(); it != it_end; ++it)
        {
            ReportLayoutEngine* engine_ptr = new ReportLayoutEngine;
            *engine_ptr = *it;
            m_layout_engines.push_back(engine_ptr);        
        }

        m_print_info = print_info;
        m_location = location;
        m_block = block;
        m_cancelled = false;
    }

    virtual ~ReportOutputBase()
    {
        std::vector<ReportLayoutEngine*>::iterator it, it_end;
        it_end = m_layout_engines.end();
        
        for (it = m_layout_engines.begin(); it != it_end; ++it)
        {
            delete *it;
        }
    }

    void cancel()
    {
        m_cancelled = true;
    }

    bool isCancelled()
    {
        return m_cancelled;
    }

protected:

    std::vector<ReportLayoutEngine*> m_layout_engines;
    ReportPrintInfo m_print_info;
    wxString m_location;
    bool m_block;
    bool m_cancelled;
};


class ReportOutputPdf : public ReportOutputBase,
                        public wxTimer
{
public:

    ReportOutputPdf(const ReportLayoutEngine& engine,
                    const ReportPrintInfo& print_info,
                    const wxString& location,
                    bool block = false);
    ReportOutputPdf(const std::vector<ReportLayoutEngine>& engines,
                    const ReportPrintInfo& print_info,
                    const wxString& location,
                    bool block = false);
    virtual ~ReportOutputPdf();

    void create();
    void Notify();

private:

    void onReportLayoutEngineUpdated();

private:

    void init();
    void destroyObject();
    bool doCreatePage();    

private:

    kcanvas::CanvasPDF* m_pdf_ptr;
    int m_pdf_output_page_idx;
};


class ReportOutputPrinter : public ReportOutputBase
{
public:

    ReportOutputPrinter(const ReportLayoutEngine& engine,
                        const ReportPrintInfo& print_info,
                        const wxString& location,
                        bool block = false);
    ReportOutputPrinter(const std::vector<ReportLayoutEngine>& engines,
                        const ReportPrintInfo& print_info,
                        const wxString& location,
                        bool block = false);
    virtual ~ReportOutputPrinter();

    void create();

private:

    void onPrint(kcanvas::IEventPtr evt);
    void onReportLayoutEngineUpdated();

private:

    void init();
    void destroyObject();
    bool doCreatePage();

private:

    kcanvas::CanvasPrintout* m_printout_ptr;
    int m_printout_output_page_idx;
};


#endif

