/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-08-10
 *
 */


#ifndef __XCM_SMARTPTR_H
#define __XCM_SMARTPTR_H


namespace xcm
{


typedef unsigned int null_type;
const null_type null = 0;


class ptr_base
{
public:
    virtual ~ptr_base() { }
    virtual IObject* get_iobject() const = 0;
};

template <class T>
class ptr : public ptr_base
{
public:

    IObject* get_iobject() const
    {
        return static_cast<IObject*>(p);
    }

    T* p;

public:

    ptr()
    {
        p = 0;
    }

    ptr(const ptr_base& c)
    {
        IObject* iobj = c.get_iobject();
        if (iobj)
        {
            p = (T*)iobj->query_interface(T::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }
    }

    ptr(const ptr<T>& c)
    {
        p = c.p;
        if (p)
        {
            p->ref();
        }
    }

    ptr(T* const& _p)
    {
        p = _p;
        if (p)
        {
            p->ref();
        }
    }

    ptr(T* const& _p, bool addref)
    {
        p = _p;
        if (p && addref)
        {
            p->ref();
        }
    }

    ptr(IObject* _p)
    {
        if (_p)
        {
            p = (T*)_p->query_interface(T::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }
    }

    ptr(const null_type& null_value)
    {
        p = 0;
    }

    ~ptr()
    {
        if (p)
        {
            p->unref();
        }
    }

    operator IObject*() const
    {
        return static_cast<IObject*>(p);
    }

    operator T*() const
    {
        return p;
    }

    T* operator->() const
    {
        return p;
    }

    bool operator!() const
    {
        return (p == 0);
    }

    operator bool() const
    {
        return (p != 0);
    }

    bool operator()() const
    {
        return (p != 0);
    }

    ptr<T>& operator=(const ptr_base& c)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        IObject* iobj = c.get_iobject();
        if (iobj)
        {
            p = (T*)iobj->query_interface(T::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }

        return *this;
    }

    ptr<T>& operator=(const ptr<T>& c)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        p = c.p;
        if (p)
        {
            p->ref();
        }
        return *this;
    }

    ptr<T>& operator=(T* _p)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        p = _p;

        if (p)
        {
            p->ref();
        }

        return *this;
    }

    ptr<T>& operator=(IObject* _p)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        if (_p)
        {
            p = (T*)_p->query_interface(T::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }

        return *this;
    }

    ptr<T>& operator=(const null_type& null_value)
    {
        if (p)
        {
            p->unref();
        }
        p = 0;
        return *this;
    }

    bool operator==(const ptr<T>& rhs) const
    {
        return (p == rhs.p);
    }

    bool operator==(T* rhs) const
    {
        return (p == rhs);
    }

    bool operator==(IObject* rhs) const
    {
        return (static_cast<IObject*>(p) == rhs);
    }

    bool operator==(const null_type& null_value) const
    {
        return (p == 0);
    }

    bool operator!=(const ptr<T>& rhs) const
    {
        return (p != rhs.p);
    }

    bool operator!=(T* rhs) const
    {
        return (p != rhs);
    }

    bool operator!=(IObject* rhs) const
    {
        return (static_cast<IObject*>(p) != rhs);
    }

    bool operator!=(const null_type& null_value) const
    {
        return (p != 0);
    }

    bool isNull() const
    {
        return (p == 0);
    }

    bool isOk() const
    {
        return (p != 0);
    }

    void clear()
    {
        if (p)
        {
            p->unref();
        }
        p = 0;
    }

    bool create_instance(const char* class_name)
    {
        if (p)
        {
            p->unref();
            p = 0;
        }
        xcm::IObject* pObject;
        pObject = xcm::create_instance(class_name);
        if (!pObject)
            return false;
        p = (T*)pObject->query_interface(T::xcm_get_interface_name());
        pObject->unref();
        if (!p)
            return false;
        return true;
    }
};





class IObjectPtr : public ptr_base
{
public:

    IObject* get_iobject() const
    {
        return p;
    }

    IObject* p;

public:

    IObjectPtr()
    {
        p = 0;
    }

    IObjectPtr(const ptr_base& c)
    {
        IObject* iobj = c.get_iobject();
        if (iobj)
        {
            p = (IObject*)iobj->query_interface(IObject::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }
    }

    IObjectPtr(IObjectPtr& c)
    {
        p = c.p;
        if (p)
        {
            p->ref();
        }
    }

    IObjectPtr(IObject* const& _p)
    {
        p = _p;
        if (p)
        {
            p->ref();
        }
    }

    IObjectPtr(IObject* const& _p, bool addref)
    {
        p = _p;
        if (p && addref)
        {
            p->ref();
        }
    }

    IObjectPtr(IObject* _p)
    {
        if (_p)
        {
            p = (IObject*)_p->query_interface(IObject::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }
    }

    IObjectPtr(const null_type& null_value)
    {
        p = 0;
    }

    ~IObjectPtr()
    {
        if (p)
        {
            p->unref();
        }
    }

    operator IObject*() const
    {
        return p;
    }

    IObject* operator->() const
    {
        return p;
    }

    bool operator!() const
    {
        return (p == 0);
    }

    operator bool() const
    {
        return (p != 0);
    }

    bool operator()() const
    {
        return (p != 0);
    }

    IObjectPtr& operator=(const ptr_base& c)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        IObject* iobj = c.get_iobject();
        if (iobj)
        {
            p = (IObject*)iobj->query_interface(IObject::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }

        return *this;
    }

    IObjectPtr& operator=(const IObjectPtr& c)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        p = c.p;
        if (p)
        {
            p->ref();
        }
        return *this;
    }

    IObjectPtr& operator=(IObject* _p)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        p = _p;

        if (p)
        {
            p->ref();
        }

        return *this;
    }

    IObjectPtr& operator=(const null_type& null_value)
    {
        if (p)
        {
            p->unref();
        }
        p = 0;
        return *this;
    }

    bool operator==(const IObjectPtr& rhs) const
    {
        return (p == rhs.p);
    }

    bool operator==(IObject* rhs) const
    {
        return (p == rhs);
    }

    bool operator==(const null_type& null_value) const
    {
        return (p == 0);
    }

    bool operator!=(const IObjectPtr& rhs) const
    {
        return (p != rhs.p);
    }

    bool operator!=(IObject* rhs) const
    {
        return (p != rhs);
    }

    bool operator!=(const null_type& null_value) const
    {
        return (p != 0);
    }

    bool isNull() const
    {
        return (p == 0);
    }

    bool isOk() const
    {
        return (p != 0);
    }

    void clear()
    {
        if (p)
        {
            p->unref();
        }
        p = 0;
    }

    bool create_instance(const char* class_name)
    {
        if (p)
        {
            p->unref();
            p = 0;
        }
        p = xcm::create_instance(class_name);
        if (!p)
            return false;
        return true;
    }
};



};


#endif

