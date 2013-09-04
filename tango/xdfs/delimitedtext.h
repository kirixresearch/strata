/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-12-11
 *
 */


#ifndef __XDFS_DELIMITEDTEXT_H
#define __XDFS_DELIMITEDTEXT_H


#include <kl/klib.h>
#include "rawtext.h"



class DelimitedTextRow
{
friend class DelimitedTextFile;

public:

    void appendValue(const std::wstring& val)
    {
        data.push_back(val);
    }
    
    bool putValue(size_t col_idx, const std::wstring& val)
    {
        // the vector is not large enough for the specified col_idx,
        // so push_back a bunch of dummy values to make it large enough
        while (col_idx > data.size())
            data.push_back(L"");
        
        if (col_idx == data.size())
        {
            // the vector is one short of being large enough,
            // so just push_back the value on the vector
            data.push_back(val);
        }
         else
        {
            // the vector is large enough, so just insert the
            // value in the approriate location in the vector
            data[col_idx] = val;
        }
        
        return true;
    }
    
    bool putString(size_t col_idx, const std::wstring& val)   { return putValue(col_idx, val); }
    bool putDateTime(size_t col_idx, const std::wstring& val) { return putValue(col_idx, val); }
    bool putDouble(size_t col_idx, double val);
    bool putInteger(size_t col_idx, int val);
    bool putBoolean(size_t col_idx, bool val)                 { return putValue(col_idx, val ? L"T" : L"F"); }
    bool putNull(size_t col_idx)                              { return putValue(col_idx, L""); }

    const std::wstring& getValue(size_t col_idx) const
    {
        static std::wstring empty_string;
        if (col_idx >= data.size())
            return empty_string;
        return data[col_idx];
    }
    
    const size_t getValueCount() const { return data.size(); }
    
    void clear() { data.clear(); }
    
private:

    std::vector<std::wstring> data;
    xf_off_t offset;
};


class DelimitedTextFile
{
public:

    enum
    {
        encodingDefault = 0,
        encodingISO88591 = 1,
        encodingUTF8 = 2,
        encodingUTF16LE = 3,
        encodingUTF16BE = 4
    };
    
public:

    DelimitedTextFile();
    ~DelimitedTextFile();

    bool openFile(const std::wstring& filename,
                  int encoding = encodingDefault);

    bool createFile(const std::wstring& filename,
                    const std::vector<std::wstring>& fields,
                    int encoding = encodingUTF8);
    
    bool isOpen() const;
    void closeFile();
    bool isUnicode();

    const std::wstring& getFilename();
    
    bool bof() const { return m_bof; }
    bool eof() const { return m_eof; }
    
    void setDelimiters(const std::wstring& val);
    void setLineDelimiters(const std::wstring& val);
    void setTextQualifiers(const std::wstring& val);
    
    const std::wstring& getDelimiters() const { return m_delimiters; }
    const std::wstring& getLineDelimiters() const { return m_line_delimiters; }
    const std::wstring& getTextQualifiers() const { return m_text_qualifiers; }
    
    size_t getRowCellCount() const { return m_row.getValueCount(); }
    xf_off_t getRowOffset() const { return m_row.offset; }

    const std::wstring& getString(size_t col_idx) const { return m_row.getValue(col_idx); }
    const DelimitedTextRow& getRow() { return m_row; }
    
    bool startInsert();
    bool finishInsert();
    bool insertRow(const DelimitedTextRow& row);
    bool flush();

    bool rewind();
    void skip(int delta);
    void goOffset(xf_off_t offset);
    double getPos() const;
    
private:

    void skipOne(DelimitedTextRow* row, int direction = 1);
    
    bool safeIsSpace(wchar_t c) const;
    bool isDelimiter(wchar_t c) const;
    bool isLineDelimiter(wchar_t c) const;
    bool isTextQualifier(wchar_t c) const;
    
    // handles all of the string translation necessary...
    // i.e. double quotes, embedded commas, etc.
    void translateString(std::wstring& retval) const;

private:

    BufferedTextFile m_file;
    std::wstring m_filename;
    
    std::wstring m_delimiters;      // characters which defines cells (i.e. comma, tab, etc)
    std::wstring m_line_delimiters; // characters which defines lines (i.e. CR/LF, etc)
    std::wstring m_text_qualifiers; // character which defines a text string (i.e. double-quotes, etc)
    
    DelimitedTextRow m_row;         // current row
    
    /*
    // zero-indexed row position of m_file's file ptr
    #ifdef _MSC_VER
    __int64 m_fileptr_row;
    #else
    long long m_fileptr_row;
    #endif
    */
    
    bool m_eof;
    bool m_bof;
    
    std::wstring m_insert_buf;
};


#endif
