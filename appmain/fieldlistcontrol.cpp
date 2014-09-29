/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2004-10-11
 *
 */


#include "appmain.h"
#include "fieldlistcontrol.h"


enum
{
    FieldListCtrlId = 7765
};


// utility functions

wxString xd2text(int xd_type)
{
    switch (xd_type)
    {
        default:
        case xd::typeUndefined:      return _("Undefined");
        case xd::typeInvalid:        return _("Invalid");
        case xd::typeCharacter:      return _("Character");
        case xd::typeWideCharacter:  return _("Wide Char.");
        case xd::typeBinary:         return _("Binary");
        case xd::typeNumeric:        return _("Numeric");
        case xd::typeDouble:         return _("Double");
        case xd::typeInteger:        return _("Integer");
        case xd::typeDate:           return _("Date");
        case xd::typeDateTime:       return _("DateTime");
        case xd::typeBoolean:        return _("Boolean");
    }

    return wxT("");
}

int text2xd(const wxString& text)
{
    if (text.CmpNoCase(_("Invalid")) == 0)
        return xd::typeInvalid;

    if (text.CmpNoCase(_("Character")) == 0)
        return xd::typeCharacter;

    if (text.CmpNoCase(_("Wide Char.")) == 0)
        return xd::typeWideCharacter;

    if (text.CmpNoCase(_("Binary")) == 0)
        return xd::typeBinary;

    if (text.CmpNoCase(_("Numeric")) == 0)
        return xd::typeNumeric;

    if (text.CmpNoCase(_("Double")) == 0)
        return xd::typeDouble;

    if (text.CmpNoCase(_("Integer")) == 0)
        return xd::typeInteger;

    if (text.CmpNoCase(_("Date")) == 0)
        return xd::typeDate;

    if (text.CmpNoCase(_("DateTime")) == 0)
        return xd::typeDateTime;

    if (text.CmpNoCase(_("Boolean")) == 0)
        return xd::typeBoolean;

    return xd::typeUndefined;
}


// predicate helper function for sorting the FieldListItem class

static bool sortByNameA(const FieldListItem& l, const FieldListItem& r)
{
    return l.name.CmpNoCase(r.name) < 0;
}

static bool sortByNameD(const FieldListItem& l, const FieldListItem& r)
{
    return l.name.CmpNoCase(r.name) > 0;
}




BEGIN_EVENT_TABLE(FieldListControl, wxPanel)
    EVT_KCLGRID_CELL_LEFT_DCLICK(FieldListControl::onGridCellDblClick)
    EVT_KCLGRID_COLUMN_LEFT_CLICK(FieldListControl::onGridColumnClicked)
    EVT_SIZE(FieldListControl::onSize)
END_EVENT_TABLE()


FieldListControl::FieldListControl(wxWindow* parent,
                                   wxWindowID id,
                                   const wxPoint& position,
                                   const wxSize& size,
                                   long style)
                    : wxPanel(parent,
                              -1,
                              position,
                              (size == wxDefaultSize) ? wxSize(140,30) : size,
                              style)
{
    m_grid = new kcl::RowSelectionGrid(this, id,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxBORDER_NONE,
                                       true, false);
    m_grid->setOptionState(kcl::Grid::optColumnResize |
                           kcl::Grid::optEdit, false);
    m_grid->setGreenBarInterval(0);

    m_grid->createModelColumn(-1, _("Name"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Type"), kcl::Grid::typeCharacter, 40, 0);
    m_grid->createModelColumn(-1, _("Width"), kcl::Grid::typeInteger, 9, 0);
    m_grid->createModelColumn(-1, _("Dec."), kcl::Grid::typeInteger, 9, 0);

    // it is very important for grid drag and drop purposes that this grid
    // has the "id" that is assigned in this constructor assigned to it
    m_grid->createDefaultView();

    m_grid->setRowHeight(m_grid->getRowHeight()-2);

    // only show the name column by default
    showColumns(ColName);


    // create main sizer
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_grid, 1, wxEXPAND);

    SetAutoLayout(true);
    SetSizer(main_sizer);

    m_sort_state = SortNone;
    
    m_minWidth = size.x;
    m_minHeight = size.y;
}

FieldListControl::~FieldListControl()
{
    for (std::vector<FieldListItem*>::iterator fit = m_to_delete.begin(); fit != m_to_delete.end(); ++fit)
        delete (*fit);
}

void FieldListControl::refresh()
{
    m_grid->refresh(kcl::Grid::refreshAll);
}

void FieldListControl::showColumns(unsigned int col_mask,
                                   bool refresh,
                                   bool proportional)
{
    m_grid->hideAllColumns();
    int col_count = m_grid->getColumnCount();

    if (col_mask & ColName)
    {
        m_grid->insertColumn(-1, ColNameIdx);
        m_grid->setColumnProportionalSize(col_count++, 10);
    }

    if (col_mask & ColType)
    {
        m_grid->insertColumn(-1, ColTypeIdx);
        m_grid->setColumnProportionalSize(col_count++, 6);
    }

    if (col_mask & ColWidth)
    {
        m_grid->insertColumn(-1, ColWidthIdx);
        m_grid->setColumnProportionalSize(col_count++, 4);
    }

    if (col_mask & ColScale)
    {
        m_grid->insertColumn(-1, ColScaleIdx);
        m_grid->setColumnProportionalSize(col_count++, 4);
    }

    if (refresh)
        m_grid->refresh(kcl::Grid::refreshAll);

    if (!proportional)
    {
        m_grid->setOptionState(kcl::Grid::optColumnResize, true);

        // harden the proportional size
        int col_count = m_grid->getColumnCount();
        int i;
        
        for (i = 0; i < col_count; ++i)
        {
            m_grid->setColumnSize(i, m_grid->getColumnSize(i));
            m_grid->setColumnProportionalSize(i, 0);
        }
    }
}

void FieldListControl::setColumnCaption(int col, const wxString& new_value)
{
    m_grid->setColumnCaption(col, new_value);
}

void FieldListControl::clearSelection()
{
    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshPaint);
}

void FieldListControl::sortList(int sort_state)
{
    m_sort_state = sort_state;
}

void FieldListControl::setDragFormat(const wxString& format)
{
    m_grid->setDragFormat(format);
}

void FieldListControl::setStructure(xd::IStructurePtr structure)
{
    m_structure = structure;
    populate();
}

void FieldListControl::setItemEnabled(const wxString& item_name, bool enabled)
{
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        wxString s = m_grid->getCellString(row, ColNameIdx);
        if (item_name.CmpNoCase(s) == 0)
        {
            FieldListItem* f = (FieldListItem*)m_grid->getRowData(row);
            if (!f)
                return;
            
            f->enabled = enabled;

            if (f->bitmap.IsOk())
            {
                wxBitmap bmp = enabled ? f->bitmap
                                       : BitmapMgr::makeDisabledBitmap(f->bitmap);
                m_grid->setCellBitmap(row, ColNameIdx, bmp);
            }
            
            return;
        }
    }
}

void FieldListControl::addCustomItem(const wxString& name,
                                     const wxBitmap& bitmap,
                                     int type,
                                     int width,
                                     int scale)
{
    FieldListItem fli;
    fli.name = name;
    fli.bitmap = bitmap;
    fli.type = type;
    fli.width = width;
    fli.scale = scale;
    fli.enabled = true;

    m_custom_items.push_back(fli);
}

std::vector<FieldListItem> FieldListControl::getSelectedItems()
{
    std::vector<FieldListItem> items;

    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        if (m_grid->isRowSelected(row))
        {
            FieldListItem* f = (FieldListItem*)m_grid->getRowData(row);
            items.push_back(*f);
        }
    }

    return items;
}

std::vector<FieldListItem> FieldListControl::getAllItems()
{
    std::vector<FieldListItem> items;

    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        FieldListItem* f = (FieldListItem*)m_grid->getRowData(row);
        items.push_back(*f);
    }

    return items;
}

void FieldListControl::populate()
{
    for (std::vector<FieldListItem*>::iterator fit = m_to_delete.begin(); fit != m_to_delete.end(); ++fit)
        delete (*fit);
    m_to_delete.clear();

    // if no structure has been set, we can't continue
    if (m_structure.isNull())
        return;

    // first, clear out the grid
    m_grid->deleteAllRows();

    
    int col_count = m_structure->getColumnCount();

    // create a temporary vector of field list items
    // to be used to populate the grid
    std::vector<FieldListItem> fields;
    for (int i = 0; i < col_count; i++)
    {
        const xd::ColumnInfo& colinfo = m_structure->getColumnInfoByIdx(i);

        FieldListItem fli;
        fli.name = makeProperIfNecessary(colinfo.name);
        fli.bitmap = colinfo.calculated ? GETBMP(gf_lightning_16) : GETBMP(gf_field_16);
        fli.type = colinfo.type;
        fli.width = colinfo.width;
        fli.scale = colinfo.scale;
        fli.enabled = true;
        
        fields.push_back(fli);
    }

    // sort the fields (disregarding the custom items)
    // and set the sort bitmap in the grid
    if (m_sort_state == SortAscending)
    {
        std::sort(fields.begin(), fields.end(), sortByNameA);
        m_grid->setColumnBitmap(0, GETBMP(xpm_columnsortup),
                                kcl::Grid::alignRight);
    }
     else if (m_sort_state == SortDescending)
    {
        std::sort(fields.begin(), fields.end(), sortByNameD);
        m_grid->setColumnBitmap(0, GETBMP(xpm_columnsortdown),
                                kcl::Grid::alignRight);
    }
     else
    {
        m_grid->setColumnBitmap(0, GETBMP(xpm_columnsortnone),
                                kcl::Grid::alignRight);
    }

    int idx = 0;
    std::vector<FieldListItem>::iterator it;

    // populate the grid with the custom items
    for (it = m_custom_items.begin(); it != m_custom_items.end(); ++it)
    {
        m_grid->insertRow(-1);

        if (m_grid->getColumnViewIdx(ColNameIdx) != -1)
        {
            m_grid->setCellString(idx, ColNameIdx, it->name);
            m_grid->setCellBitmap(idx, ColNameIdx, it->bitmap);
        }

        if (m_grid->getColumnViewIdx(ColTypeIdx) != -1)
            m_grid->setCellString(idx, ColTypeIdx, xd2text(it->type));

        if (m_grid->getColumnViewIdx(ColWidthIdx) != -1)
            m_grid->setCellInteger(idx, ColWidthIdx, it->width);

        if (m_grid->getColumnViewIdx(ColScaleIdx) != -1)
            m_grid->setCellInteger(idx, ColScaleIdx, it->scale);
        
        // create the row data to store the field list item
        FieldListItem* f = new FieldListItem;
        f->name = it->name;
        f->bitmap = it->bitmap;
        f->type = it->type;
        f->width = it->width;
        f->scale = it->scale;
        f->enabled = it->enabled;
        m_to_delete.push_back(f);
        m_grid->setRowData(idx, (long)f);
        
        idx++;
    }

    // populate grid with items from the table structure
    for (it = fields.begin(); it != fields.end(); ++it)
    {
        m_grid->insertRow(-1);

        if (m_grid->getColumnViewIdx(ColNameIdx) != -1)
        {
            m_grid->setCellString(idx, ColNameIdx, it->name);
            m_grid->setCellBitmap(idx, ColNameIdx, it->bitmap);
        }

        if (m_grid->getColumnViewIdx(ColTypeIdx) != -1)
            m_grid->setCellString(idx, ColTypeIdx, xd2text(it->type));

        if (m_grid->getColumnViewIdx(ColWidthIdx) != -1)
            m_grid->setCellInteger(idx, ColWidthIdx, it->width);

        if (m_grid->getColumnViewIdx(ColScaleIdx) != -1)
            m_grid->setCellInteger(idx, ColScaleIdx, it->scale);
        
        // create the row data to store the field list item
        FieldListItem* f = new FieldListItem;
        f->name = it->name;
        f->bitmap = it->bitmap;
        f->type = it->type;
        f->width = it->width;
        f->scale = it->scale;
        f->enabled = it->enabled;
        m_to_delete.push_back(f);
        m_grid->setRowData(idx, (long)f);
        
        idx++;
    }

    m_grid->refresh(kcl::Grid::refreshAll);
}

void FieldListControl::onGridCellDblClick(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    wxString text = m_grid->getCellString(row, 0);

    if (text.IsEmpty())
        return;

    // only allow one row to be selected if the user double clicks
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->refresh(kcl::Grid::refreshAll);
    
    sigFieldDblClicked(row, text);
}

void FieldListControl::onGridColumnClicked(kcl::GridEvent& evt)
{   
    if (evt.GetColumn() == 0)
    {
        m_sort_state++;
        if (m_sort_state == 3)
            m_sort_state = 0;
       
        populate();
    }
}

void FieldListControl::onSize(wxSizeEvent& evt)
{
    evt.Skip();
}




