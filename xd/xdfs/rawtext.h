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
    
    static const long long BUFFER_SIZE;
    static const long long WRITE_BUFFER_SIZE;
    static const long long SEEK_BUFFER_SIZE;
    static const long long FILE_BOF;
    static const long long FILE_EOF;
    
public:

    BufferedTextFile();
    ~BufferedTextFile();
    
    bool open(const std::wstring& filename, int encoding = encodingDefault);
    bool open(xd::IStream* stream, int encoding = encodingDefault);
    bool isOpen() const { return m_stream ? true : false; }
    bool isReadOnly() const { return m_read_only; }
    int getEncoding() const { return m_encoding; }
    xd::IStream* getStream() { return m_stream; }
    void close();
    void rewind();
    
    wchar_t getChar();
    void skip(int delta);
    
    long long getOffset() const;
    void goOffset(long long offset);

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

    xd::IStream* m_stream;
    long long m_data_start_offset;
    long long m_buf_start_offset;
    
    unsigned char* m_buf;
    int m_buf_length;
    int m_buf_offset;
    
    int m_encoding;
    
    unsigned char* m_write_buf;
    int m_write_buf_length;
    
    bool m_read_only;
};



#endif  // __XDFS_RAWTEXT_H




