/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-08-25
 *
 */


#ifndef H_XDCOMMON_XD_H
#define H_XDCOMMON_XD_H


#include <vector>
#include <map>

#include <kl/xcm.h>


namespace xd
{

// xd type definitions

typedef void* objhandle_t;
typedef unsigned int jobid_t;
typedef unsigned long long tableord_t;
typedef unsigned long long rowpos_t;
typedef unsigned long long rowid_t;
typedef unsigned long long datetime_t;


// data metric constants

const int min_character_width = 1;
const int max_character_width = 65535;

const int min_numeric_width = 1;
const int max_numeric_width = 18;

const int min_numeric_scale = 0;
const int max_numeric_scale = 12;


// forward declarations

xcm_interface IAttributes;
xcm_interface IDatabase;
xcm_interface IDatabaseMgr;
xcm_interface IFileInfo;
xcm_interface IIterator;
xcm_interface IJob;
xcm_interface IRelation;
xcm_interface IRowInserter;
xcm_interface ICacheRowUpdate;
xcm_interface IStream;

xcm_interface IIteratorRelation;
xcm_interface IRelationSchema;

struct FormatDefinition;

// smart pointer declarations

XCM_DECLARE_SMARTPTR(IAttributes)
XCM_DECLARE_SMARTPTR(IFileInfo)
XCM_DECLARE_SMARTPTR(IDatabase)
XCM_DECLARE_SMARTPTR(IDatabaseMgr)
XCM_DECLARE_SMARTPTR(IJob)
XCM_DECLARE_SMARTPTR(IIterator)
XCM_DECLARE_SMARTPTR(IRelation)
XCM_DECLARE_SMARTPTR(IRowInserter)
XCM_DECLARE_SMARTPTR(ICacheRowUpdate)
XCM_DECLARE_SMARTPTR(IStream)
XCM_DECLARE_SMARTPTR(IIteratorRelation)
XCM_DECLARE_SMARTPTR(IRelationSchema)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IRelationPtr>, IRelationEnumPtr)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IFileInfoPtr>, IFileInfoEnumPtr)

// common connection string parameters

// Provider    - database provider (xdnative, xdfs, xdodbc, etc.)
// Server      - name or IP of the database server
// Port        - port number
// Database    - either a database name on the remote server or
//               a file name of a local database (e.g. file.mdb) or
// User Id     - user name used for authentication
// Password    - password used for authentication




// database types

enum
{
    dbtypeUndefined = 0,
    dbtypeXdnative = 1,
    dbtypeOracle = 2,
    dbtypeSqlServer = 3,
    dbtypeMySql = 4,
    dbtypeOdbc = 5,
    dbtypeAccess = 6,
    dbtypeExcel = 7,
    dbtypeDb2 = 8,
    dbtypePostgres = 9,
    dbtypeFirebird = 10,
    dbtypeSqlite = 11,
    dbtypeClient = 12,
    dbtypeKpg = 13,

    dbtypeFilesystem = 80
};




// file type flags

enum
{
    filetypeNode =   0x00,
    filetypeFolder = 0x01,
    filetypeTable =  0x02,
    filetypeStream = 0x04,
    filetypeView =   0x08
};


// format types

enum
{
    formatDefault = 0,            // database's default table format
    formatXbase = 1,              // xbase table
    formatDelimitedText = 2,      // delimited text file, interpreted as table
    formatFixedLengthText = 3,    // fixed length text, interpreted as table
    formatText = 4,               // regular text file, as stream (not table)
    formatTypedDelimitedText = 5, // a delimited text file with type information (icsv)
    formatTTB = 6,                // TTB table format
    formatXLSX = 7                // XLSX spreadsheet format
};


// encoding types

// IMPORTANT NOTE: The following encoding type numbers are immutable.
//     This means that they never may be changed.  New types must
//     be appended to this list with a new number.

enum
{
    encodingInvalid = 0,
    encodingUndefined = 1,
    encodingASCII = 2,
    encodingUTF8 = 3,
    encodingUTF16 = 4,
    encodingUTF32 = 5,
    encodingUCS2 = 6,
    encodingUTF16BE = 7,
    encodingISO8859_1 = 80,
    encodingEBCDIC = 300,
    encodingCOMP = 301,
    encodingCOMP3 = 302
};



// column types

// IMPORTANT NOTE: The following column type numbers are immutable.
//     This means that they never may be changed.  New types must
//     be appended to this list with a new number.

enum
{
    typeInvalid = 0,
    typeUndefined = 1,
    typeCharacter = 2,
    typeBinary = 3,
    typeNumeric = 4,
    typeDouble = 5,
    typeInteger = 6,
    typeDate = 7,
    typeDateTime = 8,
    typeBoolean = 9,
    typeWideCharacter = 10,
    typeSerial = 80,
    typeBigSerial = 81
};


// job status

enum
{
    jobStopped = 0,
    jobRunning = 1,
    jobFinished = 2,
    jobCancelled = 3,
    jobFailed = 4
};



// error types

enum
{
    errorNone = 0,
    errorGeneral = -1,
    errorDisconnect = -2,
    errorTransmission = -3,
    errorNoPermission = -4,
    errorSyntax = -5,
    errorDivisionByZero = -6,
    errorMemory = -7,
    errorCancelled = -8,
    errorTimeoutExceeded = -9,
    errorDiskSpace = -10,
    errorNoDriver = -11,
    
    errorObjectNotFound = -100,
    errorColumnNotFound = -101,
    errorIndexNotFound = -102,
    
    errorObjectAlreadyExists = -103,
    errorColumnAlreadyExists = -104,
    errorIndexAlreadyExists = -105,
};




// database attributes

enum
{
    dbattrKeywords = 1000,
    dbattrTempDirectory = 1001,
    dbattrDefinitionDirectory = 1002,
    dbattrDatabaseUrl = 1003,
    dbattrDatabaseName = 1004,
    dbattrFilesystemPath = 1005,
    dbattrCookieFilePath = 1006,

    dbattrIdentifierQuoteOpenChar = 1900,
    dbattrIdentifierQuoteCloseChar = 1901,
    dbattrIdentifierCharsNeedingQuote = 1902,
    
    // columns
    dbattrColumnMaxNameLength = 2000,
    dbattrColumnInvalidChars = 2001,
    dbattrColumnInvalidStartingChars = 2002,

    // tables
    dbattrTableMaxNameLength = 3000,
    dbattrTableInvalidChars = 3001,
    dbattrTableInvalidStartingChars = 3002
};




// set flags

enum
{
    sfFastRowCount = 0x01    // set if a getRowCount() operation can
                             // be executed in O(1) time (fast)
};



// iterator flags

enum
{
    ifFastSkip = 0x01,        // (read-only) set if IIterator::skip(n)
                              //  can be executed in O(1) time (fast)
    ifForwardOnly = 0x02,     // (read-only) set if IIterator::skip(n)
                              //  only supports forward-only skips,
    ifReverseRowCache = 0x04, // (read-write) set iterators which are
                              //  forward-only, but require skip(-value),
    ifFastRowCount = 0x08,    // (read-only) set if IIterator::getRowCount()
                              //  can be executed in O(1) time (fast)
    ifTemporary = 0x10,       // (read-write) object should be cleaned up
                              //  after object destruction
};



// seek flags

enum
{
    seekCur = 0x01,
    seekEnd = 0x02,
    seekSet = 0x03
};



// flags for execute() and query()

enum
{
    sqlPassThrough = 0x01,   // pass the query through to the underlying database without modification
    sqlAlwaysCopy = 0x02,    // if set, a temporary result set table is created
    sqlBrowse = 0x04         // the iterator is made suitable for viewing and forwards/backwards scrolling         
};



// utility classes

struct xdcmpnocase : std::binary_function<const std::wstring&, const std::wstring&, bool>
{
    bool operator()(const std::wstring& lhs,  const std::wstring& rhs) const {
#ifdef _MSC_VER
        return (wcsicmp(lhs.c_str(), rhs.c_str()) < 0);
#else
        return (wcscasecmp(lhs.c_str(), rhs.c_str()) < 0);
#endif
    }
};

xcm_interface IXdUtil : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IXdUtil")

public:

    virtual std::wstring saveDefinitionToString(const xd::FormatDefinition& def) = 0;
    virtual bool loadDefinitionFromString(const std::wstring& str, xd::FormatDefinition* def) = 0;
};
XCM_DECLARE_SMARTPTR(IXdUtil)


namespace Util
{
    inline IXdUtilPtr getImpl() {
        static IXdUtilPtr res;
        if (res.isNull()) res.create_instance("xdfs.XdUtil");
        return res;
    }

    inline std::wstring saveDefinitionToString(const xd::FormatDefinition& def) { return getImpl()->saveDefinitionToString(def); }
    inline bool loadDefinitionFromString(const std::wstring& str, xd::FormatDefinition* def) { return getImpl()->loadDefinitionFromString(str, def); }
};








struct DatabaseEntry
{
    std::wstring name;
    std::wstring description;
};

typedef std::vector<DatabaseEntry> DatabaseEntryEnum;



struct ColumnInfo
{
    enum
    {
        maskName            = 0x0001,
        maskType            = 0x0002,
        maskWidth           = 0x0004,
        maskScale           = 0x0008,
        maskNullsAllowed    = 0x0010,
        maskCalculated      = 0x0020,
        maskExpression      = 0x0040,
        maskColumnOrdinal   = 0x0080,
        maskTableOrdinal    = 0x0100,
        maskSourceOffset    = 0x0200,
        maskSourceWidth     = 0x0400,
        maskSourceEncoding  = 0x0800
    };


    ColumnInfo()
    {
        type = typeInvalid;
        width = 0;
        scale = 0;
        nulls_allowed = true;
        calculated = false;
        column_ordinal = 0;
        table_ordinal = 0;

        source_offset = 0;
        source_width = 0;
        source_encoding = encodingUndefined;

        mask = 0;
    }

    bool isOk() const { return !name.empty(); }
    bool isNull() const { return name.empty(); }

    std::wstring name;
    int type;
    int width;
    int scale;
    bool nulls_allowed;
    bool calculated;
    std::wstring expression;
    int column_ordinal;
    int table_ordinal;

    int source_offset;
    int source_width;
    int source_encoding;

    unsigned int mask;
};



struct Structure
{
    static const size_t npos = (size_t)-1;

    bool isOk() const { return !columns.empty(); }
    bool isNull() const { return columns.empty(); }

    size_t getColumnCount() const { return columns.size(); }
    const std::wstring& getColumnName(size_t idx) const { return columns[idx].name; }
    const ColumnInfo& getColumnInfoByIdx(size_t idx) const { return columns[idx]; }
    const ColumnInfo& getColumnInfo(const std::wstring& column_name) const
      { static xd::ColumnInfo nullcol;
        size_t idx = getColumnIdx(column_name); 
        if (idx == (size_t)-1) return nullcol;
        return columns[idx];
      }
    bool getColumnExist(const std::wstring& column_name) const { return (getColumnIdx(column_name) != (size_t)-1); }
    size_t getColumnIdx(const std::wstring& name) const
      { if (m_map.empty()) { // populate cache
            int i = 0;
            for (std::vector<xd::ColumnInfo>::const_iterator it = columns.begin(), cend = columns.end(); it != cend; ++it)
                ((Structure*)this)->m_map[it->name] = i++;
        }
        std::map<std::wstring, int, xdcmpnocase>::const_iterator it = m_map.find(name);
        return (it == m_map.end() ? -1 : it->second);
      }

    void createColumn(const xd::ColumnInfo& col) { columns.push_back(col); m_map.clear(); }
    bool deleteColumn(const std::wstring& column_name)
      { size_t idx = getColumnIdx(column_name);
        if (idx == Structure::npos) return false;
        columns.erase(columns.begin() + idx);
        m_map.clear();
        return true;
      }
    void clear() { columns.clear(); m_map.clear(); }

    // stl compatibility
    void push_back(const xd::ColumnInfo& col) { columns.push_back(col); m_map.clear(); }
    void insert(std::vector<ColumnInfo>::iterator it, const xd::ColumnInfo& col) { columns.insert(it, col); m_map.clear(); }
    void erase(std::vector<ColumnInfo>::iterator it) { columns.erase(it); m_map.clear(); }
    size_t size() const { return columns.size(); }
    std::vector<ColumnInfo>::iterator begin() { return columns.begin(); }
    std::vector<ColumnInfo>::iterator end() { return columns.end(); }
    std::vector<ColumnInfo>::const_iterator cbegin() const { return columns.begin(); }
    std::vector<ColumnInfo>::const_iterator cend() const { return columns.end(); }
    xd::ColumnInfo& operator[](size_t idx) { return columns[idx]; }
    const xd::ColumnInfo& operator[](size_t idx) const { return columns[idx]; }
    Structure& operator=(const std::vector<ColumnInfo>& cols) { columns = cols; m_map.clear(); return *this; }

    std::vector<ColumnInfo> columns;
    std::map<std::wstring, int, xdcmpnocase> m_map;
};


struct FormatDefinition
{
    FormatDefinition()
    {
        format = formatDefault;
        object_type = filetypeTable;
        encoding = encodingUndefined;
        header_row = true;
        determine_structure = false;
        determine_delimiters = true;
        fixed_start_offset = 0;
        fixed_row_width = 0;
        fixed_line_delimited = false;
    }
    
    std::wstring object_id;               // unique object id
    int object_type;                      // one of the filetype* enum values (see above)
    int format;                           // one of the format* enum values (see above)
    int encoding;                         // one of the encoding* enum values (see above)

    std::wstring data_connection_string;  // optional connection string associated with data_file
    std::wstring data_path;               // project relative path, or file:/// url
    std::wstring base_table;              // associated concrete base table (used for cascading views)
    
    // delimited files parameters (when format = formatDelimitedText)
    std::wstring text_qualifier;
    std::wstring delimiter;
    std::wstring line_delimiter;
    bool header_row;
    bool determine_structure;
    bool determine_delimiters;            // if true, the values of text_qualifier and line_delimiter will be automatically determined, IF they are blank

    // fixed length parameters (when format = formatFixedLengthText)
    int fixed_start_offset;
    int fixed_row_width;
    bool fixed_line_delimited;

    // structure
    Structure columns;

    // helper functions
    void createColumn(const xd::ColumnInfo& params) { columns.push_back(params); }
    std::wstring toString() const { return Util::saveDefinitionToString(*this); }
    bool fromString(const std::wstring& str) { return Util::loadDefinitionFromString(str, this); }
};


struct StructureModify
{
    struct Action
    {
        enum
        {
            actionCreate = 0,
            actionModify = 1,
            actionDelete = 2,
            actionMove   = 3,
            actionInsert = 4
        };

        Action(int _action, const std::wstring& _column, const xd::ColumnInfo& _params)
            : action(_action), column(_column), params(_params) { }
        Action(int _action, const std::wstring& _column)
            : action(_action), column(_column) { }

        int action;
        std::wstring column;
        xd::ColumnInfo params;
    };

    std::vector<Action> actions;


    // helper functions

    void deleteColumn(const std::wstring& column_name)
        { actions.push_back(Action(Action::actionDelete, column_name)); }
    void moveColumn(const std::wstring& column_name, int new_idx)
        { xd::ColumnInfo c; c.mask = xd::ColumnInfo::maskColumnOrdinal; c.column_ordinal = new_idx;
          actions.push_back(Action(Action::actionMove, column_name, c)); }
    void modifyColumn(const std::wstring& column_name, const xd::ColumnInfo& params)
        { actions.push_back(Action(Action::actionModify, column_name, params)); }
    void createColumn(const xd::ColumnInfo& params)
        { actions.push_back(Action(Action::actionCreate, L"", params)); }
};


struct IndexInfo
{
    std::wstring name;
    std::wstring expression;

    bool isOk() const { return name.length() > 0 ? true : false; }
};

typedef std::vector<IndexInfo> IndexInfoEnum;





struct CopyParams
{
    CopyParams()
    {
        append = false;
        limit = -1;
    }

    IIteratorPtr iter_input;
    std::wstring input;
    std::wstring output;
    bool append;

    std::wstring where;
    std::wstring order;
    int limit;
    xd::FormatDefinition output_format;

    std::vector< std::pair<std::wstring, std::wstring> > copy_columns;

    void addCopyColumn(const std::wstring& dest_column, const std::wstring& src_column)
    { copy_columns.push_back(std::pair<std::wstring, std::wstring>(dest_column, src_column)); }
};

struct QueryParams
{
    QueryParams() { executeFlags = 0; }

    std::wstring from;
    std::wstring columns;
    std::wstring where;
    std::wstring order;
    int executeFlags;

    FormatDefinition format;

    IJobPtr job;
};

struct GroupQueryParams
{
    std::wstring input;
    std::wstring output;

    std::wstring group;
    std::wstring columns;
    std::wstring where;
    std::wstring having;
    std::wstring order;
};


xcm_interface IStream : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IStream")
    
public:

    virtual bool read(void* pv,
                      unsigned long read_size,
                      unsigned long* read_count) = 0;
                      
    virtual bool write(const void* pv,
                      unsigned long write_size,
                      unsigned long* written_count) = 0;

    virtual bool seek(long long seek_pos, int whence = seekSet) = 0;

    virtual long long getSize() = 0;
    virtual std::wstring getMimeType() = 0;
};


xcm_interface IFileInfo : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IFileInfo")

public:

    virtual const std::wstring& getName() = 0;                              // file/object name
    virtual int getType() = 0;                                              // file type enums (table, stream, etc)
    virtual int getFormat() = 0;                                            // file format enums (ttb, dbf, delimited, fixed length, etc)
    virtual unsigned int getFlags() = 0;                                    // object information enums
    virtual const std::wstring& getMimeType() = 0;                          // mime type, empty if none
    virtual long long getSize() = 0;                                        // size, in bytes (if applicable)
    virtual rowpos_t getRowCount() = 0;                                     // size, in rows (if applicable)
    virtual bool isMount() = 0;                                             // true if object is a mount
    virtual bool getMountInfo(std::wstring& cstr, std::wstring& rpath) = 0; // get mount information
    virtual const std::wstring& getPrimaryKey() = 0;                        // primary key of a table
    virtual const std::wstring& getObjectId() = 0;                          // object id (i.e. set id, oid, etc)
    virtual const std::wstring& getUrl() = 0;                               // url associated with object
};


xcm_interface IIterator : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IIterator")

public:

    virtual void setIteratorFlags(unsigned int mask, unsigned int value) = 0;
    virtual unsigned int getIteratorFlags() = 0;
    
    virtual void setTable(const std::wstring& table) = 0;
    virtual std::wstring getTable() = 0;

    virtual rowpos_t getRowCount() = 0;
    virtual IIteratorPtr clone() = 0;

    virtual void skip(int delta) = 0;
    virtual void goFirst() = 0;
    virtual void goLast() = 0;
    virtual rowid_t getRowId() = 0;
    virtual bool bof() = 0;
    virtual bool eof() = 0;
    virtual bool seek(const unsigned char* key, int length, bool soft) = 0;
    virtual bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft) = 0;
    virtual bool setPos(double pct) = 0;
    virtual double getPos() = 0;
    virtual void goRow(const xd::rowid_t& rowid) = 0;

    virtual bool refreshStructure() = 0;
    virtual Structure getStructure() = 0;
    virtual bool modifyStructure(const StructureModify& mod_params, IJob* job) = 0;
    virtual xd::FormatDefinition getFormatDefinition() { xd::FormatDefinition fd; fd.columns = this->getStructure().columns; return fd; } 

    virtual objhandle_t getHandle(const std::wstring& expr) = 0;
    virtual ColumnInfo getInfo(objhandle_t data_handle) = 0;
    virtual int getType(objhandle_t data_handle) = 0;
    virtual bool releaseHandle(objhandle_t data_handle) = 0;

    virtual const unsigned char* getRawPtr(objhandle_t data_handle) = 0;
    virtual int getRawWidth(objhandle_t data_handle) = 0;
    virtual const std::string& getString(objhandle_t data_handle) = 0;
    virtual const std::wstring& getWideString(objhandle_t data_handle) = 0;
    virtual datetime_t getDateTime(objhandle_t data_handle) = 0;
    virtual double getDouble(objhandle_t data_handle) = 0;
    virtual int getInteger(objhandle_t data_handle) = 0;
    virtual bool getBoolean(objhandle_t data_handle) = 0;
    virtual bool isNull(objhandle_t data_handle) = 0;
};


xcm_interface IRowInserter : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IRowInserter")

public:

    virtual objhandle_t getHandle(const std::wstring& column_name) = 0;

    virtual bool putRawPtr(objhandle_t column_handle, const unsigned char* value, int length) = 0;
    virtual bool putString(objhandle_t column_handle, const std::string& value) = 0;
    virtual bool putWideString(objhandle_t column_handle, const std::wstring& value) = 0;
    virtual bool putDouble(objhandle_t column_handle, double value) = 0;
    virtual bool putBoolean(objhandle_t column_handle, bool value) = 0;
    virtual bool putDateTime(objhandle_t column_handle, datetime_t value) = 0;
    virtual bool putInteger(objhandle_t column_handle, int value) = 0;
    virtual bool putNull(objhandle_t column_handle) = 0;

    virtual bool startInsert(const std::wstring& col_list) = 0;
    virtual bool insertRow() = 0;
    virtual void finishInsert() = 0;

    virtual bool flush() = 0;
};



xcm_interface IJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IJob")

public:

    virtual jobid_t getJobId() = 0;
    virtual bool getCanCancel() = 0;
    virtual bool getCancelled() = 0;
    virtual bool cancel() = 0;

    virtual time_t getStartTime() = 0;
    virtual time_t getFinishTime() = 0;

    virtual std::wstring getDescription() = 0;
    virtual std::wstring getProgressString() = 0;
    virtual int getStatus() = 0;

    virtual rowpos_t getCurrentCount() = 0;
    virtual rowpos_t getMaxCount() = 0;
    virtual double getPercentage() = 0;

    virtual int getPhaseCount() = 0;
    virtual int getCurrentPhase() = 0;
    
    virtual int getErrorCode() = 0;
    virtual std::wstring getErrorString() = 0;    
};



xcm_interface IAttributes : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IAttributes")

public:
    
    virtual void setBoolAttribute(int attr_id, bool value) = 0;
    virtual bool getBoolAttribute(int attr_id) = 0;
    
    virtual void setIntAttribute(int attr_id, int value) = 0;
    virtual int getIntAttribute(int attr_id) = 0;
    
    virtual void setStringAttribute(int attr_id, const std::wstring& value) = 0;
    virtual std::wstring getStringAttribute(int attr_id) = 0;
};



xcm_interface IDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IDatabase")

public:

    virtual int getDatabaseType() = 0;
    virtual IAttributesPtr getAttributes() = 0;
    virtual std::wstring getActiveUid() = 0;
    virtual std::wstring getErrorString() = 0;
    virtual int getErrorCode() = 0;
    virtual void setError(int error_code, const std::wstring& error_string) = 0;
    
    virtual bool cleanup() = 0;
    virtual void close() = 0;
    
    virtual IJobPtr createJob() = 0;

    virtual bool createFolder(const std::wstring& path) = 0;
    virtual bool createStream(const std::wstring& path, const std::wstring& mime_type) = 0;
    virtual bool createTable(const std::wstring& path, const FormatDefinition& format_info) = 0;

    virtual bool detectStreamFormat(const std::wstring& path, FormatDefinition* format_info, const FormatDefinition* defaults = NULL, IJob* job = NULL) { return false; }
    virtual bool loadDefinition(const std::wstring& path, FormatDefinition* format_info) { return false; }
    virtual bool saveDefinition(const std::wstring& path, const FormatDefinition& format_info) { return false; }

    virtual IDatabasePtr getMountDatabase(const std::wstring& path) = 0;
    virtual bool setMountPoint(const std::wstring& path, const std::wstring& connection_str,  const std::wstring& remote_path) = 0;
    virtual bool getMountPoint(const std::wstring& path, std::wstring& connection_str,  std::wstring& remote_path) = 0;
    
    virtual IStreamPtr openStream(const std::wstring& path) = 0;
    virtual bool renameFile(const std::wstring& path, const std::wstring& new_name) = 0;
    virtual bool moveFile(const std::wstring& path, const std::wstring& new_folder) = 0;
    virtual bool copyFile(const std::wstring& src_path, const std::wstring& dest_path) = 0;
    virtual bool copyData(const CopyParams* info, IJob* job) = 0;
    virtual bool deleteFile(const std::wstring& path) = 0;
    virtual bool getFileExist(const std::wstring& path) = 0;
    virtual IFileInfoPtr getFileInfo(const std::wstring& path) = 0;
    virtual IFileInfoEnumPtr getFolderInfo(const std::wstring& path) = 0;
    

    virtual xd::IndexInfo createIndex(const std::wstring& path,
                                      const std::wstring& name,
                                      const std::wstring& expr,
                                      IJob* job) = 0;
    virtual bool renameIndex(const std::wstring& path,
                             const std::wstring& name,
                             const std::wstring& new_name) = 0;
    virtual bool deleteIndex(const std::wstring& path,
                             const std::wstring& name) = 0;
    virtual IndexInfoEnum getIndexEnum(const std::wstring& path) = 0;

    virtual Structure describeTable(const std::wstring& path) = 0;

    virtual IRowInserterPtr bulkInsert(const std::wstring& path) = 0;

    virtual bool modifyStructure(const std::wstring& path, const StructureModify& mod_params, IJob* job) = 0;

    virtual IIteratorPtr query(const std::wstring& path,
                               const std::wstring& columns,
                               const std::wstring& wherec,
                               const std::wstring& order,
                               IJob* job)
    { QueryParams q; q.from = path; q.columns = columns; q.where = wherec; q.order = order; q.job = job; return query(q); }

    virtual IIteratorPtr query(const QueryParams& params) = 0;

    virtual bool execute(const std::wstring& command,
                         unsigned int flags,
                         xcm::IObjectPtr& result,
                         IJob* job) = 0;

    virtual ColumnInfo validateExpression(const std::wstring& expr, const xd::Structure& structure = xd::Structure(), const std::wstring& path_context = L"") { return ColumnInfo(); }
    virtual bool assignDefinition(const std::wstring& path, const xd::FormatDefinition& fd) { return false; }
    virtual bool groupQuery(GroupQueryParams* info, IJob* job) = 0;
};


xcm_interface IDatabaseMgr : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IDatabaseMgr")

public:

    virtual IDatabasePtr open(const std::wstring& connection_str) = 0;      
    virtual bool createDatabase(const std::wstring& connection_str) = 0;

    virtual DatabaseEntryEnum getDatabaseList(const std::wstring& host,
                                              int port,
                                              const std::wstring& uid,
                                              const std::wstring& password) = 0;

    virtual std::wstring getErrorString() = 0;
    virtual int getErrorCode() = 0;
};




// specialized interfaces


xcm_interface IRelation : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IRelation")

public:

    virtual const std::wstring& getRelationId() = 0;
    virtual const std::wstring& getTag() = 0;
    virtual std::wstring getLeftExpression() = 0;
    virtual std::wstring getRightExpression() = 0;
    virtual std::wstring getLeftTable() = 0;
    virtual std::wstring getRightTable() = 0;
};


xcm_interface IIteratorRelation : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IIteratorRelation")

public:
    
    virtual IIteratorPtr getChildIterator(IRelationPtr relation) = 0;
    virtual IIteratorPtr getFilteredChildIterator(IRelationPtr relation) = 0;
};


xcm_interface IRelationSchema : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IRelationSchema")

public:

    virtual IRelationPtr createRelation(const std::wstring& tag,
                                        const std::wstring& left_set_path,
                                        const std::wstring& right_set_path,
                                        const std::wstring& left_expr,
                                        const std::wstring& right_expr) = 0;
    virtual bool deleteRelation(const std::wstring& relation_id) = 0;
    virtual IRelationPtr getRelation(const std::wstring& relation_id) = 0;
    virtual IRelationEnumPtr getRelationEnum(const std::wstring& path) = 0;
};




// updating interfaces (optional)

struct ColumnUpdateInfo
{
    objhandle_t handle;
    std::string str_val;
    std::wstring wstr_val;
    double dbl_val;
    int int_val;
    datetime_t date_val;
    bool bool_val;
    bool null;
};


xcm_interface ICacheRowUpdate : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.ICacheRowUpdate")

public:

    virtual bool updateCacheRow(rowid_t rowid,
                                ColumnUpdateInfo* info,
                                size_t info_size) = 0;
};


}; // namespace xd



#include <xd/util.h>

#endif
