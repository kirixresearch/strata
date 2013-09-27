/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#include <xd/xd.h>
#include "database.h"
#include "iterator.h"
#include "pkgfile.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"
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


KpgIterator::KpgIterator(KpgDatabase* database)
{
    m_database = database;
    m_database->ref();

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

    if (m_database)
        m_database->unref();
}

bool KpgIterator::init(const std::wstring& path)
{
    m_path = path;

    // get info block and parse it
    std::wstring info;
    if (!m_database->getStreamInfoBlock(path, info))
        return false;

    if (!m_info.parse(info))
        return false;

    // get row width
    int node_idx = m_info.getChildIdx(L"phys_row_width");
    if (node_idx == -1)
        return false;
    m_row_width = kl::wtoi(m_info.getChild(node_idx).getNodeValue());

    // get structure
    node_idx = m_info.getChildIdx(L"structure");
    if (node_idx == -1)
        return false;

    kl::xmlnode& structure_node = m_info.getChild(node_idx);
    m_structure = xdkpgXmlToStructure(structure_node);
    if (m_structure.isNull())
        return false;

    // open the kpg stream
    m_reader = m_database->m_kpg->readStream(path);
    if (!m_reader || !m_reader->reopen())
        return false;


    int i, col_count = m_structure->getColumnCount();
    int off = 0;
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr colinfo = m_structure->getColumnInfoByIdx(i);

        KpgDataAccessInfo* field = new KpgDataAccessInfo;
        field->name = colinfo->getName();
        field->type = colinfo->getType();
        field->width = colinfo->getWidth();
        field->scale = colinfo->getScale();
        field->ordinal = i;

        switch (field->type)
        {
            default:
            case xd::typeCharacter:
                field->buf_width = field->width;
                break;
            case xd::typeWideCharacter:
                field->buf_width = field->width * 2;
                break;
            case xd::typeDouble:
                field->buf_width = 8;
                break;
            case xd::typeDate:
                field->buf_width = 4;
                break;
            case xd::typeDateTime:
                field->buf_width = 8;
                break;
            case xd::typeBoolean:
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


void KpgIterator::setTable(const std::wstring& tbl)
{
    m_path = tbl;
}


std::wstring KpgIterator::getTable()
{
    return m_path;
}

xd::rowpos_t KpgIterator::getRowCount()
{
    return 0;
}

xd::IDatabasePtr KpgIterator::getDatabase()
{
    return static_cast<xd::IDatabase*>(m_database);
}

xd::IIteratorPtr KpgIterator::clone()
{
    KpgIterator* iter = new KpgIterator(m_database);
    if (!iter->init(m_path))
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

    return static_cast<xd::IIterator*>(iter);
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
    m_eof = (m_data == NULL) ? true : false;
}

void KpgIterator::goLast()
{
}

xd::rowid_t KpgIterator::getRowId()
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

void KpgIterator::goRow(const xd::rowid_t& rowid)
{
}

double KpgIterator::getPos()
{
    return (double)(long long)m_row_pos;
}

xd::IStructurePtr KpgIterator::getStructure()
{
    return m_structure->clone();
}

void KpgIterator::refreshStructure()
{
}

bool KpgIterator::modifyStructure(xd::IStructure* struct_config,
                                   xd::IJob* job)
{
    return false;
}



xd::objhandle_t KpgIterator::getHandle(const std::wstring& expr)
{
    std::vector<KpgDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, expr))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        KpgDataAccessInfo* dai = new KpgDataAccessInfo;
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

    KpgDataAccessInfo* dai = new KpgDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool KpgIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<KpgDataAccessInfo*>::iterator it;
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

xd::IColumnInfoPtr KpgIterator::getInfo(xd::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
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

int KpgIterator::getType(xd::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xd::typeInvalid;
    }
    
    return dai->type;
}

int KpgIterator::getRawWidth(xd::objhandle_t data_handle)
{
    KpgDataAccessInfo* dai = (KpgDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* KpgIterator::getRawPtr(xd::objhandle_t data_handle)
{


    return NULL;
}


const std::string& KpgIterator::getString(xd::objhandle_t data_handle)
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
    if (dai->type == xd::typeWideCharacter)
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

const std::wstring& KpgIterator::getWideString(xd::objhandle_t data_handle)
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

    if (dai->type == xd::typeCharacter)
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

xd::datetime_t KpgIterator::getDateTime(xd::objhandle_t data_handle)
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

    if (eof())
        return 0;

    if (dai->type == xd::typeDate)
    {
        xd::datetime_t dt;
        dt = *(unsigned int*)(m_row+dai->offset);
        dt <<= 32;
        return dt;
    }
     else if (dai->type == xd::typeDateTime)
    {
        return *(xd::datetime_t*)(m_row+dai->offset);
    }
     else
    {
        return 0;
    }

    return (xd::datetime_t)(m_row+dai->offset);
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


double KpgIterator::getDouble(xd::objhandle_t data_handle)
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
        case xd::typeDouble:
            return *(double*)col_data;    
        case xd::typeInteger:
            return *(int*)col_data;
        case xd::typeNumeric:
            return kl::dblround(decstr2dbl((char*)col_data, dai->width, dai->scale),
                                dai->scale);
        default:
            return 0.0;
    }
}

int KpgIterator::getInteger(xd::objhandle_t data_handle)
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
        case xd::typeDouble:
            return (int)(*(double*)col_data);    
        case xd::typeInteger:
            return *(int*)col_data;
        case xd::typeNumeric:
            return (int)(kl::dblround(decstr2dbl((char*)col_data, dai->width, dai->scale),
                                      dai->scale));
        default:
            return 0;
    }
}

bool KpgIterator::getBoolean(xd::objhandle_t data_handle)
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

bool KpgIterator::isNull(xd::objhandle_t data_handle)
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
