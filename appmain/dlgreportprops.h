/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-07
 *
 */


#ifndef H_APP_DLGREPORTPROPS_H
#define H_APP_DLGREPORTPROPS_H


// forward declarations
class OrientationPanel;
class PageSizePanel;
class MarginsPanel;


class ReportGroupItem
{
public:

    enum SortDirection
    {
        SortAsc = 0,
        SortDesc = 1,
    };

    ReportGroupItem()
    {
        name = wxEmptyString;
        id = wxEmptyString;
        sort = SortAsc;
        header = false;
        footer = false;
        page_break = false;
    }
    
    ReportGroupItem(const ReportGroupItem& c)
    {
        name = c.name;
        id = c.id;
        sort = c.sort;
        header = c.header;
        footer = c.footer;
        page_break = c.page_break;
    }
    
    ReportGroupItem& operator=(const ReportGroupItem& c)
    {
        name = c.name;
        id = c.id;
        sort = c.sort;
        header = c.header;
        footer = c.footer;
        page_break = c.page_break;
        return *this;
    }
    
    wxString name;      // field name to group on
    wxString id;        // id to uniquely define data row in dialog
    int sort;           // sort order
    bool header;        // true if header is to be shown
    bool footer;        // true if footer is to be shown
    bool page_break;    // true if new page is to be created
};

class ReportSettings
{
public:
    
    ReportSettings()
    {
        source = wxEmptyString;
        int orientation = wxPORTRAIT;
        
        page_height = 11.0;
        page_width = 8.5;
        
        margin_left = 1.0;
        margin_right = 1.0;
        margin_top = 1.0;
        margin_bottom = 1.0;
        
        report_header = false;
        report_footer = false;
        page_header = true;
        page_footer = true;
    }
    
    ReportSettings(const ReportSettings& c)
    {
        source = c.source;
        orientation = c.orientation;
        
        page_height = c.page_height;
        page_width = c.page_width;
        
        margin_left = c.margin_left;
        margin_right = c.margin_right;
        margin_top = c.margin_top;
        margin_bottom = c.margin_bottom;
        
        report_header = c.report_header;
        report_footer = c.report_footer;
        page_header = c.page_header;
        page_footer = c.page_footer;
        
        groups = c.groups;
    }

    ReportSettings& operator=(const ReportSettings& c)
    {
        source = c.source;
        orientation = c.orientation;
        
        page_height = c.page_height;
        page_width = c.page_width;
        
        margin_left = c.margin_left;
        margin_right = c.margin_right;
        margin_top = c.margin_top;
        margin_bottom = c.margin_bottom;
        
        report_header = c.report_header;
        report_footer = c.report_footer;
        page_header = c.page_header;
        page_footer = c.page_footer;
        
        groups = c.groups;
        return *this;
    }

    void clearGroups()
    {
        groups.clear();
    }
    
    void addGroup(const wxString& name,
                  const wxString& id,
                  int sort = ReportGroupItem::SortAsc,
                  bool header = true,
                  bool footer = true,
                  bool page_break = false)
    {
        ReportGroupItem item;
        item.name = name;
        item.id = id;
        item.sort = sort;
        item.header = header;
        item.footer = footer;
        item.page_break = page_break;
        
        groups.push_back(item);
    }
    
    ReportGroupItem getGroup(size_t idx)
    {
        wxASSERT_MSG(idx < groups.size(), wxT("Report group item index out of bounds!"));
        if (idx < groups.size())
        {
            return groups[idx];
        }
        else
        {
            return ReportGroupItem();
        }
    }
    
    size_t getGroupCount()
    {
        return groups.size();
    }
    
    wxString source;
    int orientation;
    
    double page_height;
    double page_width;
    
    double margin_left;
    double margin_right;
    double margin_top;
    double margin_bottom;
    
    bool report_header;
    bool report_footer;
    bool page_header;
    bool page_footer;
    
    std::vector<ReportGroupItem> groups;
};





class ReportPropsDialog : public wxDialog,
                          public xcm::signal_sink
{
public:

    ReportPropsDialog(wxWindow* parent);
    ~ReportPropsDialog();
    
    // wxDialog override
    int ShowModal();
    
    void setSettings(const ReportSettings& settings);
    ReportSettings getSettings() const;

private:
    
    void populateFieldNameDropDown();
    void validateFieldNames();
    void validateFieldNameByRow(int row);
    void checkOverlayText();
    void updateEnabled();
    
private:

    // event handlers
    void onOK(wxCommandEvent& evt);
    void onBrowse(wxCommandEvent& evt);
    void onSourceTextChanged(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);

    // grid event handers    
    void onGridEndEdit(kcl::GridEvent& evt);
    void onGridNeedTooltipText(kcl::GridEvent& evt);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);
    
    // signal event handlers
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    void onDeletedRows(std::vector<int> rows);
    
private:
    
    std::vector<wxString> m_source_columns;
    
    OrientationPanel* m_orientation_panel;
    PageSizePanel* m_pagesize_panel;
    MarginsPanel* m_margins_panel;
    
    wxTextCtrl* m_source;
    wxCheckBox* m_report_header;
    wxCheckBox* m_report_footer;
    wxCheckBox* m_page_header;
    wxCheckBox* m_page_footer;
    kcl::RowSelectionGrid* m_grid;
        
    ReportSettings m_settings;
    
    DECLARE_EVENT_TABLE()
};


#endif  // __APP_DLGREPORTPROPS_H




