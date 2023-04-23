/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#ifndef H_APP_SCRIPTSYSTEM_H
#define H_APP_SCRIPTSYSTEM_H


#include <wx/process.h>
#include <wx/txtstrm.h>


class SystemColors : public kscript::ValueObject
{
    typedef kscript::ValueObject BaseClass;

public:

    BEGIN_KSCRIPT_CLASS("SystemColors", SystemColors)
    END_KSCRIPT_CLASS()

public:

    SystemColors();
    ~SystemColors();
    void populate();

    void addColor(const std::wstring& var_name,
                  const wxColor& color);
    
    kscript::Value* getMember(const std::wstring& name);

private:

    bool m_populated;
};


class SystemMetrics : public kscript::ValueObject
{
    typedef kscript::ValueObject BaseClass;

public:

    BEGIN_KSCRIPT_CLASS("SystemMetrics", SystemMetrics)
    END_KSCRIPT_CLASS()

    SystemMetrics();
    ~SystemMetrics();
    void populate();

    void addMetric(const std::wstring& var_name,
                   const int metric);

    kscript::Value* getMember(const std::wstring& name);

private:

    bool m_populated;
};


class SystemFonts : public kscript::ValueObject
{
    typedef kscript::ValueObject BaseClass;

public:

    BEGIN_KSCRIPT_CLASS("SystemFonts", SystemFonts)
        KSCRIPT_STATIC_METHOD("getInstalledFonts", SystemFonts::getInstalledFonts)
    END_KSCRIPT_CLASS()

    SystemFonts();
    ~SystemFonts();

    void populate();
    
    void addFont(const std::wstring& var_name,
                 const wxFont& font);

    static void getInstalledFonts(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    
    kscript::Value* getMember(const std::wstring& name);

private:

    bool m_populated;

};



#endif

