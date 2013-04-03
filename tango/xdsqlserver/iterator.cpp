/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-03-17
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <tds.h>
#include <ctpublic.h>
#include <tdsconvert.h>


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"


int tds2tangoType(TDSCOLUMN* tds_col)
{
    switch (tds_col->column_type)
    {
        case XSYBCHAR:
        case SYBCHAR:
            if (tds_col->on_server.column_type == XSYBNCHAR)
            {
                return tango::typeWideCharacter;
            }
            return tango::typeCharacter;

        case XSYBVARCHAR:
        case SYBVARCHAR:
            if (tds_col->on_server.column_type == SYBNVARCHAR ||
                tds_col->on_server.column_type == XSYBNVARCHAR)
            {
                return tango::typeWideCharacter;
            }

            return tango::typeCharacter;

        case SYBTEXT:
            if (tds_col->on_server.column_type == SYBNTEXT)
            {
                return tango::typeWideCharacter;
            }
            return tango::typeCharacter;

        case XSYBNCHAR:
            return tango::typeWideCharacter;

        case XSYBNVARCHAR:
        case SYBNVARCHAR:
            return tango::typeWideCharacter;

        case SYBNTEXT:
            return tango::typeWideCharacter;

        case SYBINT1:
            return tango::typeInteger;
        case SYBINT2:
            return tango::typeInteger;
        case SYBINT4:
            return tango::typeInteger;
        case SYBINTN:
            return tango::typeInteger;
        case SYBINT8:
            return tango::typeInteger;


        case SYBFLTN:
            return tango::typeDouble;
        case SYBFLT8:
            return tango::typeDouble;
        case SYBREAL:
            return tango::typeDouble;


        case SYBDECIMAL:
            return tango::typeNumeric;
        case SYBNUMERIC:
            return tango::typeNumeric;
        case SYBMONEYN:
            return tango::typeNumeric;
        case SYBMONEY:
            return tango::typeNumeric;
        case SYBMONEY4:
            return tango::typeNumeric;


        case SYBDATETIMN:
            return tango::typeDate;
        case SYBDATETIME:
            return tango::typeDate;
        case SYBDATETIME4:
            return tango::typeDate;
/*
        case SYBDATETIMN:
            return tango::typeDateTime;
        case SYBDATETIME:
            return tango::typeDateTime;
        case SYBDATETIME4:
            return tango::typeDateTime;
*/

        case SYBBITN:
            return tango::typeBoolean;
        case SYBBIT:
            return tango::typeBoolean;


        case SYBIMAGE:
            return tango::typeInvalid;
        case SYBBINARY:
            return tango::typeInvalid;
        case SYBVARBINARY:
            return tango::typeInvalid;
        case SYBVOID:
            return tango::typeInvalid;
            return tango::typeInvalid;
        case XSYBVARBINARY:
            return tango::typeInvalid;
        case XSYBBINARY:
            return tango::typeInvalid;
    }

    return tango::typeInvalid;
}



const std::string empty_string = "";
const std::wstring empty_wstring = L"";


SqlServerIterator::SqlServerIterator()
{
    m_tds = NULL;
    m_connect_info = NULL;
    m_context = NULL;

    m_name = L"";
    m_eof = false;
}

SqlServerIterator::~SqlServerIterator()
{
    // -- clean up --
    tds_free_socket(m_tds);

    // -- clean up field vector and expression vector --

    std::vector<SqlServerDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete (*it);
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        delete (*it);
    }
}

bool SqlServerIterator::init(const std::wstring& query)
{
    if (!m_connect_info)
    {
        return false;
    }

    // -- allocate socket --
    m_tds = tds_alloc_socket(m_context, 512);
    tds_set_parent(m_tds, NULL);

    // -- attempt to connect to the sql server --
    if (!m_connect_info || tds_connect(m_tds, m_connect_info) == TDS_FAIL)
    {
        fprintf(stderr, "There was a problem connecting to the server\n");
        return false;
    }


    TDS_INT res;
    TDS_INT res_type;
    TDSCOLUMN* colinfo;
    int row_type;
    int compute_id;
    int row_count = 0;
    int col_count = 0;

    // -- submit the query to the sql server --
    std::string asc_query = kl::tostring(query);
    res = tds_submit_query(m_tds, asc_query.c_str());

    if (res != TDS_SUCCEED)
    {
        // -- tds_submit_query() failed --
        return false;
    }

    // -- add columns to the table's structure --
    res = tds_process_tokens(m_tds, &res_type, NULL, TDS_HANDLE_ALL);

    if (res != TDS_SUCCEED)
        return false;

    if (!m_tds->res_info)
        return false;

    col_count = m_tds->res_info->num_cols;

    int i = 0;
    for (i = 0; i < col_count; ++i)
    {
        colinfo = m_tds->res_info->columns[i];

        int sql_type = colinfo->column_type;
        int tango_type = tds2tangoType(colinfo);

        if (tango_type == tango::typeInvalid)
        {
            // -- certain complex types are not supported --
            continue;
        }

        SqlServerDataAccessInfo* dai = new SqlServerDataAccessInfo;
        dai->m_name = kl::towstring(colinfo->column_name);
        dai->m_type = tango_type;
        dai->m_width = colinfo->column_size;
        dai->m_ordinal = i;

        if (tango_type == tango::typeDouble)
        {
            dai->m_scale = 4;
        }
         else
        {
            dai->m_scale = colinfo->column_scale;
        }


        if (sql_type == SYBMONEYN ||
            sql_type == SYBMONEY ||
            sql_type == SYBMONEY4)
        {
            dai->m_width = 18;
            dai->m_scale = 4;
        }

        if (sql_type == SYBTEXT ||
            sql_type == SYBNTEXT)
        {
            // -- we cannot determine the length of
            //    this field, so set it to a default for now --

            dai->m_width = 512;
        }

        m_fields.push_back(dai);
    }

    // -- position cursor at the beginning of the table --
    res = tds_process_tokens(m_tds, &row_type, &compute_id, TDS_HANDLE_ALL);
    if (res == TDS_NO_MORE_RESULTS)
    {
        m_eof = true;
    }

    return true;
}


tango::ISetPtr SqlServerIterator::getSet()
{
    return m_set;
}

std::wstring SqlServerIterator::getTable()
{
    if (m_set)
        return L"";
    return m_set->getObjectPath();
}

tango::rowpos_t SqlServerIterator::getRowCount()
{
    return 0;
}

tango::IDatabasePtr SqlServerIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr SqlServerIterator::clone()
{
    return xcm::null;
}

unsigned int SqlServerIterator::getIteratorFlags()
{
    return tango::ifForwardOnly;
}

void SqlServerIterator::skip(int delta)
{
    if (delta <= 0)
    {
        return;
    }

    if (m_eof)
    {
        return;
    }

    int i;
    int res;
    int row_type;
    int compute_id;

    for (i = 0; i < delta; ++i)
    {
        if (!m_eof)
        {
            res = tds_process_tokens(m_tds, &row_type, &compute_id, TDS_HANDLE_ALL);
        }

        if (res == TDS_NO_MORE_RESULTS)
        {
            m_eof = true;
        }
    }
}

void SqlServerIterator::goFirst()
{

}

void SqlServerIterator::goLast()
{

}


tango::rowid_t SqlServerIterator::getRowId()
{
    return xcm::null;
}

bool SqlServerIterator::bof()
{
    return false;
}

bool SqlServerIterator::eof()
{
    if (m_eof)
    {
        return true;
    }

    return false;
}

bool SqlServerIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool SqlServerIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool SqlServerIterator::setPos(double pct)
{
    return false;
}

double SqlServerIterator::getPos()
{
    return 0.0;
}

void SqlServerIterator::goRow(const tango::rowid_t& row)
{
}


tango::IStructurePtr SqlServerIterator::getStructure()
{
    Structure* s = new Structure;

    std::vector<SqlServerDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        tango::IColumnInfoPtr col = s->createColumn();
        col->setName((*it)->m_name);
        col->setType((*it)->m_type);
        col->setWidth((*it)->m_width);
        col->setScale((*it)->m_scale);
        col->setColumnOrdinal((*it)->m_ordinal);
    }
    
    return static_cast<tango::IStructure*>(s);
}

void SqlServerIterator::refreshStructure()
{

}

bool SqlServerIterator::modifyStructure(tango::IStructure* struct_config, tango::IJob* job)
{
    return false;
}


tango::objhandle_t SqlServerIterator::getHandle(const std::wstring& expr)
{
    std::vector<SqlServerDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->m_name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }

    kscript::ExprParser* parser = parse(expr);
    if (!parser)
    {
        return (tango::objhandle_t)0;
    }


    SqlServerDataAccessInfo* dai = new SqlServerDataAccessInfo;
    dai->m_expr = parser;
    dai->m_type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool SqlServerIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<SqlServerDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
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

tango::IColumnInfoPtr SqlServerIterator::getInfo(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    ColumnInfo* colinfo = new ColumnInfo;
    colinfo->setName(dai->m_name);
    colinfo->setType(dai->m_type);
    colinfo->setWidth(dai->m_width);
    colinfo->setScale(dai->m_scale);

    if (dai->m_type == tango::typeDate ||
        dai->m_type == tango::typeInteger)
    {
        colinfo->setWidth(4);
    }
     else if (dai->m_type == tango::typeDateTime ||
              dai->m_type == tango::typeDouble)
    {
        colinfo->setWidth(8);
    }
     else if (dai->m_type == tango::typeBoolean)
    {
        colinfo->setWidth(1);
    }
     else
    {
        colinfo->setWidth(dai->m_width);
    }

    return static_cast<tango::IColumnInfo*>(colinfo);
}

int SqlServerIterator::getType(tango::objhandle_t data_handle)
{
    return 0;
}

int SqlServerIterator::getRawWidth(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    return dai->m_width;
}

const unsigned char* SqlServerIterator::getRawPtr(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    TDSCOLUMN* col_info = m_tds->res_info->columns[dai->m_ordinal];
    unsigned char* ptr = (m_tds->res_info->current_row + col_info->column_offset);
    int col_width = col_info->column_size;
    int data_width = col_info->column_cur_size;
    if (data_width > col_width || data_width < 0)        
        data_width = col_width;
    if (data_width < col_width)
    {
        memset(ptr+data_width, ' ', col_width-data_width);
    }
    return ptr;
}

const std::string& SqlServerIterator::getString(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_string;
    }

    if (dai->m_expr)
    {
        dai->m_expr->eval(&dai->m_expr_result);
        dai->m_str_result = kl::tostring(dai->m_expr_result.getString());
        return dai->m_str_result;
    }


    // -- is the column's value null? --
/*
    if (tds_get_null(m_tds->res_info->current_row, dai->m_ordinal))
    {
        return empty_string;
    }
*/


    if (dai->m_type == tango::typeCharacter)
    {
        unsigned char* src;
        TDS_INT src_len;
        int src_type;
        TDSCOLUMN* curcol = m_tds->res_info->columns[dai->m_ordinal];

        src = m_tds->res_info->current_row + curcol->column_offset;
        src_type = curcol->column_type;
        src_len = curcol->column_cur_size;

        if (src_type == SYBTEXT)
        {
            TDSBLOB* info = (TDSBLOB*)src;
            dai->m_str_result.assign(info->textvalue + curcol->column_textpos, src_len);
            return dai->m_str_result;
        }

        dai->m_str_result.assign((char*)src, src_len);
        return dai->m_str_result;
    }
     else if (dai->m_type == tango::typeWideCharacter)
    {
        dai->m_str_result = kl::tostring(getWideString(data_handle));
        return dai->m_str_result;
    }

    return empty_string;
}

const std::wstring& SqlServerIterator::getWideString(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }


    // -- is the column's value null? --
/*
    if (tds_get_null(m_tds->res_info->current_row, dai->m_ordinal))
    {
        return empty_wstring;
    }
*/


    if (dai->m_type == tango::typeCharacter)
    {
        dai->m_wstr_result = kl::towstring(getString(data_handle));
        return dai->m_wstr_result;
    }
     else if (dai->m_type == tango::typeWideCharacter)
    {
        if (dai->m_expr)
        {
            dai->m_expr->eval(&dai->m_expr_result);
            dai->m_wstr_result = dai->m_expr_result.getString();
            return dai->m_wstr_result;
        }


        unsigned char* src;
        TDS_INT src_len;
        int src_type;
        TDSCOLUMN* curcol = m_tds->res_info->columns[dai->m_ordinal];

        src = m_tds->res_info->current_row + curcol->column_offset;
        src_type = curcol->column_type;
        src_len = curcol->column_cur_size;

        if (src_type == SYBTEXT)
        {
        /*
            if (is_unicode_type(curcol->on_server.column_type))
            {
                TDSBLOB* info = (TDSBLOB*)src;
                kl::ucsbe2wstring(dai->m_wstr_result, (unsigned char*)(info->textvalue + curcol->column_textpos), src_len);
                dai->m_wstr_result.assign((wchar_t*)(info->textvalue + curcol->column_textpos), src_len);
                return dai->m_wstr_result;
            }
             else
        */

            {
                TDSBLOB* info = (TDSBLOB*)src;
                dai->m_str_result.assign(info->textvalue + curcol->column_textpos, src_len);
                dai->m_wstr_result = kl::towstring(dai->m_str_result);
                return dai->m_wstr_result;
            }
        }

/*
        if (is_unicode_type(curcol->on_server.column_type))
        {
            //dai->m_wstr_result.assign((wchar_t*)src, src_len);
            //return dai->m_wstr_result;
        }
         else
*/
        {
            dai->m_str_result.assign((char*)src, src_len);
            dai->m_wstr_result = kl::towstring(dai->m_str_result);
            return dai->m_wstr_result;
        }
    }

    return empty_wstring;
}

tango::datetime_t SqlServerIterator::getDateTime(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->m_expr)
    {
        dai->m_expr->eval(&dai->m_expr_result);
        kscript::ExprDateTime edt = dai->m_expr_result.getDateTime();

        tango::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        
        if (dai->m_type == tango::typeDateTime)
            dt |= edt.time;

        return dt;
    }


    // -- is the column's value null? --

/*
    if (tds_get_null(m_tds->res_info->current_row, dai->m_ordinal))
    {
        return 0;
    }
*/



    unsigned char* src;
    TDS_INT src_type;
    TDS_INT src_len;
    CONV_RESULT res_conv;
    TDSDATEREC daterec;

    src = m_tds->res_info->current_row + m_tds->res_info->columns[dai->m_ordinal]->column_offset;
    src_type = m_tds->res_info->columns[dai->m_ordinal]->column_type;
    src_len = m_tds->res_info->columns[dai->m_ordinal]->column_size;
    memset(&daterec, 0, sizeof(daterec));

    switch (src_type)
    {
        case SYBDATETIMN:
        case SYBDATETIME:
            tds_convert(m_tds->tds_ctx,
                        SYBDATETIME,
                        (TDS_CHAR*)src,
                        src_len,
                        SYBDATETIME,
                        &res_conv);

            tds_datecrack(SYBDATETIME, &(res_conv.dt), &daterec);
            break;

        case SYBDATETIME4:
            tds_convert(m_tds->tds_ctx,
                        SYBDATETIME4,
                        (TDS_CHAR*)src,
                        src_len,
                        SYBDATETIME4,
                        &res_conv);

            tds_datecrack(SYBDATETIME4, &(res_conv.dt4), &daterec);
            break;
    }

    tango::datetime_t dt;

    dt = dateToJulian(daterec.year, daterec.month + 1, daterec.day);
    dt <<= 32;
    dt |= ((daterec.hour*3600000) +
           (daterec.minute*60000) +
           (daterec.second*1000) +
           daterec.millisecond);

    return dt;
}

// -- this is declared in tds/numeric.c --
extern const int tds_numeric_bytes_per_prec[];

inline double tdsnum2double(unsigned char* src)
{
    unsigned char* p;
    int lo, hi;
    double retval = 0.0;
    int precision;
    int scale;
    bool negative;
    double mult;

    p = src;

    precision = p[0];
    if (precision == 0)
        return 0.0;

    scale = p[1];
    negative = p[2] ? true : false;
    p += tds_numeric_bytes_per_prec[precision]+1;

    mult = 1.0;

    while (p > (src+2))
    {
        hi = *p >> 4;
        lo = *p & 0x0F;

        retval += (lo*mult);
        retval += (hi*mult*16);
        mult = mult*256;

        p--;
    }

    if (scale > 0)
    {
        retval /= pow10(scale);
    }

    if (negative)
    {
        retval = retval*-1.0;
    }

    return kl::dblround(retval, scale);
}


double SqlServerIterator::getDouble(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0.0;
    }

    if (dai->m_expr)
    {
        dai->m_expr->eval(&dai->m_expr_result);
        return dai->m_expr_result.getDouble();
    }

/*
    if (tds_get_null(m_tds->res_info->current_row, dai->m_ordinal))
    {
        return 0.0;
    }
*/

    unsigned char* src;
    TDS_INT src_type;
    TDS_INT src_len;
    CONV_RESULT res_conv;

    src = m_tds->res_info->current_row + m_tds->res_info->columns[dai->m_ordinal]->column_offset;
    src_type = m_tds->res_info->columns[dai->m_ordinal]->column_type;
    src_len = m_tds->res_info->columns[dai->m_ordinal]->column_size;

    switch (src_type)
    {
        case SYBFLTN:
        {
            int conv_type = tds_get_conversion_type(src_type, src_len);
            if (conv_type == SYBFLT8)
            {
                tds_convert(m_tds->tds_ctx, SYBFLT8, (TDS_CHAR*)src, src_len, SYBFLT8, &res_conv);
            }
             else if (conv_type == SYBREAL)
            {
                tds_convert(m_tds->tds_ctx, SYBREAL, (TDS_CHAR*)src, src_len, SYBFLT8, &res_conv);
            }
             else
            {
                return 0.0;
            }

            return res_conv.f;
        }
        break;

        case SYBFLT8:
            tds_convert(m_tds->tds_ctx, SYBFLT8, (TDS_CHAR*)src, src_len, SYBFLT8, &res_conv);
            return res_conv.f;
        case SYBREAL:
            tds_convert(m_tds->tds_ctx, SYBREAL, (TDS_CHAR*)src, src_len, SYBFLT8, &res_conv);
            return res_conv.f;
        case SYBDECIMAL:
        case SYBNUMERIC:
            return tdsnum2double(src);
        case SYBMONEYN:
        {
            int conv_type = tds_get_conversion_type(src_type, src_len);

            if (conv_type == SYBMONEY)
            {
                tds_convert(m_tds->tds_ctx, SYBMONEY, (TDS_CHAR*)src, src_len, SYBFLT8, &res_conv);
            }
             else if (conv_type == SYBMONEY4)
            {
                tds_convert(m_tds->tds_ctx, SYBMONEY4, (TDS_CHAR*)src, src_len, SYBFLT8, &res_conv);
            }
             else
            {
                return 0.0;
            }

            return res_conv.f;
        }
        case SYBMONEY:
            tds_convert(m_tds->tds_ctx, SYBMONEY, (TDS_CHAR*)src, src_len, SYBFLT8, &res_conv);
            return res_conv.f;
        case SYBMONEY4:
            tds_convert(m_tds->tds_ctx, SYBMONEY4, (TDS_CHAR*)src, src_len, SYBFLT8, &res_conv);
            return res_conv.f;
    }

    return 0.0;
}

int SqlServerIterator::getInteger(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->m_expr)
    {
        dai->m_expr->eval(&dai->m_expr_result);
        return dai->m_expr_result.getInteger();
    }


/*
    if (tds_get_null(m_tds->res_info->current_row, dai->m_ordinal))
    {
        return 0;
    }
*/


    unsigned char* src;
    TDS_INT src_type;
    TDS_INT src_len;
    CONV_RESULT res_conv;

    src = m_tds->res_info->current_row + m_tds->res_info->columns[dai->m_ordinal]->column_offset;
    src_type = m_tds->res_info->columns[dai->m_ordinal]->column_type;
    src_len = m_tds->res_info->columns[dai->m_ordinal]->column_size;

    switch (src_type)
    {
        case SYBINT1:
            tds_convert(m_tds->tds_ctx, SYBINT1, (TDS_CHAR*)src, src_len, SYBINT1, &res_conv);
            return res_conv.ti;
        case SYBINT2:
            tds_convert(m_tds->tds_ctx, SYBINT2, (TDS_CHAR*)src, src_len, SYBINT2, &res_conv);
            return res_conv.si;
        case SYBINT4:
            tds_convert(m_tds->tds_ctx, SYBINT4, (TDS_CHAR*)src, src_len, SYBINT4, &res_conv);
            return res_conv.i;
        case SYBINTN:
        case SYBINT8:
            tds_convert(m_tds->tds_ctx, SYBINT8, (TDS_CHAR*)src, src_len, SYBINT8, &res_conv);
            return res_conv.bi;
    }

    return 0;
}

bool SqlServerIterator::getBoolean(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    if (dai->m_expr)
    {
        dai->m_expr->eval(&dai->m_expr_result);
        return dai->m_expr_result.getBoolean();
    }


/*
    if (tds_get_null(m_tds->res_info->current_row, dai->m_ordinal))
    {
        return false;
    }
*/


    unsigned char* src;
    TDS_INT src_len;
    CONV_RESULT res_conv;

    src = m_tds->res_info->current_row + m_tds->res_info->columns[dai->m_ordinal]->column_offset;
    src_len = m_tds->res_info->columns[dai->m_ordinal]->column_size;

    tds_convert(m_tds->tds_ctx, SYBBIT, (TDS_CHAR*)src, src_len, SYBBIT, &res_conv);
    return res_conv.ti ? true : false;
}

bool SqlServerIterator::isNull(tango::objhandle_t data_handle)
{
    SqlServerDataAccessInfo* dai = (SqlServerDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

/*
    if (tds_get_null(m_tds->res_info->current_row, dai->m_ordinal))
    {
        return true;
    }
*/

    return false;
}



