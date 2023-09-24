/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2008-02-01
 *
 */


#include "kcanvas_int.h"
#include "range.h"


namespace kcanvas
{


CellRange::CellRange()
{
    // note: cell range constructor without any 
    // parameters creates an empty range

    // set the range to the global range, and 
    // set the empty flag to true
    m_row1 = -1;
    m_col1 = -1;
    m_row2 = -1;
    m_col2 = -1;
    m_empty = true;
}

CellRange::CellRange(int row, int col)
{
    // make sure the input values are good 
    normalize(&row, &col, &row, &col);

    // set the range for a single cell
    m_row1 = row;
    m_col1 = col;
    m_row2 = row;
    m_col2 = col;
    m_empty = false;
}

CellRange::CellRange(int row1, int col1, int row2, int col2)
{
    // make sure the input values are good
    normalize(&row1, &col1, &row2, &col2);

    // set the range
    m_row1 = row1;
    m_col1 = col1;
    m_row2 = row2;
    m_col2 = col2;
    m_empty = false;
}

CellRange::CellRange(const CellRange& c)
{
    m_row1 = c.m_row1;
    m_col1 = c.m_col1;
    m_row2 = c.m_row2;
    m_col2 = c.m_col2;
    m_empty = c.m_empty;
}

CellRange::~CellRange()
{
}

CellRange& CellRange::operator=(const CellRange& c)
{
    if (this == &c)
        return *this;

    m_row1 = c.m_row1;
    m_col1 = c.m_col1;
    m_row2 = c.m_row2;
    m_col2 = c.m_col2;
    m_empty = c.m_empty;

    return *this;
}

bool CellRange::operator==(const CellRange& c) const
{
    if (this == &c)
        return true;

    if (m_row1 != c.m_row1)
        return false;

    if (m_col1 != c.m_col1)
        return false;

    if (m_row2 != c.m_row2)
        return false;

    if (m_col2 != c.m_col2)
        return false;

    if (m_empty != c.m_empty)
        return false;

    return true;
}

bool CellRange::operator!=(const CellRange& c) const
{
    return !(*this == c);
}

void CellRange::set(int row, int col)
{
    set(row, col, row, col);
}

void CellRange::set(int row1, int col1, int row2, int col2)
{
    // turn the empty flag off
    m_empty = false;

    // make sure the input values are good
    normalize(&row1, &col1, &row2, &col2);
 
    // set the ranges
    m_row1 = row1;
    m_col1 = col1;
    m_row2 = row2;
    m_col2 = col2;
}

void CellRange::get(int* row1, int* col1, int* row2, int* col2) const
{
    *row1 = m_row1;
    *col1 = m_col1;
    *row2 = m_row2;
    *col2 = m_col2;
}

int CellRange::row1() const
{
    return m_row1;
}

int CellRange::col1() const
{
    return m_col1;
}

int CellRange::row2() const
{
    return m_row2;
}

int CellRange::col2() const
{
    return m_col2;
}

bool CellRange::contains(int row, int col) const
{
    return contains(CellRange(row, col));
}

bool CellRange::contains(const CellRange& range) const
{
    // if the input range is empty, return false
    if (range.isEmpty())
        return false;

    // if the intersection of this range with the non-empty
    // input range is the same as the input range, then the
    // input range is contained in this range, so return true;
    // otherwise, return false
    CellRange r(*this);
    r.intersect(range);
    return (r == range);
}

bool CellRange::intersects(const CellRange& range) const
{
    // note: this function returns true if the input range
    // intersects this range; false otherwise

    CellRange r(*this);
    return !r.intersect(range).isEmpty();
}

bool CellRange::isEmpty() const
{
    if (m_empty)
        return true;
        
    return false;
}

bool CellRange::isRow() const
{
    // if the row range markers aren't set and the
    // column range markers are set, it's a row range;
    // return true
    if (m_row1 != -1 && m_row2 != -1 &&
        m_col1 == -1 && m_col2 == -1)
    {
        return true;
    }
    
    // otherwise, return false
    return false;
}

bool CellRange::isColumn() const
{
    // if the column range markers aren't set and the
    // range range markers are set, it's a column range;
    // return true
    if (m_col1 != -1 && m_col2 != -1 &&
        m_row1 == -1 && m_row2 == -1)
    {
        return true;
    }

    // otherwise, return false
    return false;
}

CellRange& CellRange::insertRow(int idx, int count)
{
    // if the count is less than 1, there are no rows
    // to add, so we're done
    if (count < 1)
        return *this;

    // clamp the insertion idx to value of zero or more
    if (idx < 0)
        idx = 0;

    // if the row idx is contained in this range, expand the 
    // range to accomodate the new rows; if the row idx is
    // greater than the cell range, move the cell range down
    // by the number of rows inserted; note: the following 
    // code, which implements this, works properly with
    // range markers since row insertion doesn't change the
    // range marker, which results in the correct behavior
    int r1, c1, r2, c2;
    get(&r1, &c1, &r2, &c2);
    
    if (r1 >= idx)
        r1 += count;
        
    if (r2 >= idx)
        r2 += count;
        
    set(r1, c1, r2, c2);
    return *this;
}

CellRange& CellRange::removeRow(int idx, int count)
{
    // if the count is less than 1, there are no rows
    // to remove, so we're done
    if (count < 1)
        return *this;

    // set the range of cells to delete
    int i1 = idx;
    int i2 = idx + count - 1;

    // get the cell range of the properties
    int r1, c1, r2, c2;
    get(&r1, &c1, &r2, &c2);

    // if the rows being removed entirely contain this
    // range of rows, empty this range and return it
    if (i1 <= r1 && i2 >= r2)
    {
        clear();
        return *this;
    }

    // if this range spans all the rows, even if we
    // delete rows, we still have all the rows, so 
    // simply return this range
    if (r1 == -1 && r2 == -1)
    {
        return *this;
    }

    // if the rows being removed come after this range,
    // simply return this range
    if (i1 > r2)
    {
        return *this;
    }

    // if the rows being removed come before this range,
    // move up the range by the number of rows being
    // removed
    if (i2 < r1)
    {
        set(r1 - count, c1, r2 - count, c2);
        return *this;
    }
            
    // if the rows being removed are contained in this 
    // range, subract the number of rows being removed 
    // from this range
    if (i1 >= r1 && i2 <= r2)
    {
        set(r1, c1, r2 - count, c2);
        return *this;
    }

    // if the rows being removed overlap the beginning of 
    // this range, set the row portion of this range to the
    // rows that don't intersect the deleted range, shifted
    // to the index of the deleted row
    if (i1 < r1 && i2 >= r1 && i2 < r2)
    {
        set(i1, c1, i1 + (r2 - i2 - 1), c2);
        return *this;
    }
    
    // if the rows being removed overlap the end of this
    // range, set the second parameter of this range to the
    // idx of the row being deleted
    if (i1 > r1 && i1 <= r2 && i2 > r2)
    {
        set(r1, c1, i1 - 1, c2);
        return *this;
    }

    // all cases should be covered, so shouldn't come here
    return *this;
}

CellRange& CellRange::insertColumn(int idx, int count)
{
    // if the count is less than 1, there are no columns
    // to add, so we're done
    if (count < 1)
        return *this;

    // clamp the insertion idx to value of zero or more
    if (idx < 0)
        idx = 0;

    // if the column idx is contained in this range, expand the 
    // range to accomodate the new columns; if the column idx is
    // greater than the cell range, move the cell range down
    // by the number of columns inserted; note: the following 
    // code, which implements this, works properly with
    // range markers since column insertion doesn't change the
    // range marker, which results in the correct behavior
    int r1, c1, r2, c2;
    get(&r1, &c1, &r2, &c2);
    
    if (c1 >= idx)
        c1 += count;
        
    if (c2 >= idx)
        c2 += count;
        
    set(r1, c1, r2, c2);
    return *this;
}

CellRange& CellRange::removeColumn(int idx, int count)
{
    // if the count is less than 1, there are no columns
    // to remove, so we're done
    if (count < 1)
        return *this;

    // set the range of cells to delete
    int i1 = idx;
    int i2 = idx + count - 1;

    // get the cell range of the properties
    int r1, c1, r2, c2;
    get(&r1, &c1, &r2, &c2);

    // if the columns being removed entirely contain this
    // range of columns, empty this range and return it
    if (i1 <= c1 && i2 >= c2)
    {
        clear();
        return *this;
    }

    // if this range spans all the columns, even if we
    // delete columns, we still have all the columns, so 
    // simply return this range
    if (c1 == -1 && c2 == -1)
    {
        return *this;
    }

    // if the columns being removed come after this range,
    // simply return this range
    if (i1 > c2)
    {
        return *this;
    }

    // if the columns being removed come before this range,
    // move up the range by the number of columns being
    // removed
    if (i2 < c1)
    {
        set(r1, c1 - count, r2, c2 - count);
        return *this;
    }
            
    // if the columns being removed are contained in this 
    // range, subract the number of columns being removed 
    // from this range
    if (i1 >= c1 && i2 <= c2)
    {
        set(r1, c1, r2, c2 - count);
        return *this;
    }

    // if the columns being removed overlap the beginning of 
    // this range, set the column portion of this range to the
    // columns that don't intersect the deleted range, shifted
    // to the index of the deleted column
    if (i1 < c1 && i2 >= c1 && i2 < c2)
    {
        set(r1, i1, r2, i1 + (c2 - i2 - 1));
        return *this;
    }

    // if the columns being removed overlap the end of this
    // range, set the second parameter of this range to the
    // idx of the column being deleted
    if (i1 > c1 && i1 <= c2 && i2 > c2)
    {
        set(r1, c1, r2, i1 - 1);
        return *this;
    }

    // all cases should be covered, so shouldn't come here
    return *this;
}

CellRange& CellRange::expand(int row_diff, int col_diff)
{
    // if the range is empty, we're done
    if (isEmpty())
        return *this;

    // if the row and column positions aren't ranges,
    // then expand the range by the given offset
    if (m_row1 != -1)
        m_row1 -= row_diff;
        
    if (m_col1 != -1)
        m_col1 -= col_diff;
        
    if (m_row2 != -1)
        m_row2 += row_diff;
        
    if (m_col2 != -1)
        m_col2 += col_diff;
        
    // make sure the start position isn't less than zero
    m_row1 = wxMax(0, m_row1);
    m_col1 = wxMax(0, m_col1);
    
    return *this;
}

CellRange& CellRange::shift(int row_diff, int col_diff)
{
    // if the range is empty, we're done
    if (isEmpty())
        return *this;

    // shift the rows and columns that aren't ranges by
    // the row and column difference, respectively
    if (m_row1 != -1 && m_row2 != -1)
    {
        m_row1 += row_diff;
        m_row2 += row_diff;
        
        // only keep the part of the range that's in 
        // the valid area
        if (m_row1 < 0)
            m_row1 = 0;

        // if the range is shifted off the valid area,
        // set the range to empty
        if (m_row2 < 0)
            clear();
    }
        
    if (m_col1 != -1 && m_col2 != -1)
    {
        m_col1 += col_diff;
        m_col2 += col_diff;

        // only keep the part of the range that's in 
        // the valid area
        if (m_col1 < 0)
            m_col1 = 0;

        // if the range is shifted off the valid area,
        // set the range to empty
        if (m_col2 < 0)
            clear();
    }

    return *this;
}

CellRange& CellRange::move(int row, int col)
{
    // if the range is empty, we're done
    if (isEmpty())
        return *this;

    // calculate the offset of the new position
    // from the current position
    int row_diff = (m_row1 != -1) ? row - m_row1 : 0;
    int col_diff = (m_col1 != -1) ? col - m_col1 : 0;

    // move the cell range to the new position
    shift(row_diff, col_diff);
    
    return *this;
}

inline bool intersect_p(int r1v1, int r1v2, 
                        int r2v1, int r2v2, 
                        int* iv1, int* iv2)
{
    // note: takes two values from range1 (r1v1, r1v2) and two 
    // values from range2 (r2v1, r2v2), both along a  single row or 
    // column axis, and determines the range of the intersection 
    // (iv1, iv2); returns true if the ranges intersect and false 
    // otherwise

    // the projection of a range on an axis is a pair of integer 
    // values; the maximum of the first value in each range 
    // projection is the first value of the intersection, and 
    // the minimum of the second value in each range projection 
    // is the second value in the intersection; if the first value 
    // of the new range is larger than the second value of the 
    // new range, there's no intersection;  so, if the first pair 
    // of values is (2,4) and the second pair of values is (3,5), 
    // the maximum of 2 and 3 is 3 and the minimum of 4 and 5 is 
    // 4, so the intersection is (3,4); but if the first pair of 
    // values is (2,3) and the second pair of values values is 
    // (4,5), the maximum of 2 and 4 is 4 and the minimum of 3
    // and 5 is 3, and since 4 > 3, the ranges don't intersect

    // find the maximum of the first two values; note: range markers
    // (-1), don't interfere because we're taking a maximum
    int v1 = wxMax(r1v1, r2v1);

    // find the maximum of the second two values; note: in this
    // case, range markers interfere, since we're taking the 
    // minimum; to solve this, if we end up with a -1 after taking 
    // the minimum, take the other value; whether or not the other 
    // value is a -1 doesn't matter: if it isn't, it's the minimum, 
    // if it is it's equal to the original value and therefore still 
    // represents the minimum, which in this case is a range marker
    int v2 = wxMin(r1v2, r2v2);
    if (r1v2 == -1 || r2v2 == -1)
        v2 = wxMax(r1v2, r2v2);

    // if we don't have range markers and v1 > v2, we don't
    // have an intersection; return false
    if (v1 >= 0 && v2 >= 0 && v1 > v2)
        return false;

    // we have an intersection; set the return values and
    // return true
    *iv1 = v1;
    *iv2 = v2;

    return true;
}

CellRange& CellRange::intersect(const CellRange& range)
{
    // note: this function finds the intersection between the 
    // input range and this range and sets this range to the 
    // intersection; if there's no intersection, the function 
    // clears this range so that when it's returned, the empty
    // function on it returns true, indicating no intersection;
    // the intersection of two ranges is the range the two ranges 
    // have in common; so, the intersection of CellRange(1,1,2,2) 
    // and CellRange(2,2,3,3) is CellRange(2,2)

    // if the input range is this range, simply return it
    if (this == &range)
        return *this;
        
    // if this range is empty, simply return it
    if (isEmpty())
        return *this;
        
    // if the input range is emtpy, clear this range and
    // return it
    if (range.isEmpty())
        return clear();

    // get the range
    int row1, col1, row2, col2;
    range.get(&row1, &col1, &row2, &col2);
    
    // find the row intersection; if the rows don't intersect,
    // clear the range and return it
    int irow1, irow2;
    if (!intersect_p(m_row1, m_row2, row1, row2, &irow1, &irow2))
        return clear();

    // find the column intersection; if the columns don't
    // intersect, clear the range and return it        
    int icol1, icol2;
    if (!intersect_p(m_col1, m_col2, col1, col2, &icol1, &icol2))
        return clear();

    // set the range to the range of the intersection
    set(irow1, icol1, irow2, icol2);

    // return this range
    return *this;
}

CellRange& CellRange::bound(const CellRange& range)
{
    // note: this function finds the range of cells that bounds the
    // input range and this range, and sets this range to the new
    // bounding range; if either the input range or this range is
    // empty, it sets the bounding range to the non-empty range

    // if the input range is this range, return a reference, and
    // we're done
    if (this == &range)
        return *this;

    // if this range is empty, return a reference, and we're done;
    // this also covers the case where the input range may be empty,
    // in which case, both this range and the input range are empty, 
    // so the bounding range is also empty
    if (isEmpty())
        return (*this = range);

    // if the input range is empty, return a reference, and we're
    // done; this also covers the case where this range may be empty,
    // in which case, both the input range and this range are empty, 
    // so the bounding range is also empty
    if (range.isEmpty())
        return *this;

    // get the input range
    int row1, col1, row2, col2;
    range.get(&row1, &col1, &row2, &col2);

    // save a copy of the input
    int row1_old = row1;
    int col1_old = col1;
    int row2_old = row2;
    int col2_old = col2;

    // if there are any range markers on this range, set the 
    // corresponding bounding row or column to the range marker
    if (m_row1 == -1)
        row1 = m_row1;
        
    if (m_col1 == -1)
        col1 = m_col1;
        
    if (m_row2 == -1)
        row2 = m_row2;
        
    if (m_col2 == -1)
        col2 = m_col2;
        
    // if there are range markers on the input set, leave them alone;
    // otherwise, find the minimum and maximum row and column values 
    // and set the corresponding row or column start or end value to 
    // those values, respectively
    if (row1 != -1)
        row1 = wxMin(row1, m_row1);

    if (col1 != -1)
        col1 = wxMin(col1, m_col1);

    if (row2 != -1)
        row2 = wxMax(row2, m_row2);
        
    if (col2 != -1)
        col2 = wxMax(col2, m_col2);

    // set the bounding range and return a reference
    set(row1, col1, row2, col2);
    return *this;
}

CellRange& CellRange::clear()
{
    // set the row and columns to the global range, and 
    // set the empty flag to true; return a reference
    m_row1 = -1;
    m_col1 = -1;
    m_row2 = -1;
    m_col2 = -1;
    m_empty = true;
    return *this;
}

void CellRange::normalize(int* row1, int* col1, int* row2, int* col2)
{
    // note: this function makes sure row1 is less than row2 and
    // col1 is less than col2, and if any inputs are less than zero, 
    // that they are set to -1 to represent a range

    // if row1 is greater than row2, reverse them
    if (*row1 > *row2)
    {
        int t = *row2;
        *row2 = *row1;
        *row1 = t;
    }

    // if col1 is greater than col2, reverse them
    if (*col1 > *col2)
    {
        int t = *col2;
        *col2 = *col1;
        *col1 = t;
    }    

    // if row1 is less than zero, set the row range marker
    if (*row1 < 0)
    {
        *row1 = -1;
        *row2 = -1;
    }
    
    // if col1 is less than zero, set the column range marker
    if (*col1 < 0)
    {
        *col1 = -1;
        *col2 = -1;
    }
}

void CellRange::Xor(const CellRange& range1,
                    const CellRange& range2,
                    std::vector<CellRange>& xor_ranges)
{
    // note: this function finds the exclusive-or of two
    // ranges and returns the result as a series of ranges
  
    // if the two input ranges are the same, there's no
    // exclusive-or; return
    if (range1 == range2)
        return;
    
    // if range1 is empty, add range2 and we're done
    if (range1.isEmpty())
    {
        xor_ranges.push_back(range2);
        return;
    }
    
    // if range2 is empty, add range1 and we're done
    if (range2.isEmpty())
    {
        xor_ranges.push_back(range1);
        return;
    }
    
    // find the intersection between the ranges
    CellRange r = range1;
    r.intersect(range2);
    
    // if the intersection is empty, add the two starting
    // ranges to the output list and we're done
    if (r.isEmpty())
    {
        xor_ranges.push_back(range1);
        xor_ranges.push_back(range2);
        return;
    }

    // if range2 isn't contained in range1, xor the
    // intersection with each range, and we're done
    if (!range1.contains(range2))
    {
        CellRange::Xor(range1, r, xor_ranges);
        CellRange::Xor(range2, r, xor_ranges);
        return;
    }

    // range2 is contained in range1, so find the sub-ranges
    // of range1 that are outside range2 and return these;
    // first, get the rows and columns of each range
    int range1_r1, range1_c1, range1_r2, range1_c2;
    int range2_r1, range2_c1, range2_r2, range2_c2;
    range1.get(&range1_r1, &range1_c1, &range1_r2, &range1_c2);
    range2.get(&range2_r1, &range2_c1, &range2_r2, &range2_c2);

    // find the sub-range to the left of range2, and if it's 
    // not empty, add it to the list
    if (range1_c1 != range2_c1)
    {
        CellRange left(range1_r1, range1_c1, range1_r2, range2_c1-1);
        xor_ranges.push_back(left);
    }
    
    // find the sub-range to the right of range2, and if it's 
    // not empty, add it to the list
    if (range2_c2 != range1_c2)
    {
        CellRange right(range1_r1, range2_c2+1, range1_r2, range1_c2);
        xor_ranges.push_back(right);
    }

    // find the sub-range to the top of range2, and if it's 
    // not empty, add it to the list
    if (range1_r1 != range2_r1)
    {
        CellRange top(range1_r1, range2_c1, range2_r1-1, range2_c2);
        xor_ranges.push_back(top);
    }
    
    // find the sub-range to the bottom of range2, and if it's 
    // not empty, add it to the list
    if (range2_r2 != range1_r2)
    {
        CellRange bottom(range2_r2+1, range2_c1, range1_r2, range2_c2);
        xor_ranges.push_back(bottom);
    }
}

// cell properties class definition

CellProperties::CellProperties()
{
    m_flag = false;
}

CellProperties::CellProperties(const CellRange& range)
{
    m_range = range;
    m_flag = false;
}

CellProperties::CellProperties(const CellRange& range, const Properties& properties)
{
    m_range = range;
    m_properties = properties;
    m_flag = false;
}

CellProperties::CellProperties(const CellProperties& c)
{
    m_properties = c.m_properties;
    m_range = c.m_range;
    m_flag = c.m_flag;
}

CellProperties::~CellProperties()
{
}

CellProperties& CellProperties::operator=(const CellProperties& c)
{
    if (this == &c)
        return *this;

    m_range = c.m_range;
    m_properties = c.m_properties;
    m_flag = c.m_flag;

    return *this;
}

bool CellProperties::operator==(const CellProperties& c) const
{
    if (this == &c)
        return true;

    if (m_range != c.m_range)
        return false;

    if (m_properties != c.m_properties)
        return false;

    if (m_flag != c.m_flag)
        return false;

    return true;
}

bool CellProperties::operator!=(const CellProperties& c) const
{
    return !(*this == c);
}

void CellProperties::addProperty(const wxString& prop_name, const PropertyValue& value)
{
    m_properties.add(prop_name, value);
}

void CellProperties::addProperties(const Properties& properties)
{
    m_properties.add(properties);
}

void CellProperties::removeProperty(const wxString& prop_name)
{
    m_properties.remove(prop_name);
}

bool CellProperties::setProperty(const wxString& prop_name, const PropertyValue& value)
{
    return m_properties.set(prop_name, value);
}

bool CellProperties::setProperties(const Properties& properties)
{
    return m_properties.set(properties);
}

bool CellProperties::getProperty(const wxString& prop_name, PropertyValue& value) const
{
    return m_properties.get(prop_name, value);
}

bool CellProperties::getProperties(Properties& properties) const
{
    return m_properties.get(properties);
}

void CellProperties::listProperties(Properties& properties) const
{
    m_properties.list(properties);
}

void CellProperties::setRange(int row1, int col1, int row2, int col2)
{
    m_range.set(row1, col1, row2, col2);
}

void CellProperties::setRange(const CellRange& range)
{
    m_range = range;
}

void CellProperties::getRange(int* row1, int* col1, int* row2, int* col2) const
{
    m_range.get(row1, col1, row2, col2);
}

void CellProperties::getRange(CellRange& range) const
{
    range = m_range;
}

bool CellProperties::contains(int row, int col) const
{
    return m_range.contains(row, col);
}

bool CellProperties::contains(const CellRange& range) const
{
    return m_range.contains(range);
}

const Properties& CellProperties::properties() const
{
    return m_properties;
}

const CellRange& CellProperties::range() const
{
    return m_range;
}

void CellProperties::setFlag(bool value)
{
    m_flag = value;
}

bool CellProperties::isFlagged()
{
    return m_flag;
}


} // namespace kcanvas

