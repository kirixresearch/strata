/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-07-10
 *
 */


#ifndef __APP_SCRIPTBANNER_H
#define __APP_SCRIPTBANNER_H


class BannerBox : public FormControl
{
public:

// TODO: need to add "get" functions that correspond to each of the set functions
//       in order to make this class consistent with other controls.

    BEGIN_KSCRIPT_DERIVED_CLASS("BannerBox", BannerBox, FormControl)

        KSCRIPT_GUI_METHOD("constructor", BannerBox::constructor)
        KSCRIPT_GUI_METHOD("setFont", BannerBox::setFont)
        KSCRIPT_GUI_METHOD("setStartColor", BannerBox::setStartColor)
        KSCRIPT_GUI_METHOD("setEndColor", BannerBox::setEndColor)
        KSCRIPT_GUI_METHOD("setTextColor", BannerBox::setTextColor)
        KSCRIPT_GUI_METHOD("setText", BannerBox::setText)
        KSCRIPT_GUI_METHOD("setTextAlignment", BannerBox::setTextAlignment)
        KSCRIPT_GUI_METHOD("setTextPadding", BannerBox::setTextPadding)
        KSCRIPT_GUI_METHOD("setGradientDirection", BannerBox::setGradientDirection)
        KSCRIPT_GUI_METHOD("setBorder", BannerBox::setBorder)
        KSCRIPT_GUI_METHOD("setBorderColor", BannerBox::setBorderColor)
        
        KSCRIPT_CONSTANT_INTEGER("AlignTextLeft",      AlignTextLeft)
        KSCRIPT_CONSTANT_INTEGER("AlignTextCenter",    AlignTextCenter)
        KSCRIPT_CONSTANT_INTEGER("AlignTextRight",     AlignTextRight)
        KSCRIPT_CONSTANT_INTEGER("NoGradient",         NoGradient)
        KSCRIPT_CONSTANT_INTEGER("VerticalGradient",   VerticalGradient)
        KSCRIPT_CONSTANT_INTEGER("HorizontalGradient", HorizontalGradient)
        KSCRIPT_CONSTANT_INTEGER("Top",                Top)
        KSCRIPT_CONSTANT_INTEGER("Bottom",             Bottom)
        KSCRIPT_CONSTANT_INTEGER("Left",               Left)
        KSCRIPT_CONSTANT_INTEGER("Right",              Right)
        KSCRIPT_CONSTANT_INTEGER("All",                All)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        AlignTextLeft = kcl::BannerControl::alignLeft,
        AlignTextCenter = kcl::BannerControl::alignCenter,
        AlignTextRight = kcl::BannerControl::alignRight,
        
        NoGradient = kcl::BannerControl::gradientNone,
        VerticalGradient = kcl::BannerControl::gradientVertical,
        HorizontalGradient = kcl::BannerControl::gradientHorizontal
    };
    
    enum
    {
        Top = kcl::BannerControl::borderTop,
        Bottom = kcl::BannerControl::borderBottom,
        Left = kcl::BannerControl::borderLeft,
        Right = kcl::BannerControl::borderRight,
        All = kcl::BannerControl::borderAll
    };

public:

    BannerBox();
    ~BannerBox();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setFont(kscript::ExprEnv* env, kscript::Value* retval);
    void setStartColor(kscript::ExprEnv* env, kscript::Value* retval);
    void setEndColor(kscript::ExprEnv* env, kscript::Value* retval);
    void setTextColor(kscript::ExprEnv* env, kscript::Value* retval);
    void setText(kscript::ExprEnv* env, kscript::Value* retval);
    void setTextAlignment(kscript::ExprEnv* env, kscript::Value* retval);
    void setTextPadding(kscript::ExprEnv* env, kscript::Value* retval);
    void setGradientDirection(kscript::ExprEnv* env, kscript::Value* retval);
    void setBorder(kscript::ExprEnv* env, kscript::Value* retval);
    void setBorderColor(kscript::ExprEnv* env, kscript::Value* retval);

private:

    kcl::BannerControl* m_ctrl;
};


#endif  // __APP_SCRIPTBANNER_H


