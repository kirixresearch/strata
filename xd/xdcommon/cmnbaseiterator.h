/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-23
 *
 */


#ifndef H_XDCOMMON_CMNBASEITERATOR_H
#define H_XDCOMMON_CMNBASEITERATOR_H

#include "xd_private.h"

// forwards
class KeyLayout;
class CommonBaseIteratorBindInfo;
namespace kscript { class ExprParser; }
namespace kscript { struct ExprParseHookInfo; }

class CommonAggregateResult;

struct CommonBaseIteratorRelInfo
{
    std::wstring relation_id;
    std::wstring tag;
    xd::IIteratorPtr right_iter;
    xd::IIteratorKeyAccessPtr right_iter_int;
    KeyLayout* kl;
};


class CommonBaseIterator : public xd::IIterator, public xd::IIteratorRelation
{
public:

    CommonBaseIterator();
    virtual ~CommonBaseIterator();

    virtual xd::IDatabase* cmniterGetDatabase() = 0; // derivers must implement

    virtual xd::Structure getParserStructure();
    kscript::ExprParser* parse(const std::wstring& expr);
    
    void setIteratorFlags(unsigned int mask, unsigned int value)
    {
    }

    bool refreshRelInfo(CommonBaseIteratorRelInfo& info);
    xd::IIteratorPtr getFilteredChildIterator(xd::IRelationPtr relation);
    xd::IIteratorPtr getChildIterator(xd::IRelationPtr relation);
    CommonAggregateResult* getAggResultObject(int aggregate_func, const std::wstring& expr);
    void releaseAggResultObject(CommonAggregateResult* agg_res);
    void recalcAggResults();

protected:

    virtual void onParserInit(kscript::ExprParser* parser) {}

private:

    static bool script_parse_hook(kscript::ExprParseHookInfo& hook_info);

    std::vector<CommonBaseIteratorBindInfo*> m_bindings;

    std::vector<CommonBaseIteratorRelInfo> m_relations;
    xd::IRelationSchemaPtr m_relschema;
    xd::IRelationEnumPtr m_relenum;
    std::vector<CommonAggregateResult*> m_aggregate_results;
};


#endif


