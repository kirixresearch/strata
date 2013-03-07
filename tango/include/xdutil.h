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


#include <ctime>    // getTemporaryName()


namespace tango
{


// -- database manager instantiator --

inline IDatabaseMgrPtr getDatabaseMgr()
{
    IDatabaseMgrPtr dbmgr;
    dbmgr.create_instance("xdnative.DatabaseMgr");
    return dbmgr;
}

inline IConnectionStrPtr createConnectionStr()
{
    IConnectionStrPtr cstr;
    cstr.create_instance("xdnative.ConnectionStr");
    return cstr;
}



// -- Expression Binding Utility --

xcm_interface IDatabaseParserBinder : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IDatabaseParserBinder")
    
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


// -- general utility functions --

inline std::wstring dbtypeToString(int type)
{
    switch (type)
    {
        default:
        case tango::typeUndefined:     return L"undefined";    
        case tango::typeInvalid:       return L"invalid";      
        case tango::typeCharacter:     return L"character";    
        case tango::typeWideCharacter: return L"widecharacter";
        case tango::typeNumeric:       return L"numeric";      
        case tango::typeDouble:        return L"double";       
        case tango::typeInteger:       return L"integer";      
        case tango::typeDate:          return L"date";         
        case tango::typeDateTime:      return L"datetime";     
        case tango::typeBoolean:       return L"boolean";      
        case tango::typeBinary:        return L"binary";       
    }
}

inline int stringToDbtype(const std::wstring& type)
{
         if (type == L"undefined")     return tango::typeUndefined;
    else if (type == L"invalid")       return tango::typeInvalid;
    else if (type == L"character")     return tango::typeCharacter;
    else if (type == L"widecharacter") return tango::typeWideCharacter;
    else if (type == L"numeric")       return tango::typeNumeric;
    else if (type == L"double")        return tango::typeDouble;
    else if (type == L"integer")       return tango::typeInteger;
    else if (type == L"date")          return tango::typeDate;
    else if (type == L"datetime")      return tango::typeDateTime;
    else if (type == L"boolean")       return tango::typeBoolean;
    else if (type == L"binary")        return tango::typeBinary;
    else return tango::typeUndefined;
}

inline bool isTypeCompatible(int type1, int type2)
{
    // determines if two tango types are compatible

    if (type1 == type2)
        return true;

    switch (type1)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:

            if (type2 == tango::typeCharacter ||
                type2 == tango::typeWideCharacter)
            {
                return true;
            }
            break;

        case tango::typeNumeric:
        case tango::typeDouble:
        case tango::typeInteger:

            if (type2 == tango::typeNumeric ||
                type2 == tango::typeDouble ||
                type2 == tango::typeInteger)
            {
                return true;
            }
            break;

        case tango::typeDate:
        case tango::typeDateTime:

            if (type2 == tango::typeDate || type2 == tango::typeDateTime)
                return true;
            break;
    }

    return false;
}


inline std::wstring quoteIdentifier(tango::IDatabasePtr db, const std::wstring& identifier)
{
    if (db.isOk())
    {
        tango::IAttributesPtr attr = db->getAttributes();
        if (attr)
        {
            std::wstring result;
            result += attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
            result += identifier;
            result += attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);
            return result;
        }
    }
    
    return identifier;
}

inline std::wstring dequoteIdentifier(tango::IDatabasePtr db, const std::wstring& identifier)
{
    if (db.isOk())
    {
        tango::IAttributesPtr attr = db->getAttributes();
        if (attr)
        {
            size_t length = identifier.size();
            wchar_t open_char = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar)[0];
            wchar_t close_char = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar)[0];
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



inline void requoteAllIdentifiers(tango::IDatabasePtr db, std::vector<std::wstring>& identifiers)
{
    std::vector<std::wstring> identifiers_copy = identifiers;
    std::vector<std::wstring>::iterator it, it_end;
    it_end = identifiers_copy.end();

    identifiers.clear();
    for (it = identifiers_copy.begin(); it != it_end; ++it)
    {
        std::wstring identifier = tango::dequoteIdentifier(db, *it);
        identifier = tango::quoteIdentifier(db, identifier);
        identifiers.push_back(identifier);
    }
}

inline std::wstring getTemporaryPath()
{
    static unsigned int seed = (unsigned)time(NULL);
    srand(++seed);

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
    temp_path.append(L"/.temp/");
    temp_path.append(tempname);

    return temp_path;
}


// -- DateTime support --

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



}; // namespace tango


#endif
