/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2004-10-11
 *
 */


#ifndef __APP_FIELDTYPECHOICE_H
#define __APP_FIELDTYPECHOICE_H


// -- NOTE: The enum and the array of wxStrings below need to
//          correspond to each other exactly --

enum FieldTypeChoiceIndex
{
    comboCharacter = 0,
    comboWideCharacter,
    comboNumeric,
    comboDouble,
    comboInteger,
    comboDate,
    comboDateTime,
    comboBoolean
};



// -- utility functions --


static inline wxString tango2text(int tango_type)
{
    switch (tango_type)
    {
        default:
        case tango::typeUndefined:      return _("Undefined");
        case tango::typeInvalid:        return _("Invalid");
        case tango::typeCharacter:      return _("Character");
        case tango::typeWideCharacter:  return _("Wide Character");
        case tango::typeBinary:         return _("Binary");
        case tango::typeNumeric:        return _("Numeric");
        case tango::typeDouble:         return _("Double");
        case tango::typeInteger:        return _("Integer");
        case tango::typeDate:           return _("Date");
        case tango::typeDateTime:       return _("DateTime");
        case tango::typeBoolean:        return _("Boolean");
    }

    return wxT("");
}

static inline int tango2choice(int tango_type)
{
    switch(tango_type)
    {
        case tango::typeCharacter:      return comboCharacter;
        case tango::typeWideCharacter:  return comboWideCharacter;
        case tango::typeNumeric:        return comboNumeric;
        case tango::typeDouble:         return comboDouble;
        case tango::typeInteger:        return comboInteger;
        case tango::typeDate:           return comboDate;
        case tango::typeDateTime:       return comboDateTime;
        case tango::typeBoolean:        return comboBoolean;
    }

    return comboCharacter;
}

static inline int choice2tango(int choice_idx)
{
    switch(choice_idx)
    {
        case comboCharacter:        return tango::typeCharacter;
        case comboWideCharacter:    return tango::typeWideCharacter;
        case comboNumeric:          return tango::typeNumeric;
        case comboDouble:           return tango::typeDouble;
        case comboInteger:          return tango::typeInteger;
        case comboDate:             return tango::typeDate;
        case comboDateTime:         return tango::typeDateTime;
        case comboBoolean:          return tango::typeBoolean;
    }

    return tango::typeUndefined;
}




class FieldTypeChoice : public wxChoice,
                        public xcm::signal_sink
{

public: // signals
    xcm::signal1<int /* field_type */> sigFieldTypeChanged;

public:
    FieldTypeChoice(wxWindow* window,
                    wxWindowID id = -1,
                    const wxPoint& position = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize);

    ~FieldTypeChoice();

    int getTangoType();
    void setTangoChoice(int tango_type);

private:

    DECLARE_CLASS(FieldTypeChoice)
};


#endif

