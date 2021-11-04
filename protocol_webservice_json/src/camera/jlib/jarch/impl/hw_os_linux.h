/*
 *  hw_os_linux.h
 *
 *  Created on: Nov 22, 2018
 *      Author: Felipe Gabriel G. Camargo - Pumatronix
 *
 *  OS Adaptation Layer for Linux and Android
 *
 */
#ifndef __HW_OS_LINUX_H__
#define __HW_OS_LINUX_H__

//==============================================================================
// Linux
//==============================================================================
// INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <poll.h>
#include <fcntl.h>
#ifdef ANDROID
#   include <android/log.h>
#endif

// LINKAGE
#define EXPORT_TYPE
#define TEMPLATE_INLINE                     inline

// Java
#ifdef ANDROID
#define ATTACHENV
#else
#define ATTACHENV                           (void **)
#endif

// STDIO
#ifdef ANDROID
#   define PRINTF_HW(fmt,...)   __android_log_print(ANDROID_LOG_VERBOSE, "jlib-jni", fmt, ##__VA_ARGS__);
#else
#   define PRINTF_HW(fmt,...)   printf(fmt,##__VA_ARGS__); fflush(stdout)
#endif

#define FFLUSH_HW                           fflush
#define GETCHAR_HW                          getchar
#define STDOUT_HW                           stdout
#define STDIN_HW                            stdin

// STDLIB
#define FREAD_HW(dst,size,count,file)       ::fread(dst,size,count,file)
#define MALLOC_HW(a)                        ::malloc(a)
#define CALLOC_HW(a,b)                      ::calloc(a,b)
#define FREE_HW(a)                          ::free(a)
#define EXIT_HW(a)                          ::exit(a)

// SOCKETS
#define SOCKET                              int
#define INVALID_SOCKET                     -1
typedef struct pollfd                       os_pollfd_t;
#define SELECT_HW(a,b,c,d,e)                ::select(a,b,c,d,e)
#define POLL_HW                             ::poll

// THREAD LIBRARY
typedef pthread_mutex_t                     os_mutex_t;
typedef pthread_t                           os_thread_t;
typedef sem_t                               os_sem_t;
#define YIELD()                             void(0)

// FILES
#define ERRNO_HW                            errno
#define CLOSE_HW(fd)                        ::close(fd)
typedef FILE*                               os_file_t;
typedef int                                 os_fd_t;
#define SOCKET_WOULDBLOCK_HW()              (ERRNO_HW == EWOULDBLOCK)

// Used by jidosha_loop - does not support local storage with TI DSP, only Linux
//#if __cplusplus < 201103L
//#warning "C++11 not supported - disabling LTS_STATIC - single thread usage only"
//#define LTS_STATIC                          static
//#else
#ifdef ITSCAM500
#define LTS_STATIC                          static
#else
#define LTS_STATIC                          static __thread
#endif

// MEMORY
#define LOAD_4BYTES(src)                (*((uint32_t*)&src))
#define LOADHI_8BYTES(src)              ((uint32_t)((((uint64_t*)&src) >> 32) & 0xFFFFFFFF))
#define LOADLO_8BYTES(src)              ((uint32_t)((((uint64_t*)&src)      ) & 0xFFFFFFFF))
#define PACK2_8B(msb,lsb)               ((((uint16_t)msb) << 8) | ((uint16_t)lsb))
#define PACK2_16B(msb,lsb)              ((((uint16_t)msb) << 16) | ((uint16_t)lsb))
#define PACKL4(hi, lo)                  ((((uint16_t)hi) << 16) | ((uint16_t)lo))
#define STORE_2BYTES(dst,value)         \
{                                       \
    uint16_t* tmp = (uint16_t*)&dst;    \
    *tmp = value;                       \
}                                       \

#define STORE_4BYTES(dst,value)         \
{                                       \
    uint32_t* tmp = (uint32_t*)&dst;    \
    *tmp = value;                       \
}

#endif /* __HW_OS_LINUX_H__ */
