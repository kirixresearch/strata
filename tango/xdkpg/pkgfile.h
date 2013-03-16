/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-11-27
 *
 */


#ifndef __APP_PKGFILE_H
#define __APP_PKGFILE_H


class PkgFile;
class PkgDirEntry
{
public:

    std::wstring stream_name;
    xf_off_t uncompressed_size;
    xf_off_t first_block_offset;
    bool deleted;

    PkgDirEntry()
    {
        stream_name = L"";
        uncompressed_size = 0;
        first_block_offset = 0;
        deleted = false;
    }

    PkgDirEntry(const PkgDirEntry& c)
    {
        stream_name = c.stream_name;
        uncompressed_size = c.uncompressed_size;
        first_block_offset = c.first_block_offset;
        deleted = c.deleted;
    }

    PkgDirEntry& operator=(const PkgDirEntry& c)
    {
        stream_name = c.stream_name;
        uncompressed_size = c.uncompressed_size;
        first_block_offset = c.first_block_offset;
        deleted = c.deleted;
        
        return *this;
    }
};


class PkgStreamEnum
{
friend class PkgFile;

public:

    PkgStreamEnum();
    ~PkgStreamEnum();

    bool getStreamInfo(const std::wstring& stream_name, PkgDirEntry& entry);
    std::wstring getStreamName(int idx);
    int getStreamCount();

private:
    std::vector<PkgDirEntry> m_entries;
};


class PkgStreamReader
{
friend class PkgFile;

public:

    PkgStreamReader();
    ~PkgStreamReader();
    bool reopen();

    void* loadNextBlock(int* block_size);

private:

    PkgFile* m_pkgfile;
    xf_file_t m_file;
    xf_off_t m_offset;
    unsigned char* m_buf;
    unsigned char* m_temp_buf;

    int m_buf_size;
    int m_temp_buf_size;
};


class PkgStreamWriter
{
friend class PkgFile;

public:

    PkgStreamWriter();
    ~PkgStreamWriter();
    bool reopen();

    void startWrite();
    bool writeBlock(const void* data, int block_size, bool compressed);
    void finishWrite();

private:
    
    // general class members
    PkgFile* m_pkgfile;
    xf_file_t m_file;
    unsigned char* m_buf;
    int m_buf_size;

    // info for the stream's directory entry
    std::wstring m_stream_name;
    xf_off_t m_offset;
    xf_off_t m_written_size;
    xf_off_t m_source_size;
    bool m_writing;

    void allocBuf(int size);
};



class PkgFile
{
friend class PkgStreamWriter;
friend class PkgStreamReader;

public:

    enum
    {
        modeRead = 1,
        modeWrite = 2,
        modeReadWrite = modeRead | modeWrite
    };

public:

    PkgFile();
    ~PkgFile();

    bool create(const std::wstring& filename);
    bool open(const std::wstring& filename);
    bool close();
    bool isOpen();

    int getVersion();
    PkgStreamEnum* getStreamEnum();
    PkgStreamReader* readStream(const std::wstring& stream_name);
    PkgStreamWriter* createStream(const std::wstring& stream_name);

private:

    bool createDirEntry(const PkgDirEntry& entry);
    bool getAllDirEntries(std::vector<PkgDirEntry>& entries);
    bool lookupDirEntry(const std::wstring& stream_name, PkgDirEntry& entry);

private:
    std::wstring m_filename;
    xf_file_t m_file;
    int m_version;
};



#endif
