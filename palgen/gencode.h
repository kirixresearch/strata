/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Code Generator Client
 * Author:   Benjamin I. Williams
 * Created:  2004-05-08
 *
 */


#ifndef H_PALGEN_GENCODE_H
#define H_PALGEN_GENCODE_H


wxString calcActivationCode(const wxString& input_app_tag,
                            const wxString& input_site_code,
                            unsigned int input_feature_id,
                            int exp_year,
                            int exp_month,
                            int exp_day);

bool validateSiteCode(const wxString& val_site_code);

wxString calcComputerId(const wxString& input_app_tag);


#endif

