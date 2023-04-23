/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2012-04-18
 *
 */


#ifndef H_XDCOMMON_GROUPQUERYENGINE_H
#define H_XDCOMMON_GROUPQUERYENGINE_H


namespace xdcommon
{


bool runGroupQuery(xd::IDatabasePtr db, xd::GroupQueryParams* info, xd::IJob* job);


}; // namespace xdcommon

#endif

