/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-29
 *
 */


#include <ctime>
#include "xdfs.h"
#include "database.h"
#include "delimitedtext.h"
#include "delimitedtextset.h"
#include "delimitedtextiterator.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/extfileinfo.h"
#include "kl/md5.h"
#include "kl/regex.h"
#include "kl/string.h"
#include "kl/url.h"



// DelimitedTextSet class implementation

DelimitedTextSet::DelimitedTextSet(FsDatabase* database) : XdfsBaseSet(database)
{
}

DelimitedTextSet::~DelimitedTextSet()
{
    if (m_file.isOpen())
        m_file.closeFile();
}

bool DelimitedTextSet::init(const std::wstring& url, const xd::FormatDefinition& def, xd::IJob* job)
{
    if (!m_file.open(url))
        return false;
    m_file_url = url;

    // set the set info filename
    std::wstring filename = m_file_url;
    if (kl::isFileUrl(filename))
    {
        filename = kl::urlToFilename(filename);
    }

    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    setConfigFilePath(ExtFileInfo::getConfigFilenameFromPath(definition_path, filename));


    // try to load field information from the file header (for example, icsv)
    if (def.format == xd::formatDefault && loadConfigurationFromDataFile())
    {
        // we were able to determine an implicit definition from the file's data
        return true;
    }

    m_def = def;

    if (m_def.format == xd::formatDefault)
    {
        if (m_database->loadAssignedDefinition(m_file_url, &m_def))
        {
            int i = 1; // rawvalue() is 1-based
            for (auto &col : m_def.columns)
            {
                if (col.expression.find(L"$SRCFIELD"))
                {
                    std::wstring replacement = kl::stdswprintf(L"rawvalue(%d)", i++);
                    kl::replaceStr(col.expression, L"$SRCFIELD", replacement, true);
                }
            }
        }
    }

    if (m_def.format == xd::formatDefault)
    {
        // there is no specified configuration about this file, so we have to
        // run logic that tries to determine the best way to open this file
        
        // look for an extension -- if no extension, assume csv
        std::wstring ext;
        size_t ext_pos = url.find_last_of(L'.');
        if (ext_pos != url.npos)
            ext = url.substr(ext_pos+1);
             else
            ext = L"csv";
        kl::makeLower(ext);
        
        
        if (ext == L"tsv")
        {
            // these settings follows the "tsv" standard.  Note, if the file
            // has a .tsv extension, we simply assume without any further investigation
            // that the file is tab delimited
            m_def.format = xd::formatDelimitedText;
            m_def.delimiter = L"\t";
            m_def.text_qualifier = L"";
            m_def.header_row = true;
        }
         else
        {
            // these settings follows the "csv" standard
            m_def.format = xd::formatDelimitedText;
            m_def.delimiter = L",";
            m_def.text_qualifier = L"\"";
            m_def.header_row = true;
            
            // however, many csv files also use other delimiters, like semicolons
            xd::FormatDefinition info;
            if (m_database->getFileFormat(url, m_file.getStream(), &info, true /* discover_delimiters */))
            {
                m_def.delimiter = info.delimiter;
            }
        }

        // clear out columns because format is set to default
        m_def.columns.clear();
    }



    

    xd::FormatDefinition determine_info;
    bool determine_info_populated = false;

    // if a definition was specified, but the values are empty, specify defaults
    if (m_def.delimiter.empty() && m_def.determine_delimiters)
    {
        if (m_database->getFileFormat(url, m_file.getStream(), &determine_info, true /* discover_delimiters */))
        {
            m_def.delimiter = determine_info.delimiter;
            determine_info_populated = true;
        }
    }

    if (m_def.line_delimiter.empty())
    {
        if (m_def.determine_delimiters)
        {
            if (!determine_info_populated)
            {
                if (m_database->getFileFormat(url, m_file.getStream(), &determine_info, true /* discover_delimiters */))
                {
                    m_def.line_delimiter = determine_info.line_delimiter;
                }
            }
        }

        if (m_def.line_delimiter.empty())
        {
            m_def.line_delimiter = L"\n";
        }
    }

    

        
    // make sure the text-delimited file class is updated
    m_file.setDelimiters(m_def.delimiter);
    m_file.setLineDelimiters(m_def.line_delimiter);
    m_file.setTextQualifiers(m_def.text_qualifier);


    if (m_def.columns.size() == 0)
    {
        if (m_def.determine_structure)
        {
            // determine csv structure thoroughly by scanning entire file
            determineColumns(-1, -1, job);
        }
         else
        {
            // check as many rows as we can in 7 seconds or less
            int rows_to_check = -1;
            int max_seconds = 7;

            // if the file is small (<= 2MB), just read in the whole
            // file to make sure we have a good structure
            if (m_file.getStream()->getSize() < 2000000)
            {
                rows_to_check = -1;
                max_seconds = -1;
            }

            // originally, if we couldn't define the structure of the file, we
            // would bail out; now simply try to determine the structure, but
            // don't bail out if we can't determine it; the reason for this is
            // that sometimes we need to create a set and then define the
            // structure afterwards, such as when exporting to a text delimited
            // file; in this case, trying to determine the rows ahead of time
            // will cause the export to fail since the file we're exporting to
            // doesn't have rows until after the structure is set and the
            // row written out

            determineColumns(rows_to_check, max_seconds, job);
        }
    }

    m_file.rewind();

    return true;
}


std::wstring DelimitedTextSet::getSetId()
{
    std::wstring set_id;
    
    set_id = L"xdfs:";
    set_id += xf_get_network_path(m_file_url);
    
#ifdef WIN32
    // win32's filenames are case-insensitive, so
    // when generating the set id, make the whole filename
    // lowercase to avoid multiple id's for the same file
    kl::makeLower(set_id);
#endif
    
    return kl::md5str(set_id);
}

xd::IRowInserterPtr DelimitedTextSet::getRowInserter()
{
    DelimitedTextRowInserter* inserter = new DelimitedTextRowInserter(this);
    return static_cast<xd::IRowInserter*>(inserter);
}

bool DelimitedTextSet::getFormatDefinition(xd::FormatDefinition* def)
{
    *def = m_def;

    appendCalcFields(def->columns);

    return true;
}

xd::IIteratorPtr DelimitedTextSet::createIterator(const std::wstring& columns,
                                                  const std::wstring& order,
                                                  xd::IJob* job)
{
    if (order.empty())
    {
        DelimitedTextIterator* iter = new DelimitedTextIterator(m_database);
        if (!iter->init(this, columns))
        {
            delete iter;
            return xcm::null;
        }

        return static_cast<xd::IIterator*>(iter);
    }
    
    // find out where the database should put temporary files
    std::wstring temp_directory = m_database->getTempFileDirectory();

    // create a unique index file name with .idx extension
    std::wstring index_filename = getUniqueString();
    index_filename += L".idx";

    // generate a full path name from the temp path and unique idx filename
    std::wstring full_index_filename;
    full_index_filename = makePathName(temp_directory,
                                       L"",
                                       index_filename);

    IIndex* idx;
    idx = createExternalIndex(m_database,
                              getObjectPath(),
                              full_index_filename,
                              temp_directory,
                              order,
                              true,
                              true,
                              job);
    if (!idx)
        return xcm::null;

    xd::IIteratorPtr data_iter = createIterator(columns, L"", NULL);
    if (data_iter.isNull())
    {
        idx->unref();
        return xcm::null;
    }

    xd::IIteratorPtr result_iter = createIteratorFromIndex(data_iter,
                                                           idx,
                                                           columns,
                                                           order,
                                                           getObjectPath());

    idx->unref();
    return result_iter;
}

xd::rowpos_t DelimitedTextSet::getRowCount()
{
    return 0;
}





bool DelimitedTextSet::loadConfigurationFromDataFile()
{
    // if this file is an "icsv", we can get the format from the header row

    m_file.rewind();
    if (m_file.eof())
        return false;
    
    std::vector<xd::ColumnInfo> fields;
    std::vector<std::wstring> params;
    std::vector<std::wstring> tempvec;
    std::vector<std::wstring>::iterator it;
    xd::ColumnInfo colinfo;
    std::wstring params_str;

    size_t i, row_cell_count = m_file.getRowCellCount();
    for (i = 0; i < row_cell_count; ++i)
    {
        std::wstring str = m_file.getString(i);
        kl::trim(str);
        size_t open = str.find_last_of('(');
        size_t close = str.find_last_of(')');
        
        if (open == str.npos || close == str.npos || open > close || close != str.length()-1)
            return false;
        
        colinfo.name = str.substr(0, open);
        params_str = str.substr(open+1, close-open-1);
        
        kl::trim(colinfo.name);
        kl::trim(params_str);
        kl::replaceStr(params_str, L"\t", L" ");
        
        tempvec.clear();
        kl::parseDelimitedList(params_str, tempvec, L' ');
        
        // remove all empty elements from the parameters array; 
        // this takes care of multiple whitespaces in the parameters string
        params.clear();
        for (it = tempvec.begin(); it != tempvec.end(); ++it)
        {
            if (it->length() > 0)
                params.push_back(*it);
        }
        
        if (params.size() == 0)
            return false; // no parameters specified
        
        if (params[0] == L"C")
        {
            if (params.size() < 2)
                return false;
            
            int width = kl::wtoi(params[1]);
            if (width < 1)
                return false;
            
            colinfo.type = xd::typeCharacter;
            colinfo.width = width;
            colinfo.scale = 0;
        }
         else if (params[0] == L"N")
        {
            if (params.size() < 2)
                return false;
                
            int width = kl::wtoi(params[1]);
            if (width < 1)
                return false;
            
            colinfo.type = xd::typeNumeric;
            colinfo.width = width;
            colinfo.scale = 0;
           
            if (params.size() >= 3)
            {
                int scale = kl::wtoi(params[2]);
                if (scale < 0)
                    return false;
                colinfo.scale = scale;
            }
        }
         else if (params[0] == L"D")
        {
            colinfo.type = xd::typeDate;
            colinfo.width = 4;
            colinfo.scale = 0;
        }
         else if (params[0] == L"T")
        {
            colinfo.type = xd::typeDateTime;
            colinfo.width = 8;
            colinfo.scale = 0;
        }
         else if (params[0] == L"B")
        {
            colinfo.type = xd::typeBoolean;
            colinfo.width = 1;
            colinfo.scale = 0;
        }
         else
        {
            // unknown type
            return false;
        }

        fields.push_back(colinfo);
    }
    

    m_def.format = xd::formatTypedDelimitedText;
    m_def.delimiter = L",";
    m_def.text_qualifier = L"\"";
    m_def.header_row = true;
    m_def.columns = fields;

    return true;
}


xd::Structure DelimitedTextSet::getStructure()
{
    xd::Structure s;

    std::vector<xd::ColumnInfo>::iterator it, it_end = m_def.columns.end();
    int counter = 0;
    for (it = m_def.columns.begin(); it != it_end; ++it)
    {
        xd::ColumnInfo col;
        
        col.name = it->name;
        col.type = it->type;
        col.width = it->width;
        col.scale = it->scale;
        col.source_offset = 0;
        col.calculated = false;
        col.column_ordinal = counter++;
        col.table_ordinal = 0;
        col.nulls_allowed = it->nulls_allowed;

        s.createColumn(col);
    }

    XdfsBaseSet::appendCalcFields(s);
    return s;
}

xd::Structure DelimitedTextSet::getStructureWithTransformations()
{
    xd::Structure s;

    std::vector<xd::ColumnInfo>::iterator it, it_end = m_def.columns.end();
    int counter = 0;
    for (it = m_def.columns.begin(); it != it_end; ++it)
    {
        xd::ColumnInfo col;

        col.name = it->name;
        col.type = it->type;
        col.width = it->width;
        col.scale = it->scale;
        col.source_offset = 0;
        col.calculated = false;
        col.column_ordinal = counter++;
        col.table_ordinal = 0;
        col.nulls_allowed = it->nulls_allowed;
        col.expression = it->expression;
        col.calculated = it->calculated;

        s.createColumn(col);
    }

    XdfsBaseSet::appendCalcFields(s);
    return s;
}


bool DelimitedTextSet::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    bool done_flag = false;
    XdfsBaseSet::modifyStructure(mod_params, &done_flag);
    return true;
}

static std::wstring makeValidFieldName(const std::wstring& name,
                                       const std::wstring& keyword_list,
                                       const std::wstring& invalid_col_chars)
{
    static std::vector<std::wstring> keywords;
    if (keywords.size() == 0)
    {
        std::vector<std::wstring> vec;
        kl::parseDelimitedList(keyword_list, vec, L',');

        size_t i, count = vec.size();
        for (i = 0; i < count; ++i)
            keywords.push_back(vec[i]);
    }
    
    int i, count;

    std::wstring result = name;
    kl::trim(result);
    count = (int)result.length();

    // if it's an empty string, return empty
    if (count == 0)
        return result;


    for (i = 0; i < count; ++i)
    {
        if (wcschr(invalid_col_chars.c_str(), result[i]))
        {
            bool delete_char = false;

            // if the character is the first or the last character in the string, delete it
            if (i == 0 || i == (count-1))
                delete_char = true;

            // if the previous character is an underscore or a space, don't duplicate
            // the underscore; rather delete the special character
            if (i > 0 && (result[i-1] == '_' || ::iswspace(result[i-1])))
                delete_char = true;

            // if the next character is an underscore, delete the special character
            if (i < (count-1) && result[i+1] == '_')
                delete_char = true;

            if (delete_char)
            {
                result.erase(i,1);
                if (result.empty())
                    return result;
                count--;
                i--;
                continue;
            }

            result[i] = '_';
        }
    }


    // make sure the field name is not a keyword
    count = (int)keywords.size();
    for (i = 0; i < count; ++i)
    {
        if (kl::iequals(result, keywords[i]))
        {
            result = L"f_" + result;
            break;
        }
    }

    // if the field starts with a number, prefix it with f_
    if (::iswdigit(result[0]))
        result = L"f_" + result;

    // rename badly named columns to something that will work in all databases

    if (result.find('#') != result.npos)
        kl::replaceStr(result, L"#", L"no");
    if (result.find('%') != result.npos)
        kl::replaceStr(result, L"%", L"pct");
    if (result.find('-') != result.npos)
        kl::replaceStr(result, L"-", L"_");


    kl::trim(result);
    return result;
}


// determineFirstRowHeader() tries to determine if the first
// row contains field names. determineColumns*() should be called
// after this function to find out field widths

bool DelimitedTextSet::determineIfFirstRowIsHeader()
{
    // go to the beginning of the file
    m_file.rewind();
            
    size_t i, cell_count = m_file.getRowCellCount();
    for (i = 0; i < cell_count; ++i)
    {
        std::wstring text = m_file.getString(i);
        kl::trim(text);
        
        // if a cell is empty, the first row is
        // probably not a list of field names
        if (text.empty())
            return false;
        
        // if a cell starts with a numeric digit, the first
        // row is probably not a list of field names
        if (wcschr(L"0123456789!@#$%^&*()-+=:;<>,./?'\"`\\", text[0]))
            return false;
    }
    
    return true;
}


static bool isDelimitedStringNumeric(const std::wstring& str, int* field_width, int* num_decimals)
{
    int i, len = (int)str.length();
    if (len == 0)
        return false;
    
    int num_dec = 0;
    bool found_dec = false;
    
    int sign_count = 0;
    int dollarsign_count = 0;
    int period_count = 0;
    
    wchar_t ch;
    for (i = len-1; i >= 0; --i)
    {
        ch = str[i];
        if (ch == '.')
            found_dec = true;
        if (iswdigit((unsigned char)ch))
        {
            if (!found_dec)
                num_dec++;
            continue;
        }
        
        if (ch == '-' || ch == '+')
        {
            if (++sign_count > 1)
                return false;
        }
         else if (ch == '$')
        {
            if (++dollarsign_count > 1)
                return false;
        }
         else if (ch == '.')
        {
            if (++period_count > 1)
                return false;
        }
         else if (ch == ',' || ch == '%')
        {
            // these chars are allowed in numbers
        }
         else
        {
            return false;
        }
    }
    
    if (!found_dec)
        num_dec = 0;
        
    if (num_decimals)
        *num_decimals = num_dec;
    
    // discover width -- start with string width, and deduct
    // leading zeros before decimal place and trailing zeros
    // after decimal place

    int width = (int)str.length();
    for (i = 0; i < len; ++i)
    {
        if (str[i] == ' ' || str[i] == '$' || str[i] == '%')
            continue;
        if (str[i] == '0') // leading zero
            width--;
             else
            break;
    }

    if (found_dec)
    {
        for (i = len-1; i >= 0; --i)
        {
            if (str[i] == ' ' || str[i] == '$' || str[i] == '%')
                continue;
            if (str[i] == '0') // leading zero
                width--;
                 else
                break;
        }
    }

    *field_width = (width <= 0 ? 1 : width);
      

    return true;
}





static bool extractMatchResults(klregex::wmatch& matchres,
                                int* yy,
                                int* mm,
                                int* dd)
{
    const klregex::wsubmatch& year_match = matchres[L"year"];
    const klregex::wsubmatch& month_match = matchres[L"month"];
    const klregex::wsubmatch& day_match = matchres[L"day"];

    if (!year_match.isValid())
        return false;
    if (!month_match.isValid())
        return false;
    if (!day_match.isValid())
        return false;
    
    static const wchar_t* months[] = { L"JAN", L"FEB", L"MAR",
                                       L"APR", L"MAY", L"JUN",
                                       L"JUL", L"AUG", L"SEP",
                                       L"OCT", L"NOV", L"DEC" };
    if (yy)
    {
        *yy = kl::wtoi(year_match.str().c_str());
        if (*yy < 100)
        {
            if (*yy < 70)
                *yy += 2000;
                 else
                *yy += 1900;
        }
    }

    if (mm)
    {
        std::wstring month = month_match.str();
        if (month.length() == 0)
            return false;
            
        if (iswdigit(month[0]))
        {
            *mm = kl::wtoi(month.c_str());
        }
         else
        {            
            int j;
            bool found = false;
            for (j = 0; j < 12; ++j)
            {
                if (0 == wcsncasecmp(month.c_str(), months[j], 3))
                {
                    *mm = j+1;
                    found = true;
                    break;
                }
            }
            
            if (!found)
            {
                // unknown month name
                return false;
            }
        }
    }
    
    if (dd)
    {
        *dd = kl::wtoi(day_match.str().c_str());
    }

    return true;
}


bool parseDelimitedStringDate(const std::wstring& str,
                              int* year,
                              int* month,
                              int* day)
{
    if (str.length() == 0)
        return false;

    if (year)
    {
        *year = 0;
        *month = 0;
        *day = 0;
    }
    
    // CCYY-MM-DD this format is so commonly a date format that
    // we don't need to do further validation.  It does however check to make
    // sure the first digit is between 0 and 3 (should be enough for years 0 - 3999)
    static const klregex::wregex fmt1(L"(?<year>[0-3]\\d{3})[-/. ](?<month>\\d{1,2})[-/. ](?<day>\\d{1,2})");
    
    // this format checks for CCYYMMDD, but does numerical range checking on the month and day portions
    static const klregex::wregex fmt2(L"(?<year>[[0-9]{4})(?<month>0[1-9]|1[0-2])(?<day>0[1-9]|[12][0-9]|3[01])");

    // this format checks for MM/DD/CCYY
    static const klregex::wregex fmt3(L"(?<month>0?[1-9]|1[0-2])[-/. ](?<day>0?[1-9]|[12][0-9]|3[01])[-/. ](?<year>[0-9]{2,4})");

    // this format checks for DD/MM/CCYY
    static const klregex::wregex fmt4(L"(?<day>0?[1-9]|[12][0-9]|3[01])[-/. ](?<month>0?[1-9]|1[0-2])[-/. ](?<year>[0-9]{2,4})");

    // this format checks for DD-MMM-CCYY
    static const klregex::wregex fmt5(L"(?<day>0?[1-9]|[12][0-9]|3[01])[-/., ]+(?<month>\\w+)[-/., ]+(?<year>[0-9]{2,4})");
    
    // this format checks for MMM DD CCYY
    static const klregex::wregex fmt6(L"(?i)(?<month>jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)\\w*[-/., ]+(?<day>0?[1-9]|[12][0-9]|3[01])[-/., ]+(?<year>[0-9]{2,4})");

    klregex::wmatch matchres;
    
    const wchar_t* start = str.c_str();
    const wchar_t* end = start + str.length();
    
    if (fmt1.match(start, end, matchres) || 
        fmt2.match(start, end, matchres))
    {
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt3.match(start, end, matchres))
    {
        // using MDY order
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt4.match(start, end, matchres))
    {
        // using YMD order
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
    
    if (fmt5.match(start, end, matchres) ||
        fmt6.match(start, end, matchres)) 
    {
        if (year)
            extractMatchResults(matchres, year, month, day);
        return true;
    }
  
    return false;
}



struct DetermineColumnInfo
{
    std::wstring name;
    int type;
    int max_width;
    int max_numeric_width;
    int max_scale;
    
    DetermineColumnInfo()
    {
        type = xd::typeDate;
        max_width = 1;
        max_numeric_width = 10; // numeric fields should at least be 10 wide when auto-sensing
        max_scale = 0;
    }
    
    DetermineColumnInfo(const DetermineColumnInfo& c)
    {
        name = c.name;
        type = c.type;
        max_width = c.max_width;
        max_numeric_width = c.max_numeric_width;
        max_scale = c.max_scale;
    }
    
    DetermineColumnInfo& operator=(const DetermineColumnInfo& c)
    {
        name = c.name;
        type = c.type;
        max_width = c.max_width;
        max_numeric_width = c.max_numeric_width;
        max_scale = c.max_scale;
        return *this;
    }
};


bool DelimitedTextSet::determineColumns(int check_rows, int max_seconds, xd::IJob* job)
{
    // if field information is stored in the header row, no need to sense structure
    if (loadConfigurationFromDataFile())
        return true;
    
    IJobInternalPtr ijob = job;
    if (job)
    {
        ijob->setStartTime(time(NULL));
        ijob->setStatus(xd::jobRunning);
        ijob->setCanCancel(true);
        ijob->setCurrentCount(0);
        ijob->setMaxCount((check_rows != -1) ? check_rows : 0);
    }
    

    int max_clock_cycles = -1;
    clock_t c1;
    if (max_seconds > 0)
    {
        c1 = clock();
        max_clock_cycles = CLOCKS_PER_SEC * max_seconds;
    }


    // pass through file metadata to the DelimitedTextFile class
    m_file.setDelimiters(m_def.delimiter);
    m_file.setLineDelimiters(m_def.line_delimiter);
    m_file.setTextQualifiers(m_def.text_qualifier);

    // get the database keywords and invalid column characters
    // for use in the makeValidFieldName() function below
    
    xd::IAttributesPtr attr = m_database->getAttributes();
    if (attr.isNull())
        return false;
    
    std::wstring keyword_list;
    keyword_list = attr->getStringAttribute(xd::dbattrKeywords);
    
    std::wstring invalid_col_chars;
    invalid_col_chars = attr->getStringAttribute(xd::dbattrColumnInvalidChars);

    // for now, don't allow spaces in csv file field names
    invalid_col_chars += L' ';
    
    bool cancelled = false;
    int i, j, width, col_count = 0;
    int row_cell_count, rows_read = 0;
    std::vector<DetermineColumnInfo> col_stats;
    std::wstring colname;

    // go to the beginning of the file
    m_file.rewind();

    // scan through the file to gather column information
    while (!m_file.eof())
    {
        row_cell_count = (int)m_file.getRowCellCount();

        // the current row has more cells than any of the preceeding rows
        while (col_count < row_cell_count)
        {
            wchar_t temps[81];
            swprintf(temps, 81, L"Field%d", col_count+1);

            colname = L"";
            
            // get the first row's cell values for possible column names
            if (rows_read == 0)
            {
                colname = m_file.getString(col_count);
                if (colname.length() > 80)
                    colname = colname.substr(0, 80);
            }
            
            // set the column's name based on the m_header_row flag
            if (m_def.header_row)
                colname = makeValidFieldName(colname, keyword_list, invalid_col_chars);
                 else
                colname = temps;
            
            // make sure we don't have any empty column names
            if (colname.empty())
                colname = temps;


            DetermineColumnInfo dci;
            dci.name = colname;

            col_stats.push_back(dci);
            col_count++;
        }
        
        for (j = 0; j < row_cell_count; ++j)
        {
            const std::wstring& str = m_file.getString(j);
            width = (int)str.length();

            // note: extra blank lines in a delimited file can cause the field
            // type of the first row to be changed to character (e.g. if the 
            // first field is comprised of dates, extra blank lines at the end
            // will cause this field to be interpreted as a character field);
            // ignore blank lines in determining the structure
            if (row_cell_count <= 1 && width == 0)
                continue;

            // if the width of this cell is greater than the column width
            // in the structure, update the column width accordingly
            if (width > col_stats[j].max_width)
                col_stats[j].max_width = width;
            
            // if the first row contains column names, don't use it during
            // statistical gathering of column widths and types
            if (m_def.header_row == false || rows_read > 0)
            {

                if (col_stats[j].type == xd::typeDate)
                {
                    // check if the field value is a date
                    if (str.length() > 0 && !parseDelimitedStringDate(str))
                    {
                        // if not, then fall back to a numeric
                        col_stats[j].type = xd::typeNumeric;
                    }
                }
                
                if (col_stats[j].type == xd::typeNumeric)
                {
                    int numeric_width = 0;
                    int numeric_scale = 0;
                    
                    if (isDelimitedStringNumeric(str, &numeric_width, &numeric_scale))
                    {
                        // see if our max_scale needs to be increased
                        if (numeric_width > col_stats[j].max_numeric_width)
                            col_stats[j].max_numeric_width = numeric_width;

                        // see if our max_scale needs to be increased
                        if (numeric_scale > col_stats[j].max_scale)
                            col_stats[j].max_scale = numeric_scale;
                    }
                     else
                    {
                        if (str.length() > 0)
                        {
                            // field is no longer numeric 
                            col_stats[j].type = (m_file.isUnicode() ? xd::typeWideCharacter : xd::typeCharacter);
                            col_stats[j].max_scale = 0;
                        }
                    }
                }
            }
        }

        // stop iterating through the rows if we've read through
        // the required number of rows to determing the structure
        rows_read++;
        if ((check_rows != -1) && (rows_read >= check_rows))
            break;

        if (max_clock_cycles != -1 && (rows_read % 100) == 0 && rows_read > 1000)
        {
            if ((clock() - c1) > max_clock_cycles)
                break;
        }

        if (job && (rows_read % 1000) == 0)
        {
            ijob->setCurrentCount(rows_read);

            if (job->getCancelled())
            {
                cancelled = true;
                break;
            }
        }

        m_file.skip(1);
    }

    // if we still don't have any columns, we're in trouble, bail out
    if (col_count < 1)
        return false;
    
    if (check_rows != -1 || cancelled)
    {
        col_count = (int)col_stats.size();
        
        // expand all field widths by 20% just for good measure
        for (i = 0; i < col_count; ++i)
        {
            col_stats[i].max_width = (int)(ceil((double)(col_stats[i].max_width) * 1.2));
            if (col_stats[i].max_width <= 0)
                col_stats[i].max_width = 1;
        }
    }
     else
    {
        // make sure there are no zero-sized columns
        for (i = 0; i < col_count; ++i)
        {
            if (col_stats[i].max_width <= 0)
                col_stats[i].max_width = 1;
            if (col_stats[i].max_numeric_width <= 10)
                col_stats[i].max_numeric_width = 10;
        }
    }

    // check numeric-typed fields for width/scale conformance
    for (i = 0; i < col_count; ++i)
    {
        if (col_stats[i].type == xd::typeNumeric)
        {
            if (col_stats[i].max_numeric_width > xd::max_numeric_width)
            {
                col_stats[i].type = xd::typeDouble;
            }
        }
        
        if (col_stats[i].type == xd::typeNumeric ||
            col_stats[i].type == xd::typeDouble)
        {
            if (col_stats[i].max_scale > xd::max_numeric_scale)
            {
                col_stats[i].max_scale = xd::max_numeric_scale;
            }
        }
    }

    // check fields for duplicate names
    std::map<std::wstring, int, kl::cmp_nocase> field_map;
    std::map<std::wstring, int, kl::cmp_nocase> field_map_counter;
    std::map<std::wstring, int, kl::cmp_nocase>::iterator field_map_iter;

    for (i = 0; i < col_count; ++i)
    {
        field_map_iter = field_map.find(col_stats[i].name);
        if (field_map_iter == field_map.end())
            field_map[col_stats[i].name] = 1;
             else
            field_map_iter->second++;
    }

    for (i = 0; i < col_count; ++i)
    {
        field_map_iter = field_map.find(col_stats[i].name);
        if (field_map_iter->second > 1)
        {
            int cnt = ++field_map_counter[col_stats[i].name];
            col_stats[i].name = col_stats[i].name + kl::itowstring(cnt);
        }
    }


    if (job && !cancelled)
    {
        ijob->setCurrentCount(rows_read);
        ijob->setMaxCount(rows_read);
        ijob->setFinishTime(time(NULL));
        ijob->setStatus(xd::jobFinished);
    }
    

    std::vector<DetermineColumnInfo>::iterator it, it_end = col_stats.end();
    xd::ColumnInfo col;
    for (it = col_stats.begin(); it != it_end; ++it)
    {
        col.name = it->name;
        col.type = it->type;

        if (it->type == xd::typeNumeric || it->type == xd::typeDouble)
        {
            col.width = it->max_numeric_width;
        }
         else
        {
            col.width = it->max_width;
        }

        col.scale = it->max_scale;
        col.nulls_allowed = false;
        m_def.columns.push_back(col);
    }

    // once again, go back to the beginning of the file
    m_file.rewind();
    return true;
}



// -- DelimitedTextRowInserter class implementation --

DelimitedTextRowInserter::DelimitedTextRowInserter(DelimitedTextSet* set)
{
    m_set = set;
    m_set->ref();

    m_file = &(m_set->m_file);
    m_inserting = false;
}

DelimitedTextRowInserter::~DelimitedTextRowInserter()
{
    // free all insert field data
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    m_set->unref();
}

xd::objhandle_t DelimitedTextRowInserter::getHandle(const std::wstring& column_name)
{
    if (!m_inserting)
        return (xd::objhandle_t)0;
    
    std::vector<DelimitedTextDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (0 == wcscasecmp((*it)->name.c_str(), column_name.c_str()))
            return (xd::objhandle_t)(*it);
    }

    return (xd::objhandle_t)0;
}

bool DelimitedTextRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                         const unsigned char* value,
                                         int length)
{
    return false;
}

bool DelimitedTextRowInserter::putString(xd::objhandle_t column_handle,
                                         const std::string& value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putString(f->ordinal, kl::towstring(value));
}

bool DelimitedTextRowInserter::putWideString(xd::objhandle_t column_handle,
                                             const std::wstring& value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putString(f->ordinal, value);
}

bool DelimitedTextRowInserter::putDouble(xd::objhandle_t column_handle,
                                         double value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putDouble(f->ordinal, value);
}

bool DelimitedTextRowInserter::putInteger(xd::objhandle_t column_handle,
                                          int value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putInteger(f->ordinal, value);
}

bool DelimitedTextRowInserter::putBoolean(xd::objhandle_t column_handle,
                                          bool value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putBoolean(f->ordinal, value);
}

bool DelimitedTextRowInserter::putDateTime(xd::objhandle_t column_handle,
                                           xd::datetime_t value)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    if (value == 0)
    {
        // empty date
        return m_row.putDateTime(f->ordinal, L"");
    }
    
    xd::DateTime dt(value);

    int y = dt.getYear();
    int m = dt.getMonth();
    int d = dt.getDay();
    int hh = dt.getHour();
    int mm = dt.getMinute();
    int ss = dt.getSecond();

    wchar_t buf[64];

    if (hh == 0 && mm == 0 && ss == 0)
    {
        swprintf(buf, 64, L"%04d/%02d/%02d", y, m, d);
    }
     else
    {
        swprintf(buf, 64, L"%04d/%02d/%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
    }

    return m_row.putDateTime(f->ordinal, buf);
}

bool DelimitedTextRowInserter::putNull(xd::objhandle_t column_handle)
{
    DelimitedTextDataAccessInfo* f = (DelimitedTextDataAccessInfo*)column_handle;
    if (!f)
        return false;

    return m_row.putNull(f->ordinal);
}

bool DelimitedTextRowInserter::startInsert(const std::wstring& col_list)
{
    xd::Structure s = m_set->getStructure();
    
    size_t i, col_count = s.getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& colinfo = s.getColumnInfoByIdx(i);
        
        DelimitedTextDataAccessInfo* f = new DelimitedTextDataAccessInfo;
        f->name = colinfo.name;
        f->type = colinfo.type;
        f->width = colinfo.width;
        f->scale = colinfo.scale;
        f->ordinal = colinfo.column_ordinal;
        f->nulls_allowed = colinfo.nulls_allowed;
        f->expr_text = colinfo.expression;

        m_fields.push_back(f);
    }

    m_inserting = true;
    return m_file->startInsert();
}

bool DelimitedTextRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    bool res = m_file->insertRow(m_row);
    
    m_row.clear();
    
    return res;
}

void DelimitedTextRowInserter::finishInsert()
{
    m_file->finishInsert();
    m_inserting = false;
}

bool DelimitedTextRowInserter::flush()
{
    m_file->flush();
    return true;
}
