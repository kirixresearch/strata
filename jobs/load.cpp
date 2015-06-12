/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Job Automation Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-04-09
 *
 */


#include "jobspch.h"
#include "load.h"


namespace jobs
{

// example:

/*
{
    "metadata" : {
        "type" : "application/vnd.kx.load-job",
        "version" : 1,
        "description" : ""
    },
    "params": {

        "objects": [

            {
                "source_connection": "",
                "source_path": "",

                "destination_connection": "",
                "destination_path": ""
            }
        ]


    }
}
*/


// LoadJob implementation

LoadJob::LoadJob() : XdJobBase()
{
    m_config["metadata"]["type"] = L"application/vnd.kx.load-job";
    m_config["metadata"]["version"] = 1;
}

LoadJob::~LoadJob()
{
}

bool LoadJob::isInputValid()
{
    // TODO: fill out

    return true;
}


struct LoadObject
{
    std::wstring input;
    std::wstring input_connection;
    xd::IDatabasePtr input_db;
    xd::IFileInfoPtr input_fileinfo;
    xd::QueryParams query_params;

    std::wstring output;
    std::wstring output_connection;
    xd::IDatabasePtr output_db;
    xd::FormatDefinition output_format;

    bool binary_load;
    bool overwrite;
    bool add_xdrowid;
};



int LoadJob::runJob()
{
    // make sure we have a valid input
    if (!isInputValid())
    {
        // TODO: correlate input validation with job error information
    
        m_job_info->setState(jobStateFailed);
        m_job_info->setError(jobserrInvalidParameter, L"");
        return 0;
    }

    // get the parameters
    kl::JsonNode params_node;
    params_node.fromString(getParameters());


    xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
    if (dbmgr.isNull())
    {
        m_job_info->setState(jobStateFailed);
        return 0;
    }

    std::map<std::wstring, xd::IDatabasePtr> connection_pool;
    std::vector<LoadObject> load_objects;

    std::vector<kl::JsonNode> nodes;
    std::vector<kl::JsonNode>::iterator node_it;
    nodes.push_back(params_node);


    kl::JsonNode objects_node = params_node["objects"];
    size_t i, cnt = objects_node.getChildCount();
    for (i = 0; i < cnt; ++i)
    {
        nodes.push_back(objects_node[i]);
    }


    for (node_it = nodes.begin(); node_it != nodes.end(); ++node_it)
    {
        kl::JsonNode& object = *node_it;
        LoadObject lo;

        lo.input = object["input"];
        lo.input_connection = object["input_connection"];
        
        lo.output = object["output"];
        lo.output_connection = object["output_connection"];

        if (lo.input.empty())
            continue;

        if (lo.input_connection.empty())
        {
            lo.input_db = m_db;
        }
         else
        {
            lo.input_db = connection_pool[lo.input_connection];
            if (lo.input_db.isNull())
            {
                lo.input_db = dbmgr->open(lo.input_connection);
                connection_pool[lo.input_connection] = lo.input_db;
            }
        }

        if (lo.output_connection.empty())
        {
            lo.output_db = m_db;
        }
         else
        {
            lo.output_db = connection_pool[lo.output_connection];
            if (lo.output_db.isNull())
            {
                lo.output_db = dbmgr->open(lo.output_connection);
                connection_pool[lo.output_connection] = lo.output_db;
            }
        }

        if (lo.input_db.isNull() || lo.output_db.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }


        lo.input_fileinfo = lo.input_db->getFileInfo(lo.input);
        if (lo.input_fileinfo.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }


        // set up query params
        lo.query_params.from = lo.input;
        lo.binary_load = false; // by default, no binary load


        if (object.childExists("input_format"))
        {
            kl::JsonNode format_node = object["input_format"];

            std::wstring object_type = format_node.getChild("object_type").getString();
            std::wstring format = format_node.getChild("format").getString();

            if (object_type == L"stream")
            {
                lo.binary_load = true;
            }
             else
            {
                if (format == L"delimited_text")
                {
                    lo.query_params.format.format = xd::formatDelimitedText;
                    lo.query_params.format.delimiter = format_node.getChild("delimiter").getString();
                    lo.query_params.format.text_qualifier = format_node.getChild("text_qualifier").getString();
                    lo.query_params.format.header_row = format_node.getChild("header_row").getBoolean();

                    if (lo.query_params.columns.length() == 0)
                    {
                        // for csvs where we don't know the structure, perform a full scan to get correct metrics
                        lo.query_params.format.determine_structure = true; 
                    }
                }
                 else if (format == L"typed_delimited_text")
                {
                    lo.query_params.format.format = xd::formatTypedDelimitedText;
                }
                 else
                {
                    // unknown format
                    m_job_info->setState(jobStateFailed);
                    return 0;
                }

            }
        }
         else
        {
            std::wstring load_type;

            if (object.childExists("load_type"))
            {
                load_type = object["load_type"];
            }


            if (load_type == L"table")
            {
                lo.binary_load = false;
            }
             else if (load_type == L"binary")
            {
                lo.binary_load = true;
            }
             else
            {
                // if "load_type" is not specified, the object will import in the form of the
                // object type specified in "input";  i.e. if the database senses the input type
                // is a stream, it will import as a stream, otherwise it will import as a table
                lo.binary_load = (lo.input_fileinfo->getType() == xd::filetypeStream) ? true : false;
            }
        }





        if (object.childExists("output_format"))
        {
            kl::JsonNode format = object["output_format"];

            lo.output_format.format = xd::formatDelimitedText;
            if (kl::icontains(lo.output, L".icsv"))
                lo.output_format.format = xd::formatTypedDelimitedText;

            lo.output_format.delimiter = format.getChild("delimiter").getString();
            lo.output_format.text_qualifier = format.getChild("text_qualifier").getString();
            lo.output_format.header_row = format.getChild("header_row").getBoolean();
        }


        if (object.childExists("overwrite"))
        {
            lo.overwrite = object.getChild("overwrite").getBoolean();
        }
         else
        {
            lo.overwrite = true;   // default
        }


        if (object.childExists("add_xdrowid"))
        {
            lo.add_xdrowid = object.getChild("add_xdrowid").getBoolean();
        }
         else
        {
            lo.add_xdrowid = false; // default
        }

        load_objects.push_back(lo);
    }





    // perform the import

    std::vector<LoadObject>::iterator it;

    for (it = load_objects.begin(); it != load_objects.end(); ++it)
    {
        if (it->binary_load)
        {
            xd::IStreamPtr instream = it->input_db->openStream(it->input);
            if (instream.isNull())
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            if (!it->output_db->createStream(it->output, it->input_fileinfo->getMimeType()))
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            xd::IStreamPtr outstream = it->output_db->openStream(it->output);
            if (outstream.isNull())
            {
                m_job_info->setState(jobStateFailed);
                return 0;
            }

            unsigned char* buf = new unsigned char[65536];

            unsigned long read, written;
            bool done = false;
            bool error = false;
            while (!done)
            {
                if (!instream->read(buf, 65536, &read))
                    break;
                if (read != 65536)
                    done = true;
                if (read > 0)
                {
                    if (!outstream->write(buf, read, &written))
                        error = true;
                    if (!error && written != read)
                        error = true;
                    if (error)
                    {
                        // write error / disk space
                        delete[] buf;
                        instream.clear();
                        outstream.clear();
                        it->output_db->deleteFile(it->output);
                        m_job_info->setState(jobStateFailed);
                        return 0;
                    }
                }
            }

            delete[] buf;
            continue;
        }



        it->query_params.format.determine_structure = true;
        xd::IIteratorPtr source_iter = it->input_db->query(it->query_params);

        if (source_iter.isNull())
        {
            m_job_info->setState(jobStateFailed);
            return 0;
        }

        source_iter->goFirst();


        // create the destination table and copy the data

        xd::CopyParams info;
        info.iter_input = source_iter;
        info.output = it->output;
        info.append = true;



        if (it->overwrite)
        {
            it->output_db->deleteFile(it->output);
        }

        if (!it->output_db->getFileExist(it->output))
        {
            xd::FormatDefinition output_format = it->output_format;

            xd::Structure structure = source_iter->getStructure();


            // rename badly named columns to something that will work in all databases

            std::vector<xd::ColumnInfo>::iterator cit;
            for (cit = structure.begin(); cit != structure.end(); ++cit)
            {
                std::wstring orig_name = cit->name;

                if (cit->name.find('#') != cit->name.npos)
                    kl::replaceStr(cit->name, L"#", L"no");
                if (cit->name.find('%') != cit->name.npos)
                    kl::replaceStr(cit->name, L"%", L"pct");
                if (cit->name.find('-') != cit->name.npos)
                    kl::replaceStr(cit->name, L"-", L"_");
                if (cit->name.find('\t') != cit->name.npos)
                    kl::replaceStr(cit->name, L"\t", L" ");
                size_t i,len = cit->name.length();
                for (i = 0; i < len; ++i)
                {
                    if (!isalnum(cit->name[i]) && cit->name[i] != ' ')
                        cit->name[i] = '_';
                }

                info.addCopyColumn(orig_name, cit->name);
           }




            output_format.columns = structure.columns;
            output_format.columns.deleteColumn(L"xdrowid");

            if (it->add_xdrowid)
            {
                xd::ColumnInfo col;
                col.name = L"xdrowid";
                col.type = xd::typeBigSerial;
                col.width = 18;
                col.scale = 0;
                col.column_ordinal = 0;
                col.expression = L"";
                col.calculated = false;
                output_format.columns.insert(output_format.columns.begin(), col);
            }

            if (!it->output_db->createTable(it->output, output_format))
            {
                // could not create output file
                m_job_info->setState(jobStateFailed);
                return 0;
            }
        }




        






        xd::IJobPtr xd_job = it->output_db->createJob();
        setXdJob(xd_job);

        it->output_db->copyData(&info, xd_job);

        
        if (xd_job->getCancelled())
        {
            m_job_info->setState(jobStateCancelling);
            return 0;
        }

        if (xd_job->getStatus() == xd::jobFailed)
        {
            m_job_info->setState(jobStateFailed);

            // TODO: need to decide how to handle error strings; these need to 
            // be translated, so shouldn't be in this class
            //m_job_info->setProgressString(towstr(_("Modify failed: The table may be in use by another user.")));
        }
    }


    return 0;
}

void LoadJob::runPostJob()
{
}


};  // namespace jobs

