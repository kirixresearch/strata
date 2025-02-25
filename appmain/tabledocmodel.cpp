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
#include "jsonconfig.h"
#include <kl/string.h>
#include <kl/md5.h>





static std::wstring g_tabledocmodel_path;

static std::wstring getTableMetadataLocation()
{
    if (!g_tabledocmodel_path.empty())
        return g_tabledocmodel_path;

    std::wstring path = towstr(g_app->getAppDataPath());
    path += PATH_SEPARATOR_CHAR;
    path += L"Metadata";

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    path += PATH_SEPARATOR_CHAR;
    path += L"Tables";

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    g_tabledocmodel_path = path;
    return path;
}





class ModelRegistry : public xcm::signal_sink
{

public:

    ModelRegistry()
    {
        m_deleted_flag = false;
    }

    ITableDocModelPtr lookupModel(const std::wstring& set_id)
    {
        KL_AUTO_LOCK(m_obj_mutex);

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
        KL_AUTO_LOCK(m_obj_mutex);

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

    bool releaseModel(const std::wstring& set_id)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        if (set_id.empty())
            return false;

        // first find the model and delete it from the vector
        std::vector<ITableDocModelPtr>::iterator it;
        for (it = m_models.begin(); it != m_models.end(); ++it)
        {
            if (set_id == (*it)->getId())
            {
                m_models.erase(it);
                return true;
            }
        }

        return false;
    }

    void cleanup()  // table model garbage collection
    {
        KL_AUTO_LOCK(m_obj_mutex);

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
        KL_AUTO_LOCK(m_obj_mutex);
        m_models.clear();
    }

private:

    void onTableDocModelDeleted(TableDocModel* model)
    {
        KL_AUTO_LOCK(m_obj_mutex);
        m_deleted_flag = true;
    }

private:

    kl::mutex m_obj_mutex;
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

    virtual ~TableDocObjectBase()
    {
    }

    const std::wstring& getObjectId()
    {
        return m_id;
    }

    void setObjectId(const std::wstring& id)
    {
        m_id = id;
    }

    std::wstring getHash()
    {
        return kl::md5str(m_id);
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




class TableDocMark : public TableDocObjectBase,
                     public ITableDocMark
                      
{
friend class XdGridModel;

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

    std::wstring getHash()
    {
        std::wstring hashsrc = m_description + L";";
        hashsrc += m_expression + L";";
        hashsrc += m_fgcolor.GetAsString() + L";";
        hashsrc += m_bgcolor.GetAsString() + L";";
        hashsrc += m_mark_active ? L"true" : L"false";

        return kl::md5str(hashsrc);
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

    std::wstring getHash()
    {
        std::wstring hashsrc = m_name + L";";
        kl::makeLower(hashsrc);
        hashsrc += kl::itowstring(m_size) + L";";
        hashsrc += m_fgcolor.GetAsString() + L";";
        hashsrc += m_bgcolor.GetAsString() + L";";
        hashsrc += kl::itowstring(m_alignment) + L";";
        hashsrc += kl::itowstring(m_text_wrap);

        return kl::md5str(hashsrc);
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
        m_text_wrap = node["text_wrap"].getInteger();
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

    std::wstring getHash()
    {
        std::wstring hashsrc = m_description + L";";
        hashsrc += kl::itowstring(m_row_size) + L";";
        for (unsigned int i = 0; i < m_cols.size(); ++i)
        {
            ITableDocObjectPtr obj = m_cols[i];
            hashsrc += obj->getHash() + L";";
        }

        return kl::md5str(hashsrc);
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

    int getColumnIdx(const std::wstring& col_name)
    {
        std::vector<ITableDocViewColPtr>::iterator it;
        int idx = 0;
        for (it = m_cols.begin(); it != m_cols.end(); ++it)
        {
            if (0 == wcscasecmp(col_name.c_str(), (*it)->getName().c_str()))
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
        node["description"] = m_description;
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
        m_description = node["description"].getString();
        m_row_size = node["row_size"].getInteger();
        kl::JsonNode columns = node["columns"];

        m_cols.clear();

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
        load();
}

bool TableDocModel::loadWithoutUpgrade()
{
    if (m_id.empty())
    {
        // it is a temporary model and is not loaded
        return true;
    }

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    loadJson();

    return true;
}

bool TableDocModel::load()
{
    if (m_id.empty())
    {
        // it is a temporary model and is not loaded
        return true;
    }

    if (!loadWithoutUpgrade())
    {
        return false;
    }

    return loadAndConvertOldVersionToNewJson();
}

bool TableDocModel::save()
{
    if (m_id.empty())
    {
        // it is a temporary model and is not saved to the store
        return true;
    }

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    return saveJson();
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
            (*vec)[i]->setDirty(true);
            break;
        }
    }
    if (!found)
    {
        ITableDocObjectPtr c = obj->clone();
        c->setDirty(true);
        vec->push_back(c);
    }


    if (save_to_store)
        save();

    obj->setDirty(false);

    return true;
}

bool TableDocModel::writeMultipleObjects(ITableDocObjectEnumPtr objs)
{
    KL_AUTO_LOCK(m_obj_mutex);

    if (m_id.empty())
        return false;

    size_t i, count = objs->size();

    size_t dirty_count = 0;

    for (i = 0; i < count; ++i)
    {
        if (objs->getItem(i)->getDirty())
            dirty_count++;
    }

    if (dirty_count == 0)
        return true;

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

    std::wstring delete_object_id = obj->getObjectId();

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
                if ((*it)->getObjectId() == delete_object_id)
                {
                    vec->erase(it);
                    break;
                }
            }
        }
    }

    if (std::find(m_to_delete.begin(), m_to_delete.end(), delete_object_id) == m_to_delete.end())
    {
        m_to_delete.push_back(delete_object_id);
    }

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
    KL_AUTO_LOCK(m_obj_mutex);

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
    KL_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<ITableDocViewPtr>* vec;
    vec = new xcm::IVectorImpl<ITableDocViewPtr>;

    std::vector<ITableDocObjectPtr>::iterator it;
    for (it = m_views.begin(); it != m_views.end(); ++it)
    {
        vec->append((*it)->clone());
    }

    return vec;
}

bool TableDocModel::saveJson()
{
    if (m_id.empty())
    {
        // the table doc model is temporary and in memory and we shouldn't do anything here
        return true;
    }

    xd::IDatabasePtr db = g_app->getDatabase();

    std::vector<ITableDocObjectPtr> marks = m_marks;
    std::vector<ITableDocObjectPtr> views = m_views;
    std::vector<std::wstring> to_delete = m_to_delete;

    m_marks.clear();
    m_views.clear();

    // we want to merge our changes with whatever already
    // exists in the json store; so first load what is there
    // (without loading older previously upgraded versions)
    loadWithoutUpgrade();


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

    m_to_delete.clear();

    // now merge existing and add new objects
    std::vector<ITableDocObjectPtr>::iterator vit;
    for (vit = views.begin(); vit != views.end(); ++vit)
    {
        if (!(*vit)->getDirty())
            continue;

        ITableDocObjectPtr newobj = (*vit)->clone();
        newobj->setDirty(false);

        bool found = false;
        size_t i;

        for (i = 0; i < m_views.size(); ++i)
        {
            if (m_views[i]->getObjectId() == (*vit)->getObjectId())
            {
                found = true;
                m_views[i] = newobj;
                break;
            }
        }

        if (!found)
        {
            m_views.push_back(newobj);
        }
    }

    std::vector<ITableDocObjectPtr>::iterator mit;
    for (mit = marks.begin(); mit != marks.end(); ++mit)
    {
        if (!(*mit)->getDirty())
             continue;

        ITableDocObjectPtr newobj = (*mit)->clone();
        newobj->setDirty(false);

        bool found = false;
        size_t i;

        for (i = 0; i < m_marks.size(); ++i)
        {
            if (m_marks[i]->getObjectId() == (*mit)->getObjectId())
            {
                found = true;
                m_marks[i] = newobj;
                break;
            }
        }

        if (!found)
        {
            m_marks.push_back(newobj);
        }
    }


    // now serialize the object collections to json
    kl::JsonNode node;

    kl::JsonNode metadata_node = node["metadata"];
    metadata_node["type"] = L"application/vnd.kx.tabledocmodel";
    metadata_node["version"] = 1;
    metadata_node["description"] = L"";

    kl::JsonNode views_node = node["views"];
    views_node.setArray();

    for (it = m_views.begin(); it != m_views.end(); ++it)
    {
        kl::JsonNode views_child_node = views_node.appendElement();
        (*it)->writeToNode(views_child_node);
    }

    kl::JsonNode marks_node = node["marks"];
    marks_node.setArray();

    for (it = m_marks.begin(); it != m_marks.end(); ++it)
    {
        kl::JsonNode marks_child_node = marks_node.appendElement();
        (*it)->writeToNode(marks_child_node);
    }


    if (g_app->getDbDriver() == L"xdnative")
    {
        // with xdnative, tabledoc model stores its metadata in streams
        std::wstring path = L"/.appdata/%usr%/tables/%id%";
        kl::replaceStr(path, L"%usr%", db->getActiveUid());
        kl::replaceStr(path, L"%id%", m_id);

        return JsonConfig::saveToDb(node, db, path);
    }
     else
    {
        // with direct connections to sql-type servers, store metadata locally
        std::wstring path = getTableMetadataLocation();
        path += PATH_SEPARATOR_CHAR;
        path += m_id + L".json";

        return JsonConfig::saveToFile(node, path);
    }
}




bool TableDocModel::loadJson()
{
    if (m_id.empty())
    {
        return false;
    }

    xd::IDatabasePtr db = g_app->getDatabase();
    kl::JsonNode node;

    if (g_app->getDbDriver() == L"xdnative")
    {
        // with xdnative, tabledoc model stores its metadata in streams
        std::wstring path = L"/.appdata/%usr%/tables/%id%";
        kl::replaceStr(path, L"%usr%", db->getActiveUid());
        kl::replaceStr(path, L"%id%", m_id);

        node = JsonConfig::loadFromDb(db, path);
        if (!node.isOk())
            return false;
    }
     else
    {
        // with direct connections to sql-type servers, store metadata locally
        std::wstring path = getTableMetadataLocation();
        path += PATH_SEPARATOR_CHAR;
        path += m_id + L".json";

        node = JsonConfig::loadFromFile(path);
        if (!node.isOk())
            return false;
    }

    // make sure the version we're loading is valid
    if (!isValidFileVersion(node, L"application/vnd.kx.tabledocmodel", 1))
        return false;

    m_views.clear();
    m_marks.clear();
    m_to_delete.clear();

    if (node.childExists("views"))
    {
        kl::JsonNode views_node = node["views"];

        std::vector<kl::JsonNode> children = views_node.getChildren();
        std::vector<kl::JsonNode>::iterator it;
        for (it = children.begin(); it != children.end(); ++it)
        {
            TableDocView* obj = new TableDocView;
            obj->readFromNode(*it);
            m_views.push_back(static_cast<ITableDocObject*>(obj));
        }
    }

    if (node.childExists("marks"))
    {
        kl::JsonNode marks_node = node["marks"];

        std::vector<kl::JsonNode> children = marks_node.getChildren();
        std::vector<kl::JsonNode>::iterator it;
        for (it = children.begin(); it != children.end(); ++it)
        {
            TableDocMark* obj = new TableDocMark;
            obj->readFromNode(*it);
            m_marks.push_back(static_cast<ITableDocObject*>(obj));
        }
    }

    return true;
}


bool TableDocModel::fromJson(const std::wstring& json)
{
    if (m_id.empty())
    {
        // the table doc model is temporary and in memory and we shouldn't do anything here
        return true;
    }

    xd::IDatabasePtr db = g_app->getDatabase();
    kl::JsonNode node;

    if (g_app->getDbDriver() == L"xdnative")
    {
        // with xdnative, tabledoc model stores its metadata in streams
        std::wstring path = L"/.appdata/%usr%/tables/%id%";
        kl::replaceStr(path, L"%usr%", db->getActiveUid());
        kl::replaceStr(path, L"%id%", m_id);

        db->deleteFile(path);

        if (!writeStreamTextFile(db, path, json))
            return false;
    }
     else
    {
        // with direct connections to sql-type servers, store metadata locally
        std::wstring path = getTableMetadataLocation();
        path += PATH_SEPARATOR_CHAR;
        path += m_id + L".json";

        if (!xf_remove(path))
            return false;

        if (!xf_put_file_contents(path, json))
            return false;
    }

    return loadJson();
}


std::wstring TableDocModel::toJson()
{
    saveJson();

    xd::IDatabasePtr db = g_app->getDatabase();
    kl::JsonNode node;

    if (g_app->getDbDriver() == L"xdnative")
    {
        // with xdnative, tabledoc model stores its metadata in streams
        std::wstring path = L"/.appdata/%usr%/tables/%id%";
        kl::replaceStr(path, L"%usr%", db->getActiveUid());
        kl::replaceStr(path, L"%id%", m_id);

        std::wstring json;

        if (!readStreamTextFile(db, path, json))
            return L"";
        
        return json;
    }
     else
    {
        // with direct connections to sql-type servers, store metadata locally
        std::wstring path = getTableMetadataLocation();
        path += PATH_SEPARATOR_CHAR;
        path += m_id + L".json";

        return xf_get_file_contents(path);
    }
}


bool TableDocModel::loadAndConvertOldVersionToNewJson()
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        // done if no database
        return true;
    }

    std::wstring view_path = kl::stdswprintf(L"/.appdata/admin/dcfe/setinfo/%s/views", m_id.c_str());
    std::wstring mark_path = kl::stdswprintf(L"/.appdata/admin/dcfe/setinfo/%s/marks", m_id.c_str());
    std::wstring upgrades_path = kl::stdswprintf(L"/.appdata/admin/dcfe/setinfo/%s/upgrades", m_id.c_str());

    // create a json object in the new format
    bool format_converted = false;
    kl::JsonNode node;

    // load the views -- merging them into the new format (if the object id doesn't already exist) 
    node = JsonConfig::loadFromDb(db, view_path);
    if (node.isOk())
    {
        kl::JsonNode root_node = node["root"];
        if (!root_node.isOk())
            return false;

        kl::JsonNode views_node = root_node["views"];
        if (!views_node.isOk())
            return false;

        std::vector<std::wstring> views_keys = views_node.getChildKeys();
        int old_view_counter = -1;
        for (auto view_key : views_keys)
        {
            old_view_counter++;

            kl::JsonNode old_view_format_node = views_node.getChild(view_key);
            if (!old_view_format_node.isOk())
                continue;

            // create a json node in the new format from the old old format
            kl::JsonNode new_view_format_node;
            new_view_format_node["object_id"].setString(L"");    // placeholder; object id will be set below

            kl::JsonNode old_view_format_description_node = old_view_format_node["description"];
            if (!old_view_format_description_node.isOk())
                continue;
            new_view_format_node["description"].setString(old_view_format_description_node.getString());

            kl::JsonNode old_view_format_row_size_node = old_view_format_node["row_size"];
            if (!old_view_format_row_size_node.isOk())
                continue;
            new_view_format_node["row_size"].setInteger(old_view_format_row_size_node.getInteger());

            kl::JsonNode old_view_format_columns_node = old_view_format_node["columns"];
            if (!old_view_format_columns_node.isOk())
                continue;
            new_view_format_node["columns"].setArray();

            std::vector<kl::JsonNode> column_children = old_view_format_node["columns"].getChildren();
            std::vector<kl::JsonNode>::iterator it_column, it_column_end;
            it_column_end = column_children.end();

            for (it_column = column_children.begin(); it_column != it_column_end; ++it_column)
            {
                kl::JsonNode old_view_format_column_child = *it_column;
                if (!old_view_format_column_child.isOk())
                    continue;
                kl::JsonNode old_view_format_column_name = old_view_format_column_child["name"];
                if (!old_view_format_column_name.isOk())
                    continue;
                kl::JsonNode old_view_format_column_size = old_view_format_column_child["size"];
                if (!old_view_format_column_size.isOk())
                    continue;
                kl::JsonNode old_view_format_column_fgcolor = old_view_format_column_child["fgcolor"];
                if (!old_view_format_column_fgcolor.isOk())
                    continue;
                kl::JsonNode old_view_format_column_bgcolor = old_view_format_column_child["bgcolor"];
                if (!old_view_format_column_bgcolor.isOk())
                    continue;
                kl::JsonNode old_view_format_column_alignment = old_view_format_column_child["alignment"];
                if (!old_view_format_column_alignment.isOk())
                    continue;
                kl::JsonNode old_view_format_column_textwrap = old_view_format_column_child["text_wrap"];
                if (!old_view_format_column_textwrap.isOk())
                    continue;

                kl::JsonNode new_view_format_column_child = new_view_format_node["columns"].appendElement();
                new_view_format_column_child["name"].setString(old_view_format_column_name.getString());
                new_view_format_column_child["size"].setInteger(old_view_format_column_size.getInteger());
                new_view_format_column_child["fgcolor"].setString(color2string(int2color(old_view_format_column_fgcolor.getInteger())));
                new_view_format_column_child["bgcolor"].setString(color2string(int2color(old_view_format_column_bgcolor.getInteger())));
                new_view_format_column_child["alignment"].setInteger(old_view_format_column_alignment.getInteger());
                new_view_format_column_child["text_wrap"].setInteger(old_view_format_column_textwrap.getInteger());
            }

            // add the view if it doesn't exist already
            TableDocView* viewobj = new TableDocView;
            ITableDocObjectPtr obj = viewobj;
            new_view_format_node["object_id"] = obj->getObjectId();
            obj->readFromNode(new_view_format_node);
            std::wstring hash = obj->getHash();
            if (!db->getFileExist(upgrades_path + L"/" + hash))
            {
                bool done = false;

                std::wstring default_view_caption = towstr(_("Default View"));
                if (viewobj->getDescription() == default_view_caption && old_view_counter == 0)
                {
                    if (m_views.size() > 0)
                    {
                        ITableDocViewPtr view = m_views[0];
                        if (view.isOk() && view->getDescription() == default_view_caption)
                        {
                            writeStreamTextFile(db, upgrades_path + L"/" + hash, L"1");

                            // The "Default View" (if it is in the first position) of the old format should
                            // always overwrite the default view (in the first position) of the new format
                            std::wstring obj_id = m_views[0]->getObjectId();
                            m_views[0]->readFromNode(new_view_format_node);
                            m_views[0]->setObjectId(obj_id);
                            m_views[0]->setDirty(true);
                            done = true;
                            format_converted = true;
                        }
                    }
                }

                if (!done)
                {
                    writeStreamTextFile(db, upgrades_path + L"/" + hash, L"1");
                    obj->setDirty(true);
                    m_views.push_back(obj);
                    format_converted = true;
                }
            }
        }
    }

    // load the marks
    node = JsonConfig::loadFromDb(g_app->getDatabase(), mark_path);
    if (node.isOk())
    {
        kl::JsonNode root_node = node["root"];
        if (!root_node.isOk())
            return false;

        kl::JsonNode marks_node = root_node["marks"];
        if (!marks_node.isOk())
            return false;

        std::vector<kl::JsonNode> marks_children = marks_node.getChildren();
        std::vector<kl::JsonNode>::iterator it_mark, it_mark_end;
        it_mark_end = marks_children.end();

        for (it_mark = marks_children.begin(); it_mark != it_mark_end; ++it_mark)
        {
            kl::JsonNode old_mark_format_node = *it_mark;
            if (!old_mark_format_node.isOk())
                continue;

            // create a json node in the new format from the old old format
            kl::JsonNode new_mark_format_node;
            new_mark_format_node["object_id"].setString(L"");    // placeholder; object id will be set below

            kl::JsonNode old_mark_format_description_node = old_mark_format_node["description"];
            if (!old_mark_format_description_node.isOk())
                continue;
            new_mark_format_node["description"].setString(old_mark_format_description_node.getString());

            kl::JsonNode old_mark_format_expression_node = old_mark_format_node["expression"];
            if (!old_mark_format_expression_node.isOk())
                continue;
            new_mark_format_node["expression"].setString(old_mark_format_expression_node.getString());

            kl::JsonNode old_mark_format_mark_active_node = old_mark_format_node["mark_active"];
            if (!old_mark_format_mark_active_node.isOk())
                continue;
            new_mark_format_node["mark_active"].setBoolean(old_mark_format_mark_active_node.getInteger() != 0 ? true : false);

            kl::JsonNode old_mark_format_mark_fgcolor_node = old_mark_format_node["mark_fgcolor"];
            if (!old_mark_format_mark_fgcolor_node.isOk())
                continue;
            new_mark_format_node["fgcolor"].setString(color2string(int2color(old_mark_format_mark_fgcolor_node.getInteger())));

            kl::JsonNode old_mark_format_mark_bgcolor_node = old_mark_format_node["mark_bgcolor"];
            if (!old_mark_format_mark_bgcolor_node.isOk())
                continue;
            new_mark_format_node["bgcolor"].setString(color2string(int2color(old_mark_format_mark_bgcolor_node.getInteger())));

            // load the json node from the new format
            TableDocMark* markobj = new TableDocMark;
            ITableDocObjectPtr obj = markobj;
            new_mark_format_node["object_id"] = obj->getObjectId();
            obj->readFromNode(new_mark_format_node);
            std::wstring hash = obj->getHash();
            if (!db->getFileExist(upgrades_path + L"/" + hash))
            {
                writeStreamTextFile(db, upgrades_path + L"/" + hash, L"1");
                obj->setDirty(true);
                m_marks.push_back(static_cast<ITableDocObject*>(obj));
                format_converted = true;
            }
        }
    }

    // old view or mark formats were converted
    if (format_converted)
    {   
        save();
    }

    return true;
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
    int count = vec->size();

    if (count == 0)
        return DEFAULT_MARK_COLORS[0];

    // track which colors are currently used
    std::vector<bool> colors_used(DEFAULT_MARK_COLOR_COUNT, false);

    // mark which colors are already used in existing marks
    for (int i = 0; i < count; ++i)
    {
        ITableDocMarkPtr vecmark = vec->getItem(i);
        wxColour cur_color = vecmark->getBackgroundColor();

        for (int j = 0; j < DEFAULT_MARK_COLOR_COUNT; ++j)
        {
            if (cur_color == DEFAULT_MARK_COLORS[j])
            {
                colors_used[j] = true;
                break;
            }
        }
    }

    // find the first available color not yet used
    for (int j = 0; j < DEFAULT_MARK_COLOR_COUNT; ++j)
    {
        if (!colors_used[j])
        {
            return DEFAULT_MARK_COLORS[j];
        }
    }

    // if all colors are used, go to the next one based on the last used color
    ITableDocMarkPtr last_mark = vec->getItem(count - 1);
    wxColour last_color = last_mark->getBackgroundColor();

    for (int j = 0; j < DEFAULT_MARK_COLOR_COUNT; ++j)
    {
        if (last_color == DEFAULT_MARK_COLORS[j])
        {
            return DEFAULT_MARK_COLORS[(j + 1) % DEFAULT_MARK_COLOR_COUNT];
        }
    }

    // fallback in case of an unexpected issue
    return DEFAULT_MARK_COLORS[0];
}



// TableDocMgr class implementation

bool TableDocMgr::newFile(const std::wstring& _path)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    std::wstring path = xd::dequoteIdentifier(db, _path);
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

void TableDocMgr::copyModel(const std::wstring& src_id, const std::wstring& dest_id)
{
    // load and clone all objects from the source model
    ITableDocModelPtr src_model = TableDocMgr::loadModel(src_id);
    if (src_model.isNull())
    {
        return;
    }

    ITableDocObjectEnumPtr vec;
    vec = new xcm::IVectorImpl<ITableDocObjectPtr>;

    ITableDocMarkEnumPtr marks = src_model->getMarkEnum();
    ITableDocViewEnumPtr views = src_model->getViewEnum();

    int i, count;

    count = marks->size();
    for (i = 0; i < count; ++i)
    {
        ITableDocObjectPtr o = marks->getItem(i);
        if (o.isOk())
        {
            vec->append(o->clone());
        }
    }

    count = views->size();
    for (i = 0; i < count; ++i)
    {
        ITableDocObjectPtr o = views->getItem(i);
        if (o.isOk())
        {
            vec->append(o->clone());
        }
    }

    // "close" source model
    src_model.clear();

    // assign every object a new id
    count = vec->size();
    for (i = 0; i < count; ++i)
    {
        ITableDocObjectPtr obj = vec->getItem(i);
        obj->setObjectId(kl::getUniqueString());
        obj->setDirty(true);
    }

    // delete any existing model in the destination slot
    TableDocMgr::deleteModel(dest_id);

    // create new model and put objects in it
    ITableDocModelPtr dest_model = TableDocMgr::loadModel(dest_id);
    dest_model->writeMultipleObjects(vec);
    dest_model.clear();

    // released unused models
    TableDocMgr::cleanupModelRegistry();
}

ITableDocModelPtr TableDocMgr::loadModel(const std::wstring& set_id)
{
    // if the set_id is empty, we create a temporary table doc model;
    // these are not tracked by the model registry and are not saved to the database
    if (set_id.empty())
    {
        TableDocModel* model = new TableDocModel;
        model->init(L"");

        return static_cast<ITableDocModel*>(model);
    }


    return g_model_registry.loadModel(set_id);
}

bool TableDocMgr::deleteModel(const std::wstring& set_id)
{
    if (set_id.empty())
    {
        // the table doc model is temporary and in memory and we shouldn't do anything here
        return true;
    }

    g_model_registry.releaseModel(set_id);

    
    xd::IDatabasePtr db = g_app->getDatabase();
    kl::JsonNode node;

    if (g_app->getDbDriver() == L"xdnative")
    {
        // with xdnative, tabledoc model stores its metadata in streams
        std::wstring path = L"/.appdata/%usr%/tables/%id%";
        kl::replaceStr(path, L"%usr%", db->getActiveUid());
        kl::replaceStr(path, L"%id%", set_id);

        return db->deleteFile(path);
    }
     else
    {
        // with direct connections to sql-type servers, store metadata locally
        std::wstring path = getTableMetadataLocation();
        path += PATH_SEPARATOR_CHAR;
        path += set_id + L".json";

        return xf_remove(path);
    }
}

void TableDocMgr::clearModelRegistry()
{
    g_model_registry.clear();
}

void TableDocMgr::cleanupModelRegistry()
{
    g_model_registry.cleanup();
}

