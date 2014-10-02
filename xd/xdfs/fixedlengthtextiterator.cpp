/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <cmath>
#include <kl/math.h>
#include <kl/portable.h>
#include "xdfs.h"
#include "database.h"
#include "fixedlengthtextset.h"
#include "fixedlengthtextiterator.h"
#include "../xdcommon/util.h"


#ifdef _MSC_VER
#include <float.h>
#define isnan _isnan
#define finite _finite
#endif

const std::string empty_string = "";
const std::wstring empty_wstring = L"";

const int tableiterator_read_ahead_buffer_size = 2097152;




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


FixedLengthTextIterator::FixedLengthTextIterator(FsDatabase* database)
{
    m_database = database;
    m_database->ref();

    m_row_num = 1;
    m_table_rowwidth = 0;
    m_read_ahead_rowcount = 0;
    m_rowpos_buf = NULL;
    m_buf_rowcount = 0;
    m_buf_pos = 0;
    m_buf = NULL;
    m_bof = false;
    m_eof = false;
    m_set = NULL;
    m_table = &m_file;
    m_include_deleted = false;
    m_buffer_wrapper_mode = false;
}

FixedLengthTextIterator::~FixedLengthTextIterator()
{
    std::vector<TtbDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete (*it);
    }

    if (!m_buffer_wrapper_mode)
    {
        if (m_file.isOpen())
        {
            m_file.close();
        }

        if (m_set)
            m_set->unref();

        delete[] m_buf;
        delete[] m_rowpos_buf;
    }

    m_database->unref();
}

bool FixedLengthTextIterator::init(FixedLengthTextSet* set, const std::wstring& filename, const std::wstring& columns)
{
    if (!m_table->open(filename, set->m_def))
        return false;

    return init(set, &m_file, columns);
}


bool FixedLengthTextIterator::init(FixedLengthTextSet* set, FixedLengthTable* table, const std::wstring& columns)
{
    m_table = table;
    m_set = set;
    if (m_set)
        m_set->ref();

    m_row.setTable(m_table);
    m_columns = columns;
    m_table_ord = 0;
    m_table_rowwidth = m_table->getRowWidth();

    int max_read_ahead = tableiterator_read_ahead_buffer_size/m_table_rowwidth;
    if (max_read_ahead == 0)
        max_read_ahead = 1;

    m_read_ahead_rowcount = max_read_ahead;
    if (m_read_ahead_rowcount > 10000)
    {
        // some tables with a very skinny row width could have millions of
        // read ahead rows.  On files with a delete map this can lead to
        // performance degredation.  Limit read-ahead rows to 10000
        m_read_ahead_rowcount = 10000;
    }

    // don't choose a large read-ahead buffer size if there are only a few records
    xd::rowpos_t res = m_table->getRowCount();
    if (res < (xd::rowpos_t)m_read_ahead_rowcount)
        m_read_ahead_rowcount = (int)res;
    if (m_read_ahead_rowcount < 100) // but not too small either, in case the table grows
        m_read_ahead_rowcount = 100;
    if (m_read_ahead_rowcount > max_read_ahead)
        m_read_ahead_rowcount = max_read_ahead;


    m_buf = new unsigned char[m_read_ahead_rowcount * m_table_rowwidth];
    m_rowpos_buf = new xd::rowpos_t[m_read_ahead_rowcount];

    return refreshStructure();
}

bool FixedLengthTextIterator::initFromBuffer(FixedLengthTextSet* set, FixedLengthTable* table, unsigned char* buffer, const std::wstring& columns)
{
    m_buffer_wrapper_mode = true;
    m_set = set; // don't hold a reference in this mode
    m_table = table;
    m_buf = buffer;
    m_rowptr = buffer;
    m_row.setRowPtr(m_rowptr);
    m_table_rowwidth = m_table->getRowWidth();

    refreshStructure();

    return true;
}



void FixedLengthTextIterator::setTable(const std::wstring& tbl)
{
}

std::wstring FixedLengthTextIterator::getTable()
{
    if (!m_set)
        return L"";
    return m_set->getObjectPath();
}

xd::rowpos_t FixedLengthTextIterator::getRowCount()
{
    return m_table->getRowCount();
}

xd::IDatabasePtr FixedLengthTextIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr FixedLengthTextIterator::clone()
{
    if (m_buffer_wrapper_mode)
        return xcm::null;

    FixedLengthTextIterator* new_iter = new FixedLengthTextIterator(m_database);
    
    if (!new_iter->init(m_set, m_table->getFilename(), m_columns))
    {
        return xcm::null;
    }
    
    if (new_iter->m_table_rowwidth != m_table_rowwidth ||
        new_iter->m_buf_rowcount != m_buf_rowcount)
    {
        // buffer sizes aren't the same anymore; clone not possible
        delete new_iter;
        return xcm::null;
    }

    new_iter->m_table_ord = m_table_ord;
    new_iter->m_eof = m_eof;
    new_iter->m_bof = m_bof;
    new_iter->m_read_ahead_rowcount = m_read_ahead_rowcount;
    new_iter->m_include_deleted = m_include_deleted;

    new_iter->m_buf_rowcount = m_buf_rowcount;
    new_iter->m_buf_pos = m_buf_pos;
    new_iter->m_rowid = m_rowid;
    memcpy(new_iter->m_buf, m_buf, m_table_rowwidth * m_buf_rowcount);
    memcpy(new_iter->m_rowpos_buf, m_rowpos_buf, sizeof(xd::rowpos_t) * m_buf_rowcount);

    new_iter->updatePosition();
    new_iter->refreshStructure();

    return static_cast<xd::IIterator*>(new_iter);
}

void FixedLengthTextIterator::updatePosition()
{
    m_rowptr = (m_buf + (m_buf_pos * m_table_rowwidth));
    m_row.setRowPtr(m_rowptr);
    m_rowid = m_rowpos_buf[m_buf_pos];
}


void FixedLengthTextIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    if (mask & xd::ifTemporary)
    {
        bool v = (value & xd::ifTemporary) ? true : false;
        m_set->setTemporary(v);
    }
}


unsigned int FixedLengthTextIterator::getIteratorFlags()
{
    if (m_set->m_def.fixed_line_delimited)
        return 0;

    return (xd::ifFastRowCount | xd::ifFastSkip);
}


void FixedLengthTextIterator::goFirst()
{
    if (m_buffer_wrapper_mode)
        return;

    int read_ahead_rowcount = m_read_ahead_rowcount;
    if (read_ahead_rowcount > 100)
        read_ahead_rowcount = 100;
        
    m_buf_rowcount = m_table->getRows(m_buf,
                                      m_rowpos_buf,
                                      0,
                                      1,
                                      read_ahead_rowcount,
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

void FixedLengthTextIterator::goLast()
{
}

void FixedLengthTextIterator::skip(int delta)
{
    if (m_buffer_wrapper_mode)
        return;

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
            m_buf_rowcount = m_table->getRows(m_buf,
                                              m_rowpos_buf,
                                              delta,
                                              m_rowpos_buf[m_buf_pos],
                                              1,
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
            m_buf_rowcount = m_table->getRows(m_buf,
                                              m_rowpos_buf,
                                              delta,
                                              m_rowpos_buf[m_buf_pos],
                                              m_read_ahead_rowcount,
                                              m_include_deleted);

            m_buf_pos = 0;
            m_eof = (m_buf_rowcount == 0);

            if (m_eof)
            {
                m_rowpos_buf[0] = m_table->getRowCount()+1;
                m_row_num = m_rowpos_buf[0];
                return;
            }

            m_bof = false;
        }

        updatePosition();
    }

}

xd::rowid_t FixedLengthTextIterator::getRowId()
{
    return m_rowid;
}

bool FixedLengthTextIterator::bof()
{
    return m_bof;
}

bool FixedLengthTextIterator::eof()
{
    return m_eof;
}

bool FixedLengthTextIterator::seek(const unsigned char* key, int length, bool soft)
{
    if (m_buffer_wrapper_mode)
        return false;

    // keys on table iterators indicate rowid
    if (length != sizeof(xd::rowid_t))
        return false;

    xd::rowid_t* rowid = (xd::rowid_t*)key;
    xd::rowpos_t row = *rowid;

    m_buf_rowcount = m_table->getRows(m_buf,
                                    m_rowpos_buf,
                                    0,
                                    row,
                                    1,
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

bool FixedLengthTextIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool FixedLengthTextIterator::setPos(double pct)
{
    return false;
}

double FixedLengthTextIterator::getPos()
{
    double d = (double)(long long)m_rowid;
    double row_count = (double)(long long)getRowCount();
    if (kl::dblcompare(row_count, 0.0) == 0)
        return 0.0;

    return (d/row_count);
}

void FixedLengthTextIterator::goRow(const xd::rowid_t& rowid)
{
    if (m_buffer_wrapper_mode)
        return;

    xd::rowpos_t row_pos = rowid;

    m_table->getRow(row_pos, m_buf);
    m_rowptr = m_buf;
    m_row.setRowPtr(m_rowptr);
    m_buf_pos = 0;
    m_rowpos_buf[m_buf_pos] = row_pos;

    updatePosition();
}

xd::Structure FixedLengthTextIterator::getStructure()
{
    xd::Structure s;

    std::vector<TtbDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!(*it)->isColumn())
            continue;
        if (!(*it)->visible)
            continue;

        xd::ColumnInfo col;

        col.name = (*it)->name;
        col.type = (*it)->type;
        col.width = (*it)->width;
        col.scale = (*it)->scale;
        col.expression = (*it)->expr_text;
        col.calculated = (*it)->isCalculated();
        col.column_ordinal = (*it)->ordinal;

        s.createColumn(col);
    }

    return s;
}

xd::Structure FixedLengthTextIterator::getParserStructure()
{
    xd::Structure s;

    std::vector<TtbDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!(*it)->isColumn())
            continue;

        xd::ColumnInfo col;

        col.name = (*it)->name;
        col.type = (*it)->type;
        col.width = (*it)->width;
        col.scale = (*it)->scale;
        col.expression = (*it)->expr_text;
        col.calculated = (*it)->isCalculated();
        col.column_ordinal = (*it)->ordinal;

        s.createColumn(col);
    }

    return s;
}

bool FixedLengthTextIterator::refreshStructure()
{
    m_fields.clear();

    xd::Structure set_structure = m_set->getStructure();

    // add fields from structure
    bool default_structure_visible = false;
    if (m_columns.empty() || m_columns == L"*")
        default_structure_visible = true;


    size_t i, col_count = set_structure.getColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& colinfo = set_structure.getColumnInfoByIdx(i);
        
        TtbDataAccessInfo* dai = new TtbDataAccessInfo;
        dai->name = colinfo.name;
        dai->type = colinfo.type;
        dai->offset = colinfo.source_offset;
        dai->width = colinfo.width;
        dai->scale = colinfo.scale;
        dai->ordinal = colinfo.column_ordinal;
        dai->expr_text = colinfo.expression;
        dai->visible = default_structure_visible;
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
                TtbDataAccessInfo* dai = new TtbDataAccessInfo;
                dai->name = colinfo.name;
                dai->type = colinfo.type;
                dai->offset = colinfo.source_offset;
                dai->width = colinfo.width;
                dai->scale = colinfo.scale;
                dai->ordinal = colinfo.column_ordinal;
                dai->expr_text = colinfo.expression;
                dai->visible = true;
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
                    TtbDataAccessInfo* dai = new TtbDataAccessInfo;
                    dai->name = colname;
                    dai->type = colinfo.type;
                    dai->offset = colinfo.source_offset;
                    dai->width = colinfo.width;
                    dai->scale = colinfo.scale;
                    dai->ordinal = colinfo.column_ordinal;
                    dai->expr_text = colinfo.expression;
                    dai->visible = true;
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

                TtbDataAccessInfo* dai = new TtbDataAccessInfo;
                dai->name = colname;
                dai->type = xd_type;
                dai->offset = 0;
                dai->width = width;
                dai->scale = scale;
                dai->ordinal = 0;
                dai->expr_text = expr;
                dai->expr = p;
                dai->visible = true;
                m_fields.push_back(dai);
            }
        }
    }

    return true;
}

bool FixedLengthTextIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<TtbDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->column.c_str(), (*it2)->name.c_str()))
            {
                TtbDataAccessInfo* dai = *(it2);
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
            TtbDataAccessInfo* dai = new TtbDataAccessInfo;
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
        if (insert_idx < 0 || (size_t)insert_idx >= m_fields.size())
            continue;
        
        if (it->params.expression.length() > 0)
        {
            TtbDataAccessInfo* dai = new TtbDataAccessInfo;
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


xd::objhandle_t FixedLengthTextIterator::getHandle(const std::wstring& expr)
{
    std::vector<TtbDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, expr))
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

bool FixedLengthTextIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<TtbDataAccessInfo*>::iterator it;
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

xd::ColumnInfo FixedLengthTextIterator::getInfo(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xd::ColumnInfo();
    }

    xd::ColumnInfo colinfo;
    colinfo.name = dai->name;
    colinfo.type = dai->type;
    colinfo.width = dai->width;
    colinfo.scale = dai->scale;
    colinfo.expression = dai->expr_text;
    
    if (dai->type == xd::typeDate || dai->type == xd::typeInteger)
    {
        colinfo.width = 4;
    }
     else if (dai->type == xd::typeDateTime || dai->type == xd::typeDouble)
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

int FixedLengthTextIterator::getType(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;

    return dai->type;
}

int FixedLengthTextIterator::getRawWidth(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
        return dai->key_layout->getKeyLength();
    
    return m_row.getRawWidth(dai->ordinal);
}

const unsigned char* FixedLengthTextIterator::getRawPtr(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;
    if (dai == NULL)
        return NULL;
    if (!m_rowptr)
        return NULL;

    if (dai->key_layout)
        return dai->key_layout->getKey();

    return m_row.getRawPtr(dai->ordinal);
}

const std::string& FixedLengthTextIterator::getString(xd::objhandle_t data_handle)
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

        if (!dai->expr ||
            !dai->expr->eval(&dai->expr_result) ||
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
    

    return m_row.getString(dai->ordinal);

    /*
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
    */


}

const std::wstring& FixedLengthTextIterator::getWideString(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    //if (!dai->is_active)
    //    return empty_wstring;
    if (!m_rowptr)
        return empty_wstring;

    if (dai->isCalculated())
    {
        if (!dai->expr ||
            !dai->expr->eval(&dai->expr_result) ||
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
    return m_row.getWideString(dai->ordinal);
}

xd::datetime_t FixedLengthTextIterator::getDateTime(xd::objhandle_t data_handle)
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
        if (!dai->expr ||
            !dai->expr->eval(&dai->expr_result) ||
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

double FixedLengthTextIterator::getDouble(xd::objhandle_t data_handle)
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
        if (!dai->expr ||
            !dai->expr->eval(&dai->expr_result) ||
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

int FixedLengthTextIterator::getInteger(xd::objhandle_t data_handle)
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
        if (!dai->expr ||
            !dai->expr->eval(&dai->expr_result) ||
            (dai->expr->getType() != kscript::Value::typeDouble &&  dai->expr->getType() != kscript::Value::typeInteger))
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


bool FixedLengthTextIterator::getBoolean(xd::objhandle_t data_handle)
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


bool FixedLengthTextIterator::isNull(xd::objhandle_t data_handle)
{
    TtbDataAccessInfo* dai = (TtbDataAccessInfo*)data_handle;

    //if (!dai->is_active)
    //    return true;

    if (!m_rowptr)
        return true;

    if (dai->isCalculated())
    {
        if (!dai->expr)
            return true;
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





