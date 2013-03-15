/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2007-11-19
 *
 */


#ifndef __APP_REPORTDESIGN_H
#define __APP_REPORTDESIGN_H


#include "../kcanvas/kcanvas.h"
#include "../kcanvas/component.h"
#include "../kcanvas/componenttable.h"


// component type
const wxString COMP_TYPE_GROUPTABLE     = wxT("type.grouptable");

// report table row/column identifiers
const wxString NAME_TABLE_ROW_HEADER            = wxT("name.table.row.header");
const wxString NAME_TABLE_COLUMN_HEADER         = wxT("name.table.column.header");

// events
const wxString EVENT_DESIGN_PRE_GROUP_RESIZE    = wxT("event.design.pre.group.resize");
const wxString EVENT_DESIGN_GROUP_RESIZE        = wxT("event.design.group.resize");

// tag for marking state
const wxString TAG_TABLE_HEADER_DEFAULT     = wxT("table.header.default");


static bool isColumnHeader(kcanvas::ICompTablePtr table)
{
    if (table.isNull())
        return false;

    kcanvas::PropertyValue value;
    kcanvas::IComponentPtr comp = table;
    if (comp->getProperty(NAME_TABLE_COLUMN_HEADER, value))
        return true;

    return false;
}

static bool isRowHeader(kcanvas::ICompTablePtr table)
{
    if (table.isNull())
        return false;

    kcanvas::PropertyValue value;
    kcanvas::IComponentPtr comp = table;
    if (comp->getProperty(NAME_TABLE_ROW_HEADER, value))
        return true;

    return false;
}


// forward declarations
class ReportCreateInfo;
class ReportSection;
class CompReportDesign;



class SectionInfo
{
friend class CompReportDesign;

public:

    SectionInfo()
    {
        m_table = xcm::null;
        m_name = wxEmptyString;
        m_type = wxEmptyString;
        m_group_field = wxEmptyString;
        m_page_break = false;
        m_sort_desc = false;
        m_active = false;

        m_height = 0;        
        m_section_idx = 0;
        m_group_idx = -1;
    }

    virtual ~SectionInfo()
    {
    }

public:

    // section definition members
    kcanvas::ICompTablePtr m_table;  // table containing section content
    wxString m_name;                 // section name; used for referencing sections
    wxString m_type;                 // section type (e.g. header/footer/detail, etc)
    wxString m_group_field;          // group field; note: groups fields accumulate across sections
    bool m_page_break;               // section page break flag
    bool m_sort_desc;                // section sort descending flag
    bool m_active;                   // section active flag
    
    int m_section_idx;               // section index for quick reference; set automatically
    int m_group_idx;                 // group index for quick reference; set automatically
    int m_height;                    // section height; set automatically

private:

    // table row header; used to control selections
    // on a row; set automatically
    kcanvas::ICompTablePtr m_row_header;

    wxRect m_top;                    // dimensions of section top
    wxRect m_content;                // dimensions of section content
    wxRect m_bottom;                 // dimensions of section bottom
    wxRect m_gripper;                // dimensions of gripper`
};


class CompReportDesign : public kcanvas::Component
{
    XCM_CLASS_NAME("appmain.CompReportDesign")
    XCM_BEGIN_INTERFACE_MAP(CompReportDesign)
        XCM_INTERFACE_CHAIN(kcanvas::Component)
    XCM_END_INTERFACE_MAP()

public:

    CompReportDesign();
    virtual ~CompReportDesign();

    static kcanvas::IComponentPtr create();
    static void initProperties(kcanvas::Properties& properties);
    
    // IComponent
    kcanvas::IComponentPtr clone();
    void copy(kcanvas::IComponentPtr component);
    void render(const wxRect& rect = wxRect());

public:

    // functions for saving and loading the report
    bool save(const wxString& path);
    bool load(const wxString& path);
    bool load(const ReportCreateInfo& data);

    // data source functions
    void setDataSource(const wxString& path);
    wxString getDataSource();

    void setDataFilter(const wxString& filter);
    wxString getDataFilter();

    // page functions
    void setPageSize(int width, int height);
    void getPageSize(int* width, int* height);

    void setPageMargins(int left_margin,
                        int right_margin,
                        int top_margin,
                        int bottom_margin);
    
    void getPageMargins(int* left_margin,
                        int* right_margin,
                        int* top_margin,
                        int* bottom_margin);

    // function for API manipulation    
    bool setRowCount(int count);
    int getRowCount() const;

    bool setColumnCount(int count);
    int getColumnCount() const;

    bool insertRow(int idx, int count = 1);
    bool removeRow(int idx, int count = 1);

    bool insertColumn(int idx, int count = 1);
    bool removeColumn(int idx, int count = 1);

    bool setRowSize(int row, int size);
    int getRowSize(int row);
    
    bool setColumnSize(int col, int size);
    int getColumnSize(int col);

    void mergeCells(const kcanvas::CellRange& range);
    void unmergeCells(const kcanvas::CellRange& range);

    bool setCellProperty(const kcanvas::CellRange& range,
                         const wxString& prop_name,
                         const kcanvas::PropertyValue& value);

    bool setCellValue(int row, int col, const wxString& value);

    // functions for GUI manipulation; note: following "GUI" API is convoluted;
    // its evolved over time and needs to be cleaned up
    bool insertRows();
    bool deleteRows();
    bool insertColumns();
    bool deleteColumns();
    void clearContent(bool text_only = true);
    void selectAll();

    bool setSelectedCellProperty(const wxString& prop_name, const kcanvas::PropertyValue& value);
    bool setSelectedCellBorderProperties(const kcanvas::Properties& props);
    bool setMergedCells(bool merge);

    void resizeRowsToContent(kcanvas::ICompTablePtr header, int idx, int size);
    void resizeColumnsToContent(kcanvas::ICompTablePtr header, int idx, int size);

    // section functions
    bool addSection(const wxString& name, const wxString& type, int rows, bool active = true);
    void removeAllSections();

    void updateSections(const std::vector<ReportSection>& sections);
    void getSections(std::vector<ReportSection>& sections);

    bool setActiveSectionByIdx(int idx);
    bool setActiveSectionByName(const wxString& name);
    kcanvas::ICompTablePtr getActiveSectionTable() const;

    kcanvas::ICompTablePtr getSectionTableByIdx(int idx) const;
    kcanvas::ICompTablePtr getSectionTableByName(const wxString& name) const;    

    void setSectionVisible(const wxString& name, bool visible = true);
    bool getSectionVisible(const wxString& name);

    // functions for turning on/off the cursor and determining
    // if it is on/off
    void setCursorVisible(bool visible);
    bool isCursorVisible();

    // functions for laying out the component
    void layout();

    // functions for syncing the row and column sizes in the
    // section tables with their respective row or column
    // header; function for syncing the table selections,
    // in the other tables with the current table, where
    // appropriate
    void syncRowSizes(kcanvas::ICompTablePtr header, int idx, int size, bool selected);
    void syncColumnSizes(kcanvas::ICompTablePtr header, int idx, int size, bool selected);
    void syncSelections(kcanvas::ICompTablePtr table);
    void removeAllCellSelections();
    void setUpdateTableHeadersFlag();

    // functions for tagging/restoring the tables in each section
    bool tag(const wxString& tag);
    bool restore(const wxString& tag);

    // function for inserting a new group
    bool insertGroup(const wxString& name, 
                     const wxString& group_field,
                     bool sort_descending = false,
                     bool visible = false);
private:

    // event handlers
    void onMouse(kcanvas::IEventPtr evt);
    void onFocus(kcanvas::IEventPtr evt);
    void onPreRender(kcanvas::IEventPtr evt);
    void onPostRender(kcanvas::IEventPtr evt);
    void onTableEvent(kcanvas::IEventPtr evt);

private:

    // helper functions for determining where we are
    bool isSectionTop(int x, int y, int* idx);
    bool isSectionContent(int x, int y, int* idx);
    bool isSectionBottom(int x, int y, int* idx);
    bool isSectionGrip(int x, int y, int* idx);
    bool isSelectAllRect(int x, int y);

    // mouse event handler helpers
    void mouseResizeSection(kcanvas::IEventPtr evt);
    void mouseMoveSection(kcanvas::IEventPtr evt);
    
    bool startAction(kcanvas::IEventPtr evt, wxString action);
    bool endAction(kcanvas::IEventPtr evt, wxString action);
    bool isAction(kcanvas::IEventPtr evt, wxString action);
    void resetAction();

private:

    // functions for saving/loading the design template
    bool saveJson(const wxString& path);
    bool loadJson(const wxString& path);
    bool loadJsonFromNode(const wxString& path);

    // functions for adding the row and column table
    // headers and updating the group information
    void updateTableHeaderLayouts();    
    void addTableHeaders();
    void addGroupInfo();

    // functions for returning the fields that make up a group
    wxString getGroupFields(const wxString& group_name, bool all = true);

    // helper function for seeing if a given table is
    // one of the tables being used for a section
    bool hasTable(kcanvas::ICompTablePtr table) const;    

    // functions for iterating through visible tables
    kcanvas::ICompTablePtr getNextVisibleTable(int idx) const;
    kcanvas::ICompTablePtr getPrevVisibleTable(int idx) const;

    // functions for setting/getting the active section
    bool setActiveSectionByTable(kcanvas::ICompTablePtr table);    
    int getActiveSectionIdx() const;

    // extra functions for sections
    bool addSection(const SectionInfo& info);
    void setSections(const std::vector<SectionInfo>& sections);
    bool getSection(const wxString& name, SectionInfo& section);
    bool getSectionByRowHeader(kcanvas::ICompTablePtr header, SectionInfo& section);
    void setGroupsFromSortExpr(const wxString& sort_expr);

private:

    // report sections
    std::vector<SectionInfo> m_sections;

    // column and row table control
    kcanvas::ICompTablePtr m_column_header;
    kcanvas::ICompTablePtr m_row_header;
    
    // table that currently has the focus
    kcanvas::ICompTablePtr m_active_table;

    // data source a filter
    wxString m_data_source;
    wxString m_data_filter;

    // page size and margins
    int m_page_width;
    int m_page_height;
    int m_left_margin;
    int m_right_margin;
    int m_top_margin;
    int m_bottom_margin;

    // action variables
    int m_action_section_idx;   // index of section with action on it

    // update table header flag
    bool m_update_table_headers;
    
    // mouse variables
    wxString m_mouse_action;    // name of current mouse action
    bool m_mouse_ctrl_start;    // control key state at start of action
    bool m_mouse_shift_start;   // shift key state at start of action
    bool m_mouse_alt_start;     // alt key state at start of action
    
    int m_mouse_x;              // current x position of the mouse
    int m_mouse_y;              // current y position of the mouse
    int m_mouse_x_last;         // last x position of the mouse
    int m_mouse_y_last;         // last y position of the mouse
    int m_mouse_x_start;        // start x position of the mouse
    int m_mouse_y_start;        // start y position of the mouse
};


#endif

