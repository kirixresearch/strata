/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams
 * Created:  2007-04-10
 *
 */


#include "appmain.h"
#include "reportmodel.h"
#include "../kscript/util.h"


void func_pagenumber(kscript::ExprEnv* env,
                     void* param,
                     kscript::Value* retval)
{
    CellExpression* cellexpr = (CellExpression*)param;
    XdModel* xd_model = cellexpr->getModel();
    
    if (xd_model == NULL)
    {
        retval->setInteger(0);
        return;
    }

    // get the property
    kcanvas::PropertyValue value;
    xd_model->getProperty(PROP_REPORT_PAGE_NUMBER, value);
    int result = value.getInteger();

    // set the return value
    retval->setType(kscript::Value::typeInteger);
    retval->setInteger(result);
}

void func_pagecount(kscript::ExprEnv* env,
                    void* param,
                    kscript::Value* retval)
{
    CellExpression* cellexpr = (CellExpression*)param;
    XdModel* xd_model = cellexpr->getModel();
    
    if (xd_model == NULL)
    {
        retval->setInteger(0);
        return;
    }

    // get the property
    kcanvas::PropertyValue value;
    xd_model->getProperty(PROP_REPORT_PAGE_COUNT, value);
    int result = value.getInteger();

    // set the return value
    retval->setType(kscript::Value::typeInteger);
    retval->setInteger(result);
}

void func_currentdate(kscript::ExprEnv* env,
                      void* param,
                      kscript::Value* retval)
{
    CellExpression* cellexpr = (CellExpression*)param;
    XdModel* xd_model = cellexpr->getModel();
    
    if (xd_model == NULL)
    {
        retval->setString(wxEmptyString);
        return;
    }

    // get the property
    kcanvas::PropertyValue value;
    xd_model->getProperty(PROP_REPORT_CURRENT_DATE, value);
    wxString result = value.getString();

    // set the return value
    retval->setType(kscript::Value::typeString);
    retval->setString(result.c_str());
}

void func_datasource(kscript::ExprEnv* env,
                     void* param,
                     kscript::Value* retval)
{
    CellExpression* cellexpr = (CellExpression*)param;
    XdModel* xd_model = cellexpr->getModel();
    
    if (xd_model == NULL)
    {
        retval->setString(wxEmptyString);
        return;
    }

    // get the property
    kcanvas::PropertyValue value;
    xd_model->getProperty(PROP_REPORT_DATA_SOURCE, value);
    wxString result = value.getString();

    // set the return value
    retval->setType(kscript::Value::typeString);
    retval->setString(result.c_str());
}

void func_tablename(kscript::ExprEnv* env,
                    void* param,
                    kscript::Value* retval)
{
    // deprecated; used in beta reports; keep around until beta 
    // reports are converted
    func_datasource(env, param, retval);
}


class FieldExprElement : public kscript::ExprElement
{
public:

    FieldExprElement(XdModel* model, const wxString& expr, int function)
    {
        m_model = model;
        m_expr = expr;
        m_func = function;
    }

    ~FieldExprElement()
    {
    }

    int eval(kscript::ExprEnv*, kscript::Value* retval)
    {
        // TODO: add support for the count function

        // if we don't have a valid field, we're done
        int idx, type, width, scale;
        if (!m_model->getColumnInfo(m_expr, &idx, &type, &width, &scale))
            return kscript::errorUndeclaredIdentifier;

        switch (type)
        {
            case typeCharacter:
                {
                    wxString result = m_model->getString(idx, m_func);
                    retval->setString(result.c_str());
                }
                break;

            case typeDouble:
                {
                    double result = m_model->getDouble(idx, m_func);
                    retval->setDouble(result);
                    
                    /*
                    ModelValue* v = static_cast<ModelValue*>(retval);
                    if (v)
                    {
                        if (v->m_max_scale < scale)
                            v->m_max_scale = scale;
                    }
                    */
                }
                break;

            case typeInteger:
                {
                    int result = m_model->getInteger(idx, m_func);
                    retval->setInteger(result);
                    
                    /*
                    ModelValue* v = static_cast<ModelValue*>(retval);
                    if (v)
                    {
                        if (v->m_max_scale < scale)
                            v->m_max_scale = scale;
                    }
                    */
                }
                break;

            case typeDate:
            case typeDateTime:
                {
                    xd::datetime_t dt, d, t;
                    dt = m_model->getDateTime(idx, m_func);
                    d = dt >> 32;
                    t = dt & 0xffffffff;
                    retval->setDateTime((unsigned int)d, (unsigned int)t);
                    
                    /*
                    ModelValue* v = static_cast<ModelValue*>(retval);
                    if (v)
                    {
                        v->m_xd_type = type;
                    }
                    */
                }
                break;

            case typeBoolean:
                {
                    bool result = m_model->getBoolean(idx, m_func);
                    retval->setBoolean(result);
                }
                break;
                
            default:
                {
                    return kscript::errorUnknown;
                }
        }

        return kscript::errorNone;
    }

    int getType()
    {
        // default result value
        int result = kscript::Value::typeNull;
    
        // if we don't have a valid field, we're done
        int idx, type, width, scale;
        if (!m_model->getColumnInfo(m_expr, &idx, &type, &width, &scale))
            return result;

        switch (type)
        {
            case typeCharacter:
                result = kscript::Value::typeString;
                break;

            case typeDouble:
                result = kscript::Value::typeDouble;
                break;

            case typeInteger:
                result = kscript::Value::typeInteger;
                break;

            case typeDate:
            case typeDateTime:
                result = kscript::Value::typeDateTime;
                break;

            case typeBoolean:
                result = kscript::Value::typeBoolean;
                break;
        }

        return result;
    }

public:

    XdModel* m_model;
    wxString m_expr;
    int m_func;
};


CellExpression::CellExpression()
{
    // model
    m_xd_model = NULL;

    // expression parser and return value
    m_expr_parser = NULL;
    m_retval = NULL;
    
    // parse hook
    m_parsehook_func = NULL;
    m_parsehook_param = NULL;

    // expression field info
    m_xd_type = typeInvalid;
    m_max_scale = 0;
}

CellExpression::CellExpression(const CellExpression& c)
{
    // model
    m_xd_model = c.m_xd_model;

    // expression parser and return value
    m_expr_parser = c.m_expr_parser;
    m_retval = c.m_retval;
    
    // parse hook
    m_parsehook_func = c.m_parsehook_func;
    m_parsehook_param = c.m_parsehook_param;

    // expression field info
    m_xd_type = c.m_xd_type;
    m_max_scale = c.m_max_scale;
}

CellExpression::~CellExpression()
{
    clear();
}

void CellExpression::setParseHook(kscript::ExprParseHookFunc func, void* param)
{
    m_parsehook_func = func;
    m_parsehook_param = param;
}

void CellExpression::setModel(XdModel* model)
{
    m_xd_model = model;
}

XdModel* CellExpression::getModel()
{
    return m_xd_model;
}

void CellExpression::setExpression(const wxString& expr)
{
    clear();
    m_expr = expr;
}

wxString CellExpression::getExpression()
{
    return m_expr;
}

bool CellExpression::getResult(wxString& result)
{
    if (!m_expr_parser)
    {
        m_expr_parser = new kscript::ExprParser(kscript::optionLanguageGeneric |
                                                kscript::optionEpsilonNumericCompare);

        // this line adds standard database functions to the expression parser
        // (for example SUBSTR())
        xd::bindExprParser((void*)m_expr_parser);
        
        if (m_parsehook_func != NULL && m_parsehook_param != NULL)
        {
            m_expr_parser->setParseHook(0xffffffff, m_parsehook_func, m_parsehook_param);

            // TODO: adding the following functions here limits CellExpression
            // to this particular implementation; figure out an alternate
            // way to add these functions so we can use CellExpression with
            // different sets of functions

            // add the functions
            m_expr_parser->addFunction(L"pagenumber", false, func_pagenumber, false, L"i()", m_parsehook_param);
            m_expr_parser->addFunction(L"pagecount", false, func_pagecount, false, L"i()", m_parsehook_param);
            m_expr_parser->addFunction(L"datasource", false, func_datasource, false, L"s()", m_parsehook_param);

            // CurrentDate() is deprecated because default database functions
            // contain function with name CurDate(), which now provides this
            // functionality; keep following around until until beta reports
            // are converted
            m_expr_parser->addFunction(L"currentdate", false, func_currentdate, false, L"s()", m_parsehook_param);

            // TableName() is deprecated; used in beta reports; keep around 
            // until beta reports are converted
            m_expr_parser->addFunction(L"tablename", false, func_tablename, false, L"s()", this);
        }

        // parse the expression
        m_expr_parser->parse(towstr(m_expr));
    }

    if (!m_retval)
    {
        m_retval = new kscript::Value;
    }

    if (!m_expr_parser->eval(m_retval))
    {
        return false;
    }
    else
    {
        // convert the result to a string, based
        // on the parser return type
        int type = m_expr_parser->getType();

        switch (type)
        {
            case kscript::Value::typeString:
                {
                    result = m_retval->getString();
                }
                break;

            case kscript::Value::typeDouble:
                {
                    // set the return string
                    double r = m_retval->getDouble();
                    result = wxString::Format(wxT("%.*f"), m_max_scale, r);
                }
                break;

            case kscript::Value::typeInteger:
                {
                    int r = m_retval->getInteger();
                    result = wxString::Format(wxT("%d"), r);
                }
                break;

            case kscript::Value::typeDateTime:
                {
                    // default
                    result = wxEmptyString;

                    kscript::ExprDateTime dt = m_retval->getDateTime();
                    
                    if (m_xd_type != typeDateTime)
                    {
                        // if the kscript type is a date and the xd type is
                        // anything besides a datetime, format the date without
                        // the timestamp portion

                        if (dt.date > 0)
                        {
                            int year, month, day;
                            kscript::julianToDate(dt.date, &year, &month, &day);

                            result = Locale::formatDate(year, 
                                                             month, 
                                                             day).c_str();
                        }
                    }
                    else if (m_xd_type == typeDateTime)
                    {
                        // if the kscript type is a date and the xd type is
                        // datetime, format the date with the timestamp portion

                        kscript::ExprDateTime dt = m_retval->getDateTime();
                        
                        if (dt.date > 0)
                        {
                            int year, month, day, hour, minute, second;
                            kscript::julianToDate(dt.date, &year, &month, &day);

                            hour = (dt.time+1)/3600000;
                            minute = dt.time - ((dt.time+1)/3600000)*3600000;
                            minute = minute/60000;
                            second = dt.time - ((dt.time+1)/3600000)*3600000;
                            second = second - (second/60000)*60000;
                            second = second/1000;

                            result = Locale::formatDate(year,
                                                             month,
                                                             day,
                                                             hour,
                                                             minute,
                                                             second).c_str();
                        }                                                         
                    }
                }
                break;

            case kscript::Value::typeBoolean:
                {
                    bool r = m_retval->getBoolean();
                    result = r ? _("True") : _("False");
                }
                break;
        }
    }
    
    return true;
}

void CellExpression::trackType(int type)
{
    if (type == typeDate && m_xd_type != typeDateTime)
        m_xd_type = typeDate;

    if (type == typeDateTime)
        m_xd_type = typeDateTime;
}

void CellExpression::trackScale(int scale)
{
    if (scale > m_max_scale)
        m_max_scale = scale;
}

void CellExpression::clear()
{
    if (m_expr_parser)
        delete m_expr_parser;
    
    if (m_retval)
        delete m_retval;
}


ModelColumn::ModelColumn()
{
    m_name = wxT("");
    m_handle = 0;
    m_xd_type = 0;
    m_type = 0;
    m_width = 0;
    m_scale = 0;
}

ModelColumn::~ModelColumn()
{

}

void ModelColumn::setName(const wxString& new_val)
{
    m_name = new_val;
}

const wxString& ModelColumn::getName()
{
    return m_name;
}

void ModelColumn::setType(int new_val)
{
    m_type = new_val;
}

int ModelColumn::getType()
{
    return m_type;
}

void ModelColumn::setWidth(int new_val)
{
    m_width = new_val;
}

int ModelColumn::getWidth()
{
    return m_width;
}

void ModelColumn::setScale(int new_val)
{
    m_scale = new_val;
}

int ModelColumn::getScale()
{
    return m_scale;
}


ModelGroup::ModelGroup()
{
    m_handle = 0;
}

ModelGroup::~ModelGroup()
{
}

void ModelGroup::addGroupColumn(const wxString& column)
{
    // don't add column index if it's already part of the 
    // group definition
    std::vector<wxString>::iterator it, it_end;
    it_end = m_group_columns.end();

    for (it = m_group_columns.begin(); it != it_end; ++it)
    {
        if (!it->CmpNoCase(column))
            return;
    }

    m_group_columns.push_back(column);
}

void ModelGroup::removeGroupColumn(const wxString& column)
{
    std::vector<wxString>::iterator it, it_end;
    it_end = m_group_columns.end();

    for (it = m_group_columns.begin(); it != it_end; ++it)
    {
        if (!it->CmpNoCase(column))
        {
            m_group_columns.erase(it);
            return;
        }
    }
}

void ModelGroup::removeAllGroupColumns()
{
    m_group_columns.clear();
}

std::vector<wxString> ModelGroup::getColumns()
{
    return m_group_columns;
}

wxString ModelGroup::getColumnByIdx(int col_idx)
{
    if (col_idx < 0 || (size_t)col_idx >= m_group_columns.size())
        return wxT("");

    return m_group_columns[col_idx];
}

int ModelGroup::getColumnCount()
{
    return m_group_columns.size();
}

void ModelGroup::setStringCache(int col_idx, int function, const wxString& value)
{
    ModelGroupCache cache;
    cache.m_column_idx = col_idx;
    cache.m_function = function;
    cache.m_string = value;
    m_cache.push_back(cache);
}

bool ModelGroup::getStringCache(int col_idx, int function, wxString& value)
{
    std::vector<ModelGroupCache>::iterator it, it_end;
    it_end = m_cache.end();

    for (it = m_cache.begin(); it != it_end; ++it)
    {
        if (it->m_column_idx == col_idx && it->m_function == function)
        {
            value = it->m_string;
            return true;
        }
    }

    return false;
}

void ModelGroup::setDoubleCache(int col_idx, int function, double value)
{
    ModelGroupCache cache;
    cache.m_column_idx = col_idx;
    cache.m_function = function;
    cache.m_double = value;
    m_cache.push_back(cache);
}

bool ModelGroup::getDoubleCache(int col_idx, int function, double* value)
{
    std::vector<ModelGroupCache>::iterator it, it_end;
    it_end = m_cache.end();

    for (it = m_cache.begin(); it != it_end; ++it)
    {
        if (it->m_column_idx == col_idx && it->m_function == function)
        {
            *value = it->m_double;
            return true;
        }
    }

    return false;
}

void ModelGroup::setIntegerCache(int col_idx, int function, int value)
{
    ModelGroupCache cache;
    cache.m_column_idx = col_idx;
    cache.m_function = function;
    cache.m_integer = value;
    m_cache.push_back(cache);
}

bool ModelGroup::getIntegerCache(int col_idx, int function, int* value)
{
    std::vector<ModelGroupCache>::iterator it, it_end;
    it_end = m_cache.end();

    for (it = m_cache.begin(); it != it_end; ++it)
    {
        if (it->m_column_idx == col_idx && it->m_function == function)
        {
            *value = it->m_integer;
            return true;
        }
    }

    return false;
}

void ModelGroup::setBooleanCache(int col_idx, int function, bool value)
{
    ModelGroupCache cache;
    cache.m_column_idx = col_idx;
    cache.m_function = function;
    cache.m_boolean = value;
    m_cache.push_back(cache);
}

bool ModelGroup::getBooleanCache(int col_idx, int function, bool* value)
{
    std::vector<ModelGroupCache>::iterator it, it_end;
    it_end = m_cache.end();

    for (it = m_cache.begin(); it != it_end; ++it)
    {
        if (it->m_column_idx == col_idx && it->m_function == function)
        {
            *value = it->m_boolean;
            return true;
        }
    }

    return false;
}

void ModelGroup::clearCache()
{
    m_cache.clear();
}


XdModel::XdModel()
{
    m_iter = NULL;
    m_group_label = PROP_REPORT_DETAIL;

    // add a default report header/footer and
    // detail groups
    std::vector<wxString> no_columns;
    addGroup(PROP_REPORT_DETAIL, no_columns);
    addGroup(PROP_REPORT_HEADER, no_columns);
    addGroup(PROP_REPORT_FOOTER, no_columns);
    
    // set the row position
    m_rowpos = -1;
}

XdModel::~XdModel()
{
    setIterator(NULL);
}

void XdModel::setQuery(const std::vector<wxString>& queries, const wxString& order)
{
    // for now, just use the first query
    if (queries.size() > 0)
        m_query = queries[0];
        
    setIterator(NULL);
}

void XdModel::setQuery(const wxString& query)
{
    // set the query string and reset the iterator
    m_query = query;
    setIterator(NULL);
}

bool XdModel::isLoaded()
{
    return (m_iter != NULL) ? true : false;
}

void XdModel::addProperty(const wxString& prop_name, const kcanvas::PropertyValue& value)
{
    m_properties.add(prop_name, value);
}

void XdModel::addProperties(const kcanvas::Properties& properties)
{
    m_properties.add(properties);
}

void XdModel::removeProperty(const wxString& prop_name)
{
    m_properties.remove(prop_name);
}

bool XdModel::setProperty(const wxString& prop_name, const kcanvas::PropertyValue& value)
{
    return m_properties.set(prop_name, value);
}

bool XdModel::setProperties(const kcanvas::Properties& properties)
{
    return m_properties.set(properties);
}

bool XdModel::getProperty(const wxString& prop_name, kcanvas::PropertyValue& value) const
{
    return m_properties.get(prop_name, value);
}

bool XdModel::getProperties(kcanvas::Properties& properties) const
{
    return m_properties.get(properties);
}

kcanvas::Properties& XdModel::getPropertiesRef()
{
    return m_properties;
}

bool XdModel::addGroup(const wxString& label, const std::vector<wxString>& columns)
{
    if (!m_iter)
        return false;

    // if the group exists, return false
    std::map<wxString, ModelGroup*>::iterator it;
    it = m_groups.find(label);
    if (it != m_groups.end())
        return false;

    ModelGroup* group = new ModelGroup;

    // if we have columns, add the columns to the group definition
    // and set the handle
    if (columns.size() > 0)
    {
        bool first = true;
        wxString group_expression = wxT("KEY:");
    
        std::vector<wxString>::const_iterator it, it_end;
        it_end = columns.end();

        for (it = columns.begin(); it != it_end; ++it)
        {
            if (!first)
                group_expression += wxT(",");
            
            first = false;
            group_expression += *it;
        
            group->addGroupColumn(*it);
        }

        group->m_handle = m_iter->getHandle(towstr(group_expression));
    }

    // add the group and we're done
    m_groups[label] = group;
    return true;
}

bool XdModel::setGroup(const wxString& label)
{
    // if the group exists, set the label and return true;
    // return false otherwise
    
    std::map<wxString, ModelGroup*>::iterator it;
    it = m_groups.find(label);
    
    if (it == m_groups.end())
        return false;
        
    m_group_label = label;
    return true;
}

void XdModel::removeAllGroups()
{
    // if we don't have an iterator, we're done
    if (!m_iter)
        return;

    // iterate through the groups
    std::map<wxString, ModelGroup*>::iterator it, it_end;
    it_end = m_groups.end();
    
    for (it = m_groups.begin(); it != it_end; ++it)
    {
        // for each group, release the groups handle and 
        // dereference the group pointer
        ModelGroup* group = it->second;
        if (group->m_handle)
        {
            m_iter->releaseHandle(group->m_handle);
            delete group;
        }
    }
    
    // clear the groups
    m_groups.clear();
    
    // add a default detail group
    std::vector<wxString> no_columns;
    addGroup(PROP_REPORT_DETAIL, no_columns);
    addGroup(PROP_REPORT_HEADER, no_columns);
    addGroup(PROP_REPORT_FOOTER, no_columns);
}

void XdModel::goFirst()
{
    if (!m_iter)
        return;

    m_iter->goFirst();
    m_rowpos = 1;
}

void XdModel::skip(int delta_rows)
{
    if (!m_iter)
        return;

    // skip to new row
    m_iter->skip(delta_rows);
    m_rowpos += delta_rows;
    
    // if we've gone past the end of the file,
    // reset the row position
    if (m_iter->eof())
        m_rowpos = -1;
}

bool XdModel::eof()
{
    if (!m_iter)
        return true;

    return m_iter->eof();
}

bool XdModel::bog()
{
    // if we don't have an iterator, return false
    if (!m_iter)
        return false;

    // special case for report header and footer; the
    // very first record is the beginning of the group
    // for the entire table
    if (m_group_label == PROP_REPORT_HEADER ||
        m_group_label == PROP_REPORT_FOOTER)
    {
        if (m_rowpos <= 1)
            return true;

        return false;
    }

    // special case for detail; for the detail, treat every row as the
    // beginning of a group, so return true
    if (m_group_label == PROP_REPORT_DETAIL)
        return true;

    // get the current group; if we can't find the group, return false
    std::map<wxString, ModelGroup*>::const_iterator it;
    it = m_groups.find(m_group_label);
    if (it == m_groups.end())
        return false;

    // if the handle is invalid, return false
    ModelGroup* group = it->second;
    if (!group->m_handle)
        return false;

    // treat the first row as the beginning of a group
    if (m_rowpos <= 1)
        return true;

    // get the raw value of the current row in the group
    int len = m_iter->getRawWidth(group->m_handle);

    unsigned char* a = new unsigned char[len];
    unsigned char* b = (unsigned char*)m_iter->getRawPtr(group->m_handle);
    memcpy(a, b, len);

    // get the raw value of the previous row in the group; if 
    // it's different than the row before skipping, we're at 
    // the beginning of a group
    m_iter->skip(-1);
    b = (unsigned char*)m_iter->getRawPtr(group->m_handle);
    m_iter->skip(1);

    bool result;
    result = (memcmp(a, b, len) != 0);

    delete[] a;

    return result;
}

bool XdModel::eog()
{
    // if we don't have an iterator, return false
    if (!m_iter)
        return false;

    // if we're already at the end of the file, it's also
    // the end of the group
    if (m_iter->eof())
        return true;

    // special case for report header and footer; the
    // very last record is the end of the group for
    // the entire table
/*
    if (m_group_label == PROP_REPORT_HEADER ||
        m_group_label == PROP_REPORT_FOOTER)
    {
        m_iter->skip(1);
        if (m_iter->eof())
        {
            m_iter->skip(-1);
            return true;
        }
        
        return false;
    }
*/
    
    // special case for detail; for the detail, treat every 
    // row as the end of a group
    if (m_group_label == PROP_REPORT_DETAIL)
        return true;

    // get the current group; if we can't find the group, 
    // return false
    std::map<wxString, ModelGroup*>::const_iterator it;
    it = m_groups.find(m_group_label);
    if (it == m_groups.end())
        return false;

    // if the handle is invalid, return false
    ModelGroup* group = it->second;
    if (!group->m_handle)
        return false;

    // get the raw value of the current row in the group
    int len = m_iter->getRawWidth(group->m_handle);

    unsigned char* a = new unsigned char[len];
    unsigned char* b = (unsigned char*)m_iter->getRawPtr(group->m_handle);
    memcpy(a, b, len);

    // go forward a row
    m_iter->skip(1);
    
    // if we're at the end of the file, return true, thus 
    // treating the last row as the end of a group
    if (m_iter->eof())
    {
        m_iter->skip(-1);
        delete[] a;
        return true;
    }

    // get the raw value of the next row in the group; if 
    // it's different than the row before skipping, we're 
    // at the end of a group
    b = (unsigned char*)m_iter->getRawPtr(group->m_handle);
    m_iter->skip(-1);

    bool result;
    result = (memcmp(a, b, len) != 0);

    delete[] a;

    return result;
}

static bool func_xdmodel_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    if (hook_info.element_type == kscript::ExprParseHookInfo::typeFunction)
    {  
        CellExpression* cellexpr = (CellExpression*)hook_info.hook_param;
        XdModel* xd_model = cellexpr->getModel();
        
        if (xd_model == NULL)
            return false;

        int len = hook_info.expr_text.length();
        if (len == 0)
            return false;

        const wchar_t* expr_text = hook_info.expr_text.c_str();

        if (*(expr_text+len-1) != ')')
            return false;

        const wchar_t* open_paren = wcschr(expr_text, L'(');
        if (!open_paren)
            return false;

        open_paren++;
        while (iswspace(*open_paren))
            open_paren++;

        std::wstring expr;
        expr.assign(open_paren, wcslen(open_paren)-1);
        kl::trimRight(expr);

        // see if the cell starts with a formula string
        int agg_func = funcNone;
        if (!wcsncasecmp(expr_text, L"MIN(", 4))
            agg_func = funcMin;
        else if (!wcsncasecmp(expr_text, L"MAX(", 4))
            agg_func = funcMax;
        else if (!wcsncasecmp(expr_text, L"SUM(", 4))
            agg_func = funcSum;
        else if (!wcsncasecmp(expr_text, L"AVG(", 4))
            agg_func = funcAvg;
        else if (!wcsncasecmp(expr_text, L"COUNT(", 6))
            agg_func = funcCount;

        if (agg_func == funcNone)
            return false;

        // if the aggregate function is something besides count(),
        // it requires a valid field as input; see if the inner
        // expression is a valid field
        int idx, type, width, scale;
        
        if (agg_func != funcNone && 
            agg_func != funcCount &&
            !xd_model->getColumnInfo(expr, &idx, &type, &width, &scale))
        {
            return false;
        }

        // track the field type and scale so we can automatically
        // format the result
        cellexpr->trackType(type);
        cellexpr->trackScale(scale);

        // create a field expression element for actually
        // calculating the results
        FieldExprElement* e = new FieldExprElement(xd_model, expr, agg_func);
        hook_info.res_element = static_cast<kscript::ExprElement*>(e);
        return true;
    }

    // handle fieldnames
    if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        if (hook_info.expr_text.length() > 1 &&
            hook_info.expr_text[0] == '[' &&
            hook_info.expr_text[hook_info.expr_text.length()-1] == ']')
        {
            // remove brackets from beginning and end e.g. [Field Name] => Field Name
            hook_info.expr_text = hook_info.expr_text.substr(0, hook_info.expr_text.length()-1);
            hook_info.expr_text.erase(0, 1);
        }


        CellExpression* cellexpr = (CellExpression*)hook_info.hook_param;
        XdModel* xd_model = cellexpr->getModel();

        if (xd_model == NULL)
            return false;

        int len = hook_info.expr_text.length();
        if (len == 0)
            return false;

        // if we don't have a valid field, we're done
        int idx, type, width, scale;
        if (!xd_model->getColumnInfo(hook_info.expr_text, &idx, &type, &width, &scale))
            return false;

        // track the field type and scale so we can automatically
        // format the result
        cellexpr->trackType(type);
        cellexpr->trackScale(scale);

        // create a field expression element for actually
        // calculating the results
        FieldExprElement* e = new FieldExprElement(xd_model, hook_info.expr_text, funcNone);
        hook_info.res_element = static_cast<kscript::ExprElement*>(e);
        return true;
    }

    return false;
}

void XdModel::execute(bool block)
{
    // reset the iterator
    setIterator(NULL);

    if (!block)
    {
        if (m_job)
        {
            m_job->sigJobFinished().disconnect();
            m_job->cancel();
            m_job.clear();
        }

        // if we're not in blocking mode, run the query job and
        // handle everything else when the job is finished

        jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.execute-job");

        kl::JsonNode params;
        params["command"].setString(towstr(m_query));

        job->getJobInfo()->setTitle(towstr(_("Query")));
        job->setParameters(params.toString());
        job->sigJobFinished().connect(this, &XdModel::onQueryJobFinished);
        g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
    }
     else
    {
        // create a query job and run it directly
        jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.execute-job");

        kl::JsonNode params;
        params["command"].setString(towstr(m_query));

        job->setParameters(params.toString());
        job->runJob();
        job->runPostJob();

        xd::IIteratorPtr iter = job->getResultObject();
        setIterator(iter);
    }
}

bool XdModel::eval(int row, const wxString& expr, wxString& result)
{
    // try to go to the row in question; don't worry about
    // returning false if we can't get there: some expressions
    // don't depend on the row and for these, we still want
    // to be able to evaluate them regardless of whether we
    // actually have any rows in the model or not
    gotoRow(row);

    // if we already have an expression, use it
    std::map<wxString, CellExpression*>::iterator it;
    it = m_cell_expressions.find(expr);
    if (it != m_cell_expressions.end())
        return it->second->getResult(result);

    // if we don't have an expression, add it
    CellExpression* cell_expr = new CellExpression;
    cell_expr->setParseHook(func_xdmodel_parse_hook, cell_expr);
    cell_expr->setModel(this);
    cell_expr->setExpression(expr);
    m_cell_expressions[expr] = cell_expr;

    // return the results
    return cell_expr->getResult(result);
}

bool XdModel::addEventHandler(const wxString& name,
                                 kcanvas::IEventHandlerPtr handler,
                                 bool capture_phase)
{
    return m_event_target.addEventHandler(name, handler, capture_phase);
}

bool XdModel::removeEventHandler(const wxString& name,
                                    kcanvas::IEventHandlerPtr handler,
                                    bool capture_phase)
{
    return m_event_target.removeEventHandler(name, handler, capture_phase);
}

void XdModel::removeAllEventHandlers()
{
    m_event_target.removeAllEventHandlers();
}

void XdModel::dispatchEvent(kcanvas::IEventPtr evt)
{
    // because the model isn't a member of a hierarchy,
    // the event is always dispatched at the target;
    // i.e., there's no bubbling or capture phase
    evt->setPhase(kcanvas::EVENT_AT_TARGET);
    m_event_target.dispatchEvent(evt);
}

bool XdModel::getColumnInfo(const wxString& col_name, 
                               int* idx, 
                               int* type, 
                               int* width, 
                               int* scale) const
{
    int index = 0;

    std::vector<ModelColumn*>::const_iterator it, it_end;
    it_end = m_columns.end();

    for (it = m_columns.begin(); it != it_end; ++it)
    {
        if (!(*it)->m_name.CmpNoCase(col_name))
        {
            *idx = index;
            *type = (*it)->m_type;
            *width = (*it)->m_width;
            *scale = (*it)->m_scale;
            return true;
        }

        index++;
    }

    *idx = -1;
    *type = typeInvalid;
    *width = 0;
    *scale = 0;
    return false;
}

wxString XdModel::getString(int col_idx, int function)
{
    if (!m_iter)
        return wxT("");

    if (function == funcSum || function == funcAvg || function == funcCount)
        return wxT("");

    wxString value;
    int col_type = m_columns[col_idx]->m_xd_type;

    switch (col_type)
    {
        default:

        case xd::typeCharacter:
            value = m_iter->getString(m_columns[col_idx]->m_handle);
            break;

        case xd::typeWideCharacter:
            value = m_iter->getWideString(m_columns[col_idx]->m_handle);
            break;

        return wxT("");
    }

    if (function == funcInvalid || function == funcNone)
        return value;

    // find the beginning of the group
    while (!bog())
    {
        skip(-1);
    }

    // find the function value
    wxString new_value;
    while (!eog())
    {
        switch (col_type)
        {
            default:

            case xd::typeCharacter:
                new_value = m_iter->getString(m_columns[col_idx]->m_handle);
                break;

            case xd::typeWideCharacter:
                new_value = m_iter->getWideString(m_columns[col_idx]->m_handle);
                break;
        }

        switch (function)
        {
            default:

            case funcFirst:
                return new_value;

            case funcLast:
                value = new_value;
                break;

            case funcMin:
                if (new_value < value) value = new_value;
                break;

            case funcMax:
                if (new_value > value) value = new_value;
                break;
        }

        skip(1);
    }

    return value;
}

xd::DateTime XdModel::getDateTime(int col_idx, int function)
{
    // used throughout
    xd::DateTime value;

    // if we don't have an iterator, return a blank date
    if (!m_iter)
        return value;

    // if the function is something that doesn't make sense
    // for a date, return a blank date
    if (function == funcSum || function == funcAvg || function == funcCount)
        return value;

    // get the column
    int col_type = m_columns[col_idx]->m_xd_type;

    // get the value
    switch (col_type)
    {
        default:

        case xd::typeDate:
        case xd::typeDateTime:
            value = m_iter->getDateTime(m_columns[col_idx]->m_handle);
            break;

        // if for some reason, we don't have a date or
        // date time column, return a blank date
        return value;
    }

    // if we have an invalid function or no function at
    // all, we don't need to do any further calculations;
    // return the value we already have
    if (function == funcInvalid || function == funcNone)
    {
        return value;
    }

    // find the beginning of the group
    while (!bog())
    {
        skip(-1);
    }

    // find the function value
    xd::DateTime new_value;
    while (!eog())
    {
        new_value = m_iter->getDateTime(m_columns[col_idx]->m_handle);
        switch (function)
        {
            default:

            case funcFirst:
                return new_value;

            case funcLast:
                value = new_value;
                break;

            case funcMin:
                if (new_value < value) value = new_value;
                break;

            case funcMax:
                if (new_value > value) value = new_value;
                break;
        }

        skip(1);
    }

    return value;
}

double XdModel::getDouble(int col_idx, int function)
{
    if (!m_iter)
        return 0.0f;

    double value;
    int col_type = m_columns[col_idx]->m_xd_type;

    // if the type is not double or numeric, return 0
    if (col_type != xd::typeDouble && col_type != xd::typeNumeric)
        return 0.0f;

    // find the value
    value = m_iter->getDouble(m_columns[col_idx]->m_handle);

    if (function == funcInvalid || function == funcNone)
        return value;
    
    // find the beginning of the group
    while (!bog())
    {
        skip(-1);
    }

    // find the function value
    value = m_iter->getDouble(m_columns[col_idx]->m_handle);
    double new_value = value;
    double total = value;
    int count = 1;

    // initialize value for count function
    if (function == funcCount)
        value = 1.0f;

    while (!eog())
    {
        skip(1);

        if (eof())
            break;

        new_value = m_iter->getDouble(m_columns[col_idx]->m_handle);
        ++count;

        switch (function)
        {
            default:

            case funcFirst:
                return total;

            case funcLast:
                value = new_value;
                break;

            case funcMin:
                if (new_value < value) value = new_value;
                break;

            case funcMax:
                if (new_value > value) value = new_value;
                break;

            case funcSum:
                total += new_value;
                break;

            case funcAvg:
                total += new_value;
                break;

            case funcCount:
                value = count;
                break;
        }
    }

    if (function == funcSum)
        value = total;

    if (function == funcAvg)
        value = total/count;

    return value;
}

int XdModel::getInteger(int col_idx, int function)
{
    if (!m_iter)
        return 0;

    int value;
    int col_type = m_columns[col_idx]->m_xd_type;

    // if the type is not integer, return 0
    if (col_type != xd::typeInteger)
        return 0;

    // find the value
    value = m_iter->getInteger(m_columns[col_idx]->m_handle);

    if (function == funcInvalid || function == funcNone)
        return value;
    
    // find the beginning of the group
    while (!bog())
    {
        skip(-1);
    }

    // find the function value
    value = m_iter->getInteger(m_columns[col_idx]->m_handle);
    int new_value = value;
    int total = value;
    int count = 1;

    // initialize value for count function
    if (function == funcCount)
        value = 1;

    while (!eog())
    {
        skip(1);

        if (eof())
            break;

        new_value = m_iter->getInteger(m_columns[col_idx]->m_handle);
        ++count;

        switch (function)
        {
            default:

            case funcFirst:
                return total;

            case funcLast:
                value = new_value;
                break;

            case funcMin:
                if (new_value < value) value = new_value;
                break;

            case funcMax:
                if (new_value > value) value = new_value;
                break;

            case funcSum:
                total += new_value;
                break;

            case funcAvg:
                total += new_value;
                break;

            case funcCount:
                value = count;
                break;
        }
    }

    if (function == funcSum)
        value = total;

    if (function == funcAvg)
        value = total/count;

    return value;
}

bool XdModel::getBoolean(int col_idx, int function)
{
    if (!m_iter)
        return false;

    bool value = false;
    int col_type = m_columns[col_idx]->m_xd_type;

    // if the type is not boolean, return false
    if (col_type != xd::typeBoolean)
        return false;

    // find the value
    value = m_iter->getBoolean(m_columns[col_idx]->m_handle);

    if (function == funcInvalid || function == funcNone ||
        function == funcSum || function == funcAvg || function == funcCount)
        return value;
    
    // find the beginning of the group
    while (!bog())
    {
        skip(-1);
    }

    // find the function value
    bool new_value;
    while (!eog())
    {
        new_value = m_iter->getBoolean(m_columns[col_idx]->m_handle);

        switch (function)
        {
            default:

            case funcFirst:
                return new_value;

            case funcLast:
                value = new_value;
                break;

            case funcMin:
                if (new_value < value) value = new_value;
                break;

            case funcMax:
                if (new_value > value) value = new_value;
                break;
        }

        skip(1);
    }

    return value;
}

void XdModel::onQueryJobFinished(jobs::IJobPtr job)
{
    m_job.clear();
    
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;

    xd::IIteratorPtr iter = job->getResultObject();
    setIterator(iter);

    // fire a signal indicating that the data has been loaded
    sigModelLoaded().fire();

    // dispatch an event indicating that the data has been loaded;
    // note: we dispatch both an even and fire a signal so the
    // model can be used with either system of handling events
    kcanvas::INotifyEventPtr notify_evt;
    notify_evt = kcanvas::NotifyEvent::create(EVENT_MODEL_LOADED, this);
    dispatchEvent(notify_evt);
}

void XdModel::setIterator(xd::IIterator* it)
{
    // reset state variables
    m_group_label = PROP_REPORT_DETAIL;
    m_rowpos = -1;

    // if an iterator exists, clear related objects and handles
    if (m_iter)
    {
        clear();

        m_iter->unref();
        m_iter = NULL;
    }

    // if the new iterator exists, refresh the model with the
    // new iterator
    if (it)
    {
        m_iter = it;
        m_iter->ref();

        if (m_iter->getIteratorFlags() & xd::ifForwardOnly)
        {
            // if the iterator is forward-only, try to turn on
            // xd's backward scroll row cache
            m_iter->setIteratorFlags(xd::ifReverseRowCache, xd::ifReverseRowCache);
        }

        refresh();
    }
}

bool XdModel::gotoRow(int row)
{
    // note: this is a simple function to that allows us to skip
    // to a location based on knowledge of where we are so that 
    // rows can be accessed randomly in the report; this saves a 
    // lot of time rendering the report in layout mode; the reason 
    // it saves time is because when we render a cell, we have to 
    // get it's corresponding value from the data row it came from;
    // however, when we render a cell, we only know what absolute 
    // row position it came from and don't know the state of the 
    // iterator at the time we're rendering it; as a result, without 
    // this function, we have to go to the first position in the
    // model, the skip to the model row to the get the correct 
    // data value, which takes time; by factoring it out here, we 
    // can store that information and move relative to where we 
    // are currently

    // if we don't have an iterator, or we're trying to
    // go to a negative offset, return false
    if (!m_iter || row <= 0)
        return false;

    // if we're already on the row we want to
    // go to, return true
    if (row == m_rowpos)
        return true;

    // if we know what row we're on, simply skip there;
    // if we don't know what row we're on, go to the
    // first row, and skip where we want to go
    if (m_rowpos > 0)
    {
        m_iter->skip(row - m_rowpos);
        m_rowpos = row;
    }
    else
    {
        m_iter->goFirst();
        m_iter->skip(row - 1);
        m_rowpos = row;
    }
    
    // if the row position is negative, we've skipped past
    // the end of the file; return false
    if (m_rowpos == -1)
        return false;

    return true;
}

void XdModel::clear()
{
    // if we don't have an iterator, we're done
    if (!m_iter)
        return;

    // release the group handles
    std::map<wxString, ModelGroup*>::iterator it, it_end;
    it_end = m_groups.end();
    
    for (it = m_groups.begin(); it != it_end; ++it)
    {
        // for each group, release the groups handle and 
        // dereference the group pointer
        ModelGroup* group = it->second;
        if (group->m_handle)
        {
            m_iter->releaseHandle(group->m_handle);
            delete group;
        }
    }
    
    // clear the groups
    m_groups.clear();

    // release the column handles
    std::vector<ModelColumn*>::iterator it_col, it_col_end;
    it_col_end = m_columns.end();

    for (it_col = m_columns.begin(); it_col != it_col_end; ++it_col)
    {
        if (*it_col)
        {
            m_iter->releaseHandle((*it_col)->m_handle);
            delete *it_col;
        }
    }

    // clear the columns
    m_columns.clear();
    
    // release the cached expressions
    std::map<wxString, CellExpression*>::iterator it_expr, it_expr_end;
    it_expr_end = m_cell_expressions.end();
    
    for (it_expr = m_cell_expressions.begin(); it_expr != it_expr_end; ++it_expr)
    {
        delete it_expr->second;
    }
    
    m_cell_expressions.clear();
}

void XdModel::refresh()
{
    // refresh columns
    xd::Structure structure = m_iter->getStructure();
    size_t i, col_count = structure.getColumnCount();

    m_columns.resize(col_count, NULL);

    for (i = 0; i < col_count; ++i)
    {
        m_columns[i] = new ModelColumn;
        m_columns[i]->m_name = structure.getColumnName(i);
        m_columns[i]->m_handle = m_iter->getHandle(towstr(m_columns[i]->m_name));

        xd::ColumnInfo colinfo = m_iter->getInfo(m_columns[i]->m_handle);
        m_columns[i]->m_xd_type = colinfo.type;
        m_columns[i]->m_width = colinfo.width;
        m_columns[i]->m_scale = colinfo.scale;

        switch (m_columns[i]->m_xd_type)
        {
            case xd::typeCharacter:     m_columns[i]->m_type = typeCharacter; break;
            case xd::typeWideCharacter: m_columns[i]->m_type = typeCharacter; break;
            case xd::typeNumeric:       m_columns[i]->m_type = typeDouble;    break;
            case xd::typeDouble:        m_columns[i]->m_type = typeDouble;    break;
            case xd::typeInteger:       m_columns[i]->m_type = typeInteger;   break;
            case xd::typeDate:          m_columns[i]->m_type = typeDate;      break;
            case xd::typeDateTime:      m_columns[i]->m_type = typeDateTime;  break;
            case xd::typeBoolean:       m_columns[i]->m_type = typeBoolean;   break;
        }
    }
    
    // clear the cached expressions
    std::map<wxString, CellExpression*>::iterator it_expr, it_expr_end;
    it_expr_end = m_cell_expressions.end();
    
    for (it_expr = m_cell_expressions.begin(); it_expr != it_expr_end; ++it_expr)
    {
        delete it_expr->second;
    }

    m_cell_expressions.clear();
}

