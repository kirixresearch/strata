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
#include "tabledoc.h"

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




static void writeKpgMetadata(jobs::IJobPtr job)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    IConnectionPtr conn = createUnmanagedConnection();
    conn->setType(dbtypePackage);
    conn->setPath(job->getExtraValue(L"kpg"));
    conn->open();
    
    xd::IDatabasePtr kpg = conn->getDatabasePtr();
    if (kpg.isNull())
        return;

    kl::JsonNode params;
    params.fromString(job->getParameters());

    size_t i;
    for (i = 0; i < params["objects"].getChildCount(); ++i)
    {
        kl::JsonNode object = params["objects"][i];
        
        std::wstring source_path = object["input"];
        std::wstring destination_path = object["output"];

        if (source_path.empty() || destination_path.empty())
            continue;

        // get object id of the source table we exported
        xd::IFileInfoPtr finfo = db->getFileInfo(source_path);
        if (finfo.isNull())
            continue;
        std::wstring object_id = finfo->getObjectId();
        
        // load the json from the model
        ITableDocModelPtr model = TableDocMgr::loadModel(object_id);
        if (model.isNull())
            continue;
        std::wstring json = model->toJson();

        // write resource to kpg
        while (destination_path.substr(0, 1) == L"/")
            destination_path = destination_path.substr(1);
        destination_path = L".resource/" + destination_path;

        writeStreamTextFile(kpg, destination_path, json);
    }
}

static void onExportJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;

    if (job->getExtraValue(L"kpg").length() > 0)
        writeKpgMetadata(job);
}


jobs::IJobPtr ExportTemplate::createJob()
{
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.load-job");

    job->getJobInfo()->setTitle(towstr(_("Exporting Data")));

    if (m_ei.type == dbtypePackage)
    {
        job->setExtraValue(L"kpg", m_ei.path);
    }

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


    std::wstring source_connection = g_app->getDatabaseConnectionString();
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

        object["input_connection"] = source_connection;
        object["output_connection"] = destination_connection;

        object["input"] = it->input_tablename;
        object["output"] = it->output_tablename;

        object["overwrite"].setBoolean(!it->append);

        if (m_ei.type == dbtypeDelimitedText)
        {
            object["output_format"].setObject();
            kl::JsonNode format = object["output_format"];
            
            format["object_type"] = L"table";
            format["format"] = "delimited_text";
            format["delimiter"] = m_ei.delimiters;
            format["text_qualifier"] = m_ei.text_qualifier;
            format["header_row"].setBoolean(m_ei.first_row_header);
        }

        //job_export_info.append = it->append;
    }

    job->setParameters(params.toString());
    job->sigJobFinished().connect(&onExportJobFinished);
    return job;
}



jobs::IJobPtr ExportTemplate::execute()
{
    jobs::IJobPtr job = createJob();

    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);

    return job;
}

