/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-11-29
 *
 */


#include "appmain.h"
#include "tabledocmodel.h"
#include "tabledoc_private.h"
#include "jobquery.h"


static wxString getUniqueString()
{
    unsigned int t = ((unsigned int)time(NULL)) & 0xffffffff;
    
    unsigned int r = rand();
    r <<= 16;
    r |= (rand() & 0xffff);

#ifdef __WXWINCE__
    unsigned c = ((unsigned int)GetTickCount()) & 0xffffff;
#else
    unsigned c = ((unsigned int)clock()) & 0xffffff;
#endif

    static int count = 0;
    count++;
    if (count > 255)
        count = 0;

    return wxString::Format(wxT("%08x%06x%02x%08x"), t, c, count, r);
}




// -- Model Registry --

class ModelRegistry : public xcm::signal_sink
{

public:

    ITableDocModelPtr lookupModel(const wxString& set_id)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        std::vector<ITableDocModelPtr>::iterator it;
        for (it = m_models.begin(); it != m_models.end(); ++it)
        {
            if (!set_id.CmpNoCase((*it)->getId()))
            {
                return (*it);
            }
        }

        return xcm::null;
    }

    ITableDocModelPtr loadModel(const wxString& set_id)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        if (set_id.IsEmpty())
        {
            return static_cast<ITableDocModel*>(new TableDocModel);
        }

        ITableDocModelPtr model = lookupModel(set_id);
        if (model.isOk())
        {
            return model;
        }

        // -- model needs to be loaded --
        TableDocModel* model_p = new TableDocModel;
        model_p->setId(set_id);
        model_p->sigDeleted.connect(this, &ModelRegistry::onTableDocModelDeleted);
        model = model_p;
        m_models.push_back(model);

        return model;
    }

    bool deleteModel(const wxString& set_id)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        if (set_id.IsEmpty())
            return false;

        // -- first find the model and delete it from the vector --

        std::vector<ITableDocModelPtr>::iterator it;
        for (it = m_models.begin(); it != m_models.end(); ++it)
        {
            if (!set_id.CmpNoCase((*it)->getId()))
            {
                m_models.erase(it);
                break;
            }
        }


        // -- next attempt to delete the disk files --
        tango::IDatabasePtr db = g_app->getDatabase();
        if (!db)
            return false;

        wxString path;
        path = wxString::Format(wxT("/.appdata/%s/dcfe/setinfo/%s"),
                                towx(db->getActiveUid()).c_str(),
                                set_id.c_str());

        return db->deleteFile(towstr(path));
    }

    void cleanup()  // table model garbage collection
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        std::vector<ITableDocModel*> vec;
        std::vector<ITableDocModel*>::iterator vit;
        std::vector<ITableDocModelPtr>::iterator it;
        
        ITableDocModel* ptr;

        for (it = m_models.begin(); it != m_models.end(); ++it)
        {
            ptr = it->p;
            ptr->ref();
            vec.push_back(ptr);
        }
        
        m_models.clear();

        for (vit = vec.begin(); vit != vec.end(); ++vit)
        {
            m_deleted_flag = false;
            (*vit)->unref();
            if (!m_deleted_flag)
            {
                ITableDocModelPtr sp_ptr;
                sp_ptr = (*vit);
                m_models.push_back(sp_ptr);
            }
        }
    }

    void clear()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_models.clear();
    }

private:

    void onTableDocModelDeleted(TableDocModel* model)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_deleted_flag = true;
    }

private:

    xcm::mutex m_obj_mutex;
    std::vector<ITableDocModelPtr> m_models;
    bool m_deleted_flag;
};




ModelRegistry g_model_registry;




class TableDocObjectBase : public ITableDocObject
{
    XCM_CLASS_NAME("appmain.TableDocObjectBase")
    XCM_BEGIN_INTERFACE_MAP(TableDocObjectBase)
        XCM_INTERFACE_ENTRY(ITableDocObject)
    XCM_END_INTERFACE_MAP()

public:

    TableDocObjectBase()
    {
        m_id = getUniqueString();
        m_dirty = false;
    }

    wxString getObjectId()
    {
        return m_id;
    }

    void setObjectId(const wxString& id)
    {
        m_id = id;
    }

    bool getDirty()
    {
        return m_dirty;
    }

    void setDirty(bool dirty)
    {
        m_dirty = dirty;
    }

protected:

    bool m_dirty;
    wxString m_id;
    //xcm::mutex m_obj_mutex;
};




// -- TangoGridMark implementation --

class TableDocMark :  public ITableDocMark,
                      public TableDocObjectBase
{
friend class TangoGridModel;

    XCM_CLASS_NAME("appmain.TableDocMark")
    XCM_BEGIN_INTERFACE_MAP(TableDocMark)
        XCM_INTERFACE_ENTRY(ITableDocMark)
        XCM_INTERFACE_CHAIN(TableDocObjectBase)
    XCM_END_INTERFACE_MAP()

public:

    TableDocMark()
    {
        m_fgcolor.Set(0,0,0);
        m_bgcolor.Set(255,255,178);
        m_mark_active = true;
    }

    virtual ~TableDocMark()
    {
    }

    void setMarkActive(bool new_val)
    {
        if (m_mark_active == new_val)
            return;

        m_mark_active = new_val;
        setDirty(true);
    }

    bool getMarkActive()
    {
        return m_mark_active;
    }

    void setDescription(const wxString& new_val)
    {
        if (m_description == new_val)
            return;

        m_description = new_val;
        setDirty(true);
    }

    wxString getDescription()
    {
        return m_description;
    }



    void setExpression(const wxString& new_val)
    {
        if (m_expression == new_val)
            return;
            
        m_expression = new_val;
        setDirty(true);
    }
    
    wxString getExpression()
    {
        return m_expression;
    }


    void setForegroundColor(const wxColour& new_val)
    {
        if (m_fgcolor == new_val)
            return;

        m_fgcolor = new_val;
        setDirty(true);
    }

    wxColour getForegroundColor()
    {
        return m_fgcolor;
    }

    void setBackgroundColor(const wxColour& new_val)
    {
        if (m_bgcolor == new_val)
            return;

        m_bgcolor = new_val;
        setDirty(true);
    }

    wxColour getBackgroundColor()
    {
        if (!m_bgcolor.IsOk())
            return wxNullColour;
            
        return m_bgcolor;
    }


    ITableDocObjectPtr clone()
    {
        TableDocMark* mark = new TableDocMark;
        mark->m_id = m_id;
        mark->m_description = m_description;
        mark->m_expression = m_expression;
        mark->m_fgcolor = m_fgcolor;
        mark->m_bgcolor = m_bgcolor;
        mark->m_mark_active = m_mark_active;
        return static_cast<ITableDocObject*>(mark);
    }


    bool writeToNode(tango::INodeValuePtr node)
    {
        int int_color;

        tango::INodeValuePtr description_node = node->createChild(L"description");
        description_node->setString(towstr(m_description));

        tango::INodeValuePtr expression_node = node->createChild(L"expression");
        expression_node->setString(towstr(m_expression));
        
        tango::INodeValuePtr mark_active_node = node->createChild(L"mark_active");
        mark_active_node->setBoolean(m_mark_active);

        tango::INodeValuePtr mark_fgcolor_node = node->createChild(L"mark_fgcolor");
        int_color = (m_fgcolor.Red() << 16) | (m_fgcolor.Green() << 8) | m_fgcolor.Blue();
        mark_fgcolor_node->setInteger(int_color);

        tango::INodeValuePtr mark_bgcolor_node = node->createChild(L"mark_bgcolor");
        if (m_bgcolor.IsOk())
            int_color = (m_bgcolor.Red() << 16) | (m_bgcolor.Green() << 8) | m_bgcolor.Blue();
         else
            int_color = -1;
        mark_bgcolor_node->setInteger(int_color);

        tango::INodeValuePtr markset_path_node = node->createChild(L"queryset_path");
        markset_path_node->setString(L"");

        return true;
    }

    bool readFromNode(tango::INodeValuePtr node)
    {
        int int_color;

        tango::INodeValuePtr description_node = node->getChild(L"description", false);
        if (description_node.isNull())
            return false;
        m_description = towx(description_node->getString());

        tango::INodeValuePtr expression_node = node->getChild(L"expression", false);
        if (expression_node)
        {
            m_expression = towx(expression_node->getString());
        }
        
        
        tango::INodeValuePtr mark_active_node = node->getChild(L"mark_active", false);
        if (mark_active_node.isNull())
            return false;
        m_mark_active = mark_active_node->getBoolean();

        tango::INodeValuePtr mark_fgcolor_node = node->getChild(L"mark_fgcolor", false);
        if (mark_fgcolor_node.isNull())
            return false;
        int_color = mark_fgcolor_node->getInteger();
        m_fgcolor.Set(int_color >> 16, (int_color >> 8) & 0xff, int_color & 0xff);

        tango::INodeValuePtr mark_bgcolor_node = node->getChild(L"mark_bgcolor", false);
        if (mark_bgcolor_node.isNull())
            return false;
        int_color = mark_bgcolor_node->getInteger();
        if (int_color != -1)
            m_bgcolor.Set(int_color >> 16, (int_color >> 8) & 0xff, int_color & 0xff);
         else
            m_bgcolor = wxNullColour;
        

        // -- this will load the expression from the old filter set from older versions --
        tango::INodeValuePtr markset_path_node = node->getChild(L"queryset_path", false);
        if (expression_node.isNull() && markset_path_node.isOk())
        {
            // doesn't really work because the queryset folder itself is being
            // removed in the upgrade function at the bottom of this file
            
        /*
            tango::IDatabasePtr db = g_app->getDatabase();
            std::wstring set_path = markset_path_node->getString();
            
            tango::INodeValuePtr markset = db->openNodeFile(set_path);
            if (markset)
            {
                tango::INodeValuePtr setid_node = markset->getChild(L"set_id", false);
                if (setid_node)
                {
                    std::wstring set_id = setid_node->getString();
                    std::wstring path = L"/.system/objects/";
                    path += set_id;
                    
                    tango::INodeValuePtr object_node = db->openNodeFile(path);
                    if (object_node)
                    {
                        tango::INodeValuePtr expr_node = object_node->getChild(L"condition_expr", false);
                        if (expr_node)
                        {
                            m_expression = towx(expr_node->getString());
                            expr_node.clear();
                            object_node.clear();
                            setid_node.clear();
                            markset.clear();
                            
                            db->deleteFile(set_path);
                            markset_path_node->setString(L"");
                            
                            tango::INodeValuePtr expression_node = node->createChild(L"expression");
                            expression_node->setString(towstr(m_expression));
                        }
                    }
                }
            }
        */
        
        /*
            tango::IDatabasePtr db = g_app->getDatabase();
            std::wstring set_path = markset_path_node->getString();
            if (db.isOk() && set_path.length() > 0)
            {
                tango::IFilterSetPtr set = db->openSet(set_path);
                if (set)
                {
                    m_expression = towx(set->getFilterExpression());
                    
                    // get rid of filter set and upgrade tabledocmodel node
                    set.clear();
                    db->deleteFile(set_path);
                    markset_path_node->setString(L"");
                    
                    tango::INodeValuePtr expression_node = node->createChild(L"expression");
                    expression_node->setString(towstr(m_expression));
                }
            }
        */
        }

        
        return true;
    }

private:

    bool m_mark_active;
    wxColor m_fgcolor;
    wxColor m_bgcolor;
    wxString m_description;
    wxString m_expression;
};




// -- TableDocWatch implementation --

class TableDocWatch : public TableDocObjectBase,
                      public ITableDocWatch
{
friend class TableDoc;

    XCM_CLASS_NAME("appmain.TableDocWatch")
    XCM_BEGIN_INTERFACE_MAP(TableDocWatch)
        XCM_INTERFACE_ENTRY(ITableDocWatch)
        XCM_INTERFACE_CHAIN(TableDocObjectBase)
    XCM_END_INTERFACE_MAP()

public:

    TableDocWatch()
    {
        m_expression = wxT("");
    }

    virtual ~TableDocWatch()
    {
    }

    void setExpression(const wxString& expression)
    {
        if (m_expression == expression)
            return;

        m_expression = expression;
        setDirty(true);
    }

    wxString getExpression()
    {
        return m_expression;
    }

    ITableDocObjectPtr clone()
    {
        TableDocWatch* watch = new TableDocWatch;
        watch->m_id = m_id;
        watch->m_expression = m_expression;
        return static_cast<ITableDocObject*>(watch);
    }

    bool writeToNode(tango::INodeValuePtr node)
    {
        tango::INodeValuePtr expr_node = node->createChild(L"expression");
        expr_node->setString(towstr(m_expression));
        return true;
    }

    bool readFromNode(tango::INodeValuePtr node)
    {
        tango::INodeValuePtr expr_node = node->getChild(L"expression", false);
        if (expr_node.isNull())
            return false;
        m_expression = towx(expr_node->getString());
        return true;
    }

public:

    wxString m_expression;
};




class TableDocViewCol : public TableDocObjectBase,
                        public ITableDocViewCol
{
    XCM_CLASS_NAME("appmain.TableDocViewCol")
    XCM_BEGIN_INTERFACE_MAP(TableDocViewCol)
        XCM_INTERFACE_ENTRY(ITableDocViewCol)
        XCM_INTERFACE_CHAIN(TableDocObjectBase)
    XCM_END_INTERFACE_MAP()


public:

    TableDocViewCol()
    {
        m_name = wxT("");
        m_size = 0;
        m_fgcolor = wxNullColour;
        m_bgcolor = wxNullColour;
        m_alignment = tabledocAlignDefault;
        m_text_wrap = tabledocWrapDefault;
    }

    virtual ~TableDocViewCol()
    {
    }

    void setName(const wxString& new_value)
    {
        if (m_name == new_value)
            return;

        m_name = new_value;
        setDirty(true);
    }

    wxString getName()
    {
        return m_name;
    }

    void setSize(int new_value)
    {
        if (m_size == new_value)
            return;

        m_size = new_value;
        setDirty(true);
    }

    int getSize()
    {
        return m_size;
    }

    void setForegroundColor(const wxColor& new_value)
    {
        if (m_fgcolor == new_value)
            return;

        m_fgcolor = new_value;
        setDirty(true);
    }

    wxColor getForegroundColor()
    {
        return m_fgcolor;
    }

    void setBackgroundColor(const wxColor& new_value)
    {
        if (m_bgcolor == new_value)
            return;

        m_bgcolor = new_value;
        setDirty(true);
    }

    wxColor getBackgroundColor()
    {
        return m_bgcolor;
    }

    void setAlignment(int new_value)
    {
        if (m_alignment == new_value)
            return;

        m_alignment = new_value;
        setDirty(true);
    }

    int getAlignment()
    {
        return m_alignment;
    }


    void setTextWrap(int new_value)
    {
        if (m_text_wrap == new_value)
            return;

        m_text_wrap = new_value;
        setDirty(true);
    }

    int getTextWrap()
    {
        return m_text_wrap;
    }




    ITableDocObjectPtr clone()
    {
        TableDocViewCol* viewcol = new TableDocViewCol;
        viewcol->m_id = m_id;
        viewcol->m_name = m_name;
        viewcol->m_size = m_size;
        viewcol->m_fgcolor = m_fgcolor;
        viewcol->m_bgcolor = m_bgcolor;
        viewcol->m_alignment = m_alignment;
        viewcol->m_text_wrap = m_text_wrap;
        return static_cast<ITableDocObject*>(viewcol);
    }


    bool writeToNode(tango::INodeValuePtr node)
    {
        tango::INodeValuePtr col_name,
                            col_size,
                            col_fgcolor,
                            col_bgcolor,
                            col_alignment,
                            col_textwrap;

        col_name = node->createChild(L"name");
        col_name->setString(towstr(m_name));

        col_size = node->createChild(L"size");
        col_size->setInteger(m_size);

        col_fgcolor = node->createChild(L"fgcolor");
        col_fgcolor->setInteger(color2int(m_fgcolor));

        col_bgcolor = node->createChild(L"bgcolor");
        col_bgcolor->setInteger(color2int(m_bgcolor));

        col_alignment = node->createChild(L"alignment");
        col_alignment->setInteger(m_alignment);
        
        col_textwrap = node->createChild(L"text_wrap");
        col_textwrap->setInteger(m_text_wrap);

        return true;
    }

    bool readFromNode(tango::INodeValuePtr node)
    {
        tango::INodeValuePtr col_name,
                            col_size,
                            col_fgcolor,
                            col_bgcolor,
                            col_alignment,
                            col_textwrap;
            
        col_name = node->getChild(L"name", false);
        if (!col_name)
            return false;
        m_name = towx(col_name->getString());
        
        col_size = node->getChild(L"size", false);
        if (!col_size)
            return false;
        m_size = col_size->getInteger();

        col_fgcolor = node->getChild(L"fgcolor", false);
        if (!col_fgcolor)
            return false;
        m_fgcolor = int2color(col_fgcolor->getInteger());

        col_bgcolor = node->getChild(L"bgcolor", false);
        if (!col_bgcolor)
            return false;
        m_bgcolor = int2color(col_bgcolor->getInteger());

        col_alignment = node->getChild(L"alignment", false);
        if (col_alignment)
        {
            m_alignment = col_alignment->getInteger();
        }
         else
        {
            m_alignment = tabledocAlignDefault;
        }

        col_textwrap = node->getChild(L"text_wrap", false);
        if (col_textwrap)
        {
            m_text_wrap = col_alignment->getInteger();
        } 
         else
        {
            m_text_wrap = tabledocWrapDefault;
        }

        return true;
    }

private:
    
    wxString m_name;
    int m_size;
    wxColor m_fgcolor;
    wxColor m_bgcolor;
    int m_alignment;
    int m_text_wrap;
};



// -- TableDocView implementation --

class TableDocView : public TableDocObjectBase,
                     public ITableDocView
{
    XCM_CLASS_NAME("appmain.TableDocView")
    XCM_BEGIN_INTERFACE_MAP(TableDocView)
        XCM_INTERFACE_ENTRY(ITableDocView)
        XCM_INTERFACE_CHAIN(TableDocObjectBase)
    XCM_END_INTERFACE_MAP()

public:

    TableDocView()
    {
        m_description = wxT("");
        m_row_size = -1; // (default row size)
    }

    virtual ~TableDocView()
    {
    }

    wxString getDescription()
    {
        return m_description;
    }

    void setDescription(const wxString& new_val)
    {
        if (m_description == new_val)
            return;

        m_description = new_val;
        setDirty(true);
    }

    void setRowSize(int new_value)
    {
        if (m_row_size == new_value)
            return;
            
        m_row_size = new_value;
        setDirty(true);
    }
    
    int getRowSize()
    {
        return m_row_size;
    }

    int getColumnCount()
    {
        return m_cols.size();
    }

    ITableDocViewColPtr getColumn(unsigned int idx)
    {
        if (idx >= m_cols.size())
            return xcm::null;

        return m_cols[idx];
    }

    int getColumnIdx(const wxString& col_name)
    {
        std::vector<ITableDocViewColPtr>::iterator it;
        int idx = 0;
        for (it = m_cols.begin(); it != m_cols.end(); ++it)
        {
            if (!col_name.CmpNoCase((*it)->getName()))
                return idx;
            idx++;
        }
        return -1;
    }

    ITableDocViewColPtr createColumn(int pos)
    {
        ITableDocViewColPtr col = new TableDocViewCol;

        if (pos >= 0 && (size_t)pos <= m_cols.size())
        {
            m_cols.insert(m_cols.begin() + pos, col);
        }
         else
        {
            m_cols.push_back(col);
        }

        setDirty(true);
        return col;
    }

    void deleteColumn(unsigned int idx)
    {
        if (idx >= m_cols.size())
            return;

        m_cols.erase(m_cols.begin() + idx);
        setDirty(true);
    }

    void deleteAllColumns()
    {
        m_cols.clear();
        setDirty(true);
    }

    void moveColumn(unsigned int old_idx, unsigned int new_idx)
    {
        ITableDocViewColPtr col = m_cols[old_idx];
        m_cols.erase(m_cols.begin() + old_idx);
        m_cols.insert(m_cols.begin() + new_idx, col);
        setDirty(true);
    }

    bool writeToNode(tango::INodeValuePtr node)
    {
        tango::INodeValuePtr desc_node = node->createChild(L"description");
        desc_node->setString(towstr(m_description));

        tango::INodeValuePtr rowsize_node = node->createChild(L"row_size");
        rowsize_node->setInteger(m_row_size);

        tango::INodeValuePtr cols_node = node->createChild(L"columns");

        wchar_t buf[255];
        int counter = 0;

        tango::INodeValuePtr col;
        wxColor color;

        std::vector<ITableDocViewColPtr>::iterator col_it;
        for (col_it = m_cols.begin(); col_it != m_cols.end(); ++col_it)
        {
            swprintf(buf, 255, L"column_%03d", counter++);
            col = cols_node->createChild(buf);

            ITableDocObjectPtr obj = *col_it;
            if (!obj->writeToNode(col))
                return false;
        }

        return true;
    }

    bool readFromNode(tango::INodeValuePtr node)
    {
        tango::INodeValuePtr desc_node = node->getChild(L"description", false);
        if (desc_node)
        {
            m_description = towx(desc_node->getString());
        }
        
        tango::INodeValuePtr rowsize_node = node->getChild(L"row_size", false);
        if (rowsize_node)
        {
            m_row_size = rowsize_node->getInteger();
        }
         else
        {
            m_row_size = -1;
        }
        

        tango::INodeValuePtr cols_node = node->getChild(L"columns", false);
        if (!cols_node)
            return false;

        TableDocViewCol* viewcol;

        tango::INodeValuePtr col;

        unsigned int child_count = cols_node->getChildCount();
        for (unsigned int i = 0; i < child_count; i++)
        {
            col = cols_node->getChildByIdx(i);

            viewcol = new TableDocViewCol;
            if (viewcol->readFromNode(col))
            {
                m_cols.push_back(static_cast<ITableDocViewCol*>(viewcol));
            }
             else
            {
                delete viewcol;
            }
        }

        return true;
    }

    ITableDocObjectPtr clone()
    {
        TableDocView* view = new TableDocView;
        view->m_id = m_id;
        view->m_description = m_description;
        view->m_row_size = m_row_size;
        std::vector<ITableDocViewColPtr>::iterator it;
        for (it = m_cols.begin(); it != m_cols.end(); ++it)
        {
            ITableDocObjectPtr obj = *it;
            view->m_cols.push_back(obj->clone());
        }
        return static_cast<ITableDocObject*>(view);
    }


    void setDirty(bool new_val)
    {
        if (m_dirty == new_val)
            return;

        m_dirty = new_val;

        if (!m_dirty)
        {
            std::vector<ITableDocViewColPtr>::iterator it;
            for (it = m_cols.begin(); it != m_cols.end(); ++it)
            {
                ITableDocObjectPtr obj = *it;
                obj->setDirty(false);
            }
        }
    }

    bool getDirty()
    {
        if (m_dirty)
            return true;

        std::vector<ITableDocViewColPtr>::iterator it;
        for (it = m_cols.begin(); it != m_cols.end(); ++it)
        {
            ITableDocObjectPtr obj = *it;
            if (obj->getDirty())
                return true;
        }

        return false;
    }

private:

    wxString m_description;
    std::vector<ITableDocViewColPtr> m_cols;
    int m_row_size;
};



// -- TableDocModel class implementation --


TableDocModel::TableDocModel()
{
    m_watches_cache_time = 0;
    m_marks_cache_time = 0;
    m_views_cache_time = 0;
}

TableDocModel::~TableDocModel()
{
    sigDeleted(this);
}

void TableDocModel::setId(const wxString& id)
{
    XCM_AUTO_LOCK(m_obj_mutex);
    m_id = id;
}

wxString TableDocModel::getId()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    return m_id;
}

tango::INodeValuePtr TableDocModel::flushObject(ITableDocObjectPtr obj)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    if (obj.isNull())
        return xcm::null;

    if (!obj->getDirty())
        return xcm::null;

    if (m_id.IsEmpty())
        return xcm::null;

    wxString tag;

    ITableDocViewPtr view = obj;
    ITableDocMarkPtr mark = obj;
    ITableDocWatchPtr watch = obj;

    if (view.isOk())
        tag = wxT("views");
    if (mark.isOk())
        tag = wxT("marks");
    if (watch.isOk())
        tag = wxT("watches");

    if (tag.IsEmpty())
        return xcm::null;

    wxString path;
    path = wxString::Format(wxT("/.appdata/%s/dcfe/setinfo/%s/%s"),
                            towx(g_app->getDatabase()->getActiveUid()).c_str(),
                            m_id.c_str(),
                            tag.c_str());

    tango::INodeValuePtr file;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    file = db->openNodeFile(towstr(path));
    if (file.isNull())
    {
        file = db->createNodeFile(towstr(path));
        if (file.isNull())
            return xcm::null;
    }

    tango::INodeValuePtr base_node = file->getChild(towstr(tag), true);
    if (base_node.isNull())
        return xcm::null;

    std::wstring node_name = towstr(obj->getObjectId());

    tango::INodeValuePtr node = base_node->getChild(node_name, true);
    if (node.isNull())
        return xcm::null;
    node->deleteAllChildren();
    node->setString(L"");
    obj->writeToNode(node);
    
    obj->setDirty(false);

    // -- update our cached version --
    {
        std::vector<ITableDocObjectPtr>* vec = NULL;

        if (view.isOk())
            vec = &m_views;
        else if (mark.isOk())
            vec = &m_marks;
        else if (watch.isOk())
            vec = &m_watches;

        if (vec)
        {
            bool found = false;

            std::vector<ITableDocObjectPtr>::iterator it;
            for (it = vec->begin(); it != vec->end(); ++it)
            {
                if ((*it)->getObjectId() == obj->getObjectId())
                {
                    *it = obj->clone();
                    (*it)->setDirty(false);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                vec->push_back(obj->clone());
            }
        }
    }


    return file;
}

bool TableDocModel::writeObject(ITableDocObjectPtr obj)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    if (!obj->getDirty())
        return true;

    if (m_id.IsEmpty())
        return false;

    return flushObject(obj).isOk();
}

bool TableDocModel::writeMultipleObjects(ITableDocObjectEnumPtr objs)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    if (m_id.IsEmpty())
        return false;

    std::vector<tango::INodeValuePtr> v;
    
    int i;
    int count = objs->size();

    for (i = 0; i < count; ++i)
    {
        v.push_back(flushObject(objs->getItem(i)));
    }

    return true;
}


bool TableDocModel::deleteObject(ITableDocObjectPtr obj)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    if (obj.isNull())
        return false;

    if (m_id.IsEmpty())
        return false;

    wxString tag;

    ITableDocViewPtr view = obj;
    ITableDocMarkPtr mark = obj;
    ITableDocWatchPtr watch = obj;

    if (view.isOk())
        tag = wxT("views");
    if (mark.isOk())
        tag = wxT("marks");
    if (watch.isOk())
        tag = wxT("watches");

    if (tag.IsEmpty())
        return false;

    wxString path;
    path = wxString::Format(wxT("/.appdata/%s/dcfe/setinfo/%s/%s"),
                            towx(g_app->getDatabase()->getActiveUid()).c_str(),
                            m_id.c_str(),
                            tag.c_str());

    tango::INodeValuePtr file;

    tango::IDatabasePtr db = g_app->getDatabase();
    file = db->openNodeFile(towstr(path));
    if (file.isNull())
    {
        file = db->createNodeFile(towstr(path));
        if (file.isNull())
            return false;
    }

    tango::INodeValuePtr base_node = file->getChild(towstr(tag), true);
    if (base_node.isNull())
        return false;

    std::wstring node_name = towstr(obj->getObjectId());

    if (!base_node->getChildExist(node_name))
        return false;

    base_node->deleteChild(node_name);


    // -- update our cached version --
    {
        std::vector<ITableDocObjectPtr>* vec = NULL;

        if (view.isOk())
            vec = &m_views;
        else if (mark.isOk())
            vec = &m_marks;
        else if (watch.isOk())
            vec = &m_watches;

        if (vec)
        {
            std::vector<ITableDocObjectPtr>::iterator it;
            for (it = vec->begin(); it != vec->end(); ++it)
            {
                if ((*it)->getObjectId() == obj->getObjectId())
                {
                    vec->erase(it);
                    break;
                }
            }
        }
    }

    return true;
}


ITableDocWatchPtr TableDocModel::createWatchObject()
{
    TableDocWatch* obj = new TableDocWatch;
    obj->setDirty(true);
    return static_cast<ITableDocWatch*>(obj);
}

ITableDocMarkPtr TableDocModel::createMarkObject()
{
    TableDocMark* obj = new TableDocMark;
    obj->setDirty(true);
    return static_cast<ITableDocMark*>(obj);
}

ITableDocViewPtr TableDocModel::createViewObject()
{
    TableDocView* obj = new TableDocView;
    obj->setDirty(true);
    return static_cast<ITableDocView*>(obj);
}


static tango::INodeValuePtr getObjects(const wxString set_id,
                                      const wxString& tag)
{
    if (set_id.IsEmpty())
        return xcm::null;

    wxString path;
    path = wxString::Format(wxT("/.appdata/%s/dcfe/setinfo/%s/%s"),
                            towx(g_app->getDatabase()->getActiveUid()).c_str(),
                            set_id.c_str(),
                            tag.c_str());

    tango::IDatabasePtr db = g_app->getDatabase();
    tango::INodeValuePtr file = db->openNodeFile(towstr(path));
    
    if (file.isNull())
        return xcm::null;

    return file->getChild(towstr(tag), true);
}


ITableDocWatchEnumPtr TableDocModel::getWatchEnum()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<ITableDocWatchPtr>* vec;
    vec = new xcm::IVectorImpl<ITableDocWatchPtr>;

    if (m_watches_cache_time == 0)
    {
        m_watches.clear();
        m_watches_cache_time = time(NULL);

        tango::INodeValuePtr base_node = getObjects(m_id, wxT("watches"));
        if (base_node.isNull())
            return vec;
        int i, child_count = base_node->getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            TableDocWatch* obj = new TableDocWatch;
            tango::INodeValuePtr node = base_node->getChildByIdx(i);
            obj->setObjectId(towx(node->getName()));
            obj->readFromNode(node);
            m_watches.push_back(static_cast<ITableDocObject*>(obj));
        }
    }

    std::vector<ITableDocObjectPtr>::iterator it;
    for (it = m_watches.begin(); it != m_watches.end(); ++it)
    {
        vec->append((*it)->clone());
    }

    return vec;
}

ITableDocMarkEnumPtr TableDocModel::getMarkEnum()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<ITableDocMarkPtr>* vec;
    vec = new xcm::IVectorImpl<ITableDocMarkPtr>;


    if (m_marks_cache_time == 0)
    {
        tango::INodeValuePtr base_node;

        // -- <= 2.1 upgrade --
        base_node = getObjects(m_id, wxT("queries"));
        if (base_node.isOk())
        {
            // -- old filename for 2.1 and before was called 'queries'.
            //    we need to rename it to marks.xml --
            
            xcm::IVectorImpl<ITableDocObjectPtr>* mark_vec;
            mark_vec = new xcm::IVectorImpl<ITableDocObjectPtr>;

            int i, child_count = base_node->getChildCount();
            for (i = 0; i < child_count; ++i)
            {
                TableDocMark* obj = new TableDocMark;
                tango::INodeValuePtr node = base_node->getChildByIdx(i);
                obj->readFromNode(node);
                obj->setDirty(true); // -- write them to the new file --
                mark_vec->append(static_cast<ITableDocObject*>(obj));
            }

            base_node.clear();

            wxString path;
            path = wxString::Format(wxT("/.appdata/%s/dcfe/setinfo/%s/queries"),
                                    towx(g_app->getDatabase()->getActiveUid()).c_str(),
                                    m_id.c_str());

            tango::IDatabasePtr db = g_app->getDatabase();
            db->deleteFile(towstr(path));

            mark_vec->ref();
            writeMultipleObjects(mark_vec);
            mark_vec->unref();
        }


        m_marks.clear();
        m_marks_cache_time = time(NULL);

        
        base_node = getObjects(m_id, wxT("marks"));
        if (base_node.isNull())
            return vec;
        int i, child_count = base_node->getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            TableDocMark* obj = new TableDocMark;
            tango::INodeValuePtr node = base_node->getChildByIdx(i);
            obj->setObjectId(towx(node->getName()));
            obj->readFromNode(node);
            m_marks.push_back(static_cast<ITableDocObject*>(obj));
        }
    }

    std::vector<ITableDocObjectPtr>::iterator it;
    for (it = m_marks.begin(); it != m_marks.end(); ++it)
    {
        vec->append((*it)->clone());
    }

    return vec;
}

ITableDocViewEnumPtr TableDocModel::getViewEnum()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<ITableDocViewPtr>* vec;
    vec = new xcm::IVectorImpl<ITableDocViewPtr>;

    if (m_views_cache_time == 0)
    {
        m_views.clear();
        m_views_cache_time = time(NULL);

        tango::INodeValuePtr base_node = getObjects(m_id, wxT("views"));
        if (base_node.isNull())
            return vec;
        int i, child_count = base_node->getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            TableDocView* obj = new TableDocView;
            tango::INodeValuePtr node = base_node->getChildByIdx(i);
            obj->setObjectId(towx(node->getName()));
            obj->readFromNode(node);
            m_views.push_back(static_cast<ITableDocObject*>(obj));
        }
    }

    std::vector<ITableDocObjectPtr>::iterator it;
    for (it = m_views.begin(); it != m_views.end(); ++it)
    {
        vec->append((*it)->clone());
    }

    return vec;
}


const int DEFAULT_MARK_COLOR_COUNT = 7;
const wxColour DEFAULT_MARK_COLORS[7] =
{
    wxColour(255,255,178),
    wxColour(178,209,255),
    wxColour(255,224,178),
    wxColour(178,255,193),
    wxColour(224,178,255),
    wxColour(255,178,178),
    wxColour(255,178,255)
};

wxColour TableDocModel::getNextMarkColor()
{
    ITableDocMarkEnumPtr vec = getMarkEnum();
    int i, j, count = vec->size();
    
    if (count == 0)
        return DEFAULT_MARK_COLORS[0];
    
    for (i = count-1; i >= 0; --i)
    {
        ITableDocMarkPtr vecmark = vec->getItem(i);
        
        for (j = 0; j < DEFAULT_MARK_COLOR_COUNT; ++j)
        {
            if (vecmark->getBackgroundColor() == DEFAULT_MARK_COLORS[j])
            {
                if (j >= DEFAULT_MARK_COLOR_COUNT-1)
                    j = 0;
                     else
                    j++;
                
                return DEFAULT_MARK_COLORS[j];
            }
        }
    }
    
    return DEFAULT_MARK_COLORS[0];
}




// -- TableDocMgr --

bool TableDocMgr::newFile(const wxString& _path)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    wxString path = towx(tango::dequoteIdentifier(db, towstr(_path)));
    wxString sql = wxT("CREATE TABLE [");
    sql += path;
    sql += wxT("] (field1 VARCHAR(40))");
    
    xcm::IObjectPtr result;
    return db->execute(towstr(sql), 0, result, NULL);
}

ITableDocPtr TableDocMgr::createTableDoc()
{
    return static_cast<ITableDoc*>(new TableDoc);
}

ITableDocPtr TableDocMgr::getActiveTableDoc(int* site_id)
{
    cfw::IDocumentSitePtr active_site;
    
    active_site = g_app->getMainFrame()->getActiveChild();
    if (active_site.isNull())
    {
        if (site_id)
        {
            *site_id = 0;
        }

        return xcm::null;
    }

    if (site_id)
    {
        *site_id = active_site->getId();
    }

    ITableDocPtr tabledoc = active_site->getDocument();

    return tabledoc;
}

void TableDocMgr::copyModel(tango::ISetPtr _src_set,
                            tango::ISetPtr _dest_set)
{
    // -- create a new model for the result set --
    tango::ISetPtr source_set = _src_set;
    tango::ISetPtr dest_set = _dest_set;

    tango::IDynamicSetPtr dynset = source_set;
    if (dynset)
    {
        source_set = dynset->getBaseSet();
        dynset.clear();
    }


    wxString src_id = towx(source_set->getSetId());
    wxString dest_id = towx(dest_set->getSetId());


    ITableDocModelPtr src_model = TableDocMgr::loadModel(src_id);

    ITableDocObjectEnumPtr vec;
    vec = new xcm::IVectorImpl<ITableDocObjectPtr>;

    ITableDocWatchEnumPtr watches = src_model->getWatchEnum();
    ITableDocMarkEnumPtr marks = src_model->getMarkEnum();
    ITableDocViewEnumPtr views = src_model->getViewEnum();

    int i, count;

    count = watches->size();
    for (i = 0; i < count; ++i)
    {
        vec->append(watches->getItem(i));
    }

    count = marks->size();
    for (i = 0; i < count; ++i)
    {
        vec->append(marks->getItem(i));
    }

    count = views->size();
    for (i = 0; i < count; ++i)
    {
        vec->append(views->getItem(i));
    }

   
    // -- assign every object a new id --
    count = vec->size();
    for (i = 0; i < count; ++i)
    {
        ITableDocObjectPtr obj = vec->getItem(i);
        if (obj)
        {
            obj->setObjectId(getUniqueString());
            obj->setDirty(true);
        }
    }


    // -- delete any existing model in the destination slot --

    TableDocMgr::deleteModel(dest_id);


    // -- create new model and put objects in it --
    ITableDocModelPtr dest_model = TableDocMgr::loadModel(dest_id);
    dest_model->writeMultipleObjects(vec);


    src_model.clear();
    dest_model.clear();

    // -- released unused models --
    TableDocMgr::cleanupModelRegistry();
}




ITableDocModelPtr TableDocMgr::loadModel(const wxString& set_id)
{
    return g_model_registry.loadModel(set_id);
}

bool TableDocMgr::deleteModel(const wxString& set_id)
{
    return g_model_registry.deleteModel(set_id);
}

void TableDocMgr::clearModelRegistry()
{
    g_model_registry.clear();
}

void TableDocMgr::cleanupModelRegistry()
{
    g_model_registry.cleanup();
}


// this routine moves set info files (views, watches, marks) from their
// old position /.appdata/dcfe/<username>/... to the new location
// /.appdata/<username>/dcfe/setinfo/...  Other files such as relmgrpanel
// and jobscheduler are moved as well.

// After most people have upgraded from 2005.1, this routine may be
// removed.  This upgrade is not essential to the programs functionality

void TableDocMgr::upgradeFrom2005_1()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (!db)
        return;
        
    if (!db->getFileExist(L"/.appdata/dcfe"))
        return;
        
    if (db->getFileExist(L"/.appdata/dcfe/admin"))
    {
        tango::IFileInfoEnumPtr folder = db->getFolderInfo(L"/.appdata/dcfe/admin");
        
        if (!folder)
            return;
            
        if (!db->createFolder(L"/.appdata/admin/dcfe"))
            return;
        
        db->moveFile(L"/.appdata/dcfe/admin/relmgrpanel",
                     L"/.appdata/admin/dcfe/relmgrpanel");
        
        db->moveFile(L"/.appdata/dcfe/admin/jobscheduler",
                     L"/.appdata/admin/dcfe/jobscheduler");
                     
        size_t i, count = folder->size();
        for (i = 0; i < count; ++i)
        {
            tango::IFileInfoPtr fileinfo = folder->getItem(i);
            
            std::wstring name = fileinfo->getName();
            if (name == L"relmgrpanel" || name == L"jobscheduler")
                continue;
            
            
            std::wstring src, dest;
            src = L"/.appdata/dcfe/admin/";
            src += name;
            dest = L"/.appdata/admin/dcfe/setinfo/";
            dest += name;
            db->moveFile(src, dest);
        }
    }
    
    // clean up
    db->deleteFile(L"/.appdata/dcfe");
}

