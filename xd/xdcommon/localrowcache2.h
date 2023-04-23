/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-21
 *
 */


#ifndef H_XDCOMMON_LOCALROWCACHE2_H
#define H_XDCOMMON_LOCALROWCACHE2_H

#include <vector>
#include <map>
#include <kl/file.h>

namespace kl { class membuf; }

class LocalRowValue
{
    friend class LocalRow2;

public:

    enum
    {
        typeBinary = 0,
        typeCstr = 1,
        typeFloat = 2,
        typeDouble = 3,
        typeBoolean = 4
    };

public:

    LocalRowValue();
    ~LocalRowValue();

    void setNull();
    bool setData(const unsigned char* _data, size_t _len);
    const unsigned char* getData() const { return data; }
    size_t getDataLength() const { return len; }
    bool isNull() { return is_null; }
    void setType(unsigned char _type) { type = _type; }
    unsigned char getType() const { return type; }

private:

    unsigned char* data;
    size_t len;
    bool is_null;
    unsigned char type;
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

    const unsigned char* serialize(size_t* len);
    void unserialize(const unsigned char* buf, size_t len);

private:

    std::vector<LocalRowValue*> m_values;
    kl::membuf* m_buf;
};



class LocalRowCache2
{
public:

    LocalRowCache2();
    ~LocalRowCache2();

    bool init(const std::wstring& path = L"");

    bool getRow(long long rowid, LocalRow2& row);
    bool putRow(long long rowid, LocalRow2& row);

private:

    void* m_sqlite;
    std::wstring m_path;
};


#endif
