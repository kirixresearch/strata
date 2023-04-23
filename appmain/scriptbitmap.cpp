/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2007-01-03
 *
 */


#include "appmain.h"
#include "scripthost.h"
#include "scriptbitmap.h"


// -- Bitmap class implementation --

// (CLASS) Bitmap
// Category: Graphics
// Description: A class that represents a bitmap.
// Remarks: The Bitmap class encapsulates a bitmap.
//
// Property(Bitmap.FormatBMP): A type flag representing the Windows BMP file format.
// Property(Bitmap.FormatGIF): A type flag representing the GIF file format.
// Property(Bitmap.FormatXPM): A type flag representing the X Pixmap file format.
// Property(Bitmap.FormatPNG): A type flag representing the PNG file format.
// Property(Bitmap.FormatJPG): A type flag representing the JPG file format.

Bitmap::Bitmap()
{ 
}

Bitmap::~Bitmap()
{
}

wxBitmap& Bitmap::getWxBitmap()
{
    return m_bitmap;
}

void Bitmap::setWxBitmap(const wxBitmap& bmp)
{
    m_bitmap = bmp;
}

static wxBitmap xpmstrToBitmap(const std::wstring& str)
{
    if (str.length() == 0)
        return wxNullBitmap;
    
    std::string s = tostr(str);
    std::string output;
    std::vector<std::string> strs;
    std::vector<std::string>::iterator it;
    
    bool in_quote = false;
    const char* p = (const char*)s.c_str();
    while (*p)
    {
        if (*p != 0x0a && *p != 0x0d && *p != '\t' &&
            *p != '"' && *p != ',' && in_quote)
            output += *p;

        if (*p == '"')
            in_quote = !in_quote;

        if (*p == ',')
        {
            // -- store each line of the xpm --
            strs.push_back(output);
            in_quote = false;
            output = "";
        }

        p++;
    }

    // -- make sure we store the last line of the xpm --
    strs.push_back(output);
    
    int i = 0;
    char** c = new char*[strs.size()];
    for (it = strs.begin(); it != strs.end(); ++it)
    {
        c[i++] = (char*)((*it).c_str());
    }
    
    wxBitmap b = wxBitmap(c);
    delete[] c;

    return b;
}

// (CONSTRUCTOR) Bitmap.constructor
// Description: Creates a new bitmap.
//
// Syntax: Bitmap(width : Integer,
//                height : Integer)
// Syntax: Bitmap(xpm : String,
//                type : Integer)
//
// Remarks: Creates a new bitmap with the specified
//     |width| and |height| or based on the specified
//     |xpm| data and |type|, where |type| is one of
//     Bitmap.FormatBMP, Bitmap.FormatGIF, Bitmap.FormatXPM,
//     Bitmap.FormatPNG, or Bitmap.FormatJPG.
//
// Param(width): The |width| of the created bitmap.
// Param(height): The |height| of the created bitmap.
// Param(xpm): The |xpm| data to use in creating the bitmap object.
// Param(type): The |type| of data being used as input.

// TODO: there's a problem here in that the first parameter of the
//     second constructor is xpm text, yet the second parameter
//     allows multiple types of inputs. Below, in the constructor,
//     only the xpm type is supported.  We should probably expand
//     the first parameter to allow for binary data (Memory Object), 
//     so that the other format specifiers make sense.

void Bitmap::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() >= 2 && env->getParam(0)->isNumber())
    {
        m_bitmap = wxBitmap(env->getParam(0)->getInteger(),
                            env->getParam(1)->getInteger(),
                            -1);
    }
     else
    {
        std::wstring str = env->getParam(0)->getString();
        int type = env->getParam(1)->getInteger();
        
        if (type == Bitmap::FormatXPM || env->getParamCount() == 1)
        {
            m_bitmap = xpmstrToBitmap(str);
        }
    }
}


// (METHOD) Bitmap.loadFile
// Description: Loads a file into a bitmap object.
//
// Syntax: function Bitmap.loadFile(filename : String,
//                                  type : Integer) : Boolean
//
// Remarks: Loads a bitmap from a file, where the file is specified
//     |filename| and the optional file format is given by |type|.  The 
//     |type| is one of Bitmap.FormatBMP, Bitmap.FormatGIF, Bitmap.FormatXPM,
//     Bitmap.FormatPNG, or Bitmap.FormatJPG.  Returns true if the bitmap 
//     is successfully loaded from the file and false otherwise.
//
// Param(filename): The name of the file to load.
// Param(type): An optional parameter specifying the format
//     to use when loading the data from the file.
//
// Returns: Returns true if the bitmap is successfully loaded from
//     the file and false otherwise.

void Bitmap::loadFile(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    wxString filename = env->getParam(0)->getString();
    wxBitmapType bmp_type;
    int type = Bitmap::FormatDefault;
    
    if (env->getParamCount() > 1)
    {
        type = env->getParam(1)->getInteger();
    }
       
    switch (type)
    {
        case Bitmap::FormatDefault: bmp_type = wxBITMAP_TYPE_ANY; break;
        case Bitmap::FormatBMP: bmp_type = wxBITMAP_TYPE_BMP; break;
        case Bitmap::FormatXPM: bmp_type = wxBITMAP_TYPE_XPM; break;
        case Bitmap::FormatGIF: bmp_type = wxBITMAP_TYPE_GIF; break;
        case Bitmap::FormatPNG: bmp_type = wxBITMAP_TYPE_PNG; break;
        case Bitmap::FormatJPG: bmp_type = wxBITMAP_TYPE_JPEG; break;
        default:
            // unrecognized bitmap format
            retval->setBoolean(false);
            return;
    }

    if (!xf_get_file_exist(towstr(filename)))
    {
        // file does not exist
        retval->setBoolean(false);
        return;
    }
    

    retval->setBoolean(m_bitmap.LoadFile(filename, bmp_type));
}

// (METHOD) Bitmap.saveFile
// Description: Saves bitmap object to a file
//
// Syntax: function Bitmap.saveFile(filename : String,
//                                  type : Integer) : Boolean
//
// Remarks: Saves a bitmap to a file, where the file is specified
//     |filename| and the optional file format is given by |type|.  The
//     |type| is one of Bitmap.FormatBMP, Bitmap.FormatGIF, Bitmap.FormatXPM,
//     Bitmap.FormatPNG, or Bitmap.FormatJPG.  Returns true if the bitmap 
//     is successfully saved to the file and false otherwise.
//
// Param(filename): The name of the file to save.
// Param(type): An optional parameter specifying the format
//     to use when saving the data to the file.
//
// Returns: Returns true if the bitmap is successfully saved to
//     the file and false otherwise.

void Bitmap::saveFile(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    wxString filename = env->getParam(0)->getString();
    wxBitmapType bmp_type;
    int type = Bitmap::FormatDefault;
    
    if (env->getParamCount() > 1)
    {
        type = env->getParam(1)->getInteger();
    }
   
    switch (type)
    {
        case Bitmap::FormatDefault: bmp_type = wxBITMAP_TYPE_ANY; break;
        case Bitmap::FormatBMP: bmp_type = wxBITMAP_TYPE_BMP; break;
        case Bitmap::FormatXPM: bmp_type = wxBITMAP_TYPE_XPM; break;
        case Bitmap::FormatGIF: bmp_type = wxBITMAP_TYPE_GIF; break;
        case Bitmap::FormatPNG: bmp_type = wxBITMAP_TYPE_PNG; break;
        case Bitmap::FormatJPG: bmp_type = wxBITMAP_TYPE_JPEG; break;
        default:
            // unrecognized bitmap format
            retval->setBoolean(false);
            return;
    }
    
    retval->setBoolean(m_bitmap.SaveFile(filename, bmp_type));
}

// (METHOD) Bitmap.setHeight
// Description: Sets the height of the bitmap.
//
// Syntax: function Bitmap.setHeight(height : Integer)
//
// Remarks: Sets the |height| of the bitmap.
//
// Param(height): The |height| to which to set the bitmap.

void Bitmap::setHeight(kscript::ExprEnv* env, kscript::Value* retval)
{
    int height = env->getParam(0)->getInteger();
    m_bitmap.SetHeight(height);
}

// (METHOD) Bitmap.getHeight
// Description: Gets the height of the bitmap.
//
// Syntax: function Bitmap.getHeight() : Integer
//
// Remarks: Returns the height of the bitmap.
//
// Returns: Returns the height of the bitmap.

void Bitmap::getHeight(kscript::ExprEnv* env, kscript::Value* retval)
{
    int height = m_bitmap.GetHeight();
    retval->setInteger(height);
}

// (METHOD) Bitmap.setWidth
// Description: Sets the width of the bitmap.
//
// Syntax: function Bitmap.setWidth(width : Integer)
//
// Remarks: Sets the |width| of the bitmap.
//
// Param(width): The |width| to which to set the bitmap.

void Bitmap::setWidth(kscript::ExprEnv* env, kscript::Value* retval)
{
    int width = env->getParam(0)->getInteger();
    m_bitmap.SetWidth(width);
}

// (METHOD) Bitmap.getWidth
// Description: Gets the width of the bitmap.
//
// Syntax: function Bitmap.getWidth() : Integer
//
// Remarks: Returns the width of the bitmap.
//
// Returns: Returns the width of the bitmap.

void Bitmap::getWidth(kscript::ExprEnv* env, kscript::Value* retval)
{
    int width = m_bitmap.GetWidth();
    retval->setInteger(width);
}

// (METHOD) Bitmap.isOk
// Description: Indicates whether or not the Bitmap object represents
//     a valid bitmap.
//
// Syntax: function Bitmap.isOk() : Boolean
//
// Remarks: Returns true if the Bitmap object represents a valid bitmap.
//          Returns false if the Bitmap object does not represent a
//          valid bitmap.
//
// Returns: Returns true if the Bitmap object represents a valid bitmap,
//     and false otherwise.

// TODO: seems like this method could apply to lots of objects; should
//     we create a general "object" that bitmap derives from that
//     contains an isOk()?

void Bitmap::isOk(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(m_bitmap.IsOk());
}


