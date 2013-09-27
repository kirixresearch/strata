/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-10
 *
 */


#ifndef __XDNATIVE_UTIL_H
#define __XDNATIVE_UTIL_H


#include "../xdcommon/util.h"


// determine whether two key expressions match
bool getKeyExpressionMatch(const std::wstring& expr1,
                           const std::wstring& expr2);

// determine whether two order expressions match
bool getOrderExpressionMatch(const std::wstring& expr1,
                             const std::wstring& expr2);

std::wstring combineOfsPath(const std::wstring& base_folder,
                            const std::wstring& file_name);

std::vector<std::wstring> getFieldsInExpr(const std::wstring& expr,
                                          xd::IStructurePtr s,
                                          bool recurse_calcfields);

bool findFieldInExpr(const std::wstring& field,
                     const std::wstring& expr,
                     xd::IStructurePtr s,
                     bool recurse_calcfields);



double decstr2dbl(const char* c, int width, int scale);
void dbl2decstr(char* dest, double d, int width, int scale);


// conversion between set id's and table ord's

std::wstring getTableSetId(xd::tableord_t table_ordinal);
xd::tableord_t getSetTableOrd(const std::wstring& set_id);

#endif

