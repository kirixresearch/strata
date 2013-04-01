/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-01
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/portable.h>
#include <kl/math.h>
#include <algorithm>
#include "tango.h"
#include "database.h"
#include "baseset.h"
#include "baseiterator.h"
#include "util.h"
#include "stdset.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"


#ifdef _MSC_VER
#include <float.h>
#define isnan _isnan
#define finite _finite
#endif


#ifdef __APPLE__
#define isnan std::isnan
#endif


const int max_recursion_depth = 15;

const std::string empty_string = "";
const std::wstring empty_wstring = L"";



struct ParserBindInfo
{
    ParserBindInfo(BaseIterator* _iter, const std::wstring& _column)
                   : column(_column), iter(_iter), max_scale(0) { }
    std::wstring column;
    BaseIterator* iter;
    int max_scale;
};


DataAccessInfo::DataAccessInfo()
{
    // initialize all member variables
    dai_text = L"";
    name = L"";
    expr_text = L"";
    type = tango::typeInvalid;
    offset = 0;
    width = 0;
    scale = 0;
    tableord = 0;
    ordinal = 0xfdfdfdfd;
    nulls_allowed = false;
    expr = NULL;
    result = NULL;
    it = NULL;
    is_active = true;
    is_column = true;
    depth = 0;
    raw_buf = NULL;
    key_layout = NULL;
}

DataAccessInfo::~DataAccessInfo()
{
    delete expr;
    delete key_layout;
    delete result;
    delete[] raw_buf;
}


bool DataAccessInfo::eval(kscript::Value* result)
{
    // this prevents mutual recursion (when two
    //  dynamic fields reference each other

    if (++depth == max_recursion_depth)
    {
        //depth = 0;
        --depth;
        return false;
    }

    bool res = expr->eval(result);

    depth--;

    return res;
}







BaseIterator::BaseIterator()
{
    m_database = xcm::null;
    m_dbi = xcm::null;
    m_set = xcm::null;
    m_rowptr = NULL;
    m_row_dirty = false;
    m_iter_flags = 0;
    m_rowid = 0;
}

BaseIterator::~BaseIterator()
{
    // this has to be done before clearDAI(), otherwise
    // handles will get released twice, causing a crash
    
    std::vector<BaseIteratorRelInfo>::iterator r_it;
    for (r_it = m_relations.begin(); r_it != m_relations.end(); ++r_it)
        delete r_it->kl;
    
    clearDAI();

    std::vector<BaseIteratorTableEntry>::iterator it;
    for (it = m_tables.begin(); it != m_tables.end(); ++it)
        delete[] it->row_buf;

    m_tables.clear();

    if (m_set_internal)
    {
        m_set_internal->removeEventHandler(this);
    }
}


void BaseIterator::clearDAI()
{
    std::vector<DataAccessInfo*>::iterator dai_it, dai_it_end;
    dai_it_end = m_dai_entries.end();
    for (dai_it = m_dai_entries.begin(); dai_it != dai_it_end; ++dai_it)
    {
        delete (*dai_it);
        *dai_it = NULL;
    }

    m_dai_entries.clear();
    m_dai_lookup.clear();
}



void BaseIterator::refreshDAI()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    // make a copy, because m_dai_entries may be modified
    // by subsequent calls to colinfo2dai

    std::vector<DataAccessInfo*> dai_entries = m_dai_entries;
    std::vector<tango::IColumnInfoPtr>::iterator cit;

    // find out if anything changed in our existing dai entries
    std::vector<DataAccessInfo*>::iterator dai_it;
    tango::IColumnInfoPtr colinfo;
    for (dai_it = dai_entries.begin();
         dai_it != dai_entries.end();
         ++dai_it)
    {
        //if (!(*dai_it)->is_active)
        //    continue;

        // does it still exist?
        
        colinfo.clear();

        for (cit = m_calc_fields.begin();
             cit != m_calc_fields.end();
             ++cit)
        {
            if (!wcscasecmp((*cit)->getName().c_str(),
                            (*dai_it)->name.c_str()))
            {
                colinfo = *cit;
                break;
            }
        }

     
        // Ben to self: Should I add this?
/*
        if (colinfo.isNull() && m_iter_structure.isOk())
        {
            colinfo = m_iter_structure->getColumnInfo((*dai_it)->name);
        }
*/
        if (colinfo.isNull())
        {
            colinfo = m_set_structure->getColumnInfo((*dai_it)->name);
        }

        if (colinfo.isNull())
        {
            (*dai_it)->is_active = false;
            m_dai_lookup.erase((*dai_it)->name);
            continue;
        }

        // have the parameters changed?
        if ((*dai_it)->type != colinfo->getType() ||
            (*dai_it)->offset != colinfo->getOffset() ||
            (*dai_it)->width != colinfo->getWidth() ||
            (*dai_it)->scale != colinfo->getScale() ||
            (*dai_it)->tableord != colinfo->getTableOrdinal() ||
            (*dai_it)->expr_text != colinfo->getExpression())
        {
            colinfo2dai(*dai_it, colinfo.p);
        }
    }
}


DataAccessInfo* BaseIterator::lookupDAI(const std::wstring& expr)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::map<std::wstring, DataAccessInfo*, kl::cmp_nocase>::iterator it;
    it = m_dai_lookup.find(expr);

    if (it == m_dai_lookup.end())
    {
        return NULL;
    }

    //if (!it->second->is_active)
    //    return NULL;

    return it->second;
}



bool BaseIterator::baseClone(BaseIterator* new_iter)
{
    if (!new_iter->init(m_database, m_set, m_columns))
    {
        return false;
    }

    return true;
}


bool BaseIterator::init(tango::IDatabase* database,
                        tango::ISet* set,
                        const std::wstring& columns)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    // retrieve and store the IDatabaseInternal pointer
    m_database = database;
    m_dbi = m_database;

    // store the set
    m_set = set;
    m_set_internal = set;

    if (columns.empty())
    {
        m_columns = L"*";
    }
     else
    {
        m_columns = columns;
        kl::trim(m_columns);
    }

    if (!initStructure())
        return false;

    // register to get set events
    m_set_internal->addEventHandler(this);

    return true;
}

void BaseIterator::setSet(tango::ISetPtr set)
{
    if (m_set_internal)
    {
        m_set_internal->removeEventHandler(this);
    }

    m_set = set;
    m_set_internal = set;

    // register to get set events
    if (m_set_internal)
    {
        m_set_internal->addEventHandler(this);
    }
}



BaseIteratorTableEntry* BaseIterator::registerTable(ITablePtr tbl)
{
    if (tbl.isNull())
        return NULL;

    BaseIteratorTableEntry e;
    e.table_ord = tbl->getTableOrdinal();
    e.table_ptr = tbl;
    e.row_width = tbl->getRowWidth();
    e.row_buf = new unsigned char[e.row_width];
    m_tables.push_back(e);
    return &m_tables[m_tables.size()-1];
}



void BaseIterator::setRowId(const tango::rowid_t& rowid)
{
    m_rowid = rowid;
}


tango::rowid_t BaseIterator::getRowId()
{
    return m_rowid;
}


void BaseIterator::goRow(const tango::rowid_t& rowid)
{
    // find the table entry

    tango::tableord_t table_ord = rowidGetTableOrd(rowid);
    tango::rowpos_t row_pos = rowidGetRowPos(rowid);
    BaseIteratorTableEntry* te = NULL;

    std::vector<BaseIteratorTableEntry>::iterator it;
    for (it = m_tables.begin(); it != m_tables.end(); ++it)
    {
        if (table_ord == it->table_ord)
        {
            te = &(*it);
            break;
        }
    }

    if (!te)
    {
        // get the table if we don't already have it
        te = registerTable(m_dbi->openTableByOrdinal(table_ord));
    }

    if (!te)
    {
        m_rowptr = NULL;
        m_rowid = 0;
        return;
    }
    
    // check if the row width changed
    if (te->table_ptr.p->getRowWidth() != te->row_width)
    {
        te->row_width = te->table_ptr.p->getRowWidth();
        delete[] te->row_buf;

        te->row_buf = new unsigned char[te->row_width];
    }

    te->table_ptr.p->getRow(row_pos, te->row_buf);
    m_rowptr = te->row_buf;
    setRowId(rowid);
}





static tango::ISetPtr createEofSet(tango::IDatabasePtr& db,
                                   tango::ISetPtr& set)
{
    // return empty set
    EofSet* eof_set = new EofSet(db);
    eof_set->ref();
    if (!eof_set->create(set))
    {
        eof_set->unref();
        return xcm::null;
    }
    return tango::ISetPtr(static_cast<tango::ISet*>(eof_set), false);
}


bool BaseIterator::refreshRelInfo(BaseIteratorRelInfo& info)
{
    // free old expression
    if (info.kl)
    {
        delete info.kl;
        info.kl = NULL;
    }
    info.tag = L"";
    
    tango::IRelationPtr rel;

    // try to find the correct relation id
    tango::IRelationEnumPtr rel_enum = m_database->getRelationEnum(m_set->getObjectPath());
    size_t i, rel_count = rel_enum->size();
    for (i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);
        if (rel->getRelationId() == info.relation_id)
        {
            info.tag = rel->getTag();
            break;
        }
    }

    if (info.tag.length() == 0)
        return false;

    // get right set
    tango::ISetPtr right_set = rel->getRightSetPtr();
    ISetInternalPtr right_set_int = right_set;
    if (!right_set_int)
        return false;

    // lookup the index on the right set
    tango::IIndexInfoEnumPtr idx_enum = m_database->getIndexEnum(right_set->getObjectPath());
    tango::IIndexInfoPtr idx = xdLookupIndex(idx_enum, rel->getRightExpression(), false);
    if (!idx)
        return false;

    tango::IStructurePtr right_structure = right_set->getStructure();


    // construct final left expression.  This will be based off of the order
    // of the fields in the right set's index expression.  The order of the
    // fields in the index expression may be arbitrary, for example:
    // field1, field2 -or- field2, field1

    std::vector<std::wstring> left_list, right_list, idx_list;

    kl::parseDelimitedList(rel->getLeftExpression(), left_list, L',');
    kl::parseDelimitedList(rel->getRightExpression(), right_list, L',');
    kl::parseDelimitedList(idx->getExpression(), idx_list, L',');

    if (left_list.size() != right_list.size() ||
        right_list.size() != idx_list.size())
    {
        return false;
    }

    size_t x, j, count = left_list.size();


    info.kl = new KeyLayout;
    info.kl->setIterator(static_cast<tango::IIterator*>(this), false);


    for (x = 0; x < count; ++x)
    {
        for (j = 0; j < count; ++j)
        {
            if (!wcscasecmp(right_list[j].c_str(), idx_list[x].c_str()))
            {
                tango::IColumnInfoPtr colinfo;

                colinfo = right_structure->getColumnInfo(right_list[j]);
                if (colinfo.isNull())
                {
                    delete info.kl;
                    info.kl = NULL;
                    return false;
                }

                info.kl->addKeyPart(left_list[j],
                                    colinfo->getType(),
                                    colinfo->getWidth());

                break;
            }
        }    
    }

    if (info.right_iter.isNull())
    {
        info.right_iter = right_set_int->createIterator(L"",
                                                        idx->getExpression(),
                                                        NULL);
        info.right_iter_int = info.right_iter;
    }

    return true;
}

tango::IIteratorPtr BaseIterator::getChildIterator(tango::IRelationPtr relation)
{
    XCM_AUTO_LOCK(m_rel_mutex);

    BaseIteratorRelInfo* info = NULL;

    // lookup the relationship info
    std::vector<BaseIteratorRelInfo>::iterator it;
    for (it = m_relations.begin(); it != m_relations.end(); ++it)
    {
        if (it->relation_id == relation->getRelationId())
        {
            info = &(*it);
            break;
        }
    }

    // if the relationship info was not found, attempt
    // to initialize a new structure and add it to our
    // m_relations array

    if (!info || !info->kl)
    {
        BaseIteratorRelInfo i;
        i.relation_id = relation->getRelationId();
        i.kl = NULL;

        if (!refreshRelInfo(i))
        {
            return xcm::null;
        }

        m_relations.push_back(i);
        info = &m_relations.back();
    }

    // sanity check
    if (!info->kl)
        return xcm::null;

    // get the left key
    const unsigned char* left_key = info->kl->getKey();
    int left_keylen = info->kl->getKeyLength();

    // if the left key was truncated at all, that means that no record
    // can be found on the right side which satisfies the left expression

    if (info->kl->getTruncation())
        return xcm::null;

    info->right_iter_int->setKeyFilter(NULL, 0);

    if (!info->right_iter->seek(left_key, left_keylen, false))
        return xcm::null;
    
    return info->right_iter; 
}


tango::IIteratorPtr BaseIterator::getFilteredChildIterator(tango::IRelationPtr relation)
{
    XCM_AUTO_LOCK(m_rel_mutex);

    BaseIteratorRelInfo* info = NULL;

    // lookup the relationship info
    std::vector<BaseIteratorRelInfo>::iterator it;
    for (it = m_relations.begin(); it != m_relations.end(); ++it)
    {
        if (it->relation_id == relation->getRelationId())
        {
            info = &(*it);
            break;
        }
    }

    // if the relationship info was not found, attempt to initialize
    // a new structure and add it to our m_relations array

    if (!info || !info->kl)
    {
        BaseIteratorRelInfo i;
        i.relation_id = relation->getRelationId();
        i.kl = NULL;

        if (!refreshRelInfo(i))
        {
            return xcm::null;
        }

        m_relations.push_back(i);
        info = &m_relations.back();
    }

    // sanity check
    if (!info->kl)
        return xcm::null;

    // get the left key
    const unsigned char* left_key = info->kl->getKey();
    int left_keylen = info->kl->getKeyLength();

    // if the left key was truncated at all, that means that no record
    // can be found on the right side which satisfies the left expression

    if (info->kl->getTruncation())
        return xcm::null;

    // check if the iterator is already positioned on the current key

    const void* cur_key_filter;
    int cur_key_filter_len;
    info->right_iter_int->getKeyFilter(&cur_key_filter, &cur_key_filter_len);
    if (cur_key_filter && cur_key_filter_len == left_keylen)
    {
        if (memcmp(cur_key_filter, left_key, left_keylen) == 0)
        {
            info->right_iter->goFirst();
            return info->right_iter;
        }
    }

    info->right_iter_int->setKeyFilter(NULL, 0);

    if (!info->right_iter->seek(left_key, left_keylen, false))
        return xcm::null;

    info->right_iter_int->setKeyFilter(left_key, left_keylen);
    info->right_iter_int->setFirstKey();

    return info->right_iter;
}


tango::ISetPtr BaseIterator::getChildSet(tango::IRelationPtr relation)
{
    tango::IIteratorPtr right_iter = getFilteredChildIterator(relation);
    IIteratorKeyAccessPtr iter_int = right_iter;
    if (!iter_int.p)
    {
        tango::ISetPtr right_set = relation->getRightSetPtr();
        if (!right_set.p)
            return xcm::null;
        return createEofSet(m_database, right_set);
    }

    IterWrapperSet* iter_set = new IterWrapperSet(m_database);
    iter_set->ref();
    if (!iter_set->create(right_iter->getSet(), right_iter))
    {
        iter_set->unref();
        return xcm::null;
    }

    return tango::ISetPtr(static_cast<tango::ISet*>(iter_set), false);
}


tango::IStructurePtr BaseIterator::getStructure()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    tango::IStructurePtr s = m_iter_structure->clone();
    appendCalcFields(s);

    return s;
}


void BaseIterator::appendCalcFields(tango::IStructure* structure)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    IStructureInternalPtr intstruct = structure;

    std::vector<tango::IColumnInfoPtr>::iterator it;
    for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
    {
        intstruct->addColumn((*it)->clone());
    }
}




bool BaseIterator::initStructure()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    // refresh iterator structure from set

    m_set_structure = m_set->getStructure();
    m_calc_fields.clear();

    if (m_columns == L"*")
    {
        m_iter_structure = m_set_structure;
    }
     else
    {
        std::vector<std::wstring> colvec;
        std::vector<std::wstring>::iterator it;

        kl::parseDelimitedList(m_columns, colvec, L',', true);

        Structure* s = new Structure;

        int colname_counter = 0;

        for (it = colvec.begin(); it != colvec.end(); ++it)
        {
            std::wstring part = *it;
            kl::trim(part);

            tango::IColumnInfoPtr col;
            
            col = m_set_structure->getColumnInfo(part);
            if (col)
            {
                s->addColumn(col);
                continue;
            }
            
            if (part.length() > 0 && part[0] == '[')
            {
                std::wstring dequote_part = part;
                dequote(dequote_part, '[', ']');
                col = m_set_structure->getColumnInfo(dequote_part);
                if (col)
                {
                    s->addColumn(col);
                    continue;
                }
            }



            // look for 'AS' keyword
            wchar_t* temp = zl_stristr((wchar_t*)it->c_str(),
                                        L"AS",
                                        true,
                                        false);
            int as_pos = -1;

            if (temp)
            {
                as_pos = temp - it->c_str();
            }

            std::wstring colname;
            std::wstring expr;

            if (as_pos != -1)
            {
                colname = it->substr(as_pos+2);
                expr = it->substr(0, as_pos);
                    
                kl::trim(colname);
                kl::trim(expr);

                dequote(colname, '[', ']');
            }
             else
            {
                expr = *it;
                    
                wchar_t buf[255];
                do
                {
                    swprintf(buf, 255, L"EXPR%03d", ++colname_counter);
                } while (m_set_structure->getColumnExist(buf));

                colname = buf;
            }


            std::wstring dequote_expr = expr;
            dequote(dequote_expr, '[', ']');

            // check if we have a scenario where the field name is the same as the expression
            // ex:  fld123 AS fld123
            if (0 == wcscasecmp(dequote_expr.c_str(), colname.c_str()))
            {
                col = m_set_structure->getColumnInfo(colname);
                if (col)
                {
                    s->addColumn(col);
                    continue;
                }
            }


            kscript::ExprParser* p = createExprParser();
            ParserBindInfo bind_info(this, colname);
            bindExprParser(p, &bind_info);

            if (!p->parse(expr))
            {
                delete s;
                delete p;
                return false;
            }

            int expr_type = p->getType();

            delete p;

            int tango_type = kscript2tangoType(expr_type);
            if (tango_type == tango::typeInvalid ||
                tango_type == tango::typeUndefined)
            {
                delete s;
                return false;
            }

            int width;
            int scale = 0;

            switch (tango_type)
            {
                case tango::typeNumeric:
                    width = 18;
                    scale = bind_info.max_scale;
                    break;
                case tango::typeDouble:
                    width = 8;
                    scale = bind_info.max_scale;
                    break;
                case tango::typeDate:
                case tango::typeInteger:
                    width = 4;
                    break;
                case tango::typeDateTime:
                    width = 8;
                    break;
                default:
                    width = 254;
                    break;
            }


            // see if the expression is just a column and use its precise type info if it is
            col = m_set_structure->getColumnInfo(dequote_expr);
            if (col)
            {
                tango_type = col->getType();
                width = col->getWidth();
                scale = col->getScale();
            }



            ColumnInfo* c = new ColumnInfo;
            c->setName(colname);
            c->setType(tango_type);
            c->setWidth(width);
            c->setScale(scale);
            c->setExpression(expr);
            c->setCalculated(true);

            //s->addColumn(static_cast<tango::IColumnInfo*>(c));
            m_calc_fields.push_back(static_cast<tango::IColumnInfo*>(c));
        }

        m_iter_structure = s;

        clearDAI();
    }


    return true;
}


void BaseIterator::refreshStructure()
{
    std::vector<BaseIteratorRelInfo>::iterator r_it;
    for (r_it = m_relations.begin(); r_it != m_relations.end(); ++r_it)
    {
        tango::IIterator* p = r_it->right_iter.p;
        if (p != NULL && p != ((tango::IIterator*)this))
        {
            p->refreshStructure();
        }
    }


    // refresh the relation info structures
    int idx;
    for (idx = 0; idx < m_relations.size(); ++idx)
    {
        if (!refreshRelInfo(m_relations[idx]))
        {
            // relation is no longer valid, delete it
            m_relations.erase(m_relations.begin() + idx);
            idx--;
        }
    }


    initStructure();
    refreshDAI();
}







bool BaseIterator::modifyStructure(tango::IStructure* struct_config,
                                   tango::IJob* job)
{ 
    XCM_AUTO_LOCK(m_obj_mutex);
    
    bool done = false;

    IStructureInternalPtr int_struct = struct_config;

    if (!calcfieldsModifyStructure(int_struct->getStructureActions(),
                                   m_iter_structure,
                                   &m_calc_fields,
                                   &done))
    {
        return false;
    }

    refreshDAI();

    return true;
}


void BaseIterator::colinfo2dai(DataAccessInfo* dai,
                               tango::IColumnInfo* colinfo)
{
    if (dai->result)
    {
        delete dai->result;
        dai->result = NULL;
    }

    if (dai->expr)
    {
        delete dai->expr;
        dai->expr = NULL;
    }

    if (dai->raw_buf)
    {
        delete[] dai->raw_buf;
        dai->raw_buf = NULL;
    }

    dai->is_active = true;
    dai->is_column = true;
    dai->name = colinfo->getName();
    dai->type = colinfo->getType();
    dai->ordinal = colinfo->getColumnOrdinal();
    dai->offset = colinfo->getOffset();
    dai->width = colinfo->getWidth();
    dai->scale = colinfo->getScale();
    dai->nulls_allowed = colinfo->getNullsAllowed();
    dai->tableord = colinfo->getTableOrdinal();
    dai->expr_text = colinfo->getExpression();
    dai->expr = NULL;

    if (dai->expr_text.length() > 0)
    {
        dai->result = new kscript::Value;

        if (dai->type == tango::typeCharacter)
        {
            // allocate a place for dynamic field results
            // (this is needed for the getRawPtr() function
            dai->raw_buf = new unsigned char[dai->width+1];

            dai->result->setType(kscript::Value::typeString);
            dai->result->allocMem(dai->width + 1);
        }
         else if (dai->type == tango::typeDouble)
        {
            dai->raw_buf = new unsigned char[sizeof(double)];
        }
         else if (dai->type == tango::typeInteger)
        {
            dai->raw_buf = new unsigned char[sizeof(int)];
        }

        dai->expr = createExprParser();
        ParserBindInfo bind_info(this, dai->dai_text);
        bindExprParser(dai->expr, &bind_info);
        if (!dai->expr->parse(dai->expr_text))
        {
            delete dai->expr;
            dai->expr = NULL;
            dai->is_active = false;
        }
    }
}



void BaseIterator::onSetDomainUpdated()
{
}

void BaseIterator::onSetStructureUpdated()
{
}

void BaseIterator::onSetRowUpdated(tango::rowid_t rowid)
{
}

void BaseIterator::onSetRowDeleted(tango::rowid_t rowid)
{
}



tango::IDatabasePtr BaseIterator::getDatabase()
{
    return m_database;
}

tango::ISetPtr BaseIterator::getSet()
{
    return m_set;
}

tango::rowpos_t BaseIterator::getRowCount()
{
    return m_set_internal->getRowCount();
}




class AggregateResult
{
public:
   
    AggregateResult()
    {
        m_expr = L"";
        m_agg_func = GroupFunc_None;
        m_ref_count = 1;
        m_valid_rowid = 0;
        m_handle = 0;
    }

    ~AggregateResult()
    {
    }

    bool init(tango::IDatabasePtr database,
              tango::ISetPtr set,
              int agg_func,
              const std::wstring& expr)
    {
        if (set.isNull())
            return false;

        m_agg_func = agg_func;

        std::wstring column = L"";

        // find out the link tag
        if (m_agg_func == GroupFunc_Count)
        {
            m_link_tag = expr;
            kl::trim(m_link_tag);
            dequote(m_link_tag, '[', ']');

            if (m_link_tag.empty())
                return false;
        }
         else
        {
            const wchar_t* pstr = expr.c_str();
            const wchar_t* pperiod = zl_strchr((wchar_t*)pstr, '.', L"[", L"]");

            int period_pos = pperiod ? (pperiod-pstr) : -1;

            // if no period was found, or it's in the wrong
            // place, the parse was bad
            if (period_pos <= 0)
                return false;

            m_link_tag.assign(expr.c_str(), period_pos);
            kl::trim(m_link_tag);
            dequote(m_link_tag, '[', ']');

            column = expr.substr(period_pos+1);
            kl::trim(column);
            dequote(column, '[', ']');
        }

        tango::IRelationEnumPtr rel_enum;
        tango::IRelationPtr rel;

        rel_enum = database->getRelationEnum(set->getObjectPath());
        size_t i, rel_count = rel_enum->size();
        for (i = 0; i < rel_count; ++i)
        {
            rel = rel_enum->getItem(i);
            if (!wcscasecmp(rel->getTag().c_str(), m_link_tag.c_str()))
            {
                break;
            }
        }

        if (rel.isNull())
            return false;

        if (agg_func == GroupFunc_Count)
        {
            m_expr_type = tango::typeNumeric;
            
            // if it's just a count, we're done
            return true;
        }


        tango::ISetPtr right_set = rel->getRightSetPtr();
        if (right_set.isNull())
            return false;

        tango::IStructurePtr s = right_set->getStructure();
        if (s.isNull())
            return false;

        tango::IColumnInfoPtr colinfo = s->getColumnInfo(column);
        if (colinfo.isNull())
            return false;

        m_expr = column;
        m_expr_type = colinfo->getType();

        return true;
    }

public:

    int m_ref_count;

    tango::objhandle_t m_handle;

    std::wstring m_link_tag;
    std::wstring m_expr;
    int m_expr_type;
    int m_agg_func;

    std::wstring m_str_result;
    double m_dbl_result;
    bool m_bool_result;
    tango::datetime_t m_dt_result;

    // rowid for which this result is valid
    tango::rowid_t m_valid_rowid;
};


class AggregateExprElement : public kscript::ExprElement
{
public:
    AggregateResult* m_agg_result;
    BaseIterator* m_iter;

    AggregateExprElement(BaseIterator* iter, AggregateResult* agg_res)
    {
        m_iter = iter;
        m_agg_result = agg_res;
    }

    ~AggregateExprElement()
    {
        if (m_iter && m_agg_result)
        {
            m_iter->releaseAggResultObject(m_agg_result);
        }
    }

    int eval(kscript::ExprEnv*, kscript::Value* retval)
    {
        if (m_agg_result->m_valid_rowid != m_iter->m_rowid)
        {
            m_iter->recalcAggResults();
        }

        switch (m_agg_result->m_expr_type)
        {
            case tango::typeWideCharacter:
            case tango::typeCharacter:
                retval->setString(m_agg_result->m_str_result.c_str());
                break;
            case tango::typeNumeric:
            case tango::typeDouble:
            case tango::typeInteger:
                retval->setDouble(m_agg_result->m_dbl_result);
                break;
            case tango::typeBoolean:
                retval->setBoolean(m_agg_result->m_bool_result);
                break;
            case tango::typeDate:
            case tango::typeDateTime:
                retval->setDateTime((unsigned int)(m_agg_result->m_dt_result >> 32),
                                    (unsigned int)(m_agg_result->m_dt_result & 0xffffffff));
                break;
        }

        return kscript::errorNone;
    }

    int getType()
    {
        switch (m_agg_result->m_expr_type)
        {
            default:
            case tango::typeInvalid:       return kscript::Value::typeNull;
            case tango::typeUndefined:     return kscript::Value::typeUndefined;
            case tango::typeBoolean:       return kscript::Value::typeBoolean;
            case tango::typeNumeric:       return kscript::Value::typeDouble;
            case tango::typeInteger:       return kscript::Value::typeInteger;
            case tango::typeDouble:        return kscript::Value::typeDouble;
            case tango::typeCharacter:     return kscript::Value::typeString;
            case tango::typeWideCharacter: return kscript::Value::typeString;
            case tango::typeDateTime:      return kscript::Value::typeDateTime;
            case tango::typeDate:          return kscript::Value::typeDateTime;
            case tango::typeBinary:        return kscript::Value::typeBinary;
        }
    }
};


void BaseIterator::onSetRelationshipsUpdated()
{
    m_rel_mutex.lock();

    std::vector<BaseIteratorRelInfo>::iterator r_it;
    for (r_it = m_relations.begin(); r_it != m_relations.end(); ++r_it)
        delete r_it->kl;
    m_relations.clear();
    m_relenum.clear();

    m_rel_mutex.unlock();




    m_agg_mutex.lock();
    std::vector<AggregateResult*>::iterator it;
    for (it = m_aggregate_results.begin();
         it != m_aggregate_results.end(); ++it)
    {
        (*it)->m_handle = 0;
    }
    m_agg_mutex.unlock();

}


AggregateResult* BaseIterator::getAggResultObject(int agg_func,
                                                  const std::wstring& expr)
{
    XCM_AUTO_LOCK(m_agg_mutex);

    // first, try to see if a suitable aggregate result object exists
    std::vector<AggregateResult*>::iterator it;
    for (it = m_aggregate_results.begin();
         it != m_aggregate_results.end(); ++it)
    {
        if ((*it)->m_agg_func == agg_func &&
            !wcscasecmp((*it)->m_expr.c_str(), expr.c_str()))
        {
            (*it)->m_ref_count++;
            return (*it);
        }
    }

    // no suitable aggregate result object was found, so initialize a new one

    AggregateResult* agg_res = new AggregateResult;
    if (!agg_res->init(m_database, m_set, agg_func, expr))
    {
        delete agg_res;
        return NULL;
    }
    
    m_aggregate_results.push_back(agg_res);
    
    return agg_res;
}

void BaseIterator::releaseAggResultObject(AggregateResult* agg_res)
{
    XCM_AUTO_LOCK(m_agg_mutex);

    std::vector<AggregateResult*>::iterator it;
    it = std::find(m_aggregate_results.begin(),
                   m_aggregate_results.end(),
                   agg_res);
    
    if (it == m_aggregate_results.end())
        return;
    
    (*it)->m_ref_count--;
    if ((*it)->m_ref_count == 0)
    {
        delete (*it);
        m_aggregate_results.erase(it);
    }
}

void BaseIterator::recalcAggResults()
{
    if (m_aggregate_results.size() == 0)
        return;

    std::vector<AggregateResult*>::iterator it;

    m_agg_mutex.lock();
    for (it = m_aggregate_results.begin();
         it != m_aggregate_results.end(); ++it)
    {
        // initialize group results
        (*it)->m_str_result = L"";
        (*it)->m_dbl_result = 0.0;
        (*it)->m_bool_result = false;
        (*it)->m_dt_result = 0;

        (*it)->m_valid_rowid = m_rowid;
    }
    m_agg_mutex.unlock();


    m_rel_mutex.lock();
    if (m_relenum.isNull())
        m_relenum = m_database->getRelationEnum(m_set->getObjectPath());
    tango::IRelationEnumPtr rel_enum = m_relenum;
    m_rel_mutex.unlock();

    size_t rel_count = rel_enum->size();
    if (rel_count == 0)
        return;

    XCM_AUTO_LOCK(m_agg_mutex);

    std::vector<AggregateResult*> results;
    results.resize(m_aggregate_results.size());

    int r, result_count;

    tango::IRelationPtr rel;
    tango::IIteratorPtr sp_iter;
    tango::IIterator* iter;

    bool only_first;
    tango::objhandle_t obj_handle;




    for (size_t i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);
        
        only_first = true;

        result_count = 0;

        for (it = m_aggregate_results.begin();
             it != m_aggregate_results.end(); ++it)
        {
            if (!wcscasecmp((*it)->m_link_tag.c_str(),
                            rel->getTag().c_str()))
            {
                if ((*it)->m_agg_func != GroupFunc_First)
                    only_first = false;

                results[result_count] = (*it);
                result_count++;
            }
        }

        if (result_count == 0)
            continue;

        sp_iter = getFilteredChildIterator(rel);
        if (sp_iter.isNull())
            continue;

        iter = sp_iter.p;

        if (iter->eof())
            continue;
        
        if (only_first)
        {
            for (r = 0; r < result_count; ++r)
            {
                if (!results[r]->m_handle)
                {
                    results[r]->m_handle = iter->getHandle(results[r]->m_expr);
                }

                obj_handle = results[r]->m_handle;

                if (!obj_handle)
                    continue;

                switch (results[r]->m_expr_type)
                {
                    case tango::typeWideCharacter:
                    case tango::typeCharacter:
                        results[r]->m_str_result = iter->getWideString(obj_handle);
                        break;
                    case tango::typeNumeric:
                    case tango::typeInteger:
                    case tango::typeDouble:
                        results[r]->m_dbl_result = iter->getDouble(obj_handle);
                        break;
                    case tango::typeBoolean:
                        results[r]->m_bool_result = iter->getBoolean(obj_handle);
                        break;
                    case tango::typeDate:
                    case tango::typeDateTime:
                        results[r]->m_dt_result = iter->getDateTime(obj_handle);
                        break;
                }
            }
        }
         else
        {
            bool first_row = true;
            double group_row_count = 0.0;
            kl::Statistics stats;

            iter->goFirst();
            while (!iter->eof())
            {
                group_row_count++;

                for (r = 0; r < result_count; ++r)
                {
                    if (!results[r]->m_handle &&
                        results[r]->m_agg_func != GroupFunc_Count)
                    {
                        results[r]->m_handle = iter->getHandle(results[r]->m_expr);
                    }
                    obj_handle = results[r]->m_handle;

                    switch (results[r]->m_agg_func)
                    {
                        case GroupFunc_Min:
                        case GroupFunc_Max:
                        {
                            std::wstring str_result;
                            double dbl_result;
                            bool bool_result;
                            tango::datetime_t dt_result;
                            int agg_func = results[r]->m_agg_func;

                            switch (results[r]->m_expr_type)
                            {
                                case tango::typeWideCharacter:
                                case tango::typeCharacter:
                                    str_result = iter->getWideString(obj_handle);

                                    if (first_row ||
                                        (agg_func == GroupFunc_Min && str_result < results[r]->m_str_result) ||
                                        (agg_func == GroupFunc_Max && str_result > results[r]->m_str_result))
                                    {
                                        results[r]->m_str_result = str_result;
                                    }
                                    break;
                                case tango::typeNumeric:
                                case tango::typeInteger:
                                case tango::typeDouble:
                                    dbl_result = iter->getDouble(obj_handle);
                                    if (first_row ||
                                        (agg_func == GroupFunc_Min && dbl_result < results[r]->m_dbl_result) ||
                                        (agg_func == GroupFunc_Max && dbl_result > results[r]->m_dbl_result))
                                    {
                                        results[r]->m_dbl_result = dbl_result;
                                    }
                                    break;
                                case tango::typeBoolean:
                                    bool_result = iter->getBoolean(obj_handle);
                                    if (first_row ||
                                        (agg_func == GroupFunc_Min && !bool_result) ||
                                        (agg_func == GroupFunc_Max && bool_result))
                                    {
                                        results[r]->m_bool_result = bool_result;
                                    }
                                    break;
                                case tango::typeDate:
                                case tango::typeDateTime:
                                    dt_result = iter->getDateTime(obj_handle);
                                    if (first_row ||
                                        (agg_func == GroupFunc_Min && dt_result < results[r]->m_dt_result) ||
                                        (agg_func == GroupFunc_Max && dt_result > results[r]->m_dt_result))
                                    {
                                        results[r]->m_dt_result = dt_result;
                                    }
                                    break;
                            }
                        }
                        break;

                        case GroupFunc_First:
                            if (!first_row)
                                break;
                        
                        case GroupFunc_Last:
                            switch (results[r]->m_expr_type)
                            {
                                case tango::typeWideCharacter:
                                case tango::typeCharacter:
                                    results[r]->m_str_result = iter->getWideString(obj_handle);
                                    break;
                                case tango::typeNumeric:
                                case tango::typeInteger:
                                case tango::typeDouble:
                                    results[r]->m_dbl_result = iter->getDouble(obj_handle);
                                    break;
                                case tango::typeBoolean:
                                    results[r]->m_bool_result = iter->getBoolean(obj_handle);
                                    break;
                                case tango::typeDate:
                                case tango::typeDateTime:
                                    results[r]->m_dt_result = iter->getDateTime(obj_handle);
                                    break;
                            }
                            break;

                        case GroupFunc_Avg:
                        case GroupFunc_Sum:
                            results[r]->m_dbl_result += iter->getDouble(obj_handle);
                            break;

                        case GroupFunc_Stddev:
                        case GroupFunc_Variance:
                            stats.push(iter->getDouble(obj_handle));
                            break;

                        case GroupFunc_Merge:
                            results[r]->m_str_result += iter->getWideString(obj_handle);
                            break;
                    }
                }

                first_row = false;
                iter->skip(1);
            }

            for (r = 0; r < result_count; ++r)
            {
                switch (results[r]->m_agg_func)
                {
                    case GroupFunc_Count:
                        results[r]->m_dbl_result = group_row_count;
                        break;
                    case GroupFunc_Avg:
                        if (kl::dblcompare(group_row_count, 0.0) == 0)
                            results[r]->m_dbl_result = 0.0;
                             else
                            results[r]->m_dbl_result /= group_row_count;
                        break;
                    case GroupFunc_Stddev:
                        results[r]->m_dbl_result = stats.stddev();
                        break;
                    case GroupFunc_Variance:
                        results[r]->m_dbl_result = stats.variance();
                        break;
                }
            }
        }
    }
}



// field Binding helper functions for the expression parser

static void _bindFieldString(kscript::ExprEnv*,
                             void* param,
                             kscript::Value* retval)
{
    DataAccessInfo* dai = (DataAccessInfo*)param;

    if (!dai->it->m_rowptr)
    {
        retval->setString(L"");
        return;
    }

    const unsigned char* ptr = dai->it->m_rowptr+(dai->offset);


    // check for a null
    if (dai->nulls_allowed)
    {
        if (*(ptr-1) & 0x01)
        {
            retval->setNull();
            return;
        }
    }


    int width = dai->width;
    while (width && *(ptr+width-1) == ' ')
        width--;

    retval->allocString(width);
    wchar_t* strbuf = retval->getString();
    wchar_t* wc = strbuf;

    int i;
    for (i = 0; i < width; ++i)
    {
        *wc = *(ptr+i);
        if (!*wc)
            break;
        wc++;
    }

    *wc = L'\0';
    retval->setStringLen(wc - strbuf);
}

static void _bindFieldWideString(kscript::ExprEnv*,
                                 void* param,
                                 kscript::Value* retval)
{
    DataAccessInfo* dai = (DataAccessInfo*)param;

    if (!dai->it->m_rowptr)
    {
        retval->setString(L"");
        return;
    }

    const unsigned char* ptr = dai->it->m_rowptr+(dai->offset);

    // check for a null
    if (dai->nulls_allowed)
    {
        if (*(ptr-1) & 0x01)
        {
            retval->setNull();
            return;
        }
    }

    retval->allocString(dai->width);
    wchar_t* wc = retval->getString();


    // little-endian machines only
    
#ifdef WIN32
    wcsncpy(wc, (wchar_t*)ptr, dai->width);
    wc[dai->width] = 0;
#else
    int i;
    for (i = 0; i < dai->width; ++i)
    {
        wchar_t c;
        c = ptr[(i*2)+1];
        c <<= 8;
        c |= ptr[i*2];
        wc[i] = c;
        if (!c)
            break;
    }
    wc[dai->width] = 0;
#endif

    retval->setStringLen(wcslen(retval->getString()));
}

static void _bindFieldDynString(kscript::ExprEnv*,
                                void* param,
                                kscript::Value* retval)
{
    DataAccessInfo* dai = (DataAccessInfo*)param;

    if (dai->expr == NULL)
    {
        retval->setString(L"");
        return;
    }

    if (!dai->eval(retval) ||
         dai->expr->getType() != kscript::Value::typeString)
    {
        retval->setString(L"");
        return;
    }

    // crop it to calcfield's size
    wchar_t* result_str = retval->getString();
    int len = wcslen(result_str);
    if (len > dai->width)
    {
        result_str[dai->width] = 0;
    }
}

static void _bindFieldDouble(kscript::ExprEnv*,
                             void* param,
                             kscript::Value* retval)
{
    DataAccessInfo* dai = (DataAccessInfo*)param;
    if (!dai->it->m_rowptr)
    {
        retval->setDouble(0.0);
        return;
    }

    retval->setDouble(dai->it->getDouble((tango::objhandle_t)dai));
    retval->setScale(dai->scale);
}

static void _bindFieldInteger(kscript::ExprEnv*,
                              void* param,
                              kscript::Value* retval)
{
    DataAccessInfo* dai = (DataAccessInfo*)param;
    if (!dai->it->m_rowptr)
    {
        retval->setInteger(0);
        return;
    }

    retval->setInteger(dai->it->getInteger((tango::objhandle_t)dai));
}

static void _bindFieldDateTime(kscript::ExprEnv*,
                               void* param,
                               kscript::Value* retval)
{
    DataAccessInfo* dai = (DataAccessInfo*)param;
    if (!dai->it->m_rowptr)
    {
        retval->setDateTime(0,0);
        return;
    }

    tango::datetime_t dt, d, t;
    dt = dai->it->getDateTime((tango::objhandle_t)dai);
    d = dt >> 32;
    t = dt & 0xffffffff;
    retval->setDateTime((unsigned int)d, (unsigned int)t);
    retval->setDataLen((dai->type == tango::typeDateTime) ? 8 : 4);
}

static void _bindFieldBoolean(kscript::ExprEnv*,
                              void* param,
                              kscript::Value* retval)
{
    DataAccessInfo* dai = (DataAccessInfo*)param;
    if (!dai->it->m_rowptr)
    {
        retval->setBoolean(false);
        return;
    }

    retval->setBoolean(dai->it->getBoolean((tango::objhandle_t)dai));
}


void func_rownum(kscript::ExprEnv* env,
                 void* param,
                 kscript::Value* retval)
{
    BaseIterator* it = (BaseIterator*)param;
    retval->setDouble((double)(long long)it->getRowNumber());
}

void func_recno(kscript::ExprEnv* env,
                void* param,
                kscript::Value* retval)
{
    BaseIterator* it = (BaseIterator*)param;
    tango::rowpos_t rowpos = rowidGetRowPos(it->m_rowid);
    retval->setDouble((double)(long long)rowpos);
}

void func_recid(kscript::ExprEnv* env,
                void* param,
                kscript::Value* retval)
{
    BaseIterator* it = (BaseIterator*)param;
    tango::tableord_t ordinal = rowidGetTableOrd(it->m_rowid);
    tango::rowpos_t rowpos = rowidGetRowPos(it->m_rowid);
    unsigned int hrowpos = (unsigned int)(rowpos >> 32);
    unsigned int lrowpos = (unsigned int)(rowpos & 0xffffffff);
    wchar_t buf[32];
    swprintf(buf, 32, L"%07x%x%08x", (int)ordinal, hrowpos, lrowpos);
    retval->setString(buf);
}


void func_reccrc(kscript::ExprEnv* env,
                 void* param,
                 kscript::Value* retval)
{
    BaseIterator* it = (BaseIterator*)param;

    unsigned long long c;
    crc64(it->getRowBuffer(), it->getRowBufferWidth(), &c);

    retval->setType(kscript::Value::typeBinary);
    retval->allocMem(8);
    retval->setDataLen(8);
    memcpy(retval->getBinary(), &c, sizeof(unsigned long long));
}


bool BaseIterator::base_iterator_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    ParserBindInfo* bind_param = (ParserBindInfo*)hook_info.hook_param;
    BaseIterator* iter = bind_param->iter;

    if (hook_info.element_type == kscript::ExprParseHookInfo::typeFunction)
    {  
        int len = hook_info.expr_text.length();
        if (len == 0)
            return false;

        const wchar_t* expr_text = hook_info.expr_text.c_str();

        if (*(expr_text+len-1) != ')')
            return false;

        const wchar_t* open_paren = wcschr(expr_text, L'(');
        if (!open_paren)
            return false;

        open_paren++;
        while (iswspace(*open_paren))
            open_paren++;

        std::wstring expr;
        expr.assign(open_paren, wcslen(open_paren)-1);
        kl::trimRight(expr);

        int agg_func = GroupFunc_None;
        if (!wcsncasecmp(expr_text, L"FIRST(", 6))
            agg_func = GroupFunc_First;
        else if (!wcsncasecmp(expr_text, L"LAST(", 5))
            agg_func = GroupFunc_Last;
        else if (!wcsncasecmp(expr_text, L"MIN(", 4))
            agg_func = GroupFunc_Min;
        else if (!wcsncasecmp(expr_text, L"MAX(", 4))
            agg_func = GroupFunc_Max;
        else if (!wcsncasecmp(expr_text, L"SUM(", 4))
            agg_func = GroupFunc_Sum;
        else if (!wcsncasecmp(expr_text, L"AVG(", 4))
            agg_func = GroupFunc_Avg;
        else if (!wcsncasecmp(expr_text, L"COUNT(", 6))
            agg_func = GroupFunc_Count;
        else if (!wcsncasecmp(expr_text, L"STDDEV(", 7))
            agg_func = GroupFunc_Stddev;
        else if (!wcsncasecmp(expr_text, L"VARIANCE(", 9))
            agg_func = GroupFunc_Variance;
        else if (!wcsncasecmp(expr_text, L"MERGE(", 6))
            agg_func = GroupFunc_Merge;

        if (agg_func == GroupFunc_None)
            return false;

        AggregateResult* agg_res = iter->getAggResultObject(agg_func, expr);
        if (!agg_res)
            return false;

        AggregateExprElement* e = new AggregateExprElement(iter, agg_res);
        hook_info.res_element = static_cast<kscript::ExprElement*>(e);

        return true;
    }
     else if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        if (hook_info.expr_text.length() > 1 &&
            hook_info.expr_text[0] == '[' &&
            hook_info.expr_text[hook_info.expr_text.length()-1] == ']')
        {
            // remove brackets from beginning and end e.g. [Field Name] => Field Name
            hook_info.expr_text = hook_info.expr_text.substr(0, hook_info.expr_text.length()-1);
            hook_info.expr_text.erase(0, 1);
        }

        // find out the scale for numeric constants
        if (hook_info.expr_text.length() > 1 && iswdigit(hook_info.expr_text[0]))
        {
            size_t pos = hook_info.expr_text.find('.');
            if (pos != std::wstring::npos)
            {
                int scale = hook_info.expr_text.length() - (pos+1);
                bind_param->max_scale = std::max(bind_param->max_scale, scale);
                if (bind_param->max_scale > 10)
                    bind_param->max_scale = std::min(bind_param->max_scale, 4);
            }
        }
            
        tango::IColumnInfoPtr colinfo;
        colinfo = iter->m_set_structure->getColumnInfo(hook_info.expr_text);
        if (colinfo.isNull())
        {
            size_t i, calc_field_count = iter->m_calc_fields.size();
            for (i = 0; i < calc_field_count; ++i)
            {
                if (!wcscasecmp(iter->m_calc_fields[i]->getName().c_str(), hook_info.expr_text.c_str()))
                {
                    colinfo = iter->m_calc_fields[i];
                    break;
                }
            }
        }
        
        if (colinfo.isNull())
            return false;
        
        if (colinfo->getType() == tango::typeNumeric || colinfo->getType() == tango::typeDouble)
        {
            bind_param->max_scale = std::max(bind_param->max_scale, colinfo->getScale());
            if (bind_param->max_scale > 10)
                bind_param->max_scale = std::min(bind_param->max_scale, 4);
        }

        DataAccessInfo* dai = NULL;
        
        if (0 == wcscasecmp(bind_param->column.c_str(), hook_info.expr_text.c_str()))
        {
            if (colinfo->getCalculated())
            {
                // dynamic field referring to itself
                return false;
            }
             else
            {
                // the dynamic field may referring to a physical
                // field of the same name
                dai = new DataAccessInfo;
                dai->it = iter;
                dai->is_active = true;
                dai->is_column = true;
                dai->dai_text = bind_param->column;

                iter->m_dai_entries.push_back(dai);
            
                iter->colinfo2dai(dai, colinfo.p);
            }
        }
        
        
        if (!dai)
            dai = (DataAccessInfo*)iter->getHandle(colinfo->getName());
            
        if (!dai)
            return false;

        kscript::Value* val = new kscript::Value;

        switch (dai->type)
        {
            case tango::typeWideCharacter:
                if (dai->expr_text.length() == 0)
                {
                    val->setGetVal(kscript::Value::typeString,
                       _bindFieldWideString,
                       (void*)dai);
                }
                 else
                {
                    val->setGetVal(kscript::Value::typeString,
                       _bindFieldDynString,
                       (void*)dai);
                }
                break;

            case tango::typeCharacter:
                if (dai->expr_text.length() == 0)
                {
                    val->setGetVal(kscript::Value::typeString,
                       _bindFieldString,
                       (void*)dai);
                }
                 else
                {
                    val->setGetVal(kscript::Value::typeString,
                       _bindFieldDynString,
                       (void*)dai);
                }
                break;

            case tango::typeNumeric:
            case tango::typeDouble:
                val->setGetVal(kscript::Value::typeDouble,
                   _bindFieldDouble,
                   (void*)dai);
                break;

            case tango::typeInteger:
                val->setGetVal(kscript::Value::typeInteger,
                   _bindFieldInteger,
                   (void*)dai);
                break;

            case tango::typeDate:
            case tango::typeDateTime:
                val->setGetVal(kscript::Value::typeDateTime,
                   _bindFieldDateTime,
                   (void*)dai);
                break;

            case tango::typeBoolean:
                val->setGetVal(kscript::Value::typeBoolean,
                   _bindFieldBoolean,
                   (void*)dai);
                break;
        }

        hook_info.res_element = static_cast<kscript::ExprElement*>(val);
        return true;
    }
    
    return false;
}

void BaseIterator::bindExprParser(kscript::ExprParser* parser,
                                  ParserBindInfo* info)
{    
    parser->setParseHook(kscript::ExprParseHookInfo::typeFunction |
                         kscript::ExprParseHookInfo::typeIdentifier,
                         base_iterator_parse_hook,
                         info);
                         
    parser->addFunction(L"recno", false, func_recno, false, L"f()", this);
    parser->addFunction(L"rownum", false, func_rownum, false, L"f()", this);
    parser->addFunction(L"recid", false, func_recid, false, L"s()", this);
    parser->addFunction(L"reccrc", false, func_reccrc, false, L"x()", this);
}




tango::IColumnInfoPtr BaseIterator::getInfo(tango::objhandle_t data_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)data_handle;

    if (!dai)
    {
        return xcm::null;
    }

    ColumnInfo* colinfo = new ColumnInfo;
    colinfo->setName(dai->name);
    colinfo->setType(dai->type);
    colinfo->setWidth(dai->width);
    colinfo->setScale(dai->scale);
    colinfo->setExpression(dai->expr_text);
    colinfo->setCalculated(dai->expr_text.length() > 0 ? true : false);

    return static_cast<tango::IColumnInfo*>(colinfo);
}


tango::objhandle_t BaseIterator::getHandle(const std::wstring& expr)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    if (expr.length() == 0)
        return 0;

    DataAccessInfo* dai = NULL;

    dai = lookupDAI(expr);
    if (dai)
    {
        return (tango::objhandle_t)dai;
    }


    // create a key expression if requested

    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        dai = new DataAccessInfo;
        dai->is_active = true;
        dai->is_column = false;
        dai->it = this;
        dai->dai_text = expr;
        dai->key_layout = new KeyLayout;
        dai->result = new kscript::Value;
        dai->expr_text = expr;
        dai->scale = 0;

        if (!dai->key_layout->setKeyExpr(static_cast<tango::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }

        return (tango::objhandle_t)dai;
    }


    // was not found in existing dai entries, so try to look
    // in the structure and our calc fields


    // try calc fields 
    tango::IColumnInfoPtr colinfo;

    std::vector<tango::IColumnInfoPtr>::iterator cit;
    for (cit = m_calc_fields.begin(); cit != m_calc_fields.end(); ++cit)
    {
        if (!wcscasecmp((*cit)->getName().c_str(), expr.c_str()))
        {
            colinfo = *cit;
            break;
        }
    }

    if (colinfo.isNull() && m_iter_structure.isOk())
    {
        colinfo = m_iter_structure->getColumnInfo(expr);
    }

    if (colinfo.isNull())
    {
        colinfo = m_set_structure->getColumnInfo(expr);
    }

    if (colinfo)
    {
        dai = new DataAccessInfo;
        dai->it = this;
        dai->is_active = true;
        dai->is_column = true;
        dai->dai_text = expr;

        m_dai_entries.push_back(dai);
        m_dai_lookup[expr] = dai;

        colinfo2dai(dai, colinfo.p);

        return (tango::objhandle_t)dai;
    }

    // couldn't find it in the structure, so try to parse it as an expr

    dai = new DataAccessInfo;
    dai->is_active = true;
    dai->is_column = false;
    dai->it = this;
    dai->dai_text = expr;
    dai->expr = createExprParser();
    dai->result = new kscript::Value;
    dai->expr_text = expr;
    dai->scale = 255;       // we don't know the scale of the result

    ParserBindInfo bind_info(this, L"");
    bindExprParser(dai->expr, &bind_info);

    if (!dai->expr->parse(expr))
    {
        delete dai;
        return 0;
    }

    dai->type = kscript2tangoType(dai->expr->getType());
    return (tango::objhandle_t)dai;
}

int BaseIterator::getType(tango::objhandle_t data_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return tango::typeInvalid;
    }
    if (dai->expr == NULL)
    {
        return dai->type;
    }

    return kscript2tangoType(dai->expr->getType());
}

bool BaseIterator::releaseHandle(tango::objhandle_t data_handle)
{
    if (data_handle == 0)
        return false;

    DataAccessInfo* dai = (DataAccessInfo*)data_handle;

    // for now, don't delete data handles that are in the m_cols vector
    std::vector<DataAccessInfo*>::iterator dai_it;
    for (dai_it = m_dai_entries.begin();
         dai_it != m_dai_entries.end();
         ++dai_it)
    {
        if (*dai_it == dai)
        {
            return true;
        }
    }

    delete dai;
    return true;
}



unsigned int BaseIterator::getIteratorFlags()
{
    return m_iter_flags;
}

void BaseIterator::setIteratorFlags(unsigned int new_val)
{
    m_iter_flags = new_val;
}




const unsigned char* BaseIterator::getRowBuffer()
{
    return m_rowptr;
}

int BaseIterator::getRowBufferWidth()
{
    std::vector<BaseIteratorTableEntry>::iterator it;
    for (it = m_tables.begin(); it != m_tables.end(); ++it)
    {
        if (it->row_buf == m_rowptr)
            return it->row_width;
    }

    return 0;
}



const unsigned char* BaseIterator::getRawPtr(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
    {
        return NULL;
    }

    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    if (dai->expr == NULL)
    {
        return m_rowptr + dai->offset;
    }

    if (!dai->eval(dai->result))
        return NULL;

    int expr_type = dai->expr->getType();

    if (dai->type == tango::typeDouble)
    {
        if (expr_type == kscript::Value::typeDouble ||
            expr_type == kscript::Value::typeInteger)
        {
            double d = dai->result->getDouble();
            memcpy(dai->raw_buf, &d, sizeof(double));
            return dai->raw_buf;
        }

        return NULL;
    }
     else if (dai->type == tango::typeInteger)
    {
        if (expr_type == kscript::Value::typeDouble ||
            expr_type == kscript::Value::typeInteger)
        {
            int i = dai->result->getInteger();
            memcpy(dai->raw_buf, &i, sizeof(int));
            return dai->raw_buf;
        }

        return NULL;
    }
     else if (expr_type == kscript::Value::typeBinary)
    {
        return dai->result->getBinary();
    }

    return NULL;
}

int BaseIterator::getRawWidth(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
        return 0;

    if (dai->key_layout)
        return dai->key_layout->getKeyLength();

    if (dai->expr == NULL)
        return dai->width;

    return dai->result->getDataLen();
}

const std::string& BaseIterator::getString(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
        return empty_string;

    if (dai->expr == NULL)
    {
        if (!m_rowptr)
            return empty_string;

        if (dai->type == tango::typeCharacter)
        {
            // return field data
            const char* ptr = (char*)(m_rowptr + dai->offset);

            int width = dai->width;
            while (width && *(ptr+width-1) == ' ')
                width--;
            
            // look for a zero terminator
            const char* zero = (const char*)memchr(ptr, 0, width);
            if (zero)
                width = zero-ptr;
            
            dai->str_result.assign(ptr, width);

            return dai->str_result;
        }
         else if (dai->type == tango::typeWideCharacter)
        {
            kl::ucsle2string(dai->str_result,
                             m_rowptr + dai->offset,
                             dai->width);
            return dai->str_result;
        }
         else
        {
            return empty_string;
        }
    }


    if (!dai->eval(dai->result) ||
         (dai->expr->getType() != kscript::Value::typeString &&
          dai->expr->getType() != kscript::Value::typeUndefined)
       )
    {
        // upon failure, return an empty string
        return empty_string;
    }


    // crop it to calcfield's size
    wchar_t* result_str = dai->result->getString();
    int len = wcslen(result_str);
    if (dai->is_column && len > dai->width)
    {
        len = dai->width;
    }

    // convert to an ASCII string
    dai->str_result.resize(len);

    int i;
    wchar_t ch;
    for (i = 0; i < len; ++i)
    {
        ch = result_str[i];
        if (ch > 255)
            ch = '?';

        dai->str_result[i] = (char)(unsigned char)ch;
    }

    return dai->str_result;
}

const std::wstring& BaseIterator::getWideString(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
    {
        return empty_wstring;
    }

    if (dai->expr == NULL)
    {
        if (!m_rowptr)
            return empty_wstring;

        // return field data
        if (dai->type == tango::typeWideCharacter)
        {
            kl::ucsle2wstring(dai->wstr_result,
                              m_rowptr + dai->offset,
                              dai->width);
            return dai->wstr_result;
        }
         else if (dai->type == tango::typeCharacter)
        {
            const char* ptr = (char*)(m_rowptr + dai->offset);
            int width = dai->width;
            while (width && *(ptr+width-1) == ' ')
                width--;
                
            // look for a zero terminator
            const char* zero = (const char*)memchr(ptr, 0, width);
            if (zero)
                width = zero-ptr;

            kl::towstring(dai->wstr_result, ptr, width);
            return dai->wstr_result;
        } 
         else
        {
            return empty_wstring;
        }
    }

    if (!dai->eval(dai->result) ||
         (dai->expr->getType() != kscript::Value::typeString &&
          dai->expr->getType() != kscript::Value::typeUndefined)
       )
    {
        // upon failure, return an empty string
        return empty_wstring;
    }


    // crop it to calcfield's size
    wchar_t* result_str = dai->result->getString();
    int len = wcslen(result_str);
    if (dai->is_column && len > dai->width)
    {
        len = dai->width;
    }

    dai->wstr_result.assign(result_str, len);

    return dai->wstr_result;
}

tango::datetime_t BaseIterator::getDateTime(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
    {
        return 0;
    }

    if (dai->expr == NULL)
    {
        if (!m_rowptr)
            return 0;

        if (dai->type == tango::typeDate)
        {
            tango::datetime_t dt = buf2int(m_rowptr+dai->offset);
            dt <<= 32;

            return dt;
        }
        
        if (dai->type == tango::typeDateTime)
        {
            tango::datetime_t dt = buf2int(m_rowptr+dai->offset);
            tango::datetime_t ts = buf2int(m_rowptr+dai->offset+4);

            dt <<= 32;
            dt |= ts;

            return dt;
        }

        return 0;
    }

    if (!dai->eval(dai->result) ||
         dai->expr->getType() != kscript::Value::typeDateTime)
    {
        return 0;
    }

    kscript::ExprDateTime edt = dai->result->getDateTime();
    tango::datetime_t dt;
    dt = edt.date;
    dt <<= 32;
    
    if (dai->type == tango::typeDateTime)
        dt |= edt.time;

    return dt;
}


double BaseIterator::getDouble(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
        return 0.0;

    if (dai->type == tango::typeInteger)
    {
        // implicit conversion from integer -> double
        return getInteger(column_handle);
    }
     else if (dai->type == tango::typeCharacter ||
              dai->type == tango::typeWideCharacter)
    {
        return kl::nolocale_atof(getString(column_handle).c_str());
    }

    if (dai->expr == NULL)
    {
        if (!m_rowptr)
            return 0.0;

        if (dai->type == tango::typeNumeric)
        {
            return kl::dblround(
                    decstr2dbl((char*)m_rowptr + dai->offset, dai->width, dai->scale),
                    dai->scale);
        }
         else if (dai->type == tango::typeDouble)
        {
            // FIXME: this will only work on little-endian (intel) processors
            double d;
            memcpy(&d, m_rowptr + dai->offset, sizeof(double));
            return kl::dblround(d, dai->scale);
        }
         else
        {
            return 0.0;
        }
    }

    if (!dai->eval(dai->result) ||
         (dai->expr->getType() != kscript::Value::typeDouble &&
          dai->expr->getType() != kscript::Value::typeInteger &&
          dai->expr->getType() != kscript::Value::typeUndefined)
       )
    {
        return 0.0;
    }

    double d = dai->result->getDouble();

    if (isnan(d) || !finite(d))
        d = 0.0;

    if (dai->scale == 255)
        return d;
         else
        return kl::dblround(d, dai->scale);
}

bool BaseIterator::getBoolean(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
        return false;

    if (dai->expr == NULL)
    {
        if (!m_rowptr)
            return false;

        return (*(m_rowptr+dai->offset) == 'T' ? true : false);
    }

    if (!dai->eval(dai->result) ||
         dai->expr->getType() != kscript::Value::typeBoolean)
    {
        return false;
    }

    return dai->result->getBoolean();
}


int BaseIterator::getInteger(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
        return 0;

    if (dai->type == tango::typeDouble || dai->type == tango::typeNumeric)
    {
        // implicit conversion from numeric/double -> integer
        double d = getDouble(column_handle);
        return (int)kl::dblround(d, 0);
    }
     else if (dai->type == tango::typeCharacter ||
              dai->type == tango::typeWideCharacter)
    {
        return atoi(getString(column_handle).c_str());
    }

    if (dai->expr == NULL)
    {
        if (!m_rowptr)
            return 0;
            
        unsigned char* ptr = m_rowptr+dai->offset;
        unsigned int retval;

        retval = *(ptr) +
                 (*(ptr+1) << 8) +
                 (*(ptr+2) << 16) +
                 (*(ptr+3) << 24);

        return (signed int)retval;
    }

    if (!dai->eval(dai->result) ||
         (dai->expr->getType() != kscript::Value::typeDouble &&
          dai->expr->getType() != kscript::Value::typeInteger))
    {
        return 0;
    }

    return dai->result->getInteger();
}


bool BaseIterator::isNull(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;

    if (!dai->is_active)
        return true;

    if (dai->expr == NULL)
    {
        // check null
        if (dai->nulls_allowed)
        {
            if (!m_rowptr)
                return true;

            // remove the null bit, if any
            *(m_rowptr + dai->offset - 1) &= 0xfe;
        }
         else
        {
            return false;
        }
    }
     else
    {
        if (!dai->eval(dai->result))
        {
            return true;
        }

        return dai->result->isNull();
    }


    return false;
}

bool BaseIterator::putRowBuffer(const unsigned char* value, int length)
{
    if (!m_rowptr)
        return false;

    memcpy(m_rowptr, value, length);
    return true;
}


bool BaseIterator::putString(tango::objhandle_t column_handle,
                             const std::string& value)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;
    if (dai->expr != NULL || !dai->is_active || !m_rowptr)
        return false;

    // set data
    if (dai->type == tango::typeCharacter)
    {
        memset(m_rowptr + dai->offset, 0, dai->width);

        int write_len = value.length();
        if (write_len > dai->width)
            write_len = dai->width;

        memcpy(m_rowptr + dai->offset, value.c_str(), write_len);
    }
     else if (dai->type == tango::typeWideCharacter)
    {
        kl::string2ucsle(m_rowptr + dai->offset, value, dai->width);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    // this table's row needs to be written out
    m_row_dirty = true;

    return true;
}

bool BaseIterator::putWideString(tango::objhandle_t column_handle,
                                 const std::wstring& value)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;
    if (dai->expr != NULL || !dai->is_active || !m_rowptr)
        return false;

    // set data
    if (dai->type == tango::typeWideCharacter)
    {
        kl::wstring2ucsle(m_rowptr + dai->offset, value, dai->width);
    }
     else if (dai->type == tango::typeCharacter)
    {
        memset(m_rowptr + dai->offset, 0, dai->width);

        std::string ascvalue = kl::tostring(value);

        int write_len = ascvalue.length();
        if (write_len > dai->width)
            write_len = dai->width;

        memcpy(m_rowptr + dai->offset, ascvalue.c_str(), write_len);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    // this table's row needs to be written out
    m_row_dirty = true;

    return true;
}

bool BaseIterator::putDateTime(tango::objhandle_t column_handle,
                               tango::datetime_t value)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;
    if (dai->expr != NULL || !dai->is_active || !m_rowptr)
        return false;

    // set data
    if (dai->type == tango::typeDate)
    {
        unsigned int julian_day = (unsigned int)(value >> 32);

        int2buf(m_rowptr+dai->offset, julian_day);
    }
     else if (dai->type == tango::typeDateTime)
    {
        unsigned int julian_day = (unsigned int)(value >> 32);
        unsigned int time_stamp = (unsigned int)(value & 0xffffffff);

        int2buf(m_rowptr+dai->offset, julian_day);
        int2buf(m_rowptr+dai->offset+4, time_stamp);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    // this table's row needs to be written out
    m_row_dirty = true;

    return true;
}


bool BaseIterator::putDouble(tango::objhandle_t column_handle,
                             double value)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;
    if (dai->expr != NULL || !dai->is_active || !m_rowptr)
        return false;

    if (dai->width < 1)
        return false;

    if (dai->type == tango::typeNumeric)
    {
        dbl2decstr((char*)m_rowptr + dai->offset,
                   value,
                   dai->width,
                   dai->scale);
    }
     else if (dai->type == tango::typeDouble)
    {
        // FIXME: this will only work on little-endian (intel) processors
        memcpy(m_rowptr + dai->offset, &value, sizeof(double));
    }
     else if (dai->type == tango::typeInteger)
    {
        int2buf(m_rowptr + dai->offset, (int)value);
    }
     else
    {
        return false;
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    // this table's row needs to be written out
    m_row_dirty = true;

    return true;
}


bool BaseIterator::putInteger(tango::objhandle_t column_handle,
                              int value)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;
    if (dai->expr != NULL || !dai->is_active || !m_rowptr)
        return false;

    if (dai->type == tango::typeInteger)
    {
        // set data --
        unsigned char* ptr = m_rowptr+dai->offset;
        unsigned int v = (unsigned int)value;
        *(ptr)   = (v) & 0xff;
        *(ptr+1) = (v >> 8) & 0xff;
        *(ptr+2) = (v >> 16) & 0xff;
        *(ptr+3) = (v >> 24) & 0xff;
    }
     else if (dai->type == tango::typeNumeric)
    {
        dbl2decstr((char*)m_rowptr + dai->offset,
                   value,
                   dai->width,
                   dai->scale);
    }
     else if (dai->type == tango::typeDouble)
    {
        // FIXME: this will only work on little-endian (intel) processors
        double d = value;
        memcpy(m_rowptr + dai->offset, &d, sizeof(double));
    }

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    // this table's row needs to be written out
    m_row_dirty = true;

    return true;
}

bool BaseIterator::putBoolean(tango::objhandle_t column_handle, bool value)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;
    if (dai->expr != NULL || !dai->is_active || !m_rowptr)
        return false;

    // set data
    *(m_rowptr + dai->offset) = (value ? 'T' : 'F');

    // check null
    if (dai->nulls_allowed)
    {
        // remove the null bit, if any
        *(m_rowptr + dai->offset - 1) &= 0xfe;
    }

    // this table's row needs to be written out
    m_row_dirty = true;

    return true;
}


bool BaseIterator::putNull(tango::objhandle_t column_handle)
{
    DataAccessInfo* dai = (DataAccessInfo*)column_handle;
    if (dai->expr != NULL || !dai->is_active || !m_rowptr)
        return false;
    
    if (!dai->nulls_allowed)
        return false;

    *(m_rowptr + dai->offset - 1) |= 0x01;

    // this table's row needs to be written out
    m_row_dirty = true;

    return true;
}


void BaseIterator::flushRow()
{
    if (!m_row_dirty)
        return;

    tango::tableord_t table_ord = rowidGetTableOrd(m_rowid);
    tango::rowpos_t row_pos = rowidGetRowPos(m_rowid);
    BaseIteratorTableEntry* te = NULL;

    std::vector<BaseIteratorTableEntry>::iterator it;
    for (it = m_tables.begin(); it != m_tables.end(); ++it)
    {
        if (table_ord == it->table_ord)
        {
            te = &(*it);
            break;
        }
    }

    if (!te)
    {
        return;
    }

    te->table_ptr->writeRow(row_pos, m_rowptr);

    m_row_dirty = false;
}


