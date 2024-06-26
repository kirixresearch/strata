/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-05-20
 *
 */


#include <kl/klib.h>
#include <kl/md5.h>
#include <oci.h>
#include <xd/xd.h>
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/extfileinfo.h"


// -- OracleSet class implementation --

OracleSet::OracleSet(OracleDatabase* database)
{
    m_env = NULL;
    m_svc = NULL;
    m_stmt = NULL;
    m_err = NULL;

    m_database = database;
    m_database->ref();
}

OracleSet::~OracleSet()
{
    if (m_stmt)
    {
        OCIHandleFree((dvoid*)m_stmt, OCI_HTYPE_STMT);
    }

    if (m_err)
    {
        OCIHandleFree((dvoid*)m_err, OCI_HTYPE_ERROR);
    }

    m_database->unref();
}

bool OracleSet::init()
{
    // set the set info filename
    if (m_tablename.length() > 0)
    {
        xd::IAttributesPtr attr = m_database->getAttributes();
        std::wstring definition_path = attr->getStringAttribute(xd::dbattrDefinitionDirectory);

        //setConfigFilePath(ExtFileInfo::getConfigFilenameFromSetId(definition_path, getSetId()));
    }
    
    return true;
}



std::wstring OracleSet::getSetId()
{
    IOracleDatabasePtr db = m_database;
    
    std::wstring id = L"xdoracle:";
    id += db->getServer();
    id += L":";
    id += db->getDatabase();
    id += L":";
    id += m_tablename;
    
    return kl::md5str(id);
}

xd::Structure OracleSet::getStructure()
{
    xd::Structure s;

    if (!m_env)
    {
        return xd::Structure();
    }

    if (!m_svc)
    {
        return xd::Structure();
    }
    
    // allocate error handle
    if (!m_err)
    {
        OCIHandleAlloc((dvoid*)m_env,
                       (dvoid**)&m_err,
                       OCI_HTYPE_ERROR,
                       (size_t)0,
                       (dvoid**)0);
    }

    // allocate statement handle
    if (!m_stmt)
    {
        OCIHandleAlloc((dvoid*)m_env,
                       (dvoid**)&m_stmt,
                       OCI_HTYPE_STMT,
                       (size_t)0,
                       (dvoid**)0);
    }

    // initialize the structure based on our sql query

    OCIParam* mypard;
    ub4 counter;
    ub2 dtype;
    text col_name[255];
    text* col_name_ptr;
    ub4 col_name_len;
    ub2 col_width;
    ub2 col_type;
    ub1 col_charset;
    sb2 col_precision;
    sb1 col_scale;
    sb4 parm_status;

    // create select statement
    wchar_t query[1024];
    swprintf(query, 1024, L"SELECT * FROM %ls WHERE 1=0", m_tablename.c_str());

    // prepare sql statement
    std::string asc_query = kl::tostring(query);
    OCIStmtPrepare(m_stmt,
                   m_err,
                   (text*)asc_query.c_str(),
                   (ub4)asc_query.length(),
                   (ub4)OCI_NTV_SYNTAX,
                   (ub4)OCI_DEFAULT);

    // execute sql statement
    if (OCI_SUCCESS != m_database->checkerr(m_err, OCIStmtExecute(
                                           m_svc,
                                           m_stmt,
                                           m_err,
                                           (ub4)0,
                                           (ub4)0,
                                           (CONST OCISnapshot*)NULL,
                                           (OCISnapshot*)NULL,
                                           OCI_DEFAULT)))
    {
        return xd::Structure();
    }

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

        // retrieve the column width
        
        // please note that the below was OCI_ATTR_DATA_SIZE, but it
        // returned a multiplied value for some character fields
        
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_width,
                   (ub4*)0,
                   (ub4)OCI_ATTR_CHAR_SIZE,
                   (OCIError*)m_err);

        // retrieve the column precision
        OCIAttrGet((dvoid*)mypard, 
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_precision,
                   (ub4*)0,
                   (ub4)OCI_ATTR_PRECISION,
                   (OCIError*)m_err);

        // retrieve the column scale
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&col_scale,
                   (ub4*)0,
                   (ub4)OCI_ATTR_SCALE,
                   (OCIError*)m_err);

        // retrieve the data type
        OCIAttrGet((dvoid*)mypard,
                   (ub4)OCI_DTYPE_PARAM,
                   (dvoid*)&dtype,
                   (ub4*)0,
                   (ub4)OCI_ATTR_DATA_TYPE,
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
                        
        std::wstring wcol_name = kl::towstring((char*)col_name);

        // create the column in the xd::IStructure
        xd::ColumnInfo col = createColInfo(wcol_name,
                                           col_type,
                                           col_charset,
                                           col_width,
                                           col_precision,
                                           col_scale);
        s.createColumn(col);

        counter++;

        parm_status = OCIParamGet(m_stmt,
                                  OCI_HTYPE_STMT,
                                  m_err,
                                  (void**)&mypard,
                                  (ub4)counter);
    }


    //appendCalcFields(s);


    return s;
}

/*
bool OracleSet::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    KL_AUTO_LOCK(m_object_mutex);

    bool done_flag = false;
    if (!CommonBaseSet::modifyStructure(struct_config, &done_flag))
        return false;
    if (done_flag)
        return true;
        
    unsigned int processed_action_count = 0;

    xd::Structure current_struct = getStructure();
    IStructureInternalPtr s = struct_config;
    std::vector<StructureAction>& actions = s->getStructureActions();
    std::vector<StructureAction>::iterator it;

    std::wstring command;
    command.reserve(1024);

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionDelete)
        {
            command = L"ALTER TABLE ";
            command += m_tablename;
            command += L" DROP COLUMN ";
            command += it->m_colname;

            xcm::IObjectPtr result_obj;
            m_database->execute(command, 0, result_obj, NULL);
            command = L"";
        }
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionModify)
        {
        }
    }

    int i;
    int col_count = current_struct->getColumnCount();
    xd::ColumnInfo col_info;
    bool found = false;

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionCreate)
        {
            for (i = 0; i < col_count; ++i)
            {
                col_info = current_struct->getColumnInfoByIdx(i);

                if (!wcscasecmp(col_info->getName().c_str(),
                                it->m_params->getName().c_str()))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                command = L"ALTER TABLE ";
                command += m_tablename;
                command += L" ADD ";
                command += createOracleFieldString(it->m_params->getName(),
                                                   it->m_params->getType(),
                                                   it->m_params->getWidth(),
                                                   it->m_params->getScale(),
                                                   false);

                xcm::IObjectPtr result_obj;
                m_database->execute(command, 0, result_obj, NULL);
                command = L"";
            }

            found = false;
        }
    }

    return true;
}
*/

xd::IIteratorPtr OracleSet::createIterator(const std::wstring& columns,
                                              const std::wstring& order,
                                              xd::IJob* job)
{
    std::wstring query;
    query.reserve(1024);

    query = L"SELECT * FROM ";
    query += m_tablename;

    if (order.length() > 0)
    {
        query += L" ORDER BY ";
        query += order;
    }

    OracleIterator* iter = new OracleIterator(m_database, this);
    iter->m_env = m_env;
    iter->m_svc = m_svc;
    iter->m_name = m_tablename;
    if (!iter->init(query))
    {
        delete iter;
        return xcm::null;
    }

    return static_cast<xd::IIterator*>(iter);
}

xd::rowpos_t OracleSet::getRowCount()
{
    return 0;
}




// -- OracleRowInserter class implementation --


const int insert_rows_per_buf = 1000;


OracleRowInserter::OracleRowInserter(OracleSet* set)
{
    m_set = set;
    m_set->ref();

    m_env = set->m_env; 
    m_svc = set->m_svc;
    m_err = NULL;
    m_stmt = NULL;
    m_buf = NULL;

    m_inserting = false;

    // allocate error handle
    OCIHandleAlloc((dvoid*)m_env,
                   (dvoid**)&m_err,
                   OCI_HTYPE_ERROR,
                   (size_t)0,
                   (dvoid**)0);

    // allocate statement handle
    OCIHandleAlloc((dvoid*)m_env,
                   (dvoid**)&m_stmt,
                   OCI_HTYPE_STMT,
                   (size_t)0,
                   (dvoid**)0);
}

OracleRowInserter::~OracleRowInserter()
{
    // free all insert field data
    std::vector<OracleInsertData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    delete[] m_buf;

    m_set->unref();
}

xd::objhandle_t OracleRowInserter::getHandle(const std::wstring& column_name)
{
    if (!m_inserting)
        return 0;

    std::vector<OracleInsertData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (wcscasecmp(column_name.c_str(), (*it)->m_name.c_str()) == 0)
        {
            return (xd::objhandle_t)(*it);
        }
    }

    return (xd::objhandle_t)0;
}

bool OracleRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                  const unsigned char* value,
                                  int length)
{
    OracleInsertData* f = (OracleInsertData*)column_handle;
    unsigned char* pos = m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset;

    memset(pos, 0, f->m_oracle_width);
    memcpy(pos, value, length);

    return true;
}

bool OracleRowInserter::putString(xd::objhandle_t column_handle,
                                  const std::string& value)
{
    OracleInsertData* f = (OracleInsertData*)column_handle;

    size_t len = value.length();
    unsigned char* pos = m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset;

    if ((int)len <= f->m_xd_width)
    {
        memcpy(pos, value.c_str(), len+1);
    }
     else
    {
        memcpy(pos, value.c_str(), f->m_xd_width);
        pos[f->m_xd_width] = 0;
    }

    return true;
}

bool OracleRowInserter::putWideString(xd::objhandle_t column_handle,
                                      const std::wstring& value)
{
    OracleInsertData* f = (OracleInsertData*)column_handle;

    if (f->m_xd_type == xd::typeCharacter)
    {
        return putString(column_handle, kl::tostring(value));
    }

    size_t len = value.length();
    unsigned char* pos = m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset;

    if ((int)len <= f->m_xd_width)
    {
        memcpy(pos, value.c_str(), (len+1)*sizeof(wchar_t));
    }
     else
    {
        memcpy(pos, value.c_str(), f->m_xd_width * sizeof(wchar_t));
        
        // because the buffer is ucs2, we need a word-lengthed zero terminator
        pos[f->m_xd_width] = 0;
        pos[f->m_xd_width+1] = 0;
    }

    return true;
}

bool OracleRowInserter::putDouble(xd::objhandle_t column_handle,
                                  double value)
{
    OracleInsertData* f = (OracleInsertData*)column_handle;

    if (f->m_oracle_type == SQLT_INT)
    {
        int tempi = (int)value;
        memcpy(m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset,
               &tempi,
               sizeof(int));
    }
     else if (f->m_oracle_type == SQLT_FLT)
    {
        double d = kl::dblround(value, f->m_oracle_scale);
        memcpy(m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset,
               &d,
               sizeof(double));
    }
     else
    {
        return false;
    }

    return true;
}

bool OracleRowInserter::putInteger(xd::objhandle_t column_handle,
                                   int value)
{
    OracleInsertData* f = (OracleInsertData*)column_handle;

    if (f->m_oracle_type == SQLT_INT)
    {
        memcpy(m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset,
               &value,
               sizeof(int));
    }
     else if (f->m_oracle_type == SQLT_FLT)
    {
        double d = value;
        memcpy(m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset,
               &d,
               sizeof(double));
    }
     else
    {
        return false;
    }

    return true;
}

bool OracleRowInserter::putBoolean(xd::objhandle_t column_handle,
                                   bool value)
{
    OracleInsertData* f = (OracleInsertData*)column_handle;

    if (f->m_xd_type == xd::typeBoolean)
    {
        memcpy(m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset,
               &value,
               f->m_oracle_width);
        return true;
    }
    
    if (f->m_xd_type == xd::typeCharacter)
    {
        return putString(column_handle, value ? "T" : "F");
    }

    return true;
}

bool OracleRowInserter::putDateTime(xd::objhandle_t column_handle,
                                    xd::datetime_t datetime)
{
    OracleInsertData* f = (OracleInsertData*)column_handle;
    xd::DateTime dt(datetime);

    int c = dt.getYear();
    int y = c%100;
    y = y+100;
    c = c/100;
    c = c+100;

    unsigned char* p = m_buf+(m_cur_buf_row*m_row_width)+f->m_buf_offset;

    *p = c;
    *(p+1) = y;
    *(p+2) = dt.getMonth();
    *(p+3) = dt.getDay();
    *(p+4) = dt.getHour()+1;
    *(p+5) = dt.getMinute()+1;
    *(p+6) = dt.getSecond()+1;

    return true;
}


bool OracleRowInserter::putNull(xd::objhandle_t column_handle)
{
    if (!column_handle)
        return false;
        
    OracleInsertData* f = (OracleInsertData*)column_handle;
    sb2* ind = (sb2*)(m_buf+(m_cur_buf_row*m_row_width)+f->m_ind_offset);
    *ind = -1;
    return true;
}

bool OracleRowInserter::startInsert(const std::wstring& col_list)
{
    // for now insert all fields; later a parameter
    // will be passed in startInsert() which must be parsed

    xd::Structure s = m_set->getStructure();

    size_t i, col_count = s.getColumnCount();
    std::vector<std::wstring> insert_fields;

    for (i = 0; i < col_count; ++i)
    {
        insert_fields.push_back(s.getColumnName(i));
    }

    std::wstring field_str;
    std::wstring value_str;
    m_row_width = 0;

    int buf_offset = 0;
    int field_counter = 0;
    std::string var_name;  // bind the fields to variables

    std::vector<std::wstring>::iterator it;
    for (it = insert_fields.begin(); it != insert_fields.end(); ++it)
    {
        const xd::ColumnInfo& col_info = s.getColumnInfo(*it);
        if (col_info.isNull())
            return false;

        OracleInsertData* field = new OracleInsertData;
        field->m_name = col_info.name;

        // oracle buffer offsets must be two-byte aligned
        if (buf_offset % 2 != 0)
            buf_offset++;

        field->m_ind_offset = buf_offset;
        buf_offset += sizeof(sb2);
        
        field->m_buf_offset = buf_offset;

        field->m_xd_type = col_info.type;
        field->m_xd_width = col_info.width;
        field->m_xd_scale = col_info.scale;

        switch (field->m_xd_type)
        {
            case xd::typeCharacter:
                field->m_oracle_type = SQLT_STR;
                field->m_oracle_width = (field->m_xd_width+1)*sizeof(char);
                break;

            case xd::typeWideCharacter:
                field->m_oracle_type = SQLT_STR;
                field->m_oracle_width = (field->m_xd_width+1)*(sizeof(wchar_t));
                break;

            case xd::typeInteger:
                field->m_oracle_type = SQLT_INT;
                field->m_oracle_width = sizeof(int);
                break;

            case xd::typeDouble:
            case xd::typeNumeric:
                field->m_oracle_type = SQLT_FLT;
                field->m_oracle_width = sizeof(double);
                break;

            case xd::typeBoolean:
                field->m_oracle_type = SQLT_CHR;
                field->m_oracle_width = 1;
                break;

            case xd::typeDate:
            case xd::typeDateTime:
                field->m_oracle_type = SQLT_DAT;
                field->m_oracle_width = 7;
                break;
        }

        field->m_oracle_scale = field->m_xd_scale;

        m_fields.push_back(field);

        if (it != insert_fields.begin())
        {
            field_str += L", ";
            value_str += L", ";
        }
        field_str += (L"\"" + field->m_name + L"\"");

        // add a binding variable name
        value_str += L":F" + kl::itowstring(++field_counter);

        buf_offset += field->m_oracle_width;
    }
    
    m_row_width = buf_offset;
    
    // oracle buffers must be two-byte aligned
    if (m_row_width % 2 != 0)
        m_row_width++;
    
    // create the insert buffer
    m_buf = new unsigned char[m_row_width*insert_rows_per_buf];
    memset(m_buf, 0, m_row_width*insert_rows_per_buf);

    m_cur_buf_row = 0;

    // create the select statement
    wchar_t insert_stmt[65535];
    swprintf(insert_stmt, 65535, L"INSERT INTO %ls(%ls) VALUES(%ls)",
                                 m_set->m_tablename.c_str(),
                                 field_str.c_str(),
                                 value_str.c_str());

    // prepare the statement
    std::string asc_insert_stmt = kl::tostring(insert_stmt);
    m_set->m_database->checkerr(m_err, OCIStmtPrepare(m_stmt,
                                   m_err,
                                   (text*)asc_insert_stmt.c_str(),
                                   (ub4)asc_insert_stmt.length(),
                                   (ub4)OCI_NTV_SYNTAX,
                                   (ub4)OCI_DEFAULT));



    // ids for unicode fields
    //ub2 csid = OCI_UCS2ID;
    ub2 csid_ucs2 = OCI_UCS2ID;
    ub2 csid_iso8859_1 = OCINlsCharSetNameToId(m_env, (const oratext*)"WE8ISO8859P1");
    ub1 csform_nchar = SQLCS_NCHAR;



    field_counter = 0;

    std::vector<OracleInsertData*>::iterator it2;
    for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
    {
        var_name = ":F" + kl::itostring(++field_counter);

        if (OCI_SUCCESS != m_set->m_database->checkerr(m_err, OCIBindByName(m_stmt,
                                      &(*it2)->m_bind,
                                      m_err,
                                      (text*)var_name.c_str(),
                                      (sb4)var_name.length(),
                                      (dvoid*)(m_buf+(*it2)->m_buf_offset),
                                      (*it2)->m_oracle_width,
                                      (ub2)(*it2)->m_oracle_type,
                                      (dvoid*)(m_buf+(*it2)->m_ind_offset),
                                      (ub2*)0,
                                      (ub2*)0,
                                      (ub4)0,
                                      (ub4*)0,
                                      OCI_DEFAULT)))
        {
            return false;
        }
        
        // if this field is a unicode string,
        // set the bind attribute to OCI_UCS2ID
        if ((*it2)->m_xd_type == xd::typeCharacter)
        {
            m_set->m_database->checkerr(m_err, OCIAttrSet((*it2)->m_bind,
                               OCI_HTYPE_BIND,
                               &csid_iso8859_1,
                               sizeof(csid_iso8859_1),
                               OCI_ATTR_CHARSET_ID,
                               m_err));
        }
         else if ((*it2)->m_xd_type == xd::typeWideCharacter)
        {
            m_set->m_database->checkerr(m_err, OCIAttrSet((*it2)->m_bind,
                                       (ub4)OCI_HTYPE_BIND,
                                       &csform_nchar,
                                       sizeof(csform_nchar),
                                       OCI_ATTR_CHARSET_FORM,
                                       m_err));
            m_set->m_database->checkerr(m_err, OCIAttrSet((*it2)->m_bind,
                                       OCI_HTYPE_BIND,
                                       &csid_ucs2,
                                       sizeof(csid_ucs2),
                                       OCI_ATTR_CHARSET_ID,
                                       m_err));
        }

        m_set->m_database->checkerr(m_err, OCIBindArrayOfStruct((*it2)->m_bind,
                                             m_err,
                                             (ub4)m_row_width,
                                             0,
                                             0,
                                             0));
    }

    m_inserting = true;
    return true;
}

bool OracleRowInserter::insertRow()
{
    if (!m_inserting)
    {
        return false;
    }
    
    m_cur_buf_row++;

    if (m_cur_buf_row == insert_rows_per_buf)
    {
        flushRows();
    }

    return true;
}

bool OracleRowInserter::flushRows()
{
    sword ret1, ret2;

    // insert the max_insert_rows number of rows into the table
    ret1 = m_set->m_database->checkerr(m_err, OCIStmtExecute(m_svc,
                                   m_stmt,
                                   m_err,
                                   (ub4)m_cur_buf_row,
                                   (ub4)0,
                                   (CONST OCISnapshot*)0,
                                   (OCISnapshot*)0,
                                   (ub4)OCI_DEFAULT));

    ret2 = m_set->m_database->checkerr(m_err, OCITransCommit(m_svc, m_err, (ub4)0));
    memset(m_buf, 0, m_row_width*insert_rows_per_buf);
    m_cur_buf_row = 0;

    if ((ret1 != OCI_SUCCESS && ret1 != OCI_SUCCESS_WITH_INFO) ||
        (ret2 != OCI_SUCCESS && ret2 != OCI_SUCCESS_WITH_INFO))
        return false;
         else
        return true;
}

void OracleRowInserter::finishInsert()
{
    flushRows();
    m_inserting = false;

    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
    }
}

bool OracleRowInserter::flush()
{
    return flushRows();
}



