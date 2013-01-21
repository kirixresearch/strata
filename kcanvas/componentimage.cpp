/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-10
 *
 */


#include "kcanvas_int.h"
#include "componentimage.h"


namespace kcanvas
{


/* XPM */
static const char* xpm_brokenimage[] = {
"16 16 8 1",
"  c #330000",
"! c #CC6600",
"# c #CC9900",
"$ c #FF9900",
"% c #FFCC00",
"& c #FFCC33",
"' c #FFFF33",
"( c #FFFFFF",
"((((((((((((((((",
"(((((($$$$((((((",
"(((((($&&$((((((",
"((((($$''$$(((((",
"((((($&''&$(((((",
"(((($$'  '$$((((",
"((($$&'  '&$$(((",
"((($%''  ''%$(((",
"((#$&&&  &&&$#((",
"((#%&&&  &&&%#((",
"(#$&%%%%%%%%&$#(",
"(#%%%%%  %%%%%#(",
"#$%%%%%  %%%%%$#",
"#%&&&&&&&&&&&&%#",
"!!!!!!!!!!!!!!!!",
"(((((((((((((((("
};


inline wxString towx(const char* s)
{
#ifdef _UNICODE
    return wxString::From8BitData(s);
#else
    return s;
#endif
}


CompImage::CompImage()
{
    // properties
    initProperties(m_properties);
}

CompImage::~CompImage()
{
}

IComponentPtr CompImage::create()
{
    return static_cast<IComponent*>(new CompImage);
}

void CompImage::initProperties(Properties& properties)
{
    Component::initProperties(properties);
    properties.add(PROP_COMP_TYPE, COMP_TYPE_IMAGE);
    properties.add(PROP_TEXT_HALIGN, ALIGNMENT_CENTER);
    properties.add(PROP_TEXT_VALIGN, ALIGNMENT_MIDDLE);
    properties.add(PROP_CONTENT_MIMETYPE, wxT(""));
    properties.add(PROP_CONTENT_ENCODING, wxT(""));
    properties.add(PROP_CONTENT_VALUE, wxString(wxT("")));
    
    // TODO: we're using text properties for alignment; these should be 
    // generalized; in addition, we should add a scale parameter to determine 
    // whether the image should be stretched horizontally or vertically
}

IComponentPtr CompImage::clone()
{
    CompImage* c = new CompImage;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompImage::copy(IComponentPtr component)
{
    Component::copy(component);

    // if the input component isn't an image component, this 
    // is all we can do; we're done
    ICompImagePtr image = component;
    if (image.isNull())
        return;

    // clear the cache
    clearCache();
}

bool CompImage::setProperty(const wxString& prop_name, const PropertyValue& value)
{
    // try to set a component property; if the 
    // property is unchanged, return false
    if (!Component::setProperty(prop_name, value))
        return false;

    // if we're changing the content type, content encoding,
    // or content, clear the image and bitmap
    if (prop_name == PROP_CONTENT_MIMETYPE ||
        prop_name == PROP_CONTENT_ENCODING ||
        prop_name == PROP_CONTENT_VALUE)
    {
        clearCache();
    }

    // property changed, return true
    return true;
}

bool CompImage::setProperties(const Properties& properties)
{
    // try to set the component properties; if the 
    // properties are unchanged, return false
    if (!Component::setProperties(properties))
        return false;

    // if we're changing the content type, content encoding,
    // or content, clear the image and bitmap
    if (properties.has(PROP_CONTENT_MIMETYPE) ||
        properties.has(PROP_CONTENT_ENCODING) ||
        properties.has(PROP_CONTENT_VALUE))
    {
        clearCache();
    }

    // properties changed, return true
    return true;
}

void CompImage::render(const wxRect& rect)
{
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // get the component width and height
    int width = getWidth();
    int height = getHeight();    
    
    // if the width or height or is zero, we're done
    if (width <= 0 || height <= 0)
        return;

    // draw background rectangle
    canvas->setPen(m_properties);
    canvas->setBrush(m_properties);
    canvas->drawRectangle(0, 0, width, height);

    // initialize the image; if we can't initialize
    // it, set the image to a broken image icon
    if (!initImage())
    {
        // TODO: set the broken image icon so it doesn't stretch
        // to fill the area where it is being drawn
    
        wxBitmap bitmap(xpm_brokenimage);
        m_cache_image = bitmap.ConvertToImage();
    }

    // if the image doesn't exist, we're done
    if (!m_cache_image.Ok())
        return;

    // don't distort the image when trying to fit in the available
    // space; scale it so it overflows one of the dimensions and
    // completely fills the other
    int image_width = m_cache_image.GetWidth();
    int image_height = m_cache_image.GetHeight();
    
    if (image_width <= 0 || image_height <= 0)
        return;

    double perc_width, perc_height;
    perc_width = (double)width/(double)canvas->dtom_x(image_width);
    perc_height = (double)height/(double)canvas->dtom_y(image_height);

    width = canvas->dtom_x(image_width)*wxMax(perc_width, perc_height);
    height = canvas->dtom_y(image_height)*wxMax(perc_width, perc_height);

    // adjust the image based on alignment
    PropertyValue halign, valign;
    int h_offset = 0;
    int v_offset = 0;    

    // if a center or right horizontal alignment is specified, 
    // adjust the horizontal position accordingly
    if (getProperty(PROP_TEXT_HALIGN, halign))
    {
        if (halign.getString() == ALIGNMENT_CENTER)
            h_offset += (getWidth() - width)/2;

        if (halign.getString() == ALIGNMENT_RIGHT)
            h_offset += (getWidth() - width);
    }
    
    // if a middle or bottom vertical alignment is specified, 
    // adjust the vertical position accordingly
    if (getProperty(PROP_TEXT_VALIGN, valign))
    {
        if (valign.getString() == ALIGNMENT_MIDDLE)
            v_offset += (getHeight() - height)/2;

        if (valign.getString() == ALIGNMENT_BOTTOM)
            v_offset += (getHeight() - height);
    }

    // draw the image
    canvas->drawImage(m_cache_image, h_offset, v_offset, width, height);
}

void CompImage::setImage(const wxImage& image)
{
    // if the image is null, reset the content
    if (!image.IsOk())
    {
        setProperty(PROP_CONTENT_MIMETYPE, wxT(""));
        setProperty(PROP_CONTENT_ENCODING, wxT(""));
        setProperty(PROP_CONTENT_VALUE, wxT(""));
        return;
    }

    // save the image to a string
    wxString mime_type = wxT("image/png");
    wxString encoding = wxT("base64");

    setProperty(PROP_CONTENT_MIMETYPE, mime_type);
    setProperty(PROP_CONTENT_ENCODING, encoding);

    wxMemoryOutputStream stream(NULL);
    if (!image.SaveFile(stream, wxT("image/png")))
        return;

    size_t raw_size = stream.GetSize();
    char* raw_buf = new char[raw_size];
    char* base64_buf = new char[(raw_size+2)*4];
    
    stream.CopyTo(raw_buf, raw_size);

    kl::base64_encodestate state;
    kl::base64_init_encodestate(&state);
    int l1 = kl::base64_encode_block(raw_buf, raw_size, base64_buf, &state);
    int l2 = kl::base64_encode_blockend(base64_buf+l1, &state);
    int base64_len = l1+l2;
    
    if (base64_len > 0 && base64_buf[base64_len-1] == '\n')
        base64_len--;
    base64_buf[base64_len] = 0;

    // set the property
    setProperty(PROP_CONTENT_VALUE, towx(base64_buf));

    delete[] raw_buf;
    delete[] base64_buf;
}

const wxImage& CompImage::getImage()
{
    initImage();
    return m_cache_image;
}

bool CompImage::initImage()
{
    // note: this function sets the image from the properties

    // if the image exists, we're done
    if (m_cache_image.IsOk())
        return true;

    // load the image from the content; for now, only allow base64
    // encoding and require the mime type to be set
    PropertyValue prop_mime;
    if (!getProperty(PROP_CONTENT_MIMETYPE, prop_mime))
    {
        clearCache();
        return false;
    }
        
    PropertyValue prop_encoding;
    if (!getProperty(PROP_CONTENT_ENCODING, prop_encoding) ||
        prop_encoding.getString() != wxT("base64"))
    {
        clearCache();
        return false;
    }

    PropertyValue prop_content;
    if (!getProperty(PROP_CONTENT_VALUE, prop_content))
    {
        clearCache();
        return false;
    }

    // decode the base64-encoded data and load the data
    std::string content = kl::tostring(prop_content.getString());
    char* buf = new char[content.length()+1];

    kl::base64_decodestate state;
    kl::base64_init_decodestate(&state);
    int len = kl::base64_decode_block(content.c_str(), content.length(), buf, &state);

    buf[len] = 0;

    wxMemoryInputStream stream(buf, len);
    m_cache_image.LoadFile(stream, prop_mime.getString());

    delete[] buf;

    if (!m_cache_image.IsOk())
    {
        // TODO: add a default image indicating image couldn't loaded
        return false;
    }

    return true;
}

void CompImage::clearCache()
{
    m_cache_image = wxNullImage;
}


}; // namespace kcanvas

