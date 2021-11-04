#ifndef __JDEBUG_LIB_H__
#define __JDEBUG_LIB_H__

//==============================================================================
// LESS THAN MINIMAL
// DEBUG LIBRARY
//
// Author: Felipe Gabriel G. Camargo
// Pumatronix
//
//==============================================================================
// INCLUDES
//==============================================================================
#include <cstdlib>
#include <new>
#include <limits>
#include <map>
#include <string.h>
#include "camera/jlib/jarch/hw_os.h"

//==============================================================================
// DEFINES
//==============================================================================
#define DEBUG_LV_NONE                   0
#define DEBUG_LV_CRITICAL               1
#define DEBUG_LV_LOG                    2
#define DEBUG_LV_INFO                   3
#define DEBUG_LV_STACK                  4
#define DEBUG_LV_VERBOSE                5
#define DEBUG_LV_ALL                    255

#ifndef DEBUG_LEVEL
// #warning "DEBUG_LEVEL was not defined, using default DEBUG_LV_LOG"
#define DEBUG_LEVEL                     DEBUG_LV_LOG
#endif

//==============================================================================
// DEBUG BLOCKS
//==============================================================================
#define DEBUG_CODE(level) if(DEBUG_LEVEL >= level)

//==============================================================================
// LOG MACROS
//==============================================================================
#ifdef ANDROID
#   define DEBUG_CRLF "\n"
#else
#   define DEBUG_CRLF "\r\n"
#endif

#define DEBUG_PRINT_N(fmt,...) PRINTF_HW(fmt,##__VA_ARGS__)

#define DEBUG_PRINT(fmt,...)\
do{\
    PRINTF_HW("[LOG]\t" fmt "" DEBUG_CRLF , ##__VA_ARGS__);\
}while(0)

#define DEBUG_PRINT_TAG(tag, lvl, fmt , ...)\
do{\
    if(DEBUG_LEVEL >= lvl) {\
        const char* __file__ = const_cast<char*>(strrchr(__FILE__,'/'))+1; \
        PRINTF_HW("[" tag "]\t%s:%s():%d - " fmt "" DEBUG_CRLF , \
            __file__, __func__, __LINE__, ##__VA_ARGS__);\
    }\
}while(0)

#define DEBUG_PRINT_VECTOR(name, data) \
do{\
    PRINTF_HW("[LOG]\t" name ":"); \
    for(size_t i = 0; i < data.size(); i++) { \
        char t[8];\
        snprintf(t,8,"%x, ",data[i]);\
        PRINTF_HW(t); \
    }\
    PRINTF_HW("\n"); \
}while(0)

#define DEBUG_PRINT_FMT_VECTOR(name, data, size, fmt) \
do{\
    PRINTF_HW("[LOG]\t" name ":"); \
    for(size_t i = 0; i < size; i++) { \
        char t[8];\
        snprintf(t,8, fmt ",", data[i]);\
        PRINTF_HW(t); \
    }\
    PRINTF_HW("\n"); \
}while(0)

//==============================================================================
// 'DESIGN BY CONTRACT' MACROS
//==============================================================================
#define DEBUG_ASSERT(cond,...)\
do{\
    if(DEBUG_LEVEL >= DEBUG_LV_LOG){\
        if(!(cond)) {\
            const char* __file__ = const_cast<char*>(strrchr(__FILE__,'/'))+1; \
            PRINTF_HW("[ASSERT]\t%s:%s():%d - " #__VA_ARGS__ " (" #cond ")" DEBUG_CRLF,\
            __file__, __func__, __LINE__); \
            EXIT_HW(-1);\
        }\
    }\
}while(0)

#define DEBUG_ERROR(...)\
do{\
    if(DEBUG_LEVEL >= DEBUG_LV_LOG){\
        const char* __file__ = const_cast<char*>(strrchr(__FILE__,'/'))+1; \
        PRINTF_HW("[ERROR]\t%s:%s():%d - " #__VA_ARGS__ " " DEBUG_CRLF,\
        __file__, __func__, __LINE__); \
        EXIT_HW(-1);\
    }\
}while(0)

#define DEBUG_REQUIRE               DEBUG_ASSERT
#define DEBUG_ENSURE                DEBUG_ASSERT
#define DEBUG_INVARIANT             DEBUG_ASSERT

#define DEBUG_UNIMPLEMENTED_ERROR()   DEBUG_ERROR("Function not implemented");

//==============================================================================
// LOG ALIAS
//==============================================================================
#define DEBUG_PLOG(fmt , ...)       DEBUG_PRINT_TAG("LOG", DEBUG_LV_LOG, fmt, ##__VA_ARGS__)
#define DEBUG_PERROR(fmt , ...)     DEBUG_PRINT_TAG("ERR", DEBUG_LV_CRITICAL, fmt, ##__VA_ARGS__)
#define DEBUG_PWARNING(fmt , ...)   DEBUG_PRINT_TAG("WARN", DEBUG_LV_CRITICAL, fmt, ##__VA_ARGS__)
#define DEBUG_PINFO(fmt , ...)      DEBUG_PRINT_TAG("INFO", DEBUG_LV_INFO, fmt, ##__VA_ARGS__)
#define DEBUG_PVERBOSE(fmt , ...)   DEBUG_PRINT_TAG("VERBOSE", DEBUG_LV_VERBOSE, fmt, ##__VA_ARGS__)
#define DEBUG_ENTRY()               DEBUG_PRINT_TAG("STACK", DEBUG_LV_STACK, "entry")
#define DEBUG_EXIT()                DEBUG_PRINT_TAG("EXIT", DEBUG_LV_STACK, "exit")

//==============================================================================
// MEMORY MANAGEMENT MACROS
//==============================================================================
// Bookkeping
// [NOTE] Only DSP64x supports JDebug allocation bookkeping - (Not thread safe)
//==============================================================================
namespace global {
extern uint32_t g_TOTAL_ALLOCATED_BYTES;
extern std::map<void*,uint32_t> g_ALLOCATION_KEEPER;
typedef std::map<void*,uint32_t>::iterator alloc_it_t;
} /* end namespace global */

#if defined(DSP64x) && defined(JL_WITH_SANITIZE_ADDRESS)
#define DEBUG_DUMP_ALLOCATION_MAP() \
do{\
    if(DEBUG_LEVEL >= DEBUG_LV_LOG){\
        PRINTF_HW("Dumping allocation map\r\n");\
        PRINTF_HW("Total allocated memory: %u bytes", global::g_TOTAL_ALLOCATED_BYTES);\
        PRINTF_HW("Address\tSize");\
        PRINTF_HW("-------------------");\
        for(global::alloc_it_t it = global::g_ALLOCATION_KEEPER.begin(); it != global::g_ALLOCATION_KEEPER.end(); ++it) {\
            PRINTF_HW("0x%p\t%u",it->first,it->second);\
        }\
    }\
}while(0)
#define DEBUG_ADD_ALLOCATION(ptr,size) \
do{\
    if(DEBUG_LEVEL >= DEBUG_LV_LOG){\
        global::g_ALLOCATION_KEEPER[ptr] = static_cast<uint32_t>(size);\
        global::g_TOTAL_ALLOCATED_BYTES += static_cast<uint32_t>(size);\
    }\
}while(0)
#define DEBUG_REMOVE_ALLOCATION(ptr) \
do{\
    if(DEBUG_LEVEL >= DEBUG_LV_LOG){\
        global::g_TOTAL_ALLOCATED_BYTES -= global::g_ALLOCATION_KEEPER[ptr];\
        global::g_ALLOCATION_KEEPER.erase(ptr);\
    }\
}while(0)
#else
#define DEBUG_DUMP_ALLOCATION_MAP()     (void)0
#define DEBUG_ADD_ALLOCATION(ptr,size)  (void)0
#define DEBUG_REMOVE_ALLOCATION(ptr)    (void)0
#endif

//==============================================================================
// Debug-enabled allocation
//==============================================================================
#define MALLOC(_ptr_,_typecast_,_size_)\
do{\
    _ptr_ = (_typecast_)MALLOC_HW(_size_);\
    if(_ptr_ == NULL){\
        DEBUG_PERROR("malloc failed to allocate %u bytes - exiting", (uint32_t)_size_);\
        DEBUG_DUMP_ALLOCATION_MAP();\
        EXIT_HW(0);\
    } else {\
        DEBUG_PINFO("MALLOC: 0x%08llX-0x%08llX (sz=%u)", \
        (ull_t)_ptr_, (ull_t)_ptr_+_size_, (uint32_t)_size_); \
        DEBUG_ADD_ALLOCATION(_ptr_,_size_);\
    }\
}while(0)

#define CALLOC(_ptr_,_typecast_,_nr_elem_,_elem_size_)\
do{\
    _ptr_ = (_typecast_)CALLOC_HW(_nr_elem_, _elem_size_);\
    if(_ptr_ == NULL){\
        DEBUG_PERROR("calloc failed to allocate %u bytes - exiting", (uint32_t)((_nr_elem_)*(_elem_size_)));\
        DEBUG_DUMP_ALLOCATION_MAP();\
        EXIT_HW(0);\
    } else {\
        DEBUG_PINFO("CALLOC: 0x%08llX-0x%08llX (sz=%u)", \
        (ull_t)_ptr_, (ull_t)(_ptr_)+(_nr_elem_)*(_elem_size_), (uint32_t)((_nr_elem_)*(_elem_size_))); \
        DEBUG_ADD_ALLOCATION(_ptr_,((_nr_elem_)*(_elem_size_)));\
    }\
}while(0)

#define NEW_OPERATOR(ptr,typecast) \
do {\
    ptr = new (std::nothrow) typecast;\
    if(ptr == NULL){\
        DEBUG_PERROR("new failed to allocate %u bytes - exiting", (uint32_t)sizeof(typecast));\
        DEBUG_DUMP_ALLOCATION_MAP();\
        EXIT_HW(0);\
    } else {\
        DEBUG_ADD_ALLOCATION(ptr,sizeof(typecast));\
    }\
}while(0)

#define NEW_OPERATOR_ARRAY(ptr,typecast,size) \
do {\
    ptr = new (std::nothrow) typecast[size];\
    if(ptr == NULL){\
        DEBUG_PERROR("new failed to allocate %u bytes - exiting", (uint32_t)(sizeof(typecast)*size));\
        DEBUG_DUMP_ALLOCATION_MAP();\
        EXIT_HW(0);\
    } else {\
        DEBUG_ADD_ALLOCATION(ptr,(sizeof(typecast)*size));\
    }\
}while(0)

#define NEW_OPERATOR_ARRAY_ZEROED(ptr,typecast,size) \
do {\
    ptr = new (std::nothrow) typecast[size];\
    if(ptr == NULL){\
        DEBUG_PERROR("new failed to allocate %u bytes - exiting", (uint32_t)(sizeof(typecast)*size));\
        DEBUG_DUMP_ALLOCATION_MAP();\
        EXIT_HW(0);\
    } else {\
        memset(ptr, 0, size_t(size));\
        DEBUG_ADD_ALLOCATION(ptr,(sizeof(typecast)*size));\
    }\
}while(0)

#define FREE(ptr)\
do{\
    if(ptr == NULL){\
        DEBUG_PERROR("failed to free NULL pointer");\
    } else {\
        DEBUG_PINFO("FREE: 0x%08llX", (ull_t)ptr); \
        DEBUG_REMOVE_ALLOCATION(ptr);\
        FREE_HW(ptr);\
        ptr = NULL; \
    }\
}while(0)

#define DELETE_OPERATOR(ptr)\
do{\
    if(ptr == NULL){\
        DEBUG_PERROR("failed to delete NULL pointer");\
    } else {\
        DEBUG_REMOVE_ALLOCATION(ptr);\
        delete ptr;\
        ptr = NULL; \
    }\
}while(0)

#define DELETE_OPERATOR_ARRAY(ptr)\
do{\
    if(ptr == NULL){\
        DEBUG_PERROR("failed to delete NULL pointer");\
    } else {\
        DEBUG_REMOVE_ALLOCATION(ptr);\
        delete[] ptr;\
        ptr = NULL; \
    }\
}while(0)

//==============================================================================
// UTILS MACROS
//==============================================================================
#define FREAD(dst, size, count, file)\
do{\
    if(!FREAD_HW(dst,size,count,file)){\
        DEBUG_PWARNING("fread returned 0");\
    }\
}while(0)

#define EXIT(value)\
do{\
    DEBUG_PWARNING("Exiting");\
    EXIT_HW(value);\
}while(0)

#define RETURN(ret)\
    DEBUG_EXIT();\
    return ret;

//==================================================
// STRING UTILS
//==================================================
EXPORT_TYPE char* DEBUG_STR_PADD_INT(const char* str, int i);
EXPORT_TYPE char* DEBUG_STR_PADD_UINT64(const char* str, uint64_t i);

//==================================================
// TIMER FUNCTIONS
//==================================================
EXPORT_TYPE double   DEBUG_TIMER_GET_TIME_MS();
EXPORT_TYPE uint64_t DEBUG_TIMER_GET_TIME_US();

#define DEBUG_TIMER_LOG_ELAPSED(name, enable, code)\
do{\
    double dt=0, t0=0;\
    if (enable){ \
        t0 = DEBUG_TIMER_GET_TIME_MS();\
    }\
      code\
    if (enable){ \
        dt = (DEBUG_TIMER_GET_TIME_MS() - t0);\
        DEBUG_PRINT("[TMR-%s] ElapsedTime: %3.2f ms", name, dt);\
    }\
}while(0)

#define DEBUG_TIMER_LOG_ACCUMULATED(name, enable, code)\
do{\
    static double dt=0, t0=0;\
    if (enable){ \
        t0 = DEBUG_TIMER_GET_TIME_MS();\
    } \
    code\
    if (enable){ \
        dt += (DEBUG_TIMER_GET_TIME_MS() - t0);\
        DEBUG_PRINT("[TMR-%s] accumulated time: %3.2f ms", name, dt);\
    } \
}while(0)

//==================================================
// JMAT FUNCTIONS
//==================================================
#if DEBUG_ENABLE_SAVE_IMG
#ifdef JMAT_ENABLE_OPENCV
#define DEBUG_SAVE_IMG(img,name)\
do{\
    if(DEBUG_ENABLE_SAVE_IMG){\
        uint64_t uid = DEBUG_TIMER_GET_TIME_US();\
        jmat::saveToFile(img,DEBUG_STR_PADD_UINT64(name".bmp", uid));\
        DEBUG_PRINT_N("[IMG]\t%s:%s():%d - %s" "" DEBUG_CRLF , __FILE__, __func__, __LINE__, DEBUG_STR_PADD_UINT64(name".bmp", uid)); \
    }\
}while(0)
#else
#define DEBUG_SAVE_IMG(img,name)    (void)0
#endif
#else
#define DEBUG_SAVE_IMG(img,name)    (void)0
#endif

#ifdef JMAT_ENABLE_OPENCV
#define DEBUG_SAVE_IMG_N(img,name)\
do{\
    uint64_t uid = DEBUG_TIMER_GET_TIME_US();\
    jmat::saveToFile(img,DEBUG_STR_PADD_UINT64(name".bmp", uid));\
    DEBUG_PRINT_N("[IMG]\t%s:%s():%d - %s" "" DEBUG_CRLF , __FILE__, __func__, __LINE__, DEBUG_STR_PADD_UINT64(name".bmp", uid)); \
}while(0)
#else
#define DEBUG_SAVE_IMG_N(img,name)    (void)0
#endif

//==================================================
// MEMORY DEBUG UTILS
//==================================================
uint32_t DEBUG_GET_MEMFREE_KB();
uint32_t DEBUG_GET_STACK_MEMFREE_B();

#endif /* __JDEBUG_LIB_H__ */
