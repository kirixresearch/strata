/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-04-17
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "panelrelationship.h"
#include "relationdiagram.h"
#include "tabledoc.h"
#include "util.h"
#include <set>


enum
{
    ID_AddTable = 10000,
    ID_DeleteAllRelationships,
    ID_UpdateRelationships,
};




// -- RelDiagramWatcher class implementation --

RelDiagramWatcher::RelDiagramWatcher()
{
    g_app->getMainFrame()->sigFrameEvent().connect(this, &RelDiagramWatcher::onFrameEvent);
}

void RelDiagramWatcher::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_TREEPANEL_OFS_FILE_RENAMED)
    {
        RelationDiagram::renameTablePath(evt.s_param, evt.s_param2);

        IDocumentSitePtr site;
        site = g_app->getMainFrame()->lookupSite(wxT("RelationshipsPanel"));
        if (site.isOk())
        {
            IRelationshipPanelPtr panel = site->getDocument();
            panel->onSetRenamed(evt.s_param, evt.s_param2);
        }
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_TABLE_STRUCTURE_MODIFIED)
    {
        IDocumentSitePtr site;
        site = g_app->getMainFrame()->lookupSite(wxT("RelationshipsPanel"));
        if (site.isOk())
        {
            IRelationshipPanelPtr panel = site->getDocument();
            panel->onSetStructureChanged(evt.s_param);
        }
    }
     else if (evt.name == FRAMEWORK_EVT_TABLEDOC_STRUCTURE_MODIFIED)
    {
        IDocumentSitePtr site;
        site = g_app->getMainFrame()->lookupSite(wxT("RelationshipsPanel"));
        if (site.isOk())
        {
            IRelationshipPanelPtr panel = site->getDocument();
            panel->onSetStructureChanged(evt.s_param);
        }
    }
}




// -- RelationshipPanel class implementation --

BEGIN_EVENT_TABLE(RelationshipPanel, wxPanel)
    EVT_BUTTON(wxID_CANCEL, RelationshipPanel::onClose)
    EVT_BUTTON(ID_AddTable, RelationshipPanel::onAddTable)
    EVT_BUTTON(ID_DeleteAllRelationships, RelationshipPanel::onDeleteAllRelationships)
    EVT_BUTTON(ID_UpdateRelationships, RelationshipPanel::onUpdateRelationships)
END_EVENT_TABLE()


RelationshipPanel::RelationshipPanel()
{
    m_add_button = NULL;
    m_delete_button = NULL;
    m_update_button = NULL;
    m_diagram = NULL;
    m_changed = false;
    m_do_save = true;
}

RelationshipPanel::~RelationshipPanel()
{
    // save our diagram

    if (m_diagram && m_do_save)
        m_diagram->save();


    // save the relationship panel location
    
    if (m_doc_site)
    {
        // if the whole package is closing, prefs and parent pointers will
        // be null at this point.  Only save the preference panel location
        // if the user closes it while the package is not shutting down
        IAppPreferencesPtr prefs = g_app->getAppPreferences();
        if (prefs.isNull())
            return;
        wxWindow* parent = GetParent();
        if (!parent)
            return;
            
        int x, y, width, height;
        
        // sadly we can't use m_doc_site->getPosition() here
        // because the container is already being destructed -- which
        // would amount to a pure virtual call
        parent->GetPosition(&x, &y);
        g_app->getMainWindow()->ClientToScreen(&x, &y);
        parent->GetSize(&width, &height);

        prefs->setLong("window_dimensions.relationship_mgr.x", x);
        prefs->setLong("window_dimensions.relationship_mgr.y", y);
        prefs->setLong("window_dimensions.relationship_mgr.width", width);
        prefs->setLong("window_dimensions.relationship_mgr.height", height);
        prefs->flush();
    }
}


// -- IDocument methods --

bool RelationshipPanel::initDoc(IFramePtr frame,
                                IDocumentSitePtr site,
                                wxWindow* docsite_wnd,
                                wxWindow* panesite_wnd)
{
    if (!Create(docsite_wnd, -1,
                wxPoint(0,0),
                docsite_wnd->GetClientSize(),
                wxNO_FULL_REPAINT_ON_RESIZE |
                wxCLIP_CHILDREN))
    {
        return false;
    }

    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);
    SetBackgroundColour(kcl::getBaseColor());
    
    // save our site
    m_doc_site = site;

    // set site's caption
    site->setCaption(_("Relationships"));

    // create button sizer
    m_add_button = new wxButton(this, ID_AddTable, _("Add Table..."));
    m_delete_button = new wxButton(this, ID_DeleteAllRelationships, _("Delete All Relationships"));
    m_update_button = new wxButton(this, ID_UpdateRelationships, _("Update Relationships"));
    m_update_button->Enable(false);
    m_delete_button->Enable(false);
    
    // this button is created so that the ESC key can be used to
    // close the panel -- it is completely hidden to the user
    wxButton* close_button;
    close_button = new wxButton(this, wxID_CANCEL, wxEmptyString, wxDefaultPosition, wxSize(0,0));

    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddSpacer(FromDIP(5));
    button_sizer->Add(m_add_button, 0, wxALIGN_CENTER);
    // moved: stretch first, then close, then delete+update on right
    button_sizer->AddStretchSpacer();
    button_sizer->Add(close_button, 0, wxALIGN_CENTER);
    button_sizer->Add(m_delete_button, 0, wxALIGN_CENTER);
    button_sizer->AddSpacer(FromDIP(5));
    button_sizer->Add(m_update_button, 0, wxALIGN_CENTER);
    button_sizer->AddSpacer(FromDIP(5));
    button_sizer->SetMinSize(FromDIP(100), FromDIP(31));

    m_diagram = new RelationDiagram(this,
                                    -1,
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    true,
                                    true);

    m_diagram->sigLineAdded.connect(this, &RelationshipPanel::onLineAdded);
    m_diagram->sigDiagramUpdated.connect(this, &RelationshipPanel::onDiagramUpdated);

    m_diagram->load();
    loadRelationships();
    validateUpdateButton();

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(button_sizer, 0, wxEXPAND);
    main_sizer->Add(m_diagram, 1, wxEXPAND);
    SetSizer(main_sizer);
    SetAutoLayout(true);
    Layout();

    return true;
}

wxWindow* RelationshipPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void RelationshipPanel::setDocumentFocus()
{
    if (m_diagram)
        m_diagram->SetFocusIgnoringChildren();
}

void RelationshipPanel::onSetRenamed(const wxString& old_path,
                                     const wxString& new_path)
{
    m_diagram->onSetRenamed(old_path, new_path);
}

void RelationshipPanel::onSetStructureChanged(const wxString& set_path)
{
    m_diagram->onSetStructureChanged(set_path);
}

bool RelationshipPanel::onSiteClosing(bool force)
{
    if (force)
        return true;

    // we're up-to-date, so we can exit
    if (!m_changed)
        return true;
        
    int res = wxCANCEL;
    
    std::vector<RelationLine*> lines;
    m_diagram->getLines(wxEmptyString, wxEmptyString, lines);
    
    if (m_diagram->getBoxCount() > 0 && lines.size() > 0)
    {
        res = appMessageBox(_("Would you like to save your changes and activate the specified relationships?"),
                                 APPLICATION_NAME,
                                 wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);
    }
     else
    {
        res = appMessageBox(_("Would you like to save the changes made to the relationships?"),
                                 APPLICATION_NAME,
                                 wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);
    }
    
    if (res == wxYES)
    {
        wxCommandEvent cmd;
        onUpdateRelationships(cmd);
        return true;
    }
     else if (res == wxNO)
    {
        m_do_save = false;
        return true;
    }
     else if (res == wxCANCEL)
    {
        setDocumentFocus();
        return false;
    }

    return true;
}

void RelationshipPanel::validateUpdateButton()
{
    bool active = true;

    if (m_diagram->getBoxCount() == 0)
        active = false;

    m_update_button->Enable(active);

    // enable Delete All only if there are any lines
    if (m_delete_button)
    {
        std::vector<RelationLine*> lines;
        m_diagram->getLines(wxEmptyString, wxEmptyString, lines);
        m_delete_button->Enable(!lines.empty());
    }
}

void RelationshipPanel::onDiagramUpdated()
{
    validateUpdateButton();

    m_changed = true;
}




static bool hasCircular(RelationDiagram* diagram,
                        const wxString& set_path,
                        std::set<wxString>& paths)
{
    // add ourself to the 'been-there' path list
    wxString s = set_path;
    s.MakeUpper();
    if (paths.find(s) != paths.end())
        return true;
    paths.insert(s);

    // get the relationships for this set
    std::vector<RelationInfo> info;
    std::vector<RelationInfo>::iterator it;
    diagram->getRelationInfo(set_path, info);

    for (it = info.begin(); it != info.end(); ++it)
    {
        if (hasCircular(diagram, it->right_path, paths))
            return true;
    }

    return false;
}

bool RelationshipPanel::checkCircular()
{
    // returns 'true' if circular relationships
    // are found, 'false' otherwise

    std::vector<wxString> box_paths;
    std::vector<wxString>::iterator it;

    m_diagram->getBoxPaths(box_paths);

    for (it = box_paths.begin(); it != box_paths.end(); ++it)
    {
        std::set<wxString> paths;
        if (hasCircular(m_diagram, *it, paths))
        {
            return true;
        }
    }

    return false;
}

void RelationshipPanel::onLineAdded(RelationLine* line, bool* allowed)
{
    if (checkCircular())
    {
        appMessageBox(_("Circular relationships cannot be created."),
                           APPLICATION_NAME,
                           wxOK | wxCENTRE | wxICON_EXCLAMATION);

        *allowed = false;
        return;
    }

    std::vector<RelationLine*> lines;
    m_diagram->getLines(line->left_path,
                        line->right_path,
                        lines);

    if (lines.size() == 1)
    {
        // first line added
        line->left_bitmap = GETBMPSMALL(gf_ebbc13);
        line->right_bitmap = GETBMPSMALL(gf_ebbc14);
    }
     else
    {
        line->user = lines[0]->user;
        line->tag = lines[0]->tag;
        line->left_bitmap = lines[0]->left_bitmap;
        line->right_bitmap = lines[0]->right_bitmap;
    }
}

void RelationshipPanel::onAddTable(wxCommandEvent& evt)
{
    showAddTableDialog(m_diagram);
}

void RelationshipPanel::onDeleteAllRelationships(wxCommandEvent& evt)
{
    // confirmation prompt
    int res = appMessageBox(
        _("Are you sure you want to delete all relationships?\nThis action cannot be undone."),
        APPLICATION_NAME,
        wxYES_NO | wxICON_QUESTION | wxCENTER);

    if (res != wxYES)
        return;

    // remove all relationship lines (leave boxes)
    xd::IRelationSchemaPtr rels = g_app->getDatabase();
    if (rels.isOk())
    {
        xd::IRelationEnumPtr rel_enum = rels->getRelationEnum(L""); // get all relationships
        xd::IRelationPtr rel;
        size_t r, rel_count = rel_enum->size();

        std::vector<std::wstring> rel_ids;

        for (r = 0; r < rel_count; ++r)
        {
            rel = rel_enum->getItem(r);
            if (rel.isOk())
                rel_ids.push_back(rel->getRelationId());
        }

        for (r = 0; r < rel_ids.size(); ++r)
        {
            rels->deleteRelation(rel_ids[r]);
        }
    }


    m_diagram->deleteAllLines();
    m_diagram->refreshBoxes();
    m_diagram->refresh();
    m_changed = false;
    validateUpdateButton();
}

void RelationshipPanel::onClose(wxCommandEvent& evt)
{
    // we have to check the class info here to find out if we pressed the
    // ESC key, in which case the event object will be a button, since we
    // have a close button with the wxID_CANCEL id, or if we pressed the
    // close button in the panel's titlebar area, in which case we've
    // already handled the close action in onSiteClosing()
    if (evt.GetEventObject()->IsKindOf(CLASSINFO(wxButton)))
        g_app->getMainFrame()->closeSite(m_doc_site);
}




struct UpdateRel
{
    wxString tag;
    wxString left_path;    
    wxString right_path;
    wxString left_expr;
    wxString right_expr;
};

struct UpdateIdx
{
    wxString set_path;
    wxString idx_name;
    wxString idx_expr;
};

struct UpdateInfo
{
    std::vector<UpdateIdx> indexes;
    std::vector<UpdateRel> relations;
};


static void onRelationshipJobFinishedStatic(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;

    g_app->getMainFrame()->postEvent(new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_RELATIONSHIPS_UPDATED));

    // update the relationship syncing (if it was enabled)
    int synctype = g_app->getAppController()->getRelationshipSync();
    if (synctype != tabledocRelationshipSyncNone)
        g_app->getAppController()->updateTableDocRelationshipSync(synctype);
}

static xd::IRelationPtr lookupSetRelation(xd::IDatabasePtr& db, const std::wstring& table_path, const std::wstring& tag)
{
    xd::IRelationSchemaPtr rels = db;
    if (rels.isNull())
        return xcm::null;

    xd::IRelationEnumPtr rel_enum = rels->getRelationEnum(table_path);
    size_t i, n = rel_enum->size();
    for (i = 0; i < n; ++i)
    {
        xd::IRelationPtr rel = rel_enum->getItem(i);

        if (0 == wcscasecmp(rel->getTag().c_str(), tag.c_str()))
            return rel;
    }

    return xcm::null;
}

void RelationshipPanel::onUpdateRelationships(wxCommandEvent& evt)
{
    // remove relationship syncing for the duration
    // of the relationship setup operation
    if (g_app->getAppController()->getRelationshipSync() != tabledocRelationshipSyncNone)
        g_app->getAppController()->updateTableDocRelationshipSync(tabledocRelationshipSyncNone);

    // update the relationships
    xd::IDatabasePtr db = g_app->getDatabase();
    if (!db)
        return;

    xd::IRelationSchemaPtr rels = db;
    if (rels.isNull())
        return;


    UpdateInfo info;

    std::set<wxString> sets;
    std::set<wxString>::iterator it;

    std::vector<RelationLine*> orig_lines;
    std::vector<RelationLine*> new_lines;
    std::vector<RelationLine*>::iterator l_it;

    m_diagram->getOrigLines(wxEmptyString, wxEmptyString, orig_lines);
    m_diagram->getLines(wxEmptyString, wxEmptyString, new_lines);

    for (l_it = orig_lines.begin();
         l_it != orig_lines.end(); ++l_it)
    {
        wxString parent_path = (*l_it)->left_path;
        parent_path.MakeUpper();
        sets.insert(parent_path);
    }

    for (l_it = new_lines.begin();
         l_it != new_lines.end(); ++l_it)
    {
        wxString parent_path = (*l_it)->left_path;
        parent_path.MakeUpper();
        sets.insert(parent_path);
    }

    for (it = sets.begin(); it != sets.end(); ++it)
    {
        std::vector<RelationInfo> old_info;
        std::vector<RelationInfo> new_info;
        std::vector<RelationInfo>::iterator oi_it;
        std::vector<RelationInfo>::iterator ni_it;
        
        m_diagram->getOrigRelationInfo(*it, old_info);
        m_diagram->getRelationInfo(*it, new_info);

        if (!isValidTable(towstr(*it)))
            continue;

        // look for relationships that were deleted
        
        for (oi_it = old_info.begin(); oi_it != old_info.end(); ++oi_it)
        {
            bool found = false;

            for (ni_it = new_info.begin(); ni_it != new_info.end(); ++ni_it)
            {
                if (isSamePath(oi_it->left_path.ToStdWstring(), ni_it->left_path.ToStdWstring()) &&
                    isSamePath(oi_it->right_path.ToStdWstring(), ni_it->right_path.ToStdWstring()))
                {
                    // found it, is it different?
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                xd::IRelationPtr rel = lookupSetRelation(db, towstr(*it), towstr(oi_it->tag));
                if (rel)
                    rels->deleteRelation(rel->getRelationId());
            }
        }

        // create the relationships
        
        if (new_info.size() > 0)
        {
            // delete all existing relationships for this table

            xd::IRelationEnumPtr rel_enum = rels->getRelationEnum(towstr(*it));
            size_t i, rel_count = rel_enum->size();
            for (i = 0; i < rel_count; ++i)
                rels->deleteRelation(rel_enum->getItem(i)->getRelationId());
        }

        for (ni_it = new_info.begin(); ni_it != new_info.end(); ++ni_it)
        {
            wxString left_str;
            wxString right_str;

            std::vector<RelationLine>::iterator line_it;
            for (line_it = ni_it->lines.begin();
                 line_it != ni_it->lines.end(); ++line_it)
            {
                if (!left_str.IsEmpty())
                    left_str += wxT(",");

                if (!right_str.IsEmpty())
                    right_str += wxT(",");

                left_str += line_it->left_expr;
                right_str += line_it->right_expr;
            }

            UpdateRel r;
            r.tag = ni_it->tag;            
            r.left_path = *it;
            r.left_expr = left_str;
            r.right_path = ni_it->right_path;            
            r.right_expr = right_str;
            info.relations.push_back(r);
        }
    }


    // now start the job
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.relationship-job");

    kl::JsonNode params;
    kl::JsonNode relationships = params["relationships"];
    relationships.setArray();


    // add relationship information
    std::vector<UpdateRel>::iterator it_rel;
    for (it_rel = info.relations.begin();
            it_rel != info.relations.end(); ++it_rel)
    {
        kl::JsonNode relationship_item = relationships.appendElement();

        relationship_item["name"].setString(towstr(it_rel->tag));
        relationship_item["left_path"].setString(towstr(it_rel->left_path));
        relationship_item["left_expression"].setString(towstr(it_rel->left_expr));
        relationship_item["right_path"].setString(towstr(it_rel->right_path));
        relationship_item["right_expression"].setString(towstr(it_rel->right_expr));
    }

    job->getJobInfo()->setTitle(towstr(_("Creating Relationships")));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(&onRelationshipJobFinishedStatic);
    job->sigJobFinished().connect(this, &RelationshipPanel::onRelationshipJobFinished);
    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);


    m_diagram->resetModified();
    m_diagram->save();
    
    m_update_button->Enable(false);
    m_changed = false;
}

void RelationshipPanel::onRelationshipJobFinished(jobs::IJobPtr job)
{
    loadRelationships();
    m_update_button->Enable(false);
    m_changed = false;
}

void RelationshipPanel::loadRelationships()
{
    m_diagram->deleteAllLines();

    xd::IDatabasePtr db = g_app->getDatabase();

    xd::IRelationSchemaPtr rels = db;
    if (rels.isNull())
        return;

    xd::IRelationEnumPtr rel_enum = rels->getRelationEnum(L"");

    bool update_button = false;

    int i, rel_count = rel_enum->size();
    for (i = 0; i < rel_count; ++i)
    {
        xd::IRelationPtr rel = rel_enum->getItem(i);

        std::vector<wxString> left_parts;
        std::vector<wxString> right_parts;

        wxStringTokenizer left(rel->getLeftExpression(), ",");
        while (left.HasMoreTokens())
            left_parts.push_back(left.GetNextToken());

        wxStringTokenizer right(rel->getRightExpression(), ",");
        while (right.HasMoreTokens())
            right_parts.push_back(right.GetNextToken());

        if (left_parts.size() == right_parts.size())
        {
            // if there was no index for the right set
            // expression, make the lines red

            bool valid = true;
            std::wstring right_table_path = rel->getRightTable();

            if (right_table_path.length() > 0)
            {
                std::wstring cstr, rpath;
                bool relationship_on_mount = getMountPointHelper(db, right_table_path, cstr, rpath);

                if (!relationship_on_mount)
                {
                    xd::IndexInfoEnum right_table_indexes = db->getIndexEnum(right_table_path);
                    xd::IndexInfo idx = lookupIndex(right_table_indexes, rel->getRightExpression(), true);

                    if (!idx.isOk())
                    {
                        valid = false;
                        update_button = true;
                    }
                }
            }

            size_t j, count = left_parts.size();
            for (j = 0; j < count; ++j)
            {
                RelationLine* line;
                
                line = m_diagram->addLine(rel->getLeftTable(),
                                          left_parts[j],
                                          rel->getRightTable(),
                                          right_parts[j]);
                if (line)
                {
                    line->valid = valid;
                    line->tag = rel->getTag();
                }
            }
        }
    }

    m_diagram->resetModified();
    m_diagram->refreshBoxes();
    m_diagram->refresh();

    if (update_button)
        validateUpdateButton();
    else
        validateUpdateButton();

    m_changed = false;
}

