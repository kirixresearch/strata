/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2007-01-22
 *
 */


#ifndef __XDFS_RAWTEXT_H
#define __XDFS_RAWTEXT_H


#include <kl/file.h>



class BufferedFile
{
private:

    enum
    {
        bfBufSize = 13000
    };
    
    enum
    {
        boDefault = 1,   // default = 8bit
        boUCS2LE = 2,    // UCS-2 little endian
    };

public:

    BufferedFile();
    ~BufferedFile();
    
    bool openFile(const std::wstring& filename,
                  int xf_open_flags = xfReadWrite,
                  int xf_share_flags = xfShareReadWrite);
    void closeFile();
    bool isOpen() const { return (m_file != 0) ? true : false; }
    bool isUnicode();
    
    bool rewind();

    wchar_t getChar(xf_off_t offset, bool* eof = NULL);
    bool isPositionEof(xf_off_t offset);
    
    bool appendData(wchar_t* buf, size_t char_count);
    
private:

    xf_off_t getFileSize();
        
    // getByte is used internally to fetch a byte from the file
    unsigned char getByte(xf_off_t offset, bool* eof = NULL);
    
private:

    std::wstring m_filename;
    xf_file_t m_file;
    xf_off_t m_filesize;
    xf_off_t m_buf_offset;
    unsigned char* m_buf;
    int m_buf_size;
    int m_byte_order;
    int m_skip_bytes;
};








class BufferedTextFile
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
    
    static const xf_off_t BUFFER_SIZE;
    static const xf_off_t WRITE_BUFFER_SIZE;
    static const xf_off_t SEEK_BUFFER_SIZE;
    static const xf_off_t FILE_BOF;
    static const xf_off_t FILE_EOF;
    
public:

    BufferedTextFile();
    ~BufferedTextFile();
    
    bool openFile(const std::wstring& filename, int encoding = encodingDefault);
    bool isOpen() const { return (m_file != 0) ? true : false; }
    bool isReadOnly() const { return m_read_only; }
    int getEncoding() const { return m_encoding; }
    void close();
    void rewind();
    
    wchar_t getChar();
    void skip(int delta);
    
    xf_off_t getOffset() const;
    void goOffset(xf_off_t offset);

    bool bof() const { return (m_buf_offset < 0) ? true : false; }
    bool eof() const { return (m_buf_offset >= m_buf_length) ? true : false; }
    
    bool write(const std::wstring& str);
    
protected:

    inline unsigned char getByte() const
    {
        if (m_buf_offset < 0 || m_buf_offset >= m_buf_length)
            return 0;
             else
            return m_buf[m_buf_offset];
    }
    
    size_t getBytes(unsigned char* p, size_t len);
    
    inline unsigned int getUTF16Char();
    void skipByte(int delta);
    
    int detectEncoding();
    
private:

    xf_file_t m_file;
    xf_off_t m_data_start_offset;
    xf_off_t m_buf_start_offset;
    
    unsigned char* m_buf;
    int m_buf_length;
    int m_buf_offset;
    
    int m_encoding;
    
    unsigned char* m_write_buf;
    int m_write_buf_length;
    
    bool m_read_only;
};



#endif  // __XDFS_RAWTEXT_H




