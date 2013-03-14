/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
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
#include <kl/string.h>


class ModelRegistry : public xcm::signal_sink
{

public:

    ITableDocModelPtr lookupModel(const std::wstring& set_id)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        std::vector<ITableDocModelPtr>::iterator it;
        for (it = m_models.begin(); it != m_models.end(); ++it)
        {
            if ((*it)->getId() == set_id)
                return *it;
        }

        return xcm::null;
    }

    ITableDocModelPtr loadModel(const std::wstring& set_id)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        if (set_id.empty())
        {
            TableDocModel* model = new TableDocModel;
            model->init(L"");

            return static_cast<ITableDocModel*>(model);
        }

        ITableDocModelPtr model = lookupModel(set_id);
        if (model.isOk())
            return model;

        // model needs to be loaded
        TableDocModel* modelp = new TableDocModel;
        modelp->init(set_id);
        modelp->sigDeleted.connect(this, &ModelRegistry::onTableDocModelDeleted);
        model = modelp;

        m_models.push_back(modelp);

        return modelp;
    }

    bool deleteModel(const std::wstring& set_id)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        if (set_id.empty())
            return false;

        // first find the model and delete it from the vector
        std::vector<ITableDocModelPtr>::iterator it;
        for (it = m_models.begin(); it != m_models.end(); ++it)
        {
            if (set_id == (*it)->getId())
            {
                m_models.erase(it);
                break;
            }
        }


        // next attempt to delete the disk files
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
        m_id = kl::getUniqueString();
        m_dirty = false;
    }

    const std::wstring& getObjectId()
    {
        return m_id;
    }

    void setObjectId(const std::wstring& id)
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
    std::wstring m_id;
};




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

    void setDescription(const std::wstring& new_val)
    {
        if (m_description == new_val)
            return;

        m_description = new_val;
        setDirty(true);
    }

    std::wstring getDescription()
    {
        return m_description;
    }



    void setExpression(const std::wstring& new_val)
    {
        if (m_expression == new_val)
            return;
            
        m_expression = new_val;
        setDirty(true);
    }
    
    std::wstring getExpression()
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

    bool writeToNode(kl::JsonNode& node)
    {
        node["object_id"] = m_id;
        node["description"] = m_description;
        node["expression"] = m_expression;
        node["mark_active"].setBoolean(m_mark_active);
        node["fgcolor"] = color2string(m_fgcolor);
        node["bgcolor"] = color2string(m_bgcolor);
        return true;
    }

    bool readFromNode(kl::JsonNode& node)
    {
        m_id = node["object_id"].getString();
        m_description = node["description"].getString();
        m_expression = node["expression"].getString();
        m_mark_active = node["mark_active"].getBoolean();
        m_fgcolor = string2color(node["fgcolor"].getString());
        m_bgcolor = string2color(node["bgcolor"].getString());
        return true;
    }



    bool writeToNode(tango::INodeValuePtr node)
    {
        return false;
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
                
        return true;
    }

private:

    bool m_mark_active;
    wxColor m_fgcolor;
    wxColor m_bgcolor;
    std::wstring m_description;
    std::wstring m_expression;
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

    void setName(const std::wstring& new_value)
    {
        if (m_name == new_value)
            return;

        m_name = new_value;
        setDirty(true);
    }

    std::wstring getName()
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


    bool readFromNode(kl::JsonNode& node)
    {
        m_name = node["name"].getString();
        m_size = node["size"].getInteger();
        m_fgcolor = string2color(node["fgcolor"].getString());
        m_bgcolor = string2color(node["bgcolor"].getString());
        m_alignment = node["alignment"].getInteger();
        return true;
    }

    bool writeToNode(kl::JsonNode& node)
    {
        node["name"] = m_name;
        node["size"] = m_size;
        node["fgcolor"] = color2string(m_fgcolor);
        node["bgcolor"] = color2string(m_bgcolor);
        node["alignment"] = m_alignment;
        node["text_wrap"] = m_text_wrap;
        return true;
    }


    bool writeToNode(tango::INodeValuePtr node)
    {
        return false;
    }

    bool readFromNode(tango::INodeValuePtr node)
    {
        tango::INodeValuePtr col_name = node->getChild(L"name", false);
        if (!col_name)
            return false;
        m_name = towx(col_name->getString());
        
        tango::INodeValuePtr col_size = node->getChild(L"size", false);
        if (!col_size)
            return false;
        m_size = col_size->getInteger();

        tango::INodeValuePtr col_fgcolor = node->getChild(L"fgcolor", false);
        if (!col_fgcolor)
            return false;
        m_fgcolor = int2color(col_fgcolor->getInteger());

        tango::INodeValuePtr col_bgcolor = node->getChild(L"bgcolor", false);
        if (!col_bgcolor)
            return false;
        m_bgcolor = int2color(col_bgcolor->getInteger());

        tango::INodeValuePtr col_alignment = node->getChild(L"alignment", false);
        if (col_alignment)
            m_alignment = col_alignment->getInteger();
             else
            m_alignment = tabledocAlignDefault;

        tango::INodeValuePtr col_textwrap = node->getChild(L"text_wrap", false);
        if (col_textwrap)
            m_text_wrap = col_alignment->getInteger();
             else
            m_text_wrap = tabledocWrapDefault;

        return true;
    }

private:
    
    std::wstring m_name;
    int m_size;
    wxColor m_fgcolor;
    wxColor m_bgcolor;
    int m_alignment;
    int m_text_wrap;
};





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

    std::wstring getDescription()
    {
        return m_description;
    }

    void setDescription(const std::wstring& new_val)
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


    bool writeToNode(kl::JsonNode& node)
    {
        node["object_id"] = m_id;
        node["description"] = towstr(m_description);
        node["row_size"] = m_row_size;
        node["columns"].setArray();
        kl::JsonNode columns = node["columns"];

        std::vector<ITableDocViewColPtr>::iterator col_it;
        for (col_it = m_cols.begin(); col_it != m_cols.end(); ++col_it)
        {
            kl::JsonNode element = columns.appendElement();
            ITableDocObjectPtr obj = *col_it;
            if (!obj->writeToNode(element))
                return false;
        }

        return true;
    }

    bool readFromNode(kl::JsonNode& node)
    {
        m_id = node["object_id"].getString();
        m_description = towx(node["description"].getString());
        m_row_size = node["row_size"].getInteger();
        kl::JsonNode columns = node["columns"];

        size_t i, child_count = columns.getChildCount();
        for (i = 0; i < child_count; i++)
        {
            kl::JsonNode col = columns[i];

            TableDocViewCol* viewcol = new TableDocViewCol;
            viewcol->ref();
            if (viewcol->readFromNode(col))
                m_cols.push_back(static_cast<ITableDocViewCol*>(viewcol));
            viewcol->unref();
        }

        return true;
    }



    bool writeToNode(tango::INodeValuePtr node)
    {
        return false;
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
            m_row_size = rowsize_node->getInteger();
             else
            m_row_size = -1;

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

    std::wstring m_description;
    std::vector<ITableDocViewColPtr> m_cols;
    int m_row_size;
};


TableDocModel::TableDocModel()
{
}

TableDocModel::~TableDocModel()
{
    sigDeleted(this);
}

const std::wstring& TableDocModel::getId()
{
    return m_id;
}

void TableDocModel::init(const std::wstring& id)
{
    m_id = id;

    if (id.length() > 0)
    {
        upgradeOldVersionIfNecessary(id);

        load();
    }
}

bool TableDocModel::load()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    std::wstring filename = L"/.appdata/%usr%/tabledocmodel/%id%";
    kl::replaceStr(filename, L"%usr%", db->getActiveUid());
    kl::replaceStr(filename, L"%id%", m_id);
    
    std::wstring contents;

    if (!readStreamTextFile(db, filename, contents))
        return false;

    kl::JsonNode root;
    if (!root.fromString(contents))
        return false;

    m_views.clear();
    m_marks.clear();
    m_to_delete.clear();

    if (root.childExists("views"))
    {
        kl::JsonNode views_root = root["views"];

        size_t i, child_count = views_root.getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            TableDocView* obj = new TableDocView;
            obj->readFromNode(views_root[i]);
            m_views.push_back(static_cast<ITableDocObject*>(obj));
        }
    }

    if (root.childExists("marks"))
    {
        kl::JsonNode marks_root = root["marks"];

        size_t i, child_count = marks_root.getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            TableDocMark* obj = new TableDocMark;
            obj->readFromNode(marks_root[i]);
            m_marks.push_back(static_cast<ITableDocObject*>(obj));
        }
    }

    return true;
}

bool TableDocModel::save()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    std::wstring filename = L"/.appdata/%usr%/tabledocmodel/%id%";
    kl::replaceStr(filename, L"%usr%", db->getActiveUid());
    kl::replaceStr(filename, L"%id%", m_id);
    


    std::vector<ITableDocObjectPtr> marks = m_marks;
    std::vector<ITableDocObjectPtr> views = m_views;
    std::vector<std::wstring> to_delete = m_to_delete;

    m_marks.clear();
    m_views.clear();

    load();


    // first, remove all objects slated for deletion
    std::vector<ITableDocObjectPtr>::iterator it;
    std::vector<std::wstring>::iterator dit;

    for (dit = to_delete.begin(); dit != to_delete.end(); ++dit)
    {
        for (it = m_views.begin(); it != m_views.end(); ++it)
        {
            if ((*it)->getObjectId() == *dit)
            {
                m_views.erase(it);
                break;
            }
        }

        for (it = m_marks.begin(); it != m_marks.end(); ++it)
        {
            if ((*it)->getObjectId() == *dit)
            {
                m_marks.erase(it);
                break;
            }
        }
    }


    // now merge existing and add new objects
    std::vector<ITableDocObjectPtr>::iterator vit;
    for (vit = views.begin(); vit != views.end(); ++vit)
    {
        bool found = false;
        size_t i;

        for (i = 0; i < m_views.size(); ++i)
        {
            if (m_views[i]->getObjectId() == (*vit)->getObjectId())
            {
                found = true;
                m_views[i] = (*vit)->clone();
                break;
            }
        }
        if (!found)
            m_views.push_back((*vit)->clone());
    }


    std::vector<ITableDocObjectPtr>::iterator mit;
    for (mit = marks.begin(); mit != marks.end(); ++mit)
    {
        bool found = false;
        size_t i;

        for (i = 0; i < m_marks.size(); ++i)
        {
            if (m_marks[i]->getObjectId() == (*mit)->getObjectId())
            {
                found = true;
                m_marks[i] = (*mit)->clone();
                break;
            }
        }
        if (!found)
            m_marks.push_back((*mit)->clone());
    }


    // now serialize the object collections to json

    kl::JsonNode root;
    kl::JsonNode views_root = root["views"];
    views_root.setArray();

    for (it = m_views.begin(); it != m_views.end(); ++it)
    {
        kl::JsonNode node = views_root.appendElement();
        (*it)->writeToNode(node);
    }


    kl::JsonNode marks_root = root["marks"];
    marks_root.setArray();

    for (it = m_marks.begin(); it != m_marks.end(); ++it)
    {
        kl::JsonNode node = marks_root.appendElement();
        (*it)->writeToNode(node);
    }


    std::wstring contents = root.toString();

    return writeStreamTextFile(db, filename, contents);
}

bool TableDocModel::writeObject(ITableDocObjectPtr obj, bool save_to_store)
{
    if (!obj->getDirty())
        return true;

    ITableDocViewPtr view = obj;
    ITableDocMarkPtr mark = obj;

    std::vector<ITableDocObjectPtr>* vec = NULL;

    if (view.isOk())
        vec = &m_views;
    else if (mark.isOk())
        vec = &m_marks;
    else 
        return false;


    bool found = false;
    size_t i;

    for (i = 0; i < vec->size(); ++i)
    {
        if ((*vec)[i]->getObjectId() == obj->getObjectId())
        {
            found = true;
            (*vec)[i] = obj->clone();
            break;
        }
    }
    if (!found)
        vec->push_back(obj->clone());

    if (save_to_store)
        save();

    return true;
}

bool TableDocModel::writeMultipleObjects(ITableDocObjectEnumPtr objs)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    if (m_id.empty())
        return false;

    size_t i, count = objs->size();

    for (i = 0; i < count; ++i)
    {
        writeObject(objs->getItem(i), false);
    }

    return save();
}


bool TableDocModel::deleteObject(ITableDocObjectPtr obj)
{
    ITableDocViewPtr view = obj;
    ITableDocMarkPtr mark = obj;

    // update our cached version
    {
        std::vector<ITableDocObjectPtr>* vec = NULL;

        if (view.isOk())
            vec = &m_views;
        else if (mark.isOk())
            vec = &m_marks;

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

    m_to_delete.push_back(obj->getObjectId());
    save();

    return true;
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


ITableDocObjectPtr TableDocModel::lookupObject(const std::wstring& id)
{
    std::vector<ITableDocObjectPtr>::iterator it;
    for (it = m_views.begin(); it != m_views.end(); ++it)
    {
        if ((*it)->getObjectId() == id)
            return *it;
    }

    for (it = m_marks.begin(); it != m_marks.end(); ++it)
    {
        if ((*it)->getObjectId() == id)
            return *it;
    }

    return xcm::null;
}


ITableDocMarkEnumPtr TableDocModel::getMarkEnum()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<ITableDocMarkPtr>* vec;
    vec = new xcm::IVectorImpl<ITableDocMarkPtr>;

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





static tango::INodeValuePtr getOldVersionObjects(const std::wstring& id, const std::wstring& object_type)
{
    if (id.length() == 0)
        return xcm::null;

    std::wstring path = kl::stdswprintf(L"/.appdata/admin/dcfe/setinfo/%s/%s", id.c_str(), object_type.c_str());

    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return xcm::null;

    tango::INodeValuePtr file = db->openNodeFile(towstr(path));
    
    if (file.isNull())
        return xcm::null;

    return file->getChild(towstr(object_type), true);
}


void TableDocModel::upgradeOldVersionIfNecessary(const std::wstring& id)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    std::wstring path;


    path = kl::stdswprintf(L"/.appdata/admin/dcfe/setinfo/%s/views", id.c_str());

    if (!db->getFileExist(path))
        return;

    tango::INodeValuePtr base_node;
    
    base_node = getOldVersionObjects(id, L"views");
    if (base_node.isOk())
    {
        size_t i, child_count = base_node->getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            TableDocView* obj = new TableDocView;
            tango::INodeValuePtr node = base_node->getChildByIdx(i);
            obj->setObjectId(node->getName());
            obj->readFromNode(node);
            m_views.push_back(static_cast<ITableDocObject*>(obj));
        }
    }
    base_node.clear();


    base_node = getOldVersionObjects(id, L"marks");
    if (base_node.isOk())
    {
        size_t i, child_count = base_node->getChildCount();
        for (i = 0; i < child_count; ++i)
        {
            TableDocMark* obj = new TableDocMark;
            tango::INodeValuePtr node = base_node->getChildByIdx(i);
            obj->setObjectId(node->getName());
            obj->readFromNode(node);
            m_marks.push_back(static_cast<ITableDocObject*>(obj));
        }
    }
    base_node.clear();


    path = kl::stdswprintf(L"/.appdata/admin/dcfe/setinfo/%s", id.c_str());
    db->deleteFile(path);

    save();
}




// TableDocMgr class implementation

bool TableDocMgr::newFile(const std::wstring& _path)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    std::wstring path = tango::dequoteIdentifier(db, _path);
    std::wstring sql = L"CREATE TABLE [";
    sql += path;
    sql += L"] (field1 VARCHAR(40))";
    
    xcm::IObjectPtr result;
    return db->execute(sql, 0, result, NULL);
}

ITableDocPtr TableDocMgr::createTableDoc()
{
    return static_cast<ITableDoc*>(new TableDoc);
}

ITableDocPtr TableDocMgr::getActiveTableDoc(int* site_id)
{
    IDocumentSitePtr active_site;
    
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
    // create a new model for the result set
    tango::ISetPtr source_set = _src_set;
    tango::ISetPtr dest_set = _dest_set;

    tango::IDynamicSetPtr dynset = source_set;
    if (dynset)
    {
        source_set = dynset->getBaseSet();
        dynset.clear();
    }


    std::wstring src_id = source_set->getSetId();
    std::wstring dest_id = dest_set->getSetId();


    ITableDocModelPtr src_model = TableDocMgr::loadModel(src_id);

    ITableDocObjectEnumPtr vec;
    vec = new xcm::IVectorImpl<ITableDocObjectPtr>;

    ITableDocMarkEnumPtr marks = src_model->getMarkEnum();
    ITableDocViewEnumPtr views = src_model->getViewEnum();

    int i, count;

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

   
    // assign every object a new id
    count = vec->size();
    for (i = 0; i < count; ++i)
    {
        ITableDocObjectPtr obj = vec->getItem(i);
        if (obj)
        {
            obj->setObjectId(kl::getUniqueString());
            obj->setDirty(true);
        }
    }


    // delete any existing model in the destination slot
    TableDocMgr::deleteModel(dest_id);

    // create new model and put objects in it
    ITableDocModelPtr dest_model = TableDocMgr::loadModel(dest_id);
    dest_model->writeMultipleObjects(vec);

    src_model.clear();
    dest_model.clear();

    // released unused models
    TableDocMgr::cleanupModelRegistry();
}




ITableDocModelPtr TableDocMgr::loadModel(const std::wstring& set_id)
{
    return g_model_registry.loadModel(set_id);
}

bool TableDocMgr::deleteModel(const std::wstring& set_id)
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


















