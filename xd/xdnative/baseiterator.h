/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-01
 *
 */


#ifndef __XDNATIVE_BASEITERATOR_H
#define __XDNATIVE_BASEITERATOR_H


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <set>
#include <map>
#include <functional>

#include "xdnative_private.h"
#include "../xdcommon/xd_private.h"


class BaseIterator;
class KeyLayout;
struct ParserBindInfo;

namespace kscript
{
    class Value;
    class ExprParser;
    struct ExprParseHookInfo;
};


struct DataAccessInfo
{
    std::wstring dai_text;
    std::wstring name;
    std::wstring expr_text;

    kscript::Value* result;
    std::wstring wstr_result;
    std::string str_result;

    int type;
    int offset;
    int width;
    int scale;
    int ordinal;
    bool nulls_allowed;

    xd::tableord_t tableord;
    kscript::ExprParser* expr;
    unsigned int depth;   // prevents against infinite recursion
    BaseIterator* it;     // for bindings
    bool is_active;       // true if this dai is active
    bool is_column;       // true if this dai is a column/dyncol
    unsigned char* raw_buf;
    KeyLayout* key_layout;

    // methods
    DataAccessInfo();
    ~DataAccessInfo();

    bool eval(kscript::Value* result);
};


struct BaseIteratorTableEntry
{
    xd::tableord_t table_ord;
    ITablePtr table_ptr;
    int row_width;
    unsigned char* row_buf;
};

struct BaseIteratorRelInfo
{
    std::wstring relation_id;
    std::wstring tag;
    xd::IIteratorPtr right_iter;
    xd::IIteratorKeyAccessPtr right_iter_int;
    KeyLayout* kl;
};

class AggregateResult;
class XdnativeDatabase;
class BaseIterator :  public xd::IIterator,
                      public xd::IIteratorKeyAccess,
                      public xd::IIteratorRelation,
                      public IXdnativeSetEvents
{
    XCM_CLASS_NAME("xdnative.BaseIterator")
    XCM_BEGIN_INTERFACE_MAP(BaseIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_ENTRY(xd::IIteratorKeyAccess)
        XCM_INTERFACE_ENTRY(xd::IIteratorRelation)
        XCM_INTERFACE_ENTRY(IXdnativeSetEvents)
    XCM_END_INTERFACE_MAP()


public:

    BaseIterator();
    virtual ~BaseIterator();
    
    virtual void* getKey() { return NULL; }
    virtual int getKeyLength() { return 0; }
    virtual bool setKeyFilter(const void* key, int len) { return false; }
    virtual void getKeyFilter(const void** key, int* len) { *key = NULL; *len = 0; }
    virtual bool setFirstKey() { return false; }
    virtual xd::rowpos_t getRowNumber() { return 0; }
    
    void setIteratorFlags(unsigned int mask, unsigned int value) { }

    virtual bool init(XdnativeDatabase* database,
                      IXdnativeSet* set,
                      const std::wstring& columns);

    AggregateResult* getAggResultObject(int aggregate_func, const std::wstring& expr);
    void releaseAggResultObject(AggregateResult* agg_res);
    void recalcAggResults();

    // xd::IIterator
    void setTable(const std::wstring& tbl);
    std::wstring getTable();
    xd::rowpos_t getRowCount();
    void setSet(IXdnativeSetPtr set);

    xd::Structure getStructure();
    bool refreshStructure();
    bool modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job);

    unsigned int getIteratorFlags();
    void setIteratorFlags(unsigned int new_val);

    xd::IIteratorPtr getChildIterator(xd::IRelationPtr relation);
    xd::IIteratorPtr getFilteredChildIterator(xd::IRelationPtr relation);

    xd::objhandle_t getHandle(const std::wstring& expr);
    int getType(xd::objhandle_t data_handle);
    bool releaseHandle(xd::objhandle_t data_handle);
    xd::ColumnInfo getInfo(xd::objhandle_t data_handle);

    xd::rowid_t getRowId();

    const unsigned char* getRowBuffer();
    int getRowBufferWidth();
    const unsigned char* getRawPtr(xd::objhandle_t column_handle);
    int getRawWidth(xd::objhandle_t column_handle);
    const std::string& getString(xd::objhandle_t column_handle);
    const std::wstring& getWideString(xd::objhandle_t column_handle);
    xd::datetime_t getDateTime(xd::objhandle_t column_handle);
    double getDouble(xd::objhandle_t column_handle);
    int getInteger(xd::objhandle_t column_handle);
    bool getBoolean(xd::objhandle_t column_handle);
    bool isNull(xd::objhandle_t column_handle);

    // row buffer modifiers

    bool putString(xd::objhandle_t column_handle, const std::string& value);
    bool putWideString(xd::objhandle_t column_handle, const std::wstring& value);
    bool putDateTime(xd::objhandle_t column_handle, xd::datetime_t value);
    bool putDouble(xd::objhandle_t column_handle, double value);
    bool putInteger(xd::objhandle_t column_handle, int value);
    bool putBoolean(xd::objhandle_t column_handle, bool value);
    bool putNull(xd::objhandle_t column_handle);
    void flushRow();

    // IXdnativeSetEvents 
    void onSetDomainUpdated();
    void onSetStructureUpdated();
    void onSetRelationshipsUpdated();
    void onSetRowUpdated(xd::rowid_t rowid);
    void onSetRowDeleted(xd::rowid_t rowid);

protected:

    void clearDAI();
    void refreshDAI();
    DataAccessInfo* lookupDAI(const std::wstring& expr);
    void colinfo2dai(DataAccessInfo* dai, const xd::ColumnInfo& colinfo);
    BaseIteratorTableEntry* registerTable(ITablePtr tbl);
    bool baseClone(BaseIterator* new_iter);
    void bindExprParser(kscript::ExprParser* parser, ParserBindInfo* info);
    bool initStructure();
    void appendCalcFields(xd::Structure& structure);
    bool refreshRelInfo(BaseIteratorRelInfo& info);

    void goRow(const xd::rowid_t& rowid);
    void setRowId(const xd::rowid_t& rowid);

    static bool base_iterator_parse_hook(kscript::ExprParseHookInfo& hook_info);

public:
    unsigned char* m_rowptr;
    bool m_row_dirty;
    xd::rowid_t m_rowid;
    
protected:
    XdnativeDatabase* m_database;
    std::wstring m_columns;
    xd::Structure m_iter_structure;  // iter structure
    xd::Structure m_set_structure;   // set structure
    IXdnativeSetPtr m_set;               // set internal ptr

private:
    
    kl::mutex m_agg_mutex;
    kl::mutex m_rel_mutex;

    std::vector<DataAccessInfo*> m_dai_entries;
    std::map<std::wstring, DataAccessInfo*, kl::cmp_nocase> m_dai_lookup;
    std::vector<BaseIteratorTableEntry> m_tables;
    std::vector<AggregateResult*> m_aggregate_results;
    std::vector<xd::ColumnInfo> m_calc_fields;
    std::vector<BaseIteratorRelInfo> m_relations;
    xd::IRelationEnumPtr m_relenum;

    unsigned int m_iter_flags;
};




#endif

