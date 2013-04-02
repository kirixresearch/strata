/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-12-03
 *
 */


#include "appmain.h"
#include "jobimportpkg.h"
#include "pkgfile.h"
#include "appcontroller.h"
#include "tabledoc.h"
#include <kl/crypt.h>


// password for scrambling password in templates and package files
#define PASSWORD_KEY L"83401732"


void structure2xml(tango::IStructurePtr s, kl::xmlnode& node);
tango::IStructurePtr xml2structure(kl::xmlnode& node);

void tabledocview2xml(ITableDocViewPtr view, kl::xmlnode& node);
bool xml2tabledocview(kl::xmlnode& node, ITableDocViewPtr view);

void tabledocmodel2xml(ITableDocModelPtr model, kl::xmlnode& root);
void xml2tabledocmodel(kl::xmlnode& node, ITableDocModelPtr model);


// -- ImportPkgJob class implementation --


ImportPkgJob::ImportPkgJob()
{

}

ImportPkgJob::~ImportPkgJob()
{

}


void ImportPkgJob::setPkgFilename(const std::wstring& filename)
{
    m_filename = filename;
}

void ImportPkgJob::addImportObject(const std::wstring& stream_name,
                                   const std::wstring& output_path)
{
    PkgImportInfo info;
    info.stream_name = stream_name;
    info.output_path = output_path;

    m_info.push_back(info);
}





bool ImportPkgJob::importStream(PkgStreamReader* reader,
                                PkgImportInfo* info,
                                kl::xmlnode& stream_info)
{
    std::wstring mime_type;
    const unsigned char* data = NULL;
    int block_size = 0;
    bool failed = false;
    
    int node_idx = stream_info.getChildIdx(L"mime_type");
    if (node_idx == -1)
    {
        // could not find physical row width
        return false;
    }
    mime_type = stream_info.getChild(node_idx).getNodeValue();



    tango::IStreamPtr stream = g_app->getDatabase()->createStream(info->output_path, mime_type);
    if (stream.isNull())
    {
        // could not create set
        return false;
    }
    
    
    while (1)
    {
        data = (const unsigned char*)reader->loadNextBlock(&block_size);
        if (!data)
        {
            // done
            break;
        }

        unsigned long written = 0;
        stream->write(data, block_size, &written);

        if (isCancelling() || failed || written != block_size)
            break;
    }

    delete reader;

    if (failed)
    {
        return false;
    }

    if (isCancelling())
    {
        stream.clear();
        g_app->getDatabase()->deleteFile(info->output_path);
        return true;
    }

    return true;
}





struct PkgColumnInfo
{
    tango::objhandle_t handle;
    int type;
    int width;
    int scale;
    bool calculated;
};


// from xdnative/util.cpp
static double decstr2dbl(const char* c, int width, int scale)
{
    double res = 0;
    double d = kl::pow10(width-scale-1);
    bool neg = false;
    while (width)
    {
        if (*c == '-')
            neg = true;

        if (*c >= '0' && *c <= '9')
        {
            res += d * (*c - '0');
        }

        d /= 10;
        c++;
        width--;
    }

    if (neg)
    {
        res *= -1.0;
    }

    return res;
}

bool ImportPkgJob::importSet(PkgStreamReader* reader,
                             PkgImportInfo* info,
                             kl::xmlnode& stream_info)
{
    int node_idx;
    int phys_row_width;
    int rows_in_block;
    int block_size;
    int i;
    bool failed = false;
    const unsigned char* row_data;
    const unsigned char* col_data;
    const unsigned char* p;
    tango::IStructurePtr structure;

    node_idx = stream_info.getChildIdx(L"phys_row_width");
    if (node_idx == -1)
    {
        // could not find physical row width
        return false;
    }
    phys_row_width = kl::wtoi(stream_info.getChild(node_idx).getNodeValue());


    node_idx = stream_info.getChildIdx(L"structure");
    if (node_idx == -1)
    {
        // structure node couldn't be found
        return false;
    }

    kl::xmlnode& structure_node = stream_info.getChild(node_idx);

    structure = xml2structure(structure_node);
    if (structure.isNull())
    {
        // table structure could not be parsed from the xml
        return false;
    }

    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    if (!db->createTable(info->output_path, structure, NULL))
    {
        // could not create set
        return false;
    }


    tango::IRowInserterPtr sp_inserter = db->bulkInsert(towstr(info->output_path));
    tango::IRowInserter* inserter = sp_inserter.p;
    if (!inserter)
    {
        // couldn't get row inserter
        return false;
    }


    inserter->startInsert(L"*");


    std::vector<PkgColumnInfo> cols;
    int coln, col_count = structure->getColumnCount();
    for (coln = 0; coln < col_count; ++coln)
    {
        tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(coln);
        if (colinfo.isNull())
            return false;
        PkgColumnInfo col;
        col.handle = inserter->getHandle(colinfo->getName());
        if (!col.handle)
            return false;
        col.type = colinfo->getType();
        col.width = colinfo->getWidth();
        col.scale = colinfo->getScale();
        col.calculated = colinfo->getCalculated();
        cols.push_back(col);
    }
    
    std::vector<PkgColumnInfo>::iterator hit;
    std::vector<PkgColumnInfo>::iterator hit_end = cols.end();
    int real_width = 0;
    std::string str_data;
    std::wstring wstr_data;
    double dbl_data;
    tango::datetime_t dt_data;

    while (1)
    {
        row_data = (const unsigned char*)reader->loadNextBlock(&block_size);
        if (!row_data)
        {
            // done
            break;
        }

        rows_in_block = block_size / phys_row_width;
        for (i = 0; i < rows_in_block; ++i)
        {
            if (isCancelling())
                break;
            
            col_data = row_data;
            for (hit = cols.begin(); hit != hit_end; ++hit)
            {
                if (hit->calculated)
                    continue;
                
                switch (hit->type)
                {
                    case tango::typeCharacter:
                    {
                        real_width = hit->width;
                        // look for a zero terminator
                        p = (const unsigned char*)memchr(col_data, 0, real_width);
                        if (p)
                            real_width = p-col_data;
                        str_data.assign((char*)col_data, real_width);
                        inserter->putString(hit->handle, str_data);
                        col_data += hit->width;
                        break;
                    }
                    case tango::typeWideCharacter:
                        kl::ucsle2wstring(wstr_data, col_data, hit->width);
                        inserter->putWideString(hit->handle, wstr_data);
                        col_data += (hit->width*2);
                        break;
                    case tango::typeNumeric:
                        dbl_data = kl::dblround(
                                decstr2dbl((char*)col_data, hit->width, hit->scale),
                                hit->scale);
                        inserter->putDouble(hit->handle, dbl_data);
                        col_data += hit->width;
                        break;
                    case tango::typeDouble:
                        // little-endian only
                        inserter->putDouble(hit->handle, *(double*)col_data);
                        col_data += sizeof(double);
                        break;
                    case tango::typeInteger:
                        // little-endian only
                        inserter->putInteger(hit->handle, *(int*)col_data);
                        col_data += sizeof(int);
                        break;
                    case tango::typeBoolean:
                        inserter->putBoolean(hit->handle, *col_data == 'T' ? true : false);
                        col_data++;
                        break;
                    case tango::typeDate:
                        dt_data = buf2int(col_data);
                        dt_data <<= 32;
                        col_data += 4;
                        inserter->putDateTime(hit->handle, dt_data);
                        break;
                    case tango::typeDateTime:
                        dt_data = buf2int(col_data);
                        dt_data <<= 32;
                        dt_data |= ((tango::datetime_t)buf2int(col_data+4));
                        col_data += 8;
                        inserter->putDateTime(hit->handle, dt_data);
                        break;
                }
            }

            if (!inserter->insertRow())
            {
                m_job_info->setState(jobStateFailed);
                failed = true;
                break;
            }

            m_job_info->incrementCurrentCount(1.0);

            row_data += phys_row_width;
        }

        if (isCancelling() || failed)
            break;
    }

    inserter->finishInsert();


    if (failed)
    {
        return false;
    }

    delete reader;

    if (isCancelling())
    {
        return true;
    }

    // create a table model

    node_idx = stream_info.getChildIdx(L"tabledoc_model");
    if (node_idx != -1)
    {
        
        tango::IFileInfoPtr file_info = db->getFileInfo(towstr(info->output_path));
        if (file_info.isOk())
        {
            std::wstring set_id = file_info->getObjectId();
            if (set_id.length() > 0)
            {
                kl::xmlnode& node = stream_info.getChild(node_idx);

                ITableDocModelPtr model = TableDocMgr::loadModel(set_id);
                if (model.isOk())
                {
                    xml2tabledocmodel(node, model);
                }
            }
        }
    }


    return true;
}


bool ImportPkgJob::importOfsFile(PkgStreamReader* reader,
                                 PkgImportInfo* info,
                                 kl::xmlnode& stream_info)
{
    // disabled importing ofs files -- always return true
    return true;
}


bool ImportPkgJob::importMount(PkgStreamReader* reader,
                               PkgImportInfo* info,
                               kl::xmlnode& stream_info)
{
    tango::IDatabasePtr db = g_app->getDatabase();

    kl::xmlnode& data_node = stream_info.getChild(L"data");
    if (data_node.isEmpty())
        return false;
        
    std::wstring cstr = data_node.getChild(L"connection_string").getNodeValue();
    std::wstring rpath = data_node.getChild(L"remote_path").getNodeValue();
    
    if (kl::isEncryptedString(cstr))
        cstr = kl::decryptString(cstr, PASSWORD_KEY);
    if (kl::isEncryptedString(rpath))
        rpath = kl::decryptString(rpath, PASSWORD_KEY);
        
        
    db->deleteFile(towstr(info->output_path));
    db->setMountPoint(towstr(info->output_path), cstr, rpath);

    m_job_info->incrementCurrentCount(1.0);

    delete reader;
    return true;
}



int ImportPkgJob::runJob()
{
    std::wstring fn = kl::afterLast(m_filename, PATH_SEPARATOR_CHAR);
    std::wstring title = kl::stdswprintf(_("Importing from '%ls'").c_str(), fn.c_str());
    
    if (fn.length() > 0)
        m_job_info->setTitle(title);
         else
        m_job_info->setTitle(towstr(_("Importing from package file")));

    PkgFile pkg;
    if (!pkg.open(towstr(m_filename), PkgFile::modeRead))
    {
        // could not open package file
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    std::vector<PkgImportInfo>::iterator it;

    tango::IStructurePtr structure;
    int block_size;
    bool failed = false;
    
    double max_count = 0.0;

    // determine total row count
    PkgStreamEnum* pkg_enum = pkg.getStreamEnum();

    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        // change the job's title to reflect the new
        // table, report, script, etc. we're importing
        std::wstring tn = kl::afterLast(it->stream_name, '/');
        std::wstring title = kl::stdswprintf(_("Importing '%ls' from '%ls'").c_str(),
                                             tn.c_str(),
                                             fn.c_str());
        
        if (tn.empty() || fn.empty())
            m_job_info->setTitle(towstr(_("Importing from package file")));
             else
            m_job_info->setTitle(towstr(title));


        PkgStreamReader* reader = pkg.readStream(towstr(it->stream_name));
        if (!reader)
            continue;


        kl::xmlnode stream_info;

        if (pkg.getVersion() == 1)
        {
            const char* info;
            info = (const char*)reader->loadNextBlock(&block_size);
            if (!info || block_size == 0)
            {
                // could not read table info block
                delete reader;
                continue;
            }

            if (!stream_info.parse(info))
            {
                // could not parse table info xml document
                delete reader;
                continue;
            }
        }
         else
        {
            const unsigned char* info;
            info = (const unsigned char*)reader->loadNextBlock(&block_size);
            if (!info || block_size == 0)
            {
                // could not read table info block
                delete reader;
                continue;
            }

            std::wstring info_block_str;
            kl::ucsle2wstring(info_block_str, info, block_size/2);

            if (!stream_info.parse(info_block_str))
            {
                // could not parse table info xml document
                delete reader;
                continue;
            }
        }

        delete reader;


        std::wstring type = stream_info.getProperty(L"type").value;
        
        if (wcscasecmp(type.c_str(), L"set") == 0 ||
            type.empty())
        {
            xf_off_t phys_row_width = kl::wtoi(stream_info.getChild(L"phys_row_width").getNodeValue());
            if (phys_row_width == 0)
            {
                continue;
            }


            PkgDirEntry entry;
            if (!pkg_enum->getStreamInfo(towstr(it->stream_name), entry))
            {
                continue;
            }
            entry.uncompressed_size -= block_size;  // subtract info block size

            max_count += (entry.uncompressed_size/phys_row_width);
        }
         else if (wcscasecmp(type.c_str(), L"ofsfile") == 0)
        {
            // templates and other data objects increment the total by one
            max_count++;
        }
         else if (wcscasecmp(type.c_str(), L"mount") == 0)
        {
            // templates and other data objects increment the total by one
            max_count++;
        }
    }

    delete pkg_enum;

    m_job_info->setMaxCount(max_count);


    // actually import the sets
    
    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        PkgStreamReader* reader = pkg.readStream(towstr(it->stream_name));
        if (!reader)
            continue;

        kl::xmlnode stream_info;

        if (pkg.getVersion() == 1)
        {
            const char* info;
            info = (const char*)reader->loadNextBlock(&block_size);
            if (!info || block_size == 0)
            {
                // could not read table info block
                delete reader;
                continue;
            }

            if (!stream_info.parse(info))
            {
                // could not parse table info xml document
                delete reader;
                continue;
            }
        }
         else
        {
            const unsigned char* info;
            info = (const unsigned char*)reader->loadNextBlock(&block_size);
            if (!info || block_size == 0)
            {
                // could not read table info block
                delete reader;
                continue;
            }

            std::wstring info_block_str;
            kl::ucsle2wstring(info_block_str, info, block_size/2);

            if (!stream_info.parse(info_block_str))
            {
                // could not parse table info xml document
                delete reader;
                continue;
            }
        }

        kl::xmlnode& version_node = stream_info.getChild(L"version");
        if (version_node.getNodeValue() != L"1")
        {
            m_job_info->setState(jobStateFailed);
            delete reader;
            failed = true;
            break;
        }

        std::wstring type = stream_info.getProperty(L"type").value;
        
        if (wcscasecmp(type.c_str(), L"set") == 0 ||
            type.empty())
        {
            if (!importSet(reader, &(*it), stream_info))
            {
                m_job_info->setState(jobStateFailed);
                delete reader;
                failed = true;
                break;
            }
        }
         else if (wcscasecmp(type.c_str(), L"stream") == 0)
        {
            if (!importStream(reader, &(*it), stream_info))
            {
                m_job_info->setState(jobStateFailed);
                delete reader;
                failed = true;
                break;
            }
        }
         else if (wcscasecmp(type.c_str(), L"ofsfile") == 0)
        {
            if (!importOfsFile(reader, &(*it), stream_info))
            {
                m_job_info->setState(jobStateFailed);
                delete reader;
                failed = true;
                break;
            }
        }
         else if (wcscasecmp(type.c_str(), L"mount") == 0)
        {
            if (!importMount(reader, &(*it), stream_info))
            {
                m_job_info->setState(jobStateFailed);
                delete reader;
                failed = true;
                break;
            }
        }
         else
        {
            delete reader;
        }

        if (isCancelling())
        {
            break;
        }
    }

    return 0;
}


void ImportPkgJob::runPostJob()
{
    m_info.clear();

    if (m_job_info->getState() == jobStateFinished)
    {
        g_app->getAppController()->refreshDbDoc();
    }
}

