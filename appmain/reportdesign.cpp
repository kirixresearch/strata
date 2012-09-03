/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2007-11-19
 *
 */


#include "appmain.h"
#include <wx/paper.h>
#include "reportdesign.h"
#include "reportengine.h"
#include "reportstore.h"
#include "jsonconfig.h"

#include "../kcanvas/componentpage.h"
#include "../kcanvas/util.h"


// sections
const wxString PROP_REPORT_HEADER_LABEL         = wxT("Report Header");
const wxString PROP_REPORT_FOOTER_LABEL         = wxT("Report Footer");
const wxString PROP_REPORT_PAGE_HEADER_LABEL    = wxT("Page Header");
const wxString PROP_REPORT_PAGE_FOOTER_LABEL    = wxT("Page Footer");
const wxString PROP_REPORT_GROUP_HEADER_LABEL   = wxT("Group Header");
const wxString PROP_REPORT_GROUP_FOOTER_LABEL   = wxT("Group Footer");
const wxString PROP_REPORT_DETAIL_LABEL         = wxT("Detail");

const int SECTION_TOP_HEIGHT            = (int)(kcanvas::CANVAS_MODEL_DPI*0.1875);
const int SECTION_BOTTOM_HEIGHT         = (int)(kcanvas::CANVAS_MODEL_DPI*0.0625);
const int SECTION_RESIZE_GRIP_WIDTH     = (int)(kcanvas::CANVAS_MODEL_DPI*0.5);

// table header size
const int TABLE_COLUMN_HEADER_SIZE      = (int)(kcanvas::CANVAS_MODEL_DPI*0.1875);
const int TABLE_ROW_HEADER_SIZE         = (int)(kcanvas::CANVAS_MODEL_DPI*0.125);

// actions
const wxString ACTION_NONE          = wxT("grouptable.action.none");
const wxString ACTION_RESIZESECTION = wxT("grouptable.action.resizesection");
const wxString ACTION_MOVESECTION   = wxT("grouptable.action.movesection");
const wxString ACTION_EDITCONTENT   = wxT("grouptable.action.editcontent");


static bool saveTablePropertiesToJson(kcanvas::ICompTablePtr table, JsonNode& node)
{
    if (table.isNull())
        return false;

    // compact the cell properties to remove properties
    // that are completely masked by other properties on
    // top of them
    table->compact();

    node["properties"].setArray();
    JsonNode properties = node["properties"];

    // save the cell properties
    std::vector<kcanvas::CellProperties> cell_properties;
    std::vector<kcanvas::CellProperties>::iterator it, it_end;
    
    kcanvas::CellRange range(-1, -1, -1, -1);
    table->getCellProperties(range, cell_properties);
    
    it_end = cell_properties.end();
    for (it = cell_properties.begin(); it != it_end; ++it)
    {
        JsonNode prop = properties.appendElement();


        // add the range information
        JsonNode range = prop["range"];
        range.setArray();
        
        JsonNode row1 = range.appendElement();
        row1.setInteger(it->range().row1());

        JsonNode col1 = range.appendElement();
        col1.setInteger(it->range().col1());

        JsonNode row2 = range.appendElement();
        row2.setInteger(it->range().row2());

        JsonNode col2 = range.appendElement();
        col2.setInteger(it->range().col2());


        // get the property value information for this range
        std::vector<kcanvas::Property> values;
        it->properties().list(values);

        wxString property_list;
        std::vector<kcanvas::Property>::iterator it_value, it_value_end;
        it_value_end = values.end();
        
        // iterate through the properties, creating and saving a list of the 
        // properties we're saving, so we know what to ask for on load      
        for (it_value = values.begin(); it_value != it_value_end; ++it_value)
        {
            wxString value_name = it_value->getName();
            int value_type = it_value->getType();

            // if we have an invalid type, move on
            if (value_type == kcanvas::proptypeInvalid)
                continue;

            JsonNode value = prop[value_name];


            if (value_type == kcanvas::proptypeString)
                value.setString(it_value->getString());

            if (value_type == kcanvas::proptypeColor)
                value.setString(it_value->getColor().GetAsString());

            if (value_type == kcanvas::proptypeInteger)
                value.setInteger(it_value->getInteger());

            if (value_type == kcanvas::proptypeBoolean)
                value.setBoolean(it_value->getBoolean());
        }
    }


    // save the merged cells
    std::vector<kcanvas::CellRange> merged_cells;
    std::vector<kcanvas::CellRange>::iterator it_merge, it_merge_end;

    table->getMergedCells(merged_cells);
    
    it_merge_end = merged_cells.end();
    for (it_merge = merged_cells.begin(); it_merge != it_merge_end; ++it_merge)
    {
        JsonNode prop = properties.appendElement();


        // add the range information
        JsonNode range = prop["range"];
        range.setArray();

        JsonNode row1 = range.appendElement();
        row1.setInteger(it_merge->row1());

        JsonNode col1 = range.appendElement();
        col1.setInteger(it_merge->col1());

        JsonNode row2 = range.appendElement();
        row2.setInteger(it_merge->row2());

        JsonNode col2 = range.appendElement();
        col2.setInteger(it_merge->col2());


        // set the merged flag
        JsonNode merged = prop["merged"];
        merged.setBoolean(true);
    }


    return true;
}

static void loadCellProperties(kcanvas::ICompTablePtr table, kcanvas::CellRange range, JsonNode node)
{
    JsonNode value;
    kcanvas::Properties properties;
    
    
    // set the integer properties
    value = node[kcanvas::PROP_ORIGIN_X];
    if (value.isOk())
        properties.add(kcanvas::PROP_ORIGIN_X, value.getInteger());
    
    value = node[kcanvas::PROP_ORIGIN_Y];
    if (value.isOk())
        properties.add(kcanvas::PROP_ORIGIN_Y, value.getInteger());    

    value = node[kcanvas::PROP_SIZE_W];
    if (value.isOk())
        properties.add(kcanvas::PROP_SIZE_W, value.getInteger());  

    value = node[kcanvas::PROP_SIZE_H];
    if (value.isOk())
        properties.add(kcanvas::PROP_SIZE_H, value.getInteger());  

    value = node[kcanvas::PROP_LINE_WIDTH];
    if (value.isOk())
        properties.add(kcanvas::PROP_LINE_WIDTH, value.getInteger());  

    value = node[kcanvas::PROP_FONT_SIZE];
    if (value.isOk())
        properties.add(kcanvas::PROP_FONT_SIZE, value.getInteger());

    value = node[kcanvas::PROP_TEXT_SPACING];
    if (value.isOk())
        properties.add(kcanvas::PROP_TEXT_SPACING, value.getInteger());


    // set the string properties
    value = node[kcanvas::PROP_FONT_FACENAME];
    if (value.isOk())
        properties.add(kcanvas::PROP_FONT_FACENAME, value.getString());

    value = node[kcanvas::PROP_FONT_STYLE];
    if (value.isOk())
        properties.add(kcanvas::PROP_FONT_STYLE, value.getString());
    
    value = node[kcanvas::PROP_FONT_WEIGHT];
    if (value.isOk())
        properties.add(kcanvas::PROP_FONT_WEIGHT, value.getString());
    
    value = node[kcanvas::PROP_FONT_UNDERSCORE];
    if (value.isOk())
        properties.add(kcanvas::PROP_FONT_UNDERSCORE, value.getString());            

    value = node[kcanvas::PROP_TEXT_HALIGN];
    if (value.isOk())
        properties.add(kcanvas::PROP_TEXT_HALIGN, value.getString());
    
    value = node[kcanvas::PROP_TEXT_VALIGN];
    if (value.isOk())
        properties.add(kcanvas::PROP_TEXT_VALIGN, value.getString());  

    value = node[kcanvas::PROP_CONTENT_VALUE];
    if (value.isOk())
        properties.add(kcanvas::PROP_CONTENT_VALUE, value.getString());
    
    value = node[kcanvas::PROP_CONTENT_MIMETYPE];
    if (value.isOk())
        properties.add(kcanvas::PROP_CONTENT_MIMETYPE, value.getString());  

    value = node[kcanvas::PROP_CONTENT_ENCODING];
    if (value.isOk())
        properties.add(kcanvas::PROP_CONTENT_ENCODING, value.getString());

    value = node[kcanvas::PROP_BORDER_TOP_STYLE];
    if (value.isOk())
        properties.add(kcanvas::PROP_BORDER_TOP_STYLE, value.getString());

    value = node[kcanvas::PROP_BORDER_BOTTOM_STYLE];
    if (value.isOk())
        properties.add(kcanvas::PROP_BORDER_BOTTOM_STYLE, value.getString());
        
    value = node[kcanvas::PROP_BORDER_LEFT_STYLE];
    if (value.isOk())
        properties.add(kcanvas::PROP_BORDER_LEFT_STYLE, value.getString());
        
    value = node[kcanvas::PROP_BORDER_RIGHT_STYLE];
    if (value.isOk())
        properties.add(kcanvas::PROP_BORDER_RIGHT_STYLE, value.getString());                        


    // set the color properties
    kcanvas::Color color;

    value = node[kcanvas::PROP_COLOR_FG];
    if (value.isOk())
    {
        wxString color_text = value.getString();
        color.Set(color_text);
        properties.add(kcanvas::PROP_COLOR_FG, color);
    }

    value = node[kcanvas::PROP_COLOR_BG];
    if (value.isOk())
    {
        wxString color_text = value.getString();
        color.Set(color_text);
        properties.add(kcanvas::PROP_COLOR_BG, color);
    }

    value = node[kcanvas::PROP_TEXT_COLOR];
    if (value.isOk())
    {
        wxString color_text = value.getString();
        color.Set(color_text);
        properties.add(kcanvas::PROP_TEXT_COLOR, color);
    }
    
    // set the boolean properties
    value = node[kcanvas::PROP_LOCKED];
    if (value.isOk())
        properties.add(kcanvas::PROP_LOCKED, value.getBoolean());

    value = node[kcanvas::PROP_TEXT_WRAP];
    if (value.isOk())
        properties.add(kcanvas::PROP_TEXT_WRAP, value.getBoolean());    


    std::vector<kcanvas::CellProperties> cell_properties;
    kcanvas::CellProperties cp;
    cp.setRange(range);
    cp.addProperties(properties);
    
    cell_properties.push_back(cp);


    // set the cell properties
    table->addCellProperties(cell_properties, false);


    // add the merged cells
    JsonNode merged = node["merged"];
    if (merged.isOk())
        table->mergeCells(range);
}

static bool loadTablePropertiesFromJson(JsonNode& node, kcanvas::ICompTablePtr table)
{
    if (table.isNull())
        return false;

    JsonNode properties = node["properties"];
    if (properties.isOk())
    {
        size_t prop_idx, prop_count = properties.getCount();
        for (prop_idx = 0; prop_idx < prop_count; ++prop_idx)
        {
            // get the range
            JsonNode prop = properties[prop_idx];

            // if a range isn't set, move on
            JsonNode range = prop["range"];
            if (range.isNull())
                continue;

            // get the range count, and make sure there are
            // either 2 or 4 parameters specified (row, col),
            // or (row1, col1, row2, col2), which can be used
            // to represent either a cell or a range of cells
            size_t range_count = range.getCount();
            if (range_count != 2 && range_count != 4)
                continue;

            int row1 = range[0].getInteger();
            int col1 = range[1].getInteger();
            int row2 = row1;
            int col2 = col1;
            
            if (range_count == 4)
            {
                row2 = range[2].getInteger();
                col2 = range[3].getInteger();
            }

            kcanvas::CellRange cell_range(row1, col1, row2, col2);

            // load the properties and merged cells
            loadCellProperties(table, cell_range, prop);         
        }
    }

    return true;
}

static wxString getDefaultSectionLabel(const wxString& type)
{
    if (type == PROP_REPORT_HEADER)
        return PROP_REPORT_HEADER_LABEL;
        
    if (type == PROP_REPORT_FOOTER)
        return PROP_REPORT_FOOTER_LABEL;
        
    if (type == PROP_REPORT_PAGE_HEADER)
        return PROP_REPORT_PAGE_HEADER_LABEL;
        
    if (type == PROP_REPORT_PAGE_FOOTER)
        return PROP_REPORT_PAGE_FOOTER_LABEL;
        
    if (type == PROP_REPORT_GROUP_HEADER)
        return PROP_REPORT_GROUP_HEADER_LABEL;
        
    if (type == PROP_REPORT_GROUP_FOOTER)
        return PROP_REPORT_GROUP_FOOTER_LABEL;
        
    if (type == PROP_REPORT_DETAIL)
        return PROP_REPORT_DETAIL_LABEL;
        
    return wxT("");
}

static void setDefaultTablePreferences(kcanvas::ICompTablePtr table)
{
    // if we don't have a valid table, we're done
    if (table.isNull())
        return;

    // turn off row and column selection and resizing since the
    // table headers will handle it    
    table->setPreference(kcanvas::CompTable::prefSelectRows, false);
    table->setPreference(kcanvas::CompTable::prefSelectColumns, false);
    table->setPreference(kcanvas::CompTable::prefResizeRows, false);
    table->setPreference(kcanvas::CompTable::prefResizeColumns, false);

    // turn on other preferences
    table->setPreference(kcanvas::CompTable::prefCursor, true);
    table->setPreference(kcanvas::CompTable::prefMoveCells, true);
    table->setPreference(kcanvas::CompTable::prefPopulateCells, true);
    table->setPreference(kcanvas::CompTable::prefEditCells, true);
}

static void setDefaultTableFont(kcanvas::ICompTablePtr table)
{
    // if we don't have a valid table, we're done
    if (table.isNull())
        return;

    cfw::IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (!prefs.isNull())
    {
        // create a suitable default font to ensure we have
        // one that will work on a given system    
        kcanvas::Font default_font;

        #ifdef WIN32
            default_font.setFaceName(wxT("Arial"));
        #else 
            default_font.setFaceName(wxT("Bitstream Vera Sans"));
        #endif

        default_font.setStyle(kcanvas::FONT_STYLE_NORMAL);
        default_font.setWeight(kcanvas::FONT_WEIGHT_NORMAL);
        default_font.setUnderscore(kcanvas::FONT_UNDERSCORE_NORMAL);
        default_font.setSize(10);    

        // modify the font based on the preferences
        if (prefs->exists(wxT("report.font.facename")))
        {
            wxString facename = getAppPrefsString(wxT("report.font.facename"));
            if (facename.Length() > 0)
                default_font.setFaceName(facename);
        }
        
        if (prefs->exists(wxT("report.font.size")))
        {
            int size = getAppPrefsLong(wxT("report.font.size"));
            if (size > 0)
                default_font.setSize(size);
        }

        if (prefs->exists(wxT("report.font.style")))
        {
            wxString style = getAppPrefsString(wxT("report.font.style"));
            if (style.Length() > 0)
                default_font.setStyle(style);
        }
        
        if (prefs->exists(wxT("report.font.weight")))
        {
            wxString weight = getAppPrefsString(wxT("report.font.weight"));
            if (weight.Length() > 0)
                default_font.setWeight(weight);
        }
        
        if (prefs->exists(wxT("report.font.underscore")))
        {
            wxString underscore = getAppPrefsString(wxT("report.font.underscore"));
            if (underscore.Length() > 0)
                default_font.setUnderscore(underscore);
        }

        // convert the font into canvas properties
        
        kcanvas::PropertyValue fontfacename;
        kcanvas::PropertyValue fontsize;
        kcanvas::PropertyValue fontstyle;
        kcanvas::PropertyValue fontweight;
        kcanvas::PropertyValue fontunderscore;

        kcanvas::setPropertiesFromFont(default_font,
                                       fontfacename,
                                       fontsize,
                                       fontstyle,
                                       fontweight,
                                       fontunderscore);
    
        // set the default table font; note: remaining preferences 
        // are set in ReportDoc::initSettings()
        kcanvas::CellRange all_cells(-1, -1, -1, -1);
        table->setCellProperty(all_cells, kcanvas::PROP_FONT_FACENAME, fontfacename);
        table->setCellProperty(all_cells, kcanvas::PROP_FONT_SIZE, fontsize);
        table->setCellProperty(all_cells, kcanvas::PROP_FONT_STYLE, fontstyle);
        table->setCellProperty(all_cells, kcanvas::PROP_FONT_WEIGHT, fontweight);
        table->setCellProperty(all_cells, kcanvas::PROP_FONT_UNDERSCORE, fontunderscore);
    }
}

static void removeRowSelections(kcanvas::ICompTablePtr table)
{
    // note: this function removes all row selections from
    // the specified table

    // get the non-cursor table selections
    std::vector<kcanvas::CellRange> selections;
    table->getCellSelections(selections, false);
    
    if (selections.empty())
        return;

    // copy all non-row table selections
    std::vector<kcanvas::CellRange> selection_copy;
    selection_copy.reserve(selections.size());
    
    std::vector<kcanvas::CellRange>::iterator it, it_end;
    it_end = selections.end();
    
    for (it = selections.begin(); it != it_end; ++it)
    {
        if (it->col1() == -1 && it->col2() == -1)
            continue;
            
        selection_copy.push_back(*it);
    }
    
    // clear the table cell selections
    table->removeAllCellSelections();
    
    // add the non-row table cell selections
    it_end = selection_copy.end();
    for (it = selection_copy.begin(); it != it_end; ++it)
    {
        table->selectCells(*it);
    }
}

static void removeColumnSelections(kcanvas::ICompTablePtr table)
{
    // note: this function removes all column selections from
    // the specified table

    // get the non-cursor table selections
    std::vector<kcanvas::CellRange> selections;
    table->getCellSelections(selections, false);

    if (selections.empty())
        return;
    
    // copy all non-column table selections
    std::vector<kcanvas::CellRange> selection_copy;
    selection_copy.reserve(selections.size());
    
    std::vector<kcanvas::CellRange>::iterator it, it_end;
    it_end = selections.end();
    
    for (it = selections.begin(); it != it_end; ++it)
    {
        if (it->row1() == -1 && it->row2() == -1)
            continue;
            
        selection_copy.push_back(*it);
    }
    
    // clear the table cell selections
    table->removeAllCellSelections();
    
    // add the non-column table cell selections
    it_end = selection_copy.end();
    for (it = selection_copy.begin(); it != it_end; ++it)
    {
        table->selectCells(*it);
    }
}

static void getMaxColumnSizes(std::map<int,int>& existing_sizes, 
                              std::map<int,int>& new_sizes)
{
    // note: helper function that merges new sizes with
    // existing sizes; if the new size doesn't exist in
    // the existing sizes, it's added to it; if the new
    // size exists in the existing sizes, and the new
    // size is larger than the existing size, then the
    // existing size is set to the new larger size
    
    std::map<int,int>::iterator it_new, it_new_end;
    it_new_end = new_sizes.end();
    
    for (it_new = new_sizes.begin(); it_new != it_new_end; ++it_new)
    {
        // see if we can find the row in question in
        // the existing list of rows
        std::map<int,int>::iterator it = existing_sizes.find(it_new->first);
    
        // if the row doesn't exist in the existing list
        // of sizes, add it and move on
        if (it == existing_sizes.end())
        {
            existing_sizes[it_new->first] = it_new->second;
            continue;
        }
        
        // if the new size exists in the existing sizes,
        // change the existing size if the new size is
        // larger than it
        if (it_new->second > it->second)
            it->second = it_new->second;
    }
}


CompReportDesign::CompReportDesign()
{
    // create default page dimensions and margins
    if (cfw::Locale::getDefaultPaperType() == wxPAPER_A4)
    {    
        m_page_width = A4_PAGE_WIDTH;
        m_page_height = A4_PAGE_HEIGHT;
        m_left_margin = A4_PAGE_LEFT_MARGIN;
        m_right_margin = A4_PAGE_RIGHT_MARGIN;
        m_top_margin = A4_PAGE_TOP_MARGIN;
        m_bottom_margin = A4_PAGE_BOTTOM_MARGIN;
    }
    else
    {
        m_page_width = LETTER_PAGE_WIDTH;
        m_page_height = LETTER_PAGE_HEIGHT;
        m_left_margin = LETTER_PAGE_LEFT_MARGIN;
        m_right_margin = LETTER_PAGE_RIGHT_MARGIN;
        m_top_margin = LETTER_PAGE_TOP_MARGIN;
        m_bottom_margin = LETTER_PAGE_BOTTOM_MARGIN;
    }

    // properties
    initProperties(m_properties);

    // event handlers
    addEventHandler(kcanvas::EVENT_RENDER, &CompReportDesign::onPreRender, true);
    addEventHandler(kcanvas::EVENT_RENDER, &CompReportDesign::onPostRender);
    addEventHandler(kcanvas::EVENT_MOUSE_LEFT_DOWN, &CompReportDesign::onMouse);
    addEventHandler(kcanvas::EVENT_MOUSE_LEFT_UP, &CompReportDesign::onMouse);
    addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_DOWN, &CompReportDesign::onMouse);
    addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_UP, &CompReportDesign::onMouse);
    addEventHandler(kcanvas::EVENT_MOUSE_MOTION, &CompReportDesign::onMouse);
    addEventHandler(kcanvas::EVENT_FOCUS_SET, &CompReportDesign::onFocus);
    addEventHandler(kcanvas::EVENT_FOCUS_KILL, &CompReportDesign::onFocus);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZE_COLUMN, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZING_COLUMN, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZE_ROW, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZING_ROW, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW_TO_CONTENT, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN_TO_CONTENT, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_SELECT_CELL, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_SELECTING_CELL, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_SELECTION_CLEAR, &CompReportDesign::onTableEvent);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_CURSOR_MOVE, &CompReportDesign::onTableEvent);

    // action variable initialization
    m_action_section_idx = -1;      // index of section with action on it    

    // update table header flag; set to true in addTableHeaders() 
    // and false in layout() after updating header sizes
    m_update_table_headers = false;

    // initialize the mouse variables
    m_mouse_action = ACTION_NONE;   // name of current mouse action
    m_mouse_ctrl_start = false;     // control key state at start of action
    m_mouse_shift_start = false;    // shift key state at start of action
    m_mouse_alt_start = false;      // alt key state at start of action    
    m_mouse_x = 0;                  // current x position of the mouse
    m_mouse_y = 0;                  // current y position of the mouse
    m_mouse_x_last = 0;             // last x position of the mouse
    m_mouse_y_last = 0;             // last y position of the mouse
    m_mouse_x_start = 0;            // start x position of the mouse
    m_mouse_y_start = 0;            // start y position of the mouse
}

CompReportDesign::~CompReportDesign()
{
}

kcanvas::IComponentPtr CompReportDesign::create()
{
    // create a new report design component
    return static_cast<kcanvas::IComponent*>(new CompReportDesign);
}

void CompReportDesign::initProperties(kcanvas::Properties& properties)
{
    kcanvas::Component::initProperties(properties);
    properties.add(kcanvas::PROP_COMP_TYPE, COMP_TYPE_GROUPTABLE);
}

kcanvas::IComponentPtr CompReportDesign::clone()
{
    CompReportDesign* group_table = new CompReportDesign;
    group_table->copy(this);

    return static_cast<kcanvas::IComponentPtr>(group_table);
}


// fix for VC6 C2352 compiler bug
typedef kcanvas::Component kcanvasComponent;
void CompReportDesign::copy(kcanvas::IComponentPtr component)
{
    // note: CompReportDesign is a component that manages
    // other components; copy() and clone() only copy the
    // managing structures, not the child components that
    // are managed; use deepCopy() when creating copies
    // of the managed components

    kcanvasComponent::copy(component);

    if (component.isNull())
        return;
        
    CompReportDesign* c = static_cast<CompReportDesign*>(component.p);
    m_sections = c->m_sections;
    m_active_table = c->m_active_table; 
    
    // data source a filter
    m_data_source = c->m_data_source;
    m_data_filter = c->m_data_filter;

    // page size and margins
    m_page_width = c->m_page_width;
    m_page_height = c->m_page_height;
    m_left_margin = c->m_left_margin;
    m_right_margin = c->m_right_margin;
    m_top_margin = c->m_top_margin;
    m_bottom_margin = c->m_bottom_margin;    
}

void CompReportDesign::render(const wxRect& rect)
{
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // draw the header and footer resize grips
    wxColor base_color = kcl::getBaseColor();
    kcanvas::Color start_color = kcanvas::tokccolor(kcl::stepColor(base_color, 150));
    kcanvas::Color end_color = kcanvas::tokccolor(kcl::stepColor(base_color, 90));

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        // if the section isn't active, move on
        if (!(it->m_active))
            continue;
        
        // -- draw header --

        wxRect header(it->m_top);
        canvas->drawGradientFill(header, start_color, end_color, wxSOUTH);
        
        // draw vertically-centered text for header; first set
        // the font so the text measurements are accurate, then
        // get the text measurements, and finally draw the text
        wxString label = getDefaultSectionLabel(it->m_type);
        
        if (it->m_type == PROP_REPORT_GROUP_HEADER)
        {
            label += wxString::Format(wxT(" %d - ["), it->m_group_idx + 1);
            label += getGroupFields(it->m_name, false);
            label += wxT("]");
        }
        
        if (it->m_type == PROP_REPORT_GROUP_FOOTER)
            label += wxString::Format(wxT(" %d"), it->m_group_idx + 1);


        // create a default label font
        kcanvas::Font label_font;
        
        #ifdef WIN32
            label_font.setFaceName(wxT("Arial"));
        #else 
            label_font.setFaceName(wxT("Bitstream Vera Sans"));
        #endif
        
        label_font.setStyle(kcanvas::FONT_STYLE_NORMAL);
        label_font.setWeight(kcanvas::FONT_WEIGHT_BOLD);
        label_font.setUnderscore(kcanvas::FONT_UNDERSCORE_NORMAL);
        label_font.setSize(8);
        
        canvas->setFont(label_font);
        canvas->setTextForeground(kcanvas::tokccolor(kcl::getCaptionColor()));

        int text_x, text_y, text_w, text_h;
        canvas->getTextExtent(label, &text_w, &text_h);
        text_x = header.GetX() + canvas->dtom_x(7);
        text_y = header.GetY()+((header.GetHeight()-text_h)/2);

        canvas->drawText(label, text_x, text_y);

        // -- draw gripper --

        wxRect gripper(it->m_bottom);
        gripper.SetWidth(SECTION_RESIZE_GRIP_WIDTH);
        canvas->drawGradientFill(gripper, start_color, end_color, wxSOUTH);

        int x = gripper.GetX() + gripper.GetWidth()/2;
        int y = gripper.GetY() + gripper.GetHeight()/2;
        int dot_space = gripper.GetWidth()/12;
        
        // draw gripper dots
        canvas->setPen(kcanvas::Color(kcanvas::COLOR_GREY));
        canvas->drawPoint(x, y);
        canvas->drawPoint(x + dot_space, y);
        canvas->drawPoint(x + dot_space*2, y);
        canvas->drawPoint(x + dot_space*3, y);
        canvas->drawPoint(x - dot_space, y);
        canvas->drawPoint(x - dot_space*2, y);
        canvas->drawPoint(x - dot_space*3, y);
    }
}

bool CompReportDesign::save(kcanvas::IStoreValuePtr store)
{
    // save the component properties
    if (!Component::save(store))
        return false;

    // save the total number of sections
    kcanvas::IStoreValuePtr node_1;
    node_1 = store->createChild(wxT("section.count"));
    node_1->setInteger(m_sections.size());

    // iterate through the sections and save the info
    // for each section
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        // create a node to store this section
        int section_idx = it - m_sections.begin();
        wxString node_2_name = wxString::Format(wxT("section%d"), section_idx);

        kcanvas::IStoreValuePtr node_2;
        node_2 = store->createChild(node_2_name);

        // save the section type
        kcanvas::IStoreValuePtr node_2_1;
        node_2_1 = node_2->createChild(wxT("section.type"));
        node_2_1->setString(it->m_type);

        // save the section field
        kcanvas::IStoreValuePtr node_2_2;
        node_2_2 = node_2->createChild(wxT("section.field"));
        node_2_2->setString(it->m_group_field);

        // save the page break flag
        kcanvas::IStoreValuePtr node_2_3;
        node_2_3 = node_2->createChild(wxT("section.pagebreak"));
        node_2_3->setBoolean(it->m_page_break);

        // save the sort descending flag
        kcanvas::IStoreValuePtr node_2_4;
        node_2_4 = node_2->createChild(wxT("section.sortdesc"));
        node_2_4->setBoolean(it->m_sort_desc);

        // save the active flag
        kcanvas::IStoreValuePtr node_2_5;
        node_2_5 = node_2->createChild(wxT("section.active"));
        node_2_5->setBoolean(it->m_active);

        // save the section properties
        kcanvas::IStoreValuePtr node_2_6;
        node_2_6 = node_2->createChild(wxT("section.properties"));
        kcanvas::IStorablePtr table = it->m_table;
        if (!table.isNull())
            table->save(node_2_6);
    }

    return true;
}

bool CompReportDesign::load(kcanvas::IStoreValuePtr store)
{
    if (store.isNull())
        return false;

    // clear out the sections and the child tables
    removeAllSections();

    // load the component properties
    if (!Component::load(store))
        return false;

    // find out the total number of sections
    kcanvas::IStoreValuePtr node_1;
    node_1 = store->getChild(wxT("section.count"), false);
    if (node_1.isNull())
        return false;

    int section_count = node_1->getInteger();

    // load the info for each section
    for (int section_idx = 0; section_idx < section_count; ++section_idx)
    {
        // section we're creating; set when we add the section
        SectionInfo* section;

        // section info we're loading
        wxString section_type;
        wxString section_field;
        bool section_pagebreak = false;
        bool section_sortdesc = false;
        bool section_active = true;
    
        // get the section
        wxString node_2_name = wxString::Format(wxT("section%d"), section_idx);

        kcanvas::IStoreValuePtr node_2;
        node_2 = store->getChild(node_2_name, false);
        if (node_2.isNull())
            continue;

        // get the section type
        kcanvas::IStoreValuePtr node_2_1;
        node_2_1 = node_2->getChild(wxT("section.type"), false);
        if (node_2_1.isNull())
        {
            // previously, "section.type" was called "section.name";
            // if we can't find the section type node, check the
            // section name; TODO: can remove for release
            node_2_1 = node_2->getChild(wxT("section.name"), false);
            if (node_2_1.isNull())
                continue;
        }

        section_type = node_2_1->getString();

        // get the section field
        kcanvas::IStoreValuePtr node_2_2;
        node_2_2 = node_2->getChild(wxT("section.field"), false);
        if (node_2_2.isNull())
        {
            // in previous betas, sections stored all the fields as
            // a comma-delimited list; if we don't have a field
            // section, get the list of fields and load it as
            // a single string, which will allow the group to be 
            // fixed in the interface
            node_2_2 = node_2->getChild(wxT("section.group"), false);
            if (node_2_2.isNull())
                continue;
        }

        // get the section field
        section_field = node_2_2->getString();

        // get the page break flag
        kcanvas::IStoreValuePtr node_2_3;
        node_2_3 = node_2->getChild(wxT("section.pagebreak"), false);
        if (!node_2_3.isNull())
            section_pagebreak = node_2_3->getBoolean();

        // get the sort descending flag
        kcanvas::IStoreValuePtr node_2_4;
        node_2_4 = node_2->getChild(wxT("section.sortdesc"), false);
        if (!node_2_4.isNull())
            section_sortdesc = node_2_4->getBoolean();

        // get the active flag
        kcanvas::IStoreValuePtr node_2_5;
        node_2_5 = node_2->getChild(wxT("section.active"), false);
        if (!node_2_5.isNull())
            section_active = node_2_5->getBoolean();

        // add a new section with the loaded type, and set the
        // section field; note: here, the group name is set the
        // same as the section type; for default sections, such
        // as report headers/footers, page headers/footers and
        // the detail section, the name is the same as the type;
        // for group headers/footers, the name will be reset
        // in the addSection() function
        addSection(section_type, section_type, 0, section_active);
        section = &m_sections[section_idx];
        section->m_group_field = section_field;
        section->m_sort_desc = section_sortdesc;
        section->m_page_break = section_pagebreak;

        // load the properties
        kcanvas::IStoreValuePtr node_2_6;
        node_2_6 = node_2->getChild(wxT("section.properties"), false);
        if (node_2_6.isNull())
            continue;
        
        kcanvas::IStorablePtr table = section->m_table;
        if (!table.isNull())
            table->load(node_2_6);
    }

    return true;
}

bool CompReportDesign::save(const wxString& path)
{
    // TODO: for now allow JSON to be saved by directly
    // specifying it in the extension; this is to allow
    // testing of the JSON format while still saving
    // in the XML format as the primary means of report
    // saving

    // if the path ends in .json, save the file
    // in the json format, otherwise, use XML
    wxString ext = path.AfterLast(wxT('.'));
    if (ext.Length() < path.Length() && ext.CmpNoCase(wxT("json")) == 0)
        return saveJson(path);

    return saveXml(path);
}

bool CompReportDesign::load(const wxString& path)
{
    // try to load the path in the new JSON format
    if (loadJson(path))
        return true;

    // if we can't load it in the new format, try
    // to open it with the old format
    return loadXml(path);
}

bool CompReportDesign::load(const ReportCreateInfo& data)
{
    // remove all sections and add some default sections
    removeAllSections();
    addSection(PROP_REPORT_HEADER, PROP_REPORT_HEADER, 2, false);
    addSection(PROP_REPORT_PAGE_HEADER, PROP_REPORT_PAGE_HEADER, 2, true);
    addSection(PROP_REPORT_DETAIL, PROP_REPORT_DETAIL, 1, true);
    addSection(PROP_REPORT_PAGE_FOOTER, PROP_REPORT_PAGE_FOOTER, 1, true);
    addSection(PROP_REPORT_FOOTER, PROP_REPORT_FOOTER, 2, false);
    
    // add the fields to the report   
    kcanvas::ICompTablePtr table_header;
    table_header = getSectionTableByName(PROP_REPORT_PAGE_HEADER);
    if (table_header.isNull())
        return false;

    kcanvas::ICompTablePtr table_detail;
    table_detail = getSectionTableByName(PROP_REPORT_DETAIL);
    if (table_detail.isNull())
        return false;

    int row_idx, col_idx;
    std::vector<ReportCreateField>::const_iterator it, it_end;
    it_end = data.content_fields.end();

    row_idx = 0;
    col_idx = 0;
    for (it = data.content_fields.begin(); it != it_end; ++it)
    {
        wxString value = it->field_name;
        value.Trim(true);
        value.Trim(false);
        kcanvas::CellRange range(row_idx, col_idx);
        table_header->setCellProperty(range, kcanvas::PROP_CONTENT_VALUE, value);
        table_header->setCellProperty(range, kcanvas::PROP_TEXT_HALIGN, it->alignment);
        col_idx++;
    }

    row_idx = 0;
    col_idx = 0;
    for (it = data.content_fields.begin(); it != it_end; ++it)
    {
        wxString value = wxT("=") + it->field_name;
        value.Trim(true);
        value.Trim(false);
        kcanvas::CellRange range(row_idx, col_idx);
        table_detail->setCellProperty(range, kcanvas::PROP_CONTENT_VALUE, value);
        table_detail->setCellProperty(range, kcanvas::PROP_TEXT_HALIGN, it->alignment);
        col_idx++;
    }

    // get the sections
    std::vector<SectionInfo> sections = m_sections;
    std::vector<SectionInfo>::iterator it_section, it_section_end;
    it_section_end = sections.end();

    // set the font; TODO: should build a valid font first, check it, 
    // then set the cell properties based on that font info
    if (data.font_facename.Length() > 0 && data.font_size > 0)
    {
        for (it_section = sections.begin(); it_section != it_section_end; ++it_section)
        {
            kcanvas::CellRange all_cells(-1, -1, -1, -1);
            it_section->m_table->setCellProperty(all_cells, kcanvas::PROP_FONT_FACENAME, data.font_facename);
            it_section->m_table->setCellProperty(all_cells, kcanvas::PROP_FONT_SIZE, data.font_size);
        }
    }
    
    // set the column widths; note: we have to do this on all the 
    // section tables
    int total_width = 0;
    
    row_idx = 0;
    col_idx = 0;
    for (it = data.content_fields.begin(); it != it_end; ++it)
    {
        // get the column width and add it to the total width
        int width = it->column_width*1.0625;
        total_width += width;

        // adjust each of the sections with the new column width
        for (it_section = sections.begin(); it_section != it_section_end; ++it_section)
        {
            it_section->m_table->setColumnSize(col_idx, width);
        }
        
        // advance the column
        col_idx++;
    }

    // if the total width of the columns exceeds the available 
    // columns space, and we're in portrait mode, flip the page 
    // to landscape mode
    int page_width, page_height;
    getPageSize(&page_width, &page_height);
    
    int left_margin, right_margin, top_margin, bottom_margin;
    getPageMargins(&left_margin, &right_margin, &top_margin, &bottom_margin);
    
    if (page_width < page_height && total_width > (page_width - left_margin - right_margin))
        setPageSize(page_height, page_width);

    // TODO: set suitable page headers and footers

    // set the data source and filter
    setDataSource(data.path);
    setDataFilter(data.filter_expr);
    
    // set the groups from the sort expression
    setGroupsFromSortExpr(data.sort_expr);
    return true;
}

void CompReportDesign::setDataSource(const wxString& path)
{
    m_data_source = path;
}

wxString CompReportDesign::getDataSource()
{
    return m_data_source;
}

void CompReportDesign::setDataFilter(const wxString& filter)
{
    m_data_filter = filter;
}

wxString CompReportDesign::getDataFilter()
{
    return m_data_filter;
}

void CompReportDesign::setPageSize(int width, int height)
{
    m_page_width = width;
    m_page_height = height; 
}

void CompReportDesign::getPageSize(int* width, int* height)
{
    *width = m_page_width;
    *height = m_page_height;
}

void CompReportDesign::setPageMargins(int left_margin,
                                      int right_margin,
                                      int top_margin,
                                      int bottom_margin)
{
    m_left_margin = left_margin;
    m_right_margin = right_margin;
    m_top_margin = top_margin;
    m_bottom_margin = bottom_margin;
}

void CompReportDesign::getPageMargins(int* left_margin,
                                      int* right_margin,
                                      int* top_margin,
                                      int* bottom_margin)
{
    *left_margin = m_left_margin;
    *right_margin = m_right_margin;
    *top_margin = m_top_margin;
    *bottom_margin = m_bottom_margin;
}

bool CompReportDesign::setRowCount(int count)
{
    m_update_table_headers = true;
    kcanvas::ICompTablePtr table = getActiveSectionTable();    
    return table->setRowCount(count);
}

int CompReportDesign::getRowCount() const
{
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    return table->getRowCount();
}

bool CompReportDesign::setColumnCount(int count)
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        table->setColumnCount(count);
    }

    // update the headers
    m_update_table_headers = true;

    return true;
}

int CompReportDesign::getColumnCount() const
{
    kcanvas::ICompTablePtr table = getSectionTableByIdx(0);
    return table->getColumnCount();
}

bool CompReportDesign::insertRow(int idx, int count)
{
    m_update_table_headers = true;
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    return table->insertRow(idx, count);
}

bool CompReportDesign::removeRow(int idx, int count)
{
    m_update_table_headers = true;
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    return table->removeRow(idx, count);
}

bool CompReportDesign::insertColumn(int idx, int count)
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        table->insertColumn(idx, count);
    }

    // update the headers
    m_update_table_headers = true;

    return true;
}

bool CompReportDesign::removeColumn(int idx, int count)
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        table->removeColumn(idx, count);
    }

    // update the headers
    m_update_table_headers = true;

    return true;
}

bool CompReportDesign::setRowSize(int row, int size)
{
    m_update_table_headers = true;
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    return table->setRowSize(row, size);
}

int CompReportDesign::getRowSize(int row)
{
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    return table->getRowSize(row);
}

bool CompReportDesign::setColumnSize(int col, int size)
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        table->setColumnSize(col, size);
    }

    // update the headers
    m_update_table_headers = true;

    return true;
}

int CompReportDesign::getColumnSize(int col)
{
    kcanvas::ICompTablePtr table = getSectionTableByIdx(0);
    return table->getColumnSize(col);
}

void CompReportDesign::mergeCells(const kcanvas::CellRange& range)
{
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    table->mergeCells(range);
}


void CompReportDesign::unmergeCells(const kcanvas::CellRange& range)
{
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    table->unmergeCells(range);
}

bool CompReportDesign::setCellProperty(const kcanvas::CellRange& range,
                                       const wxString& prop_name,
                                       const kcanvas::PropertyValue& value)
{
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    return table->setCellProperty(range, prop_name, value);
}

bool CompReportDesign::setCellValue(int row, int col, const wxString& value)
{
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    return table->setCellProperty(kcanvas::CellRange(row, col),
                                  kcanvas::PROP_CONTENT_VALUE,
                                  value);
}

bool CompReportDesign::insertRows()
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // get the selection areas
        std::vector<kcanvas::CellRange> selections;
        table->getCellSelections(selections);
        
        // reverse iterate through the selections and add a row
        // for each of the rows contained in a row selection range
        std::vector<kcanvas::CellRange>::reverse_iterator itr;
        for (itr = selections.rbegin(); itr != selections.rend(); ++itr)
        {
            // if the selection area is the whole table, continue
            if (itr->contains(kcanvas::CellRange(-1,-1)))
                continue;

            // insert rows corresponding to the number of rows in 
            // the selection and select rows we just inserted
            if (itr->isRow())
            {
                table->insertRow(itr->row1(), itr->row2() - itr->row1() + 1);
                table->selectCells(*itr);
            }
        }
    }

    // update the headers
    m_update_table_headers = true;

    return true;
}

bool CompReportDesign::deleteRows()
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // get the selection areas
        std::vector<kcanvas::CellRange> selections;
        table->getCellSelections(selections);
        
        // reverse iterate through the selections and remove the
        // selected rows
        std::vector<kcanvas::CellRange>::reverse_iterator itr;
        for (itr = selections.rbegin(); itr != selections.rend(); ++itr)
        {
            // if the selection area is the whole table, continue
            if (itr->contains(kcanvas::CellRange(-1,-1)))
                continue;

            // remove the selected rows and select the new rows that
            // fall into the same position
            if (itr->isRow())
            {     
                table->removeRow(itr->row1(), itr->row2() - itr->row1() + 1);
                table->selectCells(*itr);
            }
        }
    }

    // update the headers
    m_update_table_headers = true;

    return true;
}

bool CompReportDesign::insertColumns()
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // get the selection areas
        std::vector<kcanvas::CellRange> selections;
        table->getCellSelections(selections);
        
        // reverse iterate through the selections and add a column
        // for each of the column contained in a column selection range
        std::vector<kcanvas::CellRange>::reverse_iterator itr;
        for (itr = selections.rbegin(); itr != selections.rend(); ++itr)
        {
            // if the selection area is the whole table, continue
            if (itr->contains(kcanvas::CellRange(-1,-1)))
                continue;
            
            // insert columns corresponding to the number of columns in 
            // the selection and select the columns
            if (itr->isColumn())
            {
                table->insertColumn(itr->col1(), itr->col2() - itr->col1() + 1);
                table->selectCells(*itr);
            }
        }
    }

    // update the headers
    m_update_table_headers = true;

    return true;
}

void CompReportDesign::clearContent(bool text_only)
{
    // iterate through the sections and clear the text 
    // contents in each table

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::IEditPtr edit = it->m_table;
        if (edit.isNull())
            continue;

        edit->clear(text_only);
    }
}

bool CompReportDesign::deleteColumns()
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // get the selection areas
        std::vector<kcanvas::CellRange> selections;
        table->getCellSelections(selections);
        
        // reverse iterate through the selections and remove the
        // selected columns
        std::vector<kcanvas::CellRange>::reverse_iterator itr;
        for (itr = selections.rbegin(); itr != selections.rend(); ++itr)
        {
            // if the selection area is the whole table, continue
            if (itr->contains(kcanvas::CellRange(-1,-1)))
                continue;
            
            // remove the selected columns and select the new columns that
            // fall into the same position
            if (itr->isColumn())
            {
                table->removeColumn(itr->col1(), itr->col2() - itr->col1() + 1);
                table->selectCells(*itr);
            }
        }
    }

    // update the headers
    m_update_table_headers = true;

    return true;
}

void CompReportDesign::selectAll()
{
    // used throughout
    kcanvas::IEditPtr edit;
    
    // select the column header
    edit = m_column_header;
    if (!edit.isNull())
        edit->selectAll();

    // iterate through the sections and select
    // all the cells in each section
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        // select the section table
        edit = it->m_table;
        if (!edit.isNull())
            edit->selectAll();

        // select the row header
        edit = it->m_row_header;
        if (!edit.isNull())
            edit->selectAll();
    }
}

bool CompReportDesign::setSelectedCellProperty(const wxString& prop_name, const kcanvas::PropertyValue& value)
{
    // returns true if any cell properties change; false otherwise
    // cycle through the selections in each of the tables of
    // the group table and try to set the property on the selection
    bool changed = false;

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // get the selection areas
        std::vector<kcanvas::CellRange> selections;
        table->getCellSelections(selections);
        
        // iterate through the selections and try to set the property
        std::vector<kcanvas::CellRange>::iterator itp;
        for (itp = selections.begin(); itp != selections.end(); ++itp)
        {
            if (table->setCellProperty(*itp, prop_name, value))
                changed = true;
        }
    }
    
    return changed;    
}

bool CompReportDesign::setSelectedCellBorderProperties(const kcanvas::Properties& props)
{
    // returns true if any cell properties change; false otherwise
    bool changed = false;

    // get the properties list
    std::vector<kcanvas::Property> properties;
    props.list(properties);

    std::vector<kcanvas::Property>::iterator it_props, it_props_end;
    it_props_end = properties.end();

    // cycle through the selections in each of the section tables
    // and try to set the property on the selection
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // TODO: for now, limit to the active table
        if (getActiveSectionTable() != it->m_table)
            continue;

        // get the selection areas
        std::vector<kcanvas::CellRange> selections;
        table->getCellSelections(selections, false);    // handle cursor separately
        
        // if we don't have any selections, use the cursor row
        if (selections.size() == 0 && 
            table->isPreference(kcanvas::CompTable::prefCursor))
        {
            int row, col;
            table->getCursorPos(&row, &col);
            selections.push_back(kcanvas::CellRange(row, col));
        }

        // iterate through the selections and try to set the property
        std::vector<kcanvas::CellRange>::iterator itp;
        for (itp = selections.begin(); itp != selections.end(); ++itp)
        {
            for (it_props = properties.begin(); it_props != it_props_end; ++it_props)
            {
            
                if (it_props->getName() == kcanvas::PROP_BORDER_STYLE && it_props->getString() == kcanvas::BORDER_STYLE_NONE)
                {
                    kcanvas::CellRange all_cells(itp->row1(), itp->col1(), itp->row2(), itp->col2()); 
                    table->setCellProperty(all_cells, kcanvas::PROP_BORDER_LEFT_STYLE, it_props->getString());
                    table->setCellProperty(all_cells, kcanvas::PROP_BORDER_RIGHT_STYLE, it_props->getString());
                    table->setCellProperty(all_cells, kcanvas::PROP_BORDER_TOP_STYLE, it_props->getString());
                    table->setCellProperty(all_cells, kcanvas::PROP_BORDER_BOTTOM_STYLE, it_props->getString());
                    changed = true;
                    continue;
                }

                if (it_props->getName() == kcanvas::PROP_BORDER_LEFT_STYLE)
                {
                    kcanvas::CellRange left_border(itp->row1(), itp->col1(), itp->row2(), itp->col1());                
                    table->setCellProperty(left_border, kcanvas::PROP_BORDER_LEFT_STYLE, kcanvas::BORDER_STYLE_SOLID);
                    changed = true;
                }

                if (it_props->getName() == kcanvas::PROP_BORDER_RIGHT_STYLE)
                {
                    kcanvas::CellRange right_border(itp->row1(), itp->col2(), itp->row2(), itp->col2());                
                    table->setCellProperty(right_border, kcanvas::PROP_BORDER_RIGHT_STYLE, kcanvas::BORDER_STYLE_SOLID);
                    changed = true;
                }

                if (it_props->getName() == kcanvas::PROP_BORDER_TOP_STYLE)
                {
                    kcanvas::CellRange top_border(itp->row1(), itp->col1(), itp->row1(), itp->col2());                
                    table->setCellProperty(top_border, kcanvas::PROP_BORDER_TOP_STYLE, kcanvas::BORDER_STYLE_SOLID);
                    changed = true;
                }

                if (it_props->getName() == kcanvas::PROP_BORDER_BOTTOM_STYLE)
                {
                    kcanvas::CellRange bottom_border(itp->row2(), itp->col1(), itp->row2(), itp->col2());                
                    table->setCellProperty(bottom_border, kcanvas::PROP_BORDER_BOTTOM_STYLE, kcanvas::BORDER_STYLE_SOLID);
                    changed = true;
                }
            }
        }
    }
    
    return changed;
}

bool CompReportDesign::setMergedCells(bool merge)
{
    // returns true if any cells are merged or unmerged; false otherwise
    bool changed = false;
    
    // cycle through the selections in each of the tables of
    // the group table and merge the cells in each selection
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // get the selection areas
        std::vector<kcanvas::CellRange> selections;
        table->getCellSelections(selections);
        
        // iterate through the selections and try to set the property
        std::vector<kcanvas::CellRange>::iterator itp;
        for (itp = selections.begin(); itp != selections.end(); ++itp)
        {
            if (merge)
            {
                table->mergeCells(*itp);
            }
            else
            {
                table->unmergeCells(*itp);
            }
            
            changed = true;
        }
    }
    
    return changed;
}

void CompReportDesign::resizeRowsToContent(kcanvas::ICompTablePtr header, 
                                           int idx, 
                                           int size)
{
    // get the row header
    kcanvas::ICompTablePtr row_header = header;
    if (row_header.isNull())
        return;

    // iterate through the sections and set the row
    // size on the section tables
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // if the row header row isn't selected, simply set the 
        // row size on the row corresponding to the row header;
        // otherwise, set the row size on all the selected rows
        if (!row_header->isRowSelected(idx))
        {
            if (header == it->m_row_header)
                table->resizeRowsToContent(kcanvas::CellRange(idx, -1, idx, -1));
        }
        else
        {
            std::vector<kcanvas::CellRange> selections;
            it->m_table->getCellSelections(selections);
        
            std::vector<kcanvas::CellRange>::iterator it, it_end;
            it_end = selections.end();

            for (it = selections.begin(); it != it_end; ++it)
            {
                // resize the content if we have a row or
                // a table selection
                if (it->col1() == -1 && it->col2() == -1)
                    table->resizeRowsToContent(*it);
            }
        }
    }
}

void CompReportDesign::resizeColumnsToContent(kcanvas::ICompTablePtr header, 
                                              int idx, 
                                              int size)
{
    // get the column header
    kcanvas::ICompTablePtr column_header = header;
    if (column_header.isNull())
        return;

    // maximum column sizes; used throughout
    std::map<int,int> max_sizes;

    // get the column selections
    std::vector<kcanvas::CellRange> selections;
    column_header->getCellSelections(selections);

    // iterate through the sections and get the
    // content size for each table
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // if the column header column isn't selected or the
        // selected flag is false, simply get the content size
        // for the column in question; otherwise get the content
        // size for the selected columns

        std::map<int,int> sizes;
        
        if (!column_header->isColumnSelected(idx))
        {
            table->getColumnContentSize(kcanvas::CellRange(-1, idx, -1, idx), sizes);
            getMaxColumnSizes(max_sizes, sizes);
        }
        else
        {
            std::vector<kcanvas::CellRange>::iterator it, it_end;
            it_end = selections.end();

            for (it = selections.begin(); it != it_end; ++it)
            {
                // resize the content if we have a row or
                // a table selection
                if (it->row1() == -1 && it->row2() == -1)
                {
                    table->getColumnContentSize(*it, sizes);
                    getMaxColumnSizes(max_sizes, sizes);
                }
            }
        }
    }
    
    // iterate through the sections and set the new column sizes
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;
            
        std::map<int,int>::iterator it_sizes, it_sizes_end;
        it_sizes_end = max_sizes.end();
        
        for (it_sizes = max_sizes.begin(); it_sizes != it_sizes_end; ++it_sizes)
        {
            it->m_table->setColumnSize(it_sizes->first, it_sizes->second);
        }
    }
}

bool CompReportDesign::addSection(const wxString& name,
                                  const wxString& type,
                                  int rows, 
                                  bool active)
{
    // create a default table
    kcanvas::ICompTablePtr table = kcanvas::CompTable::create();
    table->setRowCount(rows);
    table->setColumnCount(75);

    // create the section info
    SectionInfo info;
    info.m_name = name;
    info.m_type = type;
    info.m_active = active;
    info.m_table = table;
    
    // add the section
    return addSection(info);
}

void CompReportDesign::removeAllSections()
{
    // reset the table row and column headers
    m_column_header = xcm::null;
    m_row_header = xcm::null;

    // remove all sections
    removeAll();
    m_sections.clear();
}

void CompReportDesign::updateSections(const std::vector<ReportSection>& group_sections)
{
    // note: this function takes the input sections and updates the groups 
    // in the design template by 1) inserting new groups if they don't exist,
    // 2) removing groups that aren't in the list of specified sections, and
    // 3) if the specified groups exist in the design template, updating
    // them with the new information

    // new design template sections
    std::vector<SectionInfo> new_sections;
    std::vector<ReportSection>::const_iterator its, its_end;
    std::vector<SectionInfo>::const_iterator it, it_end;

    // iterate through the old design sections and add the 
    // report and page headers to the new sections
    it_end = m_sections.end();    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_type == PROP_REPORT_HEADER || 
            it->m_type == PROP_REPORT_PAGE_HEADER)
        {
            new_sections.push_back(*it);
        }
    }

    // set the group headers
    its_end = group_sections.end();
    for (its = group_sections.begin(); its != its_end; ++its)
    {
        if (its->m_type != PROP_REPORT_GROUP_HEADER)
            continue;

        // try to get a pre-existing section
        SectionInfo section;
        getSection(its->m_name, section);

        // set the information in the section; if the section 
        // is pre-existing, the table pointer will exist, so 
        // all the table info will be saved; if the section 
        // isn't pre-existing, the rest of the info will be 
        // filled out when the new sections are added to the 
        // design template
        section.m_type = its->m_type;
        section.m_group_field = its->m_group_field;
        section.m_page_break = its->m_page_break;
        section.m_sort_desc = its->m_sort_desc;
        section.m_active = its->m_active;

        new_sections.push_back(section);
    }

    // iterate through the old design sections and
    // add the report detail to the new sections
    it_end = m_sections.end();
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_type == PROP_REPORT_DETAIL)
        {
            new_sections.push_back(*it);
        }
    }

    // set the group headers
    its_end = group_sections.end();
    for (its = group_sections.begin(); its != its_end; ++its)
    {
        if (its->m_type != PROP_REPORT_GROUP_FOOTER)
            continue;

        // try to get a pre-existing section
        SectionInfo section;
        getSection(its->m_name, section);

        // set the information in the section; if the section 
        // is pre-existing, the table pointer will exist, so 
        // all the table info will be saved; if the section 
        // isn't pre-existing, the rest of the info will be 
        // filled out when the new sections are added to the 
        // design template
        section.m_type = its->m_type;
        section.m_group_field = its->m_group_field;
        section.m_page_break = its->m_page_break;
        section.m_sort_desc = its->m_sort_desc;
        section.m_active = its->m_active;

        new_sections.push_back(section);
    }

    // iterate through the old design sections and add the 
    // report and page footers to the new sections
    it_end = m_sections.end();
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_type == PROP_REPORT_FOOTER || 
            it->m_type == PROP_REPORT_PAGE_FOOTER)
        {
            new_sections.push_back(*it);
        }
    }

    // set the sections
    setSections(new_sections);
}

void CompReportDesign::getSections(std::vector<ReportSection>& sections)
{
    sections.clear();
    sections.reserve(sections.size());

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        ReportSection section;
        section.m_table_model = it->m_table->getModel()->clone();
        section.m_name = it->m_name;
        section.m_type = it->m_type;
        section.m_group_field = it->m_group_field;
        section.m_page_break = it->m_page_break;
        section.m_sort_desc = it->m_sort_desc;
        section.m_active = it->m_active;
        sections.push_back(section);
    }
}

bool CompReportDesign::getSection(const wxString& name, 
                                  SectionInfo& section)
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (name == it->m_name)
        {
            section = *it;
            return true;
        }
    }

    return false;
}

bool CompReportDesign::setActiveSectionByIdx(int idx)
{
    // note: this function sets the active table to
    // a particular table

    if (idx < 0 || idx >= (int)m_sections.size())
        return false;
        
    kcanvas::ICompTablePtr new_table;
    new_table = m_sections[idx].m_table;

    m_active_table = new_table;
    return true;
}

bool CompReportDesign::setActiveSectionByName(const wxString& name)
{
    // note: this function sets the focus to a particular
    // table in a section in the report design, using
    // the section name; returns true if the section is
    // active, and false otherwise

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_name == name)
        {
            m_active_table = it->m_table;
            return true;
        }
    }

    return false;
}

kcanvas::ICompTablePtr CompReportDesign::getActiveSectionTable() const
{
    return m_active_table;
}

kcanvas::ICompTablePtr CompReportDesign::getSectionTableByIdx(int idx) const
{
    if (idx < 0 || idx >= (int)m_sections.size())
        return xcm::null;

    return m_sections[idx].m_table;
}

kcanvas::ICompTablePtr CompReportDesign::getSectionTableByName(const wxString& name) const
{
    std::vector<SectionInfo>::const_iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_name == name)
            return it->m_table;
    }

    return xcm::null;
}

void CompReportDesign::setSectionVisible(const wxString& name, bool visible)
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (name == it->m_name)
            it->m_active = visible;
    }
}

bool CompReportDesign::getSectionVisible(const wxString& name)
{
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (name == it->m_name)
            return it->m_active;
    }
    
    return false;
}

void CompReportDesign::setCursorVisible(bool visible)
{
    // iterate through the section tables and
    // turn on/off the cursor
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_table.isNull())
            continue;

        it->m_table->setPreference(kcanvas::CompTable::prefCursor, visible);
    }
}

bool CompReportDesign::isCursorVisible()
{
    kcanvas::ICompTablePtr table = getActiveSectionTable();
    if (table.isNull())
        return false;

    return table->isPreference(kcanvas::CompTable::prefCursor);
}

void CompReportDesign::layout()
{
    // if the canvas is null, use the default canvas to which
    // the component belongs to layout the component
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // clear out the component
    removeAll();

    // initial offset
    int x_offset = 0;
    int y_offset = TABLE_COLUMN_HEADER_SIZE;

    // get the view origin
    int x_view, y_view;
    canvas->getViewOrigin(&x_view, &y_view);

    // create a view offset so the row controls
    // stay in the same place, but don't scroll
    // past the edge of the model
    int view_offset = x_view;
    if (view_offset < 0)
        view_offset = 0;

    // add a one pixel adjustment so the bottom of the column
    // header border doesn't cut off the first section header
    //y_offset += canvas->dtom_y(1);

    // maximum table width
    int max_width = 0;

    // iterate through the tables and set the
    // dimensions of the sections
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        // set the table size to accomodate the content
        kcanvas::IComponentPtr comp = it->m_table;
        if (comp.isNull())
            continue;

        // if the section isn't active, move on 
        if (!(it->m_active))
            continue;

        // if the section is active, add the row header
        // followed by the table
        add(it->m_table);
        add(it->m_row_header);

        // make sure the component is visible
        comp->visible(true);

        // set the size of the table
        wxRect rect;
        comp->extends(rect);
        comp->setSize(rect.GetWidth(), rect.GetHeight());

        int y1 = y_offset;
        int y2 = y1 + SECTION_TOP_HEIGHT;
        int y3 = y2 + comp->getHeight();
        int width = comp->getWidth();

        if (width > max_width)
            max_width = width;

        // set the section info
        it->m_height = comp->getHeight();                 // height of table
        it->m_section_idx = it - m_sections.begin();      // section index

        // set the section dimensions
        it->m_top.SetX(x_offset + view_offset);  // add view offset so labels don't scroll
        it->m_top.SetY(y1);
        it->m_top.SetWidth(width + TABLE_ROW_HEADER_SIZE);
        it->m_top.SetHeight(SECTION_TOP_HEIGHT);

        it->m_content.SetX(x_offset + TABLE_ROW_HEADER_SIZE);
        it->m_content.SetY(y2);
        it->m_content.SetWidth(width);
        it->m_content.SetHeight(comp->getHeight());

        it->m_bottom.SetX(x_offset + view_offset);  // add view offset so labels don't scroll
        it->m_bottom.SetY(y3);
        it->m_bottom.SetHeight(SECTION_BOTTOM_HEIGHT);
        it->m_bottom.SetWidth(width + TABLE_ROW_HEADER_SIZE);

        // set the gripper dimensions
        it->m_gripper.SetX(x_offset + view_offset);
        it->m_gripper.SetY(y3);
        it->m_gripper.SetWidth(SECTION_RESIZE_GRIP_WIDTH);
        it->m_gripper.SetHeight(SECTION_BOTTOM_HEIGHT);

        // set the position of the table
        comp->setOrigin(it->m_content.GetX(), it->m_content.GetY());

        // advance the y offset
        y_offset = y3 + it->m_bottom.GetHeight();
    }

    // add the column header
    add(m_column_header);

    // update the table headers
    updateTableHeaderLayouts();

    // set the dimensions of this component
    setSize(max_width, y_offset);
}

void CompReportDesign::syncRowSizes(kcanvas::ICompTablePtr header, 
                                    int idx, 
                                    int size,
                                    bool selected)
{
    // get the row header
    kcanvas::ICompTablePtr row_header = header;
    if (row_header.isNull())
        return;

    // iterate through the sections and set the row
    // size on the section tables
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // if the row header row isn't selected or the selected 
        // flag is false, simply set the row size on the row 
        // corresponding to the row header; otherwise, if the 
        // row header is selected, set the row size on all the 
        // selected rows to the same as the size as the resized 
        // row
        if (!row_header->isRowSelected(idx) || !selected)
        {
            if (header == it->m_row_header)
                table->setRowSize(idx, size);
        }
        else
        {
            std::vector<kcanvas::CellRange> selections;
            it->m_row_header->getCellSelections(selections);
        
            std::vector<kcanvas::CellRange>::iterator it, it_end;
            it_end = selections.end();

            for (it = selections.begin(); it != it_end; ++it)
            {
                kcanvas::Properties p;
                p.add(kcanvas::PROP_SIZE_H, size);
                table->setCellProperties(*it, p);
            }
        }
    }
}

void CompReportDesign::syncColumnSizes(kcanvas::ICompTablePtr header, 
                                       int idx, 
                                       int size,
                                       bool selected)
{
    // get the column header
    kcanvas::ICompTablePtr column_header = header;
    if (column_header.isNull())
        return;
    
    // get the column selections
    std::vector<kcanvas::CellRange> selections;
    column_header->getCellSelections(selections);

    // iterate through the sections and set the column
    // size on the section tables
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr table = it->m_table;
        if (table.isNull())
            continue;

        // if the column header column isn't selected or the
        // selected flag is false, simply reset the size on 
        // all the section tables to the same size as the column;
        // otherwise, set the width of the selected columns
        if (!column_header->isColumnSelected(idx) || !selected)
        {
            table->setColumnSize(idx, size);
        }
        else
        {
            std::vector<kcanvas::CellRange>::iterator it, it_end;
            it_end = selections.end();

            for (it = selections.begin(); it != it_end; ++it)
            {
                kcanvas::Properties p;
                p.add(kcanvas::PROP_SIZE_W, size);
                table->setCellProperties(*it, p);
            }
        }
    }
}

void CompReportDesign::syncSelections(kcanvas::ICompTablePtr table)
{
    // note: synchronize the table row/column headers with their
    // respective tables

    // if we don't have a table, we're done
    if (table.isNull())
        return;

    // get the table selections; used throughout
    std::vector<kcanvas::CellRange> selections;
    table->getCellSelections(selections);
    
    std::vector<kcanvas::CellRange>::iterator it, it_end;
    it_end = selections.end();

    // if the table is a row header, remove the row selections 
    // and then copy the row selections from the header to the 
    // associated section table
    SectionInfo section;
    if (getSectionByRowHeader(table, section) && !section.m_table.isNull())
    {
        // remove the column selections from the section table    
        removeRowSelections(section.m_table);

        // add the selections
        for (it = selections.begin(); it != it_end; ++it)
        {
            kcanvas::CellRange row(it->row1(), -1, it->row2(), -1);
            section.m_table->selectCells(row);
        }

        return;
    }

    // if the table is a column header, iterate through each of
    // the section tables, removing the column selections and
    // copying the column selections from the header to each
    // table; note: we allow any column header, since a column
    // header isn't associated with any particular section
    kcanvas::PropertyValue value;
    kcanvas::IComponentPtr comp = table;
    if (!comp->getProperty(NAME_TABLE_COLUMN_HEADER, value))
        return;
    
    // iterate through the sections
    std::vector<SectionInfo>::iterator it_section, it_section_end;
    it_section_end = m_sections.end();
    
    for (it_section = m_sections.begin(); it_section != it_section_end; ++it_section)
    {
        // if the table isn't valid, move on
        if (it_section->m_table.isNull())
            continue;

        // remove the column selections from the section table
        removeColumnSelections(it_section->m_table);

        // add the selections
        for (it = selections.begin(); it != it_end; ++it)
        {            
            kcanvas::CellRange column(-1, it->col1(), -1, it->col2());
            it_section->m_table->selectCells(column);
        }
    }

    return;
}

void CompReportDesign::removeAllCellSelections()
{
    // iterate through the sections
    std::vector<SectionInfo>::iterator it_section, it_section_end;
    it_section_end = m_sections.end();
    
    for (it_section = m_sections.begin(); it_section != it_section_end; ++it_section)
    {
        // if the table isn't valid, move on
        if (it_section->m_table.isNull())
            continue;

        // remove all cell selections from the section table
        it_section->m_table->removeAllCellSelections();
    }
}

void CompReportDesign::setUpdateTableHeadersFlag()
{
    m_update_table_headers = true;
}

bool CompReportDesign::tag(const wxString& tag)
{
    // return true if all of the non-null section tables
    // are tagged, and false if any of the non-null tables
    // cannot be tagged

    bool result = true;
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_table.isNull())
            continue;
            
        if (!it->m_table->tag(tag))
            result = false;
    }
    
    return result;
}

bool CompReportDesign::restore(const wxString& tag)
{
    // return true if all of the non-null section tables
    // are restored, and false if any of the non-null tables
    // cannot be restored

    bool result = true;
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_table.isNull())
            continue;
            
        if (!it->m_table->restore(tag))
            result = false;
    }

    return result;
}

bool CompReportDesign::insertGroup(const wxString& name, 
                                   const wxString& group_field,
                                   bool sort_descending,
                                   bool visible)
{
    // note: this function assumes we have a page header
    // and footer; if we have both of these, the function
    // will insert a group header after the page header
    // and a group footer before the page footer and
    // return true; if we don't have a page header or
    // footer, the function will return false

    bool result = false;

    // create a new list of sections
    std::vector<SectionInfo> section_copy;
    section_copy.reserve(m_sections.size() + 2);

    // iterate through the existing sections and copy them
    // to the new list of sections
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_type == PROP_REPORT_PAGE_HEADER)
        {
            // set the result flag
            result = true;
        
            // create a group header
            SectionInfo group_header;
            group_header.m_name = name + wxT(".header");
            group_header.m_type = PROP_REPORT_GROUP_HEADER;
            group_header.m_group_field = group_field;
            group_header.m_page_break = false;
            group_header.m_sort_desc = sort_descending;
            group_header.m_active = visible;
            
            // copy the page header, then the group header
            section_copy.push_back(*it);
            section_copy.push_back(group_header);
            
            // move onto the next section
            continue;
        }

        if (it->m_type == PROP_REPORT_PAGE_FOOTER)
        {
            // set the result flag
            result = true;
        
            // create a group footer
            SectionInfo group_footer;
            group_footer.m_name = name + wxT(".footer");
            group_footer.m_type = PROP_REPORT_GROUP_FOOTER;
            group_footer.m_group_field = group_field;
            group_footer.m_page_break = false;
            group_footer.m_sort_desc = sort_descending;
            group_footer.m_active = visible;
            
            // copy the group footer, then the page footer
            section_copy.push_back(group_footer);
            section_copy.push_back(*it);
            
            // move onto the next section
            continue;
        }

        // if it isn't a page header or footer, copy it
        section_copy.push_back(*it);
    }

    // set the new sections
    setSections(section_copy);

    // return true if a group section was inserted
    // and false otherwise
    return result;
}

void CompReportDesign::onMouse(kcanvas::IEventPtr evt)
{
    kcanvas::IMouseEventPtr mouse_evt = evt;

    // at this point, if we're not in the at-target phase,
    // we're done
    if (evt->getPhase() != kcanvas::EVENT_AT_TARGET)
        return;

    // set the last mouse position state variables
    m_mouse_x_last = m_mouse_x;
    m_mouse_y_last = m_mouse_y;
    
    // set the current mouse position state variables
    m_mouse_x = mouse_evt->getX();
    m_mouse_y = mouse_evt->getY();

    // if we click on the "select all" rectangle, select
    // all the cells in all the sections
    if (evt->getName() == kcanvas::EVENT_MOUSE_LEFT_DOWN &&
        isSelectAllRect(m_mouse_x, m_mouse_y))
    {
        selectAll();
        render();
        return;
    }

    int section_idx;
    if (isSectionGrip(m_mouse_x, m_mouse_y, &section_idx))
    {
        kcanvas::INotifyEventPtr notify_evt;
        notify_evt = kcanvas::NotifyEvent::create(kcanvas::EVENT_CANVAS_CURSOR, this);
        notify_evt->addProperty(kcanvas::EVENT_PROP_CURSOR, kcanvas::CURSOR_SIZENS);
        dispatchEvent(notify_evt);
    }

    mouseResizeSection(evt);
    mouseMoveSection(evt);
}

void CompReportDesign::onFocus(kcanvas::IEventPtr evt)
{
    wxString name = evt->getName();
    if (evt->getPhase() == kcanvas::EVENT_BUBBLING_PHASE)
    {
        // try to set the active target; if the event of the child 
        // receiving the focus isn't one of the table sections, the 
        // active table will remain the same
        if (name == kcanvas::EVENT_FOCUS_SET)
        {
            setActiveSectionByTable(evt->getTarget());
        }

        if (name == kcanvas::EVENT_FOCUS_KILL)
        {
        }
    }
}

void CompReportDesign::onPreRender(kcanvas::IEventPtr evt)
{
    // note: before we draw the column header, draw the
    // upper-left corner

    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // get the target
    kcanvas::ICompTablePtr target = evt->getTarget();
    kcanvas::IComponentPtr comp = target;
    
    // if we're not on a table, we're done
    if (target.isNull())
        return;

    // see if we're on a row or column header
    bool row_header = false;
    bool column_header = false;
    
    kcanvas::PropertyValue value;
    if (comp->getProperty(NAME_TABLE_ROW_HEADER, value))
        row_header = true;
        
    if (comp->getProperty(NAME_TABLE_COLUMN_HEADER, value))
        column_header = true;

    // find out the location of the upper-left of the view
    int view_x, view_y;
    canvas->getViewOrigin(&view_x, &view_y);
    
    // set the draw origin to the upper-left of the view
    int old_draw_origin_x, old_draw_origin_y;
    canvas->getDrawOrigin(&old_draw_origin_x, &old_draw_origin_y);
    canvas->setDrawOrigin(view_x, view_y);

    // if we're on a row header, clip it so the left part of
    // the table content area shows
    if (row_header)
    {
        int adj = canvas->dtom_x(1);  // 1 pixel adjustment
        canvas->addClippingRectangle(0, 0, TABLE_ROW_HEADER_SIZE - adj, canvas->getViewHeight());
    }
    
    // if we're on the column header, clip the header
    // and draw the upper-left corner    
    if (column_header)
    {
        // if we're on a column header, clip it so the vertical
        // lines span the whole column header
        int adj = canvas->dtom_x(1);  // 1 pixel adjustment
        canvas->addClippingRectangle(0, 0, canvas->getViewWidth(), TABLE_COLUMN_HEADER_SIZE - adj);
    
        // draw the top-left corner of the design component,
        // where the column header and row header meet
        kcanvas::Color start_color = kcanvas::tokccolor(kcl::getBaseColor());
        kcanvas::Color end_color = kcanvas::stepColor(kcanvas::COLOR_WHITE, start_color, 70);

        wxRect header_corner(0, 0, TABLE_ROW_HEADER_SIZE, TABLE_COLUMN_HEADER_SIZE);
        canvas->drawGradientFill(header_corner, start_color, end_color, wxNORTH);
    }    
    
    // if we're not on a row header or table header, we're on 
    // one of the content tables; clip it to the non-header area
    // so that the table cursor doesn't show through; turn on
    // the table cursor if it has the focus, and off otherwise
    if (!row_header && !column_header)
    {                                  
        canvas->addClippingRectangle(TABLE_ROW_HEADER_SIZE, 
                                     TABLE_COLUMN_HEADER_SIZE,
                                     canvas->getViewWidth(), 
                                     canvas->getViewHeight());

        kcanvas::ICompTablePtr table = evt->getTarget();
        if (!table.isNull())
        {
            if (canvas->hasFocus(table))
                table->setPreference(kcanvas::CompTable::prefCursor, true);
            else
                table->setPreference(kcanvas::CompTable::prefCursor, false);
        }
    }

    // restore the draw origin
    canvas->setDrawOrigin(old_draw_origin_x, old_draw_origin_y);
}

void CompReportDesign::onPostRender(kcanvas::IEventPtr evt)
{
    // note: after we draw the column header, draw the
    // margin indicator on top of the header

    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // get the target
    kcanvas::ICompTablePtr target = evt->getTarget();
    
    // if we're drawing the column header, draw
    // the margin indicator
    if (m_column_header == target)
    {
        kcanvas::IComponentPtr comp = target;

        wxRect header;
        comp->getOrigin(&header.x, &header.y);
        comp->getSize(&header.width, &header.height);

        int margin_location = m_page_width - m_left_margin - m_right_margin;

        int x1 = margin_location;
        int y1 = 0;
        int x2 = margin_location;
        int y2 = header.height;

        canvas->setPen(kcanvas::Color(kcanvas::COLOR_BLACK));
        canvas->drawLine(x1, y1, x2, y2);
    }
}

void CompReportDesign::onTableEvent(kcanvas::IEventPtr evt)
{
    // TODO: add rendering optimizations throughout

    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // get the event name
    wxString name = evt->getName();

    // move the cursor between the sections
    if (name == kcanvas::EVENT_TABLE_PRE_CURSOR_MOVE)
    {
        kcanvas::PropertyValue row_value, column_value;
    
        // get the cell where the cursor is going
        kcanvas::INotifyEventPtr notify_evt = evt;
        notify_evt->getProperty(kcanvas::EVENT_PROP_ROW, row_value);
        notify_evt->getProperty(kcanvas::EVENT_PROP_COLUMN, column_value);
        
        int new_row = row_value.getInteger();
        int new_col = column_value.getInteger();

        kcanvas::ICompTablePtr table = evt->getTarget();
        if (table.isNull())
            return;
            
        int row_count = table->getRowCount();
        int col_count = table->getColumnCount();
        
        if (new_row < 0)
        {
            // set the new active table
            kcanvas::ICompTablePtr new_table;
            new_table = getPrevVisibleTable(getActiveSectionIdx());
            if (new_table.isNull())
                return;
            
            setActiveSectionByTable(new_table);
            canvas->setFocus(new_table);

            new_table->setCursorPos(new_table->getRowCount()-1, new_col);
            canvas->layout();
            canvas->render();

            return;
        }

        if (new_row >= row_count)
        {
            // set the new active table
            kcanvas::ICompTablePtr new_table;
            new_table = getNextVisibleTable(getActiveSectionIdx());
            if (new_table.isNull())
                return;

            setActiveSectionByTable(new_table);
            canvas->setFocus(new_table);

            new_table->setCursorPos(0, new_col);
            canvas->layout();
            canvas->render();

            return;
        }

        // for now, don't do anything if the cursor is
        // trying to go past the column range of the table
        if (new_col < 0)
            return;
            
        if (new_col >= col_count)
            return;
    }

    if (name == kcanvas::EVENT_TABLE_SELECTION_CLEAR)
    {
        // if the selections are cleared in one table,
        // we need to clear them in all of them

        // clear out all the selections
        std::vector<SectionInfo>::iterator it, it_end;
        it_end = m_sections.end();
        
        for (it = m_sections.begin(); it != it_end; ++it)
        {
            if (it->m_table.isOk())
                it->m_table->removeAllCellSelections();

            if (it->m_row_header.isOk())
                it->m_row_header->removeAllCellSelections();
        }

        if (m_column_header.isOk())
            m_column_header->removeAllCellSelections();
        
        // invalidate the whole group table, rather than
        // trying to find out what changed
        invalidate();
    }

    // at this point, only handle handle events from
    // one of the table controls; so, if we're on one
    // of the section tables, we're done
    if (hasTable(evt->getTarget()))
        return;

    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_ROW)
    {
        // before we do anything, tag the tables so
        // that we can undo the temporary size properties
        // that accumulate during the real-time resizing
        tag(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW);
    }

    if (name == kcanvas::EVENT_TABLE_RESIZE_ROW)
    {
        // restore the table to what they were before
        // we started the real-time resizing, then
        // add on the final size properties
        restore(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW);
    
        // get the row index and size that's changed
        kcanvas::INotifyEventPtr notify_evt = evt;

        kcanvas::PropertyValue index, size;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, index);
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, size);
        
        // sync up the row sizes, including selected rows
        syncRowSizes(evt->getTarget(), 
                     index.getInteger(), 
                     size.getInteger(),
                     true);

        // make sure to update the table headers
        setUpdateTableHeadersFlag();

        // set the focus back to the active table
        canvas->setFocus(getActiveSectionTable());
    
        // if one of the table rows was resized, layout 
        // and render the canvas
        canvas->layout();
        canvas->render();
        return;
    }

    if (name == kcanvas::EVENT_TABLE_RESIZING_ROW)
    {
        // do real-time row resizing

        // get the row index and size that's changed
        kcanvas::INotifyEventPtr notify_evt = evt;

        kcanvas::PropertyValue index, size;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, index);
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, size);
        
        // sync up the row that's being resized
        syncRowSizes(evt->getTarget(), 
                     index.getInteger(), 
                     size.getInteger(), 
                     false);
    
        // if one of the table rows was resized, layout 
        // and render the canvas
        canvas->layout();
        canvas->render();
        return;
    }

    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN)
    {
        // before we do anything, tag the tables so
        // that we can undo the temporary size properties
        // that accumulate during the real-time resizing
        tag(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN);
    }

    if (name == kcanvas::EVENT_TABLE_RESIZE_COLUMN)
    {
        // restore the table to what they were before
        // we started the real-time resizing, then
        // add on the final size properties
        restore(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN);
    
        // get the column index and size that's changed
        kcanvas::INotifyEventPtr notify_evt = evt;

        kcanvas::PropertyValue index, size;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, index);
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, size);

        // update the column sizes, including selected rows
        syncColumnSizes(evt->getTarget(), 
                        index.getInteger(), 
                        size.getInteger(), 
                        true);

        // make sure to update the table headers
        setUpdateTableHeadersFlag();

        // set the focus back to the active table
        canvas->setFocus(getActiveSectionTable());

        // if one of the table columns was resized, layout 
        // and render the canvas
        canvas->layout();
        canvas->render();
        return;
    }
    
    if (name == kcanvas::EVENT_TABLE_RESIZING_COLUMN)
    {
        // do real-time row resizing

        // get the column index and size that's changed
        kcanvas::INotifyEventPtr notify_evt = evt;

        kcanvas::PropertyValue index, size;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, index);
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, size);

        // sync up the column that's being resized
        syncColumnSizes(evt->getTarget(), 
                        index.getInteger(), 
                        size.getInteger(), 
                        false);

        // if one of the table columns was resized, layout 
        // and render the canvas
        canvas->layout();
        canvas->render();
        return;
    }

    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_ROW_TO_CONTENT)
    {
        // this event is vetoed by the row header; this
        // allows us to sync up the header with the content 
        // table
        
        // get the row index and size that's about to change
        kcanvas::INotifyEventPtr notify_evt = evt;

        kcanvas::PropertyValue index, size;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, index);
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, size);
        
        // set the row sizes to their content
        resizeRowsToContent(evt->getTarget(), 
                            index.getInteger(), 
                            size.getInteger());

        // update the table headers based on the
        // newly resized rows
        setUpdateTableHeadersFlag();

        // set the focus back to the active table
        canvas->setFocus(getActiveSectionTable());
    
        // layout and render the canvas
        canvas->layout();
        canvas->render();
        return;
    }
    
    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN_TO_CONTENT)
    {
        // this event is vetoed by the column header; this
        // allows us to sync up the header with the content 
        // table
        
        // get the column index and size that's about to change
        kcanvas::INotifyEventPtr notify_evt = evt;

        kcanvas::PropertyValue index, size;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, index);
        notify_evt->getProperty(kcanvas::EVENT_PROP_SIZE, size);
        
        // set the column sizes to their content
        resizeColumnsToContent(evt->getTarget(), 
                               index.getInteger(), 
                               size.getInteger());

        // update the table headers based on the
        // newly resized columns
        setUpdateTableHeadersFlag();

        // set the focus back to the active table
        canvas->setFocus(getActiveSectionTable());
    
        // layout and render the canvas
        canvas->layout();
        canvas->render();
        return;
    }

    if (name == kcanvas::EVENT_TABLE_SELECT_CELL)
    {
        // update the selections
        syncSelections(evt->getTarget());

        // render the canvas
        canvas->render();
        return;
    }

    if (name == kcanvas::EVENT_TABLE_SELECTING_CELL)
    {
        // update the selections
        syncSelections(evt->getTarget());

        // render the canvas
        canvas->render();
        return;
    }
}

bool CompReportDesign::isSectionTop(int x, int y, int* idx)
{
    // note: checks if we're a section top; if so,
    // sets idx to the index of the section and returns
    // true; otherwise returns false
    
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_top.Contains(x, y))
        {
            *idx = (it - m_sections.begin());
            return true;
        }
    }
    
    *idx = -1;
    return false;
}

bool CompReportDesign::isSectionContent(int x, int y, int* idx)
{
    // note: checks if we're on section content; if so,
    // sets idx to the index of the section and returns
    // true; otherwise returns false
    
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_content.Contains(x, y))
        {
            *idx = (it - m_sections.begin());
            return true;
        }
    }
    
    *idx = -1;
    return false;
}

bool CompReportDesign::isSectionBottom(int x, int y, int* idx)
{
    // note: checks if we're on a section bottom; if so,
    // sets idx to the index of the section and returns
    // true; otherwise returns false
    
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_bottom.Contains(x, y))
        {
            *idx = (it - m_sections.begin());
            return true;
        }
    }
    
    *idx = -1;
    return false;
}

bool CompReportDesign::isSectionGrip(int x, int y, int* idx)
{
    // note: checks if we're on a section gripper; if so,
    // sets idx to the index of the section and returns
    // true; otherwise returns false

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_gripper.Contains(x, y))
        {
            *idx = (it - m_sections.begin());
            return true;
        }
    }
    
    *idx = -1;
    return false;
}

bool CompReportDesign::isSelectAllRect(int x, int y)
{
    wxRect rect(0, 0, TABLE_ROW_HEADER_SIZE, TABLE_COLUMN_HEADER_SIZE);
    if (!rect.Contains(x, y))
        return false;

    return true;
}

void CompReportDesign::mouseResizeSection(kcanvas::IEventPtr evt)
{
    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    if (startAction(evt, ACTION_RESIZESECTION))
    {
        // if we're not on the section bottom, we're done
        int section_idx;
        if (!isSectionGrip(m_mouse_x, m_mouse_y, &section_idx))
        {
            resetAction();
            return;
        }

        // dispatch an event the section is about to be resized
        kcanvas::INotifyEventPtr notify_evt;
        notify_evt = kcanvas::NotifyEvent::create(EVENT_DESIGN_PRE_GROUP_RESIZE, this);
        dispatchEvent(notify_evt);

        // if the event is vetoed, we're done
        if (!notify_evt->isAllowed())
        {
            resetAction();
            return;
        }

        // set the action
        m_action_section_idx = section_idx;
    }

    if (endAction(evt, ACTION_RESIZESECTION))
    {
        // dispatch an event that the section is resized
        kcanvas::INotifyEventPtr notify_evt;
        notify_evt = kcanvas::NotifyEvent::create(EVENT_DESIGN_GROUP_RESIZE, this);
        dispatchEvent(notify_evt);

        // set the focus back to the active table and re-render
        canvas->setFocus(getActiveSectionTable());
        canvas->render();
    }

    if (isAction(evt, ACTION_RESIZESECTION))
    {
        // to resize the section, we collapse the table 
        // completely, then increment the row count one 
        // row at a time until the table extends beyond 
        // the y location of the mouse event
    
        // get the active section
        if (m_action_section_idx < 0 ||
            m_action_section_idx >= (int)m_sections.size())
        {
            return;
        }

        // get the section that has the action
        SectionInfo s = m_sections[m_action_section_idx];
        kcanvas::ICompTablePtr table = s.m_table;
        kcanvas::IComponentPtr comp = table;
        
        // save the initial number of rows in the table
        int old_row_count = table->getRowCount();
        
        // set the number of table rows to zero
        int row_idx = 0;
        table->setRowCount(row_idx);
        
        // find the initial extent of the table
        wxRect table_extent;
        comp->extends(table_extent);

        int y_offset = m_mouse_y - s.m_content.GetY();
        while (y_offset > table_extent.GetHeight())
        {
            table->setRowCount(++row_idx);
            comp->extends(table_extent);
        }

        // at this point, the table extends beyond the y 
        // location of the mouse event; we only want it to
        // extend beyond the mouse position if the mouse
        // position is more than half way through the row 
        // to give the resizing a smoother feel; so, if the 
        // mouse position is in the top half of the row, 
        // subtract the last row
        if (row_idx > 0)
        {
            int y = table->getRowPosByIdx(row_idx-1);
            int w = table->getRowSize(row_idx-1);

            if (y_offset <= y + w/2)
                table->setRowCount(--row_idx);
        }
        
        // get the new row count; if the row count has changed,
        // redraw the whole component
        int new_row_count = table->getRowCount();
        if (old_row_count != new_row_count)
        {
            // update the headers
            m_update_table_headers = true;

            // layout and render
            canvas->layout();
            canvas->render();
        }
    }
}

void CompReportDesign::mouseMoveSection(kcanvas::IEventPtr evt)
{
    // TODO: for now, simply return
    return;

    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    if (startAction(evt, ACTION_MOVESECTION))
    {
        // if we're not on the section top, we're done
        int section_idx;
        if (!isSectionTop(m_mouse_x, m_mouse_y, &section_idx))
            resetAction();

        // set the action
        m_action_section_idx = section_idx;
    }
    
    if (endAction(evt, ACTION_MOVESECTION))
    {
        // update the headers
        m_update_table_headers = true;

        // layout and render
        canvas->layout();
        canvas->render();
    }
    
    if (isAction(evt, ACTION_MOVESECTION))
    {
        // to resize the section, we collapse the table 
        // completely, then increment the row count one 
        // row at a time until the table extends beyond 
        // the y location of the mouse event
    
        // get the active section
        if (m_action_section_idx < 0 ||
            m_action_section_idx >= (int)m_sections.size())
        {
            return;
        }

        // get the section that has the action
        SectionInfo* s = &m_sections[m_action_section_idx];
        kcanvas::ICompTablePtr table = s->m_table;
        kcanvas::IComponentPtr comp = table;

        int x_offset, y_offset;
        comp->getOrigin(&x_offset, &y_offset);
        
        int temp_y = y_offset + m_mouse_y - m_mouse_y_start;
        s->m_top.SetY(temp_y - s->m_top.GetHeight());
        s->m_content.SetY(temp_y);
        s->m_bottom.SetY(temp_y + s->m_content.GetHeight());

        // redraw the whole component
        //canvas->layout();
        canvas->render();
    }
}

bool CompReportDesign::startAction(kcanvas::IEventPtr evt, wxString action)
{
    // if the event is a left-mouse button down and we
    // don't already have an action, set the action and 
    // return true
    if (evt->getName() == kcanvas::EVENT_MOUSE_LEFT_DOWN && 
        m_mouse_action == ACTION_NONE)
    {
        // set the mouse action
        m_mouse_action = action;

        // set the key state at the start of the event
        kcanvas::IMouseEventPtr mouse_evt = evt;
        m_mouse_alt_start = mouse_evt->isAltDown();
        m_mouse_ctrl_start = mouse_evt->isCtrlDown();
        m_mouse_shift_start = mouse_evt->isShiftDown();

        // set the starting mouse position
        m_mouse_x_start = m_mouse_x;
        m_mouse_y_start = m_mouse_y;

        return true;
    }
    
    // otherwise, return false
    return false;
}

bool CompReportDesign::endAction(kcanvas::IEventPtr evt, wxString action)
{
    // if the event is a left-mouse button up and we
    // already have an action that matches the input 
    // action, reset the action and return true
    if (evt->getName() == kcanvas::EVENT_MOUSE_LEFT_UP &&
        m_mouse_action == action)
    {
        m_mouse_action = ACTION_NONE;
        return true;
    }
    
    // otherwise, return false
    return false;
}

bool CompReportDesign::isAction(kcanvas::IEventPtr evt, wxString action)
{
    // if the event is a mouse move event and we
    // already have an action that matches the input 
    // action, return true
    if (evt->getName() == kcanvas::EVENT_MOUSE_MOTION &&
        m_mouse_action == action)
    {
        return true;
    }

    // otherwise, return false
    return false;
}

void CompReportDesign::resetAction()
{
    // reset the mouse action state; this is used to bail out
    // of an action if some condition in the start action isn't 
    // true, so other handlers have a chance to process the event
    m_mouse_action = ACTION_NONE;
}

bool CompReportDesign::saveJson(const wxString& path)
{
    JsonNode root;


    // report info
    JsonNode metadata = root["metadata"];
    metadata["type"] = wxT("application/vnd.kx.report");
    metadata["version"] = 1;
    metadata["description"] = wxT("");
    
    
    // data source info
    JsonNode input = root["input"];
    input["table"].setString(getDataSource());
    input["filter"].setString(getDataFilter()); // TODO: save filter like this?


    // page info    
    int pagewidth, pageheight;
    int leftmargin, rightmargin, topmargin, bottommargin;
    getPageSize(&pagewidth, &pageheight);
    getPageMargins(&leftmargin, &rightmargin, &topmargin, &bottommargin);

    JsonNode page = root["page"];
    page["width"].setInteger(pagewidth);
    page["height"].setInteger(pageheight);
    page["leftmargin"].setInteger(leftmargin);
    page["rightmargin"].setInteger(rightmargin);
    page["topmargin"].setInteger(topmargin);
    page["bottommargin"].setInteger(bottommargin);


    // section info
    root["sections"].setArray();
    JsonNode sections = root["sections"];        

    std::vector<SectionInfo>::const_iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        // if the section isn't active, and the group field doesn't
        // contain anything, then there's nothing to save: move on
        if (!it->m_active && it->m_group_field.Length() == 0)
            continue;

        JsonNode section = sections.appendElement();

        // type of group and whether or not it's visible
        section["type"] = it->m_type;
        section["name"] = it->m_name;
        section["group_field"] = it->m_group_field;
        section["page_break"] = it->m_page_break;
        section["sort_desc"] = it->m_sort_desc;
        section["active"] = it->m_active;
        
        section["column_count"] = it->m_table->getColumnCount();
        section["row_count"] = it->m_table->getRowCount();

        // save the table properties
        saveTablePropertiesToJson(it->m_table, section);
    }

    return JsonConfig::saveToDb(root, g_app->getDatabase(), path, wxT("application/vnd.kx.report"));
}

bool CompReportDesign::loadJson(const wxString& path)
{
    // try to load the JSON string
    JsonNode root = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (!root.isOk())
        return false;


    // TODO: check version info


    // remove all sections
    removeAllSections();


    // data source info
    JsonNode input = root["input"];
    wxString datasource = input["table"].getString();
    wxString datafilter = input["filter"].getString();
    setDataSource(datasource);
    setDataFilter(datafilter);


    // page info    
    JsonNode page = root["page"];
    if (page.isOk())
    {
        int pagewidth = page["width"].getInteger();
        int pageheight = page["height"].getInteger();
        int leftmargin = page["leftmargin"].getInteger();
        int rightmargin = page["rightmargin"].getInteger();
        int topmargin = page["topmargin"].getInteger();
        int bottommargin = page["bottommargin"].getInteger();
        setPageSize(pagewidth, pageheight);
        setPageMargins(leftmargin, rightmargin, topmargin, bottommargin);
    }

    // section info
    JsonNode sections = root["sections"];
    if (sections.isOk())
    {
        std::vector<SectionInfo> section_info;
    
        size_t i, count = sections.getCount();
        for (i =  0; i < count; ++i)
        {
            // get the section
            JsonNode section = sections[i];

            // create the section info
            SectionInfo info;
            info.m_type = section["type"];
            info.m_name = section["name"];
            info.m_group_field = section["group_field"];
            info.m_page_break = section["page_break"].getBoolean();
            info.m_sort_desc = section["sort_desc"].getBoolean();
            info.m_active = section["active"].getBoolean();

            kcanvas::ICompTablePtr table = kcanvas::CompTable::create();
            table->setRowCount(section["row_count"].getInteger());
            table->setColumnCount(section["column_count"].getInteger());
            loadTablePropertiesFromJson(section, table);
            
            info.m_table = table;

            // add the section
            addSection(info);
        }
    }

    return true;
}

bool CompReportDesign::saveXml(const wxString& path)
{
    // create a report store
    ReportStore* store = new ReportStore;
    kcanvas::IStorePtr store_ptr = static_cast<kcanvas::IStore*>(store);

    if (store_ptr.isNull())
        return false;

    // create the file
    kcanvas::IStoreValuePtr data_node;
    data_node = store_ptr->createFile(path);

    if (data_node.isNull())
        return false;

    kcanvas::IStorablePtr storable;

    // save the data source
    kcanvas::IStoreValuePtr source_node = data_node->createChild(wxT("data.source"));
    if (source_node.isNull())
        return false;

    kcanvas::IStoreValuePtr table_name_node = source_node->createChild(wxT("table.name"));
    if (table_name_node.isNull())
        return false;

    wxString data_source = getDataSource();        
    table_name_node->setString(data_source);

    // save the page template
    kcanvas::IStoreValuePtr page_node = data_node->createChild(wxT("template.page"));
    if (page_node.isNull())
        return false;

    // note: the layout component used to have a default page
    // that contained the page size and margin info; this was
    // then serialized directly in the report as a page; this
    // is no longer the case: the design component now stores
    // page size and margin information directly; so the save
    // and load code still works, save this information as
    // a page component
    int page_width, page_height;
    getPageSize(&page_width, &page_height);
    
    int margin_left, margin_right, margin_top, margin_bottom;
    getPageMargins(&margin_left, &margin_right, &margin_top, &margin_bottom);

    kcanvas::IComponentPtr page;
    page = kcanvas::CompPage::create();
    page->setSize(page_width, page_height);

    kcanvas::ICompPagePtr p = page;
    p->setMargins(margin_left, margin_right, margin_top, margin_bottom);

    storable = page;
    if (storable.isNull())
        return false;

    storable->save(page_node);

    // save the rest of the template
    kcanvas::IStoreValuePtr table_node = data_node->createChild(wxT("template.table"));
    if (table_node.isNull())
        return false;

    storable = static_cast<kcanvas::IStorable*>(this);
    storable->save(table_node);
    return true;
}

bool CompReportDesign::loadXml(const wxString& path)
{
    // create a report store
    ReportStore* store = new ReportStore;
    kcanvas::IStorePtr store_ptr = static_cast<kcanvas::IStore*>(store);

    if (store_ptr.isNull())
        return false;

    // open the file
    kcanvas::IStoreValuePtr data_node;
    data_node = store_ptr->loadFile(path);

    if (data_node.isNull())
        return false;

    kcanvas::IStorablePtr storable;

    // load the data source
    kcanvas::IStoreValuePtr source_node = data_node->getChild(wxT("data.source"), false);
    if (source_node.isNull())
        return false;

    kcanvas::IStoreValuePtr table_name_node = source_node->getChild(wxT("table.name"), false);
    if (table_name_node.isNull())
        return false;

    wxString data_source = table_name_node->getString();
    setDataSource(data_source);

    // load the page template
    kcanvas::IStoreValuePtr page_node = data_node->getChild(wxT("template.page"), false);
    if (page_node.isNull())
        return false;

    // note: the layout component used to have a default page
    // that contained the page size and margin info; this was
    // then serialized directly in the report as a page; this
    // is no longer the case: the design component now stores
    // page size and margin information directly; so the save
    // and load code still works, load this information as
    // a page component, then use the page to set the values

    kcanvas::IComponentPtr page;
    page = kcanvas::CompPage::create();
    
    storable = page;
    if (storable.isNull())
        return false;

    storable->load(page_node);

    int page_width, page_height;
    page->getSize(&page_width, &page_height);

    kcanvas::ICompPagePtr p = page;
    int margin_left, margin_right, margin_top, margin_bottom;    
    p->getMargins(&margin_left, &margin_right, &margin_top, &margin_bottom);

    setPageSize(page_width, page_height);
    setPageMargins(margin_left, margin_right, margin_top, margin_bottom);



    // load the rest of the template
    kcanvas::IStoreValuePtr table_node = data_node->getChild(wxT("template.table"), false);
    if (table_node.isNull())
        return false;

    storable = static_cast<kcanvas::IStorable*>(this); 
    if (storable.isNull())
        return false;

    storable->load(table_node);
    return true;
}

void CompReportDesign::updateTableHeaderLayouts()
{
    // if we don't have a canvas, we're done
    kcanvas::ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // get the view origin
    int x_view, y_view;
    canvas->getViewOrigin(&x_view, &y_view);

    // create a view offset so the row controls
    // stay in the same place, but don't scroll
    // past the edge of the model
    int view_offset = x_view;
    if (view_offset < 0)
        view_offset = 0;

    // iterate through the tables and update the table headers
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        // if the row header doesn't exist, move on
        kcanvas::IComponentPtr comp = it->m_row_header;
        if (comp.isNull())
            continue;

        // if the section isn't active, move on 
        if (!(it->m_active))
            continue;

        // update the table row headers
        if (m_update_table_headers)
        {
            // restore and re-tag the table row header default settings
            it->m_row_header->restore(TAG_TABLE_HEADER_DEFAULT);
            it->m_row_header->tag(TAG_TABLE_HEADER_DEFAULT);

            // set the row header parameters
            it->m_row_header->setColumnCount(1);
            it->m_row_header->setRowCount(it->m_table->getRowCount());
            it->m_row_header->setColumnSize(0, TABLE_ROW_HEADER_SIZE);
        
            // copy the initial row sizes from the associated section table
            std::vector<kcanvas::CellProperties> row_sizes;
            it->m_table->getCellProperty(kcanvas::CellRange(-1,-1), kcanvas::PROP_SIZE_H, row_sizes);
            it->m_row_header->addCellProperties(row_sizes, false); // don't replace since we're using tags
        }

        // position the row header; do this regardless of whether
        // or not the header is updated so that the headers position
        // properly when scrolling
        kcanvas::IComponentPtr comp_header = it->m_row_header;
        comp_header->setOrigin(view_offset, it->m_content.GetY());  // view_offset is so controls don't scroll
        comp_header->setSize(TABLE_ROW_HEADER_SIZE, it->m_content.GetHeight());
    }

    // update the table column header
    if (m_update_table_headers)
    {
        // restore and re-tag the table column header default settings
        m_column_header->restore(TAG_TABLE_HEADER_DEFAULT);
        m_column_header->tag(TAG_TABLE_HEADER_DEFAULT);

        // set the column header parameters
        m_column_header->setRowCount(1);
        m_column_header->setColumnCount(m_active_table->getColumnCount());

        std::vector<kcanvas::CellProperties> column_sizes;
        kcanvas::ICompTablePtr detail_table = getSectionTableByName(PROP_REPORT_DETAIL);
        
        if (detail_table.isOk())
        {
            detail_table->getCellProperty(kcanvas::CellRange(-1,-1), kcanvas::PROP_SIZE_W, column_sizes);
            m_column_header->addCellProperties(column_sizes, false); // don't replace since we're using tags
        }
    }
    
    // position the column header; do this regardless of whether
    // or not the header is updated so that the header positions
    // properly when scrolling
    kcanvas::IComponentPtr comp_header = m_column_header;
    comp_header->setOrigin(TABLE_ROW_HEADER_SIZE, y_view);
    comp_header->setSize(getWidth(), TABLE_COLUMN_HEADER_SIZE);
    
    // set the update table header flag to false
    m_update_table_headers = false;
}

void CompReportDesign::addTableHeaders()
{
    // if we don't have a column header, create one
    if (m_column_header.isNull())
    {
        m_column_header = kcanvas::CompTableHeader::create(kcanvas::CompTableHeader::Column);

        // add a name property so we can identify the header
        kcanvas::IComponentPtr comp = m_column_header;
        comp->addProperty(NAME_TABLE_COLUMN_HEADER, wxT(""));

        // set the default column header parameters
        m_column_header->setColumnCount(25);
        m_column_header->setRowCount(1);
        m_column_header->setRowSize(0, TABLE_COLUMN_HEADER_SIZE);

        // tag the column header so we can restore it later;
        // this will allow us to clean up properties that
        // accumulate during actions
        kcanvas::ICompTablePtr table = m_column_header;
        table->tag(TAG_TABLE_HEADER_DEFAULT);
    }

    // if we don't have a row header, create one
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != m_sections.end(); ++it)
    {
        if (!it->m_row_header.isNull())
            continue;

        it->m_row_header = kcanvas::CompTableHeader::create(kcanvas::CompTableHeader::Row);

        // add a name property so we can identify the header
        kcanvas::IComponentPtr comp = it->m_row_header;
        comp->addProperty(NAME_TABLE_ROW_HEADER, wxT(""));

        // set the default row header parameters
        it->m_row_header->setColumnCount(1);
        it->m_row_header->setRowCount(25);
        it->m_row_header->setColumnSize(0, TABLE_ROW_HEADER_SIZE);

        // tag the row header so we can restore it later;
        // this will allow us to clean up properties that
        // accumulate during actions
        kcanvas::ICompTablePtr table = it->m_row_header;
        table->tag(TAG_TABLE_HEADER_DEFAULT);
    }

    // update the headers
    m_update_table_headers = true;
}

void CompReportDesign::addGroupInfo()
{
    // note: this function automatically assigns names
    // and indexes to groups; if the naming convention is 
    // chanded here, it must be changed in code that also 
    // uses this convention, so search for group%d and update
    // all instances

    // iterate through the sections and update group headers
    // with names of the format "group%d.header" where %d is
    // the zero-based index of the group
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    int header_idx = 0;
    for (it = m_sections.begin(); it != m_sections.end(); ++it)
    {
        if (it->m_type != PROP_REPORT_GROUP_HEADER)
            continue;
            
        it->m_name = wxString::Format(wxT("group%d.header"), header_idx);
        it->m_group_idx = header_idx;
        ++header_idx;
    }

    // iterate through the sections and update group footers
    // with names of the format "group%d.footer" where %d is
    // the zero-based index of the group, starting with groups
    // with higher section indexes, and going to groups with
    // lower section indexes (so that lower-order footers
    // correspond to lower-order headers)
    std::vector<SectionInfo>::reverse_iterator itr, itr_end;
    itr_end = m_sections.rend();
    
    int footer_idx = 0;
    for (itr = m_sections.rbegin(); itr != m_sections.rend(); ++itr)
    {
        if (itr->m_type != PROP_REPORT_GROUP_FOOTER)
            continue;
            
        itr->m_name = wxString::Format(wxT("group%d.footer"), footer_idx);
        itr->m_group_idx = footer_idx;
        ++footer_idx;
    }
}

wxString CompReportDesign::getGroupFields(const wxString& group_name, bool all)
{
    // note: this function returns the group fields for a 
    // given group section header; if "all" is true, the 
    // function returns a list of all the group fields from 
    // each of the previous group sections, up to and including 
    // this section; if "all" is false, the function only 
    // returns a list of the group fields from each of the 
    // previous inactive groups sections, up to and including 
    // this section

    wxString group_fields = wxEmptyString;

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    // iterate through the sections and construct a string
    bool first = true;
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        // if we're not on a group header, move on
        if (it->m_type != PROP_REPORT_GROUP_HEADER)
            continue;

        // if the group field is empty, move on
        if (it->m_group_field.IsEmpty())
            continue;
            
        // if the all flag is false, don't accumulate the fields 
        // of groups we're showing, since we only want to show
        // the group fields for inactive sections that the
        // is dependent on
        if (!all && it->m_active && it->m_name != group_name)
            continue;

        if (!first)
            group_fields += wxT(", ");
            
        group_fields += cfw::makeProper(it->m_group_field);
        first = false;

        if (it->m_name == group_name)
            break;
    }

    return group_fields;
}

bool CompReportDesign::hasTable(kcanvas::ICompTablePtr table) const
{
    if (table.isNull())
        return false;

    std::vector<SectionInfo>::const_iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (table == it->m_table)
            return true;
    }
    
    return false;
}

kcanvas::ICompTablePtr CompReportDesign::getNextVisibleTable(int idx) const
{
    // note: returns the next visible table from the
    // specified index
    
    // if we're already at the last table index, we're as 
    // far as we can go; we're done
    if (idx < 0 || idx >= (int)m_sections.size() - 1)
        return xcm::null;

    // return the first valid, visible table with an index
    // greater than than the one we started with
    while (idx < (int)m_sections.size() - 1)
    {
        SectionInfo section = m_sections[idx+1];
        kcanvas::ICompTablePtr table = section.m_table;

        if (section.m_active && !table.isNull())
            return table;

        ++idx;
    }
    
    return xcm::null;
}

kcanvas::ICompTablePtr CompReportDesign::getPrevVisibleTable(int idx) const
{
    // note: returns the previous visible table from the
    // specified index
    
    // if we're already at zero, we're as far as we 
    // can go; we're done
    if (idx <= 0 || idx >= (int)m_sections.size())
        return xcm::null;

    // return the first valid, visible table with an index
    // less than the one we started with
    while (idx > 0)
    {
        SectionInfo section = m_sections[idx-1];
        kcanvas::ICompTablePtr table = section.m_table;
                
        if (section.m_active && !table.isNull())
            return table;

        --idx;
    }
    
    return xcm::null;
}

bool CompReportDesign::setActiveSectionByTable(kcanvas::ICompTablePtr table)
{
    // note: this function sets the focus to a particular
    // table in a section in the report design; returns
    // true if this table is different from the previous
    // table and false otherwise, false otherwise

    if (m_active_table == table)
        return false;

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_table == table)
        {
            m_active_table = table;
            return true;
        }
    }

    return false;
}

int CompReportDesign::getActiveSectionIdx() const
{
    std::vector<SectionInfo>::const_iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        kcanvas::ICompTablePtr active_table;
        active_table = m_active_table;
        
        if (it->m_table == active_table)
            return (it - m_sections.begin());
    }
    
    return -1;
}

bool CompReportDesign::addSection(const SectionInfo& _info)
{
    SectionInfo info = _info;
    if (info.m_table.isNull())
    {
        // if we don't have a table, create a table
        kcanvas::ICompTablePtr table = kcanvas::CompTable::create();
        table->setRowCount(3);
        table->setColumnCount(75);
        info.m_table = table;

        // set the default  font
        setDefaultTableFont(info.m_table);
    }

    // make sure the table is configured to work
    // properly in the section context
    setDefaultTablePreferences(info.m_table);

    // add the section
    m_sections.push_back(info);

    // add the table headers
    addTableHeaders();

    // add the group names and indexes
    addGroupInfo();
    
    // if this is the first section we're adding, set the
    // active table
    if (m_sections.size() == 1)
        setActiveSectionByTable(info.m_table);
    
    return true;
}

void CompReportDesign::setSections(const std::vector<SectionInfo>& sections)
{
    // get the column properties from the report detail table, 
    // which should always exist
    kcanvas::ICompTablePtr detail_table;
    detail_table = getSectionTableByName(PROP_REPORT_DETAIL);
    
    std::vector<kcanvas::CellProperties> cell_properties;
    std::vector<kcanvas::CellProperties> column_properties;
    
    detail_table->getCellProperties(kcanvas::CellRange(-1,-1), cell_properties);
    column_properties.reserve(cell_properties.size());

    std::vector<kcanvas::CellProperties>::iterator it_props, it_props_end;
    it_props_end = cell_properties.end();
    
    for (it_props = cell_properties.begin(); it_props != it_props_end; ++it_props)
    {
        kcanvas::CellRange range = it_props->range();
        if (range.row1() == -1 && range.row2() == -1)
            column_properties.push_back(*it_props);
    }

    // remove all the existing sections
    removeAllSections();

    // reset the active table and section index
    m_active_table = xcm::null;
    m_action_section_idx = -1;

    // set the sections
    m_sections = sections;
    
    // iterate through the sections; if any of the sections don't
    // have a valid table, create a valid table, copying the column
    // properties of the report detail; then, add the table to the 
    // component
    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();
    
    for (it = m_sections.begin(); it != it_end; ++it)
    {
        // reset the row headers
        it->m_row_header = xcm::null;
    
        // set the table
        if (it->m_table.isNull())
        {
            // create a new table and copy the detail column properties
            // to the newly created table
            it->m_table = kcanvas::CompTable::create();
            if (it->m_table.isNull())
                continue;

            // set the default table preferences
            setDefaultTablePreferences(it->m_table);

            // set the row and column count
            it->m_table->setRowCount(2);
            it->m_table->setColumnCount(detail_table->getColumnCount());

            // copy the column properties
            it->m_table->addCellProperties(column_properties, true);
        }
    }

    // add the table headers
    addTableHeaders();

    // add the group names
    addGroupInfo();
    
    // try to set the active section to the detail
    setActiveSectionByName(PROP_REPORT_DETAIL);
}

bool CompReportDesign::getSectionByRowHeader(kcanvas::ICompTablePtr header, 
                                             SectionInfo& section)
{
    if (header.isNull())
        return false;

    std::vector<SectionInfo>::iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (header == it->m_row_header)
        {
            section = *it;
            return true;
        }
    }

    return false;
}

void CompReportDesign::setGroupsFromSortExpr(const wxString& sort_expr)
{
    // note: this function removes any group headers or footers
    // that exists and sets new groups from the sort expression
    
    // iterate through the existing sections and
    // copy out all non-group-related sections

    // clear out the group headers and footers
    std::vector<SectionInfo> new_sections;
    new_sections.reserve(m_sections.size());

    std::vector<SectionInfo>::const_iterator it, it_end;
    it_end = m_sections.end();

    for (it = m_sections.begin(); it != it_end; ++it)
    {
        if (it->m_type == PROP_REPORT_GROUP_HEADER ||
            it->m_type == PROP_REPORT_GROUP_FOOTER)
        {
            continue;
        }
        
        new_sections.push_back(*it);
    }

    setSections(new_sections);

    // get the sort fields
    std::vector< std::pair<wxString, bool> > sort_fields;
    sort_fields = sortExprToVector(sort_expr);

    // insert a group for each field in reverse order,
    // since the last field corresponds to the innermost
    // group
    tango::IDatabasePtr db = g_app->getDatabase();
    std::vector< std::pair<wxString, bool> >::reverse_iterator itr, itr_end;
    itr_end = sort_fields.rend();
    
    for (itr = sort_fields.rbegin(); itr != itr_end; ++itr)
    {
        wxString field = dequoteIdentifier(db, itr->first);
        insertGroup(wxT(""), field, itr->second, false);
    }
}



