/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-29
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "database.h"
#include "delimitedtextset.h"
#include "delimitedtextiterator.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
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

DelimitedTextIterator::DelimitedTextIterator()
{
    m_use_source_iterator = false;
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
}

bool DelimitedTextIterator::init(tango::IDatabasePtr db,
                                 DelimitedTextSet* set,
                                 const std::wstring& filename)
{
    if (!m_file.openFile(filename))
        return false;

    m_database = db;
    m_set = set;
    m_set->ref();
        
    m_file.setDelimiters(m_set->m_file.getDelimiters());
    m_file.setLineDelimiters(m_set->m_file.getLineDelimiters());
    m_file.setTextQualifiers(m_set->m_file.getTextQualifiers());

    if (m_set->isFirstRowColumnNames())
    {
        // read past the first row
        m_file.skip(1);
    }    
    
    refreshStructure();
    return true;
}

void DelimitedTextIterator::setUseSourceIterator(bool use_source_iterator)
{
    m_use_source_iterator = use_source_iterator;
}

tango::ISetPtr DelimitedTextIterator::getSet()
{
    return static_cast<tango::ISet*>(m_set);
}

tango::rowpos_t DelimitedTextIterator::getRowCount()
{
    return 0;
}

tango::IDatabasePtr DelimitedTextIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr DelimitedTextIterator::clone()
{
    DelimitedTextIterator* iter = new DelimitedTextIterator;
    
    if (!iter->init(m_database, m_set, m_file.getFilename()))
    {
        return xcm::null;
    }
    
    tango::rowid_t rowid = m_file.getRowOffset();
    iter->goRow(rowid);
    
    return static_cast<tango::IIterator*>(iter);
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

    if (m_set->m_first_row_column_names)
    {
        // read past the first row
        m_file.skip(1);
    }
}

void DelimitedTextIterator::goLast()
{

}

tango::rowid_t DelimitedTextIterator::getRowId()
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
    return 0.0;
}

void DelimitedTextIterator::goRow(const tango::rowid_t& rowid)
{
    m_file.goOffset(rowid);
}

tango::IStructurePtr DelimitedTextIterator::getStructure()
{
    tango::IStructurePtr s = static_cast<tango::IStructure*>(new Structure);
    IStructureInternalPtr struct_int = s;
    
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        struct_int->addColumn(col);
        
        col->setName((*it)->name);
        col->setType((*it)->type);
        col->setWidth((*it)->width);
        col->setScale((*it)->scale);
        col->setColumnOrdinal((*it)->ordinal);
        col->setExpression((*it)->expr_text);
        
        if (col->getExpression().length() > 0)
            col->setCalculated(true);
    }

    return s;
}

void DelimitedTextIterator::refreshStructure()
{
    // clear out any existing structure
    std::vector<DelimitedTextDataAccessInfo*>::iterator field_it;
    for (field_it = m_fields.begin();
         field_it != m_fields.end(); ++field_it)
    {
        delete (*field_it);
    }
    m_fields.clear();


    tango::IStructurePtr source_structure = m_set->getSourceStructure();
    tango::IStructurePtr destination_structure = m_set->getDestinationStructure();
    tango::IStructurePtr final_structure = m_set->getStructure();
    
    // if m_use_source_iterator is true, populate the DAI from the set's source
    // structure, otherwise, populate the DAI from the set's structure
    if (m_use_source_iterator)
    {
        int i, col_count = source_structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = source_structure->getColumnInfoByIdx(i);
            
            DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
            dai->name = colinfo->getName();
            dai->type = colinfo->getType();
            dai->width = colinfo->getWidth();
            dai->scale = colinfo->getScale();
            dai->ordinal = colinfo->getColumnOrdinal();
            m_fields.push_back(dai);
        }
    }
     else
    {
        int i, col_count = final_structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = final_structure->getColumnInfoByIdx(i);
            
            DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
            dai->name = colinfo->getName();
            dai->type = colinfo->getType();
            dai->width = colinfo->getWidth();
            dai->scale = colinfo->getScale();
            dai->ordinal = colinfo->getColumnOrdinal();
            dai->expr_text = colinfo->getExpression();
            
            if (dai->expr_text.length() > 0)
            {
                // this is a calculated field
                dai->expr = parseDestinationExpression(dai->expr_text);
            }
             else
            {
                // this is not a calculated field, lookup and
                // parse the destination structure's expression
                tango::IColumnInfoPtr dest_colinfo;
                dest_colinfo = destination_structure->getColumnInfoByIdx(i);
                if (dest_colinfo.isNull())
                {
                    #ifdef _DEBUG
                    int* p = NULL;
                    *p = 1; // crash, debug break
                    #endif
                    
                    continue;
                }
                
                std::wstring dest_expr_text = dest_colinfo->getExpression();
                if (dest_expr_text.length() > 0)
                    dai->expr = parseSourceExpression(dest_expr_text);
            }
            
            m_fields.push_back(dai);
        }
    }
}

bool DelimitedTextIterator::modifyStructure(tango::IStructure* struct_config,
                                            tango::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<DelimitedTextDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                DelimitedTextDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                if (it->m_params->getName().length() > 0)
                {
                    std::wstring new_name = it->m_params->getName();
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->m_params->getType() != -1)
                {
                    (*it2)->type = it->m_params->getType();
                }

                if (it->m_params->getWidth() != -1)
                {
                    (*it2)->width = it->m_params->getWidth();
                }

                if (it->m_params->getScale() != -1)
                {
                    (*it2)->scale = it->m_params->getScale();
                }

                if (it->m_params->getExpression().length() > 0)
                {
                    if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->m_params->getExpression();
                    (*it2)->expr = parse(it->m_params->getExpression());
                }
            }
        }
    }

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->type = it->m_params->getType();
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            m_fields.push_back(dai);
        }
    }

    // handle insert
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->m_pos;
        if (insert_idx < 0 || insert_idx >= (int)m_fields.size())
            continue;
        
        if (it->m_params->getExpression().length() > 0)
        {
            DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->type = it->m_params->getType();
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            m_fields.insert(m_fields.begin()+insert_idx, dai);
        }
    }
    
    return true;
}



static void _bindSourceFieldString(kscript::ExprEnv*,
                                   void* param,
                                   kscript::Value* retval)
{
    DelimitedSourceBindInfo* info = (DelimitedSourceBindInfo*)param;
    const std::wstring& s = info->file->getString(info->column_idx);
    retval->setString(s);
}

kscript::ExprParser* DelimitedTextIterator::parseSourceExpression(const std::wstring& expr)
{
    // parseSourceExpr() is used for parsing expressions which
    // use the source structure as input fields (raw input file);
    // for example, transformation expressions would use
    // this function for parsing
    
    kscript::ExprParser* parser = createExprParser();

    tango::IStructurePtr structure = m_set->getSourceStructure();
    if (!structure)
        return NULL;
        
    int i, col_count = structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);
       
        DelimitedSourceBindInfo* info = new DelimitedSourceBindInfo;
        info->file = &m_file;
        info->column_idx = (size_t)colinfo->getColumnOrdinal();
        m_src_bindings.push_back(info);

        parser->addVarBinding(colinfo->getName(),
                              false,
                              kscript::Value::typeString,
                              (void*)_bindSourceFieldString,
                              info);
    }

    if (!parser->parse(expr))
    {
        delete parser;
        return NULL;
    }

    return parser;
}

kscript::ExprParser* DelimitedTextIterator::parseDestinationExpression(const std::wstring& expr)
{
    // parseDestinationExpr() is used for parsing expressions which
    // use the destination (transformed) structure as input fields;
    // for example, calculated fields would use this function for parsing
    
    // use CommonBaseIterator::parse()
    return parse(expr);
}



tango::objhandle_t DelimitedTextIterator::getHandle(const std::wstring& expr)
{
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
        dai->expr = NULL;
        dai->expr_text = expr;
        dai->type = tango::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<tango::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }

        m_exprs.push_back(dai);
        return (tango::objhandle_t)dai;
    }
    

    kscript::ExprParser* parser = parse(expr);
    if (!parser)
        return (tango::objhandle_t)0;

    DelimitedTextDataAccessInfo* dai = new DelimitedTextDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool DelimitedTextIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
            return true;
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

tango::IColumnInfoPtr DelimitedTextIterator::getInfo(tango::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xcm::null;

    ColumnInfo* colinfo = new ColumnInfo;
    colinfo->setName(dai->name);
    colinfo->setType(dai->type);
    colinfo->setWidth(dai->width);
    colinfo->setScale(dai->scale);
    colinfo->setExpression(dai->expr_text);

    if (dai->type == tango::typeDate ||
        dai->type == tango::typeInteger)
    {
        colinfo->setWidth(4);
    }
     else if (dai->type == tango::typeDateTime ||
              dai->type == tango::typeDouble)
    {
        colinfo->setWidth(8);
    }
     else if (dai->type == tango::typeBoolean)
    {
        colinfo->setWidth(1);
    }
     else
    {
        colinfo->setWidth(dai->width);
    }
    
    if (dai->expr_text.length() > 0)
        colinfo->setCalculated(true);

    return static_cast<tango::IColumnInfo*>(colinfo);
}

int DelimitedTextIterator::getType(tango::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0;

    return dai->type;
}

int DelimitedTextIterator::getRawWidth(tango::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
        return dai->key_layout->getKeyLength();
    
    return 0;
}

const unsigned char* DelimitedTextIterator::getRawPtr(tango::objhandle_t data_handle)
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

const std::string& DelimitedTextIterator::getString(tango::objhandle_t data_handle)
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

const std::wstring& DelimitedTextIterator::getWideString(tango::objhandle_t data_handle)
{
    DelimitedTextDataAccessInfo* dai = (DelimitedTextDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

    if (dai->type == tango::typeCharacter)
    {
        dai->wstr_result = kl::towstring(getString(data_handle));
        return dai->wstr_result;
    }
     else if (dai->type == tango::typeWideCharacter)
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



inline tango::datetime_t implicitStringToDateTime(const wchar_t* str)
{
    int y,m,d;
    
    if (!parseDelimitedStringDate(str, &y, &m, &d))
        return 0;
    
    tango::datetime_t dt;
    dt = dateToJulian(y, m, d);
    dt <<= 32;
    return dt;
}


tango::datetime_t DelimitedTextIterator::getDateTime(tango::objhandle_t data_handle)
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
            tango::datetime_t dt;
            dt = edt.date;
            dt <<= 32;
            if (dai->type == tango::typeDateTime)
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
    return kl::nolocale_wtof(str);
}

double DelimitedTextIterator::getDouble(tango::objhandle_t data_handle)
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

int DelimitedTextIterator::getInteger(tango::objhandle_t data_handle)
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

bool DelimitedTextIterator::getBoolean(tango::objhandle_t data_handle)
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

bool DelimitedTextIterator::isNull(tango::objhandle_t data_handle)
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

    return false;
}



