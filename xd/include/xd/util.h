/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-08-25
 *
 */


#ifndef __XDCOMMON_XDUTIL_H
#define __XDCOMMON_XDUTIL_H


#include <map>
#include <string>
#include <algorithm>
#include <ctime>
#include <cwctype>

namespace xd
{


// database manager instantiator

inline IDatabaseMgrPtr getDatabaseMgr()
{
    IDatabaseMgrPtr dbmgr;
    dbmgr.create_instance("xdnative.DatabaseMgr");
    return dbmgr;
}


// Expression Binding Utility

xcm_interface IDatabaseParserBinder : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IDatabaseParserBinder")
    
public:

    virtual void bindExprParser(void* parser) = 0;
};

XCM_DECLARE_SMARTPTR(IDatabaseParserBinder)


inline void bindExprParser(void* parser)
{
    IDatabaseParserBinderPtr e;
    e.create_instance("xdnative.DatabaseMgr");
    if (e)
    {
        e->bindExprParser(parser);
    }
}


// general utility functions

inline std::wstring dbtypeToString(int type)
{
    switch (type)
    {
        default:
        case xd::typeUndefined:     return L"undefined";    
        case xd::typeInvalid:       return L"invalid";      
        case xd::typeCharacter:     return L"character";    
        case xd::typeWideCharacter: return L"widecharacter";
        case xd::typeNumeric:       return L"numeric";      
        case xd::typeDouble:        return L"double";       
        case xd::typeInteger:       return L"integer";      
        case xd::typeDate:          return L"date";         
        case xd::typeDateTime:      return L"datetime";     
        case xd::typeBoolean:       return L"boolean";      
        case xd::typeBinary:        return L"binary";       
    }
}

inline int stringToDbtype(const std::wstring& type)
{
         if (type == L"undefined")     return xd::typeUndefined;
    else if (type == L"invalid")       return xd::typeInvalid;
    else if (type == L"character")     return xd::typeCharacter;
    else if (type == L"widecharacter") return xd::typeWideCharacter;
    else if (type == L"numeric")       return xd::typeNumeric;
    else if (type == L"double")        return xd::typeDouble;
    else if (type == L"integer")       return xd::typeInteger;
    else if (type == L"date")          return xd::typeDate;
    else if (type == L"datetime")      return xd::typeDateTime;
    else if (type == L"boolean")       return xd::typeBoolean;
    else if (type == L"binary")        return xd::typeBinary;
    else return xd::typeUndefined;
}



// general utility functions

inline std::wstring dbencodingToString(int type)
{
    switch (type)
    {
        default:
        case xd::encodingInvalid:   return L"invalid";    
        case xd::encodingUndefined: return L"undefined";      
        case xd::encodingASCII:     return L"ascii";    
        case xd::encodingUTF8:      return L"utf8";
        case xd::encodingUTF16:     return L"utf16";      
        case xd::encodingUTF32:     return L"utf32";       
        case xd::encodingUCS2:      return L"ucs2";      
        case xd::encodingUTF16BE:   return L"utf16be";         
        case xd::encodingISO8859_1: return L"iso8859-1";     
        case xd::encodingEBCDIC:    return L"ebcdic";      
        case xd::encodingCOMP:      return L"comp";       
        case xd::encodingCOMP3:     return L"comp3"; 
    }
}


inline int stringToDbencoding(const std::wstring& type)
{
         if (type == L"invalid")   return xd::encodingInvalid;
    else if (type == L"undefined") return xd::encodingUndefined;
    else if (type == L"ascii")     return xd::encodingASCII;
    else if (type == L"utf8")      return xd::encodingUTF8;
    else if (type == L"utf16")     return xd::encodingUTF16;
    else if (type == L"utf32")     return xd::encodingUTF32;
    else if (type == L"ucs2")      return xd::encodingUCS2;
    else if (type == L"utf16be")   return xd::encodingUTF16BE;
    else if (type == L"iso8859-1") return xd::encodingISO8859_1;
    else if (type == L"ebcdic")    return xd::encodingEBCDIC;
    else if (type == L"comp")      return xd::encodingCOMP;
    else if (type == L"comp3")     return xd::encodingCOMP3;
    else return xd::encodingInvalid;
}

inline bool isTypeCompatible(int type1, int type2)
{
    // determines if two tango types are compatible

    if (type1 == type2)
        return true;

    switch (type1)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:

            if (type2 == xd::typeCharacter ||
                type2 == xd::typeWideCharacter)
            {
                return true;
            }
            break;

        case xd::typeNumeric:
        case xd::typeDouble:
        case xd::typeInteger:

            if (type2 == xd::typeNumeric ||
                type2 == xd::typeDouble ||
                type2 == xd::typeInteger)
            {
                return true;
            }
            break;

        case xd::typeDate:
        case xd::typeDateTime:

            if (type2 == xd::typeDate || type2 == xd::typeDateTime)
                return true;
            break;
    }

    return false;
}


inline std::wstring quoteIdentifier(xd::IDatabasePtr db, const std::wstring& identifier)
{
    if (db.isOk())
    {
        xd::IAttributesPtr attr = db->getAttributes();
        if (attr)
        {
            std::wstring result;
            result += attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
            result += identifier;
            result += attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);
            return result;
        }
    }
    
    return identifier;
}

inline std::wstring quoteIdentifierIfNecessary(xd::IDatabasePtr db, const std::wstring& identifier)
{
     if (identifier.find(' ') == identifier.npos)
        return identifier;

    return quoteIdentifier(db, identifier);
}


inline std::wstring dequoteIdentifier(xd::IDatabasePtr db, const std::wstring& identifier)
{
    if (db.isOk())
    {
        xd::IAttributesPtr attr = db->getAttributes();
        if (attr)
        {
            size_t length = identifier.size();
            if (length == 0)
                return identifier;

            wchar_t open_char = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar)[0];
            wchar_t close_char = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar)[0];
            wchar_t first_char = identifier[0];
            wchar_t last_char = identifier[length-1];

            if (length > 1 && first_char == open_char && last_char == close_char)
            {
                if (length == 2)
                    return L"";
            
                std::wstring part = identifier.substr(0, length-1);
                return part.substr(1);
            }      
        }
    }

    return identifier;
}

inline void dequoteIdentifier(std::wstring& str, wchar_t ch1, wchar_t ch2)
{
    size_t len = str.length();
    if (len > 1)
    {
        if (str[0] == ch1 && str[len-1] == ch2)
        {
            str.erase(len-1, 1);
            str.erase(0, 1);
        }
    }
}



inline void requoteAllIdentifiers(xd::IDatabasePtr db, std::vector<std::wstring>& identifiers)
{
    std::vector<std::wstring> identifiers_copy = identifiers;
    std::vector<std::wstring>::iterator it, it_end;
    it_end = identifiers_copy.end();

    identifiers.clear();
    for (it = identifiers_copy.begin(); it != it_end; ++it)
    {
        std::wstring identifier = xd::dequoteIdentifier(db, *it);
        identifier = xd::quoteIdentifier(db, identifier);
        identifiers.push_back(identifier);
    }
}

inline std::wstring getTemporaryPath()
{
    int i;
    wchar_t tempname[33];
    memset(tempname, 0, 33 * sizeof(wchar_t));
    
    for (i = 0; i < 8; i++)
    {
        tempname[i] = L'a' + (rand() % 26);
    }

    unsigned int t = (unsigned int)time(NULL);
    unsigned int div = 308915776;    // 26^6;
    for (i = 8; i < 15; i++)
    {
        tempname[i] = L'a' + (t/div);
        t -= ((t/div)*div);
        div /= 26;
    }

    std::wstring temp_path;
    temp_path.append(L"xtmp_");
    temp_path.append(tempname);

    return temp_path;
}

inline bool isTemporaryPath(const std::wstring& path)
{
    if (path.find(L"xtmp_") != path.npos ||
        path.find(L".temp") != path.npos)
    {
        return true;
    }

    return false;
}


inline std::wstring appendPath(const std::wstring& path, const std::wstring& newpart, wchar_t ch = '/')
{
    if (newpart.empty()) return path;
    std::wstring ret = path;
    if (ret.length() > 0 && ret[ret.length()-1] != ch)
        ret += ch;
    if (newpart[0] == ch)
        ret += newpart.substr(1);
         else
        ret += newpart;
    return ret;
}


// DateTime support

class DateTime
{
public:

    DateTime()
    {
        yy = 0; mm = 0; dd = 0;
        h = 0; m = 0; s = 0; ms = 0;
    }

    DateTime(datetime_t datetime)
    {
        setDateTime(datetime);
    }

    DateTime(int _yy, int _mm, int _dd,
             int _h = 0, int _m = 0, int _s = 0, int _ms = 0)
    {
        yy = _yy; mm = _mm; dd = _dd; h = _h; m = _m; s = _s; ms = _ms;
    }

    DateTime(const DateTime& datetime)
    {
        yy = datetime.yy; mm = datetime.mm; dd = datetime.dd;
        h = datetime.h; m = datetime.m; s = datetime.s; ms = datetime.ms;
    }

    DateTime& operator=(const datetime_t& datetime)
    {
        setDateTime(datetime);
        return *this;
    }

    DateTime& operator=(const DateTime& datetime)
    {
        yy = datetime.yy; mm = datetime.mm; dd = datetime.dd;
        h = datetime.h; m = datetime.m; s = datetime.s; ms = datetime.ms;
        return *this;
    }

    bool operator==(const DateTime& datetime) const
    {
        return getDateTime() == datetime.getDateTime();
    }

    bool operator<(const DateTime& datetime) const
    {
        return getDateTime() < datetime.getDateTime();
    }

    bool operator>(const DateTime& datetime) const
    {
        return getDateTime() > datetime.getDateTime();
    }

    bool operator<=(const DateTime& datetime) const
    {
        return getDateTime() <= datetime.getDateTime();
    }

    bool operator>=(const DateTime& datetime) const
    {
        return getDateTime() >= datetime.getDateTime();
    }

    // accessor methods

    void setYear(int year) { yy = year; }
    void setMonth(int month) { mm = month; }
    void setDay(int day) { dd = day; }
    void setHour(int hour) { h = hour; }
    void setMinute(int minute) { m = minute; }
    void setSecond(int second) { s = second; }
    void setMillisecond(int millisecond) { ms = millisecond; }

    int getYear() const { return yy; }
    int getMonth() const { return mm; }
    int getDay() const { return dd; }
    int getHour() const { return h; }
    int getMinute() const { return m; }
    int getSecond() const { return s; }
    int getMillisecond() const { return ms; }

    bool isNull() const
    {
        return (yy == -4713);   // julian day 0 computes to this year
    }

    void setDateTime(datetime_t datetime)
    {
        setJulianDay((int)(datetime >> 32));
        setTimeStamp((int)(datetime & ((datetime_t)0xffffffff)));
    }

    datetime_t getDateTime() const
    {
        datetime_t dt;
        int julian;
        int time_stamp;

        int a, b, c;
        a = (14-mm)/12;
        b = yy + 4800 - a;
        c = mm + (12 * a) - 3;
        julian = (dd + (((153*c)+2)/5) + (365*b) + (b/4) - (b/100) + (b/400) - 32045);

        dt = julian;
        dt <<= 32;
        time_stamp = (h*3600000) + (m*60000) + (s*1000) + ms;

        dt |= time_stamp;
        return dt;
    }

    int getJulianDay() const
    {
        int a, b, c;
        a = (14-mm)/12;
        b = yy + 4800 - a;
        c = mm + (12 * a) - 3;
        return (dd + (((153*c)+2)/5) + (365*b) + (b/4) - (b/100) + (b/400) - 32045);
    }

    operator datetime_t() const
    {
        return getDateTime();
    }

    void setJulianDay(int julian_day)
    {
        int a, b, c, d, e, f;
        
        a = julian_day + 32044;
        b = ((4*a)+3)/146097;
        c = a - ((146097*b) / 4);
        d = ((4*c)+3)/1461;
        e = c - ((1461*d))/4;
        f = ((5*e)+2)/153;
        dd = e - (((153*f)+2)/5) + 1;
        mm = f + 3 - (12 * (f/10));
        yy = (100*b) + d - 4800 + (f/10);
    }

    void setTimeStamp(int time_stamp)
    {
        h = time_stamp / 3600000;
        time_stamp -= (h * 3600000);

        m = time_stamp / 60000;
        time_stamp -= (m * 60000);

        s = time_stamp / 1000;
        time_stamp -= (s * 1000);

        ms = time_stamp;
    }
    
protected:
    int yy, mm, dd, h, m, s, ms;
};








inline std::wstring xdtrim(const std::wstring& s)
{
    size_t first = s.find_first_not_of(L" \t\n\r", 0, 4);
    size_t last = s.find_last_not_of(L" \t\n\r", s.npos, 4);
    if (first == s.npos || last == s.npos)
        return L"";
         else
        return s.substr(first, last - first + 1);
}

class ConnectionStringParser
{
public:

    ConnectionStringParser()
    {
    }

    ConnectionStringParser(const std::wstring& str)
    {
        parse(str);
    }

    void parse(const std::wstring& str)
    {
        size_t next_pos = 0;
    
        m_map.clear();
        m_keys.clear();
    
        bool last = false;
    
        while (1)
        {
            std::wstring chunk, key, value;
        
            // get the next chunk
            size_t delim = str.find(L';', next_pos);
            if (delim == str.npos)
            {
                last = true;
                chunk = str.substr(next_pos, str.length() - next_pos);
            }
             else
            {
                chunk.assign(str, next_pos, delim-next_pos);
            }
            next_pos = delim+1;

            // get the key name
            size_t off = chunk.find('=');
            if (off != chunk.npos) key = xdtrim(chunk.substr(0, off));
            if (off != chunk.npos) value = xdtrim(chunk.substr(off+1));

            if (key.empty())
                value = L"";
            
            m_map[key] = value;
        
            // this separate vector of keys allows us to retain
            // the order of the values when we reconstruct a
            // connection string
            m_keys.push_back(key);
        
            if (last)
                break;
        }

    }

    // returns the value of the parameter, in lowercase
    std::wstring getLowerValue(const std::wstring& param)
    {
        std::wstring result = getValue(param);
        std::transform(result.begin(), result.end(), result.begin(), ::towlower);
        return result;
    }

    std::wstring getValue(const std::wstring& param)
    {
        std::map<std::wstring, std::wstring, xdcmpnocase>::iterator it;
        it = m_map.find(param);
    
        if (it == m_map.end())
            return L"";
        
        return it->second;
    }

    bool getValueExist(const std::wstring& param)
    {
        std::map<std::wstring, std::wstring, xdcmpnocase>::iterator it;
        it = m_map.find(param);
        return (it != m_map.end() ? true : false);
    }


    void setValue(const std::wstring& param, const std::wstring& value)
    {
        std::map<std::wstring, std::wstring, xdcmpnocase>::iterator it;
        it = m_map.find(param);
        if (it != m_map.end())
        {
            it->second = value;
        }
         else
        {
            m_map[param] = value;
            m_keys.push_back(param);
        }
    }


    std::wstring getConnectionString()
    {
        std::wstring result;
    
        std::vector<std::wstring>::iterator it;
        for (it = m_keys.begin(); it != m_keys.end(); ++it)
        {
            if (!result.empty())
                result += L";";
            result += *it;
            result += L"=";
            result += m_map[*it];
        }
    
        return result;
    }

private:

    std::map<std::wstring, std::wstring, xdcmpnocase> m_map;
    std::vector<std::wstring> m_keys;
};







}; // namespace xd


#endif
