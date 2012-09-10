/*!
 *
 * Copyright (c) 2010-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2010-01-14
 *
 */


#include "appmain.h"
#include "reportengine.h"
#include "querytemplate.h"
#include "jobquery.h"

#include "../kcanvas/component.h"
#include "../kcanvas/componentpage.h"
#include "../kcanvas/componenttable.h"


// page margin tolerance
const int REPORT_MARGIN_TOLERANCE       = (int)(kcanvas::CANVAS_MODEL_DPI*0.25);


static wxString getSortExprFromGroups(const std::vector<ReportSection>& sections)
{
    wxString sort_expr = wxEmptyString;

    std::vector<ReportSection>::const_iterator it, it_end;
    it_end = sections.end();
    
    bool first = true;
    for (it = sections.begin(); it != it_end; ++it)
    {
        if (it->m_type != PROP_REPORT_GROUP_HEADER)
            continue;
            
        if (it->m_group_field.IsEmpty())
            continue;

        if (!first)
            sort_expr += wxT(",");

        wxString group_field = towx(tango::quoteIdentifier(g_app->getDatabase(), towstr(it->m_group_field)));
        sort_expr += group_field;

        if (it->m_sort_desc)
            sort_expr += wxT(" DESC");

        first = false;
    }

    return sort_expr;
}

static void layoutRows(kcanvas::ICompTablePtr target,
                       kcanvas::ITableModelPtr source,
                       int section_idx,
                       int model_start,
                       int model_rows,
                       int* layout_row_idx)
{
    // find out the starting and ending layout column indexes
    int target_col1 = 0;
    int target_col2 = target->getColumnCount() - 1;

    // calculate the number of data rows we have, and the number of
    // template (source) rows we have to replicate for each data row,
    // and the total number of layout (target) rows that we'll have
    // as a result
    int source_row1 = 0;                                         // first row of table
    int source_row2 = source->getRowCount() - 1;                 // last row of table
    int source_count = (source_row2 - source_row1 + 1);

    // if we don't have any source rows (section is collapsed), 
    // we're done
    if (source_count <= 0)
        return;

    int target_row1 = *layout_row_idx;
    int target_row2 = *layout_row_idx + model_rows*source_count - 1;

    // increment the layout row index
    *layout_row_idx += (target_row2 - target_row1 + 1);

    // for each model (data) row, copy the current template
    // table row merged cells, text values and row sizes to
    // the corresponding layout table cells/rows
    int model_idx;
    int target_idx = target_row1;
    for (model_idx = model_start; model_idx <= (model_start + model_rows - 1); ++model_idx)
    {
        // copy the merged cells from the source table to the 
        // target table
        std::vector<kcanvas::CellRange> merged_cells;
        source->getMergedCells(merged_cells);
        
        std::vector<kcanvas::CellRange>::iterator it, it_end;
        it_end = merged_cells.end();
        
        for (it = merged_cells.begin(); it != it_end; ++it)
        {
            kcanvas::CellRange target_merged_cell(*it);
            target_merged_cell.shift(target_idx, 0);
            target->mergeCells(target_merged_cell);
        }    
    
        int source_idx;
        for (source_idx = source_row1; source_idx <= source_row2; ++source_idx)
        {            
            // for each cell in the target table, put a placeholder text 
            // value corresponding to any template table cells that have
            // a text value; the actual value that shows up in the report 
            // is substituted at the last minute based on the template table
            // and the data; however, in order to ensure that the layout 
            // table cells draw in the correct order, we have to copy the
            // text from the template table to the layout table before
            // actually rendering (and replacing) the text in the layout
            // view so that the text in cells overflows other cells that
            // are blank; in addition to this, we need to populate the
            // mime type ahead of time to ensure the proper renderer is
            // used
            int col_idx;
            for (col_idx = target_col1; col_idx <= target_col2; ++col_idx)
            {
                kcanvas::CellRange source_cell(source_idx, col_idx);
                kcanvas::CellRange target_cell(target_idx, col_idx);
                
                // set the mime type
                kcanvas::PropertyValue mime;
                source->getCellProperty(source_cell, kcanvas::PROP_CONTENT_MIMETYPE, mime);
                
                if (!mime.getString().IsEmpty())
                    target->setCellProperty(target_cell, kcanvas::PROP_CONTENT_MIMETYPE, mime);
                
                // set the content
                kcanvas::PropertyValue value;
                source->getCellProperty(source_cell, kcanvas::PROP_CONTENT_VALUE, value);
                
                if (!value.getString().IsEmpty())
                    target->setCellProperty(target_cell, kcanvas::PROP_CONTENT_VALUE, value);
            }

            // set the target row size
            int row_size = source->getRowSize(source_idx);
            target->setRowSize(target_idx, row_size);
            
            // set the source section and row index used to populate the 
            // data values for this particular row; also, set the data 
            // model row index to use when filling out the data values 
            // for this particular row
            kcanvas::CellRange r(target_idx, target_col1, target_idx, target_col2);
            target->setCellProperty(r, PROP_TABLE_TEMPLATE_SECTION_IDX, section_idx);
            target->setCellProperty(r, PROP_TABLE_TEMPLATE_ROW_IDX, source_idx);
            target->setCellProperty(r, PROP_TABLE_MODEL_ROW_IDX, model_idx);
            
            // increment the target row
            ++target_idx;
        }
    }
}

static bool layoutPage(IModelPtr model,
                       std::vector<ReportSection>& sections,
                       PageLayoutInfo& info,
                       int* section_idx,
                       int* row_idx)
{
    // note: this function takes a reference to the report sections, a data 
    // model, a reference to PageInfo, a pointer to the starting section we're 
    // on, a pointer to the starting row we're on; it calculates the layout for 
    // this page based on information in the page info, fills out the page info 
    // with the layout information, sets the section to the section after the last 
    // section that was used, and sets the row to the row after the last row that 
    // was used; the function returns true if the layout was successful; false 
    // otherwise

    // if the starting section index is greater than the available sections,
    // we're done
    if (*section_idx >= (int)sections.size())
        return false;

    // if we don't have a data model, create a single default page, and set the
    // section index to the end, so the next time we come through, we're done
    if (model.isNull())
    {
        *section_idx = sections.size();
        return true;
    }

    // if the starting row is already past the end of the table, we don't have 
    // any rows, so create a single default page and set the section index to 
    // the end so that the next time we come through, we're done
    if (model->eof())
    {
        *section_idx = sections.size();
        return true;
    }

    // calculate the layout height, which is the space available on the
    // page to layout the rows (page height minus bottom and top margin,
    // minus space for page headers and footers)
    int page_header = 0;
    int page_footer = 0;

    std::vector<ReportSection>::iterator it, it_end;
    it_end = sections.end();
    for (it = sections.begin(); it != it_end; ++it)
    {
        int section_row_count = it->m_table_model->getRowCount();
        int section_height = it->m_table_model->getRowPosByIdx(section_row_count);

        if (it->m_type == PROP_REPORT_PAGE_HEADER)
            page_header += section_height;

        if (it->m_type == PROP_REPORT_PAGE_FOOTER)
            page_footer += section_height;
    }

    int layout_height = info.m_layout_rect.height - page_header - page_footer;
    
    // cycle through the rows, laying out the groups until we encounter
    // a break condition; use whatever row and group the model are currently 
    // on as the start row and the start group
    PageSegmentInfo group_segment;
    group_segment.m_row_start = *row_idx;
    group_segment.m_row_end = *row_idx;

    PageSegmentInfo detail_segment;
    detail_segment.m_row_start = *row_idx;
    detail_segment.m_row_end = *row_idx;
    
    int total_height = 0;
    int section_count = 0;
    bool save_detail_segment = false;
    bool report_beginning = false;
    bool report_ended = false;
    bool page_break = false;
        
    while (1)
    {
        // note: loop breaks when the used vertical space exceeds the
        // available space on the page or when we run out of rows; in
        // the later case, the report_ended flag is set to true and
        // the loop does one more pass to add the report footer before
        // breaking; note: we require at least one section (tracked
        // in section count) to appear on a page so that we don't enter 
        // into an infinite loop trying to accomodate sections that
        // are larger than a page

        // if we're on the first row, set the report_beginning flag
        // to true; the report ended flag is set after we advance
        // past the last row
        report_beginning = (*row_idx == 1);

        // for each row, iterate through the sections, starting at the 
        // section where we last left off
        std::vector<ReportSection>::iterator it, it_end;
        it_end = sections.end();
        for (it = sections.begin() + *section_idx; it != it_end; ++it)
        {
            // set the page break to false
            page_break = false;
        
            // set the flags for the beginning and ending of a group
            bool group_beginning = false;
            bool group_ending = false;

            // calculate the section height from the template table model
            int section_row_count = it->m_table_model->getRowCount();
            int section_height = it->m_table_model->getRowPosByIdx(section_row_count);

            // set the model group name so the model returns the correct 
            // beginning-of-group (bog) and end-of-group (eog) values for 
            // the current group
            if (model->setGroup(it->m_name))
            {
                group_beginning = model->bog();
                group_ending = model->eog();
            }

            // if it's the beginning of the report, add the report section
            if (it->m_type == PROP_REPORT_HEADER && 
                report_beginning && !report_ended)
            {
                // increment the section count
                section_count++;
            
                // set the group segment information
                group_segment.m_section_idx = (it - sections.begin());
                group_segment.m_row_start = *row_idx;
                group_segment.m_row_end = *row_idx;

                // set the detail start/end record; we'll adjust the 
                // end later
                detail_segment.m_row_start = *row_idx;
                detail_segment.m_row_end = *row_idx;

                // save the section and set the total height
                info.m_segments.push_back(group_segment);
                total_height += section_height;

                // if the section page break is set, set the
                // local page break to true
                if (it->m_page_break)
                    page_break = true;
            }

            // if it's a group header section and it's the beginning of
            // a group, add the group header
            if (it->m_type == PROP_REPORT_GROUP_HEADER && 
                group_beginning && !report_ended)
            {
                // increment the section count
                section_count++;
            
                // if we're on group header and we have previous detail
                // to save, then save it
                if (save_detail_segment)
                {
                    info.m_segments.push_back(detail_segment);
                    save_detail_segment = false;
                }

                // set the group segment information            
                group_segment.m_section_idx = (it - sections.begin());
                group_segment.m_row_start = *row_idx;
                group_segment.m_row_end = *row_idx;

                // save the section and set the total height
                info.m_segments.push_back(group_segment);
                total_height += section_height;
                
                // if the section page break is set, set the
                // local page break to true
                if (it->m_page_break)
                    page_break = true;
            }

            // if it's a detail section, always add it unless we're
            // past the end of the report
            if (it->m_type == PROP_REPORT_DETAIL && 
                !report_ended)
            {
                // increment the section count
                section_count++;
            
                // if we're beginning a new segment to save, set the new detail 
                // start/end record; we'll adjust the end later
                if (!save_detail_segment)
                {
                    detail_segment.m_row_start = *row_idx;
                    detail_segment.m_row_end = *row_idx;
                    save_detail_segment = true;
                }

                // set the detail segment information; only set the end
                // record since we want the start record to correspond
                // to the beginning of the report/page/group
                detail_segment.m_section_idx = (it - sections.begin());
                detail_segment.m_row_end = *row_idx;

                // set the total height
                total_height += section_height;
                
                // if the section page break is set, set the
                // local page break to true
                if (it->m_page_break)
                    page_break = true;
            }

            // if it's a group footer section and it's the end of a 
            // group, add the group footer
            if (it->m_type == PROP_REPORT_GROUP_FOOTER &&
                group_ending && !report_ended)
            {
                // increment the section count
                section_count++;
            
                // if we're on group footer and we have previous detail
                // to save, then save it
                if (save_detail_segment)
                {
                    info.m_segments.push_back(detail_segment);
                    save_detail_segment = false;
                }
            
                // set the group segment information            
                group_segment.m_section_idx = (it - sections.begin());
                group_segment.m_row_start = *row_idx;
                group_segment.m_row_end = *row_idx;
                
                // save the section and set the total height
                info.m_segments.push_back(group_segment);
                total_height += section_height;
                
                // if the section page break is set, set the
                // local page break to true
                if (it->m_page_break)
                    page_break = true;
            }

            // if it's the end of the report, add the report footer
            if (it->m_type == PROP_REPORT_FOOTER && 
                report_ended)
            {
                // increment the section count
                section_count++;
            
                // if we're on group footer and we have previous detail
                // to save, then save it
                if (save_detail_segment)
                {
                    info.m_segments.push_back(detail_segment);
                    save_detail_segment = false;
                }
            
                // set the group segment information; use the last
                // row of the report as the start and end row of
                // the report, which at this point, is (*row_idx - 1)
                // since we this section is only added after the
                // we're past the last actual row of the report
                group_segment.m_section_idx = (it - sections.begin());
                group_segment.m_row_start = *row_idx - 1;
                group_segment.m_row_end = *row_idx - 1;
                
                // save the section and set the total height
                info.m_segments.push_back(group_segment);
                total_height += section_height;
                
                // if the section page break is set, set the
                // local page break to true; note: it's not
                // really necessary to include this since we
                // wouldn't normally want to page break at
                // the very end of a report; however, we
                // include it here for uniformity
                if (it->m_page_break)
                    page_break = true;
            }
            
            // if there are detail rows to save and the report is ending,
            // then save the detail rows; this should only happen when
            // there is no report or page footer
            if (report_ended && save_detail_segment)
            {
                info.m_segments.push_back(detail_segment);
                save_detail_segment = false;
            }

            if (page_break)
            {
                // advance the section
                *section_idx = (it - sections.begin() + 1);
            
                // if we have more sections left over, simply
                // return true for now, and we'll pick up where
                // we left off next time around;
                if (*section_idx < (int)sections.size())
                    return true;

                // however, if the page break is on the very
                // last section, we need to advance the row
                // before we can return true
                break;
            }

            // if the total height exceeds the available height, then handle
            // the last segment and use it to set the return values for
            // section_idx and row_idx
            if (total_height > layout_height && section_count > 1)
            {
                // if save_detail_segment is true, we're on the detail; handle
                // it in a special way
                if (save_detail_segment)
                {
                    // if it's a detail segment with more than one row, divide the 
                    // detail segment into two parts: the part that's within the 
                    // valid height, and the part that caused us to go over, then 
                    // save both parts; this allows us to handle the part that caused 
                    // us to go over the same as the groups
                    int row_start_idx = detail_segment.m_row_start;
                    int row_end_idx = detail_segment.m_row_end;

                    if (row_start_idx == row_end_idx)
                    {
                        // the segment has a single row; simply save the segment
                        info.m_segments.push_back(detail_segment);
                    }
                    else
                    {
                        // the segment has multiple rows; divide the segment and
                        // save both parts
                        PageSegmentInfo detail_segment_part1 = detail_segment;
                        PageSegmentInfo detail_segment_part2 = detail_segment;
                    
                        detail_segment_part1.m_row_end = row_end_idx - 1;
                        detail_segment_part2.m_row_start = row_end_idx;
                        detail_segment_part2.m_row_end = row_end_idx;
                        
                        info.m_segments.push_back(detail_segment_part1);
                        info.m_segments.push_back(detail_segment_part2);
                    }
                }

                // set the section index
                *section_idx = it - sections.begin();

                // set the next row index from the group we're removing,
                // then remove the group
                if (!info.m_segments.empty())
                {
                    PageSegmentInfo* next_start_section = &info.m_segments.back();
                    *row_idx = next_start_section->m_row_start;
                    info.m_segments.pop_back();
                }

                return true;
            }
        }

        // reset the section index
        *section_idx = 0;

        // advance the data model
        *row_idx = *row_idx + 1;
        model->skip(1);
        
        // if the report ended flag is true, stop looping
        if (report_ended)
            break;
        
        // if we're past the last row, set the row_idx to the
        // last row so any segments triggered by the report end
        // will use the last row of the data for the row_idx;
        // set the report_ended flag to true and do one more pass 
        // to add the report footer
        if (model->eof())
            report_ended = true;
            
        // if we're on a page break and the report hasn't ended,
        // return true
        if (page_break && !report_ended)
            return true;
    }
    
    // if there are segments left-over, it's a partially filled page;
    // set the section index to index after the last index and use
    // the existing row index, which should already correspond to
    // the row after the last row
    if (info.m_segments.size() > 0)
    {
        *section_idx = sections.size();
        return true;
    }
    
    // we have no more pages, so return false
    return false;
}

static void layoutReport(const PageLayoutInfo& page_info,
                         IModelPtr model,
                         std::vector<ReportSection>& sections,
                         std::vector<PageLayoutInfo>& pages)
{
    // clear the pages
    pages.clear();

    // initialize the section and row index for values corresponding
    // to the very beginning of the report
    int section_idx = 0;
    int row_idx = 1;

    // layout each page until there are no more pages to layout; add 
    // the result of each page layout operation to the page info cache;
    // since we're laying out all the pages in the report, use the layout 
    // function in non-absolute mode, which means that the iterator
    // won't reset each time a new page is layed out, but will pick
    // up where the previous iterator left off, saving significant
    // layout time on larger reports
    if (!model.isNull())
        model->goFirst();

    while (1)
    {
        // copy the page info
        PageLayoutInfo info = page_info;

        // layout the page; if the function returns false, we have no 
        // more pages to layout, and we're done
        if (!layoutPage(model,
                        sections,
                        info,
                        &section_idx,
                        &row_idx))
        {
            return;
        }
    
        // add the page info to the cache
        pages.push_back(info);
    }
}

ReportSection::ReportSection()
{
}

ReportSection::ReportSection(const ReportSection& c)
{
    m_table_model = c.m_table_model;
    m_name = c.m_name;
    m_type = c.m_type;
    m_group_field = c.m_group_field;
    m_page_break = c.m_page_break;
    m_sort_desc = c.m_sort_desc;
    m_active = c.m_active;
}

ReportSection::~ReportSection()
{
}

ReportSection& ReportSection::operator=(const ReportSection& c)
{
    m_table_model = c.m_table_model;
    m_name = c.m_name;
    m_type = c.m_type;
    m_group_field = c.m_group_field;
    m_page_break = c.m_page_break;
    m_sort_desc = c.m_sort_desc;
    m_active = c.m_active;

    return *this;
}

bool ReportSection::operator==(const ReportSection& c) const
{
    if (this == &c)
        return true;

    if (m_table_model != c.m_table_model)
        return false;

    if (m_name != c.m_name)
        return false;

    if (m_type != c.m_type)
        return false;

    if (m_group_field != c.m_group_field)
        return false;
        
    if (m_page_break != c.m_page_break)
        return false;
        
    if (m_sort_desc != c.m_sort_desc)
        return false;
        
    if (m_active != c.m_active)
        return false;

    return true;
}

bool ReportSection::operator!=(const ReportSection& c) const
{
    return !(*this == c);
}

void ReportSection::copy(const ReportSection& c)
{
    // note: creates a deep copy of the report section
    // so that the pointer to the table model is deep copied
    m_table_model = xcm::null;
    if (c.m_table_model.isOk())
        m_table_model = c.m_table_model->clone();
    
    m_name = c.m_name;
    m_type = c.m_type;
    m_group_field = c.m_group_field;
    m_page_break = c.m_page_break;
    m_sort_desc = c.m_sort_desc;
    m_active = c.m_active;
}


ReportLayoutEngine::ReportLayoutEngine()
{
    // create a data model
    m_data_model = createDataModel();

    m_page_width = 0;
    m_page_height = 0;
    m_margin_left = 0;
    m_margin_right = 0;
    m_margin_top = 0;
    m_margin_bottom = 0;

    m_block = false;
}

ReportLayoutEngine::ReportLayoutEngine(const ReportLayoutEngine& c)
{
    // report sections
    m_sections = c.m_sections;

    // data source
    m_data_source = c.m_data_source;
    m_data_filter = c.m_data_filter;
    m_data_order = c.m_data_order;

    // page size and margins
    m_page_width = c.m_page_width;
    m_page_height = c.m_page_height;
    m_margin_left = c.m_margin_left;
    m_margin_right = c.m_margin_right;
    m_margin_top = c.m_margin_top;
    m_margin_bottom = c.m_margin_bottom;
}

ReportLayoutEngine::~ReportLayoutEngine()
{
}

ReportLayoutEngine& ReportLayoutEngine::operator=(const ReportLayoutEngine& c)
{
    if (this == &c)
        return *this;

    // report sections
    m_sections = c.m_sections;

    // data source
    m_data_source = c.m_data_source;
    m_data_filter = c.m_data_filter;
    m_data_order = c.m_data_order;

    // page size and margins
    m_page_width = c.m_page_width;
    m_page_height = c.m_page_height;
    m_margin_left = c.m_margin_left;
    m_margin_right = c.m_margin_right;
    m_margin_top = c.m_margin_top;
    m_margin_bottom = c.m_margin_bottom;

    // reset the engine to clear out the cache and data model
    reset();
    return *this;
}

bool ReportLayoutEngine::operator==(const ReportLayoutEngine& c) const
{
    if (this == &c)
        return true;

    // report sections
    if (m_sections != c.m_sections)
        return false;

    // data source
    if (m_data_source != c.m_data_source)
        return false;
    if (m_data_filter != c.m_data_filter)
        return false;
    if (m_data_order != c.m_data_order)
        return false;

    // page size and margins
    if (m_page_width != c.m_page_width)
        return false;
    if (m_page_height != c.m_page_height)
        return false;
    if (m_margin_left != c.m_margin_left)
        return false;
    if (m_margin_right != c.m_margin_right)
        return false;
    if (m_margin_top != c.m_margin_top)
        return false;
    if (m_margin_bottom != c.m_margin_bottom)
        return false;

    return true;
}

bool ReportLayoutEngine::operator!=(const ReportLayoutEngine& c) const
{
    return !(*this == c);
}

void ReportLayoutEngine::copy(const ReportLayoutEngine& c)
{
    // clear the sections
    m_sections.clear();

    // performs a deep copy of the input engine, so that the
    // report section table model pointers are deep copied
    std::vector<ReportSection>::const_iterator it, it_end;
    it_end = c.m_sections.end();

    for (it = c.m_sections.begin(); it != it_end; ++it)
    {
        ReportSection section;
        section.copy(*it);
        m_sections.push_back(section); 
    }

    m_data_source = c.m_data_source;
    m_data_filter = c.m_data_filter;
    m_data_order = c.m_data_order;
    m_page_width = c.m_page_width;
    m_page_height = c.m_page_height;
    m_margin_left = c.m_margin_left;
    m_margin_right = c.m_margin_right;
    m_margin_top = c.m_margin_top;
    m_margin_bottom = c.m_margin_bottom;

    // reset the engine to clear out the cache and data model
    reset();
}

void ReportLayoutEngine::init(const std::vector<ReportSection>& sections,
          const wxString& data_source,
          const wxString& data_filter,
          int page_width,
          int page_height,
          int margin_left,
          int margin_right,
          int margin_top,
          int margin_bottom,
          bool reset_model)
{
    if (reset_model)
        reset();

    // clear the sections
    m_sections.clear();

    // add the non-active sections
    std::vector<ReportSection>::const_iterator it, it_end;
    it_end = sections.end();

    for (it = sections.begin(); it != it_end; ++it)
    {
        // if the section isn't active, move on
        if (!it->m_active)
            continue;

        // save the section to the cache
        m_sections.push_back(*it);
    }

    // set the data source
    m_data_source = data_source;
    m_data_filter = data_filter;

    // set the data order
    m_data_order = getSortExprFromGroups(sections);

    // set the page dimensions and margins
    m_page_width = page_width;
    m_page_height = page_height;
    m_margin_left = margin_left;
    m_margin_right = margin_right;
    m_margin_top = margin_top;
    m_margin_bottom = margin_bottom;
}

bool ReportLayoutEngine::isReady()
{
    if (!hasDataModel())
        return false;

    if (!getDataModel()->isLoaded())
        return false;

    if (isCacheEmpty())
        return false;

    return true;
}

void ReportLayoutEngine::execute(bool block)
{
    if (!isCacheEmpty())
        return;

    // set the block flag
    m_block = block;

    // if the data model is ready, this will trigger
    // the layout; if the data model is not ready, this
    // will trigger a job, which will then call
    // populateCache() to finish the layout
    populateCache();
}

void ReportLayoutEngine::reset()
{
    // clear the cache and reset the data model
    clearCache();
    getDataModel()->setQuery(wxT(""));
}

kcanvas::IComponentPtr ReportLayoutEngine::getPageByIdx(int page_idx)
{
    return createPageLayout(page_idx);
}

void ReportLayoutEngine::getPageSizes(std::vector<wxRect>& pages)
{
    pages.clear();
    pages.reserve(m_cache_pages.size());
    
    std::vector<PageLayoutInfo>::iterator it, it_end;
    it_end = m_cache_pages.end();
    
    for (it = m_cache_pages.begin(); it != m_cache_pages.end(); ++it)
    {
        pages.push_back(it->m_page_rect);
    }
}

int ReportLayoutEngine::getPageCount()
{
    return m_cache_pages.size();
}

void ReportLayoutEngine::onLayoutTableRendererUpdated(kcanvas::Properties& props)
{
    // note: at this point, the layout table only has the merged cells 
    // and the text from the template table; we need to add the other 
    // template table properties for the current cell we're rendering

    // first, update the data model with the report variables, which we
    // passed through the cell properties
    kcanvas::PropertyValue page_number, page_count, current_date, data_source;

    props.get(PROP_REPORT_PAGE_NUMBER, page_number);
    props.get(PROP_REPORT_PAGE_COUNT, page_count);
    props.get(PROP_REPORT_CURRENT_DATE, current_date);
    props.get(PROP_REPORT_DATA_SOURCE, data_source);

    getDataModel()->addProperty(PROP_REPORT_PAGE_NUMBER, page_number);
    getDataModel()->addProperty(PROP_REPORT_PAGE_COUNT, page_count);
    getDataModel()->addProperty(PROP_REPORT_CURRENT_DATE, current_date);
    getDataModel()->addProperty(PROP_REPORT_DATA_SOURCE, data_source);   

    // next, replace the properties of the layout cell with the 
    // corresponding properties of the template table
    kcanvas::PropertyValue section_idx, model_row, row, col;
    if (!props.get(PROP_TABLE_TEMPLATE_SECTION_IDX, section_idx))
        return;

    if (!props.get(PROP_TABLE_MODEL_ROW_IDX, model_row))
        return;

    if (!props.get(PROP_TABLE_TEMPLATE_ROW_IDX, row))
        return;
        
    if (!props.get(kcanvas::PROP_CELL_COLUMN, col))
        return;

    if (section_idx.getInteger() < 0)
        return;

    wxString section_name;
    kcanvas::ITableModelPtr section_table_model;

    section_name = m_sections[section_idx.getInteger()].m_name;
    section_table_model = m_sections[section_idx.getInteger()].m_table_model;

    if (section_table_model.isNull())
        return;

    // TODO: currently using eval() since this accounts for
    // border properties while getCellProperties() does not
    //kcanvas::CellRange cell(row.getInteger(), col.getInteger());
    //template_table->getCellProperties(cell, props);
    section_table_model->eval(row.getInteger(), col.getInteger(), props);

    // if the mime type is something besides text, we're done
    kcanvas::PropertyValue mime_type;
    if (props.get(kcanvas::PROP_CONTENT_MIMETYPE, mime_type) &&
        mime_type.getString() != wxT("plain/text"))
    {
        return;
    } 

    // finally, get the content property of the cell and if it's
    // a formula, replace the value in the cell with the value
    // of the formula
    kcanvas::PropertyValue value;
    props.get(kcanvas::PROP_CONTENT_VALUE, value);
    wxString text = value.getString();

    if (text.Length() <= 0)
        return;

    // see if the text value is a formula; if not, we're done
    text.Trim(true);
    text.Trim(false);
    if (text.Left(1) != wxT("="))
        return;

    // trim the leading character; if there's nothing left, treat
    // the "=" as a symbol
    text = text.Mid(1);
    if (text.Length() == 0)
        return;

    // if we don't have a model, we're done
    if (!hasDataModel())
        return;

    // set the model group based on the section
    getDataModel()->setGroup(section_name);

    // evaluate the formula; if the formula is valid, set the
    // text property with the results; otherwise, set the cell
    // to a blank value
    wxString result;
    if (getDataModel()->eval(model_row.getInteger(), text, result))
        props.set(kcanvas::PROP_CONTENT_VALUE, result);
         else
        props.set(kcanvas::PROP_CONTENT_VALUE, wxT(""));
}

void ReportLayoutEngine::onModelLoaded()
{
    // if the data is now loaded, populate
    // the rest of the cache
    populateCache();
}

void ReportLayoutEngine::onLayoutTableEditorUpdated(kcanvas::Properties& props)
{
}

kcanvas::IComponentPtr ReportLayoutEngine::createPageLayout(int page_idx)
{
    // create a layout page with a default size
    kcanvas::ICompPagePtr layout_page = kcanvas::CompPage::create();
    kcanvas::IComponentPtr comp_layout_page = layout_page;

    comp_layout_page->setSize(LETTER_PAGE_WIDTH,
                              LETTER_PAGE_HEIGHT);
    layout_page->setMargins(LETTER_PAGE_LEFT_MARGIN,
                            LETTER_PAGE_RIGHT_MARGIN,
                            LETTER_PAGE_TOP_MARGIN,
                            LETTER_PAGE_BOTTOM_MARGIN);

    // if the page is outside the range of the page cache,
    // return the stock page
    if (page_idx < 0 || page_idx >= (int)m_cache_pages.size())
        return comp_layout_page;

    // used throughout
    PageLayoutInfo page_info = m_cache_pages[page_idx];
    std::vector<PageSegmentInfo>::iterator it_segment, it_segment_end;
    it_segment_end = page_info.m_segments.end();

    // set the page size and margins from the page info
    comp_layout_page->setSize(page_info.m_page_rect.width, 
                              page_info.m_page_rect.height);
    layout_page->setMargins(page_info.m_layout_rect.x, 
                            page_info.m_layout_rect.y, 
                            page_info.m_page_rect.width - page_info.m_layout_rect.width - page_info.m_layout_rect.x,
                            page_info.m_page_rect.height - page_info.m_layout_rect.height - page_info.m_layout_rect.y);

    // create a layout table
    kcanvas::ICompTablePtr layout_table = createTableLayout();
    kcanvas::IComponentPtr comp_layout_table = layout_table;

    // pass the report variables through to the cell properties; this will
    // enable us to access them in the cell-renderer callback right when
    // we need to update the formulas
    layout_table->addCellProperty(PROP_REPORT_PAGE_NUMBER, page_idx + 1);
    layout_table->addCellProperty(PROP_REPORT_PAGE_COUNT, m_report_variable_pagecount);
    layout_table->addCellProperty(PROP_REPORT_CURRENT_DATE, m_report_variable_date);
    layout_table->addCellProperty(PROP_REPORT_DATA_SOURCE, m_report_variable_source);

    // make sure the layout page and tables are empty
    comp_layout_page->removeAll();
    comp_layout_table->removeAll();

    // set the size of the table to the available layout area on
    // the page to which we're going to add it
    comp_layout_table->setOrigin(page_info.m_layout_rect.x, page_info.m_layout_rect.y);
    comp_layout_table->setSize(page_info.m_layout_rect.width, page_info.m_layout_rect.height);

    // set the number of columns in the layout and resize those
    // columns that are visible on the page; the number of columns
    // on the page is set so that the column that intersects
    // the right margin is cut off if it goes 1/4" past the margin;
    // get the table from the first section
    kcanvas::ITableModelPtr template_table_model = m_sections[0].m_table_model;
    int layout_width = comp_layout_table->getWidth() + REPORT_MARGIN_TOLERANCE;
    int col_count = template_table_model->getColumnIdxByPos(layout_width);
    layout_table->setColumnCount(col_count);

    int col_idx;
    int total_width = 0;

    for (col_idx = 0; col_idx < col_count; ++col_idx)
    {
        int col_width = template_table_model->getColumnSize(col_idx);
        layout_table->setColumnSize(col_idx, col_width);

        total_width += col_width;
        if (total_width > layout_width)
            break;
    }

    // at this point, we don't know the number of rows on the page, 
    // because it depends on the number of times a particular template 
    // section appears on the page, which is page/data-dependent; for
    // now set the row count to a very large value so we don't clip
    // properties when we set the table layout properties in the
    // layout function below
    layout_table->setRowCount(INT_MAX);

    // add rows to the layout table based on the segments stored in
    // page info cache
    int layout_row_idx = 0;
    
    // flag whether or not to draw page header and footer; set true unless
    // the report header goes to the end of the first page
    bool add_page_headerfooter = true;

    // find the first and last model rows on the page;
    // these will be used for the page header and footer
    // model rows
    int first_model_idx = -1;
    int last_model_idx = -1;

    // if we're on the first page, add the report header
    for (it_segment = page_info.m_segments.begin();
          it_segment != it_segment_end; ++it_segment)
    {
        if (first_model_idx == -1 || it_segment->m_row_start < first_model_idx)
            first_model_idx = it_segment->m_row_start;
            
        if (last_model_idx == -1 || it_segment->m_row_end > last_model_idx)
            last_model_idx = it_segment->m_row_end;
    }

    // if we're on the first page, add the report header
    for (it_segment = page_info.m_segments.begin();
          it_segment != it_segment_end; ++it_segment)
    {    
        // get the section and the number of model rows
        int section_idx = it_segment->m_section_idx;
        int model_row_idx = it_segment->m_row_start;
        int model_rows = it_segment->m_row_end - it_segment->m_row_start + 1;
        ReportSection section = m_sections[section_idx];

        // if we're on the report header, add it
        if (section.m_type != PROP_REPORT_HEADER)
            continue;

        layoutRows(layout_table,
                   section.m_table_model,
                   section_idx,
                   model_row_idx,
                   model_rows,
                   &layout_row_idx);

        // calculate if we're at the end of the first page; if so,
        // don't draw the page header and page footer
        int bottom_table = layout_table->getRowPosByIdx(layout_row_idx + 1);
        if (bottom_table >= page_info.m_layout_rect.height)
            add_page_headerfooter = false;
    }

    // add the page header
    std::vector<ReportSection>::iterator it_section, it_end_section;
    it_end_section = m_sections.end();
    for (it_section = m_sections.begin();
          it_section != it_end_section; ++it_section)
    {
        // if we're not on the page header or we're not
        // adding the page header, move on
        if (it_section->m_type != PROP_REPORT_PAGE_HEADER ||
            !add_page_headerfooter)
        {
            continue;
        }

        // add the page header
        layoutRows(layout_table,
                   it_section->m_table_model,
                   it_section - m_sections.begin(),
                   first_model_idx,                   // use model row at start of page                 
                   1,                                 // single model row to copy the header once
                   &layout_row_idx);
    }

    // add the report content
    for (it_segment = page_info.m_segments.begin();
          it_segment != it_segment_end; ++it_segment)
    {    
        // get the section and the number of model rows
        int section_idx = it_segment->m_section_idx;
        int model_row_idx = it_segment->m_row_start;
        int model_rows = it_segment->m_row_end - it_segment->m_row_start + 1;
        ReportSection section = m_sections[section_idx];        

        // if we're on the report header, we've already added
        // it, so move on
        if (section.m_type == PROP_REPORT_HEADER)
            continue;

        // add the report content
        layoutRows(layout_table,
                   section.m_table_model,
                   section_idx,
                   model_row_idx,
                   model_rows,
                   &layout_row_idx);
    }

    // add the page footer
    it_end_section = m_sections.end();
    for (it_section = m_sections.begin();
          it_section != it_end_section; ++it_section)
    {
        // if we're not on the page footer or we're not
        // not adding the page footer, move on
        if (it_section->m_type != PROP_REPORT_PAGE_FOOTER ||
            !add_page_headerfooter)
        {
            continue;
        }

        // the page footer neeeds to go at the very bottom of the
        // page, so calculate the position of the header from the
        // bottom of the page, and find the row corresponding to 
        // this position; then, set the layout_row_idx to the row
        // after the row we adjusted, and adjust the row height of 
        // the row right before the footer so that the bottom of
        // this row is right where the footer needs to start

        // TODO: we're assuming one page footer here, which is fine 
        // with our current interface; however, if we support multiple 
        // footers, this won't work because we're calculating the footer 
        // position based on the location of a single footer height,
        // not the cummulative height
        
        // calculate the approximate footer position, which is
        // the bottom of the table minus the size of the footer;
        // note: we can use the section size directly rather than
        // having to layout the rows first because the footer
        // doesn't repeat like the content (see TODO: above)
        int section_row_count = it_section->m_table_model->getRowCount();
        int section_height = it_section->m_table_model->getRowPosByIdx(section_row_count);
        int table_bottom = comp_layout_table->getHeight();
        int footer_pos = table_bottom - section_height;
        
        // calculate the row index at the footer position and
        // set the footer to start immediately after this row
        int row_idx = layout_table->getRowIdxByPos(footer_pos);
        layout_row_idx = row_idx + 1;
        
        // adjust the height of the row right before the header
        // so that the start of the footer plus the height of
        // the footer is at the same as the position at the
        // end of the table, which is also the position of the
        // bottom margin of the page
        int current_row_size = layout_table->getRowSize(row_idx);
        int current_footer_pos = layout_table->getRowPosByIdx(layout_row_idx);
        int new_row_size = current_row_size - (current_footer_pos - footer_pos);
        layout_table->setRowSize(row_idx, new_row_size);

        // layout the footer
        layoutRows(layout_table,
                   it_section->m_table_model,        
                   it_section - m_sections.begin(),
                   last_model_idx,                    // use model row at end of page
                   1,                                 // single model row to copy the header once
                   &layout_row_idx);
    }

    // now we know the number of rows on the page, so set the 
    // calculated number of rows   
    layout_table->setRowCount(layout_row_idx);

    // add the table to the page and return the page
    comp_layout_page->add(comp_layout_table);
    return comp_layout_page;
}

kcanvas::IComponentPtr ReportLayoutEngine::createTableLayout()
{
    // create a table
    kcanvas::IComponentPtr comp;
    comp = kcanvas::CompTable::create();

    kcanvas::ICompTablePtr table;
    table = comp;

    // turn off the grid lines for the layout table
    table->setPreference(kcanvas::CompTable::prefHorzGridLines, false);
    table->setPreference(kcanvas::CompTable::prefVertGridLines, false);

    // turn off ability to select and resize rows and columns from 
    // the first row or column of the layout table
    table->setPreference(kcanvas::CompTable::prefSelectRows, false);
    table->setPreference(kcanvas::CompTable::prefSelectColumns, false);
    table->setPreference(kcanvas::CompTable::prefResizeRows, false);
    table->setPreference(kcanvas::CompTable::prefResizeColumns, false);

    // turn off the cursor by default
    table->setPreference(kcanvas::CompTable::prefCursor, false);

    // add table template row and index properties to the layout table;
    // this will enable us to find out what particular template table cell 
    // was used to create a particular layout table cell; note: we could 
    // use these to actually load the template properties into the renderer 
    // at the last minute, similar to what we're doing with data values, and 
    // thus avoid copying cell properties from the template table to the
    // layout table to save time; however, we'd still have to account for 
    // merged cells; for now this will at least enable us to put newly 
    // changed cell properties in a layout table cell back into the 
    // corresponding template table cell used to create it
    table->addCellProperty(PROP_TABLE_TEMPLATE_SECTION_IDX, -1);
    table->addCellProperty(PROP_TABLE_TEMPLATE_ROW_IDX, -1);

    // add a table model row index property to the layout table; this 
    // property is set in the createPageLayout() function and stores the 
    // index of the data model row that should be used when populating 
    // cells with data for a given table layout row
    table->addCellProperty(PROP_TABLE_MODEL_ROW_IDX, -1);

    // add report state properties to the table; these are used to pass 
    // variables to the data model when rendering a table on a particular 
    // page
    table->addCellProperty(PROP_REPORT_PAGE_NUMBER, -1);
    table->addCellProperty(PROP_REPORT_PAGE_COUNT, -1);
    table->addCellProperty(PROP_REPORT_CURRENT_DATE, wxT(""));
    table->addCellProperty(PROP_REPORT_DATA_SOURCE, wxT(""));

    // connect the renderers to the property update function so we can
    // override their properties as needed; note: this is not the same as
    // a control event, which is handled the normal way; this code here
    // allows us to examine the properties of each renderer when the
    // properties change, and override them if necessary, allowing us
    // to customize the rendering on a cell-by-cell basis; this is used
    // to put the data in the cells right before they are rendered
    std::vector<kcanvas::IComponentPtr> renderers;
    table->getRenderers(renderers);
    
    std::vector<kcanvas::IComponentPtr>::iterator it_r, it_end_r;
    it_end_r = renderers.end();
    
    for (it_r = renderers.begin(); it_r != it_end_r; ++it_r)
    {
        kcanvas::IComponentPtr r = *it_r;
        r->getPropertiesRef().sigChanged().connect(this, &ReportLayoutEngine::onLayoutTableRendererUpdated);
    }

    // connect the editors
    std::vector<kcanvas::IComponentPtr> editors;
    table->getEditors(editors);
    
    std::vector<kcanvas::IComponentPtr>::iterator it_e, it_end_e;
    it_end_e = editors.end();
    
    for (it_e = editors.begin(); it_e != it_end_e; ++it_e)
    {
        kcanvas::IComponentPtr e = *it_e;
        e->getPropertiesRef().sigChanged().connect(this, &ReportLayoutEngine::onLayoutTableEditorUpdated);
    }

    // return the table
    return comp;
}

IModelPtr ReportLayoutEngine::createDataModel()
{
    // create a new tango model
    TangoModel* model = new TangoModel;
    model->sigModelLoaded().connect(this, &ReportLayoutEngine::onModelLoaded);

    // add stock properties, which we'll use for passing 
    // report info to the script function in the model
    model->addProperty(PROP_REPORT_PAGE_NUMBER, 0);
    model->addProperty(PROP_REPORT_PAGE_COUNT, 0);
    model->addProperty(PROP_REPORT_CURRENT_DATE, wxT(""));    
    model->addProperty(PROP_REPORT_DATA_SOURCE, wxT(""));

    // return the model
    return static_cast<IModel*>(model);
}

IModelPtr ReportLayoutEngine::getDataModel()
{
    return m_data_model;
}

bool ReportLayoutEngine::hasDataModel()
{
    return (m_data_model.isOk() ? true : false);
}

void ReportLayoutEngine::initReportVariables()
{
    // set the current date
    time_t ts;
    struct tm t;
    
    time(&ts);
    localtime_r(&ts, &t);

    m_report_variable_date = cfw::Locale::formatDate(1900+t.tm_year,
                                                     t.tm_mon+1,
                                                     t.tm_mday);

    // set the page count and data source
    m_report_variable_pagecount = getPageCount();
    m_report_variable_source = m_data_source;
}

void ReportLayoutEngine::populateDataModel()
{
    // if we don't have a data model, we're done
    if (!hasDataModel())
        return;

    // if the data model is loaded, simply set the model
    // groups from the sections; if the data model isn't
    // loaded, move on; when it's ready it will come back
    // here
    if (getDataModel()->isLoaded())
        return;

    wxString query_string;
    
    // set the base and browse sets
    wxString data_source = m_data_source;    
    tango::ISetPtr set = g_app->getDatabase()->openSet(towstr(data_source));
    if (set.isNull())
    {
        // if we don't have a data set, try to load the path as a query
        QueryTemplate query_template;
        if (query_template.load(data_source))
            query_string = query_template.getQueryString();
            
        // the query may specify an order; but we need to order the data 
        // according by the grouping parameters to ensure the data is
        // properly displayed; so "rewrite" the order params to use any
        // grouping params first, then append on whatever ORDER BY clause
        // existed
        if (m_data_order.Length() > 0)
        {
            // TODO: ideally, we should have better control over the following
            // query rewriting so avoid problems with things like "ORDER BY" appearing
            // in expressions, etc
        
            unsigned int order_by_pos = query_string.rfind(wxT("ORDER BY"));
            wxString query_string_part1 = query_string.Mid(0,order_by_pos);
            wxString query_string_part2 = order_by_pos != wxString::npos ? query_string.Mid(order_by_pos + 8) : wxT("");
            
            query_string = wxT("");
            query_string += query_string_part1 + wxT(" ORDER BY ") + m_data_order;
            query_string += query_string_part2.Length() > 0 ? wxT(",") + query_string_part2 : wxT("");
        }
    }
    else
    {
        // we have a data set; construct a query from the path
        query_string.Append(wxT("SELECT * FROM "));
        
        wxString quoted_data_source = towx(tango::quoteIdentifier(g_app->getDatabase(), towstr(data_source)));        
        query_string.Append(quoted_data_source);

        wxString data_filter = m_data_filter;
        if (data_filter.Length() > 0)
        {
            query_string.Append(wxT(" WHERE "));
            query_string.Append(data_filter);
        }

        // get the sort expression
        wxString data_order = m_data_order;
        if (data_order.Length() > 0)
        {
            query_string.Append(wxT(" ORDER BY "));
            query_string.Append(data_order);
        }
    }

    // load the data; use the current blocking mode to determine 
    // whether the model should process in the background or wait
    IModelPtr model = getDataModel();
    model->setQuery(query_string);
    model->execute(m_block);
}

void ReportLayoutEngine::populateDataGroups()
{
    // if the data model isn't loaded, there's nothing to do
    if (!getDataModel()->isLoaded())
        return;

    // first, remove all groups
    getDataModel()->removeAllGroups();

    // add the group headers; note: header fields accumulate,
    // so that each group is nested within previous groups
    std::vector<wxString> group_header_fields;
    
    std::vector<ReportSection>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_type != PROP_REPORT_GROUP_HEADER)
            continue;

        if (it->m_group_field.IsEmpty())
            continue;

        group_header_fields.push_back(it->m_group_field);
        getDataModel()->addGroup(it->m_name, group_header_fields);
    }

    // add the group footers; note: footer fields accumulate,
    // so that each group is nested within previous groups
    std::vector<wxString> group_footer_fields;

    std::vector<ReportSection>::reverse_iterator itr, itr_end;
    itr_end = m_sections.rend();
    
    for (itr = m_sections.rbegin(); itr != itr_end; ++itr)
    {
        if (itr->m_type != PROP_REPORT_GROUP_FOOTER)
            continue;

        if (itr->m_group_field.IsEmpty())
            continue;

        group_footer_fields.push_back(itr->m_group_field);
        getDataModel()->addGroup(itr->m_name, group_footer_fields);
    }  
}

void ReportLayoutEngine::populatePageCache()
{
    // create the page info template
    PageLayoutInfo page_info;
    page_info.m_page_rect.x = 0;
    page_info.m_page_rect.y = 0;
    page_info.m_page_rect.width = m_page_width;
    page_info.m_page_rect.height = m_page_height;
    page_info.m_layout_rect.x = m_margin_left;
    page_info.m_layout_rect.y = m_margin_top;
    page_info.m_layout_rect.width = m_page_width - m_margin_left - m_margin_right;
    page_info.m_layout_rect.height = m_page_height - m_margin_top - m_margin_bottom;

    // populate the page cache
    layoutReport(page_info, getDataModel(), m_sections, m_cache_pages);
}

void ReportLayoutEngine::populateCache()
{
    // clear the cache
    clearCache();

    // populate the cache; if the data model isn't ready, a
    // job will be triggered which will call this function
    populateDataModel();
    populateDataGroups();
    populatePageCache();
    
    // initialize the report variables
    initReportVariables();
    sigLayoutUpdated().fire();
}

void ReportLayoutEngine::clearCache()
{
    // clear the page cache
    m_cache_pages.clear();
}

bool ReportLayoutEngine::isCacheEmpty()
{
    // if the page cache is  populated, return false
    if (m_cache_pages.size() > 0)
        return false;

    // otherwise return true
    return true;
}

