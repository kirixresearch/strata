/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2004-10-11
 *
 */


#ifndef H_APP_FIELDTYPECHOICE_H
#define H_APP_FIELDTYPECHOICE_H


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


inline wxString xdtype2text(int xd_type)
{
    switch (xd_type)
    {
        default:
        case xd::typeUndefined:      return _("Undefined");
        case xd::typeInvalid:        return _("Invalid");
        case xd::typeCharacter:      return _("Character");
        case xd::typeWideCharacter:  return _("Wide Character");
        case xd::typeBinary:         return _("Binary");
        case xd::typeNumeric:        return _("Numeric");
        case xd::typeDouble:         return _("Double");
        case xd::typeInteger:        return _("Integer");
        case xd::typeDate:           return _("Date");
        case xd::typeDateTime:       return _("DateTime");
        case xd::typeBoolean:        return _("Boolean");
    }

    return wxT("");
}


inline int text2xdtype(const wxString& text)
{
    if (text.CmpNoCase(_("Invalid")) == 0)
        return xd::typeInvalid;

    if (text.CmpNoCase(_("Character")) == 0)
        return xd::typeCharacter;

    if (text.CmpNoCase(_("Wide Char.")) == 0)
        return xd::typeWideCharacter;

    if (text.CmpNoCase(_("Wide Character")) == 0)
        return xd::typeWideCharacter;

    if (text.CmpNoCase(_("Binary")) == 0)
        return xd::typeBinary;

    if (text.CmpNoCase(_("Numeric")) == 0)
        return xd::typeNumeric;

    if (text.CmpNoCase(_("Double")) == 0)
        return xd::typeDouble;

    if (text.CmpNoCase(_("Integer")) == 0)
        return xd::typeInteger;

    if (text.CmpNoCase(_("Date")) == 0)
        return xd::typeDate;

    if (text.CmpNoCase(_("DateTime")) == 0)
        return xd::typeDateTime;

    if (text.CmpNoCase(_("Boolean")) == 0)
        return xd::typeBoolean;

    return xd::typeUndefined;
}

inline int xdtype2choice(int xd_type)
{
    switch(xd_type)
    {
        case xd::typeCharacter:      return comboCharacter;
        case xd::typeWideCharacter:  return comboWideCharacter;
        case xd::typeNumeric:        return comboNumeric;
        case xd::typeDouble:         return comboDouble;
        case xd::typeInteger:        return comboInteger;
        case xd::typeDate:           return comboDate;
        case xd::typeDateTime:       return comboDateTime;
        case xd::typeBoolean:        return comboBoolean;
    }

    return comboCharacter;
}

inline int choice2xdtype(int choice_idx)
{
    switch(choice_idx)
    {
        case comboCharacter:        return xd::typeCharacter;
        case comboWideCharacter:    return xd::typeWideCharacter;
        case comboNumeric:          return xd::typeNumeric;
        case comboDouble:           return xd::typeDouble;
        case comboInteger:          return xd::typeInteger;
        case comboDate:             return xd::typeDate;
        case comboDateTime:         return xd::typeDateTime;
        case comboBoolean:          return xd::typeBoolean;
    }

    return xd::typeUndefined;
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

    int getXdType();
    void setXdChoice(int xd_type);

private:

    DECLARE_CLASS(FieldTypeChoice)
};


#endif

