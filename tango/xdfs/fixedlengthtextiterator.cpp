/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-05-12
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "database.h"
#include "fixedlengthtextiterator.h"
#include "../xdcommon/util.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include <kl/string.h>
#include <kl/portable.h>
#include <cmath>


const int BUF_SIZE = 262144;
const std::string empty_string = "";
const std::wstring empty_wstring = L"";


// -- FixedLengthTextIterator class implementation --

FixedLengthTextIterator::FixedLengthTextIterator()
{
    m_path = L"";
    m_file = 0;
    m_file_size = 0;

    m_cur_row = 0;
    m_cur_row_offset = 0;
    m_cur_row_length = 0;

    m_line_delimiters = L"";
    m_row_width = 0;
    m_skip_chars = 0;

    m_file_type = FixedLengthDefinition::LineDelimited;

    m_chunk_size = 0;
    m_chunk_offset = 0;
    m_buf = 0;
}

FixedLengthTextIterator::~FixedLengthTextIterator()
{
    // free up the dai entries
    std::vector<FixedLengthTextDataAccessInfo*>::iterator it, it_end;
    it_end = m_source_fields.end();
    for (it = m_source_fields.begin(); it != it_end; ++it)
        delete *it;
    it_end = m_fields.end();
    for (it = m_fields.begin(); it != it_end; ++it)
        delete *it;
    it_end = m_exprs.end();
    for (it = m_exprs.begin(); it != it_end; ++it)
        delete *it;
    
    // close the file if it's open
    if (m_file)
    {
        xf_close(m_file);
        m_file = 0;
    }

    // free our buffer
    delete[] m_buf;

    // release our set reference
    m_set->unref();
}


// -- this machinery creates an expression parser which
//    can perform an on-the-fly transformation (or cast)
//    of an input text field --

static void _bindCastSourceField(kscript::ExprEnv*,
                                 void* param,
                                 kscript::Value* retval)
{
/*
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)param;
    std::wstring s = kl::towstring(dai->str_result);
    retval->setString(s.c_str());
*/
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)param;
    dai->iter->updateDaiEntry(dai);
    retval->setString(dai->wstr_result);
}

//static
bool FixedLengthTextIterator::script_host_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        FixedLengthTextIterator* pThis = (FixedLengthTextIterator*)hook_info.hook_param;
        
        if (hook_info.expr_text.length() > 1 &&
            hook_info.expr_text[0] == '[' &&
            hook_info.expr_text[hook_info.expr_text.length()-1] == ']')
        {
            // remove brackets from beginning and end e.g. [Field Name] => Field Name
            hook_info.expr_text = hook_info.expr_text.substr(0, hook_info.expr_text.length()-1);
            hook_info.expr_text.erase(0, 1);
        }
        
        FixedLengthTextDataAccessInfo* dai = NULL;
        
        size_t i, count = pThis->m_source_fields.size();
        for (i = 0; i < count; ++i)
        {
            if (0 == wcscasecmp(pThis->m_source_fields[i]->name.c_str(), hook_info.expr_text.c_str()))
            {
                dai = pThis->m_source_fields[i];
                break;
            }
        }

        if (!dai)
            return false;
    
        kscript::Value* v = new kscript::Value;
        v->setGetVal(kscript::Value::typeString,
                     _bindCastSourceField,
                     (void*)dai);
        hook_info.res_element = v;
        
        return true;
    }
    
    return false;
}


kscript::ExprParser* FixedLengthTextIterator::createCastingExprParser()
{
    kscript::ExprParser* parser = createExprParser();
    
    parser->setParseHook(kscript::ExprParseHookInfo::typeIdentifier,
                         script_host_parse_hook,
                         (void*)this);
    

    return parser;
}



bool FixedLengthTextIterator::init(tango::IDatabasePtr db,
                                   FixedLengthTextSet* set,
                                   const std::wstring& columns)
{
    m_database = db;
    m_set = set;
    m_set->ref();
                           
    m_columns_string = columns;
    m_path = m_set->m_path;
    m_line_delimiters = m_set->m_definition->m_line_delimiters;
    m_skip_chars = m_set->m_definition->m_skip_chars;
    m_row_width = m_set->m_definition->m_row_width;
    m_file_type = m_set->m_definition->m_file_type;
    
    int i, col_count;


    // get the source structure, put it in m_source_fields
    tango::IStructurePtr src_structure = m_set->getSourceStructure();
    col_count = src_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr src_colinfo;
        src_colinfo = src_structure->getColumnInfoByIdx(i);

        FixedLengthTextDataAccessInfo* dai = new FixedLengthTextDataAccessInfo;
        dai->iter = this;
        dai->src_offset = src_colinfo->getOffset();
        dai->src_width = src_colinfo->getWidth();
        dai->src_encoding = src_colinfo->getEncoding();
        dai->name = src_colinfo->getName();
        dai->type = tango::typeCharacter;
        dai->src_offset = src_colinfo->getOffset();
        dai->src_width = src_colinfo->getWidth();
        dai->width = src_colinfo->getWidth();
        dai->scale = 0;
        dai->expr_text = L"";
        dai->ordinal = i;

        m_source_fields.push_back(dai);
    }
    
    
    
    // get the destination structure, put it in m_fields
    tango::IStructurePtr dest_structure = m_set->getDestinationStructure();
    col_count = dest_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo;
        tango::IColumnInfoPtr src_colinfo;
        
        colinfo = dest_structure->getColumnInfoByIdx(i);
        src_colinfo = src_structure->getColumnInfo(colinfo->getExpression());
        
        FixedLengthTextDataAccessInfo* dai = new FixedLengthTextDataAccessInfo;
        
        if (src_colinfo)
        {
            dai->src_offset = src_colinfo->getOffset();
            dai->src_width = src_colinfo->getWidth();
            dai->src_encoding = src_colinfo->getEncoding();
        }
         else
        {
            dai->src_offset = 0;
            dai->src_width = 0;
            dai->src_encoding = 0;
        }
        
        dai->iter = this;
        dai->name = colinfo->getName();
        dai->type = colinfo->getType();
        dai->width = colinfo->getWidth();
        dai->scale = colinfo->getScale();
        dai->expr_text = colinfo->getExpression();
        dai->expr = NULL;
        dai->ordinal = i;


        if (src_colinfo.isNull() && dai->expr_text.length() > 0)
        {
            kscript::ExprParser* expr = createCastingExprParser();
            if (expr->parse(dai->expr_text))
                dai->expr = expr;
                 else
                delete expr;
        }

        m_fields.push_back(dai);
    }


    // get the calculated fields, add them to m_fields
    tango::IStructurePtr final_structure = m_set->getStructure();
    col_count = final_structure->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = final_structure->getColumnInfoByIdx(i);
        
        if (!colinfo->getCalculated())
            continue;
        
        FixedLengthTextDataAccessInfo* dai = new FixedLengthTextDataAccessInfo;
        dai->iter = this;
        dai->name = colinfo->getName();
        dai->type = colinfo->getType();
        dai->width = colinfo->getWidth();
        dai->scale = colinfo->getScale();
        dai->expr_text = colinfo->getExpression();
        dai->expr = NULL;
        dai->calculated = true;

        m_fields.push_back(dai);
    }

    // parse the calculated field expressions
    col_count = m_fields.size();
    for (i = 0; i < col_count; ++i)
    {
        if (m_fields[i]->calculated)
            m_fields[i]->expr = parse(m_fields[i]->expr_text);
    }


    // parse columns array
    if (m_columns_string != L"" && m_columns_string != L"*")
    {
        std::vector<std::wstring> colvec;
        std::vector<std::wstring>::iterator it;

        kl::parseDelimitedList(m_columns_string, colvec, L',', true);

        tango::IStructurePtr structure = getParserStructure();
        
        for (it = colvec.begin(); it != colvec.end(); ++it)
        {
            tango::IColumnInfoPtr col = structure->getColumnInfo(*it);
            if (col.isOk())
            {
                m_columns.push_back(*it);
                continue;
            }
            
            // -- look for 'AS' keyword --
            wchar_t* temp = zl_stristr((wchar_t*)it->c_str(),
                                       L"AS",
                                       true,
                                       false);
                                       
            int as_pos = (temp ? (temp - it->c_str()) : -1);

            std::wstring colname;
            std::wstring expr;

            if (as_pos != -1)
            {
                colname = it->substr(as_pos+2);
                expr = it->substr(0, as_pos);
            }
             else
            {
                expr = *it;
                
                int colname_counter=0;
                wchar_t buf[255];
                do
                {
                    swprintf(buf, 255, L"EXPR%03d", ++colname_counter);
                } while (structure->getColumnExist(buf));

                colname = buf;
            }

            kl::trim(colname);
            kl::trim(expr);

            kscript::ExprParser* p = parse(expr);

            if (!p)
                return false;

            int expr_type = p->getType();

            int tango_type = kscript2tangoType(expr_type);
            if (tango_type == tango::typeInvalid ||
                tango_type == tango::typeUndefined)
            {
                delete p;
                return false;
            }

            int width;
            int scale = 0;

            switch (tango_type)
            {
                case tango::typeNumeric:
                    width = 18;
                    break;
                case tango::typeDate:
                case tango::typeInteger:
                    width = 4;
                    break;
                case tango::typeDateTime:
                case tango::typeDouble:
                    width = 8;
                    break;
                default:
                    width = 254;
                    break;
            }


            col = structure->getColumnInfo(expr);
            if (col)
            {
                tango_type = col->getType();
                width = col->getWidth();
                scale = col->getScale();
            }


            FixedLengthTextDataAccessInfo* dai = new FixedLengthTextDataAccessInfo;
            dai->iter = this;
            dai->name = colname;
            dai->type = tango_type;
            dai->width = width;
            dai->scale = scale;
            dai->expr_text = expr;
            dai->expr = p;
            dai->calculated = true;

            m_fields.push_back(dai);
            
            m_columns.push_back(colname);
        }

    }




    // -- try to open the file --

    m_file_size = xf_get_file_size(m_path);
    m_file = xf_open(m_path, xfOpen, xfRead, xfShareReadWrite);
    if (!m_file)
        return false;

    // -- create a default row width, if we don't have one --
    if (m_row_width == 0)
        m_row_width = 1000;

    // -- initialize our buffer --
    
    try
    {
        m_buf = new unsigned char[BUF_SIZE];
    }
    catch(...)
    {
        // not enough memory
        xf_close(m_file);
        m_file = 0;
        return false;
    }
    
    if (!m_buf)
    {
        // not enough memory
        xf_close(m_file);
        m_file = 0;
        return false;
    }
        
    memset(m_buf, 0, BUF_SIZE);
    m_cur_row_offset = 0;

    // make sure m_buf is pointing to the beginning of the file
    // and that our data access info is filled out
    goFirst();

    return true;
}

bool FixedLengthTextIterator::internalEof(xf_off_t offset)
{
    return (offset+m_skip_chars >= m_file_size) ? true : false;
}

size_t FixedLengthTextIterator::getCurrentRowLength()
{
    xf_off_t row_off = m_cur_row_offset;
    size_t row_len = 0;

    while (1)
    {
        if (internalEof(row_off))
            break;
        if (m_line_delimiters.find(getCharAtOffset(row_off++)) != -1)
            break;

        ++row_len;
    }

    return row_len;
}


wchar_t FixedLengthTextIterator::getChar(xf_off_t row, xf_off_t col)
{
    if (!m_file || m_row_width == 0)
        return L'\0';

    if (m_file_type == FixedLengthDefinition::LineDelimited)
        return getDelimitedChar(row, col);
        
    if (m_file_type == FixedLengthDefinition::FixedWidth)
        return getFixedChar(row, col);

    return L'\0';
}

wchar_t FixedLengthTextIterator::getCharAtOffset(xf_off_t offset)
{
    offset += m_skip_chars;

    if (offset >= m_file_size)
        return L'\0';

    // if the character is in memory, return it
    if (offset >= m_chunk_offset && offset < m_chunk_offset+m_chunk_size)
        return m_buf[offset-m_chunk_offset];

    // character was not in the buffer, so we need to read
    // in a new chunk

    memset(m_buf, 0, BUF_SIZE);

    m_chunk_offset = (offset / BUF_SIZE) * BUF_SIZE;

    if (!xf_seek(m_file, m_chunk_offset, xfSeekSet))
    {
        m_chunk_offset = 0;
        m_chunk_size = 0;
        return L'\0';
    }

    m_chunk_size = xf_read(m_file, m_buf, 1, BUF_SIZE);

    // return the character (if not beyond EOF)
    if (offset >= m_chunk_offset && offset < m_chunk_offset+m_chunk_size)
        return m_buf[offset-m_chunk_offset];

    return L'\0';
}

wchar_t FixedLengthTextIterator::getDelimitedChar(xf_off_t row, xf_off_t col)
{
    // if we want a character from the current row,
    // just get it from the buffer (and check to make
    // sure we are in the bounds of the line)
    if (m_cur_row == row && m_cur_row_length != 0)
    {
        if (col >= m_cur_row_length)
            return L'\0';
        return getCharAtOffset(m_cur_row_offset + col);
    }

    // this next chunk of code will scroll to the correct line.
    // if the desired line is ahead, we need to scroll forward,
    // otherwise we need to scroll backward

    xf_off_t off = m_cur_row_offset;
    xf_off_t diff = row-m_cur_row;

    if (diff > 0)
    {
        // -- scroll forward --
        while (diff > 0)
        {
            ++off;

            if (internalEof(off))
            {
                // make sure these member variables are updated
                m_cur_row = row;
                m_cur_row_offset = off;
                return L'\0';
            }
            
            if (m_line_delimiters.find(getCharAtOffset(off)) != -1)
                --diff;
        }

        ++off;
    }
     else
    {
        // scroll backward (we need to go "one more" delimiter
        // to get to the beginning of the line)
        while (diff <= 0)
        {
            --off;

            if (off < 0)
            {
                ++diff;
                break;
            }

            if (m_line_delimiters.find(getCharAtOffset(off)) != -1)
                ++diff;
        }

        ++off;
    }

    m_cur_row = row;
    m_cur_row_offset = off;

    // -- discover row length --
    m_cur_row_length = getCurrentRowLength();

    // -- return the desired character from the line --
    if (col >= m_cur_row_length)
        return L'\0';

    return getCharAtOffset(m_cur_row_offset + col);
}

wchar_t FixedLengthTextIterator::getFixedChar(xf_off_t row, xf_off_t col)
{
    m_cur_row = row;

    return getCharAtOffset((row*m_row_width) + col);
}


void compToString(unsigned char* buf, int len, int scale, std::wstring& result)
{
    result = L"0";
}

void comp3ToString(unsigned char* buf, int len, int scale, std::wstring& result)
{
    double tempd = 0.0;

    bool null = true;
    
    int digit = (len*2)-2;
    for (int i = 0; i < len; ++i)
    {
        if (buf[i] != '@') // EBCDIC space
            null = false;
        
        tempd += (pow((double)10, digit) * double(buf[i] >> 4));
        digit--;

        if (i+1 < len)
        {
            tempd += (pow((double)10, digit) * double(buf[i] & 0xf));
            digit--;
        }
         else
        {
            // -- check if negative or positive -- 
            if ((buf[i] & 0xf) == 0x0d ||
                (buf[i] & 0xf) == 0x0b)
            {
                tempd *= -1;
            }        
        }
    }

    if (scale)
    {
        tempd /= pow((double)10, scale);
    }
    
    if (null)
    {
        result = L"";
    }
     else
    {   
        wchar_t temps[255];
        swprintf(temps, 255, L"%.*f", scale, tempd);
        result = temps;
    }
}


void FixedLengthTextIterator::updateDaiEntry(FixedLengthTextDataAccessInfo* dai)
{
    size_t off = (size_t)(dai->src_offset);
    size_t len = (size_t)(dai->src_width);
    
    if (len == 0 || off >= m_row_width)
    {
        dai->str_result = "";
        return;
    }
    
    if (off+len > m_row_width)
        len = m_row_width-off;

    dai->wstr_result = L"";
    dai->wstr_result.reserve(len);


    if (dai->src_encoding == tango::encodingUCS2 ||
        dai->src_encoding == tango::encodingUTF16)
    {
        wchar_t ch1, ch2;
        while (len > 0)
        {
            ch1 = getChar(m_cur_row, off++);
            if (len-- == 0)
                break;
            ch2 = getChar(m_cur_row, off++);
            if (len-- == 0)
                break;
            dai->wstr_result += ((ch1)+(ch2*256));
        }
        
        kl::trim(dai->wstr_result);
    }
     else if (dai->src_encoding == tango::encodingEBCDIC)
    {
        while (len-- > 0)
        {
            wchar_t c = getChar(m_cur_row, off++);
            if (c >= 0 && c <= 255)
                dai->wstr_result += (wchar_t)ebcdic2ascii((unsigned char)c);
        }
        
        kl::trim(dai->wstr_result);
    }
     else if (dai->src_encoding == tango::encodingCOMP)
    {
        unsigned char buf[48];
        size_t i;
        if (len > 42)
            len = 42;
        for (i = 0; i < len; ++i)
            buf[i] = (unsigned char)getChar(m_cur_row, off++);
        
        compToString(buf, len, 0, dai->wstr_result);
    }
     else if (dai->src_encoding == tango::encodingCOMP3)
    {
        unsigned char buf[48];
        size_t i;
        if (len > 42)
            len = 42;
        for (i = 0; i < len; ++i)
            buf[i] = (unsigned char)getChar(m_cur_row, off++);
        
        comp3ToString(buf, len, 0, dai->wstr_result);
    }
     else
    {
        while (len-- > 0)
            dai->wstr_result += getChar(m_cur_row, off++);
    }
    
    dai->str_result = kl::tostring(dai->wstr_result);
}


std::wstring FixedLengthTextIterator::getTable()
{
    if (m_set)
        return L"";
    return m_set->getObjectPath();
}


tango::rowpos_t FixedLengthTextIterator::getRowCount()
{
    return 0;
}

tango::IDatabasePtr FixedLengthTextIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr FixedLengthTextIterator::clone()
{
    FixedLengthTextIterator* iter = new FixedLengthTextIterator;
    
    if (!iter->init(m_database, m_set, m_columns_string))
    {
        return xcm::null;
    }
    
    
    iter->m_cur_row = m_cur_row;
    iter->m_cur_row_offset = m_cur_row_offset;
    iter->m_cur_row_length = m_cur_row_length;
    
    return static_cast<tango::IIterator*>(iter);
}

unsigned int FixedLengthTextIterator::getIteratorFlags()
{
    if (m_file_type == FixedLengthDefinition::FixedWidth)
        return tango::ifFastSkip;

    return 0;
}

void FixedLengthTextIterator::skip(int delta)
{
    if (!m_file)
        return;
    
    if (delta == 0)
        return;

    if (m_cur_row+delta < 0)
        goFirst();

    // -- this just moves our buffer to the correct location in the file --
    getChar(m_cur_row+delta, 0);
}

void FixedLengthTextIterator::goFirst()
{
    if (!m_file)
        return;
    
    // -- this just moves our buffer to the beginning of the file --
    getChar(0, 0);
}

void FixedLengthTextIterator::goLast()
{
    if (!m_file)
        return;
}

tango::rowid_t FixedLengthTextIterator::getRowId()
{
    //return m_cur_row_offset;
    return m_cur_row;
}

bool FixedLengthTextIterator::bof()
{
    return false;
}

bool FixedLengthTextIterator::eof()
{
    // the +2 is probably not the right answer...the eof condition
    // was not being flagged because the cur row offset was one short
    // of the file size.  We should find a different/better solution for this
    if (m_file_type == FixedLengthDefinition::LineDelimited)
    {
         return (m_cur_row_offset+m_skip_chars+2 >= m_file_size) ? true : false;
    }
    
    if (m_file_type == FixedLengthDefinition::FixedWidth)
    {
        xf_off_t val = (xf_off_t)m_cur_row;
        val *= m_row_width;
        val += m_skip_chars;
        return (val >= m_file_size) ? true : false;
    }

    return false;
}

bool FixedLengthTextIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
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
    return 0.0;
}

void FixedLengthTextIterator::goRow(const tango::rowid_t& rowid)
{
    getChar(rowid, 0);
}


tango::IStructurePtr FixedLengthTextIterator::getParserStructure()
{
    tango::IStructurePtr s = static_cast<tango::IStructure*>(new Structure);
    IStructureInternalPtr struct_int = s;
    
    std::vector<FixedLengthTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        
        col->setName((*it)->name);
        col->setType((*it)->type);
        col->setWidth((*it)->width);
        col->setScale((*it)->scale);
        col->setColumnOrdinal((*it)->ordinal);
        
        if ((*it)->isCalculated())
        {
            col->setCalculated(true);
            col->setExpression((*it)->expr_text);
        }
            
        struct_int->addColumn(col);
    }

    return s;
}


tango::IStructurePtr FixedLengthTextIterator::getStructure()
{
    // if we want all the columns, just return the m_fields structure
    if (m_columns_string == L"" || m_columns_string == L"*")
    {
        return getParserStructure();
    }
    

    // there were columns in m_columns specified, get those instead
    tango::IStructurePtr parser_structure = getParserStructure();
    
    tango::IStructurePtr s = static_cast<tango::IStructure*>(new Structure);
    IStructureInternalPtr struct_int = s;
    
    std::vector<std::wstring>::iterator it;
    for (it = m_columns.begin(); it != m_columns.end(); ++it)
    {
        tango::IColumnInfoPtr col = parser_structure->getColumnInfo(*it);
        if (col)
            struct_int->addColumn(col);
    }
    
    return s;
}

void FixedLengthTextIterator::refreshStructure()
{

}

bool FixedLengthTextIterator::modifyStructure(tango::IStructure* struct_config,
                                            tango::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<FixedLengthTextDataAccessInfo*>::iterator it2;
    
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
                FixedLengthTextDataAccessInfo* dai = *(it2);
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
            FixedLengthTextDataAccessInfo* dai = new FixedLengthTextDataAccessInfo;
            dai->iter = this;
            dai->name = it->m_params->getName();
            dai->type = it->m_params->getType();
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            dai->calculated = true;
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
        if (insert_idx < 0 || insert_idx >= (int)m_fields.size())
            continue;
        
        if (it->m_params->getExpression().length() > 0)
        {
            FixedLengthTextDataAccessInfo* dai = new FixedLengthTextDataAccessInfo;
            dai->iter = this;
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


tango::objhandle_t FixedLengthTextIterator::getHandle(const std::wstring& expr)
{
    std::vector<FixedLengthTextDataAccessInfo*>::reverse_iterator it;
    for (it = m_fields.rbegin(); it != m_fields.rend(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }
    
    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        FixedLengthTextDataAccessInfo* dai = new FixedLengthTextDataAccessInfo;
        dai->iter = this;
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

    FixedLengthTextDataAccessInfo* dai = new FixedLengthTextDataAccessInfo;
    dai->iter = this;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool FixedLengthTextIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<FixedLengthTextDataAccessInfo*>::iterator it;
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

tango::IColumnInfoPtr FixedLengthTextIterator::getInfo(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    ColumnInfo* colinfo = new ColumnInfo;
    colinfo->setName(dai->name);
    colinfo->setType(dai->type);
    colinfo->setWidth(dai->width);
    colinfo->setScale(dai->scale);

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


    if (dai->isCalculated())
    {
        colinfo->setCalculated(true);
        colinfo->setExpression(dai->expr_text);
    }

    return static_cast<tango::IColumnInfo*>(colinfo);
}

int FixedLengthTextIterator::getType(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    return dai->type;
}

int FixedLengthTextIterator::getRawWidth(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* FixedLengthTextIterator::getRawPtr(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return NULL;
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }
    
    return NULL;
}



const std::string& FixedLengthTextIterator::getString(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return empty_string;

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

    updateDaiEntry(dai);
    return dai->str_result;
/*
    // -- NOTE: this is only for REAL strings... since all of the other
    //    get functions base their function off of getString().  This resizes
    //    the string so that it appears correctly in the preview grid --
    if (dai->type == tango::typeCharacter ||
        dai->type == tango::typeWideCharacter)
    {
        if (dai->width < dai->src_width)
        {
            dai->str_result.resize(dai->width);
            return dai->str_result;
        }
    }

    return dai->str_result;
*/
}

const std::wstring& FixedLengthTextIterator::getWideString(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return empty_wstring;

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->wstr_result = dai->expr_result.getString();

        // -- NOTE: this is only for REAL strings... since all of the other
        //    get functions base their function off of getString().  This resizes
        //    the string so that it appears correctly in the preview grid --
        if (dai->type == tango::typeCharacter ||
            dai->type == tango::typeWideCharacter)
        {
            if (dai->wstr_result.length() > (size_t)dai->width)
            {
                dai->wstr_result.resize(dai->width);
                return dai->wstr_result;
            }
        }

        return dai->wstr_result;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return empty_wstring;
    }


    updateDaiEntry(dai);
    return dai->wstr_result;

/*
    dai->wstr_result = kl::towstring(getString(data_handle));
    return dai->wstr_result;
*/
}

tango::datetime_t FixedLengthTextIterator::getDateTime(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xcm::null;

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

    updateDaiEntry(dai);
    return str2datetime(dai->str_result.c_str());
}

double FixedLengthTextIterator::getDouble(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0.0;

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

    updateDaiEntry(dai);
    return kl::nolocale_atof(dai->str_result.c_str());
}

int FixedLengthTextIterator::getInteger(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return 0;

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

    updateDaiEntry(dai);
    return atoi(dai->str_result.c_str());
}

bool FixedLengthTextIterator::getBoolean(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
        return false;

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

    updateDaiEntry(dai);
    if (dai->wstr_result.length() > 0 && 
        (towupper(dai->wstr_result[0]) == L'T' ||
         towupper(dai->wstr_result[0]) == L'Y'))
    {
        return true;
    }
     else
    {
        return false;
    }
}

bool FixedLengthTextIterator::isNull(tango::objhandle_t data_handle)
{
    FixedLengthTextDataAccessInfo* dai = (FixedLengthTextDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

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


