/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-09
 *
 */


#ifndef __APP_PANELEXPRBUILDERDOC_H
#define __APP_PANELEXPRBUILDERDOC_H


#include "exprbuilder.h"



template <class BaseT>
class BuilderDocPanel :  public BaseT,
                         public IDocument
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ExprBuilderDocPanel")
    XCM_BEGIN_INTERFACE_MAP(BuilderDocPanel<BaseT>)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    BuilderDocPanel(const wxString& caption = _("Formula Builder"))
    {
        m_caption = caption;
    }

    ~BuilderDocPanel()
    {
    }

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site)
    {
        m_doc_site = site;

        if (!this->Create(doc_site,
                          -1,
                          wxDefaultPosition,
                          wxDefaultSize,
                          wxNO_FULL_REPAINT_ON_RESIZE |
                          wxCLIP_CHILDREN |
                          wxTAB_TRAVERSAL))
        {
            return false;
        }

        this->construct();

        this->sigCancelPressed.connect(this,
                                 &BuilderDocPanel<BaseT>::onCancelPressed);

        site->setCaption(m_caption);

        // -- set the minimum size of the site --
        m_doc_site->setMinSize(300, 300);

        return true;
    }

    wxWindow* getDocumentWindow()
    {
        return static_cast<wxWindow*>(this);
    }

    void setDocumentFocus()
    {
    }


private:

    void onCancelPressed(BaseT*)
    {
        g_app->getMainFrame()->closeSite(m_doc_site);
    }

private:

    wxString m_caption;
    IDocumentSitePtr m_doc_site;
};



typedef BuilderDocPanel<ExprBuilderPanel> ExprBuilderDocPanel;
typedef BuilderDocPanel<KeyBuilderPanel> KeyBuilderDocPanel;


#endif

