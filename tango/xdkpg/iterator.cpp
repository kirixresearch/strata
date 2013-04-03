/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "pkgfile.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/sqlcommon.h"
#include "../xdcommon/localrowcache.h"
#include "../xdcommon/util.h"
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/md5.h>
#include <kl/math.h>

const int row_array_size = 1000;

const std::string empty_string = "";
const std::wstring empty_wstring = L"";


KpgIterator::KpgIterator(KpgDatabase* database, KpgSet* set)
{
    m_database = database;
    m_database->ref();

    m_set = set;
    if (m_set)
        m_set->ref();

    m_reader = NULL;
    m_cur_block = 0;
    m_row_pos = 0;
    m_data = NULL;
    m_data_len = 0;
    m_row = NULL;
    m_eof = false;
}

KpgIterator::~KpgIterator()
{
    delete m_reader;

    if (m_set)
        m_set->unref();

    if (m_database)
        m_database->unref();
}

bool KpgIterator::init()
{
    // get row width
    int node_idx = m_set->m_info.getChildIdx(L"phys_row_width");
    if (node_idx == -1)
        return false;
    m_row_width = kl::wtoi(m_set->m_info.getChild(node_idx).getNodeValue());

    m_structure = m_set->getStructure();

    int i, col_count = m_structure->getColumnCount();
    int off = 0;
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = m_structure->getColumnInfoByIdx(i);

        KpgDataAccessInfo* field = new KpgDataAccessInfo;
        field->name = colinfo->getName();
        field->type = colinfo->getType();
        field->width = colinfo->getWidth();
        field->scale = colinfo->getScale();
        field->ordinal = i;

        switch (field->type)
        {
            default:
            case tango::typeCharacter:
                field->buf_width = field->width;
                break;
            case tango::typeWideCharacter:
                field->buf_width = field->width * 2;
                break;
            case tango::typeDouble:
                field->buf_width = 8;
                break;
            case tango::typeDate:
                field->buf_width = 4;
                break;
            case tango::typeDateTime:
                field->buf_width = 8;
                break;
            case tango::typeBoolean:
                field->buf_width = 1;
                break;
        }


        field->offset = off;
        off += field->buf_width;

        m_fields.push_back(field);
    }



    refreshStructure();

    return true;
}



tango::ISetPtr KpgIterator::getSet()
{
    return static_cast<tango::ISet*>(m_set);
}

std::wstring KpgIterator::getTable()
{
    if (m_set)
        return L"";
    return m_set->getObjectPath();
}

tango::rowpos_t KpgIterator::getRowCount()
{
    return 0;
}

tango::IDatabasePtr KpgIterator::getDatabase()
{
    return static_cast<tango::IDatabase*>(m_database);
}

tango::IIteratorPtr KpgIterator::clone()
{
    KpgIterator* iter = new KpgIterator(m_database, m_set);
    iter->m_reader = m_database->m_kpg->readStream(m_set->m_tablename);
    if (!iter->m_reader->reopen())
    {
        delete iter;
        return xcm::null;
    }

    if (!iter->init())
    {
        delete iter;
        return xcm::null;
    }

    iter->m_block_offsets = m_block_offsets;
    iter->m_cur_block = m_cur_block;
    iter->m_row_pos = m_row_pos;
    iter->m_eof = m_eof;
    iter->m_row_width = m_row_width;

    iter->m_data = (unsigned char*)iter->m_reader->loadBlockAtOffset(iter->m_block_offsets[iter->m_cur_block], &iter->m_data_len);
    iter->m_row = iter->m_data + (m_row - m_data);

    return static_cast<tango::IIterator*>(iter);
}


void KpgIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
}
    
    
unsigned int KpgIterator::getIteratorFlags()
{
    return 0;
}

void KpgIterator::skip(int delta)
{
    int i;

    if (delta > 0)
    {
        for (i = 0; i < delta; ++i)
            skipForward();
    }
     else
    {
        for (i = 0; i < -delta; ++i)
            skipBackward();
    }
}

void KpgIterator::skipBackward()
{
    m_row -= m_row_width;
    if (m_row < m_data)
    {
        if (m_cur_block > 0)
        {
            m_cur_block--;
            m_data = (unsigned char*)m_reader->loadBlockAtOffset(m_block_offsets[m_cur_block], &m_data_len);
            int rows_in_block = m_data_len / m_row_width;
            if (m_data)
                m_row = m_data + (rows_in_block-1)*m_row_width;
                 else
                m_row = NULL;
            m_eof = (m_data == NULL) ? true : false;
        }
    }
}

void KpgIterator::skipForward()
{
    m_row += m_row_width;
    if (m_row >= m_data+m_data_len)
    {
        m_cur_block++;
        if (m_cur_block >= m_block_offsets.size())
            m_block_offsets.push_back(m_reader->tell());

        m_data = (unsigned char*)m_reader->loadBlockAtOffset(m_block_offsets[m_cur_block], &m_data_len);
        m_row = m_data;
        m_eof = (m_data == NULL) ? true : false;
    }
}



void KpgIterator::goFirst()
{
    m_block_offsets.clear();
    m_reader->rewind();
    
    m_data = (unsigned char*)m_reader->loadNextBlock(&m_data_len); // skip info block

    m_block_offsets.push_back(m_reader->tell());
    m_data = (unsigned char*)m_reader->loadNextBlock(&m_data_len);
    m_cur_block = 0;
    m_row = m_data;
}

void KpgIterator::goLast()
{
}

tango::rowid_t KpgIterator::getRowId()
{
    return m_row_pos;
}

bool KpgIterator::bof()
{
    return false;
}

bool KpgIterator::eof()
{
    return m_eof;
}

bool KpgIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool KpgIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool KpgIterator::setPos(double pct)
{
    return false;
}

void KpgIterator::goRow(const tango::rowid_t& rowid)
{
}

double KpgIterator::getPos()
{
    return (double)(long long)m_row_pos;
}

tango::IStructurePtr KpgIterator::getStructure()
{
    return m_set->getStructure();
}

void KpgIterator::refreshStructure()
{
}

bool KpgIterator::modifyStructure(tango::IStructure* struct_config,
                                   tango::IJob* job)
{
    return false;
}



tango::objhandle_t KpgIterator::getHandle(const std::wstring& expr)
{
    std::vector<KpgDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        KpgDataAccessInfo* dai = new KpgDataAccessInfo;
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

    KpgDataAccessInfo* dai = new KpgDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool KpgIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<KpgDataAccessInfo*>::iterator it;
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

tango::IColumnInfoPtr KpgIterator::getInfo(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
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
    /*
    return pgsqlCreateColInfo(dai->name,
                              dai->type,
                              dai->width,
                              dai->scale,
                              dai->expr_text,
                              -1);
    */
    return xcm::null;
}

int KpgIterator::getType(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return tango::typeInvalid;
    }
    
    return dai->type;
}

int KpgIterator::getRawWidth(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* KpgIterator::getRawPtr(tango::objhandle_t data_handle)
{


    return NULL;
}


const std::string& KpgIterator::getString(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
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

    if (eof())
    {
        dai->str_result = "";
        return dai->str_result;
    }

    // look for a zero terminator
    if (dai->type == tango::typeWideCharacter)
    {
        dai->str_result = kl::tostring(getWideString(data_handle));
    }
     else
    {
        int real_width = dai->buf_width;
        const unsigned char* col_data = m_row+dai->offset;
        const unsigned char* p = (const unsigned char*)memchr(col_data, 0, real_width);
        if (p)
            real_width = p-col_data;
        dai->str_result.assign((char*)col_data, real_width);
    }

    return dai->str_result;
}

const std::wstring& KpgIterator::getWideString(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
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


    if (eof())
    {
        dai->wstr_result = L"";
        return dai->wstr_result;
    }

    if (dai->type == tango::typeCharacter)
    {
        dai->wstr_result = kl::towstring(getString(data_handle));
    }
     else
    {
        const unsigned char* col_data = m_row+dai->offset;
        kl::ucsle2wstring(dai->wstr_result, col_data, dai->width);
    }

    return dai->wstr_result;
}

tango::datetime_t KpgIterator::getDateTime(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
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

    if (eof())
        return 0;

    if (dai->type == tango::typeDate)
    {
        tango::datetime_t dt;
        dt = *(unsigned int*)(m_row+dai->offset);
        dt <<= 32;
        return dt;
    }
     else if (dai->type == tango::typeDateTime)
    {
        return *(tango::datetime_t*)(m_row+dai->offset);
    }
     else
    {
        return 0;
    }

    return (tango::datetime_t)(m_row+dai->offset);
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


double KpgIterator::getDouble(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
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

    if (eof())
        return 0.0;

    const unsigned char* col_data = m_row+dai->offset;

    switch (dai->type)
    {
        case tango::typeDouble:
            return *(double*)col_data;    
        case tango::typeInteger:
            return *(int*)col_data;
        case tango::typeNumeric:
            return kl::dblround(decstr2dbl((char*)col_data, dai->width, dai->scale),
                                dai->scale);
        default:
            return 0.0;
    }
}

int KpgIterator::getInteger(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
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

    if (eof())
        return 0;

    const unsigned char* col_data = m_row+dai->offset;

    switch (dai->type)
    {
        case tango::typeDouble:
            return (int)(*(double*)col_data);    
        case tango::typeInteger:
            return *(int*)col_data;
        case tango::typeNumeric:
            return (int)(kl::dblround(decstr2dbl((char*)col_data, dai->width, dai->scale),
                                      dai->scale));
        default:
            return 0;
    }
}

bool KpgIterator::getBoolean(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
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

    if (eof())
        return false;

    const unsigned char* col_data = m_row+dai->offset;

    return (*col_data == 'T' ? true : false);
}

bool KpgIterator::isNull(tango::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
        return true;

    if (dai->expr)
        return false;

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return true;
    }

    if (eof())
        return true;

    return false;
}


tango::ISetPtr KpgIterator::getChildSet(tango::IRelationPtr relation)
{
    return xcm::null;
}

tango::IIteratorPtr KpgIterator::getChildIterator(tango::IRelationPtr relation)
{
    return xcm::null;
}

