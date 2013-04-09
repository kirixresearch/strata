/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2008-06-06
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scripttable.h"
#include "../kcanvas/component.h"
#include "../kcanvas/componenttable.h"
#include "../kcanvas/util.h"


// (TODO:CLASS) TableBox
// Category: Control
// Derives: FormControl
// Description: A class that represents a table control.
// Remarks: The TableBox class represents a table control which can
//     be added to a window.

TableBox::TableBox()
{
    m_ctrl = NULL;
    m_parser = NULL;
}

TableBox::~TableBox()
{

}

void TableBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
        m_ctrl->Enable(false);
}

// (TODO:CONSTRUCTOR) TableBox.constructor
// Description: Creates a new TableBox object.
//
// Syntax: TableBox(x_pos : Integer,
//                  y_pos : Integer,
//                  width : Integer,
//                  height : Integer)
//
// Remarks: Creates a new TableBox at the position specified 
//     by |x_pos| and |y_pos|, and having dimensions specified 
//     by |width| and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void TableBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // set the parser
    m_parser = env->getParser();

    // initialize the component
    initComponent(env);

    // add the events
    getMember(L"cellEditing")->setObject(Event::createObject(env));
    getMember(L"cellEdited")->setObject(Event::createObject(env));

    // set default values
    wxString label = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
        label = env->getParam(0)->getString();
    if (param_count > 1)
        m_x = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_y = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_width = env->getParam(3)->getInteger();
    if (param_count > 4)
        m_height = env->getParam(4)->getInteger();

    if (param_count < 2)
    {
        // create the control
        m_ctrl = new TableCtrl(getApp()->getTempParent(),
                               -1,
                               wxDefaultPosition,
                               wxDefaultSize);

        m_wnd = m_ctrl;

        // if no sizing parameters were specified, used wx's default
        // parameters, since they are equal to the exact size of the text
        wxSize s = m_ctrl->GetSize();
        m_width = s.GetWidth();
        m_height = s.GetHeight();
    }
    else
    {
        // create the control
        m_ctrl = new TableCtrl(getApp()->getTempParent(),
                              -1,
                              wxPoint(m_x, m_y),
                              wxSize(m_width, m_height));
        m_wnd = m_ctrl;

        // make sure we update the m_width and m_height member
        // variables based on the actual size of the control
        wxSize s = m_ctrl->GetSize();
        if (m_width != s.GetWidth())
            m_width = s.GetWidth();
        if (m_height != s.GetHeight())
            m_height = s.GetHeight();
    }
    
    // add an event handler
    kcanvas::EventHandler* handler = new kcanvas::EventHandler;
    handler->sigEvent().connect(this, &TableBox::onEvent);
    m_ctrl->addEventHandler(kcanvas::EVENT_TABLE_PRE_CELL_EDIT, handler);
    m_ctrl->addEventHandler(kcanvas::EVENT_TABLE_CELL_EDIT, handler);

    // connect the renderers
    std::vector<kcanvas::IComponentPtr> renderers;
    m_ctrl->getTablePtr()->getRenderers(renderers);

    std::vector<kcanvas::IComponentPtr>::iterator it_r, it_end_r;
    it_end_r = renderers.end();
    
    for (it_r = renderers.begin(); it_r != it_end_r; ++it_r)
    {
        kcanvas::IComponentPtr r = *it_r;
        r->getPropertiesRef().sigChanged().connect(this, &TableBox::onRendererUpdated);
    }
}

void TableBox::setRowCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    int count = env->getParam(0)->getInteger();
    m_ctrl->setRowCount(count);
}

void TableBox::getRowCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int count = m_ctrl->getRowCount();
    retval->setInteger(count);
}

void TableBox::setColumnCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    int count = env->getParam(0)->getInteger();
    m_ctrl->setColumnCount(count);
}

void TableBox::getColumnCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int count = m_ctrl->getColumnCount();
    retval->setInteger(count);
}

void TableBox::insertRow(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    int idx = 0;
    int count = 1;

    size_t param_count = env->getParamCount();

    if (param_count > 0)
        idx = env->getParam(0)->getInteger();

    if (param_count > 1)
        count = env->getParam(1)->getInteger();

    m_ctrl->insertRow(idx, count);
}

void TableBox::removeRow(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    int idx = 0;
    int count = 1;

    size_t param_count = env->getParamCount();

    if (param_count > 0)
        idx = env->getParam(0)->getInteger();

    if (param_count > 1)
        count = env->getParam(1)->getInteger();

    m_ctrl->removeRow(idx, count);
}

void TableBox::insertColumn(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    int idx = 0;
    int count = 1;

    size_t param_count = env->getParamCount();

    if (param_count > 0)
        idx = env->getParam(0)->getInteger();

    if (param_count > 1)
        count = env->getParam(1)->getInteger();

    m_ctrl->insertColumn(idx, count);
}

void TableBox::removeColumn(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    int idx = 0;
    int count = 1;

    size_t param_count = env->getParamCount();

    if (param_count > 0)
        idx = env->getParam(0)->getInteger();

    if (param_count > 1)
        count = env->getParam(1)->getInteger();

    m_ctrl->removeColumn(idx, count);
}

void TableBox::setRowSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    int idx = 0;
    int size = 0;

    size_t param_count = env->getParamCount();

    if (param_count < 2)
        return;

    idx = env->getParam(0)->getInteger();
    size = env->getParam(1)->getInteger();

    m_ctrl->setRowSize(idx, size);
}

void TableBox::getRowSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    int idx = 0;
    size_t param_count = env->getParamCount();

    if (param_count < 1)
        return;

    idx = env->getParam(0)->getInteger();
    retval->setInteger(m_ctrl->getRowSize(idx));
}

void TableBox::setColumnSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    int idx = 0;
    int size = 0;

    size_t param_count = env->getParamCount();

    if (param_count < 2)
        return;

    idx = env->getParam(0)->getInteger();
    size = env->getParam(1)->getInteger();

    m_ctrl->setColumnSize(idx, size);
}

void TableBox::getColumnSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    int idx = 0;
    size_t param_count = env->getParamCount();

    if (param_count < 1)
        return;

    idx = env->getParam(0)->getInteger();
    retval->setInteger(m_ctrl->getColumnSize(idx));
}

void TableBox::setCellValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    size_t param_count = env->getParamCount();

    if (param_count < 3 || !isControlValid())
        return;

    int row, col;
    row = env->getParam(0)->getInteger();
    col = env->getParam(1)->getInteger();
    wxString value = env->getParam(2)->getString();

    m_ctrl->setCellValue(row, col, value);
}

void TableBox::getCellValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    size_t param_count = env->getParamCount();

    if (param_count < 2 || !isControlValid())
        return;

    int row, col;
    row = env->getParam(0)->getInteger();
    col = env->getParam(1)->getInteger();

    wxString value;
    m_ctrl->getCellValue(row, col, value);
    retval->setString(value.wc_str());
}

void TableBox::getCursorCell(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: should create cell range binding and use it

    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setNull();

    int row, col;
    m_ctrl->getCursorPos(&row, &col);
    
    Point* p = Point::createObject(env);
    p->getMember(L"row")->setInteger(row);
    p->getMember(L"column")->setInteger(col);
    retval->setObject(p);
}

void TableBox::setCursorCell(kscript::ExprEnv* env, kscript::Value* retval)
{
    size_t param_count = env->getParamCount();

    if (param_count < 2 || !isControlValid())
        return;

    int row, col;
    row = env->getParam(0)->getInteger();
    col = env->getParam(1)->getInteger();
    m_ctrl->setCursorPos(row, col);
}

void TableBox::setProperty(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    // reset the callback object and callback function
    m_callback_object.setNull();
    m_callback_function.setNull();

    // if we have a single parameter and it's a function
    // pointer, then set the callback function
    if (env->getParamCount() > 0)
    {
        kscript::Value* param0 = env->getParam(0);
        if (param0->isFunction())
        {
            param0->eval(NULL, &m_callback_function);
        }
    }

    // if we have two parameters and the first is an object
    // and the second is a function, then set the callback
    // object and callback function
    if (env->getParamCount() > 1)
    {
        kscript::Value* param0 = env->getParam(0);
        kscript::Value* param1 = env->getParam(1);

        if (param0->isObject() && param1->isFunction())
        {
            param0->eval(NULL, &m_callback_object);
            param1->eval(NULL, &m_callback_function);
        }
    }
}

void TableBox::refresh(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->refresh();
}

void TableBox::onEvent(kcanvas::IEventPtr evt)
{
    // get the event name
    wxString name = evt->getName();

    // set the row and column members
    int row, col;
    m_ctrl->getCursorPos(&row, &col);

    kscript::Value* event_args = new kscript::Value;
    event_args->setObject();
    event_args->getMember(L"row")->setInteger(row);
    event_args->getMember(L"col")->setInteger(col);

    // invoke the js event
    if (name == kcanvas::EVENT_TABLE_PRE_CELL_EDIT)
        invokeJsEvent(L"cellEditing", event_args);

    if (name == kcanvas::EVENT_TABLE_CELL_EDIT)
        invokeJsEvent(L"cellEdited", event_args);
}

void TableBox::onRendererUpdated(kcanvas::Properties& props)
{
    // NOTE: this code was commented out because getTopEnv()
    // didn't work anymore / has been deprecated/removed
    
/*
    if (m_parser)
    {
        kscript::ExprEnv* env = m_parser->getTopEnv();
        if (env)
        {
            kscript::Value retval;
            kscript::Value* vparams[2];

            vparams[0] = new kscript::Value;
            vparams[1] = new kscript::Value;

            // set the callback function this argument
            vparams[0]->setObject(m_callback_object.getObject());

            // set the callback function parameter arguments
            vparams[1]->setObject();

            kcanvas::PropertyValue row;
            props.get(kcanvas::PROP_CELL_ROW, row);
            vparams[1]->getMember(L"row")->setInteger(row.getInteger());

            kcanvas::PropertyValue column;
            props.get(kcanvas::PROP_CELL_COLUMN, column);
            vparams[1]->getMember(L"column")->setInteger(column.getInteger());

            m_parser->invoke(env,
                             &m_callback_function,
                             &m_callback_object,
                             &retval,
                             vparams,
                             2);

            delete vparams[0];
            delete vparams[1];

            // get the object members and values
            if (retval.isObject())
            {
                kscript::ValueObject* obj = (kscript::ValueObject*)retval.getObject();

                size_t i, count = obj->getRawMemberCount();
                for (i = 0; i < count; ++i)
                {
                    wxString name = obj->getRawMemberName(i);
                    kscript::Value* value = obj->getRawMemberByIdx(i);

                    if (value->isString())
                        props.add(name, value->getString());

                    if (value->isInteger())
                        props.add(name, value->getInteger());

                    if (value->isBoolean())
                        props.add(name, value->getBoolean());

                    if (value->isObject())
                    {
                        kscript::ValueObject* obj = value->getObject();
                        if (obj->getClassName() == L"Color")
                        {
                            Color* c = (Color*)obj;
                            
                            wxColour color(c->getMember(L"red")->getInteger(),
                                           c->getMember(L"green")->getInteger(),
                                           c->getMember(L"blue")->getInteger());

                            props.add(name, color);
                        }
                    }
                }
            }
        }
    }
    */
}

// table control class implementation

TableCtrl::TableCtrl(wxWindow* parent,
                     wxWindowID id,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style) : CanvasControl(parent, id, pos, size, style)
{
    // create the main content table
    kcanvas::IComponentPtr comp_table = kcanvas::CompTable::create();
    m_table = static_cast<kcanvas::CompTable*>(comp_table.p);

    kcanvas::ICompTablePtr content_table = comp_table;
    content_table->setRowCount(50);
    content_table->setColumnCount(20);

    // turn off the column actions; we'll handle these
    // through the table headers
    content_table->setPreference(kcanvas::CompTable::prefResizeColumns, false);
    content_table->setPreference(kcanvas::CompTable::prefResizeRows, false);
    content_table->setPreference(kcanvas::CompTable::prefSelectColumns, false);
    content_table->setPreference(kcanvas::CompTable::prefSelectRows, false);

    // create the column header and sync it with the content
    kcanvas::ICompTablePtr comp_column_header;
    comp_column_header = kcanvas::CompTableHeader::create(kcanvas::CompTableHeader::Column);
    m_column_header = static_cast<kcanvas::CompTableHeader*>(comp_column_header.p);
    m_column_header->setRowCount(1);
    syncColumnSizes(m_table, m_column_header, true);

    // create the row header and sync it with the content
    kcanvas::ICompTablePtr comp_row_header;
    comp_row_header = kcanvas::CompTableHeader::create(kcanvas::CompTableHeader::Row);
    m_row_header = static_cast<kcanvas::CompTableHeader*>(comp_row_header.p);
    m_row_header->setColumnCount(1);
    syncRowSizes(m_table, m_row_header, true);

    // add the content table, row header, and column header
    add(m_table);
    add(m_row_header);
    add(m_column_header);

    // create an event handler
    kcanvas::EventHandler* handler = new kcanvas::EventHandler;
    handler->sigEvent().connect(this, &TableCtrl::onCanvasEvent);
    addEventHandler(kcanvas::EVENT_FOCUS_SET, handler);
    addEventHandler(kcanvas::EVENT_PRE_LAYOUT, handler);
    addEventHandler(kcanvas::EVENT_PRE_RENDER, handler);
    addEventHandler(kcanvas::EVENT_RENDER, handler);
    addEventHandler(kcanvas::EVENT_PRE_SELECT, handler);
    addEventHandler(kcanvas::EVENT_CANVAS_SCROLL, handler);
    addEventHandler(kcanvas::EVENT_CANVAS_CURSOR, handler);
    addEventHandler(kcanvas::EVENT_CANVAS_ZOOM, handler);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN, handler);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZE_COLUMN, handler);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZING_COLUMN, handler);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW, handler);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZE_ROW, handler);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZING_ROW, handler);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW_TO_CONTENT, handler);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZE_ROW_TO_CONTENT, handler);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN_TO_CONTENT, handler);
    addEventHandler(kcanvas::EVENT_TABLE_RESIZE_COLUMN_TO_CONTENT, handler);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_SELECT_CELL, handler);
    addEventHandler(kcanvas::EVENT_TABLE_PRE_SELECT_CELL, handler);
    addEventHandler(kcanvas::EVENT_TABLE_SELECT_CELL, handler);
    addEventHandler(kcanvas::EVENT_TABLE_SELECTING_CELL, handler);
    addEventHandler(kcanvas::EVENT_TABLE_SELECTION_CLEAR, handler);
}

TableCtrl::~TableCtrl()
{
}

void TableCtrl::setRowCount(int count)
{
    m_row_header->setRowCount(count);
    m_table->setRowCount(count);
}

int TableCtrl::getRowCount() const
{
    return m_table->getRowCount();
}

void TableCtrl::setColumnCount(int count)
{
    m_column_header->setColumnCount(count);
    m_table->setColumnCount(count);
}

int TableCtrl::getColumnCount() const
{
    return m_table->getColumnCount();
}

void TableCtrl::insertRow(int idx, int count)
{
    m_row_header->insertRow(idx, count);
    m_table->insertRow(idx, count);
}

void TableCtrl::removeRow(int idx, int count)
{
    m_row_header->removeRow(idx, count);
    m_table->removeRow(idx, count);
}

void TableCtrl::insertColumn(int idx, int count)
{
    m_column_header->insertColumn(idx, count);
    m_table->insertColumn(idx, count);
}

void TableCtrl::removeColumn(int idx, int count)
{
    m_column_header->removeColumn(idx, count);
    m_table->removeColumn(idx, count);
}

void TableCtrl::setRowSize(int row, int size)
{
    m_row_header->setRowSize(row, size);
    m_table->setRowSize(row, size);
}

int TableCtrl::getRowSize(int row)
{
    return m_table->getRowSize(row);
}

void TableCtrl::setColumnSize(int col, int size)
{
    m_column_header->setColumnSize(col, size);
    m_table->setColumnSize(col, size);
}

int TableCtrl::getColumnSize(int col)
{
    return m_table->getColumnSize(col);
}

void TableCtrl::setCellValue(int row, int col, const wxString& value)
{
    m_table->setCellProperty(kcanvas::CellRange(row, col),
                             kcanvas::PROP_CONTENT_VALUE,
                             value);
}

void TableCtrl::getCellValue(int row, int col, wxString& value) const
{
    kcanvas::PropertyValue prop;
    m_table->getCellProperty(kcanvas::CellRange(row, col),
                             kcanvas::PROP_CONTENT_VALUE,
                             prop);
    value = prop.getString();
}

void TableCtrl::setCursorPos(int row, int col)
{
    m_table->setCursorPos(row, col);
}

void TableCtrl::getCursorPos(int* row, int* col)
{
    m_table->getCursorPos(row, col);
}

void TableCtrl::refresh()
{
    // layout and render
    layout();
    render();
}

kcanvas::ICompTablePtr TableCtrl::getTablePtr()
{
    return m_table;
}

void TableCtrl::onCanvasEvent(kcanvas::IEventPtr evt)
{
    wxString name = evt->getName();

    if (name == kcanvas::EVENT_FOCUS_SET)
    {
        // if we're switching to a row or column header,
        // save the table selections
        kcanvas::ICompTablePtr table = evt->getTarget();
        
        // if we're on a column or row header, save the 
        // existing table selections so we can restore 
        // them if need be when we sync up the table
        // selections with newly added column or row 
        // selections 
        if (table == m_column_header ||
            table == m_row_header)
        {
            m_table->getCellSelections(m_saved_selections, false);
        }
    }

    if (name == kcanvas::EVENT_PRE_LAYOUT)
    {
        int x_offset = dtom_x(SIZE_TABLECTRL_LEFT);
        int y_offset = dtom_y(SIZE_TABLECTRL_TOP);
        
        // get the view origin
        int x_view, y_view;
        getViewOrigin(&x_view, &y_view);

        // make sure the view doesn't go outside the model
        if (x_view + getViewWidth() > getWidth())
            x_view = getWidth() - getViewWidth();

        if (y_view + getViewHeight() > getHeight())
            y_view = getHeight() - getViewHeight();

        if (x_view < 0)
            x_view = 0;

        if (y_view < 0)
            y_view = 0;

        // set the column header origin and size
        wxRect rect_column;
        m_column_header->setRowSize(0, y_offset);
        m_column_header->extends(rect_column);
        m_column_header->setOrigin(x_offset, y_view);
        m_column_header->setSize(rect_column.width, rect_column.height);
        
        // set the row header origin and size
        wxRect rect_row;
        m_row_header->setColumnSize(0, x_offset);
        m_row_header->extends(rect_row);
        m_row_header->setOrigin(x_view, y_offset);
        m_row_header->setSize(rect_row.width, rect_row.height);

        // set the content origin and size
        wxRect rect_content;
        m_table->extends(rect_content);
        m_table->setOrigin(x_offset, y_offset);
        m_table->setSize(rect_content.width, rect_content.height);

        // resize the canvas
        setSize(x_offset + rect_content.width, 
                y_offset + rect_content.height);
    }

    if (name == kcanvas::EVENT_PRE_RENDER)
    {
        // before drawing tables, draw header background    
    
        // get the view origin
        int x_view, y_view;
        getViewOrigin(&x_view, &y_view);
    
        // get the update rectangle
        kcanvas::PropertyValue x, y, width, height;
        kcanvas::INotifyEventPtr notify_evt = evt;

        notify_evt->getProperty(kcanvas::EVENT_PROP_X, x);
        notify_evt->getProperty(kcanvas::EVENT_PROP_Y, y);
        notify_evt->getProperty(kcanvas::EVENT_PROP_W, width);
        notify_evt->getProperty(kcanvas::EVENT_PROP_H, height);

        wxRect update_rect(x.getInteger(),
                           y.getInteger(),
                           width.getInteger(),
                           height.getInteger());

        // convert the update rectangle from model coordinates
        // to view coordinates
        update_rect.Offset(-x_view, -y_view);

        int x_offset = dtom_x(SIZE_TABLECTRL_LEFT);
        int y_offset = dtom_y(SIZE_TABLECTRL_TOP);
        
        // get the old draw origin
        int old_draw_origin_x, old_draw_origin_y;
        getDrawOrigin(&old_draw_origin_x, &old_draw_origin_y);

        // set the new origin to the view coordinates
        setDrawOrigin(x_view, y_view);

        // calculate the view rectangle in view coordinates
        wxRect rect_column(0, 0, getViewWidth(), y_offset);
        wxRect rect_row(0, 0, x_offset, getViewHeight());
        
        rect_column.Intersect(update_rect);
        rect_row.Intersect(update_rect);

        // draw the header background if it intersects the
        // update area 
        kcanvas::Color base_color = kcanvas::tokccolor(kcl::getBaseColor());
        setPen(base_color);
        setBrush(base_color);

        kcanvas::Color start_color = base_color;
        kcanvas::Color end_color = kcanvas::stepColor(kcanvas::COLOR_WHITE, start_color, 70);
        
        if (!rect_column.IsEmpty())
            drawGradientFill(rect_column, start_color, end_color, wxNORTH);
            
        if (!rect_row.IsEmpty())    
            drawGradientFill(rect_row, start_color, end_color, wxWEST);

        // restore the draw origin
        setDrawOrigin(old_draw_origin_x, old_draw_origin_y);
    }

    if (name == kcanvas::EVENT_RENDER)
    {
        // after drawing the tables, draw the upper-left
        kcanvas::ICanvasPtr canvas = evt->getTarget();
        if (!canvas.isNull())
        {
            int x_offset = dtom_x(SIZE_TABLECTRL_LEFT);
            int y_offset = dtom_y(SIZE_TABLECTRL_TOP);

            // get the view origin
            int x_view, y_view;
            getViewOrigin(&x_view, &y_view);
            
            // get the old draw origin
            int old_draw_origin_x, old_draw_origin_y;
            getDrawOrigin(&old_draw_origin_x, &old_draw_origin_y);

            // set the new origin to the view coordinates
            setDrawOrigin(x_view, y_view);

            // calculate the view rectangle in view coordinates
            wxRect rect(0, 0, x_offset, y_offset);

            // draw the upper-left corner
            kcanvas::Color base_color = kcanvas::tokccolor(kcl::getBaseColor());
            setPen(base_color);
            setBrush(base_color);

            kcanvas::Color start_color = base_color;
            kcanvas::Color end_color = kcanvas::stepColor(kcanvas::COLOR_WHITE, start_color, 70);
            drawGradientFill(rect, start_color, end_color, wxWEST);
            
            // draw grey lines on edge of row and column headers
            kcanvas::Color line_color = kcanvas::COLOR_LIGHT_GREY;
            setPen(line_color);
            drawLine(0, y_offset, getViewWidth(), y_offset);
            drawLine(x_offset, 0, x_offset, getViewHeight());

            // restore the draw origin
            setDrawOrigin(old_draw_origin_x, old_draw_origin_y);
        }
    }

    if (name == kcanvas::EVENT_PRE_SELECT)
    {
        kcanvas::INotifyEventPtr notify_evt;
        notify_evt = evt;

        // don't allow any canvas selections
        notify_evt->veto();
    }
    
    if (name == kcanvas::EVENT_CANVAS_SCROLL)
    {
        refresh();
    }
    
    if (name == kcanvas::EVENT_CANVAS_CURSOR)
    {
        kcanvas::PropertyValue prop;
        kcanvas::INotifyEventPtr notify_evt = evt;
        notify_evt->getProperty(kcanvas::EVENT_PROP_CURSOR, prop);
        wxString cursor = prop.getString();
        
        if (cursor == kcanvas::CURSOR_INVALID)
            SetCursor(wxNullCursor);
            
        if (cursor == kcanvas::CURSOR_STANDARD)
            SetCursor(*wxSTANDARD_CURSOR);
    
        if (cursor == kcanvas::CURSOR_SIZENS )
            SetCursor(wxCursor(wxCURSOR_SIZENS));
            
        if (cursor == kcanvas::CURSOR_SIZEWE)
            SetCursor(wxCursor(wxCURSOR_SIZEWE));
            
        if (cursor == kcanvas::CURSOR_SIZENWSE)
            SetCursor(wxCursor(wxCURSOR_SIZENWSE));

        if (cursor == kcanvas::CURSOR_SIZENESW)
            SetCursor(wxCursor(wxCURSOR_SIZENESW));

        if (cursor == kcanvas::CURSOR_MOVE)
            SetCursor(wxCursor(wxCURSOR_SIZING));

        if (cursor == kcanvas::CURSOR_TEXTEDIT)
            SetCursor(wxCursor(wxCURSOR_IBEAM));

        if (cursor == kcanvas::CURSOR_CROSS)
            SetCursor(*wxSTANDARD_CURSOR);
    }
    
    if (name == kcanvas::EVENT_CANVAS_ZOOM)
    {
        refresh();
    }
    
    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN)
    {
        // before we do anything, tag the tables so
        // that we can undo the temporary size properties
        // that accumulate during the real-time resizing
        m_table->tag(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN);
    }
    
    if (name == kcanvas::EVENT_TABLE_RESIZE_COLUMN)
    {
        // restore the table to what they were before
        // we started the real-time resizing, then
        // add on the final size properties
        m_table->restore(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN);
        syncColumnSizes(m_column_header, m_table, true);

        // refresh the control
        refresh();
    }
    
    if (name == kcanvas::EVENT_TABLE_RESIZING_COLUMN)
    {
        // sync the content table with the column header
        m_table->restore(kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN);
        syncColumnSizes(m_column_header, m_table, false); // no need to clear previous sizes
        
        // refresh the control
        refresh();
    }
    
    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_ROW)
    {
        // before we do anything, tag the tables so
        // that we can undo the temporary size properties
        // that accumulate during the real-time resizing
        m_table->tag(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW);
    }
    
    if (name == kcanvas::EVENT_TABLE_RESIZE_ROW)
    {
        // restore the table to what they were before
        // we started the real-time resizing, then
        // add on the final size properties
        m_table->restore(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW);
        syncRowSizes(m_row_header, m_table, true);
        
        // refresh the control
        refresh();
    }
    
    if (name == kcanvas::EVENT_TABLE_RESIZING_ROW)
    {
        // sync the content table with the column header
        m_table->restore(kcanvas::EVENT_TABLE_PRE_RESIZE_ROW);
        syncRowSizes(m_row_header, m_table, false); // no need to clear previous sizes

        // refresh the control
        refresh();
    }

    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_ROW_TO_CONTENT)
    {
        // this event is vetoed by the row header, so we can 
        // sync up the header with the content of the content 
        // table

        // get the content table row selections
        std::vector<kcanvas::CellRange> row_selections;
        
        std::vector<kcanvas::CellRange> cell_selections;
        m_table->getCellSelections(cell_selections, false);
        
        std::vector<kcanvas::CellRange>::iterator it, it_end;
        it_end = cell_selections.end();

        for (it = cell_selections.begin(); it != it_end; ++it)
        {
            // save the row selections
            if (it->col1() == -1 && it->col2() == -1)
                row_selections.push_back(*it);
        }

        // get the row being resized
        kcanvas::PropertyValue event_prop;
        kcanvas::INotifyEventPtr notify_evt = evt;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, event_prop);
        int row = event_prop.getInteger();

        // if the current row isn't selected, resize only this row;
        // otherwise resize this selected row along with the other 
        // selected rows
        bool resize = false;
        if (!m_table->isRowSelected(row))
        {
            kcanvas::CellRange row_range(row, -1, row, -1);
            resize = m_table->resizeRowsToContent(row_range);
        }
        else
        {
            std::vector<kcanvas::CellRange>::iterator it, it_end;
            it_end = row_selections.end();
            
            for (it = row_selections.begin(); it != it_end; ++it)
            {
                if (m_table->resizeRowsToContent(*it))
                    resize = true;
            }
        }

        // if rows were resized, sync the row sizes from 
        // the content back to the row header and refresh
        if (resize)
        {
            syncRowSizes(m_table, m_row_header, true);
            refresh();
        }    
    }
    
    if (name == kcanvas::EVENT_TABLE_PRE_RESIZE_COLUMN_TO_CONTENT)
    {
        // this event is vetoed by the column header, so
        // we can sync up the header with the content of
        // the content table

        // get the content table column selections
        std::vector<kcanvas::CellRange> col_selections;
        
        std::vector<kcanvas::CellRange> cell_selections;
        m_table->getCellSelections(cell_selections, false);
        
        std::vector<kcanvas::CellRange>::iterator it, it_end;
        it_end = cell_selections.end();

        for (it = cell_selections.begin(); it != it_end; ++it)
        {
            // save the column selections
            if (it->row1() == -1 && it->row2() == -1)
                col_selections.push_back(*it);
        }

        // get the column being resized
        kcanvas::PropertyValue event_prop;
        kcanvas::INotifyEventPtr notify_evt = evt;
        notify_evt->getProperty(kcanvas::EVENT_PROP_INDEX, event_prop);
        int col = event_prop.getInteger();

        // if the current column isn't selected, resize only this column;
        // otherwise resize this selected column along with the other 
        // selected columns
        bool resize = false;
        if (!m_table->isColumnSelected(col))
        {
            kcanvas::CellRange column_range(-1, col, -1, col);
            resize = m_table->resizeColumnsToContent(column_range);
        }
        else
        {
            std::vector<kcanvas::CellRange>::iterator it, it_end;
            it_end = col_selections.end();
            
            for (it = col_selections.begin(); it != it_end; ++it)
            {
                if (m_table->resizeColumnsToContent(*it))
                    resize = true;
            }
        }

        // if columns were resized, sync the column sizes from 
        // the content back to the column header and refresh
        if (resize)
        {
            syncColumnSizes(m_table, m_column_header, true);
            refresh();
        }
    }
    
    if (name == kcanvas::EVENT_TABLE_PRE_SELECT_CELL)
    {
    }
    
    if (name == kcanvas::EVENT_TABLE_SELECT_CELL)
    {
    }
    
    if (name == kcanvas::EVENT_TABLE_SELECTING_CELL)
    {
        kcanvas::ICompTablePtr table = evt->getTarget();
    
        // if we're selecting in the colum header, copy the 
        // column selections and move the cursor to the first 
        // row in the starting column
        if (table == m_column_header)
        {
            syncSelections(m_column_header, m_table, m_saved_selections);
            
            int cursor_row, cursor_col;
            m_column_header->getCursorPos(&cursor_row, &cursor_col);
            m_table->setCursorPos(0, cursor_col);
        }

        // if we're selecting in the row header, copy the 
        // row selections and move the cursor to the first 
        // column in the starting row
        if (table == m_row_header)
        {
            syncSelections(m_row_header, m_table, m_saved_selections);

            int cursor_row, cursor_col;
            m_row_header->getCursorPos(&cursor_row, &cursor_col);
            m_table->setCursorPos(cursor_row, 0);
        }

        // no need to layout; simply re-render; TODO: could
        // be more efficient by invalidating the newly selected
        // area, rather than re-rendering everything
        render();
    }
    
    if (name == kcanvas::EVENT_TABLE_SELECTION_CLEAR)
    {
        kcanvas::ICompTablePtr table = evt->getTarget();

        // clear any saved selections
        m_saved_selections.clear();

        // if we're on the column header, clear the row 
        // header and table
        if (table == m_column_header)
        {
            m_row_header->removeAllCellSelections();
            m_table->removeAllCellSelections();
        }

        // if we're on the row header, clear the column 
        // header and table
        if (table == m_row_header)
        {
            m_column_header->removeAllCellSelections();
            m_table->removeAllCellSelections();
        }

        // if we're on the table, clear the column and 
        // row headers
        if (table == m_table)
        {
            m_column_header->removeAllCellSelections();
            m_row_header->removeAllCellSelections();
        }
    }
}

void TableCtrl::syncColumnSizes(kcanvas::CompTable* source, 
                                kcanvas::CompTable* destination,
                                bool overwrite)
{
    // sync the column count
    destination->setColumnCount(source->getColumnCount());

    // sync the column sizes
    std::vector<kcanvas::CellProperties> column_sizes;
    source->getCellProperty(kcanvas::CellRange(-1,-1), kcanvas::PROP_SIZE_W, column_sizes);
    destination->addCellProperties(column_sizes, overwrite);
}

void TableCtrl::syncRowSizes(kcanvas::CompTable* source,
                             kcanvas::CompTable* destination,
                             bool overwrite)
{
    // sync the row count
    destination->setRowCount(source->getRowCount());

    // sync the row sizes
    std::vector<kcanvas::CellProperties> row_sizes;
    source->getCellProperty(kcanvas::CellRange(-1,-1), kcanvas::PROP_SIZE_H, row_sizes);
    destination->addCellProperties(row_sizes, overwrite);
}

void TableCtrl::syncSelections(kcanvas::CompTable* source,
                               kcanvas::CompTable* destination,
                               const std::vector<kcanvas::CellRange>& saved_selections)
{
    // clear the selections in the destination table
    destination->removeAllCellSelections();

    // used throughout
    std::vector<kcanvas::CellRange>::const_iterator it, it_end;

    // copy the saved table selections
    it_end = saved_selections.end();
    for (it = saved_selections.begin(); it != it_end; ++it)
    {
        destination->selectCells(*it);
    }

    // get the table selections; used throughout
    std::vector<kcanvas::CellRange> selections;
    source->getCellSelections(selections, false);

    // add the selections
    it_end = selections.end();
    for (it = selections.begin(); it != it_end; ++it)
    {
        destination->selectCells(*it);
    }
}

