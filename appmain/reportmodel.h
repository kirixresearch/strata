/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2007-04-10
 *
 */


#ifndef __APP_REPORTMODEL_H
#define __APP_REPORTMODEL_H


#include "../../kscript/kscript.h"
#include "../../kcanvas/kcanvas.h"
#include "../../kcanvas/property.h"
#include "../../kcanvas/event.h"


// report section names
const wxString PROP_REPORT_HEADER           = wxT("report.header");
const wxString PROP_REPORT_FOOTER           = wxT("report.footer");
const wxString PROP_REPORT_PAGE_HEADER      = wxT("report.page.header");
const wxString PROP_REPORT_PAGE_FOOTER      = wxT("report.page.footer");
const wxString PROP_REPORT_GROUP_HEADER     = wxT("report.group.header");
const wxString PROP_REPORT_GROUP_FOOTER     = wxT("report.group.footer");
const wxString PROP_REPORT_DETAIL           = wxT("report.detail");

// properties that store the report information; these 
// properties are passed to the data model so their values 
// can be displayed on reports with script functions
const wxString PROP_REPORT_PAGE_NUMBER          = wxT("report.page.number");
const wxString PROP_REPORT_PAGE_COUNT           = wxT("report.page.count");
const wxString PROP_REPORT_CURRENT_DATE         = wxT("report.current.date");
const wxString PROP_REPORT_DATA_SOURCE          = wxT("report.data.source");

// events
const wxString EVENT_MODEL_LOADED = wxT("event.model.loaded");


// model column type enumerations
enum ModelColumnTypes
{
    typeInvalid = 0,
    typeCharacter = 1,
    typeDouble = 2,
    typeInteger = 3,
    typeDate = 4,
    typeDateTime = 5,
    typeBoolean = 6
};


// model column group statistic enumerations
enum ModelColumnFunctions
{
    funcInvalid = 0,
    funcNone = 1,
    funcFirst = 2,
    funcLast = 3,
    funcMin = 4,
    funcMax = 5,
    funcSum = 6,
    funcAvg = 7,
    funcCount = 8
};


// forward declarations
xcm_interface IModel;
XCM_DECLARE_SMARTPTR(IModel)

class TangoModel;


// interface definitions
xcm_interface IModel : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IModel")

public:

    virtual void setQuery(const std::vector<wxString>& queries, const wxString& order) = 0;
    virtual void setQuery(const wxString& query) = 0;
    virtual bool isLoaded() = 0;

    virtual void addProperty(const wxString& prop_name, const kcanvas::PropertyValue& value) = 0;
    virtual void addProperties(const kcanvas::Properties& properties) = 0;
    virtual void removeProperty(const wxString& prop_name) = 0;

    virtual bool setProperty(const wxString& prop_name, const kcanvas::PropertyValue& value) = 0;
    virtual bool setProperties(const kcanvas::Properties& properties) = 0;
    virtual bool getProperty(const wxString& prop_name, kcanvas::PropertyValue& value) const = 0;
    virtual bool getProperties(kcanvas::Properties& properties) const = 0;
    virtual kcanvas::Properties& getPropertiesRef() = 0;

    virtual bool addGroup(const wxString& label, const std::vector<wxString>& columns) = 0;
    virtual bool setGroup(const wxString& label) = 0;
    virtual void removeAllGroups() = 0;

    virtual void goFirst() = 0;
    virtual void skip(int delta_rows) = 0;
    virtual bool eof() = 0;

    virtual bool bog() = 0;
    virtual bool eog() = 0;

    virtual void execute(bool block = false) = 0;
    virtual bool eval(int row, const wxString& expr, wxString& result) = 0;
};


// class implementations
class CellExpression
{
public:

    CellExpression();
    CellExpression(const CellExpression& c);
    virtual ~CellExpression();

    void setParseHook(kscript::ExprParseHookFunc func, void* param);

    void setModel(TangoModel* model);
    TangoModel* getModel();

    void setExpression(const wxString& expr);
    wxString getExpression();
    
    bool getResult(wxString& result);

    void trackType(int type);
    void trackScale(int scale);

private:

    void clear();

private:

    // model
    TangoModel* m_tango_model;

    // cell expression
    wxString m_expr;

    // expression parser and return value
    kscript::ExprParser* m_expr_parser;
    kscript::Value* m_retval;

    // parse hook
    kscript::ExprParseHookFunc m_parsehook_func;
    void* m_parsehook_param;

    // note: the following parameters allow us to pass extra 
    // information about any database columns used in the 
    // expression to the final result; for example, it allows 
    // us track the maximum scale of a numeric column, so that 
    // we can auto-format the final result with the maximum scale, 
    // rather than having to force it to a particular scale;
    // likewise, it allows us to know whether a datetime result 
    // value comes from a date column or a datetime column
    
    // m_tango_type is equal to 1) typeDateTime if a datetime column
    // is used in the expression, 2) typeDate if a date column is
    // used in the expression and no datetime column is used in
    // the expression, and 3) typeInvalid if neither a datetime
    // column nor a date column is used in the expression
    
    // m_max_scale stores the maximum scale of all numeric columns
    // used in the expression; if no numeric columns are used in
    // the expression, the max scale is set to -1

    int m_tango_type;
    int m_max_scale;
};


class ModelColumn
{
friend class TangoModel;

public:

    ModelColumn();
    virtual ~ModelColumn();

    void setName(const wxString& new_val);
    const wxString& getName();

    void setType(int new_val);
    int getType();

    void setWidth(int new_val);
    int getWidth();

    void setScale(int new_val);
    int getScale();

private:

    wxString m_name;
    tango::objhandle_t m_handle;
    int m_tango_type;
    int m_type;
    int m_width;
    int m_scale;
};


class ModelGroupCache
{
public:

    int m_column_idx;
    int m_function;

    wxString m_string;
    double m_double;
    int m_integer;
    bool m_boolean;
};


class ModelGroup
{
friend class TangoModel;

public:

    ModelGroup();
    virtual ~ModelGroup();

    void addGroupColumn(const wxString& column);
    void removeGroupColumn(const wxString& column);
    void removeAllGroupColumns();

    std::vector<wxString> getColumns();
    wxString getColumnByIdx(int col_idx);
    int getColumnCount();

    void setStringCache(int col_idx, int function, const wxString& value);
    bool getStringCache(int col_idx, int function, wxString& value);

    void setDoubleCache(int col_idx, int function, double value);
    bool getDoubleCache(int col_idx, int function, double* value);

    void setIntegerCache(int col_idx, int function, int value);
    bool getIntegerCache(int col_idx, int function, int* value);

    void setBooleanCache(int col_idx, int function, bool value);
    bool getBooleanCache(int col_idx, int function, bool* value);

    void clearCache();

private:

    std::vector<wxString> m_group_columns;
    std::vector<ModelGroupCache> m_cache;

    tango::objhandle_t m_handle;
};


class TangoModel : public IModel,
                   public kcanvas::IEventTarget,
                   public xcm::signal_sink
{
    XCM_CLASS_NAME("appmain.TangoModel")
    XCM_BEGIN_INTERFACE_MAP(TangoModel)
        XCM_INTERFACE_ENTRY(IModel)
        XCM_INTERFACE_ENTRY(kcanvas::IEventTarget)
    XCM_END_INTERFACE_MAP()

public:

    XCM_IMPLEMENT_SIGNAL0(sigModelLoaded)

public:

    TangoModel();
    virtual ~TangoModel();

    // IModel
    
    // two ways of settting query; single query, and multiple queries with sort
    // order; later is for union set since it's not supported by db
    void setQuery(const std::vector<wxString>& queries, const wxString& order);
    void setQuery(const wxString& query);
    bool isLoaded();

    void addProperty(const wxString& prop_name, const kcanvas::PropertyValue& value);
    void addProperties(const kcanvas::Properties& properties);
    void removeProperty(const wxString& prop_name);

    bool setProperty(const wxString& prop_name, const kcanvas::PropertyValue& value);
    bool setProperties(const kcanvas::Properties& properties);
    bool getProperty(const wxString& prop_name, kcanvas::PropertyValue& value) const;
    bool getProperties(kcanvas::Properties& properties) const;
    kcanvas::Properties& getPropertiesRef();

    bool addGroup(const wxString& label, const std::vector<wxString>& columns);
    bool setGroup(const wxString& label);
    void removeAllGroups();

    void goFirst();
    void skip(int delta_rows);
    bool eof();

    bool bog();
    bool eog();

    void execute(bool block = false);
    bool eval(int row, const wxString& expr, wxString& result);

    // IEventTarget
    bool addEventHandler(const wxString& name,
                         kcanvas::IEventHandlerPtr handler,
                         bool capture_phase = false);
    bool removeEventHandler(const wxString& name,
                            kcanvas::IEventHandlerPtr handler,
                            bool capture_phase = false);
    void removeAllEventHandlers();
    void dispatchEvent(kcanvas::IEventPtr evt);

public:

    bool getColumnInfo(const wxString& col_name, int* idx, int* type, int* width, int* scale) const;
    wxString getString(int col_idx, int function);
    tango::DateTime getDateTime(int col_idx, int function);
    double getDouble(int col_idx, int function);
    int getInteger(int col_idx, int function);
    bool getBoolean(int col_idx, int function);

protected:

    // function for adding event handler
    template<class T>
    void addEventHandler(const wxString& name,
                         void (T::*func)(kcanvas::IEventPtr),
                         bool capture_phase = false)
    {
        T* t = (T*)this;
        kcanvas::EventHandler* handler = new kcanvas::EventHandler;
        handler->sigEvent().connect(t, func);
        addEventHandler(name, handler, capture_phase);
    }

private:

    void onQueryJobFinished(jobs::IJobPtr job);

private:

    void setIterator(tango::IIterator* it);
    bool gotoRow(int row);
    void clear();
    void refresh();

private:

    // expressions, groups, and columns
    std::map<wxString, CellExpression*> m_cell_expressions;
    std::map<wxString, ModelGroup*> m_groups;
    std::vector<ModelColumn*> m_columns;

    // event handlers
    kcanvas::EventTarget m_event_target;

    // properties
    kcanvas::Properties m_properties;

    // query and iterator
    wxString m_query;
    tango::IIterator* m_iter;

    // current group and row position
    wxString m_group_label;
    int m_rowpos;
    
    jobs::IJobPtr m_job;
};


#endif

