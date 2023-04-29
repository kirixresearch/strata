/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-02-17
 *
 */


#include <kl/file.h>
#include <cstring>
#include "exkeypool.h"
#include "util.h"


static inline void qs_compexch(unsigned char*& a,
                               unsigned char*& b,
                               int len)
{
    if (memcmp(b, a, len) < 0)
    {
        unsigned char* t = a;
        a = b;
        b = t;
    }
}

static int qs_partition(unsigned char* arr[],
                        unsigned int len,
                        int left,
                        int right)
{
    int i = left-1;
    int j = right;
    unsigned char* v = arr[right];
    unsigned char* temp;
    while (1)
    {
        while (memcmp(arr[++i], v, len) < 0);
        while (memcmp(v, arr[--j], len) < 0)
        {
            if (j == left)
                break;
        }
        if (i >= j)
            break;
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
    temp = arr[i];
    arr[i] = arr[right];
    arr[right] = temp;
    return i;
}

static void qs_quicksort(unsigned char* a[],
                         unsigned int len,
                         int l,
                         int r)
{
    int i;
    unsigned char* t;

    if (r-l <= 10)
        return;

    t = a[(l+r)/2];
    a[(l+r)/2] = a[r-1];
    a[r-1] = t;

    qs_compexch(a[l], a[r-1], len);
    qs_compexch(a[l], a[r], len);
    qs_compexch(a[r-1], a[r], len);
    i = qs_partition(a, len, l+1, r-1);
    qs_quicksort(a, len, l, i-1);
    qs_quicksort(a, len, i+1, r);
}

static void qs_insertion(unsigned char* arr[],
                         unsigned len,
                         int left,
                         int right)
{
    int i, j;
    unsigned char* v;

    for (i = right; i > left; i--)
        qs_compexch(arr[i-1], arr[i], len);
    for (i = left+2; i <= right; i++)
    {
        j = i;
        v = arr[i];
        while (memcmp(v, arr[j-1], len) < 0)
        {
            arr[j] = arr[j-1];
            j--;
        }
        arr[j] = v;
    }

}

static void do_sort(unsigned char* arr[],
                    unsigned int len,
                    int left,
                    int right)
{
    qs_quicksort(arr, len, left, right);
    qs_insertion(arr, len, left, right);
}




ExKeyPool::ExKeyPool(int size_bytes, int entry_size, int compare_size)
{
    m_data = NULL;

    int try_size;
    for (int cnt = 0; cnt < 6; ++cnt)
    {
        try_size = size_bytes;

        try_size = try_size/entry_size;
        m_entries_per_buf = try_size;
        try_size *= entry_size;

        try
        {
            m_data = new unsigned char[try_size];
            if (m_data)
                break;
        }
        catch(...)
        {
        }

        // could not allocate that much memory -- try halving the requested amount
        size_bytes /= 2;
    }

    m_entry_size = entry_size;
    m_compare_size = compare_size;
    m_pos = 0;
    m_entry_count = 0;
    m_sorted_arr = NULL;
}

ExKeyPool::~ExKeyPool()
{
    delete[] m_sorted_arr;
    delete[] m_data;
}

bool ExKeyPool::appendData(const unsigned char* data)
{
    if (m_pos >= m_entries_per_buf || m_data == NULL)
        return false;
    memcpy(m_data+(m_pos*m_entry_size), data, m_entry_size);
    m_pos++;
    m_entry_count++;
    return true;
}

bool ExKeyPool::appendData2(const unsigned char* data1, int data1_len, const unsigned char* data2, int data2_len)
{
    if (m_pos >= m_entries_per_buf || m_data == NULL)
        return false;
    unsigned char* off = m_data+(m_pos*m_entry_size);
    memcpy(off, data1, data1_len);
    memcpy(off+data1_len, data2, data2_len);
    m_pos++;
    m_entry_count++;
    return true;
}

bool ExKeyPool::isFull()
{
    return (m_entry_count >= m_entries_per_buf ? true : false);
}

void ExKeyPool::reset()
{
    m_pos = 0;
    m_entry_count = 0;

    if (m_sorted_arr)
    {
        delete[] m_sorted_arr;
        m_sorted_arr = NULL;
    }
}

void ExKeyPool::sort()
{
    // create a pointer array
    delete[] m_sorted_arr;
    m_sorted_arr = new unsigned char*[m_entry_count];
    
    unsigned char* ptr = m_data;
    int i;
    for (i = 0; i < m_entry_count; ++i)
    {
        m_sorted_arr[i] = ptr;
        ptr += m_entry_size;
    }

    // sort the keys
    do_sort(m_sorted_arr, m_compare_size, 0, m_entry_count - 1);
}


bool ExKeyPool::writeToFile(const std::wstring& filename)
{
    xf_remove(filename);

    xf_file_t file = xf_open(filename, xfCreate, xfReadWrite, xfShareNone);
    if (file == NULL)
        return false;

    unsigned char* buf = new unsigned char[m_entry_size*1000];
    int buf_pos = 0;
    int counter = 0;

    goFirst();
    while (!isEof())
    {
        memcpy(buf+(buf_pos*m_entry_size), getEntryPtr(), m_entry_size);
        ++buf_pos;
        if (buf_pos == 1000)
        {
            if (buf_pos != xf_write(file, buf, m_entry_size, buf_pos))
            {
                xf_close(file);
                xf_remove(filename);
                delete[] buf;
                return false;
            }
            buf_pos = 0;
        }

        counter++;
        if (counter % 10000 == 0)
        {
            if (xf_get_free_disk_space(filename) < 50000000)
            {
                xf_close(file);
                xf_remove(filename);
                delete[] buf;
                return false;
            }
        }

        goNext();
    }

    // write filname block
    if (buf_pos != xf_write(file, buf, m_entry_size, buf_pos))
    {
        xf_close(file);
        xf_remove(filename);
        delete[] buf;
        return false;
    }
    
    xf_close(file);
    delete[] buf;
    return true;
}


// -- reading functions --

void ExKeyPool::goFirst()
{
    m_pos = 0;
}

void ExKeyPool::goNext()
{
    m_pos++;
}

bool ExKeyPool::isEof()
{
    if (m_pos < m_entry_count)
        return false;
    return true;
}

unsigned char* ExKeyPool::getEntryPtr()
{
    if (m_sorted_arr)
    {
        return m_sorted_arr[m_pos];
    }

    return (m_data+(m_pos*m_entry_size));
}








ExKeyPoolFileReader::ExKeyPoolFileReader()
{
    m_file = 0;
    m_pos = 0;
    m_entry_size = 0;
    m_entry_count = 0;
    m_buf = NULL;
    m_buf_keycount = 0;
    m_buf_idx = 0;
}

ExKeyPoolFileReader::~ExKeyPoolFileReader()
{
    if (isOpen())
        close();
}

bool ExKeyPoolFileReader::isOpen()
{
    return (m_file != NULL) ? true : false;
}

bool ExKeyPoolFileReader::open(const std::wstring& filename, int entry_size)
{
    m_file = xf_open(filename, xfOpen, xfRead, xfShareNone);
    if (m_file == NULL)
        return false;

    xf_seek(m_file, 0, xfSeekEnd);
    xf_off_t file_size = xf_get_file_pos(m_file);
    xf_seek(m_file, 0, xfSeekSet);
    m_entry_count = (int)(file_size/entry_size);

    m_entry_size = entry_size;

    m_buf = new unsigned char[m_entry_size*1000];
    m_buf_keycount = 0;
    return true;
}

void ExKeyPoolFileReader::close()
{
    xf_close(m_file);
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }
}


void ExKeyPoolFileReader::goFirst()
{
    m_pos = 0;
    m_buf_keycount = 0;
    m_buf_idx = 0;
    xf_seek(m_file, 0, xfSeekSet);
}

void ExKeyPoolFileReader::goNext()
{
    m_pos++;
    m_buf_idx++;
}

bool ExKeyPoolFileReader::isEof()
{
    if (m_pos < m_entry_count)
        return false;
    return true;
}

unsigned char* ExKeyPoolFileReader::getEntryPtr()
{
    if (m_buf_keycount == 0 || m_buf_idx >= 1000)
    {
        m_buf_keycount = xf_read(m_file, m_buf, m_entry_size, 1000);
        m_buf_idx = 0;
    }

    return m_buf+(m_buf_idx*m_entry_size);
}







ExKeyPoolCombiner::ExKeyPoolCombiner(int entry_size, int compare_size)
{
    m_pool_count = 0;
    m_eof = false;
    m_entry_size = entry_size;
    m_compare_size = compare_size;
}

ExKeyPoolCombiner::~ExKeyPoolCombiner()
{
}

void ExKeyPoolCombiner::addPool(IKeyList* pool)
{
    m_pools[m_pool_count] = pool;
    m_pool_count++;
}

void ExKeyPoolCombiner::goFirst()
{
    for (int i = 0; i < m_pool_count; ++i)
    {
        m_pools[i]->goFirst();
    }

    goNext();
}

void ExKeyPoolCombiner::goNext()
{
    bool eof = true;
    IKeyList* least = NULL;
    unsigned char* k = NULL;
    unsigned char* ptr;

    for (int i = 0; i < m_pool_count; ++i)
    {
        if (m_pools[i]->isEof())
            continue;

        eof = false;
        ptr = m_pools[i]->getEntryPtr();
        if (!k || memcmp(k, ptr, m_compare_size) > 0)
        {
            k = ptr;
            least = m_pools[i];
        }
    }

    if (least)
    {
        least->goNext();
    }

    m_eof = eof;
    m_curkey = k;
}

bool ExKeyPoolCombiner::isEof()
{
    return m_eof;
}

unsigned char* ExKeyPoolCombiner::getEntryPtr()
{
    return m_curkey;
}







ExVirtualKeyPool::ExVirtualKeyPool(const std::wstring keypool_path,
                 int pool_size_bytes,
                 int entry_size,
                 int compare_size)
{
    m_pool_dir = keypool_path;
    m_pool_size_bytes = pool_size_bytes;
    m_entry_size = entry_size;
    m_compare_size = compare_size;
    m_cur_pool = NULL;
    m_reader = NULL;
}

                 
ExVirtualKeyPool::~ExVirtualKeyPool()
{
    delete m_reader;
    
    std::vector<ExKeyPoolFileReader*>::iterator it;
    for (it = m_pools.begin(); it != m_pools.end(); ++it)
        delete (*it);
    
    delete m_cur_pool;
    
    std::vector<std::wstring>::iterator f_it;
    for (f_it = m_to_delete.begin(); f_it != m_to_delete.end(); ++f_it)
        xf_remove(*f_it);
}


bool ExVirtualKeyPool::appendData(unsigned char* data)
{
    if (m_cur_pool == NULL)
    {
        m_cur_pool = new ExKeyPool(m_pool_size_bytes, m_entry_size, m_compare_size);
    }
    
    if (m_cur_pool->isFull())
    {
        std::wstring index_filename = getUniqueString();
        index_filename += L".dat";
        std::wstring full_filename;
        full_filename = makePathName(m_pool_dir,
                                           L"",
                                           index_filename);
        m_cur_pool->sort();
        m_cur_pool->writeToFile(full_filename);
        delete m_cur_pool;
        
        
        ExKeyPoolFileReader* reader = new ExKeyPoolFileReader;
        reader->open(full_filename, m_entry_size);
        m_pools.push_back(reader);
        m_to_delete.push_back(full_filename);
        
        
        m_cur_pool = new ExKeyPool(m_pool_size_bytes, m_entry_size, m_compare_size);
    }
    
    return m_cur_pool->appendData(data);
}


void ExVirtualKeyPool::goFirst()
{
    delete m_reader;
    m_reader = new ExKeyPoolCombiner(m_entry_size, m_compare_size);
    std::vector<ExKeyPoolFileReader*>::iterator it;
    
    for (it = m_pools.begin(); it != m_pools.end(); ++it)
        m_reader->addPool(*it);
    
    if (m_cur_pool)
    {
        m_cur_pool->sort();
        m_reader->addPool(m_cur_pool);
    }
    
    m_reader->goFirst();
}




