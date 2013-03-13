/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-03
 *
 */


#ifndef __APP_SCRIPTDB_H
#define __APP_SCRIPTDB_H


class DbError;


class DbDatabaseType : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("DbDatabaseType", DbDatabaseType)

        KSCRIPT_METHOD("constructor", DbDatabaseType::constructor)
        
        KSCRIPT_CONSTANT_INTEGER("Invalid",      Invalid)
        KSCRIPT_CONSTANT_INTEGER("Undefined",    Undefined)
        KSCRIPT_CONSTANT_INTEGER("Kirix",        Xdnative)  // backward compatibility
        KSCRIPT_CONSTANT_INTEGER("Xdnative",     Xdnative)
        KSCRIPT_CONSTANT_INTEGER("Package",      Package)
        KSCRIPT_CONSTANT_INTEGER("KirixPackage", Package)   // backward compatibility
        KSCRIPT_CONSTANT_INTEGER("Oracle",       Oracle)
        KSCRIPT_CONSTANT_INTEGER("SqlServer",    SqlServer)
        KSCRIPT_CONSTANT_INTEGER("MySQL",        MySQL)
        KSCRIPT_CONSTANT_INTEGER("Odbc",         Odbc)
        KSCRIPT_CONSTANT_INTEGER("Access",       Access)
        KSCRIPT_CONSTANT_INTEGER("Excel",        Excel)
        KSCRIPT_CONSTANT_INTEGER("Db2",          Db2)
        KSCRIPT_CONSTANT_INTEGER("Postgres",     Postgres)
        KSCRIPT_CONSTANT_INTEGER("Sqlite",       Sqlite)
        KSCRIPT_CONSTANT_INTEGER("Filesystem",   Filesystem)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Invalid = 0,
        Undefined = 100,
        Xdnative = 101,
        Package = 102,
        Oracle = 110,
        SqlServer = 111,
        MySQL = 112,
        Odbc = 113,
        Access = 120,
        Excel = 121,
        Db2 = 125,
        Postgres = 126,
        Sqlite = 127,
        Filesystem = 180
    };
    
public:    

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    
    static int toConnectionDatabaseType(int type);
    static int toTangoDatabaseType(int type);
    
    static int fromString(const std::wstring& str);
};


class DbEncoding : public kscript::ValueObject
{    
public:

    BEGIN_KSCRIPT_CLASS("DbEncoding", DbEncoding)

        KSCRIPT_METHOD("constructor", DbEncoding::constructor)
        
        KSCRIPT_CONSTANT_INTEGER("Invalid",   Invalid)
        KSCRIPT_CONSTANT_INTEGER("Undefined", Undefined)
        KSCRIPT_CONSTANT_INTEGER("ASCII",     ASCII)
        KSCRIPT_CONSTANT_INTEGER("UTF8",      UTF8)
        KSCRIPT_CONSTANT_INTEGER("UTF16",     UTF16)
        KSCRIPT_CONSTANT_INTEGER("UTF32",     UTF32)
        KSCRIPT_CONSTANT_INTEGER("UCS2",      UCS2)
        KSCRIPT_CONSTANT_INTEGER("EBCDIC",    EBCDIC)
        KSCRIPT_CONSTANT_INTEGER("COMP",      COMP)
        KSCRIPT_CONSTANT_INTEGER("COMP3",     COMP3)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Invalid = 0,
        Undefined = 200,
        ASCII = 201,
        UTF8 = 202,
        UTF16 = 203,
        UTF32 = 204,
        UCS2 = 205,
        EBCDIC = 281,
        COMP = 282,
        COMP3 = 283
    };
    
public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    
    static int toTangoEncoding(int type);
    static int fromTangoEncoding(int type);
};



class DbType : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("DbType", DbType)

        KSCRIPT_METHOD("constructor", DbType::constructor)
        
        KSCRIPT_CONSTANT_INTEGER("Undefined",     Undefined)
        KSCRIPT_CONSTANT_INTEGER("Invalid",       Invalid)
        KSCRIPT_CONSTANT_INTEGER("Character",     Character)
        KSCRIPT_CONSTANT_INTEGER("WideCharacter", WideCharacter)
        KSCRIPT_CONSTANT_INTEGER("Binary",        Binary)
        KSCRIPT_CONSTANT_INTEGER("Numeric",       Numeric)
        KSCRIPT_CONSTANT_INTEGER("Number",        Number)
        KSCRIPT_CONSTANT_INTEGER("Double",        Double)
        KSCRIPT_CONSTANT_INTEGER("Integer",       Integer)
        KSCRIPT_CONSTANT_INTEGER("Date",          Date)
        KSCRIPT_CONSTANT_INTEGER("DateTime",      DateTime)
        KSCRIPT_CONSTANT_INTEGER("Boolean",       Boolean)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Invalid = 0,
        Undefined = 100,
        Character = 101,
        WideCharacter = 102,
        Binary = 103,
        Numeric = 104,
        Number = 104,
        Double = 105,
        Integer = 106,
        Date = 107,
        DateTime = 108,
        Boolean = 109
    };

public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    
    static int toTangoType(kscript::Value* val);
    static int toTangoType(int type);
    static int fromTangoType(int type);
};



class DbInsertMode : public kscript::ValueObject
{    
public:

    BEGIN_KSCRIPT_CLASS("DbInsertMode", DbInsertMode)

        KSCRIPT_METHOD("constructor", DbInsertMode::constructor)
        
        KSCRIPT_CONSTANT_INTEGER("Invalid",   Invalid)
        KSCRIPT_CONSTANT_INTEGER("Create",    Create)
        KSCRIPT_CONSTANT_INTEGER("Append",    Append)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Invalid = 0,
        Create = 200,
        Append = 201
    };
    
public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};



class DbConnection : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("DbConnection", DbConnection)
        KSCRIPT_METHOD("constructor", DbConnection::constructor)
        KSCRIPT_METHOD("connect", DbConnection::connect)
        KSCRIPT_METHOD("bulkInsert", DbConnection::bulkInsert)
        KSCRIPT_METHOD("describeTable", DbConnection::describeTable)
        KSCRIPT_METHOD("execute", DbConnection::execute)
        KSCRIPT_METHOD("exists", DbConnection::exists)
        KSCRIPT_METHOD("save", DbConnection::save)
        KSCRIPT_METHOD("getObjects", DbConnection::getObjects)
        KSCRIPT_METHOD("isConnected", DbConnection::isConnected)
        KSCRIPT_METHOD("getError", DbConnection::getError)
        KSCRIPT_METHOD("enableExceptions", DbConnection::enableExceptions)
    END_KSCRIPT_CLASS()

public:

    DbConnection();
    ~DbConnection();
    
    void setDatabase(tango::IDatabasePtr db);
    tango::IDatabasePtr getDatabase();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void connect(kscript::ExprEnv* env, kscript::Value* retval);
    void bulkInsert(kscript::ExprEnv* env, kscript::Value* retval);
    void execute(kscript::ExprEnv* env, kscript::Value* retval);
    void exists(kscript::ExprEnv* env, kscript::Value* retval);
    void save(kscript::ExprEnv* env, kscript::Value* retval);
    void getObjects(kscript::ExprEnv* env, kscript::Value* retval);
    void describeTable(kscript::ExprEnv* env, kscript::Value* retval);
    void isConnected(kscript::ExprEnv* env, kscript::Value* retval);
    void getError(kscript::ExprEnv* env, kscript::Value* retval);
    void enableExceptions(kscript::ExprEnv* env, kscript::Value* retval);
    
    DbError* getErrorObject(kscript::ExprEnv* env);
    void clearError();
    
private:

    tango::IDatabasePtr m_db;
    bool m_exceptions_enabled;
    DbError* m_error;
};







class DbError : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("DbError", DbError)
        KSCRIPT_METHOD("constructor", DbError::constructor)
        KSCRIPT_METHOD("getCode", DbError::getCode)
        KSCRIPT_METHOD("getMessage", DbError::getMessage)
    END_KSCRIPT_CLASS()

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getCode(kscript::ExprEnv* env, kscript::Value* retval);
    void getMessage(kscript::ExprEnv* env, kscript::Value* retval);

public:

    int code;
    std::wstring message;
};



class DbException : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("DbException", DbException)
        KSCRIPT_METHOD("constructor", DbException::constructor)
        KSCRIPT_METHOD("getErrors", DbException::getErrors)
        KSCRIPT_METHOD("getError", DbException::getError)
    END_KSCRIPT_CLASS()

    DbException();
    ~DbException();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getError(kscript::ExprEnv* env, kscript::Value* retval);
    void getErrors(kscript::ExprEnv* env, kscript::Value* retval);

public:
    
    void addError(DbError* err);
    
public:

    std::vector<DbError*> m_errors;
};



class DbColumn : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("DbColumn", DbColumn)
        KSCRIPT_METHOD("constructor", DbColumn::constructor)
    END_KSCRIPT_CLASS()

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};







class DbObjectInfo : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("DbObjectInfo", DbObjectInfo)
        KSCRIPT_METHOD("constructor", DbObjectInfo::constructor)
    END_KSCRIPT_CLASS()

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};











struct DbBulkInsertColumn
{
    std::wstring name;
    int type;
    kscript::Value val;
    tango::objhandle_t handle;
};

class DbBulkInsert : public kscript::ValueObject
{
    typedef kscript::ValueObject BaseClass;


    BEGIN_KSCRIPT_CLASS("DbBulkInsert", DbBulkInsert)
        KSCRIPT_METHOD("constructor", DbBulkInsert::constructor)
        KSCRIPT_METHOD("insertRow", DbBulkInsert::insertRow)
        KSCRIPT_METHOD("finishInsert", DbBulkInsert::finishInsert)
        KSCRIPT_METHOD("flush", DbBulkInsert::flush)
    END_KSCRIPT_CLASS()

public:

    DbBulkInsert();
    ~DbBulkInsert();
    
    bool init(tango::IDatabasePtr db,
              const std::wstring& table,
              const std::wstring& columns);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void insertRow(kscript::ExprEnv* env, kscript::Value* retval);
    void finishInsert(kscript::ExprEnv* env, kscript::Value* retval);
    void flush(kscript::ExprEnv* env, kscript::Value* retval);
    
    void setMember(const std::wstring& name, kscript::Value* value);
    kscript::Value* getMember(const std::wstring& name);
    
private:

    tango::IRowInserterPtr m_sp_ri;
    tango::IRowInserter* m_ri;
    std::vector<DbBulkInsertColumn> m_cols;
};




struct DbResultColumn
{
    std::wstring name;
    int type;
    int scale;
    tango::objhandle_t handle;
};

class DbResult : public kscript::ValueObject
{
    typedef kscript::ValueObject BaseClass;

    BEGIN_KSCRIPT_CLASS("DbResult", DbResult)
        KSCRIPT_METHOD("constructor", DbResult::constructor)
        KSCRIPT_METHOD("getString", DbResult::getString)
        KSCRIPT_METHOD("getValue", DbResult::getValue)
        KSCRIPT_METHOD("getColumnCount", DbResult::getColumnCount)
        KSCRIPT_METHOD("getColumnName", DbResult::getColumnName)
        KSCRIPT_METHOD("getColumnInfo", DbResult::getColumnInfo)
        KSCRIPT_METHOD("next", DbResult::next)
        KSCRIPT_METHOD("hasNext", DbResult::hasNext)
        
        KSCRIPT_METHOD("seek", DbResult::seek)
        KSCRIPT_METHOD("seekLocation", DbResult::seekLocation)
    END_KSCRIPT_CLASS()

public:

    DbResult();
    ~DbResult();

    void init(tango::IIteratorPtr iter);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getString(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getColumnCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getColumnName(kscript::ExprEnv* env, kscript::Value* retval);
    void getColumnInfo(kscript::ExprEnv* env, kscript::Value* retval);
    void next(kscript::ExprEnv* env, kscript::Value* retval);
    void hasNext(kscript::ExprEnv* env, kscript::Value* retval);
    
    void seekCmn(kscript::ExprEnv* env, kscript::Value* retval, bool soft);
    void seek(kscript::ExprEnv* env, kscript::Value* retval);
    void seekLocation(kscript::ExprEnv* env, kscript::Value* retval);

public:

    // public members that aren't part of the script api
    kscript::Value* getMember(const std::wstring& name);
    tango::IStructurePtr getStructure();
    tango::IIteratorPtr getIterator();

private:

    void getColValue(int col, kscript::Value* val);
    DbResultColumn* lookupColumn(const std::wstring& col_name);
    int lookupColumnIdx(const std::wstring& col_name);

private:

    kscript::ExprParser* m_parser;
    tango::IIteratorPtr m_iter;
    tango::IStructurePtr m_structure;
    std::vector<DbResultColumn> m_cols;
    std::map<std::wstring, int, kl::cmp_nocase> m_cols_map;
    kscript::Value m_member_val;
    bool m_gofirst_called;
    bool m_eof;
    
    const wchar_t** m_seek_arr;
    const wchar_t** m_seek_data;
    size_t m_seek_arr_size;
};


#endif

