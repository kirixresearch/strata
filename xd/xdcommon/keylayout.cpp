/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2004-01-26
 *
 */


#include <kl/klib.h>
#include <xd/xd.h>
#include "keylayout.h"
#include "../xdcommon/util.h"


KeyLayout::KeyLayout()
{
    m_buf = NULL;
    m_key_length = 0;
    m_iter = NULL;
    m_hold_ref = true;
    m_trunc = false;
}

KeyLayout::~KeyLayout()
{
    setIterator(xcm::null, false);

    if (m_buf)
    {
        delete[] m_buf;
    }
}

bool KeyLayout::setKeyExpr(xd::IIteratorPtr iter,
                           const std::wstring& expr,
                           bool hold_ref)
{
    setIterator(iter, hold_ref);

    std::vector<std::wstring> expr_vec;
    std::vector<std::wstring>::iterator it;

    kl::parseDelimitedList(expr, expr_vec, L',', true);

    for (it = expr_vec.begin();
         it != expr_vec.end(); ++it)
    {
        std::wstring expr;
        std::wstring dir;
        bool descending = false;

        expr = *it;
        
        kl::trim(expr);

        int last_space_pos = expr.find_last_of(L' ');
        if (last_space_pos != -1)
        {
            dir = kl::afterLast(expr, L' ');
            kl::makeUpper(dir);
            kl::trim(dir);

            if (dir == L"ASC")
            {
                descending = false;
                expr = kl::beforeLast(expr, L' ');
            }
             else if (dir == L"DESC")
            {
                descending = true;
                expr = kl::beforeLast(expr, L' ');
            }
        }
        
        dequote(expr, L'[', L']');

        if (!addKeyPart(expr, -1, -1, descending))
            return false;
    }

    return true;
}

const unsigned char* KeyLayout::getKey()
{
    unsigned char* ptr;

    
    if (!m_iter || m_iter->eof())
    {
        m_trunc = true;
        return m_buf;
    }
    
    m_trunc = false;

    // fill out the key buffer
    std::vector<KeyPart>::const_iterator part_end = m_parts.end();
    std::vector<KeyPart>::const_iterator part_it;

    for (part_it = m_parts.begin(); part_it != part_end; ++part_it)
    {
        ptr = m_buf + part_it->offset;
        switch (part_it->type)
        {
            case xd::typeCharacter:
            {
                const std::string& s = m_iter->getString(part_it->handle);
                int copy_len = s.length();
                if (copy_len > part_it->width)
                {
                    m_trunc = true;
                    copy_len = part_it->width;
                }
                memset(ptr, 0, part_it->width);
                memcpy(ptr, s.c_str(), copy_len);
            }
            break;

            case xd::typeWideCharacter:
            {
                const std::wstring& s = m_iter->getWideString(part_it->handle);
                int copy_len = s.length();
                if (copy_len*2 > part_it->width)
                {
                    m_trunc = true;
                    copy_len = (part_it->width/2);
                }
                memset(ptr, 0, part_it->width);
                kl::wstring2ucsbe(ptr, s, copy_len);
            }
            break;

            case xd::typeInteger:
            {
                // for debugging:
                //unsigned int tempui;
                //tempui = (unsigned int)m_iter->getInteger(part_it->handle);

                // the key stores only an integer. The source
                // may have decimal places, which would yield
                // truncation.  Find out if we will truncate
                // a decimal place

                unsigned int tempui;
                double d1, d2;

                d1 = m_iter->getDouble(part_it->handle);
                d2 = kl::dblround(d1, 0);

                if (0 != kl::dblcompare(d1, d2))
                {
                    m_trunc = true;
                }
                
                tempui = (unsigned int)d2;

                ptr[0] = (tempui >> 24) & 0xff;
                ptr[1] = (tempui >> 16) & 0xff;
                ptr[2] = (tempui >> 8) & 0xff;
                ptr[3] = (tempui & 0xff);

                // flip sign so the integers sort properly
                if (ptr[0] & 0x80)
                {
                    ptr[0] &= 0x7f;
                }
                 else
                {
                    ptr[0] |= 0x80;
                }

                break;
            }
        
            case xd::typeNumeric:
            case xd::typeDouble:
            {
                double tempd;
                unsigned char* tempd_src = (unsigned char*)&tempd;

                tempd = m_iter->getDouble(part_it->handle);

                unsigned char* dest = ptr;

                int i = 8;
                while (i--)
                {
                    *dest = *(tempd_src+i);
                    dest++;
                }

                if (ptr[0] & 0x80)
                {
                    for (i = 0; i < 8; i++)
                    {
                        ptr[i] = ~ptr[i];
                    }
                }
                 else
                {
                    ptr[0] |= 0x80;
                }
            }
            break;

            case xd::typeDate:
            {
                xd::datetime_t d1, d2;
                
                d1 = m_iter->getDateTime(part_it->handle);

                // if the time value of the source datetime
                // was anything besides 00:00:00, indicate
                // truncation
                
                d2 = d1 >> 32;
                d2 <<= 32;

                if (d2 != d1)
                {
                    m_trunc = true;
                }                

                ptr[0] = (unsigned char)((d1 >> 56) & 0xff);
                ptr[1] = (unsigned char)((d1 >> 48) & 0xff);
                ptr[2] = (unsigned char)((d1 >> 40) & 0xff);
                ptr[3] = (unsigned char)((d1 >> 32) & 0xff);
            }
            break;

            case xd::typeDateTime:
            {
                xd::datetime_t tempdt = m_iter->getDateTime(part_it->handle);

                ptr[0] = (unsigned char)((tempdt >> 56) & 0xff);
                ptr[1] = (unsigned char)((tempdt >> 48) & 0xff);
                ptr[2] = (unsigned char)((tempdt >> 40) & 0xff);
                ptr[3] = (unsigned char)((tempdt >> 32) & 0xff);
                ptr[4] = (unsigned char)((tempdt >> 24) & 0xff);
                ptr[5] = (unsigned char)((tempdt >> 16) & 0xff);
                ptr[6] = (unsigned char)((tempdt >> 8) & 0xff);
                ptr[7] = (unsigned char)(tempdt & 0xff);
            }
            break;

            case xd::typeBoolean:
            {
                bool tempb = m_iter->getBoolean(part_it->handle);
                if (tempb)
                    ptr[0] = 2;
                     else
                    ptr[0] = 1;
            }
            break;

            case xd::typeBinary:
            {
                memcpy(ptr, m_iter->getRawPtr(part_it->handle), part_it->width);
            }
            break;
        }

        if (part_it->descending)
        {
            int tempi;
            for (tempi = 0; tempi < part_it->width; ++tempi)
            {
                *(ptr+tempi) = ~*(ptr+tempi);
            }
        }
    }

    return m_buf;
}


xd::datetime_t internalParseDateTime(const wchar_t* str)
{
    int year, month, day, hour, minute, second;
    if (!parseDateTime(str, &year, &month, &day, &hour, &minute, &second))
        return 0;

    if (hour == -1 || minute == -1 || second == -1)
    {
        hour = 0;
        minute = 0;
        second = 0;
    }
    
    xd::DateTime d(year, month, day, hour, minute, second, 0);
    return d.getDateTime();
}

const unsigned char* KeyLayout::getKeyFromValues(const wchar_t* values[], size_t values_size)
{
    unsigned char* ptr;
    const wchar_t* part_ptr;
    size_t part_idx = 0;

    m_trunc = false;

    // fill out the key buffer
    std::vector<KeyPart>::const_iterator part_end = m_parts.end();
    std::vector<KeyPart>::const_iterator part_it;

    if (values_size < m_parts.size())
    {
        // not enough values for the number of key parts;
        // to remedy this, we will zero out the entire binary
        // key, and fill in as many values as possible
        memset(m_buf, 0, m_key_length);
    }
    
    for (part_it = m_parts.begin(); part_it != part_end; ++part_it)
    {
        ptr = m_buf + part_it->offset;
        
        if (part_idx >= values_size)
            break;
        part_ptr = values[part_idx];
        part_idx++;
        
        switch (part_it->type)
        {
            case xd::typeCharacter:
            {
                if (wcslen(part_ptr) > (size_t)part_it->width)
                    m_trunc = true;
                memset(ptr, 0, part_it->width);
                for (size_t i = 0; i < (size_t)part_it->width; ++i)
                {
                    if (!part_ptr[i])
                        break;
                    ptr[i] = (unsigned char)part_ptr[i];
                }
            }
            break;

            case xd::typeWideCharacter:
            {
                std::wstring s = part_ptr;
                size_t copy_len = s.length();
                if (copy_len*2 > (size_t)part_it->width)
                {
                    m_trunc = true;
                    copy_len = (part_it->width/2);
                }
                memset(ptr, 0, part_it->width);
                kl::wstring2ucsbe(ptr, s, copy_len);
            }
            break;

            case xd::typeInteger:
            {
                // for debugging:
                //unsigned int tempui;
                //tempui = (unsigned int)m_iter->getInteger(part_it->handle);

                // the key stores only an integer. The source
                // may have decimal places, which would yield
                // truncation.  Find out if we will truncate
                // a decimal place

                unsigned int tempui;
                double d1, d2;

                d1 = kl::nolocale_wtof(part_ptr);
                d2 = kl::dblround(d1, 0);

                if (0 != kl::dblcompare(d1, d2))
                {
                    m_trunc = true;
                }
                
                tempui = (unsigned int)d2;

                ptr[0] = (tempui >> 24) & 0xff;
                ptr[1] = (tempui >> 16) & 0xff;
                ptr[2] = (tempui >> 8) & 0xff;
                ptr[3] = (tempui & 0xff);

                // flip sign so the integers sort properly
                if (ptr[0] & 0x80)
                {
                    ptr[0] &= 0x7f;
                }
                 else
                {
                    ptr[0] |= 0x80;
                }

                break;
            }
        
            case xd::typeNumeric:
            case xd::typeDouble:
            {
                double tempd;
                unsigned char* tempd_src = (unsigned char*)&tempd;

                tempd = kl::nolocale_wtof(part_ptr);

                unsigned char* dest = ptr;

                int i = 8;
                while (i--)
                {
                    *dest = *(tempd_src+i);
                    dest++;
                }

                if (ptr[0] & 0x80)
                {
                    for (i = 0; i < 8; i++)
                    {
                        ptr[i] = ~ptr[i];
                    }
                }
                 else
                {
                    ptr[0] |= 0x80;
                }
            }
            break;

            case xd::typeDate:
            {
                xd::datetime_t d1, d2;
                
                d1 = internalParseDateTime(part_ptr);

                // if the time value of the source datetime
                // was anything besides 00:00:00, indicate
                // truncation
                
                d2 = d1 >> 32;
                d2 <<= 32;

                if (d2 != d1)
                {
                    m_trunc = true;
                }                

                ptr[0] = (unsigned char)((d1 >> 56) & 0xff);
                ptr[1] = (unsigned char)((d1 >> 48) & 0xff);
                ptr[2] = (unsigned char)((d1 >> 40) & 0xff);
                ptr[3] = (unsigned char)((d1 >> 32) & 0xff);
            }
            break;

            case xd::typeDateTime:
            {
                xd::datetime_t tempdt = internalParseDateTime(part_ptr);

                ptr[0] = (unsigned char)((tempdt >> 56) & 0xff);
                ptr[1] = (unsigned char)((tempdt >> 48) & 0xff);
                ptr[2] = (unsigned char)((tempdt >> 40) & 0xff);
                ptr[3] = (unsigned char)((tempdt >> 32) & 0xff);
                ptr[4] = (unsigned char)((tempdt >> 24) & 0xff);
                ptr[5] = (unsigned char)((tempdt >> 16) & 0xff);
                ptr[6] = (unsigned char)((tempdt >> 8) & 0xff);
                ptr[7] = (unsigned char)(tempdt & 0xff);
            }
            break;

            case xd::typeBoolean:
            {
                bool tempb = (towupper(*part_ptr) == 'T') ? true : false;
                if (tempb)
                    ptr[0] = 2;
                     else
                    ptr[0] = 1;
            }
            break;

            case xd::typeBinary:
            {
                memcpy(ptr, (unsigned char*)part_ptr, part_it->width);
            }
            break;
        }

        if (part_it->descending)
        {
            int tempi;
            for (tempi = 0; tempi < part_it->width; ++tempi)
            {
                *(ptr+tempi) = ~*(ptr+tempi);
            }
        }
    }

    return m_buf;
}


int KeyLayout::getKeyLength()
{
    return m_key_length;
}

bool KeyLayout::getTruncation()
{
    return m_trunc;
}

void KeyLayout::setIterator(xd::IIteratorPtr iter, bool hold_ref)
{
    // release old iterator
    if (m_iter)
    {
        std::vector<KeyPart>::const_iterator part_end = m_parts.end();
        std::vector<KeyPart>::const_iterator part_it;

        for (part_it = m_parts.begin(); part_it != part_end; ++part_it)
        {
            if (part_it->handle)
                m_iter->releaseHandle(part_it->handle);
        }
        m_parts.clear();


        if (m_hold_ref)
        {
            m_iter->unref();
        }

        m_iter = NULL;
    }
    m_iter_structure.clear();
    
    // assign new iterator
    m_iter = iter.p;

    if (hold_ref)
    {
        m_iter->ref();
    }

    if (m_iter)
    {
        m_iter_structure = m_iter->getStructure();
    }

    m_hold_ref = hold_ref;
}

bool KeyLayout::addKeyPart(const std::wstring& expr,
                           int type,
                           int expr_width,
                           bool descending)
{
    if (!m_iter)
        return false;

    KeyPart part;

    part.offset = m_key_length;
    part.handle = m_iter->getHandle(expr);
    part.width = expr_width;
    part.type = type;

    if (!part.handle)
        return false;

    const xd::ColumnInfo& info = m_iter_structure->getColumnInfo(expr);

    if (info.isOk())
    {
        if (part.type == -1)
        {
            part.type = info.type;
        }

        if (part.width == -1)
        {
            part.width = info.width;
        }
    }
     else
    {
        if (part.type == -1)
        {
            part.type = m_iter->getType(part.handle);
        }

        if (part.width == -1)
        {
            if (part.type == xd::typeCharacter)
            {
                // idx part is a character expression, so
                // the width is unpredictable. We will choose
                // a sensible default value

                part.width = 50;
            }
             else if (part.type == xd::typeWideCharacter)
            {
                part.width = 100;
            }
        }
    }

    switch (part.type)
    {
        default:
            // width is already set
            break;

        case xd::typeWideCharacter:
            part.width *= 2;
            break;

        case xd::typeInteger:
            part.width = sizeof(int);
            break;
        
        case xd::typeNumeric:
        case xd::typeDouble:
            part.width = sizeof(double);
            break;

        case xd::typeDate:
            part.width = sizeof(int);
            break;

        case xd::typeDateTime:
            part.width = sizeof(int) * 2;
            break;

        case xd::typeBoolean:
            part.width = 1;
            break;
    }

    part.descending = descending;

    if (part.width == -1)
        return false;

    m_key_length += part.width;

    m_parts.push_back(part);

    delete[] m_buf;
    m_buf = new unsigned char[m_key_length];

    return true;
}


