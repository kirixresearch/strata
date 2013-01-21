/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2008-02-01
 *
 */


#ifndef __KCANVAS_RANGE_H
#define __KCANVAS_RANGE_H


namespace kcanvas
{


// Canvas Cell Range Class

class CellRange
{
public:

    CellRange();
    CellRange(int row, int col);
    CellRange(int row1, int col1, int row2, int col2);
    CellRange(const CellRange& c);
    virtual ~CellRange();

    CellRange& operator=(const CellRange& c);
    bool operator==(const CellRange& c) const;
    bool operator!=(const CellRange& c) const;

    void set(int row, int col);
    void set(int row1, int col1, int row2, int col2);
    void get(int* row1, int* col1, int* row2, int* col2) const;

    int row1() const;
    int col1() const;
    int row2() const;
    int col2() const;

    bool contains(int row, int col) const;
    bool contains(const CellRange& range) const;

    bool intersects(const CellRange& range) const;
    bool isEmpty() const;
    bool isRow() const;
    bool isColumn() const;

    CellRange& insertRow(int idx, int count = 1);
    CellRange& removeRow(int idx, int count = 1);
    
    CellRange& insertColumn(int idx, int count = 1);
    CellRange& removeColumn(int idx, int count = 1);

    CellRange& expand(int row_diff, int col_diff);
    CellRange& shift(int row_diff, int col_diff);
    CellRange& move(int row, int col);

    CellRange& intersect(const CellRange& range);
    CellRange& bound(const CellRange& range);
    CellRange& clear();

public:

    // static utility function declarations
    static void normalize(int* row1, int* col1, int* row2, int* col2);
    static void Xor(const CellRange& range1,
                    const CellRange& range2,
                    std::vector<CellRange>& xor_ranges);

private:

    int m_row1;
    int m_col1;
    int m_row2;
    int m_col2;
    bool m_empty;
};


class CellProperties
{
public:

    CellProperties();
    CellProperties(const CellRange& range);
    CellProperties(const CellRange& range, const Properties& properties);
    CellProperties(const CellProperties& c);
    virtual ~CellProperties();

    CellProperties& operator=(const CellProperties& c);
    bool operator==(const CellProperties& c) const;
    bool operator!=(const CellProperties& c) const;

    void addProperty(const wxString& prop_name, const PropertyValue& value);
    void addProperties(const Properties& properties);
    void removeProperty(const wxString& prop_name);

    bool setProperty(const wxString& prop_name, const PropertyValue& value);
    bool setProperties(const Properties& properties);
    bool getProperty(const wxString& prop_name, PropertyValue& value) const;
    bool getProperties(Properties& properties) const;
    void listProperties(Properties& properties) const;

    void setRange(int row1, int col1, int row2, int col2);
    void setRange(const CellRange& range);
    void getRange(int* row1, int* col1, int* row2, int* col2) const;
    void getRange(CellRange& range) const;

    bool contains(int row, int col) const;
    bool contains(const CellRange& range) const;

    const Properties& properties() const;
    const CellRange& range() const;

    // function for setting flag
    void setFlag(bool value);
    bool isFlagged();

public:

    // properties and range
    Properties m_properties;
    CellRange m_range;

private:

    // flag to easily mark certain cell properties when
    // dealing with lists of cell properties
    bool m_flag;
};


}; // namespace kcanvas


#endif

