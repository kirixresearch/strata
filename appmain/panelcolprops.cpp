/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-02-14
 *
 */


#include "appmain.h"
#include "panelcolprops.h"
#include "xdgridmodel.h"
#include "exprbuilder.h"
#include "appcontroller.h"


enum
{
    ID_ColName = 10000,
    ID_ColType,
    ID_ColWidth,
    ID_ColScale
};

BEGIN_EVENT_TABLE(ColPropsPanel, wxPanel)
    EVT_TEXT(ID_ColName, ColPropsPanel::onNameChanged)
    EVT_COMBOBOX(ID_ColType, ColPropsPanel::onTypeChanged)
    EVT_TEXT(ID_ColWidth, ColPropsPanel::onWidthChanged)
    EVT_TEXT(ID_ColScale, ColPropsPanel::onScaleChanged)
    EVT_SPINCTRL(ID_ColWidth, ColPropsPanel::onWidthSpinChanged)
    EVT_SPINCTRL(ID_ColScale, ColPropsPanel::onScaleSpinChanged)
    EVT_BUTTON(wxID_CANCEL, ColPropsPanel::onCancel)
END_EVENT_TABLE()


static int dlgtype2xdtype(int dlg_type)
{
    switch (dlg_type)
    {
        case 1: return xd::typeCharacter;
        case 2: return xd::typeWideCharacter;
        case 3: return xd::typeNumeric;
        case 4: return xd::typeDouble;
        case 5: return xd::typeInteger;
        case 6: return xd::typeDate;
        case 7: return xd::typeDateTime;
        case 8: return xd::typeBoolean;
    }

    return xd::typeInvalid;
}

static int xdtype2dlgtype(int xd_type)
{
    switch (xd_type)
    {
        case xd::typeUndefined:     return 0;
        case xd::typeInvalid:       return 0;
        case xd::typeCharacter:     return 1;
        case xd::typeWideCharacter: return 2;
        case xd::typeNumeric:       return 3;
        case xd::typeDouble:        return 4;
        case xd::typeInteger:       return 5;
        case xd::typeDate:          return 6;
        case xd::typeDateTime:      return 7;
        case xd::typeBoolean:       return 8;
    }

    return 0;
}




ColPropsPanel::ColPropsPanel()
{
    m_saved_character_width = 30;
    m_saved_numeric_width = 18;
    m_saved_numeric_scale = 2;

    m_orig_name = L"";
    m_orig_type = -1;
    m_orig_width = -1;
    m_orig_scale = -1;
    m_orig_existed = false;

    m_ok_pressed = false;
}

ColPropsPanel::~ColPropsPanel()
{
}


bool ColPropsPanel::initDoc(IFramePtr frame,
                            IDocumentSitePtr site,
                            wxWindow* docsite_wnd,
                            wxWindow* panesite_wnd)
{
    if (!Create(docsite_wnd, 
                -1,
                wxDefaultPosition,
                wxDefaultSize,
                wxCLIP_CHILDREN))
    {
        return false;
    }

    m_doc_site = site;
    m_doc_site->setCaption(_("Calculated Field"));

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    wxString cbChoices[9];

    cbChoices[0] = _("(Auto)");
    cbChoices[1] = _("Character");
    cbChoices[2] = _("Wide Character");
    cbChoices[3] = _("Numeric");
    cbChoices[4] = _("Double");
    cbChoices[5] = _("Integer");
    cbChoices[6] = _("Date");
    cbChoices[7] = _("DateTime");
    cbChoices[8] = _("Boolean");

    wxStaticText* static_name = new wxStaticText(this, -1, _("Name:"));
    wxStaticText* static_type = new wxStaticText(this, -1, _("Type:"));
    wxStaticText* static_width = new wxStaticText(this, -1, _("Width:"));
    wxStaticText* static_scale = new wxStaticText(this, -1, _("Decimals:"));

    m_colname_text = NULL;
    m_coltype_combo = NULL;
    m_colwidth_text = NULL;
    m_colscale_text = NULL;

    m_colname_text = new wxTextCtrl(this, ID_ColName);
    
    m_coltype_combo = new wxComboBox(this,
                                     ID_ColType,
                                     _("Character"),
                                     wxDefaultPosition,
                                     FromDIP(wxSize(110, 14)),
                                     9,
                                     cbChoices,
                                     wxCB_READONLY);
                                     
    m_colwidth_text = new wxSpinCtrl(this,
                                     ID_ColWidth,
                                     "0",
                                     wxDefaultPosition,
                                     FromDIP(wxSize(60, 14)),
                                     wxSP_ARROW_KEYS,
                                     1,
                                     65535,
                                     20);
                                     
    m_colscale_text = new wxSpinCtrl(this,
                                     ID_ColScale,
                                     "0",
                                     wxDefaultPosition,
                                     FromDIP(wxSize(60, 14)),
                                     wxSP_ARROW_KEYS,
                                     0,
                                     xd::max_numeric_scale);

    wxBoxSizer* colparam_sizer = new wxBoxSizer(wxHORIZONTAL);

    colparam_sizer->Add(static_name, 0, wxALIGN_CENTER);
    colparam_sizer->AddSpacer(FromDIP(4));
    colparam_sizer->Add(m_colname_text, 1, wxEXPAND);
    colparam_sizer->AddSpacer(FromDIP(8));
    colparam_sizer->Add(static_type, 0, wxALIGN_CENTER);
    colparam_sizer->AddSpacer(FromDIP(4));
    colparam_sizer->Add(m_coltype_combo, 0, wxEXPAND);
    colparam_sizer->AddSpacer(FromDIP(8));
    colparam_sizer->Add(static_width, 0, wxALIGN_CENTER);
    colparam_sizer->AddSpacer(FromDIP(4));
    colparam_sizer->Add(m_colwidth_text, 0, wxEXPAND);
    colparam_sizer->AddSpacer(FromDIP(8));
    colparam_sizer->Add(static_scale, 0, wxALIGN_CENTER);
    colparam_sizer->AddSpacer(FromDIP(4));
    colparam_sizer->Add(m_colscale_text, 0, wxEXPAND);

    wxStaticLine* div_line = new wxStaticLine(this,
                                              -1,
                                              wxDefaultPosition,
                                              wxDefaultSize,
                                              wxLI_HORIZONTAL);

    m_expr_panel = new ExprBuilderPanel(this);
    m_expr_panel->sigExpressionChanged.connect(this, &ColPropsPanel::onExpressionChanged);
    m_expr_panel->sigOkPressed.connect(this, &ColPropsPanel::onOkPressed);
    m_expr_panel->sigCancelPressed.connect(this, &ColPropsPanel::onCancelPressed);

    main_sizer->AddSpacer(FromDIP(4));
    main_sizer->Add(colparam_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8));
    main_sizer->AddSpacer(FromDIP(4));
    main_sizer->Add(div_line, 0, wxEXPAND);
    main_sizer->Add(m_expr_panel, 1, wxEXPAND);

    SetSizer(main_sizer);

    // add frame event handlers
    frame->sigSiteClose().connect(this, &ColPropsPanel::onSiteClose);
    frame->sigFrameEvent().connect(this, &ColPropsPanel::onFrameEvent);


    // keep track of which document we are working on

    m_iter = xcm::null;
    m_structure = xd::Structure();
    m_tabledoc = xcm::null;

    m_tabledoc_site = g_app->getMainFrame()->getActiveChild();

    if (!m_tabledoc_site)
        return false;

    m_tabledoc = m_tabledoc_site->getDocument();

    if (!m_tabledoc)
        return false;

    m_path = m_tabledoc->getPath();

    xd::IFileInfoPtr finfo = g_app->getDatabase()->getFileInfo(m_path);
    m_structure = g_app->getDatabase()->describeTable(m_path);
    if (finfo.isNull() || m_structure.isNull())
    {
        m_doc_site = xcm::null;
        m_tabledoc = xcm::null;
        return false;
    }


    // add fields from child file(s)

    xd::IRelationSchemaPtr rels = g_app->getDatabase();
    if (rels.isOk())
    {
        xd::IRelationEnumPtr rel_enum = rels->getRelationEnum(m_path);
        xd::IRelationPtr rel;
        size_t r, rel_count = rel_enum->size();

        wxString s;

        for (r = 0; r < rel_count; ++r)
        {
            rel = rel_enum->getItem(r);

            if (rel.isNull())
                continue;

            std::wstring right_path = rel->getRightTable();

            xd::Structure right_structure = g_app->getDatabase()->describeTable(right_path);
            if (right_structure.isNull())
                continue;

            size_t i, col_count = right_structure.getColumnCount();

            for (i = 0; i < col_count; ++i)
            {
                xd::ColumnInfo colinfo = right_structure.getColumnInfoByIdx(i);
                colinfo.name = wxString::Format("%s.%s", rel->getTag().c_str(), colinfo.name.c_str());
                m_structure.columns.push_back(colinfo);
            }
        }
    }


    
    // make panel caption
    wxString caption = _("Calculated Field");


    if (!xd::isTemporaryPath(m_path))
    {
        caption += " - [";
        caption += m_path;
        caption += "]";
    }

    m_doc_site->setCaption(caption);
    
    
    m_iter = m_tabledoc->getIterator();
    kcl::Grid* grid = m_tabledoc->getGrid();

    m_expr_panel->setStructure(m_structure);

    // populate
    populate();

    return true;
}

wxWindow* ColPropsPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ColPropsPanel::setDocumentFocus()
{
}

void ColPropsPanel::closeSite(bool ok)
{
    m_ok_pressed = ok;
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void ColPropsPanel::onSiteClose(IDocumentSitePtr site)
{
    if (site == m_tabledoc_site)
    {
        m_tabledoc.clear();
        m_tabledoc_site.clear();
        closeSite(false);
    }
}

void ColPropsPanel::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_TABLEDOC_ON_SITE_CLOSING)
    {
        if (m_tabledoc_site.isOk())
        {
            ITableDocPtr t = m_tabledoc_site->getDocument();
            if (t.isOk() && evt.o_param == (void*)t.p)
            {
                // if the table doc we are working on is closing,
                // we need to cancel our edit

                revertChanges();
            }
        }
    }
}



void ColPropsPanel::setModifyField(const wxString& new_val)
{
    m_modify_field = new_val;
}

wxString ColPropsPanel::getModifyField()
{
    return m_modify_field;
}



void ColPropsPanel::populate()
{
    xd::ColumnInfo colinfo = m_iter->getInfo(m_iter->getHandle(towstr(m_modify_field)));

    if (colinfo.isNull())
        return;

    if (!colinfo.calculated)
        return;


    m_orig_name = colinfo.name;
    m_orig_type = colinfo.type;
    m_orig_width = colinfo.width;
    m_orig_scale = colinfo.scale;
    m_orig_expr = colinfo.expression;

    xd::Structure structure = g_app->getDatabase()->describeTable(m_path);
    m_orig_existed = structure.getColumnExist(towstr(m_orig_name));


    if (m_orig_type == xd::typeCharacter || m_orig_type == xd::typeWideCharacter)
    {
        m_saved_character_width = m_orig_width;
    }
     else if (m_orig_type == xd::typeNumeric || m_orig_type == xd::typeDouble)
    {
        m_saved_numeric_width = m_orig_width;
        m_saved_numeric_scale = m_orig_scale;
    }


    setName(m_orig_name);
    if (m_orig_type == xd::typeDateTime)
    {
        setType(xd::typeDateTime);
    }
     else
    {
        setType(xd::typeUndefined); // auto type
    }

    setWidth(colinfo.width);
    setScale(colinfo.scale);

    // avoid "" as an empty expression
    wxString expr = colinfo.expression;

    if (expr == "\"\"")
    {
        setExpression("");
        m_orig_expr = "";
    }
     else
    {
        setExpression(colinfo.expression);
    }

    updateSpinBoxes();
}



void ColPropsPanel::setName(const wxString& name)
{
    m_last_name = name;
    m_colname_text->SetValue(name);
}

void ColPropsPanel::setType(int type)
{
    m_last_type = type;
    m_coltype_combo->SetSelection(xdtype2dlgtype(type));
}

void ColPropsPanel::setWidth(int width)
{
    m_last_width = width;
    m_colwidth_text->SetValue(width);
}

void ColPropsPanel::setScale(int scale)
{
    m_last_scale = scale;
    m_colscale_text->SetValue(scale);
}

void ColPropsPanel::setExpression(const wxString& expr)
{
    m_last_expr = expr;
    m_expr_panel->setExpression(expr);
}

void ColPropsPanel::onNameChanged(wxCommandEvent& evt)
{
    if (!m_tabledoc)
        return;

    if (!m_colname_text)
        return;

    wxString name = m_colname_text->GetValue();
    name.Trim(false);
    name.Trim(true);

    if (name.Length() == 0)
        return;

    // we cannot set the name to an existing column
    kcl::Grid* grid = m_tabledoc->getGrid();
    kcl::IModelPtr model = grid->getModel();
    int model_idx = model->getColumnIndex(name);
    if (model_idx != -1)
        return;


    // we cannot set the name to an invalid value
    if (!isValidFieldName(name))
    {
        return;
    }


    if (name != m_last_name)
    {
        m_last_name = name;
        changeColumnCaption(name);
        grid->refresh(kcl::Grid::refreshPaint);
    }
}

void ColPropsPanel::onTypeChanged(wxCommandEvent& evt)
{
    if (!m_tabledoc)
        return;

    if (!m_coltype_combo)
        return;

    int combo_selection = m_coltype_combo->GetSelection();
    long type = dlgtype2xdtype(m_coltype_combo->GetSelection());

    if (combo_selection == 0)
    {
        // auto type; tell expression builder to allow all types
        m_expr_panel->setTypeOnly(xd::typeUndefined);
        m_expr_panel->validate();

        m_last_expr = wxT("");
        onExpressionChanged(NULL);
        return;
    }
     else
    {
        m_expr_panel->setTypeOnly(type);
    }

    if (type != m_last_type)
    {
        m_last_type = type;


        xd::ColumnInfo colinfo;
        
        colinfo.mask |= xd::ColumnInfo::maskType;
        colinfo.type = type;

        if (type == xd::typeCharacter ||
            type == xd::typeWideCharacter ||
            type == xd::typeDate ||
            type == xd::typeDateTime ||
            type == xd::typeInteger ||
            type == xd::typeBoolean)
        {
            colinfo.mask |= xd::ColumnInfo::maskScale;
            colinfo.scale = 0;
        }


        // limit numeric width, if necessary
        if (type == xd::typeNumeric && m_last_width > xd::max_numeric_width)
        {
            colinfo.mask |= xd::ColumnInfo::maskWidth;
            colinfo.width = xd::max_numeric_width;
            setWidth(xd::max_numeric_width);
        }

        xd::StructureModify mod_params;
        mod_params.modifyColumn(towstr(m_modify_field), colinfo);

        if (m_iter->modifyStructure(mod_params, NULL))
        {
            kcl::Grid* grid = m_tabledoc->getGrid();
            grid->refreshModel();
            refreshDynamicFields();
        }

        updateSpinBoxes();
    }
}


void ColPropsPanel::onWidthChanged(wxCommandEvent& evt)
{
    if (!m_tabledoc)
        return;

    if (!m_colwidth_text)
        return;

    // calling wxSpinCtrl::GetValue() in gtk causes an infinite loop and a
    // call stack overflow, because this function causes this same event to
    // be fired again.  This should be fixed in wxWidgets
    
    //int width = m_colwidth_text->GetValue();
    int width = evt.GetInt();

    if (width <= 0)
    {
        width = 1;
    }

    if (m_last_type == xd::typeNumeric)
    {
        m_saved_numeric_width = width;
    }

    if (m_last_type == xd::typeCharacter)
    {
        m_saved_character_width = width;
    }

    // check for maximum allowed numeric width
    if (m_last_type == xd::typeNumeric ||
        m_expr_panel->getExpressionType() == xd::typeNumeric)
    {
        if (width > xd::max_numeric_width)
        {
            m_saved_numeric_width = width;
            setWidth(xd::max_numeric_width);
            return;
        }
    }

    if (width != m_last_width)
    {
        m_last_width = width;

        xd::ColumnInfo colinfo;
        colinfo.mask |= xd::ColumnInfo::maskWidth;
        colinfo.width = width;

        xd::StructureModify mod_params;
        mod_params.modifyColumn(towstr(m_modify_field), colinfo);

        if (m_iter->modifyStructure(mod_params, NULL))
        {
            kcl::Grid* grid = m_tabledoc->getGrid();
            grid->refreshModel();

            refreshDynamicFields();
        }
    }
}

void ColPropsPanel::onScaleChanged(wxCommandEvent& evt)
{
    if (!m_tabledoc)
        return;

    if (!m_colscale_text)
        return;

    // calling wxSpinCtrl::GetValue() in gtk causes an infinite loop and a
    // call stack overflow, because this function causes this same event
    // to be fired again.  This should be fixed in wxWidgets
    
    //int scale = m_colscale_text->GetValue();
    int scale = evt.GetInt();
    if (scale < 0)
        scale = 0;

    if (m_last_type == xd::typeNumeric ||
        m_last_type == xd::typeDouble)
    {
        m_saved_numeric_scale = scale;
    }

    if (scale > xd::max_numeric_scale)
    {
        scale = xd::max_numeric_scale;
        m_saved_numeric_scale = scale;
        setScale(scale);
        return;
    }

    if (scale != m_last_scale)
    {
        m_last_scale = scale;

        xd::ColumnInfo colinfo;
        colinfo.mask |= xd::ColumnInfo::maskScale;
        colinfo.scale = scale;

        xd::StructureModify mod_params;
        mod_params.modifyColumn(towstr(m_modify_field), colinfo);

        if (m_iter->modifyStructure(mod_params, NULL))
        {
            kcl::Grid* grid = m_tabledoc->getGrid();
            grid->refreshModel();

            refreshDynamicFields();
        }
    }
}

void ColPropsPanel::onWidthSpinChanged(wxSpinEvent& evt)
{
    // gtk only event (see event map above)
    onWidthChanged(evt);
}

void ColPropsPanel::onScaleSpinChanged(wxSpinEvent& evt)
{
    // gtk only event (see event map above)
    onScaleChanged(evt);
}

void ColPropsPanel::onExpressionChanged(ExprBuilderPanel*)
{
    if (!m_tabledoc)
        return;

    wxString expr = m_expr_panel->getExpression();

    bool auto_type = (m_coltype_combo->GetSelection() == 0 ? true : false);

    if (expr != m_last_expr && m_expr_panel->getExpressionType() != xd::typeInvalid)
    {
        m_last_expr = expr;

        // update the calcfield in the grid

        xd::ColumnInfo colinfo;
        colinfo.mask |= xd::ColumnInfo::maskExpression;
        colinfo.expression = towstr(expr);
            
        bool type_changed = false;
        if (auto_type)
        {
            int cur_type = m_expr_panel->getExpressionType();

            if (cur_type == xd::typeDateTime)
            {
                cur_type = xd::typeDate;
            }

            if (cur_type != m_last_type)
            {
                int last_type = m_last_type;

                m_last_type = cur_type;

                colinfo.mask |= xd::ColumnInfo::maskType;
                colinfo.type = cur_type;

                type_changed = true;

                // limit numeric width, if necessary
                if (cur_type == xd::typeNumeric && m_last_width > xd::max_numeric_width)
                {
                    colinfo.mask |= xd::ColumnInfo::maskWidth;
                    colinfo.width = xd::max_numeric_width;
                    setWidth(xd::max_numeric_width);
                }

                if ((cur_type == xd::typeNumeric ||
                        cur_type == xd::typeDouble) &&
                    (last_type == xd::typeUndefined ||
                        last_type == xd::typeCharacter ||
                        last_type == xd::typeWideCharacter ||
                        last_type == xd::typeBoolean ||
                        last_type == xd::typeDate ||
                        last_type == xd::typeDateTime ||
                        last_type == xd::typeDouble ||
                        last_type == xd::typeInteger))
                {
                    colinfo.mask |= xd::ColumnInfo::maskScale;
                    colinfo.scale = m_saved_numeric_scale;
                    setScale(m_saved_numeric_scale);
                }

                // if the last type was some fixed-width type
                // and the new type is numeric, update the width

                if ((cur_type == xd::typeCharacter ||
                        cur_type == xd::typeWideCharacter) &&
                    (last_type == xd::typeBoolean ||
                        last_type == xd::typeDate ||
                        last_type == xd::typeDateTime ||
                        last_type == xd::typeDouble ||
                        last_type == xd::typeInteger))
                {
                    colinfo.mask |= xd::ColumnInfo::maskWidth;
                    colinfo.width = m_saved_character_width;
                    setWidth(m_saved_character_width);
                }

                updateSpinBoxes();
            }
        }

        xd::StructureModify mod_params;
        mod_params.modifyColumn(towstr(m_modify_field), colinfo);

        if (m_iter->modifyStructure(mod_params, NULL))
        {
            kcl::Grid* grid = m_tabledoc->getGrid();

            if (type_changed)
            {
                grid->refreshModel();
            }

            refreshDynamicFields();
        }
    }
}



void ColPropsPanel::changeColumnCaption(const wxString& new_caption)
{
    // we cannot set the name to an existing column

    kcl::Grid* grid = m_tabledoc->getGrid();
    kcl::IModelPtr model = grid->getModel();
    int model_idx = model->getColumnIndex(m_orig_name);
    if (model_idx == -1)
        return;

    int col_count = grid->getColumnCount();
    int i;
    
    for (i = 0; i < col_count; ++i)
    {
        int m = grid->getColumnModelIdx(i);
        if (m == model_idx)
        {
            grid->setColumnCaption(i, new_caption);
        }
    }
}



void ColPropsPanel::refreshDynamicFields()
{
    if (m_tabledoc.isNull())
        return;

    kcl::Grid* grid = m_tabledoc->getGrid();
    if (!grid)
        return;

    IXdGridModelPtr grid_model = grid->getModel();
    if (grid_model.isNull())
        return;


    // if there are queries/marks, we must refresh the entire grid because
    // the expression change could cause more or less rows to be marked

    ITableDocModelPtr tbldoc_model = m_tabledoc->getModel();
    if (tbldoc_model->getMarkEnum()->size() > 0 ||
        grid_model->getGroupBreakExpr().length() > 0)
    {
        grid->refresh(kcl::Grid::refreshAll);
        return;
    }


    grid->updateData();

    int col_count = grid->getColumnCount();
    for (int view_col = 0; view_col < col_count; ++view_col)
    {
        int model_idx = grid->getColumnModelIdx(view_col);
        if (model_idx == -1)
            continue;

        if (grid_model->getColumnCalculated(model_idx))
        {
            grid->refreshColumn(kcl::Grid::refreshAll & ~kcl::Grid::refreshData,
                                view_col);
        }
    }
}


void ColPropsPanel::updateSpinBoxes()
{
    // handle column width spin box
    int type = m_expr_panel->getExpressionType();
    bool enable;


    int combo_selection = m_coltype_combo->GetSelection();
    if (combo_selection > 0)
    {
        // we are not on auto, so use the type from the combo box
        type = dlgtype2xdtype(m_coltype_combo->GetSelection());
    }


    switch (type)
    {
        case xd::typeCharacter:
            setWidth(m_saved_character_width);
            m_colwidth_text->Enable(true);
            break;

        case xd::typeNumeric:
            setWidth(m_saved_numeric_width);
            setScale(m_saved_numeric_scale);
            m_colwidth_text->Enable(true);
            break;

        case xd::typeDate:
            setWidth(4);
            m_colwidth_text->Enable(false);
            break;

        case xd::typeDateTime:
            setWidth(8);
            m_colwidth_text->Enable(false);
            break;

        case xd::typeBoolean:
            setWidth(1);
            m_colwidth_text->Enable(false);
            break;

        case xd::typeDouble:
            setWidth(8);
            setScale(m_saved_numeric_scale);
            m_colwidth_text->Enable(false);
            break;

        case xd::typeInteger:
            setWidth(4);
            m_colwidth_text->Enable(false);
            break;

        default:
            m_colwidth_text->Enable(true);
            break;
    }


    // handle column scale spin box

    enable = false;

    if (type == xd::typeNumeric ||
        type == xd::typeDouble ||
        type == xd::typeInvalid)
    {
        enable = true;
    }

    if (!enable)
    {
        setScale(0);
    }

    m_colscale_text->Enable(enable);
}


void ColPropsPanel::onOkPressed(ExprBuilderPanel*)
{
    xd::IDatabasePtr db = g_app->getDatabase();


    changeColumnCaption(wxEmptyString);
    
    // check for valid field name

    if (!isValidFieldName(m_colname_text->GetValue()))
    {
        appMessageBox(_("The specified field name is invalid.  Please choose a different field name."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);

        m_colname_text->SetInsertionPointEnd();
        m_colname_text->SetFocus();

        return;
    }


    // do final set modify

    bool auto_type = (m_coltype_combo->GetSelection() == 0 ? true : false);

    int new_type;
    int new_width;
    int new_scale;

    new_type = m_last_type;
    new_width = m_last_width;
    new_scale = m_last_scale;

    if (auto_type)
    {
        new_type = m_expr_panel->getExpressionType();

        // implicit typing prefers 'Date' to 'DateTime'
        if (new_type == xd::typeDateTime)
        {
            new_type = xd::typeDate;
            new_width = 4;
            new_scale = 0;
        }
    }


    // scale/width sanity check
    if (new_type == xd::typeNumeric ||
        new_type == xd::typeDouble)
    {
        if (new_scale >= xd::max_numeric_scale)
            new_scale = xd::max_numeric_scale;
        if (new_width <= new_scale)
            new_width = new_scale+1;
    }


    if (m_orig_name.CmpNoCase(m_last_name) == 0 &&
        m_orig_type == new_type &&
        m_orig_width == new_width &&
        m_orig_scale == new_scale &&
        m_orig_expr == m_last_expr)
    {
        closeSite();
        return;
    }

    xd::StructureModify mod_params;
    xd::Structure structure = db->describeTable(m_path);

    if (structure.getColumnExist(towstr(m_orig_name)))
    {
        if (!m_orig_existed)
        {
            appMessageBox(_("The field with the specified name already exists.  Please choose a different field name."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }


        xd::ColumnInfo colinfo;

        if (m_orig_name.CmpNoCase(m_last_name) != 0)
        {
            colinfo.mask |= xd::ColumnInfo::maskName;
            colinfo.name = towstr(m_last_name);
        }

        if (m_orig_type != new_type)
        {
            colinfo.mask |= xd::ColumnInfo::maskType;
            colinfo.type = new_type;
        }

        if (m_orig_width != new_width)
        {
            colinfo.mask |= xd::ColumnInfo::maskWidth;
            colinfo.width = new_width;
        }

        if (m_orig_scale != new_scale)
        {
            colinfo.mask |= xd::ColumnInfo::maskScale;
            colinfo.scale = new_scale;
        }

        if (m_orig_expr != m_last_expr)
        {
            colinfo.mask |= xd::ColumnInfo::maskExpression;
            colinfo.expression = towstr(m_last_expr);
        }

        mod_params.modifyColumn(towstr(m_orig_name), colinfo);
    }
     else
    {
        xd::ColumnInfo colinfo;

        colinfo.name = towstr(m_last_name);
        colinfo.type = new_type;
        colinfo.width = new_width;
        colinfo.scale = new_scale;
        colinfo.expression = towstr(m_last_expr);

        mod_params.createColumn(colinfo);
    }


    xd::IndexInfoEnum old_indexes = db->getIndexEnum(m_tabledoc->getPath());

    if (!db->modifyStructure(m_path, mod_params, NULL))
    {
        appMessageBox(_("The structure of the table could not be modified, due to an invalid parameter."),
                      APPLICATION_NAME,
                      wxOK | wxICON_EXCLAMATION | wxCENTER);

        g_app->getMainFrame()->closeSite(m_doc_site);
        return;
    }

    // refresh the iterator with the set's new structure information
    m_structure = db->describeTable(m_path);

    m_iter->refreshStructure();

    // refresh the target tabledoc's grid information
    kcl::Grid* grid = m_tabledoc->getGrid();
    if (grid)
    {
        grid->refreshModel();
    }

    // let the tabledoc know that a column name has changed
    if (m_orig_name.CmpNoCase(m_last_name) != 0)
    {
        m_tabledoc->onColumnNameChanged(towstr(m_orig_name), towstr(m_last_name));
    }

    sigOkPressed(this);
    closeSite();

    // let other windows know that the structure was modified
    FrameworkEvent* evt = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_STRUCTURE_MODIFIED);
    evt->s_param = m_path;
    g_app->getMainFrame()->postEvent(evt);


    // find out if various indexes need to be recreated.
    // some of the indexes may have been deleted during
    // the modifyStructure() operation

    xd::IndexInfoEnum new_indexes = db->getIndexEnum(m_tabledoc->getPath());

    std::vector<xd::IndexInfo> to_recreate;
    int i, j, old_cnt, new_cnt;
    old_cnt = old_indexes.size();
    new_cnt = new_indexes.size();
    for (i = 0; i < old_cnt; ++i)
    {
        xd::IndexInfo oldidx = old_indexes[i];
        bool found = false;

        for (j = 0; j < new_cnt; ++j)
        {
            xd::IndexInfo newidx = new_indexes[j];

            if (kl::iequals(oldidx.name, newidx.name))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            to_recreate.push_back(oldidx);
        }
    }


    if (to_recreate.size() > 0)
    {
        wxString message = _("One or more indexes must be regenerated due to changes made in the table structure.\nWould you like to do this now?");

        int result = appMessageBox(message,
                                        APPLICATION_NAME,
                                        wxYES_NO |
                                        wxCENTRE |
                                        wxICON_QUESTION);

        // if relationship sync is currently on, we need to
        // turn it off, because it may be using an obsolete index

        AppController* controller = g_app->getAppController();
        if (controller->getRelationshipSync() != tabledocRelationshipSyncNone)
            controller->setRelationshipSync(tabledocRelationshipSyncNone);


        if (result == wxYES)
        {
            jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.index-job");

            kl::JsonNode params;
            kl::JsonNode indexes = params["indexes"];

            std::vector<xd::IndexInfo>::iterator it;
            for (it = to_recreate.begin(); it != to_recreate.end(); ++it)
            {
                kl::JsonNode index_item = indexes.appendElement();

                index_item["input"].setString(m_path);
                index_item["name"].setString(it->name);
                index_item["expression"].setString(it->expression);
            }

            job->getJobInfo()->setTitle(towstr(_("Creating Index")));
            job->setParameters(params.toString());

            g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
        }
    }
}



void ColPropsPanel::revertChanges()
{
    if (!m_tabledoc.isOk())
        return;
    
    changeColumnCaption(wxEmptyString);

    if (m_orig_name.CmpNoCase(m_last_name) == 0 &&
        m_orig_type == m_last_type &&
        m_orig_width == m_last_width &&
        m_orig_scale == m_last_scale &&
        m_orig_expr == m_last_expr)
    {
        return;
    }

    // restore calculated field

    xd::ColumnInfo colinfo;
    
    colinfo.mask = xd::ColumnInfo::maskName | xd::ColumnInfo::maskType |
                   xd::ColumnInfo::maskWidth | xd::ColumnInfo::maskScale |
                   xd::ColumnInfo::maskExpression;

    colinfo.name = towstr(m_orig_name);
    colinfo.type = m_orig_type;
    colinfo.width = m_orig_width;
    colinfo.scale = m_orig_scale;
    colinfo.expression = towstr(m_orig_expr);

    xd::StructureModify mod_params;
    mod_params.modifyColumn(towstr(m_modify_field), colinfo);

    if (m_iter->modifyStructure(mod_params, NULL))
    {
        kcl::Grid* grid = m_tabledoc->getGrid();
        grid->refreshModel();

        if (m_tabledoc)
        {
            if (m_modify_field.CmpNoCase(m_orig_name) != 0)
            {
                m_tabledoc->onColumnNameChanged(towstr(m_modify_field), towstr(m_orig_name));
            }
        }
    }

    refreshDynamicFields();
}


bool ColPropsPanel::onSiteClosing(bool force)
{
    if (!m_ok_pressed)
    {
        if (m_tabledoc.isOk())
        {
            revertChanges();
            sigCancelPressed(this);
        }
    }

    return true;
}


void ColPropsPanel::onCancelPressed(ExprBuilderPanel*)
{
    // cancel was pressed on ExprBuilder panel
    closeSite(false /* ok = false */);
}


void ColPropsPanel::onCancel(wxCommandEvent& evt)
{
    closeSite(false /* ok = false */);
}


