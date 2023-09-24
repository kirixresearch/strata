/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-24
 *
 */


#ifndef H_SCRIPTHOST_SCRIPTMEMORY_H
#define H_SCRIPTHOST_SCRIPTMEMORY_H


namespace scripthost
{



class MemoryBuffer : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("MemoryBuffer", MemoryBuffer)
        KSCRIPT_METHOD("constructor", MemoryBuffer::constructor)
        KSCRIPT_METHOD("setSize", MemoryBuffer::setSize)
        KSCRIPT_METHOD("getSize", MemoryBuffer::getSize)
        KSCRIPT_METHOD("clear", MemoryBuffer::clear)
        KSCRIPT_METHOD("copy", MemoryBuffer::copy)
        KSCRIPT_METHOD("free", MemoryBuffer::free)
        KSCRIPT_METHOD("toAsciiString", MemoryBuffer::toAsciiString)
    END_KSCRIPT_CLASS()
    
public:

    MemoryBuffer();
    ~MemoryBuffer();
    
    void alloc(size_t size);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getSize(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);
    void copy(kscript::ExprEnv* env, kscript::Value* retval);
    void free(kscript::ExprEnv* env, kscript::Value* retval);
    void toAsciiString(kscript::ExprEnv* env, kscript::Value* retval);
    
    void setMember(const std::wstring& name, kscript::Value* value);
    kscript::Value* getMember(const std::wstring& name);

    unsigned char* getBuffer();
    size_t getBufferSize();
    
    bool setSizeInternal(int new_size);
    
private:

    unsigned char* m_buf;
    size_t m_size;
    
    kscript::Value m_val; // value for getMember()
};



} // namespace scripthost


#endif

