/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2008-06-06
 *
 */


#ifndef __APP_SCRIPTTABLE_H
#define __APP_SCRIPTTABLE_H


#include "../kcanvas/kcanvas.h"
#include "../kcanvas/canvas.h"
#include "../kcanvas/canvasdc.h"
#include "../kcanvas/canvascontrol.h"
#include "../kcanvas/component.h"
#include "../kcanvas/componenttable.h"


const int SIZE_TABLECTRL_TOP = 18;
const int SIZE_TABLECTRL_LEFT = 11;


class TableCtrl;

class TableBox : public FormControl,
                 public xcm::signal_sink
{
    BEGIN_KSCRIPT_DERIVED_CLASS("TableBox", TableBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", TableBox::constructor)
        KSCRIPT_GUI_METHOD("setRowCount", TableBox::setRowCount)
        KSCRIPT_GUI_METHOD("getRowCount", TableBox::getRowCount)
        KSCRIPT_GUI_METHOD("setColumnCount", TableBox::setColumnCount)
        KSCRIPT_GUI_METHOD("getColumnCount", TableBox::getColumnCount)
        KSCRIPT_GUI_METHOD("insertRow", TableBox::insertRow)
        KSCRIPT_GUI_METHOD("removeRow", TableBox::removeRow)
        KSCRIPT_GUI_METHOD("insertColumn", TableBox::insertColumn)
        KSCRIPT_GUI_METHOD("removeColumn", TableBox::removeColumn)
        KSCRIPT_GUI_METHOD("setRowSize", TableBox::setRowSize)
        KSCRIPT_GUI_METHOD("getRowSize", TableBox::getRowSize)
        KSCRIPT_GUI_METHOD("setColumnSize", TableBox::setColumnSize)
        KSCRIPT_GUI_METHOD("getColumnSize", TableBox::getColumnSize)
        KSCRIPT_GUI_METHOD("setCellValue", TableBox::setCellValue)
        KSCRIPT_GUI_METHOD("getCellValue" , TableBox::getCellValue)
        KSCRIPT_GUI_METHOD("setCursorCell" , TableBox::setCursorCell)
        KSCRIPT_GUI_METHOD("getCursorCell" , TableBox::getCursorCell)
        KSCRIPT_GUI_METHOD("setProperty", TableBox::setProperty) // experimental
        KSCRIPT_GUI_METHOD("refresh", TableBox::refresh)
    END_KSCRIPT_CLASS()

public:

    TableBox();
    ~TableBox();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setRowCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getRowCount(kscript::ExprEnv* env, kscript::Value* retval);
    void setColumnCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getColumnCount(kscript::ExprEnv* env, kscript::Value* retval);
    void insertRow(kscript::ExprEnv* env, kscript::Value* retval);
    void removeRow(kscript::ExprEnv* env, kscript::Value* retval);
    void insertColumn(kscript::ExprEnv* env, kscript::Value* retval);
    void removeColumn(kscript::ExprEnv* env, kscript::Value* retval);
    void setRowSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getRowSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setColumnSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getColumnSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setCellValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getCellValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getCursorCell(kscript::ExprEnv* env, kscript::Value* retval);
    void setCursorCell(kscript::ExprEnv* env, kscript::Value* retval);
    void setProperty(kscript::ExprEnv* env, kscript::Value* retval);
    void refresh(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(kcanvas::IEventPtr evt);
    void onRendererUpdated(kcanvas::Properties& props);

private:

    // table control
    TableCtrl* m_ctrl;
    
    // info for function callback for properties
    kscript::ExprParser* m_parser;
    kscript::Value m_callback_object;
    kscript::Value m_callback_function;
};


// table control class definition

class TableCtrl : public kcanvas::CanvasControl
{
public:

    TableCtrl(wxWindow* parent,
              wxWindowID id = -1,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = (0 | wxHSCROLL | wxVSCROLL | wxCLIP_CHILDREN | wxWANTS_CHARS));
    virtual ~TableCtrl();

    void setRowCount(int count);
    int getRowCount() const;

    void setColumnCount(int count);
    int getColumnCount() const;

    void insertRow(int idx, int count = 1);
    void removeRow(int idx, int count = 1);

    void insertColumn(int idx, int count = 1);
    void removeColumn(int idx, int count = 1);

    void setRowSize(int row, int size);
    int getRowSize(int row);

    void setColumnSize(int col, int size);
    int getColumnSize(int col);

    void setCellValue(int row, int col, const wxString& value);
    void getCellValue(int row, int col, wxString& value) const;

    void setCursorPos(int row, int col);
    void getCursorPos(int* row, int* col);

    void refresh();

public:

    // experimental; for renderer property callback
    kcanvas::ICompTablePtr getTablePtr();

private:

    void onCanvasEvent(kcanvas::IEventPtr evt);

private:

    void syncColumnSizes(kcanvas::CompTable* source,
                         kcanvas::CompTable* destination,
                         bool overwrite = false);
    void syncRowSizes(kcanvas::CompTable* source,
                      kcanvas::CompTable* destination,
                      bool overwrite = false);
    void syncSelections(kcanvas::CompTable* source,
                        kcanvas::CompTable* destination,
                        const std::vector<kcanvas::CellRange>& saved_selections);
    
private:

    // components
    kcanvas::CompTable* m_table;
    kcanvas::CompTableHeader* m_column_header;
    kcanvas::CompTableHeader* m_row_header;
    
    // saved selections
    std::vector<kcanvas::CellRange> m_saved_selections;
};


#endif  // __APP_SCRIPTTABLE_H

