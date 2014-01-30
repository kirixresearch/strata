/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-12-11
 *
 */

#include "xdfs.h"
#include "delimitedtext.h"
#include "rawtext.h"





bool DelimitedTextRow::putDouble(size_t col_idx, double value)
{
    wchar_t buf[80];
    int i;
    
    // try to find the smallest number of decimal that
    // represent this double
    bool found = false;
    for (i = 0; i < 15; ++i)
    {
        swprintf(buf, 80, L"%.*f", i, value);

        if (kl::dblcompare(kl::wtof(buf), value) == 0)
        {
            found = true;
            break;
        }
    }
    
    if (!found)
        swprintf(buf, 80, L"%.16e", value);

    // convert a euro decimal character to a decimal point
    wchar_t* p = buf;
    while (*p)
    {
        if (*p == ',')
            *p = '.';
        ++p;
    }
    
    return putValue(col_idx, buf);
}

bool DelimitedTextRow::putInteger(size_t col_idx, int value)
{
    wchar_t buf[32];
    swprintf(buf, 32, L"%d", value);
    return putValue(col_idx, buf);
}




// -- DelimitedTextFile class implementation --

DelimitedTextFile::DelimitedTextFile()
{
    m_delimiters = L",";
    m_line_delimiters = L"\n";
    m_text_qualifiers = L"\"";
    m_eof = false;
    m_bof = false;
}

DelimitedTextFile::~DelimitedTextFile()
{
    if (isOpen())
        closeFile();
}

bool DelimitedTextFile::openFile(const std::wstring& filename, int encoding)
{
    closeFile();
    
    int f_encoding;
    switch (encoding)
    {
        case encodingDefault:   f_encoding = BufferedTextFile::encodingDefault;   break;
        case encodingISO88591:  f_encoding = BufferedTextFile::encodingISO88591;  break;
        case encodingUTF8:      f_encoding = BufferedTextFile::encodingUTF8;      break;
        case encodingUTF16LE:   f_encoding = BufferedTextFile::encodingUTF16LE;   break;
        case encodingUTF16BE:   f_encoding = BufferedTextFile::encodingUTF16BE;   break;
        default:
            // unknown encoding
            return false;
    }
    
    if (m_file.openFile(filename, f_encoding))
    {
        m_filename = filename;
        m_eof = false;
        m_bof = false;
        m_row.clear();

        skip(1);
        return true;
    }
     else
    {
        return false;
    }
}

bool DelimitedTextFile::createFile(const std::wstring& filename,
                                   const std::vector<std::wstring>& fields,
                                   int encoding)
{
    // try to open the file
    xf_file_t f = xf_open(filename, xfCreate, xfReadWrite, xfShareNone);
    if (f == NULL)
        return false;

    unsigned char bom[4];
    int bom_length = 0;
    
    if (encoding == encodingUTF8)
    {
        bom_length = 3;
        bom[0] = 0xef;
        bom[1] = 0xbb;
        bom[2] = 0xbf;
    }
     else if (encoding == encodingUTF16LE)
    {
        bom_length = 2;
        bom[0] = 0xff;
        bom[1] = 0xfe;
    }
     else if (encoding == encodingUTF16BE)
    {
        bom_length = 2;
        bom[0] = 0xfe;
        bom[1] = 0xff;
    }
    
    if (bom_length > 0)
    {
        if (bom_length != xf_write(f, bom, 1, bom_length))
        {
            xf_close(f);
            xf_remove(filename);
            return false;
        }
    }

    xf_close(f);
    
    
    if (!openFile(filename))
        return false;
    
    if (fields.size() > 0)
    {
        startInsert();
        
        DelimitedTextRow row;
        row.data = fields;
        insertRow(row);
        
        finishInsert();
    }
    

    return true;
}

bool DelimitedTextFile::isOpen() const
{
    return m_file.isOpen();
}

void DelimitedTextFile::closeFile()
{
    m_file.close();
}

bool DelimitedTextFile::isUnicode()
{
    return (m_file.getEncoding() == BufferedTextFile::encodingDefault) ? false : true;
}

const std::wstring& DelimitedTextFile::getFilename()
{
    return m_filename;
}

void DelimitedTextFile::setDelimiters(const std::wstring& val)
{
    m_delimiters = val;
}

void DelimitedTextFile::setLineDelimiters(const std::wstring& val)
{
    m_line_delimiters = val;
    if (m_line_delimiters == L"\x0d" ||
        m_line_delimiters == L"\x0a" ||
        m_line_delimiters == L"\x0d\x0a" ||
        m_line_delimiters == L"\x0a\x0d")
    {
        m_line_delimiters = L"\n";
    }
    
    if (m_line_delimiters.empty())
    {
        m_line_delimiters = L"\n";
    }
}

void DelimitedTextFile::setTextQualifiers(const std::wstring& val)
{
    m_text_qualifiers = val;
}
 
bool DelimitedTextFile::rewind()
{
    if (!isOpen())
        return false;
    
    m_file.rewind();
    skip(1);
    
    return true;
}

void DelimitedTextFile::skip(int delta)
{
    if (delta == 0)
        return;
     
    int i;
    
    if (delta > 0)
    {
        for (i = 0; i < delta; ++i)
            skipOne(&m_row, 1);
    }
     else
    {
        if (m_eof)
        {
            m_eof = false;
        }
         else
        {
            skipOne(NULL, -1);
        }
        
        for (i = 0; i < -delta; ++i)
        {
            skipOne(NULL, -1);
            if (bof())
            {
                m_row.clear();
                return;
            }
        }
        
        skipOne(&m_row, 1);
    }
}


void DelimitedTextFile::skipOne(DelimitedTextRow* row, int direction)
{    
    wchar_t ch, next_ch, last_sig_ch;
 
    std::wstring value;
        
    wchar_t quote_char = 0;
    bool is_delim = false;
    bool is_quote = false;
    
    if (row) row->clear();
    
    m_eof = false;
    m_bof = false;
    
    if (direction < 0)
    {
        // scroll back before last delimiter, which is
        // immediately to the left of our file pointer
        while (true)
        {
            m_file.skip(-1);
            if (m_file.bof())
            {
                // bof condition
                m_bof = true;
                return;
            }
            ch = m_file.getChar();
            if (!isLineDelimiter(ch))
                break;
        }
    }
    
    if (direction > 0)
    {
        if (m_file.bof())
        {
            // position to first byte of file
            m_file.skip(1);
        }
        
        if (m_file.eof())
        {
            // eof condition
            m_eof = true;
            return;
        }
    }
    
    if (row)
        row->offset = m_file.getOffset();
    
    ch = m_line_delimiters[0];
    
    while (true)
    {
        if (direction > 0 && m_file.eof())
            break;
        if (direction < 0 && m_file.bof())
        {
            m_file.skip(1);
            break;
        }
        
        if (value.length() > 16384)
        {
            // probably a mal-formed file; in any case we need to
            // reset the value to avoid a run-on file (one big field)
            quote_char = 0;
            value = L"";
        }

        if (ch != ' ')
            last_sig_ch = ch;
        
        ch = m_file.getChar();
        
        if (!quote_char && isLineDelimiter(ch))
        {
            while (isLineDelimiter(ch))
            {
                m_file.skip(1);
                ch = m_file.getChar();
            }
            
            // done
            break;
        }
        
        is_quote = isTextQualifier(ch);
        is_delim = isDelimiter(ch);
        
        if (is_delim && !quote_char)
        {
            if (row)
            {
                row->appendValue(value);
                value = L"";
            }
            
            m_file.skip(direction);
            continue;
        }
        
        if (row)
            value += ch;
        
        if (!is_quote && !is_delim)
        {
            m_file.skip(direction);
            continue;
        }
        
        if (is_quote)
        {
            if (quote_char && quote_char == ch)
            {
                m_file.skip(direction);
                if (m_file.eof())
                    break;
                
                next_ch = m_file.getChar();
                
                if (next_ch == quote_char)
                {
                    // escaped quote
                    if (row)
                        value += next_ch;
                    m_file.skip(direction);
                }
                 else if (!isDelimiter(next_ch) && !isLineDelimiter(next_ch) && next_ch != '\r')
                {
                    // malformed embedded quote, escape it
                    if (row)
                        value += quote_char;
                }
                 else
                {
                    // terminating quote
                    quote_char = 0;
                }
                
                continue;
            }
            
            if (!quote_char && (isDelimiter(last_sig_ch) || isLineDelimiter(last_sig_ch)))
                quote_char = ch;
        }


        m_file.skip(direction);
    }
    
    
    if (row)
    {
        row->appendValue(value);
        
        size_t i, cnt=row->data.size();
        for (i = 0; i < cnt; ++i)
            translateString(row->data[i]);
    }
}


void DelimitedTextFile::goOffset(xf_off_t offset)
{
    m_file.goOffset(offset);
    skip(1);
}


double DelimitedTextFile::getPos() const
{
    double offset = (double)m_file.getOffset();
    double file_size = (double)xf_get_file_size(m_filename);

    if (kl::dblcompare(file_size, 0.0) == 0)
        return 0.0;

    return (offset / file_size);
}

bool DelimitedTextFile::isDelimiter(wchar_t c) const
{
    return (m_delimiters.find(c) != m_delimiters.npos) ? true : false;
}

bool DelimitedTextFile::isLineDelimiter(wchar_t c) const
{
    return (m_line_delimiters.find(c) != m_line_delimiters.npos) ? true : false;
}

bool DelimitedTextFile::isTextQualifier(wchar_t c) const
{
    return (m_text_qualifiers.find(c) != m_text_qualifiers.npos) ? true : false;
}


void DelimitedTextFile::translateString(std::wstring& retval) const
{
    size_t len = retval.length();
    if (len == 0)
        return;
    if (retval[len-1] == '\r')
        retval.resize(len-1);
    if (len == 1)
        return;
    
    if (!isTextQualifier(retval[0]))
    {
        // not a quoted string, leave it
        return;
    }

    const wchar_t* start = retval.c_str();
    const wchar_t* end = start + retval.length() - 1;
    const wchar_t* p;
    wchar_t quote_char = 0;
    
    int idx = 0;
    p = start;
    
    while (p <= end)
    {
        if (isTextQualifier(*p))
        {
            if (quote_char && quote_char == *p)
            {
                // take care of dual quotes
                if (p+1 <= end && isTextQualifier(*(p+1)))
                {
                    retval[idx] = *p;
                    idx++;
                    
                    p += 2;
                }
                 else
                {
                    quote_char = 0;
                    p++;
                }

                continue;
            }
            
            if (!quote_char)
            {
                quote_char = *p;
                p++;
                continue;
            }
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




const int DELIMITED_TEXT_INSERTBUF_LENGTH = 500000;

bool DelimitedTextFile::startInsert()
{
    m_insert_buf.reserve(500000);
    
    return m_file.isReadOnly() ? false : true;
}
    
bool DelimitedTextFile::finishInsert()
{
    flush();
    
    m_insert_buf.reserve(10);
    m_insert_buf = L"";
    
    return true;
}
    
bool DelimitedTextFile::insertRow(const DelimitedTextRow& row)
{
    if (m_file.isReadOnly())
        return false;

    wchar_t delimiter = ',';
    wchar_t quote = 0;
    if (m_delimiters.length() > 0)
        delimiter = m_delimiters[0];
    if (m_text_qualifiers.length() > 0)
        quote = m_text_qualifiers[0];

    size_t i, cnt = row.getValueCount();
    for (i = 0; i < cnt; ++i)
    {
        if (i > 0)
            m_insert_buf += delimiter;
        
        const std::wstring& value = row.getValue(i);
        if (quote != 0 && (value.find(delimiter) != value.npos || value.find(quote) != value.npos))
        {
            std::wstring quote_search, quote_replace;
            quote_search = quote;
            quote_replace = quote;
            quote_replace += quote;

            std::wstring v = value;
            kl::replaceStr(v, quote_search, quote_replace);
            m_insert_buf += quote;
            m_insert_buf += v;
            m_insert_buf += quote;
        }
         else
        {
            m_insert_buf += value;
        }
    }
    
    m_insert_buf += L"\r\n";
    
    if (m_insert_buf.length() >= 500000)
        flush();
    
    return true;
}

bool DelimitedTextFile::flush()
{
    if (m_insert_buf.length() == 0)
        return true;
    
    if (m_file.isReadOnly())
        return false;
    
    m_file.write(m_insert_buf);
    
    m_insert_buf = L"";
    m_insert_buf.reserve(500000);
    
    return true;
}
