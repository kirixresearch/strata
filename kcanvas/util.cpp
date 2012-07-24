/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2007-03-20
 *
 */


#include "kcanvas_int.h"
#include "util.h"


namespace kcanvas
{


wxString getUniqueString()
{
    wxChar temp[33];
    int i;

    memset(temp, 0, 33 * sizeof(wxChar));
    
    for (i = 0; i < 8; i++)
    {
        temp[i] = wxT('a') + (rand() % 26);
    }

    unsigned int t = time(NULL);
    int div = 308915776;    // 26^6;
    for (i = 8; i < 15; i++)
    {
        temp[i] = wxT('a') + (t/div);
        t -= ((t/div)*div);
        div /= 26;
    }

    return temp;
}

wxColor towxcolor(const Color& color)
{
    if (color == kcanvas::COLOR_NULL)
        return wxNullColour;

    return wxColor(color.Red(), color.Green(), color.Blue());
}

Color tokccolor(const wxColor& color)
{
    if (color == wxNullColour)
        return Color(kcanvas::COLOR_NULL);

    return Color(color.Red(), color.Green(), color.Blue());
}

double blendColor(double fg, double bg, double alpha)
{
    double result = bg + (alpha * (fg - bg));
    if (result < 0.0)
        result = 0.0;
    if (result > 255)
        result = 255;
    return result;
}

Color stepColor(const Color& fc, const Color& bc, int ialpha)
{
    // clamp the alpha
    ialpha = wxMin(ialpha, 100);
    ialpha = wxMax(ialpha, 0);

    // if alpha is at 100, we're done
    if (ialpha == 100)
        return fc;

    // if either of the inputs is a null color, use a white 
    // color in its place
    const Color& fc_adj = (fc == COLOR_NULL ? COLOR_WHITE : fc);
    const Color& bc_adj = (bc == COLOR_NULL ? COLOR_WHITE : bc);
        
    double r = fc_adj.Red(), g = fc_adj.Green(), b = fc_adj.Blue();
    
    // ialpha is 0..100 where 0 is completely the background
    // color and 100 is completely the original color
    double alpha = 1.0 + ((double)(ialpha - 100.0))/100.0;
    
    r = blendColor(r, bc_adj.Red(), alpha);
    g = blendColor(g, bc_adj.Green(), alpha);
    b = blendColor(b, bc_adj.Blue(), alpha);
    
    return Color((int)r, (int)g, (int)b);
}

wxBitmap setAlpha(const wxBitmap& bitmap, int ialpha)
{
    // convert the bitmap to an image and prepare the
    // alpha channel
    wxImage image = bitmap.ConvertToImage();
    image.SetAlpha();
    
    // if we're not able to set the alpha channel, return
    // the bitmap we started with
    if (!image.HasAlpha())
        return bitmap;
    
    // set the alpha channel on the image
    int w, h;
    int width = image.GetWidth();
    int height = image.GetHeight();
    
    for (w = 0; w < width; ++w)
    {
        for (h = 0; h < height; ++h)
        {
            image.SetAlpha(w, h, ialpha);
        }
    }
    
    // convert the image back to a bitmap and return the bitmap
    return wxBitmap(image);
}

void getBitmapContentArea(const wxBitmap& bitmap,
                          const Color& invalid_color,
                          int* x, int* y, int* w, int* h)
{
    // this function finds the non-black range of a bitmap
    
    int x1, y1, x2, y2;
    
    wxImage img = bitmap.ConvertToImage();
    int width = img.GetWidth();
    int height = img.GetHeight();
    
    bool entered_image = false;
    for (int i = 0; i < width; ++i)
    {
        for (int j = 0; j < height; ++j)
        {           
            if (img.GetRed(i, j) != invalid_color.Red() ||
                img.GetBlue(i, j) != invalid_color.Blue() ||
                img.GetGreen(i, j) != invalid_color.Green())
            {
                // if we're just entering the image, record
                // the starting pixel of the image
                if (!entered_image)
                {
                    x1 = i;
                    y1 = j;
                }

                // record the pixel of the image we're on so
                // we can find the last non-black pixel in the image
                x2 = i;
                y2 = j;
                
                // set the flag that indicates we've entered the image
                entered_image = true;
            }
        }
    }
    
    *x = x1;
    *y = y1;
    *w = x2 - x1 + 1;
    *h = y2 - y1 + 1;
}

bool saveProperties(IStoreValuePtr store, const Properties& properties)
{
    // get the properties
    std::vector<Property> props;
    properties.list(props);

    wxString property_list;
    std::vector<Property>::iterator it, it_end;
    it_end = props.end();
    
    // iterate through the properties, creating and saving a list of the 
    // properties we're saving, so we know what to ask for on load      
    for (it = props.begin(); it != it_end; ++it)
    {
        wxString prop_name = it->getName();
        int prop_type = it->getType();

        // if we have an invalid type, move on
        if (prop_type == proptypeInvalid)
            continue;

        property_list += prop_name;
        property_list += wxT(":");

        if (prop_type == proptypeString)
            property_list += wxT("type.string");

        if (prop_type == proptypeColor)
            property_list += wxT("type.color");

        if (prop_type == proptypeInteger)
            property_list += wxT("type.integer");

        if (prop_type == proptypeBoolean)
            property_list += wxT("type.boolean");
        
        property_list += wxT(";");
    }

    property_list.RemoveLast(); // last semi-colon

    // save the property names and types
    IStoreValuePtr node_1;
    node_1 = store->createChild(wxT("property.types"));
    node_1->setString(property_list);

    // iterate through the property a second time, saving the values
    // of the properties
    IStoreValuePtr node_2, node_2_1;
    node_2 = store->createChild(wxT("property.values"));
    for (it = props.begin(); it != it_end; ++it)
    {
        // get the property name and type
        wxString prop_name = it->getName();
        int prop_type = it->getType();

        // if we have an invalid type, move on
        if (prop_type == proptypeInvalid)
            continue;

        node_2_1 = node_2->createChild(prop_name);

        // save a string property
        if (prop_type == proptypeString)
            node_2_1->setString(it->getString());

        // save a color property
        if (prop_type == proptypeColor)
            node_2_1->setColor(it->getColor());

        // save an integer property
        if (prop_type == proptypeInteger)
            node_2_1->setInteger(it->getInteger());

        // save a boolean property
        if (prop_type == proptypeBoolean)
            node_2_1->setBoolean(it->getBoolean());
    }

    return true;
}

bool loadProperties(IStoreValuePtr store, Properties& properties)
{
    // note: this function takes the list of input properties and
    // adds on (or replaces if they exist) the properties contained
    // in the store; it's the responsibility of the calling function
    // to clear the input properties if an exact copy of the stored
    // properties is needed; the reason for this is that components
    // may have properties that get set after it has been serialized,
    // in which case the saved properties only represent a subset of
    // the properties; if this function clears out the properties,
    // any other properties that haven't been serialized will be 
    // cleared out, which isn't always desirable; if we leave it up 
    // to the calling function, we have control over which option we 
    // want when we use this function

    // get the list of properties we need to unpack
    IStoreValuePtr node_1;
    node_1 = store->getChild(wxT("property.types"), false);
    if (node_1.isNull())
        return false;
    
    // unpack the properties and put them on list
    std::vector<Property> props;
    
    wxString property_list = node_1->getString();
    wxStringTokenizer tokenizer(property_list, wxT(";"));
    while (tokenizer.HasMoreTokens())
    {
        // get the property name
        wxString prop_token = tokenizer.GetNextToken();
        wxString prop_name = prop_token.BeforeFirst(wxT(':'));
        wxString prop_type_string = prop_token.AfterFirst(wxT(':'));
        prop_name.Trim(true);
        prop_name.Trim(false);
        prop_type_string.Trim(true);
        prop_type_string.Trim(false);

        // get the property type
        int prop_type = proptypeInvalid;

        if (prop_type_string == wxT("type.string"))
            prop_type = proptypeString;
            
        if (prop_type_string == wxT("type.color"))
            prop_type = proptypeColor;

        if (prop_type_string == wxT("type.integer"))
            prop_type = proptypeInteger;

        if (prop_type_string == wxT("type.boolean"))
            prop_type = proptypeBoolean;
 
        // if we have an invalid property type, move on
        if (prop_type == proptypeInvalid)
            continue;
       
        // the only reason we push properties back is so we can 
        // load the appropriate property a little later in the 
        // function
        props.push_back(Property(prop_name, prop_type));
    }

    // iterate through the properties from the list we unpacked
    // and save them to the properties we're restoring
    IStoreValuePtr node_2, node_2_1;
    node_2 = store->getChild(wxT("property.values"), false);
    if (node_2.isNull())
        return false;

    std::vector<Property>::iterator it, it_end;
    it_end = props.end();

    for (it = props.begin(); it != it_end; ++it)
    {
        // get the property name and type
        wxString prop_name;
        int prop_type;

        prop_name = it->getName();
        prop_type = it->getType();

        // if we have an invalid type, move on
        if (prop_type == proptypeInvalid)
            continue;

        // if we can't get the given property, move on
        node_2_1 = node_2->getChild(prop_name, false);
        if (node_2_1.isNull())
            continue;

        // note: PROP_TEXT_VALUEonly exists for <= 4.3 XML report format; 
        // PROP_CONTENT_VALUE is now used to store values; convert old
        // format to new
        if (prop_name == PROP_TEXT_VALUE)
            prop_name = PROP_CONTENT_VALUE;

        // load a string property
        if (prop_type == proptypeString)
            properties.add(prop_name, node_2_1->getString());

        // load a color property
        if (prop_type == proptypeColor)
            properties.add(prop_name, node_2_1->getColor());

        // load an integer property
        if (prop_type == proptypeInteger)
            properties.add(prop_name, node_2_1->getInteger());

        // load a boolean property
        if (prop_type == proptypeBoolean)
            properties.add(prop_name, node_2_1->getBoolean());
    }

    return true;
}

void setFontFromProperties(const PropertyValue& facename,
                           const PropertyValue& fontsize,
                           const PropertyValue& fontstyle,
                           const PropertyValue& fontweight,
                           const PropertyValue& fontunderscore,
                           Font& font)
{
    font.setFaceName(facename.getString());
    font.setSize(fontsize.getInteger());
    font.setStyle(fontstyle.getString());
    font.setWeight(fontweight.getString());
    font.setUnderscore(fontunderscore.getString());
}
                           
void setPropertiesFromFont(const Font& font,
                           PropertyValue& fontfacename,
                           PropertyValue& fontsize,
                           PropertyValue& fontstyle,
                           PropertyValue& fontweight,
                           PropertyValue& fontunderscore)
{
    fontfacename.setString(font.getFaceName());
    fontsize.setInteger(font.getSize());
    fontstyle.setString(font.getStyle());
    fontweight.setString(font.getWeight());
    fontunderscore.setString(font.getUnderscore());
}

IComponentPtr CopyComponentTree(IComponentPtr component)
{
    if (component.isNull())
        return xcm::null;

    // clone the parent component, and clear it of any
    // child references that were copied, since we want
    // the cloned component to have clones of all the
    // children rather than references to the original
    // children
    IComponentPtr clone = component->clone();
    clone->removeAll();

    // clone the child components and add them to the
    // newly cloned component
    std::vector<IComponentPtr> children;
    component->getComponents(children);
    
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = children.end();
    
    for (it = children.begin(); it != it_end; ++it)
    {
        IComponentPtr child_clone = CopyComponentTree(*it);
        clone->add(child_clone);
    }
    
    return clone;
}

bool hasChildComponent(IComponentPtr parent, IComponentPtr child)
{
    // note: this function returns true if the child component
    // is an immediate child of the parent component, and
    // false otherwise

    if (parent.isNull() || child.isNull())
        return false;

    // get the child components
    std::vector<IComponentPtr> children;
    parent->getComponents(children);

    // search for the child component; if we find it, return true
    std::vector<IComponentPtr>::iterator it;
    it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
        return true;

    // if we didn't find the child component, return false
    return false;
}

void getAllSubComponents(std::vector<Component*>& components,
                         Component* initial)
{
    // note: this function gets all the subcomponent of an
    // initial component; differs from interface version in
    // that the inputs use regular pointers, which is useful
    // where reference counting gets us into trouble, such
    // as in the component destructor

    // if we don't have an initial component, we're done
    if (!initial)
        return;

    // get the child components of the initial component
    std::vector<IComponentPtr> c;
    initial->getComponents(c);
    
    // iterate through the child components
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = c.end();
    
    for (it = c.begin(); it != it_end; ++it)
    {
        // add each child component onto the list of components
        // and in turn, get each of their children
        Component* child_ptr = static_cast<Component*>(it->p);
        components.push_back(child_ptr);
        getAllSubComponents(components, child_ptr);
    }    
}

void getAllSubComponents(std::vector<IComponentPtr>& components,
                         IComponentPtr initial)
{
    // note: this function gets all the subcomponents of an 
    // initial component
    
    // if we don't have an initial component, we're done
    if (initial.isNull())
        return;

    // get the child components of the initial component
    std::vector<IComponentPtr> c;
    initial->getComponents(c);
    
    // iterate through the child components
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = c.end();
    
    for (it = c.begin(); it != it_end; ++it)
    {
        // add each child component onto the list of components
        // and in turn, get each of their children
        components.push_back(*it);
        getAllSubComponents(components, *it);
    }
}

void getUniqueSubComponents(std::vector<IComponentPtr>& components,
                            IComponentPtr initial)
{
    // note: this function gets a unique list of the subcomponents of
    // an initial component; it does not include the initial component
    // in the list

    // if we don't have an initial component, we're done
    if (initial.isNull())
        return;

    // map of unique components
    std::map<wxString, IComponentPtr> unique;

    // clear the components vector and get all sub components from 
    // the initial component
    components.clear();
    getAllSubComponents(components, initial);
    
    // iterate through the sub components and save them to
    // the map of unique components
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = components.end();
    
    for (it = components.begin(); it != it_end; ++it)
    {
        unique[(*it)->getId()] = *it;
    }
    
    // clear the components vector again
    components.clear();
    
    // iterate through the map and add the components to the
    // the output list of components
    std::map<wxString, IComponentPtr>::iterator itu, itu_end;
    itu_end = unique.end();
    
    for (itu = unique.begin(); itu != itu_end; ++itu)
    {
        components.push_back(itu->second);
    }
}

bool getBoundingTextColumnIdx(const CellRange& range,
                              std::vector<CellProperties>& props,
                              std::vector<CellProperties>::reverse_iterator it,
                              int* idx, bool max)
{
    // note: if max is true, this function finds the maximum column 
    // index with a non-empty text value in the given range; if max 
    // is false, this function finds the minimum column index with
    // a non-empty text value in the given range; returns true if a 
    // column with a non-empty text value is found in the range, and 
    // false otherwise; note: first, this function closely parallels 
    // getTextColumns; the reason it's broken out is because it only 
    // returns a bounding column, which is much more efficient to return 
    // in some cases than a vector of all the indexes of columns with 
    // text; second, the function assumes that range markers of -1 have 
    // been replaced with the first or last row/col index of the table;
    // returns true if columns with text are added to the list, and 
    // false otherwise

    // TODO: handle locked cells
    
    // if the range is empty, we're done
    if (range.isEmpty())
        return false;
    
    // if we don't have any properties, we're done
    if (props.empty())
        return false;

    // if we're past the last element, we're done
    if (it == props.rend())
        return false;

    // get the range of cells of the current text property
    CellRange current_range = it->range();
    current_range.intersect(range);
    
    // if the range doesn't intersect the range in question, move 
    // on to the next one
    if (current_range.isEmpty())
        return getBoundingTextColumnIdx(range, props, ++it, idx, max);

    // if the mime type isn't text, we're done
    PropertyValue mime;
    if (it->getProperty(PROP_CONTENT_MIMETYPE, mime) &&
        mime.getString() != wxT("plain/text"))
    {
        return false;
    }

    // if the range does intersect the range in question,
    // find out if the range has a text property
    PropertyValue value;
    it->getProperty(PROP_CONTENT_VALUE, value);
    bool empty = value.getString().IsEmpty();
    ++it;

    // if the text property isn't empty, the bounding index
    // is the first/last (max = false/true) row in the current
    // range, unless we find another non-empty text property 
    // in the range to the right/left (max = false/true)
    // of the current range
    if (!empty)
    {
        // if we're already on the cell with the bounding
        // column idx in the range of available cells,
        // we're done; return true
        *idx = max ? current_range.col2() : current_range.col1();
        if ((max && *idx == range.col2()) || 
            (!max && *idx == range.col1()))
        {
            return true;
        }

        int c1, c2;
        c1 = max ? *idx + 1 : range.col1();
        c2 = max ? range.col2() : *idx - 1;
            
        CellRange new_range(range.row1(), c1, range.row2(), c2);
        getBoundingTextColumnIdx(new_range, props, it, idx, max);
        return true;
    }

    // if the text property is empty, search the two
    // leftover pieces, starting with the one with the
    // greater/lesser (max = true/false) column indexes
    int c1, c2;

    c1 = max ? current_range.col2() + 1 : range.col1();
    c2 = max ? range.col2() : current_range.col1() - 1;

    if (c1 <= c2)
    {
        CellRange range1(range.row1(), c1, range.row2(), c2);
        if (getBoundingTextColumnIdx(range1, props, it, idx, max))
            return true;
    }
    
    c1 = max ? range.col1() : current_range.col2() + 1;
    c2 = max ? current_range.col1() - 1 : range.col2();
    
    if (c1 <= c2)
    {
        CellRange range2(range.row1(), c1, range.row2(), c2);
        if (getBoundingTextColumnIdx(range2, props, it, idx, max))
            return true;
    }

    // we weren't able to find any columns with text 
    // properties; return false
    return false;
}

bool getTextColumns(const CellRange& range,
                    std::vector<CellProperties>& props,
                    std::vector<CellProperties>::reverse_iterator it,
                    std::vector<int>& columns)
{
    // note: helper function that returns a vector of column indexes 
    // that have non-empty text properties for the given row in a table;
    // function assumes that the input range is a single row and that
    // range markers have been accounted for (i.e., that range.row1()
    // is equal to range.row2() and that range markers of -1 have been
    // replaced with the first or last row/col index of the table;
    // returns true if columns with text are added to the list, and 
    // false otherwise

    // if the range is empty, we're done
    if (range.isEmpty())
        return false;
    
    // if we don't have any properties, we're done
    if (props.empty())
        return false;

    // if we're past the last element, we're done
    if (it == props.rend())
        return false;

    // get the range of cells of the current text property 
    CellRange current_range = it->range();
    current_range.intersect(range);
    
    // if the range doesn't intersect the range in question,
    // move on to the next one
    if (current_range.isEmpty())
        return getTextColumns(range, props, ++it, columns);

    // if the mime type isn't text, we're done
    PropertyValue mime;
    if (it->getProperty(PROP_CONTENT_MIMETYPE, mime) &&
        mime.getString() != wxT("plain/text"))
    {
        return false;
    }

    // if the range does intersect the range in question,
    // find out if the range has a text property
    PropertyValue value;
    it->getProperty(PROP_CONTENT_VALUE, value);
    bool empty = value.getString().IsEmpty();
    ++it;

    // result flag; set to true if there are any text columns
    // are added to the list in the following
    bool result = false;

    // if the text property isn't empty, save the indexes
    // of the columns in the range, then get the columns
    // in the ranges to the right and left of the current
    // range
    if (!empty)
    {
        result = true;
    
        int col;
        for (col = current_range.col1(); col <= current_range.col2(); ++col)
        {
            columns.push_back(col);
        }
    }

    int c1a = range.col1();
    int c2a = current_range.col1() - 1;
    
    if (c1a <= c2a)
    {
        CellRange range1(range.row1(), c1a, range.row2(), c2a);
        result |= getTextColumns(range1, props, it, columns);
    }
    
    int c1b = current_range.col2() + 1;
    int c2b = range.col2();

    if (c1b <= c2b)
    {
        CellRange range2(range.row1(), c1b, range.row2(), c2b);
        result |= getTextColumns(range2, props, it, columns);
    }

    // return the result flag
    return result;
}

bool getTextRows(const CellRange& range,
                 std::vector<CellProperties>& props,
                 std::vector<CellProperties>::reverse_iterator it,
                 std::vector<int>& rows)
{
    // note: helper function that returns a vector of row indexes 
    // that have non-empty text properties for the given column in a 
    // table; function assumes that the input range is a single column 
    // and that range markers have been accounted for (i.e., that 
    // range.col1() is equal to range.col2() and that range markers 
    // of -1 have been replaced with the first or last row/col index 
    // of the table; returns true if rows with text are added to the 
    // list, and false otherwise

    // if the range is empty, we're done
    if (range.isEmpty())
        return false;
    
    // if we don't have any properties, we're done
    if (props.empty())
        return false;

    // if we're past the last element, we're done
    if (it == props.rend())
        return false;

    // get the range of cells of the current text property 
    CellRange current_range = it->range();
    current_range.intersect(range);

    // if the range doesn't intersect the range in question,
    // move on to the next one
    if (current_range.isEmpty())
        return getTextRows(range, props, ++it, rows);

    // if the mime type isn't text, we're done
    PropertyValue mime;
    if (it->getProperty(PROP_CONTENT_MIMETYPE, mime) &&
        mime.getString() != wxT("plain/text"))
    {
        return false;
    }

    // if the range does intersect the range in question,
    // find out if the range has a text property
    PropertyValue value;
    it->getProperty(PROP_CONTENT_VALUE, value);
    bool empty = value.getString().IsEmpty();
    ++it;

    // result flag; set to true if there are any text rows
    // are added to the list in the following
    bool result = false;

    // if the text property isn't empty, save the indexes
    // of the rows in the range, then get the rows in the
    // ranges to the top and bottom of the current range
    if (!empty)
    {
        result = true;
    
        int row;
        for (row = current_range.row1(); row <= current_range.row2(); ++row)
        {
            rows.push_back(row);
        }
    }

    int r1a = range.row1();
    int r2a = current_range.row1() - 1;
    
    if (r1a <= r2a)
    {
        CellRange range1(r1a, range.col1(), r2a, range.col2());
        result |= getTextRows(range1, props, it, rows);
    }
    
    int r1b = current_range.row2() + 1;
    int r2b = range.row2();

    if (r1b <= r2b)
    {
        CellRange range2(r1b, range.col1(), r2b, range.col2());
        result |= getTextRows(range2, props, it, rows);
    }

    // return the result flag
    return result;
}

void getBoundingRange(const std::vector<CellRange>& cells,
                      const CellRange& input_range,
                      CellRange& bounding_range)
{
    // note: this function takes a list of cell ranges and finds the 
    // smallest range of cells that completely contains the input range 
    // and the cells in a subset of ranges of the input list; it then
    // returns this range as the bounding range; examples: 1) if the
    // list of cells is empty, the bounding range is the same as the
    // input range; 2) if the list of cells isn't empty, the bounding 
    // range may be larger than the initial range, but will be at least 
    // as large as the input range; this might happen if the list of 
    // cells contain the ranges of merged cells, and the input range
    // intersects at least one of the merged cells; then when the input
    // range is expanded to accomodate the range of this merged cell, 
    // the expanded input range may then interesect the range of another 
    // merged cell, and so on

    // set the bounding_range to the input range
    bounding_range = input_range;

    // if we don't have any cells in the list, the bounding range 
    // is the same as the input range
    if (cells.empty())
        return;

    // iterate through the list of cells
    std::vector<CellRange>::const_iterator it, it_end;
    it_end = cells.end();

    for (it = cells.begin(); it != it_end; ++it)
    {
        // if any of the ranges of the cells cells intersect the 
        // input range, see if the bounding range is different from 
        // the input range; if it is, find the new bounding range
        if (input_range.intersects(*it))
        {
            CellRange br = input_range;
            br.bound(*it);
            if (br != input_range)
            {
                getBoundingRange(cells, br, bounding_range);
                return;
            }
        }
    }
}

void getBoundingRange(const std::vector<CellProperties>& cells,
                      const CellRange& input_range,
                      CellRange& bounding_range)
{
    // note: equivalent to other version of getBoundingRange()
    // above, except this one works with the ranges in the
    // list of cell properties instead of a list of cell ranges

    // set the bounding_range to the input range
    bounding_range = input_range;

    // if we don't have any cells in the list, the bounding range 
    // is the same as the input range
    if (cells.empty())
        return;

    // iterate through the list of cells
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = cells.end();

    for (it = cells.begin(); it != it_end; ++it)
    {
        // if any of the ranges of the cells cells intersect the 
        // input range, see if the bounding range is different from 
        // the input range; if it is, find the new bounding range
        if (input_range.intersects(it->range()))
        {
            CellRange br = input_range;
            br.bound(it->range());
            if (br != input_range)
            {
                getBoundingRange(cells, br, bounding_range);
                return;
            }
        }
    }
}

void insertRowIntoProperties(std::vector<CellProperties>& properties, int idx, int count)
{
    // if the count is less than 1, we're done
    if (count < 1)
        return;

    // iterate through each of the properties and insert the row
    std::vector<CellProperties>::iterator it, it_end;
    it_end = properties.end();
    
    for (it = properties.begin(); it != it_end; ++it)
    {
        it->m_range.insertRow(idx, count);
    }
}

void removeRowFromProperties(std::vector<CellProperties>& properties, int idx, int count)
{
    // if the count is less than 1, we're done
    if (count < 1)
        return;

    // vector to store the cell properties we want to keep
    std::vector<CellProperties> cell_properties;
    cell_properties.reserve(properties.size());

    // iterate through the properties, remove the rows
    // in question and save any resulting ranges that
    // are still not empty after removing the rows
    std::vector<CellProperties>::iterator it, it_end;
    it_end = properties.end();
    
    for (it = properties.begin(); it != it_end; ++it)
    {
        // remove the rows from each set of properties
        it->m_range.removeRow(idx, count);
        
        // if the resulting range is not empty, save the 
        // properties
        if (!it->m_range.isEmpty())
            cell_properties.push_back(*it);
    }

    // replace the cell properties with the saved cell 
    // properties
    properties = cell_properties;
}

void insertColumnIntoProperties(std::vector<CellProperties>& properties, int idx, int count)
{
    // if the count is less than 1, we're done
    if (count < 1)
        return;

    // iterate through each of the properties and insert the column
    std::vector<CellProperties>::iterator it, it_end;
    it_end = properties.end();
    
    for (it = properties.begin(); it != it_end; ++it)
    {
        it->m_range.insertColumn(idx, count);
    }
}

void removeColumnFromProperties(std::vector<CellProperties>& properties, int idx, int count)
{
    // if the count is less than 1, we're done
    if (count < 1)
        return;

    // vector to store the cell properties we want to keep
    std::vector<CellProperties> cell_properties;
    cell_properties.reserve(properties.size());

    // iterate through the properties, remove the columns
    // in question and save any resulting ranges that are
    // still not empty after removing the columns
    std::vector<CellProperties>::iterator it, it_end;
    it_end = properties.end();
    
    for (it = properties.begin(); it != it_end; ++it)
    {
        // remove the columns from each set of properties
        it->m_range.removeColumn(idx, count);
        
        // if the resulting range is not empty, save the 
        // properties
        if (!it->m_range.isEmpty())
            cell_properties.push_back(*it);
    }

    // replace the cell properties with the saved cell 
    // properties
    properties = cell_properties;
}


}; // namespace kcanvas

