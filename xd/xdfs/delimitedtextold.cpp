/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2002-12-11
 *
 */


#include <kl/klib.h>
#include "delimitedtext.h"
#include "rawtext.h"


const int DELIMITEDTEXT_BUFSIZE = 1000000;    // number of bytes in each buffer


static std::string empty_string = "";


// -- DelimitedTextFile class implementation --

DelimitedTextFile::DelimitedTextFile()
{
    m_filename = L"";
    m_buf_file = new BufferedFile;

    m_offset = 0;
    
    m_delimiters = L",";
    m_line_delimiters = L"\x0d\x0a";  // CR/LF
    m_text_qualifiers = L"\"";
    m_skip_crlf = false;
    m_inserting = false;
    
    m_buf = NULL;
    m_currow_ptr = NULL;
}

DelimitedTextFile::~DelimitedTextFile()
{
    if (m_buf_file)
        delete m_buf_file;
        
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }
}

bool DelimitedTextFile::openFile(const std::wstring& filename)
{
    bool retval = m_buf_file->openFile(filename, xfReadWrite, xfShareReadWrite);
    
    if (!retval)
    {
        // try read-only
        retval = m_buf_file->openFile(filename, xfRead, xfShareReadWrite);
    }
    
    if (!retval)
    {
        // file didn't even open in read-only mode
        return false;
    }

    m_filename = filename;
    return true;
}

bool DelimitedTextFile::createFile(const std::wstring& filename,
                                   const std::vector<std::wstring>& fields)
{
    if (!m_buf_file->openOrCreateFile(filename))
        return false;

    if (!startInsert())
        return false;
    
    size_t i = 0;
     
    std::vector<std::wstring>::const_iterator it;
    for (it = fields.begin(); it != fields.end(); ++it)
    {
        putString(i++, *it);
    }
    
    // if fields were inserted, add a new row; otherwise, simply
    // finish the insert, to avoid having a blank line up at the
    // top of the file
    if (fields.size() > 0)
        insertRow();

    finishInsert();
    
    return true;
}

bool DelimitedTextFile::isOpen()
{
    return m_buf_file->isOpen();
}

void DelimitedTextFile::closeFile()
{
    m_buf_file->closeFile();
    
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }
}

bool DelimitedTextFile::isUnicode()
{
    if (!m_buf_file)
        return false;
    return m_buf_file->isUnicode();
}

const std::wstring& DelimitedTextFile::getFilename()
{
    return m_filename;
}

bool DelimitedTextFile::bof()
{
    return (m_offset == 0) ? true : false;
}

bool DelimitedTextFile::eof()
{
    return m_buf_file->isPositionEof(m_offset);
}

void DelimitedTextFile::setDelimiters(const std::wstring& val)
{
    m_delimiters = val;
}

void DelimitedTextFile::setLineDelimiters(const std::wstring& val)
{
    m_line_delimiters = val;
}

void DelimitedTextFile::setTextQualifiers(const std::wstring& val)
{
    m_text_qualifiers = val;
}

void DelimitedTextFile::setSkipCrLf(bool val)
{
    m_skip_crlf = val;
}

const std::wstring& DelimitedTextFile::getDelimiters() const
{
    return m_delimiters;
}

const std::wstring& DelimitedTextFile::getLineDelimiters() const
{
    return m_line_delimiters;
}

const std::wstring& DelimitedTextFile::getTextQualifiers() const
{
    return m_text_qualifiers;
}

bool DelimitedTextFile::getSkipCrLf() const
{
    return m_skip_crlf;
}

size_t DelimitedTextFile::getRowCellCount()
{
    return m_row.data.size();
}

xf_off_t DelimitedTextFile::getRowOffset()
{
    return m_row.offset;
}

const std::wstring& DelimitedTextFile::getString(size_t col_idx)
{
    return m_row.getCell(col_idx);
}
    
bool DelimitedTextFile::putString(size_t col_idx, const std::wstring& value)
{
    if (m_text_qualifiers.length() == 0)
    {
        m_row.putCell(col_idx, value);
        return true;
    }
    
    if (value.find(m_text_qualifiers[0]) == value.npos &&
        value.find(m_delimiters[0]) == value.npos)
    {
        m_row.putCell(col_idx, value);
        return true;
    }
    
    size_t value_length = value.length();
    
    wchar_t ch, text_qualifier = 0;
    text_qualifier = m_text_qualifiers[0];
    
    std::wstring quoted_value;
    quoted_value.reserve(value_length);

    if (text_qualifier != 0)
        quoted_value += (char)text_qualifier;
    
    size_t i;
    for (i = 0; i < value_length; ++i)
    {
        ch = value[i];
        quoted_value += (char)ch;
        if (ch == text_qualifier)
            quoted_value += (char)ch;
    }

    if (text_qualifier != 0)
        quoted_value += (char)text_qualifier;

    m_row.putCell(col_idx, quoted_value);
    return true;
}
    
bool DelimitedTextFile::putDateTime(size_t col_idx, const std::wstring& value)
{
    return putString(col_idx, value);
}
    
bool DelimitedTextFile::putDouble(size_t col_idx, double value)
{
    wchar_t buf[255];


    // try to find the smallest number of decimal that
    // represent this double
    bool found = false;
    for (int i = 0; i < 15; ++i)
    {
        swprintf(buf, 255, L"%.*f", i, value);

        if (kl::dblcompare(kl::wtof(buf), value) == 0)
        {
            found = true;
            break;
        }
    }
    
    if (!found)
        swprintf(buf, 255, L"%.16e", value);


    // convert a euro decimal character to a decimal point
    wchar_t* p = buf;
    while (*p)
    {
        if (*p == ',')
            *p = '.';
        ++p;
    }
    
    m_row.putCell(col_idx, buf);
    return true;
}
    
bool DelimitedTextFile::putInteger(size_t col_idx, int value)
{
    wchar_t buf[64];
    swprintf(buf, 64, L"%d", value);
    m_row.putCell(col_idx, buf);
    return true;
}
    
bool DelimitedTextFile::putBoolean(size_t col_idx, bool value)
{
    value ? m_row.putCell(col_idx, L"T") : m_row.putCell(col_idx, L"F");
    return true;
}
    
bool DelimitedTextFile::putNull(size_t col_idx)
{
    return false;
}
   
bool DelimitedTextFile::rewind()
{
    bool retval = m_buf_file->rewind();
    m_offset = 0;
    getRow(m_row);
    return retval;
}

xf_off_t DelimitedTextFile::findLineDelimiter(xf_off_t offset, bool forward)
{
    bool in_quotes = false;
    bool is_outer_quote = false;
    bool is_line_delimiter, is_text_qualifier;
    bool skip_next_char = false;
    wchar_t c;

    int cnt = 0;
    xf_off_t first_line_delimiter = -1;
    
    while (1)
    {
        if (++cnt >= 16384)
        {
            // a line length of 16K or more gets increasingly improbable;
            // return the first line delimiter encountered
            if (first_line_delimiter != -1)
                return first_line_delimiter;
        }
        
        // we've gone past the first character in the file
        if (offset < 0)
            return 0;
            
        // we've gone past the last character in the file
        if (m_buf_file->isPositionEof(offset))
            return offset;
            
        c = m_buf_file->getChar(offset);
        
        
        is_line_delimiter = isLineDelimiter(c);
        is_text_qualifier = isTextQualifier(c);
        
        // record first line delimiter encountered
        if (is_line_delimiter && first_line_delimiter == -1)
        {
            first_line_delimiter = offset;
        }
        
        // these are the only two characters that matter when
        // looking for a line-delimiter
        if (!is_line_delimiter && !is_text_qualifier)
        {
            if (forward)
                offset++;
                 else
                offset--;
            continue;
        }
        
        is_outer_quote = isOffsetTextQualifier(offset, &skip_next_char, in_quotes, forward);
        if (is_outer_quote)
            in_quotes = !in_quotes;
        
        // we've reached a line-delimiter that is
        // outside a quote, so we're at the line end
        if (!in_quotes && is_line_delimiter)
            return offset;
    
        if (skip_next_char)
        {
            if (forward)
                offset++;
                 else
                offset--;
            skip_next_char = false;
        }
            
        if (forward)
            offset++;
             else
            offset--;
    }
    
    return offset;
}

xf_off_t DelimitedTextFile::getPrevRowOffset()
{
    // we can expect that there is going to be a line-delimiter right before
    // the offset we're on, because we're most likely at the beginning of
    // a row, so skip these line-delimiters
    
    xf_off_t offset = m_offset;
    wchar_t c = m_buf_file->getChar(--offset);
    while (isLineDelimiter(c) && offset >= 0)
        c = m_buf_file->getChar(--offset);
    
    offset = findLineDelimiter(offset, false);

    if (offset <= 0)
        return 0;
    
    // move past the line-delimiters to get to the row's offset
    while (isLineDelimiter(m_buf_file->getChar(offset)))
        offset++;

    return offset;
}

xf_off_t DelimitedTextFile::getNextRowOffset()
{
    xf_off_t offset = findLineDelimiter(m_offset, true);

    if (m_buf_file->isPositionEof(offset))
        return offset;
        
    // move past the line-delimiters to get to the row's offset
    while (isLineDelimiter(m_buf_file->getChar(offset)))
        offset++;
        
    return offset;
}

void DelimitedTextFile::skip(int delta)
{
    if (delta == 0)
        return;

    if (delta > 0)
    {
        while (--delta >= 0)
            m_offset = getNextRowOffset();
        
        getRow(m_row);
        return;
    }
    
    if (delta < 0)
    {
        while (++delta <= 0)
            m_offset = getPrevRowOffset();
        
        getRow(m_row);
        return;
    }
}

void DelimitedTextFile::goOffset(xf_off_t offset)
{
    if (offset < 0)
        m_offset = 0;
            
    m_offset = offset;
    
    getRow(m_row);
}

bool DelimitedTextFile::getRow(DelimitedTextRow& output)
{
    // we're at the end of the file
    if (eof())
        return false;
        
    xf_off_t row_width = findLineDelimiter(m_offset, true);
    row_width -= m_offset;
        
    int col_count = 0;
    int vec_size = output.data.size();

    std::wstring s;
    wchar_t c;
    
    bool in_quotes = false;
    bool is_outside_quote = false;
    bool add_cell = false;
    bool skip_char = false;
    
    int i;
    for (i = 0; i < row_width; ++i)
    {
        c = m_buf_file->getChar(m_offset+i);
        
        is_outside_quote = isOffsetTextQualifier(m_offset+i, &skip_char, in_quotes);

        if (in_quotes)
        {
            s += c;
            
            // this character is "grouped" with the next character,
            // so we need to add them both right now and skip the
            // next char for parsing -- this takes care of correct
            // syntax such as "15"" pizza"
            if (skip_char)
            {
                i++;
                s += m_buf_file->getChar(m_offset+i);
                skip_char = false;
            }
             else
            {
                // we have a lone text qualifier inside a quote... this is
                // incorrect syntax ("15" pizza"), and we'll accomodate it
                // by doubling it and making it correct syntax
                if (!is_outside_quote && isTextQualifier(c))
                    s += c;
            }
        }
         else
        {
            // we're outside quotes and have hit a delimiter or a
            // line-delimiter, so we need to add the cell to our row data
            if (isDelimiter(c) || 
                isLineDelimiter(c))
            {
                add_cell = true;
            }
             else
            {
                // we're outside quotes, so just add the character
                s += c;
            }
        }
        
        // if we're at the end of the file, add the cell to our row data
        if (i == row_width-1)
            add_cell = true;
            
        // set the in_quotes flag
        if (is_outside_quote)
        {
            in_quotes = !in_quotes;
        }

        // add the cell to our row data
        if (add_cell)
        {
            // parse the cell's data
            translateString(s);
            
            // either replace an old cell in the vector or add a new one
            if (col_count < vec_size)
            {
                output.data[col_count++] = s;
            }
             else
            {
                output.data.push_back(s);
                col_count++;
            }
            
            // reset our variables for another run
            s = L"";
            add_cell = false;
        }
    }

    // update the DelimitedTextRow's parameters
    output.offset = m_offset;
    output.data.resize(col_count);
    return true;
}

bool DelimitedTextFile::safeIsSpace(wchar_t c)
{
    if (isLineDelimiter(c))
    {
        return false;
    }

    if (!(isDelimiter('\t') || isDelimiter(' ')))
    {
        // since the delimiter is not a tab or a space,
        // it's safe to use isspace()
        return iswspace(c) ? true : false;
    }
     else
    {
        return (iswspace(c) && (c != '\t') && (c != ' '));
    }
}

bool DelimitedTextFile::isDelimiter(wchar_t c)
{
    const wchar_t* p = m_delimiters.c_str();
    while (*p)
    {
        if (*p == c)
            return true;
        p++;
    }

    return false;
}

bool DelimitedTextFile::isLineDelimiter(wchar_t c)
{
    const wchar_t* p = m_line_delimiters.c_str();
    while (*p)
    {
        if (*p == c)
            return true;
        p++;
    }

    return false;
}

bool DelimitedTextFile::isTextQualifier(wchar_t c)
{
    const wchar_t* p = m_text_qualifiers.c_str();
    while (*p)
    {
        if (*p == c)
            return true;
        p++;
    }

    return false;
}

bool DelimitedTextFile::isOffsetTextQualifier(xf_off_t offset,
                                              bool* skip_next_char,
                                              bool in_quotes,
                                              bool forward)
{
    wchar_t c = m_buf_file->getChar(offset);
    
    // we're not even on a text-qualifier character
    if (!isTextQualifier(c))
        return false;

    // we're at the end of the file, so we can't really do any forward checks
    if (offset == 0 || m_buf_file->isPositionEof(offset+1))
        return true;

    // we're in a quote and the next character is a text-qualifier,
    // which is correct syntax (e.g. "15"" pizza"), so we need to
    // skip the next character as well, because it is "grouped"
    // with this character
    wchar_t c2;
    
    if (forward)
        c2 = m_buf_file->getChar(offset+1);
     else
        c2 = m_buf_file->getChar(offset-1);
    
    if (in_quotes && isTextQualifier(c2))
    {
        // this is correct double-quote syntax... (e.g "Embedded ""quote"" string")
        *skip_next_char = true;
        return false;
    }

    if ((in_quotes && forward) ||
        (!in_quotes && !forward))
    {
        // see if the next non-space character
        // is a delimiter or line-delimiter
        c = m_buf_file->getChar(++offset);
        while (safeIsSpace(c))
            c = m_buf_file->getChar(++offset);
    }
     else
    {
        // see if the previous non-space character
        // is a delimiter or line-delimiter
        c = m_buf_file->getChar(--offset);
        while (safeIsSpace(c))
            c = m_buf_file->getChar(--offset);
    }
    
    // 1) if the character is the beginning quote and the previous
    //    non-space character is a delimiter or a line-delimiter   OR
    // 2) if the character is an ending quote and the the next
    //    non-space character is a delimiter or a line-delimiter   THEN
    // the current character is a text-qualifier
    if (isDelimiter(c) || isLineDelimiter(c) || c == ',' || c == ';' || c == '\t' || c == ':' || c == '|' || c == '~')
        return true;
        
    return false;
}

// this function handles all of the string translation necessary...
// i.e. double quotes, embedded commas, etc.
void DelimitedTextFile::translateString(std::wstring& retval)
{
    int length = retval.length();
    const wchar_t* start = retval.c_str();
    const wchar_t* end = start + length - 1;

    if (end < start)
    {
        retval = L"";
        return;
    }

    // get rid of preceeding spaces
    while (iswspace(*start))
    {
        if (start == end)
        {
            retval = L"";
            return;
        }
        
        start++;
    }

    // get rid of trailing spaces
    while (iswspace(*end))
    {
        if (start == end)
        {
            retval = L"";
            return;
        }
            
        end--;
    }

    if (end == start)
    {
        retval.resize(1);
        retval[0] = *start;
        return;
    }

    bool quoted = false;
    int idx = 0;
    const wchar_t* p;

    if (isTextQualifier(*start) && isTextQualifier(*end) && *start == *end)
    {
        quoted = true;
        start++;
        end--;
    }

    p = start;
    while (p <= end)
    {
        if (m_skip_crlf && *p == 0x0d)
        {
            if (*(p+1) == 0x0a)
            {
                p += 2;
            }
        }

        if (isTextQualifier(*p))
        {
            retval[idx] = *p;
            idx++;
            p++;

            if (p > end)
            {
                break;
            }

            // take care of dual quotes
            if (quoted && isTextQualifier(*p))
            {
                p++;
            }

            continue;
        }

        if (*p)
        {
            retval[idx] = *p;
        } 
         else
        {
            retval[idx] = ' ';
        }

        idx++;
        p++;
    }

    retval.resize(idx);
}

 
bool DelimitedTextFile::startInsert()
{
    if (!isOpen())
        return false;
    
    // if we don't have a buffer, create one
    if (!m_buf)
    {
        m_buf = new wchar_t[DELIMITEDTEXT_BUFSIZE];
        for (size_t i = 0; i < DELIMITEDTEXT_BUFSIZE; ++i)
            m_buf[i] = L' ';
    }
    
    m_currow_ptr = m_buf;
        
    m_inserting = true;
    return true;
}
    
bool DelimitedTextFile::finishInsert()
{
    flush();

    m_inserting = false;
    return true;
}
    
bool DelimitedTextFile::insertRow()
{
    if (!m_inserting)
        return false;
        
    size_t i;
    size_t cell_count = m_row.data.size();
    size_t cell_offset = 0;
    size_t cell_length = 0;
        
    size_t buf_loc = (m_currow_ptr-m_buf);
    
    // calculate the binary width needed for the row, including
    // field delimiters and line delimiters
    
    size_t row_length = 0;
    for (i = 0; i < cell_count; ++i)
    {
        row_length += m_row.data[i].length();
        if (i+1 < cell_count)
            row_length += 1; // delimiter
    }
    row_length += 2; // CR LF
    
    
    
    // check if this row would overflow the buffer, and
    // flush the buffer if there's not enough space
    if (buf_loc + row_length > DELIMITEDTEXT_BUFSIZE)
    {
        flush();
    }
    
    for (i = 0; i < cell_count; ++i)
    {
        cell_length = m_row.getCell(i).length();
        
        wchar_t* pos = m_currow_ptr+cell_offset;
        memcpy((void*)pos, m_row.getCell(i).c_str(), cell_length * sizeof(wchar_t));
        
        // increment the cell offset
        cell_offset += cell_length;
        
        if (i+1 < cell_count)
        {
            *(m_currow_ptr+cell_offset) = m_delimiters[0];

            // increment the cell offset
            cell_offset += 1;
        }
    }
    
    // for file format consistency, always use CR LF, even on unix
    *(m_currow_ptr+cell_offset) = 0x0d;
    *(m_currow_ptr+cell_offset+1) = 0x0a;
    cell_offset += 2;
    
    m_currow_ptr += cell_offset;
    return true;
}

bool DelimitedTextFile::flush()
{
    m_buf_file->appendData(m_buf, m_currow_ptr-m_buf);

    // clear out the buffer for the next set of rows
    for (size_t i = 0; i < DELIMITEDTEXT_BUFSIZE; ++i)
        m_buf[i] = L' ';
    m_currow_ptr = m_buf;

    return true;
}
