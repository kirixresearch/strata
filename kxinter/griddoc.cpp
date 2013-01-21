/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-02-14
 *
 */


#include "kxinter.h"
#include "griddoc.h"
#include "../kappcmn/tangogridmodel.h"
#include "treecontroller.h"
#include "jobconvert.h"


enum
{
    ID_SetOrderAscending = 20000,
    ID_SetOrderDescending
};


BEGIN_EVENT_TABLE(GridDoc, wxWindow)
    EVT_MENU(ID_SetOrderAscending, GridDoc::onSetOrderAscending)
    EVT_MENU(ID_SetOrderDescending, GridDoc::onSetOrderDescending)
    EVT_KCLGRID_COLUMN_RIGHT_CLICK(GridDoc::onGridColumnRightClick)
    EVT_SIZE(GridDoc::onSize)
END_EVENT_TABLE()



GridDoc::GridDoc()
{
    m_grid = NULL;
}

GridDoc::~GridDoc()
{

}

bool GridDoc::initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd)
{
    // -- create document's window --
    bool result = Create(docsite_wnd, -1, wxDefaultPosition, docsite_wnd->GetClientSize(), wxCLIP_CHILDREN | wxNO_FULL_REPAINT_ON_RESIZE);
    
    if (!result)
    {
        return false;
    }

    int w, h;
    GetClientSize(&w, &h);


    doc_site->setCaption(wxT(""));
    wxSize min_site_size = doc_site->getSiteWindow()->GetSize();
    doc_site->setMinSize(min_site_size.x, min_site_size.y);

    m_doc_site = doc_site;

    // -- create the grid control --
    m_grid = new kcl::Grid;
    m_grid->setCursorType(kcl::Grid::cursorNormal);
    m_grid->setBorderType(wxSUNKEN_BORDER);
    m_grid->createDefaultView();
    m_grid->Create(this, -1, wxPoint(0,0), wxSize(w,h), 0, kcl::Grid::stateHidden);


    wxBoxSizer* main_sizer = new wxBoxSizer(wxHORIZONTAL);
    main_sizer->Add(m_grid, 1, wxEXPAND);

    SetSizer(main_sizer);

    return true;
}

wxWindow* GridDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void GridDoc::setDocumentFocus()
{

}

bool GridDoc::onSiteClosing(bool force)
{
    if (!strcasecmp(m_doc_site->getCaption().mbc_str(), "Query"))
    {
        int result;
        result = cfw::appMessageBox(_("Would you like to save this data set?"),
                               PRODUCT_NAME,
                               wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER,
                               g_app->getMainWindow());

        if (result == wxYES)
        {
            wxTextEntryDialog dlg(g_app->getMainWindow(), wxT("Input the table name:"),
                                  wxT("Save Query"));

            if (dlg.ShowModal() == wxID_OK)
            {
                wxString table_name = dlg.GetValue();

                IConnectionPtr c = g_app->getConnectionMgr()->getActiveConnection();
                tango::IDatabasePtr db_ptr = c->getDatabasePtr();

                // -- this function is temporary for aaron's data analysis --
                ConvertJob* job = new ConvertJob;
                job->createAndSaveSetFromIterator(db_ptr, dlg.GetValue(), m_iter);
                g_app->getTreeController()->refreshAllItems();
            }
             else
            {
                return false;
            }
        }

        if (result == wxCANCEL)
        {
            return false;
        }

    }

    // -- kill all references to everything --
    m_doc_site.clear();
    m_iter.clear();

    if (m_grid)
    {
        delete m_grid;
        m_grid = NULL;
    }

    return true;
}



void GridDoc::onSetOrderAscending(wxCommandEvent& event)
{
    wxString col_caption;

    // -- create order string --
    wxString expr;

    int i = 0;
    int col_count = m_grid->getColumnCount();
    bool first = true;

    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            if (!first)
            {
                expr += L",";
            }
            first = false;

            col_caption = m_grid->getColumnCaption(i);
            expr += col_caption;
        }
    }


    tango::ISetPtr set = m_iter->getSet();
    if (set)
    {
        tango::IIteratorPtr new_iter = set->createIterator(L"", towstr(expr), NULL);
        setIterator(new_iter);

        // -- clear selection --
        m_grid->clearSelection();
        m_grid->refresh(kcl::Grid::refreshAll);
    }
}

void GridDoc::onSetOrderDescending(wxCommandEvent& event)
{
    wxString col_caption;

    // -- create order string --
    wxString expr;

    int i = 0;
    int col_count = m_grid->getColumnCount();
    bool first = true;

    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            if (!first)
            {
                expr += L",";
            }
            first = false;

            col_caption = m_grid->getColumnCaption(i);
            expr += col_caption;
        }
    }

    expr += L" DESC";


    tango::ISetPtr set = m_iter->getSet();
    if (set)
    {
        tango::IIteratorPtr new_iter = set->createIterator(L"", towstr(expr), NULL);
        setIterator(new_iter);

        // -- clear selection --
        m_grid->clearSelection();
        m_grid->refresh(kcl::Grid::refreshAll);
    }
}

void GridDoc::onGridColumnRightClick(kcl::GridEvent& event)
{
    m_grid->moveCursor(m_grid->getCursorRow(), event.GetColumn());


    // -- remove all non-column selections --
    int i;
    int sel_count = m_grid->getSelectionCount();
    kcl::SelectionRect r;
    for (i = 0; i < sel_count; ++i)
    {
        m_grid->getSelection(i, &r);
        if (r.m_start_row != 0 || r.m_end_row != -1)
        {
            m_grid->removeSelection(i);
            i = -1;
            sel_count = m_grid->getSelectionCount();
        }
    }

    if (!m_grid->isColumnSelected(event.GetColumn()))
    {
        m_grid->clearSelection();
        m_grid->setColumnSelected(event.GetColumn(), true);
    }

    m_grid->refresh(kcl::Grid::refreshPaint);



    cfw::wxBitmapMenu menuPopup;
    menuPopup.Append(ID_SetOrderAscending, _("Sort &Ascending"), GET_XPM(xpm_sort));
    menuPopup.Append(ID_SetOrderDescending, _("&Sort Descending"), GET_XPM(xpm_sortdescending));
    

    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);
    cfw::CommandCapture* cc = new cfw::CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);


    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, command);
    ::wxPostEvent(g_app->getMainWindow(), evt);
}

void GridDoc::onSize(wxSizeEvent& event)
{
    if (m_grid)
    {
        wxWindow* window = m_grid;
        window->SetSize(event.GetSize());
    }
}

tango::ISetPtr GridDoc::getSet()
{
    return xcm::null;
}

void GridDoc::setName(const wxString& name)
{
    m_doc_site->setCaption(name);
}


bool GridDoc::setIterator(tango::IIteratorPtr iter)
{
    if (!iter)
    {
        return false;
    }

    m_iter = iter;

    ITangoGridModel* model = new TangoGridModel;
    model->setIterator(iter);

    // -- set the model for the grid control --
    m_grid->setModel(model);
    m_grid->createDefaultView();
    m_grid->setVisibleState(kcl::Grid::stateVisible);
    m_grid->refresh(kcl::Grid::refreshAll);

    return true;
}

