/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-24
 *
 */


#include <xcm/xcm.h>


#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif


#include <string>


namespace xcm
{


typedef xcm::IObject* (*xcm_ci_func_type)(const char* class_name);
typedef xcm::class_info* (*xcm_gci_func_type)(const char* class_name);
typedef void* XCM_MODULE;

std::wstring get_program_path();


struct libhandle
{
    std::string lib_name;
    XCM_MODULE mod_handle;

    libhandle()
    {
        mod_handle = 0;
    }

    ~libhandle()
    {
        if (mod_handle)
        {
            #ifdef WIN32
            FreeLibrary((HMODULE)mod_handle);
            #else
            dlclose((void*)mod_handle);
            #endif
        }
    }
};



static std::vector<libhandle*> g_libs;
static bool g_deallocating = false;
static char g_xcm_errorstr[255];


class LibDeallocator
{
public:
    LibDeallocator() { g_xcm_errorstr[0] = 0; }
    ~LibDeallocator()
    {
        if (g_deallocating)
            return;
        g_deallocating = true;

        std::vector<libhandle*>::iterator it;
        for (it = g_libs.begin(); it != g_libs.end(); ++it)
        {
            delete *it;
        }
        g_libs.clear();
    }
};
static LibDeallocator d;


std::vector<std::wstring>& path_list::get()
{
    static std::vector<std::wstring> path_list;
    if (path_list.size() == 0)
    {
        // add default paths
        #ifdef WIN32
            std::wstring path = get_program_path();
            if (path.empty() || path[path.length()-1] != L'\\')
                path += L"\\";

            path_list.push_back(path);
            //path_list.push_back(L"C:\\WINDOWS\\");
            //path_list.push_back(L"C:\\WINDOWS\\SYSTEM32\\");
            //path_list.push_back(L"C:\\WINDOWS\\SYSTEM\\");
        #else
            std::wstring path = get_program_path();
            if (path.empty() || path[path.length()-1] != L'/')
                path += L"/";
                
            path_list.push_back(path);
            //path_list.push_back(L"/usr/local/lib/");
            //path_list.push_back(L"/usr/lib/");
        #endif
    }
    return path_list;
}

void path_list::add(const std::wstring& path)
{
    if (path.empty())
        return;
    std::wstring new_path = path;
    #ifdef WIN32
    if (new_path[new_path.length()-1] != L'\\')
    {
        new_path += L"\\";
    }
    #else
    if (new_path[new_path.length()-1] != L'/')
    {
        new_path += L"/";
    }
    #endif
    path_list::get().push_back(new_path);
}









#ifdef _MSC_VER
#if defined(__LP64__) || defined(_M_X64)
long long XCM_STDCALL interlocked_increment(long long* p) { return InterlockedIncrement64(p); }
long long XCM_STDCALL interlocked_decrement(long long* p) { return InterlockedDecrement64(p); }
#else
long XCM_STDCALL interlocked_increment(long* p) { return InterlockedIncrement(p); }
long XCM_STDCALL interlocked_decrement(long* p) { return InterlockedDecrement(p); }
#endif
#endif




inline std::string tostring(const std::wstring& w)
{
    std::string result;
    size_t i, len = w.length();
    result.resize(len);
    for (i = 0; i < len; ++i)
        result[i] = (char)(unsigned char)w[i];
    return result;
}

std::wstring get_program_path()
{
#if defined(WIN32)

    wchar_t buf[MAX_PATH];
    wchar_t* slash;
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    slash = wcsrchr(buf, '\\');
    if (slash)
        *slash = 0;
    return buf;

#elif defined(__linux__)

    char buf[512];
    char* slash;

    int res;
    res = readlink("/proc/self/exe", buf, 512);
    if (res == -1)
        return L"";
    buf[res] = 0;
    slash = strrchr(buf, '/');
    if (slash)
        *slash = 0;

    std::wstring result;
    size_t i, len = strlen(buf);
    result.resize(len);
    for (i = 0; i < len; ++i)
        result[i] = buf[i];
    return result;

#elif defined(__APPLE__)

    char buf[512];
    char* slash;

    unsigned int len = 512;
    if (_NSGetExecutablePath(buf, &len) == -1)
        return wxT("");
    buf[len] = 0;
    slash = strrchr(buf, '/');
    if (slash)
        *slash = 0;

    std::wstring result;
    size_t i, len = strlen(buf);
    result.resize(len);
    for (i = 0; i < len; ++i)
        result[i] = buf[i];
    return result;

#else
    return L"";
#endif
}


XCM_MODULE get_module_handle(const std::string& lib_name)
{
    std::vector<libhandle*>::iterator it;


    for (it = g_libs.begin(); it != g_libs.end(); it++)
    {
        #ifdef WIN32
        if (!stricmp((*it)->lib_name.c_str(), lib_name.c_str()))
        #else
        if ((*it)->lib_name == lib_name)
        #endif
        {
            return (*it)->mod_handle;
        }
    }

    std::wstring wlib_name;
    wlib_name.resize(lib_name.length(), L' ');
    for (unsigned int i = 0; i < lib_name.length(); ++i)
    {
        wlib_name[i] = lib_name[i];
    }

    // save current directory and change to the path where the program
    // executable is stored.  This allows relative paths in the linux
    // dynamic loader to work
    
    #ifndef WIN32
    std::wstring wprog_path = get_program_path();
    std::string prog_path = tostring(wprog_path);

    char curdir[512];
    getcwd(curdir, 512);
    
    chdir(prog_path.c_str());
    #endif


    // was not found in the map, attempt to open
    std::vector<std::wstring>& path_vec = path_list::get();
    std::vector<std::wstring>::iterator path_it;
    std::wstring full_path;
    XCM_MODULE modh = NULL;

    for (path_it = path_vec.begin(); path_it != path_vec.end(); path_it++)
    {
        full_path = *path_it;
        full_path += wlib_name;
        
        #ifdef WIN32
        full_path += L".dll";
        #else
        full_path += L".so";
        #endif
        
        #ifdef WIN32
        #ifdef UNICODE
        modh = LoadLibraryW(full_path.c_str());
        #else
        {
            std::string asc_path = tostring(full_path);
            modh = LoadLibraryA(asc_path.c_str());
        }
        #endif
        #else
        {
            std::string asc_path = tostring(full_path);
            modh = dlopen(asc_path.c_str(), RTLD_LAZY);
        }
        #endif
        
        #ifdef WIN32
        if (!modh)
        {
            DWORD errcode = GetLastError();
            
            LPVOID lpMsgBuf;
    
            FormatMessageA( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPSTR)&lpMsgBuf,
                0,
                NULL 
            );
            LPSTR str = (LPSTR)lpMsgBuf;
            strncpy(g_xcm_errorstr, str, 254);
            g_xcm_errorstr[254] = 0;
            LocalFree(lpMsgBuf); 
        }
        #endif
        
        if (modh)
        {
            break;
        }
    }

    #ifndef WIN32
    // change the directory back to what it was (linux only)
    chdir(curdir);
    #endif


    if (modh)
    {
        libhandle* l = new libhandle;
        l->lib_name = lib_name;
        l->mod_handle = modh;
        
        g_libs.push_back(l);
    }

    return modh;
}

const char* get_last_error()
{
    return g_xcm_errorstr;
}


void* lookup_func(const std::string& class_name,
                  const std::string& func_name)
{
    // extract the library from the class_name
    std::string lib_name;

    int period = class_name.find('.');
    if (period <= 0)
        return NULL;

    lib_name = class_name.substr(0, period);

    // now attach the lib name to each path and attempt to load the library

    XCM_MODULE modh = get_module_handle(lib_name);
    if (!modh)
    {
        return NULL;
    }

    #ifdef WIN32
        return (void*)GetProcAddress((HMODULE)modh, func_name.c_str());
    #else
        return (void*)dlsym(modh, func_name.c_str());
    #endif
}


xcm::IObject* create_instance(const std::string& class_name)
{
    // lookup the appropriate dll
    xcm_ci_func_type create_instance_ptr;

    create_instance_ptr = (xcm_ci_func_type)xcm::lookup_func(class_name, "xcm_create_instance");
    if (!create_instance_ptr)
        return NULL;

    return create_instance_ptr(class_name.c_str());
}


xcm::class_info* get_class_info(const std::string& class_name)
{
    // lookup the appropriate dll
    xcm_gci_func_type get_class_info_ptr;

    get_class_info_ptr = (xcm_gci_func_type)xcm::lookup_func(class_name, "xcm_get_class_info");
    if (!get_class_info_ptr)
        return NULL;

    return get_class_info_ptr(class_name.c_str());
}


xcm::class_info* get_class_info(IObject* instance)
{
    // calling query_interface with NULL returns
    // class info
    if (!instance)
        return NULL;
   
    return (xcm::class_info*)instance->query_interface(NULL);
}





// xcm_thread implemetation


#ifdef WIN32


mutex::mutex()
{
    ::InitializeCriticalSection((PCRITICAL_SECTION)m_data);
}

mutex::~mutex()
{
    ::DeleteCriticalSection((PCRITICAL_SECTION)m_data);
}

void mutex::lock()
{
    ::EnterCriticalSection((PCRITICAL_SECTION)m_data);
}

void mutex::unlock()
{
    ::LeaveCriticalSection((PCRITICAL_SECTION)m_data);
}


threadid_t get_current_thread_id()
{
    return (threadid_t)::GetCurrentThreadId();
}


#else



mutex::mutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init((pthread_mutex_t*)m_data, &attr);
}

mutex::~mutex()
{
    pthread_mutex_destroy((pthread_mutex_t*)m_data);
}

void mutex::lock()
{
    pthread_mutex_lock((pthread_mutex_t*)m_data);
}

void mutex::unlock()
{
    pthread_mutex_unlock((pthread_mutex_t*)m_data);
}


threadid_t get_current_thread_id()
{
    return (threadid_t)pthread_self();
}


#endif




};

