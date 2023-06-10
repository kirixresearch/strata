/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-03
 *
 */


#include "scripthost.h"
#include "db.h"
#include "../kscript/jsdate.h"
#include <kl/math.h>
#include <kl/string.h>
#include <kl/portable.h>




namespace scripthost
{



// -- DbDatabaseTypes class implementation --

// TODO: should we document these?

// (TODO:CLASS) DbDatabaseType
// Category: Database
// Derives:
// Description: A class that provides an enumeration of database types.
// Remarks: The DbDatabaseType class provides an enumeration of database types.
//
// Property(DbDatabaseType.Invalid) : An invalid database.
// Property(DbDatabaseType.Undefined) : An undefined database.
// Property(DbDatabaseType.Xdnative) : An Xdnative database.
// Property(DbDatabaseType.Package) : A .kpg package file.
// Property(DbDatabaseType.Oracle) : An Oracle database.
// Property(DbDatabaseType.SqlServer) : A Microsoft SqlServer database.
// Property(DbDatabaseType.Sqlite) : A Sqlite database.
// Property(DbDatabaseType.DuckDb) : A DuckDb database.
// Property(DbDatabaseType.MySQL) : A MySQL database.
// Property(DbDatabaseType.Odbc) : An ODBC connection.
// Property(DbDatabaseType.Access) : A Microsoft Access database.
// Property(DbDatabaseType.Excel) : A Microsoft Excel file.
// Property(DbDatabaseType.Db2) : A DB2 database.
// Property(DbDatabaseType.Postgres) : A Postres database.
// Property(DbDatabaseType.Filesystem) : A filesytem connection.


void DbDatabaseType::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

int DbDatabaseType::toXdDatabaseType(int binding_database_type)
{
    switch (binding_database_type)
    {
        case DbDatabaseType::Xdnative:     return xd::dbtypeXdnative;  
        case DbDatabaseType::Package:      return xd::dbtypeUndefined; 
        case DbDatabaseType::Oracle:       return xd::dbtypeOracle;    
        case DbDatabaseType::Postgres:     return xd::dbtypePostgres;  
        case DbDatabaseType::SqlServer:    return xd::dbtypeSqlServer; 
        case DbDatabaseType::MySQL:        return xd::dbtypeMySql;     
        case DbDatabaseType::Odbc:         return xd::dbtypeOdbc;      
        case DbDatabaseType::Access:       return xd::dbtypeAccess;    
        case DbDatabaseType::Excel:        return xd::dbtypeExcel;     
        case DbDatabaseType::Filesystem:   return xd::dbtypeFilesystem;
        case DbDatabaseType::Sqlite:       return xd::dbtypeSqlite;
        case DbDatabaseType::DuckDb:       return xd::dbtypeDuckDb;
        default:                           return xd::dbtypeUndefined; 
    }
}

int DbDatabaseType::fromString(const std::wstring& str)
{
         if (str == L"mssql")             return DbDatabaseType::SqlServer;
    else if (str == L"mysql")             return DbDatabaseType::MySQL;
    else if (str == L"oracle")            return DbDatabaseType::Oracle;
    else if (str == L"postgres")          return DbDatabaseType::Postgres;
    else if (str == L"odbc")              return DbDatabaseType::Odbc;
    else if (str == L"db2")               return DbDatabaseType::Db2;
    else if (str == L"package")           return DbDatabaseType::Package;
    else if (str == L"msaccess")          return DbDatabaseType::Access;
    else if (str == L"msexcel")           return DbDatabaseType::Excel;
    else if (str == L"xbase")             return DbDatabaseType::Filesystem;
    else if (str == L"delimited_text")    return DbDatabaseType::Filesystem;
    else if (str == L"fixed_length_text") return DbDatabaseType::Filesystem;
    else if (str == L"sqlite")            return DbDatabaseType::Sqlite;
    else if (str == L"duckdb")            return DbDatabaseType::DuckDb;
    else return DbDatabaseType::Invalid;
}



// -- DbTypes class implementation --

// (CLASS) DbType
// Category: Database
// Derives:
// Description: A class that provides an enumeration of database field types.
// Remarks: The DbType class provides an enumeration of database field types.
//
// Property(DbType.Undefined) : An undefined field type.
// Property(DbType.Invalid) : An invalid field type.
// Property(DbType.Character) : A character field type.
// Property(DbType.WideCharacter) : A wide character field type.
// Property(DbType.Binary) : A binary field type.
// Property(DbType.Numeric) : A numeric field type.
// Property(DbType.Number) : A number field type.
// Property(DbType.Double) : A double field type.
// Property(DbType.Integer) : An integer field type.
// Property(DbType.Date) : A date field type.
// Property(DbType.DateTime) : A datetime field type.
// Property(DbType.Boolean) : A boolean field type.


void DbType::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


int DbType::toXdType(kscript::Value* val)
{
    if (val->isString())
    {
        return xd::stringToDbtype(val->getString());
    }
     else
    {
        return DbType::toXdType(val->getInteger());
    }
    
    return xd::typeUndefined;
}


int DbType::toXdType(int type)
{
    switch (type)
    {
        default:
        case Undefined:       return xd::typeUndefined;
        case Invalid:         return xd::typeInvalid;
        case Character:       return xd::typeCharacter;
        case WideCharacter:   return xd::typeWideCharacter;
        case Binary:          return xd::typeBinary;
        case Numeric:         return xd::typeNumeric;
        case Double:          return xd::typeDouble;
        case Integer:         return xd::typeInteger;
        case Date:            return xd::typeDate;
        case DateTime:        return xd::typeDateTime;
        case Boolean:         return xd::typeBoolean;
    }
}

int DbType::fromXdType(int type)
{
    switch (type)
    {
        default:
        case xd::typeUndefined:     return Undefined;
        case xd::typeInvalid:       return Invalid;
        case xd::typeCharacter:     return Character;
        case xd::typeWideCharacter: return WideCharacter;
        case xd::typeBinary:        return Binary;
        case xd::typeNumeric:       return Numeric;
        case xd::typeDouble:        return Double;
        case xd::typeInteger:       return Integer;
        case xd::typeDate:          return Date;
        case xd::typeDateTime:      return DateTime;
        case xd::typeBoolean:       return Boolean;
    }
}



// -- DbInsertMode class implementation --

// TODO: document; note: this class is the flag field class
// for creating or appending a result set when using
// DbConnection::save()

void DbInsertMode::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// -- DbEncodings class implementation --

// (CLASS) DbEncoding
// Category: Database
// Derives:
// Description: A class that provides an enumeration of database encoding types.
// Remarks: The DbEncoding class provides an enumeration of database encoding types.
//
// Property(DbEncoding.Undefined) : An undefined encoding type.
// Property(DbEncoding.Invalid) : An invalid encoding type.
// Property(DbEncoding.ASCII) : ASCII encoding.
// Property(DbEncoding.UTF8) : UTF-8 Unicode encoding.
// Property(DbEncoding.UTF16) : UTF-16 Unicode encoding.
// Property(DbEncoding.UTF32) : UTF-32 Unicode encoding.
// Property(DbEncoding.UCS2) : UCS-2 Unicode encoding.
// Property(DbEncoding.EBCDIC) : EBCDIC encoding.
// Property(DbEncoding.COMP) : COBOL COMP numeric encoding.
// Property(DbEncoding.COMP3) : COBOL COMP-3 numeric encoding.


void DbEncoding::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

int DbEncoding::toXdEncoding(int type)
{
    switch (type)
    {
        default:
        case DbEncoding::Invalid:   return xd::encodingInvalid;
        case DbEncoding::Undefined: return xd::encodingUndefined;
        case DbEncoding::ASCII:     return xd::encodingASCII;
        case DbEncoding::UTF8:      return xd::encodingUTF8;
        case DbEncoding::UTF16:     return xd::encodingUTF16;
        case DbEncoding::UTF32:     return xd::encodingUTF32;
        case DbEncoding::UCS2:      return xd::encodingUCS2;
        case DbEncoding::EBCDIC:    return xd::encodingEBCDIC;
        case DbEncoding::COMP:      return xd::encodingCOMP;
        case DbEncoding::COMP3:     return xd::encodingCOMP3;
    }
}

int DbEncoding::fromXdEncoding(int type)
{
    switch (type)
    {
        default:
        case xd::encodingInvalid:   return DbEncoding::Invalid;
        case xd::encodingUndefined: return DbEncoding::Undefined;
        case xd::encodingASCII:     return DbEncoding::ASCII;
        case xd::encodingUTF8:      return DbEncoding::UTF8;
        case xd::encodingUTF16:     return DbEncoding::UTF16;
        case xd::encodingUTF32:     return DbEncoding::UTF32;
        case xd::encodingUCS2:      return DbEncoding::UCS2;
        case xd::encodingEBCDIC:    return DbEncoding::EBCDIC;
        case xd::encodingCOMP:      return DbEncoding::COMP;
        case xd::encodingCOMP3:     return DbEncoding::COMP3;
    }
}




// -- DbColumn class implementation --


// note: the DbColumn members and their values are set when 
// the DbColumn object is returned; TODO: is there a way to 
// define these members in the DbColumn definition, similar to 
// how it's done with DbType?

// (CLASS) DbColumn
// Category: Database
// Derives:
// Description: A class that provides information about a column.
// Remarks: The DbColumn provides information about a column.
//
// Property(DbColumn.name) : The name of the column, given as a string.
// Property(DbColumn.type) : The type of the column, where the type is given by one of the DbType values.
// Property(DbColumn.width) : The width of the column, given as a number.
// Property(DbColumn.scale) : The scale of the column, given as a number.
// Property(DbColumn.expression) : The formula of the column, given as a string. If the column doesn't have a formula, the string is blank.


void DbColumn::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{

}







// -- DbObjectInfo class implementation --

// note: the DbObjectInfo members and their values are set when 
// an array of DbObjectInfo objects are returned; TODO: is there a 
// way to define these members in the DbObjectInfo definition, similar to 
// how it's done with DbType?

// (CLASS) DbObjectInfo
// Category: Database
// Derives:
// Description: A class that provides information about a database object.
// Remarks: The DbObjectInfo class provides information about a database object.
//
// Property(DbObjectInfo.name): The name of the object, given as a string.
// Property(DbObjectInfo.type) : The type of the object, given as a string.  Either "NODE", "DIRECTORY", "TABLE", "STREAM", or "UNKNOWN".
// Property(DbObjectInfo.catalog) : The catalog of the object, given as a string.  Currently blank.
// Property(DbObjectInfo.schema) : The schema of the object, given as a string.  Currently blank.
// Property(DbObjectInfo.mount): The object mount flag, given as a boolean value.  True if the object is mounted, and false otherwise.

void DbObjectInfo::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{

}





// -- DbResult class implementation --


// (CLASS) DbResult
// Category: Database
// Description: A class that manages results from a database query operation.
// Remarks: The DbResult class is used to retrieve tabular data from a
//     result set.  After a query has successfully been run, a DbResult
//     object is returned from the DbConnection.execute() function.  Using
//     this object, the caller can iterate through all rows in the result
//     set and retrieve the data contained in the rows.   The DbResult
//     object can be used much like an array, with field values becoming
//     named elements in that array.  When the next row is fetched using the
//     next() function, these values are replaced with the next row's data.

DbResult::DbResult()
{
    m_parser = NULL;
    m_gofirst_called = false;
    m_eof = false;
    
    m_seek_arr = NULL;
    m_seek_arr_size = 0;
}

DbResult::~DbResult()
{
}


void DbResult::init(xd::IIteratorPtr iter)
{
    m_iter = iter;
    m_structure = m_iter->getStructure();

    size_t i, colcount = m_structure.getColumnCount();

    m_cols.clear();
    m_cols_map.clear();
    
    for (i = 0; i < colcount; ++i)
    {
        const xd::ColumnInfo& colinfo = m_structure.getColumnInfoByIdx(i);

        DbResultColumn c;
        c.name = colinfo.name;
        c.type = colinfo.type;
        c.scale = colinfo.scale;
        c.handle = m_iter->getHandle(c.name);

        // this line makes the column members appear
        // visably (in situations like 'for (f in result)'...
        BaseClass::getMember(c.name);
        
        m_cols.push_back(c);
        m_cols_map[c.name] = i;
    }
}


DbResultColumn* DbResult::lookupColumn(const std::wstring& col_name)
{
    std::map<std::wstring, int, kl::cmp_nocase>::iterator it;
    it = m_cols_map.find(col_name);
    if (it == m_cols_map.end())
        return NULL;
    return &m_cols[it->second];
}

int DbResult::lookupColumnIdx(const std::wstring& col_name)
{
    std::map<std::wstring, int, kl::cmp_nocase>::iterator it;
    it = m_cols_map.find(col_name);
    if (it == m_cols_map.end())
        return -1;
    return it->second;
}


void DbResult::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_parser = env->m_parser;
}

void DbResult::getString(kscript::ExprEnv* env, kscript::Value* retval)
{
    DbResultColumn* col = NULL;

    if (env->getParamCount() != 1)
    {
        retval->setNull();
        return;
    }

    kscript::Value* param = env->getParam(0);

    if (param->isString())
    {
        col = lookupColumn(param->getString());
    }
     else if (param->isNumber())
    {
        int idx = param->getInteger();
        if (idx >= 0 && (size_t)idx < m_cols.size())
        {
            col = &(m_cols[idx]);
        }
    }

    if (!col)
    {
        retval->setNull();
        return;
    }

    switch (col->type)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
        {
            retval->setString(m_iter->getWideString(col->handle));
            break;
        }

        case xd::typeNumeric:
        case xd::typeDouble:
        {
            wchar_t buf[80];
            swprintf(buf, 79, L"%.*f",
                     col->scale,
                     kl::dblround(m_iter->getDouble(col->handle),
                                  col->scale));
            retval->setString(buf);
            break;
        }

        case xd::typeInteger:
        {
            wchar_t buf[80];
            swprintf(buf,
                     79,
                     L"%d",
                     m_iter->getInteger(col->handle));
            retval->setString(buf);
            break;
        }

        case xd::typeDate:
        {
            wchar_t buf[80];

            xd::DateTime dt = m_iter->getDateTime(col->handle);

            swprintf(buf,
                     79,
                     L"%04d-%02d-%02d",
                     dt.getYear(),
                     dt.getMonth(),
                     dt.getDay());
            retval->setString(buf);
            break;
        }

        case xd::typeDateTime:
        {
            wchar_t buf[80];

            xd::DateTime dt = m_iter->getDateTime(col->handle);

            swprintf(buf,
                     79,
                     L"%04d-%02d-%02d %02d:%02d:%02d",
                     dt.getYear(),
                     dt.getMonth(),
                     dt.getDay(),
                     dt.getHour(),
                     dt.getMinute(),
                     dt.getSecond());
            
            retval->setString(buf);
            
            break;
        }

        case xd::typeBoolean:
        {
            if (m_iter->getBoolean(col->handle))
                retval->setString(L"true");
                 else
                retval->setString(L"false");

            break;
        }
    }
}

// (METHOD) DbResult.getColumnCount
// Description: Returns the number of columns available in the result set
//
// Syntax: function DbResult.getColumnCount() : Integer
//
// Remarks: Returns the number of columns available in the result set.  The
//     field values can be referenced by either their field name or the
//     zero-based field index.
//
// Returns: An Integer which indicates the number of columns available
//     in the result set.

void DbResult::getColumnCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(m_cols.size());
}

// (METHOD) DbResult.getColumnName
// Description: Returns the name of the column for a given column index
//
// Syntax: function DbResult.getColumnName(index : Integer) : String
//
// Remarks: Returns the name of the column specified by the
//     column index passed in the |index| parameter.  Column indexes
//     are zero-based, meaning that the first column is 0, the second 1,
//     and so on.
//
// Returns: The column name in the result set as specified by the field
//     index.  If the index is out of range, an empty string is returned

void DbResult::getColumnName(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setString(L"");
        return;
    }
    
    int col = env->getParam(0)->getInteger();
    
    if (col < 0 || (size_t)col >= m_cols.size())
    {
        retval->setString(L"");
        return;
    }
    
    retval->setString(m_structure.getColumnName(col));
}

// (METHOD) DbResult.getColumnInfo
// Description: Returns information about a specified column
//
// Syntax: function DbResult.getColumnInfo(name : String) : DbColumn
// Syntax: function DbResult.getColumnInfo(index : Integer) : DbColumn
//
// Remarks: Returns a DbColumn object with information about the column
//     specified in the parameter (either by zero-based column index or
//     column name).  If the specified column index is out of range, or the
//     specified column name doesn't exist, null is returned.
//
// Returns: A DbColumn object containing the column information for the
//     specified column. If the specified column index is out of range, 
//     or the specified column name doesn't exist, null is returned.

void DbResult::getColumnInfo(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    xd::ColumnInfo col;
    
    if (env->getParam(0)->isString() ||
        env->getParam(0)->isObject())
    {
        std::wstring colname = env->getParam(0)->getString();
        col = m_structure.getColumnInfo(colname);
    }
     else
    {
        col = m_structure.getColumnInfoByIdx(env->getParam(0)->getInteger());
    }
    
    
    if (col.isNull())
    {
        retval->setNull();
        return;
    }

    // TODO: if members are added, make sure to change
    // documentation next to class definition, as well
    // as the other instance where DbColumn is used
    DbColumn* dbcol = DbColumn::createObject(env);
    dbcol->getMember(L"name")->setString(col.name);
    dbcol->getMember(L"type")->setInteger(DbType::fromXdType(col.type));
    dbcol->getMember(L"width")->setInteger(col.width);
    dbcol->getMember(L"scale")->setInteger(col.scale);
    dbcol->getMember(L"expression")->setString(col.expression);
    
    retval->setObject(dbcol);
}




// (METHOD) DbResult.getValue
// Description: Returns the name of the column for a given column index
//
// Syntax: function DbResult.getValue(column_name : String) : Object
// Syntax: function DbResult.getValue(column_index : Integer) : Object
//
// Remarks: This function yields the same results that using the array
//     operator does.  A call to the method returns the value of the field
//     specified by either the |column_name| or |column_index| parameter.
//     The type of the return value is automatically determined by the
//     column's type.  Fields with a date or date/time type are returned
//     as a string, which can be easily cast into a Date object.
//
// Param(column_name): The name of the desired column
// Param(column_index): The zero-based index of the desired column
// Returns: An object containing the field value, or null if an error
//     occurred.

void DbResult::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    int colidx = -1;

    if (env->getParamCount() != 1)
    {
        retval->setNull();
        return;
    }

    kscript::Value* param = env->getParam(0);

    if (param->isString())
    {
        colidx = lookupColumnIdx(param->getString());
    }
     else if (param->isNumber())
    {
        colidx = param->getInteger();
    }

    if (colidx < 0 || (size_t)colidx >= m_cols.size())
    {
        retval->setNull();
        return;
    }

    getColValue(colidx, retval);
}


void DbResult::getColValue(int colidx, kscript::Value* val)
{
    if (colidx < 0 || (size_t)colidx >= m_cols.size() || m_iter.isNull())
    {
        val->setNull();
        return;
    }

    DbResultColumn* col = &m_cols[colidx];

    if (col == NULL || m_iter->isNull(col->handle))
    {
        val->setNull();
        return;
    }
    

    switch (col->type)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
        {
            val->setString(m_iter->getWideString(col->handle));
            break;
        }

        case xd::typeNumeric:
        case xd::typeDouble:
        {
            val->setDouble(kl::dblround(m_iter->getDouble(col->handle),
                                        col->scale));
            break;
        }

        case xd::typeInteger:
        {
            val->setInteger(m_iter->getInteger(col->handle));
            break;
        }

        
        case xd::typeDate:
        {
            wchar_t buf[80];

            xd::datetime_t d = m_iter->getDateTime(col->handle);
            if (d == 0)
            {
                val->setString(L"");
                break;
            }
                
            xd::DateTime dt = d;

            swprintf(buf,
                     79,
                     L"%04d-%02d-%02d",
                     dt.getYear(),
                     dt.getMonth(),
                     dt.getDay());
            val->setString(buf);
            break;
        }

        case xd::typeDateTime:
        {
            wchar_t buf[80];

            xd::datetime_t d = m_iter->getDateTime(col->handle);
            if (d == 0)
            {
                val->setString(L"");
                break;
            }
                
            xd::DateTime dt = d;
            
            swprintf(buf,
                     79,
                     L"%04d-%02d-%02d %02d:%02d:%02d",
                     dt.getYear(),
                     dt.getMonth(),
                     dt.getDay(),
                     dt.getHour(),
                     dt.getMinute(),
                     dt.getSecond());
                     
            val->setString(buf);
            break;
        }


        case xd::typeBoolean:
        {
            val->setBoolean(m_iter->getBoolean(col->handle));
            break;
        }
    }
}

// (METHOD) DbResult.next
// Description: Moves the row pointer to the next row in the result set
//
// Syntax: function DbResult.next() : Boolean
//
// Remarks: Calling next() moves the row pointer to the next row in the
//     result set.  Also, after running a query, it is used to place the
//     row pointer to the first row in the result set.  If the end of the
//     result set is reached, the function will return false.  If the call
//     succeeded and a new row is available for reading, the call will
//     return true.
//
// Returns: True if the row pointer was successfully moved to the next row,
//     or false if the end of the result set was reached.

void DbResult::next(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_gofirst_called)
    {
        m_gofirst_called = true;
        m_iter->goFirst();
        m_eof = m_iter->eof();
        retval->setBoolean(!m_eof);
        return;
    }
    
    
    if (m_eof)
    {
        retval->setBoolean(false);
        return;
    }
    
    m_iter->skip(1);
    
    if (m_iter->eof())
    {
        m_eof = true;
        retval->setBoolean(false);
        return;
    }
    
    retval->setBoolean(true);
}


// (METHOD) DbResult.hasNext
// Description: Determines if the end of the result set has been reached
//
// Syntax: function DbResult.hasNext() : Boolean
//
// Remarks: Returns true if a call to next() will move the row to a valid
//     record.  If the row pointer is currently positioned on the last row
//     of the result set, the call will return false.
//
// Returns: True if the row pointer does not currently point to the
//     last record in the result set, otherwise false.

void DbResult::hasNext(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_gofirst_called)
    {
        m_iter->goFirst();
        m_eof = m_iter->eof();
        m_gofirst_called = true;
    }

    retval->setBoolean(!m_eof);
}

kscript::Value* DbResult::getMember(const std::wstring& name)
{
    std::vector<DbResultColumn>::iterator it, it_end;
    int colidx;
    
    if (name.length() > 0 && iswdigit(name[0]))
    {
        colidx = kl::wtoi(name);
        if (colidx < 0 || colidx >= (int)m_cols.size())
            return ValueObject::getMember(name);
    }
     else
    {
        colidx = lookupColumnIdx(name);
    }
    
    
    if (colidx != -1)
    {
        if (!m_gofirst_called)
        {
            m_member_val.reset();
            return &m_member_val;
        }
        
        getColValue(colidx, &m_member_val);
        return &m_member_val;
    }

    return ValueObject::getMember(name);
}

xd::Structure DbResult::getStructure()
{
    return m_structure;
}

xd::IIteratorPtr DbResult::getIterator()
{
    return m_iter;
}

void DbResult::seekCmn(kscript::ExprEnv* env, kscript::Value* retval, bool soft)
{
    kscript::ValueObject* arr = NULL;
    size_t param_count = env->getParamCount();
    std::vector<wchar_t*> to_delete;
    kscript::Value* param;
    
    if (env->getParam(0)->isObject() &&
        env->getParam(0)->getObject()->isKindOf(kscript::Array::staticGetClassId()))
    {
        arr = env->getParam(0)->getObject();
        param_count = arr->getRawMemberCount();
    }
    
    if (param_count > m_seek_arr_size)
    {
        delete[] m_seek_arr;
        m_seek_arr_size = param_count;
        m_seek_arr = new const wchar_t*[m_seek_arr_size];
    }
    
    for (size_t i = 0; i < m_seek_arr_size; ++i)
    {
        if (arr)
            param = arr->getRawMemberByIdx(i);
             else
            param = env->getParam(i);
            
        if (param->isObject())
        {
            kscript::ValueObject* obj = param->getObject();
            if (obj->isKindOf(kscript::Date::staticGetClassId()))
            {
                kscript::Date* dt = (kscript::Date*)obj;
                int year, month, day, hour, minute, second, ms;
                dt->getDateTime(&year, &month, &day, &hour, &minute, &second, &ms);
                
                wchar_t* str = new wchar_t[64];
                to_delete.push_back(str);
                swprintf(str, 64, L"%04d-%02d-%02d %02d:%02d:%02d",
                                   year, month+1, day, hour, minute, second);
                m_seek_arr[i] = str;
                continue;
            }
        }
        
        m_seek_arr[i] = param->getString();
    }
    
    bool res = m_iter->seekValues(m_seek_arr, m_seek_arr_size, soft);
    if (res || soft)
        m_gofirst_called = true;
        
    retval->setBoolean(res);
    
    if (to_delete.size() > 0)
    {
        for (size_t i = 0; i < to_delete.size(); ++i)
            delete[] to_delete[i];
    }
}

void DbResult::seek(kscript::ExprEnv* env, kscript::Value* retval)
{
    seekCmn(env, retval, false);
}

void DbResult::seekLocation(kscript::ExprEnv* env, kscript::Value* retval)
{
    // performs a "soft" seek
    seekCmn(env, retval, true);
}



// -- DbBulkInsert class implementation --


// (CLASS) DbBulkInsert
// Category: Database
// Description: A class that provides the ability to insert many rows into database 
//     tables quickly.
// Remarks: The DbBulkInsert class provides a mechanism for inserting rows
//     quickly and efficiently into a database table.  DbBulkInsert uses an
//     internal buffer to optimize performance during the bulk insert operation.

DbBulkInsert::DbBulkInsert()
{
    m_ri = NULL;
}

DbBulkInsert::~DbBulkInsert()
{
    // if script failed to call finishInsert(),
    // do it for them
    
    if (m_ri)
    {
        m_ri->finishInsert();
    }
}

bool DbBulkInsert::init(xd::IDatabasePtr db,
                        const std::wstring& table,
                        const std::wstring& _columns)
{
    std::wstring columns = _columns;
    kl::trim(columns);
        
    xd::Structure structure = db->describeTable(table);
    if (!structure.isOk())
        return false;
        
    m_sp_ri = db->bulkInsert(table);
    if (!m_sp_ri)
        return false;
    
    if (!m_sp_ri->startInsert(columns))
        return false;
    
    m_ri = m_sp_ri.p;
    

    if (columns == L"*")
    {
        size_t i, col_count = structure.getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            const xd::ColumnInfo& sp_col = structure.getColumnInfoByIdx(i);
            
            DbBulkInsertColumn col;
            col.name = sp_col.name;
            col.type = sp_col.type;
            col.handle = m_ri->getHandle(col.name);
            
            m_cols.push_back(col);
        }
    }
     else
    {
        std::vector<std::wstring> parts;
        std::vector<std::wstring>::iterator it;
        kl::parseDelimitedList(columns, parts, ',', true);
        for (it = parts.begin(); it != parts.end(); ++it)
        {
            std::wstring token = *it;
            kl::trim(token);
            
            const xd::ColumnInfo& colinfo = structure.getColumnInfo(token);
            if (colinfo.isNull())
            {
                // column not found, fail
                m_ri = NULL;
                m_sp_ri.clear();
                return false;
            }
            
            DbBulkInsertColumn col;
            col.name = colinfo.name;
            col.type = colinfo.type;
            col.handle = m_ri->getHandle(col.name);
            
            m_cols.push_back(col);
        }
    }
    
    return true;
}


void DbBulkInsert::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


static bool parseBulkInsertDateTime(const std::wstring& input,
                          int* year,
                          int* month,
                          int* day,
                          int* hour,
                          int* minute,
                          int* second)
{
    static int month_days[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    *year = -1;
    *month = -1;
    *day = -1;
    *hour = -1;
    *minute = -1;
    *second = -1;

    std::vector<int> parts;

    // parse out the parts
    wchar_t part[80];
    int idx = 0;

    part[0] = 0;

    const wchar_t* p = input.c_str();
    while (1)
    {
        if (*p == 0 || wcschr(L"/-.: ", *p))
        {
            part[idx] = 0;
            if (wcslen(part) > 0)
                parts.push_back(kl::wtoi(part));
            
            if (!*p)
                break;

            part[0] = 0;
            idx = 0;
            ++p;
            continue;
        }

        part[idx] = *p;
        ++idx;
        ++p;
    }


    // extract information

    int part_count = parts.size();

    if (part_count < 3)
        return false;

    int y = 0, m = 0, d = 0, hh = -1, mm = -1, ss = 0;

    // first assign date portion
    y = parts[0];
    m = parts[1];
    d = parts[2];

    if (part_count == 4)
        return false;

    if (part_count >= 5)
    {
        // time is included
        hh = parts[3];
        mm = parts[4];

        if (part_count > 5)
            ss = parts[5];
    }


    // check validity
    if (m <= 0 || m > 12)
        return false;
    
    if (d <= 0 || d > month_days[m-1])
        return false;

    if (y < 70)
        y += 2000;
    else if (y < 100)
        y += 1900;

    if (part_count >= 5)
    {
        if (hh == 24)
            hh = 0;

        if (hh < 0 || hh >= 24)
            return false;

        if (m < 0 || mm >= 60)
            return false;
        
        if (part_count > 5)
        {
            if (ss < 0 || ss >= 60)
                return false;
        }
    }

    *year = y;
    *month = m;
    *day = d;
    *hour = hh;
    *minute = mm;
    *second = ss;

    return true;
}


// (METHOD) DbBulkInsert.insertRow
// Description: Inserts a row into a table
//
// Syntax: function DbBulkInsert.insertRow() : Boolean
//
// Remarks: Calling insertRow() adds a new row to the target table.  The
//     field values for the new row should be set before calling this function.
//
// Returns: True upon success, false otherwise

void DbBulkInsert::insertRow(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);

    if (!m_ri)
        return;

    std::wstring class_name;
    std::vector<DbBulkInsertColumn>::iterator it, it_end;
    it_end = m_cols.end();
 
    // if an object was specified in the parameter, dump the
    // fields from that object/array into our own members, and
    // then into the file
    
    if (env->getParamCount() > 0 && env->getParam(0)->isObject())
    {
        kscript::Value* obj = env->getParam(0);
        
        for (it = m_cols.begin(); it != it_end; ++it)
        {
            it->val.setValue(obj->getMember(it->name));
        }
    }
    
    
    
    for (it = m_cols.begin(); it != it_end; ++it)
    {
        if (it->val.isNull())
        {
            m_ri->putNull(it->handle);
            continue;
        }
        
        switch (it->type)
        {
            case xd::typeCharacter:
            case xd::typeWideCharacter:
            {
                m_ri->putWideString(it->handle, it->val.getString());
                break;
            }

            case xd::typeNumeric:
            case xd::typeDouble:
            {
                m_ri->putDouble(it->handle, it->val.getDouble());
                break;
            }

            case xd::typeInteger:
            {
                m_ri->putInteger(it->handle, it->val.getInteger());
                break;
            }

            case xd::typeDateTime:
            case xd::typeDate:
            {
                int yy=-1,mm,dd,h=-1,m,s,ms = 0;
                
                if (it->val.isObject())
                {
                    kscript::ValueObject* obj = it->val.getObject();
                    class_name = obj->getClassName();
                    if (class_name == L"Date")
                    {
                        kscript::Date* d = (kscript::Date*)obj;
                        d->getDateTime(&yy, &mm, &dd, &h, &m, &s, &ms);
                        mm++;
                    }
                }
                
                if (yy == -1)
                {
                    if (!parseBulkInsertDateTime(it->val.getString(), &yy, &mm, &dd, &h, &m, &s))
                    {
                        // bad date, put a null instead (see below)
                        yy = -1;
                    }
                }
                
                if (it->type == xd::typeDate || h == -1)
                {
                    h = 0;
                    m = 0;
                    s = 0;
                }
                
                if (yy == -1)
                {
                    m_ri->putNull(it->handle);
                }
                 else
                {    
                    xd::DateTime dt(yy,mm,dd,h,m,s,ms);
                    m_ri->putDateTime(it->handle, dt);
                }
                
                break;
            }
            
            case xd::typeBoolean:
            {
                m_ri->putBoolean(it->handle, it->val.getBoolean());
                break;
            }
        }
    }
    
    
    
    retval->setBoolean(m_ri->insertRow());
}



// (METHOD) DbBulkInsert.finishInsert
// Description: Finalizes the bulk insert operation
//
// Syntax: function DbBulkInsert.finishInsert()
//
// Remarks: After inserting the desired number of rows with insertRow(),
//     the caller must call finishInsert() to finalize the insert operation.
//     DbBulkInsert uses an internal buffer to optimize performance during
//     the bulk insert operation.  When finishInsert() is called, this internal
//     buffer is flushed and written to the database table.
//
// Returns: Undefined


void DbBulkInsert::finishInsert(kscript::ExprEnv* env, kscript::Value* retval)
{ 
    retval->setUndefined();
      
    if (!m_ri)
        return;
    
    m_ri->finishInsert();
    
    m_ri = NULL;
    m_sp_ri.clear();
}


void DbBulkInsert::flush(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
      
    if (!m_ri)
        return;
        
    retval->setBoolean(m_ri->flush());
}


void DbBulkInsert::setMember(const std::wstring& name, kscript::Value* value)
{
    if (name.length() == 0)
        return;
        
    if (iswdigit(name[0]))
    {
        int idx = kl::wtoi(name);
        if (idx < 0 || idx >= (int)m_cols.size())
            return;

        m_cols[idx].val.setValue(value);
    }
     else
    {
        std::vector<DbBulkInsertColumn>::iterator it, it_end;

        it_end = m_cols.end();
        for (it = m_cols.begin(); it != it_end; ++it)
        {
            if (0 == wcscasecmp(it->name.c_str(), name.c_str()))
            {
                it->val.setValue(value);
                return;
            }
        }
    }
    
    ValueObject::setMember(name, value);
}


kscript::Value* DbBulkInsert::getMember(const std::wstring& name)
{
    if (name.length() == 0)
    {
        return BaseClass::getMember(name);
    }
        
    if (iswdigit(name[0]))
    {
        int idx = kl::wtoi(name);
        if (idx < 0 || idx >= (int)m_cols.size())
            return BaseClass::getMember(name);

        return &m_cols[idx].val;
    }
     else
    {
        std::vector<DbBulkInsertColumn>::iterator it, it_end;
        it_end = m_cols.end();
        for (it = m_cols.begin(); it != it_end; ++it)
        {
            if (0 == wcscasecmp(it->name.c_str(), name.c_str()))
            {
                return &(it->val);
            }
        }
    }
    
    return BaseClass::getMember(name);
}






// -- DbDatabase class implementation --



// (CLASS) DbConnection
// Category: Database
// Description: A class that manages a database connection to a local or 
//     remote database.
// Remarks: The DbConnection class represents and manages a connection to
//     a database.  Users of this class can open database connections to
//     local or remote database and execute queries.  It is the root class
//     of the database class tree.

DbConnection::DbConnection()
{
    m_exceptions_enabled = false;
    m_error = NULL;
}

DbConnection::~DbConnection()
{
    if (m_error)
        m_error->baseUnref();
}

// provide a way for the C++ side to set the internal
// database pointer of the DbConnection object
void DbConnection::setDatabase(xd::IDatabasePtr db)
{
    m_db = db;
}

xd::IDatabasePtr DbConnection::getDatabase()
{
    return m_db;
}



DbError* DbConnection::getErrorObject(kscript::ExprEnv* env)
{
    if (!m_error)
    {
        m_error = DbError::createObject(env);
        m_error->baseRef();
    }
    
    return m_error;
}

void DbConnection::clearError()
{
    if (m_error)
    {
        m_error->code = 0;
        m_error->message = L"";
    }
}


// (CONSTRUCTOR) DbConnection.constructor
// Description: Constructor for DbConnection class.
// 
// Syntax: DbConnection(connection_string : String)
//
// Remarks: Constructs a DbConnection object and attempts to open the database
//     specified in the connection.  If the connection_string parameter is omitted
//     or empty, the call will connect to the host application's currently open database.
//
// Param(connection_string): The connection string describing the database to connect to.
//     Passing an empty string to this parameter will cause the call to attempt a connection
//     to the host application's currently open database

void DbConnection::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() >= 1)
    {
        std::wstring cstr = env->getParam(0)->getString();
        kl::trim(cstr);
        if (!cstr.empty())
        {
            xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
            m_db = dbmgr->open(cstr);
            
            if (!m_db)
            {
                DbError* error = getErrorObject(env);
                error->code = dbmgr->getErrorCode();
                error->message = dbmgr->getErrorString();
            }
        }
    }
}

// (METHOD) DbConnection.bulkInsert
// Description: Initiate a bulk insert operation
//
// Syntax: function DbConnection.bulkInsert(table_name : String,
//                                          field_list : String) : DbBulkInsert
//
// Remarks: Initiates a bulk insert operation on |table_name|.  In the
//     |field_list| parameter the caller can specify a list of fields in which
//     values will be inserted.  The motivation for bulk inserts is speed;
//     using bulkInsert() can yield a significant performance increase over
//     SQL INSERT statements run with the execute() method.
//
// Param(table_name): The name of the table to do the insert on
// Param(field_list): A comma-delimited list of fields in which values will be
//     inserted.  Passing "*" or omitting the parameter entirely will indicate
//     that all fields will be involved in the bulk insert operation
// Returns: A DbBulkInsert object ready for insertion.  Null is returned if
//     a problem was encountered

void DbConnection::bulkInsert(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_db.isNull() || env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    std::wstring table_name = env->getParam(0)->getString();
    std::wstring columns;
    
    if (env->getParamCount() >= 2)
        columns = env->getParam(1)->getString();
         else
        columns = L"*";
    
    
    DbBulkInsert* bi = DbBulkInsert::createObject(env);
    if (!bi->init(m_db,
                  table_name,
                  columns))
    {
        delete bi;
        retval->setNull();
        return;
    }
    
    retval->setObject(bi);
}


// (METHOD) DbConnection.connect
// Description: Connects to local and remote database via a connection string
// 
// Syntax: function DbConnection.connect(connection_string : String) : Boolean
//
// Remarks: Attempts to open the database specified in the connection.  If the
//     connection_string parameter is omitted or empty, the call will connect to
//     the host application's currently open database.
//
// Param(connection_string): The connection string describing the database to connect to.
//
// Returns: True if a successful connection was established to the specified database,
//     false if an error occurred

void DbConnection::connect(kscript::ExprEnv* env, kscript::Value* retval)
{
    clearError();
    m_db.clear();
    
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
     else
    {
        std::wstring cstr = env->getParam(0)->getString();
        kl::trim(cstr);
        if (cstr.empty())
        {
            retval->setBoolean(false);
            return;
        }
         else
        {
            xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
            if (dbmgr.isNull())
            {
                retval->setBoolean(false);
                return;
            }
            
            m_db = dbmgr->open(cstr);
            
            if (m_db.isNull())
            {
                if (m_exceptions_enabled)
                {
                    DbException* exception = DbException::createObject(env);
                    DbError* dberror = DbError::createObject(env);
                    dberror->code = dbmgr->getErrorCode();
                    dberror->message = dbmgr->getErrorString();
                    exception->addError(dberror);
                    kscript::Value v;
                    v.setObject(exception);
                    env->setRuntimeError(kscript::rterrorThrownException, &v);
                }
                 else
                {
                    DbError* error = getErrorObject(env);
                    error->code = dbmgr->getErrorCode();
                    error->message = dbmgr->getErrorString();
                }
            }
        }
    }
    
    retval->setBoolean(m_db.isOk());
}


// (METHOD) DbConnection.execute
// Description: Executes a SQL statement on the database
// 
// Syntax: function DbConnection.execute(sql_query : String) : DbResult
//
// Remarks: Executes a SQL statement on the database.  If the command was a query
//     and it succeeded, a DbResult object is returned which will provide iteration
//     functionality for the resulting data set.  For non-query commands, such as
//     INSERT or DROP, boolean |true| is returned upon success.  If the command or
//     query failed, |null| is returned.
//
// Param(sql_statement): The SQL statement to execute
// Returns: A valid DbResult object upon success, null if the command failed

void DbConnection::execute(kscript::ExprEnv* env, kscript::Value* retval)
{
    clearError();
    
    if (m_db.isNull() || env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    std::wstring sql = env->getParam(0)->getString();

    if (sql.empty())
    {
        retval->setNull();
        return;
    }
    
    xcm::IObjectPtr result_obj;

    if (!m_db->execute(sql, 0, result_obj, NULL))
    {
        if (m_exceptions_enabled)
        {
            DbException* exception = DbException::createObject(env);
            DbError* dberror = DbError::createObject(env);
            dberror->code = m_db->getErrorCode();
            dberror->message = m_db->getErrorString();
            exception->addError(dberror);
            kscript::Value v;
            v.setObject(exception);
            env->setRuntimeError(kscript::rterrorThrownException, &v);
        }
         else
        {
            DbError* error = getErrorObject(env);
            error->code = m_db->getErrorCode();
            error->message = m_db->getErrorString();
        }
    
    
        retval->setNull();
        return;
    }
    
    if (result_obj.isNull())
    {
        // command succeeded, but wasn't a command that returns
        // an object; return true
        retval->setBoolean(true);
        return;
    }
    
    xd::IIteratorPtr iter = result_obj;
    if (iter)
    {
        DbResult* d = DbResult::createObject(env);
        d->init(iter);
        retval->setObject(d);
    }
     else
    {
        // unknown return object type, but statement succeeded
        retval->setBoolean(true);
    }
}

// (METHOD) DbConnection.exists
// Description: Returns true if the file at the specified project |path| exists, 
//     and false otherwise.
// 
// Syntax: function DbConnection.exists(path : String) : Boolean
//
// Remarks: Returns true if the file at the specified project |path| exists, 
//     and false otherwise. 
//
// Param(path): The location of the project file to check for existence.
// Returns: Returns true if the file at the specified project |path| exists, 
//     and false otherwise.

void DbConnection::exists(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_db.isNull() || env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring object_name = env->getParam(0)->getString();
    retval->setBoolean(m_db->getFileExist(object_name));
}

// TODO: undocumented function right now; when this function becomes
// canonical, make sure to document it
void DbConnection::save(kscript::ExprEnv* env, kscript::Value* retval)
{
    // note: this function takes a result set and saves it to the specified
    // table; the first parameter is the result set to save and the second
    // parameter is the name of the table to which to save the result; the
    // optional third parameter is a DbMode flag; if the third parameter
    // isn't specified or is DbMode.Create, the function will create a new
    // table, overwriting what is there; if the third parameter is
    // DbMode.Append, the results will be appending to an existing set;
    // the returns true upon success; false upon failure
    
    // default return value
    retval->setBoolean(false);
    
    if (m_db.isNull() || env->getParamCount() < 2)
        return;

    if (!env->getParam(0)->isObject())
        return;
    
    if (!env->getParam(0)->getObject()->isKindOf(L"DbResult"))
        return;

    DbResult* input = (DbResult*)env->getParam(0)->getObject();
    xd::IIteratorPtr input_iter = input->getIterator();
    if (input_iter.isNull())
        return;

    // option third parameter to indicate append or overwrite
    bool append = false;
    if (env->getParamCount() >= 3)
    {
        int dbmode = env->getParam(2)->getInteger();
        append = (dbmode == DbInsertMode::Append) ? true : false;
    }

    // get the output name
    std::wstring output_name = env->getParam(1)->getString();

    // add the records to the output set, reset the
    // iterator, and we're done

    xd::CopyParams info;
    info.iter_input = input_iter;
    info.output = output_name;
    info.append = append;
    bool res = m_db->copyData(&info, NULL);

    retval->setBoolean(res);
}


// (METHOD) DbConnection.getObjects
// Description: Returns an array of DbObjectInfo objects which describe 
//     objects found in the database.
// 
// Syntax: function DbConnection.getObjects(condition1 : String, 
//                                          condition2 : String, 
//                                          ...) : Array(DbObjectInfo)
//
// Remarks: Returns an array of DbObjectInfo objects which describe
//     objects found in the database.  One or more conditions may be specified as
//     parameters to limit the search results.  Condition parameters have the
//     format KEY=VALUE, where KEY can be SCHEMA, CATALOG or TABLE.  If no
//     condition is specified, the input string is interpreted as a SCHEMA.
//     The DbObjectInfo objects returned have the following members: name,
//     type, catalog, schema, and mount.
//
// Param(condition): The condition of the search, such as the schema or folder to search
// Returns: Returns an array of DbObjectInfo objects

void DbConnection::getObjects(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_db.isNull())
    {
        retval->setNull();
        return;
    }
    
    
    // place parameters into filter keys catalog schema, and table
    std::wstring catalog, schema, table;
    
    size_t i, count = env->getParamCount();
    for (i = 0; i < count; ++i)
    {
        std::wstring param = env->getParam(i)->getString();
        
        if (param.find(L'=') != -1)
        {
            std::wstring key = kl::beforeFirst(param, L'=');
            std::wstring value = kl::afterFirst(param, L'=');
            
            kl::makeUpper(key);
            kl::trim(key);
            kl::trim(value);
            
            if (key == L"SCHEMA")
                schema = value;
            else if (key == L"CATALOG")
                catalog = value;
            else if (key == L"TABLE")
                table = value;
        }
         else
        {
            schema = param;
        }
    }



    // get the info from the database
    
    xd::IFileInfoEnumPtr fi_enum = m_db->getFolderInfo(schema);
    if (fi_enum.isNull())
    {
        retval->setNull();
        return;
    }
    
    
    
    
    retval->setArray(env);
    
    
    count = fi_enum->size();
    std::wstring oi_type;
    
    for (i = 0; i < count; ++i)
    {
        xd::IFileInfoPtr fi = fi_enum->getItem(i);
        
        
        switch (fi->getType())
        {
            case xd::filetypeNode:   oi_type = L"NODE"; break;
            case xd::filetypeFolder: oi_type = L"DIRECTORY"; break;
            case xd::filetypeTable:    oi_type = L"TABLE"; break;
            case xd::filetypeStream: oi_type = L"STREAM"; break;
            default:
                oi_type = L"UNKNOWN"; break;
        }    

        // note: if members are added to DbObjectInfo, or the way the
        // members work is changed, make sure to udpate the documentation 
        // next to the DbObjectInfo constructor
        DbObjectInfo* oi = DbObjectInfo::createObject(env);
        oi->getMember(L"name")->setString(fi->getName());
        oi->getMember(L"type")->setString(oi_type);
        oi->getMember(L"catalog")->setString(L"");
        oi->getMember(L"schema")->setString(L"");
        oi->getMember(L"mount")->setBoolean(fi->isMount());

        retval->getMemberI(i)->setObject(oi);
    }
}




// (METHOD) DbConnection.isConnected
// Description: Tests whether the connection object has an active
//     connection to a database
// 
// Syntax: function DbConnection.isConnected() : Boolean
//
// Remarks: Tests whether the connection object has an active connection
//     to a database.  This method is useful when specifying the connection
//     string in the object constructor.
//
// Returns: Returns true if a connection exists to a database, otherwise false.

void DbConnection::isConnected(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(m_db.isOk());
}


// (METHOD) DbConnection.getError
// Description: Returns an error object indicating the last database error
// 
// Syntax: function DbConnection.getError() : DbError
//
// Remarks: getLastError() returns a DbError object which describes the
//    last error condition the database encountered.  If no error was encountered,
//    a DbError object is still returned, but with a zero error code.
//
// Returns: Returns a DbError object

void DbConnection::getError(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setObject(getErrorObject(env));
}



// (METHOD) DbConnection.enableExceptions
// Description: Sets whether exceptions should be thrown on database errors
// 
// Syntax: function DbConnection.enableExceptions(enabled : Boolean)
//
// Remarks: This method allows the caller to configure whether or not database
//     exception objects are thrown when database errors occur.  By default,
//     database exceptions are not thrown and errors are returned.


void DbConnection::enableExceptions(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() == 0)
        m_exceptions_enabled = true;
         else
        m_exceptions_enabled = env->getParam(0)->getBoolean();
}



// (METHOD) DbConnection.describeTable
// Description: Returns an array of DbColumn objects which describe 
//     the structure of the specified table
// 
// Syntax: function DbConnection.describeTable(path : String) : Array(DbColumn)
//
// Remarks: Returns an array of DbColumn objects which describe 
//     the structure of the specified table.  Each DbColumn object
//     in the array has the following properties: name, type, width, scale,
//     and expression.
//
// Param(path): The location of the file from which the structure shall be retrieved.
// Returns: Returns an array of DbColumn objects.  If an error is encountered,
//     null is returned.

void DbConnection::describeTable(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_db.isNull() || env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    std::wstring table_name = env->getParam(0)->getString();
    
    xd::Structure structure = m_db->describeTable(table_name);
    
    if (structure.isNull())
    {
        retval->setNull();
        return;
    }
    
    
    retval->setArray(env);
    
    size_t i, col_count = structure.getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& col = structure.getColumnInfoByIdx(i);

        // TODO: if members are added, make sure to change
        // documentation next to class definition, as well
        // as the other instance where DbColumn is used
        DbColumn* dbcol = DbColumn::createObject(env);
        dbcol->getMember(L"name")->setString(col.name);
        dbcol->getMember(L"type")->setInteger(DbType::fromXdType(col.type));
        dbcol->getMember(L"width")->setInteger(col.width);
        dbcol->getMember(L"scale")->setInteger(col.scale);
        dbcol->getMember(L"expression")->setString(col.expression);
        
        retval->getMemberI((int)i)->setObject(dbcol);
    }
}







// (CLASS) DbException
// Category: Database
// Description: An class whose objects are thrown when database errors occur
// Remarks: When exceptions are enabled, a DbException object is thrown when
//     a database error is encountered.  To enable database exceptions, which
//     are off by default, please refer to the DbConnection.enableExceptions()
//     method.


DbException::DbException()
{
}

DbException::~DbException()
{
    std::vector<DbError*>::iterator it;
    for (it = m_errors.begin(); it != m_errors.end(); ++it)
        (*it)->baseUnref();
}

void DbException::addError(DbError* err)
{
    m_errors.push_back(err);
    err->baseRef();
}


void DbException::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) DbException.getError
// Description: Tests whether the connection object has an active
//     connection to a database
// 
// Syntax: function DbException.getError() : DbError
//
// Remarks: Returns the error object contained by the exception object.  Less
//     often an exception object may contain more than one error.  In this case,
//     the first error object is returned.  To retrieve all error objects
//     contained in the exception object, use the getErrors() method.
//
// Returns: Returns a DbError object


void DbException::getError(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_errors.size() == 0)
    {
        retval->setNull();
        return;
    }
    
    retval->setObject(m_errors[0]);
}


// (METHOD) DbException.getErrors
// Description: Tests whether the connection object has an active
//     connection to a database
// 
// Syntax: function DbException.getErrors() : Array(DbError)
//
// Remarks: Returns all error objects contained by the DbException object
//
// Returns: Returns an array of DbError object


void DbException::getErrors(kscript::ExprEnv* env, kscript::Value* retval)
{
    std::vector<DbError*>::iterator it;
    int i = 0;
      
    retval->setArray(env);
    
    for (it = m_errors.begin(); it != m_errors.end(); ++it)
        retval->getMemberI(i++)->setObject(*it);
}







// (CLASS) DbError
// Category: Database
// Description: A class that describes a database error condition
// Remarks: A DbError object describes a database error condition,
//     including the error code and message.


void DbError::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    code = 0;
}


// (METHOD) DbError.getCode
// Description: Returns the numeric code of the database error
// 
// Syntax: function DbError.getCode() : Number
//
// Remarks: Returns the numeric code which represents the database error
//
// Returns: A number indicating the database error which occurred

void DbError::getCode(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(code);
}



// (METHOD) DbError.getMessage
// Description: Returns the a string describing the database error
// 
// Syntax: function DbError.getMessage() : String
//
// Remarks: Returns the a string describing the database error.  This
//     string is implementation-dependant, meaning it can vary from
//     driver to driver.
//
// Returns: A string describing the database error


void DbError::getMessage(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(message);
}




}; // namespace scripthost
