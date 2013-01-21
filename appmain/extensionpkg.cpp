/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-07-23
 *
 */


#include "appmain.h"
#include "extensionpkg.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/mstream.h>
#include <wx/zipstrm.h>
#include <kl/crypt.h>


const int ENCFILE_BLOCK_SIZE = 1024;     // must be at least 16 bytes greater than data per block
const int ENCFILE_DATA_PER_BLOCK = 1000;


#define ENC_KEY L"Vj%82lk0@1zVnqG%aM3dF!7a"


// encrypted file format
//
// offset   length   description
// -----------------------------
//    0     4096     zero (reserved)
// 4096        4     version stamp "5b ab 91 12"
// 5000        4     version number (1)
// 5004        8     64-bit LE integer - data length
// 5012        4     32-bit LE integer - block size
// 5016        4     32-bit LE integer - data per block
// 5020        4     32-bit LE integer - data start offset
// (zeroed)
// 8192              normal first block
//
// notes - block size must be 24 bytes greater than
// "data per block".  Example: if the data stored in each
// block is 1000 bytes, the block size will have to be 1024
// Block size must be a multiple of 8.
// The first block must start at the earliest at offset 8192

class WXDLLIMPEXP_BASE wxEncryptedInputStream : public wxInputStream
{
public:

    wxEncryptedInputStream(const wxString& filename, const wxString& key)
    {
        m_buf = NULL;
        m_unenc_buf = NULL;
        m_file = 0;
        m_offset = 0;
        m_file_size = 0;
        m_data_per_block = 0;
        m_block_size = 0;
        m_header_size = 0;
        m_key = towstr(key);

        Open(filename);
    }
    
    virtual ~wxEncryptedInputStream()
    {
        delete[] m_unenc_buf;
        delete[] m_buf;
    }

    void Open(const wxString& filename)
    {
        if (m_file)
        {
            xf_close(m_file);
            m_file = 0;
        }
        
        if (m_unenc_buf)
        {
            delete[] m_unenc_buf;
            m_unenc_buf = NULL;
        }
        
        if (m_buf)
        {
            delete[] m_buf;
            m_buf = NULL;
        }
        
        m_file_size = xf_get_file_size(towstr(filename));
        
        if (m_file_size < 8192)
            return;
            
        m_file = xf_open(towstr(filename), xfOpen, xfRead, xfShareRead);
        
        unsigned char info[32];
        xf_seek(m_file, 4096, xfSeekSet);
        if (32 != xf_read(m_file, info, 1, 32))
        {
            xf_close(m_file);
            m_file = 0;
            m_file_size = 0;
            return;
        }
        
        
        // check file signature
        if (info[0] != 0x5b ||
            info[1] != 0xab ||
            info[2] != 0x91 ||
            info[3] != 0x12)
        {
            xf_close(m_file);
            m_file = 0;
            m_file_size = 0;
            return;
        }
        
        
        // check version
        int version = bufToInt(info+4);
        if (version != 1)
        {
            xf_close(m_file);
            m_file = 0;
            m_file_size = 0;
            return;
        }
        
        // # of data bytes stored in file
        m_file_size = bufToInt64(info+8);
        m_block_size = bufToInt(info+16);
        m_data_per_block = bufToInt(info+20);
        m_header_size = bufToInt(info+24);
        
        m_buf = new unsigned char[m_block_size];
        m_unenc_buf = new unsigned char[m_block_size];
    }

    wxFileOffset GetLength() const
    {
        return m_file_size;
    }
    
    bool IsOk() const 
    { 
        return (wxStreamBase::IsOk() && m_file != 0);
    }

    bool IsSeekable() const
    {
        return true;
    }

protected:

    wxEncryptedInputStream()
    {
        // not allowed
    }

    size_t OnSysRead(void* _buffer, size_t size)
    {
        unsigned char* buffer = (unsigned char*)_buffer;
        
        size_t read_block;
        size_t offset_in_block;
        size_t bytes_to_read;
        size_t total_read = 0;
        
            
        while (1)
        {
            if (m_offset < 0 || m_offset >= m_file_size)
                break;
                
            read_block = (m_offset / m_data_per_block);
            offset_in_block = (m_offset % m_data_per_block);
            bytes_to_read = (m_data_per_block-offset_in_block);
            
            if (bytes_to_read > size)
                bytes_to_read = size;
                
            if (bytes_to_read > 0)
            {
                wxFileOffset o = read_block;
                o *= m_block_size;
                o += m_header_size;
                
                xf_seek(m_file, o, xfSeekSet);
            
                size_t r = xf_read(m_file, m_buf, 1, m_block_size);
                
                size_t os = m_block_size;
                kl::decryptBuffer(m_buf, r, m_unenc_buf, &os, ENC_KEY);
                
                if (offset_in_block >= os)
                {
                    bytes_to_read = 0;
                }
                 else
                {
                    if (bytes_to_read > os - offset_in_block)
                        bytes_to_read = os - offset_in_block;
                }
                
                
                memcpy(buffer, m_unenc_buf + offset_in_block, bytes_to_read);
                
                size -= bytes_to_read;
                buffer += bytes_to_read;
                m_offset += bytes_to_read;
                total_read += bytes_to_read;
            }
             else
            {
                // nothing to do here
                return 0;
            }
            
            if (size == 0)
                break;
        }
        
        return total_read;
    }
    
    wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode)
    {
        switch (mode)
        {
            case wxFromStart:
                if (pos < 0)
                    return wxInvalidOffset;
                m_offset = pos;
                return m_offset;
            case wxFromEnd:
                if (-pos > m_file_size)
                    m_offset = 0;
                     else          
                    m_offset = m_file_size + pos;
                return m_offset;
            case wxFromCurrent:
                m_offset += pos;
                return m_offset;
            default:
                return m_offset;
        }
    }
    
    wxFileOffset OnSysTell() const
    {
        return m_offset;
    }

private:


    unsigned int bufToInt(const unsigned char* buf)
    {
    #ifdef WIN32
        return *((unsigned int*)buf);
    #else
        return buf[0] + (buf[1]*256) + (buf[2]*65536) + (buf[3] * 16777216);
    #endif
    }


    tango::tango_int64_t bufToInt64(unsigned char* buf)
    {
    #ifdef WIN32
        return *((tango::tango_int64_t*)buf);
    #else
        tango::tango_int64_t result, tempv;
        tempv = buf[0];
        result = tempv;
        tempv = buf[1];
        result |= (tempv << 8);
        tempv = buf[2];
        result |= (tempv << 16);
        tempv = buf[3];
        result |= (tempv << 24);
        tempv = buf[4];
        result |= (tempv << 32);
        tempv = buf[5];
        result |= (tempv << 40);
        tempv = buf[6];
        result |= (tempv << 48);
        tempv = buf[7];
        result |= (tempv << 56);
        return result;
    #endif
    }


protected:
    
    xf_file_t m_file;
    wxFileOffset m_offset;
    wxFileOffset m_file_size;
    size_t m_data_per_block;
    size_t m_block_size;
    size_t m_header_size;
    unsigned char* m_buf;
    unsigned char* m_unenc_buf;
    std::wstring m_key;
    
    DECLARE_NO_COPY_CLASS(wxEncryptedInputStream)
};





// reads all of the data from a stream into a wxString

bool streamToString(wxInputStream* stream, wxString& res_string)
{
    if (!stream)
        return false;
    
    res_string = wxT("");
    
    // read the entire string into membuf
    wxMemoryBuffer membuf;
    
    while (1)
    {
        void* buf = membuf.GetAppendBuf(16384);
        stream->Read(buf, 16384);
        size_t read = stream->LastRead();
        membuf.UngetAppendBuf(read);
        if (read == 0)
            break;
    }
    
    unsigned char zero[4];
    memset(zero, 0, 4);
    membuf.AppendData(zero, 4);
    

    unsigned char* buf = (unsigned char*)membuf.GetData();
    size_t data_len = membuf.GetDataLen();
    
    if (buf[0] == 0xff && buf[1] == 0xfe)
    {
        // little-endian UTF-16
        std::wstring temps;
        kl::ucsle2wstring(temps, buf+2, (data_len-2)/2);
        res_string = towx(temps);
    }
     else if (buf[0] == 0xfe && buf[1] == 0xff)
    {
        // big-endian UTF-16
        std::wstring temps;
        kl::ucsbe2wstring(temps, buf+2, (data_len-2)/2);
        res_string = towx(temps);
    }
     else
    {
        // default to UTF-8
        if (buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
        {
            buf += 3;
            data_len -=3;
        }
        res_string = wxString::FromUTF8((const char*)buf, data_len);
    }
    
    return true;
}


// -- ExtensionPkg class implementation --

ExtensionPkg::ExtensionPkg()
{
    m_zip = NULL;
}

ExtensionPkg::~ExtensionPkg()
{
    std::vector<ExtensionStream*>::iterator it;
    for (it = m_streams.begin(); it != m_streams.end(); ++it)
    {
        delete (*it);
    }
    
    std::vector<wxMemoryInputStream*>::iterator mit;
    for (mit = m_to_delete.begin(); mit != m_to_delete.end(); ++mit)
    {
        delete (*mit);
    }
    
    std::vector<wxZipEntry*>::iterator zit;
    for (zit = m_zip_entries.begin(); zit != m_zip_entries.end(); ++zit)
    {
        delete (*zit);
    }
    
    delete m_zip;
}



static bool isZipFile(wxInputStream* file)
{
    unsigned char sig[4];
    file->SeekI(0);
    file->Read(sig, 4);
    
    bool result = true;
    
    if (file->LastRead() != 4 ||
        sig[0] != 0x50 ||
        sig[1] != 0x4b ||
        sig[2] != 0x03 ||
        sig[3] != 0x04)
    {
        result = false;
    }
    
    file->SeekI(0);
    
    return result;
}

static bool isEncryptedZipFile(wxInputStream* file)
{
    unsigned char sig[4];
    file->SeekI(4096);
    file->Read(sig, 4);
    
    bool result = true;
    
    if (file->LastRead() != 4 ||
        sig[0] != 0x5b ||
        sig[1] != 0xab ||
        sig[2] != 0x91 ||
        sig[3] != 0x12)
    {
        result = false;
    }
    
    file->SeekI(0);
    
    return result;
}


bool ExtensionPkg::open(const wxString& filename)
{
    XCM_AUTO_LOCK(m_obj_mutex);
    
    if (!xf_get_file_exist(towstr(filename)))
        return false;
        
    m_file = new wxFileInputStream(filename);
    if (!m_file->IsOk())
        return false;


    // discover file format
    
    if (isZipFile(m_file))
    {
        m_zip = new wxZipInputStream(m_file);
    }
     else if (isEncryptedZipFile(m_file))
    {
        // close the original input file, it's encrypted;
        // use on-the-fly deencryption
        delete m_file;
        m_file = new wxEncryptedInputStream(filename, ENC_KEY);
            
/*
        ::remove("c:\\ben.out");
        FILE* f = fopen("c:\\ben.out", "w+b");
        
        unsigned char buf[27];
        m_file->SeekI(0);
        while (1)
        {
            wxFileOffset fo = m_file->TellI();
            if (f > 0)
            {
                m_file->SeekI(fo - 1);
                m_file->Read(buf, 1);
                m_file->SeekI(fo);
            }
            
            m_file->Read(buf, 27);
            size_t l = m_file->LastRead();
            fwrite(buf, 1, l, f);
            if (l != 27)
                break;
        }
        delete m_file;
        fclose(f);
        return false;
*/      
    }
     else
    {
        // not a recognized file format
        delete m_file;
        return false;
    }
    
        m_zip = new wxZipInputStream(m_file);
    
    
    while (1)
    {
        wxZipEntry* entry = m_zip->GetNextEntry();
        if (!entry)
            break;
        m_zip_entries.push_back(entry);
    }
    
    return true;
}

wxInputStream* ExtensionPkg::getStream(const wxString& stream_name)
{
    XCM_AUTO_LOCK(m_obj_mutex);
    
    // check if we already have an open stream
    std::vector<ExtensionStream*>::iterator it;
    for (it = m_streams.begin(); it != m_streams.end(); ++it)
    {
        if (0 == stream_name.CmpNoCase((*it)->m_name))
        {
            wxMemoryInputStream* mem_stream = new wxMemoryInputStream(
                                                (*it)->m_buf.GetData(),
                                                (*it)->m_buf.GetDataLen());
            m_to_delete.push_back(mem_stream);
            
            return mem_stream;
        }
    }
    
    
    // stream is not yet open, we need to open the
    // stream from the archive file
    
    if (!m_zip)
    {
        // file not open
        return NULL;
    }
    
    
    // find the zip entry
    
    std::vector<wxZipEntry*>::iterator zit;
    for (zit = m_zip_entries.begin(); zit != m_zip_entries.end(); ++zit)
    {
        if ((*zit)->GetInternalName().CmpNoCase(stream_name) == 0)
        {
            // load the stream
            if (!m_zip->OpenEntry(*(*zit)))
                return NULL;
            
            ExtensionStream* s = new ExtensionStream;
            s->m_name = stream_name;
            
            unsigned char* buf = new unsigned char[16384];
            
            while (1)
            {
                m_zip->Read(buf, 16384);
                size_t last_read = m_zip->LastRead();
                if (last_read == 0)
                    break;
                    
                s->m_buf.AppendData(buf, last_read);
            }
            
            m_streams.push_back(s);
            
            delete[] buf;
            
            // return a memory stream
            wxMemoryInputStream* mem_stream = new wxMemoryInputStream(
                                                s->m_buf.GetData(),
                                                s->m_buf.GetDataLen());
            m_to_delete.push_back(mem_stream);
            
            return mem_stream;
        }
    }

    
    
    return NULL;
}


bool ExtensionPkg::getStreamExists(const wxString& stream_name)
{
    std::vector<wxZipEntry*>::iterator zit;
    for (zit = m_zip_entries.begin(); zit != m_zip_entries.end(); ++zit)
    {
        if ((*zit)->GetInternalName().CmpNoCase(stream_name) == 0)
            return true;
    }
    return false;
}

