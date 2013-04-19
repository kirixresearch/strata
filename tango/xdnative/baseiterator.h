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
#include "../xdcommon/tango_private.h"


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

    tango::tableord_t tableord;
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
    tango::tableord_t table_ord;
    ITablePtr table_ptr;
    int row_width;
    unsigned char* row_buf;
};

struct BaseIteratorRelInfo
{
    std::wstring relation_id;
    std::wstring tag;
    tango::IIteratorPtr right_iter;
    IIteratorKeyAccessPtr right_iter_int;
    KeyLayout* kl;
};

class AggregateResult;

class BaseIterator :  public tango::IIterator,
                      public IIteratorKeyAccess,
                      public tango::IIteratorRelation,
                      public IXdnativeSetEvents
{
    XCM_CLASS_NAME("xdnative.BaseIterator")
    XCM_BEGIN_INTERFACE_MAP(BaseIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
        XCM_INTERFACE_ENTRY(IIteratorKeyAccess)
        XCM_INTERFACE_ENTRY(tango::IIteratorRelation)
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
    virtual tango::rowpos_t getRowNumber() { return 0; }
    
    void setIteratorFlags(unsigned int mask, unsigned int value) { }

    virtual bool init(tango::IDatabase* database,
                      IXdnativeSet* set,
                      const std::wstring& columns);



    AggregateResult* getAggResultObject(int aggregate_func, const std::wstring& expr);
    void releaseAggResultObject(AggregateResult* agg_res);
    void recalcAggResults();

    // tango::IIterator
    tango::IDatabasePtr getDatabase();
    std::wstring getTable();
    tango::rowpos_t getRowCount();
    void setSet(IXdnativeSetPtr set);

    tango::IStructurePtr getStructure();
    void refreshStructure();
    bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job);

    unsigned int getIteratorFlags();
    void setIteratorFlags(unsigned int new_val);

    tango::IIteratorPtr getChildIterator(tango::IRelationPtr relation);
    tango::IIteratorPtr getFilteredChildIterator(tango::IRelationPtr relation);

    tango::objhandle_t getHandle(const std::wstring& expr);
    int getType(tango::objhandle_t data_handle);
    bool releaseHandle(tango::objhandle_t data_handle);
    tango::IColumnInfoPtr getInfo(tango::objhandle_t data_handle);

    tango::rowid_t getRowId();

    const unsigned char* getRowBuffer();
    int getRowBufferWidth();
    const unsigned char* getRawPtr(tango::objhandle_t column_handle);
    int getRawWidth(tango::objhandle_t column_handle);
    const std::string& getString(tango::objhandle_t column_handle);
    const std::wstring& getWideString(tango::objhandle_t column_handle);
    tango::datetime_t getDateTime(tango::objhandle_t column_handle);
    double getDouble(tango::objhandle_t column_handle);
    int getInteger(tango::objhandle_t column_handle);
    bool getBoolean(tango::objhandle_t column_handle);
    bool isNull(tango::objhandle_t column_handle);

    // row buffer modifiers

    bool putRowBuffer(const unsigned char* buf, int length);
    bool putString(tango::objhandle_t column_handle, const std::string& value);
    bool putWideString(tango::objhandle_t column_handle, const std::wstring& value);
    bool putDateTime(tango::objhandle_t column_handle, tango::datetime_t value);
    bool putDouble(tango::objhandle_t column_handle, double value);
    bool putInteger(tango::objhandle_t column_handle, int value);
    bool putBoolean(tango::objhandle_t column_handle, bool value);
    bool putNull(tango::objhandle_t column_handle);
    void flushRow();

    // IXdnativeSetEvents 
    void onSetDomainUpdated();
    void onSetStructureUpdated();
    void onSetRelationshipsUpdated();
    void onSetRowUpdated(tango::rowid_t rowid);
    void onSetRowDeleted(tango::rowid_t rowid);

protected:

    void clearDAI();
    void refreshDAI();
    DataAccessInfo* lookupDAI(const std::wstring& expr);
    void colinfo2dai(DataAccessInfo* dai, tango::IColumnInfo* colinfo);
    BaseIteratorTableEntry* registerTable(ITablePtr tbl);
    bool baseClone(BaseIterator* new_iter);
    void bindExprParser(kscript::ExprParser* parser, ParserBindInfo* info);
    bool initStructure();
    void appendCalcFields(tango::IStructure* structure);
    bool refreshRelInfo(BaseIteratorRelInfo& info);

    void goRow(const tango::rowid_t& rowid);
    void setRowId(const tango::rowid_t& rowid);

    static bool base_iterator_parse_hook(kscript::ExprParseHookInfo& hook_info);

public:
    unsigned char* m_rowptr;
    bool m_row_dirty;
    tango::rowid_t m_rowid;
    
protected:
    IXdnativeDatabasePtr m_dbi;
    tango::IDatabasePtr m_database;
    std::wstring m_columns;

private:
    
    xcm::mutex m_obj_mutex;
    xcm::mutex m_agg_mutex;
    xcm::mutex m_rel_mutex;

    std::vector<DataAccessInfo*> m_dai_entries;
    std::map<std::wstring, DataAccessInfo*, kl::cmp_nocase> m_dai_lookup;
    std::vector<BaseIteratorTableEntry> m_tables;
    std::vector<AggregateResult*> m_aggregate_results;
    std::vector<tango::IColumnInfoPtr> m_calc_fields;
    std::vector<BaseIteratorRelInfo> m_relations;
    tango::IRelationEnumPtr m_relenum;

    tango::IStructurePtr m_iter_structure;  // iter structure
    tango::IStructurePtr m_set_structure;   // set structure
    IXdnativeSetPtr m_set;                 // set internal ptr
    unsigned int m_iter_flags;
};




#endif

