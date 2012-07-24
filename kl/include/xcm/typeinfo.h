/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-09-04
 *
 */


#ifndef __XCM_TYPEINFO_H
#define __XCM_TYPEINFO_H


#include <vector>
#include <string>
#include <cctype>


namespace xcm
{


enum valueTypes
{
    typeException = -1,
    typeVoid = 0,
    typeInt32 = 1,
    typeInt64 = 2,
    typeUint32 = 3,
    typeUint64 = 4,
    typeBoolean = 5,
    typeString = 6,
    typeDouble = 7,
    typeBase64 = 8,
    typeStruct = 20,
    typeArray = 21,
    typeStructure = 22,
    typeObjectRef = 23,
    typePtr = 24
};


enum valueException
{
    exMethodNotFound = -1,
    exMethodBadParams = -2,
    exNoException = 0
};

class method_info;


class parameter
{
friend class xcm::method_info;

public:
    
    const char* get_name()
    {
        return m_name.c_str();
    }

    int get_type()
    {
        return m_type;
    }

private:

    std::string m_name;
    int m_type;
};


class method_info
{

public:
    
    const char* get_name()
    {
        return m_name.c_str();
    }

    parameter& get_param(int idx)
    {
        return m_params[idx];
    }

    size_t get_param_count()
    {
        return m_params.size();
    }

    int get_retval_type()
    {
        return m_retval_type;
    }

    bool parse_decl(const char* decl)
    {
        const char* p;
        const char* name;

        p = decl;

        // -- go to the first letter of the type token --
        while (isspace(*p))
            p++;
        
        m_retval_type = get_type_from_str(p);
            
        // -- go to the end of the type token --
        while (!isspace(*p))
            p++;
        while (isspace(*p))
            p++;

        // -- get method name --
        name = p;
        if (!isalpha(*p) && *p != '_')
        {
            // -- method name must start with alpha char --
            return false;
        }

        while (isalnum(*p) || *p == '_')
            p++;

        m_name.assign(name, p-name);

        while (isspace(*p))
            p++;

        if (*p != '(')
        {
            // -- open paren is mandatory after name;
            return false;
        }

        p++;

        // -- parse parameters --
        parameter param;
        while (1)
        {
            while (isspace(*p))
                p++;

            if (*p == ')' || !*p)
            {
                // (we're done)
                break;
            }

            param.m_type = get_type_from_str(p);

            // -- go to the end of the type token --
            while (!isspace(*p))
            {
                if (*p == ',' || *p == ')')
                {
                    // (there was no param name)
                    m_params.push_back(param);
                    return true;
                }
                p++;
            }
            while (isspace(*p))
                p++;

            // -- get parameter name (if any)
            name = p;
            while (1)
            {
                if (*p == ',' || *p == ')' || !*p)
                    break;
                p++;
            }

            param.m_name.assign(name, p-name);
            p++;
        }

        return true;
    }

private:

    static int get_type_from_str(const char* s)
    {
        if (!strncmp(s, "void", 4))
            return typeVoid;
        if (!strncmp(s, "bool", 4))
            return typeBoolean;
        if (!strncmp(s, "i4", 2) || !strncmp(s, "i32", 3) || !strncmp(s, "int", 3))
            return typeInt32;
        if (!strncmp(s, "i64", 3))
            return typeInt64;
        if (!strncmp(s, "ui4", 3) || !strncmp(s, "ui32", 4) || !strncmp(s, "uint", 4))
            return typeUint32;
        if (!strncmp(s, "ui64", 3))
            return typeUint64;
        if (!strncmp(s, "string", 6))
            return typeString;
        if (!strncmp(s, "double", 6))
            return typeDouble;
        return -1;
    }

    std::string m_name;
    std::vector<parameter> m_params;
    int m_retval_type;
};


class interface_info
{

public:

    const char* get_name()
    {
        return m_name.c_str();
    }

    void set_name(const char* name)
    {
        m_name = name;
    }

    size_t get_method_count()
    {
        return m_methods.size();
    }

    method_info& get_method_info(int idx)
    {
        return m_methods[idx];
    }

    method_info& add_method_info()
    {
        m_methods.push_back(method_info());
        return *(m_methods.rbegin());
    }

    void clear()
    {
        m_methods.clear();
    }

private:

    std::vector<method_info> m_methods;
    std::string m_name;
};



class class_info
{

public:

    const char* get_name()
    {
        return m_name.c_str();
    }

    void set_name(const char* name)
    {
        m_name = name;
    }

    size_t get_interface_count()
    {
        return m_ifaces.size();
    }

    interface_info& get_interface_info(int idx)
    {
        return *(m_ifaces[idx]);
    }

    void add_interface_info(interface_info* info)
    {
        if (info)
        {
            m_ifaces.push_back(info);
        }
    }

    void add_chain_info(class_info* chain_class_info)
    {
        m_ifaces.insert(m_ifaces.end(),
                        chain_class_info->m_ifaces.begin(),
                        chain_class_info->m_ifaces.end());
    }
    
private:

    std::vector<interface_info*> m_ifaces;
    std::string m_name;
};


};

#endif

