/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-12-01
 *
 */


#include "appmain.h"
#include "jobexportpkg.h"
#include "pkgfile.h"
#include "tabledoc.h"
#include <kl/crypt.h>


const int package_block_size = 1000000;
const int FILETYPE_MOUNT = 0x100000;

// password for scrambling password in templates and package files
#define PASSWORD_KEY L"83401732"


void structure2xml(tango::IStructurePtr s, kl::xmlnode& node)
{
    node.setNodeName(L"structure");

    tango::IColumnInfoPtr colinfo;
    int i, col_count;

    col_count = s->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        colinfo = s->getColumnInfoByIdx(i);

        kl::xmlnode& col = node.addChild();
        col.setNodeName(L"column");

        col.addChild(L"name", colinfo->getName());
        col.addChild(L"type", colinfo->getType());
        col.addChild(L"width", colinfo->getWidth());
        col.addChild(L"scale", colinfo->getScale());
        col.addChild(L"calculated", colinfo->getCalculated() ? 1 : 0);
        col.addChild(L"expression", colinfo->getExpression());
    }
}



tango::IStructurePtr xml2structure(kl::xmlnode& node)
{
    tango::IStructurePtr s = g_app->getDatabase()->createStructure();

    int child_count = node.getChildCount();
    int i;

    for (i = 0; i < child_count; ++i)
    {
        kl::xmlnode& col = node.getChild(i);
        if (col.getNodeName() != L"column")
        {
            continue;
        }

        kl::xmlnode& colname = col.getChild(L"name");
        kl::xmlnode& coltype = col.getChild(L"type");
        kl::xmlnode& colwidth = col.getChild(L"width");
        kl::xmlnode& colscale = col.getChild(L"scale");
        kl::xmlnode& colcalculated = col.getChild(L"calculated");
        kl::xmlnode& colexpression = col.getChild(L"expression");

        if (colname.isEmpty() ||
            coltype.isEmpty() ||
            colwidth.isEmpty() ||
            colscale.isEmpty() ||
            colcalculated.isEmpty() ||
            colexpression.isEmpty())
        {
            return xcm::null;
        }

        tango::IColumnInfoPtr colinfo = s->createColumn();
        colinfo->setName(colname.getNodeValue());
        colinfo->setType(kl::wtoi(coltype.getNodeValue()));
        colinfo->setWidth(kl::wtoi(colwidth.getNodeValue()));
        colinfo->setScale(kl::wtoi(colscale.getNodeValue()));
        colinfo->setCalculated(kl::wtoi(colcalculated.getNodeValue()) != 0 ? true : false);
        colinfo->setExpression(colexpression.getNodeValue());
    }


    return s;
}



void tabledocview2xml(ITableDocViewPtr view, kl::xmlnode& node)
{
    // set version
    kl::xmlnode& version_node = node.addChild();
    version_node.setNodeName(L"version");
    version_node.setNodeValue(1);

    int col_count = view->getColumnCount();

    kl::xmlnode& description_node = node.addChild();
    description_node.setNodeName(L"description");
    description_node.setNodeValue(towstr(view->getDescription()));

    kl::xmlnode& colcount_node = node.addChild();
    colcount_node.setNodeName(L"column_count");
    colcount_node.setNodeValue(col_count);

    kl::xmlnode& columns_node = node.addChild();
    columns_node.setNodeName(L"columns");

    ITableDocViewColPtr viewcol;
    for (int i = 0; i < col_count; ++i)
    {
        viewcol = view->getColumn(i);
    
        kl::xmlnode& col_node = columns_node.addChild();
        col_node.setNodeName(L"column");

        col_node.addChild(L"name", towstr(viewcol->getName()));
        col_node.addChild(L"size", viewcol->getSize());
        col_node.addChild(L"fgcolor", color2int(viewcol->getForegroundColor()));
        col_node.addChild(L"bgcolor", color2int(viewcol->getBackgroundColor()));
    }
}


bool xml2tabledocview(kl::xmlnode& node, ITableDocViewPtr view)
{
    view->deleteAllColumns();

    kl::xmlnode& version_node = node.getChild(L"version");
    if (version_node.isEmpty())
    {
        return false;
    }

    if (kl::wtoi(version_node.getNodeValue()) != 1)
    {
        // must be version 1
        return false;
    }

    kl::xmlnode& columns_node = node.getChild(L"columns");
    if (columns_node.isEmpty())
    {
        return false;
    }

    kl::xmlnode& description_node = node.getChild(L"description");
    if (!description_node.isEmpty())
    {
        view->setDescription(description_node.getNodeValue());
    }

    int child_count = columns_node.getChildCount();

    for (int i = 0; i < child_count; ++i)
    {
        kl::xmlnode& col_node = columns_node.getChild(i);
        if (col_node.isEmpty())
            continue;

        if (col_node.getNodeName() != L"column")
        {
            continue;
        }

        ITableDocViewColPtr viewcol = view->createColumn(-1);
        viewcol->setName(col_node.getChild(L"name").getNodeValue());
        viewcol->setSize(kl::wtoi(col_node.getChild(L"size").getNodeValue()));
        viewcol->setForegroundColor(int2color(kl::wtoi(col_node.getChild(L"fgcolor").getNodeValue())));
        viewcol->setBackgroundColor(int2color(kl::wtoi(col_node.getChild(L"bgcolor").getNodeValue())));
    }

    return true;
}





void tabledocmodel2xml(ITableDocModelPtr model, kl::xmlnode& root)
{   
    ITableDocViewEnumPtr views = model->getViewEnum();
    ITableDocMarkEnumPtr marks = model->getMarkEnum();

    ITableDocViewPtr view;
    ITableDocMarkPtr mark;

    int view_count = views->size();
    int mark_count = marks->size();

    int i;

    
    // save views from model
    kl::xmlnode& views_node = root.addChild();
    views_node.setNodeName(L"views");

    for (i = 0; i < view_count; ++i)
    {
        view = views->getItem(i);

        kl::xmlnode& view_node = views_node.addChild();
        view_node.setNodeName(L"view");

        tabledocview2xml(view, view_node);
    }


    // save marks from model

    kl::xmlnode& marks_node = root.addChild();
    marks_node.setNodeName(L"queries");

    for (i = 0; i < mark_count; ++i)
    {
        mark = marks->getItem(i);

        kl::xmlnode& mark_node = marks_node.addChild();
        mark_node.setNodeName(L"expr_query");

        kl::xmlnode& description_node = mark_node.addChild();
        description_node.setNodeName(L"description");
        description_node.setNodeValue(towstr(mark->getDescription()));

        kl::xmlnode& expression_node = mark_node.addChild();
        expression_node.setNodeName(L"expression");
        expression_node.setNodeValue(towstr(mark->getExpression()));

        kl::xmlnode& markactive_node = mark_node.addChild();
        markactive_node.setNodeName(L"mark_active");
        markactive_node.setNodeValue(mark->getMarkActive() ? true : false);

        kl::xmlnode& fgcolor_node = mark_node.addChild();
        fgcolor_node.setNodeName(L"fgcolor");
        fgcolor_node.setNodeValue(color2int(mark->getForegroundColor()));

        kl::xmlnode& bgcolor_node = mark_node.addChild();
        bgcolor_node.setNodeName(L"bgcolor");
        bgcolor_node.setNodeValue(color2int(mark->getBackgroundColor()));
    }
}


void xml2tabledocmodel(kl::xmlnode& node,
                       tango::ISetPtr output_set,
                       ITableDocModelPtr model)
{
    ITableDocObjectEnumPtr vec;
    vec = new xcm::IVectorImpl<ITableDocObjectPtr>;


    kl::xmlnode& views_node = node.getChild(L"views");
    if (!views_node.isEmpty())
    {
        int i, child_count = views_node.getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            kl::xmlnode& view_node = views_node.getChild(i);
            if (view_node.getNodeName() != L"view")
            {
                continue;
            }
            
            ITableDocViewPtr view = model->createViewObject();
            xml2tabledocview(view_node, view);
            vec->append(view);
        }
    }


    kl::xmlnode& queries_node = node.getChild(L"queries");
    if (!queries_node.isEmpty())
    {
        int i, child_count = queries_node.getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            kl::xmlnode& mark_node = queries_node.getChild(i);
            if (mark_node.getNodeName() != L"expr_query")
            {
                continue;
            }

            // create mark

            ITableDocMarkPtr mark = model->createMarkObject();
            mark->setDescription(mark_node.getChild(L"description").getNodeValue());
            mark->setExpression(mark_node.getChild(L"expression").getNodeValue());
            mark->setForegroundColor(int2color(kl::wtoi(mark_node.getChild(L"fgcolor").getNodeValue())));
            mark->setBackgroundColor(int2color(kl::wtoi(mark_node.getChild(L"bgcolor").getNodeValue())));
            mark->setMarkActive(kl::wtoi(mark_node.getChild(L"mark_active").getNodeValue()) != 0 ? true : false);
            vec->append(mark);
        }
    }


    model->writeMultipleObjects(vec);
}





// -- ExportPkgJob class implementation --


ExportPkgJob::ExportPkgJob()
{
    m_mode = modeOverwrite;
    m_version = -1;         // default version
    m_buf = NULL;
    m_raw = false;
    m_job_info->setTitle(towstr(_("Export to Package")));
}

ExportPkgJob::~ExportPkgJob()
{
}


void ExportPkgJob::setPkgFilename(const wxString& filename,
                                  int mode,
                                  int version)
{
    m_filename = filename;
    m_mode = mode;
    m_version = version;
}

void ExportPkgJob::addExportObject(const wxString& stream_name,
                                   const wxString& src_path,
                                   bool compressed)
{
    PkgStreamInfo info;
    info.src_path = src_path;
    info.stream_name = stream_name;
    info.compressed = compressed;

    m_info.push_back(info);
}

bool ExportPkgJob::writeStreamStream(tango::IDatabasePtr& db,
                                     PkgFile* pkg,
                                     PkgStreamInfo* info,
                                     bool* cancelled)
{
    tango::IFileInfoPtr finfo = db->getFileInfo(towstr(info->src_path));
    if (finfo.isNull())
        return false;
    
    tango::IStreamPtr stream = db->openStream(towstr(info->src_path));
    if (stream.isNull())
        return false;
    
    std::wstring stream_mimetype = finfo->getMimeType();

    // open package stream writer
    PkgStreamWriter* writer = NULL;
    std::wstring info_block_str;
    
    writer = pkg->createStream(towstr(info->stream_name));
    if (!writer)
        return false;

    // write out the table info block
    kl::xmlnode stream_info;

    stream_info.setNodeName(L"pkg_stream_info");
    stream_info.appendProperty(L"type", L"stream");
    stream_info.addChild(L"version", 1);

    kl::xmlnode& mime_type_node = stream_info.addChild();
    mime_type_node.setNodeName(L"mime_type");
    mime_type_node.setNodeValue(stream_mimetype);


    info_block_str = stream_info.getXML(kl::xmlnode::formattingSpaces |
                                       kl::xmlnode::formattingCrLf);

    writer->startWrite();

    if (pkg->getVersion() == 1)
    {
        writer->writeBlock(tostr(info_block_str).c_str(),
                           info_block_str.length()+1,
                           true);
    }
     else
    {
        // create a little-endian UCS-2 version of the info block xml
        int buf_len = (info_block_str.length()+1)*2;
        unsigned char* info_block = new unsigned char[buf_len];
        kl::wstring2ucsle(info_block, info_block_str, buf_len/2);
        writer->writeBlock(info_block, buf_len, true);
        delete[] info_block;
    }



    char* tempbuf = new char[4097];
    unsigned long read = 0;
    
    while (1)
    {
        if (!stream->read(tempbuf, 4096, &read))
            break;
                
        if (read > 0)
            writer->writeBlock(tempbuf, read, info->compressed);
        
        if (read != 4096)
            break;
    }
    
    delete[] tempbuf;


    writer->finishWrite();
    delete writer;

    return true;
}

bool ExportPkgJob::writeMountStream(tango::IDatabasePtr& db,
                                    PkgFile* pkg,
                                    PkgStreamInfo* info,
                                    bool* cancelled)
{
    kl::xmlnode root_node;
    root_node.setNodeName(L"pkg_stream_info");
    root_node.appendProperty(L"type", L"mount");
    root_node.addChild(L"version", 1);

    // add mount information to info xml
    std::wstring cstr, rpath;
    if (!db->getMountPoint(towstr(info->src_path), cstr, rpath))
        return false;
    
    kl::xmlnode& data_node = root_node.addChild(L"data");
    data_node.addChild(L"connection_string", kl::encryptString(cstr, PASSWORD_KEY));
    data_node.addChild(L"remote_path", kl::encryptString(rpath, PASSWORD_KEY));
    
    std::wstring xmlstr = root_node.getXML();

    unsigned char* buf = new unsigned char[(xmlstr.length()+1) * 2];
    kl::wstring2ucsle(buf, xmlstr, xmlstr.length()+1);

    PkgStreamWriter* writer;
    writer = pkg->createStream(towstr(info->stream_name));
    if (!writer)
    {
        delete[] buf;
        return false;
    }

    writer->startWrite();
    writer->writeBlock(buf, (xmlstr.length()+1)*2, true);
    writer->finishWrite();

    delete writer;

    delete[] buf;

    m_job_info->incrementCurrentCount(1.0);

    return true;
}



struct PkgSourceColumnInfo
{
    tango::objhandle_t handle;
    int type;
    int width;
    int scale;
};


// reipped from xdnative/util.cpp
static void dbl2decstr(char* dest, double d, int width, int scale)
{
    double intpart;

    // check for negative
    if (d < 0.0)
    {
        *dest = '-';
        dest++;
        width--;
        d = fabs(d);
    }

    // rounding
    d += (0.5/kl::pow10(scale));

    // put everything to the right of the decimal
    d /= kl::pow10(width-scale);

    while (width)
    {
        d *= 10;
        d = modf(d, &intpart);
        if (intpart > 9.1)
            intpart = 0.0;

        *dest = int(intpart) + '0';
        dest++;
        width--;
    }
}


static wxString getDbDriver(tango::IDatabasePtr& db)
{
    if (db.isNull())
        return wxEmptyString;
    
    xcm::class_info* class_info = xcm::get_class_info(db.p);
    return towx(class_info->get_name()).BeforeFirst('.');
}

bool ExportPkgJob::writeSetStream(tango::IDatabasePtr& db,
                                  PkgFile* pkg,
                                  PkgStreamInfo* info,
                                  bool* cancelled)
{
    tango::ISetPtr set = db->openSet(towstr(info->src_path));
    if (set.isNull())
        return false;

    tango::IStructurePtr structure;
    tango::IIteratorPtr sp_iter;
    tango::IIterator* iter;
    PkgStreamWriter* writer = NULL;
    std::wstring info_block_str;
    std::vector<PkgSourceColumnInfo> src_cols;
    std::vector<PkgSourceColumnInfo>::iterator hit;
    std::vector<PkgSourceColumnInfo>::iterator hit_end;
    unsigned char* src_buf = NULL;
    int total_phys_width = 0;
    int rows_per_buf = 0;
    int cur_bufrow;
    int col_count;
    int i;




    // get structure from set
    structure = set->getStructure();
    col_count = structure->getColumnCount();

    if (col_count < 1)
        return false;
    
    // calculate the total physical row width
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo;

        colinfo = structure->getColumnInfoByIdx(i);
        if (!colinfo->getCalculated())
        {
            int width = 0;
            
            switch (colinfo->getType())
            {
                case tango::typeCharacter:
                    width = colinfo->getWidth();
                    break;
                case tango::typeWideCharacter:
                    width = colinfo->getWidth() * 2;
                    break;
                case tango::typeNumeric:
                    width = colinfo->getWidth();
                    break;
                case tango::typeDouble:
                    width = sizeof(double);
                    break;
                case tango::typeInteger:
                    width = sizeof(int);
                    break;
                case tango::typeBoolean:
                    width = 1;
                    break;
                case tango::typeDate:
                    width = 4;
                    break;
                case tango::typeDateTime:
                    width = 8;
                    break;
            }

            total_phys_width += width;

            if (colinfo->getNullsAllowed())
                total_phys_width++;
        }
    }

    // calculate rows per buffer
    if (total_phys_width == 0)
        return false;

    rows_per_buf = package_block_size / total_phys_width;


    // create an iterator
    sp_iter = set->createIterator(L"", L"", NULL);
    if (!sp_iter)
        return false;
    iter = sp_iter.p;


    writer = pkg->createStream(towstr(info->stream_name));
    if (!writer)
        return false;

    // write out the table info block
    kl::xmlnode stream_info;

    stream_info.setNodeName(L"pkg_stream_info");
    stream_info.appendProperty(L"type", L"set");
    stream_info.addChild(L"version", 1);

    kl::xmlnode& phys_row_width_node = stream_info.addChild();
    phys_row_width_node.setNodeName(L"phys_row_width");
    phys_row_width_node.setNodeValue(total_phys_width);

    kl::xmlnode& structure_node = stream_info.addChild();
    structure_node.setNodeName(L"structure");
    structure2xml(structure, structure_node);

    kl::xmlnode& tabledoc_model_node = stream_info.addChild();
    tabledoc_model_node.setNodeName(L"tabledoc_model");

    // first load the set's model
    ITableDocModelPtr tabledoc_model;
    tabledoc_model = TableDocMgr::loadModel(set->getSetId());
    if (tabledoc_model.isOk())
    {
        tabledocmodel2xml(tabledoc_model, tabledoc_model_node);
    }

    info_block_str = stream_info.getXML(kl::xmlnode::formattingSpaces |
                                       kl::xmlnode::formattingCrLf);

    writer->startWrite();

    if (pkg->getVersion() == 1)
    {
        writer->writeBlock(tostr(info_block_str).c_str(),
                           info_block_str.length()+1,
                           true);
    }
     else
    {
        // create a little-endian UCS-2 version of the info block xml
        int buf_len = (info_block_str.length()+1)*2;
        unsigned char* info_block = new unsigned char[buf_len];
        kl::wstring2ucsle(info_block, info_block_str, buf_len/2);
        writer->writeBlock(info_block, buf_len, true);
        delete[] info_block;
    }
    
    
    // get handles to all columns -- this is used for
    // iterators that don't support getRowBuffer()
    
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);
        if (!colinfo->getCalculated())
        {
            PkgSourceColumnInfo info;
            info.handle = iter->getHandle(colinfo->getName());
            info.type = colinfo->getType();
            info.width = colinfo->getWidth();
            info.scale = colinfo->getScale();
            src_cols.push_back(info);
        }
    }
    hit_end = src_cols.end();
    src_buf = new unsigned char[total_phys_width];
    
    

    // iterator through all rows
    cur_bufrow = 0;


    unsigned char* dest_buf;
    
    tango::objhandle_t rowptr_handle = 0;
    
    
    // raw mode causes the entire row buffer to be copied, which
    // increases speed, but decreases compatibility with non-xdnative
    // database drivers; it is intended to be used for backups only
    
    if (m_raw && getDbDriver(db) == wxT("xdnative"))
    {
        rowptr_handle = src_cols[0].handle;
    }
    

    iter->goFirst();
    while (!iter->eof())
    {
        dest_buf = m_buf+(cur_bufrow*total_phys_width);
        

        if (rowptr_handle)
        {
            memcpy(dest_buf, iter->getRawPtr(rowptr_handle), total_phys_width);
        }
         else
        {
            int offset = 0;
            tango::datetime_t dt;
            double d;
            int i;
            
            memset(src_buf, 0, total_phys_width);
            
            // build buffer
            for (hit = src_cols.begin(); hit != hit_end; ++hit)
            {
                switch (hit->type)
                {
                    case tango::typeCharacter:
                    {
                        const std::string& str = iter->getString(hit->handle);
                        size_t len = str.length();
                        if (len > (size_t)hit->width)
                            len = hit->width;
                        memcpy(src_buf+offset, str.c_str(), len);
                        offset += hit->width;
                        break;
                    }
                    case tango::typeWideCharacter:
                        kl::wstring2ucsle(src_buf+offset, iter->getWideString(hit->handle), hit->width);
                        offset += (hit->width*2);
                        break;
                    case tango::typeNumeric:
                        dbl2decstr((char*)src_buf+offset, iter->getDouble(hit->handle), hit->width, hit->scale);
                        offset += hit->width;
                        break;
                    case tango::typeDouble:
                        // little-endian only
                        d = iter->getDouble(hit->handle);
                        memcpy(src_buf+offset, &d, sizeof(double));
                        offset += sizeof(double);
                        break;
                    case tango::typeInteger:
                        // little-endian only
                        i = iter->getInteger(hit->handle);
                        memcpy(src_buf+offset, &i, sizeof(int));
                        offset += sizeof(int);
                        break;
                    case tango::typeBoolean:
                        // little-endian only
                        *(src_buf+offset) = iter->getBoolean(hit->handle) ? 'T' : 'F';
                        offset++;
                        break;
                    case tango::typeDate:
                        dt = iter->getDateTime(hit->handle);
                        dt >>= 32;
                        int2buf(src_buf+offset, (unsigned int)dt);
                        offset += 4;
                        break;
                    case tango::typeDateTime:
                        dt = iter->getDateTime(hit->handle);
                        int2buf(src_buf+offset, (unsigned int)(dt >> 32));
                        int2buf(src_buf+offset+4, (unsigned int)(dt & 0xffffffff));
                        offset += 8;
                        break;
                }
            }
            
            memcpy(dest_buf,
               src_buf,
               total_phys_width);
        }
        

        cur_bufrow++;
        if (cur_bufrow == rows_per_buf)
        {
            writer->writeBlock(m_buf,
                               cur_bufrow*total_phys_width,
                               info->compressed);

            cur_bufrow = 0;
        }

        if (isCancelling())
        {
            *cancelled = true;
            break;
        }

        m_job_info->incrementCurrentCount(1.0);

        iter->skip(1);
    }

    // flush remaining rows
    if (cur_bufrow > 0)
    {
        writer->writeBlock(m_buf, cur_bufrow*total_phys_width, true);
        cur_bufrow = 0;
    }

    writer->finishWrite();


    // release handles
    for (hit = src_cols.begin(); hit != hit_end; ++hit)
        iter->releaseHandle(hit->handle);
    delete[] src_buf;
    
    
    delete writer;
    writer = NULL;

    return true;
}


static void addFolder(std::vector<PkgStreamInfo>& info, tango::IDatabasePtr& db, const std::wstring& path, const std::wstring& destination_base, bool compressed)
{
    tango::IFileInfoEnumPtr files = db->getFolderInfo(towstr(path));
    if (files.isOk())
    {
        size_t i, count = files->size();
        for (i = 0; i < count; ++i)
        {
            tango::IFileInfoPtr f = files->getItem(i);
            
            std::wstring full_path = path;
            if (full_path.length() == 0 || full_path[0] != '/')
                full_path.insert(0, L"/");
            if (full_path[full_path.length() - 1] != '/')
                full_path += L"/";
            full_path += f->getName();
            
            if (f->getType() == tango::filetypeFolder && !f->isMount())
            {
                addFolder(info, db, full_path, destination_base, compressed);
            }
             else
            {
                std::wstring stream_name = destination_base;
                if (stream_name.length() > 0 && stream_name[stream_name.length() - 1] != '/')
                    stream_name += L"/";
                stream_name += f->getName();
                
                PkgStreamInfo i;
                i.compressed = compressed;
                i.src_path = full_path;
                i.stream_name = stream_name;
                info.push_back(i);
            }
        }
    }
}

int ExportPkgJob::runJob()
{
    PkgFile pkg;

    if (m_mode == ExportPkgJob::modeOverwrite)
    {
        if (!pkg.create(towstr(m_filename), m_version))
        {
            // could not create package file
            return 0;
        }
    }
     else
    {
        if (!pkg.open(towstr(m_filename), PkgFile::modeReadWrite))
        {
            // could not open package file for writing
            return 0;
        }

        m_version = pkg.getVersion();
    }

    // default version is 2
    if (m_version == -1)
        m_version = 2;
        
    if (m_version != 2)
    {
        // we don't handle older or newer versions
        return 0;
    }

    tango::IDatabasePtr db = g_app->getDatabase();
    m_buf = new unsigned char[package_block_size];
    std::vector<PkgStreamInfo>::iterator it;
    double max_count;


    std::vector<PkgStreamInfo> info;
    for (it = m_info.begin(); it != m_info.end(); ++it)
    {
        tango::IFileInfoPtr finfo = db->getFileInfo(towstr(it->src_path));

        if (finfo.isNull())
        {
            getJobInfo()->setState(jobStateFailed);
            return 0;
        }

        if ((finfo->getType() == tango::filetypeFolder) && !finfo->isMount())
        {
            addFolder(info, db, towstr(it->src_path), towstr(it->stream_name), it->compressed);
        }
         else
        {
            info.push_back(*it);
        }
    }
    


    // calculate max row count
    m_obj_mutex.lock();
    max_count = 0.0;
    m_obj_mutex.unlock();

    for (it = info.begin(); it != info.end(); ++it)
    {
        // find out of the file exists
        tango::IFileInfoPtr file_info = db->getFileInfo(towstr(it->src_path));
        if (file_info.isNull())
        {
            getJobInfo()->setState(jobStateFailed);
            return 0;
        }

        it->file_type = file_info->getType();

        if (file_info->isMount())
        {
            it->file_type = FILETYPE_MOUNT;
            max_count++;
            continue;
        }
            
        if (file_info->getType() == tango::filetypeSet)
        {
            // attempt to open set
            tango::ISetPtr set = db->openSet(towstr(it->src_path));

            if (set.isNull())
            {
                getJobInfo()->setState(jobStateFailed);
                return 0;
            }

            if (set->getSetFlags() & tango::sfFastRowCount)
            {
                m_obj_mutex.lock();
                max_count += (tango::tango_int64_t)set->getRowCount();
                m_obj_mutex.unlock();
            }
        }
         else if (file_info->getType() == tango::filetypeStream)
        {
            tango::IStreamPtr stream = db->openStream(towstr(it->src_path));
            
            if (stream.isNull())
            {
                getJobInfo()->setState(jobStateFailed);
                return 0;
            }    
        }
         else
        {
            max_count++;
        }
    }


    m_job_info->setMaxCount(max_count);

    bool cancelled = false;
    bool compressed;

    for (it = info.begin(); it != info.end(); ++it)
    {
        compressed = it->compressed;

        if (it->file_type == tango::filetypeStream)
        {
            if (!writeStreamStream(db,
                                   &pkg,
                                   &(*it),
                                   &cancelled))
            {
                m_job_info->setState(jobStateFailed);
                cancelled = true;
                break;
            }
        }
         else if (it->file_type == tango::filetypeSet)
        {
            if (!writeSetStream(db,
                                &pkg,
                                &(*it),
                                &cancelled))
            {
                m_job_info->setState(jobStateFailed);
                cancelled = true;
                break;
            }
        }
         else if (it->file_type == FILETYPE_MOUNT)
        {
            if (!writeMountStream(db,
                                  &pkg,
                                  &(*it),
                                  &cancelled))
            {
                m_job_info->setState(jobStateFailed);
                cancelled = true;
                break;
            }
        }
         else
        {
            // no longer allow exporting of old node type files; instead
            // they should be saved in new format and exported as a stream
        }

        
        if (cancelled)
        {
            break;
        }
    }

    delete[] m_buf;
    m_buf = NULL;

    return 0;
}

void ExportPkgJob::runPostJob()
{
}