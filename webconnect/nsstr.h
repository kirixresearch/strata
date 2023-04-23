/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  wxWebConnect Embedded Web Browser Control Library
 * Author:   Benjamin I. Williams
 * Created:  2006-10-08
 *
 */


#ifndef H_WXWEBCONNECT_NSSTR_H
#define H_WXWEBCONNECT_NSSTR_H


struct nsStringContainer
{
    void* v;
    void* d1;
    PRUint32 d2;
    void* d3;
};


struct nsCStringContainer
{
    void* v;
    void* d1;
    PRUint32 d2;
    void* d3;
};


class nsAString
{
protected:

    nsAString() { }
    ~nsAString() { }
};


class nsACString
{
protected:

    nsACString() { }
    ~nsACString() { }
};


class nsString : public nsAString,
                 public nsStringContainer
{
public:
    nsString()
    {
        NS_StringContainerInit(*this);
    }
    
    nsString(const char16_t* data, size_t len, PRUint32 flags)
    {
        NS_StringContainerInit2(*this, data, len, flags);
    }
    
    ~nsString()
    {
        NS_StringContainerFinish(*this);
    }
    
    void Assign(const char16_t* str, int len = PR_UINT32_MAX)
    {
        NS_StringSetData(*this, str, len);
    }
};


class nsCString : public nsACString,
                  public nsCStringContainer
{
public:
    nsCString()
    {
        NS_CStringContainerInit(*this);
    }
    
    ~nsCString()
    {
        NS_CStringContainerFinish(*this);
    }
    
    nsCString(const char* data, size_t len, PRUint32 flags)
    {
        NS_CStringContainerInit2(*this, data, len, flags);
    }
    
    const char* get() const
    {
        const char* ptr;
        NS_CStringGetData(*this, &ptr);
        return ptr;
    }
    
    void Assign(const char* str, int len = PR_UINT32_MAX)
    {
        NS_CStringContainerFinish(*this);
        NS_CStringContainerInit2(*this, str, len);
    }
};


class nsDependentString : public nsString
{
public:
    nsDependentString(const char16_t* data, size_t len = PR_UINT32_MAX)
           : nsString(data, len, 0x02/*NS_STRING_CONTAINER_INIT_DEPEND*/)
    {
    }

#ifdef _MSC_VER

    nsDependentString(const wchar_t* data, size_t len = PR_UINT32_MAX)
           : nsString((const char16_t*)data, len, 0x02/*NS_STRING_CONTAINER_INIT_DEPEND*/)
    {
    }

#else
    // for platforms with 4-byte wchar_t
    nsDependentString(const wchar_t* wdata, size_t specified_len = PR_UINT32_MAX)
           : nsString()
    {
        size_t i, len = 0;
        // determine length
        while (*(wdata+len))
            len++;
        if (specified_len != PR_UINT32_MAX && specified_len < len)
            len = specified_len;
        char16_t* data = new char16_t[len+1];
        for (i = 0; i < len; ++i)
            data[i] = wdata[i];
        Assign(data, len);
        delete[] data;
    }
#endif

};


class nsDependentCString : public nsCString
{
public:
    nsDependentCString(const char* data, size_t len = PR_UINT32_MAX)
           : nsCString(data, len, 0x02/*NS_CSTRING_CONTAINER_INIT_DEPEND*/)
    {
    }
};


typedef nsString nsEmbedString;
typedef nsCString nsEmbedCString;


#define NS_LITERAL_STRING(s) nsDependentString(L##s)
#define NS_LITERAL_CSTRING(s) nsDependentCString(s)


#endif

