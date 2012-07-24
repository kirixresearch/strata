/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-24
 *
 */


#ifndef __APP_SCRIPTFILE_H
#define __APP_SCRIPTFILE_H


class Directory : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Directory", Directory)
        KSCRIPT_METHOD("constructor", Directory::constructor)
        KSCRIPT_STATIC_METHOD("exists", Directory::exists)
        KSCRIPT_STATIC_METHOD("createDirectory", Directory::createDirectory)
        KSCRIPT_STATIC_METHOD("delete", Directory::zdelete)
        KSCRIPT_STATIC_METHOD("getAll", Directory::getAll)
        KSCRIPT_STATIC_METHOD("getFiles", Directory::getFiles)
        KSCRIPT_STATIC_METHOD("getDirectories", Directory::getDirectories)
    END_KSCRIPT_CLASS()

public:

    Directory();
    ~Directory();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    static void exists(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void createDirectory(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void zdelete(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void getAll(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void getFiles(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void getDirectories(kscript::ExprEnv* env, void*, kscript::Value* retval);

public:

    wxString m_path;
};



class zFileInfo : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("FileInfo", zFileInfo)
        KSCRIPT_METHOD("constructor", zFileInfo::constructor)
        KSCRIPT_METHOD("getLength", zFileInfo::getLength)
        KSCRIPT_METHOD("getLastWriteTime", zFileInfo::getLastWriteTime)
    END_KSCRIPT_CLASS()

public:

    zFileInfo();
    ~zFileInfo();

    static void staticConstructor(kscript::ExprEnv* env,
                                  void* param,
                                  kscript::Value* retval);
                                  
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getLength(kscript::ExprEnv* env, kscript::Value* retval);
    void getLastWriteTime(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    std::wstring m_filename;
};




class FileMode : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("FileMode", FileMode)

        KSCRIPT_METHOD("constructor", FileMode::constructor)
        
        KSCRIPT_CONSTANT_INTEGER("Create",       Create)
        KSCRIPT_CONSTANT_INTEGER("Open",         Open)
        KSCRIPT_CONSTANT_INTEGER("OpenOrCreate", OpenOrCreate)
        KSCRIPT_CONSTANT_INTEGER("Append",       OpenOrCreate)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Create = 1,
        Open = 2,
        OpenOrCreate = 3,
        Append = 4
    };

public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval) { }
};


class FileAccess : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("FileAccess", FileAccess)

        KSCRIPT_METHOD("constructor", FileAccess::constructor)

        KSCRIPT_CONSTANT_INTEGER("Read",      Read)
        KSCRIPT_CONSTANT_INTEGER("Write",     Write)
        KSCRIPT_CONSTANT_INTEGER("ReadWrite", ReadWrite)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Read = 1,
        Write = 2,
        ReadWrite = 3
    };

public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval) { }
};


class FileShare : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("FileShare", FileShare)

        KSCRIPT_METHOD("constructor", FileShare::constructor)

        KSCRIPT_CONSTANT_INTEGER("None",      None)
        KSCRIPT_CONSTANT_INTEGER("Read",      Read)
        KSCRIPT_CONSTANT_INTEGER("Write",     Write)
        KSCRIPT_CONSTANT_INTEGER("ReadWrite", ReadWrite)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        None = 1,
        Read = 2,
        Write = 3,
        ReadWrite = 4
    };

public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval) { }
};

class SeekOrigin : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("SeekOrigin", SeekOrigin)

        KSCRIPT_METHOD("constructor", SeekOrigin::constructor)

        KSCRIPT_CONSTANT_INTEGER("Begin",    Begin)
        KSCRIPT_CONSTANT_INTEGER("Current",  Current)
        KSCRIPT_CONSTANT_INTEGER("End",      End)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Begin = 1,
        Current = 2,
        End = 3
    };

public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval) { }
};



class File : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("File", File)
        KSCRIPT_METHOD("constructor", File::constructor)
        KSCRIPT_STATIC_METHOD("open", File::open)
        KSCRIPT_STATIC_METHOD("openText", File::openText)
        KSCRIPT_STATIC_METHOD("createText", File::createText)
        KSCRIPT_STATIC_METHOD("appendText", File::appendText)
        KSCRIPT_STATIC_METHOD("copy", File::copy)        
        KSCRIPT_STATIC_METHOD("delete", File::zdelete)
        KSCRIPT_STATIC_METHOD("move", File::move)
        KSCRIPT_STATIC_METHOD("exists", File::exists)
        KSCRIPT_STATIC_METHOD("getTempFilename", File::getTempFilename)
        KSCRIPT_STATIC_METHOD("putContents", File::putContents)
        KSCRIPT_STATIC_METHOD("getContents", File::getContents)
    END_KSCRIPT_CLASS()

public:

    File();
    ~File();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    static void open(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void openText(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void createText(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void appendText(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void copy(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void zdelete(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void move(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void exists(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getTempFilename(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void putContents(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void getContents(kscript::ExprEnv* env, void* param, kscript::Value* retval);
};



class FileStream : public kscript::ValueObject
{
    friend class File;

public:

    BEGIN_KSCRIPT_CLASS("FileStream", FileStream)
        KSCRIPT_METHOD("constructor", FileStream::constructor)
        KSCRIPT_METHOD("close", FileStream::close)
        KSCRIPT_METHOD("read", FileStream::read)
        KSCRIPT_METHOD("write", FileStream::write)
        KSCRIPT_METHOD("seek", FileStream::seek)
    END_KSCRIPT_CLASS()

    FileStream();
    ~FileStream();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void close(kscript::ExprEnv* env, kscript::Value* retval);
    void read(kscript::ExprEnv* env, kscript::Value* retval);
    void write(kscript::ExprEnv* env, kscript::Value* retval);
    void seek(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    xf_file_t m_f;
};



class TextReader : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("TextReader", TextReader)
        KSCRIPT_METHOD("constructor", TextReader::constructor)
        KSCRIPT_METHOD("readLine", TextReader::readLine)
        KSCRIPT_METHOD("close", TextReader::close)
    END_KSCRIPT_CLASS()

public:

    TextReader();
    ~TextReader();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void readLine(kscript::ExprEnv* env, kscript::Value* retval);
    void close(kscript::ExprEnv* env, kscript::Value* retval);

public:

    FILE* m_f;

#ifdef WIN32
    wchar_t* m_buf;
#else
    char* m_buf;
#endif

};


class TextWriter : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("TextWriter", TextWriter)
        KSCRIPT_METHOD("constructor", TextWriter::constructor)
        KSCRIPT_METHOD("write", TextWriter::write)
        KSCRIPT_METHOD("writeLine", TextWriter::writeLine)
        KSCRIPT_METHOD("close", TextWriter::close)
    END_KSCRIPT_CLASS()

public:

    TextWriter();
    ~TextWriter();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void write(kscript::ExprEnv* env, kscript::Value* retval);
    void writeLine(kscript::ExprEnv* env, kscript::Value* retval);
    void close(kscript::ExprEnv* env, kscript::Value* retval);

public:

    FILE* m_f;
};






class DriveInfo : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("DriveInfo", DriveInfo)
        KSCRIPT_METHOD("constructor", DriveInfo::constructor)
        KSCRIPT_STATIC_METHOD("getDrives", DriveInfo::getDrives)
        KSCRIPT_METHOD("getName", DriveInfo::getName)
        KSCRIPT_METHOD("getAvailableSpace", DriveInfo::getAvailableSpace)
        KSCRIPT_METHOD("getFreeSpace", DriveInfo::getFreeSpace)
        KSCRIPT_METHOD("getTotalSize", DriveInfo::getTotalSize)
    END_KSCRIPT_CLASS()

public:

    DriveInfo();
    ~DriveInfo();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    static void getDrives(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    
    void getName(kscript::ExprEnv* env, kscript::Value* retval);
    void getAvailableSpace(kscript::ExprEnv* env, kscript::Value* retval);
    void getFreeSpace(kscript::ExprEnv* env, kscript::Value* retval);
    void getTotalSize(kscript::ExprEnv* env, kscript::Value* retval);
    
    void retrieveInfo();
    
public:

    std::wstring m_name;
    bool m_initialized;
    double m_total_size;
    double m_available_space;
    double m_free_space;
};






class Log : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Log", Log)

        KSCRIPT_METHOD("constructor",   Log::constructor)
        KSCRIPT_METHOD("setFilterLevel", Log::setFilterLevel)
        KSCRIPT_METHOD("setConsoleOutput", Log::setConsoleOutput)
        KSCRIPT_METHOD("open",  Log::open)
        KSCRIPT_METHOD("close", Log::close)
        KSCRIPT_METHOD("erase", Log::erase)
        KSCRIPT_METHOD("debug", Log::debug)
        KSCRIPT_METHOD("info",  Log::info)
        KSCRIPT_METHOD("warn",  Log::warn)
        KSCRIPT_METHOD("error", Log::error)
        KSCRIPT_METHOD("fatal", Log::fatal)

        KSCRIPT_CONSTANT_INTEGER("LevelDebug", LevelDebug)
        KSCRIPT_CONSTANT_INTEGER("LevelInfo",  LevelInfo)
        KSCRIPT_CONSTANT_INTEGER("LevelWarn",  LevelWarn)
        KSCRIPT_CONSTANT_INTEGER("LevelError", LevelError)
        KSCRIPT_CONSTANT_INTEGER("LevelFatal", LevelFatal)
        KSCRIPT_CONSTANT_INTEGER("LevelAll",   LevelAll)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        LevelDebug = 1 << 1,
        LevelInfo =  1 << 2,
        LevelWarn =  1 << 3,
        LevelError = 1 << 4,
        LevelFatal = 1 << 5,
        LevelAll = 0x0fffffff
    };
    
public:

    Log();
    ~Log();

    void addLogLine(int level, const std::wstring& str);
    void setOutputFile(const std::wstring& filename);
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setFilterLevel(kscript::ExprEnv* env, kscript::Value* retval);
    void setConsoleOutput(kscript::ExprEnv* env, kscript::Value* retval);
    void open(kscript::ExprEnv* env, kscript::Value* retval);
    void close(kscript::ExprEnv* env, kscript::Value* retval);
    void erase(kscript::ExprEnv* env, kscript::Value* retval);
    void debug(kscript::ExprEnv* env, kscript::Value* retval);
    void info(kscript::ExprEnv* env, kscript::Value* retval);
    void warn(kscript::ExprEnv* env, kscript::Value* retval);
    void error(kscript::ExprEnv* env, kscript::Value* retval);
    void fatal(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    FILE* m_f;
    std::wstring m_path;
    int m_mask;
    bool m_console_output;
};



#endif

