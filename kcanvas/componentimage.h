/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-10
 *
 */


#ifndef H_KCANVAS_COMPONENTIMAGE_H
#define H_KCANVAS_COMPONENTIMAGE_H


namespace kcanvas
{


// Canvas Image Component Classes

class CompImage : public Component,
                  public ICompImage
{
    XCM_CLASS_NAME("kcanvas.CompImage")
    XCM_BEGIN_INTERFACE_MAP(CompImage)
        XCM_INTERFACE_ENTRY(ICompImage)
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompImage();
    virtual ~CompImage();

    static IComponentPtr create();
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    bool setProperty(const wxString& prop_name, const PropertyValue& value);
    bool setProperties(const Properties& properties);
    void render(const wxRect& rect = wxRect());

    // ICompImage interface
    void setImage(const wxImage& image);
    const wxImage& getImage();

private:

    bool initImage();
    void clearCache();

private:

    wxImage m_cache_image;
};


} // namespace kcanvas


#endif

