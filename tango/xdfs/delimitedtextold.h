/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2002-12-11
 *
 */


#ifndef __XDFS_DELIMITEDTEXT_H
#define __XDFS_DELIMITEDTEXT_H


#include <kl/klib.h>


class DelimitedTextRow
{
public:

    void putCell(size_t col_idx, const std::wstring& val)
    {
        // the vector is not large enough for the specified col_idx,
        // so push_back a bunch of dummy values to make it large enough
        while (col_idx > data.size())
        {
            data.push_back(L"");
        }
        
        // the vector is one short of being large enough,
        // so just push_back the value on the vector
        if (col_idx == data.size())
        {
            data.push_back(val);
        }
        
        // the vector is large enough, so just insert the
        // value in the approriate location in the vector
        if (col_idx < data.size())
        {
            data[col_idx] = val;
        }
    }
    
    const std::wstring& getCell(size_t col_idx)
    {
        static std::wstring empty_string;
        if (col_idx >= data.size())
            return empty_string;

        return data[col_idx];
    }
    
public:

    std::vector<std::wstring> data;
    xf_off_t offset;
};


class BufferedFile;
class DelimitedTextFile
{
public:

    DelimitedTextFile();
    ~DelimitedTextFile();

    bool openFile(const std::wstring& filename);
    bool createFile(const std::wstring& filename,
                    const std::vector<std::wstring>& fields);
    bool isOpen();
    void closeFile();
    bool isUnicode();

    const std::wstring& getFilename();
    
    bool bof();
    bool eof();
    
    void setDelimiters(const std::wstring& val);
    void setLineDelimiters(const std::wstring& val);
    void setTextQualifiers(const std::wstring& val);
    void setSkipCrLf(bool val);
    
    const std::wstring& getDelimiters() const;
    const std::wstring& getLineDelimiters() const;
    const std::wstring& getTextQualifiers() const;
    bool getSkipCrLf() const;
    
    size_t getRowCellCount();
    xf_off_t getRowOffset();
    

    const std::wstring& getString(size_t col_idx);

    bool putString(size_t col_idx, const std::wstring& value);
    bool putDateTime(size_t col_idx, const std::wstring& value);
    bool putDouble(size_t col_idx, double value);
    bool putInteger(size_t col_idx, int value);
    bool putBoolean(size_t col_idx, bool value);
    bool putNull(size_t col_idx);

    bool startInsert();     // these functions are used for bulk-inserting
    bool finishInsert();    // rows at the end of a file (most likely for
    bool insertRow();       // file creation).  startInsert() must be called
                            // before insertRow() can be called. Also,
                            // insertRow() should be called after all of the
                            // puts (putString(), etc.) have been done
    bool rewind();
    void skip(int delta);
    void goOffset(xf_off_t offset);
    
    
private:
    
    // finds the next or previous line delimiter
    xf_off_t findLineDelimiter(xf_off_t offset, bool forward);

    xf_off_t getPrevRowOffset();
    xf_off_t getNextRowOffset();
    
    bool safeIsSpace(wchar_t c);
    bool isDelimiter(wchar_t c);
    bool isLineDelimiter(wchar_t c);
    bool isTextQualifier(wchar_t c);
    
    // returns false if EOF has been reached
    bool getRow(DelimitedTextRow& output);

    // figures out if a character is a text-qualifier
    // by looking at the characters that follow it
    bool isOffsetTextQualifier(xf_off_t offset,
                               bool* skip_next_char,
                               bool in_quotes = false,
                               bool forward = true);

    // handles all of the string translation necessary...
    // i.e. double quotes, embedded commas, etc.
    void translateString(std::wstring& retval);
  
    // writes the buffer to the file
    bool flush();
    
private:

    std::wstring m_filename;
    BufferedFile* m_buf_file;       // used for reading files
    
    DelimitedTextRow m_row;         // the row we're on

    xf_off_t m_offset;              // where we're at in the file
    xf_off_t m_filesize;            // size in bytes of the file

    std::wstring m_delimiters;      // characters which defines cells (i.e. comma, tab, etc)
    std::wstring m_line_delimiters; // characters which defines lines (i.e. CR/LF, etc)
    std::wstring m_text_qualifiers; // character which defines a text string (i.e. double-quotes, etc)
    bool m_skip_crlf;               // if this flag is set, completely ignore CR/LF combos

    wchar_t* m_buf;           // buffer for row insertion
    wchar_t* m_currow_ptr;    // current row pointer in the row insertion
    bool m_inserting;               // flag indicating whether startInsert() was called
};


#endif


