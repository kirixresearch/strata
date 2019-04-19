/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-29
 *
 */


#include "xdfs.h"
#include "database.h"
#include "delimitedtextset.h"
#include "delimitedtextiterator.h"
#include "../xdcommon/util.h"


static std::string empty_string = "";
static std::wstring empty_wstring = L"";


// this class is used for binding expressions
// which use source fields

class DelimitedSourceBindInfo
{
public:
    DelimitedTextFile* file;
    size_t column_idx;
};




// -- DelimitedTextIterator class implementation --

DelimitedTextIterator::DelimitedTextIterator(FsDatabase* db)
{
    m_database = db;
    m_database->ref();
    m_set = NULL;
}

DelimitedTextIterator::~DelimitedTextIterator()
{
    std::vector<DelimitedTextDataAccessInfo*>::iterator field_it;
    for (field_it = m_fields.begin();
         field_it != m_fields.end(); ++field_it)
    {
        delete (*field_it);
    }
    
    std::vector<DelimitedSourceBindInfo*>::iterator src_bindings_it;
    for (src_bindings_it = m_src_bindings.begin();
         src_bindings_it != m_src_bindings.end(); ++src_bindings_it)
    {
        delete (*src_bindings_it);
    }

    if (m_file.isOpen())
    {
        m_file.closeFile();
    }

    // if we've ref'd the pointer, we need to unref it
    if (m_set)
    {
        m_set->unref();
        m_set = NULL;
    }

    m_database->unref();
}

bool DelimitedTextIterator::init(DelimitedTextSet* set, const std::wstring& columns)
{
    if (!m_file.open(set->m_file_url))
        return false;

    m_set = set;
    m_set->ref();
    m_columns = columns;
        
    m_file.setDelimiters(m_set->m_def.delimiter);
    m_file.setLineDelimiters(m_set->m_def.line_delimiter);
    m_file.setTextQualifiers(m_set->m_def.text_qualifier);

    if (m_set->m_def.header_row)
    {
        // read past the first row
        m_file.skip(1);
    }    
    
    return refreshStructure();
}



void DelimitedTextIterator::setTable(const std::wstring& tbl)
{
}

std::wstring DelimitedTextIterator::getTable()
{
    if (m_set)
        return L"";
    return m_set->getObjectPath();
}

xd::rowpos_t DelimitedTextIterator::getRowCount()
{
    return 0;
}

xd::IIteratorPtr DelimitedTextIterator::clone()
{
    DelimitedTextIterator* iter = new DelimitedTextIterator(m_database);
    
    if (!iter->init(m_set, m_set->m_file_url))
        return xcm::null;
    
    xd::rowid_t rowid = m_file.getRowOffset();
    iter->goRow(rowid);
    
    return static_cast<xd::IIterator*>(iter);
}

unsigned int DelimitedTextIterator::getIteratorFlags()
{
    return 0;
}

void DelimitedTextIterator::skip(int delta)
{
    m_file.skip(delta);
}

void DelimitedTextIterator::goFirst()
{
    m_file.rewind();

    if (m_set->m_def.header_row)
    {
        // read past the first row
        m_file.skip(1);
    }
}

void DelimitedTextIterator::goLast()
{

}

xd::rowid_t DelimitedTextIterator::getRowId()
{
    return m_file.getRowOffset();
}

bool DelimitedTextIterator::bof()
{
    return m_file.bof();
}

bool DelimitedTextIterator::eof()
{
    return m_file.eof();
}

bool DelimitedTextIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool DelimitedTextIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool DelimitedTextIterator::setPos(double pct)
{
    return false;
}

double DelimitedTextIterator::getPos()
{
    return m_file.getPos();
}

void DelimitedTextIterator::goRow(const xd::rowid_t& rowid)
{
    m_file.goOffset(rowid);
}

xd::Structure DelimitedTextIterator::getStructure()
{
    xd::Structure s;
    
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        xd::ColumnInfo col;

        col.name = (*it)->name;
        col.type = (*it)->type;
        col.width = (*it)->width;
        col.scale = (*it)->scale;
        col.column_ordinal = (*it)->ordinal;
        col.expression = (*it)->expr_text;
        
        if (col.expression.length() > 0)
            col.calculated = true;

        s.createColumn(col);
    }

    return s;
}

bool DelimitedTextIterator::refreshStructure()
{
    m_fields.clear();

    xd::Structure set_structure = m_set->getStructureWithTransformations();

    // add fields from structure
    bool default_structure_visible = false;
    if (m_columns.empty() || m_columns == L"*")
        default_structure_visible = true;


    size_t i, col_count = set_structure.getColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& colinfo = set_structure.getColumnInfoByIdx(i);
        
        DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
        dai->name = colinfo.name;
        dai->type = colinfo.type;
        dai->width = colinfo.width;
        dai->scale = colinfo.scale;
        dai->ordinal = colinfo.column_ordinal;
        dai->expr_text = colinfo.expression;
        m_fields.push_back(dai);
        
        // parse any expression, if necessary
        if (dai->expr_text.length() > 0)
            dai->expr = parse(dai->expr_text);
    }


    if (m_columns.length() > 0 && m_columns != L"*")
    {
        std::vector<std::wstring> colvec;
        std::vector<std::wstring>::iterator it;

        kl::parseDelimitedList(m_columns, colvec, L',', true);

        for (it = colvec.begin(); it != colvec.end(); ++it)
        {
            std::wstring& part = *it;
            kl::trim(part);

            xd::ColumnInfo colinfo = set_structure.getColumnInfo(part);

            if (colinfo.isNull() && part[0] == '[')
            {
                // maybe the above just needs to be dequoted
                std::wstring dequote_part = part;
                dequote(dequote_part, '[', ']');
                colinfo = set_structure.getColumnInfo(dequote_part);
            }

            if (colinfo.isOk())
            {
                DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
                dai->name = colinfo.name;
                dai->type = colinfo.type;
                dai->width = colinfo.width;
                dai->scale = colinfo.scale;
                dai->ordinal = colinfo.column_ordinal;
                dai->expr_text = colinfo.expression;
                m_fields.push_back(dai);
        
                // parse any expression, if necessary
                if (dai->expr_text.length() > 0)
                    dai->expr = parse(dai->expr_text);
                continue;
            }
             else
            {
                // string is not a column name, so look for 'AS' keyword
                wchar_t* as_ptr = zl_stristr((wchar_t*)it->c_str(),
                                            L"AS",
                                            true,
                                            false);
                std::wstring colname;
                std::wstring expr;

                if (as_ptr)
                {
                    int as_pos = as_ptr ? (as_ptr - it->c_str()) : -1;
                    colname = it->substr(as_pos+2);
                    expr = it->substr(0, as_pos);
                    
                    kl::trim(colname);
                    kl::trim(expr);

                    dequote(colname, '[', ']');
                }
                 else
                {
                    expr = *it;
                    
                    wchar_t buf[32];
                    int colname_counter = 0;
                    do
                    {
                        swprintf(buf, 32, L"EXPR%03d", ++colname_counter);
                    } while (set_structure.getColumnExist(buf));

                    colname = buf;
                }


                std::wstring dequote_expr = expr;
                dequote(dequote_expr, '[', ']');


                // see if the expression is just a column and use its precise type info if it is
                const xd::ColumnInfo& colinfo = set_structure.getColumnInfo(dequote_expr);
                if (colinfo.isOk())
                {
                    DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
                    dai->name = colname;
                    dai->type = colinfo.type;
                    dai->width = colinfo.width;
                    dai->scale = colinfo.scale;
                    dai->ordinal = colinfo.column_ordinal;
                    dai->expr_text = colinfo.expression;
                    m_fields.push_back(dai);
        
                    // parse any expression, if necessary
                    if (dai->expr_text.length() > 0)
                        dai->expr = parse(dai->expr_text);
                    continue;
                }


                kscript::ExprParser* p = parse(expr);
                if (!p)
                    return false;

                int xd_type = kscript2xdType(p->getType());
                if (xd_type == xd::typeInvalid || xd_type == xd::typeUndefined)
                {
                    delete p;
                    return false;
                }

                int width;
                int scale = 0;

                switch (xd_type)
                {
                    case xd::typeNumeric:
                        width = 18;
                        scale = 2;
                        break;
                    case xd::typeDouble:
                        width = 8;
                        scale = 2;
                        break;
                    case xd::typeDate:
                    case xd::typeInteger:
                        width = 4;
                        break;
                    case xd::typeDateTime:
                        width = 8;
                        break;
                    default:
                        width = 254;
                        break;
                }

                DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
                dai->name = colname;
                dai->type = xd_type;
                dai->width = width;
                dai->scale = scale;
                dai->ordinal = 0;
                dai->expr_text = expr;
                dai->expr = p;
                m_fields.push_back(dai);
            }
        }
    }

    return true;
}

bool DelimitedTextIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<DelimitedTextDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                DelimitedTextDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionModify)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                if (it->params.mask & xd::ColumnInfo::maskName)
                {
                    std::wstring new_name = it->params.name;
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->params.mask & xd::ColumnInfo::maskType)
                {
                    (*it2)->type = it->params.type;
                }

                if (it->params.mask & xd::ColumnInfo::maskWidth)
                {
                    (*it2)->width = it->params.width;
                }

                if (it->params.mask & xd::ColumnInfo::maskScale)
                {
                    (*it2)->scale = it->params.scale;
                }

                if (it->params.mask & xd::ColumnInfo::maskExpression)
                {
                    if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->params.expression;
                    (*it2)->expr = parse(it->params.expression);
                }
            }
        }
    }

    // handle create
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionCreate)
            continue;

        if (it->params.expression.length() > 0)
        {
            DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
            dai->name = it->params.name;
            dai->type = it->params.type;
            dai->width = it->params.width;
            dai->scale = it->params.scale;
            dai->ordinal = m_fields.size();
            dai->expr_text = it->params.expression;
            dai->expr = parse(it->params.expression);
            m_fields.push_back(dai);
        }
    }

    // handle insert
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->params.column_ordinal;
        if (insert_idx < 0 || insert_idx >= (int)m_fields.size())
            continue;
        
        if (it->params.expression.length() > 0)
        {
            DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
            dai->name = it->params.name;
            dai->type = it->params.type;
            dai->width = it->params.width;
            dai->scale = it->params.scale;
            dai->ordinal = m_fields.size();
            dai->expr_text = it->params.expression;
            dai->expr = parse(it->params.expression);
            m_fields.insert(m_fields.begin()+insert_idx, dai);
        }
    }
    
    return true;
}


void func_rawvalue(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setString(((DelimitedTextIterator*)param)->m_file.getString(env->m_eval_params[0]->getInteger() - 1));
}

void DelimitedTextIterator::onParserInit(kscript::ExprParser* parser)
{
    parser->addFunction(L"rawvalue", false, func_rawvalue, false, L"s(i)", this);
}


xd::objhandle_t DelimitedTextIterator::getHandle(const std::wstring& expr)
{
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, expr))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
        dai->expr = NULL;
        dai->expr_text = expr;
        dai->type = xd::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<xd::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }

        m_exprs.push_back(dai);
        return (xd::objhandle_t)dai;
    }
    

    kscript::ExprParser* parser = parse(expr);
    if (!parser)
        return (xd::objhandle_t)0;

    DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool DelimitedTextIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
            return true;
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

xd::ColumnInfo DelimitedTextIterator::getInfo(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::ColumnInfo();

    xd::ColumnInfo colinfo;
    colinfo.name = dai->name;
    colinfo.type = dai->type;
    colinfo.width = dai->width;
    colinfo.scale = dai->scale;
    colinfo.expression = dai->expr_text;

    if (dai->type == xd::typeDate ||
        dai->type == xd::typeInteger)
    {
        colinfo.width = 4;
    }
     else if (dai->type == xd::typeDateTime ||
              dai->type == xd::typeDouble)
    {
        colinfo.width = 8;
    }
     else if (dai->type == xd::typeBoolean)
    {
        colinfo.width = 1;
    }
     else
    {
        colinfo.width = dai->width;
    }
    
    if (dai->expr_text.length() > 0)
        colinfo.calculated = true;

    return colinfo;
}

int DelimitedTextIterator::getType(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;

    return dai->type;
}

int DelimitedTextIterator::getRawWidth(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
        return dai->key_layout->getKeyLength();
    
    return 0;
}

const unsigned char* DelimitedTextIterator::getRawPtr(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0;
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return 0;
}

const std::string& DelimitedTextIterator::getString(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_string;
    }

    // the expression is the same as the field name
    // so just return the proper cell from the file
    if (dai->expr_text == dai->name)
    {
        dai->str_result = kl::tostring(m_file.getString(dai->ordinal));
        return dai->str_result;
    }
        
    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->str_result = kl::tostring(dai->expr_result.getString());
        return dai->str_result;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return empty_string;
    }

    dai->str_result = kl::tostring(m_file.getString(dai->ordinal));
    return dai->str_result;
}

const std::wstring& DelimitedTextIterator::getWideString(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

    if (dai->type == xd::typeCharacter)
    {
        dai->wstr_result = kl::towstring(getString(data_handle));
        return dai->wstr_result;
    }
     else if (dai->type == xd::typeWideCharacter)
    {
        if (dai->expr)
        {
            dai->expr->eval(&dai->expr_result);
            dai->wstr_result = dai->expr_result.getString();
            return dai->wstr_result;
        }

        if (dai->isCalculated())
        {
            // calculated field with bad expr
            return empty_wstring;
        }

        dai->wstr_result = m_file.getString(dai->ordinal);
        return dai->wstr_result;
    }

    return empty_wstring;
}



inline xd::datetime_t implicitStringToDateTime(const wchar_t* str)
{
    int y,m,d;
    
    if (!parseDelimitedStringDate(str, &y, &m, &d))
        return 0;
    
    xd::datetime_t dt;
    dt = dateToJulian(y, m, d);
    dt <<= 32;
    return dt;
}


xd::datetime_t DelimitedTextIterator::getDateTime(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0;

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        if (dai->expr_result.isDateTime())
        {
            kscript::ExprDateTime edt = dai->expr_result.getDateTime();
            xd::datetime_t dt;
            dt = edt.date;
            dt <<= 32;
            if (dai->type == xd::typeDateTime)
                dt |= edt.time;
            return dt;
        }
        
        // expr was some other type, do an implicit conversion
        return implicitStringToDateTime(dai->expr_result.getString());
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    return implicitStringToDateTime(m_file.getString(dai->ordinal).c_str());
}



inline double implicitStringToDouble(const wchar_t* str)
{
    while (::iswspace(*str))
        ++str;
    while (*str == '$' || *str == (wchar_t)0xa3 /* pound sign */ || *str == (wchar_t)0x20ac /* euro sign */)
        ++str;
    return kl::nolocale_wtof(str);
}

double DelimitedTextIterator::getDouble(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0.0;

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        if (dai->expr_result.isNumber())
            return dai->expr_result.getDouble();
        
        // expr was some other type, do an implicit conversion
        return implicitStringToDouble(dai->expr_result.getString());
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0.0;
    }

    return implicitStringToDouble(m_file.getString(dai->ordinal).c_str());
}




inline int implicitStringToInteger(const wchar_t* str)
{
    return kl::wtoi(str);
}

int DelimitedTextIterator::getInteger(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0;

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        if (dai->expr_result.isNumber())
            return dai->expr_result.getInteger();
        
        // expr was some other type, do an implicit conversion
        return implicitStringToInteger(dai->expr_result.getString());
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    return implicitStringToInteger(m_file.getString(dai->ordinal).c_str());
}




inline bool implicitStringToBool(const wchar_t* str)
{
    wchar_t ch = towupper(*str);
    return (ch == '1' || ch == 'T' || ch == 'Y') ? true : false;
}

bool DelimitedTextIterator::getBoolean(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return false;

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        if (dai->expr_result.isBoolean())
            return dai->expr_result.getBoolean();
        if (dai->expr_result.isNumber())
            return kl::dblcompare(0.0, dai->expr_result.getDouble()) ? false : true;
            
        // expr was some other type, do an implicit conversion
        return implicitStringToBool(dai->expr_result.getString());
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return false;
    }

    return implicitStringToBool(m_file.getString(dai->ordinal).c_str());
}

bool DelimitedTextIterator::isNull(xd::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return false;

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.isNull();
    }
            
    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return true;
    }


    const std::wstring& str = m_file.getString(dai->ordinal);

    if (dai->type == xd::typeDate || dai->type == xd::typeDateTime || dai->type == xd::typeNumeric)
    {
        if (str == L"null" || str == L"\\N" || str == L"")
            return true;
    }
     else
    {
        if (str == L"\\N")
            return true;
    }

    return false;
}



