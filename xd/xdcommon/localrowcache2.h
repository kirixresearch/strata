/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-21
 *
 */


#ifndef __XDCOMMON_LOCALROWCACHE2_H
#define __XDCOMMON_LOCALROWCACHE2_H

#include <vector>
#include <map>
#include <kl/file.h>

namespace kl { class membuf; }

class LocalRowValue
{
    friend class LocalRow2;

public:

    LocalRowValue();
    ~LocalRowValue();

    void setNull();
    bool setData(unsigned char* _data, size_t _len);
    unsigned char* getData() { return data; }
    size_t getDataLength() { return len; }
    bool isNull() { return is_null; }

private:

    bool is_null;
    unsigned char* data;
    size_t len;
};


class LocalRow2
{
    friend class LocalRowCache2;

public:

    LocalRow2();
    ~LocalRow2();
 
    LocalRowValue& getColumnData(size_t col_idx);
    size_t getColumnCount() const;

    void setColumnData(size_t col_idx, LocalRowValue& val);

    void* serialize(size_t* len);
    void unserialize(const void* buf, size_t len);

private:

    std::vector<LocalRowValue*> m_values;
    kl::membuf* m_buf;
};



class LocalRowCache2
{
public:

    LocalRowCache2();
    ~LocalRowCache2();

    bool init();

    bool getRow(long long rowid, LocalRow2& row);
    bool putRow(long long rowid, LocalRow2& row);

private:

    void* m_sqlite;
};


#endif
