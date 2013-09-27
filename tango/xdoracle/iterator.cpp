/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-04-18
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/klib.h>
#include <oci.h>
#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"


const std::string empty_string = "";
const std::wstring empty_wstring = L"";


OracleIterator::OracleIterator(OracleDatabase* database, OracleSet* set)
{
    m_database = database;
    m_database->ref();

    m_set = set;
    if (m_set)
        m_set->ref();

    m_eof = false;
    m_cache_active = false;
    m_row_pos = 0;
}

OracleIterator::~OracleIterator()
{
    if (m_stmt)
    {
        OCIHandleFree((dvoid*)m_stmt, OCI_HTYPE_STMT);
    }

    if (m_err)
    {
        OCIHandleFree((dvoid*)m_err, OCI_HTYPE_ERROR);
    }


    // clean up field vector and expression vector

    std::vector<OracleDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
        delete (*it);

    if (m_set)
        m_set->unref();

    m_database->unref();
}

bool OracleIterator::init(const std::wstring& query)
{
    // allocate error handle
    if (OCI_SUCCESS != m_database->checkerr(m_err, OCIHandleAlloc((dvoid*)m_env,
                                                      (dvoid**)&m_err,
                                                      OCI_HTYPE_ERROR,
                                                      (size_t)0,
                                                      (dvoid**)0)))
    {
        return false;
    }


    // allocate statement handle
    if (OCI_SUCCESS != m_database->checkerr(m_err, OCIHandleAlloc((dvoid*)m_env,
                                                      (dvoid**)&m_stmt,
                                                      OCI_HTYPE_STMT,
                                                      (size_t)0,
                                                      (dvoid**)0)))
    {
        return false;
    }


    // prepare sql statement
    std::string q = kl::tostring(query);
    if (OCI_SUCCESS != m_database->checkerr(m_err, OCIStmtPrepare(m_stmt,
                                                      m_err,
                                                      (text*)q.c_str(),
                                                      (ub4)q.length(),
                                                      (ub4)OCI_NTV_SYNTAX,
                                                      (ub4)OCI_DEFAULT)))
    {
        return false;
    }


    // execute sql statement
    if (OCI_SUCCESS != m_database->checkerr(m_err, OCIStmtExecute(m_svc,
                                                      m_stmt,
                                                      m_err,
                                                      (ub4)0,
                                                      (ub4)0,
                                                      (CONST OCISnapshot*)NULL,
                                                      (OCISnapshot*)NULL,
                                                      OCI_DEFAULT)))
    {
        return false;
    }


    OCIParam* mypard;
    ub4 counter;
    text col_name[255];
    text* col_name_ptr;
    ub4 col_name_len;
    //ub2 col_disp_width;
    ub2 col_width;
    ub2 col_type;
    ub1 col_charset;
    sb2 col_precision;
    sb1 col_scale;
    ub2 col_charset_id;
    sb4 parm_status;


    // clear out the vector
    m_fields.clear();

    // request parameter descriptors
    counter = 1;

    parm_status = OCIParamGet(m_stmt,
                              OCI_HTYPE_STMT,
                              m_err,
                              (void**)&mypard,
                              (ub4)counter);

    while (parm_status == OCI_SUCCESS)
    {
        // retrieve the column name
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid**)&col_name_ptr,
                   (ub4*)&col_name_len,
                   (ub4)OCI_ATTR_NAME,
                   (OCIError*)m_err);

        strncpy((char*)col_name, (char*)col_name_ptr, (size_t)col_name_len);
        if (col_name_len > 250)
            col_name_len = 250;
        col_name[col_name_len] = 0;
        
        // on linux, this part misbahaves when it is executed after the other
        // parameter fetches.  It also is dependent on the optimization level
        // used.  very strange stuff. I suspect some memory
        // is being overwritten somewhere
        //  (BW 5/16/2005 -- this is probably fixed, it was the
        //   wrong size for col_precision (was ub1, now is sb2)
        
        // retrieve the column precision
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_precision,
                   (ub4*)0,
                   (ub4)OCI_ATTR_PRECISION,
                   (OCIError*)m_err);
                  
        // retrieve the column width
        
        // please note that the below was OCI_ATTR_DATA_SIZE, but it
        // returned a multiplied value for some character fields

        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_width,
                   (ub4*)0,
                   (ub4)OCI_ATTR_CHAR_SIZE,
                   (OCIError*)m_err);

        // retrieve the column scale
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_scale,
                   (ub4*)0,
                   (ub4)OCI_ATTR_SCALE,
                   (OCIError*)m_err);

        // retrieve the column charset
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_charset_id,
                   (ub4*)0,
                   (ub4)OCI_ATTR_CHARSET_ID,
                   (OCIError*)m_err);

        // retrieve the column type
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_type,
                   (ub4*)0,
                   (ub4)OCI_ATTR_DATA_TYPE,
                   (OCIError*)m_err);
        
        // retrieve the character set
        col_charset = 0;
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_charset,
                   (ub4*)0,
                   (ub4)OCI_ATTR_CHARSET_FORM,
                   (OCIError*)m_err); 
        
        // fixups for number fields which don't have an implicit
        // scale and/or precision
        
        if (col_scale < 0)
        {
            // column scale is unknown, assume 2
            col_scale = 2;
            
            if (col_precision <= 0)
                col_precision = 18;
        }
        
        if (oracle2tangoType(col_type) == xd::typeNumeric)
        {
            if (col_precision <= 0)
            {
                col_precision = 18;
                col_scale = 2;
            }
        }
        
        // create data access info object
        OracleDataAccessInfo* field = new OracleDataAccessInfo;
        field->name = kl::towstring((char*)col_name);
        field->oracle_type = col_type;
        field->oracle_charset = col_charset;
        field->tango_type = oracle2tangoType(field->oracle_type, field->oracle_charset);
        field->width = col_width;
        field->precision = col_precision;
        field->scale = col_scale;
        field->ordinal = counter-1;
        field->str_val = new char[col_width+1];
        field->wstr_val = new wchar_t[col_width+1];
        field->str_len = 0;
        m_fields.push_back(field);

        //printf("%s - type %d tango_type %d width %d\n", col_name, col_type, field->m_tango_type, col_width);
        
        ub2 csid_ucs2 = OCI_UCS2ID;
        ub2 csid_iso8859_1 = OCINlsCharSetNameToId(m_env, (const oratext*)"WE8ISO8859P1");
        ub1 csform_nchar = SQLCS_NCHAR;

        switch (field->tango_type)
        {
            case xd::typeWideCharacter:
                m_database->checkerr(m_err, OCIDefineByPos(m_stmt,
                                               &field->define,
                                               m_err,
                                               counter,
                                               (dvoid*)field->wstr_val,
                                               (col_width+1)*sizeof(wchar_t),
                                               field->oracle_type,
                                               (dvoid*)&field->indicator,
                                               &field->str_len,
                                               (ub2*)0,
                                               (ub4)OCI_DEFAULT));
                
                m_database->checkerr(m_err, OCIAttrSet(field->define, OCI_HTYPE_DEFINE, &csform_nchar, sizeof(csform_nchar), OCI_ATTR_CHARSET_FORM, (OCIError*)m_err));
                m_database->checkerr(m_err, OCIAttrSet(field->define, OCI_HTYPE_DEFINE, &csid_ucs2, sizeof(csid_ucs2), OCI_ATTR_CHARSET_ID, (OCIError*)m_err));
                break;
                
            case xd::typeCharacter:
                m_database->checkerr(m_err, OCIDefineByPos(m_stmt, 
                                               &field->define,
                                               m_err,
                                               counter,
                                               (dvoid*)field->str_val,
                                               col_width+1,
                                               field->oracle_type,
                                               (dvoid*)&field->indicator,
                                               &field->str_len,
                                               (ub2*)0,
                                               (ub4)OCI_DEFAULT));
                m_database->checkerr(m_err, OCIAttrSet(field->define, OCI_HTYPE_DEFINE, &csid_iso8859_1, sizeof(csid_iso8859_1), OCI_ATTR_CHARSET_ID, (OCIError*)m_err));
                break;

            case xd::typeDouble:
            case xd::typeNumeric:
                m_database->checkerr(m_err, OCIDefineByPos(m_stmt,
                                               &field->define,
                                               m_err,
                                               counter,
                                               (dvoid*)&field->dbl_val,
                                               sizeof(double),
                                               SQLT_FLT,
                                               (dvoid*)&field->indicator,
                                               (ub2*)0,
                                               (ub2*)0,
                                               (ub4)OCI_DEFAULT));
                break;

            case xd::typeInteger:
                m_database->checkerr(m_err, OCIDefineByPos(m_stmt,
                                               &field->define,
                                               m_err,
                                               counter,
                                               (dvoid*)&field->int_val,
                                               col_precision,
                                               field->oracle_type,
                                               (dvoid*)&field->indicator,
                                               (ub2*)0,
                                               (ub2*)0,
                                               (ub4)OCI_DEFAULT));
                break;

            case xd::typeDate:
            case xd::typeDateTime:
                m_database->checkerr(m_err, OCIDefineByPos(m_stmt,
                                               &field->define,
                                               m_err,
                                               counter,
                                               (dvoid*)field->date_val,
                                               7,
                                               OCI_TYPECODE_DATE,
                                               (dvoid*)&field->indicator,
                                               (ub2*)0,
                                               (ub2*)0,
                                               (ub4)OCI_DEFAULT));
                break;

            case xd::typeBoolean:
                m_database->checkerr(m_err, OCIDefineByPos(m_stmt,
                                               &field->define,
                                               m_err,
                                               counter,
                                               (dvoid*)&field->bool_val,
                                               sizeof(bool),
                                               field->oracle_type,
                                               (dvoid*)&field->indicator,
                                               (ub2*)0,
                                               (ub2*)0,
                                               (ub4)OCI_DEFAULT));
                break;
                
            default:
                break;
        }

        counter++;
        
        parm_status = OCIParamGet(m_stmt,
                                  OCI_HTYPE_STMT,
                                  m_err,
                                  (void**)&mypard,
                                  (ub4)counter);
    }

    m_row_pos = 0;
    
    // position cursor at the beginning of the table
    sword status = OCIStmtFetch(m_stmt,
                                     m_err,
                                     1,
                                     OCI_FETCH_NEXT,
                                     OCI_DEFAULT);
    
    m_database->checkerr(m_err, status);
                             
    if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO)
    {
        m_eof = true;
    }


    // if m_set is null, create a placeholder set
    if (!m_set)
    {
        // create set and initialize variables
        OracleSet* set = new OracleSet(m_database);
        set->m_env = m_env;
        set->m_svc = m_svc;

        if (!set->init())
        {
            delete set;
            return false;
        }

        m_set = set;
        m_set->ref();
    }



    refreshStructure();
    
    return true;
}

void OracleIterator::setTable(const std::wstring& tbl)
{
    // TODO: implement
}

std::wstring OracleIterator::getTable()
{
    // TODO: implement
    return L"";
}


xd::rowpos_t OracleIterator::getRowCount()
{
    return 0;
}


xd::IDatabasePtr OracleIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr OracleIterator::clone()
{
    return xcm::null;
}


void OracleIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & xd::ifReverseRowCache) != 0) ? true : false;
}

unsigned int OracleIterator::getIteratorFlags()
{
    // if we have a unidirectional iterator, but the back-scroll
    // cache is on, then we still can scroll back
    if (m_cache_active)
    {
        return 0;
    }
    
    return xd::ifForwardOnly;
}


void OracleIterator::saveRowToCache()
{
    if (m_row_pos < m_cache.getRowCount())
        return; // row already saved

    if (m_eof)
        return; // don't save rows past the eof

    m_cache.createRow();
    
    std::vector<OracleDataAccessInfo*>::iterator it, it_begin, it_end;
    it_begin = m_fields.begin();
    it_end = m_fields.end();
   
            
    // place the row into the cache
    for (it = it_begin; it != it_end; ++it)
    {
        // work only on physical oracle fields
        if ((*it)->expr_text.length() > 0 || (*it)->expr)
            continue;

        if ((*it)->indicator)
        {
            m_cache.appendNullColumn();
            continue;
        }

        switch ((*it)->tango_type)
        {
            case xd::typeWideCharacter:
                m_cache.appendColumnData((unsigned char*)((*it)->wstr_val),
                                         (*it)->str_len*sizeof(wchar_t));
                break;
            
            case xd::typeCharacter:
                m_cache.appendColumnData((unsigned char*)((*it)->str_val),
                                         (*it)->str_len*sizeof(char));
                break;

            case xd::typeNumeric:
            case xd::typeDouble:
                m_cache.appendColumnData((unsigned char*)&((*it)->dbl_val),
                                         sizeof(double));
                break;

            case xd::typeInteger:
                m_cache.appendColumnData((unsigned char*)&((*it)->int_val),
                                         sizeof(int));
                break;

            case xd::typeDate:
            case xd::typeDateTime:
                m_cache.appendColumnData((unsigned char*)&((*it)->date_val),
                                         7);
                break;
   
            case xd::typeBoolean:
                m_cache.appendColumnData((unsigned char*)&((*it)->bool_val),
                                         sizeof(bool));
                break;
        }
    }
    
    m_cache.finishRow();
}


void OracleIterator::readRowFromCache(xd::rowpos_t row)
{
    m_cache.goRow((xd::rowpos_t)row);
    m_cache.getRow(m_cache_row);


    bool is_null;
    unsigned int data_size;
    unsigned char* data;
    int col = 0;


    std::vector<OracleDataAccessInfo*>::iterator it, it_begin, it_end;
    it_begin = m_fields.begin();
    it_end = m_fields.end();

    // read cached data into the dai values
    for (it = it_begin; it != it_end; ++it)
    {
        // work only on physical oracle fields
        if ((*it)->expr_text.length() > 0 || (*it)->expr)
            continue;


        data = m_cache_row.getColumnData(col, &data_size, &is_null);
        ++col;
        
        if (!data)
        {
            is_null = true;
        }
        
        if (is_null)
        {
            (*it)->indicator = 1;
            continue;
        }
         else
        {
            (*it)->indicator = 0;
        }
        
        switch ((*it)->tango_type)
        {
            case xd::typeWideCharacter:
                memcpy((*it)->wstr_val, data, data_size);
                (*it)->str_len = (data_size/sizeof(wchar_t));
                break;
            
            case xd::typeCharacter:
                memcpy((*it)->str_val, data, data_size);
                (*it)->str_len = data_size;
                break;

            case xd::typeNumeric:
            case xd::typeDouble:
                (*it)->dbl_val = *(double*)data;
                break;

            case xd::typeInteger:
                (*it)->int_val = *(int*)data;
                break;

            case xd::typeDate:
            case xd::typeDateTime:
                memcpy(&((*it)->date_val), data, 7);
                break;

            case xd::typeBoolean:
                (*it)->bool_val = *(bool*)data;
                break;
        }
    }

}


void OracleIterator::skipWithCache(int delta)
{
    if (delta == 0)
        return;
    
    if (!m_cache.isOk())
    {
        if (!m_cache.init())
        {
            // init failed, deactivate cache
            m_cache_active = false;
        }
    }
    
    

    // save row to cache if necessary
    saveRowToCache();
    
        
    long long desired_row = ((long long)m_row_pos) + delta;
    if (desired_row < 0)
        desired_row = 0;
    
    if ((xd::rowpos_t)desired_row < m_cache.getRowCount())
    {
        readRowFromCache(desired_row);
        m_row_pos = desired_row;
        m_eof = false;
    }
     else
    {
        int i;
        
        for (i = 0; i < delta; ++i)
        {
            saveRowToCache();
            
            if (!oraSucceeded(m_database->checkerr(m_err, OCIStmtFetch(m_stmt,
                             m_err,
                             1,
                             OCI_FETCH_NEXT,
                             OCI_DEFAULT))))
            {
                m_row_pos++;
                m_eof = true;
                return;
            }
            
            m_row_pos++;
            m_eof = false;
        } 
    }
}

void OracleIterator::skip(int delta)
{
    if (m_cache_active)
    {
        skipWithCache(delta);
        return;
    }
    
    if (delta <= 0)
    {
        return;
    }
     else
    {
        int i;
        for (i = 0; i < delta; ++i)
        {
            if (!oraSucceeded(m_database->checkerr(m_err, OCIStmtFetch(m_stmt,
                             m_err,
                             1,
                             OCI_FETCH_NEXT,
                             OCI_DEFAULT))))
            {
                m_eof = true;
                break;
            }
        }
    }
    return;
}

void OracleIterator::goFirst()
{
    if (m_cache_active)
    {
        skipWithCache(-((int)m_row_pos));
        return;
    }
}

void OracleIterator::goLast()
{
    return;
}

xd::rowid_t OracleIterator::getRowId()
{
    return m_row_pos;
}

bool OracleIterator::bof()
{
    return false;
}

bool OracleIterator::eof()
{
    return m_eof;
}

bool OracleIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool OracleIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool OracleIterator::setPos(double pct)
{
    return false;
}

double OracleIterator::getPos()
{
    return 0.0;
}

void OracleIterator::goRow(const xd::rowid_t& row)
{
}

xd::IStructurePtr OracleIterator::getStructure()
{
    Structure* s = new Structure;

    std::vector<OracleDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        xd::IColumnInfoPtr col;

        col = createColInfo((*it)->name,
                            (*it)->oracle_type,
                            (*it)->oracle_charset,
                            (*it)->width,
                            (*it)->precision,
                            (*it)->scale,
                            (*it)->expr_text);
        col->setColumnOrdinal((*it)->ordinal);

        //printf("getStructure() - name %ls type %d in-precision %d in-width %d width %d scale %d\n", col->getName().c_str(), col->getType(), (*it)->m_precision, (*it)->m_width, col->getWidth(), col->getScale());

        s->addColumn(col);
    }
    
    return static_cast<xd::IStructure*>(s);
}

void OracleIterator::refreshStructure()
{
    if (!m_set)
        return;

    xd::IStructurePtr set_structure = m_set->getStructure();
    if (set_structure.isNull())
        return;
        
    // find changed/deleted calc fields
    int i, col_count;
    for (i = 0; i < (int)m_fields.size(); ++i)
    {
        if (!m_fields[i]->isCalculated())
            continue;
            
        delete m_fields[i]->expr;
        m_fields[i]->expr = NULL;
                   
        xd::IColumnInfoPtr col = set_structure->getColumnInfo(m_fields[i]->name);
        if (col.isNull())
        {
            m_fields.erase(m_fields.begin() + i);
            i--;
            continue;
        }
  
        m_fields[i]->tango_type = col->getType();
        m_fields[i]->oracle_type = tango2oracleType(m_fields[i]->tango_type);
        m_fields[i]->width = col->getWidth();
        m_fields[i]->scale = col->getScale();
        m_fields[i]->expr_text = col->getExpression();
    }
    
    // find new calc fields
    
    col_count = set_structure->getColumnCount();
    std::vector<OracleDataAccessInfo*>::iterator it;
    
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr col;
        
        col = set_structure->getColumnInfoByIdx(i);
        if (!col->getCalculated())
            continue;
            
        bool found = false;
        
        for (it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (!(*it)->isCalculated())
                continue;

            if (0 == wcscasecmp((*it)->name.c_str(), col->getName().c_str()))
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // add new calc field
            OracleDataAccessInfo* dai = new OracleDataAccessInfo;
            dai->name = col->getName();
            dai->tango_type = col->getType();
            dai->oracle_type = tango2oracleType(dai->tango_type);
            dai->width = col->getWidth();
            dai->scale = col->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = col->getExpression();
            dai->expr = NULL;
                
            m_fields.push_back(dai);
        }
    }
    
    // parse all expressions
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        (*it)->expr = parse((*it)->expr_text);
    }
}

bool OracleIterator::modifyStructure(xd::IStructure* struct_config,
                                     xd::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<OracleDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                OracleDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                if (it->m_params->getName().length() > 0)
                {
                    std::wstring new_name = it->m_params->getName();
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->m_params->getType() != -1)
                {
                    (*it2)->tango_type = it->m_params->getType();
                    (*it2)->oracle_type = tango2oracleType((*it2)->tango_type);
                }

                if (it->m_params->getWidth() != -1)
                {
                    (*it2)->width = it->m_params->getWidth();
                }

                if (it->m_params->getScale() != -1)
                {
                    (*it2)->scale = it->m_params->getScale();
                }

                if (it->m_params->getExpression().length() > 0)
                {
                    if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->m_params->getExpression();
                    (*it2)->expr = parse(it->m_params->getExpression());
                }
            }
        }
    }

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            OracleDataAccessInfo* dai = new OracleDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->tango_type = it->m_params->getType();
            dai->oracle_type = tango2oracleType(dai->tango_type);
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            m_fields.push_back(dai);
        }
    }

    // handle insert
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        // the insert index is out-of-bounds, continue with other actions
        int insert_idx = it->m_pos;
        if (insert_idx < 0 || insert_idx >= (int)m_fields.size())
            continue;
        
        if (it->m_params->getExpression().length() > 0)
        {
            OracleDataAccessInfo* dai = new OracleDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->tango_type = it->m_params->getType();
            dai->oracle_type = tango2oracleType(dai->tango_type);
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
            m_fields.insert(m_fields.begin()+insert_idx, dai);
        }
    }

    return true;
}


xd::objhandle_t OracleIterator::getHandle(const std::wstring& expr)
{
    std::vector<OracleDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (xd::objhandle_t)(*it);
    }


    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        OracleDataAccessInfo* dai = new OracleDataAccessInfo;
        dai->expr = NULL;
        dai->expr_text = expr;
        dai->tango_type = xd::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<xd::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }
        
        m_exprs.push_back(dai);
        return (xd::objhandle_t)dai;
    }
    



    kscript::ExprParser* parser = parse(expr);
    if (!parser)
    {
        return (xd::objhandle_t)0;
    }


    OracleDataAccessInfo* dai = new OracleDataAccessInfo;
    dai->expr = parser;
    dai->tango_type = kscript2tangoType(parser->getType());
    dai->oracle_type = tango2oracleType(dai->tango_type);
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool OracleIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<OracleDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

xd::IColumnInfoPtr OracleIterator::getInfo(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    xd::IColumnInfoPtr col;
    col = createColInfo(dai->name,
                        dai->oracle_type,
                        dai->oracle_charset,
                        dai->width,
                        dai->precision,
                        dai->scale,
                        dai->expr_text);
                        
    return col;
}

int OracleIterator::getType(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (!dai)
        return 0;
    return dai->tango_type;
}

int OracleIterator::getRawWidth(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* OracleIterator::getRawPtr(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return NULL;
    }
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return NULL;
}

const std::string& OracleIterator::getString(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_string;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->str_result = kl::tostring(dai->expr_result.getString());
        return dai->str_result;
    }
    
    // if null, return empty string
    if (!dai->expr && dai->indicator)
        return empty_string;


    if (dai->tango_type == xd::typeWideCharacter)
    {
        dai->str_result = kl::tostring(getWideString(data_handle));
        return dai->str_result;
    }
     else if (dai->tango_type == xd::typeCharacter)
    {
        dai->str_result.assign(dai->str_val, dai->str_len);
        return dai->str_result;
    }

    return empty_string;
}

const std::wstring& OracleIterator::getWideString(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

    // if null, return empty string
    if (!dai->expr && dai->indicator)
        return empty_wstring;

    if (dai->tango_type == xd::typeCharacter)
    {
        dai->wstr_result = kl::towstring(getString(data_handle));
        return dai->wstr_result;
    }
     else if (dai->tango_type == xd::typeWideCharacter)
    {
        if (dai->expr)
        {
            dai->expr->eval(&dai->expr_result);
            dai->wstr_result = dai->expr_result.getString();
            return dai->wstr_result;
        }

        dai->wstr_result.assign(dai->wstr_val, dai->str_len);
        return dai->wstr_result;
    }

    return empty_wstring;
}

xd::datetime_t OracleIterator::getDateTime(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        kscript::ExprDateTime edt = dai->expr_result.getDateTime();

        xd::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        
        if (dai->tango_type == xd::typeDateTime)
            dt |= edt.time;

        return dt;
    }

    // if null, return null date
    if (!dai->expr && dai->indicator)
        return 0;

    int y = dai->date_val[0];
    y -= 100;
    y *= 100;
    y += (dai->date_val[1] - 100);

    int m = dai->date_val[2];
    int d = dai->date_val[3];

    xd::datetime_t retval = dateToJulian(y, m, d);
    retval <<= 32;

    retval |= (((dai->date_val[4]-1)*3600000) +
               ((dai->date_val[5]-1)*60000) +
               ((dai->date_val[6]-1)*1000));

    return retval;
}

double OracleIterator::getDouble(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0.0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getDouble();
    }

    // if null, return zero
    if (!dai->expr && dai->indicator)
        return 0.0;

    return kl::dblround(dai->dbl_val, dai->scale);
}

int OracleIterator::getInteger(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getInteger();
    }

    // if null, return zero
    if (!dai->expr && dai->indicator)
        return 0;

    return dai->int_val;
}

bool OracleIterator::getBoolean(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getBoolean();
    }

    // if null, return false
    if (!dai->expr && dai->indicator)
        return false;

    return dai->bool_val;
}

bool OracleIterator::isNull(xd::objhandle_t data_handle)
{
    OracleDataAccessInfo* dai = (OracleDataAccessInfo*)data_handle;
    if (dai == NULL)
        return true;

    if (!dai->expr && dai->indicator)
        return true;

    return false;
}



bool OracleIterator::updateCacheRow(xd::rowid_t rowid,
                                    xd::ColumnUpdateInfo* info,
                                    size_t info_size)
{
    saveRowToCache();
    
    size_t i;
    for (i = 0; i < info_size; ++i, ++info)
    {
        OracleDataAccessInfo* dai = (OracleDataAccessInfo*)info->handle;
        int column = dai->ordinal;
        
        if (info->null)
            m_cache.updateValue(m_row_pos, column, NULL, 0);
        
        switch (dai->tango_type)
        {
            case xd::typeCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->str_val.c_str(),
                                    info->str_val.length());
                break;

            case xd::typeWideCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->wstr_val.c_str(),
                                    info->wstr_val.length()*sizeof(wchar_t));
                break;


            case xd::typeNumeric:
            case xd::typeDouble:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->dbl_val,
                                    sizeof(double));
                break;

            case xd::typeInteger:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->int_val,
                                    sizeof(int));
                break;

            case xd::typeDate:
            {
                unsigned char oradate[7];
                
                xd::DateTime dt;
                dt.setDateTime(info->date_val);
                
                oradate[0] = (dt.getYear()/100)+100;
                oradate[1] = (dt.getYear()%100)+100;
                oradate[2] = dt.getMonth();
                oradate[3] = dt.getDay();
                oradate[4] = 1;
                oradate[5] = 1;
                oradate[6] = 1;
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)oradate,
                                    7);
                break;
            }
            break;
            
            case xd::typeDateTime:
            {
                unsigned char oradate[7];
                
                xd::DateTime dt;
                dt.setDateTime(info->date_val);
                
                oradate[0] = (dt.getYear()/100)+100;
                oradate[1] = (dt.getYear()%100)+100;
                oradate[2] = dt.getMonth();
                oradate[3] = dt.getDay();
                oradate[4] = dt.getHour()+1;
                oradate[5] = dt.getMinute()+1;
                oradate[6] = dt.getSecond()+1;
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)oradate,
                                    7);
                break;
            }
            break;
            
            case xd::typeBoolean:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->bool_val,
                                    sizeof(bool));
                break;
        }
    }


    readRowFromCache(m_row_pos);


    return true;
}
