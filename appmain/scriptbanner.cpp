/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-07-10
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scriptfont.h"
#include "scriptbanner.h"


// (CLASS) BannerBox
// Category: Control
// Derives: FormControl
// Description: A class that represents a banner box control.
// Remarks: The BannerBox class represents a banner box control.
//
// Property(BannerBox.Top): A flag representing whether to show the top border.
// Property(BannerBox.Bottom): A flag representing whether to show the bottom border.
// Property(BannerBox.Left): A flag representing whether to show the left border.
// Property(BannerBox.Right): A flag representing whether to show the right border.
// Property(BannerBox.All): A flag representing whether to show all borders.
// Property(BannerBox.NoGradient): A flag representing no gradient (solid fill color).
// Property(BannerBox.GradientVertical): A flag representing a vertical gradient.
// Property(BannerBox.GradientHorizontal): A flag representing a horizontal gradient.
// Property(BannerBox.AlignTextLeft): A flag representing left text alignment.
// Property(BannerBox.AlignTextCenter): A flag representing centered text alignment.
// Property(BannerBox.AlignTextRight): A flag representing right text alignment.

BannerBox::BannerBox()
{

}

BannerBox::~BannerBox()
{

}

// (CONSTRUCTOR) BannerBox.constructor
// Description: Creates a new BannerBox.
//
// Syntax: BannerBox(text : String,
//                   x_pos : Integer,
//                   y_pos : Integer,
//                   width : Integer,
//                   height : Integer)
//
// Remarks: Creates a new BannerBox having the specified caption
//     |text|, at the position specified by |x_pos| and |y_pos|, and 
//     having dimensions specified by |width| and |height|.
//
// Param(text): The caption to show for the control.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void BannerBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // set default values
    wxString text = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
        text = towx(env->getParam(0)->getString());
    if (param_count > 1)
        m_x = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_y = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_width = env->getParam(3)->getInteger();
    if (param_count > 4)
        m_height = env->getParam(4)->getInteger();
    
    if (param_count < 2)
    {
        // create the control
        m_ctrl = new kcl::BannerControl(getApp()->getTempParent(),
                                        text,
                                        wxDefaultPosition,
                                        wxDefaultSize);
        m_wnd = m_ctrl;

        // if no sizing parameters were specified, used wx's default
        // parameters, since they are equal to the exact size of the text
        wxSize s = m_ctrl->GetSize();
        m_width = s.GetWidth();
        m_height = s.GetHeight();
    }
     else
    {
        // create the control
        m_ctrl = new kcl::BannerControl(getApp()->getTempParent(),
                                        text,
                                        wxPoint(m_x, m_y),
                                        wxSize(m_width, m_height));
        m_wnd = m_ctrl;

        // make sure we update the m_width and m_height member
        // variables based on the actual size of the control
        wxSize s = m_ctrl->GetSize();
        if (m_width != s.GetWidth())
            m_width = s.GetWidth();
        if (m_height != s.GetHeight())
            m_height = s.GetHeight();
    }
}

void BannerBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
        m_ctrl->Enable(false);
}


// (METHOD) BannerBox.setFont
// Description: Sets the font for the banner text.
//
// Syntax: function BannerBox.setFont(font : Font)
//
// Remarks: Sets the |font| for the banner text.
//
// Param(font): The font to use to draw the banner text.

void BannerBox::setFont(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
            
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Font")
    {
        zFont* font = (zFont*)obj;
        m_ctrl->setFont(font->getWxFont());
    }
}

// (METHOD) BannerBox.setStartColor
// Description: Sets the starting color for the banner gradient.
//
// Syntax: function BannerBox.setStartColor(color : Color)
//
// Remarks: Sets the starting |color| for the banner gradient.
//
// Param(color): The starting |color| for the banner gradient.

void BannerBox::setStartColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Color")
    {
        Color* c = (Color*)obj;
        
        wxColour c2(c->getMember(L"red")->getInteger(),
                    c->getMember(L"green")->getInteger(),
                    c->getMember(L"blue")->getInteger());
        m_ctrl->setStartColor(c2);
        return;
    }
}

// (METHOD) BannerBox.setEndColor
// Description: Sets the ending color for the banner gradient.
//
// Syntax: function BannerBox.setEndColor(color : Color)
//
// Remarks: Sets the ending |color| for the banner gradient.
//
// Param(color): The ending |color| for the banner gradient.

void BannerBox::setEndColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Color")
    {
        Color* c = (Color*)obj;
        
        wxColour c2(c->getMember(L"red")->getInteger(),
                    c->getMember(L"green")->getInteger(),
                    c->getMember(L"blue")->getInteger());
        m_ctrl->setEndColor(c2);
        return;
    }
}

// (METHOD) BannerBox.setTextColor
// Description: Sets the text color for the banner text.
//
// Syntax: function BannerBox.setTextColor(color : Color)
//
// Remarks: Sets the |color| for the banner text.
//
// Param(color): The |color| for the banner text.

void BannerBox::setTextColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Color")
    {
        Color* c = (Color*)obj;
        
        wxColour c2(c->getMember(L"red")->getInteger(),
                    c->getMember(L"green")->getInteger(),
                    c->getMember(L"blue")->getInteger());
        m_ctrl->setTextColor(c2);
        return;
    }
}

// (METHOD) BannerBox.setText
// Description: Sets the banner text.
//
// Syntax: function BannerBox.setText(text : String)
//
// Remarks: Sets the banner |text|.
//
// Param(text): The |text| for the banner.

void BannerBox::setText(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::Value* obj = env->getParam(0);
    if (obj->isString())
    {
        m_ctrl->setText(obj->getString());
        return;
    }
}

// (METHOD) BannerBox.setTextAlignment
// Description: Sets the text alignment for the banner.
//
// Syntax: function BannerBox.setTextAlignment(alignment : Integer)
//
// Remarks: Sets the |alignment| for the banner.
//
// Param(alignment): The |alignment| for the banner.  One of 
//     BannerBox.AlignTextLeft, BannerBox.AlignTextCenter or
//     BannerBox.AlignTextRight.

void BannerBox::setTextAlignment(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::Value* obj = env->getParam(0);
    if (obj->isInteger())
    {
        int val = obj->getInteger();
        if (val != AlignTextLeft &&
            val != AlignTextCenter &&
            val != AlignTextRight)
        {
            return;
        }
        
        m_ctrl->setTextAlignment(val);
        return;
    }
}

// (METHOD) BannerBox.setTextPadding
// Description: Sets the text padding for the banner.
//
// Syntax: function BannerBox.setTextPadding(padding : Integer)
//
// Remarks: Sets the |padding| for the banner text, which is the
//     space between the text and the edge of the banner control.
//
// Param(padding): The |padding| for the banner text.

void BannerBox::setTextPadding(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::Value* obj = env->getParam(0);
    if (obj->isInteger())
    {
        int val = obj->getInteger();
        if (val < 0)
            return;
        
        m_ctrl->setTextPadding(val);
        return;
    }
}

// (METHOD) BannerBox.setGradientDirection
// Description: Sets the gradient direction for the banner.
//
// Syntax: function BannerBox.setGradientDirection(direction : Integer)
//
// Remarks: Sets the gradient direction to the value given by |direction|.
//     The gradient |direction| in which the banner color background
//     gradually changes from the starting color to the ending color.
//     If BannerBox.NoGradient is specified, the start color
//     will be used as the background fill color.
//
// Param(direction): The |direction| in which the banner color background
//     changes.  One of BannerBox.NoGradient, BannerBox.GradientVertical or
//     BannerBox.GradientHorizontal.

void BannerBox::setGradientDirection(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::Value* obj = env->getParam(0);
    if (obj->isInteger())
    {
        int val = obj->getInteger();
        if (val != BannerBox::NoGradient &&
            val != BannerBox::VerticalGradient &&
            val != BannerBox::HorizontalGradient)
        {
            return;
        }
        
        m_ctrl->setGradientDirection(val);
        return;
    }
}

// (METHOD) BannerBox.setBorder
// Description: Sets the sides of the banner control that have a border.
//
// Syntax: function BannerBox.setBorder(flags : Integer)
//
// Remarks: Sets the sides of the banner control that have a border
//     given by |flags|.
//
// Param(flags): The sides of the banner that have borders, given
//     by a combination of BannerBox.Left, BannerBox.Right, BannerBox.Top
//     and BannerBox.Bottom, or BannerBox.All for all borders.

void BannerBox::setBorder(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::Value* obj = env->getParam(0);
    if (obj->isInteger())
    {
        int val = obj->getInteger();
        int flags = 0;
        
        if (val & BannerBox::Left)
            flags |= kcl::BannerControl::borderLeft;
        if (val & BannerBox::Right)
            flags |= kcl::BannerControl::borderRight;
        if (val & BannerBox::Top)
            flags |= kcl::BannerControl::borderTop;
        if (val & BannerBox::Bottom)
            flags |= kcl::BannerControl::borderBottom;
            
        m_ctrl->setBorder(flags);
        return;
    }
}

// (METHOD) BannerBox.setBorderColor
// Description: Sets the color of the banner border.
//
// Syntax: function BannerBox.setBorderColor(color : Color)
//
// Remarks: Sets the |color| of the banner border.
//
// Param(color): The |color| to which to set the banner border.

void BannerBox::setBorderColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Color")
    {
        Color* c = (Color*)obj;
        
        wxColour c2(c->getMember(L"red")->getInteger(),
                    c->getMember(L"green")->getInteger(),
                    c->getMember(L"blue")->getInteger());
        m_ctrl->setBorderColor(c2);
        return;
    }
}




