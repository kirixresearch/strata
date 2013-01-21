/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-09-23
 *
 */


#include "appmain.h"
#include "fieldtypechoice.h"


IMPLEMENT_CLASS(FieldTypeChoice, wxChoice)


enum
{
    ID_Combo = 5224
};


// -- FieldTypeChoice class implementation --

FieldTypeChoice::FieldTypeChoice(wxWindow* parent,
                                   wxWindowID id,
                                   const wxPoint& position,
                                   const wxSize& size)
                                   : wxChoice(parent,
                                              id,
                                              position,
                                              size,
                                              0,
                                              NULL)

{
    // note these need to correspond to the enum
    // at the top of fieldtypechoice.h
    const wxString FieldTypeChoiceLabels[8] =
    {
        _("Character"),
        _("Wide Character"),
        _("Numeric"),
        _("Double"),
        _("Integer"),
        _("Date"),
        _("DateTime"),
        _("Boolean")
    };

    size_t i, count = sizeof(FieldTypeChoiceLabels)/sizeof(wxString);
    for (i = 0; i < count; ++i)
        Append(FieldTypeChoiceLabels[i]);

    SetSelection(0);
}

FieldTypeChoice::~FieldTypeChoice()
{

}

int FieldTypeChoice::getTangoType()
{
    int retval = choice2tango(GetSelection());
    return retval;
}

void FieldTypeChoice::setTangoChoice(int tango_type)
{
    int idx = tango2choice(tango_type);
    SetSelection(idx);
}




