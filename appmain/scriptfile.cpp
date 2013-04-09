/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2006-12-24
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "scripthost.h"
#include "scriptfile.h"
#include "scriptmemory.h"
#include "../kscript/jsdate.h"

#include <wx/stdpaths.h>
#include <kl/portable.h>
#include <kl/utf8.h>
#include <limits>
#include <wx/dir.h>

// (CLASS) Directory
// Category: IO
// Description: A class that provides information about directories.
// Remarks: The Directory class provides information about directories, including
//     information about what files and subdirectories are contained in a directory.

Directory::Directory()
{
}

Directory::~Directory()
{
}

void Directory::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) Directory.exists
//
// Description: Determines if a directory exists
//
// Syntax: static function Directory.exists(path : String) : Boolean
//
// Remarks: Calling this method determines whether the path specified in
//     the |path| parameter exists and is a directory.
//
// Param(path): The path to check
// Returns: True if the specified path exists and is a directory, false otherwise

void Directory::exists(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
    }
     else
    {
        std::wstring path = env->getParam(0)->getString();
        retval->setBoolean(wxDir::Exists(path));
    }
}

// (METHOD) Directory.createDirectory
//
// Description: Creates a directory
//
// Syntax: static function Directory.createDirectory(path : String) : Boolean
//
// Remarks: Calling this method creates the directory specified
//     in the |path| parameter
//
// Param(path): The directory to create
// Returns: True if the directory was successfully created, false otherwise

void Directory::createDirectory(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
    }
     else
    {
        wxLogNull log;
        std::wstring path = env->getParam(0)->getString();
        retval->setBoolean(::wxMkdir(path));
    }
}


// (METHOD) Directory.delete
//
// Description: Deletes a directory
//
// Syntax: static function Directory.delete(path : String, recursive : Boolean) : Boolean
//
// Remarks: Calling this method deletes the directory specified
//     in the |path| parameter
//
// Param(path): The directory to delete
// Param(recursive): True if the call should recursively delete sub-directories and
//     files contained in those directories.  If this parameter is not specified,
//     |false| is the default.
// Returns: True if the directory was successfully deleted, false otherwise

void Directory::zdelete(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }

    bool recursive = false;
    if (env->getParamCount() >= 2)
        recursive = env->getParam(1)->getBoolean();
    
    if (recursive)
    {
        std::wstring path = env->getParam(0)->getString();
        xf_rmtree(path);
        retval->setBoolean(xf_get_directory_exist(path) ? false : true);
    }
     else
    {
        std::wstring path = env->getParam(0)->getString();
        retval->setBoolean(::wxRmdir(path));
    }
    
}


// (METHOD) Directory.getAll
//
// Description: Returns all files and subdirectories found in a specified directory.
//
// Syntax: static function Directory.getAll(path : String) : Array(String)
//
// Remarks: Calling this method returns all files and subdirectories found in
//     the directory specified by |path|.
//
// Param(path): The path to read.
// Returns: An array containing string values which represent the files and directories
//     contained in the directory specified in the |path| parameter

void Directory::getAll(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
    
    retval->setArray(env);

    std::wstring dir_path = env->getParam(0)->getString();
    if (!xf_get_directory_exist(dir_path))
        return;

    wxDir dir(dir_path);
    wxString filename;
    size_t i = 0;
    
    bool ok = dir.GetFirst(&filename);
    while (ok)
    {
        kscript::Value val;
        val.setString(towstr(filename));
        retval->appendMember(&val);
        
        ok = dir.GetNext(&filename);
    }
}


// (METHOD) Directory.getFiles
//
// Description: Returns all files found in a specified directory.
//
// Syntax: static function Directory.getFiles(path : String) : Array(String)
//
// Remarks: Calling this method returns all files found in the directory
//     specified by |path|.  Directories and other special files are not included
//     in the resulting array.
//
// Param(path): The path to read.
// Returns: An array containing string values which represent the files
//     contained in the directory specified in the |path| parameter

void Directory::getFiles(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    retval->setArray(env);
    
    std::wstring dir_path = env->getParam(0)->getString();
    if (!xf_get_directory_exist(dir_path))
        return;

    wxDir d(dir_path);
    wxString filename;
    size_t i = 0;
    
    bool ok = d.GetFirst(&filename, wxEmptyString, wxDIR_FILES | wxDIR_HIDDEN);
    while (ok)
    {
        kscript::Value val;
        val.setString(towstr(filename));
        retval->appendMember(&val);

        ok = d.GetNext(&filename);
    }
}



// (METHOD) Directory.getDirectories
//
// Description: Returns all the subdirectories found in a specified directory.
//
// Syntax: static function Directory.getDirectories(path : String) : Array(String)
//
// Remarks: Calling this method returns all subdirectories found in the directory
//     specified by |path|.
//
// Param(path): The path to read.
// Returns: An array containing string values which represent the subdirectories
//     contained in the directory specified in the |path| parameter


void Directory::getDirectories(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
    
    retval->setArray(env);
    
    std::wstring dir_path = env->getParam(0)->getString();
    if (!xf_get_directory_exist(dir_path))
        return;

    wxDir d(dir_path);
    wxString filename;
    size_t i = 0;
    
    bool ok = d.GetFirst(&filename, wxEmptyString, wxDIR_DIRS | wxDIR_HIDDEN);
    while (ok)
    {
        kscript::Value val;
        val.setString(towstr(filename));
        retval->appendMember(&val);

        ok = d.GetNext(&filename);
    }
}




// (CLASS) FileInfo
// Category: IO
// Description: A class that provides information about a file.
// Remarks: The FileInfo class provides information about a file, including data about
//     the length of the file and the last modified date/time of the file.

zFileInfo::zFileInfo()
{
}

zFileInfo::~zFileInfo()
{
}


// (CONSTRUCTOR) FileInfo.constructor
// Description: Creates a new FileInfo object
//
// Syntax: FileInfo(filename : String)
//
// Remarks: Creates a new Log object.  The filename should be specified in the
//     constructor parameter.
// 
// Param(filename): The filename of the file.


void zFileInfo::staticConstructor(kscript::ExprEnv* env,
                                 void* param,
                                 kscript::Value* retval)
{
    zFileInfo* f = zFileInfo::createObject(env);
    f->constructor(env, retval);
    retval->setObject(f);
}

void zFileInfo::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        m_filename = env->getParam(0)->getString();
    }
}


// (METHOD) FileInfo.getLength
//
// Description: Returns the length of the file.
//
// Syntax: function FileInfo.getLength() : Number
//
// Remarks: Calling this method returns the length of the file that was specified
//     in the FileInfo constructor when the object was created.
//
// Returns: The length of the file.


void zFileInfo::getLength(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setDouble(0.0);
    
    if (m_filename.length() > 0)
    {
        retval->setDouble((double)xf_get_file_size(m_filename));
    }
}


// (METHOD) FileInfo.getLastWriteTime
//
// Description: Returns the last write time of the file.
//
// Syntax: function FileInfo.getLastWriteTime() : Date
//
// Remarks: Calling this method returns the last write time of the file that
//      was specified in the FileInfo constructor when the object was created.
//
// Returns: The 'last write time' of the file.  Null is returned if an error
//      was encountered.


void zFileInfo::getLastWriteTime(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (m_filename.length() > 0)
    {
        kscript::Date* d = kscript::Date::createObject(env);
        
        xf_timestruct_t ts;
        xf_filetime_t ft = xf_get_file_modify_time(m_filename);
        if (!ft)
            return;
        xf_filetime_to_systemtime(ft, &ts);
        
        d->setDateTime(ts.year,
                       ts.month + 1,
                       ts.day,
                       ts.hour,
                       ts.minute,
                       ts.second,
                       ts.milliseconds);
        
        retval->setObject(d);
    }
}


// -- FileStream class implementation --

// (CLASS) FileStream
// Category: IO
// Description: A class that provides functionality to access binary files.
// Remarks: The FileStream class represents a file access stream for reading and 
//     writing binary values to a file.

FileStream::FileStream()
{
    m_f = 0;
}

FileStream::~FileStream()
{
    if (m_f)
        xf_close(m_f);
}




void FileStream::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) FileStream.close
//
// Description: Closes a file stream
//
// Syntax: function FileStream.close() : Boolean
//
// Remarks: Calling the close() method will close an open file stream
//     and free resources used during the file input/output operations
//
// Returns: True if the file stream was successfully closed, or false
//     if an error was encountered.  If the file stream was not open
//     when close() was called, false is returned.

void FileStream::close(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_f)
    {
        retval->setBoolean(false);
        return;
    }
    
    xf_close(m_f);
    m_f = 0;
    
    retval->setBoolean(true);
}

// (METHOD) FileStream.read
//
// Description: Reads data from a file.
//
// Syntax: function FileStream.read(buffer : MemoryBuffer,
//                                  offset : Integer,
//                                  num_bytes : Integer) : Integer
//
// Remarks: Reads the number of bytes specified by the parameter
//     |num_bytes| into the buffer specified by the |buffer| parameter.
//     The bytes will be placed in the buffer at the offset specified in
//     the |offset| parameter.  The array needs to be large enough to hold
//     the requested number of bytes, otherwise the call will fail.
//
// Param(buffer): Target memory buffer for the read operation.
// Param(offset): Offset inside the buffer at which to place the data.
// Param(num_bytes): Number of bytes to read.
// Returns: The total number of bytes read, or zero if an
//     end of file condition was encountered, or if an error occurred.

void FileStream::read(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 3)
    {
        retval->setInteger(0);
        return;
    }
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    int offset = env->getParam(1)->getInteger();
    int bytes_to_read = env->getParam(2)->getInteger();
    
    
    if (obj->getClassName() != L"MemoryBuffer" || offset < 0 || bytes_to_read < 0)
    {
        retval->setInteger(0);
        return;
    }
    
    MemoryBuffer* membuf = (MemoryBuffer*)obj;
    
    if ((size_t)offset + (size_t)bytes_to_read > membuf->getBufferSize())
    {
        retval->setInteger(0);
        return;
    }
    
    int read_bytes = xf_read(m_f, membuf->getBuffer() + offset, 1, bytes_to_read);
    
    retval->setInteger(read_bytes);
}


// (METHOD) FileStream.write
//
// Description: Writes data to a file.
//
// Syntax: function FileStream.write(buffer : MemoryBuffer,
//                                   offset : Integer,
//                                   num_bytes : Integer) : Integer
//
// Remarks: Writes the number of bytes specified by the parameter |num_bytes|
//     from the buffer specified by the |buffer| parameter.  The bytes
//     from the buffer will be written from the offset specified in the
//     |offset| parameter.
//
// Param(buffer): Source memory buffer for the write operation.
// Param(offset): Offset inside the buffer at which to write from.
// Param(num_bytes): Number of bytes to write.
// Returns: The total number of bytes written by the operation
//     or zero if an error occurred.

void FileStream::write(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 3)
    {
        retval->setInteger(0);
        return;
    }
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    int offset = env->getParam(1)->getInteger();
    int bytes_to_write = env->getParam(2)->getInteger();
    
    
    if (obj->getClassName() != L"MemoryBuffer" || offset < 0 || bytes_to_write < 0)
    {
        retval->setInteger(0);
        return;
    }
    
    MemoryBuffer* membuf = (MemoryBuffer*)obj;
    
    if ((size_t)offset + (size_t)bytes_to_write > membuf->getBufferSize())
    {
        retval->setInteger(0);
        return;
    }
    
    int written_bytes = xf_write(m_f, membuf->getBuffer() + offset, 1, bytes_to_write);
    
    retval->setInteger(written_bytes);
}

// (METHOD) FileStream.seek
//
// Description: Seeks to a location in a file.
//
// Syntax: function FileStream.seek(offset : Integer,
//                                  origin : SeekOrigin) : Boolean
//
// Remarks: Seeks to the relative position specified by |offset|.
//     The starting point for the seek is specified by the second parameter.
//     The value of |origin| can be SeekOrigin.Begin, SeekOrigin.Current, or
//     SeekOrigin.End.
//
// Param(offset): Offset to seek to, relative to the origin parameter.
// Param(origin): Place to start seek from, one of the values from
//     the SeekOrigin enumeration.
// Returns: Returns true upon success, and false otherwise.

void FileStream::seek(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;
    
    xf_off_t seek_pos;
    double d = env->getParam(0)->getDouble();
    
    if (d >= -2147483647.0 || d <= 2147483647.0)
        seek_pos = env->getParam(0)->getInteger();
         else
        seek_pos = (xf_off_t)d;
    
    int seek_origin = SeekOrigin::Begin;
    
    if (env->getParamCount() >= 2)
        seek_origin = env->getParam(1)->getInteger();
    
    int xf_seek_origin;
    
    switch (seek_origin)
    {
        case SeekOrigin::Current: xf_seek_origin = xfSeekCur; break;
        case SeekOrigin::Begin:   xf_seek_origin = xfSeekSet; break;
        case SeekOrigin::End:     xf_seek_origin = xfSeekEnd; break;
        default:
            // bad seek origin value
            retval->setBoolean(false);
            return;
    }
    
    bool success = xf_seek(m_f, seek_pos, xf_seek_origin);
    retval->setBoolean(success);
}


// -- File class implementation --

// (CLASS) File
// Category: IO
// Description: A class that exposes functions which allow access to files.
// Remarks: The File class exposes functions which allow access to
//     binary and text files.
//
// Property(FileMode.Create):       A flag specifying that a file should explicitly be created.
// Property(FileMode.Open):         A flag specifying that a file should explicitly be opened.
// Property(FileMode.OpenOrCreate): A flag specifying that a file should be opened if it already exists or created if it doesn't exist.
// Property(FileMode.Append):       A flag specifying that a file should be opened and appended to.
// Property(FileAccess.Read):       A flag specifying that a file should be opened only with read rights.
// Property(FileAccess.Write):      A flag specifying that a file should be opened only with write rights.
// Property(FileAccess.ReadWrite):  A flag specifying that a file should be opened with both read and write rights.
// Property(FileShare.None):        A flag specifying that a file can only be accessed by one process at a time.
// Property(FileShare.Read):        A flag specifying that a file can be read by multiple processes at the same time.
// Property(FileShare.Write):       A flag specifying that a file can be written to by multiple processes at the same time.
// Property(FileShare.ReadWrite):   A flag specifying that a file can be read from and written to by multiple processes at the same time.
// Property(SeekOrigin.Begin):      A flag specifying the seek location at the beginning of the file.
// Property(SeekOrigin.Current):    A flag specifying the seel location at the current location in the file.
// Property(SeekOrigin.End):        A flag specifying the seek location at the end of the file.

// NOTE: The above properties are actually bound constants
//       of their own respective classes above

File::File()
{
}

File::~File()
{
}

void File::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) File.open
//
// Description: Opens a file for reading or writing.
//
// Syntax: static function File.open(filename : String,
//                                   file_mode : FileMode,
//                                   file_access : FileAccess,
//                                   file_share : FileShare) : FileStream
//
// Remarks: Opens the file specified by |filename|, applying the
//     functionality requested by the |file_mode|, |file_access|
//     and |file_share| parameters.
//
// Param(filename): The file to open.
// Param(file_mode): Specifies the behavior of the open call.  Specifies
//     whether the file should be created, opened, or opened and then
//     created if it doesn't already exist.  One of  FileMode.Create,
//     FileMode.Open or FileMode.OpenOrCreate.
// Param(file_access): Specifies the desired file access (read, write).
//     One of FileAccess.Append, FileAccess.Read, FileAccess.Write,
//     or FileAccess.ReadWrite.
// Param(file_share): Specifies the file sharing flags.
//     One of FileShare.None, FileShare.Read,
//     FileShare.Write or FileShare.ReadWrite.
// Returns: Returns a FileStream object, or null if an error was encountered.


void File::open(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    std::wstring filename = env->getParam(0)->getString();
    int file_mode = FileMode::Open;
    int file_access = FileAccess::ReadWrite;
    int file_share = FileShare::None;
    
    if (env->getParamCount() >= 2)
    {
        file_mode = env->getParam(1)->getInteger();
    }
    
    if (env->getParamCount() >= 3)
    {
        file_access = env->getParam(2)->getInteger();
    }
    
    if (env->getParamCount() >= 4)
    {
        file_share = env->getParam(3)->getInteger();
    }
    
    if (filename.empty())
    {
        retval->setNull();
        return;
    }


    int xf_file_mode = 0;
    switch (file_mode)
    {
        case FileMode::Open:         xf_file_mode = xfOpen; break;
        case FileMode::Create:       xf_file_mode = xfCreate; break;
        case FileMode::OpenOrCreate: xf_file_mode = xfOpenCreateIfNotExist; break;
        case FileMode::Append:       xf_file_mode = xfOpen; break;
        default:
            // unknown mode
            retval->setNull();
            return;
    }
    
    int xf_file_access = 0;
    switch (file_access)
    {
        case FileAccess::Read:      xf_file_access = xfRead; break;
        case FileAccess::Write:     xf_file_access = xfWrite; break;
        case FileAccess::ReadWrite: xf_file_access = xfReadWrite; break;
    }
  
    int xf_file_share = 0;
    switch (file_share)
    {
        case FileShare::None:      xf_file_share = xfShareNone; break;
        case FileShare::Read:      xf_file_share = xfShareRead; break;
        case FileShare::Write:     xf_file_share = xfShareWrite; break;
        case FileShare::ReadWrite: xf_file_share = xfShareReadWrite; break;
    }


    if (file_mode == FileMode::Append &&
        file_access != FileAccess::Write &&
        file_access != FileAccess::ReadWrite)
    {
        // append mode requires write access
        retval->setNull();
        return;
    }
    

    xf_file_t f = xf_open(filename,
                          xf_file_mode,
                          xf_file_access,
                          xf_file_share);
    
    if (!f)
    {
        retval->setNull();
        return;
    }
    

    FileStream* s = FileStream::createObject(env);
    s->m_f = f;
    
    retval->setObject(s);
}


// (METHOD) File.openText
//
// Description: Opens a text file for reading.
//
// Syntax: static function File.openText(filename : String) : TextReader
//
// Remarks: Opens the text file specified by |filename| and
//     returns a TextReader object.
//
// Param(filename): The file to open.
// Returns: Returns a TextReader object, or null if an error occurred.

void File::openText(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_param_count < 1)
    {
        retval->setNull();
        return;
    }
    
    std::wstring filename = env->m_eval_params[0]->getString();
    FILE* f;

    if (filename.empty())
    {
        retval->setNull();
        return;
    }
#ifdef WIN32
    f = _wfopen(filename.c_str(), L"rt, ccs=UNICODE");
#else
    std::string asc_filename = kl::tostring(filename);
    f = fopen(asc_filename.c_str(), "rt");
#endif

    if (!f)
    {
        retval->setNull();
        return;
    }

    kscript::Value val;
    env->getParser()->createObject(L"TextReader", &val);
    TextReader* tr = (TextReader*)val.getObject();
    tr->m_f = f;
    retval->setValue(val);
}


// (METHOD) File.createText
//
// Description: Creates a text file
//
// Syntax: static function File.createText(filename : String) : TextWriter
//
// Remarks: Creates a text file using the filename specified in
//     the |filename| parameter.  If the file already exists,
//     it will be overwritten. 
//
// Param(filename): The file to create.
// Returns: Returns the method returns a TextWriter object, or null
//     if a problem was encountered.

void File::createText(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_param_count < 1 ||
        !env->m_eval_params[0]->isString())
    {
        retval->setNull();
        return;
    }
    
    std::wstring filename = env->m_eval_params[0]->getString();
    FILE* f;

    if (filename.empty())
    {
        retval->setNull();
        return;
    }

#ifdef WIN32
    f = _wfopen(filename.c_str(), L"wt");
#else
    std::string asc_filename = kl::tostring(filename);
    f = fopen(asc_filename.c_str(), "wt");
#endif

    if (!f)
    {
        retval->setNull();
        return;
    }

    kscript::Value val;
    env->getParser()->createObject(L"TextWriter", &val);
    TextWriter* tw = (TextWriter*)val.getObject();
    tw->m_f = f;
    retval->setValue(val);
}


// (METHOD) File.appendText
//
// Description: Opens a text file for appending.
//
// Syntax: static function File.appendText(filename : String) : TextWriter
//
// Remarks: Opens a file specified by the |filename| parameter for appending.
//   If the file does not already exist, it is created.
//
// Param(filename): The file to append to.
// Returns: Returns a TextWriter object, or null if a problem occurred.

void File::appendText(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_param_count < 1 ||
        !env->m_eval_params[0]->isString())
    {
        retval->setNull();
        return;
    }
    
    std::wstring filename = env->m_eval_params[0]->getString();
    FILE* f;

    if (filename.empty())
    {
        retval->setNull();
        return;
    }

#ifdef WIN32
    f = _wfopen(filename.c_str(), L"at");
#else
    std::string asc_filename = kl::tostring(filename);
    f = fopen(asc_filename.c_str(), "at");
#endif

    if (!f)
    {
        retval->setNull();
        return;
    }

    kscript::Value val;
    env->getParser()->createObject(L"TextWriter", &val);
    TextWriter* tw = (TextWriter*)val.getObject();
    tw->m_f = f;
    retval->setValue(val);
}

// (METHOD) File.copy
//
// Description: Copies a file.
//
// Syntax: static function File.copy(source : String,
//                                   destination) : Boolean
//
// Remarks: Copies the |source| file to a new |destination| file.  Returns
//     true if the new file was created and false otherwise.
//
// Param(source): The file to copy.
// Param(destination): The name of the newly copied file.
//
// Returns: Returns true upon success, and false otherwise.

void File::copy(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    // check the parameter count
    retval->setBoolean(false);
    if (env->m_param_count < 2)
        return;

    // make sure parameters are populated
    std::wstring source_file_name = env->getParam(0)->getString();
    std::wstring dest_file_name = env->getParam(1)->getString();
    if (source_file_name.empty() || dest_file_name.empty())
        return;

    // open the input file
    xf_file_t source_file = xf_open(source_file_name,
                                    xfOpen,
                                    xfRead,
                                    xfShareNone);

    // if we can't open the source file, we're done
    if (!source_file)
        return;

    // open the the output file
    xf_file_t dest_file = xf_open(dest_file_name,
                                  xfOpenCreateIfNotExist,
                                  xfReadWrite,
                                  xfShareNone);

    if (!dest_file)
        return;

    int size_to_read = 4096;
    unsigned char buffer[4096];
    bool success = true;

    while (1)
    {
        // read a chunk of the file and write it out
        int sizeread = xf_read(source_file, buffer, 1, size_to_read);

        // if we didn't read anything, we're done
        if (sizeread == 0)
            break;

        int sizewrite = xf_write(dest_file, buffer, 1, sizeread);

        // if we weren't able to write what we read, 
        // something went wrong
        if (sizewrite != sizeread)
        {
            success = false;
            break;
        }
    }

    // close the files
    xf_close(source_file);
    xf_close(dest_file);

    // if something went wrong, close the output
    // file, delete it, and return false
    if (!success)
    {
        xf_remove(dest_file_name);
        return;
    }

    // success
    retval->setBoolean(true);
}

// (METHOD) File.delete
//
// Description: Deletes a file.
//
// Syntax: static function File.delete(filename : String) : Boolean
//
// Remarks: Deletes the file specified by |filename| and
//     returns a boolean value indicating success or failure.
//
// Param(filename): The file to delete.
// Returns: Returns true upon success, and false otherwise.

void File::zdelete(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    bool result = false;
    
    if (env->m_param_count < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring filename = env->getParam(0)->getString();
    if (!filename.empty())
    {
        result = xf_remove(filename);
    }
    
    retval->setBoolean(result);
}

// (METHOD) File.move
//
// Description: Moves a file.
//
// Syntax: static function File.move(file_path : String,
//                                   new_file_path) : Boolean
//
// Remarks: Moves the file specified by |file_path| to the |new_file_path| and
//     returns a boolean value indicating success or failure.
//
// Param(file_path): The file to move.
// Param(new_file_path): The path of the newly moved file.
//
// Returns: Returns true upon success, and false otherwise.

void File::move(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    bool result = false;
    
    if (env->m_param_count < 2)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring filename = env->getParam(0)->getString();
    std::wstring new_filename = env->getParam(1)->getString();
    if (!filename.empty() &&
        !new_filename.empty())
    {
        result = xf_move(filename, new_filename);
    }
    
    retval->setBoolean(result);
}


// (METHOD) File.exists
//
// Description: Determines whether a file exists.
//
// Syntax: static function File.exists(filename : String) : Boolean
//
// Remarks: Determines whether the file specified by |filename| exists.
//
// Param(filename): The path to the file to check for existence.
// Returns: Returns true if the file exists, and false otherwise.

void File::exists(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{    
    if (env->m_param_count < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring filename = env->getParam(0)->getString();
    
    retval->setBoolean(xf_get_file_exist(filename));
}

// (METHOD) File.getTempFilename
//
// Description: Generates a unique filename for a temporary file.
//
// Syntax: static function File.getTempFilename() : String
//
// Remarks: Generates a unique filename for a temporary file.  The
//     function will use the system's default temporary path.  For
//     Windows systems, the path will be the users configured temporary
//     directory (usually in C:\Documents and Settings\(user name)\Local Settings\Temp).
//     For Linux systems, this will usually be the /tmp directory.
//
// Returns: Returns fully-qualified path for a temporary file.

void File::getTempFilename(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{    
    wxString res = wxStandardPaths::Get().GetTempDir();
    if (res.Length() == 0 || res.Last() != PATH_SEPARATOR_CHAR)
        res += PATH_SEPARATOR_CHAR;
    
    static int counter = 0;
    ++counter;
    res += wxString::Format(wxT("%08x%08x%04x%04x.tmp"),
                                (unsigned int)clock(),
                                (unsigned int)time(NULL),
                                (unsigned int)(rand() & 0xffff),
                                (unsigned int)(counter & 0xffff));
    
    retval->setString(towstr(res));
}


void File::putContents(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_param_count < 2)
    {
        retval->setBoolean(false);
        return;
    }


    xf_file_t f = xf_open(env->getParam(0)->getString(), xfCreate, xfWrite, xfShareNone);
    if (!f)
    {
        retval->setBoolean(false);
        return;
    }


    std::wstring val = env->getParam(1)->getString();
    bool uses_unicode = isUnicodeString(val);
    

    size_t buf_len = (val.size() * 4) + 3;
    unsigned char* buf = new unsigned char[buf_len];
    
    if (uses_unicode)
    {
        // convert to utf8
        kl::utf8_wtoutf8((char*)buf+3, buf_len-3, val.c_str(), val.length(), &buf_len);
        
        if (buf_len > 0 && (buf+3)[buf_len-1] == 0)
            buf_len--;
            
        // add space for the byte order mark
        buf_len += 3;
        
        // add byte order mark
        buf[0] = 0xef;
        buf[1] = 0xbb;
        buf[2] = 0xbf;
    }
    /* else ( if we want to save in ucs2 le )
    {
        // little endian byte order mark
        buf[0] = 0xff;
        buf[1] = 0xfe;
        kl::wstring2ucsle(buf+2, val, val.length());
        buf_len = (val.length() * 2) + 2;
    }*/
     else
    {
        // just save as 7-bit ascii because we don't use
        // any characters > char code 127
        std::string s = kl::tostring(val);
        buf_len = val.length();
        memcpy(buf, s.c_str(), buf_len);
    }


    xf_write(f, buf, 1, buf_len);
    xf_close(f);
    
    retval->setBoolean(true);
}

void File::getContents(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_param_count < 1)
    {
        retval->setNull();
        return;
    }
    
    // load stream data
    wxMemoryBuffer buf;
    
    xf_file_t f = xf_open(env->getParam(0)->getString(), xfOpen, xfRead, xfShareReadWrite);
    if (!f)
    {
        retval->setNull();
        return;
    }

    unsigned char* tempbuf = new unsigned char[32768];
    unsigned long read;
    
    while (1)
    {
        read = xf_read(f, tempbuf, 1, 32768);
        if (read == 0)
            break;
          
        buf.AppendData(tempbuf, read);
        
        if (read != 32768)
            break;
    }
    
    xf_close(f);
    delete[] tempbuf;
    
    
       
    
    std::wstring result_text = L"";
    

    unsigned char* ptr = (unsigned char*)buf.GetData();
    size_t buf_len = buf.GetDataLen();
    if (buf_len >= 2 && ptr[0] == 0xff && ptr[1] == 0xfe)
    {
        kl::ucsle2wstring(result_text, ptr+2, (buf_len-2)/2);
    }
     else if (buf_len >= 3 && ptr[0] == 0xef && ptr[1] == 0xbb && ptr[2] == 0xbf)
    {
        // utf-8
        wchar_t* tempbuf = new wchar_t[buf_len+1];
        kl::utf8_utf8tow(tempbuf, buf_len+1, (char*)ptr+3, buf_len-3);
        result_text = tempbuf;
        delete[] tempbuf;
    }
     else
    {
        buf.AppendByte(0);
        result_text = towstr((const char*)buf.GetData());
    }

    retval->setString(result_text);
}




// -- TextReader class implementation --

// (CLASS) TextReader
// Category: IO
// Description: A class that provides functionality to read text files.
// Remarks: The TextReader class represents a file access stream for reading
//     text values from a file.

TextReader::TextReader()
{
    m_f = NULL;

#ifdef WIN32
    m_buf = new wchar_t[10000];
#else
    m_buf = new char[10000];
#endif
}

TextReader::~TextReader()
{
    if (m_f)
        fclose(m_f);
        
    delete[] m_buf;
}

void TextReader::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) TextReader.readLine
//
// Description: Read a single line from a text file
//
// Syntax: function TextReader.readLine() : String
//
// Remarks: Reads a single line from a text file.  The file pointer is
//     automatically advanced to the next line.  A string containing the
//     line read is returned, and does not include the carriage return
//     or line-feed character.  If the file is not open, a null value
//     is returned.
//
// Returns: A string containing the line read from the input text file.
//     If the file is not open, the method returns a null value.


void TextReader::readLine(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_f)
    {
        retval->setNull();
        return;
    }

    int len = 0;

#ifdef WIN32
    if (!fgetws(m_buf, 9999, m_f))
    {
        retval->setNull();
        return;
    }
    len = wcslen(m_buf);
    if (len > 0 && m_buf[len-1] == L'\n')
    {
        len--;
        *(m_buf+len) = 0;
    }
    retval->setString(m_buf, len);
#else
    if (!fgets(m_buf, 9999, m_f))
    {
        retval->setNull();
        return;
    }
    len = strlen(m_buf);
    if (len > 0)
    {
        len--;
        *(m_buf+len) = 0;
    }
    std::wstring s = kl::towstring(m_buf);
    retval->setString(s.c_str(), s.length());
#endif

}

// (METHOD) TextReader.close
//
// Description: Closes the input text file
//
// Syntax: function TextReader.close() : Boolean
//
// Remarks: Closes the input text file.  All subsequent calls to readLine()
//     will return null.
//
// Returns: True if the file was successfully closed, false if the file
//     was not open when close() was invoked.


void TextReader::close(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_f)
    {
        fclose(m_f);
        m_f = NULL;
        retval->setBoolean(true);
    }
     else
    {
        retval->setBoolean(false);
    }
}




// (CLASS) TextWriter
// Category: IO
// Description: A class that provides functionality to write to text files.
// Remarks: The TextWriter class represents a file access stream for writing
//     text data to a text file.

TextWriter::TextWriter()
{
    m_f = NULL;
}

TextWriter::~TextWriter()
{
    if (m_f)
        fclose(m_f);
}

void TextWriter::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) TextWriter.write
//
// Description: Writes text to a text file
//
// Syntax: function TextWriter.write(text : String) : Boolean
//
// Remarks: Write a single line to a file, but does not add any carriage return
//     or line feed.
//
// Returns: True if the function succeeded, false upon failure.

void TextWriter::write(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_f)
    {
        retval->setBoolean(false);
        return;
    }

    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring s = env->getParam(0)->getString();

/*
#ifdef WIN32
    if (fputws(s.c_str(), m_f) < 0)
    {
        retval->setBoolean(false);
        return;
    }
#else*/
    std::string as = kl::tostring(s);
    if (fputs(as.c_str(), m_f) < 0)
    {
        retval->setBoolean(false);
        return;
    }
//#endif

    retval->setBoolean(true);
}


// (METHOD) TextWriter.writeLine
//
// Description: Writes a single line to a text file
//
// Syntax: function TextWriter.writeLine(text : String) : Boolean
//
// Remarks: Write a single line to a file, ending it with a newline.  The
//     platforms text file format is respected, so on Windows systems, a carriage
//     return and a line feed are added, but on Linux systems, only a newline
//     character is added.
//
// Returns: True if the function succeeded, false upon failure.

void TextWriter::writeLine(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_f)
    {
        retval->setBoolean(false);
        return;
    }

    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring s = env->getParam(0)->getString();
    s += L"\n";

/*
#ifdef WIN32
    if (fputws(s.c_str(), m_f) < 0)
    {
        retval->setBoolean(false);
        return;
    }
#else
*/
    std::string as = kl::tostring(s);
    if (fputs(as.c_str(), m_f) < 0)
    {
        retval->setBoolean(false);
        return;
    }
//#endif

    retval->setBoolean(true);
}


void TextWriter::close(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_f)
    {
        fclose(m_f);
        m_f = NULL;
        retval->setBoolean(true);
        return;
    }
    
    retval->setBoolean(false);
}





// (CLASS) DriveInfo
// Category: IO
// Description: A class that provides information about drives/volumes
// Remarks: The DriveInfo class provides information about drives and/or
//     volumes mounted on a system.

DriveInfo::DriveInfo()
{
    m_initialized = false;
    m_total_size = 0.0;
    m_available_space = 0.0;
    m_free_space = 0.0;
}

DriveInfo::~DriveInfo()
{
}

void DriveInfo::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) DriveInfo.getDrives
//
// Description: Retrieves information about all drives on the system
//
// Syntax: static function DriveInfo.getDrives() : Array(DriveInfo)
//
// Remarks: Calling DriveInfo.getDrives() returns an array of DriveInfo objects
//     which represent all available mounted volumes on the system.
//
// Returns: An array of DriveInfo objects

void DriveInfo::getDrives(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setArray(env);
    
    
    #ifdef WIN32
    TCHAR buf[512];
    GetLogicalDriveStrings(511, buf);
    
    TCHAR* p = buf;
    while (*p)
    {
        DriveInfo* info = DriveInfo::createObject(env);
        info->m_name = kl::towstring(p);
        kscript::Value val;
        val.setObject(info);
        retval->appendMember(&val);
        
        p += _tcslen(p);
        p++;
    }
    
    #else
    
    // unix-style OSs only have one volume, root.
    {
        DriveInfo* info = DriveInfo::createObject(env);
        info->m_name = L"/";
        kscript::Value val;
        val.setObject(info);
        retval->appendMember(&val);
    }

    #endif
}


void DriveInfo::retrieveInfo()
{
    m_initialized = true;
    
    m_available_space = std::numeric_limits<double>::quiet_NaN();
    m_free_space = std::numeric_limits<double>::quiet_NaN();
    m_total_size = std::numeric_limits<double>::quiet_NaN();
    
    if (m_name.length() == 0)
        return;

    #ifdef WIN32
    kl::tstr vol(m_name);
    ULARGE_INTEGER available, total, total_free;
    if (GetDiskFreeSpaceEx(m_name.c_str(), &available, &total, &total_free))
    {
        m_available_space = (double)(__int64)available.QuadPart;
        m_free_space = (double)(__int64)total_free.QuadPart;
        m_total_size = (double)(__int64)total.QuadPart;
    }
    #endif
}

// (METHOD) DriveInfo.getName
//
// Description: Gets the name of the volume
//
// Syntax: function DriveInfo.getName() : String
//
// Remarks: The getName() method returns the name of the volume.  The
//     exact format of this string depends on the platform under which
//     the program is running.  For example, on WIN32 systems, a volume
//     name can be "C:\", "D:\", etc. 
//
// Returns: A string containing the volume name

void DriveInfo::getName(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(m_name);
}


// (METHOD) DriveInfo.getAvailableSpace
//
// Description: Gets the amount of available space on the volume for a user
//
// Syntax: function DriveInfo.getAvailableSpace() : Number
//
// Remarks: The getAvailableSpace() method returns the number of bytes
//     available on the volume to the current user.  This can differ
//     from the total amount of space available if a quota system is
//     active.
//
// Returns: The amount of space available to the current user, in bytes.

void DriveInfo::getAvailableSpace(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_initialized)
        retrieveInfo();
    retval->setDouble(m_available_space);
}

// (METHOD) DriveInfo.getFreeSpace
//
// Description: Gets the amount of free space on the volume
//
// Syntax: function DriveInfo.getFreeSpace() : Number
//
// Remarks: The getFreeSpace() method returns the number of
//     free bytes on the volume.
//
// Returns: The amount of free space, in bytes.

void DriveInfo::getFreeSpace(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_initialized)
        retrieveInfo();
    retval->setDouble(m_free_space);
}

// (METHOD) DriveInfo.getTotalSize
//
// Description: Gets the total size of a volume
//
// Syntax: function DriveInfo.getTotalSize() : Number
//
// Remarks: The getTotalSize() method returns the total storage
//     capacity of a volume.
//
// Returns: The total size of a volume, in bytes.

void DriveInfo::getTotalSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_initialized)
        retrieveInfo();
    retval->setDouble(m_total_size);
}






// (CLASS) Log
// Category: IO
// Description: A class that provides logging functionality.
// Remarks: The Log Class allows applications to log debug, informational,
//     and error information that is produced during the course of execution
//     of a script.  Log entries can have different levels of severity.  There
//     are five different levels, in order of least severe to most severe:
//     Debug, Info, Warn, Error, and Fatal.  The log output may optionally be filtered,
//     so that only desired error levels are placed into the output log file.

// TODO: should this derive from the File class?

Log::Log()
{
    m_f = NULL;
    m_mask = 0x0fffffff;
    m_console_output = false;
}

Log::~Log()
{
    if (m_f)
    {
        fclose(m_f);
    }
}

void Log::addLogLine(int level, const std::wstring& str)
{
    std::string line;
    char buf[60];
    time_t t;
    struct tm lt;
    
    time(&t);
    localtime_r(&t, &lt);

    strftime(buf, 60, "%a %b %d %H:%M:%S %Y", &lt);

    line.reserve(30 + str.length());
    line += buf;

    switch (level)
    {
        case LevelDebug: line += " - [debug] "; break;
        case LevelInfo:  line += " - [info ] "; break;
        case LevelWarn:  line += " - [warn ] "; break;
        case LevelError: line += " - [error] "; break;
        case LevelFatal: line += " - [fatal] "; break;
    }
    
    line += kl::tostring(str);
    line += "\n";
    
    
    if (m_f)
    {
        fputs(line.c_str(), m_f);
        fflush(m_f);
    }
    
    if (m_console_output)
    {
        g_app->getAppController()->printConsoleText(line);
    }
}

void Log::setOutputFile(const std::wstring& filename)
{
    if (m_path == filename)
        return;
        
    if (m_f)
    {
        fclose(m_f);
    }
    
    m_f = NULL;
    m_path = L"";
    
    if (filename.length() > 0)
    {
#ifdef WIN32
        m_f = _wfopen(filename.c_str(), L"at");
#else
        m_f = fopen(kl::tostring(filename).c_str(), "at");
#endif
    }
    
    if (m_f)
    {
        m_path = filename;
    }
}

// (CONSTRUCTOR) Log.constructor
// Description: Creates a new Log object
//
// Syntax: Log(filename : String)
//
// Remarks: Creates a new Log object
//
// Param(filename): Optionally, the filename may be directly specified here


void Log::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        setOutputFile(env->getParam(0)->getString());
    }
}

// (METHOD) Log.setFilterLevel
//
// Description: Sets the filter level for the log output
//
// Syntax: function Log.setFilterLevel(level : Integer) : Undefined
//
// Remarks: Calling setFilterLevel() enables you to direct the log
//     class to only output certain log entries.  For instance, calling
//     setLogLevel with the Log.LevelDebug parameter would cause only
//     subsequent debug() messages to be inserted into the log output file.
//     These levels may be combined with a bitwise |or| mask.
//
// Param(level): A bitmask representing the log levels desired for the output
//     file.  The value may be one or a combination of the following:
//     LogLevel.Debug, LogLevel.Info, LogLevel.Warn, LogLevel.Error, or
//     LogLevel.Fatal.  All values can be specified at once using LogLevel.All
//
// Returns: Undefined

void Log::setFilterLevel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        m_mask = env->getParam(0)->getInteger();
    }
     else
    {
        m_mask = 0x0fffffff;
    }
}


// (METHOD) Log.setConsoleOutput
//
// Description: Activates or deactivates console output
//
// Syntax: function Log.setConsoleOutput(new_val : Boolean)
//
// Remarks: If setConsoleOutput() is activated, all log lines
//     that appear in the log file also appear in console output.
//
// Param(new_val): New setting of the console output option.
//     The default value is |true|.

void Log::setConsoleOutput(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        m_console_output = env->getParam(0)->getBoolean();
    }
     else
    {
        // default value if no parameter is specified
        m_console_output = true;
    }
}



// (METHOD) Log.open
//
// Description: Opens a log file for appending
//
// Syntax: function Log.open(filename : String) : Boolean
//
// Remarks: Calling open() opens a log file for appending.  If the log
//     file does not already exist, it is created.  If it already exists, it
//     it appended to, not truncated.
//
// Returns: True upon success, false if the open operation failed

void Log::open(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        setOutputFile(env->getParam(0)->getString());
    }
    
    retval->setBoolean(m_f ? true : false);
}


// (METHOD) Log.close
//
// Description: Closes the log file
//
// Syntax: function Log.close() : Boolean
//
// Remarks: Closes the log file
//
// Returns: True upon success, false otherwise

void Log::close(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_f)
    {
        fclose(m_f);
        m_f = NULL;
    }
    
    m_path = L"";
    
    retval->setBoolean(true);
}



// (METHOD) Log.erase
//
// Description: Truncates the log file to zero lines
//
// Syntax: function Log.erase() : Boolean
//
// Remarks: Calling Log.erase() truncates the log file to zero lines.
//     This is useful for resetting the log file for a new set of output.
//
// Returns: True upon success, false otherwise

void Log::erase(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_f || m_path.empty())
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring path = m_path;
    close(env, retval);
    
    xf_remove(path);
    
    setOutputFile(path);
    
    retval->setBoolean(true);
}



// (METHOD) Log.debug
//
// Description: Adds a debug output line to the log
// Syntax: function Log.debug() : Boolean
// Remarks: Adds a debug output line to the log
// Returns: True upon success, false otherwise

void Log::debug(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!(m_mask & LevelDebug))
    {
        retval->setBoolean(false);
        return;
    }
    
    size_t i, param_count = env->getParamCount();
    for (i = 0; i < param_count; ++i)
        addLogLine(LevelDebug, env->getParam(i)->getString());
        
    retval->setBoolean(true);
}

// (METHOD) Log.info
//
// Description: Adds an information output line to the log
// Syntax: function Log.info() : Boolean
// Remarks: Adds an information output line to the log
// Returns: True upon success, false otherwise

void Log::info(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!(m_mask & LevelInfo))
    {
        retval->setBoolean(false);
        return;
    }
    
    size_t i, param_count = env->getParamCount();
    for (i = 0; i < param_count; ++i)
        addLogLine(LevelInfo, env->getParam(i)->getString());

    retval->setBoolean(true);
}

// (METHOD) Log.warn
//
// Description: Adds a warning output line to the log
// Syntax: function Log.warn() : Boolean
// Remarks: Adds a warning output line to the log
// Returns: True upon success, false otherwise

void Log::warn(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!(m_mask & LevelWarn))
    {
        retval->setBoolean(false);
        return;
    }
    
    size_t i, param_count = env->getParamCount();
    for (i = 0; i < param_count; ++i)
        addLogLine(LevelWarn, env->getParam(i)->getString());
        
    retval->setBoolean(true);
}


// (METHOD) Log.error
//
// Description: Adds a error output line to the log
// Syntax: function Log.error() : Boolean
// Remarks: Adds an error output line to the log
// Returns: True upon success, false otherwise

void Log::error(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!(m_mask & LevelError))
    {
        retval->setBoolean(false);
        return;
    }
    
    size_t i, param_count = env->getParamCount();
    for (i = 0; i < param_count; ++i)
        addLogLine(LevelError, env->getParam(i)->getString());
        
    retval->setBoolean(true);
}

// (METHOD) Log.fatal
//
// Description: Adds a fatal output line to the log
// Syntax: function Log.fatal() : Boolean
// Remarks: Adds a fatal output line to the log
// Returns: True upon success, false otherwise

void Log::fatal(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!(m_mask & LevelFatal))
    {
        retval->setBoolean(false);
        return;
    }
    
    size_t i, param_count = env->getParamCount();
    for (i = 0; i < param_count; ++i)
        addLogLine(LevelFatal, env->getParam(i)->getString());
        
    retval->setBoolean(true);
}

