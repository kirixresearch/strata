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
#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/sqlcommon.h"
#include "../xdcommon/localrowcache.h"
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/md5.h>

const int row_array_size = 1000;

const std::string empty_string = "";
const std::wstring empty_wstring = L"";


PgsqlIterator::PgsqlIterator(PgsqlDatabase* database, PgsqlSet* set)
{
    m_conn = NULL;
    m_database = database;
    m_set = set;

    m_row_pos = 0;
    m_block_row = 0;
    m_eof = false;
    
    m_server_side_cursor = false;
    
    m_cache_active = false;
    m_cache_dbrowpos = 0;
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
            PQexec(conn, "END");
            m_database->closeConnection(conn);
            return false;
        }


        res = PQexec(conn, "FETCH 100 from xdpgsqlcursor");
        m_block_start = 1;
        m_block_row = 0;
        m_block_rowcount = PQntuples(res);

        m_server_side_cursor = true;

        return init(conn, res);
    } 
     else
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

        return init(conn, res);
    }
}


bool PgsqlIterator::init(PGconn* conn, PGresult* res)
{
    m_conn = conn;
    m_res = res;

    std::wstring col_name;
    col_name.reserve(80);
    int col_pg_type;
    int col_tango_type;
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
        col_tango_type = pgsqlToTangoType(col_pg_type);
        col_width = 255;
        fmod = PQfmod(m_res, i);

        if (col_tango_type == tango::typeNumeric || col_tango_type == tango::typeDouble)
        {
            fmod -= 4;
            col_width = (fmod >> 16);
            col_scale = (fmod & 0xffff);
        }
         else if (col_tango_type == tango::typeCharacter || col_tango_type == tango::typeWideCharacter)
        {
            col_width = fmod - 4;
            col_scale = 0;
        }


        PgsqlDataAccessInfo* field = new PgsqlDataAccessInfo;
        field->name = col_name;
        field->pg_type = col_pg_type;
        field->type = col_tango_type;
        field->width = col_width;
        field->scale = col_scale;
        field->ordinal = i;

        m_fields.push_back(field);
    }


    refreshStructure();
    
    return true;
}

tango::ISetPtr PgsqlIterator::getSet()
{
    return static_cast<tango::ISet*>(m_set);
}

tango::IDatabasePtr PgsqlIterator::getDatabase()
{
    return static_cast<tango::IDatabase*>(m_database);
}

tango::IIteratorPtr PgsqlIterator::clone()
{
    return xcm::null;
}


void PgsqlIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & tango::ifReverseRowCache) != 0) ? true : false;
}
    
    
unsigned int PgsqlIterator::getIteratorFlags()
{
    return 0;
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
            snprintf(q, 80, "FETCH ABSOLUTE %d from xdpgsqlcursor", (int)(m_row_pos-1));
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
            return;
        }


        PQclear(m_res);

        m_res = PQexec(m_conn, "FETCH ABSOLUTE 0 from xdpgsqlcursor");
        PQclear(m_res);

        m_res = PQexec(m_conn, "FETCH 100 from xdpgsqlcursor");
        m_block_start = 1;
        m_block_rowcount = PQntuples(m_res);
        m_block_row = 0;
        if (m_block_rowcount == 0)
            m_eof = true;
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

tango::rowid_t PgsqlIterator::getRowId()
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

void PgsqlIterator::goRow(const tango::rowid_t& rowid)
{
}

double PgsqlIterator::getPos()
{
    return (double)(tango::tango_int64_t)m_row_pos;
}

tango::IStructurePtr PgsqlIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure->clone();


    tango::IStructurePtr set_structure;

/*    
    if (m_set)
    {
        set_structure = m_set->getStructure();
    }
*/

    Structure* s = new Structure;

    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        tango::IColumnInfoPtr col;
     
        // try to use the column info from the
        // set's structure, if possible

        if (set_structure)
        {
            col = set_structure->getColumnInfo((*it)->name);
        }

        if (col.isOk())
        {
            col->setColumnOrdinal((*it)->ordinal - 1);
            s->addColumn(col);
        }
         else
        {
            if ((*it)->isCalculated())
            {
                tango::IColumnInfoPtr col;
                col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
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
                tango::IColumnInfoPtr col;

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
    
    m_structure = static_cast<tango::IStructure*>(s);

    return m_structure->clone();
}

void PgsqlIterator::refreshStructure()
{
    tango::IStructurePtr set_structure = m_set->getStructure();
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

        tango::IColumnInfoPtr col = set_structure->getColumnInfo(m_fields[i]->name);
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
        tango::IColumnInfoPtr col;
        
        col = set_structure->getColumnInfoByIdx(i);
        if (!col->getCalculated())
            continue;
            
        bool found = false;
        
        for (it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (!(*it)->isCalculated())
                continue;

            if (0 == wcscasecmp((*it)->name.c_str(), col->getName().c_str()))
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

bool PgsqlIterator::modifyStructure(tango::IStructure* struct_config,
                                   tango::IJob* job)
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
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
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



tango::objhandle_t PgsqlIterator::getHandle(const std::wstring& expr)
{
    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
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
    {
        return (tango::objhandle_t)0;
    }

    PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool PgsqlIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<PgsqlDataAccessInfo*>::iterator it;
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

tango::IColumnInfoPtr PgsqlIterator::getInfo(tango::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    // try to get the column information from the set structure

    if (m_structure.isNull())
    {
        tango::IStructurePtr s = getStructure();
    }

    if (m_structure.isOk())
    {
        tango::IColumnInfoPtr colinfo;
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

int PgsqlIterator::getType(tango::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return tango::typeInvalid;
    }
    
    return dai->type;
}

int PgsqlIterator::getRawWidth(tango::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* PgsqlIterator::getRawPtr(tango::objhandle_t data_handle)
{
/*
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return NULL;
    }
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return (const unsigned char*)dai->str_val;
*/

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

const std::string& PgsqlIterator::getString(tango::objhandle_t data_handle)
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

const std::wstring& PgsqlIterator::getWideString(tango::objhandle_t data_handle)
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

tango::datetime_t PgsqlIterator::getDateTime(tango::objhandle_t data_handle)
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

        tango::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        if (dai->type == tango::typeDateTime)
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

    if (dai->type == tango::typeDate)
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

        tango::DateTime dt(y, m, d);
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

        tango::DateTime dt(y, m, d, h, mm, s, 0);
        return dt;
    }
}

double PgsqlIterator::getDouble(tango::objhandle_t data_handle)
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

    if (dai->type == tango::typeNumeric ||
        dai->type == tango::typeDouble)
    {
        return kl::nolocale_atof(c);
    }
     else if (dai->type == tango::typeInteger)
    {
        return atoi(c);
    }
     else
    {
        return 0.0;
    }
}

int PgsqlIterator::getInteger(tango::objhandle_t data_handle)
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

    if (dai->type == tango::typeNumeric ||
        dai->type == tango::typeDouble)
    {
        return (int)kl::nolocale_atof(c);
    }
     else if (dai->type == tango::typeInteger)
    {
        return atoi(c);
    }
     else
    {
        return 0;
    }
}

bool PgsqlIterator::getBoolean(tango::objhandle_t data_handle)
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

bool PgsqlIterator::isNull(tango::objhandle_t data_handle)
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


tango::ISetPtr PgsqlIterator::getChildSet(tango::IRelationPtr relation)
{
    return xcm::null;
/*
    if (eof())
        return xcm::null;

    tango::ISetPtr setptr = relation->getRightSetPtr();
    IPgsqlSetPtr set = setptr;
    if (set.isNull())
        return xcm::null;


    PgsqlIteratorRelInfo* info = NULL;
    std::vector<PgsqlIteratorRelInfo>::iterator it;
    for (it = m_relations.begin(); it != m_relations.end(); ++it)
    {
        if (it->relation_id == relation->getRelationId())
        {
            info = &(*it);
            break;
        }
    }

    if (!info)
    {
        PgsqlIteratorRelInfo relinfo;
        relinfo.relation_id = relation->getRelationId();


        std::vector<std::wstring> left_parts, right_parts;
        size_t i, cnt;

        kl::parseDelimitedList(relation->getLeftExpression(),  left_parts, L',', true);
        kl::parseDelimitedList(relation->getRightExpression(), right_parts, L',', true);

        // the number of parts in the left expression must match the count in the right expression
        if (left_parts.size() != right_parts.size())
            return xcm::null; 

        cnt = right_parts.size();
        for (i = 0; i < cnt; ++i)
        {
            PgsqlIteratorRelField f;
            f.right_field = right_parts[i];
            f.left_handle = getHandle(left_parts[i]);
            if (!f.left_handle)
                return xcm::null;
            f.left_type = ((PgsqlDataAccessInfo*)(f.left_handle))->type;
            
            relinfo.fields.push_back(f);
        }

        m_relations.push_back(relinfo);
        info = &(*m_relations.rbegin());
    }


    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);


    std::wstring expr;

    // build expression
    std::vector<PgsqlIteratorRelField>::iterator fit;
    for (fit = info->fields.begin(); fit != info->fields.end(); ++fit)
    {
        if (expr.length() > 0)
            expr += L" AND ";
        expr += quote_openchar + fit->right_field + quote_closechar + L"=";



        switch (fit->left_type)
        {
            case tango::typeCharacter:
            case tango::typeWideCharacter:
                expr += L"'";
                expr += getWideString(fit->left_handle);
                expr += L"'";
                break;
            case tango::typeInteger:
                expr += kl::itowstring(getInteger(fit->left_handle));
                break;
            case tango::typeNumeric:
            case tango::typeDouble:
                expr += kl::dbltostr(getDouble(fit->left_handle));
                break;
            case tango::typeDate:
            {
                tango::datetime_t dt = getDateTime(fit->left_handle);
                if (dt == 0)
                {
                    expr += L"NULL";
                }
                 else
                {
                    tango::DateTime d(dt);
                    expr += kl::stdswprintf(L"{d '%04d-%02d-%02d'}", d.getYear(), d.getMonth(), d.getDay());
                }
                break;
            }
            case tango::typeDateTime:
            {
                tango::datetime_t dt = getDateTime(fit->left_handle);
                if (dt == 0)
                {
                    expr += L"NULL";
                }
                 else
                {
                    tango::DateTime d(dt);
                    expr += kl::stdswprintf(L"{ts '%04d-%02d-%02d %02d:%02d:%02d.%03d'}", d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(), d.getSecond(), d.getMillisecond());
                }
                break;
            }
        }

    }



    set->setWhereCondition(expr);

    return setptr;
*/
}


tango::IIteratorPtr PgsqlIterator::getChildIterator(tango::IRelationPtr relation)
{
    return xcm::null;
/*
    tango::ISetPtr set = getChildSet(relation);

    if (set.isNull())
        return xcm::null;

    return set->createIterator(L"", L"", NULL);
*/
}



// tango::ICacheRowUpdate::updateCacheRow()

bool PgsqlIterator::updateCacheRow(tango::rowid_t rowid,
                                   tango::ColumnUpdateInfo* info,
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
            case tango::typeCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->str_val.c_str(),
                                    info->str_val.length());
                break;

            case tango::typeWideCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->wstr_val.c_str(),
                                    info->wstr_val.length()*sizeof(wchar_t));
                break;


            case tango::typeNumeric:
            case tango::typeDouble:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->dbl_val,
                                    sizeof(double));
                break;

            case tango::typeInteger:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->int_val,
                                    sizeof(int));
                break;

            case tango::typeDate:
            {
                SQL_DATE_STRUCT date;
                
                tango::DateTime dt;
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
            
            case tango::typeDateTime:
            {
                SQL_TIMESTAMP_STRUCT datetime;
                
                tango::DateTime dt;
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

            case tango::typeBoolean:
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

