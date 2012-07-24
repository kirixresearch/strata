/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-02-17
 *
 */


#ifndef __XDCOMMON_EXKEYPOOL_H
#define __XDCOMMON_EXKEYPOOL_H


#include <string>
#include <vector>


class IKeyList
{
public:
    virtual void goFirst() = 0;
    virtual void goNext() = 0;
    virtual bool isEof() = 0;
    virtual unsigned char* getEntryPtr() = 0;
};





class ExKeyPool : public IKeyList
{


public:

    ExKeyPool(int size_bytes, int entry_size, int compare_size);
    ~ExKeyPool();

    bool appendData(unsigned char* data);
    bool appendData2(unsigned char* data1, int data1_len, unsigned char* data2, int data2_len);
    bool isFull();
    void reset();
    void sort();
    bool writeToFile(const std::wstring& filename);

    // -- reading functions --

    void goFirst();
    void goNext();
    bool isEof();
    unsigned char* getEntryPtr();

private:
    
    unsigned char* m_data;
    int m_pos;
    int m_entry_size;
    int m_compare_size;
    int m_entries_per_buf;
    int m_entry_count;

    // -- sorted ptr array --
    unsigned char** m_sorted_arr;
};



class ExKeyPoolFileReader : public IKeyList
{

public:

    ExKeyPoolFileReader();
    ~ExKeyPoolFileReader();

    bool isOpen();
    bool open(const std::wstring& filename, int entry_size);
    void close();

    // -- reading functions --

    void goFirst();
    void goNext();
    bool isEof();
    unsigned char* getEntryPtr();

private:

    // -- file member variables --
    xf_file_t m_file;
    unsigned char* m_buf;
    int m_buf_keycount;
    int m_buf_idx;
    int m_entry_size;
    int m_entry_count;
    int m_pos;
};



class ExKeyPoolCombiner : public IKeyList
{
public:

    ExKeyPoolCombiner(int entry_size, int compare_size);
    ~ExKeyPoolCombiner();

    void addPool(IKeyList* pool);

    void goFirst();
    void goNext();
    bool isEof();
    unsigned char* getEntryPtr();

private:

    IKeyList* m_pools[512];
    int m_pool_count;
    unsigned char* m_curkey;
    bool m_eof;
    int m_entry_size;
    int m_compare_size;
};





class ExVirtualKeyPool
{
public:

    ExVirtualKeyPool(const std::wstring keypool_path,
                     int pool_size_bytes,
                     int entry_size,
                     int compare_size);
                     
    ~ExVirtualKeyPool();

    bool appendData(unsigned char* data);

    void goFirst();
    void goNext() { m_reader->goNext(); }
    bool isEof() { return m_reader->isEof(); }
    unsigned char* getEntryPtr() { return m_reader->getEntryPtr(); }

private:

    std::wstring m_pool_dir;
    std::vector<ExKeyPoolFileReader*> m_pools;
    std::vector<std::wstring> m_to_delete;
    ExKeyPool* m_cur_pool;
    ExKeyPoolCombiner* m_reader;
    
    int m_pool_size_bytes;
    int m_entry_size;
    int m_compare_size;
};




#endif

