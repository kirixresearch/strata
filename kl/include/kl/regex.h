/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-01-25
 *
 */


#ifndef H_KL_REGEX_H
#define H_KL_REGEX_H


#include <vector>
#include <set>
#include <string>
#include <wctype.h>


namespace klregex
{


enum
{
    matchFailed = 0,
    matchSucceeded = 1,
};

template <class charT>
class regpart_data
{
public:

    regpart_data()
    {
        start = NULL;
    }

    const charT* start;
};



// this is a quick and easy stack class,
// which handles rapid expansion quickly

template <class T>
class reg_stack
{
public:

    reg_stack()
    {
        stack = NULL;
        size = 0;
        alloc_size = 0;
    }

    ~reg_stack()
    {
        delete[] stack;
    }

    void reserve(int size)
    {
        if (size <= alloc_size)
            return;

        T* new_stack = new T[size];
        for (int i = 0; i < alloc_size; ++i)
            new_stack[i] = stack[i];
        delete[] stack;
        stack = new_stack;
        alloc_size = size;
    }

    void pop()
    {
        if (size > 0)
            size--;
    }

    void push(const T& val)
    {
        if (size >= alloc_size)
        {
            reserve(alloc_size*2);
        }

        stack[size] = val;
        size++;
    }

    T& top()
    {
        return stack[size-1];
    }

    void clear()
    {
        size = 0;
    }

public:

    int* stack;
    int size;
    int alloc_size;
};



template <class charT>
inline int quickAtoi(const charT* s)
{
    int res = 0;
    bool neg = false;
    while (iswspace(*s))
        ++s;
    if (*s == L'-')
        neg = true;
    while (*s >= '0' && *s <= '9')
    {
        res = (res*10);
        res += (*s - '0');
        ++s;
    }
    return res;
}

template <class charT>
inline size_t quickStrlen(const charT* s)
{
    const charT* p = s;
    while (*p)
        ++p;
    return p-s;
}

template <class charT>
inline const charT* quickStrchr(const charT* s, charT ch)
{
    while (*s)
    {
        if (*s == ch)
            return s;
        ++s;
    }
    return 0;
}

template <class charT>
inline int quickStrncmp(const charT* s1, const wchar_t* s2, int len)
{
    while (*s2)
    {
        if (!*s1 || (wchar_t)*s1 < *s2)
            return -1;
        if ((wchar_t)*s1 > *s2)
            return 1;
        ++s1;
        ++s2;
    }
    return 0;
}


template <class charT>
class sub_match
{
public:

    sub_match()
    {
        temp_first = NULL;
        first = NULL;
        second = NULL;
        idx = -1;
    }

    bool isValid() const
    {
        return (first != 0) ? true : false;
    }

    size_t length() const
    {
        if (second < first || !first || !second)
            return 0;

        return (second - first);
    }

    std::basic_string<charT> str() const
    {
        // the following 4 lines are not a permanent fix
        // and should be removed in the future
        if (second < first || !first || !second)
        {
            return std::basic_string<charT>();
        }

        return std::basic_string<charT>(first, second - first);
    }

    std::basic_string<charT> getName() const
    {
        return name;
    }

    void clear()
    {
        temp_first = NULL;
        first = NULL;
        second = NULL;
        idx = -1;
        name = std::basic_string<charT>();
    }

public:

    const charT* first;
    const charT* second;
    const charT* temp_first;
    std::basic_string<charT> name;
    int idx;
};

template <class charT>
class match_results
{
public:

    match_results()
    {
        p_unwind = NULL;
        stack.reserve(8);
    }

    ~match_results()
    {
    }

    const sub_match<charT>& operator[](const std::basic_string<charT>& name) const
    {
        size_t i, submatch_count = submatches.size();
        for (i = 0; i < submatch_count; ++i)
        {
            if (submatches[i].name == name)
                return submatches[i];
        }
        return null_submatch;
    }

    const sub_match<charT>& operator[](size_t idx) const
    {
        if (idx < submatches.size())
            return submatches[idx];

        return null_submatch;
    }

    size_t size() const
    {
        return submatches.size();
    }

    void clear()
    {
        submatches.clear();
        stack.clear();
        p_unwind = NULL;
    }


public:

    sub_match<charT>& getSubMatch(const std::basic_string<charT>& name)
    {
        size_t i, submatch_count = submatches.size();
        for (i = 0; i < submatch_count; ++i)
        {
            if (submatches[i].name == name)
                return submatches[i];
        }

        sub_match<charT> sm;
        sm.name = name;
        submatches.push_back(sm);
        return submatches[submatches.size() - 1];
    }
    
    sub_match<charT>& getSubMatch(int idx)
    {
        if (idx >= 0 && idx < (int)submatches.size())
        {
            sub_match<charT>& s = submatches[idx];
            if (s.idx == idx)
            {
                return s;
            }
        }


        if ((size_t)idx < submatches.size() && submatches[idx].name.length() > 0)
        {
            // we need to move this named match elsewhere
            sub_match<charT> temps = submatches[idx];
            submatches.push_back(temps);

            submatches[idx].clear();
            submatches[idx].idx = idx;
            return submatches[idx];
        }


        while ((size_t)idx >= submatches.size())
        {
            sub_match<charT> s;
            submatches.push_back(s);
        }

        submatches[idx].idx = idx;
        return submatches[idx];
    }


    std::basic_string<charT> format(const std::basic_string<charT>& fmt)
    {
        return format(fmt.c_str(), fmt.length());
    }

    std::basic_string<charT> format(const charT* fmt)
    {
        return format(fmt, quickStrlen(fmt));
    }


private:

    std::basic_string<charT> format(const charT* fmt, size_t len)
    {
        std::basic_string<charT> result;
        result.reserve(len * 2);

        while (*fmt)
        {
            if (*fmt == L'$' && *(fmt+1))
            {
                if (*(fmt+1) == L'&')
                {
                    // replace with whole match text
                    const sub_match<charT>& submatch = (*this)[0];
                    if (submatch.isValid())
                        result.append(submatch.first, submatch.length());
                    fmt += 2;
                    continue;
                }
                 else if (*(fmt+1) == L'`')
                {
                    // replace with the text between the string start and
                    // the start of the first submatch
                    const sub_match<charT>& submatch = (*this)[0];
                    if (submatch.isValid())
                        result.append(src_begin, submatch.first - src_begin);
                    fmt += 2;
                    continue;
                }
                 else if (*(fmt+1) == L'\'')
                {
                    // replace with the text between the end of the match and
                    // the end of the source string
                    const sub_match<charT>& submatch = (*this)[0];
                    if (submatch.isValid())
                        result.append(submatch.second, src_end - submatch.second);
                    fmt += 2;
                    continue;
                }
                 else if (*(fmt+1) == L'$')
                {
                    result += *(fmt+1);
                    fmt += 2;
                    continue;
                }
                 else if (::iswdigit(*(fmt+1)))
                {
                    // replace with text from indexed submatch
                    const sub_match<charT>& submatch = (*this)[(int)(*(fmt+1) - '0')];
                    if (submatch.isValid())
                        result.append(submatch.first, submatch.length());
                    fmt += 2;
                    continue;
                }
                 else if (*(fmt+2) && *(fmt+1) == L'{')
                {
                    // replace with text from named submatch
                    const charT* close_brace = quickStrchr(fmt+2, L'}');
                    if (close_brace)
                    {
                        if (::iswdigit(*(fmt+2)))
                        {
                            const sub_match<charT>& submatch = (*this)[quickAtoi(fmt+2)];
                            if (submatch.isValid())
                                result.append(submatch.first, submatch.length());
                            fmt = close_brace+1;
                            continue;
                        }
                         else
                        {
                            std::basic_string<charT> tag(fmt+2, close_brace);
                            const sub_match<charT>& submatch = (*this)[tag];
                            if (submatch.isValid())
                                result.append(submatch.first, submatch.length());
                            fmt = close_brace+1;
                            continue;
                        }
                    }
                }
           }

            result += *fmt;
            ++fmt;
        }

        return result;
    }

public:
    
    std::vector< sub_match<charT> > submatches;

    sub_match<charT> null_submatch;
    const charT* src_begin;
    const charT* src_end;

    const charT* p_unwind;
    reg_stack<int> stack;
    std::vector< regpart_data<charT> > partdata;
};


template <class charT>
class reg_stack_popper
{
public:

    reg_stack_popper(match_results<charT>& _m) : m(_m)
    {
        save_val = m.stack.top();
        m.stack.pop();
    }

    ~reg_stack_popper()
    {
        m.stack.push(save_val);
    }

    int getValue() const
    {
        return m.stack.top();
    }

private:

    match_results<charT>& m;
    int save_val;
};



template <class charT>
class regpart
{
public:

    regpart(regpart<charT>* parent)
    {
        m_next_part = NULL;
        m_parent = parent;
        m_id = 0;
        m_allparts_count = 0;

        if (m_parent)
        {
            m_id = m_parent->assignId();
            m_ignore_case = parent->m_ignore_case;
            m_single_line = parent->m_single_line;
            m_multi_line = parent->m_multi_line;
            m_look_around = parent->m_look_around;
        }
         else
        {
            m_id = 0;
            m_ignore_case = false;
            m_single_line = false;
            m_multi_line = false;
            m_look_around = false;
        }
    }

    virtual ~regpart()
    {
    }

    virtual int tryRest(const charT* start,
                        const charT* end,
                        const charT** next,
                        match_results<charT>& matchres)
    {
        reg_stack_popper<charT> st(matchres);

        if (!m_parent)
        {
            *next = start;
            return matchSucceeded;
        }

        return m_parent->tryRest(start, end, next, matchres);
    }

    virtual void setNextPart(regpart<charT>* part)
    {
        m_next_part = part;
    }

    virtual int matchInternal(const charT* start,
                              const charT* end,
                              const charT** next,
                              match_results<charT>& matchres) = 0;
private:

    int assignId()
    {
        if (m_parent)
            return m_parent->assignId();

        return ++m_allparts_count;
    }

public:

    regpart<charT>* m_next_part;
    regpart<charT>* m_parent;

    bool m_look_around;
    bool m_ignore_case;
    bool m_single_line;
    bool m_multi_line;

    int m_allparts_count;
    int m_id;
};


template <class charT>
class reg_match_scope
{
public:

    reg_match_scope(regpart<charT>* part,
                    const charT* start,
                    match_results<charT>& m) : m_match(m)
    {
        m_match.stack.push(0);

        m_part_id = part->m_id;
        m_old_start = m_match.partdata[m_part_id].start;
        m_match.partdata[m_part_id].start = start;
    }

    ~reg_match_scope()
    {
        m_match.stack.pop();
        m_match.partdata[m_part_id].start = m_old_start;
    }

    void setValue(int val)
    {
        m_match.stack.top() = val;
    }

    int getValue() const
    {
        return m_match.stack.top();
    }

private:

    match_results<charT>& m_match;
    int m_part_id;
    const charT* m_old_start;
};


template <class charT>
class reg_period : public regpart<charT>
{
public:

    reg_period(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        if (regpart<charT>::m_single_line)
        {
            // in "single line" mode, period matches everything
            if (end <= start)
                return matchFailed;
        }
         else
        {
            // in "non-single line" mode, period matches
            // everything except CR and LF 
            if (end <= start || *start == L'\n' || *start == L'\r')
                return matchFailed;
        }


        *next = start+1;

        return matchSucceeded;
    }
};


template <class charT>
class reg_linebegin : public regpart<charT>
{
public:

    reg_linebegin(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        if (regpart<charT>::m_multi_line)
        {
            if (start <= matchres.src_begin ||
                *(start-1) == L'\n' || *(start-1) == L'\r')
            {
                *next = start;
                return matchSucceeded;
            }
        }
         else
        {
            if (start <= matchres.src_begin)
            {
                *next = start;
                return matchSucceeded;
            }
        }

        return matchFailed;
    }
};

template <class charT>
class reg_lineend : public regpart<charT>
{
public:

    reg_lineend(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    int matchInternal(const charT* start,
               const charT* end,
               const charT** next,
               match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        if (start >= matchres.src_end)
        {
            *next = start;
            return matchSucceeded;
        }

        if (regpart<charT>::m_multi_line)
        {
            const charT* p = start;
            while (*p == L'\n' || *p == L'\r')
                ++p;
        
            if (p > start)
            {
                *next = p+1;
                return matchSucceeded;
            }
        }

        return matchFailed;
    }
};

template <class charT>
class reg_wordbreak : public regpart<charT>
{
public:

    reg_wordbreak(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        int res = matchFailed;

        if (isWordChar(*start) &&
            (start == matchres.src_begin || !isWordChar(*(start-1))))
                res = matchSucceeded;

        if (start > matchres.src_begin && isWordChar(*(start-1)) &&
            (start >= matchres.src_end || !isWordChar(*start)))
                res = matchSucceeded;



        if ((!negated && res == matchSucceeded) ||
            (negated && res == matchFailed))
        {
            *next = start;
            return matchSucceeded;
        }

        return matchFailed;
    }

public:

    bool isWordChar(wchar_t ch) const
    {
        if (::iswalpha(ch) || ::isdigit(ch) || ch == L'_')
            return true;
        return false;
    }

public:
    bool negated;
};


template <class charT>
class reg_or : public regpart<charT>
{
public:

    reg_or(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    ~reg_or()
    {
        size_t i, part_count;
        for (i = 0, part_count = parts.size(); i < part_count; ++i)
            delete parts[i];
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        int res;
        size_t i, part_count;
        const charT* n = NULL;
        const charT* snext = NULL;
        const charT* rnext;

        part_count = parts.size();

        for (i = 0; i < part_count; ++i)
        {               
            res = parts[i]->matchInternal(start, end, &n, matchres);

            if (res == matchSucceeded)
            {
                if (snext && snext != n)
                {
                    // this means that there are more than one in the or
                    // equation that could potentially satisfy the
                    // expression.  They also have different lengths.
                    // We must test the rest of the string.

                    if (regpart<charT>::m_parent->tryRest(snext, end, &rnext, matchres) == matchSucceeded)
                    {
                        *next = snext;
                        return matchSucceeded;
                    }
                }

                snext = n;
            }

        }

        if (!snext)
            return matchFailed;

        // the whole regular expression will not work,
        // but continue on anyway until the inevitable failure

        *next = snext;
        return matchSucceeded;
    }

    void setNextPart(regpart<charT>* part)
    {
        size_t i, part_count;
        for (i = 0, part_count = parts.size(); i < part_count; ++i)
            parts[i]->setNextPart(part);
        regpart<charT>::m_next_part = part;
    }

public:

    std::vector< regpart<charT>* > parts;
};

template <class charT>
class reg_minmax : public regpart<charT>
{
public:

    reg_minmax(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    ~reg_minmax()
    {
        delete part;
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        bool rest_checked;
        return doMatch(0, start, end, next, matchres, &rest_checked);
    }

    int tryRest(const charT* start,
                const charT* end,
                const charT** next,
                match_results<charT>& matchres)
    {
        reg_stack_popper<charT> st(matchres);

        // figure out how many matches were already found
        // prior to the present start location
        int num_matches = matchres.stack.top() + 1;

        if (num_matches >= max_matches)
        {
            if (regpart<charT>::m_parent)
                return regpart<charT>::m_parent->tryRest(start, end, next, matchres);
            *next = start;
            return matchSucceeded;
        }

        // prevent feedback loop

        if (matchres.partdata[regpart<charT>::m_id].start == start)
        {
            int matches = matchres.stack.top();
            if (matches >= min_matches && matches <= max_matches)
            {
                if (regpart<charT>::m_parent)
                    return regpart<charT>::m_parent->tryRest(start, end, next, matchres);

                *next = start;
                return matchSucceeded;
            }
            return matchFailed;
        }


        bool rest_checked;
        const charT* snext;
        {
            reg_stack_popper<charT> st(matchres);

            if (doMatch(num_matches, start, end, &snext,
                        matchres, &rest_checked) == matchFailed)
            {
                return matchFailed;
            }
        }

        if (regpart<charT>::m_parent && !rest_checked)
            return regpart<charT>::m_parent->tryRest(snext, end, next, matchres);

        *next = snext;
        return matchSucceeded;
    }

    int doMatch(int initial_matches,
                const charT* mstart,
                const charT* end,
                const charT** next,
                match_results<charT>& matchres,
                bool* rest_checked)
    {
        reg_match_scope<charT> rs(this, mstart, matchres);

        *rest_checked = true;

        const charT* min_rest_start = NULL;
        const charT* start = mstart;
        const charT* snext = NULL;
        std::vector<const charT*> rest_locs;

        int num_matches = initial_matches;

        if (num_matches >= min_matches)
            min_rest_start = start;

        while (1)
        {
            if (greedy && num_matches >= min_matches)
                rest_locs.push_back(start);

            if (!greedy && regpart<charT>::m_next_part && num_matches >= min_matches)
            {
                // try to match the rest of the regular expression

                const charT* n;
                int res = regpart<charT>::m_parent->tryRest(start, end, &n, matchres);

                if (res == matchSucceeded && n > start)
                {
                    // match for the rest worked! we are not greedy, so the first time 
                    // the rest works out, we are done
                    *next = start;
                    return matchSucceeded;
                }

                if (num_matches >= min_matches && res == matchSucceeded)
                    break;
            }

            if (num_matches >= max_matches)
                break;

            if (!*start)
                break;



            rs.setValue(num_matches);

            if (part->matchInternal(start, end, &snext, matchres) != matchFailed)
            {
                // unwind: see comment in regex::doMatch()
                if (matchres.p_unwind)
                {
                    *next = matchres.p_unwind;
                    return matchSucceeded;
                }


                num_matches++;

                if (num_matches == min_matches)
                    min_rest_start = snext;

                if (snext > start)
                {
                    start = snext;
                }
                 else
                {
                    // if we aren't moving forward, it's an infinite match (e.g. "()*")
                    // which doesn't advance our position. break out of the loop
                    if (max_matches == reg_minmax::match_limit)
                    {
                        *next = start;
                        return matchSucceeded;
                    }
                }
            }
             else
            {
                break;
            }
        }

        if (num_matches < min_matches)
            return matchFailed;

        if (greedy)
        {
            if (num_matches != max_matches && min_matches == max_matches)
            {
                return matchFailed;
            }

            if (num_matches == max_matches && min_matches == max_matches)
            {
                // there are a fixed number of required matches e.g."(){3}"
                // no tryRest is required
                *rest_checked = false;
                *next = start;
                return matchSucceeded;
            }

            if (!snext)
            {
                // no matches, but that's ok, because minimum was 0
                *rest_checked = false;
                *next = start;
                return matchSucceeded;
            }



            if (rest_locs.size() > 0)
            {
                const charT* rest_start;
                const charT* n;
                for (int i = ((int)rest_locs.size())-1; i >= 0; --i)
                {
                    rest_start = rest_locs[i];

                    if (regpart<charT>::m_parent->tryRest(rest_start, end, &n, matchres) == matchSucceeded)
                    {
                        // unwind: see comment in regex::doMatch()
                        if (matchres.p_unwind)
                        {
                            *next = matchres.p_unwind;
                            return matchSucceeded;
                        }

                        *next = rest_start;
                        return matchSucceeded;
                    }
                }
            }

            //*next = start;
            //return matchSucceeded;

            // since all the tryRests failed, the string is doomed
            // to not match the regex.  Even though this reg_minmax
            // succeeded, we will fail the rest of the expression.
            return matchFailed;
        }
         else
        {
            // match worked as far as we're concerned, but the rest
            // could be doomed to fail.
            *next = min_rest_start;
            return matchSucceeded;
        }

        // execution never reaches here
        return matchFailed;
    }


    void setNextPart(regpart<charT>* next_part)
    {
        regpart<charT>::m_next_part = next_part;
        part->setNextPart(next_part);
    }


public:

    enum
    {
        match_limit = 2147483647
    };

    bool greedy;
    int min_matches;
    int max_matches;
    regpart<charT>* part;
};

template <class charT>
class reg_literal : public regpart<charT>
{
public:

    reg_literal(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        const charT* m_it = match_str.c_str();
        const charT* s_it = start;
        const charT* snext;

        snext = start;

        // if the match string is longer than the input
        // string, there is no match; stop here

        if (match_str.length() > (size_t)(end - start))
            return matchFailed;

        while (*m_it)
        {
            if (regpart<charT>::m_ignore_case)
            {
                if (towupper(*m_it) != towupper(*s_it))
                    return matchFailed;
            }
             else
            {
                if (*m_it != *s_it)
                    return matchFailed;
            }

            ++m_it;
            ++s_it;

            ++snext;
        }

        *next = snext;

        return matchSucceeded;
    }
                
public:

    std::basic_string<charT> match_str;
};



template <class charT>
class reg_charclass : public regpart<charT>
{
public:

    reg_charclass(regpart<charT>* parent) : regpart<charT>(parent)
    {
        negated = false;

        int i;
        for (i = 0; i < 256; ++i)
            states1[i] = false;
        for (i = 0; i < 20; ++i)
            builtin[i] = false;
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        if (!*start)
            return matchFailed;

        if (negated)
        {
            if (getState(*start))
            {
                return matchFailed;
            }
             else
            {
                *next = start+1;
                return matchSucceeded;
            }
        }
         else
        {
            if (getState(*start))
            {
                *next = start+1;
                return matchSucceeded;
            }
             else
            {
                return matchFailed;
            }
        }

        return matchFailed;
    }

    void setStateRange(unsigned int first, unsigned int last)
    {
        for (unsigned int i = first; i <= last; ++i)
            setState(i);
    }

    void setState(unsigned int ch)
    {
        if (ch < 256)
            states1[ch] = true;
             else
            states2.insert(ch);

        if (regpart<charT>::m_ignore_case)
        {
            wchar_t case_ch = towupper(ch);
            if (ch != case_ch)
            {
                if (case_ch < 256)
                    states1[case_ch] = true;
                     else
                    states2.insert(case_ch);
            }
            
            case_ch = towlower(ch);
            if (ch != case_ch)
            {
                if (case_ch < 256)
                    states1[case_ch] = true;
                     else
                    states2.insert(case_ch);
            }
        }
    }

    bool getState(unsigned int ch)
    {
        // check the speedy 0..255 array

        if (ch < 256 && states1[ch])
            return true;

        // check the builtin arrays

        if (builtin[builtinSpace] && ::iswspace(ch))
            return true;
        if (builtin[builtinDigit] && ::iswdigit(ch))
            return true;
        if (builtin[builtinUpper] && ::iswupper(ch))
            return true;
        if (builtin[builtinLower] && ::iswlower(ch))
            return true;
        if (builtin[builtinAlpha] && ::iswalpha(ch))
            return true;
        if (builtin[builtinAlnum] && ::iswalnum(ch))
            return true;
        if (builtin[builtinXdigit] && ::iswxdigit(ch))
            return true;
        if (builtin[builtinPrint] && ::iswprint(ch))
            return true;
        if (builtin[builtinPunct] && ::iswpunct(ch))
            return true;
        if (builtin[builtinGraph] && ::iswgraph(ch))
            return true;
        if (builtin[builtinCntrl] && ::iswcntrl(ch))
            return true;
        if (builtin[builtinWord] && isWordChar(ch))
            return true;
        if (builtin[builtinBlank] && isBlank(ch))
            return true;
        if (builtin[builtinAscii] && (ch > 0 && ch <= 0x7f))
            return true;
        if (builtin[builtinInverseWord] && !isWordChar(ch))
            return true;
        if (builtin[builtinInverseDigit] && !::iswdigit(ch))
            return true;
        if (builtin[builtinInverseSpace] && !::iswspace(ch))
            return true;

        return (states2.find(ch) != states2.end()) ? true : false;
    }

    void setBuiltIn(int idx)
    {
        builtin[idx] = true;
    }

    bool isWordChar(wchar_t ch) const
    {
        if (::iswalpha(ch) || ::isdigit(ch) || ch == L'_')
            return true;
        return false;
    }

    bool isBlank(wchar_t ch) const
    {
        if (ch == L' ' || ch == L'\t')
            return true;
        return false;
    }

public:

    enum
    {
        builtinSpace = 0,
        builtinBlank = 1,
        builtinAlpha = 2,
        builtinDigit = 3,
        builtinUpper = 4,
        builtinLower = 5,
        builtinAlnum = 6,
        builtinXdigit = 7,
        builtinPrint = 8,
        builtinPunct = 9,
        builtinGraph = 10,
        builtinCntrl = 11,
        builtinWord = 12,
        builtinAscii = 13,
        builtinInverseWord = 14,
        builtinInverseDigit = 15,
        builtinInverseSpace = 16
    };

public:

    bool negated;
    bool states1[256];              // states for characters 0..255
    bool builtin[20];
    std::set<unsigned int> states2; // states for characters 256..2^32
};

template <class charT>
class reg_backref : public regpart<charT>
{
public:

    reg_backref(regpart<charT>* parent) : regpart<charT>(parent)
    {
        match_idx = -1;
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        bool used = false;
        const charT* m_it = NULL;
        const charT* m_end = NULL;

        if (match_idx != -1)
        {
            const sub_match<charT>& s = matchres.getSubMatch(match_idx);
            m_it = s.first;
            m_end = s.second;
            used = (s.temp_first != 0) ? true : false;
        }
         else
        {
            const sub_match<charT>& s = matchres.getSubMatch(match_tag);
            m_it = s.first;
            m_end = s.second;
            used = (s.temp_first != 0) ? true : false;
        }

        if (!used)
        {
            return matchFailed;
        }

        if (m_it == NULL)
        {
            // an empty backref will match anything
            *next = start;
            return matchSucceeded;
        }


        const charT* s_it = start;
        const charT* snext = start;

        // if the match string is longer than the input
        // string, there is no match; stop here

        if (m_end - m_it > end - start)
            return matchFailed;

        while (*m_it && m_it < m_end)
        {
            if (*m_it != *s_it)
                return matchFailed;

            ++m_it;
            ++s_it;

            ++snext;
        }

        *next = snext;

        return matchSucceeded;
    }
                
public:

    int match_idx;
    std::basic_string<charT> match_tag;
};


template <class charT>
class reg_lookahead : public regpart<charT>
{
public:

    reg_lookahead(regpart<charT>* parent) : regpart<charT>(parent)
    {
        part = NULL;
        negate = false;
        regpart<charT>::m_look_around = true;
    }

    ~reg_lookahead()
    {
        delete part;
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        const charT* snext;
        int part_res = part->matchInternal(start, end, &snext, matchres);
        int res = matchFailed;

        if (part_res == matchSucceeded && !negate)
            res = matchSucceeded;
        if (part_res == matchFailed && negate)
            res = matchSucceeded;

        if (res == matchSucceeded)
            *next = start;

        return res;
    }
              
    void setNextPart(regpart<charT>* next_part)
    {
        regpart<charT>::m_next_part = next_part;
        //part->setNextPart(next_part);
    }
      
public:

    regpart<charT>* part;
    bool negate;
};

template <class charT>
class reg_lookbehind : public regpart<charT>
{
public:

    reg_lookbehind(regpart<charT>* parent) : regpart<charT>(parent)
    {
        part = NULL;
        negate = false;
        regpart<charT>::m_look_around = true;
    }

    ~reg_lookbehind()
    {
        delete part;
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        const charT* snext;
        const charT* p;
        int r, part_res;


        part_res = matchFailed;
        for (p = start-1; p >= matchres.src_begin; --p)
        {
            r = part->matchInternal(p, start, &snext, matchres);
            if (r == matchSucceeded)
            {
                if (snext == start)
                {
                    part_res = matchSucceeded;
                    break;
                }
            }
        }


        int res = matchFailed;

        if (part_res == matchSucceeded && !negate)
            res = matchSucceeded;
        if (part_res == matchFailed && negate)
            res = matchSucceeded;

        if (res == matchSucceeded)
            *next = start;

        return res;
    }
                
public:

    regpart<charT>* part;
    bool negate;
};

template <class charT>
class reg_conditional : public regpart<charT>
{
public:

    reg_conditional(regpart<charT>* parent) : regpart<charT>(parent)
    {
        cond_regpart = NULL;
        then_part = NULL;
        else_part = NULL;
        cond_match_idx = -1;
    }

    ~reg_conditional()
    {
        delete cond_regpart;
        delete then_part;
        delete else_part;
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        bool condition_result = false;

        // evaluate the condition
        if (cond_regpart)
        {
            const charT* snext;
            if (matchFailed != cond_regpart->matchInternal(start, end, 
                                                           &snext, matchres))
                condition_result = true;
        }
         else if (cond_match_idx != -1)
        {
            const sub_match<charT>& s = matchres.getSubMatch(cond_match_idx);
            if (s.second)
                condition_result = true;
        }
         else if (cond_match_tag.length() > 0)
        {
            const sub_match<charT>& s = matchres.getSubMatch(cond_match_tag);
            if (s.second)
                condition_result = true;
        }
         else
        {
            return matchFailed;
        }


        // evaluate the appropriate branch
        if (condition_result)
        {
            return then_part->matchInternal(start, end, next, matchres);
        }
         else
        {
            if (else_part)
            {
                return else_part->matchInternal(start, end, next, matchres);
            }
             else
            {
                *next = start;
                return matchSucceeded;
            }
        }
            
        return matchFailed;
    }
              
    void setNextPart(regpart<charT>* next_part)
    {
        regpart<charT>::m_next_part = next_part;
        if (cond_regpart)
            cond_regpart->setNextPart(next_part);
        if (then_part)
            then_part->setNextPart(next_part);
        if (else_part)
            else_part->setNextPart(next_part);
    }
      
public:

    int cond_match_idx;
    std::basic_string<charT> cond_match_tag;
    regpart<charT>* cond_regpart;

    regpart<charT>* then_part;
    regpart<charT>* else_part;
};





template <class charT>
class reg_sequence : public regpart<charT>
{
public:

    reg_sequence(regpart<charT>* parent) : regpart<charT>(parent)
    {
        m_match_idx = -1;
    }

    ~reg_sequence()
    {
        size_t i, part_count;
        for (i = 0, part_count = m_parts.size(); i < part_count; ++i)
            delete m_parts[i];
    }

    int tryRest(const charT* start,
                const charT* end,
                const charT** next,
                match_results<charT>& matchres)
    {
        reg_stack_popper<charT> st(matchres);


        // unwind: see comment below and in doMatch()
        if (matchres.p_unwind)
        {
            *next = matchres.p_unwind;
            return matchSucceeded;
        }


        // figure out where to pick up from
        int pos = matchres.stack.top() + 1;


        const charT* snext;
        {
            reg_stack_popper<charT> st(matchres);
            if (doMatch(pos, start, end, &snext, matchres) == matchFailed)
                return matchFailed;
        }


        // unwind: see comment below and in doMatch()
        if (matchres.p_unwind)
        {
            *next = matchres.p_unwind;
            return matchSucceeded;
        }


        if (regpart<charT>::m_parent)
            return regpart<charT>::m_parent->tryRest(snext, end, next, matchres);

        // if the look ahead succeeded in matching the whole
        // expression, we will set this variable.  This will cause
        // regex::doMatch to unwind the call stack and return success
        if (!regpart<charT>::m_look_around)
            matchres.p_unwind = snext;

        *next = snext;
        return matchSucceeded;
    }


    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        return doMatch(0, start, end, next, matchres);
    }

    int doMatch(int part_idx,
                const charT* start,
                const charT* end,
                const charT** next,
                match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        size_t i, part_count;
        const charT* scur = start;
        const charT* snext;

        int res = matchSucceeded;

        for (i = part_idx, part_count = m_parts.size(); i < part_count; ++i)
        {
            // save the current value item we are working
            // on so that tryRest() knows where to pick up processing
            rs.setValue((int)i);
            
            res = m_parts[i]->matchInternal(scur, end, &snext, matchres);

            if (res == matchFailed)
                break;

            // if the look ahead succeeded in matching the whole
            // expression, tryRest() will have set this variable. We will
            // unwind the call stack and return success

            if (matchres.p_unwind)
            {
                *next = matchres.p_unwind;
                return matchSucceeded;
            }

            scur = snext;
        }

        if (res == matchSucceeded)
            *next = scur;

        return res;
    }

    void setNextPart(regpart<charT>* part)
    {
        regpart<charT>::m_next_part = part;

        if (m_parts.size() > 0)
        {
            m_parts[m_parts.size() - 1]->setNextPart(part);
        }
    }

public:

    std::vector<regpart<charT>*> m_parts;

    // back references
    int m_match_idx;
    std::basic_string<charT> m_match_tag;
};

template <class charT>
class reg_exprbegin : public regpart<charT>
{
public:

    reg_exprbegin(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        reg_sequence<charT>* parent = (reg_sequence<charT>*)regpart<charT>::m_parent;

        if (parent->m_match_idx != -1)
        {
            sub_match<charT>& s = matchres.getSubMatch(parent->m_match_idx);
            s.temp_first = start;
        }
         else if (parent->m_match_tag.length() > 0)
        {
            sub_match<charT>& s = matchres.getSubMatch(parent->m_match_tag);
            s.temp_first = start;
        }

        *next = start;
        return matchSucceeded;
    }
};

template <class charT>
class reg_exprend : public regpart<charT>
{
public:

    reg_exprend(regpart<charT>* parent) : regpart<charT>(parent)
    {
    }

    int matchInternal(const charT* start,
                      const charT* end,
                      const charT** next,
                      match_results<charT>& matchres)
    {
        reg_match_scope<charT> rs(this, start, matchres);

        reg_sequence<charT>* parent = (reg_sequence<charT>*)regpart<charT>::m_parent;

        if (parent->m_match_idx != -1)
        {
            sub_match<charT>& s = matchres.getSubMatch(parent->m_match_idx);
            s.first = s.temp_first;
            s.second = start;
        }
         else if (parent->m_match_tag.length() > 0)
        {
            sub_match<charT>& s = matchres.getSubMatch(parent->m_match_tag);
            s.first = s.temp_first;
            s.second = start;
        }

        *next = start;
        return matchSucceeded;
    }
};



template <class charT>
class basic_regex
{
public:

    basic_regex()
    {
        m_seq = NULL;
    }

    basic_regex(const charT* regex)
    {
        m_seq = NULL;
        assign(regex);
    }
    
    basic_regex(const std::basic_string<charT>& regex)
    {
        m_seq = NULL;
        assign(regex);
    }
    
    ~basic_regex()
    {
        delete m_seq;
    }


    bool assign(const charT* expr)
    {
        return assign(expr, expr + quickStrlen(expr));
    }

    bool assign(const std::basic_string<charT>& str)
    {
        const charT* src = str.c_str();
        return assign(src, src + str.length());
    }

    bool assign(const charT* expr,
                const charT* end)
    {
        if (end == NULL)
        {
            end = expr + quickStrlen(expr);
        }

        regpart<charT>* r = parseInternal(NULL, expr, end);

        if (!r)
            return false;

        delete m_seq;
        m_seq = r;

        return true;
    }


    bool empty() const
    {
        return (m_seq == NULL) ? true : false;
    }
    

    bool search(const std::basic_string<charT>& str) const
    {
        match_results<charT> matchres;
        const charT* ch = str.c_str();
        return search(ch, ch + str.length(), matchres);
    }
    
    bool search(const std::basic_string<charT>& str,
                match_results<charT>& matchres) const
    {
        const charT* ch = str.c_str();
        return search(ch, ch + str.length(), matchres);
    }
    
    bool search(const charT* str) const
    {
        match_results<charT> matchres;
        return search(str, str + quickStrlen(str), matchres);
    }
    
    bool search(const charT* str,
                match_results<charT>& matchres) const
    {
        return search(str, str + quickStrlen(str), matchres);
    }

    bool search(const charT* start,
                const charT* end) const
    {
        match_results<charT> matchres;
        return search(start, end, matchres);
    }

    bool search(const charT* start,
                const charT* end,
                match_results<charT>& matchres) const
    {
        const charT* next = NULL;

        matchres.src_begin = start;
        matchres.src_end = end;

        while (start <= end)
        {
            if (matchInternal(start, end, &next, matchres))
                return true;

            ++start;
        }

        return false;
    }


    bool match(const charT* str) const
    {
        match_results<charT> matchres;

        matchres.src_begin = str;
        matchres.src_end = str + quickStrlen(str);

        const charT* next = NULL;
        return matchInternal(str, matchres.src_end, &next, matchres);
    }


    bool match(const charT* start,
               const charT* end) const
    {
        match_results<charT> matchres;

        if (!end)
            end = start + quickStrlen(start);

        matchres.src_begin = start;
        matchres.src_end = end;

        const charT* next = NULL;
        return matchInternal(start, end, &next, matchres);
    }


    bool match(const charT* start,
               match_results<charT>& matchres) const
    {        
        matchres.src_begin = start;
        matchres.src_end = start + quickStrlen(start);

        const charT* next = NULL;
        return matchInternal(start, matchres.src_end, &next, matchres);
    }

    bool match(const charT* start,
               const charT* end,
               match_results<charT>& matchres) const
    {
        if (!end)
            end = start + quickStrlen(start);
        
        matchres.src_begin = start;
        matchres.src_end = end;

        const charT* next = NULL;
        return matchInternal(start, end, &next, matchres);
    }



    int replace(std::basic_string<charT>& str,
                const std::basic_string<charT>& replace_fmt,
                bool replace_all = true) const
    {
        const charT* start;
        const charT* end;
        int offset, next_offset = 0;
        match_results<charT> matchres;
        std::basic_string<charT> replace_str;

        int num_replaces = 0;

        while (1)
        {
            offset = next_offset;

            start = str.c_str() + offset;
            end = str.c_str() + str.length();

            if (!search(start, end, matchres))
                return num_replaces;

            const sub_match<charT>& result = matchres[0];
            if (!result.isValid())
                return num_replaces;

            if (result.length() == 0)
                return num_replaces;

            num_replaces++;

            replace_str = matchres.format(replace_fmt);

            next_offset += (int)((result.second - start) - (result.length() - replace_str.length()));

            str.replace(result.first - start + offset,
                        result.length(),
                        replace_str);
                        
            if (!replace_all)
                return num_replaces;
        }

        return 0;
    }


private:


    bool matchInternal(const charT* start,
                       const charT* end,
                       const charT** next,
                       match_results<charT>& matchres) const
    {
        if (empty())
            return matchFailed;

        matchres.clear();
        matchres.stack.reserve(4);
        matchres.partdata.resize(m_seq->m_allparts_count + 1);

        if (m_seq->matchInternal(start, end, next, matchres) == matchSucceeded)
            return true;

        return false;
    }

    reg_sequence<charT>* parseInternal(regpart<charT>* seq_parent,
                                       const charT* expr,
                                       const charT* end)
    {
        reg_sequence<charT>* seq = new reg_sequence<charT>(seq_parent);

        if (seq_parent == NULL)
            seq->m_match_idx = 0;


        // first, look for any "or"s
        const charT* or_pos = zl_strchr(expr, end, L'|', L"([", L")]");
        if (or_pos)
        {
            reg_or<charT>* ror = new reg_or<charT>(seq);

            const charT* start = expr;
            // this must be '<=' (and not '<') to support "A|"
            while (start <= end)
            {
                or_pos = zl_strchr(start, end, L'|', L"({[", L")}]");
                if (!or_pos)
                    or_pos = end;

                regpart<charT>* rp = parseInternal(ror, start, or_pos);
                if (!rp)
                {
                    delete ror;
                    return NULL;
                }

                ror->parts.push_back(rp);

                start = or_pos+1;
            }



            reg_exprbegin<charT>* expr_begin = new reg_exprbegin<charT>(seq);
            expr_begin->setNextPart(ror);
            seq->m_parts.push_back(expr_begin);

            seq->m_parts.push_back(ror);

            reg_exprend<charT>* expr_end = new reg_exprend<charT>(seq);
            ror->setNextPart(expr_end);
            seq->m_parts.push_back(expr_end);

            return seq;
        }



        // add begin expression term (used for recording match indexes)

        //if (m_match_idx != -1 || m_match_tag.length() > 0)
        {
            reg_exprbegin<charT>* expr_begin = new reg_exprbegin<charT>(seq);
            seq->m_parts.push_back(expr_begin);
        }


        // parse terms, one by one


        int match_idx = 1;    // used for enumerating backrefs in the top level
        const charT* term_start;
        const charT* next = expr;
        bool error;

        while (1)
        {
            term_start = next;

            if (!getNextTerm(term_start, end, &next, &error))
            {
                // if there is a parse error, return --
                if (error)
                    return NULL;

                break;
            }



            std::basic_string<charT> term(term_start, next-term_start);
            std::basic_string<charT> oper;


            regpart<charT>* part = NULL;


            if (term[0] == L'(')
            {
                if (term.length() > 2 && term[1] == L'?' &&
                    (term[2] == L'=' || term[2] == L'!'))
                {
                    // lookahead subexpression
                    reg_lookahead<charT>* look = new reg_lookahead<charT>(seq);
                    look->negate = (term[2] == L'!' ? true:false);

                    regpart<charT>* r = parseInternal(look, term_start+3, next-1);
                    if (!r)
                    {
                        delete seq;
                        delete look;
                        return NULL;
                    }

                    // will prevent tryRest() from going too far
                    look->m_parent = NULL;

                    look->part = r;
                    part = look;
                }
                 else
                if (term.length() > 3 && term[1] == L'?' && term[2] == L'<' &&
                    (term[3] == L'=' || term[3] == L'!'))
                {
                    // lookbehind subexpression
                    reg_lookbehind<charT>* look = new reg_lookbehind<charT>(seq);
                    look->negate = (term[3] == L'!' ? true:false);

                    regpart<charT>* r = parseInternal(look, term_start+4, next-1);
                    if (!r)
                    {
                        delete seq;
                        delete look;
                        return NULL;
                    }

                    // will prevent tryRest() from going too far
                    look->m_parent = NULL;

                    look->part = r;
                    part = look;
                }
                 else
                {
                    const charT* subexpr_begin = term_start+1;
                    const charT* subexpr_end = next-1;

                    const charT* matchname_begin = NULL;
                    const charT* matchname_end = NULL;
                    
                    // normal subexpression
                    bool backref = false;
                    int ignore_case = -1;
                    int single_line = -1;
                    int multi_line = -1;

                    if (seq_parent == NULL)
                        backref = true;

                    if (term.length() > 2 && term[1] == L'?' &&
                        (term[2] == L'<' || term[2] == L'\''))
                    {
                        char close = '>';
                        if (term[2] == L'\'')
                            close = '\'';
                        matchname_begin = term_start+3;
                        matchname_end = quickStrchr<charT>(matchname_begin, close);
                        if (!matchname_end || matchname_end-matchname_begin < 1)
                            return NULL;
                        subexpr_begin = matchname_end+1;
                        backref = true;
                    }
                     else if (term.length() > 2 && term[1] == L'?')
                    {
                        // this may be a modifier, a modifier span, or a
                        // conditional; we need to find out which

                        // 0 = modifier
                        // 1 = modifier span
                        // 2 = conditional
                        int type = 0;

                        const charT* p;
                        const charT* modifier_end = NULL;

                        for (p = term_start+2; p < next; ++p)
                        {
                            if (0 == quickStrchr<wchar_t>(L"ismn-", *p))
                            {
                                modifier_end = p;

                                if (*p == ')')
                                    type = 0;
                                 else if (*p == ':')
                                    type = 1;
                                 else
                                    type = 2;
                                break;
                            }
                        }

                        if (!modifier_end)
                            return NULL;

                        if (type == 0 || type == 1)
                        {
                            bool negate = false;

                            for (p = term_start+2; p < modifier_end; ++p)
                            {
                                if (*p == L'-')
                                    negate = true;
                                if (*p == L'i')
                                    ignore_case = (negate ? 0 : 1);
                                if (*p == L's')
                                    single_line = (negate ? 0 : 1);
                                if (*p == L'm')
                                    multi_line = (negate ? 0 : 1);
                            }

                            if (type == 0)
                            {
                                if (ignore_case != -1)
                                    seq->m_ignore_case = (ignore_case != 0) ? true : false;
                                continue;
                            }
                             else
                            {
                                subexpr_begin = modifier_end+1;
                                backref = false;
                            }
                        }
                         else
                        {
                            part = parseConditional(seq, term_start, next);
                            if (!part)
                                return NULL;

                            if (seq->m_parts.size() > 0)
                                seq->m_parts[seq->m_parts.size() - 1]->setNextPart(part);

                            seq->m_parts.push_back(part);
                            continue;
                        }
                    }

                    
                    // set the modifier settings
                    bool save_ignore_case = seq->m_ignore_case;
                    if (ignore_case != -1)
                        seq->m_ignore_case = (ignore_case != 0) ? true : false;

                    bool save_multi_line = seq->m_multi_line;
                    if (multi_line != -1)
                        seq->m_multi_line = (multi_line != 0) ? true : false;

                    bool save_single_line = seq->m_single_line;
                    if (single_line != -1)
                        seq->m_single_line = (single_line != 0) ? true : false;
                    

                    reg_sequence<charT>* r = parseInternal(seq, subexpr_begin, subexpr_end);
                    if (!r)
                    {
                        delete seq;
                        return NULL;
                    }

                    // restore the modifier settings to their previous state
                    seq->m_ignore_case = save_ignore_case;
                    seq->m_multi_line = save_multi_line;
                    seq->m_single_line = save_single_line;



                    if (backref)
                    {
                        if (matchname_begin)
                        {
                            if (iswdigit(*matchname_begin))
                            {
                                std::basic_string<charT> s(matchname_begin, matchname_end);
                                r->m_match_idx = quickAtoi(s.c_str());
                            }
                             else
                            {
                                r->m_match_tag.assign(matchname_begin,
                                                      matchname_end);
                            }
                        }
                         else
                        {
                            r->m_match_idx = match_idx++;
                        }
                    }

                    part = r;
                }
            }
             else if (term[0] == L'[')
            {
                part = parseCharClass(seq, term.c_str());
                if (part == NULL)
                    return NULL;
            }
             else if (term.length() == 1 && term[0] == L'.')
            {
                reg_period<charT>* r = new reg_period<charT>(seq);
                part = r;
            }
             else if (term.length() == 1 && term[0] == L'^')
            {
                reg_linebegin<charT>* r = new reg_linebegin<charT>(seq);
                part = r;
            }
             else if (term.length() == 1 && term[0] == L'$')
            {
                reg_lineend<charT>* r = new reg_lineend<charT>(seq);
                part = r;
            }
             else if (term[0] == L'\\')
            {
                part = parseEscapeSequence(seq, term.c_str());
                if (part == NULL)
                    return NULL;
            }
             else if (quickStrchr<wchar_t>(L"\\^$|.?*+()[", term[0]))
            {
                // operators, etc, don't belong as the first char --
                return NULL;
            }
             else if (term.length() > 0)
            {
                reg_literal<charT>* r = new reg_literal<charT>(seq);
                part = r;
                r->match_str = term;
            }


            const charT* oper_start = next;
            const charT* oper_end = NULL;
            if (*oper_start && quickStrchr<wchar_t>(L"+*?{", *oper_start))
            {
                if (getNextTerm(oper_start, end, &oper_end, &error))
                {
                    oper.assign(oper_start, oper_end-oper_start);
                }
                
                if (error)
                {
                    // return parse error
                    return NULL;
                }
            }


            if (oper.length() > 0)
            {
                bool greedy = true;

                if (oper.length() > 1 && oper[oper.length()-1] == L'?')
                    greedy = false;

                int min_matches = -1;
                int max_matches = -1;

                switch (oper[0])
                {
                    case L'*':
                        min_matches = 0;
                        max_matches = reg_minmax<charT>::match_limit;
                        next = oper_end;
                        break;
                    case L'+':
                        min_matches = 1;
                        max_matches = reg_minmax<charT>::match_limit;
                        next = oper_end;
                        break;
                    case L'?':
                        min_matches = 0;
                        max_matches = 1;
                        next = oper_end;
                        break;
                    case L'{':
                    {
                        min_matches = quickAtoi(oper.c_str()+1);
                        max_matches = min_matches;
                        size_t comma_pos = oper.find(L',');
                        if (comma_pos != oper.npos)
                        {
                            max_matches = quickAtoi(oper.c_str()+comma_pos+1);
                            
                            // now handle the "{x,}" case
                            if (max_matches == 0 && oper[comma_pos+1] != L'0')
                                max_matches = reg_minmax<charT>::match_limit;
                        }
                        if (min_matches > 0 || max_matches > 0)
                            next = oper_end;
                        break;
                    }
                    default:
                        return NULL;
                }


                if (min_matches != -1)
                {
                    // if the minmax is anything besides a {1} (which doesn't
                    // require a minmax), create the reg_minmax and attach
                    // the term part to it
                    if (!(min_matches == 1 && max_matches == 1))
                    {
                        reg_minmax<charT>* r = new reg_minmax<charT>(seq);
                        r->greedy = greedy;
                        r->min_matches = min_matches;
                        r->max_matches = max_matches;
                        r->part = part;
                        part->m_parent = r;
                        part = r;
                    }
                }
            }


            // finally, add the part
            if (seq->m_parts.size() > 0)
                seq->m_parts[seq->m_parts.size() - 1]->setNextPart(part);

            seq->m_parts.push_back(part);
        }

        // add end expression term (used for recording match indexes)

        //if (m_match_idx != -1 || m_match_tag.length() > 0)
        {
            reg_exprend<charT>* expr_end = new reg_exprend<charT>(seq);
            seq->m_parts[seq->m_parts.size()-1]->setNextPart(expr_end);
            seq->m_parts.push_back(expr_end);
        }

        return seq;
    }


    regpart<charT>* parseConditional(regpart<charT>* parent,
                                     const charT* start,
                                     const charT* end)
    {
        const charT* p = start;

        // skip past '(?'
        p += 2;

        // ignore close paren
        --end;

        // if there is no open parenthesis at the beginning,
        // we need to fail as this is required syntax
        if (*p != L'(')
            return NULL;

        reg_conditional<charT>* r = new reg_conditional<charT>(parent);
        const charT* close_paren;

        close_paren = zl_strchr(p, end, L')', L"([", L")]");
        if (!close_paren)
        {
            delete r;
            return NULL;
        }

        // determine what type (lookaround, match idx, or match tag)
        if (p+1 < end && *(p+1) == L'?')
        {
            // it's a conditional based on a look-around
            r->cond_regpart = parseInternal(r, p, close_paren+1);
            if (!r->cond_regpart)
            {
                delete r;
                return NULL;
            }
        }
         else if (p+1 < end && isdigit(*(p+1)))
        {
            // it's a conditional based on a numbered backref
            r->cond_match_idx = quickAtoi(p+1);
        }
         else
        {
            // it's a conditional based on a named backref
            r->cond_match_tag.assign(p+1, close_paren);
        }



        const charT* or_pos = zl_strchr(close_paren+1, end, '|', L"([", L")]");
        
        r->then_part = parseInternal(r, close_paren+1, or_pos ? or_pos : end);
        if (!r->then_part)
        {
            delete r;
            return NULL;
        }

        if (or_pos)
        {
            r->else_part = parseInternal(r, or_pos+1, end);
            if (!r->else_part)
            {
                delete r;
                return NULL;
            }
        }

        return r;
    }


    bool getNextTerm(const charT* start,
                     const charT* end,
                     const charT** next,
                     bool* error)
    {
        *error = false;

        if (!*start || start >= end)
        {
            *next = start;
            return false;
        }

        const charT* p = start;

        // look for an operator
        switch (*p)
        {
            case L'|':
            case L'$':
            case L'^':
                *next = start+1;
                return true;
            case L'\\':
            {
                int i;

                switch (*(start+1))
                {
                    case 0:
                        *next = start+1;
                        break;
                    case L'x':
                        *next = start+2;
                        for (i = 0; i < 2; ++i)
                        {
                            if (*(start+i+2))
                                ++(*next);
                        }
                        break;
                    case L'u':
                        *next = start+2;
                        for (i = 0; i < 4; ++i)
                        {
                            if (*(start+i+2))
                                ++(*next);
                        }
                        break;
                    case L'P':
                    case L'p':
                        if (*(start+2) && *(start+2) == L'{')
                        {
                            const charT* p = quickStrchr<charT>(start, L'}');
                            if (!*p)
                                return false;
                            *next = p+1;
                            break;
                        }
                    case L'k':
                        if (*(start+2) &&
                           (*(start+2) == L'<' || *(start+2) == L'\''))
                        {
                            char close = '>';
                            if (*(start+2) == L'\'')
                                close = L'\'';

                            const charT* p = quickStrchr<charT>(start, close);
                            if (!*p)
                                return false;
                            *next = p+1;
                            break;
                        }

                    default:

                        // look for octal escape code or backref > 9
                        if (iswdigit(*(start+1)))
                        {
                            const charT* p = start+1;
                            size_t len;

                            // find out how many digits are after the '\'
                            while (iswdigit(*p) && p < end)
                                ++p;
                            len = p-start-1;

                            if (len > 3)
                                len = 3;
                            *next = start+len+1;
                            break;
                        }

                        *next = start+2;
                        break;
                }

                return true;
            }
            case L'.':
                *next = start+1;
                return true;
            case L'*':
            case L'?':
            case L'+':
                *next = start+1;
                if (**next == L'?')
                    ++(*next);
                return true;
            default:
                break;
        }

        // look for a group

        if (quickStrchr<wchar_t>(L"({[", *p))
        {
            switch (*p)
            {
                case L'(':
                    *next = zl_strchr(start, end, L')', L"([", L")]");
                    break;
                case L'[':
                    *next = zl_closebracket(start, end);
                    break;
                case L'{':
                    *next = zl_closebrace(start, end);
                    if (!*next)
                    {
                        //just return '{'
                        *next = start+1;
                        return true;
                    }
                    if (*next && **next != L'}')
                        return true;
                    if (*next && *(*next+1) == L'?')
                    {
                        // { } with lazy '?'
                        ++(*next);
                    }
                    break;
            }

            if (!*next)
            {
                *error = true;
                return false;
            }

            ++(*next);
            return true;
        }

        // else, look for a literal

        while (0 == quickStrchr<wchar_t>(L"\\^$|.?*+({[", *p))
        {
            if (!*p || p >= end)
                break;
            ++p;
        }

        // if there is a single char before an operator, don't include it

        if (*p && p-start > 1 && quickStrchr<wchar_t>(L"?*+{", *p))
            --p;

        *next = p;

        return true;
    }

    const charT* zl_strchr(const charT* str,
                           const charT* end,
                           charT ch,
                           const wchar_t* open_parens,
                           const wchar_t* close_parens)
    {
        const wchar_t* lch;
        int i, levels[10];
        for (i = 0; i < 10; ++i)
            levels[i] = 0;

        // this is an exception to allow "[[]" or "[]]"
        if (ch == L']' && *str == L'[')
        {
            const charT* p = str+1;
            if (*p == L'^')
                ++p;
            if (*p == L'[')
            {
                if (*(p+1) != L':')
                {
                    lch = quickStrchr<wchar_t>(close_parens, ch);
                    if (lch)
                        --levels[lch-close_parens];
                }
            }
             else if (*p == L']')
            {
                lch = quickStrchr<wchar_t>(close_parens, ch);
                if (lch)
                    ++levels[lch-close_parens];
            }
        }

        while (*str && str < end)
        {
            if (quickStrchr<wchar_t>(L"\\", *str))
            {
                str += 2;
                continue;
            }

            lch = quickStrchr<wchar_t>(open_parens, *str);
            if (lch)
            {
                levels[lch-open_parens]++;
            }
             else
            {
                lch = quickStrchr<wchar_t>(close_parens, *str);
                if (lch)
                {
                    levels[lch-close_parens]--;
                }
            }

            
            bool top_level = true;
            for (i = 0; i < 10; ++i)
            {
                if (levels[i] > 0)
                {
                    top_level = false;
                    break;
                }
            }
            
            
            if (top_level && *str == ch)
                return str;

            ++str;
        }

        return NULL;
    }


    const charT* zl_closebracket(const charT* start,
                                   const charT* end)
    {
        // this is an exception to allow "[[]" or "[]]"
        const charT* p = start+1;
        if (*p == L']')
            ++p;

        bool in_seq = false;

        while (*p && p < end)
        {
            if (p > start && p+1 < end && *p == '[' && *(p+1) == L':')
                in_seq = true;

            if (in_seq && *p == L']')
            {
                in_seq = false;
                ++p;
                continue;
            }
                    
            if (*p == L']' && !in_seq)
                return p;

            if (*p == L'\\')
            {
                p += 2;
                continue;
            }

            ++p;
        }

        return NULL;
    }

    const charT* zl_closebrace(const charT* start,
                                 const charT* end)
    {
        const charT* p;
        p = start+1;
        
        while (p < end)
        {
            if (*p != L',' && !::iswdigit(*p))
                return p;
            ++p;
        }

        // } not found
        return NULL;
    }


    regpart<charT>* parseCharClass(regpart<charT>* parent, 
                                   const charT* s)
    {
        const charT* begin = s;
        const charT* next;
        wchar_t ch;

        if (*s != L'[')
            return NULL;
        ++s;

        reg_charclass<charT>* r = new reg_charclass<charT>(parent);

        // check for charset negation
        r->negated = false;

        if (*s == L'^')
        {
            r->negated = true;
            ++s;
        }

        // add stuff to the charset

        while (*s)
        {
            // look for end bracket

            if (*s == ']')
            {
                if (s-1 == begin)
                {
                    r->setState(*s);
                    ++s;
                    continue;
                }
                 else
                {
                    break;
                }
            }


            // check for posix character class

            if (*s == L'[' && *(s+1) == L':')
            {
                if (0 == quickStrncmp(s, L"[:alpha:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinAlpha);
                else if (0 == quickStrncmp(s, L"[:upper:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinUpper);
                else if (0 == quickStrncmp(s, L"[:lower:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinLower);
                else if (0 == quickStrncmp(s, L"[:digit:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinDigit);
                else if (0 == quickStrncmp(s, L"[:alnum:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinAlnum);
                else if (0 == quickStrncmp(s, L"[:xdigit:]", 10))
                    r->setBuiltIn(reg_charclass<charT>::builtinXdigit);
                else if (0 == quickStrncmp(s, L"[:space:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinSpace);
                else if (0 == quickStrncmp(s, L"[:print:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinPrint);
                else if (0 == quickStrncmp(s, L"[:punct:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinPunct);
                else if (0 == quickStrncmp(s, L"[:graph:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinGraph);
                else if (0 == quickStrncmp(s, L"[:cntrl:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinCntrl);
                else if (0 == quickStrncmp(s, L"[:blank:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinBlank);
                else if (0 == quickStrncmp(s, L"[:word:]", 8))
                    r->setBuiltIn(reg_charclass<charT>::builtinWord);
                else if (0 == quickStrncmp(s, L"[:ascii:]", 9))
                    r->setBuiltIn(reg_charclass<charT>::builtinAscii);
                else
                {
                   delete r;
                   return NULL;
                }

                s = quickStrchr<charT>(s, L']')+1;
                continue;
            }

            // check for backslash

            if (*s == L'\\')
            {
                switch (*(s+1))
                {
                    case L's':
                        r->setBuiltIn(reg_charclass<charT>::builtinSpace);
                        s += 2;
                        continue;
                    case L'd':
                        r->setBuiltIn(reg_charclass<charT>::builtinDigit);
                        s += 2;
                        continue;
                    case L'w':
                        r->setBuiltIn(reg_charclass<charT>::builtinWord);
                        s += 2;
                        continue;
                    case L'S':
                        r->setBuiltIn(reg_charclass<charT>::builtinInverseSpace);
                        s += 2;
                        continue;
                    case L'D':
                        r->setBuiltIn(reg_charclass<charT>::builtinInverseDigit);
                        s += 2;
                        continue;
                    case L'W':
                        r->setBuiltIn(reg_charclass<charT>::builtinInverseWord);
                        s += 2;
                        continue;
                    default:
                        break;
                }
            }

            // get next character
            ch = parseEscapeCode(s, &next);
            if (next == s)
            {
                // failure
                delete r;
                return NULL;
            }

            // check for hyphen
            if (*next == L'-' && *(next+1) != L']')
            {
                const charT* next2;
                wchar_t ch2 = parseEscapeCode(next+1, &next2);
                if (next2 == next+1 || ch2 < ch)
                {
                    // failure
                    delete r;
                    return NULL;
                }
                
                for (wchar_t i = ch; i <= ch2; ++i)
                    r->setState(i);

                s = next2;
                continue;
            }

            r->setState(ch);
            s = next;
        }

        return r;
    }


    regpart<charT>* parseEscapeSequence(regpart<charT>* parent,
                                        const charT* s)
    {
        size_t len = quickStrlen(s);

        if (len < 2)
        {
            // lone backslash, which is a problem
            return NULL;
        }
        
        if (s[1] == '0' || (iswdigit(s[1]) && len == 4))
        {
            // octal character
            const charT* next;
            wchar_t ch = parseEscapeCode(s, &next);
            if (s == next)
                return NULL;

            reg_literal<charT>* r = new reg_literal<charT>(parent);
            r->match_str = (charT)ch;
            return r;
        }
         else if (s[1] >= L'1' && s[1] <= L'9')
        {
            reg_backref<charT>* r = new reg_backref<charT>(parent);
            r->match_idx = quickAtoi(s+1);
            return r;
        }
         else if (s[1] == L'd')
        {
            reg_charclass<charT>* r = new reg_charclass<charT>(parent);
            r->setBuiltIn(reg_charclass<charT>::builtinDigit);
            r->negated = false;
            return r;
        }
         else if (s[1] == L'w')
        {
            reg_charclass<charT>* r = new reg_charclass<charT>(parent);
            r->setBuiltIn(reg_charclass<charT>::builtinWord);
            r->negated = false;
            return r;
        }
         else if (s[1] == L's')
        {
            reg_charclass<charT>* r = new reg_charclass<charT>(parent);
            r->setBuiltIn(reg_charclass<charT>::builtinSpace);
            r->negated = false;
            return r;
        }
         else if (s[1] == L'b')
        {
            reg_wordbreak<charT>* r = new reg_wordbreak<charT>(parent);
            r->negated = false;
            return r;
        }
         else if (s[1] == L'D')
        {
            reg_charclass<charT>* r = new reg_charclass<charT>(parent);
            r->setBuiltIn(reg_charclass<charT>::builtinDigit);
            r->negated = true;
            return r;
        }
         else if (s[1] == L'W')
        {
            reg_charclass<charT>* r = new reg_charclass<charT>(parent);
            r->setBuiltIn(reg_charclass<charT>::builtinWord);
            r->negated = true;
            return r;
        }
         else if (s[1] == L'S')
        {
            reg_charclass<charT>* r = new reg_charclass<charT>(parent);
            r->setBuiltIn(reg_charclass<charT>::builtinSpace);
            r->negated = true;
            return r;
        }
         else if (s[1] == L'B')
        {
            reg_wordbreak<charT>* r = new reg_wordbreak<charT>(parent);
            r->negated = true;
            return r;
        }
         else if (s[1] == L'k')
        {
            if (len < 5)
                return NULL;

            reg_backref<charT>* r = new reg_backref<charT>(parent);
            if (iswdigit(*(s+3)))
            {
                std::basic_string<charT> temps(s+3, len-4);
                r->match_idx = quickAtoi(temps.c_str());
            }
             else
            {
                r->match_tag.assign(s+3, len-4);
            }

            return r;
        }
         else if (len > 4 && (s[1] == L'p' || s[1] == L'P') && s[2] == L'{')
        {
            reg_charclass<charT>* r = new reg_charclass<charT>(parent);

            r->negated = false;
            if (s[1] == 'P')
                r->negated = true;
            
            const charT* name = s+3;
            if (0 == quickStrncmp(name, L"Is", 2))
                name += 2;

            if (0 == quickStrncmp(name, L"Alpha", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinAlpha);
            else if (0 == quickStrncmp(name, L"Upper", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinUpper);
            else if (0 == quickStrncmp(name, L"Lower", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinLower);
            else if (0 == quickStrncmp(name, L"Digit", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinDigit);
            else if (0 == quickStrncmp(name, L"Alnum", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinAlnum);
            else if (0 == quickStrncmp(name, L"XDigit", 6))
                r->setBuiltIn(reg_charclass<charT>::builtinXdigit);
            else if (0 == quickStrncmp(name, L"Space", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinSpace);
            else if (0 == quickStrncmp(name, L"Print", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinPrint);
            else if (0 == quickStrncmp(name, L"Punct", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinPunct);
            else if (0 == quickStrncmp(name, L"Graph", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinGraph);
            else if (0 == quickStrncmp(name, L"Cntrl", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinCntrl);
            else if (0 == quickStrncmp(name, L"Blank", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinBlank);
            else if (0 == quickStrncmp(name, L"Word", 4))
                r->setBuiltIn(reg_charclass<charT>::builtinWord);
            else if (0 == quickStrncmp(name, L"Ascii", 5))
                r->setBuiltIn(reg_charclass<charT>::builtinAscii);
            else if (0 == quickStrncmp(name, L"BasicLatin", 10))
                r->setBuiltIn(reg_charclass<charT>::builtinAscii);
            else if (0 == quickStrncmp(name, L"Lu", 2))
                r->setBuiltIn(reg_charclass<charT>::builtinUpper);
            else if (0 == quickStrncmp(name, L"Ll", 2))
                r->setBuiltIn(reg_charclass<charT>::builtinLower);
            else if (0 == quickStrncmp(name, L"L", 1))
                r->setBuiltIn(reg_charclass<charT>::builtinAlpha);
            else if (0 == quickStrncmp(name, L"N", 1))
                r->setBuiltIn(reg_charclass<charT>::builtinDigit);
            else
                {
                    delete r;
                    return NULL;
                }

            return r;
        }
         else
        {
            const charT* next;
            wchar_t ch = parseEscapeCode(s, &next);
            if (s == next)
                return NULL;


            reg_literal<charT>* r = new reg_literal<charT>(parent);
            r->match_str = (charT)ch;
            return r;
        }

        return NULL;
    }

    wchar_t parseEscapeCode(const charT* s, const charT** end)
    {
        static const wchar_t* hexchars = L"0123456789ABCDEF";

        if (!*s)
        {
            *end = s;
            return 0;
        }

        if (*s != '\\')
        {
            *end = s+1;
            return *s;
        }

        ++s;
        if (!*s)
            return 0;

        const wchar_t* res;
        wchar_t ch = 0;


        // look for octal escape sequences
        if (iswdigit(*s))
        {
            const charT* p = s;
            size_t len;

            // find out how many digits are after the '\'
            while (*p && iswdigit(*p))
                ++p;
            len = p-s;

            if (len >= 3 || *s == L'0')
            {
                // it is an octal escape sequence
                if (len > 3)
                    len = 3;

                unsigned int mul = 1;
                wchar_t res = 0;
                if (len == 2)
                    mul = 8;
                 else if (len == 3)
                    mul = 64;

                for (size_t i = 0; i < len; ++i)
                {
                    res += (*(s+i) - L'0') * mul;
                    mul /= 8;
                }

                *end = s+len;
                return res;
            }
        }


        switch (*s)
        {
            case L'x':
            {            
                ++s;
                res = quickStrchr<wchar_t>(hexchars, ::towupper(*s));
                if (!res || !*s)
                    return L'\0';
                ch += (wchar_t)((res-hexchars)*16);

                ++s;
                res = quickStrchr<wchar_t>(hexchars, ::towupper(*s));
                if (!res || !*s)
                    return L'\0';
                ch += (wchar_t)(res-hexchars);

                if (end)
                    *end = s+1;
            }
            return ch;

            case L'U':
            case L'u':
            {
                unsigned int i, len = 4, mul = 4096;

                if (*s == L'U')
                {
                    len = 8;
                    mul = 268435456;
                }

                ++s;
                for (i = 0; i < len; ++i)
                {
                    res = quickStrchr<wchar_t>(hexchars, ::towupper(*s));
                    if (!res || !*s)
                        return 0;
                    ++s;
                    ch += (wchar_t)((res-hexchars)*mul);
                    mul /= 16;
                }

                if (end)
                    *end = s+1;
            }
            return ch;

            case L'0': *end = s+1; return 0;
            case L'n': *end = s+1; return L'\n';
            case L'r': *end = s+1; return L'\r';
            case L't': *end = s+1; return L'\t';
            case L'f': *end = s+1; return L'\f';
            case L'v': *end = s+1; return L'\v';
            case L'a': *end = s+1; return L'\a';
            case L'b': *end = s+1; return L'\b';
            case L'e': *end = s+1; return 27;

            default:
                *end = s+1;
                return *s;
        }


        // never arrives here
        return 0;
    }



private:

    regpart<charT>* m_seq;
};



typedef basic_regex<char> regex;
typedef basic_regex<wchar_t> wregex;

typedef match_results<char> match;
typedef match_results<wchar_t> wmatch;

typedef sub_match<char> submatch;
typedef sub_match<wchar_t> wsubmatch;

}; // namespace klregex


#endif
