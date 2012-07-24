/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-08-25
 *
 */


#ifndef __XDCOMMON_TANGO_H
#define __XDCOMMON_TANGO_H


#include <xcm/xcm.h>


namespace tango
{


// -- int64 support --
#ifdef _MSC_VER
typedef __int64 tango_int64_t;
typedef unsigned __int64 tango_uint64_t;
#else
typedef long long tango_int64_t;
typedef unsigned long long tango_uint64_t;
#endif


// -- tango type definitions --
typedef void* objhandle_t;
typedef unsigned int jobid_t;
typedef tango_uint64_t tableord_t;
typedef tango_uint64_t rowpos_t;
typedef tango_uint64_t rowid_t;
typedef tango_uint64_t datetime_t;


// -- tango data metric constants --
const int max_character_width = 65535;
const int max_numeric_width = 18;
const int max_numeric_scale = 12;


// -- forward declarations --
xcm_interface IAttributes;
xcm_interface IColumnInfo;
xcm_interface IConnectionStr;
xcm_interface IDatabase;
xcm_interface IDatabaseEntry;
xcm_interface IDatabaseMgr;
xcm_interface IDelimitedTextSet;
xcm_interface IDynamicSet;
xcm_interface IFileInfo;
xcm_interface IFixedLengthDefinition;
xcm_interface IIndexInfo;
xcm_interface IIterator;
xcm_interface IJob;
xcm_interface INodeValue;
xcm_interface IRelation;
xcm_interface IRowDeleter;
xcm_interface IRowInserter;
xcm_interface ISet;
xcm_interface ISetRowUpdate;
xcm_interface ICacheRowUpdate;
xcm_interface IStream;
xcm_interface IStructure;
xcm_interface IIteratorRelation;


// -- smart pointer declarations --
XCM_DECLARE_SMARTPTR(IAttributes)
XCM_DECLARE_SMARTPTR(INodeValue)
XCM_DECLARE_SMARTPTR(IFileInfo)
XCM_DECLARE_SMARTPTR(IColumnInfo)
XCM_DECLARE_SMARTPTR(IConnectionStr)
XCM_DECLARE_SMARTPTR(IDatabase)
XCM_DECLARE_SMARTPTR(IDatabaseEntry)
XCM_DECLARE_SMARTPTR(IDatabaseMgr)
XCM_DECLARE_SMARTPTR(IDelimitedTextSet)
XCM_DECLARE_SMARTPTR(IIndexInfo)
XCM_DECLARE_SMARTPTR(IJob)
XCM_DECLARE_SMARTPTR(IIterator)
XCM_DECLARE_SMARTPTR(IDynamicSet)
XCM_DECLARE_SMARTPTR(IStructure)
XCM_DECLARE_SMARTPTR(IRelation)
XCM_DECLARE_SMARTPTR(IRowInserter)
XCM_DECLARE_SMARTPTR(IRowDeleter)
XCM_DECLARE_SMARTPTR(ISet)
XCM_DECLARE_SMARTPTR(ISetRowUpdate)
XCM_DECLARE_SMARTPTR(ICacheRowUpdate)
XCM_DECLARE_SMARTPTR(IStream)
XCM_DECLARE_SMARTPTR(IFixedLengthDefinition)
XCM_DECLARE_SMARTPTR(IIteratorRelation)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IIndexInfoPtr>, IIndexInfoEnumPtr)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IRelationPtr>, IRelationEnumPtr)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IFileInfoPtr>, IFileInfoEnumPtr)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IDatabaseEntryPtr>, IDatabaseEntryEnumPtr)



// -- common connection string parameters --

// Provider    - database provider (xdnative, xdfs, xdodbc, etc.)
// Server      - name or IP of the database server
// Port        - port number
// Database    - either a database name on the remote server or
//               a file name of a local database (e.g. file.mdb) or
// User Id     - user name used for authentication
// Password    - password used for authentication




// -- database types --

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
    dbtypeHttp = 12,
    dbtypeFilesystem = 80
};



// -- format types --

enum
{
    formatNative = 0,          // database's native format (table)
    formatXbase = 1,           // xbase (table)
    formatDelimitedText = 2,   // delimited text file, interpreted as table
    formatFixedLengthText = 3, // fixed length text, interpreted as table
    formatText = 4,            // regular text file, as stream (not table)
};


// -- encoding types --

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



// -- column types --

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
    typeWideCharacter = 10
};


// -- job status --

enum
{
    jobStopped = 0,
    jobRunning = 1,
    jobFinished = 2,
    jobCancelled = 3,
    jobFailed = 4
};



// -- error types --

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
    
    errorObjectNotFound = -10,
    errorColumnNotFound = -11,
    errorIndexNotFound = -12,
    
    errorObjectAlreadyExists = -13,
    errorColumnAlreadyExists = -14,
    errorIndexAlreadyExists = -15,
};




// -- database attributes --

enum
{
    dbattrKeywords = 1000,
    dbattrTempDirectory = 1001,
    dbattrDefinitionDirectory = 1002,
    
    dbattrIdentifierQuoteOpenChar = 1900,
    dbattrIdentifierQuoteCloseChar = 1901,
    
    // -- columns --
    dbattrColumnMaxNameLength = 2000,
    dbattrColumnInvalidChars = 2001,
    dbattrColumnInvalidStartingChars = 2002,

    // -- tables --
    dbattrTableMaxNameLength = 3000,
    dbattrTableInvalidChars = 3001,
    dbattrTableInvalidStartingChars = 3002
};




// -- set flags --

enum
{
    sfFastRowCount = 0x01    // set if an ISet::getRowCount() operation can
                             //   be executed in O(1) time (fast)
};



// -- iterator flags --

enum
{
    ifFastSkip = 0x01,       // (read-only) set if IIterator::skip(n)
                             //  can be executed in O(1) time (fast)
    ifForwardOnly = 0x02,    // (read-only) set if IIterator::skip(n)
                             //  only supports forward-only skips,
    ifReverseRowCache = 0x04 // (read-write) set iterators which are
                             //  forward-only, but require skip(-value)
};




// -- file type flags --

enum
{
    filetypeNode =   0x00,
    filetypeFolder = 0x01,
    filetypeSet =    0x02,
    filetypeStream = 0x04
};



// -- execute() flags --

enum
{
    sqlPassThrough = 0x01,
    sqlAlwaysCopy = 0x02
};





// -- main tango interfaces --


xcm_interface INodeValue : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.INodeValue")

public:

    virtual const std::wstring& getName() = 0;
    virtual int getType() = 0;

    virtual const std::wstring& getString() = 0;
    virtual void setString(const std::wstring& value) = 0;
    virtual double getDouble() = 0;
    virtual void setDouble(double value) = 0;
    virtual int getInteger() = 0;
    virtual void setInteger(int value) = 0;
    virtual bool getBoolean() = 0;
    virtual void setBoolean(bool value) = 0;

    virtual unsigned int getChildCount() = 0;
    virtual const std::wstring& getChildName(unsigned int idx) = 0;
    virtual INodeValuePtr getChildByIdx(unsigned int idx) = 0;
    virtual INodeValuePtr getChild(const std::wstring& name,
                                  bool create_if_not_exist) = 0;
    virtual INodeValuePtr createChild(const std::wstring& name) = 0;
    virtual bool getChildExist(const std::wstring& name) = 0;
    virtual bool renameChild(const std::wstring& name,
                             const std::wstring& new_name) = 0;
    virtual bool deleteChild(const std::wstring& name) = 0;
    virtual bool deleteAllChildren() = 0;
};


xcm_interface IStream : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IStream")
    
public:

    virtual bool read(void* pv,
                      unsigned long read_size,
                      unsigned long* read_count) = 0;
                      
    virtual bool write(const void* pv,
                      unsigned long write_size,
                      unsigned long* written_count) = 0;
};


xcm_interface IFileInfo : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IFileInfo")

public:

    virtual const std::wstring& getName() = 0;       // filename
    virtual int getType() = 0;                       // one of the filetype enums
    virtual int getFormat() = 0;                     // one of the format enums
    virtual const std::wstring& getMimeType() = 0;   // mime type, empty if none
    virtual tango_int64_t getSize() = 0;             // size, in bytes
    virtual bool isMount() = 0;                      // true if file/folder is a mount
    virtual const std::wstring& getPrimaryKey() = 0; // primary key of a table
};



xcm_interface IColumnInfo : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IColumnInfo")

public:

    virtual void setName(const std::wstring& new_val) = 0;
    virtual std::wstring getName() = 0;

    virtual void setType(int new_val) = 0;
    virtual int getType() = 0;

    virtual void setWidth(int new_val) = 0;
    virtual int getWidth() = 0;

    virtual void setScale(int new_val) = 0;
    virtual int getScale() = 0;

    virtual void setNullsAllowed(bool new_val) = 0;
    virtual bool getNullsAllowed() = 0;

    virtual void setCalculated(bool new_val) = 0;
    virtual bool getCalculated() = 0;

    virtual void setExpression(const std::wstring& new_val) = 0;
    virtual std::wstring getExpression() = 0;

    virtual void setOffset(int new_val) = 0;
    virtual int getOffset() = 0;
    
    virtual void setEncoding(int new_val) = 0;
    virtual int getEncoding() = 0;

    virtual void setColumnOrdinal(int new_val) = 0;
    virtual int getColumnOrdinal() = 0;

    virtual void setTableOrdinal(int new_val) = 0;
    virtual int getTableOrdinal() = 0;

    virtual IColumnInfoPtr clone() = 0;
};


xcm_interface IStructure : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IStructure")

public:

    virtual IStructurePtr clone() = 0;
    virtual int getColumnCount() = 0;
    virtual std::wstring getColumnName(int idx) = 0;
    virtual IColumnInfoPtr getColumnInfoByIdx(int idx) = 0;
    virtual IColumnInfoPtr getColumnInfo(const std::wstring& column_name) = 0;
    virtual bool getColumnExist(const std::wstring& column_name) = 0;

    virtual bool deleteColumn(const std::wstring& column_name) = 0;
    virtual IColumnInfoPtr modifyColumn(const std::wstring& column_name) = 0;
    virtual IColumnInfoPtr createColumn() = 0;
    virtual IColumnInfoPtr insertColumn(int idx) = 0;

    virtual int getExprType(const std::wstring& expression) = 0;
    virtual bool isValid() = 0;
};


xcm_interface IIndexInfo : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IIndexInfo")

public:

    virtual const std::wstring& getTag() = 0;
    virtual const std::wstring& getExpression() = 0;
};


xcm_interface IRelation : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IRelation")

public:

    virtual std::wstring getRelationId() = 0;
    virtual const std::wstring& getTag() = 0;
    virtual std::wstring getLeftExpression() = 0;
    virtual std::wstring getRightExpression() = 0;
    virtual std::wstring getLeftSet() = 0;
    virtual std::wstring getRightSet() = 0;
    virtual ISetPtr getRightSetPtr() = 0;
};




// -- iterator interface --

xcm_interface IIterator : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IIterator")

public:

    virtual void setIteratorFlags(unsigned int mask, unsigned int value) = 0;
    virtual unsigned int getIteratorFlags() = 0;
    
    virtual ISetPtr getSet() = 0;
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
    virtual void goRow(const tango::rowid_t& rowid) = 0;

    virtual void refreshStructure() = 0;
    virtual IStructurePtr getStructure() = 0;
    virtual bool modifyStructure(IStructure* struct_config, IJob* job) = 0;

    virtual objhandle_t getHandle(const std::wstring& expr) = 0;
    virtual IColumnInfoPtr getInfo(objhandle_t data_handle) = 0;
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
    XCM_INTERFACE_NAME("tango.IRowInserter")

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


xcm_interface IRowDeleter : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IRowDeleter")

public:

    virtual void startDelete() = 0;
    virtual bool deleteRow(const rowid_t& rowid) = 0;
    virtual void finishDelete() = 0;
    virtual void cancelDelete() = 0;
};


xcm_interface ISet : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.ISet")

public:

    virtual void setObjectPath(const std::wstring& path) = 0;
    virtual std::wstring getObjectPath() = 0;
    virtual bool isTemporary() = 0;
    virtual bool storeObject(const std::wstring& path) = 0;

    virtual unsigned int getSetFlags() = 0;
    virtual std::wstring getSetId() = 0;

    virtual IStructurePtr getStructure() = 0;
    virtual bool modifyStructure(IStructure* struct_config, IJob* job) = 0;

    virtual IRowInserterPtr getRowInserter() = 0;
    virtual IRowDeleterPtr getRowDeleter() = 0;

    virtual int insert(IIteratorPtr source_iter,
                       const std::wstring& where_condition,
                       int max_rows,
                       IJob* job) = 0;

    virtual IIndexInfoPtr createIndex(const std::wstring& name,
                                      const std::wstring& expr,
                                      IJob* job) = 0;
    virtual IIndexInfoEnumPtr getIndexEnum() = 0;
    virtual bool deleteIndex(const std::wstring& name) = 0;
    virtual bool renameIndex(const std::wstring& name,
                             const std::wstring& new_name) = 0;
    virtual tango::IIndexInfoPtr lookupIndex(const std::wstring& expr,
                                             bool exact_column_order) = 0;


    virtual IRelationPtr createRelation(const std::wstring& tag,
                                        const std::wstring& right_set_path,
                                        const std::wstring& left_expr,
                                        const std::wstring& right_expr) = 0;
    virtual IRelationEnumPtr getRelationEnum() = 0;
    virtual IRelationPtr getRelation(const std::wstring& tag) = 0;
    virtual int getRelationCount() = 0;
    virtual bool deleteRelation(const std::wstring& tag) = 0;
    virtual bool deleteAllRelations() = 0;

    virtual IIteratorPtr createIterator(const std::wstring& columns,
                                        const std::wstring& expr,
                                        IJob* job) = 0;

    virtual rowpos_t getRowCount() = 0;
};



xcm_interface IDynamicSet : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IDynamicSet")

public:

    virtual ISetPtr getBaseSet() = 0;
    virtual void startBulkInsert() = 0;
    virtual void finishBulkInsert() = 0;
    virtual bool insertRow(const rowid_t& rowid) = 0;
    virtual bool deleteRow(const rowid_t& rowid) = 0;
};


xcm_interface IJob : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IJob")

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



xcm_interface IConnectionStr : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IConnectionStr")

public:

    virtual void parse(const std::wstring& str) = 0;
    virtual std::wstring getConnectionString() = 0;
    virtual std::wstring getLowerValue(const std::wstring& param) = 0;
    virtual std::wstring getValue(const std::wstring& param) = 0;
    virtual bool getValueExist(const std::wstring& param) = 0;
    virtual void setValue(const std::wstring& param, const std::wstring& value) = 0;
};



xcm_interface IAttributes : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IAttributes")

public:
    
    virtual void setBoolAttribute(int attr_id, bool value) = 0;
    virtual bool getBoolAttribute(int attr_id) = 0;
    
    virtual void setIntAttribute(int attr_id, int value) = 0;
    virtual int getIntAttribute(int attr_id) = 0;
    
    virtual void setStringAttribute(int attr_id, const std::wstring& value) = 0;
    virtual std::wstring getStringAttribute(int attr_id) = 0;
};


struct FormatInfo
{
    FormatInfo()
    {
        table_format = formatNative;
        default_encoding = encodingUndefined;
        first_row_column_names = true;
    };
    
    int table_format;
    int default_encoding;
    
    // extras for delimited files
    std::wstring text_qualifiers;
    std::wstring delimiters;
    std::wstring line_delimiters;
    bool first_row_column_names;
};


xcm_interface IDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IDatabase")

public:

    virtual void close() = 0;

    virtual void setDatabaseName(const std::wstring& name) = 0;
    virtual std::wstring getDatabaseName() = 0;
    virtual int getDatabaseType() = 0;
    virtual IAttributesPtr getAttributes() = 0;
    virtual std::wstring getActiveUid() = 0;
    virtual std::wstring getErrorString() = 0;
    virtual int getErrorCode() = 0;
    virtual void setError(int error_code, const std::wstring& error_string) = 0;
    
    virtual double getFreeSpace() = 0;
    virtual double getUsedSpace() = 0;
    virtual bool cleanup() = 0;
    
    virtual IJobPtr createJob() = 0;
    virtual IJobPtr getJob(jobid_t job_id) = 0;
    
    virtual IStructurePtr createStructure() = 0;

    virtual bool storeObject(xcm::IObject* obj, const std::wstring& path) = 0;

    virtual bool createFolder(const std::wstring& path) = 0;
    virtual INodeValuePtr createNodeFile(const std::wstring& path) = 0;
    virtual IStreamPtr createStream(const std::wstring& path, const std::wstring& mime_type) = 0;
    virtual ISetPtr createSet(const std::wstring& path, IStructurePtr struct_config, FormatInfo* format_info) = 0;
    
    virtual IStreamPtr openStream(const std::wstring& path) = 0;
    virtual INodeValuePtr openNodeFile(const std::wstring& path) = 0;
    virtual ISetPtr openSet(const std::wstring& path) = 0;
    virtual ISetPtr openSetEx(const std::wstring& path,
                              int format) = 0;

    virtual bool renameFile(const std::wstring& path, const std::wstring& new_name) = 0;
    virtual bool moveFile(const std::wstring& path, const std::wstring& new_folder) = 0;
    virtual bool copyFile(const std::wstring& src_path, const std::wstring& dest_path) = 0;
    virtual bool deleteFile(const std::wstring& path) = 0;
    virtual bool getFileExist(const std::wstring& path) = 0;
    virtual IFileInfoPtr getFileInfo(const std::wstring& path) = 0;
    virtual IFileInfoEnumPtr getFolderInfo(const std::wstring& path) = 0;
    
    virtual tango::IDatabasePtr getMountDatabase(const std::wstring& path) = 0;
    
    virtual bool setMountPoint(const std::wstring& path,
                               const std::wstring& connection_str,
                               const std::wstring& remote_path) = 0;
                                  
    virtual bool getMountPoint(const std::wstring& path,
                               std::wstring& connection_str,
                               std::wstring& remote_path) = 0;
                             
    virtual IRelationEnumPtr getRelationEnum() = 0;

    virtual bool execute(const std::wstring& command,
                         unsigned int flags,
                         xcm::IObjectPtr& result,
                         IJob* job) = 0;

    virtual ISetPtr runGroupQuery(ISetPtr set,
                                  const std::wstring& group,
                                  const std::wstring& output,
                                  const std::wstring& where_expr,
                                  const std::wstring& having,
                                  IJob* job) = 0;
};




xcm_interface IDatabaseEntry : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IDatabaseEntry")

public:

    virtual std::wstring getName() = 0;
    virtual std::wstring getDescription() = 0;
};


xcm_interface IDatabaseMgr : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IDatabaseMgr")

public:

    virtual IDatabasePtr open(const std::wstring& connection_str) = 0;
    
    virtual IDatabasePtr createDatabase(const std::wstring& location,
                                        const std::wstring& dbname) = 0;
                                        
    virtual bool createDatabase(const std::wstring& location, int db_type) = 0;

    virtual IDatabaseEntryEnumPtr getDatabaseList(
                                        const std::wstring& host,
                                        int port,
                                        const std::wstring& uid,
                                        const std::wstring& password) = 0;

    virtual std::wstring getErrorString() = 0;
    virtual int getErrorCode() = 0;
};




// -- specialized interfaces --


xcm_interface IDelimitedTextSet : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IDelimitedTextSet")

public:

    virtual bool saveConfiguration() = 0;
    virtual bool deleteConfiguration() = 0;
    
    virtual tango::IStructurePtr getSourceStructure() = 0;
    virtual tango::IStructurePtr getDestinationStructure() = 0;
    virtual tango::IStructurePtr getStructure() = 0;
    
    virtual bool modifySourceStructure(IStructure* struct_config, IJob* job) = 0;
    virtual bool modifyDestinationStructure(IStructure* struct_config, IJob* job) = 0;
    virtual bool modifyStructure(IStructure* struct_config, IJob* job) = 0;
    
    virtual bool renameSourceColumn(const std::wstring& source_col,
                                    const std::wstring& new_val) = 0;
    
    virtual void setDelimiters(const std::wstring& new_val,
                               bool refresh_structure) = 0;
    virtual std::wstring getDelimiters() = 0;
    
    virtual void setLineDelimiters(const std::wstring& new_val,
                                   bool refresh_structure) = 0;
    virtual std::wstring getLineDelimiters() = 0;
    
    virtual void setTextQualifier(const std::wstring& new_val,
                                  bool refresh_structure) = 0;
    virtual std::wstring getTextQualifier() = 0;
    
    virtual void setDiscoverFirstRowColumnNames(bool new_val) = 0;
    virtual void setFirstRowColumnNames(bool new_val) = 0;
    virtual bool isFirstRowColumnNames() = 0;
    
    virtual IIteratorPtr createSourceIterator(IJob* job) = 0;
    
    // check rows specifies the number of rows to check; -1 instructs
    // the function to check all rows
    virtual bool determineColumns(int check_rows, tango::IJob* job) = 0;
};


xcm_interface IFixedLengthDefinition : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IFixedLengthDefinition")

public:

    virtual bool saveConfiguration() = 0;
    virtual bool deleteConfiguration() = 0;

    virtual tango::IStructurePtr getSourceStructure() = 0;
    virtual tango::IStructurePtr getDestinationStructure() = 0;
    virtual tango::IStructurePtr getStructure() = 0;
    
    virtual bool modifySourceStructure(IStructure* struct_config, IJob* job) = 0;
    virtual bool modifyDestinationStructure(IStructure* struct_config, IJob* job) = 0;
    virtual bool modifyStructure(IStructure* struct_config, IJob* job) = 0;
        
    virtual void setBeginningSkipCharacterCount(size_t new_val) = 0;
    virtual size_t getBeginningSkipCharacterCount() = 0;
    
    virtual void setRowWidth(size_t new_val) = 0;
    virtual size_t getRowWidth() = 0;
    
    virtual void setLineDelimited(bool new_val) = 0;
    virtual bool isLineDelimited() = 0;
};



xcm_interface IIteratorRelation : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IteratorRelation")

public:
    
    virtual ISetPtr getChildSet(const std::wstring& relation_tag) = 0;
    virtual IIteratorPtr getChildIterator(const std::wstring& relation_tag) = 0;
};




// -- updating interfaces (optional) --

struct ColumnUpdateInfo
{
    tango::objhandle_t handle;
    std::string str_val;
    std::wstring wstr_val;
    double dbl_val;
    int int_val;
    tango::datetime_t date_val;
    bool bool_val;
    bool null;
};


xcm_interface ISetRowUpdate : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.ISetRowUpdate")

public:

    virtual bool updateRow(tango::rowid_t rowid,
                           tango::ColumnUpdateInfo* info,
                           size_t info_size) = 0;
};


xcm_interface ICacheRowUpdate : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.ICacheRowUpdate")

public:

    virtual bool updateCacheRow(tango::rowid_t rowid,
                                tango::ColumnUpdateInfo* info,
                                size_t info_size) = 0;
};




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
