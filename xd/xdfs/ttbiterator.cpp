/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <cmath>
#include <xd/xd.h>
#include <kl/math.h>
#include <kl/portable.h>
#include "xdfs.h"
#include "database.h"
#include "ttbset.h"
#include "ttbiterator.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/util.h"


#ifdef _MSC_VER
#include <float.h>
#define isnan _isnan
#define finite _finite
#endif

const std::string empty_string = "";
const std::wstring empty_wstring = L"";

const int tableiterator_read_ahead_buffer_size = 2097152;


inline xd::rowid_t rowidCreate(xd::tableord_t table_ordinal,
                                  xd::rowpos_t row_num)
{
    xd::rowid_t r;
    r = ((xd::rowid_t)table_ordinal) << 36;
    r |= row_num;
    return r;
}

inline xd::rowpos_t rowidGetRowPos(xd::rowid_t rowid)
{
    return (rowid & 0xfffffffffLL);
}

inline xd::tableord_t rowidGetTableOrd(xd::rowid_t rowid)
{
    return (rowid >> 36);
}


static double decstr2dbl(const char* c, int width, int scale)
{
    double res = 0;
    double d = kl::pow10(width-scale-1);
    bool neg = false;
    while (width)
    {
        if (*c == '-')
            neg = true;

        if (*c >= '0' && *c <= '9')
        {
            res += d * (*c - '0');
        }

        d /= 10;
        c++;
        width--;
    }

    if (neg)
    {
        res *= -1.0;
    }

    return res;
}


TtbIterator::TtbIterator()
{
    m_row_num = 1;
    m_row_count = 0;
    m_table_rowwidth = 0;
    m_read_ahead_rowcount = 0;
    m_rowpos_buf = NULL;
    m_buf_rowcount = 0;
    m_buf_pos = 0;
    m_buf = NULL;
    m_bof = false;
    m_eof = false;
    m_set = NULL;
}

TtbIterator::~TtbIterator()
{
    std::vector<TtbDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete (*it);
    }

    if (m_file.isOpen())
    {
        m_file.close();
    }

    if (m_set)
        m_set->unref();
}

bool TtbIterator::init(xd::IDatabasePtr db,
                       TtbSet* set,
                       const std::wstring& filename)
{
    if (!m_file.open(filename))
        return false;

    m_database = db;
    m_set = set;
    m_set->ref();

    m_table_ord = 0;
    m_table_rowwidth = m_file.getRowWidth();
    m_row_count = m_file.getRowCount();

    m_read_ahead_rowcount = tableiterator_read_ahead_buffer_size/m_table_rowwidth;
    if (m_read_ahead_rowcount == 0)
        m_read_ahead_rowcount = 1;
    if (m_read_ahead_rowcount > 10000)
    {
        // some tables with a very skinny row width could have millions of
        // read ahead rows.  On files with a delete map this can lead to
        // performance degredation.  Limit read-ahead rows to 10000
        m_read_ahead_rowcount = 10000;
    }

    m_buf = new unsigned char[m_read_ahead_rowcount * m_table_rowwidth];
    m_rowpos_buf = new xd::rowpos_t[m_read_ahead_rowcount];


    refreshStructure();

    return true;
}

void TtbIterator::setTable(const std::wstring& tbl)
{
}

std::wstring TtbIterator::getTable()
{
    if (m_set)
        return L"";
    return m_set->getObjectPath();
}

xd::rowpos_t TtbIterator::getRowCount()
{
    return m_row_count;
}

xd::IDatabasePtr TtbIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr TtbIterator::clone()
{
    TtbIterator* iter = new TtbIterator;
    
    if (!iter->init(m_database, m_set, m_file.getFilename()))
    {
        return xcm::null;
    }
    
   // iter->goRow(m_current_row);
    
    return static_cast<xd::IIterator*>(iter);
}

void TtbIterator::updatePosition()
{
    m_rowptr = (m_buf + (m_buf_pos * m_table_rowwidth));
    m_rowid = rowidCreate(m_table_ord, m_rowpos_buf[m_buf_pos]);
}


unsigned int TtbIterator::getIteratorFlags()
{
    return xd::ifFastRowCount;
}


void TtbIterator::goFirst()
{
    int read_ahead_rowcount = m_read_ahead_rowcount;
    if (read_ahead_rowcount > 100)
        read_ahead_rowcount = 100;
        
    m_buf_rowcount = m_file.getRows(m_buf,
                                    m_rowpos_buf,
                                    0,
                                    1,
                                    read_ahead_rowcount,
                                    true,
                                    m_include_deleted);

    m_buf_pos = 0;
    m_row_num = 1;
    m_eof = (m_buf_rowcount == 0 ? true : false);
    m_bof = (m_buf_rowcount == 0 ? true : false);

    if (m_eof)
    {
        memset(m_buf, 0, m_table_rowwidth);
    }
     else
    {
        updatePosition();
    }
}

void TtbIterator::goLast()
{
}

void TtbIterator::skip(int delta)
{
    if (delta == 0)
        return;

    
    if (delta < 0)
    {
        if (m_bof)
            return;

        int new_pos = m_buf_pos+delta;
        m_row_num += delta;

        if (new_pos >= 0 && new_pos < m_buf_rowcount)
        {
            m_buf_pos = new_pos;
        }
         else
        {
            m_buf_rowcount = m_file.getRows(m_buf,
                                              m_rowpos_buf,
                                              delta,
                                              m_rowpos_buf[m_buf_pos],
                                              1,
                                              true,
                                              m_include_deleted);

            m_bof = (m_buf_rowcount == 0);

            if (m_bof)
            {
                m_buf_pos = 0;
                m_row_num = 0;
                m_rowpos_buf[0] = 0;
                return;
            }

            m_eof = false;
            m_buf_pos = 0;
        }

        updatePosition();
    }
     else
    {
        // skip forward
        if (m_eof)
            return;

        int new_pos = m_buf_pos+delta;
        m_row_num += delta;

        if (new_pos >= 0 && new_pos < m_buf_rowcount)
        {
            m_buf_pos = new_pos;
        }
         else
        {
            // ask table to find out the next start row for our buffer
            m_buf_rowcount = m_file.getRows(m_buf,
                                              m_rowpos_buf,
                                              delta,
                                              m_rowpos_buf[m_buf_pos],
                                              m_read_ahead_rowcount,
                                              true,
                                              m_include_deleted);

            m_buf_pos = 0;
            m_eof = (m_buf_rowcount == 0);

            if (m_eof)
            {
                m_rowpos_buf[0] = m_file.getRowCount(NULL)+1;
                m_row_num = m_rowpos_buf[0];
                return;
            }

            m_bof = false;
        }

        updatePosition();
    }

}

xd::rowid_t TtbIterator::getRowId()
{
    return m_rowid;
}

bool TtbIterator::bof()
{
    return m_bof;
}

bool TtbIterator::eof()
{
    return m_eof;
}

bool TtbIterator::seek(const unsigned char* key, int length, bool soft)
{
    // keys on table iterators indicate rowid
    if (length != sizeof(xd::rowid_t))
        return false;

    xd::rowid_t* rowid = (xd::rowid_t*)key;

    if (rowidGetTableOrd(*rowid) != m_table_ord)
        return false;

    xd::rowpos_t row = rowidGetRowPos(*rowid);

    m_buf_rowcount = m_file.getRows(m_buf,
                                      m_rowpos_buf,
                                      0,
                                      row,
                                      1,
                                      true,
                                      m_include_deleted);
    m_buf_pos = 0;
    m_eof = (m_buf_rowcount == 0 ? true : false);
    m_bof = (m_buf_rowcount == 0 ? true : false);

    if (m_eof || m_bof)
    {
        memset(m_buf, 0, m_table_rowwidth);
        return false;
    }
     else
    {
        updatePosition();
    }

    return true;
}

bool TtbIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool TtbIterator::setPos(double pct)
{
    return false;
}

double TtbIterator::getPos()
{
    double d = (double)(long long)rowidGetRowPos(m_rowid);
    double row_count = (double)(long long)m_row_count;
    if (kl::dblcompare(row_count, 0.0) == 0)
        return 0.0;

    return (d/row_count);
}

void TtbIterator::goRow(const xd::rowid_t& rowid)
{

}

xd::IStructurePtr TtbIterator::getStructure()
{
    xd::IStructurePtr s = m_file.getStructure();
    //appendCalcFields(s);
    return s;
}

void TtbIterator::refreshStructure()
{
    m_fields.clear();

    xd::IStructurePtr s = m_set->getStructure();
    int col_count = s->getColumnCount();
    int i;

    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
        
        TtbDataAccessInfo* dai = new TtbDataAccessInfo;
        dai->name = colinfo->getName();
        dai->type = colinfo->getType();
        dai->offset = colinfo->getOffset();
        dai->width = colinfo->getWidth();
        dai->scale = colinfo->getScale();
        dai->ordinal = colinfo->getColumnOrdinal();
        dai->expr_text = colinfo->getExpression();
        m_fields.push_back(dai);
        
        // parse any expression, if necessary
        if (dai->expr_text.length() > 0)
            dai->expr = parse(dai->expr_text);
    }
}

bool TtbIterator::modifyStructure(xd::IStructure* struct_config,
                                  xd::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<TtbDataAccessInfo*>::iterator it2;
    
    // -- handle delete --
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
                TtbDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // -- handle modify --
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

    // -- handle create --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            TtbDataAccessInfo* dai = new TtbDataAccessInfo;
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

    // -- handle insert --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->m_pos;
        if (insert_idx < 0 || (size_t)insert_idx >= m_fields.size())
            continue;
        
        if (it->m_params->getExpression().length() > 0)
        {
            TtbDataAccessInfo* dai = new TtbDataAccessInfo;
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


xd::objhandle_t TtbIterator::getHandle(const std::wstring& expr)
{
    std::vector<TtbDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        TtbDataAccessInfo* dai = new TtbDataAccessInfo;
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
    {
        return (xd::objhandle_t)0;
    }

    TtbDataAccessInfo* dai = new TtbDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool TtbIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<TtbDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
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

xd::IColumnInfoPtr TtbIterator::getInfo(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    ColumnInfo* colinfo = new ColumnInfo;
    colinfo->setName(dai->name);
    colinfo->setType(dai->type);
    colinfo->setWidth(dai->width);
    colinfo->setScale(dai->scale);
    colinfo->setExpression(dai->expr_text);
    
    if (dai->type == xd::typeDate ||
        dai->type == xd::typeInteger)
    {
        colinfo->setWidth(4);
    }
     else if (dai->type == xd::typeDateTime ||
              dai->type == xd::typeDouble)
    {
        colinfo->setWidth(8);
    }
     else if (dai->type == xd::typeBoolean)
    {
        colinfo->setWidth(1);
    }
     else
    {
        colinfo->setWidth(dai->width);
    }
    
    if (dai->expr_text.length() > 0)
        colinfo->setCalculated(true);

    return static_cast<xd::IColumnInfo*>(colinfo);
}

int TtbIterator::getType(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    return dai->type;
}

int TtbIterator::getRawWidth(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* TtbIterator::getRawPtr(xd::objhandle_t data_handle)
{
/*
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0;

    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return m_file.getRaw(dai->ordinal);
*/
    return NULL;
}

const std::string& TtbIterator::getString(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    if (dai == NULL)
        return empty_string;
    if (!m_rowptr)
        return empty_string;


    if (dai->isCalculated())
    {
        if (!dai->expr)
            return empty_string;

        if (!dai->expr->eval(&dai->expr_result) ||
             (dai->expr->getType() != kscript::Value::typeString &&
              dai->expr->getType() != kscript::Value::typeUndefined)
           )
        {
            // upon failure, return an empty string
            return empty_string;
        }

        // crop it to calcfield's size
        wchar_t* result_str = dai->expr_result.getString();
        int len = wcslen(result_str);
        if (dai->isColumn() && len > dai->width)
        {
            len = dai->width;
        }

        // convert to an ASCII string
        dai->str_result.resize(len);

        int i;
        wchar_t ch;
        for (i = 0; i < len; ++i)
        {
            ch = result_str[i];
            if (ch > 255)
                ch = '?';

            dai->str_result[i] = (char)(unsigned char)ch;
        }

        return dai->str_result;

    }
    

    if (dai->type == xd::typeCharacter)
    {
        // return field data
        const char* ptr = (char*)(m_rowptr + dai->offset);

        int width = dai->width;
        while (width && *(ptr+width-1) == ' ')
            width--;
            
        // look for a zero terminator
        const char* zero = (const char*)memchr(ptr, 0, width);
        if (zero)
            width = zero-ptr;
            
        dai->str_result.assign(ptr, width);

        return dai->str_result;
    }
     else if (dai->type == xd::typeWideCharacter)
    {
        kl::ucsle2string(dai->str_result,
                            m_rowptr + dai->offset,
                            dai->width);
        return dai->str_result;
    }
     else
    {
        return empty_string;
    }
}

const std::wstring& TtbIterator::getWideString(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    //if (!dai->is_active)
    //    return empty_wstring;
    if (!m_rowptr)
        return empty_wstring;

    if (dai->isCalculated())
    {
        if (!dai->expr->eval(&dai->expr_result) ||
             (dai->expr->getType() != kscript::Value::typeString &&
              dai->expr->getType() != kscript::Value::typeUndefined)
           )
        {
            // upon failure, return an empty string
            return empty_wstring;
        }


        // crop it to calcfield's size
        wchar_t* result_str = dai->expr_result.getString();
        int len = wcslen(result_str);
        if (dai->isColumn() && len > dai->width)
        {
            len = dai->width;
        }

        dai->wstr_result.assign(result_str, len);

        return dai->wstr_result;
    }



    // return field data
    if (dai->type == xd::typeWideCharacter)
    {
        kl::ucsle2wstring(dai->wstr_result,
                            m_rowptr + dai->offset,
                            dai->width);
        return dai->wstr_result;
    }
        else if (dai->type == xd::typeCharacter)
    {
        const char* ptr = (char*)(m_rowptr + dai->offset);
        int width = dai->width;
        while (width && *(ptr+width-1) == ' ')
            width--;
                
        // look for a zero terminator
        const char* zero = (const char*)memchr(ptr, 0, width);
        if (zero)
            width = zero-ptr;

        kl::towstring(dai->wstr_result, ptr, width);
        return dai->wstr_result;
    } 
        else
    {
        return empty_wstring;
    }
}

xd::datetime_t TtbIterator::getDateTime(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    //if (!dai->is_active)
    //    return empty_wstring;
    if (!m_rowptr)
    {
        xd::DateTime dt;
        return dt;
    }


    if (dai->isCalculated())
    {
        if (!dai->expr->eval(&dai->expr_result) ||
             dai->expr->getType() != kscript::Value::typeDateTime)
        {
            // upon failure, return an empty date
            xd::DateTime dt;
            return dt;
        }

        kscript::ExprDateTime edt = dai->expr_result.getDateTime();
        xd::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        
        if (dai->type == xd::typeDateTime)
            dt |= edt.time;

        return dt;
    }


    if (dai->type == xd::typeDate)
    {
        xd::datetime_t dt = buf2int(m_rowptr+dai->offset);
        dt <<= 32;

        return dt;
    }
        else if (dai->type == xd::typeDateTime)
    {
        xd::datetime_t dt = buf2int(m_rowptr+dai->offset);
        xd::datetime_t ts = buf2int(m_rowptr+dai->offset+4);

        dt <<= 32;
        dt |= ts;

        return dt;
    }

    return 0;
}

double TtbIterator::getDouble(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    //if (!dai->is_active)
    //    return empty_wstring;
    if (!m_rowptr)
        return 0.0;

    if (dai->type == xd::typeInteger)
    {
        // implicit conversion from integer -> double
        return getInteger(data_handle);
    }
     else if (dai->type == xd::typeCharacter ||
              dai->type == xd::typeWideCharacter)
    {
        return kl::nolocale_atof(getString(data_handle).c_str());
    }


    if (dai->isCalculated())
    {
        if (!dai->expr->eval(&dai->expr_result) ||
             (dai->expr->getType() != kscript::Value::typeDouble &&
              dai->expr->getType() != kscript::Value::typeInteger &&
              dai->expr->getType() != kscript::Value::typeUndefined)
           )
        {
            return 0.0;
        }

        double d = dai->expr_result.getDouble();

        if (isnan(d) || !finite(d))
            d = 0.0;

        if (dai->scale == 255)
            return d;
             else
            return kl::dblround(d, dai->scale);
    }


    if (dai->type == xd::typeNumeric)
    {
        return kl::dblround(
                decstr2dbl((char*)m_rowptr + dai->offset, dai->width, dai->scale),
                dai->scale);
    }
     else if (dai->type == xd::typeDouble)
    {
        // FIXME: this will only work on little-endian (intel) processors
        double d;
        memcpy(&d, m_rowptr + dai->offset, sizeof(double));
        return kl::dblround(d, dai->scale);
    }
     else
    {
        return 0.0;
    }


    return 0;
}

int TtbIterator::getInteger(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    if (!dai)
        return false;
    if (!m_rowptr)
        return 0;

    if (dai->type == xd::typeDouble || dai->type == xd::typeNumeric)
    {
        // implicit conversion from numeric/double -> integer
        double d = getDouble(data_handle);
        return (int)kl::dblround(d, 0);
    }
     else if (dai->type == xd::typeCharacter || dai->type == xd::typeWideCharacter)
    {
        return atoi(getString(data_handle).c_str());
    }

    if (dai->isCalculated())
    {
        if (!dai->expr->eval(&dai->expr_result) ||
             (dai->expr->getType() != kscript::Value::typeDouble &&
              dai->expr->getType() != kscript::Value::typeInteger))
        {
            return 0;
        }

        return dai->expr_result.getInteger();
    }


    unsigned char* ptr = m_rowptr+dai->offset;
    unsigned int retval;

    retval = *(ptr) +
                (*(ptr+1) << 8) +
                (*(ptr+2) << 16) +
                (*(ptr+3) << 24);

    return (signed int)retval;
}


bool TtbIterator::getBoolean(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    if (!dai)
        return false;
    if (!m_rowptr)
        return false;

    if (dai->isCalculated())
    {
        if (!dai->expr->eval(&dai->expr_result) ||
             dai->expr->getType() != kscript::Value::typeBoolean)
        {
            return false;
        }

        return dai->expr_result.getBoolean();

    }

    return (*(m_rowptr+dai->offset) == 'T' ? true : false);
}


bool TtbIterator::isNull(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    //if (!dai->is_active)
    //    return true;

    if (!m_rowptr)
        return true;

    if (dai->isCalculated())
    {
        if (!dai->expr->eval(&dai->expr_result))
            return true;

        return dai->expr_result.isNull();
    }
     else
    {
        // check null
        if (dai->nulls_allowed)
        {
            // remove the null bit, if any
            *(m_rowptr + dai->offset - 1) &= 0xfe;
        }
         else
        {
            return false;
        }
    }

    return false;
}





