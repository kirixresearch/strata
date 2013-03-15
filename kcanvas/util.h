/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2007-03-20
 *
 */


#ifndef __KCANVAS_UTIL_H
#define __KCANVAS_UTIL_H


namespace kcanvas
{


// TODO: determine which of these should be inline

// function for creating a unique string
wxString getUniqueString();

// conversion functions
wxColor towxcolor(const Color& color);
Color tokccolor(const wxColor& color);

// functions for working with colors
double blendColor(double fg, double bg, double alpha);
Color stepColor(const Color& fc, const Color& bc, int ialpha);

// functions for working with bitmaps
wxBitmap setAlpha(const wxBitmap& bitmap, int ialpha);
void getBitmapContentArea(const wxBitmap& bitmap,
                          const Color& invalid_color,
                          int* x, int* y, int* w, int* h);

// functions for converting between fonts and properties
void setFontFromProperties(const PropertyValue& fontfacename,
                           const PropertyValue& fontsize,
                           const PropertyValue& fontstyle,
                           const PropertyValue& fontweight,
                           const PropertyValue& fontunderscore,
                           Font& font);

void setPropertiesFromFont(const Font& font,
                           PropertyValue& fontfacename,
                           PropertyValue& fontsize,
                           PropertyValue& fontstyle,
                           PropertyValue& fontweight,
                           PropertyValue& fontunderscore);

// functions for components child-manipulation
IComponentPtr CopyComponentTree(IComponentPtr component);
bool hasChildComponent(IComponentPtr parent, IComponentPtr child);
void getAllSubComponents(std::vector<Component*>& components, Component* initial);
void getAllSubComponents(std::vector<IComponentPtr>& components, IComponentPtr initial);
void getUniqueSubComponents(std::vector<IComponentPtr>& components, IComponentPtr initial);

// helper functions for table
bool getBoundingTextColumnIdx(const CellRange& range,
                              std::vector<CellProperties>& props,
                              std::vector<CellProperties>::reverse_iterator it,
                              int* idx, bool max);

bool getTextColumns(const CellRange& range,
                    std::vector<CellProperties>& props,
                    std::vector<CellProperties>::reverse_iterator it,
                    std::vector<int>& columns);

bool getTextRows(const CellRange& range,
                 std::vector<CellProperties>& props,
                 std::vector<CellProperties>::reverse_iterator it,
                 std::vector<int>& rows);

void getBoundingRange(const std::vector<CellRange>& cells,
                      const CellRange& input_range,
                      CellRange& bounding_range);

void getBoundingRange(const std::vector<CellProperties>& cells,
                      const CellRange& input_range,
                      CellRange& bounding_range);

void insertRowIntoProperties(std::vector<CellProperties>& properties, int idx, int count);
void removeRowFromProperties(std::vector<CellProperties>& properties, int idx, int count);

void insertColumnIntoProperties(std::vector<CellProperties>& properties, int idx, int count);
void removeColumnFromProperties(std::vector<CellProperties>& properties, int idx, int count);


};  // namespace kcanvas


#endif

