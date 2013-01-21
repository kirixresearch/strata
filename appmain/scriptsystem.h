/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#ifndef __APP_SCRIPTSYSTEM_H
#define __APP_SCRIPTSYSTEM_H


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


class System : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("System", System)
        KSCRIPT_METHOD("constructor", System::constructor)
        KSCRIPT_STATIC_GUI_METHOD("beep", System::beep)
        KSCRIPT_STATIC_METHOD("time", System::ztime)
        KSCRIPT_STATIC_METHOD("clock", System::clock)
        KSCRIPT_STATIC_METHOD("getClocksPerSecond", System::getClocksPerSecond)
        KSCRIPT_STATIC_METHOD("sleep", System::sleep)        
        KSCRIPT_STATIC_GUI_METHOD("execute", System::execute)
    END_KSCRIPT_CLASS()

public:

    System();
    ~System();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

    static void beep(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void ztime(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void clock(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getClocksPerSecond(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void sleep(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void execute(kscript::ExprEnv* env, void* param, kscript::Value* retval);
};


#endif

