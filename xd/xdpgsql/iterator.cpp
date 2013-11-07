/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#include "libpq-fe.h"
#include <xd/xd.h>
#include "database.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/localrowcache.h"
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/md5.h>

const int row_array_size = 1000;

const std::string empty_string = "";
const std::wstring empty_wstring = L"";


PgsqlIterator::PgsqlIterator(PgsqlDatabase* database)
{
    m_conn = NULL;
    m_res = NULL;

    m_database = database;
    m_database->ref();

    m_row_pos = 0;
    m_block_row = 0;
    m_eof = false;
    
    m_server_side_cursor = false;
    
    m_cache_active = false;
    m_cache_dbrowpos = 0;

    m_row_count = (xd::rowpos_t)-1;
}

PgsqlIterator::~PgsqlIterator()
{
    // free up each structure containing relation info

    std::vector<PgsqlIteratorRelInfo>::iterator rit;
    for (rit = m_relations.begin(); rit != m_relations.end(); ++rit)
    {
        std::vector<PgsqlIteratorRelField>::iterator fit;

        // free up each field part making up a relationship
        for (fit = rit->fields.begin(); fit != rit->fields.end(); ++fit)
            releaseHandle(fit->left_handle);
    }


    // clean up field vector and expression vector

    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
        delete (*it);

    
    if (m_res)
        PQclear(m_res);

    if (m_conn)
    {
        if (m_server_side_cursor)
            PQexec(m_conn, "END");
        m_database->closeConnection(m_conn);
    }

    if (m_database)
        m_database->unref();
}

bool PgsqlIterator::init(const std::wstring& query)
{
    bool use_server_side_cursor = true;

    PGconn* conn = m_database->createConnection();
    if (!conn)
        return false;

    if (use_server_side_cursor)
    {
        PGresult* res;
        int status;


        res = PQexec(conn, "BEGIN");
        status = PQresultStatus(res);
        PQclear(res);

        if (status != PGRES_COMMAND_OK)
        {
            m_database->closeConnection(conn);
            return false;
        }

        res = PQexec(conn, kl::toUtf8(L"DECLARE xdpgsqlcursor SCROLL CURSOR FOR " + query));
        status = PQresultStatus(res);
        PQclear(res);

        if (status != PGRES_COMMAND_OK)
        {
            use_server_side_cursor = false;

            PQexec(conn, "END");
            m_database->closeConnection(conn);
            return false;
        }


        
        std::wstring command = L"explain " + query;
        const char* info = NULL;
        res = PQexec(conn, kl::toUtf8(command));
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            info = PQgetvalue(res, 0, 0);
            const char* rows = strstr(info, "rows=");
            if (rows)
            {
                xd::rowpos_t rowcnt = atoi(rows+5);
                if (rowcnt <= 10000000)
                {
                    m_row_count = rowcnt;
                }
            }
        }
        PQclear(res);




        res = PQexec(conn, "FETCH 100 from xdpgsqlcursor");
        m_block_start = 1;
        m_block_row = 0;
        m_block_rowcount = PQntuples(res);
        m_server_side_cursor = true;



        return init(conn, res);
    } 
    
    if (!use_server_side_cursor)
    {
        PGresult* res = PQexec(conn, kl::toUtf8(query));
        if (!res)
            return false;

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            m_database->closeConnection(conn);
            return false;
        }

        if (!init(conn, res))
            return false;
    }

    return true;
}


bool PgsqlIterator::init(PGconn* conn, PGresult* res)
{
    m_conn = conn;
    m_res = res;

    std::wstring col_name;
    col_name.reserve(80);
    int col_pg_type;
    int col_xd_type;
    int col_width = 20;
    int col_scale = 0;
    int fmod;
    bool col_nullable = true;

    int i;
    int col_count = PQnfields(m_res);

    for (i = 0; i < col_count; ++i)
    {
        col_name = kl::towstring(PQfname(m_res, i));
        col_pg_type = PQftype(m_res, i);
        col_xd_type = pgsqlToXdType(col_pg_type);
        col_width = 255;
        fmod = PQfmod(m_res, i);

        if (col_xd_type == xd::typeNumeric || col_xd_type == xd::typeDouble)
        {
            fmod -= 4;
            col_width = (fmod >> 16);
            col_scale = (fmod & 0xffff);
        }
         else if (col_xd_type == xd::typeCharacter || col_xd_type == xd::typeWideCharacter)
        {
            col_width = fmod - 4;
            col_scale = 0;
        }


        PgsqlDataAccessInfo* field = new PgsqlDataAccessInfo;
        field->name = col_name;
        field->pg_type = col_pg_type;
        field->type = col_xd_type;
        field->width = col_width;
        field->scale = col_scale;
        field->ordinal = i;

        m_fields.push_back(field);
    }


    refreshStructure();
    
    return true;
}

void PgsqlIterator::setTable(const std::wstring& table)
{
    m_path = table;
}


std::wstring PgsqlIterator::getTable()
{
    // m_path is usually empty, but if the iterator specifically represents a
    // concrete table object, we can express that here
    return m_path;
}

void PgsqlIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & xd::ifReverseRowCache) != 0) ? true : false;
}
    
unsigned int PgsqlIterator::getIteratorFlags()
{
    if (m_row_count != (xd::rowpos_t)-1)
        return xd::ifFastRowCount;
    
    return 0;
}

xd::rowpos_t PgsqlIterator::getRowCount()
{
    if (m_row_count != (xd::rowpos_t)-1)
        return m_row_count;
    return 0;
}

xd::IDatabasePtr PgsqlIterator::getDatabase()
{
    return static_cast<xd::IDatabase*>(m_database);
}

xd::IIteratorPtr PgsqlIterator::clone()
{
    return xcm::null;
}


void PgsqlIterator::skip(int delta)
{
    m_row_pos += delta;

    if (m_server_side_cursor)
    {
        m_eof = false;

        if (m_row_pos >= m_block_start && m_row_pos < m_block_start + m_block_rowcount)
        {
            m_block_row = (int)(m_row_pos - m_block_start);
            return;
        }


        if (m_row_pos == m_block_start + m_block_rowcount)
        {
            // no need to reposition, just get the next block

            PQclear(m_res);
            m_res = PQexec(m_conn, "FETCH 100 from xdpgsqlcursor");
            m_block_start += m_block_rowcount;
            m_block_rowcount = PQntuples(m_res);
            m_block_row = 0;
            if (m_block_rowcount == 0)
                m_eof = true;
        }
         else
        {
            PQclear(m_res);

            // reposition
            char q[80];
            snprintf(q, 80, "MOVE ABSOLUTE %d from xdpgsqlcursor", (int)(m_row_pos-1));
            m_res = PQexec(m_conn, q);
            PQclear(m_res);

            m_res = PQexec(m_conn, "FETCH 100 from xdpgsqlcursor");
            m_block_start = m_row_pos;
            m_block_rowcount = PQntuples(m_res);
            m_block_row = 0;
            if (m_block_rowcount == 0)
                m_eof = true;
       }
    }
     else
    {
        m_block_row += delta;
        m_eof = (m_block_row >= PQntuples(m_res));
    }
}

void PgsqlIterator::goFirst()
{
    if (m_server_side_cursor)
    {
        m_row_pos = 1;

        if (m_block_start == 1 && m_block_rowcount > 0)
        {
            m_block_row = 0;
            m_eof = false;
            return;
        }


        PQclear(m_res);

        m_res = PQexec(m_conn, "MOVE ABSOLUTE 0 from xdpgsqlcursor");
        PQclear(m_res);

        m_res = PQexec(m_conn, "FETCH 100 from xdpgsqlcursor");
        m_block_start = 1;
        m_block_rowcount = PQntuples(m_res);
        m_block_row = 0;
        m_eof = (m_block_row >= PQntuples(m_res));
    }
     else
    {
        m_row_pos = 1;
        m_block_row = 0;
        m_eof = (m_block_row >= PQntuples(m_res));
    }
}

void PgsqlIterator::goLast()
{
}

xd::rowid_t PgsqlIterator::getRowId()
{
    return m_row_pos;
}

bool PgsqlIterator::bof()
{
    return false;
}

bool PgsqlIterator::eof()
{
    return m_eof;
}

bool PgsqlIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool PgsqlIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool PgsqlIterator::setPos(double pct)
{
    return false;
}

void PgsqlIterator::goRow(const xd::rowid_t& rowid)
{
}

double PgsqlIterator::getPos()
{
    return (double)(long long)m_row_pos;
}

xd::IStructurePtr PgsqlIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure->clone();



    Structure* s = new Structure;

    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        xd::IColumnInfoPtr col;
     
        if (col.isOk())
        {
            col->setColumnOrdinal((*it)->ordinal - 1);
            s->addColumn(col);
        }
         else
        {
            if ((*it)->isCalculated())
            {
                xd::IColumnInfoPtr col;
                col = static_cast<xd::IColumnInfo*>(new ColumnInfo);
                col->setName((*it)->name);
                col->setType((*it)->type);
                col->setWidth((*it)->width);
                col->setScale((*it)->scale);
                col->setExpression((*it)->expr_text);
                col->setCalculated(true);
                col->setColumnOrdinal((*it)->ordinal - 1);
                s->addColumn(col);
            }
             else
            {
                // generate column info from the
                // field info from the query result
                xd::IColumnInfoPtr col;

                col = pgsqlCreateColInfo((*it)->name,
                                         (*it)->pg_type,
                                         (*it)->width,
                                         (*it)->scale,
                                         (*it)->expr_text,
                                         -1);

                col->setColumnOrdinal((*it)->ordinal - 1);
                s->addColumn(col);
            }
        }
    }
    
    m_structure = static_cast<xd::IStructure*>(s);

    return m_structure->clone();
}

void PgsqlIterator::refreshStructure()
{
    xd::IStructurePtr set_structure = m_database->describeTable(getTable());
    if (set_structure.isNull())
        return;

    // find changed/deleted calc fields
    size_t i, col_count;
    for (i = 0; i < m_fields.size(); ++i)
    {
        if (!m_fields[i]->isCalculated())
            continue;
            
        delete m_fields[i]->expr;
        m_fields[i]->expr = NULL;

        xd::IColumnInfoPtr col = set_structure->getColumnInfo(m_fields[i]->name);
        if (col.isNull())
        {
            m_fields.erase(m_fields.begin() + i);
            i--;
            continue;
        }
  
        m_fields[i]->type = col->getType();
        m_fields[i]->width = col->getWidth();
        m_fields[i]->scale = col->getScale();
        m_fields[i]->expr_text = col->getExpression();
    }
    
    // find new calc fields
    
    col_count = set_structure->getColumnCount();
    
    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr col;
        
        col = set_structure->getColumnInfoByIdx(i);
        if (!col->getCalculated())
            continue;
            
        bool found = false;
        
        for (it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (!(*it)->isCalculated())
                continue;

            if (kl::iequals((*it)->name, col->getName()))
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // add new calc field
            PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
            dai->name = col->getName();
            dai->type = col->getType();
            dai->width = col->getWidth();
            dai->scale = col->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = col->getExpression();
            dai->expr = NULL;
            
            m_fields.push_back(dai);
        }
    }
    
    // parse all expressions
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        (*it)->expr = parse((*it)->expr_text);
    }
    
    
    // let our m_structure cache regenerate from m_fields
    m_structure.clear();
}

bool PgsqlIterator::modifyStructure(xd::IStructure* struct_config,
                                   xd::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<PgsqlDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (kl::iequals(it->m_colname, (*it2)->name))
            {
                PgsqlDataAccessInfo* dai = *(it2);
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
            if (kl::iequals(it->m_colname, (*it2)->name))
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
            PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
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
    
    // the next call to getStructure() will refresh m_structure
    m_structure.clear();
    
    return true;
}



xd::objhandle_t PgsqlIterator::getHandle(const std::wstring& expr)
{
    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, expr))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
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

    PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool PgsqlIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<PgsqlDataAccessInfo*>::iterator it;
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

xd::IColumnInfoPtr PgsqlIterator::getInfo(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    // try to get the column information from the set structure

    if (m_structure.isNull())
    {
        xd::IStructurePtr s = getStructure();
    }

    if (m_structure.isOk())
    {
        xd::IColumnInfoPtr colinfo;
        colinfo = m_structure->getColumnInfo(dai->name);
        if (colinfo.isOk())
        {
            return colinfo->clone();
        }
    }


    // generate column information from our internal info

    return pgsqlCreateColInfo(dai->name,
                              dai->type,
                              dai->width,
                              dai->scale,
                              dai->expr_text,
                              -1);
}

int PgsqlIterator::getType(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xd::typeInvalid;
    }
    
    return dai->type;
}

int PgsqlIterator::getRawWidth(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* PgsqlIterator::getRawPtr(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
        return NULL;
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return NULL;
}


static unsigned char hex2byte(unsigned char b1, unsigned char b2)
{
    if (b1 >= '0' && b1 <= '9')
        b1 -= '0';
    else if (b1 >= 'A' && b1 <= 'F')
        b1 = b1 - 55;
    else
        return 0;
    
    if (b2 >= '0' && b2 <= '9')
        b2 -= '0';
    else if (b2 >= 'A' && b2 <= 'F')
        b2 = b2 - 55;
    else
        return 0;
    
    return (b1*16)+b2;
}

static void decodeHexString(const char* buf, std::string& out)
{
    out = "";
    char ch1, ch2;
    
    while (*buf)
    {
        ch1 = (char)towupper(*buf);
        
        ++buf;
        if (!*buf)
            break;
            
        ch2 = (char)towupper(*buf);
    
        out += (char)hex2byte((unsigned char)ch1, (unsigned char)ch2);
    
        ++buf;
    }
}

const std::string& PgsqlIterator::getString(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_string;
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

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return empty_string;

    dai->str_result = PQgetvalue(m_res, m_block_row, dai->ordinal);
    return dai->str_result;
}

const std::wstring& PgsqlIterator::getWideString(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

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

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return empty_wstring;

    dai->wstr_result = kl::fromUtf8((PQgetvalue(m_res, m_block_row, dai->ordinal)));
    return dai->wstr_result;
}

xd::datetime_t PgsqlIterator::getDateTime(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        kscript::ExprDateTime edt = dai->expr_result.getDateTime();

        xd::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        if (dai->type == xd::typeDateTime)
            dt |= edt.time;

        return dt;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return 0;

    const char* c = PQgetvalue(m_res, m_block_row, dai->ordinal);
    char buf[8];
    int y, m, d, h, mm, s;

    if (dai->type == xd::typeDate)
    {
        memcpy(buf, c, 4);
        buf[4] = 0;
        y = atoi(buf);

        memcpy(buf, c+5, 2);
        buf[2] = 0;
        m = atoi(buf);

        memcpy(buf, c+8, 2);
        buf[2] = 0;
        d = atoi(buf);

        if (y == 0)
            return 0;

        xd::DateTime dt(y, m, d);
        return dt;
    }
     else
    {
        memcpy(buf, c, 4);
        buf[4] = 0;
        y = atoi(buf);

        memcpy(buf, c+5, 2);
        buf[2] = 0;
        m = atoi(buf);

        memcpy(buf, c+8, 2);
        buf[2] = 0;
        d = atoi(buf);

        memcpy(buf, c+11, 2);
        buf[2] = 0;
        h = atoi(buf);

        memcpy(buf, c+14, 2);
        buf[2] = 0;
        mm = atoi(buf);

        memcpy(buf, c+17, 2);
        buf[2] = 0;
        s = atoi(buf);

        if (y == 0)
            return 0;

        xd::DateTime dt(y, m, d, h, mm, s, 0);
        return dt;
    }
}

double PgsqlIterator::getDouble(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0.0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getDouble();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0.0;
    }

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return 0.0;

    const char* c = PQgetvalue(m_res, m_block_row, dai->ordinal);

    if (dai->type == xd::typeNumeric ||
        dai->type == xd::typeDouble)
    {
        return kl::nolocale_atof(c);
    }
     else if (dai->type == xd::typeInteger)
    {
        return atoi(c);
    }
     else
    {
        return 0.0;
    }
}

int PgsqlIterator::getInteger(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getInteger();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return 0;

    const char* c = PQgetvalue(m_res, m_block_row, dai->ordinal);

    if (dai->type == xd::typeNumeric ||
        dai->type == xd::typeDouble)
    {
        return (int)kl::nolocale_atof(c);
    }
     else if (dai->type == xd::typeInteger)
    {
        return atoi(c);
    }
     else
    {
        return 0;
    }
}

bool PgsqlIterator::getBoolean(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getBoolean();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return false;
    }

    const char* c = PQgetvalue(m_res, m_block_row, dai->ordinal);

    return false;
    //return dai->bool_val ? true : false;
}

bool PgsqlIterator::isNull(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
        return true;

    if (dai->expr)
        return false;

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return true;
    }

    return PQgetisnull(m_res, m_block_row, dai->ordinal) ? true : false;
}



xd::IIteratorPtr PgsqlIterator::getChildIterator(xd::IRelationPtr relation)
{
    return xcm::null;
}

xd::IIteratorPtr PgsqlIterator::getFilteredChildIterator(xd::IRelationPtr relation)
{
    return xcm::null;
}


// xd::ICacheRowUpdate::updateCacheRow()

bool PgsqlIterator::updateCacheRow(xd::rowid_t rowid,
                                   xd::ColumnUpdateInfo* info,
                                   size_t info_size)
{
/*
    saveRowToCache();
    
    size_t i;
    for (i = 0; i < info_size; ++i, ++info)
    {
        PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)info->handle;
        int column = dai->ordinal - 1;
        
        if (info->null)
            m_cache.updateValue(m_row_pos, column, NULL, 0);
        

        switch (dai->type)
        {
            case xd::typeCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->str_val.c_str(),
                                    info->str_val.length());
                break;

            case xd::typeWideCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->wstr_val.c_str(),
                                    info->wstr_val.length()*sizeof(wchar_t));
                break;


            case xd::typeNumeric:
            case xd::typeDouble:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->dbl_val,
                                    sizeof(double));
                break;

            case xd::typeInteger:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->int_val,
                                    sizeof(int));
                break;

            case xd::typeDate:
            {
                SQL_DATE_STRUCT date;
                
                xd::DateTime dt;
                dt.setDateTime(info->date_val);
                
                date.year = dt.getYear();
                date.month = dt.getMonth();
                date.day = dt.getDay();
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&date,
                                    sizeof(SQL_DATE_STRUCT));
                break;
            }
            break;
            
            case xd::typeDateTime:
            {
                SQL_TIMESTAMP_STRUCT datetime;
                
                xd::DateTime dt;
                dt.setDateTime(info->date_val);
                
                datetime.year = dt.getYear();
                datetime.month = dt.getMonth();
                datetime.day = dt.getDay();
                datetime.hour = dt.getHour();
                datetime.minute = dt.getMinute();
                datetime.second = dt.getSecond();
                datetime.fraction = dt.getMillisecond() * 1000000;
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&datetime,
                                    sizeof(SQL_TIMESTAMP_STRUCT));
            }
            break;

            case xd::typeBoolean:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->bool_val,
                                    sizeof(bool));
                break;
        }
    }


    readRowFromCache(m_row_pos);

    */
    return true;
}

