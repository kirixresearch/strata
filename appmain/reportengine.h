/*!
 *
 * Copyright (c) 2010-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2010-01-14
 *
 */


#ifndef H_APP_REPORTENGINE_H
#define H_APP_REPORTENGINE_H


#include "reportmodel.h"
#include "../kcanvas/kcanvas.h"


// page sizes
const int LETTER_PAGE_WIDTH = (int)(kcanvas::CANVAS_MODEL_DPI*8.5);
const int LETTER_PAGE_HEIGHT = (int)(kcanvas::CANVAS_MODEL_DPI*11.0);
const int LETTER_PAGE_LEFT_MARGIN = (int)(kcanvas::CANVAS_MODEL_DPI*0.75);
const int LETTER_PAGE_RIGHT_MARGIN = (int)(kcanvas::CANVAS_MODEL_DPI*0.75);
const int LETTER_PAGE_TOP_MARGIN = (int)(kcanvas::CANVAS_MODEL_DPI*0.75);
const int LETTER_PAGE_BOTTOM_MARGIN = (int)(kcanvas::CANVAS_MODEL_DPI*0.75);

const int A4_PAGE_WIDTH = (int)(kcanvas::CANVAS_MODEL_DPI*8.27);
const int A4_PAGE_HEIGHT = (int)(kcanvas::CANVAS_MODEL_DPI*11.69);
const int A4_PAGE_LEFT_MARGIN = (int)(kcanvas::CANVAS_MODEL_DPI*0.7874);
const int A4_PAGE_RIGHT_MARGIN = (int)(kcanvas::CANVAS_MODEL_DPI*0.7874);
const int A4_PAGE_TOP_MARGIN = (int)(kcanvas::CANVAS_MODEL_DPI*0.7874);
const int A4_PAGE_BOTTOM_MARGIN = (int)(kcanvas::CANVAS_MODEL_DPI*0.7874);


// properties that store template section, row, and data 
// row in the layout table
const wxString PROP_TABLE_TEMPLATE_SECTION_IDX  = wxT("table.template.section.idx");
const wxString PROP_TABLE_TEMPLATE_ROW_IDX      = wxT("table.template.row.idx");
const wxString PROP_TABLE_MODEL_ROW_IDX         = wxT("table.model.row.idx");

// report layout events
const wxString EVENT_REPORT_UPDATED = wxT("event.report.updated");


// forward delcarations
class ReportSection;
class ReportLayoutEngine;

class ReportCreateField;
class ReportCreateInfo;
class ReportPrintInfo;

class PageSegmentInfo;
class PageLayoutInfo;


class ReportSection
{
public:

    ReportSection();
    ReportSection(const ReportSection& c);
    virtual ~ReportSection();

    ReportSection& operator=(const ReportSection& c);
    bool operator==(const ReportSection& c) const;
    bool operator!=(const ReportSection& c) const;

    void copy(const ReportSection& c);

public:

    kcanvas::ITableModelPtr m_table_model;  // table model that contains layout used to generate report
    wxString m_name;                        // section name; used for referencing sections
    wxString m_type;                        // section type (e.g. header/footer/detail, etc)
    wxString m_group_field;                 // group field; note: groups fields accumulate across sections
    bool m_page_break;                      // section page break flag
    bool m_sort_desc;                       // section sort descending flag
    bool m_active;                          // section active flag
};


class ReportLayoutEngine : public xcm::signal_sink
{
public:

    XCM_IMPLEMENT_SIGNAL0(sigLayoutUpdated)

public:

    ReportLayoutEngine();
    ReportLayoutEngine(const ReportLayoutEngine& c);
    virtual ~ReportLayoutEngine();

    ReportLayoutEngine& operator=(const ReportLayoutEngine& data);
    bool operator==(const ReportLayoutEngine& c) const;
    bool operator!=(const ReportLayoutEngine& c) const;

    void copy(const ReportLayoutEngine& c);
    void init(const std::vector<ReportSection>& sections,
              const wxString& data_source,
              const wxString& data_filter,
              int page_width,
              int page_height,
              int margin_left,
              int margin_right,
              int margin_top,
              int margin_bottom,
              bool reset_model = true);

    bool isReady();
    void execute(bool block = false);
    void reset();
 
    kcanvas::IComponentPtr getPageByIdx(int page_idx);
    void getPageSizes(std::vector<wxRect>& pages);
    int getPageCount();

private:

    // signal sink functions for table renderer and editor
    void onLayoutTableRendererUpdated(kcanvas::Properties& props);
    void onLayoutTableEditorUpdated(kcanvas::Properties& props);
    void onModelLoaded();

private:

    kcanvas::IComponentPtr createPageLayout(int page_idx);
    kcanvas::IComponentPtr createTableLayout();

    IModelPtr createDataModel();
    IModelPtr getDataModel();
    bool hasDataModel();

    // function for initializing report variables
    void initReportVariables();

    // utility functions for cache
    void populateDataModel();
    void populateDataGroups();
    void populatePageCache();
    void populateCache();
    void clearCache();
    bool isCacheEmpty();

private:

    // page cache
    std::vector<PageLayoutInfo> m_cache_pages;

    // report sections
    std::vector<ReportSection> m_sections;

    // data model and source
    IModelPtr m_data_model;

    // data source
    wxString m_data_source;
    wxString m_data_filter;
    wxString m_data_order;

    // page size and margins
    int m_page_width;
    int m_page_height;
    int m_margin_left;
    int m_margin_right;
    int m_margin_top;
    int m_margin_bottom;
    
    // current date; report variables set when creating a report
    int m_report_variable_pagecount;    
    wxString m_report_variable_date;
    wxString m_report_variable_source;

    // block flag
    bool m_block;
};


class ReportCreateField
{
public:

    ReportCreateField()
    {
        field_name = wxEmptyString;
        caption = wxEmptyString;
        alignment = wxEmptyString;
        column_width = 0;
    }

    virtual ~ReportCreateField()
    {
    }

public:

    wxString field_name;
    wxString caption;
    wxString alignment;
    int column_width;
};


class ReportCreateInfo
{
public:

    ReportCreateInfo()
    {
        iterator = xcm::null;
        
        path = wxEmptyString;
        filter_expr = wxEmptyString;
        
        font_facename = wxEmptyString;
        font_size = 0;
    }

    virtual ~ReportCreateInfo()
    {
    }

public:

    std::vector<ReportCreateField> content_fields;
    xd::IIteratorPtr iterator;

    wxString path;
    wxString filter_expr;
    wxString sort_expr;
    
    wxString font_facename;
    int font_size;
};


class ReportPrintInfo
{
public:

    ReportPrintInfo()
    {
        m_page_width = 0;
        m_page_height = 0;
        m_print_quality = 0;
        m_print_number_copies = 0;
        m_print_min_page = 0;
        m_print_max_page = 0;
        m_print_from_page = 0;
        m_print_to_page = 0;
        m_print_all_pages = false;    
    }
    
	ReportPrintInfo(const ReportPrintInfo& c)
    {
        m_page_width = c.m_page_width;
        m_page_height = c.m_page_height;
        m_print_quality = c.m_print_quality;
        m_print_number_copies = c.m_print_number_copies;
        m_print_min_page = c.m_print_min_page;
        m_print_max_page = c.m_print_max_page;
        m_print_from_page = c.m_print_from_page;
        m_print_to_page = c.m_print_to_page;
        m_print_all_pages = c.m_print_all_pages;
    }

    ReportPrintInfo& operator=(const ReportPrintInfo& c)
    {
        m_page_width = c.m_page_width;
        m_page_height = c.m_page_height;
        m_print_quality = c.m_print_quality;
        m_print_number_copies = c.m_print_number_copies;
        m_print_min_page = c.m_print_min_page;
        m_print_max_page = c.m_print_max_page;
        m_print_from_page = c.m_print_from_page;
        m_print_to_page = c.m_print_to_page;
        m_print_all_pages = c.m_print_all_pages;
        return *this;
    }

    void setPageSize(int width, int height) { m_page_width = width; m_page_height = height; }
    void getPageSize(int* width, int* height) { *width = m_page_width; *height = m_page_height; }

    void setQuality(int quality) { m_print_quality = quality; }
    int getQuality() { return m_print_quality; }
    
    void setNumberCopies(int number_copies) { m_print_number_copies = number_copies; }
    int getNumberCopies() { return m_print_number_copies; }
    
    void setMinPage(int min_page) { m_print_min_page = min_page; }
    int getMinPage() { return m_print_min_page; }
    
    void setMaxPage(int max_page) { m_print_max_page = max_page; }
    int getMaxPage() { return m_print_max_page; }
    
    void setFromPage(int from_page) { m_print_from_page = from_page; }
    int getFromPage() { return m_print_from_page; }
    
    void setToPage(int to_page) { m_print_to_page = to_page; }
    int getToPage() { return m_print_to_page; }
    
    void setAllPages(bool all_pages) { m_print_all_pages = all_pages; }
    bool getAllPages() { return m_print_all_pages; }

private:

    int m_page_width;           // canvas model coordinates
    int m_page_height;          // canvas model coordinates
    int m_print_quality;
    int m_print_number_copies;
    int m_print_min_page;
    int m_print_max_page;
    int m_print_from_page;
    int m_print_to_page;
    bool m_print_all_pages;
};


class PageSegmentInfo
{
public:

    PageSegmentInfo()
    {
        m_section_idx = -1;
        m_row_start = 0;
        m_row_end = 0;
    }

    PageSegmentInfo(const PageSegmentInfo& c)
    {
        m_section_idx = c.m_section_idx;
        m_row_start = c.m_row_start;
        m_row_end = c.m_row_end;
    }

    PageSegmentInfo& operator=(const PageSegmentInfo& c)
    {
        m_section_idx = c.m_section_idx;
        m_row_start = c.m_row_start;
        m_row_end = c.m_row_end;
        return *this;
    }

public:

    int m_section_idx;          // the index of the section containing the 
                                // template definition for this section
    int m_row_start;            // the data model row the section start with
    int m_row_end;              // the data model row the section ends with
};


class PageLayoutInfo
{
public:

    PageLayoutInfo()
    {
    }

    virtual ~PageLayoutInfo()
    {
    }

public:

    std::vector<PageSegmentInfo> m_segments;
    wxRect m_page_rect;         // page dimensions
    wxRect m_layout_rect;       // actual space on the page to use for the layout (page minus margins)
};


#endif

