/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2004-01-26
 *
 */


#ifndef __XDCOMMON_KEYLAYOUT_H
#define __XDCOMMON_KEYLAYOUT_H


// -- key layout class --

class KeyPart
{
public:
    xd::objhandle_t handle;
    int type;
    int offset;
    int width;
    bool descending;
};

class KeyLayout
{
public:

    KeyLayout();
    ~KeyLayout();

    bool setKeyExpr(xd::IIteratorPtr iter,
                    const std::wstring& expr,
                    bool hold_ref = true);

    const unsigned char* getKey();
    const unsigned char* getKeyFromValues(const wchar_t* values[], size_t values_size);
    int getKeyLength();
    bool getTruncation();    // returns true if last call to
                             // getKey() had to truncate char data

    // -- manual functions --
    void setIterator(xd::IIteratorPtr iter,
                     bool hold_ref = true);

    bool addKeyPart(const std::wstring& expr,
                    int type = -1,
                    int expr_width = -1,
                    bool descending = false);

private:

    unsigned char* m_buf;
    std::vector<KeyPart> m_parts;
    xd::IIterator* m_iter;
    xd::IIteratorPtr m_sp_iter;
    xd::IStructurePtr m_iter_structure;
    int m_key_length;
    bool m_hold_ref;
    bool m_trunc;
};


#endif

