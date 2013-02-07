/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-01-21
 *
 */


#include "appmain.h"
#include "panelcolumnlist.h"


class ColumnListInfoLess
{
public:

     bool operator()(const ColumnListItem& c1,
                     const ColumnListItem& c2) const                
     {
         return wcscasecmp(c1.text.c_str(),
                           c2.text.c_str()) < 0 ? true : false;
     }
};


enum
{
    FieldListCtrlId = 7765
};


BEGIN_EVENT_TABLE(ColumnListPanel, wxPanel)
    EVT_KCLGRID_CELL_LEFT_DCLICK(ColumnListPanel::onGridCellDblClick)
    EVT_KCLGRID_COLUMN_LEFT_CLICK(ColumnListPanel::onGridColumnClick)
    EVT_SIZE(ColumnListPanel::onSize)
    EVT_SET_FOCUS(ColumnListPanel::onSetFocus)
    EVT_KILL_FOCUS(ColumnListPanel::onKillFocus)
END_EVENT_TABLE()


ColumnListPanel::ColumnListPanel()
{
    m_grid = NULL;
    m_last_target_site_id = -1;
}

ColumnListPanel::~ColumnListPanel()
{
}


// -- IDocument --
bool ColumnListPanel::initDoc(IFramePtr frame,
                         IDocumentSitePtr site,
                         wxWindow* docsite_wnd,
                         wxWindow* panesite_wnd)
{
    if (!Create(docsite_wnd, -1,
                wxDefaultPosition,
                wxDefaultSize,
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
    {
        return false;
    }

    site->setCaption(_("Fields"));

    // -- save site id --
    m_site_id = site->getId();

    m_grid = new kcl::RowSelectionGrid(this, wxID_ANY,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxBORDER_NONE,
                                       true, false);
    m_grid->setOptionState(kcl::Grid::optColumnResize |
                           kcl::Grid::optEdit, false);
    m_grid->setOptionState(kcl::Grid::optInvalidAreaClickable, true);
    m_grid->setDragFormat(wxT("fieldspanel"));
    m_grid->setGreenBarInterval(0);

    m_grid->createModelColumn(-1, wxEmptyString, kcl::Grid::typeCharacter, 256, 0);
    m_grid->createDefaultView();

    m_grid->setColumnBitmap(0, wxNullBitmap,
                            kcl::Grid::alignRight);

    m_grid->setRowHeight(m_grid->getRowHeight()-2);
    m_grid->setColumnProportionalSize(0, 1);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_grid, 1, wxEXPAND);
    SetSizer(sizer);
    SetAutoLayout(true);
    
    m_sort_state = sortNone;

    frame->sigFrameEvent().connect(this, &ColumnListPanel::onFrameEvent);
    frame->sigActiveChildChanged().connect(this, &ColumnListPanel::onActiveChildChanged);

    // -- populate based on active container --
    m_grid->clearSelection();
    populate();

    return true;
}

wxWindow* ColumnListPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ColumnListPanel::setDocumentFocus()
{
    m_grid->SetFocus();
}

void ColumnListPanel::onActiveChildChanged(IDocumentSitePtr doc_site)
{
    m_grid->clearSelection();
    populate();
}

bool ColumnListPanel::onSiteClosing(bool force)
{
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSiteById(m_site_id);
    
    site->setVisible(false);
    
    // if we close the site and reopen it, it seems very strange to have
    // an existing selection in the grid, so just remove the selection here
    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshAll);
    
    return true;
}

void ColumnListPanel::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_TABLEDOC_STRUCTURE_MODIFIED ||
        evt.name == FRAMEWORK_EVT_APPMAIN_RELATIONSHIPS_UPDATED ||
        evt.name == FRAMEWORK_EVT_TABLEDOC_VIEW_CHANGED ||
        evt.name == FRAMEWORK_EVT_COLUMNLISTPANEL_UPDATE)
    {
        m_grid->clearSelection();
        populate();
    }

    // populate if the column ordering was changed (insert or delete)...
    // don't populate if the column was just resized
    if (evt.name == FRAMEWORK_EVT_TABLEDOC_VIEW_MODIFIED && evt.l_param == 0)
    {
        if (evt.s_param != wxT("colmove"))
        {
            populate();
        }
    }
}

void ColumnListPanel::populate()
{
    int saved_offset = m_grid->getRowOffset();
    
    // -- first, delete all present items --
    m_grid->deleteAllRows();

    if (g_app->getMainFrame().isNull())
    {
        m_grid->setColumnBitmap(0, wxNullBitmap,
                                kcl::Grid::alignRight);
        m_grid->refresh(kcl::Grid::refreshAll);
        return;
    }

    IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
    if (doc_site.isNull())
    {
        m_grid->setColumnBitmap(0, wxNullBitmap,
                                kcl::Grid::alignRight);
        m_grid->refresh(kcl::Grid::refreshAll);
        return;
    }

    IColumnListTargetPtr target = doc_site->getDocument();
    
    if (target.isNull())
    {
        // -- document doesn't support IColumnListTarget --
        m_grid->setColumnBitmap(0, wxNullBitmap,
                                kcl::Grid::alignRight);
        m_grid->refresh(kcl::Grid::refreshAll);
        return;
    }
        
        
    std::vector<ColumnListItem> cols;
    target->getColumnListItems(cols);
    
    
    
    if (m_sort_state == sortAscending)
    {
        std::sort(cols.begin(), cols.end(), ColumnListInfoLess());

        m_grid->setColumnBitmap(0, GETBMP(xpm_columnsortup),
                                kcl::Grid::alignRight);
    }
     else if (m_sort_state == sortDescending)
    {
        std::sort(cols.rbegin(), cols.rend(), ColumnListInfoLess());

        m_grid->setColumnBitmap(0, GETBMP(xpm_columnsortdown),
                                kcl::Grid::alignRight);
    }
     else
    {
        m_grid->setColumnBitmap(0, GETBMP(xpm_columnsortnone),
                                kcl::Grid::alignRight);
    }

    int idx = 0;
    std::vector<ColumnListItem>::iterator it, it_end = cols.end();
    for (it = cols.begin(); it != it_end; ++it)
    {
        // -- insert the row for the string and bitmap --
        m_grid->insertRow(-1);

        m_grid->setCellBitmap(idx, 0, it->bitmap);
        m_grid->setCellString(idx++, 0, it->text);
    }


    // if we are refreshing, and the target document hasn't changed,
    // then simply restore the grid offset where it was; otherwise
    // grid's offset row will be reset to zero
    
    if (doc_site->getId() == m_last_target_site_id)
    {
        m_grid->setRowOffset(saved_offset);
    }
     else
    {
        m_grid->setRowOffset(0);
        m_last_target_site_id = doc_site->getId();
    }


    m_grid->refresh(kcl::Grid::refreshAll);
}


void ColumnListPanel::onGridCellDblClick(kcl::GridEvent& evt)
{
    IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
    if (!doc_site)
        return;
        
    IColumnListTargetPtr target = doc_site->getDocument();
    if (!target)
    {
        return;
    }

    // now that we have more comprehensive drag-and-drop, it seems like a
    // good practice to limit the field list double-click to a single row
    
    int row = evt.GetRow();
    
    int i, counter = 0, row_count = m_grid->getRowCount();
    for (i = 0; i < row_count; ++i)
    {
        if (m_grid->isRowSelected(i))
        {
            counter++;
        }
    }

    if (counter > 1)
    {
        m_grid->clearSelection();
        m_grid->setRowSelected(row, true);
        m_grid->refresh(kcl::Grid::refreshAll);
    }
    
    std::vector<wxString> items;
    items.push_back(m_grid->getCellString(row, 0));
    target->onColumnListDblClicked(items);
}

void ColumnListPanel::onGridColumnClick(kcl::GridEvent& evt)
{
    m_sort_state++;
    if (m_sort_state == 3)
        m_sort_state = 0;

    m_grid->clearSelection();
    populate();
}

void ColumnListPanel::onSize(wxSizeEvent& evt)
{
    evt.Skip();
}

void ColumnListPanel::onSetFocus(wxFocusEvent& evt)
{
    setDocumentFocus();
}

void ColumnListPanel::onKillFocus(wxFocusEvent& evt)
{
}
