/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-04-17
 *
 */


#ifndef H_APP_PANELRELATIONSHIP_H
#define H_APP_PANELRELATIONSHIP_H


// this class watches for files that are being renamed and updates
// the internal relationship diagram, whether the panel is open or not

class RelDiagramWatcher : public xcm::signal_sink
{
public:
    
    RelDiagramWatcher();
    void onFrameEvent(FrameworkEvent& evt);
};



xcm_interface IRelationshipPanel : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IRelationshipPanel")

public:

    virtual void onSetRenamed(const wxString& old_path,
                              const wxString& new_path) = 0;
    virtual void onSetStructureChanged(const wxString& set_path) = 0;
};


XCM_DECLARE_SMARTPTR(IRelationshipPanel)


class RelationBox;
class RelationLine;
class RelationDiagram;
class RelationLineDataObject;
class RelationshipPanel : public wxPanel,
                          public IDocument,
                          public IRelationshipPanel,
                          public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.RelationshipPanel")
    XCM_BEGIN_INTERFACE_MAP(RelationshipPanel)
        XCM_INTERFACE_ENTRY(IDocument)
        XCM_INTERFACE_ENTRY(IRelationshipPanel)
    XCM_END_INTERFACE_MAP()

public:

    RelationshipPanel();
    ~RelationshipPanel();

private:

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    bool onSiteClosing(bool force);

    // -- IRelationshipPanel --
    void onSetRenamed(const wxString& old_path,
                      const wxString& new_path);
    void onSetStructureChanged(const wxString& set_path);

    void loadRelationships();
    bool checkCircular();
    void validateUpdateButton();

    void onLineAdded(RelationLine* line, bool* allowed);
    void onDiagramUpdated();
    
private:
    
    void onClose(wxCommandEvent& evt);
    void onAddTable(wxCommandEvent& evt);
    void onUpdateRelationships(wxCommandEvent& evt);
    void onRelationshipJobFinished(jobs::IJobPtr job);

private:

    IDocumentSitePtr m_doc_site;
    RelationDiagram* m_diagram;
    wxButton* m_add_button;
    wxButton* m_update_button;
    bool m_changed;
    bool m_do_save;

    DECLARE_EVENT_TABLE()
};


#endif  // __APP_PANELRELATIONSHIP_H




