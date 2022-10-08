/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-11-27
 *
 */


#include <string>
#include <vector>
#include <kl/file.h>
#include <kl/string.h>
#include <kl/portable.h>
#include "zlib.h"
#include "pkgfile.h"
#include "../xdcommon/util.h"


// ------------------ FILE FORMAT: Package File (.kpg) -----------------
//
// All integers are stored in little-endian format
//
//
// -- file header (1024 bytes) --
// offset   00:   (uint32) signature 0xffaa0011;
// offset   04:   (uint32) version (== 01)
// offset   08:   (uint64) offset of first directory block
// (filler, initialized to zero)
//
// -- directory block --
// offset   00:   (uint32) directory entry count
// offset   04:   (uint64) pointer to the next directory block (or zero if last block)
// offset   12:   (filler, initialized to zero)
// offset   32:   (directory entries)
//
// -- directory entry (512 bytes) --
// offset   00:   (uint32) directory entry flags
// offset   04:   (uint64) total size of the stream data, uncompressed
// offset   12:   (uint64) file offset of the first block
// offset   20:   (filler, initialized to zero)
// offset   32:   (UCS-2 string, zero-terminated, length 128) filename
// offset  288:   (filler, initialized to zero)
//
// -- stream data block format --
// offset   00:   (uint32) block flags
// offset   04:   (uint32) compressed data size (equals zero if end-of-stream)
// offset   08:   (uint32) uncompressed data size (equals zero if end-of-stream)
// offset   12:   (filler, initialized to zero)
// offset   32:   block data begin
//
//
// -- directory entry flags --
// bit 0x0001:    (0 = entry exists; 1 = entry deleted)
//
// -- stream data block flags --
// bit 0x0001:    (0 = block data uncompressed; 1 = block data compressed)




const int kpg_header_size = 1024;
const int kpg_direntry_size = 512;
const int kpg_direntries_per_block = 10;
const int kpg_dirblock_size = (kpg_direntry_size * kpg_direntries_per_block) + 32;






PkgStreamEnum::PkgStreamEnum()
{
}

PkgStreamEnum::~PkgStreamEnum()
{
}

bool PkgStreamEnum::getStreamInfo(const std::wstring& stream_name, PkgDirEntry& entry)
{
    std::vector<PkgDirEntry>::iterator it;
    for (it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        if (kl::iequals(it->stream_name, stream_name))
        {
            entry = *it;
            return true;
        }
    }

    return false;
}


std::wstring PkgStreamEnum::getStreamName(int idx)
{
    return m_entries[idx].stream_name;
}

int PkgStreamEnum::getStreamCount()
{
    return m_entries.size();
}






PkgStreamReader::PkgStreamReader()
{
    m_file = NULL;
    m_pkgfile = NULL;
    m_offset = 0;
    m_buf = NULL;
    m_temp_buf = NULL;
    m_buf_size = 0;
    m_temp_buf_size = 0;
}

PkgStreamReader::~PkgStreamReader()
{
    if (m_buf)
    {
        free(m_buf);
    }

    if (m_temp_buf)
    {
        free(m_temp_buf);
    }

    if (m_file && m_file != m_pkgfile->m_file)
    {
        xf_close(m_file);
    }
}

bool PkgStreamReader::reopen()
{
    xf_file_t f = xf_open(m_pkgfile->m_filename, xfOpen, xfReadWrite, xfShareReadWrite);
    if (!f)
    {
        f = xf_open(m_pkgfile->m_filename, xfOpen, xfRead, xfShareRead);
        if (!f)
        {
            return false;
        }
    }

    if (m_file && m_file != m_pkgfile->m_file)
    {
        xf_close(m_file);
    }
    m_file = f;
    return true;
}

void PkgStreamReader::rewind()
{
    m_offset = m_first_block_offset;
}

void* PkgStreamReader::loadBlockAtOffset(xf_off_t offset, int* block_size)
{
    m_offset = offset;
    return loadNextBlock(block_size);
}

void* PkgStreamReader::loadNextBlock(int* block_size)
{
    unsigned char block_header[32];

    xf_seek(m_file, m_offset, xfSeekSet);
    if (1 != xf_read(m_file, block_header, 32, 1))
    {
        *block_size = 0;
        return NULL;
    }
    m_offset += 32;

    // read in information about this data block

    bool is_compressed = (buf2int(block_header) & 0x01 ? true : false);
    int compressed_block_size = buf2int(block_header+4);
    int uncompressed_block_size = buf2int(block_header+8);


    // check for eof block
    if (compressed_block_size == 0 && uncompressed_block_size == 0)
    {
        // eof
        *block_size = 0;
        return NULL;
    }

    // adjust internal buffer sizes if necessary

    if (uncompressed_block_size > m_buf_size)
    {
        m_buf_size = uncompressed_block_size;
        m_buf = (unsigned char*)realloc(m_buf, m_buf_size);
    }

    if (is_compressed)
    {
        if (compressed_block_size > m_temp_buf_size)
        {
            m_temp_buf_size = compressed_block_size;
            m_temp_buf = (unsigned char*)realloc(m_temp_buf, m_temp_buf_size);
        }
    }


    // read the block data
    unsigned char* read_buf = m_temp_buf;

    if (!is_compressed)
    {
        // block is uncompressed, so just read it into m_buf
        read_buf = m_buf;
    }


    if (1 != xf_read(m_file, read_buf, compressed_block_size, 1))
    {
        *block_size = 0;
        return NULL;
    }
    m_offset += compressed_block_size;

    if (is_compressed)
    {
        // uncompress block data

        unsigned long dest_buf_size = m_buf_size;
        if (Z_OK != uncompress(m_buf, &dest_buf_size, m_temp_buf, compressed_block_size))
        {
            // could not uncompress
            *block_size = 0;
            return NULL;
        }
    }

    if (block_size)
    {
        *block_size = uncompressed_block_size;
    }

    return m_buf;
}






PkgStreamWriter::PkgStreamWriter()
{
    m_file = NULL;
    m_pkgfile = NULL;
    m_buf = NULL;
    m_buf_size = 0;

    m_offset = 0;
    m_source_size = 0;
    m_written_size = 0;

    m_writing = false;
}

PkgStreamWriter::~PkgStreamWriter()
{
    if (m_buf)
    {
        free(m_buf);
    }

    if (m_file && m_file != m_pkgfile->m_file)
    {
        xf_close(m_file);
    }
}


bool PkgStreamWriter::reopen()
{
    xf_file_t f = xf_open(m_pkgfile->m_filename, xfOpen, xfReadWrite, xfShareReadWrite);
    if (!f)
        return false;
    m_file = f;
    return true;
}


void PkgStreamWriter::allocBuf(int size)
{
    if (size > m_buf_size)
    {
        m_buf = (unsigned char*)realloc(m_buf, size);
        m_buf_size = size;
    }
}


void PkgStreamWriter::startWrite()
{
    m_writing = true;
}

bool PkgStreamWriter::writeBlock(const void* data, int block_size, bool compressed)
{
    if (!m_writing)
    {
        // must start a writing block with startWrite() and finish it
        // with finishWrite()
    
        return false;
    }
    
        
    // make sure we have enough space for our data, compressed or not
    allocBuf(block_size+(block_size/10)+100);

    // first 32 bytes are used for block meta-data, initialize to zeros
    memset(m_buf, 0, 32);

    int write_size = 0;

    if (compressed)
    {
        unsigned long buflen = m_buf_size-32;
        if (Z_OK == compress2((Bytef*)(m_buf+32), &buflen, (Bytef*)data, block_size, 9 /*(max compression)*/))
        {
            write_size = buflen;
        }
         else
        {
            compressed = false;
        }
    }

    if (!compressed)
    {
        write_size = block_size;
        memcpy(m_buf+32, data, block_size);
    }


    int2buf(m_buf, compressed ? 0x01 : 0x00);
    int2buf(m_buf+4, write_size);
    int2buf(m_buf+8, block_size);

    xf_seek(m_file, 0, xfSeekEnd);

    if (m_offset == 0)
    {
        m_offset = xf_get_file_pos(m_file);
    }

    if (1 != xf_write(m_file, m_buf, write_size+32, 1))
    {
        // write failed
        return false;
    }

    m_source_size += block_size;
    m_written_size += write_size;

    return true;
}


void PkgStreamWriter::finishWrite()
{
    // write eof marker for this stream
    unsigned char eof[32];
    memset(eof, 0, 32);
    xf_write(m_file, eof, 32, 1);

    // write directory entry
    PkgDirEntry e;
    e.deleted = false;
    e.stream_name = m_stream_name;
    e.uncompressed_size = m_source_size;
    e.first_block_offset = m_offset;
    
    m_pkgfile->createDirEntry(e);

    m_writing = false;
}




PkgFile::PkgFile()
{
    m_file = NULL;
    m_version = 2;   // current version is 2
}

PkgFile::~PkgFile()
{
    if (isOpen())
    {
        close();
    }
}


bool PkgFile::create(const std::wstring& filename)
{
    // if the file is already open, close it

    if (isOpen())
    {
        close();
    }


    // attempt to open the file

    m_file = xf_open(filename, xfCreate, xfReadWrite, xfShareReadWrite);
    if (!m_file)
        return false;

    m_filename = filename;

    // create file header
    unsigned char* header = new unsigned char[kpg_header_size];
    memset(header, 0, kpg_header_size);

    // kpg file signature
    int2buf(header+0, 0xffaa0011);

    // file version
    int2buf(header+4, m_version);

    // offset of first directory block
    int2buf(header+8, 1024); // lower 32 bits


    if (1 != xf_write(m_file, header, kpg_header_size, 1))
    {
        delete[] header;
        xf_close(m_file);
        xf_remove(filename);
        m_filename = L"";
        return false;
    }

    delete[] header;


    // now write an empty directory block
    unsigned char* dir_block = new unsigned char[kpg_dirblock_size];
    memset(dir_block, 0, kpg_dirblock_size);
    
    if (1 != xf_write(m_file, dir_block, kpg_dirblock_size, 1))
    {
        delete[] dir_block;
        xf_close(m_file);
        xf_remove(filename);
        return false;
    }

    delete[] dir_block;

    return true;
}


bool PkgFile::open(const std::wstring& filename)
{
    // if the file is already open, close it

    if (isOpen())
    {
        close();
    }


    // attempt to open the file

    int mode_flags = xfReadWrite;
    int share_flags = xfShareReadWrite;

    m_file = xf_open(filename, xfOpen, mode_flags, share_flags);
    if (!m_file)
    {
        mode_flags = xfRead;
        share_flags = xfShareRead;
        m_file = xf_open(filename, xfOpen, mode_flags, share_flags);
        if (!m_file)
        {
            return false;
        }
    }

    m_filename = filename;

    unsigned char* header = new unsigned char[kpg_header_size];
    xf_seek(m_file, 0, xfSeekSet);
    
    if (1 != xf_read(m_file, header, kpg_header_size, 1))
    {
        delete[] header;
        return false;
    }
    
    // couldn't find package file signature, bail out
    if (buf2int(header+0) != 0xffaa0011)
    {
        delete[] header;
        return false;
    }
    
    // get offset of first directory block
    m_version = buf2int(header+4);

    delete[] header;
    
    if (m_version > 2)
        return false;

    return true;
}

int PkgFile::getVersion()
{
    return m_version;
}

bool PkgFile::close()
{
    if (m_file)
        xf_close(m_file);
    m_file = NULL;
    m_filename = L"";
    return false;
}

bool PkgFile::isOpen()
{
    return (m_file != NULL);
}



bool PkgFile::createDirEntry(const PkgDirEntry& entry)
{
    unsigned char* header = new unsigned char[kpg_header_size];
    unsigned char* dir_block = new unsigned char[kpg_dirblock_size];
    unsigned char* dir_entry = new unsigned char[kpg_direntry_size];

    memset(dir_entry, 0, kpg_direntry_size);

    xf_off_t hi, lo;

    // set block flags
    int2buf(dir_entry, entry.deleted ? 0x01 : 0x00);

    // set total stream size (uncompressed)
    hi = entry.uncompressed_size >> 32;
    lo = entry.uncompressed_size & 0xffffffff;

    int2buf(dir_entry+4, (unsigned int)lo);
    int2buf(dir_entry+8, (unsigned int)hi);

    // set file offset of the stream
    hi = entry.first_block_offset >> 32;
    lo = entry.first_block_offset & 0xffffffff;

    int2buf(dir_entry+12, (unsigned int)lo);
    int2buf(dir_entry+16, (unsigned int)hi);

    // write out little-endian UCS-2 stream name
    int i, len;
    len = entry.stream_name.length();
    if (len > 127) len = 127;
    unsigned char* ptr = dir_entry+32;
    for (i = 0; i < len; ++i)
    {
        wchar_t ch = entry.stream_name[i];
        *ptr = ch & 0xff;
        ptr++;
        *ptr = (ch >> 8) & 0xff;
        ptr++;
    }


    // find the next free directory entry
    xf_off_t dir_block_offset, new_offset;
    int dir_entry_count;

    xf_seek(m_file, 0, xfSeekSet);
    
    if (1 != xf_read(m_file, header, kpg_header_size, 1))
    {
        delete[] header;
        delete[] dir_block;
        delete[] dir_entry;
        return false;
    }


    // get offset of first directory block
    lo = (unsigned int)buf2int(header+8);
    hi = (unsigned int)buf2int(header+12);
    dir_block_offset = (hi << 32) | lo;

    while (1)
    {
        xf_seek(m_file, dir_block_offset, xfSeekSet);
        if (1 != xf_read(m_file, dir_block, kpg_dirblock_size, 1))
        {
            delete[] header;
            delete[] dir_block;
            delete[] dir_entry;
            return false;
        }

        dir_entry_count = buf2int(dir_block);

        if (dir_entry_count < kpg_direntries_per_block)
        {
            bool success = true;

            memcpy(dir_block+32+(dir_entry_count*kpg_direntry_size),
                   dir_entry, kpg_direntry_size);

            dir_entry_count++;
            int2buf(dir_block, dir_entry_count);

            xf_seek(m_file, dir_block_offset, xfSeekSet);
            if (1 != xf_write(m_file, dir_block, kpg_dirblock_size, 1))
            {
                success = false;
            }

            delete[] header;
            delete[] dir_block;
            delete[] dir_entry;

            return success;
        }
         else
        {
            // see if this is the last block
            lo = buf2int(dir_block+4);
            hi = buf2int(dir_block+8);
            new_offset = (hi << 32) | lo;
            
            if (new_offset != 0)
            {
                // go to the next dir block
                dir_block_offset = new_offset;
                continue;
            }

            // this is the last dir block in the file, and it's full.
            // we must therefore create a new entry at the end of the file

            unsigned char* new_block = new unsigned char[kpg_dirblock_size];
            memset(new_block, 0, kpg_dirblock_size);

            // set entry count
            int2buf(new_block, 1);

            // set next dirblock offset to zero
            int2buf(new_block+4, 0);
            int2buf(new_block+8, 0);

            memcpy(new_block+32, dir_entry, kpg_direntry_size);

            xf_seek(m_file, 0, xfSeekEnd);
            new_offset = xf_get_file_pos(m_file);

            if (1 != xf_write(m_file, new_block, kpg_dirblock_size, 1))
            {
                delete[] header;
                delete[] dir_block;
                delete[] dir_entry;
                delete[] new_block;
                return false;
            }

            delete[] new_block;


            // now write the new dirblock's offset in the previous block in
            // the dirblock chain

            lo = new_offset & 0xffffffff;
            hi = new_offset >> 32;
            int2buf(dir_block+4, (unsigned int)lo);
            int2buf(dir_block+8, (unsigned int)hi);

            xf_seek(m_file, dir_block_offset, xfSeekSet);
            xf_write(m_file, dir_block, kpg_dirblock_size, 1);

            delete[] header;
            delete[] dir_block;
            delete[] dir_entry;

            return true;
        }
    }
}


bool PkgFile::lookupDirEntry(const std::wstring& stream_name, PkgDirEntry& entry)
{
    std::vector<PkgDirEntry> entries;
    if (!getAllDirEntries(entries))
    {
        return false;
    }

    std::vector<PkgDirEntry>::iterator it;
    for (it = entries.begin(); it != entries.end(); ++it)
    {
        if (kl::iequals(it->stream_name, stream_name))
        {
            entry = *it;
            return true;
        }
    }

    return false;
}

bool PkgFile::getAllDirEntries(std::vector<PkgDirEntry>& entries)
{
    unsigned char* header = new unsigned char[kpg_header_size];
    unsigned char* dir_block = new unsigned char[kpg_dirblock_size];
    unsigned char* dir_entry = new unsigned char[kpg_direntry_size];

    xf_off_t lo, hi;
    int tempi;

    // find the next free directory entry
    xf_off_t dir_block_offset;
    int i, dir_entry_count;

    xf_seek(m_file, 0, xfSeekSet);
    
    if (1 != xf_read(m_file, header, kpg_header_size, 1))
    {
        delete[] header;
        delete[] dir_block;
        delete[] dir_entry;
        return false;
    }
    


    // get offset of first directory block
    lo = buf2int(header+8);
    hi = buf2int(header+12);
    dir_block_offset = (hi << 32) | lo;



    PkgDirEntry entry;

    while (1)
    {
        xf_seek(m_file, dir_block_offset, xfSeekSet);

        if (1 != xf_read(m_file, dir_block, kpg_dirblock_size, 1))
        {
            delete[] header;
            delete[] dir_block;
            delete[] dir_entry;
            return false;
        }

        dir_entry_count = buf2int(dir_block);

        for (i = 0; i < dir_entry_count; ++i)
        {
            memcpy(dir_entry, dir_block+32+(i*kpg_direntry_size), kpg_direntry_size);
            
            tempi = buf2int(dir_entry);
            if (tempi & 0x01)
            {
                // this entry is deleted, so skip it
                continue;
            }

            // read little-endian UCS-2 stream name
            entry.stream_name = L"";
            unsigned char* ptr = dir_entry+32;
            while (1)
            {
                wchar_t ch;
                ch = *ptr;
                ptr++;
                ch |= (((wchar_t)*ptr) << 8);
                ptr++;
                if (ch == 0)
                    break;
                entry.stream_name += ch;
            }


            entry.directory_entry_offset = dir_block_offset + 32 + (i*kpg_direntry_size);
            entry.deleted = false;

            lo = (unsigned int)buf2int(dir_entry+4);
            hi = (unsigned int)buf2int(dir_entry+8);
            entry.uncompressed_size = (hi << 32) | lo;

            lo = (unsigned int)buf2int(dir_entry+12);
            hi = (unsigned int)buf2int(dir_entry+16);
            entry.first_block_offset = (hi << 32) | lo;

            entries.push_back(entry);
        }

        lo = (unsigned int)buf2int(dir_block+4);
        hi = (unsigned int)buf2int(dir_block+8);
        dir_block_offset = (hi << 32) | lo;

        if (dir_block_offset == 0)
        {
            // we are at the end of the dirblock chain, so we are done
            break;
        }
    }

    delete[] header;
    delete[] dir_block;
    delete[] dir_entry;

    return true;
}





PkgStreamEnum* PkgFile::getStreamEnum()
{
    PkgStreamEnum* stream_enum = new PkgStreamEnum;

    if (!getAllDirEntries(stream_enum->m_entries))
    {
        delete stream_enum;
        return NULL;
    }

    return stream_enum;
}


PkgStreamReader* PkgFile::readStream(const std::wstring& stream_name)
{
    PkgDirEntry entry;

    if (!lookupDirEntry(stream_name, entry))
    {
        return NULL;
    }

    PkgStreamReader* reader = new PkgStreamReader;
    reader->m_file = m_file;
    reader->m_pkgfile = this;
    reader->m_first_block_offset = entry.first_block_offset;
    reader->m_offset = entry.first_block_offset;
    return reader;
}

PkgStreamWriter* PkgFile::createStream(const std::wstring& stream_name)
{
    PkgStreamWriter* writer = new PkgStreamWriter;
    writer->m_file = m_file;
    writer->m_pkgfile = this;
    writer->m_stream_name = stream_name;
    return writer;
}

bool PkgFile::deleteStream(const std::wstring& stream_name)
{
    std::vector<PkgDirEntry> entries;
    std::vector<PkgDirEntry>::iterator it;

    if (!getAllDirEntries(entries))
        return false;

    for (it = entries.begin(); it != entries.end(); ++it)
    {
        if (!it->deleted && kl::iequals(it->stream_name, stream_name))
        {
            unsigned char buf[1];
            buf[0] = 0x01;

            xf_seek(m_file, it->directory_entry_offset, xfSeekSet);
            return (xf_write(m_file, buf, 1, 1) == 1 ? true : false);
        }
    }

    return false;
}


bool PkgFile::renameStream(const std::wstring& stream_name, const std::wstring& new_name)
{
    std::vector<PkgDirEntry> entries;
    std::vector<PkgDirEntry>::iterator it;

    if (!getAllDirEntries(entries))
        return false;

    // see if the new name for the stream is already taken by
    // an existing one
    for (it = entries.begin(); it != entries.end(); ++it)
    {
        if (!it->deleted && kl::iequals(it->stream_name, new_name))
            return false;   // new name already exists
    }

    // rename stream
    for (it = entries.begin(); it != entries.end(); ++it)
    {
        if (!it->deleted && kl::iequals(it->stream_name, stream_name))
        {
            unsigned char buf[kpg_direntry_size];
            xf_seek(m_file, it->directory_entry_offset, xfSeekSet);
            if (kpg_direntry_size != xf_read(m_file, buf, 1, kpg_direntry_size))
                return false;

            // update the name
            unsigned char* ptr = buf+32;
            memset(ptr, 0, 256);
            int i, len;
            len = new_name.length();
            if (len > 127) len = 127;
            for (i = 0; i < len; ++i)
            {
                wchar_t ch = new_name[i];
                *ptr = ch & 0xff;
                ptr++;
                *ptr = (ch >> 8) & 0xff;
                ptr++;
            }

            return (xf_write(m_file, buf, 1, kpg_direntry_size) == kpg_direntry_size ? true : false);
        }
    }

    return false;
}

