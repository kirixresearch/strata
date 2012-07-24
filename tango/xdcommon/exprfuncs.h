/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-10-03
 *
 */


#ifndef __XDCOMMON_EXPRFUNCS_H
#define __XDCOMMON_EXPRFUNCS_H


void func_abs(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_sign(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_exp(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_ln(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_power(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_log(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_cos(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_acos(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_sin(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_asin(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_tan(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_atan(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_rand(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_pi(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_ceiling(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_floor(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_trunc(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_round(kscript::ExprEnv* env, void* param, kscript::Value* retval);

void func_chr(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_trim(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_avg(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_sum(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_stddev(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_variance(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_clean(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_filterdigits(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_filterchars(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_stripchars(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_sortchars(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_contains(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_date(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_day(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_dayofweek(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_striptime(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_empty(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_isnull(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_iif(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_int(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_len(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_left(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_lower(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_ltrim(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_max(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_min(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_month(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_lpad(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_rpad(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_reverse(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_right(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_rtrim(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_space(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_str(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_substr(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_strpart(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_upper(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_val(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_year(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_asckey(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_ascend(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_descend(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_lfind(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_rfind(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_repeat(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_translate(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_replace(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_proper(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_soundex(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_hour(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_minute(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_second(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_tochar(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_monthname(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_dayname(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_curdate(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_curdatetime(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_week(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_binhex(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_crc64(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_md5(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_regexp_m(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_regexp(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_regexp_replace(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_regexp_match(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_hostname(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_json_extract(kscript::ExprEnv* env, void* param, kscript::Value* retval);
void func_strdistance(kscript::ExprEnv* env, void* param, kscript::Value* retval);


#endif


