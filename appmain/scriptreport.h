/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2011-03-28
 *
 */


#ifndef H_APP_SCRIPTREPORT_H
#define H_APP_SCRIPTREPORT_H


#include "../kcanvas/kcanvas.h"
#include "../kcanvas/canvas.h"


class CompReportDesign;


class Report : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Report", Report)
        KSCRIPT_METHOD("constructor", Report::constructor)
        KSCRIPT_METHOD("load", Report::load)
        KSCRIPT_METHOD("save", Report::save)
        KSCRIPT_METHOD("saveAsPdf", Report::saveAsPdf)
        KSCRIPT_METHOD("setDataSource", Report::setDataSource)
        KSCRIPT_METHOD("setPageSize", Report::setPageSize)
        KSCRIPT_METHOD("setPageMargins", Report::setPageMargins)
        KSCRIPT_METHOD("setRowCount", Report::setRowCount)
        KSCRIPT_METHOD("getRowCount", Report::getRowCount)
        KSCRIPT_METHOD("setColumnCount", Report::setColumnCount)
        KSCRIPT_METHOD("getColumnCount", Report::getColumnCount)
        KSCRIPT_METHOD("insertRow", Report::insertRow)
        KSCRIPT_METHOD("removeRow", Report::removeRow)
        KSCRIPT_METHOD("insertColumn", Report::insertColumn)
        KSCRIPT_METHOD("removeColumn", Report::removeColumn)
        KSCRIPT_METHOD("setRowSize", Report::setRowSize)
        KSCRIPT_METHOD("getRowSize", Report::getRowSize)
        KSCRIPT_METHOD("setColumnSize", Report::setColumnSize)
        KSCRIPT_METHOD("getColumnSize", Report::getColumnSize)
        KSCRIPT_METHOD("insertSection", Report::insertSection)
        KSCRIPT_METHOD("setSection", Report::setSection)
        KSCRIPT_METHOD("setCellProperty", Report::setCellProperty)
        KSCRIPT_METHOD("setCellValue", Report::setCellValue)
        KSCRIPT_METHOD("mergeCells", Report::mergeCells)
        KSCRIPT_METHOD("unmergeCells", Report::mergeCells)
        KSCRIPT_METHOD("getTextSize", Report::getTextSize)
    END_KSCRIPT_CLASS()

public:

    Report();
    ~Report();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    
    void load(kscript::ExprEnv* env, kscript::Value* retval);
    void save(kscript::ExprEnv* env, kscript::Value* retval);
    void saveAsPdf(kscript::ExprEnv* env, kscript::Value* retval);

    void setDataSource(kscript::ExprEnv* env, kscript::Value* retval);
    void setPageSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setPageMargins(kscript::ExprEnv* env, kscript::Value* retval);

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

    void insertSection(kscript::ExprEnv* env, kscript::Value* retval);
    void setSection(kscript::ExprEnv* env, kscript::Value* retval);

    void setCellProperty(kscript::ExprEnv* env, kscript::Value* retval);
    void setCellValue(kscript::ExprEnv* env, kscript::Value* retval);

    void mergeCells(kscript::ExprEnv* env, kscript::Value* retval);
    void unmergeCells(kscript::ExprEnv* env, kscript::Value* retval);

    void getTextSize(kscript::ExprEnv* env, kscript::Value* retval);

private:

    CompReportDesign* getDesignComponentRaw();

private:

    // report design components
    kcanvas::IComponentPtr m_design_component;

    // canvas control
    kcanvas::ICanvasPtr m_canvas;
};


#endif  // __APP_SCRIPTREPORT_H

