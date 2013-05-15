/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2013-05-08
 *
 */


#include "appmain.h"
#include "exportwizard.h"
#include "exporttemplate.h"


// ExportTemplate class implementation

ExportTemplate::ExportTemplate()
{

}

bool ExportTemplate::load(const wxString& path)
{
    return false;
}

bool ExportTemplate::save(const wxString& path)
{
    return false;
}

jobs::IJobPtr ExportTemplate::execute()
{
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.load-job");

    job->getJobInfo()->setTitle(towstr(_("Exporting Data")));

    /*
    ExportJob* job = new ExportJob;
    job->setExportType(m_ei.type);
    job->setFilename(m_ei.path, m_ei.overwrite_file);
    job->setFixInvalidFieldnames(m_ei.fix_invalid_fieldnames);
    job->setConnectionInfo(m_ei.server,
                           m_ei.port,
                           m_ei.database,
                           m_ei.username,
                           m_ei.password);

    job->setDelimiters(towstr(m_ei.delimiters));
    job->setTextQualifier(towstr(m_ei.text_qualifier));
    job->setFirstRowHeader(m_ei.first_row_header);
    */


    if (m_ei.path.length() > 0 && m_ei.overwrite_file)
    {
        xf_remove(m_ei.path);
    }


    // determine destination connection string
    IConnectionPtr conn = createUnmanagedConnection();
    conn->setType(m_ei.type);
    conn->setHost(m_ei.server);
    conn->setPort(m_ei.port);
    conn->setDatabase(m_ei.database);
    conn->setUsername(m_ei.username);
    conn->setPassword(m_ei.password);

    conn->setPath(m_ei.path);




    std::wstring source_connection = towstr(g_app->getDatabaseConnectionString());
    std::wstring destination_connection = conn->getConnectionString();
    destination_connection += L";create_if_not_exists=true";

    // configure the job parameters
    kl::JsonNode params;

    params["objects"].setArray();
    kl::JsonNode objects = params["objects"];


    std::vector<ExportTableSelection>::iterator it;
    for (it = m_ei.tables.begin(); it != m_ei.tables.end(); ++it)
    {
        kl::JsonNode object = objects.appendElement();

        object["source_connection"] = source_connection;
        object["destination_connection"] = destination_connection;

        object["source_path"] = it->input_tablename;
        object["destination_path"] = it->output_tablename;

        object["overwrite"].setBoolean(true);

        if (m_ei.type == dbtypeDelimitedText)
        {
            object["destination_format"].setObject();
            kl::JsonNode format = object["destination_format"];
            
            format["delimiter"] = m_ei.delimiters;
            format["text_qualifier"] = m_ei.text_qualifier;
            format["header_row"].setBoolean(m_ei.first_row_header);
        }

        //job_export_info.append = it->append;
    }

    job->setParameters(params.toString());
    g_app->getJobQueue()->addJob(job, jobStateRunning);

    return job;
}


